// greedy.h - Greedy algorithms:
//   greedy 0/1 reading plan (heuristic), fractional knapsack (exact for fractions, upper bound for 0/1),
//   activity selection (one reading room), interval partitioning (minimum rooms), Huffman coding.
#ifndef GREEDY_H
#define GREEDY_H

#include <string>
#include <utility>
#include <vector>

#include "model.h"

// Sort by value/minute, take whole items while they fit. Theta(n log n). NOT always optimal.
KnapsackResult greedyKnapsack01(const std::vector<Item>& items, int capacity);

// Best value if items could be taken fractionally. Theta(n log n). Optimal for the fractional problem.
double fractionalKnapsack(const std::vector<Item>& items, int capacity, long long& ops);

// Maximum number of non-overlapping bookings in ONE room: sort by finish time, pick greedily.
// Theta(n log n). Provably optimal (exchange argument).
std::vector<Interval> activitySelection(std::vector<Interval> bookings, long long& ops);

// Baseline for comparison: accept bookings in order of START time if they do not overlap. Not optimal.
std::vector<Interval> firstComeSelection(std::vector<Interval> bookings, long long& ops);

struct PartitionResult {
    int rooms = 0;                                  // minimum rooms needed
    std::vector<std::pair<int, int>> assignment;    // (booking id, room number)
};
// Minimum rooms so that every booking is served. Theta(n log n) with a min-heap of room finish times.
PartitionResult intervalPartitioning(std::vector<Interval> bookings, long long& ops);

struct HuffmanResult {
    std::vector<std::pair<unsigned char, std::string>> codes;   // symbol -> bit string
    long long originalBits = 0;
    long long encodedBits = 0;
    int distinctSymbols = 0;
    bool roundTripOk = false;    // decode(encode(text)) == text
};
// Optimal prefix code. Theta(n + k log k) for n characters and k distinct symbols.
HuffmanResult huffmanCompress(const std::string& text);

#endif
