/*
 * Copyright (c) 2009-2011 h.yamagata, based on the ideas of AviSynth's color converters
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
#ifndef FFDSHOW_CONVERTERS_H_
#define FFDSHOW_CONVERTERS_H_

#include <emmintrin.h>
#include <stddef.h>        // ptrdiff_t
#include "inttypes.h"
#include "ffImgfmt.h"      // Just to use some names of color spaces
#include "TYCbCr2RGB_coeffs.h"

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
//  calculations in 11bit or higher
//  Supported color spaces
//    input:  progressive 4:2:0 8/10-bit YCbCr planar, 4:2:2 8/10-bit YCbCr planar, 4:4:4 8/10-bit YCbCr planar, NV12, YUY2
//    output: RGB32,RGB24,BGR32,BGR24
//  SSE2 required
//  Multithreaded and very fast on modern CPUs
//  Portable (should work on UNIX)

// Limitations
// The stride of destination must be multiple of 4.
static inline bool incsp_sup_ffdshow_converter(uint64_t incsp)
{
    return !!((incsp & FF_CSPS_MASK) & (FF_CSP_420P | FF_CSP_NV12 | FF_CSP_YUY2 | FF_CSP_422P | FF_CSP_444P | FF_CSP_420P10 | FF_CSP_422P10 | FF_CSP_444P10));
}

static inline bool outcsp_sup_ffdshow_converter(uint64_t outcsp)
{
    return !!((outcsp & FF_CSPS_MASK) & (FF_CSP_RGB24 | FF_CSP_RGB32 | FF_CSP_BGR24 | FF_CSP_BGR32));
}

class TffdshowConverters
{
public:
    void init(uint64_t incsp,                 // FF_CSP_420P, FF_CSP_NV12, FF_CSP_YUY2, FF_CSP_422P, FF_CSP_444P, FF_CSP_420P10, FF_CSP_422P10 or FF_CSP_444P10 (progressive only)
              uint64_t outcsp,                // FF_CSP_RGB32, FF_CSP_RGB24, FF_CSP_BGR32 or FF_CSP_BGR24
              ffYCbCr_RGB_MatrixCoefficientsType cspOptionsIturBt,  // ffYCbCr_RGB_coeff_ITUR_BT601, ffYCbCr_RGB_coeff_ITUR_BT709 or ffYCbCr_RGB_coeff_SMPTE240M
              int input_Y_white_level,        // input Y level (TV:235, PC:255)
              int input_Y_black_level,        // input Y level (TV:16, PC:0)
              int input_Cb_bluest_level,      // input chroma level (TV:16, PC:1)
              double output_RGB_white_level,  // output RGB level (TV:235, PC:255)
              double output_RGB_black_level,  // output RGB level (TV:16, PC:0)
              bool dithering,                 // dithering (On:1, Off:0)
              bool isMPEG1);                  // horizontal chroma position (left:0, center:1)

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
    uint64_t m_incsp, m_outcsp;
    int m_thread_count;
    bool m_rgb_limit;
    bool m_dithering;
    bool m_isMPEG1;
    int m_old_width;
    static const int dither_lineoffset = 16;
    uint16_t *dither;
    boost::threadpool::pool threadpool;
    void init_dither(int width);
    struct Tcoeffs {
        __m128i Ysub;
        __m128i CbCr_center;
        __m128i rgb_limit_low;
        __m128i rgb_limit_high;
        __m128i rgb_add;
        __m128i cy;
        __m128i cR_Cr;
        __m128i cG_Cb_cG_Cr;
        __m128i cB_Cb;
    } *m_coeffs;

    template<uint64_t incsp, uint64_t outcsp, int left_edge, int right_edge, int rgb_limit, int aligned, bool dithering, bool isMPEG1> static
    void convert_two_lines(const unsigned char* &srcY,
                           const unsigned char* &srcCb,
                           const unsigned char* &srcCr,
                           unsigned char* &dst,
                           int xCount,
                           const stride_t stride_Y,
                           const stride_t stride_CbCr,
                           const stride_t stride_dst,
                           const Tcoeffs *coeffs,
                           const uint16_t *dither_ptr);

    // translate stack arguments to template arguments.
    template <bool rgb_limit> void convert_translate_align(
        const uint8_t* srcY,
        const uint8_t* srcCb,
        const uint8_t* srcCr,
        uint8_t* dst,
        int dx,
        int dy,
        stride_t stride_Y,
        stride_t stride_CbCr,
        stride_t stride_dst);

    template <bool aligned, bool rgb_limit> void convert_translate_dithering(
        const uint8_t* srcY,
        const uint8_t* srcCb,
        const uint8_t* srcCr,
        uint8_t* dst,
        int dx,
        int dy,
        stride_t stride_Y,
        stride_t stride_CbCr,
        stride_t stride_dst);

    template <bool dithering, bool aligned, bool rgb_limit> void convert_translate_isMPEG1(
        const uint8_t* srcY,
        const uint8_t* srcCb,
        const uint8_t* srcCr,
        uint8_t* dst,
        int dx,
        int dy,
        stride_t stride_Y,
        stride_t stride_CbCr,
        stride_t stride_dst);

    template <bool isMPEG1, bool dithering, bool aligned, bool rgb_limit> void convert_translate_outcsp(
        const uint8_t* srcY,
        const uint8_t* srcCb,
        const uint8_t* srcCr,
        uint8_t* dst,
        int dx,
        int dy,
        stride_t stride_Y,
        stride_t stride_CbCr,
        stride_t stride_dst);

    template <uint64_t outcsp, bool isMPEG1, bool dithering, bool aligned, bool rgb_limit> void convert_translate_incsp(
        const uint8_t* srcY,
        const uint8_t* srcCb,
        const uint8_t* srcCr,
        uint8_t* dst,
        int dx,
        int dy,
        stride_t stride_Y,
        stride_t stride_CbCr,
        stride_t stride_dst);

    template <uint64_t incsp, uint64_t outcsp, bool isMPEG1, bool dithering, bool aligned, bool rgb_limit> void convert_main(
        const uint8_t* srcY,
        const uint8_t* srcCb,
        const uint8_t* srcCr,
        uint8_t* dst,
        int dx,
        int dy,
        stride_t stride_Y,
        stride_t stride_CbCr,
        stride_t stride_dst);

    template <uint64_t incsp, uint64_t outcsp, bool isMPEG1, bool dithering, bool aligned, bool rgb_limit> void convert_main_loop(
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
        int endy);

    template<uint64_t incsp, uint64_t outcsp, bool isMPEG1, bool dithering, bool aligned, bool rgb_limit> struct Tfunc_obj {
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
        const Tcoeffs *coeffs;
        TffdshowConverters *self;
    public:
        void operator()(void) {
            self->convert_main_loop<incsp, outcsp, isMPEG1, dithering, aligned, rgb_limit>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst, starty, endy);
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
                  TffdshowConverters *Iself) :
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
            self(Iself)
        {}
    };
};

#endif // FFDSHOW_CONVERTERS_H_
