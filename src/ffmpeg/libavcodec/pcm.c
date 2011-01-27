/*
 * PCM codecs
 * Copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * PCM codecs
 */

#include "avcodec.h"
#include "pcm_tablegen.h"

#define MAX_CHANNELS 64

typedef struct PCMDecode {
    short table[256];
} PCMDecode;

static av_cold int pcm_decode_init(AVCodecContext * avctx)
{
    PCMDecode *s = avctx->priv_data;
    int i;

    switch(avctx->codec->id) {
    case CODEC_ID_PCM_ALAW:
        for(i=0;i<256;i++)
            s->table[i] = alaw2linear(i);
        break;
    case CODEC_ID_PCM_MULAW:
        for(i=0;i<256;i++)
            s->table[i] = ulaw2linear(i);
        break;
    default:
        break;
    }

    avctx->sample_fmt = avctx->codec->sample_fmts[0];

    if (avctx->sample_fmt == AV_SAMPLE_FMT_S32)
        avctx->bits_per_raw_sample = av_get_bits_per_sample(avctx->codec->id);

    return 0;
}

static int pcm_decode_frame(AVCodecContext *avctx,
                            void *data, int *data_size,
                            AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    PCMDecode *s = avctx->priv_data;
    int sample_size, n;
    short *samples;
    const uint8_t *src;

    samples = data;
    src = buf;

    if (avctx->sample_fmt!=avctx->codec->sample_fmts[0]) {
        av_log(avctx, AV_LOG_ERROR, "invalid sample_fmt\n");
        return -1;
    }

    if(avctx->channels <= 0 || avctx->channels > MAX_CHANNELS){
        av_log(avctx, AV_LOG_ERROR, "PCM channels out of bounds\n");
        return -1;
    }

    sample_size = av_get_bits_per_sample(avctx->codec_id)/8;

    if (sample_size == 0) {
        av_log(avctx, AV_LOG_ERROR, "Invalid sample_size\n");
        return AVERROR(EINVAL);
    }

    n = avctx->channels * sample_size;

    if(n && buf_size % n){
        if (buf_size < n) {
            av_log(avctx, AV_LOG_ERROR, "invalid PCM packet\n");
            return -1;
        }else
            buf_size -= buf_size % n;
    }

    buf_size= FFMIN(buf_size, *data_size/2);
    *data_size=0;

    n = buf_size/sample_size;

    switch(avctx->codec->id) {
    case CODEC_ID_PCM_ALAW:
    case CODEC_ID_PCM_MULAW:
        for(;n>0;n--) {
            *samples++ = s->table[*src++];
        }
        break;
    default:
        return -1;
    }
    *data_size = (uint8_t *)samples - (uint8_t *)data;
    return src - buf;
}

#if CONFIG_DECODERS
#define PCM_DECODER(id_,sample_fmt_,name_,long_name_)         \
AVCodec ff_ ## name_ ## _decoder = {            \
    .name           = #name_,                   \
    .type           = AVMEDIA_TYPE_AUDIO,       \
    .id             = id_,                      \
    .priv_data_size = sizeof(PCMDecode),        \
    .init           = pcm_decode_init,          \
    .decode         = pcm_decode_frame,         \
    .sample_fmts = (const enum AVSampleFormat[]){sample_fmt_,AV_SAMPLE_FMT_NONE}, \
    .long_name = NULL_IF_CONFIG_SMALL(long_name_), \
};
#else
#define PCM_DECODER(id,sample_fmt_,name,long_name_)
#endif

PCM_DECODER(CODEC_ID_PCM_ALAW,  AV_SAMPLE_FMT_S16, pcm_alaw,  "A-law PCM" );
PCM_DECODER(CODEC_ID_PCM_MULAW, AV_SAMPLE_FMT_S16, pcm_mulaw, "mu-law PCM");
