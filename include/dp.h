/* dp.h - Dynamic programming: 0/1 knapsack (reading plan) and LCS (document similarity). */
#ifndef DP_H
#define DP_H

#include "model.h"

/* Exact 0/1 knapsack. Theta(n*W) time (pseudo-polynomial), Theta(W) values + n*W decision bytes.
 * best[w] = best value using the items seen so far within w minutes. */
KnapsackResult knapsack_dp(const Item *items, int n, int capacity);

/* A text split into lower-case words (letters/digits only). */
typedef struct {
    char **words;
    int count;
} Tokens;

void tokenize(const char *s, Tokens *out);
void tokens_free(Tokens *t);

typedef struct {
    int length;
    const char **sequence;   /* one longest common subsequence (points into a's words) */
    long long cells;         /* table cells filled */
} LcsResult;

/* Full table with reconstruction. Theta(n*m) time and space. Free with lcs_result_free. */
LcsResult lcs_tokens(const Tokens *a, const Tokens *b);
void lcs_result_free(LcsResult *r);

/* Length only with two rolling rows. Theta(n*m) time, Theta(min(n,m)) space. */
int lcs_length(const Tokens *a, const Tokens *b, long long *cells);

/* 2*LCS / (|a| + |b|), in [0, 1]. */
double lcs_similarity(const Tokens *a, const Tokens *b, long long *cells);

#endif
