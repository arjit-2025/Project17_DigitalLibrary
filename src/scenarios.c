/* scenarios.c - Scripted, reproducible demonstrations (fixed seeds). */
#include "scenarios.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backtrack.h"
#include "bench.h"
#include "datagen.h"
#include "divide_conquer.h"
#include "dp.h"
#include "graph.h"
#include "greedy.h"
#include "library_engine.h"
#include "rng.h"
#include "string_match.h"
#include "timer.h"

static void heading(const char *s) {
    printf("\n==============================================================================\n");
    printf("  %s\n", s);
    printf("==============================================================================\n");
}

static void section(const char *s) { printf("\n-- %s\n", s); }

static void row(const char *algo, double ms, long long ops, const char *extra) {
    printf("   %-32s %10.3f ms %12lld ops   %s\n", algo, ms, ops, extra);
}

static void knap_row(const KnapsackResult *r, double ms, double optimum) {
    printf("   %-32s value %7.1f  minutes %4d  books %3d %10lld ops %9.3f ms", r->algorithm, r->value, r->weight,
           r->chosen.size, r->ops, ms);
    if (optimum > 0) printf("  %.1f%% of optimum", 100.0 * r->value / optimum);
    if (!r->complete) printf(" (search limit hit)");
    printf("\n");
}

static void print_title(const Library *lib, int id) {
    printf("#%d %s (%.1f)", id, lib->books[id].title, lib->books[id].rating);
}

static void compare_plans(const Item *items, int n, int budget) {
    Timer t;
    KnapsackResult dp, gr, bb;
    double tdp, tgr, tbb, frac;
    long long fops = 0;
    timer_reset(&t); dp = knapsack_dp(items, n, budget); tdp = timer_ms(&t);
    timer_reset(&t); gr = greedy_knapsack01(items, n, budget); tgr = timer_ms(&t);
    timer_reset(&t); bb = knapsack_branch_and_bound(items, n, budget, 5000000); tbb = timer_ms(&t);
    frac = fractional_knapsack(items, n, budget, &fops);
    knap_row(&dp, tdp, dp.value);
    knap_row(&gr, tgr, dp.value);
    knap_row(&bb, tbb, dp.value);
    printf("   Fractional upper bound (greedy, can split a book): %.1f\n", frac);
    knapsack_result_free(&dp);
    knapsack_result_free(&gr);
    knapsack_result_free(&bb);
}

static int cmp_rating_desc(const void *a, const void *b, void *ctx) {
    const Book *x = (const Book *)a, *y = (const Book *)b;
    (void)ctx;
    if (x->rating != y->rating) return x->rating > y->rating ? -1 : 1;
    return x->id < y->id ? -1 : (x->id > y->id ? 1 : 0);
}

static int cmp_rating_asc(const void *a, const void *b, void *ctx) {
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

static const Matcher ALL_MATCHERS[4] = {MATCH_NAIVE, MATCH_RABIN_KARP, MATCH_AUTOMATON, MATCH_KMP};

/* ============================================================================ */
void run_scenario_normal(void) {
    Library lib;
    SearchEngine eng;
    Timer t;
    Book *m, *q;
    SortStats sm = {0, 0}, sq = {0, 0};
    double tm, tq;
    int i, same = 1, k, first_count = 0, agree = 1;
    char extra[128];

    heading("SCENARIO 1 - NORMAL OPERATION  (500 books, 20 readers, seed 17)");
    library_init(&lib);
    generate_library(&lib, 500, 20, 17);
    timer_reset(&t);
    engine_build(&eng, &lib);
    printf("Inverted index built: %d terms in %.3f ms\n", eng.num_terms, timer_ms(&t));

    section("1. Catalogue ordering by rating (highest first): merge sort vs quick sort");
    m = (Book *)xmalloc((size_t)lib.num_books * sizeof(Book));
    q = (Book *)xmalloc((size_t)lib.num_books * sizeof(Book));
    memcpy(m, lib.books, (size_t)lib.num_books * sizeof(Book));
    memcpy(q, lib.books, (size_t)lib.num_books * sizeof(Book));
    timer_reset(&t); merge_sort(m, (size_t)lib.num_books, sizeof(Book), cmp_rating_desc, NULL, &sm); tm = timer_ms(&t);
    timer_reset(&t); quick_sort(q, (size_t)lib.num_books, sizeof(Book), cmp_rating_desc, NULL, &sq, PIVOT_RANDOM, 2026); tq = timer_ms(&t);
    for (i = 0; i < lib.num_books; ++i) same = same && m[i].id == q[i].id;
    snprintf(extra, sizeof(extra), "recursion depth %d", sm.max_depth);
    row("Merge sort", tm, sm.comparisons, extra);
    snprintf(extra, sizeof(extra), "recursion depth %d", sq.max_depth);
    row("Quick sort (random pivot)", tq, sq.comparisons, extra);
    printf("   Same order: %s.  Top 3:\n", same ? "yes" : "NO");
    for (i = 0; i < 3; ++i) { printf("     "); print_title(&lib, m[i].id); printf("\n"); }
    free(m);
    free(q);

    section("2. Find book #321: binary search vs linear scan (catalogue sorted by id)");
    {
        long long p1 = 0, p2 = 0, p3 = 0;
        int key = 321;
        int r1 = linear_search(lib.books, lib.num_books, sizeof(Book), &key, key_book_id, &p1);
        int r2 = binary_search_iterative(lib.books, lib.num_books, sizeof(Book), &key, key_book_id, &p2);
        int r3 = binary_search_recursive(lib.books, lib.num_books, sizeof(Book), &key, key_book_id, &p3);
        printf("   Linear: index %d after %lld probes | Binary iterative: index %d after %lld probes | "
               "Binary recursive: index %d after %lld probes\n", r1, p1, r2, p2, r3, p3);
    }

    section("3. Keyword search \"graphs algorithms\" (inverted index + sorted-list intersection)");
    {
        IntVec hits;
        long long ops = 0;
        intvec_init(&hits);
        timer_reset(&t);
        engine_keyword_search(&eng, "graphs algorithms", &hits, &ops);
        printf("   %d books in %.3f ms, %lld ops. Best rated:\n", hits.size, timer_ms(&t), ops);
        for (i = 0; i < hits.size && i < 3; ++i) { printf("     "); print_title(&lib, hits.data[i]); printf("\n"); }
        intvec_free(&hits);
    }

    section("4. Phrase search \"shortest path\" in all abstracts: four matchers, same input");
    for (k = 0; k < 4; ++k) {
        IntVec ph;
        MatchStats st;
        double tt;
        intvec_init(&ph);
        match_stats_init(&st);
        timer_reset(&t);
        engine_phrase_search(&eng, "shortest path", ALL_MATCHERS[k], &ph, &st);
        tt = timer_ms(&t);
        if (k == 0) first_count = ph.size;
        else agree = agree && ph.size == first_count;
        snprintf(extra, sizeof(extra), "%d books, preprocessing %lld", ph.size, st.preprocessing);
        row(matcher_name(ALL_MATCHERS[k]), tt, st.comparisons, extra);
        intvec_free(&ph);
    }
    printf("   All matchers agree: %s\n", agree ? "yes" : "NO");

    printf("\n-- 5. Books similar to ");
    print_title(&lib, 0);
    printf("  (LCS of abstracts + keyword overlap)\n");
    {
        ScoredBook sim[3];
        long long cells = 0;
        int ns;
        timer_reset(&t);
        ns = engine_similar_books(&eng, 0, 3, sim, &cells);
        for (i = 0; i < ns; ++i) { printf("     %.3f  ", sim[i].score); print_title(&lib, sim[i].book_id); printf("\n"); }
        printf("   %lld DP cells in %.3f ms\n", cells, timer_ms(&t));
    }

    printf("\n-- 6. Recommendations for %s (interests + citation distance + rating)\n", lib.users[0].name);
    {
        Recommendation recs[5];
        int nr;
        printf("   Interests:");
        for (i = 0; i < lib.users[0].num_interests; ++i) printf(" %s", lib.users[0].interests[i]);
        printf(" | already read %d books\n", lib.users[0].history.size);
        nr = engine_recommend(&eng, 0, 5, recs);
        for (i = 0; i < nr; ++i) {
            printf("     %.3f  ", recs[i].score);
            print_title(&lib, recs[i].book_id);
            printf("  [topic %.2f, citation distance %lld]\n", recs[i].topic_score, recs[i].citation_distance);
        }
    }

    printf("\n-- 7. Reading plan for %s: 25 candidates, 240-minute budget\n", lib.users[0].name);
    {
        Item items[25];
        int n = engine_reading_candidates(&eng, 0, 25, items);
        compare_plans(items, n, 240);
    }

    section("8. Citation route from book #10 to book #400 (Dijkstra)");
    {
        int src = 10;
        PathResult pr = dijkstra(lib.citations, lib.num_books, &src, 1);
        IntVec path;
        intvec_init(&path);
        extract_path(&pr, 400, &path);
        printf("   Distance %lld via %d books:", pr.dist[400], path.size);
        for (i = 0; i < path.size; ++i) printf("%s#%d", i ? " -> " : " ", path.data[i]);
        printf("\n   %lld edge relaxations\n", pr.relaxations);
        intvec_free(&path);
        path_result_free(&pr);
    }

    section("9. Best borrowing growth period over 60 days: divide & conquer vs Kadane");
    {
        long long days[60];
        Rng rng;
        SubarrayResult d, kd;
        rng_seed(&rng, 60);
        for (i = 0; i < 60; ++i) days[i] = rng_range(&rng, -20, 20);
        d = max_subarray_dc(days, 60);
        kd = max_subarray_kadane(days, 60);
        printf("   D&C:    days %d..%d, net +%lld (%lld ops)\n", d.left + 1, d.right + 1, d.sum, d.ops);
        printf("   Kadane: days %d..%d, net +%lld (%lld ops)\n", kd.left + 1, kd.right + 1, kd.sum, kd.ops);
    }

    section("10. Compressing all abstracts for archival (Huffman)");
    {
        size_t total = 1, pos = 0;
        char *all;
        HuffmanResult h;
        for (i = 0; i < lib.num_books; ++i) total += strlen(lib.books[i].abstract_text) + 1;
        all = (char *)xmalloc(total);
        for (i = 0; i < lib.num_books; ++i) {
            size_t len = strlen(lib.books[i].abstract_text);
            memcpy(all + pos, lib.books[i].abstract_text, len);
            pos += len;
            all[pos++] = ' ';
        }
        all[pos] = '\0';
        h = huffman_compress(all, (int)pos);
        printf("   %lld bytes -> %lld bytes (%.1f%%), %d symbols, decode check: %s\n", h.original_bits / 8,
               (h.encoded_bits + 7) / 8, 100.0 * h.encoded_bits / h.original_bits, h.distinct_symbols,
               h.round_trip_ok ? "ok" : "FAILED");
        huffman_result_free(&h);
        free(all);
    }
    engine_free(&eng);
    library_free(&lib);
}

/* ============================================================================ */
void run_scenario_shock(void) {
    Library lib;
    SearchEngine eng;
    Item items[25];
    int n_items, b, i, r;
    char extra[128];

    heading("SCENARIO 2 - CAPACITY / CONSTRAINT SHOCK  (same library, seed 17)");
    library_init(&lib);
    generate_library(&lib, 500, 20, 17);
    engine_build(&eng, &lib);

    section("A. Exam week: reading budget for Reader_0 drops from 240 to 45 minutes");
    n_items = engine_reading_candidates(&eng, 0, 25, items);
    {
        int budgets[] = {240, 90, 45};
        for (b = 0; b < 3; ++b) {
            printf("   Budget %d min:\n", budgets[b]);
            compare_plans(items, n_items, budgets[b]);
        }
    }

    section("A2. Textbook case where greedy is clearly wrong (budget 50 min)");
    {
        Item tiny[3] = {{1, 10, 60}, {2, 20, 100}, {3, 30, 120}};
        printf("   Books: #1 10 min value 60 | #2 20 min value 100 | #3 30 min value 120\n");
        compare_plans(tiny, 3, 50);
        printf("   Greedy takes #1 and #2 (best ratios) = 160; the optimum takes #2 and #3 = 220.\n");
    }

    section("B. Demand surge: 25 readers want 20 single-copy titles (each asks for a popular one first)");
    {
        enum { READERS = 25, COPIES = 20 };
        IntVec wants[READERS];
        char taken[COPIES];
        Rng rng;
        Timer t;
        int served = 0;
        long long fc_ops = 0;
        double tfc, tmm;
        MatchingResult mm;
        rng_seed(&rng, 99);
        for (r = 0; r < READERS; ++r) {
            int extra_wants = rng_range(&rng, 1, 2), target;
            intvec_init(&wants[r]);
            intvec_push(&wants[r], rng_range(&rng, 0, 4));   /* popular titles 0..4 first */
            target = 1 + extra_wants;
            while (wants[r].size < target) {
                int title = rng_range(&rng, 0, COPIES - 1), dup = 0, k;
                for (k = 0; k < wants[r].size; ++k) if (wants[r].data[k] == title) dup = 1;
                if (!dup) intvec_push(&wants[r], title);
            }
        }
        /* Strategy 1: first come, first served - each reader takes their first free choice. */
        timer_reset(&t);
        memset(taken, 0, sizeof(taken));
        for (r = 0; r < READERS; ++r) {
            int k;
            for (k = 0; k < wants[r].size; ++k) {
                fc_ops++;
                if (!taken[wants[r].data[k]]) { taken[wants[r].data[k]] = 1; ++served; break; }
            }
        }
        tfc = timer_ms(&t);
        timer_reset(&t);
        mm = max_bipartite_matching(READERS, COPIES, wants);
        tmm = timer_ms(&t);
        snprintf(extra, sizeof(extra), "%d readers served", served);
        row("First come, first served", tfc, fc_ops, extra);
        snprintf(extra, sizeof(extra), "%d readers served (optimal)", mm.size);
        row("Max bipartite matching", tmm, mm.steps, extra);
        printf("   Upper limit is %d (one copy each). Matching re-routes earlier readers to their\n"
               "   second choice through augmenting paths so that more readers get a book.\n", COPIES);
        matching_result_free(&mm);
        for (r = 0; r < READERS; ++r) intvec_free(&wants[r]);
    }

    section("B2. Smallest example: A wants {X, Y}, B wants {X}");
    {
        IntVec small[2];
        MatchingResult sm;
        intvec_init(&small[0]);
        intvec_init(&small[1]);
        intvec_push(&small[0], 0);
        intvec_push(&small[0], 1);
        intvec_push(&small[1], 0);
        sm = max_bipartite_matching(2, 2, small);
        printf("   First come: A gets X, B gets nothing (1 served). Matching: A gets %s, B gets %s (%d served).\n",
               sm.match_of_left[0] == 1 ? "Y" : "X", sm.match_of_left[1] == 0 ? "X" : "-", sm.size);
        matching_result_free(&sm);
        intvec_free(&small[0]);
        intvec_free(&small[1]);
    }

    section("C. Reading rooms closed for maintenance: 4 rooms -> 1 room, 30 booking requests");
    {
        Interval bookings[30], fc[30], as[30];
        Rng rng;
        Timer t;
        long long op1 = 0, op2 = 0, op3 = 0;
        int rooms, nfc, nas;
        double t1, t2;
        rng_seed(&rng, 30);
        for (i = 0; i < 30; ++i) {
            int start = 9 * 60 + rng_range(&rng, 0, 8 * 60);   /* 09:00 .. 17:00 */
            bookings[i].id = i;
            bookings[i].start = start;
            bookings[i].end = start + rng_range(&rng, 30, 120);
        }
        rooms = interval_partitioning(bookings, 30, NULL, &op1);
        printf("   Rooms needed to serve all 30 bookings (interval partitioning): %d  (%lld ops)\n", rooms, op1);
        timer_reset(&t); nfc = first_come_selection(bookings, 30, fc, &op2); t1 = timer_ms(&t);
        timer_reset(&t); nas = activity_selection(bookings, 30, as, &op3); t2 = timer_ms(&t);
        printf("   With only ONE room left:\n");
        snprintf(extra, sizeof(extra), "%d bookings accepted", nfc);
        row("Earliest start first", t1, op2, extra);
        snprintf(extra, sizeof(extra), "%d bookings accepted (optimal)", nas);
        row("Earliest finish first (greedy)", t2, op3, extra);
    }
    engine_free(&eng);
    library_free(&lib);
}

/* ============================================================================ */
void run_scenario_stress(const char *csv_path) {
    Library lib;
    SearchEngine eng;
    BenchTable rows;
    Timer t;
    long long edges = 0;
    double ms;
    int i, k;
    char extra[128];

    heading("SCENARIO 3 - LARGE-DATA STRESS  (100,000 books, 2,000 readers, seed 2026)");
    bench_init(&rows);
    library_init(&lib);
    timer_reset(&t);
    generate_library(&lib, 100000, 2000, 2026);
    for (i = 0; i < lib.num_books; ++i) edges += lib.citations[i].size;
    printf("Generated %d books, %lld citations in %.1f ms\n", lib.num_books, edges / 2, timer_ms(&t));
    timer_reset(&t);
    engine_build(&eng, &lib);
    ms = timer_ms(&t);
    printf("Inverted index: %d terms in %.1f ms (merge sort of all term/book pairs)\n", eng.num_terms, ms);
    bench_add(&rows, "Build inverted index", "100k", "Merge sort + grouping", 100000, ms, 0, 0, "");

    section("1. Sort 100,000 books by rating");
    {
        Book *v = (Book *)xmalloc((size_t)lib.num_books * sizeof(Book));
        for (k = 0; k < 2; ++k) {
            SortStats st = {0, 0};
            memcpy(v, lib.books, (size_t)lib.num_books * sizeof(Book));
            timer_reset(&t);
            if (k == 0) merge_sort(v, (size_t)lib.num_books, sizeof(Book), cmp_rating_asc, NULL, &st);
            else quick_sort(v, (size_t)lib.num_books, sizeof(Book), cmp_rating_asc, NULL, &st, PIVOT_RANDOM, 2026);
            ms = timer_ms(&t);
            if (k == 0) snprintf(extra, sizeof(extra), "depth %d", st.max_depth);
            else snprintf(extra, sizeof(extra), "depth %d - only 41 distinct ratings, 3-way partition groups equal keys", st.max_depth);
            row(k == 0 ? "Merge sort" : "Quick sort (random, 3-way)", ms, st.comparisons, extra);
            bench_add(&rows, "Sort by rating", "100k", k == 0 ? "Merge sort" : "Quick sort (random)", 100000, ms,
                      st.comparisons, 0, "");
        }
        free(v);
    }

    section("2. Adversarial: 20,000 ids that are already sorted");
    {
        int *v = (int *)xmalloc(20000 * sizeof(int));
        const char *names[] = {"Quick sort (random pivot)", "Quick sort (last pivot)"};
        for (k = 0; k < 2; ++k) {
            SortStats st = {0, 0};
            for (i = 0; i < 20000; ++i) v[i] = i;
            timer_reset(&t);
            quick_sort(v, 20000, sizeof(int), cmp_int, NULL, &st, k == 0 ? PIVOT_RANDOM : PIVOT_LAST, 2026);
            ms = timer_ms(&t);
            snprintf(extra, sizeof(extra), "depth %d", st.max_depth);
            row(names[k], ms, st.comparisons, extra);
            bench_add(&rows, "Sort sorted ids", "20k adversarial", names[k], 20000, ms, st.comparisons, 0, "");
        }
        printf("   Last-element pivot on sorted data splits n into (n-1, 0): T(n) = T(n-1) + n = Theta(n^2).\n");
        free(v);
    }

    section("3. 1,000 id lookups (half are misses)");
    {
        const char *names[] = {"Linear search", "Binary search (iterative)", "Binary search (recursive)"};
        for (k = 0; k < 3; ++k) {
            Rng rng;
            long long probes = 0;
            rng_seed(&rng, 1);
            timer_reset(&t);
            for (i = 0; i < 1000; ++i) {
                int id = rng_range(&rng, 0, 199999);
                if (k == 0) linear_search(lib.books, lib.num_books, sizeof(Book), &id, key_book_id, &probes);
                else if (k == 1) binary_search_iterative(lib.books, lib.num_books, sizeof(Book), &id, key_book_id, &probes);
                else binary_search_recursive(lib.books, lib.num_books, sizeof(Book), &id, key_book_id, &probes);
            }
            ms = timer_ms(&t);
            row(names[k], ms, probes, "");
            bench_add(&rows, "1000 lookups", "100k", names[k], 100000, ms, probes, 0, "");
        }
    }

    section("4. Keyword search \"graphs algorithms\"");
    {
        IntVec hits;
        long long ops = 0;
        intvec_init(&hits);
        timer_reset(&t);
        engine_keyword_search(&eng, "graphs algorithms", &hits, &ops);
        ms = timer_ms(&t);
        snprintf(extra, sizeof(extra), "%d books", hits.size);
        row("Index + intersection + rank", ms, ops, extra);
        bench_add(&rows, "Keyword search", "100k", "Inverted index", 100000, ms, ops, 0, "");
        intvec_free(&hits);
    }

    section("5. Phrase search \"dynamic programming\" across 100,000 abstracts");
    for (k = 0; k < 4; ++k) {
        IntVec hits;
        MatchStats st;
        intvec_init(&hits);
        match_stats_init(&st);
        timer_reset(&t);
        engine_phrase_search(&eng, "dynamic programming", ALL_MATCHERS[k], &hits, &st);
        ms = timer_ms(&t);
        snprintf(extra, sizeof(extra), "%d books", hits.size);
        row(matcher_name(ALL_MATCHERS[k]), ms, st.comparisons, extra);
        bench_add(&rows, "Phrase search", "100k", matcher_name(ALL_MATCHERS[k]), 100000, ms, st.comparisons, 0, "");
        intvec_free(&hits);
    }
    printf("   Note: the automaton rebuilds its 256-column table for every abstract; on many short\n"
           "   texts that preprocessing dominates. KMP preprocessing is only Theta(m).\n");

    section("6. Recommendations for Reader_0 (multi-source Dijkstra over 100k books)");
    {
        Recommendation recs[5];
        int nr;
        timer_reset(&t);
        nr = engine_recommend(&eng, 0, 5, recs);
        ms = timer_ms(&t);
        for (i = 0; i < nr; ++i) { printf("     %.3f  ", recs[i].score); print_title(&lib, recs[i].book_id); printf("\n"); }
        printf("   computed in %.1f ms\n", ms);
        bench_add(&rows, "Recommend top 5", "100k", "Dijkstra + scoring + merge sort", 100000, ms, 0, 0, "");
    }

    section("7. Reading plan: 1,000 candidates, 600-minute budget");
    {
        Item *items = (Item *)xmalloc(1000 * sizeof(Item));
        int n = engine_reading_candidates(&eng, 0, 1000, items);
        KnapsackResult dp, gr, bb;
        double tdp, tgr, tbb;
        timer_reset(&t); dp = knapsack_dp(items, n, 600); tdp = timer_ms(&t);
        timer_reset(&t); gr = greedy_knapsack01(items, n, 600); tgr = timer_ms(&t);
        timer_reset(&t); bb = knapsack_branch_and_bound(items, n, 600, 2000000); tbb = timer_ms(&t);
        knap_row(&dp, tdp, dp.value);
        knap_row(&gr, tgr, dp.value);
        knap_row(&bb, tbb, dp.value);
        bench_add(&rows, "Reading plan", "1000 items", dp.algorithm, 1000, tdp, dp.ops, dp.aux_bytes, "");
        bench_add(&rows, "Reading plan", "1000 items", gr.algorithm, 1000, tgr, gr.ops, gr.aux_bytes, "");
        bench_add(&rows, "Reading plan", "1000 items", bb.algorithm, 1000, tbb, bb.ops, bb.aux_bytes, "");
        knapsack_result_free(&dp);
        knapsack_result_free(&gr);
        knapsack_result_free(&bb);
        free(items);
    }

    if (csv_path) {
        if (write_bench_csv(&rows, csv_path)) printf("\nStress results exported to %s\n", csv_path);
        else printf("\nCould not write %s\n", csv_path);
    }
    bench_free(&rows);
    engine_free(&eng);
    library_free(&lib);
}
