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
    uint64_t incsp,
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
    // clean up input args to helf alignment checking
    if (incsp == FF_CSP_NV12)
        srcCr = NULL;
    if (incsp == FF_CSP_444P || incsp == FF_CSP_444P10)
        dstCbCr = NULL;

#ifndef WIN64
    if (Tconfig::cpu_flags&FF_CPU_SSE2)
#endif
        convert_check_src_align<Tsse2>(incsp, srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
#ifndef WIN64
    else if (Tconfig::cpu_flags&FF_CPU_MMXEXT)
        convert_check_src_align<Tmmxext>(incsp, srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
    else
        convert_check_src_align<Tmmx>(incsp, srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
#endif
}

bool TffdshowConverters2::csp_sup_ffdshow_converter2(uint64_t incsp, uint64_t outcsp)
{
    switch (incsp) {
        case FF_CSP_420P:
            if (outcsp == FF_CSP_P016 || outcsp == FF_CSP_P010)
                return true;
            else
                return false;
        case FF_CSP_420P10:
            if (outcsp == FF_CSP_P010 || outcsp == FF_CSP_P016)
                return true;
            else
                return false;
        case FF_CSP_444P10:
            if (outcsp == FF_CSP_Y416)
                return true;
            else
                return false;
        case FF_CSP_422P10:
            if (outcsp == FF_CSP_P210 || outcsp == FF_CSP_P216)
                return true;
            else
                return false;
        case FF_CSP_444P:
            if (outcsp == FF_CSP_AYUV)
                return true;
            else
                return false;
        case FF_CSP_NV12:
            if (outcsp == FF_CSP_P016 || outcsp == FF_CSP_P010)
                return true;
            else
                return false;
    }
}

template <class _mm> void TffdshowConverters2::convert_check_src_align(
    uint64_t incsp,
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
    if ((stride_Y & 0xf) || (stride_CbCr & 0xf) || (stride_t(srcY) & 0xf) || (stride_t(srcCb) & 0xf) || (stride_t(srcCr) & 0xf))
        convert_check_dst_align<_mm, 0>(incsp, srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
    else
        convert_check_dst_align<_mm, 1>(incsp, srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
}

template <class _mm, int src_aligned> void TffdshowConverters2::convert_check_dst_align(
    uint64_t incsp,
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
        convert_translate_incsp<_mm, src_aligned, 0>(incsp, srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
    else
        convert_translate_incsp<_mm, src_aligned, 1>(incsp, srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
}

template <class _mm, int src_aligned, int dst_aligned> void TffdshowConverters2::convert_translate_incsp(
    uint64_t incsp,
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
    if (incsp == FF_CSP_420P10)
        convert_simd<_mm, src_aligned, dst_aligned, FF_CSP_420P10>(srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
    else if (incsp == FF_CSP_420P)
        convert_simd<_mm, 1          , dst_aligned, FF_CSP_420P>  (srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
    else if (incsp == FF_CSP_NV12)
        convert_simd<_mm, 1          , dst_aligned, FF_CSP_NV12>  (srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
    else if (incsp == FF_CSP_422P10)
        convert_simd<_mm, src_aligned, dst_aligned, FF_CSP_422P10>(srcY, srcCb, srcCr, dstY, dstCbCr, dx, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
    else if (incsp == FF_CSP_444P)
        convert_simd_AYUV<_mm, src_aligned, dst_aligned>(srcY, srcCb, srcCr, dstY, dx, dy, stride_Y, stride_dstY);
    else if (incsp == FF_CSP_444P10)
        convert_simd_Y416<_mm, src_aligned, dst_aligned>(srcY, srcCb, srcCr, dstY, dx, dy, stride_Y, stride_dstY);
    else
        return;
}

template <class _mm, int src_aligned, int dst_aligned, uint64_t incsp> void TffdshowConverters2::convert_simd(
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

    _mm::__m _mm0,_mm1,_mm2,_mm3;
    _mm::__m zero;
    pxor(zero,zero);
    for (int y = 0 ; y < dy ; y++) {
        const uint8_t *src = srcY + y * stride_Y;
        uint8_t *dst = dstY + y * stride_dstY;
        int x = xCount;
        do {
            if (incsp == FF_CSP_420P10 || incsp == FF_CSP_422P10) {
                if (src_aligned)
                    movVqa(_mm0, src);
                else
                    movVqu(_mm0, src);
                psllw(_mm0, 6);
                src += _mm::size;
            } else if (incsp == FF_CSP_420P || incsp == FF_CSP_NV12) {
                movHalf(_mm1, src);
                _mm0 = zero;
                punpcklbw(_mm0, _mm1);
                src += _mm::size/2;
            }
            if (dst_aligned)
                _mm::movntVq(dst, _mm0);
            else
                movVqu(dst, _mm0);
            dst += _mm::size;
        } while(--x);
    }
    int dyCbCr = (incsp == FF_CSP_422P10) ? dy : dy/2;
    for (int y = 0 ; y < dyCbCr ; y++) {
        const uint8_t *Cb = srcCb + y * stride_CbCr;
        const uint8_t *Cr = srcCr + y * stride_CbCr;
        uint8_t *dst = dstCbCr + y * stride_dstCbCr;
        int x = xCount;
        do {
            if (incsp == FF_CSP_420P10 || incsp == FF_CSP_422P10) {
                movHalf(_mm0, Cb);
                movHalf(_mm1, Cr);
                Cb += _mm::size/2;
                Cr += _mm::size/2;
                punpcklwd(_mm0, _mm1);
                psllw(_mm0, 6);
            } else if (incsp == FF_CSP_420P) {
                movQuarter(_mm2, Cb);
                movQuarter(_mm3, Cr);
                _mm0 = zero;
                _mm1 = zero;
                Cb += _mm::size/4;
                Cr += _mm::size/4;
                punpcklbw(_mm0, _mm2);
                punpcklbw(_mm1, _mm3);
                punpcklwd(_mm0, _mm1);
            } else if (incsp == FF_CSP_NV12) {
                movHalf(_mm1, Cb);
                Cb += _mm::size/2;
                _mm0 = zero;
                punpcklbw(_mm0, _mm1);
            }

            if (dst_aligned)
                _mm::movntVq(dst, _mm0);
            else
                movVqu(dst, _mm0);
            dst += _mm::size;
        } while(--x);
    }

    if (xCount * (int)_mm::size < dx * 2 && dx > _mm::size/2) {
        int dxDone = dx - _mm::size / 2;
        switch (incsp) {
        case FF_CSP_420P:
            srcY  += dxDone;
            srcCb += dxDone/2;
            srcCr += dxDone/2;
            break;
        case FF_CSP_NV12:
            srcY  += dxDone;
            srcCb += dxDone;
            break;
        case FF_CSP_420P10:
        case FF_CSP_422P10:
            srcY  += dxDone * 2;
            srcCb += dxDone;
            srcCr += dxDone;
            break;
        }
        dstY    += dxDone * 2;
        dstCbCr += dxDone * 2;
        convert_simd<_mm, 0, 0, incsp>(srcY, srcCb, srcCr, dstY, dstCbCr, _mm::size/2, dy, stride_Y, stride_CbCr, stride_dstY, stride_dstCbCr);
    }

    _mm::empty();
}

template <class _mm, int src_aligned, int dst_aligned> void TffdshowConverters2::convert_simd_AYUV(
    const uint8_t* srcY,
    const uint8_t* srcCb,
    const uint8_t* srcCr,
    uint8_t* dst,
    int dx,
    int dy,
    stride_t stride_src,
    stride_t stride_dst)
{
    int xCount = dx / _mm::size;
    if (xCount <= 0)
        return;

    _mm::__m _mm0,_mm1,_mm2,_mm3,_mm4,_mm5;
    _mm::__m ffff;
    pxor(ffff,ffff);
    pcmpeqb(ffff,ffff);
    for (int y = 0 ; y < dy ; y++) {
        if (y == dy-1)
            int a=0;
        const uint8_t *Y = srcY + y * stride_src;
        uint8_t *dst1 = dst + y * stride_dst;
        const uint8_t *Cb = srcCb + y * stride_src;
        const uint8_t *Cr = srcCr + y * stride_src;
        int x = xCount;
        do {
            if (src_aligned) {
                movVqa(_mm1, Y);
                movVqa(_mm4, Cb);
                movVqa(_mm2, Cr);
            } else {
                movVqu(_mm1, Y);
                movVqu(_mm4, Cb);
                movVqu(_mm2, Cr);
            }
            Y  += _mm::size;
            Cb += _mm::size;
            Cr += _mm::size;
            _mm3 = _mm2;
            punpcklbw(_mm2, _mm4);
            punpckhbw(_mm3, _mm4);
            _mm5 = _mm1;
            punpcklbw(_mm1, ffff);
            punpckhbw(_mm5, ffff);
            _mm4 = _mm2;
            punpcklwd(_mm2, _mm1);
            punpckhwd(_mm4, _mm1);
            _mm0 = _mm3;
            punpcklwd(_mm0, _mm5);
            punpckhwd(_mm3, _mm5);
            if (dst_aligned) {
                _mm::movntVq(dst1,                 _mm2);
                _mm::movntVq(dst1 + _mm::size,     _mm4);
                _mm::movntVq(dst1 + _mm::size * 2, _mm0);
                _mm::movntVq(dst1 + _mm::size * 3, _mm3);
            } else {
                movVqu(dst1,                 _mm2);
                movVqu(dst1 + _mm::size,     _mm4);
                movVqu(dst1 + _mm::size * 2, _mm0);
                movVqu(dst1 + _mm::size * 3, _mm3);
            }
            dst1 += _mm::size * 4;
        } while(--x);
    }

    if (xCount * (int)_mm::size < dx && dx > _mm::size) {
        // handle non-mod 8 resolution.
        int dxDone = dx - _mm::size;
        srcY  += dxDone;
        srcCb += dxDone;
        srcCr += dxDone;
        dst   += dxDone * 4;
        convert_simd_AYUV<_mm, 0, 0>(srcY, srcCb, srcCr, dst, _mm::size, dy, stride_src, stride_dst);
    }

    _mm::empty();
}

template <class _mm, int src_aligned, int dst_aligned> void TffdshowConverters2::convert_simd_Y416(
    const uint8_t* srcY,
    const uint8_t* srcCb,
    const uint8_t* srcCr,
    uint8_t* dst,
    int dx,
    int dy,
    stride_t stride_src,
    stride_t stride_dst)
{
    int xCount = dx * 2 / _mm::size;
    if (xCount <= 0)
        return;

    _mm::__m _mm0,_mm1,_mm2,_mm3,_mm4,_mm5;
    _mm::__m ffff;
    pxor(ffff,ffff);
    pcmpeqb(ffff,ffff);
    for (int y = 0 ; y < dy ; y++) {
        const uint8_t *Y = srcY + y * stride_src;
        uint8_t *dst1 = dst + y * stride_dst;
        const uint8_t *Cb = srcCb + y * stride_src;
        const uint8_t *Cr = srcCr + y * stride_src;
        int x = xCount;
        do {
            if (src_aligned) {
                movVqa(_mm1, Y);
                movVqa(_mm2, Cb);
                movVqa(_mm4, Cr);
            } else {
                movVqu(_mm1, Y);
                movVqu(_mm2, Cb);
                movVqu(_mm4, Cr);
            }
            Y  += _mm::size;
            Cb += _mm::size;
            Cr += _mm::size;
            _mm5 = _mm1;
            punpcklwd(_mm1, _mm2);
            punpckhwd(_mm5, _mm2);
            _mm2 = ffff;
            _mm3 = ffff;
            punpcklwd(_mm2, _mm4);
            punpckhwd(_mm3, _mm4);
            _mm4 = _mm2;
            punpckldq(_mm2, _mm1);
            punpckhdq(_mm4, _mm1);
            _mm0 = _mm3;
            punpckldq(_mm0, _mm5);
            punpckhdq(_mm3, _mm5);
            psllw(_mm2, 6);
            psllw(_mm4, 6);
            psllw(_mm0, 6);
            psllw(_mm3, 6);
            if (dst_aligned) {
                _mm::movntVq(dst1,                 _mm2);
                _mm::movntVq(dst1 + _mm::size,     _mm4);
                _mm::movntVq(dst1 + _mm::size * 2, _mm0);
                _mm::movntVq(dst1 + _mm::size * 3, _mm3);
            } else {
                movVqu(dst1,                 _mm2);
                movVqu(dst1 + _mm::size,     _mm4);
                movVqu(dst1 + _mm::size * 2, _mm0);
                movVqu(dst1 + _mm::size * 3, _mm3);
            }
            dst1 += _mm::size * 4;
        } while(--x);
    }

    if (xCount * (int)_mm::size < dx * 2 && dx * 2 > _mm::size) {
        // handle non-mod 8 resolution.
        int dxDone = dx * 2 - _mm::size;
        srcY  += dxDone;
        srcCb += dxDone;
        srcCr += dxDone;
        dst   += dxDone * 4;
        convert_simd_Y416<_mm, 0, 0>(srcY, srcCb, srcCr, dst, _mm::size/2, dy, stride_src, stride_dst);
    }

    _mm::empty();
}

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
    #pragma warning (pop)
#endif
