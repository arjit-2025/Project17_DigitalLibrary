/* datagen.h - Deterministic synthetic dataset generator.
 * The same (n_books, n_users, seed) always produces the same library. */
#ifndef DATAGEN_H
#define DATAGEN_H

#include "model.h"

/* Frees whatever lib held before and fills it with a new library. */
void generate_library(Library *lib, int n_books, int n_users, unsigned long long seed);

#endif
