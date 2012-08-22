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
/*
 * Th264RandomAccess, isReallyMPEG2
 * Copyright (c) 2008-2012 Haruhiko Yamagata
 */

#include "stdafx.h"

#include "ffmpeg/libavcodec/avcodec.h"
#include "ffmpeg/Tlibavcodec.h"
#include "ffmpeg/libavutil/intreadwrite.h"
#include "IffdshowBase.h"
#include "IffdshowDecVideo.h"
#include "IffdshowEnc.h"
#include "TvideoCodecLibavcodec.h"
#include "TglobalSettings.h"
#include "ffdshow_mediaguids.h"
#include "TcodecSettings.h"
#include "rational.h"
#include "qtpalette.h"
#include "line.h"
#include "simd.h"
#include "dsutil.h"
#include "cc_decoder.h"
#include "TffdshowVideoInputPin.h"

TvideoCodecLibavcodec::TvideoCodecLibavcodec(IffdshowBase *Ideci, IdecVideoSink *IsinkD):
    Tcodec(Ideci), TcodecDec(Ideci, IsinkD),
    TvideoCodec(Ideci),
    TvideoCodecDec(Ideci, IsinkD),
    TvideoCodecEnc(Ideci, NULL),
    h264RandomAccess(this),
    bReorderBFrame(true)
{
    create();
}
TvideoCodecLibavcodec::TvideoCodecLibavcodec(IffdshowBase *Ideci, IencVideoSink *IsinkE):
    Tcodec(Ideci), TcodecDec(Ideci, NULL),
    TvideoCodec(Ideci),
    TvideoCodecDec(Ideci, NULL),
    TvideoCodecEnc(Ideci, IsinkE),
    h264RandomAccess(this)
{
    create();
    if (ok) {
        encoders.push_back(new Tencoder(_l("MJPEG"), AV_CODEC_ID_MJPEG));
        encoders.push_back(new Tencoder(_l("HuffYUV (FFmpeg variant)"), AV_CODEC_ID_FFVHUFF));
        encoders.push_back(new Tencoder(_l("FFV1"), AV_CODEC_ID_FFV1));
        encoders.push_back(new Tencoder(_l("DV"), AV_CODEC_ID_DVVIDEO));
    }
}
void TvideoCodecLibavcodec::create(void)
{
    ownmatrices = false;
    deci->getLibavcodec(&libavcodec);
    ok = libavcodec ? libavcodec->ok : false;
    avctx = NULL;
    avcodec = NULL;
    frame = NULL;
    quantBytes = 1;
    statsfile = NULL;
    threadcount = 0;
    codecinited = false;
    extradata = NULL;
    theorart = false;
    ffbuf = NULL;
    ffbuflen = 0;
    codecName[0] = '\0';
    ccDecoder = NULL;
    autoSkipingLoopFilter = false;
    inPosB = 1;
    firstSeek = true;
    mpeg2_new_sequence = true;
    parser = NULL;
}

TvideoCodecLibavcodec::~TvideoCodecLibavcodec()
{
    end();
    if (libavcodec) {
        libavcodec->Release();
    }
    if (extradata) {
        delete extradata;
    }
    if (ffbuf) {
        free(ffbuf);
    }
    if (ccDecoder) {
        delete ccDecoder;
    }
}
void TvideoCodecLibavcodec::end(void)
{
    if (statsfile) {
        fflush(statsfile);
        fclose(statsfile);
        statsfile = NULL;
    }
    if (parser) {
        libavcodec->av_parser_close(parser);
        parser = NULL;
    }
    if (avctx) {
        if (ownmatrices) {
            if (avctx->intra_matrix) {
                free(avctx->intra_matrix);
            }
            if (avctx->inter_matrix) {
                free(avctx->inter_matrix);
            }
            ownmatrices = false;
        }
        if (avctx->slice_offset) {
            free(avctx->slice_offset);
        }
        if (codecinited) {
            libavcodec->avcodec_close(avctx);
        }
        codecinited = false;
        libavcodec->av_free(avctx);
        avctx = NULL;
        libavcodec->av_free(frame);
        frame = NULL;
    }
    avcodec = NULL;
}

//----------------------------- decompression -----------------------------
bool TvideoCodecLibavcodec::beginDecompress(TffPictBase &pict, FOURCC fcc, const CMediaType &mt, int sourceFlags)
{
    palette_size = 0;
    prior_out_rtStart = REFTIME_INVALID;
    prior_out_rtStop = 0;
    rtStart = rtStop = REFTIME_INVALID;
    prior_in_rtStart = prior_in_rtStop = REFTIME_INVALID;
    mpeg2_in_doubt = codecId == AV_CODEC_ID_MPEG2VIDEO;

    int using_dxva = 0;

    int numthreads = deci->getParam2(IDFF_numLAVCdecThreads);
    int thread_type = 0;
    if (numthreads > 1 && sup_threads_dec_frame(codecId)) {
        thread_type = FF_THREAD_FRAME;
    } else if (numthreads > 1 && sup_threads_dec_slice(codecId)) {
        thread_type = FF_THREAD_SLICE;
    }

    if (numthreads > 1 && thread_type != 0) {
        threadcount = numthreads;
#if 0
        /* hack to reduce sync issues for H.264 in AVI */
        if (threadcount > 2 && codecId == CODEC_ID_H264) {
            ffstring sourceExt;
            extractfileext(deci->getSourceName(), sourceExt);
            sourceExt.ConvertToLowerCase();
            if (sourceExt == L"avi") {
                threadcount = 2;
            }
        }
#endif
    } else {
        threadcount = 1;
    }

    if (codecId == CODEC_ID_H264_DXVA) {
        codecId = AV_CODEC_ID_H264;
        using_dxva = 1;
    } else if (codecId == CODEC_ID_VC1_DXVA) {
        codecId = AV_CODEC_ID_VC1;
        using_dxva = 1;
    }

    avcodec = libavcodec->avcodec_find_decoder(codecId);
    if (!avcodec) {
        return false;
    }
    avctx = libavcodec->avcodec_alloc_context(avcodec, this);
    avctx->thread_type = thread_type;
    avctx->thread_count = threadcount;
    avctx->h264_using_dxva = using_dxva;
    if (codecId == AV_CODEC_ID_H264) {
        // If we do not set this, first B-frames before the IDR pictures are dropped.
        avctx->has_b_frames = 1;
    }

    frame = libavcodec->avcodec_alloc_frame();
    avctx->width = pict.rectFull.dx;
    avctx->height = pict.rectFull.dy;
    intra_matrix = avctx->intra_matrix = (uint16_t*)calloc(sizeof(uint16_t), 64);
    inter_matrix = avctx->inter_matrix = (uint16_t*)calloc(sizeof(uint16_t), 64);
    ownmatrices = true;


    // Fix for new Haali custom media type and fourcc. ffmpeg does not understand it, we have to change it to FOURCC_AVC1
    if (fcc == FOURCC_CCV1) {
        fcc = FOURCC_AVC1;
    }

    avctx->codec_tag = fcc;
    avctx->workaround_bugs = deci->getParam2(IDFF_workaroundBugs);
#if 0
    avctx->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
    avctx->err_recognition   = AV_EF_CRCCHECK | AV_EF_BITSTREAM | AV_EF_BUFFER | AV_EF_COMPLIANT | AV_EF_AGGRESSIVE;
#endif
    if (codecId == AV_CODEC_ID_MJPEG) {
        avctx->flags |= CODEC_FLAG_TRUNCATED;
    }
    if (mpeg12_codec(codecId) && deci->getParam2(IDFF_fastMpeg2)) {
        avctx->flags2 = CODEC_FLAG2_FAST;
    }
    if (codecId == AV_CODEC_ID_H264)
        if (int skip = deci->getParam2(IDFF_fastH264)) {
            avctx->skip_loop_filter = skip & 2 ? AVDISCARD_ALL : AVDISCARD_NONREF;
        }
    initialSkipLoopFilter = avctx->skip_loop_filter;

    avctx->debug_mv = !using_dxva; //(deci->getParam2(IDFF_isVis) & deci->getParam2(IDFF_visMV));

    avctx->idct_algo = limit(deci->getParam2(IDFF_idct), 0, 6);
    if (extradata) {
        delete extradata;
    }
    extradata = new Textradata(mt, FF_INPUT_BUFFER_PADDING_SIZE);
    if (extradata->size > 0 && (codecId != AV_CODEC_ID_H264 || fcc == FOURCC_AVC1)) {
        avctx->extradata_size = (int)extradata->size;
        avctx->extradata = extradata->data;
        sendextradata = mpeg12_codec(codecId);
        if ((fcc == FOURCC_AVC1) && mt.formattype == FORMAT_MPEG2Video) {
            const MPEG2VIDEOINFO *mpeg2info = (const MPEG2VIDEOINFO*)mt.pbFormat;
            avctx->nal_length_size = mpeg2info->dwFlags;
            bReorderBFrame = false;
        } else if (fcc == FOURCC_THEO) {
            if (mt.formattype == FORMAT_RLTheora) {
                theorart = true;
                const uint8_t *src = (const uint8_t*)avctx->extradata;
                size_t dstsize = extradata->size;
                uint8_t *dst = (uint8_t*)malloc(dstsize), *dst0 = dst;
                dst[1] = src[0 + 0 * sizeof(DWORD)];
                dst[0] = src[1 + 0 * sizeof(DWORD)];
                DWORD len0 = *(DWORD*)&src[0 * sizeof(DWORD)];
                memcpy(dst + 2, src + 16, len0);
                dst += 2 + len0;

                dst[1] = src[0 + 1 * sizeof(DWORD)];
                dst[0] = src[1 + 1 * sizeof(DWORD)];
                DWORD len1 = *(DWORD*)&src[1 * sizeof(DWORD)];
                memcpy(dst + 2, src + 16 + len0, len1);
                dst += 2 + len1;

                dst[1] = src[0 + 2 * sizeof(DWORD)];
                dst[0] = src[1 + 2 * sizeof(DWORD)];
                DWORD len2 = *(DWORD*)&src[2 * sizeof(DWORD)];
                memcpy(dst + 2, src + 16 + len0 + len1, len2);

                extradata->clear();
                extradata->set(dst0, dstsize, 0, true);
                free(dst0);
                avctx->extradata = extradata->data;
                avctx->extradata_size = (int)extradata->size;
            }
            if (extradata->size > 2 && extradata->data[2] == 0) {
                const uint8_t *src = (const uint8_t*)avctx->extradata;
                size_t dstsize = extradata->size;
                uint8_t *dst = (uint8_t*)malloc(dstsize);
                long len = *(long*)src;
                dst[1] = ((uint8_t*)&len)[0];
                dst[0] = ((uint8_t*)&len)[1];
                memcpy(dst + 2, src + 4, len);
                *((int16_t*)(dst + 2 + len)) = int16_t(extradata->size - 4 - len);
                memcpy(dst + 2 + len + 2, src + 4 + len, extradata->size - 4 - len);
                extradata->clear();
                extradata->set(dst, dstsize, 0, true);
                free(dst);
                avctx->extradata = extradata->data;
                avctx->extradata_size = (int)extradata->size;
            }
        }
    } else if (codecId == AV_CODEC_ID_VP6 || codecId == AV_CODEC_ID_VP6A || codecId == AV_CODEC_ID_VP6F) {
        // Copyright (C) 2011 Hendrik Leppkes
        // for FLV cropping
        int cropWidth = 0, cropHeight = 0;
        if (mt.formattype == FORMAT_VideoInfo || mt.formattype == FORMAT_MPEGVideo) {
            VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)mt.Format();
            if (vih->rcTarget.right != 0 && vih->rcTarget.bottom != 0) {
                cropWidth  = vih->bmiHeader.biWidth - vih->rcTarget.right;
                cropHeight = vih->bmiHeader.biHeight - vih->rcTarget.bottom;
            }
        } else if (mt.formattype == FORMAT_VideoInfo2 || mt.formattype == FORMAT_MPEG2Video) {
            VIDEOINFOHEADER2 *vih2 = (VIDEOINFOHEADER2 *)mt.Format();
            if (vih2->rcTarget.right != 0 && vih2->rcTarget.bottom != 0) {
                cropWidth  = vih2->bmiHeader.biWidth - vih2->rcTarget.right;
                cropHeight = vih2->bmiHeader.biHeight - vih2->rcTarget.bottom;
            }
        }
        if (cropWidth < 0) {
            cropWidth = 0;
        }
        if (cropHeight < 0) {
            cropHeight = 0;
        }
        if (cropWidth > 0 || cropHeight > 0) {
            delete extradata;
            extradata = new Textradata(cropWidth, cropHeight, FF_INPUT_BUFFER_PADDING_SIZE);
            avctx->extradata = extradata->data;
            avctx->extradata_size = (int)extradata->size;
        } else {
            sendextradata = false;
        }
    } else {
        sendextradata = false;
    }

    if (fcc == FOURCC_RLE4 || fcc == FOURCC_RLE8 || fcc == FOURCC_CSCD || sup_palette(codecId)) {
        BITMAPINFOHEADER bih;
        ExtractBIH(mt, &bih);
        avctx->bits_per_coded_sample = bih.biBitCount;
        if (avctx->bits_per_coded_sample <= 8 || codecId == AV_CODEC_ID_PNG) {
            const void *pal;
            if (!extradata->data)
                switch (avctx->bits_per_coded_sample) {
                    case 2:
                        pal = qt_default_palette_4;
                        palette_size = 4 * 4;
                        break;
                    case 4:
                        pal = qt_default_palette_16;
                        palette_size = 16 * 4;
                        break;
                    default:
                    case 8:
                        pal = qt_default_palette_256;
                        palette_size = 256 * 4;
                        break;
                }
            else {
                palette_size = std::min(AVPALETTE_SIZE, (int)extradata->size);
                pal = extradata->data;
            }
            memcpy(palette, pal, palette_size);
        }
    } else if (extradata->data && (codecId == AV_CODEC_ID_RV10 || codecId == AV_CODEC_ID_RV20)) {
#pragma pack(push, 1)
        struct rvinfo {
            /*     DWORD dwSize, fcc1, fcc2;
                   WORD w, h, bpp;
                   DWORD unk1, fps, type1, type2;*/
            DWORD type1, type2;
            BYTE w2, h2, w3, h3;
        } __attribute__((packed));
#pragma pack(pop)

        const uint8_t *src = (const uint8_t*)avctx->extradata;
        size_t dstsize = extradata->size - 26;
        uint8_t *dst = (uint8_t*)malloc(dstsize);
        memcpy(dst, src + 26, dstsize);
        extradata->clear();
        extradata->set(dst, dstsize, 0, true);
        free(dst);
        avctx->extradata = extradata->data;
        avctx->extradata_size = (int)extradata->size;

        rvinfo *info = (rvinfo*)extradata->data;
        avctx->sub_id = info->type2;
        bswap(avctx->sub_id);
    }

    if (pict.csp == FF_CSP_UNSUPPORTED) {
        return false;
    }

    if (libavcodec->avcodec_open(avctx, avcodec) < 0) {
        return false;
    }

    ffstring sourceExt;
    extractfileext(deci->getSourceName(), sourceExt);
    sourceExt.ConvertToLowerCase();

    if (mpeg12_codec(codecId)
            || (codecId == AV_CODEC_ID_H264
                && !(mt.subtype == MEDIASUBTYPE_AVC1 || mt.subtype == MEDIASUBTYPE_avc1 || mt.subtype == MEDIASUBTYPE_CCV1))) {
        // avi and ogm files do not have access unit delimiter
        // Neuview Source is an AVI splitter that does not implement IFileSourceFilter.
        //if ( sourceExt != L"avi"
        //        && sourceExt != L"ogg"
        //        && sourceExt != L"ogm"
        //        && sourceExt != L"ogv"
        //        && sourceExt != L"mkv" // old MKVtoolnix use MEDIASUBTYPE_H264 and does not add access unit delimiter.
        //        && !(deci->getParam2(IDFF_filterMode) & IDFF_FILTERMODE_VFW) // VFW: avi files
        //        && !(sourceExt == L"" && connectedSplitter == TffdshowVideoInputPin::NeuviewSource)) {
        parser = libavcodec->av_parser_init(codecId);
        //}
    }


    if (avctx->pix_fmt != PIX_FMT_NONE) { pict.csp = csp_lavc2ffdshow(avctx->pix_fmt); }
    if (pict.csp == FF_CSP_NULL) { pict.csp = FF_CSP_420P; }
    if (avctx->sample_aspect_ratio.num && avctx->sample_aspect_ratio.den) {
        pict.setSar(avctx->sample_aspect_ratio);
    }

    // check color space of FRAPS (in AVI). Works only if the file has proper chunk.
    if (codecId == AV_CODEC_ID_FRAPS && avctx->pix_fmt == PIX_FMT_NONE) {
        BITMAPINFOHEADER bih;
        ExtractBIH(mt, &bih);
        if (bih.biBitCount == 24) {
            pict.csp = FF_CSP_RGB24;
        }
    }

    containerSar = pict.rectFull.sar;
    dont_use_rtStop_from_upper_stream = (sourceFlags & SOURCE_REORDER && sourceExt != L"avi" && avctx->codec_tag != FOURCC_THEO) || avctx->codec_tag == FOURCC_MPG1 || avctx->codec_tag == FOURCC_MPG2;
    avgTimePerFrame = -1;
    codecinited = true;
    wasKey = false;
    segmentTimeStart = 0;
    avctx->isDVD = isdvdproc;
    return true;
}

HRESULT TvideoCodecLibavcodec::BeginFlush()
{
    onSeek(0);
    return S_OK;
}

void TvideoCodecLibavcodec::onGetBuffer(AVFrame *pic)
{
}

void TvideoCodecLibavcodec::handle_user_data(const uint8_t *buf, int buf_len)
{
    TffdshowVideoInputPin::TrateAndFlush *rateInfo = (TffdshowVideoInputPin::TrateAndFlush*)deciV->getRateInfo();
    if (rateInfo->rate.Rate == 10000
            && buf_len > 4
            && *(DWORD*)buf == 0xf8014343) {
        if (!ccDecoder) {
            ccDecoder = new TccDecoder(deciV);
        }
        ccDecoder->decode(buf + 2, buf_len - 2);
    }
}

HRESULT TvideoCodecLibavcodec::flushDec(void)
{
    HRESULT hr;
    do {
        hr = decompress(NULL, 0, NULL);
    } while (got_picture && hr == S_OK);
    return hr;
}

REFERENCE_TIME TvideoCodecLibavcodec::getDuration()
{
    REFERENCE_TIME duration = REF_SECOND_MULT / 100;
    if (avctx && avctx->time_base.num && avctx->time_base.den) {
        duration = REF_SECOND_MULT * avctx->time_base.num / avctx->time_base.den;
        if (codecId == AV_CODEC_ID_H264) {
            duration *= 2;
        }
    }
    if (duration == 0) {
        return REF_SECOND_MULT / 100;
    }
    return duration;
}

// Let's enjoy some hack. Some H.264 byte stream format might have slipped into ffdshow as MPEG-2.
// In that case, they are labeled MPEG-2 because they are on MPEG-2 TS. Then the streams always have access unit delimiters.
// For H.264 streams, MPEG-2 parser never parse a frame.
// And this function detects H.264's access unit delimiters.
// So if we call this function until MPEG-2 parser can parse a frame, we always successfully find H.264 without breaking MPEG-2.
// return 0:H.264, 1:uncertain, 2:MPEG-2
int TvideoCodecLibavcodec::isReallyMPEG2(const unsigned char *src, size_t srcLen)
{
    if (srcLen < 10) {
        return 1;
    }
    for (size_t i = 0 ; i < srcLen - 7 ; i++) {
        if (src[i] == 0 && src[i + 1] == 0 && src[i + 2] == 1) {
            int h264nal = src[i + 3] & 0x1f;
            if (h264nal == 0 || (h264nal > 13 && h264nal != 19)) {
                return 2;
            }
            // H.264 access unit delimiter have only one byte rbsp (i+4).
            if (src[i + 3] == 9 && src[i + 5] == 0 && src[i + 6] == 0) {
                i += 7;
                for (; i < srcLen - 1 ; i++) {
                    if (src[i]) {
                        break;
                    }
                }
                if (src[i] == 1) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

HRESULT TvideoCodecLibavcodec::decompress(const unsigned char *src, size_t srcLen0, IMediaSample *pIn)
{
    HRESULT hr = S_OK;
    TffdshowVideoInputPin::TrateAndFlush *rateInfo = (TffdshowVideoInputPin::TrateAndFlush*)deciV->getRateInfo();

    if (mpeg2_in_doubt) {
        // If 5 byte access unit delimiter plus next start code is splitted into chunks, it would be slipped by.
        // Don't care too much, let's wait for the next one.
        int isMPEG2 = isReallyMPEG2(src, srcLen0);
        if (codecId == AV_CODEC_ID_MPEG2VIDEO && !isMPEG2) {
            DPRINTF(L"Re-initializing libavcodec. Changing from CODEC_ID_MPEG2VIDEO to CODEC_ID_H264.");
            end();
            codecId = AV_CODEC_ID_H264;
            FOURCC fcc = FOURCC_H264;
            TffPictBase pict;
            CMediaType mt(&MEDIATYPE_Video);
            mt.subtype = MEDIASUBTYPE_H264;
            mt.bFixedSizeSamples = false;
            mt.bTemporalCompression = true;
            mt.lSampleSize = 0;
            mt.formattype = FORMAT_None;
            mt.pUnk = NULL;
            mt.cbFormat = 0;
            mt.pbFormat = NULL;
            beginDecompress(pict, fcc, mt, 0);
        }
        if (isMPEG2 == 2) {
            // We are sure it's MPEG-2.
            mpeg2_in_doubt = false;
        }
    }

    bool isSyncPoint = pIn && pIn->IsSyncPoint() == S_OK;
    if (codecId == AV_CODEC_ID_FFV1) {
        // libavcodec can crash or loop infinitely when first frame after seeking is not keyframe
        if (!wasKey)
            if (isSyncPoint) {
                wasKey = true;
            } else {
                return S_OK;
            }
    }

    if (pIn && pIn->IsDiscontinuity() == S_OK) {
        rateInfo->isDiscontinuity = true;
    }

    unsigned int skip = 0;

    if (src && (codecId == AV_CODEC_ID_RV10 || codecId == AV_CODEC_ID_RV20) && avctx->sub_id) {
        avctx->slice_count = src[0] + 1;
        if (!avctx->slice_offset) {
            avctx->slice_offset = (int*)malloc(sizeof(int) * 1000);
        }
        for (int i = 0; i < avctx->slice_count; i++) {
            avctx->slice_offset[i] = ((DWORD*)(src + 1))[2 * i + 1];
        }
        skip = 1 + 2 * sizeof(DWORD) * avctx->slice_count;
    } else if (src && theorart) {
        struct _TheoraPacket {
            long  bytes;
            long  b_o_s;
            long  e_o_s;
            int64_t  granulepos;
            int64_t  packetno;
        };
        _TheoraPacket *packet = (_TheoraPacket*)src;
        if (packet->bytes == 0 || src[sizeof(_TheoraPacket)] & 0x80) {
            return S_OK;
        }
        skip += sizeof(_TheoraPacket);
        avctx->granulepos = packet->granulepos;
    }

    src += skip;
    int size = int(srcLen0 - skip);

    if (pIn) {
        HRESULT hr_GetTime = pIn->GetTime(&rtStart, &rtStop);
#if 0
        REFERENCE_TIME_to_hhmmssmmm start(rtStart);
        REFERENCE_TIME_to_hhmmssmmm stop(rtStop);
        ffstring error_msg = L"Unknown";
        if (hr_GetTime == S_OK) {
            error_msg = L"S_OK";
        } else if (hr_GetTime == VFW_S_NO_STOP_TIME) {
            error_msg = L"VFW_S_NO_STOP_TIME";
        } else if (hr_GetTime == VFW_E_SAMPLE_TIME_NOT_SET) {
            error_msg = L"VFW_E_SAMPLE_TIME_NOT_SET";
        }
        DPRINTF(L"IMediaSample::GetTime hr=%s rtStart=%s rtStop=%s", error_msg.c_str(), start.get_str(), stop.get_str());
#endif
    }

    b[inPosB].rtStart = rtStart;
    b[inPosB].rtStop = rtStop;
    b[inPosB].srcSize = size;
    inPosB++;
    if (inPosB >= countof(b)) {
        inPosB = 0;
    }

    if (codecId == AV_CODEC_ID_H264) {
        if (autoSkipingLoopFilter) {
            if (deciV->getLate() <= 0) {
                avctx->skip_loop_filter = initialSkipLoopFilter;
                //avctx->skip_frame = AVDISCARD_NONE;
                autoSkipingLoopFilter = false;
            }
        } else {
            if (deciV->shouldSkipH264loopFilter()) {
                avctx->skip_loop_filter = AVDISCARD_ALL;
                //avctx->skip_frame = AVDISCARD_NONREF;
                autoSkipingLoopFilter = true;
            }
        }
    }

    AVPacket avpkt;
    libavcodec->av_init_packet(&avpkt);
    if (palette_size) {
        uint32_t *pal = (uint32_t *)libavcodec->av_packet_new_side_data(&avpkt, AV_PKT_DATA_PALETTE, AVPALETTE_SIZE);
        for (int i = 0; i < palette_size / 4; i++) {
            pal[i] = 0xFF << 24 | AV_RL32(palette + i);
        }
    }

    while (!src || size > 0) {
        int used_bytes;

        avctx->reordered_opaque = rtStart;
        avctx->reordered_opaque2 = rtStop;
        avctx->reordered_opaque3 = size;

        if (sendextradata && extradata->data && extradata->size > 0) {
            avpkt.data = (uint8_t *)extradata->data;
            avpkt.size = (int)extradata->size;
            used_bytes = libavcodec->avcodec_decode_video2(avctx, frame, &got_picture, &avpkt);
            sendextradata = false;
            if (used_bytes > 0) {
                used_bytes = 0;
            }
            if (mpeg12_codec(codecId)) {
                avctx->extradata = NULL;
                avctx->extradata_size = 0;
            }
        } else {
            unsigned int neededsize = size + FF_INPUT_BUFFER_PADDING_SIZE;

            if (ffbuflen < neededsize) {
                ffbuf = (unsigned char*)realloc(ffbuf, ffbuflen = neededsize);
            }

            if (src) {
                memcpy(ffbuf, src, size);
                memset(ffbuf + size, 0, FF_INPUT_BUFFER_PADDING_SIZE);
            }
            if (parser) {
                uint8_t *outBuf = NULL;
                int out_size = 0;
                used_bytes = libavcodec->av_parser_parse2(parser, avctx, &outBuf, &out_size, src ? ffbuf : NULL, size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
                if (prior_in_rtStart == REFTIME_INVALID) {
                    prior_in_rtStart = rtStart;
                    prior_in_rtStop = rtStop;
                }
                if (out_size > 0 || !src) {
                    mpeg2_in_doubt = false;
                    avpkt.data = out_size > 0 ? outBuf : NULL;
                    avpkt.size = out_size;
                    if (out_size > used_bytes) {
                        avctx->reordered_opaque = prior_in_rtStart;
                        avctx->reordered_opaque2 = prior_in_rtStop;
                    } else {
                        avctx->reordered_opaque = rtStart;
                        avctx->reordered_opaque2 = rtStop;
                    }
                    prior_in_rtStart = rtStart;
                    prior_in_rtStop = rtStop;
                    avctx->reordered_opaque3 = out_size;
                    if (h264RandomAccess.search(avpkt.data, avpkt.size)) {
                        libavcodec->avcodec_decode_video2(avctx, frame, &got_picture, &avpkt);
                        h264RandomAccess.judgeUsability(&got_picture);
                    } else {
                        got_picture = 0;
                    }
                } else {
                    got_picture = 0;
                }
            } else {
                avpkt.data = src ? ffbuf : NULL;
                avpkt.size = size;
                if (codecId == AV_CODEC_ID_H264) {
                    if (h264RandomAccess.search(avpkt.data, avpkt.size)) {
                        used_bytes = libavcodec->avcodec_decode_video2(avctx, frame, &got_picture, &avpkt);
                        if (used_bytes < 0) {
                            return S_OK;
                        }
                        h264RandomAccess.judgeUsability(&got_picture);
                    } else {
                        got_picture = 0;
                        return S_OK;
                    }
                } else {
                    used_bytes = libavcodec->avcodec_decode_video2(avctx, frame, &got_picture, &avpkt);
                }
            }
        }

        if (used_bytes < 0) {
            return S_OK;
        }

        if (got_picture && frame->data[0]) {
            int frametype;
            if (avctx->codec_id == AV_CODEC_ID_H261) {
                frametype = FRAME_TYPE::I;
            } else {
                switch (frame->pict_type) {
                    case AV_PICTURE_TYPE_P:
                        frametype = FRAME_TYPE::P;
                        break;
                    case AV_PICTURE_TYPE_B:
                        frametype = FRAME_TYPE::B;
                        break;
                    case AV_PICTURE_TYPE_I:
                        frametype = FRAME_TYPE::I;
                        break;
                    case AV_PICTURE_TYPE_S:
                        frametype = FRAME_TYPE::GMC;
                        break;
                    case AV_PICTURE_TYPE_SI:
                        frametype = FRAME_TYPE::SI;
                        break;
                    case AV_PICTURE_TYPE_SP:
                        frametype = FRAME_TYPE::SP;
                        break;
                    case 0:
                        frametype = pIn && pIn->IsSyncPoint() == S_OK ?
                                    FRAME_TYPE::I :
                                    FRAME_TYPE::P;
                        break;
                    default:
                        frametype = FRAME_TYPE::UNKNOWN;
                        break;
                }
            }

            if (pIn && pIn->IsPreroll() == S_OK) {
                sinkD->deliverPreroll(frametype);
            } else {
                int fieldtype = frame->interlaced_frame ?
                                (frame->top_field_first ?
                                 FIELD_TYPE::INT_TFF :
                                 FIELD_TYPE::INT_BFF) :
                                    FIELD_TYPE::PROGRESSIVE_FRAME;

                if (codecId == AV_CODEC_ID_MPEG2VIDEO) {
                    if (mpeg2_new_sequence) {
                        fieldtype |= FIELD_TYPE::SEQ_START;
                    }

                    if (frame->mpeg2_sequence_end_flag) {
                        fieldtype |= FIELD_TYPE::SEQ_END;
                        mpeg2_new_sequence = true;
                        frame->mpeg2_sequence_end_flag = 0;
                    } else {
                        mpeg2_new_sequence = false;
                    }
                }

                if (frame->play_flags & CODEC_FLAG_QPEL) {
                    frametype |= FRAME_TYPE::QPEL;
                }

                uint64_t csp = csp_lavc2ffdshow(avctx->pix_fmt);

                Trect r(0, 0, avctx->width, avctx->height);

                if (avctx->sample_aspect_ratio.num) {
                    r.sar = avctx->sample_aspect_ratio;
                } else {
                    r.sar = containerSar;
                }

                // Correct impossible sar for DVD
                if (codecId == AV_CODEC_ID_MPEG2VIDEO) {
                    r.sar = guessMPEG2sar(r, avctx->sample_aspect_ratio2, containerSar);
                }

                quants = frame->qscale_table;
                quantsStride = frame->qstride;
                quantType = frame->qscale_type;
                quantsDx = (r.dx + 15) >> 4;
                quantsDy = (r.dy + 15) >> 4;

                const stride_t linesize[4] = {frame->linesize[0], frame->linesize[1], frame->linesize[2], frame->linesize[3]};

                TffPict pict(csp, frame->data, linesize, r, true, frametype, fieldtype, srcLen0, pIn, palette_size ? palette : NULL); //TODO: src frame size
                pict.gmcWarpingPoints = frame->num_sprite_warping_points;
                pict.gmcWarpingPointsReal = frame->real_sprite_warping_points;
                pict.setFullRange(avctx->color_range);
                pict.YCbCr_RGB_matrix_coefficients = avctx->colorspace;
                pict.chroma_sample_location = avctx->chroma_sample_location;
                pict.color_primaries = avctx->color_primaries;
                pict.color_trc = avctx->color_trc;

#ifdef OSD_H264POC
                if (codecId == CODEC_ID_H264) {
                    pict.h264_poc = frame->h264_poc_outputed;
                }
#endif

                if (mpeg12_codec(codecId)) {
                    if (frametype == FRAME_TYPE::I) {
                        pict.rtStart = frame->reordered_opaque;
                    } else {
                        pict.rtStart = prior_out_rtStop;
                    }

                    // cope with a change in rate
                    if (rateInfo->rate.Rate != rateInfo->ratechange.Rate
                            && rateInfo->flushed
                            && frametype == FRAME_TYPE::I) {
                        // Buggy DVD navigator does not work as it is documented.
                        // DPRINTF(_l("rateInfo->ratechange.StartTime = %s rateInfo->rate.StartTime = %s rateInfo->rate.Rate %d"), Trt2str(rateInfo->ratechange.StartTime).c_str(),Trt2str(rateInfo->rate.StartTime).c_str(),rateInfo->rate.Rate);

                        rateInfo->rate.StartTime = pict.rtStart;
                        rateInfo->rate.Rate = rateInfo->ratechange.Rate;
                        rateInfo->isDiscontinuity = true;
                        // DPRINTF(_l("Got Rate StartTime = %s Rate = %d\n"), Trt2str(rateInfo->rate.StartTime).c_str(), rateInfo->rate.Rate);
                    }

                    if ((rateInfo->isDiscontinuity || rateInfo->correctTS) && frametype == FRAME_TYPE::I) {
                        // if we're at a Discontinuity use the times we're being sent in
                        // DPRINTF((ffstring(L"rateInfo->isDiscontinuity found. pict.rtStart ") + Trt2str(pict.rtStart) + L" rateInfo->rate.StartTime " + Trt2str(rateInfo->rate.StartTime)).c_str());
                        pict.rtStart = rateInfo->rate.StartTime + (pict.rtStart - rateInfo->rate.StartTime) * abs(rateInfo->rate.Rate) / 10000;

                        // DPRINTF(_l("rateInfo->isDiscontinuity found. updating rtStart %s prior_out_rtStop %s"),Trt2str(pict.rtStart).c_str(), Trt2str(prior_out_rtStop).c_str());
                        pict.discontinuity = rateInfo->isDiscontinuity;
                        rateInfo->isDiscontinuity = false;
                    } else {
                        pict.rtStart = prior_out_rtStop;
                    }

                    REFERENCE_TIME duration = getDuration();

                    if (rateInfo->rate.Rate < (10000 / TffdshowVideoInputPin::MAX_SPEED)) {
                        pict.rtStop = pict.rtStart + duration;
                        pict.fieldtype |= FIELD_TYPE::SEQ_START | FIELD_TYPE::SEQ_END;
                    } else
                        pict.rtStop = pict.rtStart +
                                      (duration * (frame->repeat_pict ? 3 : 2) * abs(rateInfo->rate.Rate) / (2 * 10000));
                    if (rateInfo->isDiscontinuity) {
                        telecineManager.onSeek();
                    }
                } else if (parser) {
                    if (prior_out_rtStart >= frame->reordered_opaque && prior_out_rtStart != REFTIME_INVALID) {
                        pict.rtStart = pict.rtStop = prior_out_rtStop;
                    } else {
                        pict.rtStart = frame->reordered_opaque;
                        pict.rtStop = frame->reordered_opaque2;
                    }
                    pict.srcSize = (size_t)frame->reordered_opaque3;
                } else if (dont_use_rtStop_from_upper_stream) {
                    if (prior_out_rtStart >= frame->reordered_opaque && prior_out_rtStart != REFTIME_INVALID) {
                        pict.rtStart = prior_out_rtStop;
                    } else {
                        pict.rtStart = frame->reordered_opaque;
                    }
                    pict.srcSize = (size_t)frame->reordered_opaque3; // FIXME this is not correct for MPEG-1/2 that use SOURCE_TRUNCATED. (Just for OSD, not that important bug)

                    if (pict.rtStart == REFTIME_INVALID) {
                        pict.rtStart = prior_out_rtStop;
                    }

                    if (avgTimePerFrame == -1) {
                        deciV->getAverageTimePerFrame(&avgTimePerFrame);
                    }

                    if (avgTimePerFrame) {
                        pict.rtStop = pict.rtStart + avgTimePerFrame + frame->repeat_pict * avgTimePerFrame / 2;
                    } else if (avctx->time_base.num && avctx->time_base.den) {
                        REFERENCE_TIME duration = getDuration();
                        if (duration <= 0) { duration = REF_SECOND_MULT / 10; }
                        pict.rtStop = pict.rtStart + duration;
                        if (frame->repeat_pict) {
                            pict.rtStop += (duration >> 1) * frame->repeat_pict;
                        }
                    } else {
                        pict.rtStop = pict.rtStart + 1;
                    }

                    if (avctx->codec_tag == FOURCC_MPG1 || avctx->codec_tag == FOURCC_MPG2) {
                        pict.mediatimeStart = pict.mediatimeStop = REFTIME_INVALID;
                    }
                } else if (theorart) {
                    pict.rtStart = frame->reordered_opaque - segmentTimeStart;
                    pict.rtStop  = pict.rtStart + 1;
                } else if (avctx->has_b_frames && bReorderBFrame) {
                    // do not reorder timestamps in this case.
                    // Timestamps simply increase.
                    // ex: AVI files

                    int pos = inPosB - 2;

                    if (pos < 0) {
                        pos += countof(b);
                    }

                    pict.rtStart = b[pos].rtStart;
                    pict.rtStop = b[pos].rtStop;
                    pict.srcSize = b[pos].srcSize;
                }

                // soft telecine detection
                // if "Detect soft telecine and average frame durations" is enabled,
                // flames are flagged as progressive, frame durations are averaged.
                // pict.film is valid even if the setting is disabled.
                telecineManager.new_frame(frame->top_field_first, frame->repeat_pict, pict.rtStart, pict.rtStop);
                telecineManager.get_fieldtype(pict);
                telecineManager.get_timestamps(pict);

                if (pict.rtStop  <= pict.rtStart + 1) {
                    // In this case, frames are dropped just because its delivery is slightly delayed.
                    // We need to fix it.
                    // getDuration returns 10ms if there is no available information.
                    // This meas all videos exceeding 100 fps must have valid frame duration.
                    pict.rtStop = pict.rtStart + getDuration();
                }

                prior_out_rtStart = pict.rtStart;
                prior_out_rtStop = pict.rtStop;
                hr = sinkD->deliverDecodedSample(pict);
                if (hr != S_OK
                        || (used_bytes && sinkD->acceptsManyFrames() != S_OK)
                        || avctx->codec_id == AV_CODEC_ID_LOCO) {
                    return hr;
                }
            }
        } else {
            if (!src) {
                break;
            }
        }

        if (!used_bytes && codecId == AV_CODEC_ID_SVQ3) {
            return S_OK;
        }

        if (avctx->active_thread_type == FF_THREAD_FRAME && !parser) {
            return hr;
        }

        src += used_bytes;
        size -= used_bytes;
    }
    return S_OK;
}

bool TvideoCodecLibavcodec::onSeek(REFERENCE_TIME segmentStart)
{
    rtStart = rtStop = REFTIME_INVALID;
    prior_in_rtStart = prior_in_rtStop = REFTIME_INVALID;
    prior_out_rtStart = REFTIME_INVALID;
    prior_out_rtStop = 0;

    wasKey = false;
    segmentTimeStart = segmentStart;
    inPosB = 1;

    for (int pos = 0 ; pos < countof(b) ; pos++) {
        b[pos].rtStart = REFTIME_INVALID;
        b[pos].rtStop = REFTIME_INVALID;
        b[pos].srcSize = 0;
    }

    if (ccDecoder) {
        ccDecoder->onSeek();
    }
    h264RandomAccess.onSeek();
    telecineManager.onSeek();
    mpeg2_new_sequence = true;

    if (avctx) {
        if (!firstSeek && connectedSplitter == TffdshowVideoInputPin::Haali_Media_splitter) {
            avctx->h264_has_to_drop_first_non_ref = 1;
        } else {
            firstSeek = false;
        }

        libavcodec->avcodec_flush_buffers(avctx);
        return true;
    }
    if (parser) {
        libavcodec->av_parser_close(parser);
        parser = libavcodec->av_parser_init(codecId);
    }
    return false;
}

bool TvideoCodecLibavcodec::onDiscontinuity(void)
{
    wasKey = false;
    if (ccDecoder) {
        ccDecoder->onSeek();
    }
    h264RandomAccess.onSeek();
    return avctx ? (libavcodec->avcodec_flush_buffers(avctx), true) : false;
}

const char_t* TvideoCodecLibavcodec::getName(void) const
{
    if (avcodec) {
        static const char_t *libname = _l("libavcodec");
        tsnprintf_s(codecName, countof(codecName), _TRUNCATE, _l("%s %s"), libname, (const char_t*)text<char_t>(avcodec->name));
        return codecName;
    } else {
        return _l("libavcodec");
    }
}

void TvideoCodecLibavcodec::getEncoderInfo(char_t *buf, size_t buflen) const
{
    int xvid_build, divx_version, divx_build, lavc_build;
    if (avctx && (mpeg12_codec(codecId) || mpeg4_codec(codecId) || codecId == AV_CODEC_ID_FLV1)) {
        libavcodec->avcodec_get_encoder_info(avctx, &xvid_build, &divx_version, &divx_build, &lavc_build);
        if (xvid_build) {
            tsnprintf_s(buf, buflen, _TRUNCATE, _l("XviD build %i"), xvid_build);
        } else if (lavc_build) {
            tsnprintf_s(buf, buflen, _TRUNCATE, _l("libavcodec build %i"), lavc_build);
        } else if (divx_version || divx_build) {
            tsnprintf_s(buf, buflen, _TRUNCATE, _l("DivX version %i.%02i, build %i"), divx_version / 100, divx_version % 100, divx_build);
        } else {
            ff_strncpy(buf, _l("unknown"), buflen);
        }
    } else {
        ff_strncpy(buf, _l("unknown"), buflen);
    }
    buf[buflen - 1] = '\0';
}

void TvideoCodecLibavcodec::line(unsigned char *dst, unsigned int _x0, unsigned int _y0, unsigned int _x1, unsigned int _y1, stride_t strideY)
{
    drawline< TaddColor<100> >(_x0, _y0, _x1, _y1, 100, dst, strideY);
}

void TvideoCodecLibavcodec::draw_arrow(uint8_t *buf, int sx, int sy, int ex, int ey, stride_t stride, int mulx, int muly, int dstdx, int dstdy)
{
    sx = limit(mulx * sx >> 12, 0, dstdx - 1);
    sy = limit(muly * sy >> 12, 0, dstdy - 1);
    ex = limit(mulx * ex >> 12, 0, dstdx - 1);
    ey = limit(muly * ey >> 12, 0, dstdy - 1);
    int dx, dy;

    dx = ex - sx;
    dy = ey - sy;

    if (dx * dx + dy * dy > 3 * 3) {
        int rx =  dx + dy;
        int ry = -dx + dy;
        int length = ff_sqrt((rx * rx + ry * ry) << 8);

        //FIXME subpixel accuracy
        rx = roundDiv(rx * 3 << 4, length);
        ry = roundDiv(ry * 3 << 4, length);

        line(buf, sx, sy, sx + rx, sy + ry, stride);
        line(buf, sx, sy, sx - ry, sy + rx, stride);
    }
    line(buf, sx, sy, ex, ey, stride);
}

bool TvideoCodecLibavcodec::drawMV(unsigned char *dst, unsigned int dstdx, stride_t stride, unsigned int dstdy) const
{
    if (!frame->motion_val || !frame->mb_type || !frame->motion_val[0]) {
        return false;
    }

#define IS_8X8(a)  ((a)&MB_TYPE_8x8)
#define IS_16X8(a) ((a)&MB_TYPE_16x8)
#define IS_8X16(a) ((a)&MB_TYPE_8x16)
#define IS_INTERLACED(a) ((a)&MB_TYPE_INTERLACED)
#define USES_LIST(a, list) ((a) & ((MB_TYPE_P0L0|MB_TYPE_P1L0)<<(2*(list))))

    const int shift = 1 + ((frame->play_flags & CODEC_FLAG_QPEL) ? 1 : 0);
    const int mv_sample_log2 = 4 - frame->motion_subsample_log2;
    const int mv_stride = (frame->mb_width << mv_sample_log2) + (avctx->codec_id == AV_CODEC_ID_H264 ? 0 : 1);
    int direction = 0;

    int mulx = (dstdx << 12) / avctx->width;
    int muly = (dstdy << 12) / avctx->height;

    for (int mb_y = 0; mb_y < frame->mb_height; mb_y++)
        for (int mb_x = 0; mb_x < frame->mb_width; mb_x++) {
            const int mb_index = mb_x + mb_y * frame->mb_stride;
            if (!USES_LIST(frame->mb_type[mb_index], direction)) {
                continue;
            }
            if (IS_8X8(frame->mb_type[mb_index]))
                for (int i = 0; i < 4; i++) {
                    int sx = mb_x * 16 + 4 + 8 * (i & 1) ;
                    int sy = mb_y * 16 + 4 + 8 * (i >> 1);
                    int xy = (mb_x * 2 + (i & 1) + (mb_y * 2 + (i >> 1)) * mv_stride) << (mv_sample_log2 - 1);
                    int mx = (frame->motion_val[direction][xy][0] >> shift) + sx;
                    int my = (frame->motion_val[direction][xy][1] >> shift) + sy;
                    draw_arrow(dst, sx, sy, mx, my, stride, mulx, muly, dstdx, dstdy);
                }
            else if (IS_16X8(frame->mb_type[mb_index]))
                for (int i = 0; i < 2; i++) {
                    int sx = mb_x * 16 + 8;
                    int sy = mb_y * 16 + 4 + 8 * i;
                    int xy = (mb_x * 2 + (mb_y * 2 + i) * mv_stride) << (mv_sample_log2 - 1);
                    int mx = frame->motion_val[direction][xy][0] >> shift;
                    int my = frame->motion_val[direction][xy][1] >> shift;
                    if (IS_INTERLACED(frame->mb_type[mb_index])) {
                        my *= 2;
                    }
                    draw_arrow(dst, sx, sy, mx + sx, my + sy, stride, mulx, muly, dstdx, dstdy);
                }
            else if (IS_8X16(frame->mb_type[mb_index]))
                for (int i = 0; i < 2; i++) {
                    int sx = mb_x * 16 + 4 + 8 * i;
                    int sy = mb_y * 16 + 8;
                    int xy = (mb_x * 2 + i + mb_y * 2 * mv_stride) << (mv_sample_log2 - 1);
                    int mx = (frame->motion_val[direction][xy][0] >> shift);
                    int my = (frame->motion_val[direction][xy][1] >> shift);
                    if (IS_INTERLACED(frame->mb_type[mb_index])) {
                        my *= 2;
                    }
                    draw_arrow(dst, sx, sy, mx + sx, my + sy, stride, mulx, muly, dstdx, dstdy);
                }
            else {
                int sx = mb_x * 16 + 8;
                int sy = mb_y * 16 + 8;
                int xy = (mb_x + mb_y * mv_stride) << mv_sample_log2;
                int mx = (frame->motion_val[direction][xy][0] >> shift) + sx;
                int my = (frame->motion_val[direction][xy][1] >> shift) + sy;
                draw_arrow(dst, sx, sy, mx, my, stride, mulx, muly, dstdx, dstdy);
            }
        }
#undef IS_8X8
#undef IS_16X8
#undef IS_8X16
#undef IS_INTERLACED
#undef USES_LIST
    return true;
}

//------------------------------ compression ------------------------------
void TvideoCodecLibavcodec::getCompressColorspaces(Tcsps &csps, unsigned int outDx, unsigned int outDy)
{
    switch (coCfg->codecId) {
        case AV_CODEC_ID_FFVHUFF:
            if (coCfg->huffyuv_csp == 0) {
                csps.add(FF_CSP_422P);
            } else {
                csps.add(FF_CSP_420P);
            }
            break;
        case AV_CODEC_ID_FFV1:
            switch (coCfg->ffv1_csp) {
                case FOURCC_YV12:
                    csps.add(FF_CSP_420P);
                    break;
                case FOURCC_444P:
                    csps.add(FF_CSP_444P);
                    break;
                case FOURCC_422P:
                    csps.add(FF_CSP_422P);
                    break;
                case FOURCC_411P:
                    csps.add(FF_CSP_411P);
                    break;
                case FOURCC_410P:
                    csps.add(FF_CSP_410P);
                    break;
                case FOURCC_RGB3:
                    csps.add(FF_CSP_RGB32);
                    break;
            }
            break;
        case AV_CODEC_ID_MJPEG:
            csps.add(FF_CSP_420P | FF_CSP_FLAGS_YUV_JPEG);
            break;
        default:
            csps.add(FF_CSP_420P);
            break;
    }
}

bool TvideoCodecLibavcodec::supExtradata(void)
{
    return coCfg->codecId == AV_CODEC_ID_HUFFYUV || coCfg->codecId == AV_CODEC_ID_FFVHUFF;
}
bool TvideoCodecLibavcodec::getExtradata(const void* *ptr, size_t *len)
{
    if (!avctx || !len) {
        return false;
    }
    *len = avctx->extradata_size;
    if (ptr) {
        *ptr = avctx->extradata;
    }
    return true;
}

LRESULT TvideoCodecLibavcodec::beginCompress(int cfgcomode, uint64_t csp, const Trect &r)
{
    _mm_empty();

    avcodec = libavcodec->avcodec_find_encoder((AVCodecID)coCfg->codecId);
    if (!avcodec) {
        return ICERR_ERROR;
    }

    avctx = libavcodec->avcodec_alloc_context(avcodec);
    frame = libavcodec->avcodec_alloc_frame();

    this->cfgcomode = cfgcomode;

    avctx->thread_count = 1;

    avctx->width = r.dx;
    avctx->height = r.dy;
    mb_width = (avctx->width + 15) / 16;
    mb_height = (avctx->height + 15) / 16;
    mb_count = mb_width * mb_height;
    avctx->time_base.den = deci->getParam2(IDFF_enc_fpsRate);
    avctx->time_base.num = deci->getParam2(IDFF_enc_fpsScale);
    if (avctx->time_base.den > (1 << 16) - 1) {
        avctx->time_base.num = (int)(0.5 + (double)avctx->time_base.num / avctx->time_base.den * ((1 << 16) - 1));
        avctx->time_base.den = (1 << 16) - 1;
    }
    if (coCfg->codecId == AV_CODEC_ID_FFV1) {
        avctx->gop_size = coCfg->ffv1_key_interval;
    }

    avctx->codec_tag = coCfg->fourcc;
    psnr = deci->getParam2(IDFF_enc_psnr);
    if (psnr) {
        avctx->flags |= CODEC_FLAG_PSNR;
    }
    if (sup_gray(coCfg->codecId) && coCfg->gray) {
        avctx->flags |= CODEC_FLAG_GRAY;
    }

    if (coCfg->isQuantControlActive()) {
        avctx->qmin_i = coCfg->limitq(coCfg->q_i_min);
        avctx->qmax_i = coCfg->limitq(coCfg->q_i_max);
        avctx->qmin  = coCfg->limitq(coCfg->q_p_min);
        avctx->qmax  = coCfg->limitq(coCfg->q_p_max);
        avctx->qmin_b = coCfg->limitq(coCfg->q_b_min);
        avctx->qmax_b = coCfg->limitq(coCfg->q_b_max);
    } else {
        avctx->qmin_i = avctx->qmin = avctx->qmin_b = coCfg->getMinMaxQuant().first;
        avctx->qmax_i = avctx->qmax = avctx->qmax_b = coCfg->getMinMaxQuant().second;
    }

    if (coCfg->codecId != AV_CODEC_ID_MJPEG) {
        avctx->i_quant_factor = coCfg->i_quant_factor / 100.0f;
        avctx->i_quant_offset = coCfg->i_quant_offset / 100.0f;
    } else {
        avctx->i_quant_factor = 1.0f;
        avctx->i_quant_offset = 0.0f;
    }

    if (sup_quantBias(coCfg->codecId)) {
        if (coCfg->isIntraQuantBias) {
            avctx->intra_quant_bias = coCfg->intraQuantBias;
        }
        if (coCfg->isInterQuantBias) {
            avctx->inter_quant_bias = coCfg->interQuantBias;
        }
    }
    avctx->dct_algo = coCfg->dct_algo;
    if (sup_qns(coCfg->codecId)) {
        avctx->quantizer_noise_shaping = coCfg->qns;
    }

    avctx->pix_fmt = PIX_FMT_YUV420P;
    switch (coCfg->codecId) {
        case AV_CODEC_ID_FFVHUFF: {
            avctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
            avctx->prediction_method = coCfg->huffyuv_pred;
            switch (coCfg->huffyuv_csp) {
                case 0:
                    avctx->pix_fmt = PIX_FMT_YUV422P;
                    break;
                case 1:
                    avctx->pix_fmt = PIX_FMT_YUV420P;
                    break;
            }
            avctx->context_model = coCfg->huffyuv_ctx;
            break;
        }
        case AV_CODEC_ID_FFV1: {
            avctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
            avctx->coder_type = coCfg->ffv1_coder;
            avctx->context_model = coCfg->ffv1_context;
            switch (coCfg->ffv1_csp) {
                case FOURCC_YV12:
                    avctx->pix_fmt = PIX_FMT_YUV420P;
                    break;
                case FOURCC_444P:
                    avctx->pix_fmt = PIX_FMT_YUV444P;
                    break;
                case FOURCC_422P:
                    avctx->pix_fmt = PIX_FMT_YUV422P;
                    break;
                case FOURCC_411P:
                    avctx->pix_fmt = PIX_FMT_YUV411P;
                    break;
                case FOURCC_410P:
                    avctx->pix_fmt = PIX_FMT_YUV410P;
                    break;
                case FOURCC_RGB3:
                    avctx->pix_fmt = PIX_FMT_RGB32;
                    break;
            }
            break;
        }
        case AV_CODEC_ID_MJPEG:
            avctx->pix_fmt = PIX_FMT_YUVJ420P;
            break;
    }

    switch (cfgcomode) {
        case ENC_MODE::CBR:
            avctx->bit_rate = coCfg->bitrate1000 * 1000;
            avctx->bit_rate_tolerance = coCfg->ff1_vratetol * 8 * 1000;
            avctx->qcompress = coCfg->ff1_vqcomp / 100.0f;
            avctx->max_qdiff = coCfg->ff1_vqdiff;
            avctx->rc_qsquish = coCfg->ff1_rc_squish ? 1.0f : 0.0f;
            avctx->rc_max_rate   = coCfg->ff1_rc_max_rate1000 * 1000;
            avctx->rc_min_rate   = coCfg->ff1_rc_min_rate1000 * 1000;
            avctx->rc_buffer_size = coCfg->ff1_rc_buffer_size;
            avctx->rc_initial_buffer_occupancy = avctx->rc_buffer_size * 3 / 4;
            avctx->rc_buffer_aggressivity = 1.0f;
            break;
        case ENC_MODE::VBR_QUAL:
        case ENC_MODE::VBR_QUANT:
            avctx->bit_rate = 400000;
            break;
        case ENC_MODE::UNKNOWN:
            break;
        default:
            return ICERR_ERROR;
    }

    if (sup_quantProps(coCfg->codecId) && coCfg->is_lavc_nr) {
        avctx->noise_reduction = coCfg->lavc_nr;
    }

    RcOverride rces[2];
    int rcescount = 0;
    if ((avctx->rc_override_count = rcescount) != 0) {
        avctx->rc_override = rces;
    }

    // save av_log_callback and set custom av_log that shows message box.
    void (*avlogOldFunc)(AVCodecContext*, int, const char*, va_list);
    int errorbox;
    deci->getParam(IDFF_errorbox, &errorbox);
    if (errorbox) {
        avlogOldFunc = (void (*)(AVCodecContext*, int, const char*, va_list))(libavcodec->av_log_get_callback());
        libavcodec->av_log_set_callback(Tlibavcodec::avlogMsgBox);
    }

    int err = libavcodec->avcodec_open(avctx, avcodec);

    // restore av_log_callback
    if (errorbox) {
        libavcodec->av_log_set_callback(avlogOldFunc);
    }
    if (err < 0) {
        avctx->codec = NULL;
        return ICERR_ERROR;
    }
    if (avctx->stats_in) {
        free(avctx->stats_in);
        avctx->stats_in = NULL;
    }
    codecinited = true;
    return ICERR_OK;
}

HRESULT TvideoCodecLibavcodec::compress(const TffPict &pict, TencFrameParams &params)
{
    if (coCfg->mode == ENC_MODE::VBR_QUAL) {
        frame->quality = (100 - coCfg->qual) * 40;
        avctx->flags |= CODEC_FLAG_QSCALE;
    } else if (params.quant == -1) {
        avctx->flags &= ~CODEC_FLAG_QSCALE;
    } else {
        avctx->flags |= CODEC_FLAG_QSCALE;
        frame->quality = coCfg->limitq(params.quant) * FF_QP2LAMBDA;
        avctx->qmin_i = avctx->qmax_i = 0;
        avctx->qmin_b = avctx->qmax_b = 0;
    }

    if (isAdaptive) {
        avctx->lmin = coCfg->q_mb_min * FF_QP2LAMBDA;
        avctx->lmax = coCfg->q_mb_max * FF_QP2LAMBDA;
    }

    frame->top_field_first = (avctx->flags & CODEC_FLAG_INTERLACED_DCT && coCfg->interlacing_tff) ? 1 : 0;

    switch (params.frametype) {
        case FRAME_TYPE::I:
            frame->pict_type = AV_PICTURE_TYPE_I;
            break;
        case FRAME_TYPE::P:
            frame->pict_type = AV_PICTURE_TYPE_P;
            break;
        case FRAME_TYPE::B:
            frame->pict_type = AV_PICTURE_TYPE_B;
            break;
        default:
            //frame->pict_type=0;
            break;
    }
    bool flushing = !pict.data[0];
    if (!flushing)
        for (int i = 0; i < 4; i++) {
            frame->data[i] = (uint8_t*)pict.data[i];
            frame->linesize[i] = (int)pict.stride[i];
        }
    HRESULT hr = S_OK;
    while (frame->data[0] || flushing) {
        frame->pts = AV_NOPTS_VALUE;
        TmediaSample sample;
        if (FAILED(hr = sinkE->getDstBuffer(&sample, pict))) {
            return hr;
        }
        params.length = libavcodec->avcodec_encode_video(avctx, sample, sample.size(), !flushing ? frame : NULL);
        if ((int)params.length < 0) {
            return sinkE->deliverError();
        } else if (params.length == 0 && flushing) {
            break;
        }

        if (/*!isAdaptive || */!avctx->coded_frame->qscale_table) {
            params.quant = int(avctx->coded_frame->quality / FF_QP2LAMBDA + 0.5);
        } else {
            unsigned int sum = 0;
            for (unsigned int y = 0; y < mb_height; y++)
                for (unsigned int x = 0; x < mb_width; x++) {
                    sum += avctx->coded_frame->qscale_table[x + y * avctx->coded_frame->qstride];
                }
            params.quant = roundDiv(sum, mb_count);
        }

        params.kblks = avctx->i_count;
        params.mblks = avctx->p_count;
        params.ublks = avctx->skip_count;

        switch (avctx->coded_frame->pict_type) {
            case AV_PICTURE_TYPE_I:
                params.frametype = FRAME_TYPE::I;
                break;
            case AV_PICTURE_TYPE_P:
                params.frametype = FRAME_TYPE::P;
                break;
            case AV_PICTURE_TYPE_B:
                params.frametype = FRAME_TYPE::B;
                break;
            case AV_PICTURE_TYPE_S:
                params.frametype = FRAME_TYPE::GMC;
                break;
            case AV_PICTURE_TYPE_SI:
                params.frametype = FRAME_TYPE::SI;
                break;
            case AV_PICTURE_TYPE_SP:
                params.frametype = FRAME_TYPE::SP;
                break;
            case 0:
                params.frametype = FRAME_TYPE::DELAY;
                break;
        }
        params.keyframe = !!avctx->coded_frame->key_frame;

        if (psnr) {
            params.psnrY = avctx->coded_frame->error[0];
            params.psnrU = avctx->coded_frame->error[1];
            params.psnrV = avctx->coded_frame->error[2];
        }
        if (FAILED(hr = sinkE->deliverEncodedSample(sample, params))) {
            return hr;
        }
        frame->data[0] = NULL;
    }
    return hr;
}

const char* TvideoCodecLibavcodec::get_current_idct(void)
{
    if (avctx && (mpeg12_codec(codecId) || mpeg4_codec(codecId) || codecId == AV_CODEC_ID_FLV1)) {
        return libavcodec->avcodec_get_current_idct(avctx);
    } else {
        return NULL;
    }
}

void TvideoCodecLibavcodec::reorderBFrames(REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop)
{
    // Re-order B-frames if needed
    if (avctx->has_b_frames && bReorderBFrame) {
        rtStart = b[inPosB].rtStart;
        rtStop  = b[inPosB].rtStop;
    }
}

TvideoCodecLibavcodec::Th264RandomAccess::Th264RandomAccess(TvideoCodecLibavcodec *Iparent):
    parent(Iparent)
{
    recovery_mode = 1;
    recovery_frame_cnt = 0;
}

void TvideoCodecLibavcodec::Th264RandomAccess::onSeek(void)
{
    recovery_mode = 1;
    recovery_frame_cnt = 0;

    if (parent->avctx->active_thread_type == FF_THREAD_FRAME) {
        thread_delay = parent->avctx->thread_count;
    } else {
        thread_delay = 1;
    }
}

// return 0:not found, don't send it to libavcodec, 1:send it anyway.
int TvideoCodecLibavcodec::Th264RandomAccess::search(uint8_t* buf, int buf_size)
{
    if (parent->codecId == AV_CODEC_ID_H264 && recovery_mode == 1) {
        if (!buf || !buf_size) {
            return 0;
        }
        int is_recovery_point = parent->libavcodec->avcodec_h264_search_recovery_point(parent->avctx, buf, buf_size, &recovery_frame_cnt);
        if (is_recovery_point == 3) {
            // IDR
            DPRINTF(L"Th264RandomAccess IDR");
            recovery_mode = 0;
            return 1;
        } else if (is_recovery_point == 2) {
            // GDR, recovery_frame_cnt is valid.
            DPRINTF(L"Th264RandomAccess GDR recovery_frame_cnt=%d", recovery_frame_cnt);
            recovery_mode = 2;
            return 1;
        } else if (is_recovery_point == 1) {
            // I frames are not ideal for recovery, but if we ignore them, better frames may not come forever. recovery_frame_cnt is not valid.
            DPRINTF(L"Th264RandomAccess using I-frame as an entry point. This is non-compliant to the spec.");
            recovery_mode = 2;
            recovery_frame_cnt = 0;
            return 1;
        } else {
            return 0;
        }
    } else {
        return 1;
    }
}

void TvideoCodecLibavcodec::Th264RandomAccess::judgeUsability(int *got_picture_ptr)
{
    if (parent->codecId != AV_CODEC_ID_H264) {
        return;
    }

    AVFrame *frame = parent->frame;

    if (thread_delay > 1 && --thread_delay > 0 || frame->h264_max_frame_num == 0) {
        return;
    }

    if (recovery_mode == 1 || recovery_mode == 2) {
        recovery_frame_cnt = (recovery_frame_cnt + frame->h264_frame_num_decoded) % frame->h264_max_frame_num;
        recovery_mode = 3;
    }

    if (recovery_mode == 3) {
        if (recovery_frame_cnt <= frame->h264_frame_num_decoded) {
            recovery_poc = frame->h264_poc_decoded;
            recovery_mode = 4;
        }
    }

    if (recovery_mode == 4) {
        if (frame->h264_poc_outputed >= recovery_poc) {
            recovery_mode = 0;
        }
    }

    if (recovery_mode != 0) {
        *got_picture_ptr = 0;
    }
}
