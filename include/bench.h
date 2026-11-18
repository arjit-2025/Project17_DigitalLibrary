// bench.h - Benchmark runner: time, operation counts, memory estimate and quality
// on small, medium, large and adversarial datasets. Results can be exported to CSV.
#ifndef BENCH_H
#define BENCH_H

#include <iosfwd>
#include <string>
#include <vector>

struct BenchRow {
    std::string experiment;
    std::string dataset;
    std::string algorithm;
    long long n;
    double ms;
    long long ops;
    long long auxBytes;
    std::string note;
};

BenchRow makeRow(const std::string& experiment, const std::string& dataset, const std::string& algorithm,
                 long long n, double ms, long long ops, long long auxBytes, const std::string& note);

std::vector<BenchRow> runBenchmarks(std::ostream& log);
void printBenchTable(const std::vector<BenchRow>& rows, std::ostream& out);
bool writeBenchCSV(const std::vector<BenchRow>& rows, const std::string& path);

std::string fmtNum(double x, int precision);

#endif
