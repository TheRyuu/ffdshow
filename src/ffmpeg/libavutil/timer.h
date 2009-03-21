/**
 * @file timer.h
 * high precision timer, useful to profile code
 *
 * copyright (c) 2006 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVUTIL_TIMER_H
#define AVUTIL_TIMER_H

#include <stdlib.h>
#ifdef __GNUC__
#include <stdint.h>
#endif
#include "config.h"

#if defined(ARCH_X86)
#define AV_READ_TIME read_time
static inline uint64_t read_time(void)
{
    uint32_t a, d;
    __asm__ volatile("rdtsc\n\t"
                 : "=a" (a), "=d" (d));
    return ((uint64_t)d << 32) + a;
}
#elif defined(HAVE_GETHRTIME)
#define AV_READ_TIME gethrtime
#endif

#define START_TIMER
#define STOP_TIMER(id) {}

#endif /* AVUTIL_TIMER_H */
