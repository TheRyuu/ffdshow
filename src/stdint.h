/* stdint.h - integer types custom code from ffdshow-tryouts

   Copyright 2003, 2006, 2007 Red Hat, Inc.

This file is part of Cygwin.

This software is a copyrighted work licensed under the terms of the
Cygwin license.  Please consult the file "CYGWIN_LICENSE" for
details. */

#ifndef _STDINT_H
#define _STDINT_H

#ifndef _WIN64
 #define __WORDSIZE 32
#else
 #define __WORDSIZE 64
#endif

/* Exact-width integer types */

typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
#ifdef _MSC_VER
 typedef __int64 int64_t;
#else
 typedef long long int64_t;
#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#ifdef _MSC_VER
 typedef unsigned __int64 uint64_t;
#else
 typedef unsigned long long uint64_t;
#endif

/* Fastest minimum-width integer types */

typedef signed char int_fast8_t;
typedef int int_fast16_t;
typedef int int_fast32_t;
#ifdef _MSC_VER
 typedef __int64 int_fast64_t;
#else
 typedef long long int_fast64_t;
#endif

typedef unsigned char uint_fast8_t;
typedef unsigned int uint_fast16_t;
typedef unsigned int uint_fast32_t;
#ifdef _MSC_VER
 typedef unsigned __int64 uint_fast64_t;
#else
 typedef unsigned long long uint_fast64_t;
#endif

/* Integer types capable of holding object pointers */

#ifndef _WIN64
 typedef int intptr_t;
 typedef unsigned int uintptr_t;
#else
 typedef __int64 intptr_t;
 typedef unsigned __int64 uintptr_t;
#endif

/* Limits of exact-width integer types */

#define INT8_MIN (-128)
#define INT16_MIN (-32768)
#define INT32_MIN (-2147483647 - 1)
#define INT64_MIN (-9223372036854775807LL - 1LL)

#define INT8_MAX (127)
#define INT16_MAX (32767)
#define INT32_MAX (2147483647)
#define INT64_MAX (9223372036854775807LL)

#define UINT8_MAX (255)
#define UINT16_MAX (65535)
#define UINT32_MAX (4294967295U)
#define UINT64_MAX (18446744073709551615ULL)

/* Limits of fastest minimum-width integer types */

#define INT_FAST8_MIN (-128)
#define INT_FAST16_MIN (-2147483647 - 1)
#define INT_FAST32_MIN (-2147483647 - 1)
#define INT_FAST64_MIN (-9223372036854775807LL - 1LL)

#define INT_FAST8_MAX (127)
#define INT_FAST16_MAX (2147483647)
#define INT_FAST32_MAX (2147483647)
#define INT_FAST64_MAX (9223372036854775807LL)

#define UINT_FAST8_MAX (255)
#define UINT_FAST16_MAX (4294967295U)
#define UINT_FAST32_MAX (4294967295U)
#define UINT_FAST64_MAX (18446744073709551615ULL)

/* Macros for minimum-width integer constant expressions */

#define INT8_C(x) x
#define INT16_C(x) x
#define INT32_C(x) x
#define INT64_C(x) x ## LL

#define UINT8_C(x) x
#define UINT16_C(x) x
#define UINT32_C(x) x ## U
#define UINT64_C(x) x ## ULL

#endif /* _STDINT_H */
