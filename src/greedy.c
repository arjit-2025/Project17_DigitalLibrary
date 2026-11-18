/* greedy.c - Greedy scheduling, allocation and compression modules. */
#include "greedy.h"

#include <stdlib.h>
#include <string.h>

#include "divide_conquer.h"
#include "heap.h"

static double ratio(const Item *it) { return it->weight == 0 ? 1e18 : it->value / it->weight; }

static int cmp_ratio_desc(const void *a, const void *b, void *ctx) {
    double ra = ratio((const Item *)a), rb = ratio((const Item *)b);
    (void)ctx;
    return ra > rb ? -1 : (ra < rb ? 1 : 0);
}

/* Copies items with weight >= 0 and value >= 0; returns the count. */
static int valid_items(const Item *items, int n, Item *out) {
    int i, k = 0;
    for (i = 0; i < n; ++i)
        if (items[i].weight >= 0 && items[i].value >= 0) out[k++] = items[i];
    return k;
}

KnapsackResult greedy_knapsack01(const Item *items, int n, int capacity) {
    KnapsackResult r;
    Item *v = (Item *)xmalloc((size_t)(n > 0 ? n : 1) * sizeof(Item));
    SortStats st = {0, 0};
    int k = valid_items(items, n, v), i, remaining = capacity < 0 ? 0 : capacity;

    knapsack_result_init(&r, "Greedy (best value per minute)");
    merge_sort(v, (size_t)k, sizeof(Item), cmp_ratio_desc, NULL, &st);
    r.ops = st.comparisons;
    for (i = 0; i < k; ++i) {
        r.ops++;
        if (v[i].weight <= remaining) {       /* take the whole book if it fits */
            remaining -= v[i].weight;
            intvec_push(&r.chosen, v[i].id);
            r.value += v[i].value;
            r.weight += v[i].weight;
        }
    }
    r.aux_bytes = (long long)k * 2 * (long long)sizeof(Item);   /* copy + merge buffer */
    free(v);
    return r;
}

double fractional_knapsack(const Item *items, int n, int capacity, long long *ops) {
    Item *v = (Item *)xmalloc((size_t)(n > 0 ? n : 1) * sizeof(Item));
    SortStats st = {0, 0};
    int k = valid_items(items, n, v), i;
    double value = 0.0, remaining = capacity < 0 ? 0 : capacity;
    merge_sort(v, (size_t)k, sizeof(Item), cmp_ratio_desc, NULL, &st);
    *ops += st.comparisons;
    for (i = 0; i < k && remaining > 0; ++i) {
        (*ops)++;
        if (v[i].weight <= remaining) {
            value += v[i].value;
            remaining -= v[i].weight;
        } else {
            value += v[i].value * (remaining / v[i].weight);   /* take the fitting fraction */
            remaining = 0;
        }
    }
    free(v);
    return value;
}

/* ---- Interval scheduling ---- */

static int cmp_by_end(const void *a, const void *b, void *ctx) {
    const Interval *x = (const Interval *)a, *y = (const Interval *)b;
    (void)ctx;
    if (x->end != y->end) return x->end < y->end ? -1 : 1;
    return x->start < y->start ? -1 : (x->start > y->start ? 1 : 0);
}

static int cmp_by_start(const void *a, const void *b, void *ctx) {
    const Interval *x = (const Interval *)a, *y = (const Interval *)b;
    (void)ctx;
    if (x->start != y->start) return x->start < y->start ? -1 : 1;
    return x->end < y->end ? -1 : (x->end > y->end ? 1 : 0);
}

/* Shared greedy loop: sort with cmp, then accept every booking that starts after the last accepted one ends. */
static int select_compatible(const Interval *in, int n, Interval *out, long long *ops, CompareFn cmp) {
    Interval *v = (Interval *)xmalloc((size_t)(n > 0 ? n : 1) * sizeof(Interval));
    SortStats st = {0, 0};
    int i, k = 0, chosen = 0, last_end = -2147483647;
    for (i = 0; i < n; ++i)
        if (in[i].end > in[i].start) v[k++] = in[i];   /* reject empty / reversed bookings */
    merge_sort(v, (size_t)k, sizeof(Interval), cmp, NULL, &st);
    *ops += st.comparisons;
    for (i = 0; i < k; ++i) {
        (*ops)++;
        if (v[i].start >= last_end) {   /* compatible with everything chosen so far */
            out[chosen++] = v[i];
            last_end = v[i].end;
        }
    }
    free(v);
    return chosen;
}

int activity_selection(const Interval *in, int n, Interval *out, long long *ops) {
    return select_compatible(in, n, out, ops, cmp_by_end);     /* earliest FINISH first: optimal */
}

int first_come_selection(const Interval *in, int n, Interval *out, long long *ops) {
    return select_compatible(in, n, out, ops, cmp_by_start);   /* earliest START first: not optimal */
}

typedef struct {
    Interval iv;
    int pos;   /* position in the caller's array */
} PosInterval;

static int cmp_pos_by_start(const void *a, const void *b, void *ctx) {
    return cmp_by_start(&((const PosInterval *)a)->iv, &((const PosInterval *)b)->iv, ctx);
}

int interval_partitioning(const Interval *in, int n, int *room_of, long long *ops) {
    PosInterval *v = (PosInterval *)xmalloc((size_t)(n > 0 ? n : 1) * sizeof(PosInterval));
    SortStats st = {0, 0};
    MinHeap heap;   /* (time the room becomes free, room number) */
    int i, k = 0, rooms = 0;
    for (i = 0; i < n; ++i) {
        if (room_of) room_of[i] = -1;
        if (in[i].end > in[i].start) {
            v[k].iv = in[i];
            v[k].pos = i;
            ++k;
        }
    }
    merge_sort(v, (size_t)k, sizeof(PosInterval), cmp_pos_by_start, NULL, &st);
    *ops += st.comparisons;
    heap_init(&heap);
    for (i = 0; i < k; ++i) {
        int room;
        (*ops)++;
        if (!heap_empty(&heap) && heap_top(&heap).key <= v[i].iv.start) {
            room = heap_pop(&heap).value;   /* reuse the room that frees up earliest */
        } else {
            room = rooms++;                 /* all rooms busy: open a new one */
        }
        if (room_of) room_of[v[i].pos] = room;
        heap_push(&heap, v[i].iv.end, room);
    }
    heap_free(&heap);
    free(v);
    return rooms;
}

/* ---- Huffman coding ---- */

typedef struct {
    long long freq;
    int left, right;
    int sym;   /* -1 for internal nodes */
} HNode;

static void assign_codes(const HNode *nodes, int idx, char *buf, int depth, HuffmanResult *r) {
    if (nodes[idx].sym >= 0) {
        buf[depth] = '\0';
        r->codes[nodes[idx].sym] = xstrdup(buf);
        return;
    }
    buf[depth] = '0';
    assign_codes(nodes, nodes[idx].left, buf, depth + 1, r);
    buf[depth] = '1';
    assign_codes(nodes, nodes[idx].right, buf, depth + 1, r);
}

HuffmanResult huffman_compress(const char *text, int n) {
    HuffmanResult r;
    long long freq[256] = {0};
    HNode nodes[511];
    int num_nodes = 0, c, i, root;
    MinHeap pq;
    char buf[257];

    memset(&r, 0, sizeof(r));
    r.original_bits = 8LL * n;
    if (n <= 0) {
        r.round_trip_ok = 1;
        return r;
    }
    for (i = 0; i < n; ++i) freq[(unsigned char)text[i]]++;

    heap_init(&pq);
    for (c = 0; c < 256; ++c) {
        if (freq[c]) {
            nodes[num_nodes].freq = freq[c];
            nodes[num_nodes].left = nodes[num_nodes].right = -1;
            nodes[num_nodes].sym = c;
            heap_push(&pq, freq[c], num_nodes);
            ++num_nodes;
        }
    }
    r.distinct_symbols = num_nodes;

    if (num_nodes == 1) {
        root = 0;
        r.codes[nodes[0].sym] = xstrdup("0");   /* a single symbol still needs one bit */
    } else {
        /* Greedy step: repeatedly merge the two least frequent trees. */
        while (pq.size > 1) {
            HeapItem a = heap_pop(&pq), b = heap_pop(&pq);
            nodes[num_nodes].freq = a.key + b.key;
            nodes[num_nodes].left = a.value;
            nodes[num_nodes].right = b.value;
            nodes[num_nodes].sym = -1;
            heap_push(&pq, nodes[num_nodes].freq, num_nodes);
            ++num_nodes;
        }
        root = heap_top(&pq).value;
        assign_codes(nodes, root, buf, 0, &r);
    }
    heap_free(&pq);

    /* Encode, then decode, to prove it is a correct prefix code. */
    for (c = 0; c < 256; ++c)
        if (freq[c]) r.encoded_bits += freq[c] * (long long)strlen(r.codes[c]);
    {
        char *bits = (char *)xmalloc((size_t)r.encoded_bits + 1);
        char *decoded = (char *)xmalloc((size_t)n + 1);
        long long pos = 0, b;
        int out = 0, cur = root;
        for (i = 0; i < n; ++i) {
            const char *code = r.codes[(unsigned char)text[i]];
            size_t len = strlen(code);
            memcpy(bits + pos, code, len);
            pos += (long long)len;
        }
        if (num_nodes == 1) {
            for (b = 0; b < r.encoded_bits; ++b) decoded[out++] = (char)nodes[0].sym;
        } else {
            for (b = 0; b < r.encoded_bits && out <= n; ++b) {
                cur = bits[b] == '0' ? nodes[cur].left : nodes[cur].right;
                if (nodes[cur].sym >= 0) {
                    if (out < n) decoded[out] = (char)nodes[cur].sym;
                    ++out;
                    cur = root;
                }
            }
        }
        r.round_trip_ok = (out == n) && memcmp(decoded, text, (size_t)n) == 0;
        free(bits);
        free(decoded);
    }
    return r;
}

void huffman_result_free(HuffmanResult *r) {
    int c;
    for (c = 0; c < 256; ++c) {
        free(r->codes[c]);
        r->codes[c] = NULL;
    }
}
