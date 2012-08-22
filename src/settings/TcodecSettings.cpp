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
#include "ffdshow_constants.h"
#include "ffdshow_mediaguids.h"
#include "TcodecSettings.h"
#include "reg.h"
#include "TffProc.h"
#include "xvidcore/xvid.h"
#include "ffImgfmt.h"
#include "Tmuxer.h"
#include "libavcodec/avcodec.h"
#include "ffmpeg/libavcodec/dv_profile.c"

const char_t* TcoSettings::huffYUVcsps[] = {
    _l("YUY2"),
    _l("YV12"),
    NULL
};
const char_t* TcoSettings::huffYUVpreds[] = {
    _l("Left"),
    _l("Plane"),
    _l("Median"),
    NULL
};

const TcspFcc TcoSettings::ffv1csps[] = {
    {_l("YV12") , FOURCC_YV12},
    {_l("444P") , FOURCC_444P},
    {_l("422P") , FOURCC_422P},
    {_l("411P") , FOURCC_411P},
    {_l("410P") , FOURCC_410P},
    {_l("RGB32"), FOURCC_RGB3},
    NULL
};
const char_t* TcoSettings::ffv1coders[] = {
    _l("VLC"),
    _l("AC"),
    NULL
};
const char_t* TcoSettings::ffv1contexts[] = {
    _l("Small"),
    _l("Large"),
    NULL
};

TcoSettings::TcoSettings(TintStrColl *Icoll): Toptions(Icoll), options(Icoll)
{
    static const TintOptionT<TcoSettings> iopts[] = {
        IDFF_enc_mode        , &TcoSettings::mode       , 0,          5, _l(""), 1,
        _l("mode"), ENC_MODE::CBR,
        IDFF_enc_bitrate1000 , &TcoSettings::bitrate1000, 1,      10000, _l(""), 1,
        _l("bitrate1000"), 900,
        IDFF_enc_desiredSize , &TcoSettings::desiredSize, 1, 4 * 1024 * 1024, _l(""), 1,
        _l("desiredSize"), 570000,
        IDFF_enc_quant       , &TcoSettings::quant      , minQuant, maxQuant, _l(""), 1,
        _l("quant"), 2,
        IDFF_enc_qual        , &TcoSettings::qual       , 0,        100, _l(""), 1,
        _l("qual"), 85,

        IDFF_enc_codecId , &TcoSettings::codecId         , 1, 10000, _l(""), 1,
        _l("codecId"), AV_CODEC_ID_MJPEG,
        IDFF_enc_fourcc  , (TintVal)&TcoSettings::fourcc , 1,    1, _l(""), 1,
        _l("fourcc"), FOURCC_MJPG,

        IDFF_enc_interlacing     , &TcoSettings::interlacing     , 0,  0, _l(""), 1,
        _l("interlacing"), 0,
        IDFF_enc_interlacing_tff , &TcoSettings::interlacing_tff , 0,  0, _l(""), 1,
        _l("interlacing_tff"), 0,
        IDFF_enc_gray            , &TcoSettings::gray            , 0,  0, _l(""), 1,
        _l("gray"), 0,

        IDFF_enc_huffyuv_csp  , &TcoSettings::huffyuv_csp , 0, 1, _l(""), 1,
        _l("huffyuv_csp"), 1,
        IDFF_enc_huffyuv_pred , &TcoSettings::huffyuv_pred, 0, 2, _l(""), 1,
        _l("huffyuv_pred"), 1,
        IDFF_enc_huffyuv_ctx  , &TcoSettings::huffyuv_ctx , 0, 1, _l(""), 1,
        _l("huffyuv_ctx"), 0,

        IDFF_enc_ljpeg_csp  , &TcoSettings::ljpeg_csp , 0, 0, _l(""), 1,
        _l("ljpeg_csp"), FOURCC_YV12,

        IDFF_enc_ffv1_coder  , &TcoSettings::ffv1_coder  , 0, 1, _l(""), 1,
        _l("ffv1_coder"), 0,
        IDFF_enc_ffv1_context, &TcoSettings::ffv1_context, 0, 10, _l(""), 1,
        _l("ffv1_context"), 0,
        IDFF_enc_ffv1_key_interval, &TcoSettings::ffv1_key_interval, 1, 500, _l(""), 1,
        _l("ffv1_key_interval"), 10,
        IDFF_enc_ffv1_csp    , &TcoSettings::ffv1_csp    , 1, 1, _l(""), 1,
        _l("ffv1_csp"), FOURCC_YV12,

        IDFF_enc_dv_profile  , &TcoSettings::dv_profile  , DV_PROFILE_AUTO, countof(dv_profiles) - 1, _l(""), 1,
        _l("dv_profile"), DV_PROFILE_AUTO,

        IDFF_enc_forceIncsp, &TcoSettings::forceIncsp , 0, 0, _l(""), 1,
        _l("forceIncsp"), 0,
        IDFF_enc_incsp     , &TcoSettings::incspFourcc, 1, 1, _l(""), 1,
        _l("incsp"), FOURCC_YV12,
        IDFF_enc_isProc    , &TcoSettings::isProc     , 0, 0, _l(""), 1,
        _l("isProc"), 0,
        IDFF_enc_flip      , &TcoSettings::flip       , 0, 0, _l(""), 1,
        _l("flip"), 0,
        IDFF_enc_dropFrames, &TcoSettings::dropFrames , 0, 0, _l(""), 1,
        _l("dropFrames"), 0,

        IDFF_enc_storeAVI       , &TcoSettings::storeAVI       , 0, 0, _l(""), 1,
        _l("storeAVI"), 1,
        IDFF_enc_storeExt       , &TcoSettings::storeExt       , 0, 0, _l(""), 1,
        _l("storeExt"), 0,
        IDFF_enc_muxer          , &TcoSettings::muxer          , 0, 4, _l(""), 1,
        _l("muxer"), Tmuxer::MUXER_FILE,

        IDFF_enc_q_i_min          , &TcoSettings::q_i_min          , minQuant, maxQuant, _l(""), 1,
        _l("q_i_min"), 2,
        IDFF_enc_q_i_max          , &TcoSettings::q_i_max          , minQuant, maxQuant, _l(""), 1,
        _l("q_i_max"), 31,
        IDFF_enc_i_quant_factor   , &TcoSettings::i_quant_factor   , -3100, 3100, _l(""), 1,
        _l("i_quant_factor"), -80,
        IDFF_enc_i_quant_offset   , &TcoSettings::i_quant_offset   , -3100, 3100, _l(""), 1,
        _l("i_quant_offset"), 0,
        IDFF_enc_dct_algo         , &TcoSettings::dct_algo         ,    0,   4, _l(""), 1,
        _l("dct_algo"), 0,

        IDFF_enc_ff1_vratetol       , &TcoSettings::ff1_vratetol       , 1, 1024 * 10, _l(""), 1,
        _l("ff1_vratetol"), 1024,
        IDFF_enc_ff1_vqcomp         , &TcoSettings::ff1_vqcomp         , 0, 100, _l(""), 1,
        _l("ff1_vqcomp"), 50,
        IDFF_enc_ff1_vqblur1        , &TcoSettings::ff1_vqblur1        , 0, 100, _l(""), 1,
        _l("ff1_vqblur1"), 50,
        IDFF_enc_ff1_vqblur2        , &TcoSettings::ff1_vqblur2        , 0, 100, _l(""), 1,
        _l("ff1_vqblur2"), 50,
        IDFF_enc_ff1_vqdiff         , &TcoSettings::ff1_vqdiff         , 0, 31, _l(""), 1,
        _l("ff1_vqdiff"), 3,
        IDFF_enc_ff1_rc_squish      , &TcoSettings::ff1_rc_squish      , 0,  0, _l(""), 1,
        _l("ff1_rc_squish"), 0,
        IDFF_enc_ff1_rc_max_rate1000, &TcoSettings::ff1_rc_max_rate1000, 1,  1, _l(""), 1,
        _l("ff1_rc_max_rate1000"), 0,
        IDFF_enc_ff1_rc_min_rate1000, &TcoSettings::ff1_rc_min_rate1000, 1,  1, _l(""), 1,
        _l("ff1_rc_min_rate1000"), 0,
        IDFF_enc_ff1_rc_buffer_size , &TcoSettings::ff1_rc_buffer_size , 1,  1, _l(""), 1,
        _l("ff1_rc_buffer_size"), 0,

        IDFF_enc_is_lavc_nr                  , &TcoSettings::is_lavc_nr                  , 0, 0, _l(""), 1,
        _l("is_lavc_nr"), 0,
        IDFF_enc_lavc_nr                     , &TcoSettings::lavc_nr                     , 0, 10000, _l(""), 1,
        _l("lavc_nr"), 100,
        IDFF_enc_raw_fourcc, (TintVal)&TcoSettings::raw_fourcc, 0, 0, _l(""), 1,
        _l("raw_fourcc"), FOURCC_RGB2,
        0,
    };
    addOptions(iopts);
    setOnChange(IDFF_enc_forceIncsp, this, &TcoSettings::onIncspChange);
    setOnChange(IDFF_enc_incsp, this, &TcoSettings::onIncspChange);

    static const TcreateParamList1 listHuffYUVcaps(huffYUVcsps);
    setParamList(IDFF_enc_huffyuv_csp, &listHuffYUVcaps);
    static const TcreateParamList1 listHuffYUVpreds(huffYUVpreds);
    setParamList(IDFF_enc_huffyuv_pred, &listHuffYUVpreds);
    static const TcreateParamList1 listFFV1coders(ffv1coders);
    setParamList(IDFF_enc_ffv1_coder, &listFFV1coders);
    static const TcreateParamList1 listFFV1contexts(ffv1contexts);
    setParamList(IDFF_enc_ffv1_context, &listFFV1contexts);
    static const TcreateParamList2<TcspFcc> listFFV1csp(ffv1csps, &TcspFcc::name);
    setParamList(IDFF_enc_ffv1_csp, &listFFV1csp);
    static const TcreateParamList1 listMuxer(Tmuxer::muxers);
    setParamList(IDFF_enc_muxer, &listMuxer);
}

void TcoSettings::saveReg(void)
{
    TregOpRegWrite t(HKEY_CURRENT_USER, FFDSHOW_REG_PARENT _l("\\") FFDSHOWENC);
    reg_op(t);
}
void TcoSettings::loadReg(void)
{
    TregOpRegRead t(HKEY_CURRENT_USER, FFDSHOW_REG_PARENT _l("\\") FFDSHOWENC);
    reg_op(t);
    fillIncsps();
}
const DVprofile* TcoSettings::getDVprofile(unsigned int dx, unsigned int dy, PixelFormat lavc_pix_fmt) const
{
    if (dv_profile == DV_PROFILE_AUTO) {
        AVCodecContext avctx;
        avctx.width = dx;
        avctx.height = dy;
        avctx.pix_fmt = lavc_pix_fmt;
        return avpriv_dv_codec_profile2(&avctx);
    } else {
        const DVprofile *profile = dv_profiles + dv_profile;
        return (profile->width == (int)dx && profile->height == (int)dy && profile->pix_fmt == lavc_pix_fmt) ? profile : NULL;
    }
}
std::vector<const DVprofile*> TcoSettings::getDVprofile(unsigned int dx, unsigned int dy) const
{
    std::vector<const DVprofile*> profiles;
    if (dv_profile == DV_PROFILE_AUTO) {
        for (int i = 0; i < countof(dv_profiles); i++)
            if ((int)dx == dv_profiles[i].width && (int)dy == dv_profiles[i].height) {
                profiles.push_back(dv_profiles + i);
            }
    } else {
        const DVprofile *profile = dv_profiles + dv_profile;
        if (profile->width == (int)dx && profile->height == (int)dy) {
            profiles.push_back(profile);
        }
    }
    return profiles;
}
void TcoSettings::onIncspChange(int, int)
{
    fillIncsps();
}
void TcoSettings::fillIncsps(void)
{
    incsps.clear();
    if (forceIncsp)
        if (incspFourcc < 10) {
            for (int i = 0; i < FF_CSPS_NUM; i++)
                if (csp_inFOURCCmask(1ULL << i, incspFourcc)) {
                    incsps.push_back(cspInfos + i);
                }
        } else {
            incsps.push_back(csp_getInfoFcc(incspFourcc));
        }
}
