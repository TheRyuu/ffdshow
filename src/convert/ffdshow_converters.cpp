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

#include "stdafx.h"
#include <malloc.h>
#include "ffdshow_converters.h"
#include "simd_common.h" // __align16(t,v)

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#pragma warning (push)
#pragma warning (disable: 4799) // EMMS
#endif

void TffdshowConverters::init(uint64_t incsp,                 // FF_CSP_420P, FF_CSP_NV12, FF_CSP_YUY2, FF_CSP_420P, FF_CSP_420P10, FF_CSP_422P10 or FF_CSP_440P10 (progressive only)
                              uint64_t outcsp,                // FF_CSP_RGB32 or FF_CSP_RGB24
                              ffYCbCr_RGB_MatrixCoefficientsType cspOptionsIturBt,  // ffYCbCr_RGB_coeff_ITUR_BT601, ffYCbCr_RGB_coeff_ITUR_BT709 or ffYCbCr_RGB_coeff_SMPTE240M
                              int input_Y_white_level,        // input Y level (TV:235, PC:255)
                              int input_Y_black_level,        // input Y level (TV:16, PC:0)
                              int input_Cb_bluest_level,      // input chroma level (TV:16, PC:1)
                              double output_RGB_white_level,  // output RGB level (TV:235, PC:255)
                              double output_RGB_black_level,  // output RGB level (TV:16, PC:0)
                              bool dithering,                 // dithering (On:1, Off:0)
                              bool isMPEG1)                   // horizontal chroma position: left:false center:true
{
    const int bitDepthMul = (incsp & FF_CSPS_MASK_HIGH_BIT) ? 4 : 1;  // 10bit color spaces should be listed here.
    m_incsp = incsp;
    m_outcsp = outcsp;
    m_isMPEG1 = isMPEG1;
    if (output_RGB_white_level != 255 || output_RGB_black_level != 0) {
        m_rgb_limit = true;
    } else {
        m_rgb_limit = false;
    }

    TYCbCr2RGB_coeffs YCbCr2RGB_coeffs(cspOptionsIturBt, input_Y_white_level, input_Y_black_level, input_Cb_bluest_level, output_RGB_white_level, output_RGB_black_level);

    short cy = short(YCbCr2RGB_coeffs.y_mul * 16384 + 0.5);
    short crv = short(YCbCr2RGB_coeffs.vr_mul * 8192 + 0.5);
    short cgu = short(-YCbCr2RGB_coeffs.ug_mul * 8192 - 0.5);
    short cgv = short(-YCbCr2RGB_coeffs.vg_mul * 8192 - 0.5);
    short cbu = short(YCbCr2RGB_coeffs.ub_mul * 8192 + 0.5);

    m_coeffs->Ysub = _mm_set1_epi16(short(YCbCr2RGB_coeffs.Ysub * bitDepthMul));
    m_coeffs->cy = _mm_set1_epi16(cy);
    m_coeffs->CbCr_center = _mm_set1_epi16(128 * 16 * bitDepthMul);

    m_coeffs->cR_Cr = _mm_set1_epi32(crv << 16);               // R
    m_coeffs->cG_Cb_cG_Cr = _mm_set1_epi32((cgv << 16) + cgu); // G
    m_coeffs->cB_Cb = _mm_set1_epi32(cbu);                     // B


    // The following stuffs (rgb_add, rgb_limit_high and rgb_limit_low) are not affected by the input bit depth.
    m_coeffs->rgb_add = _mm_set1_epi16((YCbCr2RGB_coeffs.RGB_add1 << 4) + (dithering ? 0 : 8));

    uint32_t rgb_white = uint32_t(output_RGB_white_level);
    rgb_white = 0xff000000 + (rgb_white << 16) + (rgb_white << 8) + rgb_white;
    m_coeffs->rgb_limit_high = _mm_set1_epi32(rgb_white);

    uint32_t rgb_black = uint32_t(output_RGB_black_level);
    rgb_black = 0xff000000 + (rgb_black << 16) + (rgb_black << 8) + rgb_black;
    m_coeffs->rgb_limit_low = _mm_set1_epi32(rgb_black);
    m_dithering = dithering || (incsp & FF_CSPS_MASK_HIGH_BIT);
}

// note YV12 and YV16 is YCrCb order. Make sure Cr and Cb is swapped.
// NV12: srcCr not used.
// YUY2: srcCb,srcCr,stride_CbCr not used.
void TffdshowConverters::convert(const uint8_t* srcY,
                                 const uint8_t* srcCb,
                                 const uint8_t* srcCr,
                                 uint8_t* dst,         // 16 bytes alignment is prefered for RGB32 (6% faster).
                                 int dx,
                                 int dy,
                                 stride_t stride_Y,
                                 stride_t stride_CbCr,
                                 stride_t stride_dst)
{
    if (m_rgb_limit) {
        convert_translate_align<1>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
    } else {
        convert_translate_align<0>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
    }

#ifndef WIN64
    _mm_empty();
#endif
}

// translate stack arguments to template arguments.
template <bool rgb_limit> void TffdshowConverters::convert_translate_align(
    const uint8_t* srcY,
    const uint8_t* srcCb,
    const uint8_t* srcCr,
    uint8_t* dst,
    int dx,
    int dy,
    stride_t stride_Y,
    stride_t stride_CbCr,
    stride_t stride_dst)
{
    if ((stride_dst & 0xf) || (stride_t(dst) & 0xf)) {
        convert_translate_dithering<0, rgb_limit>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
    } else {
        convert_translate_dithering<1, rgb_limit>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
    }
}

template <bool aligned, bool rgb_limit> void TffdshowConverters::convert_translate_dithering(
    const uint8_t* srcY,
    const uint8_t* srcCb,
    const uint8_t* srcCr,
    uint8_t* dst,
    int dx,
    int dy,
    stride_t stride_Y,
    stride_t stride_CbCr,
    stride_t stride_dst)
{
    if (m_dithering) {
        convert_translate_isMPEG1<1, aligned, rgb_limit>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
    } else {
        convert_translate_isMPEG1<0, aligned, rgb_limit>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
    }
}

template <bool dithering, bool aligned, bool rgb_limit> void TffdshowConverters::convert_translate_isMPEG1(
    const uint8_t* srcY,
    const uint8_t* srcCb,
    const uint8_t* srcCr,
    uint8_t* dst,
    int dx,
    int dy,
    stride_t stride_Y,
    stride_t stride_CbCr,
    stride_t stride_dst)
{
    if (m_isMPEG1) {
        // omit optimization by rgb_limit and aligned to reduce compilation time and code size
        convert_translate_outcsp<1, dithering, 0, 1>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
    } else {
        convert_translate_outcsp<0, dithering, aligned, rgb_limit>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
    }
}

template <bool isMPEG1, bool dithering, bool aligned, bool rgb_limit> void TffdshowConverters::convert_translate_outcsp(
    const uint8_t* srcY,
    const uint8_t* srcCb,
    const uint8_t* srcCr,
    uint8_t* dst,
    int dx,
    int dy,
    stride_t stride_Y,
    stride_t stride_CbCr,
    stride_t stride_dst)
{
    switch (m_outcsp) {
        case FF_CSP_RGB32:
            convert_translate_incsp<FF_CSP_RGB32, isMPEG1, dithering, aligned, rgb_limit>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
            return;
        case FF_CSP_RGB24:
            // omit optimization by aligned and rgb_limit to reduce compilation time and code size
            convert_translate_incsp<FF_CSP_RGB24, isMPEG1, dithering, 0, 1>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
            return;
        case FF_CSP_BGR32:
            convert_translate_incsp<FF_CSP_BGR32, isMPEG1, dithering, 0, 1>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
            return;
        case FF_CSP_BGR24:
            convert_translate_incsp<FF_CSP_BGR24, isMPEG1, dithering, 0, 1>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
            return;
    }
}

template <uint64_t outcsp, bool isMPEG1, bool dithering, bool aligned, bool rgb_limit> void TffdshowConverters::convert_translate_incsp(
    const uint8_t* srcY,
    const uint8_t* srcCb,
    const uint8_t* srcCr,
    uint8_t* dst,
    int dx,
    int dy,
    stride_t stride_Y,
    stride_t stride_CbCr,
    stride_t stride_dst)
{
    switch (m_incsp) {
        case FF_CSP_420P:
            convert_main<FF_CSP_420P, outcsp, isMPEG1, dithering, aligned, rgb_limit>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
            return;
        case FF_CSP_NV12:
            convert_main<FF_CSP_NV12, outcsp, isMPEG1, dithering, aligned, rgb_limit>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
            return;
        case FF_CSP_YUY2:
            // omit optimization by rgb_limit to reduce compilation time and code size
            convert_main<FF_CSP_YUY2, outcsp, 0, dithering, 0, 1>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
            return;
        case FF_CSP_422P:
            convert_main<FF_CSP_422P, outcsp, 0, dithering, 0, 1>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
            return;
        case FF_CSP_444P:
            convert_main<FF_CSP_444P, outcsp, 0, dithering, 0, 1>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
            return;
        case FF_CSP_420P10:
            convert_main<FF_CSP_420P10, outcsp, 0, 1, aligned, rgb_limit>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
            return;
        case FF_CSP_422P10:
            convert_main<FF_CSP_422P10, outcsp, 0, 1, 0, 1>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
            return;
        case FF_CSP_444P10:
            convert_main<FF_CSP_444P10, outcsp, 0, 1, 0, 1>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
            return;
    }
}

template <uint64_t incsp, uint64_t outcsp, bool isMPEG1, bool dithering, bool aligned, bool rgb_limit> void TffdshowConverters::convert_main(
    const uint8_t* srcY,
    const uint8_t* srcCb,
    const uint8_t* srcCr,
    uint8_t* dst,
    int dx,
    int dy,
    stride_t stride_Y,
    stride_t stride_CbCr,
    stride_t stride_dst)
{
    init_dither(dx);
    if (m_thread_count == 1) {
        convert_main_loop<incsp, outcsp, isMPEG1, dithering, aligned, rgb_limit>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst, 0, dy);
    } else {
        int is_odd;
        int starty = 0;
        int lines_per_thread = (dy / m_thread_count)&~1;

        if (incsp == FF_CSP_420P || incsp == FF_CSP_NV12 || incsp == FF_CSP_420P10) {
            is_odd = 1;
        } else {
            is_odd = 0;
        }

        for (int i = 0 ; i < m_thread_count ; i++) {
            int endy = (i == m_thread_count - 1) ?
                       dy :
                       starty + lines_per_thread + is_odd;
            threadpool.schedule(Tfunc_obj<incsp, outcsp, isMPEG1, dithering, aligned, rgb_limit>
                                (srcY,
                                 srcCb,
                                 srcCr,
                                 dst,
                                 dx,
                                 dy,
                                 stride_Y,
                                 stride_CbCr,
                                 stride_dst,
                                 starty + (i ? is_odd : 0),
                                 endy,
                                 this));
            starty += lines_per_thread;
        }
        threadpool.wait();
    }
}

template <uint64_t incsp, uint64_t outcsp, bool isMPEG1, bool dithering, bool aligned, bool rgb_limit> void TffdshowConverters::convert_main_loop(
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
    int endy)
{
    const uint8_t *srcYln = srcY;
    const uint8_t *srcCbln = srcCb;
    const uint8_t *srcCrln = srcCr;
    uint8_t *dstln = dst;
    int y = starty;
    int endy0 = endy;
    int dither_pos = 0;
    Tcoeffs *coeffs = m_coeffs;

    int xCount; // loop counter
    if (incsp == FF_CSP_444P10 || incsp == FF_CSP_444P) {
        xCount = dx / 4;
    } else {
        xCount = dx / 4 - 2;
    }

    if (incsp == FF_CSP_420P || incsp == FF_CSP_NV12 || incsp == FF_CSP_420P10) {
        if (y == 0) { // if this is the first thread,
            // Top

            // left
            if (dx > 4)
                convert_two_lines<incsp, outcsp, 1, 0, rgb_limit, aligned, dithering, isMPEG1>(srcYln, srcCbln, srcCrln, dstln, 1,
                        0, 0, 0,
                        coeffs, dither + dither_pos);
            // inner
            if (xCount > 0)
                convert_two_lines<incsp, outcsp, 0, 0, rgb_limit, aligned, dithering, isMPEG1>(srcYln, srcCbln, srcCrln, dstln, xCount,
                        0, 0, 0,
                        coeffs, dither + dither_pos + 8);
            // right
            convert_two_lines<incsp, outcsp, 0, 1, rgb_limit, aligned, dithering, isMPEG1>(srcYln, srcCbln, srcCrln, dstln, 1,
                    0, 0, 0,
                    coeffs, dither + dither_pos);
            y = 1;
        }
        if (endy == dy) {
            endy--;
        }
        if (dithering) {
            dither_pos += dither_lineoffset;
        }
    }

    // Mid lines
    for (; y < endy ; y += 2) {
        srcYln = srcY + y * stride_Y;
        if (incsp == FF_CSP_420P || incsp == FF_CSP_NV12 || incsp == FF_CSP_420P10) {
            srcCbln = srcCb + (y >> 1) * stride_CbCr;
            srcCrln = srcCr + (y >> 1) * stride_CbCr;
        } else {
            srcCbln = srcCb + y * stride_CbCr;
            srcCrln = srcCr + y * stride_CbCr;
        }
        dstln = dst + y * stride_dst;
        // left
        if (incsp != FF_CSP_444P10 && incsp != FF_CSP_444P && dx > 4) {
            convert_two_lines<incsp, outcsp, 1, 0, rgb_limit, aligned, dithering, isMPEG1>(srcYln, srcCbln, srcCrln, dstln, 1,
                    stride_Y, stride_CbCr, stride_dst,
                    coeffs, dither + dither_pos);
        }
        // inner
        if (xCount > 0)
            convert_two_lines<incsp, outcsp, 0, 0, rgb_limit, aligned, dithering, isMPEG1>(srcYln, srcCbln, srcCrln, dstln, xCount,
                    stride_Y, stride_CbCr, stride_dst,
                    coeffs, dither + dither_pos + 8);
        // right
        if (incsp != FF_CSP_444P10 && incsp != FF_CSP_444P) {
            convert_two_lines<incsp, outcsp, 0, 1, rgb_limit, aligned, dithering, isMPEG1>(srcYln, srcCbln, srcCrln, dstln, 1,
                    stride_Y, stride_CbCr, stride_dst,
                    coeffs, dither + dither_pos);
        }
        if (dithering) {
            dither_pos += dither_lineoffset;
            if (dither_pos >= dx * 2) {
                dither_pos = 0;
            }
        }
    }

    if (incsp == FF_CSP_420P || incsp == FF_CSP_NV12 || incsp == FF_CSP_420P10) {
        if (endy0 == dy) { // if this is the last thread,
            // Bottom
            srcYln = srcY + (dy - 1) * stride_Y;
            srcCbln = srcCb + ((dy >> 1) - 1) * stride_CbCr;
            srcCrln = srcCr + ((dy >> 1) - 1) * stride_CbCr;
            dstln = dst + (dy - 1) * stride_dst;
            // left
            if (dx > 4)
                convert_two_lines<incsp, outcsp, 1, 0, rgb_limit, aligned, dithering, isMPEG1>(srcYln, srcCbln, srcCrln, dstln, 1,
                        0, 0, 0,
                        coeffs, dither + dither_pos);
            // inner
            if (xCount > 0)
                convert_two_lines<incsp, outcsp, 0, 0, rgb_limit, aligned, dithering, isMPEG1>(srcYln, srcCbln, srcCrln, dstln, xCount,
                        0, 0, 0,
                        coeffs, dither + dither_pos + 8);
            // right
            convert_two_lines<incsp, outcsp, 0, 1, rgb_limit, aligned, dithering, isMPEG1>(srcYln, srcCbln, srcCrln, dstln, 1,
                    0, 0, 0,
                    coeffs, dither + dither_pos);
        }
    }

    if (dx & 3) {
        int dxDone = dx - 4;
        switch (incsp) {
            case FF_CSP_420P:
            case FF_CSP_422P:
                srcY += dxDone;
                srcCb += dxDone / 2 - 1;
                srcCr += dxDone / 2 - 1;
                break;
            case FF_CSP_444P:
                srcY += dxDone;
                srcCb += dxDone;
                srcCr += dxDone;
                break;
            case FF_CSP_NV12:
                srcY += dxDone;
                srcCb += dxDone - 2;
                break;
            case FF_CSP_YUY2:
                srcY += dxDone * 2 - 4;
                break;
            case FF_CSP_420P10:
            case FF_CSP_422P10:
                srcY += dxDone * 2;
                srcCb += dxDone - 2;
                srcCr += dxDone - 2;
                break;
            case FF_CSP_444P10:
                srcY += dxDone * 2;
                srcCb += dxDone * 2;
                srcCr += dxDone * 2;
                break;
        };
        switch (outcsp) {
            case FF_CSP_RGB32:
            case FF_CSP_BGR32:
                dst += dxDone * 4;
                break;
            case FF_CSP_RGB24:
            case FF_CSP_BGR24:
                dst += dxDone * 3;
                break;
        }
        convert_main_loop<incsp, outcsp, isMPEG1, dithering, 0, rgb_limit>(srcY, srcCb, srcCr, dst, 4, dy, stride_Y, stride_CbCr, stride_dst, starty, endy);
    }
}

static __forceinline void loadLeftEdge10(const __m128i* src, __m128i &xmm, __m128i &temp)
{
    xmm = _mm_loadl_epi64(src);                                                // xmm1 = Cb03,Cb02,Cb01,Cb00
    temp = xmm;
    xmm = _mm_slli_si128(xmm, 2);                                              // xmm1 = Cb02,Cb01,Cb00,   0
    temp = _mm_slli_si128(temp, 14);
    temp = _mm_srli_si128(temp, 14);                                           // xmm2 =    0,   0,   0,Cb00
    xmm = _mm_or_si128(xmm, temp);                                             // xmm1 = Cb02,Cb01,Cb00,Cb00
}

static __forceinline void loadRightEdge10(const __m128i* src, __m128i &xmm, __m128i &temp)
{
    xmm = _mm_loadl_epi64(src);                                                // xmm1 = Cb01,Cb00,Cb00-1,Cb0-2
    temp = xmm;
    xmm = _mm_srli_si128(xmm, 2);                                              // xmm1 =    0,Cb01,Cb00,Cb00-1
    temp = _mm_srli_si128(temp, 6);
    temp = _mm_slli_si128(temp, 6);                                            // xmm2 = Cb01,   0,   0,   0
    xmm = _mm_or_si128(xmm, temp);                                             // xmm1 = Cb01,Cb01,Cb00,Cb00-1
}

template<int left_edge, int right_edge>
static __forceinline void load42CbCr(const unsigned char* &srcCb,
                                     const unsigned char* &srcCr,
                                     const stride_t stride_CbCr,
                                     __m128i &xmm0, __m128i &xmm1, __m128i &xmm2, __m128i &xmm3, __m128i &xmm4, __m128i &xmm5)
{
    if (left_edge) {
        loadLeftEdge10((const __m128i*)(srcCb),               xmm1, xmm0);     // xmm1 = Cb02,Cb01,Cb00,Cb00
        loadLeftEdge10((const __m128i*)(srcCb + stride_CbCr), xmm3, xmm2);     // xmm3 = Cb12,Cb11,Cb10,Cb10
        loadLeftEdge10((const __m128i*)(srcCr),               xmm0, xmm4);     // xmm0 = Cr02,Cr01,Cr00,Cr00
        loadLeftEdge10((const __m128i*)(srcCr + stride_CbCr), xmm2, xmm5);     // xmm2 = Cr12,Cr11,Cr10,Cr10
        srcCb += 2;
        srcCr += 2;
    } else if (right_edge) {
        loadRightEdge10((const __m128i*)(srcCb - 2),               xmm1, xmm0);// xmm1 = Cb01,Cb00,Cb0-1,Cb0-2
        loadRightEdge10((const __m128i*)(srcCb + stride_CbCr - 2), xmm3, xmm2);// xmm3 = Cb11,Cb10,Cb1-1,Cb1-2
        loadRightEdge10((const __m128i*)(srcCr - 2),               xmm0, xmm4);// xmm0 = Cr01,Cr00,Cr0-1,Cr0-2
        loadRightEdge10((const __m128i*)(srcCr + stride_CbCr - 2), xmm2, xmm5);// xmm2 = Cr11,Cr10,Cr1-1,Cr1-2
    } else {
        xmm1 = _mm_loadl_epi64((const __m128i*)(srcCb));                       // xmm1 = Cb02,Cb01,Cb00,Cb0-1
        xmm3 = _mm_loadl_epi64((const __m128i*)(srcCb + stride_CbCr));         // xmm3 = Cb12,Cb11,Cb10,Cb1-1
        xmm0 = _mm_loadl_epi64((const __m128i*)(srcCr));                       // xmm0 = Cr02,Cr01,Cr00,Cr0-1
        xmm2 = _mm_loadl_epi64((const __m128i*)(srcCr + stride_CbCr));         // xmm2 = Cr12,Cr11,Cr10,Cr1-1
        srcCb += 4;
        srcCr += 4;
    }
}

template<uint64_t incsp, uint64_t outcsp, int left_edge, int right_edge, int rgb_limit, int aligned, bool dithering, bool isMPEG1>
void TffdshowConverters::convert_two_lines(const unsigned char* &srcY,
        const unsigned char* &srcCb,
        const unsigned char* &srcCr,
        unsigned char* &dst,
        int xCount,
        const stride_t stride_Y,
        const stride_t stride_CbCr,
        const stride_t stride_dst,
        const Tcoeffs *coeffs,
        const uint16_t *dither_ptr)
{
    const int bitDepth = (incsp & FF_CSPS_MASK_HIGH_BIT) ? 10 : 8;
    __m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
    xmm7 = _mm_setzero_si128();

    do {
        // output 4x2 RGB pixels
        if (incsp == FF_CSP_444P || incsp == FF_CSP_444P10) {
            if (incsp == FF_CSP_444P) {
                // 4:4:4 YCbCr 8-bit
                xmm1 = _mm_cvtsi32_si128(*(const int*)(srcCb));                        // xmm1 = Cb03,Cb02,Cb01,Cb00
                xmm3 = _mm_cvtsi32_si128(*(const int*)(srcCb + stride_CbCr));          // xmm3 = Cb13,Cb12,Cb11,Cb10
                xmm0 = _mm_cvtsi32_si128(*(const int*)(srcCr));                        // xmm0 = Cr03,Cr02,Cr01,Cr00
                xmm2 = _mm_cvtsi32_si128(*(const int*)(srcCr + stride_CbCr));          // xmm2 = Cr13,Cr12,Cr11,Cr10
                xmm1 = _mm_unpacklo_epi8(xmm1, xmm7);
                xmm3 = _mm_unpacklo_epi8(xmm3, xmm7);
                xmm0 = _mm_unpacklo_epi8(xmm0, xmm7);
                xmm2 = _mm_unpacklo_epi8(xmm2, xmm7);
                srcCb += 4;
                srcCr += 4;
            } else if (incsp == FF_CSP_444P10) {
                // 4:4:4 YCbCr 10-bit
                xmm1 = _mm_loadl_epi64((const __m128i*)(srcCb));                       // xmm1 = Cb03,Cb02,Cb01,Cb00
                xmm3 = _mm_loadl_epi64((const __m128i*)(srcCb + stride_CbCr));         // xmm3 = Cb13,Cb12,Cb11,Cb10
                xmm0 = _mm_loadl_epi64((const __m128i*)(srcCr));                       // xmm0 = Cr03,Cr02,Cr01,Cr00
                xmm2 = _mm_loadl_epi64((const __m128i*)(srcCr + stride_CbCr));         // xmm2 = Cr13,Cr12,Cr11,Cr10
                srcCb += 8;
                srcCr += 8;
                // xmm1 = 16*P03, 16*P02, 16*P01, 16*P00 (14bit)
                // xmm3 = 16*P13, 16*P12, 16*P11, 16*P10 (14bit)
            }
            xmm1 = _mm_unpacklo_epi16(xmm1, xmm0);                                 // xmm1 = Cr03,Cb03,Cr02,Cb02,Cr01,Cb01,Cr00,Cb00
            xmm3 = _mm_unpacklo_epi16(xmm3, xmm2);                                 // xmm3 = Cr13,Cb13,Cr12,Cb12,Cr11,Cb11,Cr10,Cb10
            xmm1 = _mm_slli_epi16(xmm1, 4);
            xmm3 = _mm_slli_epi16(xmm3, 4);
        } else {
            if (incsp == FF_CSP_420P || incsp == FF_CSP_NV12 || incsp == FF_CSP_420P10) {
                // 4:2:0 color spaces
                if (incsp == FF_CSP_420P10) {
                    // 4:2:0 YCbCr 10bit
                    load42CbCr<left_edge, right_edge>(srcCb, srcCr, stride_CbCr, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5);
                    xmm0 = _mm_unpacklo_epi16(xmm1, xmm0);                                     // xmm0 = Cr02,Cb02,Cr01,Cb01,Cr00,Cb00,Cr0-1,Cb0-1
                    xmm2 = _mm_unpacklo_epi16(xmm3, xmm2);                                     // xmm2 = Cr12,Cb12,Cr11,Cb11,Cr10,Cb10,Cr1-1,Cb1-1
                } else if (incsp == FF_CSP_420P) {
                    // YV12
                    if (left_edge) {
                        uint32_t eax;
                        eax = *(uint32_t *)(srcCb);                                        // eax  = Cb03,Cb02,Cb01,Cb00
                        xmm1 = _mm_cvtsi32_si128((eax << 8) + (eax & 0xff));               // xmm1 = Cb02,Cb01,Cb00,Cb00
                        eax = *(uint32_t *)(srcCb + stride_CbCr);                          // eax  = Cb13,Cb12,Cb11,Cb10
                        xmm3 = _mm_cvtsi32_si128((eax << 8) + (eax & 0xff));               // xmm3 = Cb12,Cb11,Cb10,Cb10
                        eax = *(uint32_t *)(srcCr);
                        xmm0 = _mm_cvtsi32_si128((eax << 8) + (eax & 0xff));
                        eax = *(uint32_t *)(srcCr + stride_CbCr);
                        xmm2 = _mm_cvtsi32_si128((eax << 8) + (eax & 0xff));
                        srcCb ++;
                        srcCr ++;
                    } else if (right_edge) {
                        uint16_t ax;
                        uint8_t dl;
                        ax = *(uint16_t *)(srcCb);                                         // Cb00,Cb0-1
                        dl = *(uint8_t *)(srcCb + 2);                                      // Cb01
                        xmm1 = _mm_cvtsi32_si128((uint32_t(dl) << 24) + (uint32_t(dl) << 16) + ax); // xmm1 = Cb01,Cb01,Cb00,Cb0-1
                        ax = *(uint16_t *)(srcCr);                                         // Cr00,Cr0-1
                        dl = *(uint8_t *)(srcCr + 2);                                      // Cr01
                        xmm0 = _mm_cvtsi32_si128((uint32_t(dl) << 24) + (uint32_t(dl) << 16) + ax); // xmm0 = Cr01,Cr01,Cr00,Cr0-1
                        ax = *(uint16_t *)(srcCb + stride_CbCr);                           // Cb10,Cb1-1
                        dl = *(uint8_t *)(srcCb + stride_CbCr + 2);                        // Cb11
                        xmm3 = _mm_cvtsi32_si128((uint32_t(dl) << 24) + (uint32_t(dl) << 16) + ax); // xmm3 = Cb11,Cb11,Cb10,Cb1-1
                        ax = *(uint16_t *)(srcCr + stride_CbCr);                           // Cr10,Cr1-1
                        dl = *(uint8_t *)(srcCr + stride_CbCr + 2);                        // Cr11
                        xmm2 = _mm_cvtsi32_si128((uint32_t(dl) << 24) + (uint32_t(dl) << 16) + ax); // xmm2 = Cr11,Cr11,Cr10,Cr1-1
                    } else {
                        xmm1 = _mm_cvtsi32_si128(*(const int*)(srcCb));                   // xmm1 = Cb02,Cb01,Cb00,Cb0-1
                        xmm3 = _mm_cvtsi32_si128(*(const int*)(srcCb + stride_CbCr));     // xmm3 = Cb12,Cb11,Cb10,Cb1-0
                        xmm0 = _mm_cvtsi32_si128(*(const int*)(srcCr));                   // xmm0 = Cr02,Cr01,Cr00,Cr0-1
                        xmm2 = _mm_cvtsi32_si128(*(const int*)(srcCr + stride_CbCr));     // xmm2 = Cr12,Cr11,Cr10,Cr1-1
                        srcCb += 2;
                        srcCr += 2;
                    }
                    xmm0 = _mm_unpacklo_epi8(xmm1, xmm0);                                  // xmm0 = Cr02,Cb02,Cr01,Cb01,Cr00,Cb00,Cr-01,Cb0-0
                    xmm2 = _mm_unpacklo_epi8(xmm3, xmm2);                                  // xmm2 = Cr12,Cb12,Cr11,Cb11,Cr00,Cb00,Cr-11,Cb1-1
                    xmm0 = _mm_unpacklo_epi8(xmm0, xmm7);                                  // xmm0 = 0,Cr02,0,Cb02,0,Cr01,0,Cb01,0,Cr00,0,Cb00,0,Cr-01,0,Cb0-1
                    xmm2 = _mm_unpacklo_epi8(xmm2, xmm7);                                  // xmm2 = 0,Cr12,0,Cb12,0,Cr11,0,Cb11,0,Cr10,0,Cb10,0,Cr-11,0,Cb1-1
                } else {
                    // NV12
                    if (left_edge) {
                        xmm0 = _mm_loadl_epi64((const __m128i*)srcCb);                     // xmm0 = Cb03,Cr03,Cb02,Cr02,Cb01,Cr01,Cb00,Cr00
                        xmm2 = _mm_loadl_epi64((const __m128i*)(srcCb + stride_CbCr));     // xmm2 = Cb13,Cr13,Cb12,Cr12,Cb11,Cr11,Cb10,Cr10
                        xmm4 = xmm0;
                        xmm5 = xmm2;
                        xmm0 = _mm_slli_si128(xmm0, 2);                                    // xmm0 = Cb02,Cr02,Cb01,Cr01,Cb00,Cr00,0,0
                        xmm2 = _mm_slli_si128(xmm2, 2);                                    // xmm2 = Cb12,Cr12,Cb11,Cr11,Cb10,Cr10,0,0
                        xmm4 = _mm_slli_si128(xmm4, 14);                                   // clear upper 14 bytes
                        xmm5 = _mm_slli_si128(xmm5, 14);
                        xmm4 = _mm_srli_si128(xmm4, 14);
                        xmm5 = _mm_srli_si128(xmm5, 14);
                        xmm0 = _mm_or_si128(xmm0, xmm4);                                   // xmm0 = Cb02,Cr02,Cb01,Cr01,Cb00,Cr00,Cb00,Cr00
                        xmm2 = _mm_or_si128(xmm2, xmm5);                                   // xmm2 = Cb12,Cr12,Cb11,Cr11,Cb10,Cr10,Cb10,Cr10
                        srcCb += 2;
                    } else if (right_edge) {
                        xmm0 = _mm_loadl_epi64((const __m128i*)(srcCb - 2));               // xmm0 = Cb01,Cr01,Cb00,Cr00,Cb-01,Cr-01,Cb-02,Cb0-2
                        xmm2 = _mm_loadl_epi64((const __m128i*)(srcCb - 2 + stride_CbCr)); // xmm2 = Cb11,Cr11,Cb10,Cr10,Cb-11,Cr-11,Cb-12,Cb1-2
                        xmm4 = xmm0;
                        xmm5 = xmm2;
                        xmm0 = _mm_srli_si128(xmm0, 2);                                    // xmm0 = --,--,Cb01,Cr01,Cb00,Cr00,Cb-01,Cr0-1
                        xmm2 = _mm_srli_si128(xmm2, 2);                                    // xmm2 = --,--,Cb11,Cr11,Cb10,Cr10,Cb-11,Cr1-1

                        xmm4 = _mm_srli_si128(xmm4, 6);                                    // clear lower 6 bytes
                        xmm5 = _mm_srli_si128(xmm5, 6);
                        xmm4 = _mm_slli_si128(xmm4, 6);
                        xmm5 = _mm_slli_si128(xmm5, 6);
                        xmm0 = _mm_or_si128(xmm0, xmm4);                                   // xmm0 = Cb01,Cr01,Cb01,Cr01,Cb00,Cr00,Cb-01,Cr0-1
                        xmm2 = _mm_or_si128(xmm2, xmm5);                                   // xmm2 = Cb11,Cr11,Cb11,Cr11,Cb10,Cr10,Cb-11,Cr1-1
                    } else {
                        xmm0 = _mm_loadl_epi64((const __m128i*)(srcCb));                   // Cb02,Cr02,Cb01,Cr01,Cb00,Cr00,Cb-01,Cr0-1
                        xmm2 = _mm_loadl_epi64((const __m128i*)(srcCb + stride_CbCr));     // Cb12,Cr12,Cb11,Cr11,Cb10,Cr10,Cb-11,Cr1-1
                        srcCb += 4;
                    }
                    //xmm0 = _mm_shufflelo_epi16(xmm0,0xb1); xmm0 = _mm_shufflehi_epi16(xmm0,0xb1); xmm2 = ... // for NV21
                    xmm0 = _mm_unpacklo_epi8(xmm0, xmm7);
                    xmm2 = _mm_unpacklo_epi8(xmm2, xmm7);
                }

                xmm1 = xmm0;
                xmm1 = _mm_add_epi16(xmm1, xmm0);
                xmm1 = _mm_add_epi16(xmm1, xmm0);
                xmm1 = _mm_add_epi16(xmm1, xmm2);                                          // xmm1 = 3Cr02+Cr12,3Cb02+Cb12,... = P02,P01,P00,P0-1 (10bit)
                xmm3 = xmm2;
                xmm3 = _mm_add_epi16(xmm3, xmm2);
                xmm3 = _mm_add_epi16(xmm3, xmm2);
                xmm3 = _mm_add_epi16(xmm3, xmm0);                                          // xmm3 = Cr02+3Cr12,Cb02+3Cb12,... = P12,P11,P10,P1-1 (10bit)
            } else {
                // 4:2:2 color spaces
                if (incsp == FF_CSP_422P10) {
                    load42CbCr<left_edge, right_edge>(srcCb, srcCr, stride_CbCr, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5);
                    xmm1 = _mm_unpacklo_epi16(xmm1, xmm0);                                     // xmm1 = Cr02,Cb02,Cr01,Cb01,Cr00,Cb00,Cr0-1,Cb0-1
                    xmm3 = _mm_unpacklo_epi16(xmm3, xmm2);                                     // xmm2 = Cr12,Cb12,Cr11,Cb11,Cr10,Cb10,Cr1-1,Cb1-1
                } else if (incsp == FF_CSP_422P) {
                    // YV16
                    if (left_edge) {
                        uint32_t eax;
                        eax = *(uint32_t *)(srcCb);                                        // eax  = Cb03,Cb02,Cb01,Cb00
                        xmm0 = _mm_cvtsi32_si128((eax << 8) + (eax & 0xff));               // xmm0 = Cb02,Cb01,Cb00,Cb00
                        eax = *(uint32_t *)(srcCb + stride_CbCr);                          // eax  = Cb13,Cb12,Cb11,Cb10
                        xmm2 = _mm_cvtsi32_si128((eax << 8) + (eax & 0xff));               // xmm2 = Cb12,Cb11,Cb10,Cb10
                        eax = *(uint32_t *)(srcCr);
                        xmm1 = _mm_cvtsi32_si128((eax << 8) + (eax & 0xff));
                        eax = *(uint32_t *)(srcCr + stride_CbCr);
                        xmm3 = _mm_cvtsi32_si128((eax << 8) + (eax & 0xff));
                        srcCb ++;
                        srcCr ++;
                    } else if (right_edge) {
                        uint16_t ax;
                        uint8_t dl;
                        ax = *(uint16_t *)(srcCb);                                         // Cb00,Cb0-1
                        dl = *(uint8_t *)(srcCb + 2);                                      // Cb01
                        xmm0 = _mm_cvtsi32_si128((uint32_t(dl) << 24) + (uint32_t(dl) << 16) + ax); // xmm0 = Cb01,Cb01,Cb00,Cb0-1
                        ax = *(uint16_t *)(srcCb + stride_CbCr);                           // Cb10,Cb1-1
                        dl = *(uint8_t *)(srcCb + stride_CbCr + 2);                        // Cb11
                        xmm2 = _mm_cvtsi32_si128((uint32_t(dl) << 24) + (uint32_t(dl) << 16) + ax); // xmm1 = Cb11,Cb11,Cb10,Cb1-1
                        ax = *(uint16_t *)(srcCr);                                         // Cr00,Cr0-1
                        dl = *(uint8_t *)(srcCr + 2);                                      // Cr01
                        xmm1 = _mm_cvtsi32_si128((uint32_t(dl) << 24) + (uint32_t(dl) << 16) + ax); // xmm2 = Cr01,Cr01,Cr00,Cr0-1
                        ax = *(uint16_t *)(srcCr + stride_CbCr);                           // Cr10,Cr1-1
                        dl = *(uint8_t *)(srcCr + stride_CbCr + 2);                        // Cr11
                        xmm3 = _mm_cvtsi32_si128((uint32_t(dl) << 24) + (uint32_t(dl) << 16) + ax); // xmm3 = Cr11,Cr11,Cr10,Cr1-1
                    } else {
                        xmm0 = _mm_cvtsi32_si128(*(const int*)(srcCb));                   // xmm0 = Cb02,Cb01,Cb00,Cb0-1
                        xmm2 = _mm_cvtsi32_si128(*(const int*)(srcCb + stride_CbCr));     // xmm2 = Cb12,Cb11,Cb10,Cb1-0
                        xmm1 = _mm_cvtsi32_si128(*(const int*)(srcCr));                   // xmm1 = Cr02,Cr01,Cr00,Cr0-1
                        xmm3 = _mm_cvtsi32_si128(*(const int*)(srcCr + stride_CbCr));     // xmm3 = Cr12,Cr11,Cr10,Cr1-1
                        srcCb += 2;
                        srcCr += 2;
                    }
                    xmm1 = _mm_unpacklo_epi8(xmm0, xmm1);                                  // xmm1 = Cr02,Cb02,Cr01,Cb01,Cr00,Cb00,Cr0-1,Cb0-1
                    xmm3 = _mm_unpacklo_epi8(xmm2, xmm3);                                  // xmm2 = Cr12,Cb12,Cr11,Cb11,Cr00,Cb00,Cr1-1,Cb1-1
                    xmm1 = _mm_unpacklo_epi8(xmm1, xmm7);                                  // xmm1 = 0,Cr02,0,Cb02,0,Cr01,0,Cb01,0,Cr00,0,Cb00,0,Cr0-1,0,Cb0-1
                    xmm3 = _mm_unpacklo_epi8(xmm3, xmm7);                                  // xmm3 = 0,Cr12,0,Cb12,0,Cr11,0,Cb11,0,Cr10,0,Cb10,0,Cr1-1,0,Cb1-1
                } else if (incsp == FF_CSP_YUY2) {
                    if (left_edge) {
                        xmm1 = _mm_loadu_si128((const __m128i*)(srcY));                    // xmm1 = y,Cr03,y,Cb03,y,Cr02,y,Cb02,y,Cr01,y,Cb01,y,Cr00,y,Cb00
                        xmm3 = _mm_loadu_si128((const __m128i*)(srcY + stride_Y));         // xmm3 = y,Cr13,y,Cb13,y,Cr12,y,Cb12,y,Cr11,y,Cb11,y,Cr10,y,Cb10
                        xmm2 = xmm1;
                        xmm6 = xmm3;
                        xmm1 = _mm_slli_si128(xmm1, 4);                                    // xmm1 = y,Cr02,y,Cb02,y,Cr01,y,Cb01,y,Cr00,y,Cb00,0,0,0,0
                        xmm3 = _mm_slli_si128(xmm3, 4);                                    // xmm3 = y,Cr12,y,Cb12,y,Cr11,y,Cb11,y,Cr10,y,Cb10,0,0,0,0
                        xmm5 = xmm1;                                                       // For Y value. Next block does not use xmm4,5
                        xmm4 = xmm3;
                        xmm2 = _mm_slli_si128(xmm2, 12);                                   // clear upper 12bytes
                        xmm6 = _mm_slli_si128(xmm6, 12);
                        xmm2 = _mm_srli_si128(xmm2, 12);
                        xmm6 = _mm_srli_si128(xmm6, 12);
                        xmm1 = _mm_or_si128(xmm1, xmm2);
                        xmm3 = _mm_or_si128(xmm3, xmm6);

                        xmm1 = _mm_srli_epi16(xmm1, 8);                                    // xmm1 = 0,Cr02,0,Cb02,0,Cr01,0,Cb01,0,Cr00,0,Cb00,0,Cr00,0,Cb00
                        xmm3 = _mm_srli_epi16(xmm3, 8);                                    // xmm3 = 0,Cr12,0,Cb12,0,Cr11,0,Cb11,0,Cr10,0,Cb10,0,Cr10,0,Cb10

                        srcY += 4;
                    } else if (right_edge) {
                        xmm1 = _mm_loadu_si128((const __m128i*)(srcY - 4));                // xmm1 = y,Cr01,y,Cb01,y,Cr00,y,Cb00,y,Cr0-1,y,Cb0-1,y,Cr0-2,y,Cb0-2
                        xmm3 = _mm_loadu_si128((const __m128i*)(srcY - 4 + stride_Y));     // xmm3 = y,Cr11,y,Cb11,y,Cr10,y,Cb10,y,Cr1-1,y,Cb1-1,y,Cr1-2,y,Cb1-2
                        xmm2 = xmm1;
                        xmm6 = xmm3;
                        xmm1 = _mm_srli_si128(xmm1, 4);                                    // xmm1 = 0,0,0,0,y,Cr01,y,Cb01,y,Cr00,y,Cb00,Cr0-1,y,Cb0-1
                        xmm3 = _mm_srli_si128(xmm3, 4);                                    // xmm3 = 0,0,0,0,y,Cr11,y,Cb11,y,Cr10,y,Cb10,Cr1-1,y,Cb1-1
                        xmm5 = xmm1;                                                       // For Y value. Next block does not use xmm4,5
                        xmm4 = xmm3;
                        xmm2 = _mm_srli_si128(xmm2, 12);                                   // clear lower 12bytes
                        xmm6 = _mm_srli_si128(xmm6, 12);
                        xmm2 = _mm_slli_si128(xmm2, 12);
                        xmm6 = _mm_slli_si128(xmm6, 12);
                        xmm1 = _mm_or_si128(xmm1, xmm2);
                        xmm3 = _mm_or_si128(xmm3, xmm6);

                        xmm1 = _mm_srli_epi16(xmm1, 8);                                    // xmm1 = 0,Cr01,0,Cb01,0,Cr01,0,Cb01,0,Cr00,0,Cb00,0,Cr00,0,Cb00
                        xmm3 = _mm_srli_epi16(xmm3, 8);                                    // xmm3 = 0,Cr11,0,Cb11,0,Cr11,0,Cb11,0,Cr10,0,Cb10,0,Cr10,0,Cb10
                    } else {
                        xmm1 = _mm_loadu_si128((const __m128i*)(srcY));
                        xmm3 = _mm_loadu_si128((const __m128i*)(srcY + stride_Y));
                        xmm5 = xmm1;                                                       // For Y value. Next block does not use xmm4,5
                        xmm4 = xmm3;
                        xmm1 = _mm_srli_epi16(xmm1, 8);                                    // xmm1 = 0,Cr02,0,Cb02,0,Cr01,0,Cb01,0,Cr00,0,Cb00,0,Cr0-1,0,Cb0-1
                        xmm3 = _mm_srli_epi16(xmm3, 8);                                    // xmm3 = 0,Cr12,0,Cb12,0,Cr11,0,Cb11,0,Cr10,0,Cb10,0,Cr1-1,0,Cb1-1
                        srcY += 8;
                    }
                }
                xmm1 = _mm_slli_epi16(xmm1, 2);
                xmm3 = _mm_slli_epi16(xmm3, 2);
            }
            xmm2 = xmm1;
            if (isMPEG1) {
                // MPEG1 chroma position
                xmm2 = _mm_srli_si128(xmm2, 4);                                                // xmm2 = 0000,P02,P01,P00

                xmm0 = xmm1;
                xmm0 = _mm_add_epi16(xmm0, xmm1);
                xmm0 = _mm_add_epi16(xmm0, xmm1);
                xmm0 = _mm_add_epi16(xmm0, xmm2);                                              // xmm0 = ----,3*P01+P02,3*P00+P01,3*P-01+P00
                xmm0 = _mm_srli_si128(xmm0, 4);                                                // xmm0 = ----,----,3*P01+P02,3*P00+P01

                xmm1 = _mm_add_epi16(xmm1, xmm2);
                xmm1 = _mm_add_epi16(xmm1, xmm2);
                xmm1 = _mm_add_epi16(xmm1, xmm2);                                              // xmm1 = ----,----,P00+3*P01,P-01+3*P00

                xmm2 = xmm3;                                                                   // xmm2 = P12,P11,P10,P-11
                xmm6 = xmm3;
                xmm2 = _mm_srli_si128(xmm2, 4);                                                // xmm2 = 0000,P12,P11,P10

                xmm6 = _mm_add_epi16(xmm6, xmm3);
                xmm6 = _mm_add_epi16(xmm6, xmm3);
                xmm6 = _mm_add_epi16(xmm6, xmm2);                                              // xmm6 = ----,3*P11+P12,3*P10+P11,3*P-11+P10
                xmm6 = _mm_srli_si128(xmm6, 4);                                                // xmm6 = ----,----,3*P11+P12,3*P10+P11

                xmm3 = _mm_add_epi16(xmm3, xmm2);
                xmm3 = _mm_add_epi16(xmm3, xmm2);
                xmm3 = _mm_add_epi16(xmm3, xmm2);                                              // xmm3 = ----,----,P10+3*P11,P-11+3*P10

                xmm1 = _mm_unpacklo_epi32(xmm1, xmm0);                                         // 3*P01+P02, P00+3*P01, 3*P00+P01,P-01+3*P00
                xmm3 = _mm_unpacklo_epi32(xmm3, xmm6);                                         // 3*P11+P12, P10+3*P11, 3*P10+P01,P-11+3*P10
            } else {
                // MPEG2/4/AVC chroma position
                xmm1 = _mm_srli_si128(xmm1, 4);                                                // xmm1 = ----,----, P01, P00
                xmm2 = _mm_srli_si128(xmm2, 8);                                                // xmm2 = ----,----, P02, P01

                xmm0 = xmm1;                                                                   // xmm0 = ----,----, P01, P00
                xmm1 = _mm_slli_epi16(xmm1, 2);                                                // xmm1 = ----,----,4*P01,4*P00
                xmm0 = _mm_add_epi16(xmm0, xmm2);                                              // xmm0 = ----,----,P01+P02,P00+P01
                xmm0 = _mm_slli_epi16(xmm0, 1);                                                // xmm0 = ----,----,2*(P01+P02),2*(P00+P01)
                xmm1 = _mm_unpacklo_epi32(xmm1, xmm0);                                         // xmm1 = 2*(P01+P02),4*P01,2*(P00+P01),4*P00

                xmm6 = xmm3;                                                                   // xmm6 = P12,P11,P10,P-11
                xmm3 = _mm_srli_si128(xmm3, 4);                                                // xmm3 = ----,----,P11,P10
                xmm6 = _mm_srli_si128(xmm6, 8);                                                // xmm6 = ----,----,P12,P11

                xmm2 = xmm3;                                                                   // xmm2 = ----,----,P11,P10
                xmm3 = _mm_slli_epi16(xmm3, 2);                                                // xmm3 = ----,----,4*P11,4*P10
                xmm2 = _mm_add_epi16(xmm2, xmm6);                                              // xmm2 = ----,----,P11+P12,P10+P11
                xmm2 = _mm_slli_epi16(xmm2, 1);                                                // xmm2 = ----,----,2*(P11+P12),2*(P10+P11)
                xmm3 = _mm_unpacklo_epi32(xmm3, xmm2);                                         // xmm3 = 2*(P11+P12),4*P11,2*(P10+P11),4*P10
            }
        }

        xmm2 = coeffs->CbCr_center;
        xmm1 = _mm_subs_epi16(xmm1, xmm2);                                             // xmm1 = 2*(P01+P02) -128*16, 4*P01 -128*16, 2*(P00+P01) -128*16, 4*P00 -128*16 (12bit/14bit)
        xmm3 = _mm_subs_epi16(xmm3, xmm2);                                             // xmm3 = 2*(P11+P12) -128*16, 4*P11 -128*16, 2*(P10+P11) -128*16, 4*P10 -128*16 (12bit/14bit)
        // chroma upscaling finished.

        if (incsp & FF_CSPS_MASK_HIGH_BIT) {
            xmm5 = _mm_loadl_epi64((const __m128i*)(srcY));                                   // Y03,Y02,Y01,Y00
            xmm0 = _mm_loadl_epi64((const __m128i*)(srcY + stride_Y));                        // Y13,Y12,Y11,Y10
            srcY += 8;
        } else if (incsp != FF_CSP_YUY2) {
            xmm5 = _mm_cvtsi32_si128(*(int*)(srcY));                                   // Y03,Y02,Y01,Y00
            xmm0 = _mm_cvtsi32_si128(*(int*)(srcY + stride_Y));                        // Y13,Y12,Y11,Y10
            srcY += 4;
            xmm5 = _mm_unpacklo_epi8(xmm5, xmm7);                                      // 0,Y03,0,Y01,0,Y01,0,Y00
            xmm0 = _mm_unpacklo_epi8(xmm0, xmm7);                                      // 0,Y13,0,Y12,0,Y11,0,Y10
        } else {
            xmm0 = xmm4;
            // xmm5 = --,Y03,Y02,Y01,Y00,Y0-1,Y0-2
            xmm5 = _mm_srli_si128(xmm5, 4);                                            // xmm5 = --,Y03,Y02,Y01,Y00
            xmm0 = _mm_srli_si128(xmm0, 4);                                            // xmm0 = --,Y13,Y12,Y11,Y10
            xmm5 = _mm_slli_epi16(xmm5, 8);                                            // clear chroma
            xmm0 = _mm_slli_epi16(xmm0, 8);
            xmm5 = _mm_srli_epi16(xmm5, 8);
            xmm0 = _mm_srli_epi16(xmm0, 8);
        }
        xmm0 = _mm_unpacklo_epi64(xmm0, xmm5);                                         // 0,Y03,0,Y02,0,Y01,0,Y00,0,Y13,0,Y12,0,Y11,0,Y10

        xmm0 = _mm_subs_epu16(xmm0, coeffs->Ysub);                                     // Y-16, unsigned saturate
        xmm0 = _mm_slli_epi16(xmm0, 14 - bitDepth);
        xmm0 = _mm_mulhi_epi16(xmm0, coeffs->cy);                                      // Y*cy (12bit)
        xmm0 = _mm_add_epi16(xmm0, coeffs->rgb_add);
        xmm6 = xmm1;
        xmm4 = xmm3;
        xmm6 = _mm_madd_epi16(xmm6, coeffs->cR_Cr);
        xmm4 = _mm_madd_epi16(xmm4, coeffs->cR_Cr);
        xmm6 = _mm_srai_epi32(xmm6, bitDepth + 5);
        xmm4 = _mm_srai_epi32(xmm4, bitDepth + 5);
        xmm6 = _mm_packs_epi32(xmm6, xmm7);
        xmm4 = _mm_packs_epi32(xmm4, xmm7);
        xmm6 = _mm_unpacklo_epi64(xmm4, xmm6);
        xmm6 = _mm_add_epi16(xmm6, xmm0);                                              // R (12bit)
        xmm5 = xmm1;
        xmm4 = xmm3;
        xmm5 = _mm_madd_epi16(xmm5, coeffs->cG_Cb_cG_Cr);
        xmm4 = _mm_madd_epi16(xmm4, coeffs->cG_Cb_cG_Cr);
        xmm5 = _mm_srai_epi32(xmm5, bitDepth + 5);
        xmm4 = _mm_srai_epi32(xmm4, bitDepth + 5);
        xmm5 = _mm_packs_epi32(xmm5, xmm7);
        xmm4 = _mm_packs_epi32(xmm4, xmm7);
        xmm5 = _mm_unpacklo_epi64(xmm4, xmm5);
        xmm5 = _mm_add_epi16(xmm5, xmm0);                                              // G (12bit)
        xmm1 = _mm_madd_epi16(xmm1, coeffs->cB_Cb);
        xmm3 = _mm_madd_epi16(xmm3, coeffs->cB_Cb);
        xmm1 = _mm_srai_epi32(xmm1, bitDepth + 5);
        xmm3 = _mm_srai_epi32(xmm3, bitDepth + 5);
        xmm1 = _mm_packs_epi32(xmm1, xmm7);
        xmm3 = _mm_packs_epi32(xmm3, xmm7);
        xmm1 = _mm_unpacklo_epi64(xmm3, xmm1);
        xmm1 = _mm_add_epi16(xmm1, xmm0);                                              // B (12bit)

        if (dithering || bitDepth > 8) {
            xmm6 = _mm_add_epi16(xmm6, *(const __m128i *)(dither_ptr));
            xmm5 = _mm_add_epi16(xmm5, *(const __m128i *)(dither_ptr + 16));
            xmm1 = _mm_add_epi16(xmm1, *(const __m128i *)(dither_ptr + 32));
            dither_ptr += 8;
        }
        xmm6 = _mm_srai_epi16(xmm6, 4);
        xmm5 = _mm_srai_epi16(xmm5, 4);
        xmm1 = _mm_srai_epi16(xmm1, 4);

        xmm2 = _mm_cmpeq_epi8(xmm2, xmm2);                                             // 0xffffffff,0xffffffff,0xffffffff,0xffffffff
        xmm6 = _mm_packus_epi16(xmm6, xmm7);                                           // R (lower 8bytes,8bit) * 8
        xmm5 = _mm_packus_epi16(xmm5, xmm7);                                           // G (lower 8bytes,8bit) * 8
        xmm1 = _mm_packus_epi16(xmm1, xmm7);                                           // B (lower 8bytes,8bit) * 8
        if (outcsp == FF_CSP_RGB32 || outcsp == FF_CSP_RGB24) {
            // RGB
            xmm6 = _mm_unpacklo_epi8(xmm6, xmm2);                                      // 0xff,R
            xmm1 = _mm_unpacklo_epi8(xmm1, xmm5);                                      // G,B
            xmm2 = xmm1;
        } else {
            // BGR
            xmm6 = _mm_unpacklo_epi8(xmm6, xmm5);                                      // G,R
            xmm1 = _mm_unpacklo_epi8(xmm1, xmm2);                                      // 0xff,B
            xmm2 = xmm6;
            xmm6 = xmm1;
            xmm1 = xmm2;
        }

        xmm1 = _mm_unpackhi_epi16(xmm1, xmm6);                                         // 0xff,RGB * 4 (line 0)
        xmm2 = _mm_unpacklo_epi16(xmm2, xmm6);                                         // 0xff,RGB * 4 (line 1)

        if (rgb_limit) {
            xmm6 = coeffs->rgb_limit_low;
            xmm4 = coeffs->rgb_limit_high;
            xmm1 = _mm_max_epu8(xmm1, xmm6);
            xmm1 = _mm_min_epu8(xmm1, xmm4);
            xmm2 = _mm_max_epu8(xmm2, xmm6);
            xmm2 = _mm_min_epu8(xmm2, xmm4);
        }

        if (outcsp == FF_CSP_RGB32 || outcsp == FF_CSP_BGR32) {
            if (aligned) {
                // 6% faster
                _mm_stream_si128((__m128i *)(dst), xmm1);
                _mm_stream_si128((__m128i *)(dst + stride_dst), xmm2);
            } else {
                // Very rare cases. Don't optimize too much.
#ifdef WIN64
                // MSVC does not support MMX intrinsics on x64.
                _mm_storeu_si128((__m128i *)(dst), xmm1);
                _mm_storeu_si128((__m128i *)(dst + stride_dst), xmm2);
#else
                // SSE2 doesn't have un-aligned version of movntdq. Use MMX (movntq) here. This is much faster than movdqu.
                __m64 mm0, mm1, mm2, mm3;
                mm0 = _mm_movepi64_pi64(xmm1);
                xmm1 = _mm_srli_si128(xmm1, 8);
                mm1 = _mm_movepi64_pi64(xmm1);
                _mm_stream_pi((__m64 *)(dst), mm0);
                _mm_stream_pi((__m64 *)(dst + 8), mm1);

                mm2 = _mm_movepi64_pi64(xmm2);
                xmm2 = _mm_srli_si128(xmm2, 8);
                mm3 = _mm_movepi64_pi64(xmm2);
                _mm_stream_pi((__m64 *)(dst + stride_dst), mm2);
                _mm_stream_pi((__m64 *)(dst + stride_dst + 8), mm3);
#endif
            }
            dst += 16;
        } else { // RGB24,BGR24
            uint32_t eax;
            __align16(uint8_t, rgbbuf[32]);
            *(int *)(rgbbuf) = _mm_cvtsi128_si32(xmm1);
            xmm1 = _mm_srli_si128(xmm1, 4);
            *(int *)(rgbbuf + 3) = _mm_cvtsi128_si32(xmm1);
            xmm1 = _mm_srli_si128(xmm1, 4);
            *(int *)(rgbbuf + 6) = _mm_cvtsi128_si32(xmm1);
            xmm1 = _mm_srli_si128(xmm1, 4);
            *(int *)(rgbbuf + 9) = _mm_cvtsi128_si32(xmm1);

            *(int *)(rgbbuf + 16) = _mm_cvtsi128_si32(xmm2);
            xmm2 = _mm_srli_si128(xmm2, 4);
            *(int *)(rgbbuf + 19) = _mm_cvtsi128_si32(xmm2);
            xmm2 = _mm_srli_si128(xmm2, 4);
            *(int *)(rgbbuf + 22) = _mm_cvtsi128_si32(xmm2);
            xmm2 = _mm_srli_si128(xmm2, 4);
            *(int *)(rgbbuf + 25) = _mm_cvtsi128_si32(xmm2);

            xmm1 = _mm_loadl_epi64((const __m128i *)(rgbbuf));
            xmm2 = _mm_loadl_epi64((const __m128i *)(rgbbuf + 16));
#ifdef WIN64
            _mm_storel_epi64((__m128i *)(dst), xmm1);
            eax = *(uint32_t *)(rgbbuf + 8);
            *(uint32_t *)(dst + 8) = eax;
            _mm_storel_epi64((__m128i *)(dst + stride_dst), xmm2);
            eax = *(uint32_t *)(rgbbuf + 24);
            *(uint32_t *)(dst + stride_dst + 8) = eax;
#else
            __m64 mm0;
            mm0 = _mm_movepi64_pi64(xmm1);
            eax = *(uint32_t *)(rgbbuf + 8);
            _mm_stream_pi((__m64 *)(dst), mm0);
            _mm_stream_si32((int*)(dst + 8), eax);

            mm0 = _mm_movepi64_pi64(xmm2);
            eax = *(uint32_t *)(rgbbuf + 24);
            _mm_stream_pi((__m64 *)(dst + stride_dst), mm0);
            _mm_stream_si32((int*)(dst + stride_dst + 8), eax);
#endif
            dst += 12;
        }
    } while (--xCount);
}

void TffdshowConverters::init_dither(int width)
{
    if (!m_dithering) {
        return;
    }
    width = width * 4 + 64;
    if (width < m_old_width) {
        return;
    }
    m_old_width = width;
    if (dither) {
        _aligned_free(dither);
    }
    dither = (uint16_t *)_aligned_malloc(width * sizeof(uint16_t), 16);
    for (int i = 0 ; i < width ; i++) {
        dither[i] = rand() & 0xf;
    }
}

TffdshowConverters::TffdshowConverters(int thread_count) :
    m_thread_count((thread_count > 0 && thread_count <= MAX_THREADS) ? thread_count : 1),
    threadpool(m_thread_count),
    m_old_width(0),
    dither(NULL)
{
    m_coeffs = (Tcoeffs*)_aligned_malloc(sizeof(Tcoeffs), 16);
}

TffdshowConverters::~TffdshowConverters()
{
    if (dither) {
        _aligned_free(dither);
    }
    _aligned_free(m_coeffs);
}

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#pragma warning (pop)
#endif
