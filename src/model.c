/* model.c - Construction and clean-up of the domain records. */
#include "model.h"

#include <stdlib.h>

void library_init(Library *lib) {
    lib->books = NULL;
    lib->num_books = 0;
    lib->citations = NULL;
    lib->users = NULL;
    lib->num_users = 0;
}

void library_free(Library *lib) {
    int i;
    for (i = 0; i < lib->num_books; ++i) free(lib->books[i].abstract_text);
    if (lib->citations)
        for (i = 0; i < lib->num_books; ++i) free(lib->citations[i].e);
    for (i = 0; i < lib->num_users; ++i) intvec_free(&lib->users[i].history);
    free(lib->books);
    free(lib->citations);
    free(lib->users);
    library_init(lib);
}

void edgelist_push(EdgeList *l, int to, int weight) {
    if (l->size == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 4;
        l->e = (Edge *)xrealloc(l->e, (size_t)l->cap * sizeof(Edge));
    }
    l->e[l->size].to = to;
    l->e[l->size].weight = weight;
    l->size++;
}

void knapsack_result_init(KnapsackResult *r, const char *algorithm) {
    r->algorithm = algorithm;
    r->value = 0.0;
    r->weight = 0;
    intvec_init(&r->chosen);
    r->ops = 0;
    r->aux_bytes = 0;
    r->complete = 1;
}

void knapsack_result_free(KnapsackResult *r) { intvec_free(&r->chosen); }
