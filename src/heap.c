/* heap.c - Array-based binary min-heap.
 * Node i has children 2i+1 and 2i+2, parent (i-1)/2.
 * Heap property: every parent is <= its children. */
#include "heap.h"

#include <stdlib.h>

#include "vec.h"

static int less_item(HeapItem x, HeapItem y) {
    return x.key < y.key || (x.key == y.key && x.value < y.value);
}

void heap_init(MinHeap *h) {
    h->a = NULL;
    h->size = 0;
    h->cap = 0;
}

void heap_free(MinHeap *h) {
    free(h->a);
    heap_init(h);
}

int heap_empty(const MinHeap *h) { return h->size == 0; }

void heap_push(MinHeap *h, long long key, int value) {
    int i;
    if (h->size == h->cap) {
        h->cap = h->cap ? h->cap * 2 : 16;
        h->a = (HeapItem *)xrealloc(h->a, (size_t)h->cap * sizeof(HeapItem));
    }
    i = h->size++;
    h->a[i].key = key;
    h->a[i].value = value;
    /* sift up: swap with parent while smaller than it */
    while (i > 0) {
        int p = (i - 1) / 2;
        HeapItem t;
        if (!less_item(h->a[i], h->a[p])) break;
        t = h->a[i];
        h->a[i] = h->a[p];
        h->a[p] = t;
        i = p;
    }
}

HeapItem heap_top(const MinHeap *h) { return h->a[0]; }

HeapItem heap_pop(MinHeap *h) {
    HeapItem top = h->a[0];
    int i = 0;
    h->a[0] = h->a[--h->size];
    /* sift down: swap with the smaller child while larger than it */
    for (;;) {
        int l = 2 * i + 1, r = l + 1, m = i;
        HeapItem t;
        if (l < h->size && less_item(h->a[l], h->a[m])) m = l;
        if (r < h->size && less_item(h->a[r], h->a[m])) m = r;
        if (m == i) break;
        t = h->a[i];
        h->a[i] = h->a[m];
        h->a[m] = t;
        i = m;
    }
    return top;
}
