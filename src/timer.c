/* timer.c */
#include "timer.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static long long now_ticks(void) {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}
static double ticks_per_ms(void) {
    static double f = 0;
    if (f == 0) {
        LARGE_INTEGER q;
        QueryPerformanceFrequency(&q);
        f = q.QuadPart / 1000.0;
    }
    return f;
}
#else
#define _POSIX_C_SOURCE 199309L
#include <time.h>
static long long now_ticks(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}
static double ticks_per_ms(void) { return 1e6; }
#endif

void timer_reset(Timer *t) { t->start = now_ticks(); }
double timer_ms(const Timer *t) { return (now_ticks() - t->start) / ticks_per_ms(); }
