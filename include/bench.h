/* bench.h - Benchmark runner: time, operation counts, memory estimate and quality on
 * small, medium, large and adversarial datasets. Results can be exported to CSV. */
#ifndef BENCH_H
#define BENCH_H

typedef struct {
    char experiment[64];
    char dataset[48];
    char algorithm[48];
    long long n;
    double ms;
    long long ops;
    long long aux_bytes;
    char note[96];
} BenchRow;

typedef struct {
    BenchRow *rows;
    int size;
    int cap;
} BenchTable;

void bench_init(BenchTable *t);
void bench_free(BenchTable *t);
void bench_add(BenchTable *t, const char *experiment, const char *dataset, const char *algorithm, long long n,
               double ms, long long ops, long long aux_bytes, const char *note);

void run_benchmarks(BenchTable *t);
void print_bench_table(const BenchTable *t);
int write_bench_csv(const BenchTable *t, const char *path);

#endif
