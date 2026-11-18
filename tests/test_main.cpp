// test_main.cpp - Unit, oracle, edge-case and integration tests (no external framework).
// Oracles: std::sort for sorting, naive matcher for string matching, brute force for
// knapsack and matching, Floyd-Warshall for shortest paths.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "backtrack.h"
#include "datagen.h"
#include "divide_conquer.h"
#include "dp.h"
#include "graph.h"
#include "greedy.h"
#include "io.h"
#include "library_engine.h"
#include "string_match.h"

static int g_pass = 0, g_fail = 0;
#define CHECK(cond)                                                                  \
    do {                                                                             \
        if (cond) ++g_pass;                                                          \
        else { ++g_fail; std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
    } while (0)

static void testSorting() {
    std::mt19937 rng(1);
    std::vector<std::vector<int>> cases;
    cases.push_back(std::vector<int>());
    cases.push_back(std::vector<int>(1, 5));
    cases.push_back(std::vector<int>(50, 7));   // all equal
    std::vector<int> asc(300), desc(300), rnd(1000), dup(1000);
    for (int i = 0; i < 300; ++i) { asc[i] = i; desc[i] = 300 - i; }
    for (int i = 0; i < 1000; ++i) { rnd[i] = (int)(rng() % 100000) - 50000; dup[i] = (int)(rng() % 5); }
    cases.push_back(asc); cases.push_back(desc); cases.push_back(rnd); cases.push_back(dup);

    auto less = [](int a, int b) { return a < b; };
    for (size_t c = 0; c < cases.size(); ++c) {
        std::vector<int> expect = cases[c];
        std::sort(expect.begin(), expect.end());
        std::vector<int> m = cases[c]; SortStats s1; mergeSort(m, less, s1); CHECK(m == expect);
        PivotRule rules[] = {PivotRule::Last, PivotRule::Random, PivotRule::MedianOfThree};
        for (int r = 0; r < 3; ++r) {
            std::vector<int> q = cases[c]; SortStats s2; quickSort(q, less, s2, rules[r]); CHECK(q == expect);
        }
    }
    // Merge sort is stable: equal keys keep their original order.
    std::vector<std::pair<int, int>> v;
    for (int i = 0; i < 200; ++i) v.push_back(std::make_pair((int)(rng() % 4), i));
    SortStats st;
    mergeSort(v, [](const std::pair<int, int>& a, const std::pair<int, int>& b) { return a.first < b.first; }, st);
    bool stable = true;
    for (size_t i = 1; i < v.size(); ++i)
        if (v[i - 1].first == v[i].first && v[i - 1].second > v[i].second) stable = false;
    CHECK(stable);
    // Last pivot on sorted input really is quadratic (~n^2/2 comparisons).
    std::vector<int> s(2000);
    for (int i = 0; i < 2000; ++i) s[i] = i;
    SortStats qs; quickSort(s, less, qs, PivotRule::Last);
    CHECK(qs.comparisons > 1000000LL);
}

static void testSearching() {
    std::vector<int> a;
    for (int i = 0; i < 500; ++i) a.push_back(i * 3);
    auto key = [](int x) { return x; };
    for (int t = -5; t < 1505; ++t) {
        long long p = 0;
        int lin = linearSearch(a, key, t, p);
        int it = binarySearchIterative(a, key, t, p);
        int rc = binarySearchRecursive(a, key, t, p);
        CHECK(lin == it && it == rc);
    }
    std::vector<int> empty;
    long long p = 0;
    CHECK(binarySearchIterative(empty, key, 1, p) == -1);
    CHECK(binarySearchRecursive(empty, key, 1, p) == -1);
    p = 0;
    binarySearchIterative(a, key, 999999, p);
    CHECK(p <= 10);   // ceil(log2(500)) + 1
}

static void testMaxSubarrayAndPow() {
    std::mt19937 rng(2);
    for (int t = 0; t < 200; ++t) {
        int n = 1 + (int)(rng() % 40);
        std::vector<long long> a(n);
        for (int i = 0; i < n; ++i) a[i] = (long long)(rng() % 41) - 20;
        long long brute = a[0];
        for (int i = 0; i < n; ++i) {
            long long s = 0;
            for (int j = i; j < n; ++j) { s += a[j]; brute = std::max(brute, s); }
        }
        SubarrayResult d = maxSubarrayDC(a), k = maxSubarrayKadane(a);
        CHECK(d.sum == brute && k.sum == brute);
        long long s = 0;
        for (int i = d.left; i <= d.right; ++i) s += a[i];
        CHECK(s == d.sum);
    }
    std::vector<long long> neg(3, -4);
    CHECK(maxSubarrayDC(neg).sum == -4 && maxSubarrayKadane(neg).sum == -4);
    CHECK(maxSubarrayDC(std::vector<long long>()).left == -1);

    CHECK(powMod(2, 10, 1000000007LL) == 1024);
    CHECK(powMod(3, 0, 7) == 1);
    CHECK(powMod(5, 3, 1) == 0);
    long long mults = 0;
    powMod(7, 1000000, 1000000007LL, &mults);
    CHECK(mults <= 2 * 20);   // Theta(log e)
    long long slow = 1;
    for (int i = 0; i < 77; ++i) slow = slow * 123 % 1000003;
    CHECK(powMod(123, 77, 1000003) == slow);
}

static void testStrings() {
    std::mt19937 rng(3);
    for (int t = 0; t < 300; ++t) {
        int n = (int)(rng() % 60), m = 1 + (int)(rng() % 5);
        std::string text, pat;
        for (int i = 0; i < n; ++i) text += (char)('a' + rng() % 2);
        for (int i = 0; i < m; ++i) pat += (char)('a' + rng() % 2);
        MatchStats s0, s1, s2, s3, s4;
        std::vector<int> expect = naiveSearch(text, pat, s0);
        CHECK(kmpSearch(text, pat, s1) == expect);
        CHECK(rabinKarpSearch(text, pat, s2) == expect);
        CHECK(automatonSearch(text, pat, s3) == expect);
        CHECK(rabinKarpSearch(text, pat, s4, 3) == expect);   // tiny modulus: collisions verified away
    }
    MatchStats st;
    CHECK(kmpSearch("abc", "", st).empty());
    CHECK(naiveSearch("ab", "abc", st).empty());
    CHECK(automatonSearch("aaaa", "aa", st).size() == 3);   // overlapping matches
    std::vector<int> pi = computePrefixFunction("ababaca");
    int expectPi[] = {0, 0, 1, 2, 3, 0, 1};
    CHECK(pi == std::vector<int>(expectPi, expectPi + 7));
    // KMP never does more than 2n comparisons in the scan.
    MatchStats k;
    kmpSearch(std::string(10000, 'a'), std::string(20, 'a') + "b", k);
    CHECK(k.comparisons <= 20000);
}

static double bruteKnapsack(const std::vector<Item>& items, int cap) {
    int n = (int)items.size();
    double best = 0;
    for (int mask = 0; mask < (1 << n); ++mask) {
        int w = 0;
        double v = 0;
        for (int i = 0; i < n; ++i)
            if (mask & (1 << i)) { w += items[i].weight; v += items[i].value; }
        if (w <= cap) best = std::max(best, v);
    }
    return best;
}

static void testKnapsack() {
    std::mt19937 rng(4);
    for (int t = 0; t < 150; ++t) {
        int n = (int)(rng() % 13);
        std::vector<Item> items;
        int total = 0;
        for (int i = 0; i < n; ++i) {
            Item it = {i, 1 + (int)(rng() % 30), (double)(1 + rng() % 50)};
            items.push_back(it);
            total += it.weight;
        }
        int cap = (int)(rng() % (total + 2));
        double opt = bruteKnapsack(items, cap);
        KnapsackResult dp = knapsackDP(items, cap);
        KnapsackResult bb = knapsackBranchAndBound(items, cap);
        KnapsackResult gr = greedyKnapsack01(items, cap);
        long long ops = 0;
        double frac = fractionalKnapsack(items, cap, ops);
        CHECK(std::abs(dp.value - opt) < 1e-9);
        CHECK(std::abs(bb.value - opt) < 1e-9 && bb.complete);
        CHECK(gr.value <= opt + 1e-9);
        CHECK(frac >= opt - 1e-9);
        CHECK(dp.weight <= cap && bb.weight <= cap && gr.weight <= cap);
        double sum = 0;   // the reconstructed set really has the reported value
        for (size_t i = 0; i < dp.chosen.size(); ++i) sum += items[dp.chosen[i]].value;
        CHECK(std::abs(sum - dp.value) < 1e-9);
    }
    std::vector<Item> classic;
    Item a = {1, 10, 60}, b = {2, 20, 100}, c = {3, 30, 120};
    classic.push_back(a); classic.push_back(b); classic.push_back(c);
    CHECK(knapsackDP(classic, 50).value == 220);
    CHECK(greedyKnapsack01(classic, 50).value == 160);   // greedy is not optimal
    CHECK(knapsackDP(classic, 0).value == 0);
    CHECK(knapsackDP(std::vector<Item>(), 100).chosen.empty());
}

static void testLcs() {
    std::vector<std::string> x, y;
    std::string s1 = "ABCBDAB", s2 = "BDCABA";
    for (size_t i = 0; i < s1.size(); ++i) x.push_back(std::string(1, s1[i]));
    for (size_t i = 0; i < s2.size(); ++i) y.push_back(std::string(1, s2[i]));
    LcsResult r = lcsTokens(x, y);
    CHECK(r.length == 4 && (int)r.sequence.size() == 4);
    CHECK(lcsLength(x, y) == 4);
    CHECK(lcsLength(x, std::vector<std::string>()) == 0);
    CHECK(std::abs(lcsSimilarity(x, x) - 1.0) < 1e-12);
    std::vector<std::string> t = tokenize("Hello, World! 42 times");
    CHECK(t.size() == 4 && t[0] == "hello" && t[2] == "42");
}

static void testScheduling() {
    // Classic CLRS activity set: optimum is 4 activities.
    int s[] = {1, 3, 0, 5, 3, 5, 6, 8, 8, 2, 12};
    int f[] = {4, 5, 6, 7, 9, 9, 10, 11, 12, 14, 16};
    std::vector<Interval> v;
    for (int i = 0; i < 11; ++i) { Interval iv = {i, s[i], f[i]}; v.push_back(iv); }
    long long ops = 0;
    CHECK(activitySelection(v, ops).size() == 4);
    CHECK(firstComeSelection(v, ops).size() <= 4);
    PartitionResult p = intervalPartitioning(v, ops);
    // Brute-force maximum overlap = minimum rooms.
    int maxOverlap = 0;
    for (int t = 0; t <= 16; ++t) {
        int c = 0;
        for (int i = 0; i < 11; ++i) if (s[i] <= t && t < f[i]) ++c;
        maxOverlap = std::max(maxOverlap, c);
    }
    CHECK(p.rooms == maxOverlap);
    Interval bad = {99, 10, 5};   // invalid booking is ignored
    std::vector<Interval> onlyBad(1, bad);
    CHECK(activitySelection(onlyBad, ops).empty());
}

static void testHuffman() {
    HuffmanResult h = huffmanCompress("abracadabra alakazam");
    CHECK(h.roundTripOk);
    CHECK(h.encodedBits < h.originalBits);
    HuffmanResult one = huffmanCompress("aaaa");
    CHECK(one.roundTripOk && one.encodedBits == 4);
    CHECK(huffmanCompress("").roundTripOk);
}

static void testGraphs() {
    std::mt19937 rng(5);
    for (int t = 0; t < 40; ++t) {
        int n = 2 + (int)(rng() % 12);
        std::vector<std::vector<Edge>> g(n);
        const long long INF = (long long)1e15;
        std::vector<std::vector<long long>> fw(n, std::vector<long long>(n, INF));
        for (int i = 0; i < n; ++i) fw[i][i] = 0;
        int m = (int)(rng() % (n * 2));
        for (int e = 0; e < m; ++e) {
            int a = (int)(rng() % n), b = (int)(rng() % n), w = (int)(rng() % 10);
            g[a].push_back(Edge(b, w));
            g[b].push_back(Edge(a, w));
            fw[a][b] = std::min(fw[a][b], (long long)w);
            fw[b][a] = std::min(fw[b][a], (long long)w);
        }
        for (int k = 0; k < n; ++k)   // Floyd-Warshall oracle
            for (int i = 0; i < n; ++i)
                for (int j = 0; j < n; ++j)
                    if (fw[i][k] + fw[k][j] < fw[i][j]) fw[i][j] = fw[i][k] + fw[k][j];
        PathResult pr = dijkstra(g, std::vector<int>(1, 0));
        for (int v = 0; v < n; ++v) {
            long long d = pr.dist[v] >= INF_DIST ? INF : pr.dist[v];
            CHECK(d == fw[0][v]);
            std::vector<int> path = extractPath(pr, v);
            CHECK(fw[0][v] >= INF ? path.empty() : (!path.empty() && path.front() == 0 && path.back() == v));
        }
    }
    // Matching vs brute force on tiny random bipartite graphs.
    for (int t = 0; t < 60; ++t) {
        int nl = 1 + (int)(rng() % 5), nr = 1 + (int)(rng() % 5);
        std::vector<std::vector<int>> adj(nl);
        for (int u = 0; u < nl; ++u)
            for (int v = 0; v < nr; ++v)
                if (rng() % 3 == 0) adj[u].push_back(v);
        int best = 0;   // try every assignment pattern via recursion on bitmask of used right vertices
        std::function<void(int, int, int)> go = [&](int u, int used, int cnt) {
            if (u == nl) { best = std::max(best, cnt); return; }
            go(u + 1, used, cnt);
            for (size_t k = 0; k < adj[u].size(); ++k)
                if (!(used & (1 << adj[u][k]))) go(u + 1, used | (1 << adj[u][k]), cnt + 1);
        };
        go(0, 0, 0);
        MatchingResult mr = maxBipartiteMatching(nl, nr, adj);
        CHECK(mr.size == best);
        std::vector<int> seen(nr, 0);
        bool valid = true;
        for (int u = 0; u < nl; ++u)
            if (mr.matchOfLeft[u] >= 0 && seen[mr.matchOfLeft[u]]++) valid = false;
        CHECK(valid);
    }
}

static void testDatagenAndEngine() {
    Library a, b;
    generateLibrary(a, 300, 10, 42);
    generateLibrary(b, 300, 10, 42);
    CHECK(a.books.size() == 300 && a.users.size() == 10 && a.citations.size() == 300);
    CHECK(a.books[123].title == b.books[123].title);   // deterministic
    bool idsOk = true, weightsOk = true;
    for (size_t i = 0; i < a.books.size(); ++i) {
        if (a.books[i].id != (int)i) idsOk = false;
        for (size_t k = 0; k < a.citations[i].size(); ++k) if (a.citations[i][k].second < 1) weightsOk = false;
    }
    CHECK(idsOk && weightsOk);

    SearchEngine eng(a);
    long long ops = 0;
    std::vector<int> hits = eng.keywordSearch("graphs", ops);
    bool allHave = true;
    for (size_t i = 0; i < hits.size(); ++i) {
        bool has = false;
        for (size_t k = 0; k < a.books[hits[i]].keywords.size(); ++k) has = has || a.books[hits[i]].keywords[k] == "graphs";
        has = has || toLowerCopy(a.books[hits[i]].title).find("graphs") != std::string::npos;
        allHave = allHave && has;
    }
    CHECK(!hits.empty() && allHave);
    CHECK(eng.keywordSearch("nonexistentword", ops).empty());
    CHECK(eng.keywordSearch("   ", ops).empty());

    MatchStats s1, s2, s3, s4;
    std::vector<int> p1 = eng.phraseSearch("Shortest Path", Matcher::Naive, s1);
    CHECK(p1 == eng.phraseSearch("shortest path", Matcher::KMP, s2));
    CHECK(p1 == eng.phraseSearch("shortest path", Matcher::RabinKarp, s3));
    CHECK(p1 == eng.phraseSearch("shortest path", Matcher::Automaton, s4));
    CHECK(eng.phraseSearch("", Matcher::KMP, s4).empty());

    std::vector<Recommendation> recs = eng.recommend(0, 10);
    CHECK(recs.size() == 10);
    bool notRead = true, sorted = true;
    for (size_t i = 0; i < recs.size(); ++i) {
        for (size_t h = 0; h < a.users[0].history.size(); ++h) if (a.users[0].history[h] == recs[i].bookId) notRead = false;
        if (i && recs[i].score > recs[i - 1].score) sorted = false;
    }
    CHECK(notRead && sorted);
    CHECK(eng.recommend(-1, 5).empty() && eng.recommend(999, 5).empty());
    long long cells = 0;
    CHECK(eng.similarBooks(0, 3, cells).size() == 3);
    CHECK(eng.similarBooks(5000, 3, cells).empty());
}

static void testPersistence() {
    Library a;
    generateLibrary(a, 120, 6, 9);
    std::string err;
    ensureDirectory("results");
    CHECK(saveLibrary(a, "results/test_roundtrip", err));
    Library b;
    int skipped = -1;
    CHECK(loadLibrary(b, "results/test_roundtrip", err, skipped));
    CHECK(skipped == 0 && b.books.size() == a.books.size() && b.users.size() == a.users.size());
    CHECK(b.books[77].title == a.books[77].title && b.books[77].keywords == a.books[77].keywords);
    CHECK(b.users[3].history == a.users[3].history);
    long long ea = 0, eb = 0;
    for (size_t i = 0; i < a.citations.size(); ++i) { ea += a.citations[i].size(); eb += b.citations[i].size(); }
    CHECK(ea == eb);
    Library c;
    CHECK(!loadLibrary(c, "results/does_not_exist", err, skipped));
    CHECK(c.books.empty());
    std::remove("results/test_roundtrip_books.csv");
    std::remove("results/test_roundtrip_citations.csv");
    std::remove("results/test_roundtrip_users.csv");
}

int main() {
    struct T { const char* name; void (*fn)(); };
    T tests[] = {{"sorting", testSorting},         {"searching", testSearching},
                 {"max subarray / powMod", testMaxSubarrayAndPow}, {"string matching", testStrings},
                 {"knapsack", testKnapsack},       {"LCS", testLcs},
                 {"scheduling", testScheduling},   {"Huffman", testHuffman},
                 {"graphs", testGraphs},           {"datagen + engine", testDatagenAndEngine},
                 {"persistence", testPersistence}};
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        int before = g_fail;
        tests[i].fn();
        std::printf("[%s] %s\n", g_fail == before ? " OK " : "FAIL", tests[i].name);
    }
    std::printf("\n%d checks passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
