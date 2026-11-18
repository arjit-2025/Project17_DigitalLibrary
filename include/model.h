/* model.h - Domain model for the Digital Library Search and Recommendation Engine.
 * Plain structs (typed records) so algorithms can be swapped and tested fairly. */
#ifndef MODEL_H
#define MODEL_H

#include "vec.h"

#define MAX_TITLE 96
#define MAX_AUTHOR 48
#define MAX_WORD 32
#define MAX_KEYWORDS 6
#define MAX_INTERESTS 6

/* A book / document. Invariant inside a Library: books[i].id == i. */
typedef struct {
    int id;
    char title[MAX_TITLE];
    char author[MAX_AUTHOR];
    int year;
    double rating;                           /* 0.0 .. 5.0 */
    int read_minutes;                        /* time to read (knapsack weight) */
    int num_keywords;
    char keywords[MAX_KEYWORDS][MAX_WORD];   /* subject terms (indexed) */
    char *abstract_text;                     /* heap-allocated free text */
} Book;

/* Edge of the citation graph: neighbour book id and distance weight (>= 0). */
typedef struct {
    int to;
    int weight;
} Edge;

/* Adjacency list of one vertex (growable array of edges). */
typedef struct {
    Edge *e;
    int size;
    int cap;
} EdgeList;

typedef struct {
    int id;
    char name[MAX_AUTHOR];
    IntVec history;                          /* ids of books already read */
    int num_interests;
    char interests[MAX_INTERESTS][MAX_WORD];
} User;

typedef struct {
    Book *books;
    int num_books;
    EdgeList *citations;   /* undirected weighted graph, one list per book */
    User *users;
    int num_users;
} Library;

void library_init(Library *lib);
void library_free(Library *lib);          /* frees everything and re-initialises */
void edgelist_push(EdgeList *l, int to, int weight);

/* ---- Types shared by the optimisation modules ---- */

/* Candidate book in a reading plan (0/1 knapsack item). */
typedef struct {
    int id;
    int weight;     /* minutes */
    double value;   /* interest score */
} Item;

typedef struct {
    const char *algorithm;
    double value;        /* total value of chosen items */
    int weight;          /* total minutes */
    IntVec chosen;       /* ids of chosen items */
    long long ops;       /* basic operations (cells / nodes / comparisons) */
    long long aux_bytes; /* extra memory estimate */
    int complete;        /* 0 if a search limit stopped an exact method early */
} KnapsackResult;

void knapsack_result_init(KnapsackResult *r, const char *algorithm);
void knapsack_result_free(KnapsackResult *r);

/* A reading-room booking [start, end) in minutes from midnight. */
typedef struct {
    int id;
    int start;
    int end;
} Interval;

#endif
