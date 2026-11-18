// divide_conquer.h - Divide-and-conquer algorithms implemented by hand:
//   merge sort, quick sort (3 pivot rules), iterative + recursive binary search,
//   linear search (baseline), maximum subarray (D&C vs Kadane), fast modular power.
//
// Sorting/searching are templates so they work on Books, Items or plain ints.
#ifndef DIVIDE_CONQUER_H
#define DIVIDE_CONQUER_H

#include <random>
#include <utility>
#include <vector>

struct SortStats {
    long long comparisons = 0;   // key comparisons performed
    int maxDepth = 0;            // deepest level of the recursion tree reached
};

enum class PivotRule { Last, Random, MedianOfThree };

namespace dc_detail {

// Sorts a[lo, hi) . Recurrence: T(n) = 2T(n/2) + Theta(n)  =>  Theta(n log n).
template <typename T, typename Less>
void mergeRec(std::vector<T>& a, std::vector<T>& tmp, int lo, int hi, Less& less, SortStats& st, int depth) {
    if (depth > st.maxDepth) st.maxDepth = depth;
    if (hi - lo <= 1) return;
    int mid = lo + (hi - lo) / 2;
    mergeRec(a, tmp, lo, mid, less, st, depth + 1);
    mergeRec(a, tmp, mid, hi, less, st, depth + 1);

    int i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        ++st.comparisons;
        // Take from the right half only if strictly smaller -> merge sort is STABLE.
        if (less(a[j], a[i])) tmp[k++] = std::move(a[j++]);
        else tmp[k++] = std::move(a[i++]);
    }
    while (i < mid) tmp[k++] = std::move(a[i++]);
    while (j < hi) tmp[k++] = std::move(a[j++]);
    for (int t = lo; t < hi; ++t) a[t] = std::move(tmp[t]);
}

// Sorts a[lo..hi] (inclusive) with a 3-way (Dutch national flag) partition.
// Best/average: Theta(n log n). Worst (bad pivots, e.g. Last on sorted input): Theta(n^2).
// We recurse on the smaller side and loop on the larger one, so the real call
// stack stays O(log n) even in the worst case.
template <typename T, typename Less>
void quickRec(std::vector<T>& a, int lo, int hi, Less& less, SortStats& st, PivotRule rule, std::mt19937& rng,
              int depth) {
    while (lo < hi) {
        if (depth > st.maxDepth) st.maxDepth = depth;

        int p = hi;
        if (rule == PivotRule::Random) {
            p = std::uniform_int_distribution<int>(lo, hi)(rng);
        } else if (rule == PivotRule::MedianOfThree) {
            int x = lo, y = lo + (hi - lo) / 2, z = hi;
            st.comparisons += 3;
            if (less(a[y], a[x])) std::swap(x, y);
            if (less(a[z], a[y])) std::swap(y, z);
            if (less(a[y], a[x])) std::swap(x, y);
            p = y;   // index of the median value
        }
        T pivot = a[p];

        // Invariant: a[lo..lt-1] < pivot, a[lt..i-1] == pivot, a[gt+1..hi] > pivot.
        int lt = lo, i = lo, gt = hi;
        while (i <= gt) {
            ++st.comparisons;
            if (less(a[i], pivot)) {
                std::swap(a[lt], a[i]);
                ++lt;
                ++i;
            } else {
                ++st.comparisons;
                if (less(pivot, a[i])) {
                    std::swap(a[i], a[gt]);
                    --gt;
                } else {
                    ++i;
                }
            }
        }

        if (lt - lo < hi - gt) {
            quickRec(a, lo, lt - 1, less, st, rule, rng, depth + 1);
            lo = gt + 1;
        } else {
            quickRec(a, gt + 1, hi, less, st, rule, rng, depth + 1);
            hi = lt - 1;
        }
        ++depth;
    }
}

template <typename T, typename KeyFn, typename K>
int binarySearchRec(const std::vector<T>& a, KeyFn key, const K& target, long long& probes, int lo, int hi) {
    if (lo > hi) return -1;                 // base case: empty range
    int mid = lo + (hi - lo) / 2;           // avoids (lo + hi) overflow
    ++probes;
    if (key(a[mid]) == target) return mid;
    if (key(a[mid]) < target) return binarySearchRec(a, key, target, probes, mid + 1, hi);
    return binarySearchRec(a, key, target, probes, lo, mid - 1);
}

}  // namespace dc_detail

// ---- Sorting ---------------------------------------------------------------

// Stable, Theta(n log n) in all cases, Theta(n) extra memory.
template <typename T, typename Less>
void mergeSort(std::vector<T>& a, Less less, SortStats& st) {
    if (a.size() < 2) return;
    std::vector<T> tmp(a.size());
    dc_detail::mergeRec(a, tmp, 0, (int)a.size(), less, st, 1);
}

// In-place, not stable. Expected Theta(n log n) with Random pivot.
template <typename T, typename Less>
void quickSort(std::vector<T>& a, Less less, SortStats& st, PivotRule rule = PivotRule::Random,
               unsigned seed = 2026u) {
    if (a.size() < 2) return;
    std::mt19937 rng(seed);
    dc_detail::quickRec(a, 0, (int)a.size() - 1, less, st, rule, rng, 1);
}

// ---- Searching (a must be sorted ascending by key) -------------------------

// Theta(log n) time, O(1) space. Returns an index with key == target, or -1.
template <typename T, typename KeyFn, typename K>
int binarySearchIterative(const std::vector<T>& a, KeyFn key, const K& target, long long& probes) {
    int lo = 0, hi = (int)a.size() - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        ++probes;
        if (key(a[mid]) == target) return mid;
        if (key(a[mid]) < target) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

// Theta(log n) time, O(log n) stack. Recurrence: T(n) = T(n/2) + O(1).
template <typename T, typename KeyFn, typename K>
int binarySearchRecursive(const std::vector<T>& a, KeyFn key, const K& target, long long& probes) {
    return dc_detail::binarySearchRec(a, key, target, probes, 0, (int)a.size() - 1);
}

// Baseline: Theta(n) worst case, works on unsorted data.
template <typename T, typename KeyFn, typename K>
int linearSearch(const std::vector<T>& a, KeyFn key, const K& target, long long& probes) {
    for (int i = 0; i < (int)a.size(); ++i) {
        ++probes;
        if (key(a[i]) == target) return i;
    }
    return -1;
}

// ---- Maximum subarray (e.g. best period of net borrowing growth) -----------

struct SubarrayResult {
    long long sum = 0;
    int left = -1;      // inclusive start index (-1 if input empty)
    int right = -1;     // inclusive end index
    long long ops = 0;
};

// Divide and conquer: T(n) = 2T(n/2) + Theta(n) => Theta(n log n).
SubarrayResult maxSubarrayDC(const std::vector<long long>& a);
// Kadane (dynamic programming / greedy scan): Theta(n).
SubarrayResult maxSubarrayKadane(const std::vector<long long>& a);

// ---- Fast exponentiation ---------------------------------------------------

// (base^exp) mod m by repeated squaring. T(e) = T(e/2) + O(1) => Theta(log e).
// Requires 1 <= m < 3,000,000,000 so products fit in 64-bit integers.
long long powMod(long long base, long long exp, long long mod, long long* multiplications = nullptr);

#endif
