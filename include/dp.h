// dp.h - Dynamic programming: 0/1 knapsack (reading plan) and LCS (document similarity).
#ifndef DP_H
#define DP_H

#include <string>
#include <vector>

#include "model.h"

// Exact 0/1 knapsack. Theta(n*W) time (pseudo-polynomial), Theta(W + n*W/8..n*W) memory.
// dp[w] = best value using the items seen so far within w minutes.
KnapsackResult knapsackDP(const std::vector<Item>& items, int capacity);

// Lower-case word tokens (letters/digits only).
std::vector<std::string> tokenize(const std::string& s);

struct LcsResult {
    int length = 0;
    std::vector<std::string> sequence;   // one longest common subsequence
    long long cells = 0;                 // table cells filled
};

// Full table with reconstruction. Theta(n*m) time and space.
LcsResult lcsTokens(const std::vector<std::string>& a, const std::vector<std::string>& b);

// Length only, two rolling rows. Theta(n*m) time, Theta(min(n,m)) space.
int lcsLength(const std::vector<std::string>& a, const std::vector<std::string>& b, long long* cells = nullptr);

// 2*LCS / (|a| + |b|), in [0, 1].
double lcsSimilarity(const std::vector<std::string>& a, const std::vector<std::string>& b, long long* cells = nullptr);

#endif
