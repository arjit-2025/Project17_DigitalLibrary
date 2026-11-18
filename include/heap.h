/* heap.h - Binary min-heap of (key, value) pairs, implemented by hand.
 * Used by Dijkstra (key = distance), interval partitioning (key = room free time)
 * and Huffman coding (key = frequency).
 * Order: smaller key first; equal keys -> smaller value first (deterministic ties).
 * push / pop: O(log n).  top: O(1). */
#ifndef HEAP_H
#define HEAP_H

typedef struct {
    long long key;
    int value;
} HeapItem;

typedef struct {
    HeapItem *a;
    int size;
    int cap;
} MinHeap;

void heap_init(MinHeap *h);
void heap_free(MinHeap *h);
int heap_empty(const MinHeap *h);
void heap_push(MinHeap *h, long long key, int value);
HeapItem heap_top(const MinHeap *h);   /* precondition: not empty */
HeapItem heap_pop(MinHeap *h);         /* precondition: not empty */

#endif
