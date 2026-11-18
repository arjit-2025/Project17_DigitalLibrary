/* vec.c */
#include "vec.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void out_of_memory(void) {
    fprintf(stderr, "Fatal: out of memory\n");
    exit(1);
}

void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) out_of_memory();
    return p;
}

void *xcalloc(size_t count, size_t size) {
    void *p = calloc(count ? count : 1, size ? size : 1);
    if (!p) out_of_memory();
    return p;
}

void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n ? n : 1);
    if (!q) out_of_memory();
    return q;
}

char *xstrdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *)xmalloc(n);
    memcpy(p, s, n);
    return p;
}

void str_copy(char *dst, size_t cap, const char *src) {
    size_t n;
    if (cap == 0) return;
    n = strlen(src);
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

void intvec_init(IntVec *v) {
    v->data = NULL;
    v->size = 0;
    v->cap = 0;
}

void intvec_push(IntVec *v, int x) {
    if (v->size == v->cap) {
        v->cap = v->cap ? v->cap * 2 : 8;
        v->data = (int *)xrealloc(v->data, (size_t)v->cap * sizeof(int));
    }
    v->data[v->size++] = x;
}

void intvec_clear(IntVec *v) { v->size = 0; }

void intvec_free(IntVec *v) {
    free(v->data);
    intvec_init(v);
}

int intvec_equal(const IntVec *a, const IntVec *b) {
    int i;
    if (a->size != b->size) return 0;
    for (i = 0; i < a->size; ++i)
        if (a->data[i] != b->data[i]) return 0;
    return 1;
}
