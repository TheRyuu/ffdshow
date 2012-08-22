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
#include "IffdshowBase.h"
#include "IffdshowDecVideo.h"
#include "IffdshowEnc.h"
#include "TvideoCodec.h"
#include "TvideoCodecLibavcodec.h"
#include "TvideoCodecLibavcodecDxva.h"
#include "TvideoCodecUncompressed.h"
#include "TvideoCodecXviD4.h"
#include "TvideoCodecLibmpeg2.h"
#include "TvideoCodecWmv9.h"
#include "TvideoCodecAvisynth.h"
#include "TvideoCodecQuickSync.h"
#include "dsutil.h"
#include "ffmpeg/libavcodec/avcodec.h"

//======================================= TvideoCodec =======================================
TvideoCodec::TvideoCodec(IffdshowBase *Ideci): Tcodec(Ideci)
{
    ok = false;
}
TvideoCodec::~TvideoCodec()
{
}

//===================================== TvideoCodecDec ======================================
TvideoCodecDec* TvideoCodecDec::initDec(IffdshowBase *deci, IdecVideoSink *sink, AVCodecID codecId, FOURCC fcc, const CMediaType &mt)
{
    // DXVA mode is a preset setting
    switch (codecId) {
        case AV_CODEC_ID_H264:
            if (deci->getParam2(IDFF_filterMode) & IDFF_FILTERMODE_VIDEODXVA) {
                if (deci->getParam2(IDFF_dec_DXVA_H264)) {
                    codecId = CODEC_ID_H264_DXVA;
                } else {
                    return NULL;
                }
            }
            break;
        case AV_CODEC_ID_VC1:
        case CODEC_ID_WMV9_LIB:
            if (deci->getParam2(IDFF_filterMode) & IDFF_FILTERMODE_VIDEODXVA) {
                if (deci->getParam2(IDFF_dec_DXVA_VC1)) {
                    codecId = CODEC_ID_VC1_DXVA;
                } else {
                    return NULL;
                }
            }
            break;
        default:
            break;
    }

    TvideoCodecDec *movie = NULL;

    if (is_quicksync_codec(codecId)) {
        movie = new TvideoCodecQuickSync(deci, sink, codecId);
    } else if (lavc_codec(codecId)) {
        movie = new TvideoCodecLibavcodec(deci, sink);
    } else if (raw_codec(codecId)) {
        movie = new TvideoCodecUncompressed(deci, sink);
    } else if (wmv9_codec(codecId)) {
        movie = new TvideoCodecWmv9(deci, sink);
    } else if (codecId == CODEC_ID_XVID4) {
        movie = new TvideoCodecXviD4(deci, sink);
    } else if (codecId == CODEC_ID_LIBMPEG2) {
        movie = new TvideoCodecLibmpeg2(deci, sink);
    } else if (codecId == CODEC_ID_AVISYNTH) {
        movie = new TvideoCodecAvisynth(deci, sink);
    } else if (codecId == CODEC_ID_H264_DXVA || codecId == CODEC_ID_VC1_DXVA) {
        movie = new TvideoCodecLibavcodecDxva(deci, sink, codecId);
    } else {
        return NULL;
    }
    if (!movie) {
        return NULL;
    }
    if (movie->ok && movie->testMediaType(fcc, mt)) {
        movie->codecId = codecId;
        return movie;
    } else if (is_quicksync_codec(codecId)) {
        // QuickSync decoder init failed, revert to internal decoder.
        switch (codecId) {
            case CODEC_ID_H264_QUICK_SYNC:
                codecId = AV_CODEC_ID_H264;
                break;
            case CODEC_ID_MPEG2_QUICK_SYNC:
                codecId = CODEC_ID_LIBMPEG2;
                break;
            case CODEC_ID_VC1_QUICK_SYNC:
                codecId = CODEC_ID_WMV9_LIB;
                break;
            default:
                ASSERT(FALSE); // this shouldn't happen!
        }

        delete movie;

        // Call this function again with the new codecId.
        return initDec(deci, sink, codecId, fcc, mt);
    } else {
        delete movie;
        return NULL;
    }
}

TvideoCodecDec::TvideoCodecDec(IffdshowBase *Ideci, IdecVideoSink *Isink):
    Tcodec(Ideci), TcodecDec(Ideci, Isink), TvideoCodec(Ideci),
    sinkD(Isink),
    deciV(Ideci),
    quantsDx(0),
    quantsDy(0),
    quants(NULL),
    quantType(FF_QSCALE_TYPE_MPEG1),
    inter_matrix(NULL),
    intra_matrix(NULL),
    telecineManager(this)
{
    isdvdproc = !!deci->getParam2(IDFF_dvdproc);
}

TvideoCodecDec::~TvideoCodecDec()
{
}

float TvideoCodecDec::calcMeanQuant(void)
{
    if (!quants || !quantsDx || !quantsDy) {
        return 0;
    }
    unsigned int sum = 0, num = quantsDx * quantsDy;
    unsigned char *quants1 = (unsigned char*)quants;
    for (unsigned int y = 0; y < quantsDy; y++)
        for (unsigned int x = 0; x < quantsDx; x++) {
            sum += quants1[(y * quantsStride + x) * quantBytes];
        }
    return float(sum) / num;
}

Rational TvideoCodecDec::guessMPEG2sar(const Trect &r, const Rational &sar2, const Rational &containerSar)
{
    const Rational &sar1 = r.sar;
    if (codecId != AV_CODEC_ID_MPEG2VIDEO && codecId != CODEC_ID_LIBMPEG2) {
        return sar1;
    }
    if (isdvdproc) {
        // for DVD, use containerSar
        return containerSar;
    }
    if (r.dx * sar1.num * 9 == r.dy * sar1.den * 16)
        // 16:9, sar1 OK
    {
        return sar1;
    }
    if (r.dx * sar1.num * 3 == r.dy * sar1.den * 4)
        // 4:3, sar1 OK
    {
        return sar1;
    }
    if (sar1.num * containerSar.den == sar1.den * containerSar.num)
        // containerSar, OK
    {
        return containerSar;
    }
    if (r.dx * sar2.num * 9 == r.dy * sar2.den * 16) {
        // 16:9, use sar2
        return sar2;
    }
    if (r.dx * sar2.num * 3 == r.dy * sar2.den * 4) {
        // 4:3, use sar2
        return sar2;
    }
    if (sar1.num * containerSar.den == sar1.den * containerSar.num) {
        // containerSar == sar2
        return containerSar;
    }
    if (r.dx * containerSar.num * 9 == r.dy * containerSar.den * 16) {
        // 16:9, use containerSar
        return containerSar;
    }
    if (r.dx * containerSar.num * 3 == r.dy * containerSar.den * 4) {
        // 4:3, use containerSar
        return containerSar;
    }

    // shouldn't reach here, but don't interfere too much in this case.
    return sar1;
}

//============================= TvideoCodecDec::TtelecineManager ==============================
TvideoCodecDec::TtelecineManager::TtelecineManager(TvideoCodecDec* Iparent):
    parent(Iparent)
{
    onSeek();
}

void TvideoCodecDec::TtelecineManager::onSeek(void)
{
    segment_count = pos_in_group = -1;
    group_rtStart = REFTIME_INVALID;
    film = false;
}

void TvideoCodecDec::TtelecineManager::new_frame(int top_field_first, int repeat_pict, const REFERENCE_TIME &rtStart, const REFERENCE_TIME &rtStop)
{
    segment_count++;
    int pos = segment_count & 1;

    if (repeat_pict == 1) {
        group[pos].fieldtype = FIELD_TYPE::PROGRESSIVE_FRAME;
    } else {
        group[pos].fieldtype = top_field_first ? FIELD_TYPE::INT_TFF : FIELD_TYPE::INT_BFF;
    }

    // egur - repeat_pict may be equal to 2 or 3 for H264 frame doubling/tripling feature.
    //        this is not telecined content - just simple progressive frame.
    group[pos].repeat_pict = (repeat_pict == 1) ? 1 : 0;
    film = false;

    if (segment_count >= 2) {
        for (int i = 0 ; i < 2 ; i++) {
            if (group[i].repeat_pict) {
                film = true;
                break;
            }
        }
    }
    group[pos].rtStart = rtStart;

    if (film) {
        pos_in_group++;
    } else {
        group_rtStart = REFTIME_INVALID;
        pos_in_group = -1;
    }

    if (film && (pos_in_group == 0 || pos_in_group >= 2) && rtStart != REFTIME_INVALID && group_rtStart == REFTIME_INVALID) {
        pos_in_group = 0;
        group_rtStart = rtStart;
    }
    cfg_softTelecine = parent->deci->getParam2(IDFF_softTelecine);
    if (parent->deci->getParam2(IDFF_isAvisynth)
            && parent->deci->getParam2(IDFF_showAvisynth)
            && parent->deci->getParam2(IDFF_avisynthApplyPulldown) == 1) {
        cfg_softTelecine = false;
    }
}

void TvideoCodecDec::TtelecineManager::get_fieldtype(TffPict &pict)
{
    if (!film) {
        return;
    }

    pict.film = true;
    if (cfg_softTelecine) {
        pict.fieldtype = FIELD_TYPE::PROGRESSIVE_FRAME;
    } else {
        pict.repeat_first_field = group[segment_count & 1].repeat_pict != 0;
        if (pict.repeat_first_field) {
            pict.fieldtype = group[(segment_count - 1) & 1].fieldtype;
        } else {
            pict.fieldtype = group[segment_count & 1].fieldtype;
        }
    }
}

void TvideoCodecDec::TtelecineManager::get_timestamps(TffPict &pict)
{
    static const double average_duration = 1e7 / (24.0 * 1000.0 / 1001.0); // duration of 23.976 frame

    if (film && cfg_softTelecine && group_rtStart != REFTIME_INVALID) {
        pict.rtStart = group_rtStart + (REFERENCE_TIME)(average_duration * pos_in_group);
        pict.rtStop = pict.rtStart + 1;
    }

    return;
}

//===================================== TvideoCodecEnc ======================================
TvideoCodecEnc::TvideoCodecEnc(IffdshowBase *Ideci, IencVideoSink *Isink):
    Tcodec(Ideci), TvideoCodec(deci),
    deciE(Ideci),
    sinkE(Isink)
{
    if (deciE) {
        deciE->getCoSettingsPtr(&coCfg);
    }
}
TvideoCodecEnc::~TvideoCodecEnc()
{
}

bool TvideoCodecEnc::getExtradata(const void* *ptr, size_t *len)
{
    if (len) {
        *len = 0;
    } else {
        return false;
    }
    if (ptr) {
        *ptr = NULL;
    } else {
        return false;
    }
    return true;
}

//======================================== TencLibs =========================================
void TvideoCodecs::init(IffdshowBase *deci, IencVideoSink *sink)
{
    push_back(new TvideoCodecLibavcodec(deci, sink));
    //push_back(new TvideoCodecXviD4(deci,sink));
    //push_back(new TvideoCodecWmv9(deci,sink));
    push_back(new TvideoCodecUncompressed(deci, sink));
}
TvideoCodecEnc* TvideoCodecs::getEncLib(int codecId)
{
    for (const_iterator l = begin(); l != end(); l++)
        if (*l)
            for (Tencoders::const_iterator e = (*l)->encoders.begin(); e != (*l)->encoders.end(); e++)
                if ((*e)->id == codecId) {
                    return *l;
                }
    return NULL;
}
const Tencoder* TvideoCodecs::getEncoder(int codecId) const
{
    for (const_iterator l = begin(); l != end(); l++)
        if (*l)
            for (Tencoders::const_iterator e = (*l)->encoders.begin(); e != (*l)->encoders.end(); e++)
                if ((*e)->id == codecId) {
                    return *e;
                }
    return NULL;
}
