/* divide_conquer.h - Divide-and-conquer algorithms implemented by hand:
 *   merge sort, quick sort (3 pivot rules), iterative + recursive binary search,
 *   linear search (baseline), maximum subarray (D&C vs Kadane), fast modular power.
 *
 * Sorting and searching are GENERIC (like the C library's qsort/bsearch): they work on
 * any array through a void pointer, the element size, and a comparison function.
 * The same merge_sort sorts Books, Items, Intervals or plain ints. */
#ifndef DIVIDE_CONQUER_H
#define DIVIDE_CONQUER_H

#include <stddef.h>

typedef struct {
    long long comparisons;   /* calls to the comparison function */
    int max_depth;           /* deepest level of the recursion tree reached */
} SortStats;

typedef enum { PIVOT_LAST, PIVOT_RANDOM, PIVOT_MEDIAN3 } PivotRule;

/* Three-way comparison: < 0 if a goes before b, 0 if equal, > 0 if a goes after b.
 * ctx is passed through unchanged (e.g. a pointer to the library). */
typedef int (*CompareFn)(const void *a, const void *b, void *ctx);

/* Stable. Theta(n log n) in every case. Theta(n) extra memory.
 * Recurrence: T(n) = 2T(n/2) + Theta(n). */
void merge_sort(void *base, size_t n, size_t size, CompareFn cmp, void *ctx, SortStats *st);

/* In place, not stable. 3-way partition. Expected Theta(n log n) with PIVOT_RANDOM,
 * worst case Theta(n^2) (e.g. PIVOT_LAST on sorted input). Stack depth O(log n). */
void quick_sort(void *base, size_t n, size_t size, CompareFn cmp, void *ctx, SortStats *st, PivotRule rule,
                unsigned long long seed);

/* Compares a search key with an array element: < 0 if key is smaller than the element's key. */
typedef int (*KeyCompareFn)(const void *key, const void *elem);

/* Array must be sorted by the key. Return an index whose key matches, or -1.
 * probes counts elements examined. */
int binary_search_iterative(const void *base, int n, size_t size, const void *key, KeyCompareFn cmp,
                            long long *probes);   /* Theta(log n) time, O(1) space */
int binary_search_recursive(const void *base, int n, size_t size, const void *key, KeyCompareFn cmp,
                            long long *probes);   /* Theta(log n) time, O(log n) stack */
int linear_search(const void *base, int n, size_t size, const void *key, KeyCompareFn cmp,
                  long long *probes);             /* Theta(n), works on unsorted data */

/* ---- Maximum subarray (best period of net borrowing growth) ---- */
typedef struct {
    long long sum;
    int left;        /* inclusive start, -1 if input empty */
    int right;       /* inclusive end */
    long long ops;
} SubarrayResult;

SubarrayResult max_subarray_dc(const long long *a, int n);      /* Theta(n log n) */
SubarrayResult max_subarray_kadane(const long long *a, int n);  /* Theta(n) */

/* ---- Fast exponentiation: (base^exp) mod m by repeated squaring ----
 * T(e) = T(e/2) + O(1) => Theta(log e). Requires 1 <= mod < 3e9. */
long long pow_mod(long long base, long long exp, long long mod, long long *multiplications);

#endif
