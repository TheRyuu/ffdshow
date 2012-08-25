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
#include "Tlibavcodec.h"
#include "Tdll.h"
#include "ffmpeg/libavcodec/avcodec.h"
#include "ffmpeg/libswscale/swscale.h"
#include "ffmpeg/libpostproc/postprocess_internal.h"
#include "TvideoCodecLibavcodec.h"

const char_t* Tlibavcodec::idctNames[] = {
    _l("auto"),
    _l("libmpeg2"),
    _l("simple MMX"),
    _l("Xvid"),
    _l("simple"),
    _l("integer"),
    _l("FAAN"),
    NULL
};
const char_t* Tlibavcodec::errorRecognitions[] = {
    _l("none"),
    _l("careful"),
    _l("compliant"),
    _l("aggressive"),
    _l("very aggressive"),
    NULL
};
const char_t* Tlibavcodec::errorConcealments[] = {
    _l("none"),
    _l("guess MVS"),
    _l("deblock"),
    _l("guess MVS + deblock"),
    NULL
};
const Tlibavcodec::Tdia_size Tlibavcodec::dia_sizes[] = {
    -3, _l("adaptive with size 3"),
    -2, _l("adaptive with size 2"),
    -1, _l("experimental"),
    0, _l("default"),
    1, _l("size 1 diamond"),
    2, _l("size 2 diamond"),
    3, _l("size 3 diamond"),
    4, _l("size 4 diamond"),
    5, _l("size 5 diamond"),
    6, _l("size 6 diamond"),
    0, NULL
};


//===================================== Tlibavcodec ====================================
Tlibavcodec::Tlibavcodec(const Tconfig *config): refcount(0)
{
    dll = new Tdll(_l("ffmpeg.dll"), config);

    dll->loadFunction(avcodec_register_all, "avcodec_register_all");
    dll->loadFunction(avcodec_find_decoder, "avcodec_find_decoder");
    dll->loadFunction(avcodec_open0, "avcodec_open2");
    dll->loadFunction(avcodec_alloc_context0, "avcodec_alloc_context3");
    dll->loadFunction(avcodec_alloc_frame, "avcodec_alloc_frame");
    dll->loadFunction(avcodec_decode_video2, "avcodec_decode_video2");
    dll->loadFunction(avcodec_flush_buffers, "avcodec_flush_buffers");
    dll->loadFunction(avcodec_close0, "avcodec_close");
    dll->loadFunction(av_log_set_callback, "av_log_set_callback");
    dll->loadFunction(av_log_get_callback, "av_log_get_callback");
    dll->loadFunction(av_log_get_level, "av_log_get_level");
    dll->loadFunction(av_log_set_level, "av_log_set_level");
    dll->loadFunction(av_set_cpu_flags_mask, "av_set_cpu_flags_mask");
    dll->loadFunction(av_mallocz, "av_mallocz");
    dll->loadFunction(av_free, "av_free");
    dll->loadFunction(avcodec_default_get_buffer, "avcodec_default_get_buffer");
    dll->loadFunction(avcodec_default_release_buffer, "avcodec_default_release_buffer");
    dll->loadFunction(avcodec_default_reget_buffer, "avcodec_default_reget_buffer");
    dll->loadFunction(avcodec_get_current_idct, "avcodec_get_current_idct");
    dll->loadFunction(avcodec_get_encoder_info, "avcodec_get_encoder_info");
    dll->loadFunction(av_init_packet, "av_init_packet");
    dll->loadFunction(av_packet_new_side_data, "av_packet_new_side_data");
    dll->loadFunction(avcodec_h264_search_recovery_point, "avcodec_h264_search_recovery_point");

    dll->loadFunction(avcodec_decode_audio3, "avcodec_decode_audio3");

    dll->loadFunction(avcodec_find_encoder, "avcodec_find_encoder");
    dll->loadFunction(avcodec_encode_video, "avcodec_encode_video");
    dll->loadFunction(avcodec_encode_audio, "avcodec_encode_audio");

    dll->loadFunction(av_parser_init, "av_parser_init");
    dll->loadFunction(av_parser_parse2, "av_parser_parse2");
    dll->loadFunction(av_parser_close, "av_parser_close");

    //libswscale methods
    dll->loadFunction(sws_getCachedContext, "sws_getCachedContext");
    dll->loadFunction(sws_freeContext, "sws_freeContext");
    dll->loadFunction(sws_getDefaultFilter, "sws_getDefaultFilter");
    dll->loadFunction(sws_freeFilter, "sws_freeFilter");
    dll->loadFunction(libswscale_sws_scale, "sws_scale");

    dll->loadFunction(GetCPUCount, "GetCPUCount");
    dll->loadFunction(sws_getConstVec, "sws_getConstVec");
    dll->loadFunction(sws_getGaussianVec, "sws_getGaussianVec");
    dll->loadFunction(sws_normalizeVec, "sws_normalizeVec");
    dll->loadFunction(sws_freeVec, "sws_freeVec");
    dll->loadFunction(sws_setColorspaceDetails, "sws_setColorspaceDetails");
    dll->loadFunction(sws_getCoefficients, "sws_getCoefficients");

    //libpostproc methods
    dll->loadFunction(pp_postprocess, "pp_postprocess");
    dll->loadFunction(pp_get_context, "pp_get_context");
    dll->loadFunction(pp_free_context, "pp_free_context");
    dll->loadFunction(ff_simple_idct_mmx, "ff_simple_idct_mmx");

    //DXVA methods
    dll->loadFunction(av_h264_decode_frame, "av_h264_decode_frame");
    dll->loadFunction(av_vc1_decode_frame, "av_vc1_decode_frame");

    dll->loadFunction(FFH264CheckCompatibility, "FFH264CheckCompatibility");
    dll->loadFunction(FFH264DecodeBuffer, "FFH264DecodeBuffer");
    dll->loadFunction(FFH264BuildPicParams, "FFH264BuildPicParams");
    dll->loadFunction(FFH264SetCurrentPicture, "FFH264SetCurrentPicture");
    dll->loadFunction(FFH264UpdateRefFramesList, "FFH264UpdateRefFramesList");
    dll->loadFunction(FFH264IsRefFrameInUse, "FFH264IsRefFrameInUse");
    dll->loadFunction(FF264UpdateRefFrameSliceLong, "FF264UpdateRefFrameSliceLong");
    dll->loadFunction(FFH264SetDxvaSliceLong, "FFH264SetDxvaSliceLong");

    dll->loadFunction(FFVC1UpdatePictureParam, "FFVC1UpdatePictureParam");
    dll->loadFunction(FFIsSkipped, "FFIsSkipped");

    dll->loadFunction(GetFFMpegPictureType, "GetFFMpegPictureType");
    dll->loadFunction(FFGetMBNumber, "FFGetMBNumber");

    //yadif methods
    dll->loadFunction(yadif_init, "yadif_init");
    dll->loadFunction(yadif_uninit, "yadif_uninit");
    dll->loadFunction(yadif_filter, "yadif_filter");

    //gradfun
    dll->loadFunction(gradfunInit, "gradfunInit");
    dll->loadFunction(gradfunFilter, "gradfunFilter");

    ok = dll->ok;

    if (ok) {
        avcodec_register_all();
        av_log_set_callback(avlog);
    }
}
Tlibavcodec::~Tlibavcodec()
{
    delete dll;
}

//Used by libswscale start
void Tlibavcodec::swsInitParams(SwsParams *params, int resizeMethod)
{
    memset(params, 0, sizeof(*params));
    params->methodLuma.method = params->methodChroma.method = resizeMethod;
    params->methodLuma.param[0] = params->methodChroma.param[0] = SWS_PARAM_DEFAULT;
    params->methodLuma.param[1] = params->methodChroma.param[1] = SWS_PARAM_DEFAULT;

}
void Tlibavcodec::swsInitParams(SwsParams *params, int resizeMethod, int flags)
{
    swsInitParams(params, resizeMethod);
    params->methodLuma.method |= flags;
    params->methodChroma.method |= flags;
}
//Used by libswscale end

//Used by libpostproc start
int Tlibavcodec::ppCpuCaps(uint64_t csp)
{
    int cpu = 0;
    if (Tconfig::cpu_flags & FF_CPU_MMX) { cpu |= PP_CPU_CAPS_MMX; }
    if (Tconfig::cpu_flags & FF_CPU_MMXEXT) { cpu |= PP_CPU_CAPS_MMX2; }
    if (Tconfig::cpu_flags & FF_CPU_3DNOW) { cpu |= PP_CPU_CAPS_3DNOW; }

    switch (csp & FF_CSPS_MASK) {
        case 0:
        case FF_CSP_420P:
            cpu |= PP_FORMAT_420;
            break;
        case FF_CSP_422P:
            cpu |= PP_FORMAT_422;
            break;
        case FF_CSP_411P:
            cpu |= PP_FORMAT_411;
            break;
        case FF_CSP_444P:
            cpu |= PP_FORMAT_444;
            break;
            //case FF_CSP_410P:cpu|=PP_FORMAT_410;break;
    }

    return cpu;
}

void Tlibavcodec::pp_mode_defaults(PPMode &ppMode)
{
    ppMode.lumMode = 0;
    ppMode.chromMode = 0;
    ppMode.maxTmpNoise[0] = 700;
    ppMode.maxTmpNoise[1] = 1500;
    ppMode.maxTmpNoise[2] = 3000;
    ppMode.maxAllowedY = 234;
    ppMode.minAllowedY = 16;
    ppMode.baseDcDiff = 256 / 8;
    ppMode.flatnessThreshold = 56 - 16 - 1;
    ppMode.maxClippedThreshold = 0.01f;
    ppMode.error = 0;
    ppMode.forcedQuant = 0;
}

int Tlibavcodec::getPPmode(const TpostprocSettings *cfg, int currentq)
{
    int result = 0;
    if (!cfg->isCustom) {
        int ppqual = cfg->autoq ? currentq : cfg->qual;
        if (ppqual < 0) { ppqual = 0; }
        if (ppqual > PP_QUALITY_MAX) { ppqual = PP_QUALITY_MAX; }
        static const int ppPresets[1 + PP_QUALITY_MAX] = {
            0,
            LUM_H_DEBLOCK,
            LUM_H_DEBLOCK | LUM_V_DEBLOCK,
            LUM_H_DEBLOCK | LUM_V_DEBLOCK | CHROM_H_DEBLOCK,
            LUM_H_DEBLOCK | LUM_V_DEBLOCK | CHROM_H_DEBLOCK | CHROM_V_DEBLOCK,
            LUM_H_DEBLOCK | LUM_V_DEBLOCK | CHROM_H_DEBLOCK | CHROM_V_DEBLOCK | LUM_DERING,
            LUM_H_DEBLOCK | LUM_V_DEBLOCK | CHROM_H_DEBLOCK | CHROM_V_DEBLOCK | LUM_DERING | CHROM_DERING
        };
        result = ppPresets[ppqual];
    } else {
        result = cfg->custom;
    }
    if (cfg->levelFixLum) { result |= LUM_LEVEL_FIX; }
    //if (cfg->levelFixChrom) result|=CHROM_LEVEL_FIX;
    return result;
}
//Used by libpostproc end

AVCodecContext* Tlibavcodec::avcodec_alloc_context(AVCodec *codec, TlibavcodecExt *ext)
{
    AVCodecContext *ctx = avcodec_alloc_context0(codec);
    if (ext) {
        ext->connectTo(ctx, this);
    }
    ctx->scenechange_factor = 1;
    return ctx;
}
int Tlibavcodec::avcodec_open(AVCodecContext *avctx, AVCodec *codec)
{
    CAutoLock l(&csOpenClose);
    return avcodec_open0(avctx, codec, NULL);
}
int Tlibavcodec::avcodec_close(AVCodecContext *avctx)
{
    CAutoLock l(&csOpenClose);
    return avcodec_close0(avctx);
}

bool Tlibavcodec::getVersion(const Tconfig *config, ffstring &vers, ffstring &license)
{
    Tdll *dl = new Tdll(_l("ffmpeg.dll"), config);

    void (*av_getVersion)(char **version, char **build, char **datetime, const char* *license);
    dl->loadFunction(av_getVersion, "getVersion");
    bool res;
    if (av_getVersion) {
        res = true;
        char *version, *build, *datetime;
        const char *lic;
        av_getVersion(&version, &build, &datetime, &lic);
        vers = (const char_t*)text<char_t>(version) + ffstring(_l(" (")) + (const char_t*)text<char_t>(datetime) + _l(")");
        license = text<char_t>(lic);
    } else {
        res = false;
        vers.clear();
        license.clear();
    }
    delete dl;
    return res;
}
bool Tlibavcodec::check(const Tconfig *config)
{
    return Tdll::check(_l("ffmpeg.dll"), config);
}

void Tlibavcodec::avlog(AVCodecContext *avctx, int level, const char *fmt, va_list valist)
{
    DPRINTFvaA(fmt, valist);
}

void Tlibavcodec::avlogMsgBox(AVCodecContext *avctx, int level, const char *fmt, va_list valist)
{
    if (level > AV_LOG_ERROR) {
        DPRINTFvaA(fmt, valist);
    } else {
        char buf[1024];
        int len = _vsnprintf(buf, 1023, fmt, valist);
        if (len > 0) {
            buf[len] = '\0';
            MessageBoxA(NULL, buf, "ffdshow libavcodec encoder error", MB_ICONERROR | MB_OK);
        }
    }
}

int Tlibavcodec::sws_scale(struct SwsContext *context, const uint8_t* const srcSlice[], const stride_t srcStride[],
                           int srcSliceY, int srcSliceH, uint8_t* const dst[], const stride_t dstStride[])
{
    int srcStride1[4] = {int(srcStride[0]), int(srcStride[1]), int(srcStride[2]), int(srcStride[3])};
    int dstStride1[4] = {int(dstStride[0]), int(dstStride[1]), int(dstStride[2]), int(dstStride[3])};
    return libswscale_sws_scale(context, srcSlice, srcStride1, srcSliceY, srcSliceH, dst, dstStride1);
}

//=================================== TlibavcodecExt ===================================
void TlibavcodecExt::connectTo(AVCodecContext *ctx, Tlibavcodec *libavcodec)
{
    ctx->opaque = this;
    ctx->get_buffer = get_buffer;
    default_get_buffer = libavcodec->avcodec_default_get_buffer;
    ctx->reget_buffer = reget_buffer;
    default_reget_buffer = libavcodec->avcodec_default_reget_buffer;
    ctx->release_buffer = release_buffer;
    default_release_buffer = libavcodec->avcodec_default_release_buffer;
    ctx->handle_user_data = handle_user_data0;
}
int TlibavcodecExt::get_buffer(AVCodecContext *c, AVFrame *pic)
{
    int ret = c->opaque->default_get_buffer(c, pic);
    if (ret == 0) {
        c->opaque->onGetBuffer(pic);
    }
    return ret;
}
int TlibavcodecExt::reget_buffer(AVCodecContext *c, AVFrame *pic)
{
    int ret = c->opaque->default_reget_buffer(c, pic);
    if (ret == 0) {
        c->opaque->onRegetBuffer(pic);
    }
    return ret;
}
void TlibavcodecExt::release_buffer(AVCodecContext *c, AVFrame *pic)
{
    c->opaque->default_release_buffer(c, pic);
    c->opaque->onReleaseBuffer(pic);
}
void TlibavcodecExt::handle_user_data0(AVCodecContext *c, const uint8_t *buf, int buf_len)
{
    c->opaque->handle_user_data(buf, buf_len);
}
