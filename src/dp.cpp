// dp.cpp - Dynamic programming modules.
#include "dp.h"

#include <algorithm>
#include <cctype>

KnapsackResult knapsackDP(const std::vector<Item>& items, int capacity) {
    KnapsackResult r;
    r.algorithm = "Dynamic programming (0/1 table)";
    const int W = capacity < 0 ? 0 : capacity;

    std::vector<Item> v;
    for (size_t i = 0; i < items.size(); ++i)
        if (items[i].weight >= 0 && items[i].value >= 0) v.push_back(items[i]);
    const int n = (int)v.size();

    // best[w]: optimal value with capacity w using items 0..i (rolled over i).
    // keep[i][w] = 1 if the optimum for (i, w) takes item i -> used to rebuild the chosen set.
    std::vector<double> best(W + 1, 0.0);
    std::vector<unsigned char> keep((size_t)n * (W + 1), 0);

    for (int i = 0; i < n; ++i) {
        const int wi = v[i].weight;
        const double vi = v[i].value;
        // Iterate w downwards so best[w - wi] still refers to the previous row (each item used once).
        for (int w = W; w >= wi; --w) {
            ++r.ops;
            double take = best[w - wi] + vi;
            if (take > best[w] + 1e-12) {
                best[w] = take;
                keep[(size_t)i * (W + 1) + w] = 1;
            }
        }
    }

    int w = W;
    for (int i = n - 1; i >= 0; --i) {
        if (keep[(size_t)i * (W + 1) + w]) {
            r.chosen.push_back(v[i].id);
            r.weight += v[i].weight;
            w -= v[i].weight;
        }
    }
    std::reverse(r.chosen.begin(), r.chosen.end());
    r.value = best[W];
    r.auxBytes = (long long)(W + 1) * (long long)sizeof(double) + (long long)n * (W + 1);
    return r;
}

std::vector<std::string> tokenize(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = (unsigned char)s[i];
        if (std::isalnum(c)) {
            cur += (char)std::tolower(c);
        } else if (!cur.empty()) {
            out.push_back(cur);
            cur.clear();
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

LcsResult lcsTokens(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    LcsResult r;
    const int n = (int)a.size(), m = (int)b.size();
    // L[i][j] = LCS length of a[0..i-1] and b[0..j-1]
    std::vector<int> L((size_t)(n + 1) * (m + 1), 0);
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            ++r.cells;
            if (a[i - 1] == b[j - 1]) L[(size_t)i * (m + 1) + j] = L[(size_t)(i - 1) * (m + 1) + (j - 1)] + 1;
            else L[(size_t)i * (m + 1) + j] = std::max(L[(size_t)(i - 1) * (m + 1) + j], L[(size_t)i * (m + 1) + (j - 1)]);
        }
    }
    r.length = L[(size_t)n * (m + 1) + m];

    int i = n, j = m;   // walk back through the table to recover one LCS
    while (i > 0 && j > 0) {
        if (a[i - 1] == b[j - 1]) {
            r.sequence.push_back(a[i - 1]);
            --i;
            --j;
        } else if (L[(size_t)(i - 1) * (m + 1) + j] >= L[(size_t)i * (m + 1) + (j - 1)]) {
            --i;
        } else {
            --j;
        }
    }
    std::reverse(r.sequence.begin(), r.sequence.end());
    return r;
}

int lcsLength(const std::vector<std::string>& a, const std::vector<std::string>& b, long long* cells) {
    const std::vector<std::string>& lng = a.size() >= b.size() ? a : b;
    const std::vector<std::string>& sht = a.size() >= b.size() ? b : a;
    const int m = (int)sht.size();
    std::vector<int> prev(m + 1, 0), cur(m + 1, 0);
    for (size_t i = 1; i <= lng.size(); ++i) {
        for (int j = 1; j <= m; ++j) {
            if (cells) ++*cells;
            if (lng[i - 1] == sht[j - 1]) cur[j] = prev[j - 1] + 1;
            else cur[j] = std::max(prev[j], cur[j - 1]);
        }
        prev.swap(cur);
    }
    return prev[m];
}

double lcsSimilarity(const std::vector<std::string>& a, const std::vector<std::string>& b, long long* cells) {
    if (a.empty() && b.empty()) return 0.0;
    return 2.0 * lcsLength(a, b, cells) / (double)(a.size() + b.size());
}
