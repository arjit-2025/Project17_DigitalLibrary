/* bench.c - Reproducible experiments comparing alternative algorithms on the same input. */
#include "bench.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backtrack.h"
#include "datagen.h"
#include "divide_conquer.h"
#include "dp.h"
#include "graph.h"
#include "greedy.h"
#include "library_engine.h"
#include "rng.h"
#include "string_match.h"
#include "timer.h"

void bench_init(BenchTable *t) {
    t->rows = NULL;
    t->size = 0;
    t->cap = 0;
}

void bench_free(BenchTable *t) {
    free(t->rows);
    bench_init(t);
}

void bench_add(BenchTable *t, const char *experiment, const char *dataset, const char *algorithm, long long n,
               double ms, long long ops, long long aux_bytes, const char *note) {
    BenchRow *r;
    if (t->size == t->cap) {
        t->cap = t->cap ? t->cap * 2 : 32;
        t->rows = (BenchRow *)xrealloc(t->rows, (size_t)t->cap * sizeof(BenchRow));
    }
    r = &t->rows[t->size++];
    str_copy(r->experiment, sizeof(r->experiment), experiment);
    str_copy(r->dataset, sizeof(r->dataset), dataset);
    str_copy(r->algorithm, sizeof(r->algorithm), algorithm);
    r->n = n;
    r->ms = ms;
    r->ops = ops;
    r->aux_bytes = aux_bytes;
    str_copy(r->note, sizeof(r->note), note);
}

static const char *size_name(int n) { return n <= 1000 ? "small" : (n <= 10000 ? "medium" : "large"); }

static int cmp_rating(const void *a, const void *b, void *ctx) {
    double x = ((const Book *)a)->rating, y = ((const Book *)b)->rating;
    (void)ctx;
    return x < y ? -1 : (x > y ? 1 : 0);
}

static int cmp_int(const void *a, const void *b, void *ctx) {
    int x = *(const int *)a, y = *(const int *)b;
    (void)ctx;
    return x < y ? -1 : (x > y ? 1 : 0);
}

static int key_book_id(const void *key, const void *elem) {
    int k = *(const int *)key, id = ((const Book *)elem)->id;
    return k < id ? -1 : (k > id ? 1 : 0);
}

static void bench_sorting(const Library *lib, BenchTable *t) {
    const int n = lib->num_books;
    Book *v = (Book *)xmalloc((size_t)n * sizeof(Book));
    char ds[48], note[96];
    const char *names[] = {"Merge sort", "Quick sort (random pivot)", "Quick sort (median of 3)", "Quick sort (last pivot)"};
    PivotRule rules[] = {PIVOT_RANDOM, PIVOT_RANDOM, PIVOT_MEDIAN3, PIVOT_LAST};
    int a;
    snprintf(ds, sizeof(ds), "%s random", size_name(n));
    for (a = 0; a < 4; ++a) {
        SortStats st = {0, 0};
        Timer tm;
        memcpy(v, lib->books, (size_t)n * sizeof(Book));   /* same input for every algorithm */
        timer_reset(&tm);
        if (a == 0) merge_sort(v, (size_t)n, sizeof(Book), cmp_rating, NULL, &st);
        else quick_sort(v, (size_t)n, sizeof(Book), cmp_rating, NULL, &st, rules[a], 2026);
        snprintf(note, sizeof(note), "depth=%d", st.max_depth);
        bench_add(t, "Sort by rating", ds, names[a], n, timer_ms(&tm), st.comparisons,
                  a == 0 ? (long long)n * (long long)sizeof(Book) : (long long)st.max_depth * 64, note);
    }
    free(v);
}

/* Already-sorted input: the worst case for a "last element" pivot. */
static void bench_sorting_adversarial(BenchTable *t) {
    int sizes[] = {1000, 5000, 20000}, s, i, a;
    const char *names[] = {"Merge sort", "Quick sort (random pivot)", "Quick sort (last pivot)"};
    for (s = 0; s < 3; ++s) {
        int n = sizes[s];
        int *v = (int *)xmalloc((size_t)n * sizeof(int));
        for (a = 0; a < 3; ++a) {
            SortStats st = {0, 0};
            Timer tm;
            char note[96];
            for (i = 0; i < n; ++i) v[i] = i;
            timer_reset(&tm);
            if (a == 0) merge_sort(v, (size_t)n, sizeof(int), cmp_int, NULL, &st);
            else quick_sort(v, (size_t)n, sizeof(int), cmp_int, NULL, &st, a == 1 ? PIVOT_RANDOM : PIVOT_LAST, 2026);
            snprintf(note, sizeof(note), a == 2 ? "depth=%d (Theta(n^2))" : "depth=%d", st.max_depth);
            bench_add(t, "Sort sorted ids", "adversarial (sorted)", names[a], n, timer_ms(&tm), st.comparisons,
                      a == 0 ? (long long)n * 4 : (long long)st.max_depth * 64, note);
        }
        free(v);
    }
}

static void bench_search(const Library *lib, BenchTable *t) {
    const int n = lib->num_books, Q = 2000;
    int *queries = (int *)xmalloc((size_t)Q * sizeof(int));
    const char *names[] = {"Linear search", "Binary search (iterative)", "Binary search (recursive)"};
    Rng rng;
    int i, a;
    rng_seed(&rng, 7);
    for (i = 0; i < Q; ++i) {
        int id = rng_range(&rng, 0, n - 1);
        queries[i] = (i % 2 == 0) ? id : id + n;   /* half of the lookups are misses */
    }
    for (a = 0; a < 3; ++a) {
        long long probes = 0;
        int found = 0;
        Timer tm;
        char note[96];
        timer_reset(&tm);
        for (i = 0; i < Q; ++i) {
            int r = a == 0 ? linear_search(lib->books, n, sizeof(Book), &queries[i], key_book_id, &probes)
                  : a == 1 ? binary_search_iterative(lib->books, n, sizeof(Book), &queries[i], key_book_id, &probes)
                           : binary_search_recursive(lib->books, n, sizeof(Book), &queries[i], key_book_id, &probes);
            if (r >= 0) ++found;
        }
        snprintf(note, sizeof(note), "found=%d", found);
        bench_add(t, "2000 id lookups", size_name(n), names[a], n, timer_ms(&tm), probes, a == 2 ? 20 * 32 : 0, note);
    }
    free(queries);
}

/* All abstracts joined into one lower-case text (caller frees). */
static char *join_abstracts(const Library *lib, int count, int lower, int *out_len) {
    size_t total = 1, pos = 0;
    int i;
    char *text;
    for (i = 0; i < count; ++i) total += strlen(lib->books[i].abstract_text) + 1;
    text = (char *)xmalloc(total);
    for (i = 0; i < count; ++i) {
        size_t len = strlen(lib->books[i].abstract_text), k;
        memcpy(text + pos, lib->books[i].abstract_text, len);
        if (lower)
            for (k = 0; k < len; ++k)
                if (text[pos + k] >= 'A' && text[pos + k] <= 'Z') text[pos + k] = (char)(text[pos + k] - 'A' + 'a');
        pos += len;
        text[pos++] = ' ';
    }
    text[pos] = '\0';
    *out_len = (int)pos;
    return text;
}

static void bench_strings(const Library *lib, BenchTable *t) {
    int len, k;
    char *text = join_abstracts(lib, lib->num_books, 1, &len);
    const char *pat = "dynamic programming";
    const int m = (int)strlen(pat);
    Matcher ms[] = {MATCH_NAIVE, MATCH_RABIN_KARP, MATCH_AUTOMATON, MATCH_KMP};
    long long aux[4];
    char ds[48];
    aux[0] = 0; aux[1] = 0; aux[2] = (long long)(m + 1) * 256 * 4; aux[3] = (long long)m * 4;
    snprintf(ds, sizeof(ds), "%s abstracts", size_name(lib->num_books));
    for (k = 0; k < 4; ++k) {
        MatchStats st;
        Timer tm;
        char note[96];
        int hits;
        match_stats_init(&st);
        timer_reset(&tm);
        hits = run_matcher(ms[k], text, len, pat, m, NULL, &st);
        snprintf(note, sizeof(note), "matches=%d pre=%lld", hits, st.preprocessing);
        bench_add(t, "Phrase 'dynamic programming'", ds, matcher_name(ms[k]), len, timer_ms(&tm), st.comparisons,
                  aux[k], note);
    }
    free(text);
}

static void bench_strings_adversarial(BenchTable *t) {
    const int n = 200000;
    char *text = (char *)xmalloc((size_t)n + 1), pat[51], *t2 = (char *)xmalloc((size_t)n + 1);
    Matcher ms[] = {MATCH_NAIVE, MATCH_RABIN_KARP, MATCH_AUTOMATON, MATCH_KMP};
    long long mods[] = {101, 1000000007LL};
    Rng rng;
    int k, i;
    memset(text, 'a', (size_t)n);
    text[n] = '\0';
    memset(pat, 'a', 49);   /* a^49 b: matches almost fully at every shift */
    pat[49] = 'b';
    pat[50] = '\0';
    for (k = 0; k < 4; ++k) {
        MatchStats st;
        Timer tm;
        char note[96];
        int hits;
        match_stats_init(&st);
        timer_reset(&tm);
        hits = run_matcher(ms[k], text, n, pat, 50, NULL, &st);
        snprintf(note, sizeof(note), "matches=%d", hits);
        bench_add(t, "Pattern a^49 b", "adversarial a^n", matcher_name(ms[k]), n, timer_ms(&tm), st.comparisons, 0, note);
    }
    /* Rabin-Karp with a tiny modulus: many hash collisions -> spurious verifications. */
    rng_seed(&rng, 3);
    for (i = 0; i < n; ++i) t2[i] = (char)('a' + rng_range(&rng, 0, 3));
    t2[n] = '\0';
    for (k = 0; k < 2; ++k) {
        MatchStats st;
        Timer tm;
        char note[96], algo[48];
        int hits;
        match_stats_init(&st);
        timer_reset(&tm);
        hits = rabin_karp_search(t2, n, "abcdabcdab", 10, NULL, &st, mods[k]);
        snprintf(note, sizeof(note), "matches=%d spurious=%lld", hits, st.spurious_hits);
        snprintf(algo, sizeof(algo), "Rabin-Karp mod=%lld", mods[k]);
        bench_add(t, "Rabin-Karp modulus", "random abcd text", algo, n, timer_ms(&tm), st.comparisons, 0, note);
    }
    free(text);
    free(t2);
}

static void bench_knapsack(const Library *lib, BenchTable *t) {
    int sizes[] = {20, 40, 100, 1000}, s, i;
    for (s = 0; s < 4; ++s) {
        int n = sizes[s], cap;
        long long total = 0;
        Item *items;
        KnapsackResult dp, gr, bb;
        double tdp, tgr, tbb;
        Timer tm;
        char ds[48], note[96];
        if (n > lib->num_books) break;
        items = (Item *)xmalloc((size_t)n * sizeof(Item));
        for (i = 0; i < n; ++i) {
            items[i].id = i;
            items[i].weight = lib->books[i].read_minutes;
            items[i].value = lib->books[i].rating * 10 + (i * 7) % 13;
            total += items[i].weight;
        }
        cap = (int)(total / 4);
        snprintf(ds, sizeof(ds), "%d candidates", n);
        timer_reset(&tm); dp = knapsack_dp(items, n, cap); tdp = timer_ms(&tm);
        timer_reset(&tm); gr = greedy_knapsack01(items, n, cap); tgr = timer_ms(&tm);
        timer_reset(&tm); bb = knapsack_branch_and_bound(items, n, cap, 3000000); tbb = timer_ms(&tm);
        snprintf(note, sizeof(note), "value=%.1f (100%%) W=%d", dp.value, cap);
        bench_add(t, "Reading plan (0/1 knapsack)", ds, dp.algorithm, n, tdp, dp.ops, dp.aux_bytes, note);
        snprintf(note, sizeof(note), "value=%.1f (%.2f%%)", gr.value, 100.0 * gr.value / dp.value);
        bench_add(t, "Reading plan (0/1 knapsack)", ds, gr.algorithm, n, tgr, gr.ops, gr.aux_bytes, note);
        snprintf(note, sizeof(note), "value=%.1f (%.2f%%)%s", bb.value, 100.0 * bb.value / dp.value,
                 bb.complete ? "" : " node limit hit");
        bench_add(t, "Reading plan (0/1 knapsack)", ds, bb.algorithm, n, tbb, bb.ops, bb.aux_bytes, note);
        knapsack_result_free(&dp);
        knapsack_result_free(&gr);
        knapsack_result_free(&bb);
        free(items);
    }
}

static void bench_dijkstra(const Library *lib, BenchTable *t) {
    long long edges = 0;
    int src = 0, i;
    Timer tm;
    PathResult pr;
    char ds[48], note[96];
    for (i = 0; i < lib->num_books; ++i) edges += lib->citations[i].size;
    timer_reset(&tm);
    pr = dijkstra(lib->citations, lib->num_books, &src, 1);
    snprintf(ds, sizeof(ds), "%s graph", size_name(lib->num_books));
    snprintf(note, sizeof(note), "settled=%d E=%lld", pr.settled, edges / 2);
    bench_add(t, "Citation distances", ds, "Dijkstra (binary heap)", lib->num_books, timer_ms(&tm), pr.relaxations,
              (long long)lib->num_books * 12 + edges * 8, note);
    path_result_free(&pr);
}

static void bench_huffman_and_similarity(const Library *lib, BenchTable *t) {
    int len;
    char *text = join_abstracts(lib, lib->num_books, 0, &len);
    Timer tm;
    HuffmanResult h;
    SearchEngine eng;
    ScoredBook top[5];
    long long cells = 0;
    int found;
    char note[96];
    timer_reset(&tm);
    h = huffman_compress(text, len);
    snprintf(note, sizeof(note), "size=%.1f%% roundtrip=%s", 100.0 * h.encoded_bits / h.original_bits,
             h.round_trip_ok ? "ok" : "FAIL");
    bench_add(t, "Compress abstracts", size_name(lib->num_books), "Huffman coding", len, timer_ms(&tm), len,
              h.distinct_symbols * 64LL, note);
    huffman_result_free(&h);
    free(text);

    engine_build(&eng, lib);
    timer_reset(&tm);
    found = engine_similar_books(&eng, 0, 5, top, &cells);
    snprintf(note, sizeof(note), "top=#%d", found > 0 ? top[0].book_id : -1);
    bench_add(t, "Similar to book #0", size_name(lib->num_books), "LCS (2-row DP) + Jaccard", lib->num_books,
              timer_ms(&tm), cells, 2 * 51 * 4, note);
    engine_free(&eng);
}

static void bench_matching(BenchTable *t) {
    int sizes[] = {1000, 10000}, s, u, k;
    for (s = 0; s < 2; ++s) {
        int n = sizes[s];
        IntVec *adj = (IntVec *)xmalloc((size_t)n * sizeof(IntVec));
        Rng rng;
        Timer tm;
        MatchingResult m;
        char ds[48], note[96];
        rng_seed(&rng, 11);
        for (u = 0; u < n; ++u) {
            intvec_init(&adj[u]);
            for (k = 0; k < 3; ++k) intvec_push(&adj[u], rng_range(&rng, 0, n - 1));
        }
        timer_reset(&tm);
        m = max_bipartite_matching(n, n, adj);
        snprintf(ds, sizeof(ds), "%d x %d", n, n);
        snprintf(note, sizeof(note), "matched=%d", m.size);
        bench_add(t, "Readers -> copies", ds, "Bipartite matching (augmenting paths)", n, timer_ms(&tm), m.steps,
                  n * 9LL, note);
        matching_result_free(&m);
        for (u = 0; u < n; ++u) intvec_free(&adj[u]);
        free(adj);
    }
}

static void bench_max_subarray(BenchTable *t) {
    int sizes[] = {100000, 1000000}, s, i;
    for (s = 0; s < 2; ++s) {
        int n = sizes[s];
        long long *a = (long long *)xmalloc((size_t)n * sizeof(long long));
        Rng rng;
        Timer tm;
        SubarrayResult d, k;
        double td, tk;
        char note[96];
        rng_seed(&rng, 5);
        for (i = 0; i < n; ++i) a[i] = rng_range(&rng, -50, 50);
        timer_reset(&tm); d = max_subarray_dc(a, n); td = timer_ms(&tm);
        timer_reset(&tm); k = max_subarray_kadane(a, n); tk = timer_ms(&tm);
        snprintf(note, sizeof(note), "sum=%lld", d.sum);
        bench_add(t, "Best borrowing period", "daily changes", "Max subarray (divide & conquer)", n, td, d.ops, 64 * 20, note);
        snprintf(note, sizeof(note), "sum=%lld", k.sum);
        bench_add(t, "Best borrowing period", "daily changes", "Max subarray (Kadane)", n, tk, k.ops, 0, note);
        free(a);
    }
}

void run_benchmarks(BenchTable *t) {
    int sizes[] = {1000, 10000, 100000}, s;
    for (s = 0; s < 3; ++s) {
        Library lib;
        library_init(&lib);
        printf("  generating %d books...\n", sizes[s]);
        fflush(stdout);
        generate_library(&lib, sizes[s], sizes[s] / 50, 2026);
        printf("  sorting / searching / matching / graphs on %d books...\n", sizes[s]);
        fflush(stdout);
        bench_sorting(&lib, t);
        bench_search(&lib, t);
        bench_strings(&lib, t);
        bench_dijkstra(&lib, t);
        if (sizes[s] <= 10000) bench_huffman_and_similarity(&lib, t);
        if (sizes[s] == 10000) bench_knapsack(&lib, t);
        library_free(&lib);
    }
    printf("  adversarial inputs...\n");
    bench_sorting_adversarial(t);
    bench_strings_adversarial(t);
    printf("  matching and max-subarray...\n");
    bench_matching(t);
    bench_max_subarray(t);
}

void print_bench_table(const BenchTable *t) {
    char last[128] = "";
    int i;
    for (i = 0; i < t->size; ++i) {
        const BenchRow *r = &t->rows[i];
        char group[128];
        snprintf(group, sizeof(group), "%s | %s", r->experiment, r->dataset);
        if (strcmp(group, last) != 0) {
            printf("\n  %s\n", group);
            printf("    %-38s %9s %11s %13s %10s  note\n", "Algorithm", "n", "time ms", "ops", "aux KB");
            str_copy(last, sizeof(last), group);
        }
        printf("    %-38s %9lld %11.3f %13lld %10.1f  %s\n", r->algorithm, r->n, r->ms, r->ops, r->aux_bytes / 1024.0,
               r->note);
    }
}

int write_bench_csv(const BenchTable *t, const char *path) {
    FILE *f = fopen(path, "w");
    int i;
    if (!f) return 0;
    fprintf(f, "experiment,dataset,algorithm,n,time_ms,operations,aux_bytes,note\n");
    for (i = 0; i < t->size; ++i) {
        const BenchRow *r = &t->rows[i];
        fprintf(f, "%s,%s,%s,%lld,%.4f,%lld,%lld,%s\n", r->experiment, r->dataset, r->algorithm, r->n, r->ms, r->ops,
                r->aux_bytes, r->note);
    }
    fclose(f);
    return 1;
}
