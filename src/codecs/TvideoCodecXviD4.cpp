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
#include "TvideoCodecXviD4.h"
#include "xvidcore/xvid.h"
#include "xvidcore/decoder.h"
#include "Tdll.h"
#include "TcodecSettings.h"
#include "IffdshowBase.h"
#include "dsutil.h"
#include "../compiler.h"

const TmeXviDpreset meXviDpresets[] = {
    _l("None")      , 0,
    _l("Very low")  , 0,
    _l("Low")       , 0,
    _l("Medium")    , 0,
    _l("High")      , XVID_ME::HALFPELREFINE16,
    _l("Very high") , XVID_ME::HALFPELREFINE16 | XVID_ME::ADVANCEDDIAMOND16,
    _l("Ultra high"), XVID_ME::HALFPELREFINE16 | XVID_ME::EXTSEARCH16 | XVID_ME::HALFPELREFINE8 | XVID_ME::USESQUARES16,
    NULL, 0
};
const TmeXviDpreset vhqXviDpresets[] = {
    _l("Off")           , 0,
    _l("Mode decision") , 0,
    _l("Limited search"), XVID_ME_RD::HALFPELREFINE16 | XVID_ME_RD::QUARTERPELREFINE16,
    _l("Medium search") , XVID_ME_RD::HALFPELREFINE16 | XVID_ME_RD::HALFPELREFINE8 | XVID_ME_RD::QUARTERPELREFINE16 | XVID_ME_RD::QUARTERPELREFINE8 | XVID_ME_RD::CHECKPREDICTION,
    _l("Full search")   , XVID_ME_RD::HALFPELREFINE16 | XVID_ME_RD::HALFPELREFINE8 | XVID_ME_RD::QUARTERPELREFINE16 | XVID_ME_RD::QUARTERPELREFINE8 | XVID_ME_RD::CHECKPREDICTION | XVID_ME_RD::EXTSEARCH,
    NULL, 0
};

const char_t* TvideoCodecXviD4::dllname = _l("xvidcore.dll");

int TvideoCodecXviD4::me_hq(int rd3)
{
    int rd4 = 0;
    if (rd3 & XVID_ME_RD::HALFPELREFINE16) {
        rd4 |= XVID_ME_HALFPELREFINE16_RD;
    }
    if (rd3 & XVID_ME_RD::HALFPELREFINE8) {
        rd4 |= XVID_ME_HALFPELREFINE8_RD;
    }
    if (rd3 & XVID_ME_RD::QUARTERPELREFINE16) {
        rd4 |= XVID_ME_QUARTERPELREFINE16_RD;
    }
    if (rd3 & XVID_ME_RD::QUARTERPELREFINE8) {
        rd4 |= XVID_ME_QUARTERPELREFINE8_RD;
    }
    if (rd3 & XVID_ME_RD::EXTSEARCH) {
        rd4 |= XVID_ME_EXTSEARCH_RD;
    }
    if (rd3 & XVID_ME_RD::CHECKPREDICTION) {
        rd4 |= XVID_ME_CHECKPREDICTION_RD;
    }
    return rd4;
}
int TvideoCodecXviD4::me_(int me3)
{
    int me4 = 0;
    if (me3 & XVID_ME::ADVANCEDDIAMOND16) {
        me4 |= XVID_ME_ADVANCEDDIAMOND16;
    }
    if (me3 & XVID_ME::HALFPELREFINE16) {
        me4 |= XVID_ME_HALFPELREFINE16;
    }
    if (me3 & XVID_ME::EXTSEARCH16) {
        me4 |= XVID_ME_EXTSEARCH16;
    }
    if (me3 & XVID_ME::USESQUARES16) {
        me4 |= XVID_ME_USESQUARES16;
    }
    if (me3 & XVID_ME::ADVANCEDDIAMOND8) {
        me4 |= XVID_ME_ADVANCEDDIAMOND8;
    }
    if (me3 & XVID_ME::HALFPELREFINE8) {
        me4 |= XVID_ME_HALFPELREFINE8;
    }
    if (me3 & XVID_ME::EXTSEARCH8) {
        me4 |= XVID_ME_EXTSEARCH8;
    }
    if (me3 & (int)XVID_ME::USESQUARES8) {
        me4 |= XVID_ME_USESQUARES8;
    }
    return me4;
}

TvideoCodecXviD4::TvideoCodecXviD4(IffdshowBase *Ideci, IdecVideoSink *IsinkD):
    Tcodec(Ideci), TcodecDec(Ideci, IsinkD),
    TvideoCodec(Ideci),
    TvideoCodecDec(Ideci, IsinkD)
{
    create();
}

void TvideoCodecXviD4::create(void)
{
    dechandle = NULL;
    extradata = NULL;
    dll = new Tdll(dllname, config);
    dll->loadFunction(xvid_global, "xvid_global");
    dll->loadFunction(xvid_decore, "xvid_decore");
    dll->loadFunction(xvid_plugin_single, "xvid_plugin_single");
    dll->loadFunction(xvid_plugin_lumimasking, "xvid_plugin_lumimasking");
    if (dll->ok) {
        xvid_gbl_init_t init;
        memset(&init, 0, sizeof(init));
        init.version = XVID_VERSION;
        init.cpu_flags = (unsigned int)(XVID_CPU_ASM | XVID_CPU_FORCE);
        if (Tconfig::cpu_flags & FF_CPU_MMX) {
            init.cpu_flags |= XVID_CPU_MMX;
        }
        if (Tconfig::cpu_flags & FF_CPU_MMXEXT) {
            init.cpu_flags |= XVID_CPU_MMXEXT;
        }
        if (Tconfig::cpu_flags & FF_CPU_SSE) {
            init.cpu_flags |= XVID_CPU_SSE;
        }
        if (Tconfig::cpu_flags & FF_CPU_SSE2) {
            init.cpu_flags |= XVID_CPU_SSE2;
        }
        if (Tconfig::cpu_flags & FF_CPU_SSE3) {
            init.cpu_flags |= XVID_CPU_SSE3;
        }
        if (Tconfig::cpu_flags & FF_CPU_SSE41) {
            init.cpu_flags |= XVID_CPU_SSE41;
        }
        if (Tconfig::cpu_flags & FF_CPU_3DNOW) {
            init.cpu_flags |= XVID_CPU_3DNOW;
        }
        if (Tconfig::cpu_flags & FF_CPU_3DNOWEXT) {
            init.cpu_flags |= XVID_CPU_3DNOWEXT;
        }
        if (xvid_global(NULL, XVID_GBL_INIT, &init, NULL) == 0) {
            ok = true;
            quantBytes = 4;
            return;
        }
    }
    ok = false;
}
TvideoCodecXviD4::~TvideoCodecXviD4()
{
    end();
    if (dll) {
        delete dll;
        dll = NULL;
    }
}

//-------------------------- decompression ----------------------------
bool TvideoCodecXviD4::beginDecompress(TffPictBase &pict, FOURCC infcc, const CMediaType &mt, int sourceFlags)
{
    xvid_dec_create_t dr;
    dr.version = XVID_VERSION;
    dr.width = 0;
    dr.height = 0;
    extradata = new Textradata(mt, 8);
    int res = xvid_decore(NULL, XVID_DEC_CREATE, &dr, NULL);
    if (res < 0) {
        return false;
    }
    dechandle = dr.handle;
    pictbuf.clear();
    quantsDx = (pict.rectFull.dx + 15) >> 4;
    quantsDy = (pict.rectFull.dy + 15) >> 4;
    pict.csp = FF_CSP_420P;
    return true;
}
HRESULT TvideoCodecXviD4::flushDec(void)
{
    return decompress(NULL, 0, NULL);
}
HRESULT TvideoCodecXviD4::decompress(const unsigned char *src0, size_t srcLen0, IMediaSample *pIn)
{
    xvid_dec_frame_t xframe;
    xframe.version = XVID_VERSION;
    xframe.general = XVID_LOWDELAY;
    //TODO: XVID_DISCONTINUITY

    xframe.output.csp = XVID4_CSP_PLANAR;
    xframe.brightness = 0;
    const unsigned char *src;
    size_t srcLen;
    TbyteBuffer buf;
    if (extradata && extradata->size) {
        buf.append(extradata->data, extradata->size);
        buf.append(src0, srcLen0);
        src = &*buf.begin();
        srcLen = buf.size();
        delete extradata;
        extradata = NULL;
    } else {
        src = src0;
        srcLen = srcLen0;
    }
    if (pIn)
        if (FAILED(pIn->GetTime(&rtStart, &rtStop))) {
            rtStart = rtStop = _I64_MIN;
        }
repeat:
    xframe.bitstream = (void*)src;
    xframe.length = src0 ? (int)srcLen : -1;
    xframe.output.plane[0] = pict.data[0];
    xframe.output.plane[1] = pict.data[1];
    xframe.output.plane[2] = pict.data[2];
    xframe.output.stride[0] = (int)pict.stride[0];
    xframe.output.stride[1] = (int)pict.stride[1];
    xframe.output.stride[2] = (int)pict.stride[2];
    xvid_dec_stats_t stats;
    memset(&stats, 0, sizeof(stats));
    stats.version = XVID_VERSION;
    int length = xvid_decore(dechandle, XVID_DEC_DECODE, &xframe, &stats);
    if (extradata) {
        delete extradata;
        extradata = NULL;
    }
    if (length < 0 || (stats.type == XVID_TYPE_NOTHING && length > 0)) {
        return S_FALSE;
    }
    if (stats.type == XVID_TYPE_VOL) {
        pict.alloc(stats.data.vol.width, stats.data.vol.height, FF_CSP_420P, pictbuf);
        if (stats.data.vol.par == XVID_PAR_EXT) {
            pict.rectFull.sar = pict.rectClip.sar = Rational(stats.data.vol.par_width, stats.data.vol.par_height);
        } else if (connectedSplitter && stats.data.vol.par == XVID_PAR_11_VGA) { // With MPC's internal matroska splitter, AR is not reliable.
            pict.rectFull.sar = containerSar;
        } else {
            static const int pars[][2] = {
                { 1, 1},
                {12, 11},
                {10, 11},
                {16, 11},
                {40, 33},
                { 0, 0},
            };
            pict.rectFull.sar = pict.rectClip.sar = Rational(pars[stats.data.vol.par - 1][0], pars[stats.data.vol.par - 1][1]);
        }
        src += length;
        srcLen -= length;
        goto repeat;
    } else {
        switch (stats.type) {
            case XVID_TYPE_IVOP:
                pict.frametype = FRAME_TYPE::I;
                break;
            case XVID_TYPE_PVOP:
                pict.frametype = FRAME_TYPE::P;
                break;
            case XVID_TYPE_BVOP:
                pict.frametype = FRAME_TYPE::B;
                break;
            case XVID_TYPE_SVOP:
                pict.frametype = FRAME_TYPE::GMC;
                break;
        }
        if (pIn && pIn->IsPreroll() == S_OK) {
            return sinkD->deliverPreroll(pict.frametype);
        }
        quants = stats.data.vop.qscale;
        quantsStride = stats.data.vop.qscale_stride;
        quantType = 1;
        TffPict p1 = pict;
        DECODER *dec = (DECODER *)dechandle;
        p1.fieldtype = dec->interlacing ? (dec->top_field_first ? FIELD_TYPE::INT_TFF : FIELD_TYPE::INT_BFF) : FIELD_TYPE::PROGRESSIVE_FRAME;
        if (pIn) {
            p1.rtStart = rtStart;
            p1.rtStop = rtStop;
        } else {
            p1.rtStart = rtStop;
            p1.rtStop = rtStop + (rtStop - rtStart);
        }
        p1.setRO(true);
        return sinkD->deliverDecodedSample(p1);
    }
}
