/*
 * Copyright (c) 2002-2006 Milan Cutka
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
#include "TimgFilterBlur.h"
#include "TblurSettings.h"
#include "T3x3blur.h"
#include "libswscale/swscale.h"
#include "Tlibavcodec.h"
#include "Tconfig.h"
#include "IffdshowBase.h"
#include "simd.h"

TimgFilterBlur::TimgFilterBlur(IffdshowBase *Ideci, Tfilters *Iparent): TimgFilter(Ideci, Iparent)
{
    bluredPict = NULL;
    blur = NULL;
    if (Tconfig::cpu_flags & FF_CPU_SSE2) {
        mergeFc = &TimgFilterBlur::merge<Tsse2>;
    } else {
        mergeFc = &TimgFilterBlur::merge<Tmmx>;
    }
}

void TimgFilterBlur::done(void)
{
    if (bluredPict) {
        aligned_free(bluredPict);
        bluredPict = NULL;
    }
    if (blur) {
        delete blur;
        blur = NULL;
    }
}
void TimgFilterBlur::onSizeChange(void)
{
    done();
}

bool TimgFilterBlur::is(const TffPictBase &pict, const TfilterSettingsVideo *cfg0)
{
    const TblurSettings *cfg = (const TblurSettings*)cfg0;
    return super::is(pict, cfg) && cfg->isSoften && cfg->soften;
}

template<class _mm> void TimgFilterBlur::merge(const TblurSettings *cfg, const unsigned char *srcY, unsigned char *dstY)
{
    int strength = cfg->soften / 2, invstrength = 127 - cfg->soften / 2;
    typename _mm::__m strength64 = _mm::set1_pi16(short(strength));
    typename _mm::__m invstrength64 = _mm::set1_pi16(short(invstrength));
    typename _mm::__m m0 = _mm::setzero_si64();
    const int cnt3 = dx1[0] - _mm::size / 2 + 1;
    for (const unsigned char *blurPtr = bluredPict, *srcEnd = srcY + dy1[0] * stride1[0]; srcY != srcEnd; srcY += stride1[0], blurPtr += bluredStride, dstY += stride2[0]) {
        int x = 0;
        for (; x < cnt3; x += _mm::size / 2) {
            typename _mm::__m r = _mm::adds_pu16(_mm::mullo_pi16(_mm::unpacklo_pi8(_mm::load2(srcY + x), m0), invstrength64), _mm::mullo_pi16(_mm::unpacklo_pi8(_mm::load2(blurPtr + x), m0), strength64));
            _mm::store2(dstY + x, _mm::packs_pu16(_mm::srli_pi16(r, 7), m0));
        }
        for (; x < (int)dx1[0]; x++) {
            dstY[x] = uint8_t((srcY[x] * invstrength + blurPtr[x] * strength) >> 7);
        }
    }
    _mm::empty();
}

HRESULT TimgFilterBlur::process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0)
{
    if (is(pict, cfg0)) {
        const TblurSettings *cfg = (const TblurSettings*)cfg0;
        init(pict, cfg->full, cfg->half);
        const unsigned char *srcY;
        getCur(FF_CSPS_MASK_YUV_PLANAR, pict, cfg->full, &srcY, NULL, NULL, NULL);
        unsigned char *dstY;
        getNext(csp1, pict, cfg->full, &dstY, NULL, NULL, NULL);

        if (!blur) {
            blur = new T3x3blurSWS(deci, dx1[0], dy1[0]);
        }
        if (!bluredPict) {
            bluredStride = (dx1[0] / 16 + 2) * 16;
            bluredPict = (unsigned char*)aligned_malloc(bluredStride * dy1[0]);
        }

        blur->process(srcY, stride1[0], bluredPict, bluredStride);
        (this->*mergeFc)(cfg, srcY, dstY);
    }
    return parent->processSample(++it, pict);
}

//==================================== TimgFilterAvcodecBlur ===================================
TimgFilterAvcodecBlur::TimgFilterAvcodecBlur(IffdshowBase *Ideci, Tfilters *Iparent): TimgFilter(Ideci, Iparent)
{
    swsc = NULL;
    libavcodec = NULL;
}
TimgFilterAvcodecBlur::~TimgFilterAvcodecBlur()
{
    if (libavcodec) {
        libavcodec->Release();
    }
}

void TimgFilterAvcodecBlur::done(void)
{
    if (swsc) {
        libavcodec->sws_freeContext(swsc);
    }
    swsc = NULL;
}
void TimgFilterAvcodecBlur::onSizeChange(void)
{
    done();
}

bool TimgFilterAvcodecBlur::is(const TffPictBase &pict, const TfilterSettingsVideo *cfg0)
{
    const TblurSettings *cfg = (const TblurSettings*)cfg0;
    return super::is(pict, cfg) && cfg->isAvcodecBlur && (cfg->avcodecBlurLuma || cfg->avcodecBlurChroma);
}

HRESULT TimgFilterAvcodecBlur::process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0)
{
    if (is(pict, cfg0)) {
        const TblurSettings *cfg = (const TblurSettings*)cfg0;
        init(pict, cfg->full, cfg->half);
        bool cspChanged = false;
        const unsigned char *src[4];
        cspChanged |= getCur(SWS_IN_CSPS, pict, cfg->full, &src[0], &src[1], &src[2], &src[3]);
        unsigned char *dst[4];
        cspChanged |= getNext(SWS_OUT_CSPS, pict, cfg->full, &dst[0], &dst[1], &dst[2], &dst[3]);
        if (cspChanged || !swsc || oldradius != cfg->avcodecBlurRadius || oldluma != cfg->avcodecBlurLuma || oldchroma != cfg->avcodecBlurChroma) {
            done();
            if (!libavcodec) {
                deci->getLibavcodec(&libavcodec);
            }
            oldradius = cfg->avcodecBlurRadius;
            oldluma = cfg->avcodecBlurLuma;
            oldchroma = cfg->avcodecBlurChroma;
            SwsFilter swsf;
            if (oldluma) {
                swsf.lumH = libavcodec->sws_getGaussianVec(oldluma / 100.0, oldradius);
                libavcodec->sws_normalizeVec(swsf.lumH, 1.0);
                swsf.lumV = libavcodec->sws_getGaussianVec(oldluma / 100.0, oldradius);
                libavcodec->sws_normalizeVec(swsf.lumV, 1.0);
            } else {
                swsf.lumH = swsf.lumV = NULL;
            }
            if (oldchroma) {
                swsf.chrH = libavcodec->sws_getGaussianVec(oldchroma / 100.0, oldradius);
                libavcodec->sws_normalizeVec(swsf.chrH, 1.0);
                swsf.chrV = libavcodec->sws_getGaussianVec(oldchroma / 100.0, oldradius);
                libavcodec->sws_normalizeVec(swsf.chrV, 1.0);
            } else {
                swsf.chrH = swsf.chrV = NULL;
            }
            int swsflags = 0;
            SwsParams params;
            Tlibavcodec::swsInitParams(&params, 0, swsflags);
            swsc = libavcodec->sws_getCachedContext(NULL, dx1[0], dy1[0], csp_ffdshow2lavc(csp1), dx1[0], dy1[0], csp_ffdshow2lavc(csp2), swsflags, &swsf, NULL, NULL, &params);
            if (oldluma) {
                libavcodec->sws_freeVec(swsf.lumH);
                libavcodec->sws_freeVec(swsf.lumV);
            }
            if (oldchroma) {
                libavcodec->sws_freeVec(swsf.chrH);
                libavcodec->sws_freeVec(swsf.chrV);
            }
        }

        if (swsc) {
            libavcodec->sws_scale(swsc, src, stride1, 0, dy1[0], dst, stride2);
        }
    }
    return parent->processSample(++it, pict);
}

//==================================== TimgFilterAvcodecTNR ===================================
TimgFilterAvcodecTNR::TimgFilterAvcodecTNR(IffdshowBase *Ideci, Tfilters *Iparent): TimgFilter(Ideci, Iparent)
{
    pp_ctx = NULL;
    Tlibavcodec::pp_mode_defaults(pp_mode);
    libavcodec = NULL;
}
TimgFilterAvcodecTNR::~TimgFilterAvcodecTNR()
{
    if (libavcodec) {
        libavcodec->Release();
    }
}

void TimgFilterAvcodecTNR::done(void)
{
    if (pp_ctx) {
        libavcodec->pp_free_context(pp_ctx);
    }
    pp_ctx = NULL;
}
void TimgFilterAvcodecTNR::onSizeChange(void)
{
    done();
}

bool TimgFilterAvcodecTNR::is(const TffPictBase &pict, const TfilterSettingsVideo *cfg0)
{
    const TblurSettings *cfg = (const TblurSettings*)cfg0;
    if (super::is(pict, cfg) && cfg->isAvcodecTNR) {
        Trect newRect = pict.getRect(cfg->full, cfg->half);
        return newRect.dx >= 16 && newRect.dy >= 16;
    } else {
        return false;
    }
}

HRESULT TimgFilterAvcodecTNR::process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0)
{
    if (is(pict, cfg0)) {
        const TblurSettings *cfg = (const TblurSettings*)cfg0;
        init(pict, cfg->full, cfg->half);
        bool cspChanged = false;
        const unsigned char *tempPict1[4];
        cspChanged |= getCur(FF_CSPS_MASK_YUV_PLANAR, pict, cfg->full, tempPict1);
        unsigned char *tempPict2[4];
        cspChanged |= getNext(csp1, pict, cfg->full, tempPict2);
        if (cspChanged) {
            done();
        }

        if (!pp_ctx) {
            if (!libavcodec) {
                deci->getLibavcodec(&libavcodec);
            }
            pp_ctx = libavcodec->pp_get_context(dx1[0], dy1[0], Tlibavcodec::ppCpuCaps(csp1));
        }
        pp_mode.lumMode = TEMP_NOISE_FILTER;
        pp_mode.chromMode = TEMP_NOISE_FILTER;
        pp_mode.maxTmpNoise[0] = cfg->avcodecTNR1;
        pp_mode.maxTmpNoise[1] = cfg->avcodecTNR2;
        pp_mode.maxTmpNoise[2] = cfg->avcodecTNR3;

        libavcodec->pp_postprocess(tempPict1, stride1,
                                   tempPict2, stride2,
                                   dx1[0], dy1[0],
                                   NULL, 0,
                                   &pp_mode, pp_ctx, pict.frametype & FRAME_TYPE::typemask);
    }
    return parent->processSample(++it, pict);
}

void TimgFilterAvcodecTNR::onSeek(void)
{
    done();
}
