/* library_engine.c - Search and recommendation built on the algorithm modules. */
#include "library_engine.h"

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "divide_conquer.h"
#include "dp.h"
#include "graph.h"

char *to_lower_copy(const char *s) {
    char *r = xstrdup(s);
    size_t i;
    for (i = 0; r[i]; ++i) r[i] = (char)tolower((unsigned char)r[i]);
    return r;
}

/* ---------------- Index construction ---------------- */

typedef struct {
    char word[MAX_WORD];
    int id;
} TermPair;

typedef struct {
    TermPair *a;
    int size, cap;
} PairVec;

static void pairs_push(PairVec *p, const char *word, int id) {
    if (p->size == p->cap) {
        p->cap = p->cap ? p->cap * 2 : 1024;
        p->a = (TermPair *)xrealloc(p->a, (size_t)p->cap * sizeof(TermPair));
    }
    str_copy(p->a[p->size].word, MAX_WORD, word);
    p->a[p->size].id = id;
    p->size++;
}

static int cmp_pair_word(const void *a, const void *b, void *ctx) {
    (void)ctx;
    return strcmp(((const TermPair *)a)->word, ((const TermPair *)b)->word);
}

static void add_tokens(PairVec *p, const char *text, int id) {
    Tokens t;
    int k;
    tokenize(text, &t);
    for (k = 0; k < t.count; ++k) pairs_push(p, t.words[k], id);
    tokens_free(&t);
}

void engine_build(SearchEngine *e, const Library *lib) {
    PairVec pairs = {NULL, 0, 0};
    SortStats st = {0, 0};
    int i, k, start;

    e->lib = lib;
    e->terms = NULL;
    e->num_terms = 0;
    e->lowered_abstracts = (char **)xmalloc((size_t)(lib->num_books > 0 ? lib->num_books : 1) * sizeof(char *));

    /* 1. Collect (term, book id) pairs, books in id order. */
    for (i = 0; i < lib->num_books; ++i) {
        const Book *b = &lib->books[i];
        for (k = 0; k < b->num_keywords; ++k) add_tokens(&pairs, b->keywords[k], b->id);
        add_tokens(&pairs, b->title, b->id);
        e->lowered_abstracts[i] = to_lower_copy(b->abstract_text ? b->abstract_text : "");
    }

    /* 2. Sort by term only. Merge sort is STABLE, so the ids of each term stay in ascending order. */
    merge_sort(pairs.a, (size_t)pairs.size, sizeof(TermPair), cmp_pair_word, NULL, &st);

    /* 3. Group runs of equal terms into posting lists (skipping duplicate ids). */
    e->terms = (Posting *)xmalloc((size_t)(pairs.size > 0 ? pairs.size : 1) * sizeof(Posting));
    start = 0;
    while (start < pairs.size) {
        int end = start;
        Posting *p = &e->terms[e->num_terms++];
        while (end < pairs.size && strcmp(pairs.a[end].word, pairs.a[start].word) == 0) ++end;
        str_copy(p->word, MAX_WORD, pairs.a[start].word);
        p->ids = (int *)xmalloc((size_t)(end - start) * sizeof(int));
        p->count = 0;
        for (k = start; k < end; ++k)
            if (p->count == 0 || p->ids[p->count - 1] != pairs.a[k].id) p->ids[p->count++] = pairs.a[k].id;
        start = end;
    }
    e->terms = (Posting *)xrealloc(e->terms, (size_t)(e->num_terms > 0 ? e->num_terms : 1) * sizeof(Posting));
    free(pairs.a);
}

void engine_free(SearchEngine *e) {
    int i;
    for (i = 0; i < e->num_terms; ++i) free(e->terms[i].ids);
    free(e->terms);
    if (e->lowered_abstracts && e->lib)
        for (i = 0; i < e->lib->num_books; ++i) free(e->lowered_abstracts[i]);
    free(e->lowered_abstracts);
    e->terms = NULL;
    e->num_terms = 0;
    e->lowered_abstracts = NULL;
}

/* ---------------- Keyword search ---------------- */

static int cmp_key_posting(const void *key, const void *elem) {
    return strcmp((const char *)key, ((const Posting *)elem)->word);
}

/* Intersection of two sorted id lists in Theta(|a| + |b|) (two pointers, like merging). */
static void intersect_sorted(const int *a, int na, const int *b, int nb, IntVec *out, long long *ops) {
    int i = 0, j = 0;
    intvec_clear(out);
    while (i < na && j < nb) {
        (*ops)++;
        if (a[i] < b[j]) ++i;
        else if (b[j] < a[i]) ++j;
        else { intvec_push(out, a[i]); ++i; ++j; }
    }
}

static int cmp_rating_desc(const void *a, const void *b, void *ctx) {
    const Library *lib = (const Library *)ctx;
    int x = *(const int *)a, y = *(const int *)b;
    double rx = lib->books[x].rating, ry = lib->books[y].rating;
    if (rx != ry) return rx > ry ? -1 : 1;
    return x < y ? -1 : (x > y ? 1 : 0);
}

int engine_keyword_search(const SearchEngine *e, const char *query, IntVec *out, long long *ops) {
    Tokens t;
    IntVec cur, tmp;
    SortStats st = {0, 0};
    int k, i;
    intvec_clear(out);
    tokenize(query, &t);
    intvec_init(&cur);
    intvec_init(&tmp);
    for (k = 0; k < t.count; ++k) {
        long long probes = 0;
        int idx = binary_search_iterative(e->terms, e->num_terms, sizeof(Posting), t.words[k], cmp_key_posting,
                                          &probes);   /* O(log T) */
        const Posting *p;
        *ops += probes;
        if (idx < 0) { intvec_clear(&cur); break; }   /* unknown word: AND query has no results */
        p = &e->terms[idx];
        if (k == 0) {
            for (i = 0; i < p->count; ++i) intvec_push(&cur, p->ids[i]);
        } else {
            IntVec swap;
            intersect_sorted(cur.data, cur.size, p->ids, p->count, &tmp, ops);
            swap = cur; cur = tmp; tmp = swap;
        }
        if (cur.size == 0) break;
    }
    if (cur.size > 0) {
        merge_sort(cur.data, (size_t)cur.size, sizeof(int), cmp_rating_desc, (void *)e->lib, &st);
        *ops += st.comparisons;
        for (i = 0; i < cur.size; ++i) intvec_push(out, cur.data[i]);
    }
    intvec_free(&cur);
    intvec_free(&tmp);
    tokens_free(&t);
    return out->size;
}

/* ---------------- Phrase search ---------------- */

int engine_phrase_search(const SearchEngine *e, const char *phrase, Matcher m, IntVec *out, MatchStats *st) {
    char *p = to_lower_copy(phrase);
    char *start = p, *end;
    int i, len;
    intvec_clear(out);
    while (*start && isspace((unsigned char)*start)) ++start;   /* trim */
    end = start + strlen(start);
    while (end > start && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    len = (int)strlen(start);
    if (len > 0) {
        for (i = 0; i < e->lib->num_books; ++i) {
            const char *text = e->lowered_abstracts[i];
            if (run_matcher(m, text, (int)strlen(text), start, len, NULL, st) > 0) intvec_push(out, i);
        }
    }
    free(p);
    return out->size;
}

/* ---------------- Similar books ---------------- */

static double jaccard(const Book *a, const Book *b) {
    int i, j, inter = 0, ua = 0, ub = 0;
    for (i = 0; i < a->num_keywords; ++i) {
        int seen = 0, found = 0;
        for (j = 0; j < i; ++j) if (strcmp(a->keywords[i], a->keywords[j]) == 0) seen = 1;
        if (seen) continue;
        ++ua;
        for (j = 0; j < b->num_keywords; ++j) if (strcmp(a->keywords[i], b->keywords[j]) == 0) found = 1;
        inter += found;
    }
    for (i = 0; i < b->num_keywords; ++i) {
        int seen = 0;
        for (j = 0; j < i; ++j) if (strcmp(b->keywords[i], b->keywords[j]) == 0) seen = 1;
        if (!seen) ++ub;
    }
    if (ua + ub - inter == 0) return 0.0;
    return (double)inter / (double)(ua + ub - inter);
}

static int cmp_scored_desc(const void *a, const void *b, void *ctx) {
    const ScoredBook *x = (const ScoredBook *)a, *y = (const ScoredBook *)b;
    (void)ctx;
    if (x->score != y->score) return x->score > y->score ? -1 : 1;
    return x->book_id < y->book_id ? -1 : (x->book_id > y->book_id ? 1 : 0);
}

int engine_similar_books(const SearchEngine *e, int book_id, int k, ScoredBook *out, long long *cells) {
    const Library *lib = e->lib;
    ScoredBook *all;
    Tokens target;
    SortStats st = {0, 0};
    int j, count = 0, written;
    if (book_id < 0 || book_id >= lib->num_books || k <= 0) return 0;
    tokenize(lib->books[book_id].abstract_text, &target);
    all = (ScoredBook *)xmalloc((size_t)(lib->num_books > 0 ? lib->num_books : 1) * sizeof(ScoredBook));
    for (j = 0; j < lib->num_books; ++j) {
        Tokens other;
        if (j == book_id) continue;
        tokenize(lib->books[j].abstract_text, &other);
        all[count].book_id = j;
        all[count].score = 0.6 * lcs_similarity(&target, &other, cells) + 0.4 * jaccard(&lib->books[book_id], &lib->books[j]);
        ++count;
        tokens_free(&other);
    }
    merge_sort(all, (size_t)count, sizeof(ScoredBook), cmp_scored_desc, NULL, &st);
    written = count < k ? count : k;
    memcpy(out, all, (size_t)written * sizeof(ScoredBook));
    free(all);
    tokens_free(&target);
    return written;
}

/* ---------------- Recommendations ---------------- */

static int str_ieq(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        ++a;
        ++b;
    }
    return *a == *b;
}

static int cmp_rec_desc(const void *a, const void *b, void *ctx) {
    const Recommendation *x = (const Recommendation *)a, *y = (const Recommendation *)b;
    (void)ctx;
    if (x->score != y->score) return x->score > y->score ? -1 : 1;
    return x->book_id < y->book_id ? -1 : (x->book_id > y->book_id ? 1 : 0);
}

int engine_recommend(const SearchEngine *e, int user_id, int k, Recommendation *out) {
    const Library *lib = e->lib;
    const User *u;
    char *already_read;
    Recommendation *all;
    PathResult pr;
    SortStats st = {0, 0};
    int n = lib->num_books, b, i, count = 0, written;
    if (user_id < 0 || user_id >= lib->num_users || k <= 0) return 0;
    u = &lib->users[user_id];

    already_read = (char *)xcalloc((size_t)(n > 0 ? n : 1), 1);
    for (i = 0; i < u->history.size; ++i)
        if (u->history.data[i] >= 0 && u->history.data[i] < n) already_read[u->history.data[i]] = 1;
    /* Distance from the nearest already-read book to every book: ONE multi-source Dijkstra run. */
    pr = dijkstra(lib->citations, n, u->history.data, u->history.size);

    all = (Recommendation *)xmalloc((size_t)(n > 0 ? n : 1) * sizeof(Recommendation));
    for (b = 0; b < n; ++b) {
        const Book *bk = &lib->books[b];
        Recommendation *r;
        int overlap = 0, x, y;
        if (already_read[b]) continue;
        for (x = 0; x < bk->num_keywords; ++x)
            for (y = 0; y < u->num_interests; ++y)
                if (str_ieq(bk->keywords[x], u->interests[y])) { ++overlap; break; }
        r = &all[count++];
        r->book_id = b;
        r->topic_score = u->num_interests ? (double)overlap / u->num_interests : 0.0;
        r->citation_distance = pr.dist[b];
        r->graph_score = pr.dist[b] >= INF_DIST ? 0.0 : 1.0 / (1.0 + pr.dist[b] / 5.0);
        r->rating_score = bk->rating / 5.0;
        r->score = 0.5 * r->topic_score + 0.3 * r->graph_score + 0.2 * r->rating_score;
    }
    merge_sort(all, (size_t)count, sizeof(Recommendation), cmp_rec_desc, NULL, &st);
    written = count < k ? count : k;
    memcpy(out, all, (size_t)written * sizeof(Recommendation));
    free(all);
    free(already_read);
    path_result_free(&pr);
    return written;
}

int engine_reading_candidates(const SearchEngine *e, int user_id, int max_items, Item *out) {
    Recommendation *recs;
    int n, i;
    if (max_items <= 0) return 0;
    recs = (Recommendation *)xmalloc((size_t)max_items * sizeof(Recommendation));
    n = engine_recommend(e, user_id, max_items, recs);
    for (i = 0; i < n; ++i) {
        out[i].id = recs[i].book_id;
        out[i].weight = e->lib->books[recs[i].book_id].read_minutes;
        out[i].value = floor(recs[i].score * 1000.0 + 0.5) / 10.0;   /* score*100, one decimal */
    }
    free(recs);
    return n;
}
