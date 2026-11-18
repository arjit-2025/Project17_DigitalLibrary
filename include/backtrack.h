// backtrack.h - Exact 0/1 knapsack by backtracking with a branch-and-bound prune.
// Depth-first (LIFO) exploration of the include/exclude tree; a branch is cut when
// its fractional-knapsack upper bound cannot beat the best solution found so far.
#ifndef BACKTRACK_H
#define BACKTRACK_H

#include <vector>

#include "model.h"

// Worst case Theta(2^n) nodes, usually far fewer thanks to the bound.
// If more than nodeLimit nodes are explored, the search stops and result.complete = false.
KnapsackResult knapsackBranchAndBound(const std::vector<Item>& items, int capacity, long long nodeLimit = 5000000);

#endif
