/*
 * PCM codecs
 * Copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of Libav.
 *
 * Libav is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Libav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Libav; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * PCM codecs
 */

#include "avcodec.h"
#include "bytestream.h"
#include "internal.h"
#include "pcm_tablegen.h"

#define MAX_CHANNELS 64

typedef struct PCMDecode {
    AVFrame frame;
    short   table[256];
} PCMDecode;

static av_cold int pcm_decode_init(AVCodecContext *avctx)
{
    PCMDecode *s = avctx->priv_data;
    int i;

    if (avctx->channels <= 0 || avctx->channels > MAX_CHANNELS) {
        av_log(avctx, AV_LOG_ERROR, "PCM channels out of bounds\n");
        return AVERROR(EINVAL);
    }

    switch (avctx->codec->id) {
    case CODEC_ID_PCM_ALAW:
        for (i = 0; i < 256; i++)
            s->table[i] = alaw2linear(i);
        break;
    case CODEC_ID_PCM_MULAW:
        for (i = 0; i < 256; i++)
            s->table[i] = ulaw2linear(i);
        break;
    default:
        break;
    }

    avctx->sample_fmt = avctx->codec->sample_fmts[0];

    if (avctx->sample_fmt == AV_SAMPLE_FMT_S32)
        avctx->bits_per_raw_sample = av_get_bits_per_sample(avctx->codec->id);

    avcodec_get_frame_defaults(&s->frame);
    avctx->coded_frame = &s->frame;

    return 0;
}

/**
 * Read PCM samples macro
 * @param size   Data size of native machine format
 * @param endian bytestream_get_xxx() endian suffix
 * @param src    Source pointer (variable name)
 * @param dst    Destination pointer (variable name)
 * @param n      Total number of samples (variable name)
 * @param shift  Bitshift (bits)
 * @param offset Sample value offset
 */
#define DECODE(size, endian, src, dst, n, shift, offset)                \
    for (; n > 0; n--) {                                                \
        uint ## size ## _t v = bytestream_get_ ## endian(&src);         \
        AV_WN ## size ## A(dst, (v - offset) << shift);                 \
        dst += size / 8;                                                \
    }

static int pcm_decode_frame(AVCodecContext *avctx, void *data,
                            int *got_frame_ptr, AVPacket *avpkt)
{
    const uint8_t *src = avpkt->data;
    int buf_size       = avpkt->size;
    PCMDecode *s       = avctx->priv_data;
    int sample_size, c, n, ret, samples_per_block;
    uint8_t *samples;
    int32_t *dst_int32_t;

    sample_size = av_get_bits_per_sample(avctx->codec_id) / 8;

    samples_per_block = 1;

    if (sample_size == 0) {
        av_log(avctx, AV_LOG_ERROR, "Invalid sample_size\n");
        return AVERROR(EINVAL);
    }

    n = avctx->channels * sample_size;

    if (n && buf_size % n) {
        if (buf_size < n) {
            av_log(avctx, AV_LOG_ERROR, "invalid PCM packet\n");
            return -1;
        } else
            buf_size -= buf_size % n;
    }

    n = buf_size / sample_size;

    /* get output buffer */
    s->frame.nb_samples = n * samples_per_block / avctx->channels;
    if ((ret = avctx->get_buffer(avctx, &s->frame)) < 0) {
        av_log(avctx, AV_LOG_ERROR, "get_buffer() failed\n");
        return ret;
    }
    samples = s->frame.data[0];

    switch (avctx->codec->id) {
    case CODEC_ID_PCM_ALAW:
    case CODEC_ID_PCM_MULAW:
        for (; n > 0; n--) {
            AV_WN16A(samples, s->table[*src++]);
            samples += 2;
        }
        break;
    default:
        return -1;
    }

    *got_frame_ptr   = 1;
    *(AVFrame *)data = s->frame;

    return buf_size;
}

#if CONFIG_DECODERS
#define PCM_DECODER(id_, sample_fmt_, name_, long_name_)                    \
AVCodec ff_ ## name_ ## _decoder = {                                        \
    .name           = #name_,                                               \
    .type           = AVMEDIA_TYPE_AUDIO,                                   \
    .id             = id_,                                                  \
    .priv_data_size = sizeof(PCMDecode),                                    \
    .init           = pcm_decode_init,                                      \
    .decode         = pcm_decode_frame,                                     \
    .capabilities   = CODEC_CAP_DR1,                                        \
    .sample_fmts    = (const enum AVSampleFormat[]){ sample_fmt_,           \
                                                     AV_SAMPLE_FMT_NONE },  \
    .long_name      = NULL_IF_CONFIG_SMALL(long_name_),                     \
}
#else
#define PCM_DECODER(id, sample_fmt_, name, long_name_)
#endif

PCM_DECODER(CODEC_ID_PCM_ALAW,  AV_SAMPLE_FMT_S16, pcm_alaw,  "PCM A-law");
PCM_DECODER(CODEC_ID_PCM_MULAW, AV_SAMPLE_FMT_S16, pcm_mulaw, "PCM mu-law");
