/*
 * Copyright (C) 2006-2010 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of Libav.
 *
 * Libav is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Libav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Libav; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef AVFILTER_VF_YADIF_H_
#define AVFILTER_VF_YADIF_H_

#include "../../imgFilters/ffImgfmt.h"
#include "libavutil/pixdesc.h"
#include "avfilter.h"

typedef struct YadifThreadContext{
    struct YADIFContext *yadctx;
    int plane_start;
    int plane_end;
    int y_start[3];
    int y_end[3];
} YadifThreadContext;

/**
 * YADIFContext
 */
typedef struct YADIFContext {
    int field_order_mode;
    int64_t buffered_rtStart;
    int64_t buffered_rtStop;
    int64_t frame_duration;
    stride_t stride[3];
    uint8_t *ref[4][3];
    unsigned int shiftX[4],shiftY[4];

    /**
     * do_deinterlace
     * 0:not initialized
     * 1:before buffuring the first frame
     * 2:first frame buffered, input the first frame again
     * 3:input the second frame
     * 4:normal (running)
     */
    int do_deinterlace;

    /*
     * Thread stuffs
     */
    int thread_count;
    void *thread_opaque;
    int (*execute)(struct YADIFContext *yadctx, int (*func)(YadifThreadContext *yadThreadCtx), int count);
    YadifThreadContext *threadCtx;

    /*
     * temporary & private stuffs
     * parameters to be handed to threads
     */
    uint8_t **dst;
    stride_t *dst_stride;
    int width;
    int height;
    int tff;

    /**
     * 0: send 1 frame for each frame
     * 1: send 1 frame for each field
     * 2: like 0 but skips spatial interlacing check
     * 3: like 1 but skips spatial interlacing check
     */
    int mode;

    /**
     *  0: top field first
     *  1: bottom field first
     * -1: auto-detection
     */
    int parity;

    int frame_pending;

    AVFilterBufferRef *cur;
    AVFilterBufferRef *next;
    AVFilterBufferRef *prev;
    AVFilterBufferRef *out;
    void (*filter_line)(uint8_t *dst,
                        uint8_t *prev, uint8_t *cur, uint8_t *next,
                        int w, int prefs, int mrefs, int parity, int mode);

    const AVPixFmtDescriptor *csp;
} YADIFContext;

void yadif_filter(YADIFContext *p, uint8_t *dst[3], stride_t dst_stride[3], int width, int height, int parity, int tff);
void yadif_init(YADIFContext *yadctx);
void yadif_uninit(YADIFContext *yadctx);

#endif // AVFILTER_VF_YADIF_H_
