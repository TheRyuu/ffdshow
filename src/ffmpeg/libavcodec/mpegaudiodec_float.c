/*
 * Float MPEG Audio decoder
 * Copyright (c) 2010 Michael Niedermayer
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

#define CONFIG_FLOAT 1
#include "mpegaudiodec.c"

#if CONFIG_MP1FLOAT_DECODER
AVCodec mp1float_decoder =
{
    "mp1float",
    AVMEDIA_TYPE_AUDIO,
    CODEC_ID_MP1,
    sizeof(MPADecodeContext),
    /*.init = */decode_init,
    /*.encode = */NULL,
    /*.close = */NULL,
    /*.decode = */decode_frame,
    /*.capabilities = */CODEC_CAP_PARSE_ONLY,
    /*.next = */NULL,
    /*.flush = */.flush= flush,
    /*.supported_framerates = */NULL,
    /*.pix_fmts = */NULL,
    /*.long_name = */.long_name= NULL_IF_CONFIG_SMALL("MP1 (MPEG audio layer 1)"),
};
#endif
#if CONFIG_MP2FLOAT_DECODER
AVCodec mp2float_decoder =
{
    "mp2float",
    AVMEDIA_TYPE_AUDIO,
    CODEC_ID_MP2,
    sizeof(MPADecodeContext),
    /*.init = */decode_init,
    /*.encode = */NULL,
    /*.close = */NULL,
    /*.decode = */decode_frame,
    /*.capabilities = */CODEC_CAP_PARSE_ONLY,
    /*.next = */NULL,
    /*.flush = */.flush= flush,
    /*.supported_framerates = */NULL,
    /*.pix_fmts = */NULL,
    /*.long_name = */.long_name= NULL_IF_CONFIG_SMALL("MP2 (MPEG audio layer 2)"),
};
#endif
#if CONFIG_MP3FLOAT_DECODER
AVCodec mp3float_decoder =
{
    "mp3float",
    AVMEDIA_TYPE_AUDIO,
    CODEC_ID_MP3,
    sizeof(MPADecodeContext),
    /*.init = */decode_init,
    /*.encode = */NULL,
    /*.close = */NULL,
    /*.decode = */decode_frame,
    /*.capabilities = */CODEC_CAP_PARSE_ONLY,
    /*.next = */NULL,
    /*.flush = */.flush= flush,
    /*.supported_framerates = */NULL,
    /*.pix_fmts = */NULL,
    /*.long_name = */.long_name= NULL_IF_CONFIG_SMALL("MP3 (MPEG audio layer 3)"),
};
#endif
