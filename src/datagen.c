/* datagen.c - Builds a synthetic but realistic library: books with keywords and
 * abstracts, a citation graph, and readers with histories and interests. */
#include "datagen.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rng.h"

static const char *TOPICS[] = {
    "algorithms", "graphs", "sorting", "searching", "dynamic", "programming", "greedy", "networks",
    "databases", "compilers", "security", "cryptography", "learning", "vision", "robotics", "physics",
    "biology", "chemistry", "history", "economics", "statistics", "geometry", "optimization",
    "complexity", "parallel", "distributed", "systems", "language", "retrieval", "compression"};
#define NUM_TOPICS ((int)(sizeof(TOPICS) / sizeof(TOPICS[0])))

static const char *FILLER[] = {
    "study", "method", "analysis", "approach", "model", "results", "efficient", "novel", "data",
    "performance", "framework", "evaluation", "problem", "solution", "theory", "experiments", "design",
    "structure", "technique", "system", "the", "of", "and", "for", "with", "in", "a", "new", "large", "scale"};
#define NUM_FILLER ((int)(sizeof(FILLER) / sizeof(FILLER[0])))

static const char *PREFIX[] = {"Introduction to", "Advanced", "Practical", "Foundations of", "Modern",
                               "Applied", "Essentials of", "Principles of", "Handbook of", "Topics in"};
#define NUM_PREFIX ((int)(sizeof(PREFIX) / sizeof(PREFIX[0])))

static const char *FIRST[] = {"Asha", "Ravi", "Meera", "Arjun", "Kavya", "Rohan", "Isha", "Vikram",
                              "Neha", "Karan", "Divya", "Aman", "Pooja", "Sahil", "Tara", "Nikhil"};
static const char *LAST[] = {"Sharma", "Verma", "Iyer", "Gupta", "Rao", "Nair", "Singh", "Das",
                             "Mehta", "Joshi", "Kapoor", "Bose", "Reddy", "Pillai"};
#define NUM_FIRST ((int)(sizeof(FIRST) / sizeof(FIRST[0])))
#define NUM_LAST ((int)(sizeof(LAST) / sizeof(LAST[0])))

/* Picks k distinct values from [0, range) into out (k <= range). */
static void pick_distinct(Rng *rng, int k, int range, int *out) {
    int count = 0;
    while (count < k) {
        int x = rng_range(rng, 0, range - 1), j, dup = 0;
        for (j = 0; j < count; ++j)
            if (out[j] == x) dup = 1;
        if (!dup) out[count++] = x;
    }
}

static int shared_keywords(const Book *a, const Book *b) {
    int i, j, c = 0;
    for (i = 0; i < a->num_keywords; ++i)
        for (j = 0; j < b->num_keywords; ++j)
            if (strcmp(a->keywords[i], b->keywords[j]) == 0) ++c;
    return c;
}

/* Appends word to a growable text buffer. */
static void append(char **buf, size_t *len, size_t *cap, const char *word) {
    size_t w = strlen(word);
    if (*len + w + 2 > *cap) {
        while (*len + w + 2 > *cap) *cap *= 2;
        *buf = (char *)xrealloc(*buf, *cap);
    }
    if (*len > 0) (*buf)[(*len)++] = ' ';
    memcpy(*buf + *len, word, w);
    *len += w;
    (*buf)[*len] = '\0';
}

void generate_library(Library *lib, int n_books, int n_users, unsigned long long seed) {
    Rng rng;
    int i, u;
    library_free(lib);
    if (n_books < 0) n_books = 0;
    if (n_users < 0) n_users = 0;
    rng_seed(&rng, seed);

    /* ---- Books ---- */
    lib->books = (Book *)xcalloc((size_t)n_books, sizeof(Book));
    lib->num_books = n_books;
    for (i = 0; i < n_books; ++i) {
        Book *b = &lib->books[i];
        int picked[5], nk = rng_range(&rng, 3, 5), k, words, w;
        char kw0[MAX_WORD], kw1[MAX_WORD];
        size_t len = 0, cap = 256;

        b->id = i;
        pick_distinct(&rng, nk, NUM_TOPICS, picked);   /* random order already */
        b->num_keywords = nk;
        for (k = 0; k < nk; ++k) str_copy(b->keywords[k], MAX_WORD, TOPICS[picked[k]]);

        str_copy(kw0, MAX_WORD, b->keywords[0]);
        str_copy(kw1, MAX_WORD, b->keywords[1]);
        kw0[0] = (char)toupper((unsigned char)kw0[0]);
        kw1[0] = (char)toupper((unsigned char)kw1[0]);
        snprintf(b->title, MAX_TITLE, "%s %s and %s Vol %d", PREFIX[rng_range(&rng, 0, NUM_PREFIX - 1)], kw0, kw1,
                 i % 7 + 1);
        snprintf(b->author, MAX_AUTHOR, "%s %s", FIRST[rng_range(&rng, 0, NUM_FIRST - 1)],
                 LAST[rng_range(&rng, 0, NUM_LAST - 1)]);
        b->year = rng_range(&rng, 1970, 2026);
        b->rating = rng_range(&rng, 10, 50) / 10.0;
        b->read_minutes = rng_range(&rng, 10, 120);

        b->abstract_text = (char *)xmalloc(cap);
        b->abstract_text[0] = '\0';
        words = rng_range(&rng, 25, 50);
        for (w = 0; w < words; ++w) {
            int r = rng_range(&rng, 0, 99);
            const char *word;
            if (r < 4) word = "dynamic programming";
            else if (r < 7) word = "shortest path";
            else if (r < 40) word = b->keywords[rng_range(&rng, 0, nk - 1)];
            else word = FILLER[rng_range(&rng, 0, NUM_FILLER - 1)];
            append(&b->abstract_text, &len, &cap, word);
        }
    }

    /* ---- Citation graph: each book cites 1..3 earlier books ---- */
    lib->citations = (EdgeList *)xcalloc((size_t)(n_books > 0 ? n_books : 1), sizeof(EdgeList));
    for (i = 1; i < n_books; ++i) {
        int k = rng_range(&rng, 1, 3), c;
        for (c = 0; c < k; ++c) {
            /* 70% of citations go to recent books (locality), 30% anywhere earlier. */
            int lo = i - 50 > 0 ? i - 50 : 0;
            int j = rng_range(&rng, 0, 9) < 7 ? rng_range(&rng, lo, i - 1) : rng_range(&rng, 0, i - 1);
            int wgt = 10 - 3 * shared_keywords(&lib->books[i], &lib->books[j]);
            if (wgt < 1) wgt = 1;
            edgelist_push(&lib->citations[i], j, wgt);
            edgelist_push(&lib->citations[j], i, wgt);
        }
    }

    /* ---- Users ---- */
    lib->users = (User *)xcalloc((size_t)(n_users > 0 ? n_users : 1), sizeof(User));
    lib->num_users = n_users;
    for (u = 0; u < n_users; ++u) {
        User *us = &lib->users[u];
        int topics[3], k;
        us->id = u;
        snprintf(us->name, MAX_AUTHOR, "Reader_%d", u);
        pick_distinct(&rng, 3, NUM_TOPICS, topics);
        us->num_interests = 3;
        for (k = 0; k < 3; ++k) str_copy(us->interests[k], MAX_WORD, TOPICS[topics[k]]);
        intvec_init(&us->history);
        if (n_books > 0) {
            int h = rng_range(&rng, 3, 8), *hist;
            if (h > n_books) h = n_books;
            hist = (int *)xmalloc((size_t)h * sizeof(int));
            pick_distinct(&rng, h, n_books, hist);
            for (k = 0; k < h; ++k) intvec_push(&us->history, hist[k]);
            free(hist);
        }
    }
}
