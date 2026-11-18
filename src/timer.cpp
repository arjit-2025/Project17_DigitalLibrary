// timer.cpp
#include "timer.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static long long nowTicks() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}
static double ticksPerMs() {
    static double f = 0;
    if (f == 0) {
        LARGE_INTEGER q;
        QueryPerformanceFrequency(&q);
        f = q.QuadPart / 1000.0;
    }
    return f;
}
#else
#include <chrono>
static long long nowTicks() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
static double ticksPerMs() { return 1e6; }
#endif

Timer::Timer() { reset(); }
void Timer::reset() { start_ = nowTicks(); }
double Timer::elapsedMs() const { return (nowTicks() - start_) / ticksPerMs(); }
