/* graph.h - Graph algorithms on the citation network and on reader/copy assignment. */
#ifndef GRAPH_H
#define GRAPH_H

#include "model.h"

#define INF_DIST 4000000000000000000LL

typedef struct {
    int n;
    long long *dist;         /* INF_DIST if unreachable */
    int *parent;             /* -1 for sources / unreachable */
    long long relaxations;   /* edges examined */
    int settled;             /* vertices finalised */
} PathResult;

/* Multi-source Dijkstra with a binary min-heap. Theta((V + E) log V).
 * Requires weights >= 0 (negative edges are skipped as invalid input). */
PathResult dijkstra(const EdgeList *g, int n, const int *sources, int num_sources);
void path_result_free(PathResult *pr);

/* Vertices from a source to target (cleared and left empty if unreachable). */
void extract_path(const PathResult *pr, int target, IntVec *out);

typedef struct {
    int size;
    int n_left;
    int *match_of_left;   /* right vertex matched to each left vertex, or -1 */
    long long steps;      /* edges examined while searching augmenting paths */
} MatchingResult;

/* Maximum bipartite matching by augmenting paths (Ford-Fulkerson on a unit-capacity
 * network, a.k.a. Kuhn's algorithm). adj[u] lists the right vertices left vertex u accepts. O(V * E). */
MatchingResult max_bipartite_matching(int n_left, int n_right, const IntVec *adj);
void matching_result_free(MatchingResult *m);

#endif
