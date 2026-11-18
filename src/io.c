/* io.c - CSV persistence with defensive validation. */
#include "io.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

int ensure_directory(const char *path) {
#ifdef _WIN32
    int rc = _mkdir(path);
#else
    int rc = mkdir(path, 0755);
#endif
    return rc == 0 || errno == EEXIST;
}

/* Writes s with separators/newlines replaced by spaces. */
static void put_clean(FILE *f, const char *s) {
    for (; *s; ++s) fputc((*s == ',' || *s == ';' || *s == '\n' || *s == '\r') ? ' ' : *s, f);
}

static FILE *open_file(const char *prefix, const char *suffix, const char *mode) {
    char path[1024];
    snprintf(path, sizeof(path), "%s%s", prefix, suffix);
    return fopen(path, mode);
}

int save_library(const Library *lib, const char *prefix, char *error, size_t error_cap) {
    FILE *fb = open_file(prefix, "_books.csv", "w");
    FILE *fc = open_file(prefix, "_citations.csv", "w");
    FILE *fu = open_file(prefix, "_users.csv", "w");
    int i, k;
    if (!fb || !fc || !fu) {
        snprintf(error, error_cap, "cannot create files with prefix '%s' (does the folder exist?)", prefix);
        if (fb) fclose(fb);
        if (fc) fclose(fc);
        if (fu) fclose(fu);
        return 0;
    }
    fprintf(fb, "id,title,author,year,rating,readMinutes,keywords,abstract\n");
    for (i = 0; i < lib->num_books; ++i) {
        const Book *b = &lib->books[i];
        fprintf(fb, "%d,", b->id);
        put_clean(fb, b->title);
        fputc(',', fb);
        put_clean(fb, b->author);
        fprintf(fb, ",%d,%.1f,%d,", b->year, b->rating, b->read_minutes);
        for (k = 0; k < b->num_keywords; ++k) {
            if (k) fputc(';', fb);
            put_clean(fb, b->keywords[k]);
        }
        fputc(',', fb);
        put_clean(fb, b->abstract_text ? b->abstract_text : "");
        fputc('\n', fb);
    }
    fprintf(fc, "from,to,weight\n");
    for (i = 0; i < lib->num_books; ++i)
        for (k = 0; k < lib->citations[i].size; ++k)
            if (i < lib->citations[i].e[k].to)   /* each undirected edge once */
                fprintf(fc, "%d,%d,%d\n", i, lib->citations[i].e[k].to, lib->citations[i].e[k].weight);
    fprintf(fu, "id,name,interests,history\n");
    for (i = 0; i < lib->num_users; ++i) {
        const User *u = &lib->users[i];
        fprintf(fu, "%d,", u->id);
        put_clean(fu, u->name);
        fputc(',', fu);
        for (k = 0; k < u->num_interests; ++k) {
            if (k) fputc(';', fu);
            put_clean(fu, u->interests[k]);
        }
        fputc(',', fu);
        for (k = 0; k < u->history.size; ++k) fprintf(fu, "%s%d", k ? ";" : "", u->history.data[k]);
        fputc('\n', fu);
    }
    fclose(fb);
    fclose(fc);
    fclose(fu);
    return 1;
}

/* ---- Reading helpers ---- */

/* Reads one line of any length into *buf (grown as needed), without the newline.
 * Returns 1 on success, 0 at end of file. */
static int read_line(FILE *f, char **buf, size_t *cap) {
    size_t len = 0;
    int c;
    if (*cap == 0) {
        *cap = 512;
        *buf = (char *)xmalloc(*cap);
    }
    while ((c = fgetc(f)) != EOF && c != '\n') {
        if (len + 1 >= *cap) {
            *cap *= 2;
            *buf = (char *)xrealloc(*buf, *cap);
        }
        (*buf)[len++] = (char)c;
    }
    if (c == EOF && len == 0) return 0;
    if (len > 0 && (*buf)[len - 1] == '\r') --len;
    (*buf)[len] = '\0';
    return 1;
}

/* Splits s in place at sep. Returns number of fields (up to max). */
static int split(char *s, char sep, char **fields, int max) {
    int n = 0;
    fields[n++] = s;
    for (; *s; ++s) {
        if (*s == sep) {
            *s = '\0';
            if (n < max) fields[n++] = s + 1;
            else return max + 1;   /* too many fields */
        }
    }
    return n;
}

static int parse_long(const char *s, long long *out) {
    char *end;
    long long v;
    if (!*s) return 0;
    errno = 0;
    v = strtoll(s, &end, 10);
    if (errno || *end) return 0;
    *out = v;
    return 1;
}

static int parse_double(const char *s, double *out) {
    char *end;
    double v;
    if (!*s) return 0;
    v = strtod(s, &end);
    if (*end) return 0;
    *out = v;
    return 1;
}

/* ---- Small open-addressing hash map: id in file -> dense id ---- */
typedef struct {
    long long *keys;
    int *vals;
    char *used;
    size_t cap;   /* power of two */
} IdMap;

static void idmap_init(IdMap *m, size_t expected) {
    m->cap = 16;
    while (m->cap < expected * 2 + 2) m->cap *= 2;
    m->keys = (long long *)xmalloc(m->cap * sizeof(long long));
    m->vals = (int *)xmalloc(m->cap * sizeof(int));
    m->used = (char *)xcalloc(m->cap, 1);
}

static size_t idmap_slot(const IdMap *m, long long key) {
    size_t h = (size_t)((unsigned long long)key * 0x9E3779B97F4A7C15ULL >> 16) & (m->cap - 1);
    while (m->used[h] && m->keys[h] != key) h = (h + 1) & (m->cap - 1);   /* linear probing */
    return h;
}

static int idmap_get(const IdMap *m, long long key) {
    size_t h = idmap_slot(m, key);
    return m->used[h] ? m->vals[h] : -1;
}

static void idmap_put(IdMap *m, long long key, int val) {
    size_t h = idmap_slot(m, key);
    m->used[h] = 1;
    m->keys[h] = key;
    m->vals[h] = val;
}

static void idmap_free(IdMap *m) {
    free(m->keys);
    free(m->vals);
    free(m->used);
}

static size_t count_lines(FILE *f) {
    size_t n = 0;
    int c;
    while ((c = fgetc(f)) != EOF)
        if (c == '\n') ++n;
    rewind(f);
    return n + 1;
}

int load_library(Library *lib, const char *prefix, char *error, size_t error_cap, int *skipped_rows) {
    FILE *fb = open_file(prefix, "_books.csv", "r"), *fc, *fu;
    Library tmp;
    IdMap map;
    char *line = NULL, *f[16];
    size_t cap = 0, lines;
    int book_cap, user_cap = 16;

    *skipped_rows = 0;
    if (!fb) {
        snprintf(error, error_cap, "cannot open %s_books.csv", prefix);
        return 0;
    }
    library_init(&tmp);
    lines = count_lines(fb);
    idmap_init(&map, lines);
    book_cap = (int)lines;
    tmp.books = (Book *)xcalloc((size_t)book_cap, sizeof(Book));

    read_line(fb, &line, &cap);   /* header */
    while (read_line(fb, &line, &cap)) {
        long long id, year, minutes;
        double rating;
        int nf, k;
        Book *b;
        char *kw[MAX_KEYWORDS + 8];
        if (!line[0]) continue;
        nf = split(line, ',', f, 8);
        if (nf != 8 || !parse_long(f[0], &id) || !f[1][0] || !parse_long(f[3], &year) || year < 0 || year > 3000 ||
            !parse_double(f[4], &rating) || rating < 0 || rating > 5 || !parse_long(f[5], &minutes) ||
            minutes <= 0 || minutes > 100000 || idmap_get(&map, id) >= 0 || tmp.num_books >= book_cap) {
            (*skipped_rows)++;
            continue;
        }
        b = &tmp.books[tmp.num_books];
        b->id = tmp.num_books;
        idmap_put(&map, id, b->id);
        str_copy(b->title, MAX_TITLE, f[1]);
        str_copy(b->author, MAX_AUTHOR, f[2]);
        b->year = (int)year;
        b->rating = rating;
        b->read_minutes = (int)minutes;
        nf = split(f[6], ';', kw, MAX_KEYWORDS + 7);
        b->num_keywords = 0;
        for (k = 0; k < nf && k < MAX_KEYWORDS + 7; ++k)
            if (kw[k][0] && b->num_keywords < MAX_KEYWORDS) str_copy(b->keywords[b->num_keywords++], MAX_WORD, kw[k]);
        b->abstract_text = xstrdup(f[7]);
        tmp.num_books++;
    }
    fclose(fb);
    if (tmp.num_books == 0) {
        snprintf(error, error_cap, "no valid book rows found");
        library_free(&tmp);
        idmap_free(&map);
        free(line);
        return 0;
    }
    tmp.citations = (EdgeList *)xcalloc((size_t)tmp.num_books, sizeof(EdgeList));

    fc = open_file(prefix, "_citations.csv", "r");
    if (fc) {
        read_line(fc, &line, &cap);
        while (read_line(fc, &line, &cap)) {
            long long a, b, w;
            int u, v;
            if (!line[0]) continue;
            if (split(line, ',', f, 3) != 3 || !parse_long(f[0], &a) || !parse_long(f[1], &b) ||
                !parse_long(f[2], &w) || w < 0 || (u = idmap_get(&map, a)) < 0 || (v = idmap_get(&map, b)) < 0 ||
                u == v) {
                (*skipped_rows)++;
                continue;
            }
            edgelist_push(&tmp.citations[u], v, (int)w);
            edgelist_push(&tmp.citations[v], u, (int)w);
        }
        fclose(fc);
    }

    tmp.users = (User *)xcalloc((size_t)user_cap, sizeof(User));
    fu = open_file(prefix, "_users.csv", "r");
    if (fu) {
        read_line(fu, &line, &cap);
        while (read_line(fu, &line, &cap)) {
            char *in[MAX_INTERESTS + 8], **hist;
            int nin, nh, k;
            User *u;
            if (!line[0]) continue;
            if (split(line, ',', f, 4) != 4 || !f[1][0]) {
                (*skipped_rows)++;
                continue;
            }
            if (tmp.num_users == user_cap) {
                user_cap *= 2;
                tmp.users = (User *)xrealloc(tmp.users, (size_t)user_cap * sizeof(User));
            }
            u = &tmp.users[tmp.num_users];
            memset(u, 0, sizeof(*u));
            u->id = tmp.num_users;
            str_copy(u->name, MAX_AUTHOR, f[1]);
            nin = split(f[2], ';', in, MAX_INTERESTS + 7);
            for (k = 0; k < nin && k < MAX_INTERESTS + 7; ++k)
                if (in[k][0] && u->num_interests < MAX_INTERESTS)
                    str_copy(u->interests[u->num_interests++], MAX_WORD, in[k]);
            intvec_init(&u->history);
            hist = (char **)xmalloc((strlen(f[3]) + 2) * sizeof(char *));
            nh = split(f[3], ';', hist, (int)strlen(f[3]) + 1);
            for (k = 0; k < nh; ++k) {
                long long bid;
                int dense;
                if (parse_long(hist[k], &bid) && (dense = idmap_get(&map, bid)) >= 0) intvec_push(&u->history, dense);
            }
            free(hist);
            tmp.num_users++;
        }
        fclose(fu);
    }
    idmap_free(&map);
    free(line);

    library_free(lib);   /* success: replace the old library */
    *lib = tmp;
    return 1;
}
