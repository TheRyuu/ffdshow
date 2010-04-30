#ifndef _COMPILER_H_
#define _COMPILER_H_

#ifndef STRINGIFY
 #define STRINGIFY(s) TOSTRING(s)
 #define TOSTRING(s) #s
#endif

#if defined(__INTEL_COMPILER)
  #if __INTEL_COMPILER  >= 1100
    #define COMPILER "icl 11"
  #elif __INTEL_COMPILER  >= 1000
    #define COMPILER "icl 10"
  #else
    #define COMPILER "icl"
  #endif
#elif defined(_MSC_VER)
  #if _MSC_VER>=1500
    #define COMPILER "msvc 2008"
  #else
#define COMPILER "unknown and not supported"
  #endif
#elif defined(__GNUC__)
  #ifdef __SSE__
    #define COMPILER_SSE " sse"
    #ifdef __SSE2__
      #define COMPILER_SSE2 ",sse2"
    #else
      #define COMPILER_SSE2 ""
    #endif
  #else
    #define COMPILER_SSE ""
    #define COMPILER_SSE2 ""
  #endif
  #define COMPILER "MinGW GCC "STRINGIFY(__GNUC__)"."STRINGIFY(__GNUC_MINOR__)"."STRINGIFY(__GNUC_PATCHLEVEL__) COMPILER_SSE COMPILER_SSE2
#else
  #define COMPILER "unknown and not supported"
#endif

#ifdef WIN64
  #define COMPILER_X64 ", x64"
#else
  #define COMPILER_X64 ", x86"
#endif

#ifdef UNICODE
 #define UNICODE_BUILD "unicode"
#else
 #define UNICODE_BUILD "ansi"
#endif

#ifdef DEBUG
  #define COMPILER_INFO ", d"
#else
  #define COMPILER_INFO ", r"
#endif

#endif
