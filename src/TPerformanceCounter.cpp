/*
 * Copyright (c) 2012 h.yamagata
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stdafx.h"
#include "TPerformanceCounter.h"

//================================= TPerformanceCounter =================================
TPerformanceCounter::TPerformanceCounter(const wchar_t *msg)
{
#if ENABLE_PERFORMANCE_COUNTER
    timer = 0;
    counter_for_timer = 0;
    if (msg) {
        this->msg = msg;
    }
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    frequency = f.QuadPart / 1000.0;
#endif
}

void TPerformanceCounter::start()
{
#if ENABLE_PERFORMANCE_COUNTER
    QueryPerformanceCounter(&tmp_timer);
#endif
}

void TPerformanceCounter::stop()
{
#if ENABLE_PERFORMANCE_COUNTER
    int64_t t0 = tmp_timer.QuadPart;
    QueryPerformanceCounter(&tmp_timer);
    uint64_t t = tmp_timer.QuadPart - t0;
    timer += t;
    counter_for_timer++;
    DPRINTF(L"%s %I64d average: %4.3fms", msg.c_str(), t, (double)timer / counter_for_timer / frequency);
#endif
}

//=============================== TautoPerformanceCounter ===============================
TautoPerformanceCounter::TautoPerformanceCounter(TPerformanceCounter *performanceCounter)
{
#if ENABLE_PERFORMANCE_COUNTER
    this->performanceCounter = performanceCounter;
    performanceCounter->start();
#endif
}

#if ENABLE_PERFORMANCE_COUNTER
TautoPerformanceCounter::~TautoPerformanceCounter()
{
    performanceCounter->stop();
}
#endif
