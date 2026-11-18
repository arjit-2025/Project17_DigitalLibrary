/* rng.c - SplitMix64 */
#include "rng.h"

void rng_seed(Rng *r, unsigned long long seed) { r->state = seed; }

unsigned long long rng_next(Rng *r) {
    unsigned long long z = (r->state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

int rng_range(Rng *r, int lo, int hi) {
    unsigned long long span;
    if (hi <= lo) return lo;
    span = (unsigned long long)((long long)hi - (long long)lo + 1);
    return lo + (int)(rng_next(r) % span);
}
