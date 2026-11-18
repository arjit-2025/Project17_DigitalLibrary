// timer.h - High-resolution wall-clock timer (QueryPerformanceCounter on Windows).
#ifndef TIMER_H
#define TIMER_H

class Timer {
public:
    Timer();
    void reset();
    double elapsedMs() const;

private:
    long long start_;
};

#endif
