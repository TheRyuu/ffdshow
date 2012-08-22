/*
 * Copyright (c) 2003-2006 Milan Cutka
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
#include "TaudioCodecLibavcodec.h"
#include "IffdshowBase.h"
#include "IffdshowDec.h"
#include "IffdshowDecAudio.h"
#include "dsutil.h"
#include "ffdshow_mediaguids.h"
#include "codecs/ogg headers/vorbisformat.h"
#include "ffmpeg/libavcodec/avcodec.h"
#include "ffmpeg/libavutil/audioconvert.h"

TaudioCodecLibavcodec::TaudioCodecLibavcodec(IffdshowBase *deci, IdecAudioSink *Isink):
    Tcodec(deci),
    TaudioCodec(deci, Isink)
{
    avctx = NULL;
    avcodec = NULL;
    codecInited = false;
    contextInited = false;
    parser = NULL;
}

bool TaudioCodecLibavcodec::init(const CMediaType &mt)
{
    deci->getLibavcodec(&ffmpeg);
    if (ffmpeg->ok) {
        avcodec = ffmpeg->avcodec_find_decoder(codecId);

        if (!avcodec) {
            return false;
        }

        if (codecId == AV_CODEC_ID_AMR_NB) {
            fmt.setChannels(1);
            fmt.freq = 8000;
        }

        avctx = ffmpeg->avcodec_alloc_context(avcodec);
        avctx->sample_rate = fmt.freq;
        avctx->channels = fmt.nchannels;

        if (parser) {
            ffmpeg->av_parser_close(parser);
            parser = NULL;
        }

        // Disable AAC parser, as of 03/2011 ffmpeg returns "More than one AAC RDB per ADTS frame is not implemented. Update your FFmpeg..."
        if (codecId != AV_CODEC_ID_AAC && codecId != AV_CODEC_ID_AAC_LATM) {
            parser = ffmpeg->av_parser_init(codecId);
        }

        if (mt.formattype == FORMAT_WaveFormatEx) {
            const WAVEFORMATEX *wfex = (const WAVEFORMATEX*)mt.pbFormat;
            avctx->bit_rate = wfex->nAvgBytesPerSec * 8;
            avctx->bits_per_coded_sample = wfex->wBitsPerSample;
            if (wfex->wBitsPerSample == 0 && codecId == AV_CODEC_ID_COOK) {
                avctx->bits_per_coded_sample = 16;
            }
            avctx->block_align = wfex->nBlockAlign;
        } else {
            avctx->bit_rate = fmt.avgBytesPerSec() * 8;
            avctx->bits_per_coded_sample = fmt.bitsPerSample();
            avctx->block_align = fmt.blockAlign();
        }

        Textradata extradata(mt, FF_INPUT_BUFFER_PADDING_SIZE);

        if (codecId == AV_CODEC_ID_COOK && mt.formattype == FORMAT_WaveFormatEx && mt.pbFormat) {
            /* Cook specifications : extradata is located after the real audio info, 4 or 5 depending on the version
               @See http://wiki.multimedia.cx/index.php?title=RealMedia the audio block information
               TODO : add support for header version 3
            */

            DWORD cbSize = ((const WAVEFORMATEX*)mt.pbFormat)->cbSize;
            BYTE* fmt = mt.Format() + sizeof(WAVEFORMATEX) + cbSize;

            for (int i = 0, len = mt.FormatLength() - (sizeof(WAVEFORMATEX) + cbSize); i < len - 4; i++, fmt++) {
                if (fmt[0] == '.' || fmt[1] == 'r' || fmt[2] == 'a') {
                    break;
                }
            }

            m_realAudioInfo = *(TrealAudioInfo*) fmt;
            m_realAudioInfo.bswap();

            BYTE* p = NULL;
            if (m_realAudioInfo.version2 == 4) {
                // Skip the TrealAudioInfo4 data
                p = (BYTE*)((TrealAudioInfo4*)fmt + 1);
                int len = *p++;
                p += len;
                len = *p++;
                p += len;
                ASSERT(len == 4);
                //DPRINTF(_l("TaudioCodecLibavcodec cook version 4"));
            } else if (m_realAudioInfo.version2 == 5) {
                // Skip the TrealAudioInfo5 data
                p = (BYTE*)((TrealAudioInfo5*)fmt + 1);
                //DPRINTF(_l("TaudioCodecLibavcodec cook version 5"));
            } else {
                return false; //VFW_E_TYPE_NOT_ACCEPTED;
            }

            /* Cook specifications : after the end of TrealAudio4 or TrealAudio5 structure, we have this :
              byte[3]  Unknown
              #if version == 5
               byte     Unknown
              #endif
               dword    Codec extradata length
               byte[]   Codec extradata
            */

            // Skip the 3 unknown bytes
            p += 3;
            // + skip 1 byte if version 5
            if (m_realAudioInfo.version2 == 5) {
                p++;
            }

            // Extradata size : next 4 bytes
            avctx->extradata_size = std::min((DWORD)((mt.Format() + mt.FormatLength()) - (p + 4)), *(DWORD*)p);
            // Extradata starts after the 4 bytes size
            avctx->extradata = (uint8_t*)(p + 4);
            avctx->block_align = m_realAudioInfo.coded_frame_size;
        } else {
            avctx->extradata = (uint8_t*)extradata.data;
            avctx->extradata_size = (int)extradata.size;
        }

        if (codecId == AV_CODEC_ID_VORBIS && mt.formattype == FORMAT_VorbisFormat2) {
            const VORBISFORMAT2 *vf2 = (const VORBISFORMAT2*)mt.pbFormat;
            avctx->vorbis_header_size[0] = vf2->HeaderSize[0];
            avctx->vorbis_header_size[1] = vf2->HeaderSize[1];
            avctx->vorbis_header_size[2] = vf2->HeaderSize[2];
        }

        if (ffmpeg->avcodec_open(avctx, avcodec) < 0) {
            return false;
        }

        codecInited = true;

        switch (avctx->sample_fmt) {
            case AV_SAMPLE_FMT_S16:
                fmt.sf = TsampleFormat::SF_PCM16;
                break;
            case AV_SAMPLE_FMT_S32:
                fmt.sf = TsampleFormat::SF_PCM32;
                break;
            case AV_SAMPLE_FMT_FLT:
                fmt.sf = TsampleFormat::SF_FLOAT32;
                break;
            case AV_SAMPLE_FMT_DBL:
                fmt.sf = TsampleFormat::SF_FLOAT64;
                break;
        }

        ffmpeg->av_log_set_level(AV_LOG_QUIET);

        // Handle truncated streams
        if (avctx->codec->capabilities & CODEC_CAP_TRUNCATED) {
            avctx->flags |= CODEC_FLAG_TRUNCATED;
        }

        return true;

    } else {
        return false;
    }
}
TaudioCodecLibavcodec::~TaudioCodecLibavcodec()
{
    if (parser) {
        ffmpeg->av_parser_close(parser);
        parser = NULL;
    }
    if (avctx) {
        if (codecInited) {
            ffmpeg->avcodec_close(avctx);
        }
        codecInited = false;
        ffmpeg->av_free(avctx);
    }
    if (ffmpeg) {
        ffmpeg->Release();
    }
}
const char_t* TaudioCodecLibavcodec::getName(void) const
{
    return _l("libavcodec");
}
void TaudioCodecLibavcodec::getInputDescr1(char_t *buf, size_t buflen) const
{

    if (avcodec) {
        if (!strcmp(text<char_t>(avcodec->name), _l("mp3"))) {
            ff_strncpy(buf, _l("MP3"), buflen);
        } else if (!strcmp(text<char_t>(avcodec->name), _l("mp2"))) {
            ff_strncpy(buf, _l("MP2"), buflen);
        } else if (!strcmp(text<char_t>(avcodec->name), _l("mp1"))) {
            ff_strncpy(buf, _l("MP1"), buflen);
        } else if (!strcmp(text<char_t>(avcodec->name), _l("mp3float"))) {
            ff_strncpy(buf, _l("MP3"), buflen);
        } else if (!strcmp(text<char_t>(avcodec->name), _l("mp2float"))) {
            ff_strncpy(buf, _l("MP2"), buflen);
        } else if (!strcmp(text<char_t>(avcodec->name), _l("mp1float"))) {
            ff_strncpy(buf, _l("MP1"), buflen);
        } else if (!strcmp(text<char_t>(avcodec->name), _l("ac3"))) {
            ff_strncpy(buf, _l("AC3"), buflen);
        } else if (!strcmp(text<char_t>(avcodec->name), _l("eac3"))) {
            ff_strncpy(buf, _l("E-AC3"), buflen);
        } else if (!strcmp(text<char_t>(avcodec->name), _l("truehd"))) {
            ff_strncpy(buf, _l("TrueHD"), buflen);
        } else if (!strcmp(text<char_t>(avcodec->name), _l("mlp"))) {
            ff_strncpy(buf, _l("MLP"), buflen);
        } else if (!strcmp(text<char_t>(avcodec->name), _l("dca"))) {
            ff_strncpy(buf, _l("DTS"), buflen);
        } else if (!strcmp(text<char_t>(avcodec->name), _l("aac"))) {
            ff_strncpy(buf, _l("AAC"), buflen);
        } else if (!strcmp(text<char_t>(avcodec->name), _l("vorbis"))) {
            ff_strncpy(buf, _l("Vorbis"), buflen);
        } else if (!strcmp(text<char_t>(avcodec->name), _l("flac"))) {
            ff_strncpy(buf, _l("FLAC"), buflen);
        } else if (!strcmp(text<char_t>(avcodec->name), _l("tta"))) {
            ff_strncpy(buf, _l("TTA"), buflen);
        } else if (!strcmp(text<char_t>(avcodec->name), _l("wmav2"))) {
            ff_strncpy(buf, _l("WMAV2"), buflen);
        } else if (!strcmp(text<char_t>(avcodec->name), _l("cook"))) {
            ff_strncpy(buf, _l("COOK"), buflen);
        } else {
            ff_strncpy(buf, (const char_t *)text<char_t>(avcodec->name), buflen);
        }
    }
    buf[buflen - 1] = '\0';
}

static DWORD get_lav_channel_layout(uint64_t layout)
{
    if (layout > _UI32_MAX) {
        if (layout & AV_CH_WIDE_LEFT) {
            layout = (layout & ~AV_CH_WIDE_LEFT) | AV_CH_FRONT_LEFT_OF_CENTER;
        }
        if (layout & AV_CH_WIDE_RIGHT) {
            layout = (layout & ~AV_CH_WIDE_RIGHT) | AV_CH_FRONT_RIGHT_OF_CENTER;
        }

        if (layout & AV_CH_SURROUND_DIRECT_LEFT) {
            layout = (layout & ~AV_CH_SURROUND_DIRECT_LEFT) | AV_CH_SIDE_LEFT;
        }
        if (layout & AV_CH_SURROUND_DIRECT_RIGHT) {
            layout = (layout & ~AV_CH_SURROUND_DIRECT_RIGHT) | AV_CH_SIDE_RIGHT;
        }
    }

    // correct libavcodec layouts for AC3 and DTS
    if (layout == 0x60f) {
        layout = 0x3f;
    }

    return (DWORD)layout;
}


HRESULT TaudioCodecLibavcodec::decode(TbyteBuffer &src0)
{
    int srcBufferLength = (int)src0.size();
    uint8_t *srcBuffer = srcBufferLength ? &src0[0] : NULL;

    // Dynamic range compression for AC3/DTS formats
    if (deci->getParam2(IDFF_audio_decoder_DRC)) {
        float drcLevel = (float)deci->getParam2(IDFF_audio_decoder_DRC_Level) / 100;
        avctx->drc_scale = drcLevel;
    } else {
        avctx->drc_scale = 0.0;
    }

    //Special behaviour for real audio cook decoder
    if (codecId == AV_CODEC_ID_COOK) {
        int w = m_realAudioInfo.coded_frame_size;
        int h = m_realAudioInfo.sub_packet_h;
        int sps = m_realAudioInfo.sub_packet_size;
        size_t len = w * h;
        avctx->block_align = m_realAudioInfo.sub_packet_size;

        BYTE *pBuf = (BYTE*)srcBuf.resize(len * 2);
        memcpy(pBuf + buflen, &*src0.begin(), src0.size());

        buflen += src0.size();

        if (buflen >= len) {
            srcBuffer = (BYTE*) pBuf;
            BYTE *src_end = pBuf + len;

            if (sps > 0) {
                for (int y = 0; y < h; y++) {
                    for (int x = 0, w2 = w / sps; x < w2; x++) {
                        memcpy(src_end + sps * (h * x + ((h + 1) / 2) * (y & 1) + (y >> 1)), srcBuffer, sps);
                        srcBuffer += sps;
                    }
                }
                srcBuffer = pBuf + len;
                src_end = pBuf + len * 2;
            }
            srcBufferLength = (int)(src_end - srcBuffer);
            buflen = 0;
        } else {
            src0.clear();
            return S_OK;
        }
    }

    while (srcBufferLength > 0) {
        if ((codecId == AV_CODEC_ID_MLP || codecId == AV_CODEC_ID_TRUEHD) && srcBufferLength == 1) {
            break;    // workaround for silence when skipping TrueHD in MPC-HC
        }
        AVPacket avpkt;
        ffmpeg->av_init_packet(&avpkt);
        int parserBufferLength = AVCODEC_MAX_AUDIO_FRAME_SIZE, dstBufferLength = AVCODEC_MAX_AUDIO_FRAME_SIZE;
        uint8_t *parserBuffer = (uint8_t *)getDst(parserBufferLength);
        int16_t *dstBuffer = (int16_t *)getDst(dstBufferLength);
        int parsed_bytes = 0, decoded_bytes = 0;

        // Use parser if available
        if (parser) {
            // Parse the input buffer srcBuffer and put the parsed data in parserBuffer
            parsed_bytes = ffmpeg->av_parser_parse2(parser, avctx, &parserBuffer, &parserBufferLength, srcBuffer, srcBufferLength, AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);

            // If parserBufferLength = 0, nothing could be parsed
            if (parsed_bytes < 0 || (parsed_bytes == 0 && parserBufferLength == 0)) {
                break;
            }

            srcBufferLength -= parsed_bytes;
            srcBuffer += parsed_bytes;
            avpkt.data = parserBuffer;
            avpkt.size = parserBufferLength;

            // The source could be parsed, so decode it
            if (parserBufferLength > 0) {
                // Decode parsedBuffer to dstBuffer
                decoded_bytes = ffmpeg->avcodec_decode_audio3(avctx, dstBuffer, &dstBufferLength, &avpkt);

                // If nothing could be decoded, skip this data and continue
                if (decoded_bytes < 0 || (decoded_bytes == 0 && dstBufferLength == 0)) {
                    continue;
                }
            } else { // The source could not be parsed
                continue;
            }
        } else {
            avpkt.data = srcBuffer;
            avpkt.size = srcBufferLength;

            // Decode srcBuffer to dstBuffer
            decoded_bytes = ffmpeg->avcodec_decode_audio3(avctx, dstBuffer, &dstBufferLength, &avpkt);

            if (decoded_bytes < 0 || (decoded_bytes == 0 && dstBufferLength == 0)) {
                DPRINTF(_l("ffmpeg was unable to decode this frame"));
                TaudioParser *pAudioParser = NULL;
                this->sinkA->getAudioParser(&pAudioParser);
                if (pAudioParser != NULL) {
                    pAudioParser->NewSegment();
                }
                break;
            }

            srcBuffer += decoded_bytes;
            srcBufferLength -= decoded_bytes;
        }

        lastbps = avctx->bit_rate / 1000;
        bpssum += lastbps;
        numframes++;

        if (dstBufferLength > 0) {
            // Correct the output media type from what has been decoded
            //DPRINTF(_l("libavcodec channels=%d channel layout=%x,%x"), avctx->channels, avctx->channel_layout, get_lav_channel_layout(avctx->channel_layout));
            fmt.setChannels(avctx->channels, get_lav_channel_layout(avctx->channel_layout));
            fmt.freq = avctx->sample_rate;

            switch (avctx->sample_fmt) {
                case AV_SAMPLE_FMT_S16:
                    fmt.sf = TsampleFormat::SF_PCM16;
                    break;
                case AV_SAMPLE_FMT_S32:
                    fmt.sf = TsampleFormat::SF_PCM32;
                    break;
                case AV_SAMPLE_FMT_FLT:
                    fmt.sf = TsampleFormat::SF_FLOAT32;
                    break;
                case AV_SAMPLE_FMT_DBL:
                    fmt.sf = TsampleFormat::SF_FLOAT64;
                    break;
            }

            HRESULT hr = sinkA->deliverDecodedSample(dstBuffer, dstBufferLength / fmt.blockAlign(), fmt);
            if (hr != S_OK) {
                return hr;
            }
        }
    }
    src0.clear();
    return S_OK;
}

bool TaudioCodecLibavcodec::onSeek(REFERENCE_TIME segmentStart)
{
    buflen = 0;
    return avctx ? (ffmpeg->avcodec_flush_buffers(avctx), true) : false;
}
