#pragma once

#define ENABLE_PERFORMANCE_COUNTER 0

class TPerformanceCounter
{
#if ENABLE_PERFORMANCE_COUNTER
    double frequency;
    LONGLONG timer;
    LARGE_INTEGER tmp_timer;
    int counter_for_timer;
    ffstring msg;
#endif

public:
    TPerformanceCounter(const wchar_t *msg);
    void start();
    void stop();
};

class TautoPerformanceCounter
{
#if ENABLE_PERFORMANCE_COUNTER
    TPerformanceCounter *performanceCounter;
#endif

public:
    TautoPerformanceCounter(TPerformanceCounter *performanceCounter);

#if ENABLE_PERFORMANCE_COUNTER
    ~TautoPerformanceCounter();
#endif
};
