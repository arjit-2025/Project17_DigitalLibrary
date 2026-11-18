// io.h - Persistence layer: save/load the library as CSV files, create output folders.
//   <prefix>_books.csv      id,title,author,year,rating,readMinutes,keywords(;),abstract
//   <prefix>_citations.csv  from,to,weight          (optional on load)
//   <prefix>_users.csv      id,name,interests(;),history(;)   (optional on load)
#ifndef IO_H
#define IO_H

#include <string>

#include "model.h"

bool saveLibrary(const Library& lib, const std::string& prefix, std::string& error);

// Invalid rows are skipped (counted in skippedRows); ids are renumbered densely 0..n-1.
// On failure the given library is left unchanged.
bool loadLibrary(Library& lib, const std::string& prefix, std::string& error, int& skippedRows);

bool ensureDirectory(const std::string& path);

#endif
