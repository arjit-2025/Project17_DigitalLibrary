/* library_engine.h - The search & recommendation engine that ties the algorithms to the domain.
 *   * inverted index: sorted array of (term -> sorted book ids), BUILT with merge sort and
 *     QUERIED with binary search
 *   * keyword search  = index lookups + sorted-list intersection + merge-sort ranking
 *   * phrase search   = any of the four string matchers over abstracts
 *   * similar books   = LCS similarity of abstracts + keyword Jaccard
 *   * recommendations = interest overlap + citation proximity (multi-source Dijkstra) + rating */
#ifndef LIBRARY_ENGINE_H
#define LIBRARY_ENGINE_H

#include "model.h"
#include "string_match.h"

typedef struct {
    char word[MAX_WORD];
    int *ids;     /* sorted ascending, no duplicates */
    int count;
} Posting;

typedef struct {
    const Library *lib;        /* not owned: rebuild the engine whenever the library changes */
    Posting *terms;            /* sorted by word */
    int num_terms;
    char **lowered_abstracts;  /* lower-case copy of every abstract */
} SearchEngine;

typedef struct {
    int book_id;
    double score;
} ScoredBook;

typedef struct {
    int book_id;
    double score;                 /* 0.5*topic + 0.3*graph + 0.2*rating */
    double topic_score;           /* fraction of the reader's interests covered by the book */
    double graph_score;           /* 1 / (1 + distance/5) to the nearest book already read */
    double rating_score;          /* rating / 5 */
    long long citation_distance;
} Recommendation;

void engine_build(SearchEngine *e, const Library *lib);
void engine_free(SearchEngine *e);

/* AND query over keywords / title words; results ranked by rating (desc). Returns count. */
int engine_keyword_search(const SearchEngine *e, const char *query, IntVec *out, long long *ops);

/* Book ids whose abstract contains the phrase (case-insensitive). Returns count. */
int engine_phrase_search(const SearchEngine *e, const char *phrase, Matcher m, IntVec *out, MatchStats *st);

/* Top-k most similar books; out needs room for k. Returns count written. */
int engine_similar_books(const SearchEngine *e, int book_id, int k, ScoredBook *out, long long *cells);

/* Top-k recommendations for a reader; out needs room for k. Returns count written. */
int engine_recommend(const SearchEngine *e, int user_id, int k, Recommendation *out);

/* Top recommendations as knapsack items (weight = minutes, value = score*100). out needs max_items. */
int engine_reading_candidates(const SearchEngine *e, int user_id, int max_items, Item *out);

/* Lower-case copy (caller frees). */
char *to_lower_copy(const char *s);

#endif
