/*
 * Copyright (c) 2009 h.yamagata, based on the ideas of AviSynth's color converters 
 *  Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
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
#ifndef _FFDSHOW_CONVERTERS_H_
#define _FFDSHOW_CONVERTERS_H_

#include <emmintrin.h>
#include <stddef.h>        // ptrdiff_t
#include "inttypes.h"
#include "ffImgfmt.h"      // Just to use some names of color spaces
#include "ffYCbCr_RGB_MatrixCoefficients.h"

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
 #pragma warning (push)
 #pragma warning (disable: 4244 4819)
#endif

// Make sure you have Boost 1.37.0 in your include path, if you are porting this to other project. ffdshow has it in svn.
// http://www.boost.org/

// threadpool by Philipp Henkel.
// http://sourceforge.net/projects/threadpool/

#include "threadpool/threadpool.hpp"

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
 #pragma warning (pop)
#endif

#ifndef MAX_THREADS
 #define MAX_THREADS 32
#endif

typedef ptrdiff_t stride_t;

// Features
//  High quality chroma upscaling: 75:25 averaging vertically and horizontally
//  Support for color primary parameters, such as ITU-R BT.601/709, input and output levels
//  10bit or more calculations
//  Supported color spaces
//    input:  progressive YV12, progressive NV12, YV16, YUY2
//    output: RGB32,RGB24
//  SSE2 required
//  Multithreaded and very fast on modern CPUs
//  Portable (should work on UNIX)

class TffdshowConverters
{
public:
 void init(int incsp,                      // FF_CSP_420P, FF_CSP_NV12, FF_CSP_YUY2 or FF_CSP_420P (progressive only)
           int outcsp,                     // FF_CSP_RGB32 or FF_CSP_RGB24
           ffYCbCr_RGB_MatrixCoefficientsType cspOptionsIturBt,  // ffYCbCr_RGB_coeff_ITUR_BT601, ffYCbCr_RGB_coeff_ITUR_BT709 or ffYCbCr_RGB_coeff_SMPTE240M
           int input_Y_white_level,        // input Y level (TV:235, PC:255)
           int input_Y_black_level,        // input Y level (TV:16, PC:0)
           int input_Cb_bluest_level,      // input chroma level (TV:16, PC:1)
           double output_RGB_white_level,  // output RGB level (TV:235, PC:255)
           double output_RGB_black_level); // output RGB level (TV:16, PC:0)

 // note YV12 and YV16 is YCrCb order. Make sure Cr and Cb is swapped.
 // NV12: srcCr not used.
 // YUY2: srcCb,srcCr,stride_CbCr not used.
 void convert(const uint8_t* srcY,
              const uint8_t* srcCb,
              const uint8_t* srcCr,
              uint8_t* dst,         // 16 bytes alignment is prefered for RGB32 (6% faster).
              int dx,
              int dy,
              stride_t stride_Y,
              stride_t stride_CbCr,
              stride_t stride_dst);

 TffdshowConverters(int thread_count);
 ~TffdshowConverters();

private:
 uint8_t *m_coeffs;
 int m_incsp, m_outcsp;
 int m_thread_count;
 bool m_rgb_limit;
 boost::threadpool::pool threadpool;
   static const int ofs_Ysub=0;
   static const int ofs_128mul16=16;
   static const int ofs_rgb_limit_low=32;
   static const int ofs_rgb_limit_high=48;
   static const int ofs_xFF000000_FF000000=64;
   static const int ofs_xFFFFFFFF_FFFFFFFF=80;
   static const int ofs_rgb_add=96;
   static const int ofs_cy=112;
   static const int ofs_cR_Cr=128;
   static const int ofs_cG_Cb_cG_Cr=144;
   static const int ofs_cB_Cb=160;

 template<int incsp, int outcsp, int left_edge, int right_edge, int rgb_limit, int aligned> static __forceinline 
  void convert_a_unit(const unsigned char* &srcY,
                      const unsigned char* &srcCb,
                      const unsigned char* &srcCr,
                      unsigned char* &dst,
                      const stride_t stride_Y,
                      const stride_t stride_CbCr,
                      const stride_t stride_dst,
                      const unsigned char* const coeffs);

 // translate stack arguments to template arguments.
 template <int rgb_limit> void convert_translate_incsp(
              const uint8_t* srcY,
              const uint8_t* srcCb,
              const uint8_t* srcCr,
              uint8_t* dst,
              int dx,
              int dy,
              stride_t stride_Y,
              stride_t stride_CbCr,
              stride_t stride_dst);

 template <int incsp, int rgb_limit> void convert_translate_outcsp(
              const uint8_t* srcY,
              const uint8_t* srcCb,
              const uint8_t* srcCr,
              uint8_t* dst,
              int dx,
              int dy,
              stride_t stride_Y,
              stride_t stride_CbCr,
              stride_t stride_dst);

 template <int incsp, int outcsp, int rgb_limit> void convert_translate_align(
              const uint8_t* srcY,
              const uint8_t* srcCb,
              const uint8_t* srcCr,
              uint8_t* dst,
              int dx,
              int dy,
              stride_t stride_Y,
              stride_t stride_CbCr,
              stride_t stride_dst);

 template <int incsp, int outcsp, int rgb_limit, int aligned> void convert_main(
              const uint8_t* srcY,
              const uint8_t* srcCb,
              const uint8_t* srcCr,
              uint8_t* dst,
              int dx,
              int dy,
              stride_t stride_Y,
              stride_t stride_CbCr,
              stride_t stride_dst);

 template <int incsp, int outcsp, int rgb_limit, int aligned> static void convert_main_loop(
              const uint8_t* srcY,
              const uint8_t* srcCb,
              const uint8_t* srcCr,
              uint8_t* dst,
              int dx,
              int dy,
              stride_t stride_Y,
              stride_t stride_CbCr,
              stride_t stride_dst,
              int starty,
              int endy,
              const unsigned char* const coeffs);

 template<int incsp, int outcsp, int rgb_limit, int aligned> struct Tfunc_obj {
     private:
        const uint8_t* srcY;
        const uint8_t* srcCb;
        const uint8_t* srcCr;
        uint8_t* dst;
        int dx;
        int dy;
        stride_t stride_Y;
        stride_t stride_CbCr;
        stride_t stride_dst;
        int starty;
        int endy;
        const unsigned char* const coeffs;
     public:
        void operator()(void) {
            convert_main_loop<incsp,outcsp,rgb_limit,aligned>(srcY,srcCb,srcCr,dst,dx,dy,stride_Y,stride_CbCr,stride_dst,starty,endy,coeffs);
        }
        Tfunc_obj(const uint8_t* IsrcY,
                  const uint8_t* IsrcCb,
                  const uint8_t* IsrcCr,
                  uint8_t* Idst,
                  int Idx,
                  int Idy,
                  stride_t Istride_Y,
                  stride_t Istride_CbCr,
                  stride_t Istride_dst,
                  int Istarty,
                  int Iendy,
                  const unsigned char* const Icoeffs) :
            srcY(IsrcY),
            srcCb(IsrcCb),
            srcCr(IsrcCr),
            dst(Idst),
            dx(Idx),
            dy(Idy),
            stride_Y(Istride_Y),
            stride_CbCr(Istride_CbCr),
            stride_dst(Istride_dst),
            starty(Istarty),
            endy(Iendy),
            coeffs(Icoeffs)
        {}
 };

};

#endif // _FFDSHOW_CONVERTERS_H_
