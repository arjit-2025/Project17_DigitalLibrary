/* string_match.h - Exact pattern matching: naive, Rabin-Karp, finite automaton, KMP.
 * Used for phrase search inside book abstracts.
 * Each function finds every (possibly overlapping) occurrence of pat in text,
 * appends the 0-based start positions to out (if out is not NULL) and returns the count. */
#ifndef STRING_MATCH_H
#define STRING_MATCH_H

#include "vec.h"

typedef struct {
    long long comparisons;     /* character comparisons / hash checks / DFA transitions */
    long long spurious_hits;   /* Rabin-Karp: hash matched but text did not */
    long long preprocessing;   /* work done before scanning the text */
} MatchStats;

typedef enum { MATCH_NAIVE, MATCH_RABIN_KARP, MATCH_AUTOMATON, MATCH_KMP } Matcher;

void match_stats_init(MatchStats *st);
const char *matcher_name(Matcher m);

/* Theta((n-m+1)*m) worst case, O(1) extra space. */
int naive_search(const char *text, int n, const char *pat, int m, IntVec *out, MatchStats *st);

/* Rolling hash. Expected Theta(n+m), worst Theta(n*m) with many collisions.
 * mod must be in [2, 3e9). A tiny mod (e.g. 101) demonstrates spurious hits. */
int rabin_karp_search(const char *text, int n, const char *pat, int m, IntVec *out, MatchStats *st, long long mod);

/* DFA over 256 symbols. Preprocessing Theta(m * 256), scan exactly n transitions. */
int automaton_search(const char *text, int n, const char *pat, int m, IntVec *out, MatchStats *st);

/* Knuth-Morris-Pratt. Preprocessing Theta(m), scan Theta(n), at most 2n comparisons. */
int kmp_search(const char *text, int n, const char *pat, int m, IntVec *out, MatchStats *st);

/* pi[q] = length of the longest proper prefix of pat[0..q] that is also a suffix of it.
 * pi must have room for m ints. */
void compute_prefix_function(const char *pat, int m, int *pi, long long *ops);

int run_matcher(Matcher which, const char *text, int n, const char *pat, int m, IntVec *out, MatchStats *st);

#endif
