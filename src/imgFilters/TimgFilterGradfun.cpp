/*
 * Copyright (c) 2011 ffdshow tryouts project
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
#include "TimgFilterGradfun.h"
#include "TgradFunSettings.h"
#include "IffdshowBase.h"

//=================================== TimgFilterGradfun ===================================
TimgFilterGradfun::TimgFilterGradfun(IffdshowBase *Ideci, Tfilters *Iparent): TimgFilter(Ideci, Iparent)
{
    deci->getLibavcodec(&ffmpeg);
    dllok = ffmpeg->ok;
    oldThreshold = -1;
    oldRadius = -1;
    oldWidth = 0;
    oldHeight = 0;
    gradFunContext = NULL;
    reconfigure = 1;
}
TimgFilterGradfun::~TimgFilterGradfun()
{
    done();
    if (ffmpeg) {
        ffmpeg->Release();
    }
}
void TimgFilterGradfun::done(void)
{
    if (gradFunContext != NULL) {
        if (gradFunContext->buf != NULL) {
            ffmpeg->av_free(gradFunContext->buf);
            gradFunContext->buf = NULL;
        }
        ffmpeg->av_free(gradFunContext);
        gradFunContext = NULL;
    }
}
void TimgFilterGradfun::onSizeChange(void)
{
    done();
    reconfigure = 1;
}
void TimgFilterGradfun::onDiscontinuity(void)
{
    done();
    reconfigure = 1;
}
GradFunContext* TimgFilterGradfun::configure(float threshold, int radius, TffPict &pict)
{
    char gradfun_args[20];
    _snprintf(gradfun_args, 20, "%.2f:%d", threshold, radius);
    GradFunContext *gradFunContext = (GradFunContext*)ffmpeg->av_mallocz(sizeof(GradFunContext));
    if (gradFunContext == NULL) {
        return NULL;
    }
    ffmpeg->gradfunInit(gradFunContext, gradfun_args, NULL);
    gradFunContext->chroma_w = pict.rectFull.dx >> pict.cspInfo.shiftX[1];
    gradFunContext->chroma_h = pict.rectFull.dy >> pict.cspInfo.shiftY[1];
    gradFunContext->chroma_r = ((((gradFunContext->radius >> pict.cspInfo.shiftX[1]) + (gradFunContext->radius >> pict.cspInfo.shiftY[1])) / 2) + 1) & ~1;
    if (gradFunContext->chroma_r < 4) {
        gradFunContext->chroma_r = 4;
    } else if (gradFunContext->chroma_r > 32) {
        gradFunContext->chroma_r = 32;
    }
    gradFunContext->buf = (uint16_t*)ffmpeg->av_mallocz((ffalign(pict.rectFull.dx, 16) * (gradFunContext->radius + 1) / 2 + 32) * sizeof(uint16_t));
    if (gradFunContext->buf == NULL) {
        ffmpeg->av_free(gradFunContext);
        gradFunContext = NULL;
        return NULL;
    }

    return gradFunContext;
}
void TimgFilterGradfun::filter(GradFunContext *gradFunContext, uint8_t *src[4], TffPict &pict)
{
    for (unsigned int p = 0; p < pict.cspInfo.numPlanes; p++) {
        int w = pict.rectFull.dx;
        int h = pict.rectFull.dy;
        int r = gradFunContext->radius;
        if (p > 0) {
            w = gradFunContext->chroma_w;
            h = gradFunContext->chroma_h;
            r = gradFunContext->chroma_r;
        }

        if (((w) > (h) ? (h) : (w)) > 2 * r) {
            ffmpeg->gradfunFilter(gradFunContext, pict.data[p], src[p], w, h, (int)pict.stride[p], (int)pict.stride[p], r);
        }
    }
}
HRESULT TimgFilterGradfun::process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0)
{
    if (!dllok) {
        return parent->processSample(++it, pict);
    }

    const TgradFunSettings *cfg = (const TgradFunSettings*)cfg0;
    init(pict, 1, 0);

    uint8_t *src[4];
    bool cspChange = getCurNext(FF_CSPS_MASK_YUV_PLANAR, pict, 1, COPYMODE_FULL, src);

    // mod2 only
    if ((pict.rectFull.dx % 2) != 0 || (pict.rectFull.dy % 2) != 0 || ((pict.rectFull.dx >> pict.cspInfo.shiftX[1]) % 2 != 0) || ((pict.rectFull.dy >> pict.cspInfo.shiftY[1]) % 2 != 0)) {
        return parent->processSample(++it, pict);
    }

    if (cspChange || oldThreshold != cfg->threshold || oldRadius != cfg->radius || oldWidth != pict.rectFull.dx || oldHeight != pict.rectFull.dy) {
        oldThreshold = cfg->threshold;
        oldRadius = cfg->radius;
        oldWidth = pict.rectFull.dx;
        oldHeight = pict.rectFull.dy;
        reconfigure = 1;
    }

    if (reconfigure) {
        done();
        gradFunContext = configure((float)oldThreshold / 100, oldRadius, pict);
        if (gradFunContext == NULL) {
            reconfigure = 0;
            return parent->processSample(++it, pict);
        }
        reconfigure = 0;
    }

    if (gradFunContext != NULL) {
        filter(gradFunContext, src, pict);
    }

    return parent->processSample(++it, pict);
}
