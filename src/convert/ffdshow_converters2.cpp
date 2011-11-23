/*
 * Copyright (c) 2011 h.yamagata
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
#include "ffdshow_converters2.h"
#include "simd.h"
#include "Tconfig.h"  // CPU flags

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#pragma warning (push)
#pragma warning (disable: 4799) // EMMS
#endif

void TffdshowConverters2::convert(
    const uint8_t* srcY,
    const uint8_t* srcCb,
    const uint8_t* srcCr,
    uint8_t* dstY,
    uint8_t* dstCbCr,
    int dx,
    int dy,
    stride_t stride_Y,
    stride_t stride_CbCr,
    stride_t stride_dstY,
    stride_t stride_dstCbCr)
{
#ifndef WIN64
    if (Tconfig::cpu_flags&FF_CPU_SSE2)
#endif
        convert_check_src_align<Tsse2>(srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
#ifndef WIN64
    else if (Tconfig::cpu_flags&FF_CPU_MMXEXT)
        convert_check_src_align<Tmmxext>(srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
    else
        convert_check_src_align<Tmmx>(srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
#endif
}

template <class _mm>static void TffdshowConverters2::convert_check_src_align(
    const uint8_t* srcY,
    const uint8_t* srcCb,
    const uint8_t* srcCr,
    uint8_t* dstY,
    uint8_t* dstCbCr,
    int dx,
    int dy,
    stride_t stride_Y,
    stride_t stride_CbCr,
    stride_t stride_dstY,
    stride_t stride_dstCbCr)
{
    if ((stride_Y & 0xf) || (stride_CbCr & 0xf) || (stride_t(srcY) & 0xf))
        convert_check_dst_align<0, _mm>(srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
    else
        convert_check_dst_align<1, _mm>(srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
}

template <int src_aligned, class _mm> void TffdshowConverters2::convert_check_dst_align(
    const uint8_t* srcY,
    const uint8_t* srcCb,
    const uint8_t* srcCr,
    uint8_t* dstY,
    uint8_t* dstCbCr,
    int dx,
    int dy,
    stride_t stride_Y,
    stride_t stride_CbCr,
    stride_t stride_dstY,
    stride_t stride_dstCbCr)
{
    if ((stride_dstY & 0xf) || (stride_dstCbCr & 0xf) || (stride_t(dstY) & 0xf) || (stride_t(dstCbCr) & 0xf))
        convert_simd<src_aligned, 0, _mm>(srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
    else
        convert_simd<src_aligned, 1, _mm>(srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
}

template <int src_aligned, int dst_aligned, class _mm> void TffdshowConverters2::convert_simd(
    const uint8_t* srcY,
    const uint8_t* srcCb,
    const uint8_t* srcCr,
    uint8_t* dstY,
    uint8_t* dstCbCr,
    int dx,
    int dy,
    stride_t stride_Y,
    stride_t stride_CbCr,
    stride_t stride_dstY,
    stride_t stride_dstCbCr)
{
    int xCount = dx * 2 / _mm::size;
    if (xCount <= 0)
        return;
    _mm::__m _mm0,_mm1;
    for (int y = 0 ; y < dy ; y++) {
        const uint8_t *src = srcY + y * stride_Y;
        uint8_t *dst = dstY + y * stride_dstY;
        int x = xCount;
        do {
            if (src_aligned)
                movVqa(_mm0, src);
            else
                movVqu(_mm0, src);
            psllw(_mm0, 6);
            src += _mm::size;
            if (dst_aligned || typeid(_mm) == typeid(Tmmxext))
                _mm::movntVq(dst, _mm0);
            else
                movVqu(dst, _mm0);
            dst += _mm::size;
        } while(--x);
    }
    int dyCbCr = dy/2;
    for (int y = 0 ; y < dyCbCr ; y++) {
        const uint8_t *Cb = srcCb + y * stride_CbCr;
        const uint8_t *Cr = srcCr + y * stride_CbCr;
        uint8_t *dst = dstCbCr + y * stride_dstCbCr;
        int x = xCount;
        do {
            _mm0 = _mm::load2(Cb);
            _mm1 = _mm::load2(Cr);
            Cb += _mm::size/2;
            Cr += _mm::size/2;
            punpcklwd(_mm0, _mm1);
            psllw(_mm0, 6);

            if (dst_aligned || typeid(_mm) == typeid(Tmmxext))
                _mm::movntVq(dst, _mm0);
            else
                movVqu(dst, _mm0);
            dst += _mm::size;
        } while(--x);
    }
    _mm::empty();
}

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
    #pragma warning (pop)
#endif
