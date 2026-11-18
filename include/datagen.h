// datagen.h - Deterministic synthetic dataset generator.
// The same (nBooks, nUsers, seed) always produces the same library, so
// benchmarks and demonstrations are reproducible.
#ifndef DATAGEN_H
#define DATAGEN_H

#include "model.h"

void generateLibrary(Library& lib, int nBooks, int nUsers, unsigned seed);

#endif
