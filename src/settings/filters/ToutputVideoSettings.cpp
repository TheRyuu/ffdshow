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
#include "Tconvert.h"
#include "ToutputVideoSettings.h"
#include "Tconfig.h"

const TfilterIDFF ToutputVideoSettings::idffs = {
    /*name*/      _l("Output"),
    /*id*/        IDFF_filterOutputVideo,
    /*is*/        0,
    /*order*/     0,
    /*show*/      0,
    /*full*/      0,
    /*half*/      0,
    /*dlgId*/     0,
};

const char_t* ToutputVideoSettings::dvNorms[] = {
    _l("PAL"),
    _l("NTSC"),
    _l("closest"),
    NULL
};

const char_t* ToutputVideoSettings::deintMethods[] = {
    _l("Auto"),
    _l("Force weave"),
    _l("Force bob"),
    NULL
};

const char_t* ToutputVideoSettings::deintFieldOrder[] = {
    _l("Auto"),
    _l("Top field first"),
    _l("Bottom field first"),
    NULL
};

const char_t* ToutputVideoSettings::rgbInterlaceMethods[] = {
    _l("Auto"),
    _l("Force progressive"),
    _l("Force interlace"),
    NULL
};

const uint64_t ToutputVideoSettings::primaryColorSpaces[] = {
    FF_CSP_420P,
    FF_CSP_NV12,
    FF_CSP_RGB32,
    FF_CSP_YUY2,
    FF_CSP_AYUV,
    FF_CSP_P010,
    FF_CSP_P210,
    FF_CSP_Y416,
    0
};

ToutputVideoSettings::ToutputVideoSettings(TintStrColl *Icoll, TfilterIDFFs *filters): TfilterSettingsVideo(sizeof(*this), Icoll, filters, &idffs, false)
{
    half = 0;
    full = 1;
    static const TintOptionT<ToutputVideoSettings> iopts[] = {
        IDFF_flip               , &ToutputVideoSettings::flip               , 0, 0, _l(""), 1,
        _l("flip"), 0,

        IDFF_hwOverlayAspect    , &ToutputVideoSettings::hwOverlayAspect    , 1, 1, _l(""), 1,
        _l("hwOverlayAspect"), 0,
        IDFF_setDeintInOutSample, &ToutputVideoSettings::hwDeinterlace      , 0, 0, _l(""), 1,
        _l("setDeintInOutSample"), 1,
        IDFF_hwDeintMethod      , &ToutputVideoSettings::hwDeintMethod      , 0, 2, _l(""), 1,
        _l("hwDeintMethod"), 0,
        IDFF_hwDeintFieldOrder  , &ToutputVideoSettings::hwDeintFieldOrder  , 0, 2, _l(""), 1,
        _l("hwDeintFieldOrder"), 0,

        IDFF_outPrimaryCSP      , &ToutputVideoSettings::outPrimaryCsp    , 0, 58, _l(""), 0,
        _l("outPrimaryCsp"), 0, // 0:Auto

        IDFF_outYV12            , &ToutputVideoSettings::yv12               , 0, 0, _l(""), 0,
        _l("outYV12"), 1,
        IDFF_outYUY2            , &ToutputVideoSettings::yuy2               , 0, 0, _l(""), 0,
        _l("outYUY2"), 1,
        IDFF_outUYVY            , &ToutputVideoSettings::uyvy               , 0, 0, _l(""), 0,
        _l("outUYVY"), 1,
        IDFF_outNV12            , &ToutputVideoSettings::nv12               , 0, 0, _l(""), 0,
        _l("outNV12"), 1,
        IDFF_outRGB32           , &ToutputVideoSettings::rgb32              , 0, 0, _l(""), 0,
        _l("outRGB32"), 1,
        IDFF_outRGB24           , &ToutputVideoSettings::rgb24              , 0, 0, _l(""), 0,
        _l("outRGB24"), 1,
        IDFF_outP016            , &ToutputVideoSettings::p016               , 0, 0, _l(""), 0,
        _l("outP016"), 1,
        IDFF_outP010            , &ToutputVideoSettings::p010               , 0, 0, _l(""), 0,
        _l("outP010"), 1,
        IDFF_outP210            , &ToutputVideoSettings::p210               , 0, 0, _l(""), 0,
        _l("outP210"), 1,
        IDFF_outP216            , &ToutputVideoSettings::p216               , 0, 0, _l(""), 0,
        _l("outP216"), 1,
        IDFF_outAYUV            , &ToutputVideoSettings::ayuv               , 0, 0, _l(""), 0,
        _l("outAYUV"), 0,
        IDFF_outY416            , &ToutputVideoSettings::y416               , 0, 0, _l(""), 0,
        _l("outY416"), 1,
        IDFF_highQualityRGB     , &ToutputVideoSettings::highQualityRGB     , 0, 0, _l(""), 1,
        _l("highQualityRGB"), 1,
        IDFF_RGB_dithering      , &ToutputVideoSettings::dithering          , 0, 0, _l(""), 1,
        _l("dithering"), 1,
        IDFF_cspOptionsIturBt                , &ToutputVideoSettings::cspOptionsIturBt           , 0, TrgbPrimaries::ITUR_BT_MAX - 1, _l(""), 1,
        _l("cspOptionsIturBt2"), TrgbPrimaries::ITUR_BT_AUTO,
        IDFF_cspOptionsInputLevelsMode       , &ToutputVideoSettings::cspOptionsInputLevelsMode  , 0, 3, _l(""), 1,
        _l("cspOptionsInputLevelsMode"), TrgbPrimaries::RecYCbCr,
        IDFF_cspOptionsOutputLevelsMode      , &ToutputVideoSettings::cspOptionsOutputLevelsMode , 0, 1, _l(""), 1,
        _l("cspOptionsOutputLevelsMode"), TrgbPrimaries::PcRGB,
        IDFF_cspOptionsBlackCutoff           , &ToutputVideoSettings::cspOptionsBlackCutoff      , 0, 32, _l(""), 1,
        _l("cspOptionsBlackCutoff"), 16,
        IDFF_cspOptionsWhiteCutoff           , &ToutputVideoSettings::cspOptionsWhiteCutoff      , 215, 255, _l(""), 1,
        _l("cspOptionsWhiteCutoff"), 235,
        IDFF_cspOptionsChromaCutoff          , &ToutputVideoSettings::cspOptionsChromaCutoff     , 1, 32, _l(""), 1,
        _l("cspOptionsChromaCutoff"), 16,
        IDFF_cspOptionsRgbInterlaceMode      , &ToutputVideoSettings::cspOptionsRgbInterlaceMode , 0, 2, _l(""), 1,
        _l("cspOptionsRgbInterlaceMode"), 0,
        0
    };
    addOptions(iopts);
    static const TcreateParamList1 listDeintMethods(deintMethods);
    setParamList(IDFF_hwDeintMethod, &listDeintMethods);
    static const TcreateParamList1 listDeintFieldOrder(deintFieldOrder);
    setParamList(IDFF_hwDeintFieldOrder, &listDeintFieldOrder);
    static const TcreateParamList1 listRgbInterlaceMethods(rgbInterlaceMethods);
    setParamList(IDFF_cspOptionsRgbInterlaceMode, &listRgbInterlaceMethods);
}

int ToutputVideoSettings::getDefault(int id)
{
    switch (id) { // for upgrade. IDFF_setDeintInOutSample is now independent from IDFF_setSARinOutSample.
        case IDFF_setDeintInOutSample:
            return 1;
        default:
            return TfilterSettingsVideo::getDefault(id);
    }
}

void ToutputVideoSettings::reg_op_outcsps(TregOp &t)
{
    t._REG_OP_N(IDFF_outYV12  , _l("outYV12")  , yv12  , 1);
    t._REG_OP_N(IDFF_outYUY2  , _l("outYUY2")  , yuy2  , 1);
    t._REG_OP_N(IDFF_outUYVY  , _l("outUYVY")  , uyvy  , 1);
    t._REG_OP_N(IDFF_outNV12  , _l("outNV12")  , nv12  , 1);
    t._REG_OP_N(IDFF_outRGB32 , _l("outRGB32") , rgb32 , 1);
    t._REG_OP_N(IDFF_outRGB24 , _l("outRGB24") , rgb24 , 1);
    t._REG_OP_N(IDFF_outP016  , _l("outP016")  , p016  , 1);
    t._REG_OP_N(IDFF_outP010  , _l("outP010")  , p010  , 1);
    t._REG_OP_N(IDFF_outP210  , _l("outP210")  , p210  , 1);
    t._REG_OP_N(IDFF_outP216  , _l("outP216")  , p216  , 1);
    t._REG_OP_N(IDFF_outAYUV  , _l("outAYUV")  , ayuv  , 0);
    t._REG_OP_N(IDFF_outY416  , _l("outY416")  , y416  , 1);
    t._REG_OP_N(IDFF_hwOverlayAspect, _l("hwOverlayAspect"), hwOverlayAspect, 0);
}

const int* ToutputVideoSettings::getResets(unsigned int pageId)
{
    static const int idResets[] = {IDFF_flip, IDFF_outYV12, IDFF_outYUY2, IDFF_outUYVY, IDFF_outNV12,
                                   IDFF_outP016, IDFF_outP010, IDFF_outP210, IDFF_outP216, IDFF_outAYUV, IDFF_outY416,
                                   IDFF_outRGB32, IDFF_outRGB24, IDFF_setDeintInOutSample, IDFF_hwDeintMethod, IDFF_hwDeintFieldOrder, 0
                                  };
    return idResets;
}

void ToutputVideoSettings::getOutputColorspaces(TlistFF_CSPS &ocsps)
{
    ocsps.clear();
    if (yv12) {
        ocsps.push_back(FF_CSP_420P);
    }
    if (nv12) {
        ocsps.push_back(FF_CSP_NV12);
    }
    if (rgb32) {
        ocsps.push_back(FF_CSP_RGB32);
    }
    if (rgb24) {
        ocsps.push_back(FF_CSP_RGB24);
    }
    if (yuy2) {
        ocsps.push_back(FF_CSP_YUY2);
    }
    if (uyvy) {
        ocsps.push_back(FF_CSP_UYVY);
    }
    if (p016) {
        ocsps.push_back(FF_CSP_P016);
    }
    if (p010) {
        ocsps.push_back(FF_CSP_P010);
    }
    if (p210) {
        ocsps.push_back(FF_CSP_P210);
    }
    if (p216) {
        ocsps.push_back(FF_CSP_P216);
    }
    if (ayuv) {
        ocsps.push_back(FF_CSP_AYUV);
    }
    if (y416) {
        ocsps.push_back(FF_CSP_Y416);
    }
    if (outPrimaryCsp) {
        ocsps.push_back(csp_reg2ffdshow(outPrimaryCsp));
    }
    ocsps.unique();
}

void ToutputVideoSettings::getOutputColorspaces(TcspInfos &ocsps)
{
    TlistFF_CSPS ocspsi;
    getOutputColorspaces(ocspsi);
    ocsps.clear();
    foreach(uint64_t o, ocspsi) {
        ocsps.push_back(csp_getInfo(o));
    }
}

int ToutputVideoSettings::get_cspOptionsWhiteCutoff(enum AVColorRange video_full_range_flag) const
{
    if (cspOptionsInputLevelsMode == TrgbPrimaries::AutoYCbCr) {
        return video_full_range_flag == AVCOL_RANGE_JPEG ? 255 : 235;
    } else if (cspOptionsInputLevelsMode == TrgbPrimaries::RecYCbCr) {
        return 235;
    } else  if (cspOptionsInputLevelsMode == TrgbPrimaries::PcYCbCr) {
        return 255;
    }

    return cspOptionsWhiteCutoff;
}

int ToutputVideoSettings::get_cspOptionsBlackCutoff(enum AVColorRange video_full_range_flag) const
{
    if (cspOptionsInputLevelsMode == TrgbPrimaries::AutoYCbCr) {
        return video_full_range_flag == AVCOL_RANGE_JPEG ? 0 : 16;
    } else if (cspOptionsInputLevelsMode == TrgbPrimaries::RecYCbCr) {
        return 16;
    } else  if (cspOptionsInputLevelsMode == TrgbPrimaries::PcYCbCr) {
        return 0;
    }

    return cspOptionsBlackCutoff;
}

int ToutputVideoSettings::get_cspOptionsChromaCutoff(enum AVColorRange video_full_range_flag) const
{
    if (cspOptionsInputLevelsMode == TrgbPrimaries::AutoYCbCr) {
        return video_full_range_flag == AVCOL_RANGE_JPEG ? 1 : 16;
    } else if (cspOptionsInputLevelsMode == TrgbPrimaries::RecYCbCr) {
        return 16;
    } else  if (cspOptionsInputLevelsMode == TrgbPrimaries::PcYCbCr) {
        return 1;
    }

    return get_cspOptionsChromaCutoffStatic(cspOptionsBlackCutoff, cspOptionsWhiteCutoff, cspOptionsChromaCutoff);
}

int ToutputVideoSettings::get_cspOptionsChromaCutoffStatic(int blackCutoff, int whiteCutoff, int chromaCutoff)
{
    int result = 16;
    result = int(double(255 - (whiteCutoff - blackCutoff)) / 36.0 * 16.0);

    if (result < 1) {
        result = 1;
    }
    return result;
}

int ToutputVideoSettings::brightness2luma(int brightness, enum AVColorRange video_full_range_flag) const
{
    int b = get_cspOptionsBlackCutoff(video_full_range_flag);
    int w = get_cspOptionsWhiteCutoff(video_full_range_flag);
    int luma = int(0.5 + b + brightness * (w - b) / 255.0);
    return limit(luma, 0, 255);
}

#if 0
int ToutputVideoSettings::luma2brightness(int luma) const
{
    double Kr;
    int b = get_cspOptionsBlackCutoff();
    int w = get_cspOptionsWhiteCutoff();
    int c = 128 - get_cspOptionsChromaCutoff();
    if (cspOptionsIturBt == TrgbPrimaries::ITUR_BT601) {
        Kr = 0.299;
    } else {
        Kr = 0.2125;
    }
    int brightness = int(255.0 / (w - b) * luma + 255.0 / c * 128.0 * (1 - Kr) - (255.0 * b / (w - b) + 255.0 * 128.0 / c * (1 - Kr)));
    return limit(brightness, 0, 255);
}
#endif

