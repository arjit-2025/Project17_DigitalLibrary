/* timer.h - High-resolution wall-clock timer (QueryPerformanceCounter on Windows). */
#ifndef TIMER_H
#define TIMER_H

typedef struct {
    long long start;
} Timer;

void timer_reset(Timer *t);
double timer_ms(const Timer *t);   /* milliseconds since the last reset */

#endif
