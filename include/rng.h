/* rng.h - Deterministic pseudo-random generator (SplitMix64).
 * Same seed -> same sequence on every compiler and OS, so datasets and
 * demonstrations are reproducible (rand() differs between platforms). */
#ifndef RNG_H
#define RNG_H

typedef struct {
    unsigned long long state;
} Rng;

void rng_seed(Rng *r, unsigned long long seed);
unsigned long long rng_next(Rng *r);
/* Uniform-ish integer in [lo, hi] (inclusive). */
int rng_range(Rng *r, int lo, int hi);

#endif
