/*
 * Copyright (C) 2006 Michael Niedermayer <michaelni@gmx.at>
 * Ported to ffdshow by h.yamagata.
 *
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "stdafx.h"
#include "TdeinterlaceSettings.h"
#include "TimgFilterYadif.h"
#include "yadif/vf_yadif.h"
#include "IffdshowBase.h"
#include "Tconfig.h"
#include "ffdebug.h"

static inline void * memcpy_pic(unsigned char * dst, const unsigned char * src, int bytesPerLine, int height, int dstStride, int srcStride)
{
    int i;
    void *retval=dst;

    if(dstStride == srcStride)
    {
        if (srcStride < 0) {
            src += (height-1)*srcStride;
            dst += (height-1)*dstStride;
            srcStride = -srcStride;
        }

        memcpy(dst, src, srcStride*height);
    }
    else
    {
        for(i=0; i<height; i++)
        {
            memcpy(dst, src, bytesPerLine);
            src+= srcStride;
            dst+= dstStride;
        }
    }

    return retval;
}

void TimgFilterYadif::store_ref(const uint8_t *src[3], stride_t src_stride[3], int width, int height)
{
    int i;

    memcpy (yadctx->ref[3], yadctx->ref[0], sizeof(uint8_t *)*3);
    memmove(yadctx->ref[0], yadctx->ref[1], sizeof(uint8_t *)*3*3);


    // src[0] == NULL, at the end of stream, means it's the last picture
    if(src[0]){
        for(i=0; i<3; i++){
            int is_chroma= !!i;

            memcpy_pic(yadctx->ref[2][i], src[i], width>>is_chroma, height>>is_chroma, yadctx->stride[i], src_stride[i]);
        }
    }else{
        for(i=0; i<3; i++){
            int is_chroma= !!i;

            memcpy_pic(yadctx->ref[2][i], yadctx->ref[1][i], width>>is_chroma, height>>is_chroma, yadctx->stride[i], yadctx->stride[i]);
        }
    }
}

HRESULT TimgFilterYadif::put_image(TffPict &pict, const unsigned char *src[4], int full)
{
    int tff = 0;

    if(yadctx->parity < 0) {
        if (pict.fieldtype & FIELD_TYPE::INT_TFF)
            tff = 1;
    }
    else tff = (yadctx->parity&1)^1;

    int dx,dy;
    if (full)
     {
      dx = pict.rectFull.dx;
      dy = pict.rectFull.dy;
     }
    else
     {
      dx = pict.rectClip.dx;
      dy = pict.rectClip.dy;
     }

    store_ref(src, pict.stride, dx, dy);

    yadctx->frame_duration = pict.rtStart - yadctx->buffered_rtStart;
    std::swap(pict.rtStart, yadctx->buffered_rtStart);
    std::swap(pict.rtStop,  yadctx->buffered_rtStop);

    if(yadctx->do_deinterlace == 0)
        return S_OK;
    else if(yadctx->do_deinterlace == 1){
        yadctx->do_deinterlace = 2;
        return S_OK;
    }

    REFERENCE_TIME pts = yadctx->buffered_rtStart;
    int i;
    int hr = S_OK;

    for(i = 0 ; i <= (yadctx->mode & 1); i++){
        unsigned char *dst[4];
        bool cspChanged = getNext(csp1, pict, full, dst);

        libmplayer->yadif_filter(yadctx,
                                 dst,
                                 pict.stride,
                                 dx, dy,
                                 i ^ tff ^ 1, tff);

        pict.fieldtype = (pict.fieldtype & ~FIELD_TYPE::MASK_PROG) | FIELD_TYPE::PROGRESSIVE_FRAME;

        if (yadctx->mode & 1){
            pict.rtStop = pict.rtStart + (yadctx->frame_duration >> 1) - 1;
            if (pict.rtStop <= pict.rtStart)
                pict.rtStop = pict.rtStart + 1;
        }

        TffPict inputPicture;
        if(yadctx->mode & 1)
            inputPicture = pict;

        HRESULT hr = parent->deliverSample(++it, pict);
        --it;

        if (i == (yadctx->mode & 1))
            break;

        // only if frame doubler is used and it has just delivered the first image.

        pict = inputPicture;
        pict.discontinuity = false;
        if (pict.rtStart != REFTIME_INVALID && yadctx->frame_duration > 0)
            pict.rtStart += yadctx->frame_duration >> 1;
    }
    return hr;
}

int TimgFilterYadif::config(TffPict &pict)
{
        int i, j;

        for(i = 0 ; i < 3 ; i++){
            int is_chroma = !!i;
            stride_t ffdshow_w = ((pict.rectFull.dx >> is_chroma)/16 + 2) * 16; // from void TffPict::convertCSP(int Icsp,Tbuffer &buf,int edge)
            int w = std::max(pict.stride[i], ffdshow_w);
            int h = ((pict.rectFull.dy + (6 << is_chroma) + 31) & (~31)) >> is_chroma;

            yadctx->stride[i]= w;
            for(j = 0 ; j < 3 ; j++){
                yadctx->ref[j][i]= (uint8_t*)malloc(w * h * sizeof(uint8_t)) + 3 * w;
                memset(yadctx->ref[j][i] - 3 * w, 128 * is_chroma, w * h * sizeof(uint8_t));
            }
        }

	return 1;
}

void TimgFilterYadif::done(void)
{
    if (libmplayer && yadctx)
    {
        int i;

        for(i=0; i<3*3; i++){
            uint8_t **p= &yadctx->ref[i%3][i/3];
            if(*p) ::free(*p - 3 * yadctx->stride[i/3]);
            *p= NULL;
        }
        ::free(yadctx);
        yadctx = NULL;
        hasImageInBuffer = false;
    }
}

YadifContext* TimgFilterYadif::getContext(int mode, int parity){

    YadifContext *yadctx = (YadifContext*)malloc(sizeof(YadifContext));
    if (!yadctx) return yadctx;

    memset(yadctx, 0, sizeof(YadifContext));
    yadctx->do_deinterlace=1;

    yadctx->mode = mode;
    yadctx->parity = parity;

    return yadctx;
}

//==========================================================================================================

/*
 * Copyright (c) 2008 h.yamagata
 */

TimgFilterYadif::TimgFilterYadif(IffdshowBase *Ideci,Tfilters *Iparent,bool Ibob):TimgFilter(Ideci,Iparent)
{
    hasImageInBuffer = false;
    deci->getPostproc(&libmplayer);
    if (libmplayer) libmplayer->yadif_init();
    yadctx = NULL;
    // oldcfg.cfgId=-1;
}

TimgFilterYadif::~TimgFilterYadif()
{
    done();
    if (libmplayer) libmplayer->Release();
}

bool TimgFilterYadif::is(const TffPictBase &pict,const TfilterSettingsVideo *cfg)
{
    return libmplayer && super::is(pict,cfg);
}

void TimgFilterYadif::onSeek(void)
{
    done();
}

void TimgFilterYadif::onSizeChange(void)
{
    done();
}

bool TimgFilterYadif::onPullImageFromSubtitlesFilter(void)
{
    if (!yadctx || !libmplayer || yadctx->do_deinterlace != 2 || !hasImageInBuffer)
        return false;

     for (int i=0 ; i<3 ; i++){
        oldpict.data[i] = yadctx->ref[2][i];
        oldpict.stride[i] = yadctx->stride[i];
    }
    hasImageInBuffer = false;
    parent->setStopAtSubtitles(true);
    parent->deliverSample(++it,oldpict);
    return true;
}

HRESULT TimgFilterYadif::onEndOfStream(void)
{
    if (!yadctx || !libmplayer)
        return S_OK;

    // here, it is guaranteed that we have copied last pict to oldpict.
    oldpict.rtStart += yadctx->frame_duration;
    oldpict.rtStop += yadctx->frame_duration;
    const unsigned char *src[4];
    src[0] = src[1] = src[2] = src[3] = NULL;
    return put_image(oldpict, src, oldcfg.full);
}

HRESULT TimgFilterYadif::onDiscontinuity(TffPict pict)
{
    if (!yadctx || !libmplayer)
        return S_OK;

    const unsigned char *src[4];
    src[0] = src[1] = src[2] = src[3] = NULL;
    return put_image(pict, src, cfg->full);
}

HRESULT TimgFilterYadif::process(TfilterQueue::iterator it0,TffPict &pict,const TfilterSettingsVideo *cfg0)
{
    it = it0;
    cfg = (const TdeinterlaceSettings*)cfg0;
    hasImageInBuffer = false;

    if ((pict.fieldtype == FIELD_TYPE::PROGRESSIVE_FRAME && !cfg->deinterlaceAlways) || !libmplayer){
        done();
        return parent->deliverSample(++it,pict);
    }

    if (pict.rectClip != pict.rectFull && !cfg->full)
        parent->dirtyBorder=1;

    init(pict,cfg->full,cfg->half);
    if(pict.discontinuity || (pict.fieldtype & FIELD_TYPE::MASK_SEQ)){
        onDiscontinuity(pict);
        done();
    }

    const unsigned char *src[4];
    bool cspChanged = getCur(FF_CSP_420P, pict, cfg->full,src);

    if (!yadctx){
        yadctx = getContext(cfg->yadifMode, cfg->yadifParity);
        config(pict);
    }

    oldpict = pict;
    oldcfg = *cfg;

    yadctx->mode = cfg->yadifMode;
    yadctx->parity = cfg->yadifParity;

    if (yadctx->do_deinterlace == 1){
        REFERENCE_TIME rtStart = pict.rtStart,rtStop = pict.rtStop;
        pict.rtStop = pict.rtStart + 1;
        put_image(pict, src, cfg->full);
        pict.rtStart = rtStart;
        pict.rtStop = rtStop;
    }

    hasImageInBuffer = true;
    return put_image(pict, src, cfg->full);
}
