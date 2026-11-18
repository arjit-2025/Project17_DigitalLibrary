# Digital Library Search and Recommendation Engine
**DAA Project 17** · C++17 · menu-driven CLI · no external libraries

An information-retrieval system for a digital library. It stores **books, keywords, readers, citations and reading histories** and supports **fast search, similarity analysis and recommendations**. Every feature is built on a hand-written DAA algorithm, and each one is benchmarked against an alternative strategy on the same input.

---

## 1. Build and run

Requirements: `g++` with C++17 support (tested with MinGW g++ 6.3 on Windows).

```bash
mingw32-make          # Windows (on Linux/macOS: make)
```
or, without make:
```bash
build.bat
```

| Command | What it does |
|---|---|
| `library.exe` | Interactive menu (starts with a 500-book demo library already loaded) |
| `library.exe --scenario 1` | Scenario 1: normal operation |
| `library.exe --scenario 2` | Scenario 2: capacity / constraint shock |
| `library.exe --scenario 3` | Scenario 3: large-data stress (100,000 books) |
| `library.exe --scenario all` | All three scenarios |
| `library.exe --bench` | Full benchmark, exported to `results/benchmark.csv` |
| `run_tests.exe` | Test suite (4,700+ checks) |
| `demo.bat` | Runs the 3 scenarios one after another with pauses |

---

## 2. Project structure

```
include/  model.h            Book, User, Library, Item, Interval, KnapsackResult
          divide_conquer.h   merge sort, quick sort, binary search, max subarray, fast power
          string_match.h     naive, Rabin-Karp, finite automaton, KMP
          greedy.h           greedy knapsack, fractional knapsack, activity selection,
                             interval partitioning, Huffman
          dp.h               0/1 knapsack DP, LCS
          backtrack.h        0/1 knapsack backtracking with branch-and-bound
          graph.h            Dijkstra, maximum bipartite matching
          library_engine.h   inverted index, search, similarity, recommendation
          io.h               CSV load/save (persistence layer)
          bench.h            benchmark runner
          scenarios.h        the 3 demonstration scenarios
          datagen.h, timer.h
src/      one .cpp per header + main.cpp (CLI)
tests/    test_main.cpp      unit, oracle, edge-case and integration tests
data/     library_*.csv      sample dataset (500 books, 20 readers)
results/  benchmark.csv, stress_scenario.csv, recommendations.csv
docs/     VIVA_NOTES.md      complexity, recurrences, correctness, reduction, likely questions
```

The layers are kept separate so that algorithms can be swapped fairly: **domain model → algorithm modules → engine → experiment runner / CLI → persistence**.

---

## 3. Feature → algorithm map

| Library feature | Algorithm(s) | Paradigm | Time | Extra space |
|---|---|---|---|---|
| Order catalogue (rating / year / title) | Merge sort | Divide & conquer | Θ(n log n) all cases | Θ(n) |
| | Quick sort, 3-way partition (random / median-of-3 / last pivot) | Divide & conquer | avg Θ(n log n), worst Θ(n²) | O(log n) stack |
| Find a book by id | Binary search, iterative and recursive | Divide & conquer | Θ(log n) | O(1) / O(log n) |
| | Linear search (baseline) | Brute force | Θ(n) | O(1) |
| Keyword search | Inverted index in a balanced BST (`std::map`) + sorted-list intersection + merge-sort ranking | Data structure + merging | O(q log T + Σ postings + r log r) | Θ(index) |
| Phrase search in abstracts | Naive | Brute force | O((n−m+1)·m) | O(1) |
| | Rabin-Karp (rolling hash; uses fast power) | Hashing | expected Θ(n+m), worst Θ(nm) | O(1) |
| | Finite automaton | Automata | Θ(m·Σ) preprocessing + Θ(n) | Θ(m·Σ) |
| | Knuth-Morris-Pratt | Prefix function | Θ(m) + Θ(n) | Θ(m) |
| Similar books | LCS of abstract word sequences (2-row DP) + keyword Jaccard | Dynamic programming | Θ(n·m) per pair | Θ(min(n,m)) |
| Recommendations | Multi-source Dijkstra on the citation graph + interest overlap + rating, ranked by merge sort | Greedy (graph) | Θ((V+E) log V) + Θ(V log V) | Θ(V) |
| Reading plan under a time budget | 0/1 knapsack DP | Dynamic programming | Θ(n·W) pseudo-polynomial | Θ(n·W) bits + Θ(W) |
| | Greedy by value-per-minute | Greedy (heuristic) | Θ(n log n) | Θ(n) |
| | Backtracking + fractional bound | Branch and bound | worst Θ(2ⁿ) | Θ(n) |
| | Fractional knapsack (upper bound) | Greedy (exact for fractions) | Θ(n log n) | Θ(n) |
| Citation route between two books | Dijkstra with a binary heap | Greedy | Θ((V+E) log V) | Θ(V) |
| Assign single-copy books to readers | Maximum bipartite matching (augmenting paths, Ford-Fulkerson with unit capacities) | Network flow | O(V·E) | Θ(V) |
| | First-come-first-served (baseline) | Greedy | O(E) | Θ(V) |
| Reading-room bookings | Activity selection (earliest finish) | Greedy, optimal | Θ(n log n) | Θ(n) |
| | Interval partitioning (minimum rooms, min-heap) | Greedy, optimal | Θ(n log n) | Θ(n) |
| Best borrowing-growth period | Maximum subarray, divide & conquer vs Kadane | D&C vs DP | Θ(n log n) vs Θ(n) | O(log n) / O(1) |
| Archive compression | Huffman coding | Greedy | Θ(N + k log k) | Θ(k) |

---

## 4. Demonstration scenarios (the minimum demonstration requirement)

Every scenario uses a fixed seed, so it gives the same output on every run.

**Scenario 1: normal operation** (500 books, 20 readers): sorting, lookup, keyword and phrase search, similar books, recommendations, a reading plan, a citation route, trend analysis and compression. Each step compares two or more algorithms on the same input.

**Scenario 2: capacity / constraint shock**
- *Budget shock:* a reader's time budget drops 240 → 90 → 45 minutes. At 45 minutes, greedy falls to 96.2% of the optimum while DP and B&B stay optimal. The textbook case shows greedy at 72.7% (160 vs 220).
- *Demand surge:* 60 readers want 20 single-copy titles. First-come-first-served serves 19; maximum matching serves 20, the maximum possible.
- *Room closure:* 30 bookings need 7 rooms. With 1 room, earliest-start accepts 5 bookings and earliest-finish accepts 6, which is optimal.

**Scenario 3: large-data stress** (100,000 books, ~200,000 citations, 2,000 readers). Results are exported to `results/stress_scenario.csv`.
- Sorting 100k books: merge sort 1.5M comparisons; 3-way quick sort 0.96M (only 41 distinct ratings).
- Adversarial sorted input (20k): random pivot ≈ 0.5M comparisons; last pivot = **200M comparisons**, recursion depth 19,999 → Θ(n²).
- 1,000 lookups: linear 74M probes vs binary 16K probes.
- 0/1 knapsack with 1,000 candidates: DP is optimal in about 1 ms, greedy reaches 99.4%, B&B is optimal.

Results measured on the development machine (Windows, g++ 6.3 -O2). Times vary between machines; operation counts do not.

---

## 5. Testing

`run_tests.exe` checks every module against an independent **oracle**:
- sorting vs `std::sort` (empty, single element, all-equal, sorted, reversed, duplicates) and merge-sort stability
- all string matchers vs the naive matcher on 300 random texts, including Rabin-Karp with modulus 3 (forced collisions)
- knapsack DP and B&B vs brute force over all 2ⁿ subsets; greedy ≤ optimum ≤ fractional bound
- Dijkstra vs Floyd-Warshall on random graphs; matching vs brute force
- maximum subarray vs O(n²) brute force; fast power vs repeated multiplication
- invalid inputs: empty pattern, unknown ids, reversed intervals, missing files
- CSV save → load round trip

---

## 6. Scope and limitations

- The dataset is synthetic but deterministic, generated by `datagen.cpp`.
- The "memory" column in the benchmarks is an analytical estimate of auxiliary memory, not measured RSS.
- B&B has a node limit (it reports "limit hit" instead of running forever on hard instances).
- Recommendation weights (0.5 topic, 0.3 citation, 0.2 rating) are a design choice, not learned.
- Not implemented yet (other syllabus items from the handbook): Strassen matrix multiplication, celebrity problem, Prim/Kruskal, matrix-chain, optimal BST, Bellman-Ford/Floyd-Warshall as features (Floyd-Warshall is used only as a test oracle), TSP, N-Queens, FIFO/LIFO/least-cost B&B variants, CRT.
- Git is not installed on the development machine. To meet the repository requirement, install Git, then run `git init`, commit, and tag a release (`git tag v1.0`).
