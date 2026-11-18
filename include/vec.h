/* vec.h - Small utilities that C does not provide: checked allocation,
 * a growable int array (IntVec) and safe string copy. */
#ifndef VEC_H
#define VEC_H

#include <stddef.h>

/* malloc/calloc/realloc that print an error and exit instead of returning NULL. */
void *xmalloc(size_t n);
void *xcalloc(size_t count, size_t size);
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);

/* Copies src into dst (capacity cap bytes), truncating and always NUL-terminating. */
void str_copy(char *dst, size_t cap, const char *src);

/* Growable array of int. Amortised O(1) push (capacity doubles when full). */
typedef struct {
    int *data;
    int size;
    int cap;
} IntVec;

void intvec_init(IntVec *v);
void intvec_push(IntVec *v, int x);
void intvec_clear(IntVec *v);
void intvec_free(IntVec *v);
int intvec_equal(const IntVec *a, const IntVec *b);

#endif
