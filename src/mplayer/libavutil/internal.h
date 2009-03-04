/*
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

/**
 * @file libavutil/internal.h
 * common internal API header
 */

#ifndef AVUTIL_INTERNAL_H
#define AVUTIL_INTERNAL_H

#define AV_GCC_VERSION_AT_LEAST(x,y) (defined(__GNUC__) && (__GNUC__ > x || __GNUC__ == x && __GNUC_MINOR__ >= y))

#ifndef attribute_align_arg
#if (!defined(__ICC) || __ICC > 1100) && AV_GCC_VERSION_AT_LEAST(4,2)
#    define attribute_align_arg __attribute__((force_align_arg_pointer))
#else
#    define attribute_align_arg
#endif
#endif

#ifndef attribute_used
#if AV_GCC_VERSION_AT_LEAST(3,1)
#    define attribute_used __attribute__((used))
#else
#    define attribute_used
#endif
#endif

#ifndef INT16_MIN
#define INT16_MIN       (-0x7fff-1)
#endif

#ifndef INT16_MAX
#define INT16_MAX       0x7fff
#endif

#ifndef INT32_MIN
#define INT32_MIN       (-0x7fffffff-1)
#endif

#ifndef INT32_MAX
#define INT32_MAX       0x7fffffff
#endif

#ifndef UINT32_MAX
#define UINT32_MAX      0xffffffff
#endif

#ifndef INT64_MIN
#define INT64_MIN       (-0x7fffffffffffffffLL-1)
#endif

#ifndef INT64_MAX
#define INT64_MAX INT64_C(9223372036854775807)
#endif

#ifndef UINT64_MAX
#define UINT64_MAX UINT64_C(0xFFFFFFFFFFFFFFFF)
#endif

#ifndef INT_BIT
#    define INT_BIT (CHAR_BIT * sizeof(int))
#endif

#if ( defined(__PIC__) || defined(__pic__) ) && ! defined(PIC)
#    define PIC
#endif

#define snprintf _snprintf
#define vsnprintf _vsnprintf

#define CHECKED_ALLOCZ(p, size)\
{\
    p= av_mallocz(size);\
    if(p==NULL && (size)!=0){\
        av_log(NULL, AV_LOG_ERROR, "Cannot allocate memory.");\
        goto fail;\
    }\
}

#if defined(__ICC) || defined(__SUNPRO_C)
    #define DECLARE_ALIGNED(n,t,v)      t v __attribute__ ((aligned (n)))
    #define DECLARE_ASM_CONST(n,t,v)    const t __attribute__ ((aligned (n))) v
#elif defined(__GNUC__)
    #define DECLARE_ALIGNED(n,t,v)      t v __attribute__ ((aligned (n)))
    #define DECLARE_ASM_CONST(n,t,v)    static const t v attribute_used __attribute__ ((aligned (n)))
#elif defined(_MSC_VER)
    #define DECLARE_ALIGNED(n,t,v)      __declspec(align(n)) t v
    #define DECLARE_ASM_CONST(n,t,v)    __declspec(align(n)) static const t v
#elif HAVE_INLINE_ASM
    #error The asm code needs alignment, but we do not know how to do it for this compiler.
#else
    #define DECLARE_ALIGNED(n,t,v)      t v
    #define DECLARE_ASM_CONST(n,t,v)    static const t v
#endif

#endif /* AVUTIL_INTERNAL_H */
