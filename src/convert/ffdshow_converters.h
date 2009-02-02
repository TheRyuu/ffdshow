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

#include <emmintrin.h>
#include "inttypes.h"
#include "ffImgfmt.h"      // Just to use some names of color spaces and stride_t
#include "TrgbPrimaries.h" // depends on YCbCr2RGBdata_common_inint

#pragma warning (push)
#pragma warning (disable: 4799) // EMMS

// Features
//  High quality chroma upscaling: 75:25 averaging vertically and horizontally
//  Support for color primary parameters, such as ITU-R BT.601/709, input and output levels
//  10bit or more calculations
//  Supported color spaces
//    input:  progressive YV12, progressive NV12, YV16, YUY2
//    output: RGB32,RGB24
//  SSE2 required

template <int incsp, int outcsp> class TffdshowConverters
{
 uint8_t *m_coeffs;
 bool m_rgb_limit;
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

 template<int left_edge, int right_edge, int rgb_limit> static __forceinline 
  void convert_a_unit(const unsigned char* &srcY,
                      const unsigned char* &srcCb,
                      const unsigned char* &srcCr,
                      unsigned char* &dst,
                      const stride_t stride_Y,
                      const stride_t stride_CbCr,
                      const stride_t stride_dst,
                      const unsigned char* const coeffs)
  {
    // left_edge:  output left 1x2 RGB pixel. Do not increment Cb and Cr ptr. Increment Y ptr 1.
    // right_edge: output right 3x2 RGB pixels.
    // mid:        output 4x2 RGB pixels.
    __m128i xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7;
    xmm7 = _mm_setzero_si128 ();

    if (incsp == FF_CSP_420P || incsp == FF_CSP_NV12) {
        // 4:2:0 color spaces
        if (incsp == FF_CSP_420P) {
            if (left_edge) {
                uint16_t ax;
                char al;
                al = *srcCb;
                ax = (al << 8) + al;
                xmm1 = _mm_cvtsi32_si128(ax);                    // --,--,Cb00,Cb00
                al = *srcCr;
                ax = (al << 8) + al;
                xmm0 = _mm_cvtsi32_si128(ax);                    // --,--,Cr00,Cr00
                al = *(srcCb + stride_CbCr);
                ax = (al << 8) + al;
                xmm3 = _mm_cvtsi32_si128(ax);                    // --,--,Cb10,Cb10
                al = *(srcCr + stride_CbCr);
                ax = (al << 8) + al;
                xmm2 = _mm_cvtsi32_si128(ax);                    // --,--,Cr10,Cr10
            } else if (right_edge) {
                uint16_t ax;
                ax = *(uint16_t *)(srcCb);                                         // Cb01,Cb00
                xmm1 = _mm_cvtsi32_si128(((uint32_t(ax) << 8) & 0x00ff0000) | ax); // --,Cb01,Cb01,Cb00
                ax = *(uint16_t *)(srcCr);                                         // Cb01,Cb00
                xmm0 = _mm_cvtsi32_si128(((uint32_t(ax) << 8) & 0x00ff0000) | ax); // --,Cr01,Cr01,Cr00
                ax = *(uint16_t *)(srcCb + stride_CbCr);                           // Cb11,Cb10
                xmm3 = _mm_cvtsi32_si128(((uint32_t(ax) << 8) & 0x00ff0000) | ax); // --,Cb11,Cb11,Cb10
                ax = *(uint16_t *)(srcCr + stride_CbCr);                           // Cb11,Cb10
                xmm2 = _mm_cvtsi32_si128(((uint32_t(ax) << 8) & 0x00ff0000) | ax); // --,Cr11,Cr11,Cr10
            } else {
                xmm1 = _mm_loadl_epi64((const __m128i*)(srcCb));                   // Cb03,Cb02,Cb01,Cb00  (Cb03: not used)
                xmm3 = _mm_loadl_epi64((const __m128i*)(srcCb + stride_CbCr));     // Cb13,Cb12,Cb11,Cb10  (Cb13: not used)
                xmm0 = _mm_loadl_epi64((const __m128i*)(srcCr));                   // Cr03,Cr02,Cr01,Cr00  (Cr03: not used)
                xmm2 = _mm_loadl_epi64((const __m128i*)(srcCr + stride_CbCr));     // Cr13,Cr12,Cr11,Cr10  (Cr13: not used)
                srcCb += 2;
                srcCr += 2;
            }
            xmm0 = _mm_unpacklo_epi8(xmm1,xmm0);                                   // xmm0 = Cr03,Cb03,Cr02,Cb02,Cr01,Cb01,Cr00,Cb00
            xmm2 = _mm_unpacklo_epi8(xmm3,xmm2);                                   // xmm2 = Cr13,Cb13,Cr12,Cb12,Cr11,Cb11,Cr00,Cb00
            xmm0 = _mm_unpacklo_epi8(xmm0,xmm7);                                   // xmm0 = 0,Cr03,0,Cb03,0,Cr02,0,Cb02,0,Cr01,0,Cb01,0,Cr00,0,Cb00
            xmm2 = _mm_unpacklo_epi8(xmm2,xmm7);                                   // xmm2 = 0,Cr13,0,Cb13,0,Cr12,0,Cb12,0,Cr11,0,Cb11,0,Cr10,0,Cb10
        } else /*NV12*/ {
            if (left_edge) {
                uint16_t ax;
                ax = *(uint16_t *)(srcCb);
                xmm0 = _mm_cvtsi32_si128((uint32_t(ax) << 16) | ax);
                ax = *(uint16_t *)(srcCb + stride_CbCr);
                xmm2 = _mm_cvtsi32_si128((uint32_t(ax) << 16) | ax);
            } else if (right_edge) {
                xmm0 = _mm_cvtsi32_si128(*(uint32_t *)(srcCb));
                xmm2 = _mm_cvtsi32_si128(*(uint32_t *)(srcCb + stride_CbCr));
                xmm1 = xmm0;
                xmm3 = xmm2;
                xmm1 = _mm_srli_si128(xmm1,2);
                xmm3 = _mm_srli_si128(xmm3,2);
                xmm1 = _mm_slli_si128(xmm1,4);
                xmm3 = _mm_slli_si128(xmm3,4);
                xmm0 = _mm_or_si128(xmm0,xmm1);
                xmm2 = _mm_or_si128(xmm2,xmm3);
            } else {
                xmm0 = _mm_loadl_epi64((const __m128i*)srcCb);                 // Cb03,Cr03,Cb02,Cr02,Cb01,Cr01,Cb00,Cr00
                xmm2 = _mm_loadl_epi64((const __m128i*)(srcCb + stride_CbCr)); // Cb13,Cr13,Cb12,Cr12,Cb11,Cr11,Cb10,Cr10
                //xmm0 = _mm_shufflelo_epi16(xmm0,0xb1); xmm0 = _mm_shufflehi_epi16(xmm0,0xb1); // for NV21
                srcCb += 4;
            }
            xmm0 = _mm_unpacklo_epi8(xmm0,xmm7);
            xmm2 = _mm_unpacklo_epi8(xmm2,xmm7);
        }

        xmm1 = xmm0;
        xmm1 = _mm_add_epi16(xmm1,xmm0);
        xmm1 = _mm_add_epi16(xmm1,xmm0);
        xmm1 = _mm_add_epi16(xmm1,xmm2);                                       // xmm1 = 3Cr03+Cr13,3Cb03+Cb13,... = P03,P02,P01,P00 (10bit)
        xmm3 = xmm2;
        xmm3 = _mm_add_epi16(xmm3,xmm2);
        xmm3 = _mm_add_epi16(xmm3,xmm2);
        xmm3 = _mm_add_epi16(xmm3,xmm0);                                       // xmm3 = Cr03+3Cr13,Cb03+3Cb13,... = P13,P12,P11,P10 (10bit)
    } else {
        // 4:2:2 color spaces
        if (incsp == FF_CSP_422P) {
            // YV16
            if (left_edge) {
                uint16_t ax;
                char al;
                al = *srcCb;
                ax = (al << 8) + al;
                xmm0 = _mm_cvtsi32_si128(ax);                    // --,--,Cb00,Cb00
                al = *srcCr;
                ax = (al << 8) + al;
                xmm1 = _mm_cvtsi32_si128(ax);                    // --,--,Cr00,Cr00
                al = *(srcCb + stride_CbCr);
                ax = (al << 8) + al;
                xmm2 = _mm_cvtsi32_si128(ax);                    // --,--,Cb10,Cb10
                al = *(srcCr + stride_CbCr);
                ax = (al << 8) + al;
                xmm3 = _mm_cvtsi32_si128(ax);                    // --,--,Cr10,Cr10
            } else if (right_edge) {
                uint16_t ax;
                ax = *(uint16_t *)(srcCb);                                         // Cb01,Cb00
                xmm0 = _mm_cvtsi32_si128(((uint32_t(ax) << 8) & 0x00ff0000) | ax); // --,Cb01,Cb01,Cb00
                ax = *(uint16_t *)(srcCr);                                         // Cb01,Cb00
                xmm1 = _mm_cvtsi32_si128(((uint32_t(ax) << 8) & 0x00ff0000) | ax); // --,Cr01,Cr01,Cr00
                ax = *(uint16_t *)(srcCb + stride_CbCr);                           // Cb11,Cb10
                xmm2 = _mm_cvtsi32_si128(((uint32_t(ax) << 8) & 0x00ff0000) | ax); // --,Cb11,Cb11,Cb10
                ax = *(uint16_t *)(srcCr + stride_CbCr);                           // Cb11,Cb10
                xmm3 = _mm_cvtsi32_si128(((uint32_t(ax) << 8) & 0x00ff0000) | ax); // --,Cr11,Cr11,Cr10
            } else {
                xmm0 = _mm_loadl_epi64((const __m128i*)(srcCb));                   // Cb03,Cb02,Cb01,Cb00  (Cb03: not used)
                xmm2 = _mm_loadl_epi64((const __m128i*)(srcCb + stride_CbCr));     // Cb13,Cb12,Cb11,Cb10  (Cb13: not used)
                xmm1 = _mm_loadl_epi64((const __m128i*)(srcCr));                   // Cr03,Cr02,Cr01,Cr00  (Cr03: not used)
                xmm3 = _mm_loadl_epi64((const __m128i*)(srcCr + stride_CbCr));     // Cr13,Cr12,Cr11,Cr10  (Cr13: not used)
                srcCb += 2;
                srcCr += 2;
            }
            xmm1 = _mm_unpacklo_epi8(xmm0,xmm1);                                   // xmm1 = Cr03,Cb03,Cr02,Cb02,Cr01,Cb01,Cr00,Cb00
            xmm3 = _mm_unpacklo_epi8(xmm2,xmm3);                                   // xmm2 = Cr13,Cb13,Cr12,Cb12,Cr11,Cb11,Cr00,Cb00
            xmm1 = _mm_unpacklo_epi8(xmm1,xmm7);                                   // xmm1 = 0,Cr03,0,Cb03,0,Cr02,0,Cb02,0,Cr01,0,Cb01,0,Cr00,0,Cb00
            xmm3 = _mm_unpacklo_epi8(xmm3,xmm7);                                   // xmm3 = 0,Cr13,0,Cb13,0,Cr12,0,Cb12,0,Cr11,0,Cb11,0,Cr10,0,Cb10
        } else if (incsp == FF_CSP_YUY2) {
            if (left_edge) {
                uint32_t eax;
                eax = *(uint32_t *)srcY;
                xmm1 = _mm_cvtsi32_si128(eax);
                xmm2 = xmm1;
                xmm1 = _mm_slli_si128(xmm1,4);
                xmm1 = _mm_or_si128(xmm1,xmm2);                                   // xmm1 = ---------,Cr00,y,Cb00,y,Cr00,y,Cb00,y
                xmm5 = _mm_cvtsi32_si128((eax & 0xff) << 16);                     // xmm5 = ---------,00,Y00,00,00
                xmm1 = _mm_srli_epi16(xmm1,8);                                    // xmm1 = ---------0,Cr00,0,Cb00,0,Cr00,0,Cb00
                eax = *(uint32_t *)(srcY + stride_Y);
                xmm3 = _mm_cvtsi32_si128(eax);
                xmm2 = xmm3;
                xmm3 = _mm_slli_si128(xmm3,4);
                xmm3 = _mm_or_si128(xmm3,xmm2);                                   // xmm3 = ---------,Cr10,y,Cb10,y,Cr10,y,Cb10,y
                xmm4 = _mm_cvtsi32_si128((eax & 0xff) << 16);                     // xmm5 = ---------,00,Y10,00,00
                xmm3 = _mm_srli_epi16(xmm3,8);
            } else if (right_edge) {
                xmm1 = _mm_loadl_epi64((const __m128i*)(srcY));                   // xmm1 = Cr01,y,Cb01,y,Cr00,y,Cb00,y
                xmm2 = xmm1;
                xmm5 = xmm1;
                xmm1 = _mm_srli_si128(xmm1,4);                                    // xmm1 = Cr01,y,Cb01,y
                xmm1 = _mm_slli_si128(xmm1,8);                                    // xmm1 = Cr01,y,Cb01,y,0,0,0,0,0,0,0,0
                xmm1 = _mm_or_si128(xmm1,xmm2);                                   // xmm1 = Cr01,y,Cb01,y,Cr01,y,Cb01,y,Cr00,y,Cb00,y
                xmm1 = _mm_srli_epi16(xmm1,8);                                    // xmm1 = 0,Cr01,0,Cb01,0,Cr01,0,Cb01,0,Cr00,0,Cb00
                xmm3 = _mm_loadl_epi64((const __m128i*)(srcY));                   // xmm3 = Cr11,y,Cb11,y,Cr10,y,Cb10,y
                xmm2 = xmm3;
                xmm4 = xmm3;
                xmm3 = _mm_srli_si128(xmm3,4);                                    // xmm3 = Cr11,y,Cb11,y
                xmm3 = _mm_slli_si128(xmm3,8);                                    // xmm3 = Cr11,y,Cb11,y,0,0,0,0,0,0,0,0
                xmm3 = _mm_or_si128(xmm3,xmm2);                                   // xmm3 = Cr11,y,Cb11,y,Cr11,y,Cb11,y,Cr10,y,Cb10,y
                xmm3 = _mm_srli_epi16(xmm3,8);                                    // xmm3 = 0,Cr11,0,Cb11,0,Cr11,0,Cb11,0,Cr10,0,Cb10
            } else {
                xmm1 = _mm_loadu_si128((const __m128i*)(srcY));
                xmm3 = _mm_loadu_si128((const __m128i*)(srcY + stride_Y));
                xmm5 = xmm1;                                                       // For Y value. Next block does not use xmm4,5
                xmm4 = xmm3;
                xmm1 = _mm_srli_epi16(xmm1,8);                                     // xmm1 = 0,Cr03,0,Cb03,0,Cr02,0,Cb02,0,Cr01,0,Cb01,0,Cr00,0,Cb00
                xmm3 = _mm_srli_epi16(xmm3,8);                                     // xmm3 = 0,Cr13,0,Cb13,0,Cr12,0,Cb12,0,Cr11,0,Cb11,0,Cr10,0,Cb10
                srcY += 8;
            }
        }
        xmm1 = _mm_slli_epi16(xmm1,2);
        xmm3 = _mm_slli_epi16(xmm3,2);
    }
    xmm2 = xmm1;
    xmm2 = _mm_srli_si128(xmm2,4);                       // xmm2 = 0000,P03,P02,P01

    xmm0 = xmm1;
    xmm0 = _mm_add_epi16(xmm0,xmm1);
    xmm0 = _mm_add_epi16(xmm0,xmm1);
    xmm0 = _mm_add_epi16(xmm0,xmm2);                     // xmm0 = ----,----,3*P01+P02,3*P00+P01

    xmm1 = _mm_add_epi16(xmm1,xmm2);
    xmm1 = _mm_add_epi16(xmm1,xmm2);
    xmm1 = _mm_add_epi16(xmm1,xmm2);                     // xmm5_1 = ----,----,P01+3*P02,P00+3*P01

    xmm2 = xmm3;                                         // xmm2 = P13,P12,P11,P10
    xmm6 = xmm3;
    xmm2 = _mm_srli_si128(xmm2, 4);                      // xmm2 = 0000,P13,P12,P11

    xmm6 = _mm_add_epi16(xmm6,xmm3);
    xmm6 = _mm_add_epi16(xmm6,xmm3);
    xmm6 = _mm_add_epi16(xmm6,xmm2);                     // xmm6 = ----,----,3*P11+P12,3*P10+P11

    xmm3 = _mm_add_epi16(xmm3,xmm2);
    xmm3 = _mm_add_epi16(xmm3,xmm2);
    xmm3 = _mm_add_epi16(xmm3,xmm2);                     // xmm2_3 = ----,----,P11+3*P12,P10+3*P11

    xmm2 = _mm_load_si128((const __m128i *)(coeffs + ofs_128mul16));
    xmm0 = _mm_unpacklo_epi32(xmm0,xmm1);                // P01+3*P02, 3*P01+P02, P00+3*P01, 3*P00+P01
    xmm6 = _mm_unpacklo_epi32(xmm6,xmm3);                // P11+3*P12, 3*P11+P12, P10+3*P11, 3*P10+P01
    xmm0 = _mm_subs_epi16(xmm0, xmm2);                   // xmm0 = P01+3*P02 -128*16, 3*P01+P02 -128*16, P00+3*P01 -128*16, 3*P00+P01 -128*16 (12bit)
    xmm6 = _mm_subs_epi16(xmm6, xmm2);                   // xmm6 = P11+3*P12 -128*16, 3*P11+P12 -128*16, P10+3*P11 -128*16, 3*P10+P01 -128*16 (12bit)
    // chroma upscaling finished.

    if (incsp != FF_CSP_YUY2) {
        if (left_edge) {
            uint32_t eax;
            eax = *(int *)(srcY);
            xmm5 = _mm_cvtsi32_si128(eax << 8);               // Y02,Y01,Y00,0
            eax = *(int *)(srcY + stride_Y);
            xmm1 = _mm_cvtsi32_si128(eax << 8);               // Y12,Y11,Y10,0
            srcY ++;
        } else if (right_edge) {
            uint16_t ax;
            uint8_t dl;
            ax = *(uint16_t *)(srcY);
            dl = *(uint8_t  *)(srcY + 2);
            xmm5 = _mm_cvtsi32_si128((uint32_t(dl) << 16) + ax);
            ax = *(uint16_t *)(srcY + stride_Y);
            dl = *(uint8_t  *)(srcY + 2 + stride_Y);
            xmm1 = _mm_cvtsi32_si128((uint32_t(dl) << 16) + ax);
        } else {
            xmm5 = _mm_cvtsi32_si128(*(int*)(srcY));           // Y04,Y03,Y02,Y01
            xmm1 = _mm_cvtsi32_si128(*(int*)(srcY + stride_Y));// Y14,Y13,Y12,Y11
            srcY += 4;
        }
        xmm5 = _mm_unpacklo_epi8(xmm5,xmm7);                   // 0,Y04,0,Y03,0,Y02,0,Y01
        xmm1 = _mm_unpacklo_epi8(xmm1,xmm7);                   // 0,Y14,0,Y13,0,Y12,0,Y11
    } else {
        xmm1 = xmm4;
        if (!left_edge) {
                                                               // xmm5 = --,Y04,Y03,Y02,Y01,Y00
            xmm5 = _mm_srli_si128(xmm5,2);                     // xmm5 = --,Y04,Y03,Y02,Y01
            xmm1 = _mm_srli_si128(xmm1,2);                     // xmm1 = --,Y14,Y13,Y12,Y11
            xmm5 = _mm_slli_epi16(xmm5,8);                     // clear chroma
            xmm1 = _mm_slli_epi16(xmm1,8);
            xmm5 = _mm_srli_epi16(xmm5,8);
            xmm1 = _mm_srli_epi16(xmm1,8);
        }
    }
    xmm1 = _mm_unpacklo_epi64(xmm1,xmm5);                              // 0,Y04,0,Y03,0,Y02,0,Y01,0,Y14,0,Y13,0,Y12,0,Y11

    xmm1 = _mm_subs_epu16(xmm1,*(const __m128i *)(coeffs + ofs_Ysub)); // Y-16, unsigned saturate
    xmm1 = _mm_slli_epi16(xmm1,4);
    xmm1 = _mm_mulhi_epi16(xmm1,*(const __m128i *)(coeffs + ofs_cy));  // Y*cy (10bit)
    xmm1 = _mm_add_epi16(xmm1,*(const __m128i *)(coeffs + ofs_rgb_add));
    xmm3 = xmm0;
    xmm4 = xmm6;
    xmm3 = _mm_madd_epi16(xmm3,*(const __m128i *)(coeffs + ofs_cR_Cr));
    xmm4 = _mm_madd_epi16(xmm4,*(const __m128i *)(coeffs + ofs_cR_Cr));
    xmm3 = _mm_srai_epi32(xmm3,15);
    xmm4 = _mm_srai_epi32(xmm4,15);
    xmm3 = _mm_packs_epi32(xmm3,xmm7);
    xmm4 = _mm_packs_epi32(xmm4,xmm7);
    xmm3 = _mm_unpacklo_epi64(xmm4,xmm3);
    xmm3 = _mm_add_epi16(xmm3,xmm1);                                   // R (10bit)
    xmm5 = xmm0;
    xmm4 = xmm6;
    xmm5 = _mm_madd_epi16(xmm5,*(const __m128i *)(coeffs + ofs_cG_Cb_cG_Cr));
    xmm4 = _mm_madd_epi16(xmm4,*(const __m128i *)(coeffs + ofs_cG_Cb_cG_Cr));
    xmm5 = _mm_srai_epi32(xmm5,15);
    xmm4 = _mm_srai_epi32(xmm4,15);
    xmm5 = _mm_packs_epi32(xmm5,xmm7);
    xmm4 = _mm_packs_epi32(xmm4,xmm7);
    xmm5 = _mm_unpacklo_epi64(xmm4,xmm5);
    xmm5 = _mm_add_epi16(xmm5,xmm1);                                   // G (10bit)
    xmm0 = _mm_madd_epi16(xmm0,*(const __m128i *)(coeffs + ofs_cB_Cb));
    xmm6 = _mm_madd_epi16(xmm6,*(const __m128i *)(coeffs + ofs_cB_Cb));
    xmm0 = _mm_srai_epi32(xmm0,15);
    xmm6 = _mm_srai_epi32(xmm6,15);
    xmm0 = _mm_packs_epi32(xmm0,xmm7);
    xmm6 = _mm_packs_epi32(xmm6,xmm7);
    xmm0 = _mm_unpacklo_epi64(xmm6,xmm0);
    xmm0 = _mm_add_epi16(xmm0,xmm1);                                   // B (10bit)
    xmm3 = _mm_srai_epi16(xmm3,2);
    xmm5 = _mm_srai_epi16(xmm5,2);
    xmm0 = _mm_srai_epi16(xmm0,2);
    xmm2 = _mm_load_si128((const __m128i *)(coeffs + ofs_xFFFFFFFF_FFFFFFFF));
    xmm3 = _mm_packus_epi16(xmm3,xmm7);                                // R (lower 8bytes,8bit) * 8
    xmm5 = _mm_packus_epi16(xmm5,xmm7);                                // G (lower 8bytes,8bit) * 8
    xmm0 = _mm_packus_epi16(xmm0,xmm7);                                // B (lower 8bytes,8bit) * 8
    xmm3 = _mm_unpacklo_epi8(xmm3,xmm2);                               // 0xff,R
    xmm0 = _mm_unpacklo_epi8(xmm0,xmm5);                               // G,B
    xmm2 = xmm0;
    xmm0 = _mm_unpackhi_epi16(xmm0,xmm3);                              // 0xff,RGB * 4 (line 0)
    xmm2 = _mm_unpacklo_epi16(xmm2,xmm3);                              // 0xff,RGB * 4 (line 1)

    if (rgb_limit) {
        xmm3 = _mm_load_si128((const __m128i *)(coeffs + ofs_rgb_limit_low));
        xmm4 = _mm_load_si128((const __m128i *)(coeffs + ofs_rgb_limit_high));
        xmm0 = _mm_max_epu8(xmm0,xmm3);
        xmm0 = _mm_min_epu8(xmm0,xmm4);
        xmm2 = _mm_max_epu8(xmm2,xmm3);
        xmm2 = _mm_min_epu8(xmm2,xmm4);
    }

    if (outcsp == FF_CSP_RGB32) {
        if (left_edge) {
            _mm_srli_si128(xmm0,4);
            _mm_srli_si128(xmm2,4);
            _mm_stream_si32((int*)(dst), _mm_cvtsi128_si32(xmm0));
            _mm_stream_si32((int*)(dst + stride_dst) ,_mm_cvtsi128_si32(xmm2));
            dst += 4;
        } else if (right_edge) {
            // write 3 pixels
#ifdef WIN64
            _mm_storel_epi64((__m128i *)(dst),xmm0);
            _mm_storel_epi64((__m128i *)(dst + stride_dst),xmm2);
            xmm0 = _mm_srli_si128(xmm0,8);
            xmm2 = _mm_srli_si128(xmm2,8);
            *(int*)(dst + 8)             = _mm_cvtsi128_si32(xmm0);
            *(int*)(dst + stride_dst + 8) = _mm_cvtsi128_si32(xmm2);
#else
            __m64 mm0;
            mm0 = _mm_movepi64_pi64(xmm0);
            xmm0 = _mm_srli_si128(xmm0,8);
            _mm_stream_pi((__m64 *)(dst),mm0);
            _mm_stream_si32((int*)(dst + 8), _mm_cvtsi128_si32(xmm0));

            mm0 = _mm_movepi64_pi64(xmm2);
            xmm2 = _mm_srli_si128(xmm2,8);
            _mm_stream_pi((__m64 *)(dst + stride_dst),mm0);
            _mm_stream_si32((int*)(dst + stride_dst + 8), _mm_cvtsi128_si32(xmm2));
#endif
        } else {
#ifdef WIN64
            // MSVC does not support MMX intrinsics on x64.
            // If you need very fast x64 version, store the right pixel for the next and use movntdq (check alignement and when safe, (nearly always safe)).
            // or use 8 _mm_stream_si32 (movnti)?
            _mm_storeu_si128((__m128i *)(dst),xmm0);
            _mm_storeu_si128((__m128i *)(dst + stride_dst),xmm2);
#else
            // SSE2 doesn't have un-aligned version of movntdq. Use MMX (movntq) here. This is much faster than movdqu.
            // I tried alignment hack (by storing right pixel for next), which didn't earn speed (but not too slow, maybe good for x64).
            __m64 mm0,mm1,mm2,mm3;
            mm0 = _mm_movepi64_pi64(xmm0);
            xmm0 = _mm_srli_si128(xmm0,8);
            mm1 = _mm_movepi64_pi64(xmm0);
            _mm_stream_pi((__m64 *)(dst),mm0);
            _mm_stream_pi((__m64 *)(dst+8),mm1);

            mm2 = _mm_movepi64_pi64(xmm2);
            xmm2 = _mm_srli_si128(xmm2,8);
            mm3 = _mm_movepi64_pi64(xmm2);
            _mm_stream_pi((__m64 *)(dst + stride_dst),mm2);
            _mm_stream_pi((__m64 *)(dst + stride_dst + 8),mm3);
#endif
            dst += 16;
        }
    } else { // RGB24
        uint32_t eax;
        __align16(uint8_t, rgbbuf[32]);
        *(int *)(rgbbuf) = _mm_cvtsi128_si32 (xmm0);
        xmm0 = _mm_srli_si128(xmm0,4);
        *(int *)(rgbbuf+3) = _mm_cvtsi128_si32 (xmm0);
        xmm0 = _mm_srli_si128(xmm0,4);
        *(int *)(rgbbuf+6) = _mm_cvtsi128_si32 (xmm0);
        xmm0 = _mm_srli_si128(xmm0,4);
        *(int *)(rgbbuf+9) = _mm_cvtsi128_si32 (xmm0);

        *(int *)(rgbbuf+16) = _mm_cvtsi128_si32 (xmm2);
        xmm2 = _mm_srli_si128(xmm2,4);
        *(int *)(rgbbuf+19) = _mm_cvtsi128_si32 (xmm2);
        xmm2 = _mm_srli_si128(xmm2,4);
        *(int *)(rgbbuf+22) = _mm_cvtsi128_si32 (xmm2);
        xmm2 = _mm_srli_si128(xmm2,4);
        *(int *)(rgbbuf+25) = _mm_cvtsi128_si32 (xmm2);

        if (left_edge) {
            eax = *(uint32_t *)(rgbbuf);
            _mm_stream_si32((int *)(dst), eax);
            eax = *(uint32_t *)(rgbbuf + 16);
            _mm_stream_si32((int *)(dst + stride_dst), eax);
            dst += 3;
        } else if (right_edge) {
            xmm0 = _mm_loadl_epi64((const __m128i *)(rgbbuf));
            xmm2 = _mm_loadl_epi64((const __m128i *)(rgbbuf+16));
#ifdef WIN64
            _mm_storel_epi64((__m128i *)(dst),xmm0);
            uint8_t al;
            al = *(rgbbuf + 8);
            *(dst + 8) = al;
            _mm_storel_epi64((__m128i *)(dst + stride_dst),xmm2);
            al = *(rgbbuf + 24);
            *(dst + stride_dst + 8) = al;
#else
            __m64 mm0;
            mm0 = _mm_movepi64_pi64(xmm0);
            _mm_stream_pi((__m64 *)(dst),mm0);
            eax = *(uint32_t *)(rgbbuf + 5);
            _mm_stream_si32((int*)(dst + 5), eax);
            
            mm0 = _mm_movepi64_pi64(xmm2);
            _mm_stream_pi((__m64 *)(dst + stride_dst),mm0);
            eax = *(uint32_t *)(rgbbuf + 21);
            _mm_stream_si32((int*)(dst + stride_dst + 5), eax);
#endif
        } else {
            xmm0 = _mm_loadl_epi64((const __m128i *)(rgbbuf));
            xmm2 = _mm_loadl_epi64((const __m128i *)(rgbbuf+16));
#ifdef WIN64
            _mm_storel_epi64((__m128i *)(dst),xmm0);
            eax = *(uint32_t *)(rgbbuf + 8);
            *(uint32_t *)(dst + 8) = eax;
            _mm_storel_epi64((__m128i *)(dst + stride_dst),xmm2);
            eax = *(uint32_t *)(rgbbuf + 24);
            *(uint32_t *)(dst + stride_dst + 8) = eax;
#else
            __m64 mm0;
            mm0 = _mm_movepi64_pi64(xmm0);
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
  }

 template <int rgb_limit> void convert_internal(const uint8_t* srcY,
              const uint8_t* srcCb,
              const uint8_t* srcCr,
              uint8_t* dst,
              int dx,
              int dy,
              stride_t stride_Y,
              stride_t stride_CbCr,
              stride_t stride_dst)
  {
    int endx = dx - 3;
    const uint8_t *srcYln = srcY;
    const uint8_t *srcCbln = srcCb;
    const uint8_t *srcCrln = srcCr;
    uint8_t *dstln = dst;
    int y,endy;

    if (incsp == FF_CSP_420P || incsp == FF_CSP_NV12) {
        // Top
        convert_a_unit<1,0,rgb_limit>(srcYln, srcCbln, srcCrln, dstln,
                                0, 0, 0,
                                m_coeffs);
        for (int x = 1 ; x < endx ; x += 4) {
            convert_a_unit<0,0,rgb_limit>(srcYln, srcCbln, srcCrln, dstln,
                                    0, 0, 0,
                                    m_coeffs);
        }
        convert_a_unit<0,1,rgb_limit>(srcYln, srcCbln, srcCrln, dstln,
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
        convert_a_unit<1,0,rgb_limit>(srcYln, srcCbln, srcCrln, dstln,
                                stride_Y, stride_CbCr, stride_dst,
                                m_coeffs);
        for (int x = 1 ; x < endx ; x += 4) {
            convert_a_unit<0,0,rgb_limit>(srcYln, srcCbln, srcCrln, dstln,
                                    stride_Y, stride_CbCr, stride_dst,
                                    m_coeffs);
        }
        convert_a_unit<0,1,rgb_limit>(srcYln, srcCbln, srcCrln, dstln,
                                stride_Y, stride_CbCr, stride_dst,
                                m_coeffs);
    }

    if (incsp == FF_CSP_420P || incsp == FF_CSP_NV12) {
        // Bottom
        srcYln = srcY + (dy - 1) * stride_Y;
        srcCbln = srcCb + ((dy >> 1) - 1) * stride_CbCr;
        srcCrln = srcCr + ((dy >> 1) - 1) * stride_CbCr;
        dstln = dst + (dy - 1) * stride_dst;
        convert_a_unit<1,0,rgb_limit>(srcYln, srcCbln, srcCrln, dstln,
                                0, 0, 0,
                                m_coeffs);
        for (int x = 1 ; x < endx ; x += 4) {
            convert_a_unit<0,0,rgb_limit>(srcYln, srcCbln, srcCrln, dstln,
                                    0, 0, 0,
                                    m_coeffs);
        }
        convert_a_unit<0,1,rgb_limit>(srcYln, srcCbln, srcCrln, dstln,
                                0, 0, 0,
                                m_coeffs);
    }
  }

public:
 TffdshowConverters()
  {
    m_coeffs = (uint8_t *)aligned_malloc(256);
  }
 ~TffdshowConverters()
  {
    aligned_free(m_coeffs);
  }
 void init(const int cspOptionsIturBt,
           const int cspOptionsWhiteCutoff,
           const int cspOptionsBlackCutoff,
           const int cspOptionsChromaCutoff,
           const double cspOptionsRGB_WhiteLevel,
           const double cspOptionsRGB_BlackLevel)
  {
    double Kr, Kg, Kb, chr_range, y_mul, vr_mul, ug_mul, vg_mul, ub_mul;
    int Ysub, RGB_add;
    if (cspOptionsRGB_WhiteLevel != 255 || cspOptionsRGB_BlackLevel != 0)
        m_rgb_limit = true;
    else
        m_rgb_limit = false;
    YCbCr2RGBdata_common_inint(Kr,Kg,Kb,chr_range,y_mul,vr_mul,ug_mul,vg_mul,ub_mul,Ysub,RGB_add,
       cspOptionsIturBt,cspOptionsWhiteCutoff,cspOptionsBlackCutoff,cspOptionsChromaCutoff,cspOptionsRGB_WhiteLevel,cspOptionsRGB_BlackLevel);
    
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
    uint32_t rgb_white = uint32_t(cspOptionsRGB_WhiteLevel);
    rgb_white = 0xff000000 + (rgb_white << 16) + (rgb_white << 8) + rgb_white;
    rgb_limit_highs[0] = rgb_limit_highs[1] = rgb_limit_highs[2] = rgb_limit_highs[3] = rgb_white;

    uint32_t *rgb_limit_lows = (uint32_t *)(m_coeffs + ofs_rgb_limit_low);
    uint32_t rgb_black = uint32_t(cspOptionsRGB_BlackLevel);
    rgb_black = 0xff000000 + (rgb_black << 16) + (rgb_black << 8) + rgb_black;
    rgb_limit_lows[0] = rgb_limit_lows[1] = rgb_limit_lows[2] = rgb_limit_lows[3] = rgb_black;
  }
 // note YV12 and YV16 is YCrCb order. Make sure Cr and Cb is swapped.
 void convert(const uint8_t* srcY,
              const uint8_t* srcCb,
              const uint8_t* srcCr,
              uint8_t* dst,
              int dx,
              int dy,
              stride_t stride_Y,
              stride_t stride_CbCr,
              stride_t stride_dst)
  {
    if (m_rgb_limit)
        convert_internal<1>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);
    else
        convert_internal<0>(srcY, srcCb, srcCr, dst, dx, dy, stride_Y, stride_CbCr, stride_dst);

#ifndef WIN64
    _mm_empty();
#endif
  }
};

#pragma warning (pop)
