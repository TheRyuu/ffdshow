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
#pragma once

#include "ffImgfmt.h"      // Just to use some names of color spaces

typedef ptrdiff_t stride_t;

//    converts FF_CSP_420P10, FF_CSP_420P or FF_CSP_NV12 to FF_CSP_P010/FF_CSP_P016 (MFVideoFormat_P010/16).
// or converts FF_CSP_422P10 to FF_CSP_P210/FF_CSP_P216 (MFVideoFormat_P210/MFVideoFormat_P216).
// or converts FF_CSP_444P to FF_CSP_AYUV
// or converts FF_CSP_444P10 to FF_CSP_Y416 or FF_CSP_Y410
// 4:2:0 <-> 4:2:2 conversion is not implemented.
// If the input is 4:2:0, the output is 4:2:0.
// If the input is 4:2:2, the output is 4:2:2.
// If the input is 4:4:4 8-bit, the output is 4:4:4 8-bit.
// If the input is 4:4:4 10-bit, the output is 4:4:4 10-bit.
// Because there is no data member, no initialization required.
// Just TffdshowConverters2::convert.
class TffdshowConverters2
{
public:
    static void convert(
        uint64_t incsp,
        uint64_t outcsp,
        const uint8_t* srcY,
        const uint8_t* srcCb,
        const uint8_t* srcCr,
        uint8_t* dstY,
        uint8_t* dstCb,
        uint8_t* dstCr,
        int dx,
        int dy,
        stride_t stride_Y,
        stride_t stride_CbCr,
        stride_t stride_dstY,
        stride_t stride_dstCbCr);

    static bool csp_sup_ffdshow_converter2(uint64_t incsp, uint64_t outcsp);
private:
    template <class _mm> static void convert_check_src_align(
        uint64_t incsp,
        uint64_t outcsp,
        const uint8_t* srcY,
        const uint8_t* srcCb,
        const uint8_t* srcCr,
        uint8_t* dstY,
        uint8_t* dstCb,
        uint8_t* dstCr,
        int dx,
        int dy,
        stride_t stride_Y,
        stride_t stride_CbCr,
        stride_t stride_dstY,
        stride_t stride_dstCbCr);

    template <class _mm, int src_aligned> static void convert_check_dst_align(
        uint64_t incsp,
        uint64_t outcsp,
        const uint8_t* srcY,
        const uint8_t* srcCb,
        const uint8_t* srcCr,
        uint8_t* dstY,
        uint8_t* dstCb,
        uint8_t* dstCr,
        int dx,
        int dy,
        stride_t stride_Y,
        stride_t stride_CbCr,
        stride_t stride_dstY,
        stride_t stride_dstCbCr);

    template <class _mm, int src_aligned, int dst_aligned> static void convert_translate_incsp(
        uint64_t incsp,
        uint64_t outcsp,
        const uint8_t* srcY,
        const uint8_t* srcCb,
        const uint8_t* srcCr,
        uint8_t* dstY,
        uint8_t* dstCb,
        uint8_t* dstCr,
        int dx,
        int dy,
        stride_t stride_Y,
        stride_t stride_CbCr,
        stride_t stride_dstY,
        stride_t stride_dstCbCr);

    template <class _mm, int src_aligned, int dst_aligned, uint64_t incsp> static void convert_simd(
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
        stride_t stride_dstCbCr);

    template <class _mm, int src_aligned, int dst_aligned> static void convert_YV12toNV12(
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
        stride_t stride_dstCbCr);

    template <class _mm, int src_aligned, int dst_aligned> static void convert_NV12toYV12(
        const uint8_t* srcY,
        const uint8_t* srcCbCr,
        uint8_t* dstY,
        uint8_t* dstCb,
        uint8_t* dstCr,
        int dx,
        int dy,
        stride_t stride_Y,
        stride_t stride_CbCr,
        stride_t stride_dstY,
        stride_t stride_dstCbCr);

    template <class _mm, int src_aligned, int dst_aligned> static void convert_simd_AYUV(
        const uint8_t* srcY,
        const uint8_t* srcCb,
        const uint8_t* srcCr,
        uint8_t* dst,
        int dx,
        int dy,
        stride_t stride_src,
        stride_t stride_dst);

    template <class _mm, int src_aligned, int dst_aligned> static void convert_simd_Y416(
        const uint8_t* srcY,
        const uint8_t* srcCb,
        const uint8_t* srcCr,
        uint8_t* dst,
        int dx,
        int dy,
        stride_t stride_src,
        stride_t stride_dst);

    template <class _mm, int src_aligned, int dst_aligned> static void convert_simd_GBRPtoRGB(
        const uint8_t* srcG,
        const uint8_t* srcB,
        const uint8_t* srcR,
        uint8_t* dst,
        int dx,
        int dy,
        stride_t stride_src,
        stride_t stride_dst);
};
