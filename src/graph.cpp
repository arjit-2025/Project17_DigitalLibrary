// graph.cpp - Dijkstra shortest paths and maximum bipartite matching.
#include "graph.h"

#include <algorithm>
#include <functional>
#include <queue>
#include <utility>

PathResult dijkstra(const std::vector<std::vector<Edge>>& g, const std::vector<int>& sources) {
    PathResult r;
    const int n = (int)g.size();
    r.dist.assign(n, INF_DIST);
    r.parent.assign(n, -1);

    typedef std::pair<long long, int> QE;   // (tentative distance, vertex)
    std::priority_queue<QE, std::vector<QE>, std::greater<QE>> pq;
    for (size_t i = 0; i < sources.size(); ++i) {
        int s = sources[i];
        if (s >= 0 && s < n && r.dist[s] != 0) {
            r.dist[s] = 0;
            pq.push(QE(0, s));
        }
    }

    while (!pq.empty()) {
        QE top = pq.top();
        pq.pop();
        int u = top.second;
        if (top.first > r.dist[u]) continue;   // stale heap entry (lazy deletion)
        ++r.settled;
        for (size_t k = 0; k < g[u].size(); ++k) {
            ++r.relaxations;
            int v = g[u][k].first;
            int w = g[u][k].second;
            if (v < 0 || v >= n || w < 0) continue;   // invalid edge
            long long nd = top.first + w;
            if (nd < r.dist[v]) {                     // relax
                r.dist[v] = nd;
                r.parent[v] = u;
                pq.push(QE(nd, v));
            }
        }
    }
    return r;
}

std::vector<int> extractPath(const PathResult& pr, int target) {
    std::vector<int> path;
    if (target < 0 || target >= (int)pr.dist.size() || pr.dist[target] >= INF_DIST) return path;
    for (int v = target; v != -1; v = pr.parent[v]) path.push_back(v);
    std::reverse(path.begin(), path.end());
    return path;
}

namespace {

// Try to find an augmenting path starting at left vertex u.
bool tryAugment(int u, const std::vector<std::vector<int>>& adj, int nRight, std::vector<int>& matchRight,
                std::vector<char>& visited, long long& steps) {
    for (size_t k = 0; k < adj[u].size(); ++k) {
        ++steps;
        int v = adj[u][k];
        if (v < 0 || v >= nRight || visited[v]) continue;
        visited[v] = 1;
        // v is free, or the reader holding v can be moved to another copy.
        if (matchRight[v] == -1 || tryAugment(matchRight[v], adj, nRight, matchRight, visited, steps)) {
            matchRight[v] = u;
            return true;
        }
    }
    return false;
}

}  // namespace

MatchingResult maxBipartiteMatching(int nLeft, int nRight, const std::vector<std::vector<int>>& adj) {
    MatchingResult r;
    if (nLeft < 0) nLeft = 0;
    if (nRight < 0) nRight = 0;
    std::vector<int> matchRight(nRight, -1);
    std::vector<char> visited(nRight, 0);
    for (int u = 0; u < nLeft && u < (int)adj.size(); ++u) {
        std::fill(visited.begin(), visited.end(), 0);
        if (tryAugment(u, adj, nRight, matchRight, visited, r.steps)) ++r.size;
    }
    r.matchOfLeft.assign(nLeft, -1);
    for (int v = 0; v < nRight; ++v)
        if (matchRight[v] != -1) r.matchOfLeft[matchRight[v]] = v;
    return r;
}
