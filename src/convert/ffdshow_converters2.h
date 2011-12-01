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
// 4:2:0 <-> 4:2:2 conversion is not implemented.
// If the input is 4:2:0, output must be 4:2:0.
// If the input is 4:2:2, output must be 4:2:2.
// Because there is no data member, no initialization required.
// just TffdshowConverters2::convert.
class TffdshowConverters2
{
public:
    static void convert(
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
        stride_t stride_dstCbCr);

private:
    template <class _mm> static void convert_check_src_align(
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
        stride_t stride_dstCbCr);

    template <class _mm, int src_aligned> static void convert_check_dst_align(
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
        stride_t stride_dstCbCr);

    template <class _mm, int src_aligned, int dst_aligned> static void convert_translate_incsp(
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
};
