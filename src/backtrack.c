/* backtrack.c - Branch-and-bound / backtracking solver for the 0/1 reading plan. */
#include "backtrack.h"

#include <stdlib.h>
#include <string.h>

#include "divide_conquer.h"

typedef struct {
    Item *v;             /* items sorted by value/weight, best first */
    int n;
    int capacity;
    long long node_limit;
    long long nodes;
    int stopped;
    double best_value;
    char *cur;           /* cur[i] = 1 if item i is taken on the current path */
    char *best;          /* best set found so far */
} BBSolver;

/* Upper bound for the subtree at item i: fill the remaining capacity greedily, allowing a fraction. */
static double bound(const BBSolver *s, int i, int weight, double value) {
    double b = value;
    int remaining = s->capacity - weight, k;
    for (k = i; k < s->n && remaining > 0; ++k) {
        if (s->v[k].weight <= remaining) {
            remaining -= s->v[k].weight;
            b += s->v[k].value;
        } else {
            b += s->v[k].value * ((double)remaining / s->v[k].weight);
            remaining = 0;
        }
    }
    return b;
}

static void dfs(BBSolver *s, int i, int weight, double value) {
    if (s->stopped) return;
    if (++s->nodes > s->node_limit) { s->stopped = 1; return; }
    if (value > s->best_value + 1e-12) {          /* new best solution */
        s->best_value = value;
        memcpy(s->best, s->cur, (size_t)s->n);
    }
    if (i == s->n) return;
    if (bound(s, i, weight, value) <= s->best_value + 1e-12) return;   /* prune: cannot improve */

    if (weight + s->v[i].weight <= s->capacity) { /* branch 1: take item i */
        s->cur[i] = 1;
        dfs(s, i + 1, weight + s->v[i].weight, value + s->v[i].value);
        s->cur[i] = 0;
    }
    dfs(s, i + 1, weight, value);                 /* branch 2: skip item i */
}

static double ratio(const Item *it) { return it->weight == 0 ? 1e18 : it->value / it->weight; }

static int cmp_ratio_desc(const void *a, const void *b, void *ctx) {
    double ra = ratio((const Item *)a), rb = ratio((const Item *)b);
    (void)ctx;
    return ra > rb ? -1 : (ra < rb ? 1 : 0);
}

KnapsackResult knapsack_branch_and_bound(const Item *items, int n, int capacity, long long node_limit) {
    KnapsackResult r;
    BBSolver s;
    SortStats st = {0, 0};
    int i;

    knapsack_result_init(&r, "Backtracking + bound (B&B)");
    s.v = (Item *)xmalloc((size_t)(n > 0 ? n : 1) * sizeof(Item));
    s.n = 0;
    for (i = 0; i < n; ++i)
        if (items[i].weight >= 0 && items[i].value >= 0) s.v[s.n++] = items[i];
    merge_sort(s.v, (size_t)s.n, sizeof(Item), cmp_ratio_desc, NULL, &st);

    s.capacity = capacity < 0 ? 0 : capacity;
    s.node_limit = node_limit;
    s.nodes = 0;
    s.stopped = 0;
    s.best_value = 0.0;
    s.cur = (char *)xcalloc((size_t)s.n + 1, 1);
    s.best = (char *)xcalloc((size_t)s.n + 1, 1);
    dfs(&s, 0, 0, 0.0);

    for (i = 0; i < s.n; ++i) {
        if (s.best[i]) {
            intvec_push(&r.chosen, s.v[i].id);
            r.weight += s.v[i].weight;
        }
    }
    r.value = s.best_value;
    r.ops = s.nodes + st.comparisons;
    r.complete = !s.stopped;
    r.aux_bytes = (long long)s.n * (2 + (long long)sizeof(Item)) + (long long)s.n * 32;   /* arrays + stack */
    free(s.v);
    free(s.cur);
    free(s.best);
    return r;
}
