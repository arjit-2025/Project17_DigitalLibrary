// bench.cpp - Reproducible experiments comparing alternative algorithms on the same input.
#include "bench.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>

#include "backtrack.h"
#include "datagen.h"
#include "divide_conquer.h"
#include "dp.h"
#include "graph.h"
#include "greedy.h"
#include "library_engine.h"
#include "string_match.h"
#include "timer.h"

std::string fmtNum(double x, int precision) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(precision) << x;
    return o.str();
}

BenchRow makeRow(const std::string& experiment, const std::string& dataset, const std::string& algorithm,
                 long long n, double ms, long long ops, long long auxBytes, const std::string& note) {
    BenchRow r;
    r.experiment = experiment;
    r.dataset = dataset;
    r.algorithm = algorithm;
    r.n = n;
    r.ms = ms;
    r.ops = ops;
    r.auxBytes = auxBytes;
    r.note = note;
    return r;
}

namespace {

std::string sizeName(size_t n) {
    if (n <= 1000) return "small";
    if (n <= 10000) return "medium";
    return "large";
}

void benchSorting(const Library& lib, std::vector<BenchRow>& rows) {
    const long long n = (long long)lib.books.size();
    const std::string ds = sizeName(lib.books.size()) + " random";
    auto byRating = [](const Book& a, const Book& b) { return a.rating < b.rating; };
    {
        std::vector<Book> v = lib.books;
        SortStats st;
        Timer t;
        mergeSort(v, byRating, st);
        rows.push_back(makeRow("Sort by rating", ds, "Merge sort", n, t.elapsedMs(), st.comparisons,
                               n * (long long)sizeof(Book), "depth=" + std::to_string(st.maxDepth)));
    }
    struct P { PivotRule rule; const char* name; };
    P rules[] = {{PivotRule::Random, "Quick sort (random pivot)"},
                 {PivotRule::MedianOfThree, "Quick sort (median of 3)"},
                 {PivotRule::Last, "Quick sort (last pivot)"}};
    for (size_t k = 0; k < 3; ++k) {
        std::vector<Book> v = lib.books;
        SortStats st;
        Timer t;
        quickSort(v, byRating, st, rules[k].rule);
        rows.push_back(makeRow("Sort by rating", ds, rules[k].name, n, t.elapsedMs(), st.comparisons,
                               (long long)st.maxDepth * 64, "depth=" + std::to_string(st.maxDepth)));
    }
}

// Already-sorted input: the worst case for a "last element" pivot.
void benchSortingAdversarial(std::vector<BenchRow>& rows) {
    int sizes[] = {1000, 5000, 20000};
    for (size_t s = 0; s < 3; ++s) {
        std::vector<int> base(sizes[s]);
        for (int i = 0; i < sizes[s]; ++i) base[i] = i;
        auto less = [](int a, int b) { return a < b; };
        const std::string ds = "adversarial (sorted)";
        {
            std::vector<int> v = base; SortStats st; Timer t;
            mergeSort(v, less, st);
            rows.push_back(makeRow("Sort sorted ids", ds, "Merge sort", sizes[s], t.elapsedMs(), st.comparisons,
                                   (long long)sizes[s] * 4, "depth=" + std::to_string(st.maxDepth)));
        }
        {
            std::vector<int> v = base; SortStats st; Timer t;
            quickSort(v, less, st, PivotRule::Random);
            rows.push_back(makeRow("Sort sorted ids", ds, "Quick sort (random pivot)", sizes[s], t.elapsedMs(),
                                   st.comparisons, (long long)st.maxDepth * 64, "depth=" + std::to_string(st.maxDepth)));
        }
        {
            std::vector<int> v = base; SortStats st; Timer t;
            quickSort(v, less, st, PivotRule::Last);
            rows.push_back(makeRow("Sort sorted ids", ds, "Quick sort (last pivot)", sizes[s], t.elapsedMs(),
                                   st.comparisons, 64, "depth=" + std::to_string(st.maxDepth) + " (Theta(n^2))"));
        }
    }
}

void benchSearch(const Library& lib, std::vector<BenchRow>& rows) {
    const int n = (int)lib.books.size();
    const std::string ds = sizeName(lib.books.size());
    std::mt19937 rng(7);
    std::vector<int> queries(2000);
    for (size_t i = 0; i < queries.size(); ++i) {
        int id = std::uniform_int_distribution<int>(0, n - 1)(rng);
        queries[i] = (i % 2 == 0) ? id : id + n;   // half of the lookups are misses
    }
    auto key = [](const Book& b) { return b.id; };
    const char* names[] = {"Linear search", "Binary search (iterative)", "Binary search (recursive)"};
    for (int alg = 0; alg < 3; ++alg) {
        long long probes = 0;
        int found = 0;
        Timer t;
        for (size_t i = 0; i < queries.size(); ++i) {
            int r = alg == 0 ? linearSearch(lib.books, key, queries[i], probes)
                  : alg == 1 ? binarySearchIterative(lib.books, key, queries[i], probes)
                             : binarySearchRecursive(lib.books, key, queries[i], probes);
            if (r >= 0) ++found;
        }
        rows.push_back(makeRow("2000 id lookups", ds, names[alg], n, t.elapsedMs(), probes, alg == 2 ? 20 * 32 : 0,
                               "found=" + std::to_string(found)));
    }
}

void benchStrings(const Library& lib, std::vector<BenchRow>& rows) {
    std::string text;
    for (size_t i = 0; i < lib.books.size(); ++i) {
        text += toLowerCopy(lib.books[i].abstractText);
        text += ' ';
    }
    const std::string pat = "dynamic programming";
    const std::string ds = sizeName(lib.books.size()) + " abstracts";
    Matcher ms[] = {Matcher::Naive, Matcher::RabinKarp, Matcher::Automaton, Matcher::KMP};
    long long aux[] = {0, 0, (long long)(pat.size() + 1) * 256 * 4, (long long)pat.size() * 4};
    for (int k = 0; k < 4; ++k) {
        MatchStats st;
        Timer t;
        std::vector<int> hits = runMatcher(ms[k], text, pat, st);
        rows.push_back(makeRow("Phrase 'dynamic programming'", ds, matcherName(ms[k]), (long long)text.size(),
                               t.elapsedMs(), st.comparisons, aux[k],
                               "matches=" + std::to_string(hits.size()) + " pre=" + std::to_string(st.preprocessing)));
    }
}

void benchStringsAdversarial(std::vector<BenchRow>& rows) {
    std::string text(200000, 'a');
    std::string pat = std::string(49, 'a') + "b";   // matches almost fully at every shift
    const std::string ds = "adversarial a^n";
    Matcher ms[] = {Matcher::Naive, Matcher::RabinKarp, Matcher::Automaton, Matcher::KMP};
    for (int k = 0; k < 4; ++k) {
        MatchStats st; Timer t;
        std::vector<int> hits = runMatcher(ms[k], text, pat, st);
        rows.push_back(makeRow("Pattern a^49 b", ds, matcherName(ms[k]), (long long)text.size(), t.elapsedMs(),
                               st.comparisons, 0, "matches=" + std::to_string(hits.size())));
    }
    // Rabin-Karp with a tiny modulus: many hash collisions -> spurious verifications.
    std::string t2;
    std::mt19937 rng(3);
    for (int i = 0; i < 200000; ++i) t2 += (char)('a' + std::uniform_int_distribution<int>(0, 3)(rng));
    long long mods[] = {101, 1000000007LL};
    for (int k = 0; k < 2; ++k) {
        MatchStats st; Timer t;
        std::vector<int> hits = rabinKarpSearch(t2, "abcdabcdab", st, mods[k]);
        rows.push_back(makeRow("Rabin-Karp modulus", "random abcd text", "Rabin-Karp mod=" + std::to_string(mods[k]),
                               200000, t.elapsedMs(), st.comparisons, 0,
                               "matches=" + std::to_string(hits.size()) + " spurious=" + std::to_string(st.spuriousHits)));
    }
}

void benchKnapsack(const Library& lib, std::vector<BenchRow>& rows) {
    int sizes[] = {20, 40, 100, 1000};
    for (size_t s = 0; s < 4; ++s) {
        int n = sizes[s];
        if (n > (int)lib.books.size()) break;
        std::vector<Item> items;
        long long total = 0;
        for (int i = 0; i < n; ++i) {
            Item it;
            it.id = i;
            it.weight = lib.books[i].readMinutes;
            it.value = lib.books[i].rating * 10 + (i * 7) % 13;
            items.push_back(it);
            total += it.weight;
        }
        int cap = (int)(total / 4);
        const std::string ds = std::to_string(n) + " candidates";
        Timer t;
        KnapsackResult dp = knapsackDP(items, cap);
        double tdp = t.elapsedMs();
        t.reset();
        KnapsackResult gr = greedyKnapsack01(items, cap);
        double tgr = t.elapsedMs();
        t.reset();
        KnapsackResult bb = knapsackBranchAndBound(items, cap, 3000000);
        double tbb = t.elapsedMs();
        std::string capS = " W=" + std::to_string(cap);
        rows.push_back(makeRow("Reading plan (0/1 knapsack)", ds, dp.algorithm, n, tdp, dp.ops, dp.auxBytes,
                               "value=" + fmtNum(dp.value, 1) + " (100%)" + capS));
        rows.push_back(makeRow("Reading plan (0/1 knapsack)", ds, gr.algorithm, n, tgr, gr.ops, gr.auxBytes,
                               "value=" + fmtNum(gr.value, 1) + " (" + fmtNum(100.0 * gr.value / dp.value, 2) + "%)"));
        rows.push_back(makeRow("Reading plan (0/1 knapsack)", ds, bb.algorithm, n, tbb, bb.ops, bb.auxBytes,
                               "value=" + fmtNum(bb.value, 1) + " (" + fmtNum(100.0 * bb.value / dp.value, 2) + "%)" +
                                   (bb.complete ? "" : " node limit hit")));
    }
}

void benchDijkstra(const Library& lib, std::vector<BenchRow>& rows) {
    std::vector<int> src(1, 0);
    long long edges = 0;
    for (size_t i = 0; i < lib.citations.size(); ++i) edges += (long long)lib.citations[i].size();
    Timer t;
    PathResult pr = dijkstra(lib.citations, src);
    rows.push_back(makeRow("Citation distances", sizeName(lib.books.size()) + " graph", "Dijkstra (binary heap)",
                           (long long)lib.books.size(), t.elapsedMs(), pr.relaxations,
                           (long long)lib.books.size() * 12 + edges * 6, "settled=" + std::to_string(pr.settled) +
                           " E=" + std::to_string(edges / 2)));
}

void benchHuffmanAndSimilarity(const Library& lib, std::vector<BenchRow>& rows) {
    std::string text;
    for (size_t i = 0; i < lib.books.size(); ++i) text += lib.books[i].abstractText + " ";
    const std::string ds = sizeName(lib.books.size());
    Timer t;
    HuffmanResult h = huffmanCompress(text);
    rows.push_back(makeRow("Compress abstracts", ds, "Huffman coding", (long long)text.size(), t.elapsedMs(),
                           (long long)text.size(), h.distinctSymbols * 64LL,
                           "size=" + fmtNum(100.0 * h.encodedBits / h.originalBits, 1) + "% roundtrip=" +
                               (h.roundTripOk ? "ok" : "FAIL")));
    SearchEngine eng(lib);
    long long cells = 0;
    t.reset();
    std::vector<ScoredBook> sim = eng.similarBooks(0, 5, cells);
    rows.push_back(makeRow("Similar to book #0", ds, "LCS (2-row DP) + Jaccard", (long long)lib.books.size(),
                           t.elapsedMs(), cells, 2 * 51 * 4, "top=#" + std::to_string(sim.empty() ? -1 : sim[0].bookId)));
}

void benchMatching(std::vector<BenchRow>& rows) {
    int sizes[] = {1000, 10000};
    for (size_t s = 0; s < 2; ++s) {
        int n = sizes[s];
        std::mt19937 rng(11);
        std::vector<std::vector<int>> adj(n);
        for (int u = 0; u < n; ++u)
            for (int k = 0; k < 3; ++k) adj[u].push_back(std::uniform_int_distribution<int>(0, n - 1)(rng));
        Timer t;
        MatchingResult m = maxBipartiteMatching(n, n, adj);
        rows.push_back(makeRow("Readers -> copies", std::to_string(n) + " x " + std::to_string(n),
                               "Bipartite matching (augmenting paths)", n, t.elapsedMs(), m.steps, n * 9LL,
                               "matched=" + std::to_string(m.size)));
    }
}

void benchMaxSubarray(std::vector<BenchRow>& rows) {
    int sizes[] = {100000, 1000000};
    for (size_t s = 0; s < 2; ++s) {
        std::mt19937 rng(5);
        std::vector<long long> a(sizes[s]);
        for (size_t i = 0; i < a.size(); ++i) a[i] = std::uniform_int_distribution<int>(-50, 50)(rng);
        Timer t;
        SubarrayResult d = maxSubarrayDC(a);
        double td = t.elapsedMs();
        t.reset();
        SubarrayResult k = maxSubarrayKadane(a);
        double tk = t.elapsedMs();
        rows.push_back(makeRow("Best borrowing period", "daily changes", "Max subarray (divide & conquer)", sizes[s],
                               td, d.ops, 64 * 20, "sum=" + std::to_string(d.sum)));
        rows.push_back(makeRow("Best borrowing period", "daily changes", "Max subarray (Kadane)", sizes[s], tk, k.ops,
                               0, "sum=" + std::to_string(k.sum)));
    }
}

}  // namespace

std::vector<BenchRow> runBenchmarks(std::ostream& log) {
    std::vector<BenchRow> rows;
    int sizes[] = {1000, 10000, 100000};
    for (size_t s = 0; s < 3; ++s) {
        log << "  generating " << sizes[s] << " books..." << std::endl;
        Library lib;
        generateLibrary(lib, sizes[s], sizes[s] / 50, 2026);
        log << "  sorting / searching / matching / graphs on " << sizes[s] << " books..." << std::endl;
        benchSorting(lib, rows);
        benchSearch(lib, rows);
        benchStrings(lib, rows);
        benchDijkstra(lib, rows);
        if (sizes[s] <= 10000) benchHuffmanAndSimilarity(lib, rows);
        if (sizes[s] == 10000) benchKnapsack(lib, rows);
    }
    log << "  adversarial inputs..." << std::endl;
    benchSortingAdversarial(rows);
    benchStringsAdversarial(rows);
    log << "  matching and max-subarray..." << std::endl;
    benchMatching(rows);
    benchMaxSubarray(rows);
    return rows;
}

void printBenchTable(const std::vector<BenchRow>& rows, std::ostream& out) {
    std::string last;
    for (size_t i = 0; i < rows.size(); ++i) {
        const BenchRow& r = rows[i];
        std::string group = r.experiment + " | " + r.dataset;
        if (group != last) {
            out << "\n  " << group << "\n";
            out << "    " << std::left << std::setw(38) << "Algorithm" << std::right << std::setw(9) << "n"
                << std::setw(12) << "time ms" << std::setw(14) << "ops" << std::setw(11) << "aux KB" << "  note\n";
            last = group;
        }
        out << "    " << std::left << std::setw(38) << r.algorithm << std::right << std::setw(9) << r.n << std::setw(12)
            << fmtNum(r.ms, 3) << std::setw(14) << r.ops << std::setw(11) << fmtNum(r.auxBytes / 1024.0, 1) << "  "
            << r.note << "\n";
    }
}

bool writeBenchCSV(const std::vector<BenchRow>& rows, const std::string& path) {
    std::ofstream f(path.c_str());
    if (!f) return false;
    f << "experiment,dataset,algorithm,n,time_ms,operations,aux_bytes,note\n";
    for (size_t i = 0; i < rows.size(); ++i) {
        const BenchRow& r = rows[i];
        f << r.experiment << ',' << r.dataset << ',' << r.algorithm << ',' << r.n << ',' << fmtNum(r.ms, 4) << ','
          << r.ops << ',' << r.auxBytes << ',' << r.note << '\n';
    }
    return true;
}
