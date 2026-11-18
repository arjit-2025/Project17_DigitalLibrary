/* divide_conquer.c - Generic sorting/searching plus maximum subarray and fast power. */
#include "divide_conquer.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "rng.h"
#include "vec.h"

/* Address of element i in an array of elements of `size` bytes. */
#define ELEM(base, i, size) ((char *)(base) + (size_t)(i) * (size))
#define CELEM(base, i, size) ((const char *)(base) + (size_t)(i) * (size))

/* ======================= Merge sort ======================= */

typedef struct {
    char *a;        /* array being sorted */
    char *tmp;      /* one shared buffer of n elements, allocated once */
    size_t size;
    CompareFn cmp;
    void *ctx;
    SortStats *st;
} MergeCtx;

/* Sorts a[lo, hi). */
static void merge_rec(MergeCtx *m, int lo, int hi, int depth) {
    int mid, i, j, k;
    size_t sz = m->size;
    if (depth > m->st->max_depth) m->st->max_depth = depth;
    if (hi - lo <= 1) return;                 /* base case: 0 or 1 element is sorted */
    mid = lo + (hi - lo) / 2;
    merge_rec(m, lo, mid, depth + 1);         /* divide */
    merge_rec(m, mid, hi, depth + 1);

    i = lo; j = mid; k = lo;                  /* combine: merge two sorted halves */
    while (i < mid && j < hi) {
        m->st->comparisons++;
        /* Take from the right only if strictly smaller -> equal keys keep order (STABLE). */
        if (m->cmp(ELEM(m->a, j, sz), ELEM(m->a, i, sz), m->ctx) < 0) {
            memcpy(ELEM(m->tmp, k, sz), ELEM(m->a, j, sz), sz);
            ++j;
        } else {
            memcpy(ELEM(m->tmp, k, sz), ELEM(m->a, i, sz), sz);
            ++i;
        }
        ++k;
    }
    if (i < mid) { memcpy(ELEM(m->tmp, k, sz), ELEM(m->a, i, sz), (size_t)(mid - i) * sz); k += mid - i; }
    if (j < hi) memcpy(ELEM(m->tmp, k, sz), ELEM(m->a, j, sz), (size_t)(hi - j) * sz);
    memcpy(ELEM(m->a, lo, sz), ELEM(m->tmp, lo, sz), (size_t)(hi - lo) * sz);
}

void merge_sort(void *base, size_t n, size_t size, CompareFn cmp, void *ctx, SortStats *st) {
    MergeCtx m;
    if (n < 2) return;
    m.a = (char *)base;
    m.tmp = (char *)xmalloc(n * size);
    m.size = size;
    m.cmp = cmp;
    m.ctx = ctx;
    m.st = st;
    merge_rec(&m, 0, (int)n, 1);
    free(m.tmp);
}

/* ======================= Quick sort ======================= */

typedef struct {
    char *a;
    size_t size;
    CompareFn cmp;
    void *ctx;
    SortStats *st;
    PivotRule rule;
    Rng rng;
    char *pivot;     /* copy of the pivot element */
    char *swapbuf;   /* scratch space for swapping two elements */
} QuickCtx;

static void swap_elems(QuickCtx *q, int i, int j) {
    size_t sz = q->size;
    if (i == j) return;
    memcpy(q->swapbuf, ELEM(q->a, i, sz), sz);
    memcpy(ELEM(q->a, i, sz), ELEM(q->a, j, sz), sz);
    memcpy(ELEM(q->a, j, sz), q->swapbuf, sz);
}

/* Sorts a[lo..hi] inclusive. */
static void quick_rec(QuickCtx *q, int lo, int hi, int depth) {
    size_t sz = q->size;
    while (lo < hi) {
        int p = hi, lt, i, gt;
        if (depth > q->st->max_depth) q->st->max_depth = depth;

        if (q->rule == PIVOT_RANDOM) {
            p = rng_range(&q->rng, lo, hi);
        } else if (q->rule == PIVOT_MEDIAN3) {
            int x = lo, y = lo + (hi - lo) / 2, z = hi, t;
            q->st->comparisons += 3;
            if (q->cmp(ELEM(q->a, y, sz), ELEM(q->a, x, sz), q->ctx) < 0) { t = x; x = y; y = t; }
            if (q->cmp(ELEM(q->a, z, sz), ELEM(q->a, y, sz), q->ctx) < 0) { t = y; y = z; z = t; }
            if (q->cmp(ELEM(q->a, y, sz), ELEM(q->a, x, sz), q->ctx) < 0) { t = x; x = y; y = t; }
            p = y;   /* index of the median of the three values */
        }
        memcpy(q->pivot, ELEM(q->a, p, sz), sz);

        /* 3-way partition (Dutch national flag). Invariant:
         *   a[lo..lt-1] < pivot,  a[lt..i-1] == pivot,  a[gt+1..hi] > pivot */
        lt = lo; i = lo; gt = hi;
        while (i <= gt) {
            int c;
            q->st->comparisons++;
            c = q->cmp(ELEM(q->a, i, sz), q->pivot, q->ctx);
            if (c < 0) { swap_elems(q, lt, i); ++lt; ++i; }
            else if (c > 0) { swap_elems(q, i, gt); --gt; }
            else ++i;
        }

        /* Recurse on the smaller side, loop on the larger: call stack stays O(log n). */
        if (lt - lo < hi - gt) { quick_rec(q, lo, lt - 1, depth + 1); lo = gt + 1; }
        else { quick_rec(q, gt + 1, hi, depth + 1); hi = lt - 1; }
        ++depth;
    }
}

void quick_sort(void *base, size_t n, size_t size, CompareFn cmp, void *ctx, SortStats *st, PivotRule rule,
                unsigned long long seed) {
    QuickCtx q;
    if (n < 2) return;
    q.a = (char *)base;
    q.size = size;
    q.cmp = cmp;
    q.ctx = ctx;
    q.st = st;
    q.rule = rule;
    rng_seed(&q.rng, seed);
    q.pivot = (char *)xmalloc(size);
    q.swapbuf = (char *)xmalloc(size);
    quick_rec(&q, 0, (int)n - 1, 1);
    free(q.pivot);
    free(q.swapbuf);
}

/* ======================= Searching ======================= */

int binary_search_iterative(const void *base, int n, size_t size, const void *key, KeyCompareFn cmp,
                            long long *probes) {
    int lo = 0, hi = n - 1;
    while (lo <= hi) {                       /* invariant: if key exists, it is in [lo, hi] */
        int mid = lo + (hi - lo) / 2;        /* avoids overflow of (lo + hi) */
        int c;
        (*probes)++;
        c = cmp(key, CELEM(base, mid, size));
        if (c == 0) return mid;
        if (c > 0) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

static int bs_rec(const void *base, size_t size, const void *key, KeyCompareFn cmp, long long *probes, int lo,
                  int hi) {
    int mid, c;
    if (lo > hi) return -1;                  /* base case: empty range */
    mid = lo + (hi - lo) / 2;
    (*probes)++;
    c = cmp(key, CELEM(base, mid, size));
    if (c == 0) return mid;
    if (c > 0) return bs_rec(base, size, key, cmp, probes, mid + 1, hi);
    return bs_rec(base, size, key, cmp, probes, lo, mid - 1);
}

int binary_search_recursive(const void *base, int n, size_t size, const void *key, KeyCompareFn cmp,
                            long long *probes) {
    return bs_rec(base, size, key, cmp, probes, 0, n - 1);
}

int linear_search(const void *base, int n, size_t size, const void *key, KeyCompareFn cmp, long long *probes) {
    int i;
    for (i = 0; i < n; ++i) {
        (*probes)++;
        if (cmp(key, CELEM(base, i, size)) == 0) return i;
    }
    return -1;
}

/* ======================= Maximum subarray ======================= */

static SubarrayResult max_sub_rec(const long long *a, int lo, int hi, long long *ops) {
    SubarrayResult L, R, C;
    long long s = 0, best_l = LLONG_MIN, best_r = LLONG_MIN;
    int mid, i, li, ri;
    if (lo == hi) {
        C.sum = a[lo];
        C.left = C.right = lo;
        C.ops = 0;
        return C;
    }
    mid = lo + (hi - lo) / 2;
    L = max_sub_rec(a, lo, mid, ops);
    R = max_sub_rec(a, mid + 1, hi, ops);

    /* Best subarray crossing the midpoint = best suffix of left + best prefix of right. */
    li = mid;
    for (i = mid; i >= lo; --i) {
        (*ops)++;
        s += a[i];
        if (s > best_l) { best_l = s; li = i; }
    }
    s = 0;
    ri = mid + 1;
    for (i = mid + 1; i <= hi; ++i) {
        (*ops)++;
        s += a[i];
        if (s > best_r) { best_r = s; ri = i; }
    }
    C.sum = best_l + best_r;
    C.left = li;
    C.right = ri;
    C.ops = 0;

    if (L.sum >= R.sum && L.sum >= C.sum) return L;
    if (R.sum >= C.sum) return R;
    return C;
}

SubarrayResult max_subarray_dc(const long long *a, int n) {
    SubarrayResult r;
    long long ops = 0;
    if (n <= 0) {
        r.sum = 0; r.left = r.right = -1; r.ops = 0;
        return r;
    }
    r = max_sub_rec(a, 0, n - 1, &ops);
    r.ops = ops;
    return r;
}

SubarrayResult max_subarray_kadane(const long long *a, int n) {
    SubarrayResult r;
    long long best, cur;
    int i, bl = 0, br = 0, cl = 0;
    r.ops = 0;
    if (n <= 0) {
        r.sum = 0; r.left = r.right = -1;
        return r;
    }
    best = cur = a[0];
    for (i = 1; i < n; ++i) {
        r.ops++;
        /* Either extend the current run or start a new one at i. */
        if (cur < 0) { cur = a[i]; cl = i; }
        else cur += a[i];
        if (cur > best) { best = cur; bl = cl; br = i; }
    }
    r.sum = best;
    r.left = bl;
    r.right = br;
    return r;
}

/* ======================= Fast power ======================= */

long long pow_mod(long long base, long long exp, long long mod, long long *multiplications) {
    long long half, result;
    if (mod <= 1) return 0;
    base %= mod;
    if (base < 0) base += mod;
    if (exp <= 0) return 1 % mod;
    half = pow_mod(base, exp / 2, mod, multiplications);   /* solve the half-size problem once */
    result = half * half % mod;
    if (multiplications) (*multiplications)++;
    if (exp & 1) {
        result = result * base % mod;
        if (multiplications) (*multiplications)++;
    }
    return result;
}
