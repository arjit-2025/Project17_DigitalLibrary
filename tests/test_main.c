/* test_main.c - Unit, oracle, edge-case and integration tests (no external framework).
 * Oracles: C library qsort for sorting, the naive matcher for string matching, brute force
 * for knapsack / matching / max subarray / rooms, Floyd-Warshall for shortest paths. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backtrack.h"
#include "datagen.h"
#include "divide_conquer.h"
#include "dp.h"
#include "graph.h"
#include "greedy.h"
#include "heap.h"
#include "io.h"
#include "library_engine.h"
#include "rng.h"
#include "string_match.h"

static int g_pass = 0, g_fail = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (cond) ++g_pass;                                                      \
        else { ++g_fail; printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
    } while (0)

static int cmp_int(const void *a, const void *b, void *ctx) {
    int x = *(const int *)a, y = *(const int *)b;
    (void)ctx;
    return x < y ? -1 : (x > y ? 1 : 0);
}
static int qsort_int(const void *a, const void *b) { return cmp_int(a, b, NULL); }
static int key_int(const void *key, const void *elem) {
    int k = *(const int *)key, e = *(const int *)elem;
    return k < e ? -1 : (k > e ? 1 : 0);
}

typedef struct { int key, order; } Pair;
static int cmp_pair_key(const void *a, const void *b, void *ctx) {
    (void)ctx;
    return cmp_int(&((const Pair *)a)->key, &((const Pair *)b)->key, NULL);
}

static void check_sorts(const int *input, int n) {
    int *expect = (int *)xmalloc((size_t)(n > 0 ? n : 1) * sizeof(int));
    int *v = (int *)xmalloc((size_t)(n > 0 ? n : 1) * sizeof(int));
    PivotRule rules[] = {PIVOT_LAST, PIVOT_RANDOM, PIVOT_MEDIAN3};
    SortStats st = {0, 0};
    int r;
    memcpy(expect, input, (size_t)n * sizeof(int));
    qsort(expect, (size_t)n, sizeof(int), qsort_int);
    memcpy(v, input, (size_t)n * sizeof(int));
    merge_sort(v, (size_t)n, sizeof(int), cmp_int, NULL, &st);
    CHECK(memcmp(v, expect, (size_t)n * sizeof(int)) == 0);
    for (r = 0; r < 3; ++r) {
        memcpy(v, input, (size_t)n * sizeof(int));
        quick_sort(v, (size_t)n, sizeof(int), cmp_int, NULL, &st, rules[r], 99);
        CHECK(memcmp(v, expect, (size_t)n * sizeof(int)) == 0);
    }
    free(expect);
    free(v);
}

static void test_sorting(void) {
    Rng rng;
    int a[1000], i;
    Pair p[200];
    SortStats st = {0, 0};
    int stable = 1;
    rng_seed(&rng, 1);
    check_sorts(a, 0);
    a[0] = 5; check_sorts(a, 1);
    for (i = 0; i < 50; ++i) a[i] = 7;                                     /* all equal */
    check_sorts(a, 50);
    for (i = 0; i < 300; ++i) a[i] = i;                                    /* sorted */
    check_sorts(a, 300);
    for (i = 0; i < 300; ++i) a[i] = 300 - i;                              /* reversed */
    check_sorts(a, 300);
    for (i = 0; i < 1000; ++i) a[i] = rng_range(&rng, -50000, 50000);      /* random */
    check_sorts(a, 1000);
    for (i = 0; i < 1000; ++i) a[i] = rng_range(&rng, 0, 4);               /* duplicates */
    check_sorts(a, 1000);

    for (i = 0; i < 200; ++i) { p[i].key = rng_range(&rng, 0, 3); p[i].order = i; }
    merge_sort(p, 200, sizeof(Pair), cmp_pair_key, NULL, &st);
    for (i = 1; i < 200; ++i)
        if (p[i - 1].key == p[i].key && p[i - 1].order > p[i].order) stable = 0;
    CHECK(stable);   /* merge sort keeps equal keys in their original order */

    {
        int *s = (int *)xmalloc(2000 * sizeof(int));
        SortStats qs = {0, 0};
        for (i = 0; i < 2000; ++i) s[i] = i;
        quick_sort(s, 2000, sizeof(int), cmp_int, NULL, &qs, PIVOT_LAST, 1);
        CHECK(qs.comparisons > 1000000LL);   /* last pivot on sorted input really is ~n^2/2 */
        free(s);
    }
}

static void test_searching(void) {
    int a[500], i, t;
    long long p = 0;
    for (i = 0; i < 500; ++i) a[i] = i * 3;
    for (t = -5; t < 1505; ++t) {
        int lin = linear_search(a, 500, sizeof(int), &t, key_int, &p);
        int it = binary_search_iterative(a, 500, sizeof(int), &t, key_int, &p);
        int rc = binary_search_recursive(a, 500, sizeof(int), &t, key_int, &p);
        CHECK(lin == it && it == rc);
    }
    t = 1;
    CHECK(binary_search_iterative(a, 0, sizeof(int), &t, key_int, &p) == -1);
    CHECK(binary_search_recursive(a, 0, sizeof(int), &t, key_int, &p) == -1);
    p = 0;
    t = 999999;
    binary_search_iterative(a, 500, sizeof(int), &t, key_int, &p);
    CHECK(p <= 10);   /* ceil(log2(500)) + 1 */
}

static void test_heap(void) {
    MinHeap h;
    Rng rng;
    int i, ok = 1;
    long long last = -1;
    heap_init(&h);
    rng_seed(&rng, 8);
    for (i = 0; i < 1000; ++i) heap_push(&h, rng_range(&rng, 0, 500), i);
    while (!heap_empty(&h)) {
        HeapItem x = heap_pop(&h);
        if (x.key < last) ok = 0;
        last = x.key;
    }
    CHECK(ok);   /* items come out in non-decreasing key order */
    heap_free(&h);
}

static void test_max_subarray_and_pow(void) {
    Rng rng;
    int t, i, j;
    long long a[40], mults = 0, slow = 1, neg[3] = {-4, -4, -4};
    rng_seed(&rng, 2);
    for (t = 0; t < 200; ++t) {
        int n = rng_range(&rng, 1, 40);
        long long brute, s;
        SubarrayResult d, k;
        for (i = 0; i < n; ++i) a[i] = rng_range(&rng, -20, 20);
        brute = a[0];
        for (i = 0; i < n; ++i) {
            s = 0;
            for (j = i; j < n; ++j) { s += a[j]; if (s > brute) brute = s; }
        }
        d = max_subarray_dc(a, n);
        k = max_subarray_kadane(a, n);
        CHECK(d.sum == brute && k.sum == brute);
        s = 0;
        for (i = d.left; i <= d.right; ++i) s += a[i];
        CHECK(s == d.sum);
    }
    CHECK(max_subarray_dc(neg, 3).sum == -4 && max_subarray_kadane(neg, 3).sum == -4);
    CHECK(max_subarray_dc(neg, 0).left == -1);

    CHECK(pow_mod(2, 10, 1000000007LL, NULL) == 1024);
    CHECK(pow_mod(3, 0, 7, NULL) == 1);
    CHECK(pow_mod(5, 3, 1, NULL) == 0);
    pow_mod(7, 1000000, 1000000007LL, &mults);
    CHECK(mults <= 40);   /* Theta(log e) */
    for (i = 0; i < 77; ++i) slow = slow * 123 % 1000003;
    CHECK(pow_mod(123, 77, 1000003, NULL) == slow);
}

static void test_strings(void) {
    Rng rng;
    int t, i, pi[7], expect_pi[7] = {0, 0, 1, 2, 3, 0, 1};
    MatchStats st;
    rng_seed(&rng, 3);
    for (t = 0; t < 300; ++t) {
        char text[64], pat[8];
        int n = rng_range(&rng, 0, 59), m = rng_range(&rng, 1, 5);
        IntVec e, r1, r2, r3, r4;
        MatchStats s0, s1, s2, s3, s4;
        for (i = 0; i < n; ++i) text[i] = (char)('a' + rng_range(&rng, 0, 1));
        text[n] = '\0';
        for (i = 0; i < m; ++i) pat[i] = (char)('a' + rng_range(&rng, 0, 1));
        pat[m] = '\0';
        intvec_init(&e); intvec_init(&r1); intvec_init(&r2); intvec_init(&r3); intvec_init(&r4);
        match_stats_init(&s0); match_stats_init(&s1); match_stats_init(&s2); match_stats_init(&s3); match_stats_init(&s4);
        naive_search(text, n, pat, m, &e, &s0);
        kmp_search(text, n, pat, m, &r1, &s1);
        rabin_karp_search(text, n, pat, m, &r2, &s2, 1000000007LL);
        automaton_search(text, n, pat, m, &r3, &s3);
        rabin_karp_search(text, n, pat, m, &r4, &s4, 3);   /* tiny modulus: collisions verified away */
        CHECK(intvec_equal(&e, &r1));
        CHECK(intvec_equal(&e, &r2));
        CHECK(intvec_equal(&e, &r3));
        CHECK(intvec_equal(&e, &r4));
        intvec_free(&e); intvec_free(&r1); intvec_free(&r2); intvec_free(&r3); intvec_free(&r4);
    }
    match_stats_init(&st);
    CHECK(kmp_search("abc", 3, "", 0, NULL, &st) == 0);
    CHECK(naive_search("ab", 2, "abc", 3, NULL, &st) == 0);
    CHECK(automaton_search("aaaa", 4, "aa", 2, NULL, &st) == 3);   /* overlapping matches */
    compute_prefix_function("ababaca", 7, pi, NULL);
    CHECK(memcmp(pi, expect_pi, sizeof(pi)) == 0);
    {
        char *big = (char *)xmalloc(10001), pat[22];
        MatchStats k;
        memset(big, 'a', 10000); big[10000] = '\0';
        memset(pat, 'a', 20); pat[20] = 'b'; pat[21] = '\0';
        match_stats_init(&k);
        kmp_search(big, 10000, pat, 21, NULL, &k);
        CHECK(k.comparisons <= 20000);   /* KMP scan never exceeds 2n comparisons */
        free(big);
    }
}

static double brute_knapsack(const Item *items, int n, int cap) {
    double best = 0;
    int mask, i;
    for (mask = 0; mask < (1 << n); ++mask) {
        int w = 0;
        double v = 0;
        for (i = 0; i < n; ++i)
            if (mask & (1 << i)) { w += items[i].weight; v += items[i].value; }
        if (w <= cap && v > best) best = v;
    }
    return best;
}

static void test_knapsack(void) {
    Rng rng;
    int t, i;
    Item classic[3] = {{1, 10, 60}, {2, 20, 100}, {3, 30, 120}};
    KnapsackResult r;
    rng_seed(&rng, 4);
    for (t = 0; t < 150; ++t) {
        Item items[13];
        int n = rng_range(&rng, 0, 12), total = 0, cap;
        double opt, frac, sum = 0;
        long long ops = 0;
        KnapsackResult dp, bb, gr;
        for (i = 0; i < n; ++i) {
            items[i].id = i;
            items[i].weight = rng_range(&rng, 1, 30);
            items[i].value = rng_range(&rng, 1, 50);
            total += items[i].weight;
        }
        cap = rng_range(&rng, 0, total + 1);
        opt = brute_knapsack(items, n, cap);
        dp = knapsack_dp(items, n, cap);
        bb = knapsack_branch_and_bound(items, n, cap, 5000000);
        gr = greedy_knapsack01(items, n, cap);
        frac = fractional_knapsack(items, n, cap, &ops);
        CHECK(fabs(dp.value - opt) < 1e-9);
        CHECK(fabs(bb.value - opt) < 1e-9 && bb.complete);
        CHECK(gr.value <= opt + 1e-9);
        CHECK(frac >= opt - 1e-9);
        CHECK(dp.weight <= cap && bb.weight <= cap && gr.weight <= cap);
        for (i = 0; i < dp.chosen.size; ++i) sum += items[dp.chosen.data[i]].value;
        CHECK(fabs(sum - dp.value) < 1e-9);   /* the rebuilt set really has the reported value */
        knapsack_result_free(&dp);
        knapsack_result_free(&bb);
        knapsack_result_free(&gr);
    }
    r = knapsack_dp(classic, 3, 50);    CHECK(r.value == 220); knapsack_result_free(&r);
    r = greedy_knapsack01(classic, 3, 50); CHECK(r.value == 160); knapsack_result_free(&r);   /* greedy not optimal */
    r = knapsack_dp(classic, 3, 0);     CHECK(r.value == 0); knapsack_result_free(&r);
    r = knapsack_dp(classic, 0, 100);   CHECK(r.chosen.size == 0); knapsack_result_free(&r);
}

static void make_tokens(const char *letters, Tokens *t) {
    char buf[128];
    size_t i, k = 0;
    for (i = 0; letters[i]; ++i) { buf[k++] = letters[i]; buf[k++] = ' '; }   /* "ABC" -> "A B C" */
    buf[k] = '\0';
    tokenize(buf, t);
}

static void test_lcs(void) {
    Tokens x, y, empty, t;
    LcsResult r;
    make_tokens("ABCBDAB", &x);
    make_tokens("BDCABA", &y);
    make_tokens("", &empty);
    r = lcs_tokens(&x, &y);
    CHECK(r.length == 4);
    CHECK(lcs_length(&x, &y, NULL) == 4);
    CHECK(lcs_length(&x, &empty, NULL) == 0);
    CHECK(fabs(lcs_similarity(&x, &x, NULL) - 1.0) < 1e-12);
    lcs_result_free(&r);
    tokenize("Hello, World! 42 times", &t);
    CHECK(t.count == 4 && strcmp(t.words[0], "hello") == 0 && strcmp(t.words[2], "42") == 0);
    tokens_free(&t);
    tokens_free(&x);
    tokens_free(&y);
    tokens_free(&empty);
}

static void test_scheduling(void) {
    /* Classic CLRS activity set: the optimum is 4 activities. */
    int s[] = {1, 3, 0, 5, 3, 5, 6, 8, 8, 2, 12};
    int f[] = {4, 5, 6, 7, 9, 9, 10, 11, 12, 14, 16};
    Interval v[11], out[11], bad = {99, 10, 5};
    long long ops = 0;
    int i, t, max_overlap = 0, room_of[11], valid_rooms = 1;
    for (i = 0; i < 11; ++i) { v[i].id = i; v[i].start = s[i]; v[i].end = f[i]; }
    CHECK(activity_selection(v, 11, out, &ops) == 4);
    CHECK(first_come_selection(v, 11, out, &ops) <= 4);
    for (t = 0; t <= 16; ++t) {   /* brute-force maximum overlap = minimum rooms */
        int c = 0;
        for (i = 0; i < 11; ++i) if (s[i] <= t && t < f[i]) ++c;
        if (c > max_overlap) max_overlap = c;
    }
    CHECK(interval_partitioning(v, 11, room_of, &ops) == max_overlap);
    for (i = 0; i < 11; ++i) {   /* no two bookings in the same room overlap */
        int j;
        for (j = i + 1; j < 11; ++j)
            if (room_of[i] == room_of[j] && s[i] < f[j] && s[j] < f[i]) valid_rooms = 0;
    }
    CHECK(valid_rooms);
    CHECK(activity_selection(&bad, 1, out, &ops) == 0);   /* invalid booking ignored */
}

static void test_huffman(void) {
    const char *s = "abracadabra alakazam";
    HuffmanResult h = huffman_compress(s, (int)strlen(s));
    CHECK(h.round_trip_ok);
    CHECK(h.encoded_bits < h.original_bits);
    huffman_result_free(&h);
    h = huffman_compress("aaaa", 4);
    CHECK(h.round_trip_ok && h.encoded_bits == 4);
    huffman_result_free(&h);
    h = huffman_compress("", 0);
    CHECK(h.round_trip_ok);
    huffman_result_free(&h);
}

/* Brute-force maximum matching for tiny graphs. */
static int brute_match(const IntVec *adj, int nl, int u, int used) {
    int best, k;
    if (u == nl) return 0;
    best = brute_match(adj, nl, u + 1, used);
    for (k = 0; k < adj[u].size; ++k) {
        int v = adj[u].data[k];
        if (!(used & (1 << v))) {
            int c = 1 + brute_match(adj, nl, u + 1, used | (1 << v));
            if (c > best) best = c;
        }
    }
    return best;
}

static void test_graphs(void) {
    Rng rng;
    int t;
    rng_seed(&rng, 5);
    for (t = 0; t < 40; ++t) {
        int n = rng_range(&rng, 2, 13), m = rng_range(&rng, 0, 2 * n - 1), e, i, j, k, v, src = 0;
        const long long INF = 1000000000000000LL;
        EdgeList *g = (EdgeList *)xcalloc((size_t)n, sizeof(EdgeList));
        long long fw[13][13];
        PathResult pr;
        IntVec path;
        for (i = 0; i < n; ++i) for (j = 0; j < n; ++j) fw[i][j] = i == j ? 0 : INF;
        for (e = 0; e < m; ++e) {
            int a = rng_range(&rng, 0, n - 1), b = rng_range(&rng, 0, n - 1), w = rng_range(&rng, 0, 9);
            edgelist_push(&g[a], b, w);
            edgelist_push(&g[b], a, w);
            if (w < fw[a][b]) fw[a][b] = fw[b][a] = w;
        }
        for (k = 0; k < n; ++k)   /* Floyd-Warshall oracle */
            for (i = 0; i < n; ++i)
                for (j = 0; j < n; ++j)
                    if (fw[i][k] + fw[k][j] < fw[i][j]) fw[i][j] = fw[i][k] + fw[k][j];
        pr = dijkstra(g, n, &src, 1);
        intvec_init(&path);
        for (v = 0; v < n; ++v) {
            long long d = pr.dist[v] >= INF_DIST ? INF : pr.dist[v];
            CHECK(d == fw[0][v]);
            extract_path(&pr, v, &path);
            CHECK(fw[0][v] >= INF ? path.size == 0 : (path.size > 0 && path.data[0] == 0 && path.data[path.size - 1] == v));
        }
        intvec_free(&path);
        path_result_free(&pr);
        for (i = 0; i < n; ++i) free(g[i].e);
        free(g);
    }
    for (t = 0; t < 60; ++t) {   /* matching vs brute force */
        int nl = rng_range(&rng, 1, 5), nr = rng_range(&rng, 1, 5), u, v, valid = 1, seen[5] = {0};
        IntVec adj[5];
        MatchingResult mr;
        for (u = 0; u < nl; ++u) {
            intvec_init(&adj[u]);
            for (v = 0; v < nr; ++v) if (rng_range(&rng, 0, 2) == 0) intvec_push(&adj[u], v);
        }
        mr = max_bipartite_matching(nl, nr, adj);
        CHECK(mr.size == brute_match(adj, nl, 0, 0));
        for (u = 0; u < nl; ++u)
            if (mr.match_of_left[u] >= 0 && seen[mr.match_of_left[u]]++) valid = 0;
        CHECK(valid);
        matching_result_free(&mr);
        for (u = 0; u < nl; ++u) intvec_free(&adj[u]);
    }
}

static void test_datagen_and_engine(void) {
    Library a, b;
    SearchEngine eng;
    IntVec hits, p1, p2;
    MatchStats st;
    Recommendation recs[10];
    ScoredBook sim[3];
    long long ops = 0, cells = 0;
    int i, k, ids_ok = 1, weights_ok = 1, all_have = 1, not_read = 1, sorted = 1, n;
    library_init(&a);
    library_init(&b);
    generate_library(&a, 300, 10, 42);
    generate_library(&b, 300, 10, 42);
    CHECK(a.num_books == 300 && a.num_users == 10);
    CHECK(strcmp(a.books[123].title, b.books[123].title) == 0);   /* deterministic */
    for (i = 0; i < a.num_books; ++i) {
        if (a.books[i].id != i) ids_ok = 0;
        for (k = 0; k < a.citations[i].size; ++k) if (a.citations[i].e[k].weight < 1) weights_ok = 0;
    }
    CHECK(ids_ok && weights_ok);

    engine_build(&eng, &a);
    for (i = 1; i < eng.num_terms; ++i) CHECK(strcmp(eng.terms[i - 1].word, eng.terms[i].word) < 0);
    intvec_init(&hits);
    engine_keyword_search(&eng, "graphs", &hits, &ops);
    for (i = 0; i < hits.size; ++i) {
        const Book *bk = &a.books[hits.data[i]];
        int has = strstr(bk->title, "Graphs") != NULL;
        for (k = 0; k < bk->num_keywords; ++k) has = has || strcmp(bk->keywords[k], "graphs") == 0;
        all_have = all_have && has;
    }
    CHECK(hits.size > 0 && all_have);
    CHECK(engine_keyword_search(&eng, "nonexistentword", &hits, &ops) == 0);
    CHECK(engine_keyword_search(&eng, "   ", &hits, &ops) == 0);

    intvec_init(&p1);
    intvec_init(&p2);
    match_stats_init(&st);
    engine_phrase_search(&eng, "Shortest Path", MATCH_NAIVE, &p1, &st);
    engine_phrase_search(&eng, "shortest path", MATCH_KMP, &p2, &st);   CHECK(intvec_equal(&p1, &p2));
    engine_phrase_search(&eng, "shortest path", MATCH_RABIN_KARP, &p2, &st); CHECK(intvec_equal(&p1, &p2));
    engine_phrase_search(&eng, "shortest path", MATCH_AUTOMATON, &p2, &st);  CHECK(intvec_equal(&p1, &p2));
    CHECK(engine_phrase_search(&eng, "", MATCH_KMP, &p2, &st) == 0);

    n = engine_recommend(&eng, 0, 10, recs);
    CHECK(n == 10);
    for (i = 0; i < n; ++i) {
        for (k = 0; k < a.users[0].history.size; ++k) if (a.users[0].history.data[k] == recs[i].book_id) not_read = 0;
        if (i && recs[i].score > recs[i - 1].score) sorted = 0;
    }
    CHECK(not_read && sorted);
    CHECK(engine_recommend(&eng, -1, 5, recs) == 0 && engine_recommend(&eng, 999, 5, recs) == 0);
    CHECK(engine_similar_books(&eng, 0, 3, sim, &cells) == 3);
    CHECK(engine_similar_books(&eng, 5000, 3, sim, &cells) == 0);

    intvec_free(&hits);
    intvec_free(&p1);
    intvec_free(&p2);
    engine_free(&eng);
    library_free(&a);
    library_free(&b);
}

static void test_persistence(void) {
    Library a, b, c;
    char err[256];
    int skipped = -1, i;
    long long ea = 0, eb = 0;
    library_init(&a);
    library_init(&b);
    library_init(&c);
    generate_library(&a, 120, 6, 9);
    ensure_directory("results");
    CHECK(save_library(&a, "results/test_roundtrip", err, sizeof(err)));
    CHECK(load_library(&b, "results/test_roundtrip", err, sizeof(err), &skipped));
    CHECK(skipped == 0 && b.num_books == a.num_books && b.num_users == a.num_users);
    CHECK(strcmp(b.books[77].title, a.books[77].title) == 0 && b.books[77].num_keywords == a.books[77].num_keywords);
    CHECK(strcmp(b.books[77].abstract_text, a.books[77].abstract_text) == 0);
    CHECK(intvec_equal(&b.users[3].history, &a.users[3].history));
    for (i = 0; i < a.num_books; ++i) { ea += a.citations[i].size; eb += b.citations[i].size; }
    CHECK(ea == eb);
    CHECK(!load_library(&c, "results/does_not_exist", err, sizeof(err), &skipped));
    CHECK(c.num_books == 0);
    remove("results/test_roundtrip_books.csv");
    remove("results/test_roundtrip_citations.csv");
    remove("results/test_roundtrip_users.csv");
    library_free(&a);
    library_free(&b);
}

int main(void) {
    struct { const char *name; void (*fn)(void); } tests[] = {
        {"sorting", test_sorting},           {"searching", test_searching},
        {"min-heap", test_heap},             {"max subarray / pow_mod", test_max_subarray_and_pow},
        {"string matching", test_strings},   {"knapsack", test_knapsack},
        {"LCS", test_lcs},                   {"scheduling", test_scheduling},
        {"Huffman", test_huffman},           {"graphs", test_graphs},
        {"datagen + engine", test_datagen_and_engine}, {"persistence", test_persistence}};
    size_t i;
    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        int before = g_fail;
        tests[i].fn();
        printf("[%s] %s\n", g_fail == before ? " OK " : "FAIL", tests[i].name);
    }
    printf("\n%d checks passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
