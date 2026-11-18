/* io.h - Persistence layer: save/load the library as CSV files, create folders.
 *   <prefix>_books.csv      id,title,author,year,rating,readMinutes,keywords(;),abstract
 *   <prefix>_citations.csv  from,to,weight                    (optional on load)
 *   <prefix>_users.csv      id,name,interests(;),history(;)   (optional on load) */
#ifndef IO_H
#define IO_H

#include <stddef.h>

#include "model.h"

/* Return 1 on success, 0 on failure (message in error). */
int save_library(const Library *lib, const char *prefix, char *error, size_t error_cap);

/* Invalid rows are skipped and counted; ids are renumbered densely 0..n-1.
 * On failure lib is left unchanged. */
int load_library(Library *lib, const char *prefix, char *error, size_t error_cap, int *skipped_rows);

int ensure_directory(const char *path);

#endif
