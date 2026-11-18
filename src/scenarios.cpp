// scenarios.cpp - Scripted, reproducible demonstrations (fixed seeds).
#include "scenarios.h"

#include <iomanip>
#include <iostream>
#include <random>
#include <set>

#include "backtrack.h"
#include "bench.h"
#include "datagen.h"
#include "divide_conquer.h"
#include "dp.h"
#include "graph.h"
#include "greedy.h"
#include "library_engine.h"
#include "string_match.h"
#include "timer.h"

namespace {

void heading(std::ostream& o, const std::string& s) {
    o << "\n" << std::string(78, '=') << "\n  " << s << "\n" << std::string(78, '=') << "\n";
}
void section(std::ostream& o, const std::string& s) { o << "\n-- " << s << "\n"; }

void row(std::ostream& o, const std::string& algo, double ms, long long ops, const std::string& extra) {
    o << "   " << std::left << std::setw(32) << algo << std::right << std::setw(10) << fmtNum(ms, 3) << " ms"
      << std::setw(13) << ops << " ops   " << extra << "\n";
}

void knapRow(std::ostream& o, const KnapsackResult& r, double ms, double optimum) {
    std::string q = optimum > 0 ? fmtNum(100.0 * r.value / optimum, 1) + "% of optimum" : "";
    o << "   " << std::left << std::setw(32) << r.algorithm << std::right << " value " << std::setw(7)
      << fmtNum(r.value, 1) << "  minutes " << std::setw(4) << r.weight << "  books " << std::setw(3)
      << r.chosen.size() << std::setw(11) << r.ops << " ops " << std::setw(9) << fmtNum(ms, 3) << " ms  " << q
      << (r.complete ? "" : " (search limit hit)") << "\n";
}

std::string title(const Library& lib, int id) {
    return "#" + std::to_string(id) + " " + lib.books[id].title + " (" + fmtNum(lib.books[id].rating, 1) + ")";
}

void comparePlans(std::ostream& out, const std::vector<Item>& items, int budget) {
    Timer t;
    KnapsackResult dp = knapsackDP(items, budget);
    double tdp = t.elapsedMs();
    t.reset();
    KnapsackResult gr = greedyKnapsack01(items, budget);
    double tgr = t.elapsedMs();
    t.reset();
    KnapsackResult bb = knapsackBranchAndBound(items, budget);
    double tbb = t.elapsedMs();
    long long fops = 0;
    double frac = fractionalKnapsack(items, budget, fops);
    knapRow(out, dp, tdp, dp.value);
    knapRow(out, gr, tgr, dp.value);
    knapRow(out, bb, tbb, dp.value);
    out << "   Fractional upper bound (greedy, can split a book): " << fmtNum(frac, 1) << "\n";
}

}  // namespace

// ============================================================================
void runScenarioNormal(std::ostream& out) {
    heading(out, "SCENARIO 1 - NORMAL OPERATION  (500 books, 20 readers, seed 17)");
    Library lib;
    generateLibrary(lib, 500, 20, 17);
    Timer t;
    SearchEngine eng(lib);
    out << "Inverted index built: " << eng.indexedTerms() << " terms in " << fmtNum(t.elapsedMs(), 3) << " ms\n";

    section(out, "1. Catalogue ordering by rating (highest first): merge sort vs quick sort");
    auto byRatingDesc = [](const Book& a, const Book& b) {
        return a.rating != b.rating ? a.rating > b.rating : a.id < b.id;
    };
    std::vector<Book> m = lib.books, q = lib.books;
    SortStats sm, sq;
    t.reset();
    mergeSort(m, byRatingDesc, sm);
    double tm = t.elapsedMs();
    t.reset();
    quickSort(q, byRatingDesc, sq, PivotRule::Random);
    double tq = t.elapsedMs();
    bool same = true;
    for (size_t i = 0; i < m.size(); ++i) same = same && m[i].id == q[i].id;
    row(out, "Merge sort", tm, sm.comparisons, "recursion depth " + std::to_string(sm.maxDepth));
    row(out, "Quick sort (random pivot)", tq, sq.comparisons, "recursion depth " + std::to_string(sq.maxDepth));
    out << "   Same order: " << (same ? "yes" : "NO") << ".  Top 3: \n";
    for (int i = 0; i < 3; ++i) out << "     " << title(lib, m[i].id) << "\n";

    section(out, "2. Find book #321: binary search vs linear scan (catalogue sorted by id)");
    auto key = [](const Book& b) { return b.id; };
    long long p1 = 0, p2 = 0, p3 = 0;
    int r1 = linearSearch(lib.books, key, 321, p1);
    int r2 = binarySearchIterative(lib.books, key, 321, p2);
    int r3 = binarySearchRecursive(lib.books, key, 321, p3);
    out << "   Linear: index " << r1 << " after " << p1 << " probes | Binary iterative: index " << r2 << " after " << p2
        << " probes | Binary recursive: index " << r3 << " after " << p3 << " probes\n";

    section(out, "3. Keyword search \"graphs algorithms\" (inverted index + sorted-list intersection)");
    long long kops = 0;
    t.reset();
    std::vector<int> hits = eng.keywordSearch("graphs algorithms", kops);
    out << "   " << hits.size() << " books in " << fmtNum(t.elapsedMs(), 3) << " ms, " << kops << " ops. Best rated:\n";
    for (size_t i = 0; i < hits.size() && i < 3; ++i) out << "     " << title(lib, hits[i]) << "\n";

    section(out, "4. Phrase search \"shortest path\" in all abstracts: four matchers, same input");
    Matcher ms[] = {Matcher::Naive, Matcher::RabinKarp, Matcher::Automaton, Matcher::KMP};
    size_t firstCount = 0;
    bool agree = true;
    for (int k = 0; k < 4; ++k) {
        MatchStats st;
        t.reset();
        std::vector<int> ph = eng.phraseSearch("shortest path", ms[k], st);
        double tt = t.elapsedMs();
        if (k == 0) firstCount = ph.size();
        else agree = agree && ph.size() == firstCount;
        row(out, matcherName(ms[k]), tt, st.comparisons,
            std::to_string(ph.size()) + " books, preprocessing " + std::to_string(st.preprocessing));
    }
    out << "   All matchers agree: " << (agree ? "yes" : "NO") << "\n";

    section(out, "5. Books similar to " + title(lib, 0) + "  (LCS of abstracts + keyword overlap)");
    long long cells = 0;
    t.reset();
    std::vector<ScoredBook> sim = eng.similarBooks(0, 3, cells);
    for (size_t i = 0; i < sim.size(); ++i)
        out << "     " << fmtNum(sim[i].score, 3) << "  " << title(lib, sim[i].bookId) << "\n";
    out << "   " << cells << " DP cells in " << fmtNum(t.elapsedMs(), 3) << " ms\n";

    section(out, "6. Recommendations for " + lib.users[0].name + " (interests + citation distance + rating)");
    out << "   Interests:";
    for (size_t i = 0; i < lib.users[0].interests.size(); ++i) out << " " << lib.users[0].interests[i];
    out << " | already read " << lib.users[0].history.size() << " books\n";
    std::vector<Recommendation> recs = eng.recommend(0, 5);
    for (size_t i = 0; i < recs.size(); ++i)
        out << "     " << fmtNum(recs[i].score, 3) << "  " << title(lib, recs[i].bookId) << "  [topic "
            << fmtNum(recs[i].topicScore, 2) << ", citation distance " << recs[i].citationDistance << "]\n";

    section(out, "7. Reading plan for " + lib.users[0].name + ": 25 candidates, 240-minute budget");
    std::vector<Item> items = eng.readingCandidates(0, 25);
    comparePlans(out, items, 240);

    section(out, "8. Citation route from book #10 to book #400 (Dijkstra)");
    PathResult pr = dijkstra(lib.citations, std::vector<int>(1, 10));
    std::vector<int> path = extractPath(pr, 400);
    out << "   Distance " << pr.dist[400] << " via " << path.size() << " books:";
    for (size_t i = 0; i < path.size(); ++i) out << (i ? " -> #" : " #") << path[i];
    out << "\n   " << pr.relaxations << " edge relaxations\n";

    section(out, "9. Best borrowing growth period over 60 days: divide & conquer vs Kadane");
    std::mt19937 rng(60);
    std::vector<long long> days(60);
    for (size_t i = 0; i < days.size(); ++i) days[i] = std::uniform_int_distribution<int>(-20, 20)(rng);
    SubarrayResult d = maxSubarrayDC(days), kd = maxSubarrayKadane(days);
    out << "   D&C:    days " << d.left + 1 << ".." << d.right + 1 << ", net +" << d.sum << " (" << d.ops << " ops)\n";
    out << "   Kadane: days " << kd.left + 1 << ".." << kd.right + 1 << ", net +" << kd.sum << " (" << kd.ops << " ops)\n";

    section(out, "10. Compressing all abstracts for archival (Huffman)");
    std::string all;
    for (size_t i = 0; i < lib.books.size(); ++i) all += lib.books[i].abstractText + " ";
    HuffmanResult h = huffmanCompress(all);
    out << "   " << h.originalBits / 8 << " bytes -> " << (h.encodedBits + 7) / 8 << " bytes ("
        << fmtNum(100.0 * h.encodedBits / h.originalBits, 1) << "%), " << h.distinctSymbols
        << " symbols, decode check: " << (h.roundTripOk ? "ok" : "FAILED") << "\n";
}

// ============================================================================
void runScenarioShock(std::ostream& out) {
    heading(out, "SCENARIO 2 - CAPACITY / CONSTRAINT SHOCK  (same library, seed 17)");
    Library lib;
    generateLibrary(lib, 500, 20, 17);
    SearchEngine eng(lib);

    section(out, "A. Exam week: reading budget for Reader_0 drops from 240 to 45 minutes");
    std::vector<Item> items = eng.readingCandidates(0, 25);
    int budgets[] = {240, 90, 45};
    for (int b = 0; b < 3; ++b) {
        out << "   Budget " << budgets[b] << " min:\n";
        comparePlans(out, items, budgets[b]);
    }

    section(out, "A2. Textbook case where greedy is clearly wrong (budget 50 min)");
    std::vector<Item> tiny;
    Item a = {1, 10, 60}, b = {2, 20, 100}, c = {3, 30, 120};
    tiny.push_back(a); tiny.push_back(b); tiny.push_back(c);
    out << "   Books: #1 10 min value 60 | #2 20 min value 100 | #3 30 min value 120\n";
    comparePlans(out, tiny, 50);
    out << "   Greedy takes #1 and #2 (best ratios) = 160; the optimum takes #2 and #3 = 220.\n";

    section(out, "B. Demand surge: 60 readers want 20 single-copy titles (most ask for a popular one)");
    std::mt19937 rng(99);
    const int readers = 60, copies = 20;
    std::vector<std::vector<int>> wants(readers);
    for (int r = 0; r < readers; ++r) {
        std::set<int> s;
        int pop = std::uniform_int_distribution<int>(0, 4)(rng);   // popular titles 0..4 first
        wants[r].push_back(pop);
        s.insert(pop);
        int extra = std::uniform_int_distribution<int>(1, 2)(rng);
        while ((int)s.size() < 1 + extra) {
            int t = std::uniform_int_distribution<int>(0, copies - 1)(rng);
            if (s.insert(t).second) wants[r].push_back(t);
        }
    }
    // Strategy 1: first come, first served - each reader takes their first free choice.
    Timer t;
    std::vector<char> taken(copies, 0);
    int served = 0;
    long long fcOps = 0;
    for (int r = 0; r < readers; ++r)
        for (size_t k = 0; k < wants[r].size(); ++k) {
            ++fcOps;
            if (!taken[wants[r][k]]) { taken[wants[r][k]] = 1; ++served; break; }
        }
    double tfc = t.elapsedMs();
    t.reset();
    MatchingResult mm = maxBipartiteMatching(readers, copies, wants);
    double tmm = t.elapsedMs();
    row(out, "First come, first served", tfc, fcOps, std::to_string(served) + " readers served");
    row(out, "Max bipartite matching", tmm, mm.steps, std::to_string(mm.size) + " readers served (optimal)");
    out << "   Upper limit is " << copies << " (one copy each). Matching re-routes earlier readers to their\n"
           "   second choice through augmenting paths so that more readers get a book.\n";

    section(out, "B2. Smallest example: A wants {X, Y}, B wants {X}");
    std::vector<std::vector<int>> small(2);
    small[0].push_back(0); small[0].push_back(1);
    small[1].push_back(0);
    MatchingResult sm = maxBipartiteMatching(2, 2, small);
    out << "   First come: A gets X, B gets nothing (1 served). Matching: A gets "
        << (sm.matchOfLeft[0] == 1 ? "Y" : "X") << ", B gets " << (sm.matchOfLeft[1] == 0 ? "X" : "-") << " ("
        << sm.size << " served).\n";

    section(out, "C. Reading rooms closed for maintenance: 4 rooms -> 1 room, 30 booking requests");
    std::vector<Interval> bookings;
    std::mt19937 rng2(30);
    for (int i = 0; i < 30; ++i) {
        int start = 9 * 60 + std::uniform_int_distribution<int>(0, 8 * 60)(rng2);   // 09:00 .. 17:00
        int len = std::uniform_int_distribution<int>(30, 120)(rng2);
        Interval iv = {i, start, start + len};
        bookings.push_back(iv);
    }
    long long op1 = 0, op2 = 0, op3 = 0;
    PartitionResult part = intervalPartitioning(bookings, op1);
    out << "   Rooms needed to serve all 30 bookings (interval partitioning): " << part.rooms << "  (" << op1
        << " ops)\n";
    t.reset();
    std::vector<Interval> fc = firstComeSelection(bookings, op2);
    double t1 = t.elapsedMs();
    t.reset();
    std::vector<Interval> as = activitySelection(bookings, op3);
    double t2 = t.elapsedMs();
    out << "   With only ONE room left:\n";
    row(out, "Earliest start first", t1, op2, std::to_string(fc.size()) + " bookings accepted");
    row(out, "Earliest finish first (greedy)", t2, op3, std::to_string(as.size()) + " bookings accepted (optimal)");
}

// ============================================================================
void runScenarioStress(std::ostream& out, const std::string& csvPath) {
    heading(out, "SCENARIO 3 - LARGE-DATA STRESS  (100,000 books, 2,000 readers, seed 2026)");
    std::vector<BenchRow> rows;
    Timer t;
    Library lib;
    generateLibrary(lib, 100000, 2000, 2026);
    long long edges = 0;
    for (size_t i = 0; i < lib.citations.size(); ++i) edges += (long long)lib.citations[i].size();
    out << "Generated " << lib.books.size() << " books, " << edges / 2 << " citations in " << fmtNum(t.elapsedMs(), 1)
        << " ms\n";
    t.reset();
    SearchEngine eng(lib);
    double tIndex = t.elapsedMs();
    out << "Inverted index: " << eng.indexedTerms() << " terms in " << fmtNum(tIndex, 1) << " ms\n";
    rows.push_back(makeRow("Build inverted index", "100k", "std::map index", 100000, tIndex, 0, 0, ""));

    section(out, "1. Sort 100,000 books by rating");
    auto byRating = [](const Book& a, const Book& b) { return a.rating < b.rating; };
    {
        std::vector<Book> v = lib.books; SortStats st; t.reset();
        mergeSort(v, byRating, st);
        double ms = t.elapsedMs();
        row(out, "Merge sort", ms, st.comparisons, "depth " + std::to_string(st.maxDepth));
        rows.push_back(makeRow("Sort by rating", "100k", "Merge sort", 100000, ms, st.comparisons, 0, ""));
    }
    {
        std::vector<Book> v = lib.books; SortStats st; t.reset();
        quickSort(v, byRating, st, PivotRule::Random);
        double ms = t.elapsedMs();
        row(out, "Quick sort (random, 3-way)", ms, st.comparisons, "depth " + std::to_string(st.maxDepth) +
            " - only 41 distinct ratings, 3-way partition groups equal keys");
        rows.push_back(makeRow("Sort by rating", "100k", "Quick sort (random)", 100000, ms, st.comparisons, 0, ""));
    }

    section(out, "2. Adversarial: 20,000 ids that are already sorted");
    {
        std::vector<int> base(20000);
        for (int i = 0; i < 20000; ++i) base[i] = i;
        auto less = [](int a, int b) { return a < b; };
        PivotRule rules[] = {PivotRule::Random, PivotRule::Last};
        const char* names[] = {"Quick sort (random pivot)", "Quick sort (last pivot)"};
        for (int k = 0; k < 2; ++k) {
            std::vector<int> v = base; SortStats st; t.reset();
            quickSort(v, less, st, rules[k]);
            double ms = t.elapsedMs();
            row(out, names[k], ms, st.comparisons, "depth " + std::to_string(st.maxDepth));
            rows.push_back(makeRow("Sort sorted ids", "20k adversarial", names[k], 20000, ms, st.comparisons, 0, ""));
        }
        out << "   Last-element pivot on sorted data splits n into (n-1, 0): T(n) = T(n-1) + n = Theta(n^2).\n";
    }

    section(out, "3. 1,000 id lookups (half are misses)");
    {
        auto key = [](const Book& b) { return b.id; };
        const char* names[] = {"Linear search", "Binary search (iterative)", "Binary search (recursive)"};
        for (int alg = 0; alg < 3; ++alg) {
            std::mt19937 r2(1);
            long long probes = 0;
            t.reset();
            for (int i = 0; i < 1000; ++i) {
                int id = std::uniform_int_distribution<int>(0, 199999)(r2);
                if (alg == 0) linearSearch(lib.books, key, id, probes);
                else if (alg == 1) binarySearchIterative(lib.books, key, id, probes);
                else binarySearchRecursive(lib.books, key, id, probes);
            }
            double ms = t.elapsedMs();
            row(out, names[alg], ms, probes, "");
            rows.push_back(makeRow("1000 lookups", "100k", names[alg], 100000, ms, probes, 0, ""));
        }
    }

    section(out, "4. Keyword search \"graphs algorithms\"");
    {
        long long ops = 0;
        t.reset();
        std::vector<int> hits = eng.keywordSearch("graphs algorithms", ops);
        double ms = t.elapsedMs();
        row(out, "Index + intersection + rank", ms, ops, std::to_string(hits.size()) + " books");
        rows.push_back(makeRow("Keyword search", "100k", "Inverted index", 100000, ms, ops, 0, ""));
    }

    section(out, "5. Phrase search \"dynamic programming\" across 100,000 abstracts");
    {
        Matcher ms[] = {Matcher::Naive, Matcher::RabinKarp, Matcher::Automaton, Matcher::KMP};
        for (int k = 0; k < 4; ++k) {
            MatchStats st;
            t.reset();
            std::vector<int> hits = eng.phraseSearch("dynamic programming", ms[k], st);
            double tt = t.elapsedMs();
            row(out, matcherName(ms[k]), tt, st.comparisons, std::to_string(hits.size()) + " books");
            rows.push_back(makeRow("Phrase search", "100k", matcherName(ms[k]), 100000, tt, st.comparisons, 0, ""));
        }
        out << "   Note: the automaton rebuilds its 256-column table for every abstract; on many short\n"
               "   texts that preprocessing dominates. KMP preprocessing is only Theta(m).\n";
    }

    section(out, "6. Recommendations for Reader_0 (multi-source Dijkstra over 100k books)");
    {
        t.reset();
        std::vector<Recommendation> recs = eng.recommend(0, 5);
        double ms = t.elapsedMs();
        for (size_t i = 0; i < recs.size(); ++i)
            out << "     " << fmtNum(recs[i].score, 3) << "  " << title(lib, recs[i].bookId) << "\n";
        out << "   computed in " << fmtNum(ms, 1) << " ms\n";
        rows.push_back(makeRow("Recommend top 5", "100k", "Dijkstra + scoring + merge sort", 100000, ms, 0, 0, ""));
    }

    section(out, "7. Reading plan: 1,000 candidates, 600-minute budget");
    {
        std::vector<Item> items = eng.readingCandidates(0, 1000);
        t.reset();
        KnapsackResult dp = knapsackDP(items, 600);
        double tdp = t.elapsedMs();
        t.reset();
        KnapsackResult gr = greedyKnapsack01(items, 600);
        double tgr = t.elapsedMs();
        t.reset();
        KnapsackResult bb = knapsackBranchAndBound(items, 600, 2000000);
        double tbb = t.elapsedMs();
        knapRow(out, dp, tdp, dp.value);
        knapRow(out, gr, tgr, dp.value);
        knapRow(out, bb, tbb, dp.value);
        rows.push_back(makeRow("Reading plan", "1000 items", dp.algorithm, 1000, tdp, dp.ops, dp.auxBytes, ""));
        rows.push_back(makeRow("Reading plan", "1000 items", gr.algorithm, 1000, tgr, gr.ops, gr.auxBytes, ""));
        rows.push_back(makeRow("Reading plan", "1000 items", bb.algorithm, 1000, tbb, bb.ops, bb.auxBytes, ""));
    }

    if (!csvPath.empty()) {
        if (writeBenchCSV(rows, csvPath)) out << "\nStress results exported to " << csvPath << "\n";
        else out << "\nCould not write " << csvPath << "\n";
    }
}
