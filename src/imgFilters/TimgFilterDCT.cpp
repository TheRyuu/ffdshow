/*
 * Copyright (c) 2002-2006 Milan Cutka
 * Copyright (c) 2002 Tom Barry.  All rights reserved.
 *      trbarry@trbarry.com
 * idct, fdct, quantization and dequantization routines from XviD
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "stdafx.h"
#include "Tconfig.h"
#include "TimgFilterDCT.h"
#include "TdctSettings.h"

#pragma warning(disable:4799)

extern "C" void idct_mmx(short *block);
extern "C" void idct_xmm(short *block);
extern "C" void idct_sse2_dmitry(short *block);
extern "C" void fdct_mmx_skal(short *block);
extern "C" void fdct_xmm_skal(short *block);
extern "C" void fdct_sse2_skal(short *block);
TimgFilterDCT::TimgFilterDCT(IffdshowBase *Ideci, Tfilters *Iparent): TimgFilter(Ideci, Iparent)
{
    if (Tconfig::cpu_flags & FF_CPU_SSE2) {
        fdct = fdct_sse2_skal;
        idct = idct_sse2_dmitry;
    } else if (Tconfig::cpu_flags & FF_CPU_MMXEXT) {
        fdct = fdct_xmm_skal;
        idct = idct_xmm;
    } else if (Tconfig::cpu_flags & FF_CPU_MMX) {
        fdct = fdct_mmx_skal;
        idct = idct_mmx;
    } else {
        fdct = fdct_c;
        idct = idct_c;
        idct_c_init();
    }
    oldfac[0] = INT_MAX;
    oldMode = -1;
    oldmatrix[0] = 0;
    pWorkArea = (short*)aligned_malloc(64 * sizeof(short), 16);
}
TimgFilterDCT::~TimgFilterDCT()
{
    aligned_free(pWorkArea);
}

void TimgFilterDCT::multiply(void)
{
    const char * const factors8 = (const char*)&factors[0][0];

    *(__m64*)(pWorkArea + 0 * 8 + 0) = _mm_srai_pi16(_mm_mullo_pi16(*(__m64*)(pWorkArea + 0 * 8 + 0), *(__m64*)(factors8 + 0 * 16)), 3);
    *(__m64*)(pWorkArea + 0 * 8 + 4) = _mm_srai_pi16(_mm_mullo_pi16(*(__m64*)(pWorkArea + 0 * 8 + 4), *(__m64*)(factors8 + 0 * 16 + 8)), 3);

    *(__m64*)(pWorkArea + 1 * 8 + 0) = _mm_srai_pi16(_mm_mullo_pi16(*(__m64*)(pWorkArea + 1 * 8 + 0), *(__m64*)(factors8 + 1 * 16)), 3);
    *(__m64*)(pWorkArea + 1 * 8 + 4) = _mm_srai_pi16(_mm_mullo_pi16(*(__m64*)(pWorkArea + 1 * 8 + 4), *(__m64*)(factors8 + 1 * 16 + 8)), 3);

    *(__m64*)(pWorkArea + 2 * 8 + 0) = _mm_srai_pi16(_mm_mullo_pi16(*(__m64*)(pWorkArea + 2 * 8 + 0), *(__m64*)(factors8 + 2 * 16)), 3);
    *(__m64*)(pWorkArea + 2 * 8 + 4) = _mm_srai_pi16(_mm_mullo_pi16(*(__m64*)(pWorkArea + 2 * 8 + 4), *(__m64*)(factors8 + 2 * 16 + 8)), 3);

    *(__m64*)(pWorkArea + 3 * 8 + 0) = _mm_srai_pi16(_mm_mullo_pi16(*(__m64*)(pWorkArea + 3 * 8 + 0), *(__m64*)(factors8 + 3 * 16)), 3);
    *(__m64*)(pWorkArea + 3 * 8 + 4) = _mm_srai_pi16(_mm_mullo_pi16(*(__m64*)(pWorkArea + 3 * 8 + 4), *(__m64*)(factors8 + 3 * 16 + 8)), 3);

    *(__m64*)(pWorkArea + 4 * 8 + 0) = _mm_srai_pi16(_mm_mullo_pi16(*(__m64*)(pWorkArea + 4 * 8 + 0), *(__m64*)(factors8 + 4 * 16)), 3);
    *(__m64*)(pWorkArea + 4 * 8 + 4) = _mm_srai_pi16(_mm_mullo_pi16(*(__m64*)(pWorkArea + 4 * 8 + 4), *(__m64*)(factors8 + 4 * 16 + 8)), 3);

    *(__m64*)(pWorkArea + 5 * 8 + 0) = _mm_srai_pi16(_mm_mullo_pi16(*(__m64*)(pWorkArea + 5 * 8 + 0), *(__m64*)(factors8 + 5 * 16)), 3);
    *(__m64*)(pWorkArea + 5 * 8 + 4) = _mm_srai_pi16(_mm_mullo_pi16(*(__m64*)(pWorkArea + 5 * 8 + 4), *(__m64*)(factors8 + 5 * 16 + 8)), 3);

    *(__m64*)(pWorkArea + 6 * 8 + 0) = _mm_srai_pi16(_mm_mullo_pi16(*(__m64*)(pWorkArea + 6 * 8 + 0), *(__m64*)(factors8 + 6 * 16)), 3);
    *(__m64*)(pWorkArea + 6 * 8 + 4) = _mm_srai_pi16(_mm_mullo_pi16(*(__m64*)(pWorkArea + 6 * 8 + 4), *(__m64*)(factors8 + 6 * 16 + 8)), 3);

    *(__m64*)(pWorkArea + 7 * 8 + 0) = _mm_srai_pi16(_mm_mullo_pi16(*(__m64*)(pWorkArea + 7 * 8 + 0), *(__m64*)(factors8 + 7 * 16)), 3);
    *(__m64*)(pWorkArea + 7 * 8 + 4) = _mm_srai_pi16(_mm_mullo_pi16(*(__m64*)(pWorkArea + 7 * 8 + 4), *(__m64*)(factors8 + 7 * 16 + 8)), 3);
}

void TimgFilterDCT::quant_h263_inter(int16_t * coeff, const uint32_t quant, const uint16_t *)
{
#define SCALEBITS       16
#define FIX(X)          ((1L << SCALEBITS) / (X) + 1)
    static const uint32_t multipliers[32] = {
        0,       FIX(2),  FIX(4),  FIX(6),
        FIX(8),  FIX(10), FIX(12), FIX(14),
        FIX(16), FIX(18), FIX(20), FIX(22),
        FIX(24), FIX(26), FIX(28), FIX(30),
        FIX(32), FIX(34), FIX(36), FIX(38),
        FIX(40), FIX(42), FIX(44), FIX(46),
        FIX(48), FIX(50), FIX(52), FIX(54),
        FIX(56), FIX(58), FIX(60), FIX(62)
    };
#undef FIX

    const uint32_t mult = multipliers[quant];
    const uint16_t quant_m_2 = uint16_t(quant << 1);
    const uint16_t quant_d_2 = uint16_t(quant >> 1);
    uint32_t sum = 0;
    uint32_t i;

    for (i = 0; i < 64; i++) {
        int16_t acLevel = coeff[i];

        if (acLevel < 0) {
            acLevel = (-acLevel) - quant_d_2;
            if (acLevel < quant_m_2) {
                coeff[i] = 0;
                continue;
            }

            acLevel = int16_t((acLevel * mult) >> SCALEBITS);
            sum += acLevel;         /* sum += |acLevel| */
            coeff[i] = -acLevel;
        } else {
            acLevel = int16_t(acLevel - quant_d_2);
            if (acLevel < quant_m_2) {
                coeff[i] = 0;
                continue;
            }
            acLevel = int16_t((acLevel * mult) >> SCALEBITS);
            sum += acLevel;
            coeff[i] = acLevel;
        }
    }
#undef SCALEBITS
}

void TimgFilterDCT::dequant_h263_inter(int16_t * data, const uint32_t quant, const uint16_t *)
{
    const uint16_t quant_m_2 = uint16_t(quant << 1);
    const uint16_t quant_add = uint16_t (quant & 1 ? quant : quant - 1);
    int i;

    for (i = 0; i < 64; i++) {
        int16_t acLevel = data[i];

        if (acLevel == 0) {
            data[i] = 0;
        } else if (acLevel < 0) {
            acLevel = acLevel * quant_m_2 - quant_add;
            data[i] = (acLevel >= -2048 ? acLevel : -2048);
        } else {
            acLevel = acLevel * quant_m_2 + quant_add;
            data[i] = (acLevel <= 2047 ? acLevel : 2047);
        }
    }
}

void TimgFilterDCT::h263(void)
{
    quant_h263_inter(pWorkArea, quant);
    dequant_h263_inter(pWorkArea, quant);
}

void TimgFilterDCT::quant_mpeg_inter(int16_t * coeff, const uint32_t quant, const uint16_t * mpeg_quant_matrices)
{
#define SCALEBITS 17
#define FIX(X)      ((1UL << SCALEBITS) / (X) + 1)
    static const uint32_t multipliers[32] = {
        0,       FIX(2),  FIX(4),  FIX(6),
        FIX(8),     FIX(10), FIX(12), FIX(14),
        FIX(16), FIX(18), FIX(20), FIX(22),
        FIX(24), FIX(26), FIX(28), FIX(30),
        FIX(32), FIX(34), FIX(36), FIX(38),
        FIX(40), FIX(42), FIX(44), FIX(46),
        FIX(48), FIX(50), FIX(52), FIX(54),
        FIX(56), FIX(58), FIX(60), FIX(62)
    };
#undef FIX
#undef SCALEBITS

    const uint32_t mult = multipliers[quant];
    const uint16_t *inter_matrix = mpeg_quant_matrices;
    uint32_t sum = 0;
    int i;

    for (i = 0; i < 64; i++) {
        if (coeff[i] < 0) {
            uint32_t level = -coeff[i];

            level = ((level << 4) + (inter_matrix[i] >> 1)) / inter_matrix[i];
            level = (level * mult) >> 17;
            sum += level;
            coeff[i] = -(int16_t) level;
        } else if (coeff[i] > 0) {
            uint32_t level = coeff[i];

            level = ((level << 4) + (inter_matrix[i] >> 1)) / inter_matrix[i];
            level = (level * mult) >> 17;
            sum += level;
            coeff[i] = int16_t(level);
        } else {
            coeff[i] = 0;
        }
    }
}

void TimgFilterDCT::dequant_mpeg_inter(int16_t * data, const uint32_t quant, const uint16_t * mpeg_quant_matrices)
{
    uint32_t sum = 0;
    const uint16_t *inter_matrix = (mpeg_quant_matrices);
    int i;

    for (i = 0; i < 64; i++) {
        if (data[i] == 0) {
            data[i] = 0;
        } else if (data[i] < 0) {
            int32_t level = -data[i];

            level = ((2 * level + 1) * inter_matrix[i] * quant) >> 4;
            data[i] = int16_t(level <= 2048 ? -level : -2048);
        } else {
            uint32_t level = data[i];

            level = ((2 * level + 1) * inter_matrix[i] * quant) >> 4;
            data[i] = int16_t(level <= 2047 ? level : 2047);
        }

        sum ^= data[i];
    }

    /*      mismatch control */
    if ((sum & 1) == 0) {
        data[63] ^= 1;
    }
}

void TimgFilterDCT::mpeg(void)
{
    quant_mpeg_inter(pWorkArea, quant, (const uint16_t*)&factors[0][0]);
    dequant_mpeg_inter(pWorkArea, quant, (const uint16_t*)&factors[0][0]);
}

HRESULT TimgFilterDCT::process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0)
{
    const TdctSettings *cfg = (const TdctSettings*)cfg0;
    init(pict, cfg->full, cfg->half);
    if (pictRect.dx >= 8 && pictRect.dy >= 8) {
        bool modechange = oldMode != cfg->mode;
        if (modechange)
            switch (oldMode = cfg->mode) {
                case 1:
                    processDct = &TimgFilterDCT::h263;
                    break;
                case 2:
                    processDct = &TimgFilterDCT::mpeg;
                    break;
                default:
                case 0:
                    processDct = &TimgFilterDCT::multiply;
                    break;
            }
        if (oldMode == 0 && (modechange || memcmp(oldfac, &cfg->fac0, sizeof(oldfac)) != 0)) {
            memcpy(oldfac, &cfg->fac0, sizeof(oldfac));
            for (int i = 0; i <= 7; i++)
                for (int j = 0; j <= 7; j++) {
                    factors[i][j] = (short)((oldfac[i] / 1000.0) * (oldfac[j] / 1000.0) * 8);
                }
        }
        if (oldMode == 2 && (modechange || memcpy(oldmatrix, &cfg->matrix0, sizeof(oldmatrix)) != 0)) {
            memcpy(oldmatrix, &cfg->matrix0, sizeof(oldmatrix));
            const unsigned char *m = (const unsigned char*)&cfg->matrix0;
            for (int i = 0; i < 8; i++)
                for (int j = 0; j < 8; j++) {
                    factors[i][j] = (short)limit<int>(*m++, 1, 255);
                }
        }
        quant = cfg->quant;
        const unsigned char *srcY;
        getCur(FF_CSPS_MASK_YUV_PLANAR, pict, cfg->full, &srcY, NULL, NULL, NULL);
        unsigned char *dstY;
        getNext(csp1, pict, cfg->full, &dstY, NULL, NULL, NULL);

        unsigned int cycles = dx1[0]&~7;

        if (dx1[0] & 7) {
            TffPict::copy(dstY + cycles, stride2[0], srcY + cycles, stride1[0], dx1[0] & 7, dy1[0]);
        }

        __m64 m0 = _mm_setzero_si64();
        const stride_t stride1_0 = stride1[0], stride2_0 = stride2[0];
        for (unsigned int y = 0; y <= dy1[0] - 7; srcY += 8 * stride1_0, dstY += 8 * stride2_0, y += 8) {
            const unsigned char *srcLn = srcY;
            unsigned char *dstLn = dstY, *dstLnEnd = dstLn + cycles;
            for (; dstLn < dstLnEnd; srcLn += 8, dstLn += 8) {
                __m64 mm0 = *(__m64*)(srcLn + 0 * stride1_0);
                __m64 mm2 = *(__m64*)(srcLn + 1 * stride1_0);
                *(__m64*)(pWorkArea + 0) = _mm_unpacklo_pi8(mm0, m0);
                *(__m64*)(pWorkArea + 4) = _mm_unpackhi_pi8(mm0, m0);
                *(__m64*)(pWorkArea + 8) = _mm_unpacklo_pi8(mm2, m0);
                *(__m64*)(pWorkArea + 12) = _mm_unpackhi_pi8(mm2, m0);

                mm0 = *(__m64*)(srcLn + 2 * stride1_0);
                mm2 = *(__m64*)(srcLn + 3 * stride1_0);
                *(__m64*)(pWorkArea + 16) = _mm_unpacklo_pi8(mm0, m0);
                *(__m64*)(pWorkArea + 20) = _mm_unpackhi_pi8(mm0, m0);
                *(__m64*)(pWorkArea + 24) = _mm_unpacklo_pi8(mm2, m0);
                *(__m64*)(pWorkArea + 28) = _mm_unpackhi_pi8(mm2, m0);

                mm0 = *(__m64*)(srcLn + 4 * stride1_0);
                mm2 = *(__m64*)(srcLn + 5 * stride1_0);
                *(__m64*)(pWorkArea + 32) = _mm_unpacklo_pi8(mm0, m0);
                *(__m64*)(pWorkArea + 36) = _mm_unpackhi_pi8(mm0, m0);
                *(__m64*)(pWorkArea + 40) = _mm_unpacklo_pi8(mm2, m0);
                *(__m64*)(pWorkArea + 44) = _mm_unpackhi_pi8(mm2, m0);

                mm0 = *(__m64*)(srcLn + 6 * stride1_0);
                mm2 = *(__m64*)(srcLn + 7 * stride1_0);
                *(__m64*)(pWorkArea + 48) = _mm_unpacklo_pi8(mm0, m0);
                *(__m64*)(pWorkArea + 52) = _mm_unpackhi_pi8(mm0, m0);
                *(__m64*)(pWorkArea + 56) = _mm_unpacklo_pi8(mm2, m0);
                *(__m64*)(pWorkArea + 60) = _mm_unpackhi_pi8(mm2, m0);

                fdct(pWorkArea);
                (this->*processDct)();
                idct(pWorkArea);

                *(__m64*)(dstLn + 0 * stride2_0) = _mm_packs_pu16(*(__m64*)(pWorkArea + 0 * 8), *(__m64*)(pWorkArea + 0 * 8 + 4));
                *(__m64*)(dstLn + 1 * stride2_0) = _mm_packs_pu16(*(__m64*)(pWorkArea + 1 * 8), *(__m64*)(pWorkArea + 1 * 8 + 4));
                *(__m64*)(dstLn + 2 * stride2_0) = _mm_packs_pu16(*(__m64*)(pWorkArea + 2 * 8), *(__m64*)(pWorkArea + 2 * 8 + 4));
                *(__m64*)(dstLn + 3 * stride2_0) = _mm_packs_pu16(*(__m64*)(pWorkArea + 3 * 8), *(__m64*)(pWorkArea + 3 * 8 + 4));
                *(__m64*)(dstLn + 4 * stride2_0) = _mm_packs_pu16(*(__m64*)(pWorkArea + 4 * 8), *(__m64*)(pWorkArea + 4 * 8 + 4));
                *(__m64*)(dstLn + 5 * stride2_0) = _mm_packs_pu16(*(__m64*)(pWorkArea + 5 * 8), *(__m64*)(pWorkArea + 5 * 8 + 4));
                *(__m64*)(dstLn + 6 * stride2_0) = _mm_packs_pu16(*(__m64*)(pWorkArea + 6 * 8), *(__m64*)(pWorkArea + 6 * 8 + 4));
                *(__m64*)(dstLn + 7 * stride2_0) = _mm_packs_pu16(*(__m64*)(pWorkArea + 7 * 8), *(__m64*)(pWorkArea + 7 * 8 + 4));
            }
        }
        _mm_empty();
        if (dy1[0] & 7) {
            TffPict::copy(dstY, stride2[0], srcY, stride1[0], dx1[0], dy1[0] & 7);
        }
    }
    return parent->processSample(++it, pict);
}

short TimgFilterDCT::iclip[1024], *TimgFilterDCT::iclp;

void TimgFilterDCT::idct_c_init(void)
{
    iclp = iclip + 512;
    for (int i = -512; i < 512; i++) {
        iclp[i] = (short)limit(i, -256, 255);
    }
}
void TimgFilterDCT::idct_c(short *block)
{

    /*
     * idct_int32_init() must be called before the first call to this
     * function!
     */

    static const int W1 = 2841;             /* 2048*sqrt(2)*cos(1*pi/16) */
    static const int W2 = 2676;             /* 2048*sqrt(2)*cos(2*pi/16) */
    static const int W3 = 2408;             /* 2048*sqrt(2)*cos(3*pi/16) */
    static const int W5 = 1609;             /* 2048*sqrt(2)*cos(5*pi/16) */
    static const int W6 = 1108;             /* 2048*sqrt(2)*cos(6*pi/16) */
    static const int W7 = 565;              /* 2048*sqrt(2)*cos(7*pi/16) */

    short *blk;
    long i;
    long X0, X1, X2, X3, X4, X5, X6, X7, X8;


    for (i = 0; i < 8; i++) {      /* idct rows */
        blk = block + (i << 3);
        if (!
                ((X1 = blk[4] << 11) | (X2 = blk[6]) | (X3 = blk[2]) | (X4 =
                            blk[1]) |
                 (X5 = blk[7]) | (X6 = blk[5]) | (X7 = blk[3]))) {
            blk[0] = blk[1] = blk[2] = blk[3] = blk[4] = blk[5] = blk[6] =
                                                    blk[7] = blk[0] << 3;
            continue;
        }

        X0 = (blk[0] << 11) + 128;    /* for proper rounding in the fourth stage  */

        /* first stage  */
        X8 = W7 * (X4 + X5);
        X4 = X8 + (W1 - W7) * X4;
        X5 = X8 - (W1 + W7) * X5;
        X8 = W3 * (X6 + X7);
        X6 = X8 - (W3 - W5) * X6;
        X7 = X8 - (W3 + W5) * X7;

        /* second stage  */
        X8 = X0 + X1;
        X0 -= X1;
        X1 = W6 * (X3 + X2);
        X2 = X1 - (W2 + W6) * X2;
        X3 = X1 + (W2 - W6) * X3;
        X1 = X4 + X6;
        X4 -= X6;
        X6 = X5 + X7;
        X5 -= X7;

        /* third stage  */
        X7 = X8 + X3;
        X8 -= X3;
        X3 = X0 + X2;
        X0 -= X2;
        X2 = (181 * (X4 + X5) + 128) >> 8;
        X4 = (181 * (X4 - X5) + 128) >> 8;

        /* fourth stage  */

        blk[0] = (short)((X7 + X1) >> 8);
        blk[1] = (short)((X3 + X2) >> 8);
        blk[2] = (short)((X0 + X4) >> 8);
        blk[3] = (short)((X8 + X6) >> 8);
        blk[4] = (short)((X8 - X6) >> 8);
        blk[5] = (short)((X0 - X4) >> 8);
        blk[6] = (short)((X3 - X2) >> 8);
        blk[7] = (short)((X7 - X1) >> 8);

    }                            /* end for ( i = 0; i < 8; ++i ) IDCT-rows */



    for (i = 0; i < 8; i++) {      /* idct columns */
        blk = block + i;
        /* shortcut  */
        if (!
                ((X1 = (blk[8 * 4] << 8)) | (X2 = blk[8 * 6]) | (X3 =
                            blk[8 *
                                2]) | (X4 =
                                           blk[8 *
                                               1])
                 | (X5 = blk[8 * 7]) | (X6 = blk[8 * 5]) | (X7 = blk[8 * 3]))) {
            blk[8 * 0] = blk[8 * 1] = blk[8 * 2] = blk[8 * 3] = blk[8 * 4] =
                    blk[8 * 5] = blk[8 * 6] = blk[8 * 7] =
                                                  iclp[(blk[8 * 0] + 32) >> 6];
            continue;
        }

        X0 = (blk[8 * 0] << 8) + 8192;

        /* first stage  */
        X8 = W7 * (X4 + X5) + 4;
        X4 = (X8 + (W1 - W7) * X4) >> 3;
        X5 = (X8 - (W1 + W7) * X5) >> 3;
        X8 = W3 * (X6 + X7) + 4;
        X6 = (X8 - (W3 - W5) * X6) >> 3;
        X7 = (X8 - (W3 + W5) * X7) >> 3;

        /* second stage  */
        X8 = X0 + X1;
        X0 -= X1;
        X1 = W6 * (X3 + X2) + 4;
        X2 = (X1 - (W2 + W6) * X2) >> 3;
        X3 = (X1 + (W2 - W6) * X3) >> 3;
        X1 = X4 + X6;
        X4 -= X6;
        X6 = X5 + X7;
        X5 -= X7;

        /* third stage  */
        X7 = X8 + X3;
        X8 -= X3;
        X3 = X0 + X2;
        X0 -= X2;
        X2 = (181 * (X4 + X5) + 128) >> 8;
        X4 = (181 * (X4 - X5) + 128) >> 8;

        /* fourth stage  */
        blk[8 * 0] = iclp[(X7 + X1) >> 14];
        blk[8 * 1] = iclp[(X3 + X2) >> 14];
        blk[8 * 2] = iclp[(X0 + X4) >> 14];
        blk[8 * 3] = iclp[(X8 + X6) >> 14];
        blk[8 * 4] = iclp[(X8 - X6) >> 14];
        blk[8 * 5] = iclp[(X0 - X4) >> 14];
        blk[8 * 6] = iclp[(X3 - X2) >> 14];
        blk[8 * 7] = iclp[(X7 - X1) >> 14];
    }

}                                /* end function idct_int32(block) */

void TimgFilterDCT::fdct_c(short *block)
{
    static const int CONST_BITS = 13;
    static const int PASS1_BITS = 2;

    static const int FIX_0_298631336 = 2446;   /* FIX(0.298631336) */
    static const int FIX_0_390180644 = 3196;   /* FIX(0.390180644) */
    static const int FIX_0_541196100 = 4433;   /* FIX(0.541196100) */
    static const int FIX_0_765366865 = 6270;   /* FIX(0.765366865) */
    static const int FIX_0_899976223 = 7373;   /* FIX(0.899976223) */
    static const int FIX_1_175875602 = 9633;   /* FIX(1.175875602) */
    static const int FIX_1_501321110 = 12299;  /* FIX(1.501321110) */
    static const int FIX_1_847759065 = 15137;  /* FIX(1.847759065) */
    static const int FIX_1_961570560 = 16069;  /* FIX(1.961570560) */
    static const int FIX_2_053119869 = 16819;  /* FIX(2.053119869) */
    static const int FIX_2_562915447 = 20995;  /* FIX(2.562915447) */
    static const int FIX_3_072711026 = 25172;  /* FIX(3.072711026) */

    int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    int tmp10, tmp11, tmp12, tmp13;
    int z1, z2, z3, z4, z5;
    short *blkptr;
    int *dataptr;
    int data[64];
    int i;

    /* Pass 1: process rows. */
    /* Note results are scaled up by sqrt(8) compared to a true DCT; */
    /* furthermore, we scale the results by 2**PASS1_BITS. */

    dataptr = data;
    blkptr = block;
    for (i = 0; i < 8; i++) {
        tmp0 = blkptr[0] + blkptr[7];
        tmp7 = blkptr[0] - blkptr[7];
        tmp1 = blkptr[1] + blkptr[6];
        tmp6 = blkptr[1] - blkptr[6];
        tmp2 = blkptr[2] + blkptr[5];
        tmp5 = blkptr[2] - blkptr[5];
        tmp3 = blkptr[3] + blkptr[4];
        tmp4 = blkptr[3] - blkptr[4];

        /* Even part per LL&M figure 1 --- note that published figure is faulty;
         * rotator "sqrt(2)*c1" should be "sqrt(2)*c6".
         */

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        dataptr[0] = (tmp10 + tmp11) << PASS1_BITS;
        dataptr[4] = (tmp10 - tmp11) << PASS1_BITS;

        z1 = (tmp12 + tmp13) * FIX_0_541196100;
        dataptr[2] =
            roundRshift(z1 + tmp13 * FIX_0_765366865, CONST_BITS - PASS1_BITS);
        dataptr[6] =
            roundRshift(z1 + tmp12 * (-FIX_1_847759065), CONST_BITS - PASS1_BITS);

        /* Odd part per figure 8 --- note paper omits factor of sqrt(2).
         * cK represents cos(K*pi/16).
         * i0..i3 in the paper are tmp4..tmp7 here.
         */

        z1 = tmp4 + tmp7;
        z2 = tmp5 + tmp6;
        z3 = tmp4 + tmp6;
        z4 = tmp5 + tmp7;
        z5 = (z3 + z4) * FIX_1_175875602;    /* sqrt(2) * c3 */

        tmp4 *= FIX_0_298631336;    /* sqrt(2) * (-c1+c3+c5-c7) */
        tmp5 *= FIX_2_053119869;    /* sqrt(2) * ( c1+c3-c5+c7) */
        tmp6 *= FIX_3_072711026;    /* sqrt(2) * ( c1+c3+c5-c7) */
        tmp7 *= FIX_1_501321110;    /* sqrt(2) * ( c1+c3-c5-c7) */
        z1 *= -FIX_0_899976223;     /* sqrt(2) * (c7-c3) */
        z2 *= -FIX_2_562915447;     /* sqrt(2) * (-c1-c3) */
        z3 *= -FIX_1_961570560;     /* sqrt(2) * (-c3-c5) */
        z4 *= -FIX_0_390180644;     /* sqrt(2) * (c5-c3) */

        z3 += z5;
        z4 += z5;

        dataptr[7] = roundRshift(tmp4 + z1 + z3, CONST_BITS - PASS1_BITS);
        dataptr[5] = roundRshift(tmp5 + z2 + z4, CONST_BITS - PASS1_BITS);
        dataptr[3] = roundRshift(tmp6 + z2 + z3, CONST_BITS - PASS1_BITS);
        dataptr[1] = roundRshift(tmp7 + z1 + z4, CONST_BITS - PASS1_BITS);

        dataptr += 8;            /* advance pointer to next row */
        blkptr += 8;
    }

    /* Pass 2: process columns.
     * We remove the PASS1_BITS scaling, but leave the results scaled up
     * by an overall factor of 8.
     */

    dataptr = data;
    for (i = 0; i < 8; i++) {
        tmp0 = dataptr[0] + dataptr[56];
        tmp7 = dataptr[0] - dataptr[56];
        tmp1 = dataptr[8] + dataptr[48];
        tmp6 = dataptr[8] - dataptr[48];
        tmp2 = dataptr[16] + dataptr[40];
        tmp5 = dataptr[16] - dataptr[40];
        tmp3 = dataptr[24] + dataptr[32];
        tmp4 = dataptr[24] - dataptr[32];

        /* Even part per LL&M figure 1 --- note that published figure is faulty;
         * rotator "sqrt(2)*c1" should be "sqrt(2)*c6".
         */

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        dataptr[0] = roundRshift(tmp10 + tmp11, PASS1_BITS);
        dataptr[32] = roundRshift(tmp10 - tmp11, PASS1_BITS);

        z1 = (tmp12 + tmp13) * FIX_0_541196100;
        dataptr[16] =
            roundRshift(z1 + tmp13 * FIX_0_765366865, CONST_BITS + PASS1_BITS);
        dataptr[48] =
            roundRshift(z1 + tmp12 * (-FIX_1_847759065), CONST_BITS + PASS1_BITS);

        /* Odd part per figure 8 --- note paper omits factor of sqrt(2).
         * cK represents cos(K*pi/16).
         * i0..i3 in the paper are tmp4..tmp7 here.
         */

        z1 = tmp4 + tmp7;
        z2 = tmp5 + tmp6;
        z3 = tmp4 + tmp6;
        z4 = tmp5 + tmp7;
        z5 = (z3 + z4) * FIX_1_175875602;    /* sqrt(2) * c3 */

        tmp4 *= FIX_0_298631336;    /* sqrt(2) * (-c1+c3+c5-c7) */
        tmp5 *= FIX_2_053119869;    /* sqrt(2) * ( c1+c3-c5+c7) */
        tmp6 *= FIX_3_072711026;    /* sqrt(2) * ( c1+c3+c5-c7) */
        tmp7 *= FIX_1_501321110;    /* sqrt(2) * ( c1+c3-c5-c7) */
        z1 *= -FIX_0_899976223;     /* sqrt(2) * (c7-c3) */
        z2 *= -FIX_2_562915447;     /* sqrt(2) * (-c1-c3) */
        z3 *= -FIX_1_961570560;     /* sqrt(2) * (-c3-c5) */
        z4 *= -FIX_0_390180644;     /* sqrt(2) * (c5-c3) */

        z3 += z5;
        z4 += z5;

        dataptr[56] = roundRshift(tmp4 + z1 + z3, CONST_BITS + PASS1_BITS);
        dataptr[40] = roundRshift(tmp5 + z2 + z4, CONST_BITS + PASS1_BITS);
        dataptr[24] = roundRshift(tmp6 + z2 + z3, CONST_BITS + PASS1_BITS);
        dataptr[8] = roundRshift(tmp7 + z1 + z4, CONST_BITS + PASS1_BITS);

        dataptr++;                /* advance pointer to next column */
    }
    /* descale */
    for (i = 0; i < 64; i++) {
        block[i] = (short int) roundRshift(data[i], 3);
    }
}
