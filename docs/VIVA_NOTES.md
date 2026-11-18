# Viva Notes: Digital Library Engine (Project 17, C version)

Format: 8-minute demo + 4-minute viva, solo. Examiners care about *why* an algorithm works, *when* to use it, and its *complexity*, more than the UI.

---

## 1. 8-minute demo script

| Time | What to show | Command |
|---|---|---|
| 0:00–0:45 | Problem: a library needs fast search, similar-book discovery and personalised recommendations under limits (time, copies, rooms). Show the feature → algorithm table in the README. | — |
| 0:45–3:00 | **Scenario 1**: sorting (merge vs quick, same order), binary 9 probes vs linear 322, four matchers agreeing, recommendations with explanation, the reading plan (greedy 99.4%). | `.\library.exe --scenario 1` |
| 3:00–5:30 | **Scenario 2**: greedy drops to 96.9% / 72.7% while DP stays optimal; matching 18 vs first-come 17; activity selection 7 vs 6. | `.\library.exe --scenario 2` |
| 5:30–7:30 | **Scenario 3**: 100k books; last-pivot quick sort makes 200M comparisons vs 0.37M (Θ(n²) vs Θ(n log n)); linear 73.7M vs binary 16K probes. | `.\library.exe --scenario 3` |
| 7:30–8:00 | Tests: `.\run_tests.exe` → 4,887 checks, 0 failed. Conclusion: which algorithm to use when. | `.\run_tests.exe` |

Be ready for the examiner to ask you to **run a new input**. Use the menu (`.\library.exe`): option 10 (reading plan with a custom budget), 7 (phrase search with any phrase), 5 (find any id).

---

## 2. C-specific design questions

- **How is one merge sort used for Books, Items, Intervals and ints?** Like the C library's `qsort`: it receives `void *base`, the element count, the element size, and a comparison function `int cmp(const void *a, const void *b, void *ctx)`. Elements are moved with `memcpy`. The extra `ctx` pointer passes data to the comparison, e.g. the library when ranking book ids by rating.
- **Why a three-way comparison (<0, 0, >0)?** One call tells quick sort whether an element is smaller than, equal to, or larger than the pivot, which is exactly what the 3-way partition needs.
- **Where is the priority queue?** `heap.c`: an array-based binary min-heap (children of i are 2i+1 and 2i+2). push and pop are O(log n): sift up / sift down. Used by Dijkstra, interval partitioning and Huffman.
- **How is the inverted index built without a map?** Collect every (word, book id) pair, sort by word with merge sort, then group equal words. Books are added in id order and **merge sort is stable**, so each word's ids come out already sorted. Lookups use binary search over the sorted words.
- **Why your own random generator (SplitMix64)?** `rand()` gives different sequences on different compilers, so a demo could print different numbers on another machine. SplitMix64 gives the same sequence everywhere.
- **Memory management:** every structure that allocates has a matching `_free` function (`library_free`, `knapsack_result_free`, `path_result_free`, ...). `xmalloc` stops the program with a message if memory runs out, instead of returning NULL.

---

## 3. Recurrences (solve them out loud)

| Algorithm | Recurrence | Method | Solution |
|---|---|---|---|
| Merge sort | T(n) = 2T(n/2) + cn | Master theorem, case 2 (a=2, b=2, f(n)=Θ(n^log_b a)) | Θ(n log n) |
| Binary search | T(n) = T(n/2) + c | Master theorem, case 2 (a=1, b=2) | Θ(log n) |
| Quick sort, balanced split | T(n) = 2T(n/2) + cn | Master theorem | Θ(n log n) |
| Quick sort, worst split | T(n) = T(n−1) + cn | Iteration: cn + c(n−1) + … = c·n(n+1)/2 | Θ(n²) |
| Max subarray (D&C) | T(n) = 2T(n/2) + cn | Recursion tree: log n levels × cn per level | Θ(n log n) |
| Fast power | T(e) = T(e/2) + c | Master theorem | Θ(log e) |
| Heap push / pop | one swap per level of a tree of height log n | — | Θ(log n) |
| Backtracking knapsack | T(n) = 2T(n−1) + c | Iteration | O(2ⁿ) |

**When the Master theorem does not apply:** T(n) = T(n−1) + n (subtracting, not dividing), T(n) = 2T(n−1) + 1, or T(n) = 2T(n/2) + n/log n (falls in the gap between the cases).

---

## 4. Why each algorithm is correct (one line each)

- **Merge sort:** by induction, both halves come back sorted, and merging two sorted lists by repeatedly taking the smaller head gives a sorted list. It is stable because ties take from the left half.
- **3-way quick sort:** loop invariant `a[lo..lt-1] < pivot`, `a[lt..i-1] == pivot`, `a[gt+1..hi] > pivot`. The pivot block is already in its final place.
- **Binary search:** invariant: if the target exists, it lies in `[lo, hi]`. Each step halves the range.
- **Min-heap:** heap property: every parent ≤ its children, so the root is the minimum. Sift-up and sift-down restore the property along one root-to-leaf path.
- **KMP:** π[q] is the longest proper prefix of the pattern that is also a suffix of `pat[0..q]`. On a mismatch, shifting by π never skips a possible match. The text pointer never moves back, so at most 2n comparisons.
- **Rabin-Karp:** equal strings have equal hashes. Unequal hashes ⇒ not equal. Equal hashes are verified character by character (spurious hits).
- **Finite automaton:** state = length of the longest pattern prefix that is a suffix of the text read so far.
- **0/1 knapsack DP:** optimal substructure: `best(i, w) = max(best(i−1, w), best(i−1, w−wᵢ) + vᵢ)`. Iterating w downwards in a 1-D array ensures each item is used at most once.
- **Greedy knapsack fails for 0/1:** counter-example (10, 60), (20, 100), (30, 120) with W = 50: greedy gets 160, the optimum is 220. Greedy is optimal only for the **fractional** version (exchange argument).
- **Branch and bound:** the fractional-knapsack value of the remaining items is an upper bound. If bound ≤ best found, that subtree cannot contain a better answer.
- **Activity selection:** exchange argument: the activity that finishes first can replace the first activity of any optimal solution, so the greedy choice is always safe.
- **Interval partitioning:** the number of rooms used equals the maximum number of overlapping bookings (the depth), which is a lower bound for any schedule.
- **Dijkstra:** with non-negative weights, the vertex with the smallest tentative distance is final, because any other path to it goes through a vertex that is already at least as far. **Fails with negative edges** (use Bellman-Ford).
- **Bipartite matching:** Berge's theorem: a matching is maximum ⇔ there is no augmenting path. Each augmentation adds 1. This is Ford-Fulkerson on a unit-capacity network (source → readers → copies → sink).
- **Huffman:** greedy choice: the two least frequent symbols can be siblings at the deepest level of some optimal tree. Repeat on the reduced alphabet.
- **LCS:** `L[i][j] = L[i−1][j−1] + 1` if the words match, otherwise `max(L[i−1][j], L[i][j−1])`.

---

## 5. Complexity classes and reduction

- Sorting, searching, string matching, LCS, shortest paths, bipartite matching, activity selection: **P**.
- **0/1 knapsack (decision version):** "is there a subset with weight ≤ W and value ≥ V?" is **NP-complete**.
  - *In NP:* given a subset, check its total weight and value in O(n).
  - *NP-hard by reduction from Subset-Sum:* given numbers s₁…sₙ and target t, build items with wᵢ = vᵢ = sᵢ, W = t, V = t. A subset summing to exactly t exists ⇔ the knapsack instance answers yes. The reduction takes polynomial time.
- **Why the DP is not a contradiction:** Θ(n·W) is **pseudo-polynomial**. W is a number, and its input size is only log W bits, so n·W is exponential in the input length. It is fast here because budgets are small (minutes).
- The optimisation version (maximise value) is **NP-hard**.

---

## 6. Likely viva questions (short answers)

1. **Merge sort or quick sort?** Quick sort is faster in practice: in-place, and our 3-way version handles the 41 repeated rating values well (0.62M vs 1.53M comparisons on 100k books). Merge sort guarantees Θ(n log n) and is stable, so use it when ties must keep their order, as in building the index.
2. **How do you avoid the quick-sort worst case?** Random or median-of-3 pivot. Recursing on the smaller side keeps the stack at O(log n).
3. **Why `mid = lo + (hi − lo)/2`?** `lo + hi` can overflow `int`.
4. **When does Rabin-Karp beat KMP?** Searching for many patterns at once (compare one hash against a set). Its weakness is collisions: with mod 101 the benchmark shows 2,012 spurious hits, with mod 10⁹+7 none.
5. **Why is the automaton slow in scenario 3?** It rebuilds a (m+1)×256 table for every one of the 100k abstracts. Preprocessing dominates on short texts. Build it once and reuse it.
6. **Space of the knapsack DP?** Θ(W) for the values (one row reused) plus n·W bytes of "take" flags to rebuild the chosen set. If only the value is needed, Θ(W).
7. **Why Dijkstra for recommendations?** The citation distance from the nearest book the reader already read measures relatedness. One multi-source run computes it for all books in Θ((V+E) log V).
8. **What if the graph had negative weights?** Dijkstra could be wrong. Use Bellman-Ford, Θ(VE), which also detects negative cycles.
9. **Why matching instead of first-come-first-served?** FCFS can block: A takes X when A could have taken Y, leaving B with nothing. Augmenting paths re-route A.
10. **Huffman vs fixed-length codes?** Fixed: 8 bits per char. Huffman: ~4.2 bits per char on our abstracts (53%), because frequent letters get short codes. It is a prefix code, so decoding is unambiguous.
11. **How does keyword search find a word fast?** The index is a sorted array of words, so binary search finds one in O(log T). Posting lists are sorted, so an AND query is a linear two-pointer merge.
12. **What did you test, and how?** Each algorithm against an independent oracle (`qsort`, brute force, Floyd-Warshall, the naive matcher) on random and edge-case inputs: 4,887 checks.
