// backtrack.cpp - Branch-and-bound / backtracking solver for the 0/1 reading plan.
#include "backtrack.h"

#include "divide_conquer.h"

namespace {

struct BBSolver {
    std::vector<Item> v;      // items sorted by value/weight, best first
    int capacity = 0;
    long long nodeLimit = 0;
    long long nodes = 0;
    bool stopped = false;
    double bestValue = 0.0;
    std::vector<char> cur, best;

    // Upper bound for the subtree at item i: fill remaining capacity greedily, allowing a fraction.
    double bound(int i, int weight, double value) const {
        double b = value;
        int remaining = capacity - weight;
        for (int k = i; k < (int)v.size() && remaining > 0; ++k) {
            if (v[k].weight <= remaining) {
                remaining -= v[k].weight;
                b += v[k].value;
            } else {
                b += v[k].value * ((double)remaining / v[k].weight);
                remaining = 0;
            }
        }
        return b;
    }

    void dfs(int i, int weight, double value) {
        if (stopped) return;
        if (++nodes > nodeLimit) { stopped = true; return; }
        if (value > bestValue + 1e-12) {   // new incumbent
            bestValue = value;
            best = cur;
        }
        if (i == (int)v.size()) return;
        if (bound(i, weight, value) <= bestValue + 1e-12) return;   // prune: cannot improve

        if (weight + v[i].weight <= capacity) {   // branch 1: take item i
            cur[i] = 1;
            dfs(i + 1, weight + v[i].weight, value + v[i].value);
            cur[i] = 0;
        }
        dfs(i + 1, weight, value);                // branch 2: skip item i
    }
};

double ratio(const Item& it) { return it.weight == 0 ? 1e18 : it.value / it.weight; }

}  // namespace

KnapsackResult knapsackBranchAndBound(const std::vector<Item>& items, int capacity, long long nodeLimit) {
    KnapsackResult r;
    r.algorithm = "Backtracking + bound (B&B)";
    BBSolver s;
    for (size_t i = 0; i < items.size(); ++i)
        if (items[i].weight >= 0 && items[i].value >= 0) s.v.push_back(items[i]);
    SortStats st;
    mergeSort(s.v, [](const Item& a, const Item& b) { return ratio(a) > ratio(b); }, st);

    s.capacity = capacity < 0 ? 0 : capacity;
    s.nodeLimit = nodeLimit;
    s.cur.assign(s.v.size(), 0);
    s.best.assign(s.v.size(), 0);
    s.dfs(0, 0, 0.0);

    for (size_t i = 0; i < s.v.size(); ++i) {
        if (s.best[i]) {
            r.chosen.push_back(s.v[i].id);
            r.weight += s.v[i].weight;
        }
    }
    r.value = s.bestValue;
    r.ops = s.nodes + st.comparisons;
    r.complete = !s.stopped;
    r.auxBytes = (long long)s.v.size() * (2 + (long long)sizeof(Item)) + (long long)s.v.size() * 32;   // vectors + stack
    return r;
}
