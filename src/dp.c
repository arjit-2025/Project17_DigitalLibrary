/* dp.c - Dynamic programming modules. */
#include "dp.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

KnapsackResult knapsack_dp(const Item *items, int n, int capacity) {
    KnapsackResult r;
    const int W = capacity < 0 ? 0 : capacity;
    Item *v = (Item *)xmalloc((size_t)(n > 0 ? n : 1) * sizeof(Item));
    double *best;
    unsigned char *keep;
    int k = 0, i, w;

    knapsack_result_init(&r, "Dynamic programming (0/1 table)");
    for (i = 0; i < n; ++i)
        if (items[i].weight >= 0 && items[i].value >= 0) v[k++] = items[i];

    /* best[w]: optimal value with capacity w using items 0..i (one row, reused for every i).
     * keep[i][w] = 1 if the optimum for (i, w) takes item i -> used to rebuild the chosen set. */
    best = (double *)xcalloc((size_t)W + 1, sizeof(double));
    keep = (unsigned char *)xcalloc((size_t)k * (W + 1) + 1, 1);

    for (i = 0; i < k; ++i) {
        const int wi = v[i].weight;
        const double vi = v[i].value;
        /* w goes DOWN so best[w - wi] still holds the previous row: each item used at most once. */
        for (w = W; w >= wi; --w) {
            double take = best[w - wi] + vi;
            r.ops++;
            if (take > best[w] + 1e-12) {
                best[w] = take;
                keep[(size_t)i * (W + 1) + w] = 1;
            }
        }
    }

    w = W;   /* walk back through the decisions */
    for (i = k - 1; i >= 0; --i) {
        if (keep[(size_t)i * (W + 1) + w]) {
            intvec_push(&r.chosen, v[i].id);
            r.weight += v[i].weight;
            w -= v[i].weight;
        }
    }
    for (i = 0; i < r.chosen.size / 2; ++i) {   /* restore original item order */
        int t = r.chosen.data[i];
        r.chosen.data[i] = r.chosen.data[r.chosen.size - 1 - i];
        r.chosen.data[r.chosen.size - 1 - i] = t;
    }
    r.value = best[W];
    r.aux_bytes = (long long)(W + 1) * (long long)sizeof(double) + (long long)k * (W + 1);
    free(best);
    free(keep);
    free(v);
    return r;
}

void tokenize(const char *s, Tokens *out) {
    int cap = 16;
    char cur[MAX_WORD];
    int len = 0;
    size_t i, n = strlen(s);
    out->words = (char **)xmalloc((size_t)cap * sizeof(char *));
    out->count = 0;
    for (i = 0; i <= n; ++i) {
        unsigned char c = (unsigned char)s[i];   /* s[n] == '\0' flushes the last word */
        if (c != '\0' && isalnum(c)) {
            if (len < MAX_WORD - 1) cur[len++] = (char)tolower(c);
        } else if (len > 0) {
            cur[len] = '\0';
            if (out->count == cap) {
                cap *= 2;
                out->words = (char **)xrealloc(out->words, (size_t)cap * sizeof(char *));
            }
            out->words[out->count++] = xstrdup(cur);
            len = 0;
        }
    }
}

void tokens_free(Tokens *t) {
    int i;
    for (i = 0; i < t->count; ++i) free(t->words[i]);
    free(t->words);
    t->words = NULL;
    t->count = 0;
}

LcsResult lcs_tokens(const Tokens *a, const Tokens *b) {
    LcsResult r;
    const int n = a->count, m = b->count;
    int *L = (int *)xcalloc((size_t)(n + 1) * (m + 1), sizeof(int));
    int i, j, k;
#define AT(i, j) L[(size_t)(i) * (m + 1) + (j)]
    r.cells = 0;
    /* L[i][j] = LCS length of a[0..i-1] and b[0..j-1] */
    for (i = 1; i <= n; ++i) {
        for (j = 1; j <= m; ++j) {
            r.cells++;
            if (strcmp(a->words[i - 1], b->words[j - 1]) == 0) AT(i, j) = AT(i - 1, j - 1) + 1;
            else AT(i, j) = AT(i - 1, j) >= AT(i, j - 1) ? AT(i - 1, j) : AT(i, j - 1);
        }
    }
    r.length = AT(n, m);
    r.sequence = (const char **)xmalloc((size_t)(r.length > 0 ? r.length : 1) * sizeof(char *));

    i = n; j = m; k = r.length;   /* walk back through the table to recover one LCS */
    while (i > 0 && j > 0) {
        if (strcmp(a->words[i - 1], b->words[j - 1]) == 0) {
            r.sequence[--k] = a->words[i - 1];
            --i;
            --j;
        } else if (AT(i - 1, j) >= AT(i, j - 1)) {
            --i;
        } else {
            --j;
        }
    }
#undef AT
    free(L);
    return r;
}

void lcs_result_free(LcsResult *r) {
    free((void *)r->sequence);
    r->sequence = NULL;
}

int lcs_length(const Tokens *a, const Tokens *b, long long *cells) {
    const Tokens *lng = a->count >= b->count ? a : b;
    const Tokens *sht = a->count >= b->count ? b : a;
    const int m = sht->count;
    int *prev = (int *)xcalloc((size_t)m + 1, sizeof(int));
    int *cur = (int *)xcalloc((size_t)m + 1, sizeof(int));
    int i, j, result;
    for (i = 1; i <= lng->count; ++i) {
        int *t;
        for (j = 1; j <= m; ++j) {
            if (cells) (*cells)++;
            if (strcmp(lng->words[i - 1], sht->words[j - 1]) == 0) cur[j] = prev[j - 1] + 1;
            else cur[j] = prev[j] >= cur[j - 1] ? prev[j] : cur[j - 1];
        }
        t = prev; prev = cur; cur = t;   /* the new row becomes the previous row */
    }
    result = prev[m];
    free(prev);
    free(cur);
    return result;
}

double lcs_similarity(const Tokens *a, const Tokens *b, long long *cells) {
    if (a->count == 0 && b->count == 0) return 0.0;
    return 2.0 * lcs_length(a, b, cells) / (double)(a->count + b->count);
}
