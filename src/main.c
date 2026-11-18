/* main.c - Menu-driven CLI for the Digital Library Search and Recommendation Engine.
 *
 * Usage:
 *   library                       interactive menu
 *   library --scenario 1|2|3|all  demonstration scenario(s)
 *   library --bench               full benchmark, exported to results/benchmark.csv */
#include <ctype.h>
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
#include "io.h"
#include "library_engine.h"
#include "rng.h"
#include "scenarios.h"
#include "string_match.h"
#include "timer.h"

static int g_eof = 0;

/* ---------------- Defensive input helpers ---------------- */

/* Reads one trimmed line into buf. Returns 0 at end of input. */
static int read_line(char *buf, size_t cap) {
    size_t len;
    char *start;
    if (!fgets(buf, (int)cap, stdin)) {
        g_eof = 1;
        return 0;
    }
    len = strlen(buf);
    if (len == cap - 1 && buf[len - 1] != '\n') {   /* discard the rest of an over-long line */
        int c;
        while ((c = getchar()) != EOF && c != '\n') {
        }
    }
    start = buf;
    if ((unsigned char)start[0] == 0xEF && (unsigned char)start[1] == 0xBB && (unsigned char)start[2] == 0xBF)
        start += 3;   /* strip UTF-8 byte-order mark from piped input */
    while (*start && isspace((unsigned char)*start)) ++start;
    len = strlen(start);
    while (len > 0 && isspace((unsigned char)start[len - 1])) start[--len] = '\0';
    memmove(buf, start, len + 1);
    return 1;
}

/* Asks for an integer in [lo, hi]. A default outside the range means "must type a value". */
static long long ask_int(const char *prompt, long long lo, long long hi, long long def) {
    const int has_default = def >= lo && def <= hi;
    char buf[256];
    while (!g_eof) {
        char *end;
        long long v;
        printf("%s [%lld..%lld", prompt, lo, hi);
        if (has_default) printf(", Enter = %lld", def);
        printf("]: ");
        fflush(stdout);
        if (!read_line(buf, sizeof(buf))) return def;
        if (!buf[0]) {
            if (has_default) return def;
            continue;
        }
        v = strtoll(buf, &end, 10);
        if (*end == '\0' && v >= lo && v <= hi) return v;
        printf("  Invalid input - please enter a whole number in range.\n");
    }
    return def;
}

static void ask_string(const char *prompt, const char *def, char *out, size_t cap) {
    printf("%s [Enter = \"%s\"]: ", prompt, def);
    fflush(stdout);
    if (!read_line(out, cap) || !out[0]) str_copy(out, cap, def);
}

/* ---------------- Application state ---------------- */

typedef struct {
    Library lib;
    SearchEngine engine;
    int has_engine;
} App;

static void reindex(App *app) {
    if (app->has_engine) engine_free(&app->engine);
    engine_build(&app->engine, &app->lib);
    app->has_engine = 1;
}

static int has_books(const App *app) {
    if (app->lib.num_books == 0) printf("  The library is empty - generate or load data first.\n");
    return app->lib.num_books > 0;
}

static void print_book(const Library *lib, int id) {
    const Book *b = &lib->books[id];
    printf("#%-6d %-47.47s %d  %.1f*  %3d min", b->id, b->title, b->year, b->rating, b->read_minutes);
}

static void show_summary(const App *app) {
    long long edges = 0;
    int i;
    for (i = 0; i < app->lib.num_books; ++i) edges += app->lib.citations[i].size;
    printf("  Library: %d books, %lld citations, %d readers, %d indexed terms\n", app->lib.num_books, edges / 2,
           app->lib.num_users, app->has_engine ? app->engine.num_terms : 0);
}

/* ---------------- Menu actions ---------------- */

static void menu_generate(App *app) {
    int n = (int)ask_int("Number of books", 1, 200000, 500);
    int u = (int)ask_int("Number of readers", 1, 20000, 20);
    long long seed = ask_int("Random seed", 0, 2000000000, 17);
    Timer t;
    timer_reset(&t);
    if (app->has_engine) { engine_free(&app->engine); app->has_engine = 0; }
    generate_library(&app->lib, n, u, (unsigned long long)seed);
    reindex(app);
    printf("  Generated and indexed in %.1f ms.\n", timer_ms(&t));
    show_summary(app);
}

static void menu_load(App *app) {
    char prefix[512], err[256];
    int skipped = 0;
    Library tmp;
    ask_string("File prefix to load (reads <prefix>_books.csv etc.)", "data/library", prefix, sizeof(prefix));
    library_init(&tmp);
    if (load_library(&tmp, prefix, err, sizeof(err), &skipped)) {
        if (app->has_engine) { engine_free(&app->engine); app->has_engine = 0; }
        library_free(&app->lib);
        app->lib = tmp;
        reindex(app);
        printf("  Loaded. Skipped invalid rows: %d\n", skipped);
        show_summary(app);
    } else {
        printf("  Load failed: %s\n", err);
    }
}

static void menu_save(App *app) {
    char prefix[512], err[256], dir[512];
    char *slash;
    ask_string("File prefix to save", "data/library", prefix, sizeof(prefix));
    str_copy(dir, sizeof(dir), prefix);
    slash = strrchr(dir, '/');
    if (!slash) slash = strrchr(dir, '\\');
    if (slash) { *slash = '\0'; ensure_directory(dir); }
    if (save_library(&app->lib, prefix, err, sizeof(err)))
        printf("  Saved %s_books.csv, _citations.csv, _users.csv\n", prefix);
    else
        printf("  Save failed: %s\n", err);
}

static int g_sort_key = 1;   /* comparison context for the sort menu */

static int cmp_menu(const void *a, const void *b, void *ctx) {
    const Book *x = (const Book *)a, *y = (const Book *)b;
    int key = *(const int *)ctx, c;
    if (key == 1 && x->rating != y->rating) return x->rating > y->rating ? -1 : 1;
    if (key == 2 && x->year != y->year) return x->year > y->year ? -1 : 1;
    if (key == 3 && (c = strcmp(x->title, y->title)) != 0) return c;
    return x->id < y->id ? -1 : (x->id > y->id ? 1 : 0);
}

static void menu_sort(App *app) {
    const char *names[] = {"", "Merge sort", "Quick sort (random)", "Quick sort (median-of-3)", "Quick sort (last pivot)"};
    PivotRule rules[] = {PIVOT_RANDOM, PIVOT_RANDOM, PIVOT_RANDOM, PIVOT_MEDIAN3, PIVOT_LAST};
    Book *v;
    int alg, a, i;
    if (!has_books(app)) return;
    printf("  Sort key: 1) rating (high first)  2) year (new first)  3) title (A-Z)\n");
    g_sort_key = (int)ask_int("Key", 1, 3, 1);
    printf("  Algorithm: 1) merge sort  2) quick sort random pivot  3) quick sort median-of-3\n"
           "             4) quick sort last pivot  5) compare all\n");
    alg = (int)ask_int("Algorithm", 1, 5, 5);
    v = (Book *)xmalloc((size_t)app->lib.num_books * sizeof(Book));
    for (a = 1; a <= 4; ++a) {
        SortStats st = {0, 0};
        Timer t;
        if (alg != 5 && alg != a) continue;
        memcpy(v, app->lib.books, (size_t)app->lib.num_books * sizeof(Book));
        timer_reset(&t);
        if (a == 1) merge_sort(v, (size_t)app->lib.num_books, sizeof(Book), cmp_menu, &g_sort_key, &st);
        else quick_sort(v, (size_t)app->lib.num_books, sizeof(Book), cmp_menu, &g_sort_key, &st, rules[a], 2026);
        printf("  %-26s %10.3f ms %13lld comparisons   depth %d\n", names[a], timer_ms(&t), st.comparisons, st.max_depth);
    }
    printf("  First 10 in order:\n");
    for (i = 0; i < app->lib.num_books && i < 10; ++i) { printf("    "); print_book(&app->lib, v[i].id); printf("\n"); }
    free(v);
}

static int key_book_id(const void *key, const void *elem) {
    int k = *(const int *)key, id = ((const Book *)elem)->id;
    return k < id ? -1 : (k > id ? 1 : 0);
}

static void menu_find(App *app) {
    long long p1 = 0, p2 = 0, p3 = 0;
    int id, r1, r2, r3, k;
    if (!has_books(app)) return;
    id = (int)ask_int("Book id to find", -1000000, 1000000, 0);
    r1 = linear_search(app->lib.books, app->lib.num_books, sizeof(Book), &id, key_book_id, &p1);
    r2 = binary_search_iterative(app->lib.books, app->lib.num_books, sizeof(Book), &id, key_book_id, &p2);
    r3 = binary_search_recursive(app->lib.books, app->lib.num_books, sizeof(Book), &id, key_book_id, &p3);
    printf("  Linear search:             index %d, %lld probes\n", r1, p1);
    printf("  Binary search (iterative): index %d, %lld probes\n", r2, p2);
    printf("  Binary search (recursive): index %d, %lld probes\n", r3, p3);
    if (r2 >= 0) {
        printf("    ");
        print_book(&app->lib, r2);
        printf("\n    Keywords:");
        for (k = 0; k < app->lib.books[r2].num_keywords; ++k) printf(" %s", app->lib.books[r2].keywords[k]);
        printf("\n");
    } else {
        printf("  Not found.\n");
    }
}

static void menu_keyword(App *app) {
    char q[256];
    IntVec hits;
    long long ops = 0;
    Timer t;
    int i;
    if (!has_books(app)) return;
    ask_string("Keywords (all must match)", "graphs algorithms", q, sizeof(q));
    intvec_init(&hits);
    timer_reset(&t);
    engine_keyword_search(&app->engine, q, &hits, &ops);
    printf("  %d books in %.3f ms (%lld ops)\n", hits.size, timer_ms(&t), ops);
    for (i = 0; i < hits.size && i < 10; ++i) { printf("    "); print_book(&app->lib, hits.data[i]); printf("\n"); }
    intvec_free(&hits);
}

static void menu_phrase(App *app) {
    const Matcher ms[] = {MATCH_NAIVE, MATCH_RABIN_KARP, MATCH_AUTOMATON, MATCH_KMP};
    char p[256];
    IntVec hits;
    int m, k, i;
    if (!has_books(app)) return;
    ask_string("Phrase to find in abstracts", "shortest path", p, sizeof(p));
    printf("  Matcher: 1) naive  2) Rabin-Karp  3) finite automaton  4) KMP  5) compare all\n");
    m = (int)ask_int("Matcher", 1, 5, 5);
    intvec_init(&hits);
    for (k = 0; k < 4; ++k) {
        MatchStats st;
        Timer t;
        if (m != 5 && m != k + 1) continue;
        match_stats_init(&st);
        timer_reset(&t);
        engine_phrase_search(&app->engine, p, ms[k], &hits, &st);
        printf("  %-18s %10.3f ms %12lld comparisons  %d books  (preprocessing %lld, spurious %lld)\n",
               matcher_name(ms[k]), timer_ms(&t), st.comparisons, hits.size, st.preprocessing, st.spurious_hits);
    }
    for (i = 0; i < hits.size && i < 5; ++i) { printf("    "); print_book(&app->lib, hits.data[i]); printf("\n"); }
    intvec_free(&hits);
}

static void menu_similar(App *app) {
    int id, k, n, i;
    long long cells = 0;
    ScoredBook *sim;
    Timer t;
    if (!has_books(app)) return;
    id = (int)ask_int("Book id", 0, app->lib.num_books - 1, 0);
    k = (int)ask_int("How many", 1, 50, 5);
    sim = (ScoredBook *)xmalloc((size_t)k * sizeof(ScoredBook));
    timer_reset(&t);
    n = engine_similar_books(&app->engine, id, k, sim, &cells);
    printf("  Target: ");
    print_book(&app->lib, id);
    printf("\n");
    for (i = 0; i < n; ++i) { printf("    %.3f  ", sim[i].score); print_book(&app->lib, sim[i].book_id); printf("\n"); }
    printf("  %lld LCS cells in %.2f ms\n", cells, timer_ms(&t));
    if (n > 0) {
        Tokens a, b;
        LcsResult l;
        tokenize(app->lib.books[id].abstract_text, &a);
        tokenize(app->lib.books[sim[0].book_id].abstract_text, &b);
        l = lcs_tokens(&a, &b);
        printf("  LCS with best match has %d words: ", l.length);
        for (i = 0; i < l.length && i < 15; ++i) printf("%s ", l.sequence[i]);
        printf("%s\n", l.length > 15 ? "..." : "");
        lcs_result_free(&l);
        tokens_free(&a);
        tokens_free(&b);
    }
    free(sim);
}

static void menu_recommend(App *app) {
    int u, k, n, i;
    Recommendation *recs;
    const User *us;
    Timer t;
    double ms;
    FILE *f;
    if (!has_books(app) || app->lib.num_users == 0) return;
    u = (int)ask_int("Reader id", 0, app->lib.num_users - 1, 0);
    k = (int)ask_int("How many", 1, 50, 5);
    recs = (Recommendation *)xmalloc((size_t)k * sizeof(Recommendation));
    timer_reset(&t);
    n = engine_recommend(&app->engine, u, k, recs);
    ms = timer_ms(&t);
    us = &app->lib.users[u];
    printf("  %s interests:", us->name);
    for (i = 0; i < us->num_interests; ++i) printf(" %s", us->interests[i]);
    printf("\n  Already read:");
    for (i = 0; i < us->history.size; ++i) printf(" #%d", us->history.data[i]);
    printf("\n");
    for (i = 0; i < n; ++i) {
        printf("    %.3f  ", recs[i].score);
        print_book(&app->lib, recs[i].book_id);
        printf("  topic %.2f dist %lld\n", recs[i].topic_score, recs[i].citation_distance);
    }
    printf("  Computed in %.2f ms\n", ms);
    ensure_directory("results");
    f = fopen("results/recommendations.csv", "w");
    if (f) {
        fprintf(f, "rank,book_id,title,score,topic,graph,rating,citation_distance\n");
        for (i = 0; i < n; ++i)
            fprintf(f, "%d,%d,%s,%.4f,%.4f,%.4f,%.4f,%lld\n", i + 1, recs[i].book_id,
                    app->lib.books[recs[i].book_id].title, recs[i].score, recs[i].topic_score, recs[i].graph_score,
                    recs[i].rating_score, recs[i].citation_distance);
        fclose(f);
        printf("  Exported to results/recommendations.csv\n");
    }
    free(recs);
}

static void menu_plan(App *app) {
    int u, n, budget, count, i, run_bb, k;
    Item *items;
    KnapsackResult res[3];
    double ts[3], frac;
    long long fops = 0;
    Timer t;
    if (!has_books(app) || app->lib.num_users == 0) return;
    u = (int)ask_int("Reader id", 0, app->lib.num_users - 1, 0);
    n = (int)ask_int("Number of candidate books", 1, 5000, 25);
    budget = (int)ask_int("Time budget in minutes", 0, 100000, 240);
    items = (Item *)xmalloc((size_t)n * sizeof(Item));
    count = engine_reading_candidates(&app->engine, u, n, items);
    run_bb = count <= 2000;
    timer_reset(&t); res[0] = knapsack_dp(items, count, budget); ts[0] = timer_ms(&t);
    timer_reset(&t); res[1] = greedy_knapsack01(items, count, budget); ts[1] = timer_ms(&t);
    if (run_bb) { timer_reset(&t); res[2] = knapsack_branch_and_bound(items, count, budget, 3000000); ts[2] = timer_ms(&t); }
    frac = fractional_knapsack(items, count, budget, &fops);
    for (k = 0; k < (run_bb ? 3 : 2); ++k) {
        printf("  %-34s value %7.1f  minutes %5d  books %3d %10lld ops %9.3f ms", res[k].algorithm, res[k].value,
               res[k].weight, res[k].chosen.size, res[k].ops, ts[k]);
        if (res[0].value > 0) printf("  %.1f%%", 100.0 * res[k].value / res[0].value);
        printf("%s\n", res[k].complete ? "" : " (limit hit)");
    }
    printf("  Fractional upper bound: %.1f\n  Optimal plan (DP):\n", frac);
    for (i = 0; i < res[0].chosen.size; ++i) { printf("    "); print_book(&app->lib, res[0].chosen.data[i]); printf("\n"); }
    for (k = 0; k < (run_bb ? 3 : 2); ++k) knapsack_result_free(&res[k]);
    free(items);
}

static void menu_route(App *app) {
    int a, b, i;
    PathResult pr;
    IntVec path;
    Timer t;
    if (!has_books(app)) return;
    a = (int)ask_int("From book id", 0, app->lib.num_books - 1, 0);
    b = (int)ask_int("To book id", 0, app->lib.num_books - 1, app->lib.num_books - 1);
    timer_reset(&t);
    pr = dijkstra(app->lib.citations, app->lib.num_books, &a, 1);
    intvec_init(&path);
    extract_path(&pr, b, &path);
    if (path.size == 0) {
        printf("  No citation route exists.\n");
    } else {
        printf("  Distance %lld, %d books, %lld relaxations, %.3f ms\n", pr.dist[b], path.size, pr.relaxations, timer_ms(&t));
        for (i = 0; i < path.size; ++i) { printf("    "); print_book(&app->lib, path.data[i]); printf("\n"); }
    }
    intvec_free(&path);
    path_result_free(&pr);
}

static void menu_matching(App *app) {
    int copies, u, b, i, k, served = 0, shown = 0, def;
    IntVec *wants;
    char *taken;
    MatchingResult m;
    Timer t;
    if (!has_books(app) || app->lib.num_users == 0) return;
    def = app->lib.num_books < 20 ? app->lib.num_books : 20;
    copies = (int)ask_int("Number of single-copy titles on the hold shelf (books 0..k-1)", 1, app->lib.num_books, def);
    /* Each reader requests the shelf books that match at least one of their interests. */
    wants = (IntVec *)xmalloc((size_t)app->lib.num_users * sizeof(IntVec));
    for (u = 0; u < app->lib.num_users; ++u) {
        const User *us = &app->lib.users[u];
        intvec_init(&wants[u]);
        for (b = 0; b < copies; ++b) {
            int ok = 0;
            for (i = 0; i < us->num_interests && !ok; ++i)
                for (k = 0; k < app->lib.books[b].num_keywords && !ok; ++k)
                    ok = strcmp(us->interests[i], app->lib.books[b].keywords[k]) == 0;
            if (ok) intvec_push(&wants[u], b);
        }
    }
    taken = (char *)xcalloc((size_t)copies, 1);
    for (u = 0; u < app->lib.num_users; ++u)
        for (k = 0; k < wants[u].size; ++k)
            if (!taken[wants[u].data[k]]) { taken[wants[u].data[k]] = 1; ++served; break; }
    timer_reset(&t);
    m = max_bipartite_matching(app->lib.num_users, copies, wants);
    printf("  First come, first served: %d readers get a book\n", served);
    printf("  Maximum bipartite matching: %d readers get a book (%lld steps, %.3f ms)\n", m.size, m.steps, timer_ms(&t));
    for (u = 0; u < m.n_left && shown < 8; ++u)
        if (m.match_of_left[u] >= 0) {
            printf("    %s -> ", app->lib.users[u].name);
            print_book(&app->lib, m.match_of_left[u]);
            printf("\n");
            ++shown;
        }
    matching_result_free(&m);
    for (u = 0; u < app->lib.num_users; ++u) intvec_free(&wants[u]);
    free(wants);
    free(taken);
}

static void menu_rooms(void) {
    int n = (int)ask_int("Number of booking requests", 1, 100000, 30), i, rooms, nfc, nas;
    long long seed = ask_int("Seed", 0, 2000000000, 30), o1 = 0, o2 = 0, o3 = 0;
    Interval *v = (Interval *)xmalloc((size_t)n * sizeof(Interval));
    Interval *fc = (Interval *)xmalloc((size_t)n * sizeof(Interval));
    Interval *as = (Interval *)xmalloc((size_t)n * sizeof(Interval));
    Rng rng;
    rng_seed(&rng, (unsigned long long)seed);
    for (i = 0; i < n; ++i) {
        v[i].id = i;
        v[i].start = 540 + rng_range(&rng, 0, 480);
        v[i].end = v[i].start + rng_range(&rng, 30, 120);
    }
    rooms = interval_partitioning(v, n, NULL, &o1);
    nfc = first_come_selection(v, n, fc, &o2);
    nas = activity_selection(v, n, as, &o3);
    printf("  Rooms needed for all bookings (interval partitioning): %d\n", rooms);
    printf("  One room, earliest start first:   %d bookings\n", nfc);
    printf("  One room, earliest finish first:  %d bookings (optimal)\n  Accepted:", nas);
    for (i = 0; i < nas && i < 12; ++i)
        printf(" [%d:%02d-%d:%02d]", as[i].start / 60, as[i].start % 60, as[i].end / 60, as[i].end % 60);
    printf("\n");
    free(v);
    free(fc);
    free(as);
}

static void menu_trend(void) {
    int days = (int)ask_int("Number of days", 1, 5000000, 365), i;
    long long seed = ask_int("Seed", 0, 2000000000, 60);
    long long *a = (long long *)xmalloc((size_t)days * sizeof(long long));
    Rng rng;
    Timer t;
    SubarrayResult d, k;
    double td, tk;
    rng_seed(&rng, (unsigned long long)seed);
    for (i = 0; i < days; ++i) a[i] = rng_range(&rng, -20, 20);
    timer_reset(&t); d = max_subarray_dc(a, days); td = timer_ms(&t);
    timer_reset(&t); k = max_subarray_kadane(a, days); tk = timer_ms(&t);
    printf("  Divide & conquer: days %d..%d net %lld  (%lld ops, %.3f ms)\n", d.left + 1, d.right + 1, d.sum, d.ops, td);
    printf("  Kadane:           days %d..%d net %lld  (%lld ops, %.3f ms)\n", k.left + 1, k.right + 1, k.sum, k.ops, tk);
    free(a);
}

static void menu_huffman(App *app) {
    int n, i, shown = 0, c;
    size_t total = 1, pos = 0;
    char *text;
    HuffmanResult h;
    Timer t;
    if (!has_books(app)) return;
    n = (int)ask_int("Compress abstracts of the first N books", 1, app->lib.num_books,
                     app->lib.num_books < 500 ? app->lib.num_books : 500);
    for (i = 0; i < n; ++i) total += strlen(app->lib.books[i].abstract_text) + 1;
    text = (char *)xmalloc(total);
    for (i = 0; i < n; ++i) {
        size_t len = strlen(app->lib.books[i].abstract_text);
        memcpy(text + pos, app->lib.books[i].abstract_text, len);
        pos += len;
        text[pos++] = ' ';
    }
    text[pos] = '\0';
    timer_reset(&t);
    h = huffman_compress(text, (int)pos);
    printf("  %lld bytes -> %lld bytes (%.1f%%), %d symbols, %.2f ms, round trip %s\n  Codes:", h.original_bits / 8,
           (h.encoded_bits + 7) / 8, 100.0 * h.encoded_bits / h.original_bits, h.distinct_symbols, timer_ms(&t),
           h.round_trip_ok ? "ok" : "FAILED");
    for (c = 0; c < 256; ++c) {
        if (!h.codes[c]) continue;
        printf("%s", shown % 6 == 0 ? "\n    " : "  ");
        if (c == ' ') printf("'space'=%s", h.codes[c]);
        else printf("'%c'=%s", c, h.codes[c]);
        ++shown;
    }
    printf("\n");
    huffman_result_free(&h);
    free(text);
}

static void run_bench(void) {
    BenchTable t;
    bench_init(&t);
    printf("Running benchmarks (small 1k, medium 10k, large 100k, adversarial)...\n");
    run_benchmarks(&t);
    print_bench_table(&t);
    ensure_directory("results");
    if (write_bench_csv(&t, "results/benchmark.csv")) printf("\nExported to results/benchmark.csv\n");
    bench_free(&t);
}

static void run_scenario(const char *which) {
    int all = strcmp(which, "all") == 0;
    if (all || strcmp(which, "1") == 0) run_scenario_normal();
    if (all || strcmp(which, "2") == 0) run_scenario_shock();
    if (all || strcmp(which, "3") == 0) {
        ensure_directory("results");
        run_scenario_stress("results/stress_scenario.csv");
    }
}

static void print_help(void) {
    printf("Digital Library Search and Recommendation Engine (DAA Project 17)\n"
           "  library                       interactive menu\n"
           "  library --scenario 1|2|3|all  run demonstration scenario(s)\n"
           "  library --bench               run benchmarks, export results/benchmark.csv\n");
}

int main(int argc, char **argv) {
    App app;
    if (argc >= 2) {
        if (strcmp(argv[1], "--scenario") == 0 && argc >= 3) { run_scenario(argv[2]); return 0; }
        if (strcmp(argv[1], "--bench") == 0) { run_bench(); return 0; }
        print_help();
        return strcmp(argv[1], "--help") == 0 ? 0 : 1;
    }

    library_init(&app.lib);
    app.has_engine = 0;
    generate_library(&app.lib, 500, 20, 17);   /* start with a ready-to-use demo library */
    reindex(&app);

    while (!g_eof) {
        int c;
        printf("\n================ DIGITAL LIBRARY ENGINE (Project 17) ================\n");
        show_summary(&app);
        printf("  DATA        1) Generate dataset      2) Load CSV          3) Save CSV\n"
               "  SEARCH      4) Sort catalogue        5) Find book by id   6) Keyword search\n"
               "              7) Phrase search (string matching)\n"
               "  RECOMMEND   8) Similar books (LCS)   9) Recommend for reader\n"
               "             10) Reading plan under time budget (knapsack)\n"
               "  GRAPHS     11) Citation route (Dijkstra)  12) Assign hold-shelf copies (matching)\n"
               "  OTHER      13) Reading-room bookings  14) Borrowing trend (max subarray)\n"
               "             15) Compress abstracts (Huffman)\n"
               "  DEMO       16) Scenario 1 normal  17) Scenario 2 shock  18) Scenario 3 stress\n"
               "             19) Full benchmark (CSV)\n"
               "              0) Exit\n");
        c = (int)ask_int("Choice", 0, 19, -1);
        if (g_eof || c == 0) break;
        switch (c) {
            case 1: menu_generate(&app); break;
            case 2: menu_load(&app); break;
            case 3: menu_save(&app); break;
            case 4: menu_sort(&app); break;
            case 5: menu_find(&app); break;
            case 6: menu_keyword(&app); break;
            case 7: menu_phrase(&app); break;
            case 8: menu_similar(&app); break;
            case 9: menu_recommend(&app); break;
            case 10: menu_plan(&app); break;
            case 11: menu_route(&app); break;
            case 12: menu_matching(&app); break;
            case 13: menu_rooms(); break;
            case 14: menu_trend(); break;
            case 15: menu_huffman(&app); break;
            case 16: run_scenario("1"); break;
            case 17: run_scenario("2"); break;
            case 18: run_scenario("3"); break;
            case 19: run_bench(); break;
        }
    }
    if (app.has_engine) engine_free(&app.engine);
    library_free(&app.lib);
    return 0;
}
