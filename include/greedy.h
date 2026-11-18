/* greedy.h - Greedy algorithms:
 *   greedy 0/1 reading plan (heuristic), fractional knapsack (exact for fractions,
 *   upper bound for 0/1), activity selection (one room), interval partitioning
 *   (minimum rooms), Huffman coding. */
#ifndef GREEDY_H
#define GREEDY_H

#include "model.h"

/* Sort by value/minute, take whole items while they fit. Theta(n log n). NOT always optimal. */
KnapsackResult greedy_knapsack01(const Item *items, int n, int capacity);

/* Best value if items could be split. Theta(n log n). Optimal for the fractional problem. */
double fractional_knapsack(const Item *items, int n, int capacity, long long *ops);

/* Maximum number of non-overlapping bookings in ONE room: sort by finish time, pick greedily.
 * Theta(n log n). Optimal (exchange argument). out must have room for n; returns count chosen. */
int activity_selection(const Interval *in, int n, Interval *out, long long *ops);

/* Baseline: accept in order of START time if no overlap. Not optimal. */
int first_come_selection(const Interval *in, int n, Interval *out, long long *ops);

/* Minimum rooms so every booking is served. Theta(n log n) using a min-heap of room free times.
 * If room_of is not NULL (room for n ints) it receives the room of each input booking
 * (-1 for invalid bookings). */
int interval_partitioning(const Interval *in, int n, int *room_of, long long *ops);

typedef struct {
    char *codes[256];        /* bit string per symbol (NULL if symbol absent) */
    long long original_bits;
    long long encoded_bits;
    int distinct_symbols;
    int round_trip_ok;       /* decode(encode(text)) == text */
} HuffmanResult;

/* Optimal prefix code. Theta(N + k log k) for N characters and k distinct symbols. */
HuffmanResult huffman_compress(const char *text, int n);
void huffman_result_free(HuffmanResult *r);

#endif
