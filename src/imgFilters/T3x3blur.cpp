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
#include "T3x3blur.h"
#include "ffImgfmt.h"
#include "Tlibavcodec.h"
#include "libswscale/swscale.h"
#include "Tconfig.h"
#include "TffPict.h"
#include "IffdshowBase.h"

T3x3blurSWS::T3x3blurSWS(IffdshowBase *deci, unsigned int Idx, unsigned int Idy): dx(Idx), dy(Idy)
{
    deci->getLibavcodec(&libavcodec);
    SwsFilter swsf;
    swsf.lumV = swsf.lumH = libavcodec->sws_getConstVec(1 / 3.0, 3);
    swsf.chrH = swsf.chrV = NULL;
    int sws_flags = 0;
    SwsParams params;
    Tlibavcodec::swsInitParams(&params, 0, sws_flags);
    swsc = libavcodec->sws_getCachedContext(NULL, dx, dy, PIX_FMT_GRAY8, dx, dy, PIX_FMT_GRAY8, sws_flags, &swsf, NULL, NULL, &params);
    libavcodec->sws_freeVec(swsf.lumH);
}
T3x3blurSWS::~T3x3blurSWS()
{
    if (swsc) {
        libavcodec->sws_freeContext(swsc);
    }
    libavcodec->Release();
}
void T3x3blurSWS::process(const unsigned char *src, stride_t srcStride, unsigned char *dst, stride_t dstStride)
{
    if (swsc) {
        libavcodec->sws_scale(swsc, &src, &srcStride, 0, dy, &dst, &dstStride);
    } else {
        TffPict::copy(dst, dstStride, src, srcStride, dx, dy);
    }
}
