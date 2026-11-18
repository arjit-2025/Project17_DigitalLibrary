/* graph.c - Dijkstra shortest paths and maximum bipartite matching. */
#include "graph.h"

#include <stdlib.h>
#include <string.h>

#include "heap.h"

PathResult dijkstra(const EdgeList *g, int n, const int *sources, int num_sources) {
    PathResult r;
    MinHeap pq;   /* (tentative distance, vertex) */
    int i;
    r.n = n;
    r.dist = (long long *)xmalloc((size_t)(n > 0 ? n : 1) * sizeof(long long));
    r.parent = (int *)xmalloc((size_t)(n > 0 ? n : 1) * sizeof(int));
    r.relaxations = 0;
    r.settled = 0;
    for (i = 0; i < n; ++i) {
        r.dist[i] = INF_DIST;
        r.parent[i] = -1;
    }
    heap_init(&pq);
    for (i = 0; i < num_sources; ++i) {
        int s = sources[i];
        if (s >= 0 && s < n && r.dist[s] != 0) {
            r.dist[s] = 0;
            heap_push(&pq, 0, s);
        }
    }

    while (!heap_empty(&pq)) {
        HeapItem top = heap_pop(&pq);
        int u = top.value, k;
        if (top.key > r.dist[u]) continue;   /* stale entry: u was already settled (lazy deletion) */
        r.settled++;
        for (k = 0; k < g[u].size; ++k) {
            int v = g[u].e[k].to, w = g[u].e[k].weight;
            long long nd;
            r.relaxations++;
            if (v < 0 || v >= n || w < 0) continue;   /* invalid edge */
            nd = top.key + w;
            if (nd < r.dist[v]) {                     /* relax */
                r.dist[v] = nd;
                r.parent[v] = u;
                heap_push(&pq, nd, v);
            }
        }
    }
    heap_free(&pq);
    return r;
}

void path_result_free(PathResult *pr) {
    free(pr->dist);
    free(pr->parent);
    pr->dist = NULL;
    pr->parent = NULL;
}

void extract_path(const PathResult *pr, int target, IntVec *out) {
    int v, i;
    intvec_clear(out);
    if (target < 0 || target >= pr->n || pr->dist[target] >= INF_DIST) return;
    for (v = target; v != -1; v = pr->parent[v]) intvec_push(out, v);
    for (i = 0; i < out->size / 2; ++i) {   /* reverse: source first */
        int t = out->data[i];
        out->data[i] = out->data[out->size - 1 - i];
        out->data[out->size - 1 - i] = t;
    }
}

typedef struct {
    const IntVec *adj;
    int n_right;
    int *match_right;   /* left vertex holding each right vertex, or -1 */
    char *visited;
    long long steps;
} MatchCtx;

/* Try to find an augmenting path starting at left vertex u. */
static int try_augment(MatchCtx *c, int u) {
    int k;
    for (k = 0; k < c->adj[u].size; ++k) {
        int v = c->adj[u].data[k];
        c->steps++;
        if (v < 0 || v >= c->n_right || c->visited[v]) continue;
        c->visited[v] = 1;
        /* v is free, or the reader holding v can be moved to another copy. */
        if (c->match_right[v] == -1 || try_augment(c, c->match_right[v])) {
            c->match_right[v] = u;
            return 1;
        }
    }
    return 0;
}

MatchingResult max_bipartite_matching(int n_left, int n_right, const IntVec *adj) {
    MatchingResult r;
    MatchCtx c;
    int u, v;
    if (n_left < 0) n_left = 0;
    if (n_right < 0) n_right = 0;
    c.adj = adj;
    c.n_right = n_right;
    c.match_right = (int *)xmalloc((size_t)(n_right > 0 ? n_right : 1) * sizeof(int));
    c.visited = (char *)xmalloc((size_t)(n_right > 0 ? n_right : 1));
    c.steps = 0;
    for (v = 0; v < n_right; ++v) c.match_right[v] = -1;

    r.size = 0;
    for (u = 0; u < n_left; ++u) {
        memset(c.visited, 0, (size_t)(n_right > 0 ? n_right : 1));
        if (try_augment(&c, u)) r.size++;
    }
    r.n_left = n_left;
    r.match_of_left = (int *)xmalloc((size_t)(n_left > 0 ? n_left : 1) * sizeof(int));
    for (u = 0; u < n_left; ++u) r.match_of_left[u] = -1;
    for (v = 0; v < n_right; ++v)
        if (c.match_right[v] != -1) r.match_of_left[c.match_right[v]] = v;
    r.steps = c.steps;
    free(c.match_right);
    free(c.visited);
    return r;
}

void matching_result_free(MatchingResult *m) {
    free(m->match_of_left);
    m->match_of_left = NULL;
}
