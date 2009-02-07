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

#include "stdafx.h"
#include "ffdshow_converters.h"
#include "simd_common.h" // __align16(t,v)

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
 #pragma warning (push)
 #pragma warning (disable: 4799) // EMMS
#endif

void TffdshowConverters::init(int incsp,   // FF_CSP_420P, FF_CSP_NV12, FF_CSP_YUY2 or FF_CSP_420P (progressive only)
           int outcsp,                     // FF_CSP_RGB32 or FF_CSP_RGB24
           ffYCbCr_RGB_MatrixCoefficientsType cspOptionsIturBt,  // ffYCbCr_RGB_coeff_ITUR_BT601, ffYCbCr_RGB_coeff_ITUR_BT709 or ffYCbCr_RGB_coeff_SMPTE240M
           int input_Y_white_level,        // input Y level (TV:235, PC:255)
           int input_Y_black_level,        // input Y level (TV:16, PC:0)
           int input_Cb_bluest_level,      // input chroma level (TV:16, PC:1)
           double output_RGB_white_level,  // output RGB level (TV:235, PC:255)
           double output_RGB_black_level)  // output RGB level (TV:16, PC:0)
{
    m_incsp = incsp;
    m_outcsp = outcsp;
    double Kr, Kg, Kb, chr_range, y_mul, vr_mul, ug_mul, vg_mul, ub_mul;
    int Ysub, RGB_add;
    if (output_RGB_white_level != 255 || output_RGB_black_level != 0)
        m_rgb_limit = true;
    else
        m_rgb_limit = false;
    YCbCr2RGBdata_common_inint(Kr,Kg,Kb,chr_range,y_mul,vr_mul,ug_mul,vg_mul,ub_mul,Ysub,RGB_add,
       cspOptionsIturBt,input_Y_white_level,input_Y_black_level,input_Cb_bluest_level,output_RGB_white_level,output_RGB_black_level);
    
    short cy =short(y_mul * 16384 + 0.5);
    short crv=short(vr_mul * 8192 + 0.5);
    short cgu=short(-ug_mul * 8192 - 0.5);
    short cgv=short(-vg_mul * 8192 - 0.5);
    short cbu=short(ub_mul * 8192 + 0.5);

    short *Ysubs = (short *)(m_coeffs + ofs_Ysub);
    Ysubs[0] = Ysubs[1] = Ysubs[2] = Ysubs[3] = Ysubs[4] = Ysubs[5] = Ysubs[6] = Ysubs[7] = short(Ysub);
    short *cys = (short *)(m_coeffs + ofs_cy);
    cys[0] = cys[1] = cys[2] = cys[3] = cys[4] = cys[5] = cys[6] = cys[7] = cy;
    short *sub128mul16 = (short *)(m_coeffs + ofs_128mul16);
    sub128mul16[0] = sub128mul16[1] = sub128mul16[2] = sub128mul16[3] =
    sub128mul16[4] = sub128mul16[5] = sub128mul16[6] = sub128mul16[7] = 128*16;

    // R
    short *cR_Crs = (short *)(m_coeffs + ofs_cR_Cr);
    cR_Crs[0] = cR_Crs[2] = cR_Crs[4] = cR_Crs[6] = 0;
    cR_Crs[1] = cR_Crs[3] = cR_Crs[5] = cR_Crs[7] = crv;
    // G
    short *cG_Cbs = (short *)(m_coeffs + ofs_cG_Cb_cG_Cr);
    cG_Cbs[0] = cG_Cbs[2] = cG_Cbs[4] = cG_Cbs[6] = cgu;
    cG_Cbs[1] = cG_Cbs[3] = cG_Cbs[5] = cG_Cbs[7] = cgv;
    // B
    short *cB_Cbs = (short *)(m_coeffs + ofs_cB_Cb);
    cB_Cbs[0] = cB_Cbs[2] = cB_Cbs[4] = cB_Cbs[6] = cbu;
    cB_Cbs[1] = cB_Cbs[3] = cB_Cbs[5] = cB_Cbs[7] = 0;

    short *rgb_adds = (short *)(m_coeffs + ofs_rgb_add);
    rgb_adds[0] = rgb_adds[1] = rgb_adds[2] = rgb_adds[3] = rgb_adds[4] = rgb_adds[5] = rgb_adds[6] = rgb_adds[7] = ((RGB_add & 0xff) << 2) + 2;
    uint32_t *xFF000000_FF000000s = (uint32_t *)(m_coeffs + ofs_xFF000000_FF000000);
    xFF000000_FF000000s[0] = xFF000000_FF000000s[1] = xFF000000_FF000000s[2] = xFF000000_FF000000s[3] = 0xff000000;

    uint32_t *xFFFFFFFF = (uint32_t *)(m_coeffs + ofs_xFFFFFFFF_FFFFFFFF);
    xFFFFFFFF[0] = xFFFFFFFF[1] = xFFFFFFFF[2] = xFFFFFFFF[3] = 0xffffffff;

    uint32_t *rgb_limit_highs = (uint32_t *)(m_coeffs + ofs_rgb_limit_high);
    uint32_t rgb_white = uint32_t(output_RGB_white_level);
    rgb_white = 0xff000000 + (rgb_white << 16) + (rgb_white << 8) + rgb_white;
    rgb_limit_highs[0] = rgb_limit_highs[1] = rgb_limit_highs[2] = rgb_limit_highs[3] = rgb_white;

    uint32_t *rgb_limit_lows = (uint32_t *)(m_coeffs + ofs_rgb_limit_low);
    uint32_t rgb_black = uint32_t(output_RGB_black_level);
    rgb_black = 0xff000000 + (rgb_black << 16) + (rgb_black << 8) + rgb_black;
    rgb_limit_lows[0] = rgb_limit_lows[1] = rgb_limit_lows[2] = rgb_limit_lows[3] = rgb_black;
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
    if (m_rgb_limit)
        convert_translate_incsp<1>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
    else
        convert_translate_incsp<0>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);

#ifndef WIN64
    _mm_empty();
#endif
}


template<int incsp, int outcsp, int left_edge, int right_edge, int rgb_limit, int aligned> static __forceinline 
  void TffdshowConverters::convert_a_unit(const unsigned char* &srcY,
                      const unsigned char* &srcCb,
                      const unsigned char* &srcCr,
                      unsigned char* &dst,
                      const stride_t stride_Y,
                      const stride_t stride_CbCr,
                      const stride_t stride_dst,
                      const unsigned char* const coeffs)
{
    // output 4x2 RGB pixels.
    __m128i xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7;
    xmm7 = _mm_setzero_si128 ();

    if (incsp == FF_CSP_420P || incsp == FF_CSP_NV12) {
        // 4:2:0 color spaces
        if (incsp == FF_CSP_420P) {
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
                xmm1 = _mm_loadl_epi64((const __m128i*)(srcCb));                   // xmm1 = Cb02,Cb01,Cb00,Cb0-1
                xmm3 = _mm_loadl_epi64((const __m128i*)(srcCb + stride_CbCr));     // xmm3 = Cb12,Cb11,Cb10,Cb1-0
                xmm0 = _mm_loadl_epi64((const __m128i*)(srcCr));                   // xmm0 = Cr02,Cr01,Cr00,Cr0-1
                xmm2 = _mm_loadl_epi64((const __m128i*)(srcCr + stride_CbCr));     // xmm2 = Cr12,Cr11,Cr10,Cr1-1
                srcCb += 2;
                srcCr += 2;
            }
            xmm0 = _mm_unpacklo_epi8(xmm1,xmm0);                                   // xmm0 = Cr02,Cb02,Cr01,Cb01,Cr00,Cb00,Cr-01,Cb0-0
            xmm2 = _mm_unpacklo_epi8(xmm3,xmm2);                                   // xmm2 = Cr12,Cb12,Cr11,Cb11,Cr00,Cb00,Cr-11,Cb1-1
            xmm0 = _mm_unpacklo_epi8(xmm0,xmm7);                                   // xmm0 = 0,Cr02,0,Cb02,0,Cr01,0,Cb01,0,Cr00,0,Cb00,0,Cr-01,0,Cb0-1
            xmm2 = _mm_unpacklo_epi8(xmm2,xmm7);                                   // xmm2 = 0,Cr12,0,Cb12,0,Cr11,0,Cb11,0,Cr10,0,Cb10,0,Cr-11,0,Cb1-1
        } else /*NV12*/ {
            if (left_edge) {
                xmm0 = _mm_loadl_epi64((const __m128i*)srcCb);                     // xmm0 = Cb03,Cr03,Cb02,Cr02,Cb01,Cr01,Cb00,Cr00
                xmm2 = _mm_loadl_epi64((const __m128i*)(srcCb + stride_CbCr));     // xmm2 = Cb13,Cr13,Cb12,Cr12,Cb11,Cr11,Cb10,Cr10
                xmm4 = xmm0;
                xmm5 = xmm2;
                xmm0 = _mm_slli_si128(xmm0,2);                                     // xmm0 = Cb02,Cr02,Cb01,Cr01,Cb00,Cr00,0,0
                xmm2 = _mm_slli_si128(xmm2,2);                                     // xmm2 = Cb12,Cr12,Cb11,Cr11,Cb10,Cr10,0,0
                xmm4 = _mm_slli_si128(xmm4,14);                                    // clear upper 14 bytes
                xmm5 = _mm_slli_si128(xmm5,14);
                xmm4 = _mm_srli_si128(xmm4,14);
                xmm5 = _mm_srli_si128(xmm5,14);
                xmm0 = _mm_or_si128(xmm0,xmm4);                                    // xmm0 = Cb02,Cr02,Cb01,Cr01,Cb00,Cr00,Cb00,Cr00
                xmm2 = _mm_or_si128(xmm2,xmm5);                                    // xmm2 = Cb12,Cr12,Cb11,Cr11,Cb10,Cr10,Cb10,Cr10
                srcCb += 2;
            } else if (right_edge) {
                xmm0 = _mm_loadl_epi64((const __m128i*)(srcCb - 2));               // xmm0 = Cb01,Cr01,Cb00,Cr00,Cb-01,Cr-01,Cb-02,Cb0-2
                xmm2 = _mm_loadl_epi64((const __m128i*)(srcCb - 2 + stride_CbCr)); // xmm2 = Cb11,Cr11,Cb10,Cr10,Cb-11,Cr-11,Cb-12,Cb1-2
                xmm4 = xmm0;
                xmm5 = xmm2;
                xmm0 = _mm_srli_si128(xmm0,2);                                     // xmm0 = --,--,Cb01,Cr01,Cb00,Cr00,Cb-01,Cr0-1
                xmm2 = _mm_srli_si128(xmm2,2);                                     // xmm2 = --,--,Cb11,Cr11,Cb10,Cr10,Cb-11,Cr1-1

                xmm4 = _mm_srli_si128(xmm4,6);                                     // clear lower 6 bytes
                xmm5 = _mm_srli_si128(xmm5,6);
                xmm4 = _mm_slli_si128(xmm4,6);
                xmm5 = _mm_slli_si128(xmm5,6);
                xmm0 = _mm_or_si128(xmm0,xmm4);                                    // xmm0 = Cb01,Cr01,Cb01,Cr01,Cb00,Cr00,Cb-01,Cr0-1
                xmm2 = _mm_or_si128(xmm2,xmm5);                                    // xmm2 = Cb11,Cr11,Cb11,Cr11,Cb10,Cr10,Cb-11,Cr1-1
            } else {
                xmm0 = _mm_loadl_epi64((const __m128i*)(srcCb));                   // Cb02,Cr02,Cb01,Cr01,Cb00,Cr00,Cb-01,Cr0-1
                xmm2 = _mm_loadl_epi64((const __m128i*)(srcCb + stride_CbCr));     // Cb12,Cr12,Cb11,Cr11,Cb10,Cr10,Cb-11,Cr1-1
                srcCb += 4;
            }
            //xmm0 = _mm_shufflelo_epi16(xmm0,0xb1); xmm0 = _mm_shufflehi_epi16(xmm0,0xb1); xmm2 = ... // for NV21
            xmm0 = _mm_unpacklo_epi8(xmm0,xmm7);
            xmm2 = _mm_unpacklo_epi8(xmm2,xmm7);
        }

        xmm1 = xmm0;
        xmm1 = _mm_add_epi16(xmm1,xmm0);
        xmm1 = _mm_add_epi16(xmm1,xmm0);
        xmm1 = _mm_add_epi16(xmm1,xmm2);                                           // xmm1 = 3Cr02+Cr12,3Cb02+Cb12,... = P02,P01,P00,P0-1 (10bit)
        xmm3 = xmm2;
        xmm3 = _mm_add_epi16(xmm3,xmm2);
        xmm3 = _mm_add_epi16(xmm3,xmm2);
        xmm3 = _mm_add_epi16(xmm3,xmm0);                                           // xmm3 = Cr02+3Cr12,Cb02+3Cb12,... = P12,P11,P10,P1-1 (10bit)
    } else {
        // 4:2:2 color spaces
        if (incsp == FF_CSP_422P) {
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
                xmm0 = _mm_loadl_epi64((const __m128i*)(srcCb));                   // xmm0 = Cb02,Cb01,Cb00,Cb0-1
                xmm2 = _mm_loadl_epi64((const __m128i*)(srcCb + stride_CbCr));     // xmm2 = Cb12,Cb11,Cb10,Cb1-0
                xmm1 = _mm_loadl_epi64((const __m128i*)(srcCr));                   // xmm1 = Cr02,Cr01,Cr00,Cr0-1
                xmm3 = _mm_loadl_epi64((const __m128i*)(srcCr + stride_CbCr));     // xmm3 = Cr12,Cr11,Cr10,Cr1-1
                srcCb += 2;
                srcCr += 2;
            }
            xmm1 = _mm_unpacklo_epi8(xmm0,xmm1);                                   // xmm1 = Cr02,Cb02,Cr01,Cb01,Cr00,Cb00,Cr0-1,Cb0-1
            xmm3 = _mm_unpacklo_epi8(xmm2,xmm3);                                   // xmm2 = Cr12,Cb12,Cr11,Cb11,Cr00,Cb00,Cr1-1,Cb1-1
            xmm1 = _mm_unpacklo_epi8(xmm1,xmm7);                                   // xmm1 = 0,Cr02,0,Cb02,0,Cr01,0,Cb01,0,Cr00,0,Cb00,0,Cr0-1,0,Cb0-1
            xmm3 = _mm_unpacklo_epi8(xmm3,xmm7);                                   // xmm3 = 0,Cr12,0,Cb12,0,Cr11,0,Cb11,0,Cr10,0,Cb10,0,Cr1-1,0,Cb1-1
        } else if (incsp == FF_CSP_YUY2) {
            if (left_edge) {
                xmm1 = _mm_loadu_si128((const __m128i*)(srcY));                    // xmm1 = y,Cr03,y,Cb03,y,Cr02,y,Cb02,y,Cr01,y,Cb01,y,Cr00,y,Cb00
                xmm3 = _mm_loadu_si128((const __m128i*)(srcY + stride_Y));         // xmm3 = y,Cr13,y,Cb13,y,Cr12,y,Cb12,y,Cr11,y,Cb11,y,Cr10,y,Cb10
                xmm2 = xmm1;
                xmm6 = xmm3;
                xmm1 = _mm_slli_si128(xmm1,4);                                     // xmm1 = y,Cr02,y,Cb02,y,Cr01,y,Cb01,y,Cr00,y,Cb00,0,0,0,0
                xmm3 = _mm_slli_si128(xmm3,4);                                     // xmm3 = y,Cr12,y,Cb12,y,Cr11,y,Cb11,y,Cr10,y,Cb10,0,0,0,0
                xmm5 = xmm1;                                                       // For Y value. Next block does not use xmm4,5
                xmm4 = xmm3;
                xmm2 = _mm_slli_si128(xmm2,12);                                    // clear upper 12bytes
                xmm6 = _mm_slli_si128(xmm6,12);
                xmm2 = _mm_srli_si128(xmm2,12);
                xmm6 = _mm_srli_si128(xmm6,12);
                xmm1 = _mm_or_si128(xmm1,xmm2);
                xmm3 = _mm_or_si128(xmm3,xmm6);

                xmm1 = _mm_srli_epi16(xmm1,8);                                     // xmm1 = 0,Cr02,0,Cb02,0,Cr01,0,Cb01,0,Cr00,0,Cb00,0,Cr00,0,Cb00
                xmm3 = _mm_srli_epi16(xmm3,8);                                     // xmm3 = 0,Cr12,0,Cb12,0,Cr11,0,Cb11,0,Cr10,0,Cb10,0,Cr10,0,Cb10

                srcY += 4;
            } else if (right_edge) {
                xmm1 = _mm_loadu_si128((const __m128i*)(srcY - 4));                // xmm1 = y,Cr01,y,Cb01,y,Cr00,y,Cb00,y,Cr0-1,y,Cb0-1,y,Cr0-2,y,Cb0-2
                xmm3 = _mm_loadu_si128((const __m128i*)(srcY - 4 + stride_Y));     // xmm3 = y,Cr11,y,Cb11,y,Cr10,y,Cb10,y,Cr1-1,y,Cb1-1,y,Cr1-2,y,Cb1-2
                xmm2 = xmm1;
                xmm6 = xmm3;
                xmm1 = _mm_srli_si128(xmm1,4);                                     // xmm1 = 0,0,0,0,y,Cr01,y,Cb01,y,Cr00,y,Cb00,Cr0-1,y,Cb0-1
                xmm3 = _mm_srli_si128(xmm3,4);                                     // xmm3 = 0,0,0,0,y,Cr11,y,Cb11,y,Cr10,y,Cb10,Cr1-1,y,Cb1-1
                xmm5 = xmm1;                                                       // For Y value. Next block does not use xmm4,5
                xmm4 = xmm3;
                xmm2 = _mm_srli_si128(xmm2,12);                                    // clear lower 12bytes
                xmm6 = _mm_srli_si128(xmm6,12);
                xmm2 = _mm_slli_si128(xmm2,12);
                xmm6 = _mm_slli_si128(xmm6,12);
                xmm1 = _mm_or_si128(xmm1,xmm2);
                xmm3 = _mm_or_si128(xmm3,xmm6);

                xmm1 = _mm_srli_epi16(xmm1,8);                                     // xmm1 = 0,Cr01,0,Cb01,0,Cr01,0,Cb01,0,Cr00,0,Cb00,0,Cr00,0,Cb00
                xmm3 = _mm_srli_epi16(xmm3,8);                                     // xmm3 = 0,Cr11,0,Cb11,0,Cr11,0,Cb11,0,Cr10,0,Cb10,0,Cr10,0,Cb10
            } else {
                xmm1 = _mm_loadu_si128((const __m128i*)(srcY));
                xmm3 = _mm_loadu_si128((const __m128i*)(srcY + stride_Y));
                xmm5 = xmm1;                                                       // For Y value. Next block does not use xmm4,5
                xmm4 = xmm3;
                xmm1 = _mm_srli_epi16(xmm1,8);                                     // xmm1 = 0,Cr02,0,Cb02,0,Cr01,0,Cb01,0,Cr00,0,Cb00,0,Cr0-1,0,Cb0-1
                xmm3 = _mm_srli_epi16(xmm3,8);                                     // xmm3 = 0,Cr12,0,Cb12,0,Cr11,0,Cb11,0,Cr10,0,Cb10,0,Cr1-1,0,Cb1-1
                srcY += 8;
            }
        }
        xmm1 = _mm_slli_epi16(xmm1,2);
        xmm3 = _mm_slli_epi16(xmm3,2);
    }
    xmm2 = xmm1;
    xmm2 = _mm_srli_si128(xmm2,4);                                                 // xmm2 = 0000,P02,P01,P00

    xmm0 = xmm1;
    xmm0 = _mm_add_epi16(xmm0,xmm1);
    xmm0 = _mm_add_epi16(xmm0,xmm1);
    xmm0 = _mm_add_epi16(xmm0,xmm2);                                               // xmm0 = ----,3*P01+P02,3*P00+P01,3*P-01+P00
    xmm0 = _mm_srli_si128(xmm0,4);                                                 // xmm0 = ----,----,3*P01+P02,3*P00+P01

    xmm1 = _mm_add_epi16(xmm1,xmm2);
    xmm1 = _mm_add_epi16(xmm1,xmm2);
    xmm1 = _mm_add_epi16(xmm1,xmm2);                                               // xmm1 = ----,----,P00+3*P01,P-01+3*P00

    xmm2 = xmm3;                                                                   // xmm2 = P12,P11,P10,P-11
    xmm6 = xmm3;
    xmm2 = _mm_srli_si128(xmm2, 4);                                                // xmm2 = 0000,P12,P11,P10

    xmm6 = _mm_add_epi16(xmm6,xmm3);
    xmm6 = _mm_add_epi16(xmm6,xmm3);
    xmm6 = _mm_add_epi16(xmm6,xmm2);                                               // xmm6 = ----,3*P11+P12,3*P10+P11,3*P-11+P10
    xmm6 = _mm_srli_si128(xmm6,4);                                                 // xmm6 = ----,----,3*P11+P12,3*P10+P11

    xmm3 = _mm_add_epi16(xmm3,xmm2);
    xmm3 = _mm_add_epi16(xmm3,xmm2);
    xmm3 = _mm_add_epi16(xmm3,xmm2);                                               // xmm3 = ----,----,P10+3*P11,P-11+3*P10

    xmm2 = _mm_load_si128((const __m128i *)(coeffs + ofs_128mul16));
    xmm1 = _mm_unpacklo_epi32(xmm1,xmm0);                                          // 3*P01+P02, P00+3*P01, 3*P00+P01,P-01+3*P00
    xmm3 = _mm_unpacklo_epi32(xmm3,xmm6);                                          // 3*P11+P12, P10+3*P11, 3*P10+P01,P-11+3*P10

    xmm1 = _mm_subs_epi16(xmm1, xmm2);                                             // xmm1 = P01+3*P02 -128*16, 3*P01+P02 -128*16, P00+3*P01 -128*16, 3*P00+P01 -128*16 (12bit)
    xmm3 = _mm_subs_epi16(xmm3, xmm2);                                             // xmm3 = P11+3*P12 -128*16, 3*P11+P12 -128*16, P10+3*P11 -128*16, 3*P10+P01 -128*16 (12bit)
    // chroma upscaling finished.

    if (incsp != FF_CSP_YUY2) {
        xmm5 = _mm_cvtsi32_si128(*(int*)(srcY));                                   // Y03,Y02,Y01,Y00
        xmm0 = _mm_cvtsi32_si128(*(int*)(srcY + stride_Y));                        // Y13,Y12,Y11,Y10
        srcY += 4;
        xmm5 = _mm_unpacklo_epi8(xmm5,xmm7);                                       // 0,Y03,0,Y01,0,Y01,0,Y00
        xmm0 = _mm_unpacklo_epi8(xmm0,xmm7);                                       // 0,Y13,0,Y12,0,Y11,0,Y10
    } else {
        xmm0 = xmm4;
                                                                                   // xmm5 = --,Y03,Y02,Y01,Y00,Y0-1,Y0-2
        xmm5 = _mm_srli_si128(xmm5,4);                                             // xmm5 = --,Y03,Y02,Y01,Y00
        xmm0 = _mm_srli_si128(xmm0,4);                                             // xmm0 = --,Y13,Y12,Y11,Y10
        xmm5 = _mm_slli_epi16(xmm5,8);                                             // clear chroma
        xmm0 = _mm_slli_epi16(xmm0,8);
        xmm5 = _mm_srli_epi16(xmm5,8);
        xmm0 = _mm_srli_epi16(xmm0,8);
    }
    xmm0 = _mm_unpacklo_epi64(xmm0,xmm5);                                          // 0,Y03,0,Y02,0,Y01,0,Y00,0,Y13,0,Y12,0,Y11,0,Y10

    xmm0 = _mm_subs_epu16(xmm0,*(const __m128i *)(coeffs + ofs_Ysub));             // Y-16, unsigned saturate
    xmm0 = _mm_slli_epi16(xmm0,4);
    xmm0 = _mm_mulhi_epi16(xmm0,*(const __m128i *)(coeffs + ofs_cy));              // Y*cy (10bit)
    xmm0 = _mm_add_epi16(xmm0,*(const __m128i *)(coeffs + ofs_rgb_add));
    xmm6 = xmm1;
    xmm4 = xmm3;
    xmm6 = _mm_madd_epi16(xmm6,*(const __m128i *)(coeffs + ofs_cR_Cr));
    xmm4 = _mm_madd_epi16(xmm4,*(const __m128i *)(coeffs + ofs_cR_Cr));
    xmm6 = _mm_srai_epi32(xmm6,15);
    xmm4 = _mm_srai_epi32(xmm4,15);
    xmm6 = _mm_packs_epi32(xmm6,xmm7);
    xmm4 = _mm_packs_epi32(xmm4,xmm7);
    xmm6 = _mm_unpacklo_epi64(xmm4,xmm6);
    xmm6 = _mm_add_epi16(xmm6,xmm0);                                               // R (10bit)
    xmm5 = xmm1;
    xmm4 = xmm3;
    xmm5 = _mm_madd_epi16(xmm5,*(const __m128i *)(coeffs + ofs_cG_Cb_cG_Cr));
    xmm4 = _mm_madd_epi16(xmm4,*(const __m128i *)(coeffs + ofs_cG_Cb_cG_Cr));
    xmm5 = _mm_srai_epi32(xmm5,15);
    xmm4 = _mm_srai_epi32(xmm4,15);
    xmm5 = _mm_packs_epi32(xmm5,xmm7);
    xmm4 = _mm_packs_epi32(xmm4,xmm7);
    xmm5 = _mm_unpacklo_epi64(xmm4,xmm5);
    xmm5 = _mm_add_epi16(xmm5,xmm0);                                               // G (10bit)
    xmm1 = _mm_madd_epi16(xmm1,*(const __m128i *)(coeffs + ofs_cB_Cb));
    xmm3 = _mm_madd_epi16(xmm3,*(const __m128i *)(coeffs + ofs_cB_Cb));
    xmm1 = _mm_srai_epi32(xmm1,15);
    xmm3 = _mm_srai_epi32(xmm3,15);
    xmm1 = _mm_packs_epi32(xmm1,xmm7);
    xmm3 = _mm_packs_epi32(xmm3,xmm7);
    xmm1 = _mm_unpacklo_epi64(xmm3,xmm1);
    xmm1 = _mm_add_epi16(xmm1,xmm0);                                               // B (10bit)
    xmm6 = _mm_srai_epi16(xmm6,2);
    xmm5 = _mm_srai_epi16(xmm5,2);
    xmm1 = _mm_srai_epi16(xmm1,2);
    xmm2 = _mm_load_si128((const __m128i *)(coeffs + ofs_xFFFFFFFF_FFFFFFFF));
    xmm6 = _mm_packus_epi16(xmm6,xmm7);                                            // R (lower 8bytes,8bit) * 8
    xmm5 = _mm_packus_epi16(xmm5,xmm7);                                            // G (lower 8bytes,8bit) * 8
    xmm1 = _mm_packus_epi16(xmm1,xmm7);                                            // B (lower 8bytes,8bit) * 8
    xmm6 = _mm_unpacklo_epi8(xmm6,xmm2);                                           // 0xff,R
    xmm1 = _mm_unpacklo_epi8(xmm1,xmm5);                                           // G,B
    xmm2 = xmm1;
    xmm1 = _mm_unpackhi_epi16(xmm1,xmm6);                                          // 0xff,RGB * 4 (line 0)
    xmm2 = _mm_unpacklo_epi16(xmm2,xmm6);                                          // 0xff,RGB * 4 (line 1)

    if (rgb_limit) {
        xmm6 = _mm_load_si128((const __m128i *)(coeffs + ofs_rgb_limit_low));
        xmm4 = _mm_load_si128((const __m128i *)(coeffs + ofs_rgb_limit_high));
        xmm1 = _mm_max_epu8(xmm1,xmm6);
        xmm1 = _mm_min_epu8(xmm1,xmm4);
        xmm2 = _mm_max_epu8(xmm2,xmm6);
        xmm2 = _mm_min_epu8(xmm2,xmm4);
    }

    if (outcsp == FF_CSP_RGB32) {
        if (aligned) {
            _mm_stream_si128((__m128i *)(dst),xmm1);
            _mm_stream_si128((__m128i *)(dst + stride_dst),xmm2);
        } else {
            // Very rare cases. Don't optimize too much.
            // SSE2 doesn't have un-aligned version of movntdq. Use MMX (movntq) here. This is much faster than movdqu.
#ifdef WIN64
            // MSVC does not support MMX intrinsics on x64.
            _mm_storeu_si128((__m128i *)(dst),xmm1);
            _mm_storeu_si128((__m128i *)(dst + stride_dst),xmm2);
#else
            __m64 mm0,mm1,mm2,mm3;
            mm0 = _mm_movepi64_pi64(xmm1);
            xmm1 = _mm_srli_si128(xmm1,8);
            mm1 = _mm_movepi64_pi64(xmm1);
            _mm_stream_pi((__m64 *)(dst),mm0);
            _mm_stream_pi((__m64 *)(dst+8),mm1);

            mm2 = _mm_movepi64_pi64(xmm2);
            xmm2 = _mm_srli_si128(xmm2,8);
            mm3 = _mm_movepi64_pi64(xmm2);
            _mm_stream_pi((__m64 *)(dst + stride_dst),mm2);
            _mm_stream_pi((__m64 *)(dst + stride_dst + 8),mm3);
#endif
        }
        dst += 16;
    } else { // RGB24
        uint32_t eax;
        __align16(uint8_t, rgbbuf[32]);
        *(int *)(rgbbuf) = _mm_cvtsi128_si32 (xmm1);
        xmm1 = _mm_srli_si128(xmm1,4);
        *(int *)(rgbbuf+3) = _mm_cvtsi128_si32 (xmm1);
        xmm1 = _mm_srli_si128(xmm1,4);
        *(int *)(rgbbuf+6) = _mm_cvtsi128_si32 (xmm1);
        xmm1 = _mm_srli_si128(xmm1,4);
        *(int *)(rgbbuf+9) = _mm_cvtsi128_si32 (xmm1);

        *(int *)(rgbbuf+16) = _mm_cvtsi128_si32 (xmm2);
        xmm2 = _mm_srli_si128(xmm2,4);
        *(int *)(rgbbuf+19) = _mm_cvtsi128_si32 (xmm2);
        xmm2 = _mm_srli_si128(xmm2,4);
        *(int *)(rgbbuf+22) = _mm_cvtsi128_si32 (xmm2);
        xmm2 = _mm_srli_si128(xmm2,4);
        *(int *)(rgbbuf+25) = _mm_cvtsi128_si32 (xmm2);

        xmm1 = _mm_loadl_epi64((const __m128i *)(rgbbuf));
        xmm2 = _mm_loadl_epi64((const __m128i *)(rgbbuf+16));
#ifdef WIN64
        _mm_storel_epi64((__m128i *)(dst),xmm1);
        eax = *(uint32_t *)(rgbbuf + 8);
        *(uint32_t *)(dst + 8) = eax;
        _mm_storel_epi64((__m128i *)(dst + stride_dst),xmm2);
        eax = *(uint32_t *)(rgbbuf + 24);
        *(uint32_t *)(dst + stride_dst + 8) = eax;
#else
        __m64 mm0;
        mm0 = _mm_movepi64_pi64(xmm1);
        eax = *(uint32_t *)(rgbbuf + 8);
        _mm_stream_pi((__m64 *)(dst),mm0);
        _mm_stream_si32((int*)(dst + 8), eax);

        mm0 = _mm_movepi64_pi64(xmm2);
        eax = *(uint32_t *)(rgbbuf + 24);
        _mm_stream_pi((__m64 *)(dst + stride_dst),mm0);
        _mm_stream_si32((int*)(dst + stride_dst + 8), eax);
#endif
        dst += 12;
    }
}

// translate stack arguments to template arguments.
template <int rgb_limit> void TffdshowConverters::convert_translate_incsp(const uint8_t* srcY,
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
        convert_translate_outcsp<FF_CSP_420P, rgb_limit>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
        return;
    case FF_CSP_NV12:
        convert_translate_outcsp<FF_CSP_NV12, rgb_limit>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
        return;
    case FF_CSP_YUY2:
        convert_translate_outcsp<FF_CSP_YUY2, rgb_limit>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
        return;
    case FF_CSP_422P:
        convert_translate_outcsp<FF_CSP_422P, rgb_limit>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
        return;
    }
}

template <int incsp, int rgb_limit> void TffdshowConverters::convert_translate_outcsp(const uint8_t* srcY,
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
        convert_translate_align<incsp, FF_CSP_RGB32, rgb_limit>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
        return;
    case FF_CSP_RGB24:
        convert_translate_align<incsp, FF_CSP_RGB24, rgb_limit>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
        return;
    }
}

template <int incsp, int outcsp, int rgb_limit> void TffdshowConverters::convert_translate_align(const uint8_t* srcY,
              const uint8_t* srcCb,
              const uint8_t* srcCr,
              uint8_t* dst,
              int dx,
              int dy,
              stride_t stride_Y,
              stride_t stride_CbCr,
              stride_t stride_dst)
{
    if ((stride_dst & 0xf) || (stride_t(dst) & 0xf))
        convert_main_loop<incsp, outcsp, rgb_limit, 0>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
    else
        convert_main_loop<incsp, outcsp, rgb_limit, 1>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
}

template <int incsp, int outcsp, int rgb_limit, int aligned> void TffdshowConverters::convert_main_loop(const uint8_t* srcY,
              const uint8_t* srcCb,
              const uint8_t* srcCr,
              uint8_t* dst,
              int dx,
              int dy,
              stride_t stride_Y,
              stride_t stride_CbCr,
              stride_t stride_dst)
{
    int endx = dx - 4;
    const uint8_t *srcYln = srcY;
    const uint8_t *srcCbln = srcCb;
    const uint8_t *srcCrln = srcCr;
    uint8_t *dstln = dst;
    int y,endy;

    if (incsp == FF_CSP_420P || incsp == FF_CSP_NV12) {
        // Top
        convert_a_unit<incsp,outcsp,1,0,rgb_limit,aligned>(srcYln, srcCbln, srcCrln, dstln,
                                0, 0, 0,
                                m_coeffs);
        for (int x = 4 ; x < endx ; x += 4) {
            convert_a_unit<incsp,outcsp,0,0,rgb_limit,aligned>(srcYln, srcCbln, srcCrln, dstln,
                                    0, 0, 0,
                                    m_coeffs);
        }
        convert_a_unit<incsp,outcsp,0,1,rgb_limit,aligned>(srcYln, srcCbln, srcCrln, dstln,
                                0, 0, 0,
                                m_coeffs);
        y = 1;
        endy = dy - 1;
    } else {
        y = 0;
        endy = dy;
    }

    // Mid lines
    for (; y < endy ; y += 2) {
        srcYln = srcY + y * stride_Y;
        if (incsp == FF_CSP_420P || incsp == FF_CSP_NV12) {
            srcCbln = srcCb + (y>>1) * stride_CbCr;
            srcCrln = srcCr + (y>>1) * stride_CbCr;
        } else {
            srcCbln = srcCb + y * stride_CbCr;
            srcCrln = srcCr + y * stride_CbCr;
        }
        dstln = dst + y * stride_dst;
        convert_a_unit<incsp,outcsp,1,0,rgb_limit,aligned>(srcYln, srcCbln, srcCrln, dstln,
                                stride_Y, stride_CbCr, stride_dst,
                                m_coeffs);
        for (int x = 4 ; x < endx ; x += 4) {
            convert_a_unit<incsp,outcsp,0,0,rgb_limit,aligned>(srcYln, srcCbln, srcCrln, dstln,
                                    stride_Y, stride_CbCr, stride_dst,
                                    m_coeffs);
        }
        convert_a_unit<incsp,outcsp,0,1,rgb_limit,aligned>(srcYln, srcCbln, srcCrln, dstln,
                                stride_Y, stride_CbCr, stride_dst,
                                m_coeffs);
    }

    if (incsp == FF_CSP_420P || incsp == FF_CSP_NV12) {
        // Bottom
        srcYln = srcY + (dy - 1) * stride_Y;
        srcCbln = srcCb + ((dy >> 1) - 1) * stride_CbCr;
        srcCrln = srcCr + ((dy >> 1) - 1) * stride_CbCr;
        dstln = dst + (dy - 1) * stride_dst;
        convert_a_unit<incsp,outcsp,1,0,rgb_limit,aligned>(srcYln, srcCbln, srcCrln, dstln,
                                0, 0, 0,
                                m_coeffs);
        for (int x = 4 ; x < endx ; x += 4) {
            convert_a_unit<incsp,outcsp,0,0,rgb_limit,aligned>(srcYln, srcCbln, srcCrln, dstln,
                                    0, 0, 0,
                                    m_coeffs);
        }
        convert_a_unit<incsp,outcsp,0,1,rgb_limit,aligned>(srcYln, srcCbln, srcCrln, dstln,
                                0, 0, 0,
                                m_coeffs);
    }
}

TffdshowConverters::TffdshowConverters()
{
    m_coeffs = (uint8_t *)aligned_malloc(256);
}

TffdshowConverters::~TffdshowConverters()
{
    aligned_free(m_coeffs);
}

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
 #pragma warning (pop)
#endif
