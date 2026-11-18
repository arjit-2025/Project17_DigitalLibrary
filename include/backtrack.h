/* backtrack.h - Exact 0/1 knapsack by backtracking with a branch-and-bound prune.
 * Depth-first (LIFO) exploration of the take/skip tree; a branch is cut when its
 * fractional-knapsack upper bound cannot beat the best solution found so far. */
#ifndef BACKTRACK_H
#define BACKTRACK_H

#include "model.h"

/* Worst case Theta(2^n) nodes, usually far fewer thanks to the bound.
 * Stops after node_limit nodes and sets result.complete = 0. */
KnapsackResult knapsack_branch_and_bound(const Item *items, int n, int capacity, long long node_limit);

#endif
