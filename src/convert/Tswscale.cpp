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
#include "Tswscale.h"
#include "Tlibavcodec.h"
#include "libswscale/swscale.h"
#include "ffImgfmt.h"
#include "Tconfig.h"

Tswscale::Tswscale(Tlibavcodec *Ilibavcodec): libavcodec(Ilibavcodec)
{
    swsc = NULL;
}
Tswscale::~Tswscale()
{
    done();
}
bool Tswscale::init(unsigned int Idx, unsigned int Idy, uint64_t incsp, uint64_t outcsp)
{
    done();
    PixelFormat sw_incsp = csp_ffdshow2lavc(incsp), sw_outcsp = csp_ffdshow2lavc(outcsp);
    dx = Idx;
    dy = Idy;
    sws_flags = SWS_BILINEAR | SWS_FULL_CHR_H_INT | SWS_FULL_CHR_H_INP; //Resize method
    SwsParams params;
    Tlibavcodec::swsInitParams(&params, SWS_BILINEAR, sws_flags);
    swsc = libavcodec->sws_getCachedContext(NULL, dx, dy, sw_incsp, dx, dy, sw_outcsp, sws_flags, NULL, NULL, NULL, &params);
    return !!swsc;
}
void Tswscale::done(void)
{
    if (swsc) {
        libavcodec->sws_freeContext(swsc);
    }
    swsc = NULL;
}
bool Tswscale::convert(const uint8_t* src[], const stride_t srcStride[], uint8_t* dst[], stride_t dstStride[], const TrgbPrimaries &rgbPrimaries)
{
    const int *table = libavcodec->sws_getCoefficients(rgbPrimaries.get_sws_cs());
    libavcodec->sws_setColorspaceDetails(swsc, table, rgbPrimaries.isYCbCrFullRange(), table, rgbPrimaries.isRGB_FullRange(),  0, 1 << 16, 1 << 16);
    return swsc && libavcodec->sws_scale(swsc, src, srcStride, 0, dy, dst, dstStride) > 0;
}
