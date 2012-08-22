/*
 * Copyright (c) 2002-2006 Milan Cutka
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "stdafx.h"
#include "wmv9/ff_wmv9.h"
#include "TvideoCodecWmv9.h"
#include "Tdll.h"
#include "ffcodecs.h"
#include "TcodecSettings.h"
#include "IffdshowBase.h"
#include "IffdshowDecVideo.h"
#include "dsutil.h"

const char_t* TvideoCodecWmv9::dllname = _l("ff_wmv9.dll");

TvideoCodecWmv9::TvideoCodecWmv9(IffdshowBase *Ideci, IdecVideoSink *IsinkD):
    Tcodec(Ideci), TcodecDec(Ideci, IsinkD),
    TvideoCodec(Ideci),
    TvideoCodecDec(Ideci, IsinkD)
{
    create();
}

void TvideoCodecWmv9::create(void)
{
    ok = false;
    wmv9 = NULL;
    infos = NULL;
    dll = new Tdll(dllname, config);
    dll->loadFunction(createWmv9, "createWmv9");
    dll->loadFunction(destroyWmv9, "destroyWmv9");
    if (dll->ok) {
        wmv9 = createWmv9();
        ok = wmv9->getOk();
    }
}
TvideoCodecWmv9::~TvideoCodecWmv9()
{
    end();
    if (dll) {
        if (wmv9) {
            destroyWmv9(wmv9);
        }
        delete dll;
    }
    if (infos) {
        delete []infos;
    }
}

const char_t* TvideoCodecWmv9::getName(void) const
{
    tsprintf(codecName, _l("wmv9"), codecIndex >= 0 ? _l(" (") : _l(""), codecIndex >= 0 ? infos[codecIndex].name : _l(""), codecIndex >= 0 ? _l(")") : _l(""));
    return codecName;
}

bool TvideoCodecWmv9::getExtradata(const void* *ptr, size_t *len)
{
    if (!len || !wmv9) {
        return false;
    }
    wmv9->getExtradata(ptr, len);
    return true;
}

bool TvideoCodecWmv9::testMediaType(FOURCC fcc, const CMediaType &mt)
{
    if (wmv9) {
        const Tff_wmv9codecInfo *codec = wmv9->findCodec(fcc);
        if (!codec) {
            return AV_CODEC_ID_NONE;
        }
        codecIndex = codec->index;
        return !!codec;
    } else {
        return AV_CODEC_ID_NONE;
    }
}

bool TvideoCodecWmv9::beginDecompress(TffPictBase &pict, FOURCC infcc, const CMediaType &mt, int sourceFlags)
{
    Textradata extradata(mt);
    rd = pict.rectFull;
    if (wmv9->decStart(infcc, deciV->getAVIfps1000_2() / 1000.0, pict.rectFull.dx, pict.rectFull.dy, extradata.data, extradata.size, &csp)) {
        pict.csp = csp;
        return true;
    } else {
        return false;
    }
}

HRESULT TvideoCodecWmv9::decompress(const unsigned char *src, size_t srcLen, IMediaSample *pIn)
{
    unsigned char *dst[4] = {NULL, NULL, NULL, NULL};
    stride_t stride[4] = {0, 0, 0, 0};
    if (wmv9->decompress(src, srcLen, &dst[0], &stride[0]) != 0 && dst[0]) {
        if (pIn->IsPreroll() == S_OK) {
            return sinkD->deliverPreroll(pIn->IsSyncPoint() ? FRAME_TYPE::I : FRAME_TYPE::P);
        }
        TffPict pict(csp, dst, stride, rd, true, pIn);
        return sinkD->deliverDecodedSample(pict);
    } else {
        return S_FALSE;
    }
}
