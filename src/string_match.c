/* string_match.c - Four exact string matchers with operation counters. */
#include "string_match.h"

#include <stdlib.h>

#include "divide_conquer.h"   /* pow_mod (fast exponentiation) for Rabin-Karp */

void match_stats_init(MatchStats *st) {
    st->comparisons = 0;
    st->spurious_hits = 0;
    st->preprocessing = 0;
}

const char *matcher_name(Matcher m) {
    switch (m) {
        case MATCH_NAIVE: return "Naive";
        case MATCH_RABIN_KARP: return "Rabin-Karp";
        case MATCH_AUTOMATON: return "Finite automaton";
        case MATCH_KMP: return "KMP";
    }
    return "?";
}

static void report(IntVec *out, int pos, int *count) {
    if (out) intvec_push(out, pos);
    (*count)++;
}

int naive_search(const char *text, int n, const char *pat, int m, IntVec *out, MatchStats *st) {
    int s, j, count = 0;
    if (m <= 0 || m > n) return 0;
    for (s = 0; s <= n - m; ++s) {            /* try every shift */
        j = 0;
        while (j < m) {
            st->comparisons++;
            if (text[s + j] != pat[j]) break;
            ++j;
        }
        if (j == m) report(out, s, &count);
    }
    return count;
}

int rabin_karp_search(const char *text, int n, const char *pat, int m, IntVec *out, MatchStats *st, long long mod) {
    const long long base = 256;
    long long h, ph = 0, th = 0;
    int i, s, count = 0;
    if (m <= 0 || m > n || mod < 2 || mod >= 3000000000LL) return 0;

    h = pow_mod(base, m - 1, mod, NULL);      /* weight of the leading character */
    for (i = 0; i < m; ++i) {
        ph = (ph * base + (unsigned char)pat[i]) % mod;
        th = (th * base + (unsigned char)text[i]) % mod;
    }
    st->preprocessing += m;

    for (s = 0;; ++s) {
        st->comparisons++;                    /* one hash comparison per window */
        if (ph == th) {
            int j = 0;
            while (j < m) {                   /* verify, because different strings can share a hash */
                st->comparisons++;
                if (text[s + j] != pat[j]) break;
                ++j;
            }
            if (j == m) report(out, s, &count);
            else st->spurious_hits++;
        }
        if (s == n - m) break;
        /* Roll the window: remove text[s], append text[s+m]. O(1) per shift. */
        th = (th - (long long)(unsigned char)text[s] * h % mod + mod) % mod;
        th = (th * base + (unsigned char)text[s + m]) % mod;
    }
    return count;
}

int automaton_search(const char *text, int n, const char *pat, int m, IntVec *out, MatchStats *st) {
    const int A = 256;
    int *dfa, x = 0, q, c, i, count = 0;
    if (m <= 0 || m > n) return 0;

    /* dfa[q*A + c] = next state after reading c in state q (state = length of matched prefix). */
    dfa = (int *)xcalloc((size_t)(m + 1) * A, sizeof(int));
    dfa[(unsigned char)pat[0]] = 1;
    for (q = 1; q <= m; ++q) {
        /* x = restart state: where the automaton would be after reading pat[1..q-1] */
        for (c = 0; c < A; ++c) dfa[(size_t)q * A + c] = dfa[(size_t)x * A + c];   /* mismatch transitions */
        if (q < m) {
            dfa[(size_t)q * A + (unsigned char)pat[q]] = q + 1;                     /* match transition */
            x = dfa[(size_t)x * A + (unsigned char)pat[q]];
        }
    }
    st->preprocessing += (long long)(m + 1) * A;

    q = 0;
    for (i = 0; i < n; ++i) {
        st->comparisons++;                    /* exactly one transition per character, never backs up */
        q = dfa[(size_t)q * A + (unsigned char)text[i]];
        if (q == m) report(out, i - m + 1, &count);
    }
    free(dfa);
    return count;
}

void compute_prefix_function(const char *pat, int m, int *pi, long long *ops) {
    int k = 0, q;
    if (m <= 0) return;
    pi[0] = 0;
    for (q = 1; q < m; ++q) {
        while (k > 0 && pat[k] != pat[q]) {
            if (ops) (*ops)++;
            k = pi[k - 1];
        }
        if (ops) (*ops)++;
        if (pat[k] == pat[q]) ++k;
        pi[q] = k;
    }
}

int kmp_search(const char *text, int n, const char *pat, int m, IntVec *out, MatchStats *st) {
    int *pi, q = 0, i, count = 0;
    long long pre = 0;
    if (m <= 0 || m > n) return 0;
    pi = (int *)xmalloc((size_t)m * sizeof(int));
    compute_prefix_function(pat, m, pi, &pre);
    st->preprocessing += pre;

    for (i = 0; i < n; ++i) {                 /* q = pattern characters currently matched */
        while (q > 0 && pat[q] != text[i]) {  /* fall back with pi instead of re-reading text */
            st->comparisons++;
            q = pi[q - 1];
        }
        st->comparisons++;
        if (pat[q] == text[i]) ++q;
        if (q == m) {
            report(out, i - m + 1, &count);
            q = pi[q - 1];
        }
    }
    free(pi);
    return count;
}

int run_matcher(Matcher which, const char *text, int n, const char *pat, int m, IntVec *out, MatchStats *st) {
    switch (which) {
        case MATCH_NAIVE: return naive_search(text, n, pat, m, out, st);
        case MATCH_RABIN_KARP: return rabin_karp_search(text, n, pat, m, out, st, 1000000007LL);
        case MATCH_AUTOMATON: return automaton_search(text, n, pat, m, out, st);
        case MATCH_KMP: return kmp_search(text, n, pat, m, out, st);
    }
    return 0;
}
