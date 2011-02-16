/*
 * Misc image conversion routines
 * Copyright (c) 2001, 2002, 2003 Fabrice Bellard
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
 * misc image conversion routines
 */

/* TODO:
 * - write 'ffimg' program to test all the image related stuff
 * - move all api to slice based system
 * - integrate deinterlacing, postprocessing and scaling in the conversion process
 */

#include "avcodec.h"
#include "dsputil.h"
#include "internal.h"
#include "imgconvert.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"

#if HAVE_MMX && HAVE_YASM
#include "x86/dsputil_mmx.h"
#endif

#define xglue(x, y) x ## y
#define glue(x, y) xglue(x, y)

#define FF_COLOR_RGB      0 /**< RGB color space */
#define FF_COLOR_GRAY     1 /**< gray color space */
#define FF_COLOR_YUV      2 /**< YUV color space. 16 <= Y <= 235, 16 <= U, V <= 240 */
#define FF_COLOR_YUV_JPEG 3 /**< YUV color space. 0 <= Y <= 255, 0 <= U, V <= 255 */

#define FF_PIXEL_PLANAR   0 /**< each channel has one component in AVPicture */
#define FF_PIXEL_PACKED   1 /**< only one components containing all the channels */
#define FF_PIXEL_PALETTE  2  /**< one components containing indexes for a palette */

typedef struct PixFmtInfo {
    uint8_t nb_channels;     /**< number of channels (including alpha) */
    uint8_t color_type;      /**< color type (see FF_COLOR_xxx constants) */
    uint8_t pixel_type;      /**< pixel storage type (see FF_PIXEL_xxx constants) */
    uint8_t is_alpha : 1;    /**< true if alpha can be specified */
    uint8_t depth;           /**< bit depth of the color components */
} PixFmtInfo;

/* this table gives more information about formats */
static const PixFmtInfo pix_fmt_info[PIX_FMT_NB] = {
    /* YUV formats */
    [PIX_FMT_YUV420P] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 8,
    },
    [PIX_FMT_YUV422P] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 8,
    },
    [PIX_FMT_YUV444P] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 8,
    },
    [PIX_FMT_YUYV422] = {
        .nb_channels = 1,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 8,
    },
    [PIX_FMT_UYVY422] = {
        .nb_channels = 1,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 8,
    },
    [PIX_FMT_YUV410P] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 8,
    },
    [PIX_FMT_YUV411P] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 8,
    },
    [PIX_FMT_YUV440P] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 8,
    },
    [PIX_FMT_YUV420P16LE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 16,
    },
    [PIX_FMT_YUV422P16LE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 16,
    },
    [PIX_FMT_YUV444P16LE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 16,
    },
    [PIX_FMT_YUV420P16BE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 16,
    },
    [PIX_FMT_YUV422P16BE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 16,
    },
    [PIX_FMT_YUV444P16BE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 16,
    },


    /* YUV formats with alpha plane */
    [PIX_FMT_YUVA420P] = {
        .nb_channels = 4,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 8,
    },

    /* JPEG YUV */
    [PIX_FMT_YUVJ420P] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_YUV_JPEG,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 8,
    },
    [PIX_FMT_YUVJ422P] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_YUV_JPEG,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 8,
    },
    [PIX_FMT_YUVJ444P] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_YUV_JPEG,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 8,
    },
    [PIX_FMT_YUVJ440P] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_YUV_JPEG,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 8,
    },

    /* RGB formats */
    [PIX_FMT_RGB24] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 8,
    },
    [PIX_FMT_BGR24] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 8,
    },
    [PIX_FMT_ARGB] = {
        .nb_channels = 4, .is_alpha = 1,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 8,
    },
    [PIX_FMT_RGB48BE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 16,
    },
    [PIX_FMT_RGB48LE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 16,
    },
    [PIX_FMT_RGB565BE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 5,
    },
    [PIX_FMT_RGB565LE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 5,
    },
    [PIX_FMT_RGB555BE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 5,
    },
    [PIX_FMT_RGB555LE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 5,
    },
    [PIX_FMT_RGB444BE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 4,
    },
    [PIX_FMT_RGB444LE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 4,
    },

    /* gray / mono formats */
    [PIX_FMT_GRAY16BE] = {
        .nb_channels = 1,
        .color_type = FF_COLOR_GRAY,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 16,
    },
    [PIX_FMT_GRAY16LE] = {
        .nb_channels = 1,
        .color_type = FF_COLOR_GRAY,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 16,
    },
    [PIX_FMT_GRAY8] = {
        .nb_channels = 1,
        .color_type = FF_COLOR_GRAY,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 8,
    },
    [PIX_FMT_MONOWHITE] = {
        .nb_channels = 1,
        .color_type = FF_COLOR_GRAY,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 1,
    },
    [PIX_FMT_MONOBLACK] = {
        .nb_channels = 1,
        .color_type = FF_COLOR_GRAY,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 1,
    },

    /* paletted formats */
    [PIX_FMT_PAL8] = {
        .nb_channels = 4, .is_alpha = 1,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PALETTE,
        .depth = 8,
    },
    [PIX_FMT_UYYVYY411] = {
        .nb_channels = 1,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 8,
    },
    [PIX_FMT_ABGR] = {
        .nb_channels = 4, .is_alpha = 1,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 8,
    },
    [PIX_FMT_BGR565BE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 5,
    },
    [PIX_FMT_BGR565LE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 5,
    },
    [PIX_FMT_BGR555BE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 5,
    },
    [PIX_FMT_BGR555LE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 5,
    },
    [PIX_FMT_BGR444BE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 4,
    },
    [PIX_FMT_BGR444LE] = {
        .nb_channels = 3,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 4,
    },
    [PIX_FMT_RGB8] = {
        .nb_channels = 1,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 8,
    },
    [PIX_FMT_RGB4] = {
        .nb_channels = 1,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 4,
    },
    [PIX_FMT_RGB4_BYTE] = {
        .nb_channels = 1,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 8,
    },
    [PIX_FMT_BGR8] = {
        .nb_channels = 1,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 8,
    },
    [PIX_FMT_BGR4] = {
        .nb_channels = 1,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 4,
    },
    [PIX_FMT_BGR4_BYTE] = {
        .nb_channels = 1,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 8,
    },
    [PIX_FMT_NV12] = {
        .nb_channels = 2,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 8,
    },
    [PIX_FMT_NV21] = {
        .nb_channels = 2,
        .color_type = FF_COLOR_YUV,
        .pixel_type = FF_PIXEL_PLANAR,
        .depth = 8,
    },

    [PIX_FMT_BGRA] = {
        .nb_channels = 4, .is_alpha = 1,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 8,
    },
    [PIX_FMT_RGBA] = {
        .nb_channels = 4, .is_alpha = 1,
        .color_type = FF_COLOR_RGB,
        .pixel_type = FF_PIXEL_PACKED,
        .depth = 8,
    },
};

void avcodec_get_chroma_sub_sample(enum PixelFormat pix_fmt, int *h_shift, int *v_shift)
{
    *h_shift = av_pix_fmt_descriptors[pix_fmt].log2_chroma_w;
    *v_shift = av_pix_fmt_descriptors[pix_fmt].log2_chroma_h;
}

const char *avcodec_get_pix_fmt_name(enum PixelFormat pix_fmt)
{
    if ((unsigned)pix_fmt >= PIX_FMT_NB)
        return NULL;
    else
        return av_pix_fmt_descriptors[pix_fmt].name;
}

int ff_is_hwaccel_pix_fmt(enum PixelFormat pix_fmt)
{
    return av_pix_fmt_descriptors[pix_fmt].flags & PIX_FMT_HWACCEL;
}

void av_picture_copy(AVPicture *dst, const AVPicture *src,
                     enum PixelFormat pix_fmt, int width, int height)
{
    av_image_copy(dst->data, dst->linesize, src->data,
                  src->linesize, pix_fmt, width, height);
}
