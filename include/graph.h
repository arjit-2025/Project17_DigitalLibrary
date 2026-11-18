// graph.h - Graph algorithms on the citation network and on reader/copy assignment.
#ifndef GRAPH_H
#define GRAPH_H

#include <vector>

#include "model.h"

const long long INF_DIST = (long long)4e18;

struct PathResult {
    std::vector<long long> dist;   // INF_DIST if unreachable
    std::vector<int> parent;       // -1 for sources / unreachable
    long long relaxations = 0;     // edges examined
    int settled = 0;               // vertices finalised
};

// Multi-source Dijkstra with a binary heap. Theta((V + E) log V). Requires weights >= 0
// (negative edges are skipped as invalid input).
PathResult dijkstra(const std::vector<std::vector<Edge>>& g, const std::vector<int>& sources);

// Vertices from a source to target (empty if unreachable).
std::vector<int> extractPath(const PathResult& pr, int target);

struct MatchingResult {
    int size = 0;
    std::vector<int> matchOfLeft;   // right vertex matched to each left vertex, or -1
    long long steps = 0;            // edges examined while searching augmenting paths
};

// Maximum bipartite matching by augmenting paths (Ford-Fulkerson on a unit-capacity network,
// a.k.a. Kuhn's algorithm). Theta(V * E).
MatchingResult maxBipartiteMatching(int nLeft, int nRight, const std::vector<std::vector<int>>& adj);

#endif
