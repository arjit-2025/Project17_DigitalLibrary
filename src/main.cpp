// main.cpp - Menu-driven CLI for the Digital Library Search and Recommendation Engine.
//
// Usage:
//   library                     interactive menu
//   library --scenario 1|2|3|all
//   library --bench             full benchmark, exported to results/benchmark.csv
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <random>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "backtrack.h"
#include "bench.h"
#include "datagen.h"
#include "divide_conquer.h"
#include "dp.h"
#include "graph.h"
#include "greedy.h"
#include "io.h"
#include "library_engine.h"
#include "scenarios.h"
#include "string_match.h"
#include "timer.h"

namespace {

bool g_eof = false;

// ---- Defensive input helpers -------------------------------------------------

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

bool readLine(std::string& s) {
    if (!std::getline(std::cin, s)) {
        g_eof = true;
        return false;
    }
    if (s.compare(0, 3, "\xEF\xBB\xBF") == 0) s.erase(0, 3);   // strip UTF-8 byte-order mark from piped input
    s = trim(s);
    return true;
}

long long askInt(const std::string& prompt, long long lo, long long hi, long long def) {
    const bool hasDefault = def >= lo && def <= hi;   // a default outside the range means "must type a value"
    while (!g_eof) {
        std::cout << prompt << " [" << lo << ".." << hi;
        if (hasDefault) std::cout << ", Enter = " << def;
        std::cout << "]: ";
        std::string s;
        if (!readLine(s)) return def;
        if (s.empty()) {
            if (hasDefault) return def;
            continue;
        }
        char* end = nullptr;
        long long v = std::strtoll(s.c_str(), &end, 10);
        if (*end == '\0' && v >= lo && v <= hi) return v;
        std::cout << "  Invalid input - please enter a whole number in range.\n";
    }
    return def;
}

std::string askString(const std::string& prompt, const std::string& def) {
    std::cout << prompt << " [Enter = \"" << def << "\"]: ";
    std::string s;
    if (!readLine(s) || s.empty()) return def;
    return s;
}

std::string bookLine(const Library& lib, int id) {
    const Book& b = lib.books[id];
    std::ostringstream o;
    o << "#" << std::left << std::setw(6) << b.id << std::setw(48) << b.title.substr(0, 47) << " " << b.year << "  "
      << fmtNum(b.rating, 1) << "*  " << std::setw(3) << b.readMinutes << " min";
    return o.str();
}

// ---- Application state ---------------------------------------------------------

struct App {
    Library lib;
    std::unique_ptr<SearchEngine> engine;

    void reindex() { engine.reset(new SearchEngine(lib)); }
    bool hasBooks() const {
        if (lib.books.empty()) std::cout << "  The library is empty - generate or load data first.\n";
        return !lib.books.empty();
    }
};

void showSummary(const App& app) {
    long long edges = 0;
    for (size_t i = 0; i < app.lib.citations.size(); ++i) edges += (long long)app.lib.citations[i].size();
    std::cout << "  Library: " << app.lib.books.size() << " books, " << edges / 2 << " citations, "
              << app.lib.users.size() << " readers, " << (app.engine ? app.engine->indexedTerms() : 0)
              << " indexed terms\n";
}

void menuGenerate(App& app) {
    int n = (int)askInt("Number of books", 1, 200000, 500);
    int u = (int)askInt("Number of readers", 1, 20000, 20);
    unsigned seed = (unsigned)askInt("Random seed", 0, 2000000000, 17);
    Timer t;
    generateLibrary(app.lib, n, u, seed);
    app.reindex();
    std::cout << "  Generated and indexed in " << fmtNum(t.elapsedMs(), 1) << " ms.\n";
    showSummary(app);
}

void menuLoad(App& app) {
    std::string prefix = askString("File prefix to load (reads <prefix>_books.csv etc.)", "data/library");
    std::string err;
    int skipped = 0;
    if (loadLibrary(app.lib, prefix, err, skipped)) {
        app.reindex();
        std::cout << "  Loaded. Skipped invalid rows: " << skipped << "\n";
        showSummary(app);
    } else {
        std::cout << "  Load failed: " << err << "\n";
    }
}

void menuSave(App& app) {
    std::string prefix = askString("File prefix to save", "data/library");
    std::string err;
    size_t slash = prefix.find_last_of("/\\");
    if (slash != std::string::npos) ensureDirectory(prefix.substr(0, slash));
    if (saveLibrary(app.lib, prefix, err)) std::cout << "  Saved " << prefix << "_books.csv, _citations.csv, _users.csv\n";
    else std::cout << "  Save failed: " << err << "\n";
}

void menuSort(App& app) {
    if (!app.hasBooks()) return;
    std::cout << "  Sort key: 1) rating (high first)  2) year (new first)  3) title (A-Z)\n";
    int key = (int)askInt("Key", 1, 3, 1);
    std::cout << "  Algorithm: 1) merge sort  2) quick sort random pivot  3) quick sort median-of-3\n"
                 "             4) quick sort last pivot  5) compare all\n";
    int alg = (int)askInt("Algorithm", 1, 5, 5);
    auto less = [key](const Book& a, const Book& b) {
        if (key == 1 && a.rating != b.rating) return a.rating > b.rating;
        if (key == 2 && a.year != b.year) return a.year > b.year;
        if (key == 3 && a.title != b.title) return a.title < b.title;
        return a.id < b.id;
    };
    std::vector<Book> result;
    for (int a = 1; a <= 4; ++a) {
        if (alg != 5 && alg != a) continue;
        std::vector<Book> v = app.lib.books;
        SortStats st;
        Timer t;
        const char* name = "Merge sort";
        if (a == 1) mergeSort(v, less, st);
        else if (a == 2) { name = "Quick sort (random)"; quickSort(v, less, st, PivotRule::Random); }
        else if (a == 3) { name = "Quick sort (median-of-3)"; quickSort(v, less, st, PivotRule::MedianOfThree); }
        else { name = "Quick sort (last pivot)"; quickSort(v, less, st, PivotRule::Last); }
        std::cout << "  " << std::left << std::setw(26) << name << std::right << std::setw(10)
                  << fmtNum(t.elapsedMs(), 3) << " ms" << std::setw(14) << st.comparisons << " comparisons   depth "
                  << st.maxDepth << "\n";
        result.swap(v);
    }
    std::cout << "  First 10 in order:\n";
    for (size_t i = 0; i < result.size() && i < 10; ++i) std::cout << "    " << bookLine(app.lib, result[i].id) << "\n";
}

void menuFind(App& app) {
    if (!app.hasBooks()) return;
    int id = (int)askInt("Book id to find", -1000000, 1000000, 0);
    auto key = [](const Book& b) { return b.id; };
    long long p1 = 0, p2 = 0, p3 = 0;
    int r1 = linearSearch(app.lib.books, key, id, p1);
    int r2 = binarySearchIterative(app.lib.books, key, id, p2);
    int r3 = binarySearchRecursive(app.lib.books, key, id, p3);
    std::cout << "  Linear search:             index " << r1 << ", " << p1 << " probes\n"
              << "  Binary search (iterative): index " << r2 << ", " << p2 << " probes\n"
              << "  Binary search (recursive): index " << r3 << ", " << p3 << " probes\n";
    if (r2 >= 0) std::cout << "    " << bookLine(app.lib, r2) << "\n    Keywords:" << [&]() {
        std::string s;
        for (size_t k = 0; k < app.lib.books[r2].keywords.size(); ++k) s += " " + app.lib.books[r2].keywords[k];
        return s;
    }() << "\n";
    else std::cout << "  Not found.\n";
}

void menuKeyword(App& app) {
    if (!app.hasBooks()) return;
    std::string q = askString("Keywords (all must match)", "graphs algorithms");
    long long ops = 0;
    Timer t;
    std::vector<int> hits = app.engine->keywordSearch(q, ops);
    std::cout << "  " << hits.size() << " books in " << fmtNum(t.elapsedMs(), 3) << " ms (" << ops << " ops)\n";
    for (size_t i = 0; i < hits.size() && i < 10; ++i) std::cout << "    " << bookLine(app.lib, hits[i]) << "\n";
}

void menuPhrase(App& app) {
    if (!app.hasBooks()) return;
    std::string p = askString("Phrase to find in abstracts", "shortest path");
    std::cout << "  Matcher: 1) naive  2) Rabin-Karp  3) finite automaton  4) KMP  5) compare all\n";
    int m = (int)askInt("Matcher", 1, 5, 5);
    Matcher ms[] = {Matcher::Naive, Matcher::RabinKarp, Matcher::Automaton, Matcher::KMP};
    std::vector<int> hits;
    for (int k = 0; k < 4; ++k) {
        if (m != 5 && m != k + 1) continue;
        MatchStats st;
        Timer t;
        hits = app.engine->phraseSearch(p, ms[k], st);
        std::cout << "  " << std::left << std::setw(18) << matcherName(ms[k]) << std::right << std::setw(10)
                  << fmtNum(t.elapsedMs(), 3) << " ms" << std::setw(12) << st.comparisons << " comparisons  "
                  << hits.size() << " books  (preprocessing " << st.preprocessing << ", spurious " << st.spuriousHits
                  << ")\n";
    }
    for (size_t i = 0; i < hits.size() && i < 5; ++i) std::cout << "    " << bookLine(app.lib, hits[i]) << "\n";
}

void menuSimilar(App& app) {
    if (!app.hasBooks()) return;
    int id = (int)askInt("Book id", 0, (long long)app.lib.books.size() - 1, 0);
    int k = (int)askInt("How many", 1, 50, 5);
    long long cells = 0;
    Timer t;
    std::vector<ScoredBook> sim = app.engine->similarBooks(id, k, cells);
    std::cout << "  Target: " << bookLine(app.lib, id) << "\n";
    for (size_t i = 0; i < sim.size(); ++i)
        std::cout << "    " << fmtNum(sim[i].score, 3) << "  " << bookLine(app.lib, sim[i].bookId) << "\n";
    std::cout << "  " << cells << " LCS cells in " << fmtNum(t.elapsedMs(), 2) << " ms\n";
    if (!sim.empty()) {
        LcsResult l = lcsTokens(tokenize(app.lib.books[id].abstractText),
                                tokenize(app.lib.books[sim[0].bookId].abstractText));
        std::cout << "  LCS with best match has " << l.length << " words: ";
        for (size_t i = 0; i < l.sequence.size() && i < 15; ++i) std::cout << l.sequence[i] << " ";
        std::cout << (l.sequence.size() > 15 ? "..." : "") << "\n";
    }
}

void menuRecommend(App& app) {
    if (!app.hasBooks() || app.lib.users.empty()) return;
    int u = (int)askInt("Reader id", 0, (long long)app.lib.users.size() - 1, 0);
    int k = (int)askInt("How many", 1, 50, 5);
    Timer t;
    std::vector<Recommendation> recs = app.engine->recommend(u, k);
    double ms = t.elapsedMs();
    const User& us = app.lib.users[u];
    std::cout << "  " << us.name << " interests:";
    for (size_t i = 0; i < us.interests.size(); ++i) std::cout << " " << us.interests[i];
    std::cout << "\n  Already read:";
    for (size_t i = 0; i < us.history.size(); ++i) std::cout << " #" << us.history[i];
    std::cout << "\n";
    for (size_t i = 0; i < recs.size(); ++i)
        std::cout << "    " << fmtNum(recs[i].score, 3) << "  " << bookLine(app.lib, recs[i].bookId) << "  topic "
                  << fmtNum(recs[i].topicScore, 2) << " dist " << recs[i].citationDistance << "\n";
    std::cout << "  Computed in " << fmtNum(ms, 2) << " ms\n";

    ensureDirectory("results");
    std::ofstream f("results/recommendations.csv");
    if (f) {
        f << "rank,book_id,title,score,topic,graph,rating,citation_distance\n";
        for (size_t i = 0; i < recs.size(); ++i)
            f << i + 1 << ',' << recs[i].bookId << ',' << app.lib.books[recs[i].bookId].title << ','
              << recs[i].score << ',' << recs[i].topicScore << ',' << recs[i].graphScore << ',' << recs[i].ratingScore
              << ',' << recs[i].citationDistance << '\n';
        std::cout << "  Exported to results/recommendations.csv\n";
    }
}

void menuPlan(App& app) {
    if (!app.hasBooks() || app.lib.users.empty()) return;
    int u = (int)askInt("Reader id", 0, (long long)app.lib.users.size() - 1, 0);
    int n = (int)askInt("Number of candidate books", 1, 5000, 25);
    int budget = (int)askInt("Time budget in minutes", 0, 100000, 240);
    std::vector<Item> items = app.engine->readingCandidates(u, n);
    Timer t;
    KnapsackResult dp = knapsackDP(items, budget);
    double tdp = t.elapsedMs();
    t.reset();
    KnapsackResult gr = greedyKnapsack01(items, budget);
    double tgr = t.elapsedMs();
    KnapsackResult bb;
    double tbb = 0;
    bool runBB = n <= 2000;
    if (runBB) {
        t.reset();
        bb = knapsackBranchAndBound(items, budget, 3000000);
        tbb = t.elapsedMs();
    }
    long long fops = 0;
    double frac = fractionalKnapsack(items, budget, fops);
    const KnapsackResult* rs[] = {&dp, &gr, &bb};
    double ts[] = {tdp, tgr, tbb};
    for (int k = 0; k < (runBB ? 3 : 2); ++k)
        std::cout << "  " << std::left << std::setw(34) << rs[k]->algorithm << std::right << " value "
                  << std::setw(7) << fmtNum(rs[k]->value, 1) << "  minutes " << std::setw(5) << rs[k]->weight
                  << "  books " << std::setw(3) << rs[k]->chosen.size() << std::setw(11) << rs[k]->ops << " ops"
                  << std::setw(10) << fmtNum(ts[k], 3) << " ms  "
                  << (dp.value > 0 ? fmtNum(100.0 * rs[k]->value / dp.value, 1) + "%" : "")
                  << (rs[k]->complete ? "" : " (limit hit)") << "\n";
    std::cout << "  Fractional upper bound: " << fmtNum(frac, 1) << "\n  Optimal plan (DP):\n";
    for (size_t i = 0; i < dp.chosen.size(); ++i) std::cout << "    " << bookLine(app.lib, dp.chosen[i]) << "\n";
}

void menuRoute(App& app) {
    if (!app.hasBooks()) return;
    int a = (int)askInt("From book id", 0, (long long)app.lib.books.size() - 1, 0);
    int b = (int)askInt("To book id", 0, (long long)app.lib.books.size() - 1, (long long)app.lib.books.size() - 1);
    Timer t;
    PathResult pr = dijkstra(app.lib.citations, std::vector<int>(1, a));
    std::vector<int> path = extractPath(pr, b);
    if (path.empty()) {
        std::cout << "  No citation route exists.\n";
        return;
    }
    std::cout << "  Distance " << pr.dist[b] << ", " << path.size() << " books, " << pr.relaxations
              << " relaxations, " << fmtNum(t.elapsedMs(), 3) << " ms\n";
    for (size_t i = 0; i < path.size(); ++i) std::cout << "    " << bookLine(app.lib, path[i]) << "\n";
}

void menuMatching(App& app) {
    if (!app.hasBooks() || app.lib.users.empty()) return;
    int copies = (int)askInt("Number of single-copy titles on the hold shelf (books 0..k-1)", 1,
                             (long long)app.lib.books.size(), std::min<long long>(20, app.lib.books.size()));
    // Each reader requests the shelf books that match at least one of their interests.
    std::vector<std::vector<int>> wants(app.lib.users.size());
    for (size_t u = 0; u < app.lib.users.size(); ++u)
        for (int b = 0; b < copies; ++b) {
            bool ok = false;
            for (size_t i = 0; i < app.lib.users[u].interests.size() && !ok; ++i)
                for (size_t k = 0; k < app.lib.books[b].keywords.size() && !ok; ++k)
                    ok = app.lib.users[u].interests[i] == app.lib.books[b].keywords[k];
            if (ok) wants[u].push_back(b);
        }
    std::vector<char> taken(copies, 0);
    int served = 0;
    for (size_t u = 0; u < wants.size(); ++u)
        for (size_t k = 0; k < wants[u].size(); ++k)
            if (!taken[wants[u][k]]) { taken[wants[u][k]] = 1; ++served; break; }
    Timer t;
    MatchingResult m = maxBipartiteMatching((int)wants.size(), copies, wants);
    std::cout << "  First come, first served: " << served << " readers get a book\n"
              << "  Maximum bipartite matching: " << m.size << " readers get a book (" << m.steps << " steps, "
              << fmtNum(t.elapsedMs(), 3) << " ms)\n";
    int shown = 0;
    for (size_t u = 0; u < m.matchOfLeft.size() && shown < 8; ++u)
        if (m.matchOfLeft[u] >= 0) {
            std::cout << "    " << app.lib.users[u].name << " -> " << bookLine(app.lib, m.matchOfLeft[u]) << "\n";
            ++shown;
        }
}

void menuRooms() {
    int n = (int)askInt("Number of booking requests", 1, 100000, 30);
    unsigned seed = (unsigned)askInt("Seed", 0, 2000000000, 30);
    std::mt19937 rng(seed);
    std::vector<Interval> v;
    for (int i = 0; i < n; ++i) {
        int s = 540 + std::uniform_int_distribution<int>(0, 480)(rng);
        Interval iv = {i, s, s + std::uniform_int_distribution<int>(30, 120)(rng)};
        v.push_back(iv);
    }
    long long o1 = 0, o2 = 0, o3 = 0;
    PartitionResult p = intervalPartitioning(v, o1);
    std::vector<Interval> fc = firstComeSelection(v, o2);
    std::vector<Interval> as = activitySelection(v, o3);
    std::cout << "  Rooms needed for all bookings (interval partitioning): " << p.rooms << "\n"
              << "  One room, earliest start first:   " << fc.size() << " bookings\n"
              << "  One room, earliest finish first:  " << as.size() << " bookings (optimal)\n  Accepted:";
    for (size_t i = 0; i < as.size() && i < 12; ++i) {
        std::cout << " [" << as[i].start / 60 << ":" << std::setw(2) << std::setfill('0') << as[i].start % 60 << "-"
                  << as[i].end / 60 << ":" << std::setw(2) << as[i].end % 60 << std::setfill(' ') << "]";
    }
    std::cout << "\n";
}

void menuTrend() {
    int days = (int)askInt("Number of days", 1, 5000000, 365);
    unsigned seed = (unsigned)askInt("Seed", 0, 2000000000, 60);
    std::mt19937 rng(seed);
    std::vector<long long> a(days);
    for (int i = 0; i < days; ++i) a[i] = std::uniform_int_distribution<int>(-20, 20)(rng);
    Timer t;
    SubarrayResult d = maxSubarrayDC(a);
    double td = t.elapsedMs();
    t.reset();
    SubarrayResult k = maxSubarrayKadane(a);
    double tk = t.elapsedMs();
    std::cout << "  Divide & conquer: days " << d.left + 1 << ".." << d.right + 1 << " net " << d.sum << "  (" << d.ops
              << " ops, " << fmtNum(td, 3) << " ms)\n"
              << "  Kadane:           days " << k.left + 1 << ".." << k.right + 1 << " net " << k.sum << "  (" << k.ops
              << " ops, " << fmtNum(tk, 3) << " ms)\n";
}

void menuHuffman(App& app) {
    if (!app.hasBooks()) return;
    int n = (int)askInt("Compress abstracts of the first N books", 1, (long long)app.lib.books.size(),
                        std::min<long long>(500, app.lib.books.size()));
    std::string text;
    for (int i = 0; i < n; ++i) text += app.lib.books[i].abstractText + " ";
    Timer t;
    HuffmanResult h = huffmanCompress(text);
    std::cout << "  " << h.originalBits / 8 << " bytes -> " << (h.encodedBits + 7) / 8 << " bytes ("
              << fmtNum(100.0 * h.encodedBits / h.originalBits, 1) << "%), " << h.distinctSymbols << " symbols, "
              << fmtNum(t.elapsedMs(), 2) << " ms, round trip " << (h.roundTripOk ? "ok" : "FAILED") << "\n  Codes:";
    for (size_t i = 0; i < h.codes.size(); ++i) {
        char c = (char)h.codes[i].first;
        std::cout << (i % 6 == 0 ? "\n    " : "  ") << "'" << (c == ' ' ? std::string("space") : std::string(1, c))
                  << "'=" << h.codes[i].second;
    }
    std::cout << "\n";
}

void runBench() {
    std::cout << "Running benchmarks (small 1k, medium 10k, large 100k, adversarial)...\n";
    std::vector<BenchRow> rows = runBenchmarks(std::cout);
    printBenchTable(rows, std::cout);
    ensureDirectory("results");
    if (writeBenchCSV(rows, "results/benchmark.csv")) std::cout << "\nExported to results/benchmark.csv\n";
}

void runScenario(const std::string& which) {
    if (which == "1" || which == "all") runScenarioNormal(std::cout);
    if (which == "2" || which == "all") runScenarioShock(std::cout);
    if (which == "3" || which == "all") {
        ensureDirectory("results");
        runScenarioStress(std::cout, "results/stress_scenario.csv");
    }
}

void printHelp() {
    std::cout << "Digital Library Search and Recommendation Engine (DAA Project 17)\n"
                 "  library                      interactive menu\n"
                 "  library --scenario 1|2|3|all  run demonstration scenario(s)\n"
                 "  library --bench              run benchmarks, export results/benchmark.csv\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc >= 2) {
        std::string a = argv[1];
        if (a == "--scenario" && argc >= 3) { runScenario(argv[2]); return 0; }
        if (a == "--bench") { runBench(); return 0; }
        printHelp();
        return a == "--help" ? 0 : 1;
    }

    App app;
    generateLibrary(app.lib, 500, 20, 17);   // start with a ready-to-use demo library
    app.reindex();

    while (!g_eof) {
        std::cout << "\n================ DIGITAL LIBRARY ENGINE (Project 17) ================\n";
        showSummary(app);
        std::cout << "  DATA        1) Generate dataset      2) Load CSV          3) Save CSV\n"
                     "  SEARCH      4) Sort catalogue        5) Find book by id   6) Keyword search\n"
                     "              7) Phrase search (string matching)\n"
                     "  RECOMMEND   8) Similar books (LCS)   9) Recommend for reader\n"
                     "             10) Reading plan under time budget (knapsack)\n"
                     "  GRAPHS     11) Citation route (Dijkstra)  12) Assign hold-shelf copies (matching)\n"
                     "  OTHER      13) Reading-room bookings  14) Borrowing trend (max subarray)\n"
                     "             15) Compress abstracts (Huffman)\n"
                     "  DEMO       16) Scenario 1 normal  17) Scenario 2 shock  18) Scenario 3 stress\n"
                     "             19) Full benchmark (CSV)\n"
                     "              0) Exit\n";
        int c = (int)askInt("Choice", 0, 19, -1);
        if (g_eof) break;
        switch (c) {
            case 0: return 0;
            case 1: menuGenerate(app); break;
            case 2: menuLoad(app); break;
            case 3: menuSave(app); break;
            case 4: menuSort(app); break;
            case 5: menuFind(app); break;
            case 6: menuKeyword(app); break;
            case 7: menuPhrase(app); break;
            case 8: menuSimilar(app); break;
            case 9: menuRecommend(app); break;
            case 10: menuPlan(app); break;
            case 11: menuRoute(app); break;
            case 12: menuMatching(app); break;
            case 13: menuRooms(); break;
            case 14: menuTrend(); break;
            case 15: menuHuffman(app); break;
            case 16: runScenario("1"); break;
            case 17: runScenario("2"); break;
            case 18: runScenario("3"); break;
            case 19: runBench(); break;
        }
    }
    return 0;
}
