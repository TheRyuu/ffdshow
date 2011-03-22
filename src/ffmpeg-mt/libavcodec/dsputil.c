/*
 * DSP utils
 * Copyright (c) 2000, 2001 Fabrice Bellard
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
 *
 * gmc & q-pel & 32/64 bit based MC by Michael Niedermayer <michaelni@gmx.at>
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
 * DSP utils
 */

#include "libavutil/imgutils.h"
#include "avcodec.h"
#include "dsputil.h"
#include "simple_idct.h"
#include "mathops.h"
#include "mpegvideo.h"
#include "config.h"

uint8_t ff_cropTbl[256 + 2 * MAX_NEG_CROP] = {0, };
uint32_t ff_squareTbl[512] = {0, };

// 0x7f7f7f7f or 0x7f7f7f7f7f7f7f7f or whatever, depending on the cpu's native arithmetic size
#define pb_7f (~0UL/255 * 0x7f)
#define pb_80 (~0UL/255 * 0x80)

const uint8_t ff_zigzag_direct[64] = {
    0,   1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

/* not permutated inverse zigzag_direct + 1 for MMX quantizer */
DECLARE_ALIGNED(16, uint16_t, inv_zigzag_direct16)[64];

const uint8_t ff_alternate_horizontal_scan[64] = {
    0,  1,   2,  3,  8,  9, 16, 17,
    10, 11,  4,  5,  6,  7, 15, 14,
    13, 12, 19, 18, 24, 25, 32, 33,
    26, 27, 20, 21, 22, 23, 28, 29,
    30, 31, 34, 35, 40, 41, 48, 49,
    42, 43, 36, 37, 38, 39, 44, 45,
    46, 47, 50, 51, 56, 57, 58, 59,
    52, 53, 54, 55, 60, 61, 62, 63,
};

const uint8_t ff_alternate_vertical_scan[64] = {
    0,  8,  16, 24,  1,  9,  2, 10,
    17, 25, 32, 40, 48, 56, 57, 49,
    41, 33, 26, 18,  3, 11,  4, 12,
    19, 27, 34, 42, 50, 58, 35, 43,
    51, 59, 20, 28,  5, 13,  6, 14,
    21, 29, 36, 44, 52, 60, 37, 45,
    53, 61, 22, 30,  7, 15, 23, 31,
    38, 46, 54, 62, 39, 47, 55, 63,
};

/* Input permutation for the simple_idct_mmx */
static const uint8_t simple_mmx_permutation[64]={
        0x00, 0x08, 0x04, 0x09, 0x01, 0x0C, 0x05, 0x0D,
        0x10, 0x18, 0x14, 0x19, 0x11, 0x1C, 0x15, 0x1D,
        0x20, 0x28, 0x24, 0x29, 0x21, 0x2C, 0x25, 0x2D,
        0x12, 0x1A, 0x16, 0x1B, 0x13, 0x1E, 0x17, 0x1F,
        0x02, 0x0A, 0x06, 0x0B, 0x03, 0x0E, 0x07, 0x0F,
        0x30, 0x38, 0x34, 0x39, 0x31, 0x3C, 0x35, 0x3D,
        0x22, 0x2A, 0x26, 0x2B, 0x23, 0x2E, 0x27, 0x2F,
        0x32, 0x3A, 0x36, 0x3B, 0x33, 0x3E, 0x37, 0x3F,
};

static const uint8_t idct_sse2_row_perm[8] = {0, 4, 1, 5, 2, 6, 3, 7};

void ff_init_scantable(uint8_t *permutation, ScanTable *st, const uint8_t *src_scantable){
    int i;
    int end;

    st->scantable= src_scantable;

    for(i=0; i<64; i++){
        int j;
        j = src_scantable[i];
        st->permutated[i] = permutation[j];
#if ARCH_PPC
        st->inverse[j] = i;
#endif
    }

    end=-1;
    for(i=0; i<64; i++){
        int j;
        j = st->permutated[i];
        if(j>end) end=j;
        st->raster_end[i]= end;
    }
}

static void bswap_buf(uint32_t *dst, const uint32_t *src, int w){
    int i;

    for(i=0; i+8<=w; i+=8){
        dst[i+0]= av_bswap32(src[i+0]);
        dst[i+1]= av_bswap32(src[i+1]);
        dst[i+2]= av_bswap32(src[i+2]);
        dst[i+3]= av_bswap32(src[i+3]);
        dst[i+4]= av_bswap32(src[i+4]);
        dst[i+5]= av_bswap32(src[i+5]);
        dst[i+6]= av_bswap32(src[i+6]);
        dst[i+7]= av_bswap32(src[i+7]);
    }
    for(;i<w; i++){
        dst[i+0]= av_bswap32(src[i+0]);
    }
}

/* draw the edges of width 'w' of an image of size width, height */
//FIXME check that this is ok for mpeg4 interlaced
static void draw_edges_c(uint8_t *buf, int wrap, int width, int height, int w, int sides)
{
    uint8_t *ptr, *last_line;
    int i;

    last_line = buf + (height - 1) * wrap;
    for(i=0;i<w;i++) {
        /* top and bottom */
        if (sides&EDGE_TOP)    memcpy(buf - (i + 1) * wrap, buf, width);
        if (sides&EDGE_BOTTOM) memcpy(last_line + (i + 1) * wrap, last_line, width);
    }
    /* left and right */
    ptr = buf;
    for(i=0;i<height;i++) {
        memset(ptr - w, ptr[0], w);
        memset(ptr + width, ptr[width-1], w);
        ptr += wrap;
    }
    /* corners */
    for(i=0;i<w;i++) {
        if (sides&EDGE_TOP) {
            memset(buf - (i + 1) * wrap - w, buf[0], w); /* top left */
            memset(buf - (i + 1) * wrap + width, buf[width-1], w); /* top right */
        }

        if (sides&EDGE_BOTTOM) {
            memset(last_line + (i + 1) * wrap - w, last_line[0], w); /* top left */
            memset(last_line + (i + 1) * wrap + width, last_line[width-1], w); /* top right */
        }
    }
}

/**
 * Copy a rectangular area of samples to a temporary buffer and replicate the border samples.
 * @param buf destination buffer
 * @param src source buffer
 * @param linesize number of bytes between 2 vertically adjacent samples in both the source and destination buffers
 * @param block_w width of block
 * @param block_h height of block
 * @param src_x x coordinate of the top left sample of the block in the source buffer
 * @param src_y y coordinate of the top left sample of the block in the source buffer
 * @param w width of the source buffer
 * @param h height of the source buffer
 */
void ff_emulated_edge_mc(uint8_t *buf, const uint8_t *src, int linesize, int block_w, int block_h,
                                    int src_x, int src_y, int w, int h){
    int x, y;
    int start_y, start_x, end_y, end_x;

    if(src_y>= h){
        src+= (h-1-src_y)*linesize;
        src_y=h-1;
    }else if(src_y<=-block_h){
        src+= (1-block_h-src_y)*linesize;
        src_y=1-block_h;
    }
    if(src_x>= w){
        src+= (w-1-src_x);
        src_x=w-1;
    }else if(src_x<=-block_w){
        src+= (1-block_w-src_x);
        src_x=1-block_w;
    }

    start_y= FFMAX(0, -src_y);
    start_x= FFMAX(0, -src_x);
    end_y= FFMIN(block_h, h-src_y);
    end_x= FFMIN(block_w, w-src_x);
    assert(start_y < end_y && block_h);
    assert(start_x < end_x && block_w);

    w    = end_x - start_x;
    src += start_y*linesize + start_x;
    buf += start_x;

    //top
    for(y=0; y<start_y; y++){
        memcpy(buf, src, w);
        buf += linesize;
    }

    // copy existing part
    for(; y<end_y; y++){
        memcpy(buf, src, w);
        src += linesize;
        buf += linesize;
    }

    //bottom
    src -= linesize;
    for(; y<block_h; y++){
        memcpy(buf, src, w);
        buf += linesize;
    }

    buf -= block_h * linesize + start_x;
    while (block_h--){
       //left
        for(x=0; x<start_x; x++){
            buf[x] = buf[start_x];
        }

       //right
        for(x=end_x; x<block_w; x++){
            buf[x] = buf[end_x - 1];
        }
        buf += linesize;
    }
}

void ff_put_pixels_clamped_c(const DCTELEM *block, uint8_t *restrict pixels,
                             int line_size)
{
    int i;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    /* read the pixels */
    for(i=0;i<8;i++) {
        pixels[0] = cm[block[0]];
        pixels[1] = cm[block[1]];
        pixels[2] = cm[block[2]];
        pixels[3] = cm[block[3]];
        pixels[4] = cm[block[4]];
        pixels[5] = cm[block[5]];
        pixels[6] = cm[block[6]];
        pixels[7] = cm[block[7]];

        pixels += line_size;
        block += 8;
    }
}

static void put_pixels_clamped4_c(const DCTELEM *block, uint8_t *restrict pixels,
                                 int line_size)
{
    int i;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    /* read the pixels */
    for(i=0;i<4;i++) {
        pixels[0] = cm[block[0]];
        pixels[1] = cm[block[1]];
        pixels[2] = cm[block[2]];
        pixels[3] = cm[block[3]];

        pixels += line_size;
        block += 8;
    }
}

static void put_pixels_clamped2_c(const DCTELEM *block, uint8_t *restrict pixels,
                                 int line_size)
{
    int i;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    /* read the pixels */
    for(i=0;i<2;i++) {
        pixels[0] = cm[block[0]];
        pixels[1] = cm[block[1]];

        pixels += line_size;
        block += 8;
    }
}

void ff_put_signed_pixels_clamped_c(const DCTELEM *block,
                                    uint8_t *restrict pixels,
                                    int line_size)
{
    int i, j;

    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            if (*block < -128)
                *pixels = 0;
            else if (*block > 127)
                *pixels = 255;
            else
                *pixels = (uint8_t)(*block + 128);
            block++;
            pixels++;
        }
        pixels += (line_size - 8);
    }
}

void ff_add_pixels_clamped_c(const DCTELEM *block, uint8_t *restrict pixels,
                             int line_size)
{
    int i;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    /* read the pixels */
    for(i=0;i<8;i++) {
        pixels[0] = cm[pixels[0] + block[0]];
        pixels[1] = cm[pixels[1] + block[1]];
        pixels[2] = cm[pixels[2] + block[2]];
        pixels[3] = cm[pixels[3] + block[3]];
        pixels[4] = cm[pixels[4] + block[4]];
        pixels[5] = cm[pixels[5] + block[5]];
        pixels[6] = cm[pixels[6] + block[6]];
        pixels[7] = cm[pixels[7] + block[7]];
        pixels += line_size;
        block += 8;
    }
}

static void add_pixels_clamped4_c(const DCTELEM *block, uint8_t *restrict pixels,
                          int line_size)
{
    int i;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    /* read the pixels */
    for(i=0;i<4;i++) {
        pixels[0] = cm[pixels[0] + block[0]];
        pixels[1] = cm[pixels[1] + block[1]];
        pixels[2] = cm[pixels[2] + block[2]];
        pixels[3] = cm[pixels[3] + block[3]];
        pixels += line_size;
        block += 8;
    }
}

static void add_pixels_clamped2_c(const DCTELEM *block, uint8_t *restrict pixels,
                          int line_size)
{
    int i;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    /* read the pixels */
    for(i=0;i<2;i++) {
        pixels[0] = cm[pixels[0] + block[0]];
        pixels[1] = cm[pixels[1] + block[1]];
        pixels += line_size;
        block += 8;
    }
}

static void add_pixels8_c(uint8_t *restrict pixels, DCTELEM *block, int line_size)
{
    int i;
    for(i=0;i<8;i++) {
        pixels[0] += block[0];
        pixels[1] += block[1];
        pixels[2] += block[2];
        pixels[3] += block[3];
        pixels[4] += block[4];
        pixels[5] += block[5];
        pixels[6] += block[6];
        pixels[7] += block[7];
        pixels += line_size;
        block += 8;
    }
}

static void add_pixels4_c(uint8_t *restrict pixels, DCTELEM *block, int line_size)
{
    int i;
    for(i=0;i<4;i++) {
        pixels[0] += block[0];
        pixels[1] += block[1];
        pixels[2] += block[2];
        pixels[3] += block[3];
        pixels += line_size;
        block += 4;
    }
}

#if 0

#define PIXOP2(OPNAME, OP) \
static void OPNAME ## _pixels(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
    int i;\
    for(i=0; i<h; i++){\
        OP(*((uint64_t*)block), AV_RN64(pixels));\
        pixels+=line_size;\
        block +=line_size;\
    }\
}\
\
static void OPNAME ## _no_rnd_pixels_x2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
    int i;\
    for(i=0; i<h; i++){\
        const uint64_t a= AV_RN64(pixels  );\
        const uint64_t b= AV_RN64(pixels+1);\
        OP(*((uint64_t*)block), (a&b) + (((a^b)&0xFEFEFEFEFEFEFEFEULL)>>1));\
        pixels+=line_size;\
        block +=line_size;\
    }\
}\
\
static void OPNAME ## _pixels_x2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
    int i;\
    for(i=0; i<h; i++){\
        const uint64_t a= AV_RN64(pixels  );\
        const uint64_t b= AV_RN64(pixels+1);\
        OP(*((uint64_t*)block), (a|b) - (((a^b)&0xFEFEFEFEFEFEFEFEULL)>>1));\
        pixels+=line_size;\
        block +=line_size;\
    }\
}\
\
static void OPNAME ## _no_rnd_pixels_y2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
    int i;\
    for(i=0; i<h; i++){\
        const uint64_t a= AV_RN64(pixels          );\
        const uint64_t b= AV_RN64(pixels+line_size);\
        OP(*((uint64_t*)block), (a&b) + (((a^b)&0xFEFEFEFEFEFEFEFEULL)>>1));\
        pixels+=line_size;\
        block +=line_size;\
    }\
}\
\
static void OPNAME ## _pixels_y2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
    int i;\
    for(i=0; i<h; i++){\
        const uint64_t a= AV_RN64(pixels          );\
        const uint64_t b= AV_RN64(pixels+line_size);\
        OP(*((uint64_t*)block), (a|b) - (((a^b)&0xFEFEFEFEFEFEFEFEULL)>>1));\
        pixels+=line_size;\
        block +=line_size;\
    }\
}\
\
static void OPNAME ## _pixels_xy2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
        int i;\
        const uint64_t a= AV_RN64(pixels  );\
        const uint64_t b= AV_RN64(pixels+1);\
        uint64_t l0=  (a&0x0303030303030303ULL)\
                    + (b&0x0303030303030303ULL)\
                    + 0x0202020202020202ULL;\
        uint64_t h0= ((a&0xFCFCFCFCFCFCFCFCULL)>>2)\
                   + ((b&0xFCFCFCFCFCFCFCFCULL)>>2);\
        uint64_t l1,h1;\
\
        pixels+=line_size;\
        for(i=0; i<h; i+=2){\
            uint64_t a= AV_RN64(pixels  );\
            uint64_t b= AV_RN64(pixels+1);\
            l1=  (a&0x0303030303030303ULL)\
               + (b&0x0303030303030303ULL);\
            h1= ((a&0xFCFCFCFCFCFCFCFCULL)>>2)\
              + ((b&0xFCFCFCFCFCFCFCFCULL)>>2);\
            OP(*((uint64_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0F0F0F0F0FULL));\
            pixels+=line_size;\
            block +=line_size;\
            a= AV_RN64(pixels  );\
            b= AV_RN64(pixels+1);\
            l0=  (a&0x0303030303030303ULL)\
               + (b&0x0303030303030303ULL)\
               + 0x0202020202020202ULL;\
            h0= ((a&0xFCFCFCFCFCFCFCFCULL)>>2)\
              + ((b&0xFCFCFCFCFCFCFCFCULL)>>2);\
            OP(*((uint64_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0F0F0F0F0FULL));\
            pixels+=line_size;\
            block +=line_size;\
        }\
}\
\
static void OPNAME ## _no_rnd_pixels_xy2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
        int i;\
        const uint64_t a= AV_RN64(pixels  );\
        const uint64_t b= AV_RN64(pixels+1);\
        uint64_t l0=  (a&0x0303030303030303ULL)\
                    + (b&0x0303030303030303ULL)\
                    + 0x0101010101010101ULL;\
        uint64_t h0= ((a&0xFCFCFCFCFCFCFCFCULL)>>2)\
                   + ((b&0xFCFCFCFCFCFCFCFCULL)>>2);\
        uint64_t l1,h1;\
\
        pixels+=line_size;\
        for(i=0; i<h; i+=2){\
            uint64_t a= AV_RN64(pixels  );\
            uint64_t b= AV_RN64(pixels+1);\
            l1=  (a&0x0303030303030303ULL)\
               + (b&0x0303030303030303ULL);\
            h1= ((a&0xFCFCFCFCFCFCFCFCULL)>>2)\
              + ((b&0xFCFCFCFCFCFCFCFCULL)>>2);\
            OP(*((uint64_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0F0F0F0F0FULL));\
            pixels+=line_size;\
            block +=line_size;\
            a= AV_RN64(pixels  );\
            b= AV_RN64(pixels+1);\
            l0=  (a&0x0303030303030303ULL)\
               + (b&0x0303030303030303ULL)\
               + 0x0101010101010101ULL;\
            h0= ((a&0xFCFCFCFCFCFCFCFCULL)>>2)\
              + ((b&0xFCFCFCFCFCFCFCFCULL)>>2);\
            OP(*((uint64_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0F0F0F0F0FULL));\
            pixels+=line_size;\
            block +=line_size;\
        }\
}\
\
CALL_2X_PIXELS(OPNAME ## _pixels16_c    , OPNAME ## _pixels_c    , 8)\
CALL_2X_PIXELS(OPNAME ## _pixels16_x2_c , OPNAME ## _pixels_x2_c , 8)\
CALL_2X_PIXELS(OPNAME ## _pixels16_y2_c , OPNAME ## _pixels_y2_c , 8)\
CALL_2X_PIXELS(OPNAME ## _pixels16_xy2_c, OPNAME ## _pixels_xy2_c, 8)\
CALL_2X_PIXELS(OPNAME ## _no_rnd_pixels16_x2_c , OPNAME ## _no_rnd_pixels_x2_c , 8)\
CALL_2X_PIXELS(OPNAME ## _no_rnd_pixels16_y2_c , OPNAME ## _no_rnd_pixels_y2_c , 8)\
CALL_2X_PIXELS(OPNAME ## _no_rnd_pixels16_xy2_c, OPNAME ## _no_rnd_pixels_xy2_c, 8)

#define op_avg(a, b) a = ( ((a)|(b)) - ((((a)^(b))&0xFEFEFEFEFEFEFEFEULL)>>1) )
#else // 64 bit variant

#define PIXOP2(OPNAME, OP) \
static void OPNAME ## _pixels2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    int i;\
    for(i=0; i<h; i++){\
        OP(*((uint16_t*)(block  )), AV_RN16(pixels  ));\
        pixels+=line_size;\
        block +=line_size;\
    }\
}\
static void OPNAME ## _pixels4_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    int i;\
    for(i=0; i<h; i++){\
        OP(*((uint32_t*)(block  )), AV_RN32(pixels  ));\
        pixels+=line_size;\
        block +=line_size;\
    }\
}\
static void OPNAME ## _pixels8_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    int i;\
    for(i=0; i<h; i++){\
        OP(*((uint32_t*)(block  )), AV_RN32(pixels  ));\
        OP(*((uint32_t*)(block+4)), AV_RN32(pixels+4));\
        pixels+=line_size;\
        block +=line_size;\
    }\
}\
static inline void OPNAME ## _no_rnd_pixels8_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _pixels8_c(block, pixels, line_size, h);\
}\
\
static inline void OPNAME ## _no_rnd_pixels8_l2(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, int dst_stride, \
                                                int src_stride1, int src_stride2, int h){\
    int i;\
    for(i=0; i<h; i++){\
        uint32_t a,b;\
        a= AV_RN32(&src1[i*src_stride1  ]);\
        b= AV_RN32(&src2[i*src_stride2  ]);\
        OP(*((uint32_t*)&dst[i*dst_stride  ]), no_rnd_avg32(a, b));\
        a= AV_RN32(&src1[i*src_stride1+4]);\
        b= AV_RN32(&src2[i*src_stride2+4]);\
        OP(*((uint32_t*)&dst[i*dst_stride+4]), no_rnd_avg32(a, b));\
    }\
}\
\
static inline void OPNAME ## _pixels8_l2(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, int dst_stride, \
                                                int src_stride1, int src_stride2, int h){\
    int i;\
    for(i=0; i<h; i++){\
        uint32_t a,b;\
        a= AV_RN32(&src1[i*src_stride1  ]);\
        b= AV_RN32(&src2[i*src_stride2  ]);\
        OP(*((uint32_t*)&dst[i*dst_stride  ]), rnd_avg32(a, b));\
        a= AV_RN32(&src1[i*src_stride1+4]);\
        b= AV_RN32(&src2[i*src_stride2+4]);\
        OP(*((uint32_t*)&dst[i*dst_stride+4]), rnd_avg32(a, b));\
    }\
}\
\
static inline void OPNAME ## _pixels4_l2(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, int dst_stride, \
                                                int src_stride1, int src_stride2, int h){\
    int i;\
    for(i=0; i<h; i++){\
        uint32_t a,b;\
        a= AV_RN32(&src1[i*src_stride1  ]);\
        b= AV_RN32(&src2[i*src_stride2  ]);\
        OP(*((uint32_t*)&dst[i*dst_stride  ]), rnd_avg32(a, b));\
    }\
}\
\
static inline void OPNAME ## _pixels2_l2(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, int dst_stride, \
                                                int src_stride1, int src_stride2, int h){\
    int i;\
    for(i=0; i<h; i++){\
        uint32_t a,b;\
        a= AV_RN16(&src1[i*src_stride1  ]);\
        b= AV_RN16(&src2[i*src_stride2  ]);\
        OP(*((uint16_t*)&dst[i*dst_stride  ]), rnd_avg32(a, b));\
    }\
}\
\
static inline void OPNAME ## _pixels16_l2(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, int dst_stride, \
                                                int src_stride1, int src_stride2, int h){\
    OPNAME ## _pixels8_l2(dst  , src1  , src2  , dst_stride, src_stride1, src_stride2, h);\
    OPNAME ## _pixels8_l2(dst+8, src1+8, src2+8, dst_stride, src_stride1, src_stride2, h);\
}\
\
static inline void OPNAME ## _no_rnd_pixels16_l2(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, int dst_stride, \
                                                int src_stride1, int src_stride2, int h){\
    OPNAME ## _no_rnd_pixels8_l2(dst  , src1  , src2  , dst_stride, src_stride1, src_stride2, h);\
    OPNAME ## _no_rnd_pixels8_l2(dst+8, src1+8, src2+8, dst_stride, src_stride1, src_stride2, h);\
}\
\
static inline void OPNAME ## _no_rnd_pixels8_x2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _no_rnd_pixels8_l2(block, pixels, pixels+1, line_size, line_size, line_size, h);\
}\
\
static inline void OPNAME ## _pixels8_x2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _pixels8_l2(block, pixels, pixels+1, line_size, line_size, line_size, h);\
}\
\
static inline void OPNAME ## _no_rnd_pixels8_y2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _no_rnd_pixels8_l2(block, pixels, pixels+line_size, line_size, line_size, line_size, h);\
}\
\
static inline void OPNAME ## _pixels8_y2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _pixels8_l2(block, pixels, pixels+line_size, line_size, line_size, line_size, h);\
}\
\
static inline void OPNAME ## _pixels8_l4(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, const uint8_t *src3, const uint8_t *src4,\
                 int dst_stride, int src_stride1, int src_stride2,int src_stride3,int src_stride4, int h){\
    int i;\
    for(i=0; i<h; i++){\
        uint32_t a, b, c, d, l0, l1, h0, h1;\
        a= AV_RN32(&src1[i*src_stride1]);\
        b= AV_RN32(&src2[i*src_stride2]);\
        c= AV_RN32(&src3[i*src_stride3]);\
        d= AV_RN32(&src4[i*src_stride4]);\
        l0=  (a&0x03030303UL)\
           + (b&0x03030303UL)\
           + 0x02020202UL;\
        h0= ((a&0xFCFCFCFCUL)>>2)\
          + ((b&0xFCFCFCFCUL)>>2);\
        l1=  (c&0x03030303UL)\
           + (d&0x03030303UL);\
        h1= ((c&0xFCFCFCFCUL)>>2)\
          + ((d&0xFCFCFCFCUL)>>2);\
        OP(*((uint32_t*)&dst[i*dst_stride]), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
        a= AV_RN32(&src1[i*src_stride1+4]);\
        b= AV_RN32(&src2[i*src_stride2+4]);\
        c= AV_RN32(&src3[i*src_stride3+4]);\
        d= AV_RN32(&src4[i*src_stride4+4]);\
        l0=  (a&0x03030303UL)\
           + (b&0x03030303UL)\
           + 0x02020202UL;\
        h0= ((a&0xFCFCFCFCUL)>>2)\
          + ((b&0xFCFCFCFCUL)>>2);\
        l1=  (c&0x03030303UL)\
           + (d&0x03030303UL);\
        h1= ((c&0xFCFCFCFCUL)>>2)\
          + ((d&0xFCFCFCFCUL)>>2);\
        OP(*((uint32_t*)&dst[i*dst_stride+4]), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
    }\
}\
\
static inline void OPNAME ## _pixels4_x2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _pixels4_l2(block, pixels, pixels+1, line_size, line_size, line_size, h);\
}\
\
static inline void OPNAME ## _pixels4_y2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _pixels4_l2(block, pixels, pixels+line_size, line_size, line_size, line_size, h);\
}\
\
static inline void OPNAME ## _pixels2_x2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _pixels2_l2(block, pixels, pixels+1, line_size, line_size, line_size, h);\
}\
\
static inline void OPNAME ## _pixels2_y2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _pixels2_l2(block, pixels, pixels+line_size, line_size, line_size, line_size, h);\
}\
\
static inline void OPNAME ## _no_rnd_pixels8_l4(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, const uint8_t *src3, const uint8_t *src4,\
                 int dst_stride, int src_stride1, int src_stride2,int src_stride3,int src_stride4, int h){\
    int i;\
    for(i=0; i<h; i++){\
        uint32_t a, b, c, d, l0, l1, h0, h1;\
        a= AV_RN32(&src1[i*src_stride1]);\
        b= AV_RN32(&src2[i*src_stride2]);\
        c= AV_RN32(&src3[i*src_stride3]);\
        d= AV_RN32(&src4[i*src_stride4]);\
        l0=  (a&0x03030303UL)\
           + (b&0x03030303UL)\
           + 0x01010101UL;\
        h0= ((a&0xFCFCFCFCUL)>>2)\
          + ((b&0xFCFCFCFCUL)>>2);\
        l1=  (c&0x03030303UL)\
           + (d&0x03030303UL);\
        h1= ((c&0xFCFCFCFCUL)>>2)\
          + ((d&0xFCFCFCFCUL)>>2);\
        OP(*((uint32_t*)&dst[i*dst_stride]), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
        a= AV_RN32(&src1[i*src_stride1+4]);\
        b= AV_RN32(&src2[i*src_stride2+4]);\
        c= AV_RN32(&src3[i*src_stride3+4]);\
        d= AV_RN32(&src4[i*src_stride4+4]);\
        l0=  (a&0x03030303UL)\
           + (b&0x03030303UL)\
           + 0x01010101UL;\
        h0= ((a&0xFCFCFCFCUL)>>2)\
          + ((b&0xFCFCFCFCUL)>>2);\
        l1=  (c&0x03030303UL)\
           + (d&0x03030303UL);\
        h1= ((c&0xFCFCFCFCUL)>>2)\
          + ((d&0xFCFCFCFCUL)>>2);\
        OP(*((uint32_t*)&dst[i*dst_stride+4]), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
    }\
}\
static inline void OPNAME ## _pixels16_l4(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, const uint8_t *src3, const uint8_t *src4,\
                 int dst_stride, int src_stride1, int src_stride2,int src_stride3,int src_stride4, int h){\
    OPNAME ## _pixels8_l4(dst  , src1  , src2  , src3  , src4  , dst_stride, src_stride1, src_stride2, src_stride3, src_stride4, h);\
    OPNAME ## _pixels8_l4(dst+8, src1+8, src2+8, src3+8, src4+8, dst_stride, src_stride1, src_stride2, src_stride3, src_stride4, h);\
}\
static inline void OPNAME ## _no_rnd_pixels16_l4(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, const uint8_t *src3, const uint8_t *src4,\
                 int dst_stride, int src_stride1, int src_stride2,int src_stride3,int src_stride4, int h){\
    OPNAME ## _no_rnd_pixels8_l4(dst  , src1  , src2  , src3  , src4  , dst_stride, src_stride1, src_stride2, src_stride3, src_stride4, h);\
    OPNAME ## _no_rnd_pixels8_l4(dst+8, src1+8, src2+8, src3+8, src4+8, dst_stride, src_stride1, src_stride2, src_stride3, src_stride4, h);\
}\
\
static inline void OPNAME ## _pixels2_xy2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
        int i, a0, b0, a1, b1;\
        a0= pixels[0];\
        b0= pixels[1] + 2;\
        a0 += b0;\
        b0 += pixels[2];\
\
        pixels+=line_size;\
        for(i=0; i<h; i+=2){\
            a1= pixels[0];\
            b1= pixels[1];\
            a1 += b1;\
            b1 += pixels[2];\
\
            block[0]= (a1+a0)>>2; /* FIXME non put */\
            block[1]= (b1+b0)>>2;\
\
            pixels+=line_size;\
            block +=line_size;\
\
            a0= pixels[0];\
            b0= pixels[1] + 2;\
            a0 += b0;\
            b0 += pixels[2];\
\
            block[0]= (a1+a0)>>2;\
            block[1]= (b1+b0)>>2;\
            pixels+=line_size;\
            block +=line_size;\
        }\
}\
\
static inline void OPNAME ## _pixels4_xy2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
        int i;\
        const uint32_t a= AV_RN32(pixels  );\
        const uint32_t b= AV_RN32(pixels+1);\
        uint32_t l0=  (a&0x03030303UL)\
                    + (b&0x03030303UL)\
                    + 0x02020202UL;\
        uint32_t h0= ((a&0xFCFCFCFCUL)>>2)\
                   + ((b&0xFCFCFCFCUL)>>2);\
        uint32_t l1,h1;\
\
        pixels+=line_size;\
        for(i=0; i<h; i+=2){\
            uint32_t a= AV_RN32(pixels  );\
            uint32_t b= AV_RN32(pixels+1);\
            l1=  (a&0x03030303UL)\
               + (b&0x03030303UL);\
            h1= ((a&0xFCFCFCFCUL)>>2)\
              + ((b&0xFCFCFCFCUL)>>2);\
            OP(*((uint32_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
            pixels+=line_size;\
            block +=line_size;\
            a= AV_RN32(pixels  );\
            b= AV_RN32(pixels+1);\
            l0=  (a&0x03030303UL)\
               + (b&0x03030303UL)\
               + 0x02020202UL;\
            h0= ((a&0xFCFCFCFCUL)>>2)\
              + ((b&0xFCFCFCFCUL)>>2);\
            OP(*((uint32_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
            pixels+=line_size;\
            block +=line_size;\
        }\
}\
\
static inline void OPNAME ## _pixels8_xy2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
    int j;\
    for(j=0; j<2; j++){\
        int i;\
        const uint32_t a= AV_RN32(pixels  );\
        const uint32_t b= AV_RN32(pixels+1);\
        uint32_t l0=  (a&0x03030303UL)\
                    + (b&0x03030303UL)\
                    + 0x02020202UL;\
        uint32_t h0= ((a&0xFCFCFCFCUL)>>2)\
                   + ((b&0xFCFCFCFCUL)>>2);\
        uint32_t l1,h1;\
\
        pixels+=line_size;\
        for(i=0; i<h; i+=2){\
            uint32_t a= AV_RN32(pixels  );\
            uint32_t b= AV_RN32(pixels+1);\
            l1=  (a&0x03030303UL)\
               + (b&0x03030303UL);\
            h1= ((a&0xFCFCFCFCUL)>>2)\
              + ((b&0xFCFCFCFCUL)>>2);\
            OP(*((uint32_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
            pixels+=line_size;\
            block +=line_size;\
            a= AV_RN32(pixels  );\
            b= AV_RN32(pixels+1);\
            l0=  (a&0x03030303UL)\
               + (b&0x03030303UL)\
               + 0x02020202UL;\
            h0= ((a&0xFCFCFCFCUL)>>2)\
              + ((b&0xFCFCFCFCUL)>>2);\
            OP(*((uint32_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
            pixels+=line_size;\
            block +=line_size;\
        }\
        pixels+=4-line_size*(h+1);\
        block +=4-line_size*h;\
    }\
}\
\
static inline void OPNAME ## _no_rnd_pixels8_xy2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
    int j;\
    for(j=0; j<2; j++){\
        int i;\
        const uint32_t a= AV_RN32(pixels  );\
        const uint32_t b= AV_RN32(pixels+1);\
        uint32_t l0=  (a&0x03030303UL)\
                    + (b&0x03030303UL)\
                    + 0x01010101UL;\
        uint32_t h0= ((a&0xFCFCFCFCUL)>>2)\
                   + ((b&0xFCFCFCFCUL)>>2);\
        uint32_t l1,h1;\
\
        pixels+=line_size;\
        for(i=0; i<h; i+=2){\
            uint32_t a= AV_RN32(pixels  );\
            uint32_t b= AV_RN32(pixels+1);\
            l1=  (a&0x03030303UL)\
               + (b&0x03030303UL);\
            h1= ((a&0xFCFCFCFCUL)>>2)\
              + ((b&0xFCFCFCFCUL)>>2);\
            OP(*((uint32_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
            pixels+=line_size;\
            block +=line_size;\
            a= AV_RN32(pixels  );\
            b= AV_RN32(pixels+1);\
            l0=  (a&0x03030303UL)\
               + (b&0x03030303UL)\
               + 0x01010101UL;\
            h0= ((a&0xFCFCFCFCUL)>>2)\
              + ((b&0xFCFCFCFCUL)>>2);\
            OP(*((uint32_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
            pixels+=line_size;\
            block +=line_size;\
        }\
        pixels+=4-line_size*(h+1);\
        block +=4-line_size*h;\
    }\
}\
\
CALL_2X_PIXELS(OPNAME ## _pixels16_c  , OPNAME ## _pixels8_c  , 8)\
CALL_2X_PIXELS(OPNAME ## _pixels16_x2_c , OPNAME ## _pixels8_x2_c , 8)\
CALL_2X_PIXELS(OPNAME ## _pixels16_y2_c , OPNAME ## _pixels8_y2_c , 8)\
CALL_2X_PIXELS(OPNAME ## _pixels16_xy2_c, OPNAME ## _pixels8_xy2_c, 8)\
av_unused CALL_2X_PIXELS(OPNAME ## _no_rnd_pixels16_c  , OPNAME ## _pixels8_c         , 8)\
CALL_2X_PIXELS(OPNAME ## _no_rnd_pixels16_x2_c , OPNAME ## _no_rnd_pixels8_x2_c , 8)\
CALL_2X_PIXELS(OPNAME ## _no_rnd_pixels16_y2_c , OPNAME ## _no_rnd_pixels8_y2_c , 8)\
CALL_2X_PIXELS(OPNAME ## _no_rnd_pixels16_xy2_c, OPNAME ## _no_rnd_pixels8_xy2_c, 8)\

#define op_avg(a, b) a = rnd_avg32(a, b)
#endif
#define op_put(a, b) a = b

PIXOP2(avg, op_avg)
PIXOP2(put, op_put)
#undef op_avg
#undef op_put

#define put_no_rnd_pixels8_c  put_pixels8_c
#define put_no_rnd_pixels16_c put_pixels16_c

#define avg2(a,b) ((a+b+1)>>1)
#define avg4(a,b,c,d) ((a+b+c+d+2)>>2)

static void put_no_rnd_pixels16_l2_c(uint8_t *dst, const uint8_t *a, const uint8_t *b, int stride, int h){
    put_no_rnd_pixels16_l2(dst, a, b, stride, stride, stride, h);
}

static void put_no_rnd_pixels8_l2_c(uint8_t *dst, const uint8_t *a, const uint8_t *b, int stride, int h){
    put_no_rnd_pixels8_l2(dst, a, b, stride, stride, stride, h);
}

static void gmc1_c(uint8_t *dst, uint8_t *src, int stride, int h, int x16, int y16, int rounder)
{
    const int A=(16-x16)*(16-y16);
    const int B=(   x16)*(16-y16);
    const int C=(16-x16)*(   y16);
    const int D=(   x16)*(   y16);
    int i;

    for(i=0; i<h; i++)
    {
        dst[0]= (A*src[0] + B*src[1] + C*src[stride+0] + D*src[stride+1] + rounder)>>8;
        dst[1]= (A*src[1] + B*src[2] + C*src[stride+1] + D*src[stride+2] + rounder)>>8;
        dst[2]= (A*src[2] + B*src[3] + C*src[stride+2] + D*src[stride+3] + rounder)>>8;
        dst[3]= (A*src[3] + B*src[4] + C*src[stride+3] + D*src[stride+4] + rounder)>>8;
        dst[4]= (A*src[4] + B*src[5] + C*src[stride+4] + D*src[stride+5] + rounder)>>8;
        dst[5]= (A*src[5] + B*src[6] + C*src[stride+5] + D*src[stride+6] + rounder)>>8;
        dst[6]= (A*src[6] + B*src[7] + C*src[stride+6] + D*src[stride+7] + rounder)>>8;
        dst[7]= (A*src[7] + B*src[8] + C*src[stride+7] + D*src[stride+8] + rounder)>>8;
        dst+= stride;
        src+= stride;
    }
}

void ff_gmc_c(uint8_t *dst, uint8_t *src, int stride, int h, int ox, int oy,
                  int dxx, int dxy, int dyx, int dyy, int shift, int r, int width, int height)
{
    int y, vx, vy;
    const int s= 1<<shift;

    width--;
    height--;

    for(y=0; y<h; y++){
        int x;

        vx= ox;
        vy= oy;
        for(x=0; x<8; x++){ //XXX FIXME optimize
            int src_x, src_y, frac_x, frac_y, index;

            src_x= vx>>16;
            src_y= vy>>16;
            frac_x= src_x&(s-1);
            frac_y= src_y&(s-1);
            src_x>>=shift;
            src_y>>=shift;

            if((unsigned)src_x < width){
                if((unsigned)src_y < height){
                    index= src_x + src_y*stride;
                    dst[y*stride + x]= (  (  src[index         ]*(s-frac_x)
                                           + src[index       +1]*   frac_x )*(s-frac_y)
                                        + (  src[index+stride  ]*(s-frac_x)
                                           + src[index+stride+1]*   frac_x )*   frac_y
                                        + r)>>(shift*2);
                }else{
                    index= src_x + av_clip(src_y, 0, height)*stride;
                    dst[y*stride + x]= ( (  src[index         ]*(s-frac_x)
                                          + src[index       +1]*   frac_x )*s
                                        + r)>>(shift*2);
                }
            }else{
                if((unsigned)src_y < height){
                    index= av_clip(src_x, 0, width) + src_y*stride;
                    dst[y*stride + x]= (  (  src[index         ]*(s-frac_y)
                                           + src[index+stride  ]*   frac_y )*s
                                        + r)>>(shift*2);
                }else{
                    index= av_clip(src_x, 0, width) + av_clip(src_y, 0, height)*stride;
                    dst[y*stride + x]=    src[index         ];
                }
            }

            vx+= dxx;
            vy+= dyx;
        }
        ox += dxy;
        oy += dyy;
    }
}

static inline void put_tpel_pixels_mc00_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    switch(width){
    case 2: put_pixels2_c (dst, src, stride, height); break;
    case 4: put_pixels4_c (dst, src, stride, height); break;
    case 8: put_pixels8_c (dst, src, stride, height); break;
    case 16:put_pixels16_c(dst, src, stride, height); break;
    }
}

static inline void put_tpel_pixels_mc10_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    int i,j;
    for (i=0; i < height; i++) {
      for (j=0; j < width; j++) {
        dst[j] = (683*(2*src[j] + src[j+1] + 1)) >> 11;
      }
      src += stride;
      dst += stride;
    }
}

static inline void put_tpel_pixels_mc20_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    int i,j;
    for (i=0; i < height; i++) {
      for (j=0; j < width; j++) {
        dst[j] = (683*(src[j] + 2*src[j+1] + 1)) >> 11;
      }
      src += stride;
      dst += stride;
    }
}

static inline void put_tpel_pixels_mc01_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    int i,j;
    for (i=0; i < height; i++) {
      for (j=0; j < width; j++) {
        dst[j] = (683*(2*src[j] + src[j+stride] + 1)) >> 11;
      }
      src += stride;
      dst += stride;
    }
}

static inline void put_tpel_pixels_mc11_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    int i,j;
    for (i=0; i < height; i++) {
      for (j=0; j < width; j++) {
        dst[j] = (2731*(4*src[j] + 3*src[j+1] + 3*src[j+stride] + 2*src[j+stride+1] + 6)) >> 15;
      }
      src += stride;
      dst += stride;
    }
}

static inline void put_tpel_pixels_mc12_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    int i,j;
    for (i=0; i < height; i++) {
      for (j=0; j < width; j++) {
        dst[j] = (2731*(3*src[j] + 2*src[j+1] + 4*src[j+stride] + 3*src[j+stride+1] + 6)) >> 15;
      }
      src += stride;
      dst += stride;
    }
}

static inline void put_tpel_pixels_mc02_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    int i,j;
    for (i=0; i < height; i++) {
      for (j=0; j < width; j++) {
        dst[j] = (683*(src[j] + 2*src[j+stride] + 1)) >> 11;
      }
      src += stride;
      dst += stride;
    }
}

static inline void put_tpel_pixels_mc21_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    int i,j;
    for (i=0; i < height; i++) {
      for (j=0; j < width; j++) {
        dst[j] = (2731*(3*src[j] + 4*src[j+1] + 2*src[j+stride] + 3*src[j+stride+1] + 6)) >> 15;
      }
      src += stride;
      dst += stride;
    }
}

static inline void put_tpel_pixels_mc22_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    int i,j;
    for (i=0; i < height; i++) {
      for (j=0; j < width; j++) {
        dst[j] = (2731*(2*src[j] + 3*src[j+1] + 3*src[j+stride] + 4*src[j+stride+1] + 6)) >> 15;
      }
      src += stride;
      dst += stride;
    }
}

static inline void avg_tpel_pixels_mc00_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    switch(width){
    case 2: avg_pixels2_c (dst, src, stride, height); break;
    case 4: avg_pixels4_c (dst, src, stride, height); break;
    case 8: avg_pixels8_c (dst, src, stride, height); break;
    case 16:avg_pixels16_c(dst, src, stride, height); break;
    }
}

static inline void avg_tpel_pixels_mc10_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    int i,j;
    for (i=0; i < height; i++) {
      for (j=0; j < width; j++) {
        dst[j] = (dst[j] + ((683*(2*src[j] + src[j+1] + 1)) >> 11) + 1) >> 1;
      }
      src += stride;
      dst += stride;
    }
}

static inline void avg_tpel_pixels_mc20_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    int i,j;
    for (i=0; i < height; i++) {
      for (j=0; j < width; j++) {
        dst[j] = (dst[j] + ((683*(src[j] + 2*src[j+1] + 1)) >> 11) + 1) >> 1;
      }
      src += stride;
      dst += stride;
    }
}

static inline void avg_tpel_pixels_mc01_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    int i,j;
    for (i=0; i < height; i++) {
      for (j=0; j < width; j++) {
        dst[j] = (dst[j] + ((683*(2*src[j] + src[j+stride] + 1)) >> 11) + 1) >> 1;
      }
      src += stride;
      dst += stride;
    }
}

static inline void avg_tpel_pixels_mc11_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    int i,j;
    for (i=0; i < height; i++) {
      for (j=0; j < width; j++) {
        dst[j] = (dst[j] + ((2731*(4*src[j] + 3*src[j+1] + 3*src[j+stride] + 2*src[j+stride+1] + 6)) >> 15) + 1) >> 1;
      }
      src += stride;
      dst += stride;
    }
}

static inline void avg_tpel_pixels_mc12_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    int i,j;
    for (i=0; i < height; i++) {
      for (j=0; j < width; j++) {
        dst[j] = (dst[j] + ((2731*(3*src[j] + 2*src[j+1] + 4*src[j+stride] + 3*src[j+stride+1] + 6)) >> 15) + 1) >> 1;
      }
      src += stride;
      dst += stride;
    }
}

static inline void avg_tpel_pixels_mc02_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    int i,j;
    for (i=0; i < height; i++) {
      for (j=0; j < width; j++) {
        dst[j] = (dst[j] + ((683*(src[j] + 2*src[j+stride] + 1)) >> 11) + 1) >> 1;
      }
      src += stride;
      dst += stride;
    }
}

static inline void avg_tpel_pixels_mc21_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    int i,j;
    for (i=0; i < height; i++) {
      for (j=0; j < width; j++) {
        dst[j] = (dst[j] + ((2731*(3*src[j] + 4*src[j+1] + 2*src[j+stride] + 3*src[j+stride+1] + 6)) >> 15) + 1) >> 1;
      }
      src += stride;
      dst += stride;
    }
}

static inline void avg_tpel_pixels_mc22_c(uint8_t *dst, const uint8_t *src, int stride, int width, int height){
    int i,j;
    for (i=0; i < height; i++) {
      for (j=0; j < width; j++) {
        dst[j] = (dst[j] + ((2731*(2*src[j] + 3*src[j+1] + 3*src[j+stride] + 4*src[j+stride+1] + 6)) >> 15) + 1) >> 1;
      }
      src += stride;
      dst += stride;
    }
}
#if 0
#define TPEL_WIDTH(width)\
static void put_tpel_pixels ## width ## _mc00_c(uint8_t *dst, const uint8_t *src, int stride, int height){\
    void put_tpel_pixels_mc00_c(dst, src, stride, width, height);}\
static void put_tpel_pixels ## width ## _mc10_c(uint8_t *dst, const uint8_t *src, int stride, int height){\
    void put_tpel_pixels_mc10_c(dst, src, stride, width, height);}\
static void put_tpel_pixels ## width ## _mc20_c(uint8_t *dst, const uint8_t *src, int stride, int height){\
    void put_tpel_pixels_mc20_c(dst, src, stride, width, height);}\
static void put_tpel_pixels ## width ## _mc01_c(uint8_t *dst, const uint8_t *src, int stride, int height){\
    void put_tpel_pixels_mc01_c(dst, src, stride, width, height);}\
static void put_tpel_pixels ## width ## _mc11_c(uint8_t *dst, const uint8_t *src, int stride, int height){\
    void put_tpel_pixels_mc11_c(dst, src, stride, width, height);}\
static void put_tpel_pixels ## width ## _mc21_c(uint8_t *dst, const uint8_t *src, int stride, int height){\
    void put_tpel_pixels_mc21_c(dst, src, stride, width, height);}\
static void put_tpel_pixels ## width ## _mc02_c(uint8_t *dst, const uint8_t *src, int stride, int height){\
    void put_tpel_pixels_mc02_c(dst, src, stride, width, height);}\
static void put_tpel_pixels ## width ## _mc12_c(uint8_t *dst, const uint8_t *src, int stride, int height){\
    void put_tpel_pixels_mc12_c(dst, src, stride, width, height);}\
static void put_tpel_pixels ## width ## _mc22_c(uint8_t *dst, const uint8_t *src, int stride, int height){\
    void put_tpel_pixels_mc22_c(dst, src, stride, width, height);}
#endif

#define H264_CHROMA_MC(OPNAME, OP)\
static void OPNAME ## h264_chroma_mc2_c(uint8_t *dst/*align 8*/, uint8_t *src/*align 1*/, int stride, int h, int x, int y){\
    const int A=(8-x)*(8-y);\
    const int B=(  x)*(8-y);\
    const int C=(8-x)*(  y);\
    const int D=(  x)*(  y);\
    int i;\
    \
    assert(x<8 && y<8 && x>=0 && y>=0);\
\
    if(D){\
        for(i=0; i<h; i++){\
            OP(dst[0], (A*src[0] + B*src[1] + C*src[stride+0] + D*src[stride+1]));\
            OP(dst[1], (A*src[1] + B*src[2] + C*src[stride+1] + D*src[stride+2]));\
            dst+= stride;\
            src+= stride;\
        }\
    }else{\
        const int E= B+C;\
        const int step= C ? stride : 1;\
        for(i=0; i<h; i++){\
            OP(dst[0], (A*src[0] + E*src[step+0]));\
            OP(dst[1], (A*src[1] + E*src[step+1]));\
            dst+= stride;\
            src+= stride;\
        }\
    }\
}\
\
static void OPNAME ## h264_chroma_mc4_c(uint8_t *dst/*align 8*/, uint8_t *src/*align 1*/, int stride, int h, int x, int y){\
    const int A=(8-x)*(8-y);\
    const int B=(  x)*(8-y);\
    const int C=(8-x)*(  y);\
    const int D=(  x)*(  y);\
    int i;\
    \
    assert(x<8 && y<8 && x>=0 && y>=0);\
\
    if(D){\
        for(i=0; i<h; i++){\
            OP(dst[0], (A*src[0] + B*src[1] + C*src[stride+0] + D*src[stride+1]));\
            OP(dst[1], (A*src[1] + B*src[2] + C*src[stride+1] + D*src[stride+2]));\
            OP(dst[2], (A*src[2] + B*src[3] + C*src[stride+2] + D*src[stride+3]));\
            OP(dst[3], (A*src[3] + B*src[4] + C*src[stride+3] + D*src[stride+4]));\
            dst+= stride;\
            src+= stride;\
        }\
    }else{\
        const int E= B+C;\
        const int step= C ? stride : 1;\
        for(i=0; i<h; i++){\
            OP(dst[0], (A*src[0] + E*src[step+0]));\
            OP(dst[1], (A*src[1] + E*src[step+1]));\
            OP(dst[2], (A*src[2] + E*src[step+2]));\
            OP(dst[3], (A*src[3] + E*src[step+3]));\
            dst+= stride;\
            src+= stride;\
        }\
    }\
}\
\
static void OPNAME ## h264_chroma_mc8_c(uint8_t *dst/*align 8*/, uint8_t *src/*align 1*/, int stride, int h, int x, int y){\
    const int A=(8-x)*(8-y);\
    const int B=(  x)*(8-y);\
    const int C=(8-x)*(  y);\
    const int D=(  x)*(  y);\
    int i;\
    \
    assert(x<8 && y<8 && x>=0 && y>=0);\
\
    if(D){\
        for(i=0; i<h; i++){\
            OP(dst[0], (A*src[0] + B*src[1] + C*src[stride+0] + D*src[stride+1]));\
            OP(dst[1], (A*src[1] + B*src[2] + C*src[stride+1] + D*src[stride+2]));\
            OP(dst[2], (A*src[2] + B*src[3] + C*src[stride+2] + D*src[stride+3]));\
            OP(dst[3], (A*src[3] + B*src[4] + C*src[stride+3] + D*src[stride+4]));\
            OP(dst[4], (A*src[4] + B*src[5] + C*src[stride+4] + D*src[stride+5]));\
            OP(dst[5], (A*src[5] + B*src[6] + C*src[stride+5] + D*src[stride+6]));\
            OP(dst[6], (A*src[6] + B*src[7] + C*src[stride+6] + D*src[stride+7]));\
            OP(dst[7], (A*src[7] + B*src[8] + C*src[stride+7] + D*src[stride+8]));\
            dst+= stride;\
            src+= stride;\
        }\
    }else{\
        const int E= B+C;\
        const int step= C ? stride : 1;\
        for(i=0; i<h; i++){\
            OP(dst[0], (A*src[0] + E*src[step+0]));\
            OP(dst[1], (A*src[1] + E*src[step+1]));\
            OP(dst[2], (A*src[2] + E*src[step+2]));\
            OP(dst[3], (A*src[3] + E*src[step+3]));\
            OP(dst[4], (A*src[4] + E*src[step+4]));\
            OP(dst[5], (A*src[5] + E*src[step+5]));\
            OP(dst[6], (A*src[6] + E*src[step+6]));\
            OP(dst[7], (A*src[7] + E*src[step+7]));\
            dst+= stride;\
            src+= stride;\
        }\
    }\
}

#define op_avg(a, b) a = (((a)+(((b) + 32)>>6)+1)>>1)
#define op_put(a, b) a = (((b) + 32)>>6)

H264_CHROMA_MC(put_       , op_put)
H264_CHROMA_MC(avg_       , op_avg)
#undef op_avg
#undef op_put

#define QPEL_MC(r, OPNAME, RND, OP) \
static void OPNAME ## mpeg4_qpel8_h_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride, int h){\
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;\
    int i;\
    for(i=0; i<h; i++)\
    {\
        OP(dst[0], (src[0]+src[1])*20 - (src[0]+src[2])*6 + (src[1]+src[3])*3 - (src[2]+src[4]));\
        OP(dst[1], (src[1]+src[2])*20 - (src[0]+src[3])*6 + (src[0]+src[4])*3 - (src[1]+src[5]));\
        OP(dst[2], (src[2]+src[3])*20 - (src[1]+src[4])*6 + (src[0]+src[5])*3 - (src[0]+src[6]));\
        OP(dst[3], (src[3]+src[4])*20 - (src[2]+src[5])*6 + (src[1]+src[6])*3 - (src[0]+src[7]));\
        OP(dst[4], (src[4]+src[5])*20 - (src[3]+src[6])*6 + (src[2]+src[7])*3 - (src[1]+src[8]));\
        OP(dst[5], (src[5]+src[6])*20 - (src[4]+src[7])*6 + (src[3]+src[8])*3 - (src[2]+src[8]));\
        OP(dst[6], (src[6]+src[7])*20 - (src[5]+src[8])*6 + (src[4]+src[8])*3 - (src[3]+src[7]));\
        OP(dst[7], (src[7]+src[8])*20 - (src[6]+src[8])*6 + (src[5]+src[7])*3 - (src[4]+src[6]));\
        dst+=dstStride;\
        src+=srcStride;\
    }\
}\
\
static void OPNAME ## mpeg4_qpel8_v_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){\
    const int w=8;\
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;\
    int i;\
    for(i=0; i<w; i++)\
    {\
        const int src0= src[0*srcStride];\
        const int src1= src[1*srcStride];\
        const int src2= src[2*srcStride];\
        const int src3= src[3*srcStride];\
        const int src4= src[4*srcStride];\
        const int src5= src[5*srcStride];\
        const int src6= src[6*srcStride];\
        const int src7= src[7*srcStride];\
        const int src8= src[8*srcStride];\
        OP(dst[0*dstStride], (src0+src1)*20 - (src0+src2)*6 + (src1+src3)*3 - (src2+src4));\
        OP(dst[1*dstStride], (src1+src2)*20 - (src0+src3)*6 + (src0+src4)*3 - (src1+src5));\
        OP(dst[2*dstStride], (src2+src3)*20 - (src1+src4)*6 + (src0+src5)*3 - (src0+src6));\
        OP(dst[3*dstStride], (src3+src4)*20 - (src2+src5)*6 + (src1+src6)*3 - (src0+src7));\
        OP(dst[4*dstStride], (src4+src5)*20 - (src3+src6)*6 + (src2+src7)*3 - (src1+src8));\
        OP(dst[5*dstStride], (src5+src6)*20 - (src4+src7)*6 + (src3+src8)*3 - (src2+src8));\
        OP(dst[6*dstStride], (src6+src7)*20 - (src5+src8)*6 + (src4+src8)*3 - (src3+src7));\
        OP(dst[7*dstStride], (src7+src8)*20 - (src6+src8)*6 + (src5+src7)*3 - (src4+src6));\
        dst++;\
        src++;\
    }\
}\
\
static void OPNAME ## mpeg4_qpel16_h_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride, int h){\
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;\
    int i;\
    \
    for(i=0; i<h; i++)\
    {\
        OP(dst[ 0], (src[ 0]+src[ 1])*20 - (src[ 0]+src[ 2])*6 + (src[ 1]+src[ 3])*3 - (src[ 2]+src[ 4]));\
        OP(dst[ 1], (src[ 1]+src[ 2])*20 - (src[ 0]+src[ 3])*6 + (src[ 0]+src[ 4])*3 - (src[ 1]+src[ 5]));\
        OP(dst[ 2], (src[ 2]+src[ 3])*20 - (src[ 1]+src[ 4])*6 + (src[ 0]+src[ 5])*3 - (src[ 0]+src[ 6]));\
        OP(dst[ 3], (src[ 3]+src[ 4])*20 - (src[ 2]+src[ 5])*6 + (src[ 1]+src[ 6])*3 - (src[ 0]+src[ 7]));\
        OP(dst[ 4], (src[ 4]+src[ 5])*20 - (src[ 3]+src[ 6])*6 + (src[ 2]+src[ 7])*3 - (src[ 1]+src[ 8]));\
        OP(dst[ 5], (src[ 5]+src[ 6])*20 - (src[ 4]+src[ 7])*6 + (src[ 3]+src[ 8])*3 - (src[ 2]+src[ 9]));\
        OP(dst[ 6], (src[ 6]+src[ 7])*20 - (src[ 5]+src[ 8])*6 + (src[ 4]+src[ 9])*3 - (src[ 3]+src[10]));\
        OP(dst[ 7], (src[ 7]+src[ 8])*20 - (src[ 6]+src[ 9])*6 + (src[ 5]+src[10])*3 - (src[ 4]+src[11]));\
        OP(dst[ 8], (src[ 8]+src[ 9])*20 - (src[ 7]+src[10])*6 + (src[ 6]+src[11])*3 - (src[ 5]+src[12]));\
        OP(dst[ 9], (src[ 9]+src[10])*20 - (src[ 8]+src[11])*6 + (src[ 7]+src[12])*3 - (src[ 6]+src[13]));\
        OP(dst[10], (src[10]+src[11])*20 - (src[ 9]+src[12])*6 + (src[ 8]+src[13])*3 - (src[ 7]+src[14]));\
        OP(dst[11], (src[11]+src[12])*20 - (src[10]+src[13])*6 + (src[ 9]+src[14])*3 - (src[ 8]+src[15]));\
        OP(dst[12], (src[12]+src[13])*20 - (src[11]+src[14])*6 + (src[10]+src[15])*3 - (src[ 9]+src[16]));\
        OP(dst[13], (src[13]+src[14])*20 - (src[12]+src[15])*6 + (src[11]+src[16])*3 - (src[10]+src[16]));\
        OP(dst[14], (src[14]+src[15])*20 - (src[13]+src[16])*6 + (src[12]+src[16])*3 - (src[11]+src[15]));\
        OP(dst[15], (src[15]+src[16])*20 - (src[14]+src[16])*6 + (src[13]+src[15])*3 - (src[12]+src[14]));\
        dst+=dstStride;\
        src+=srcStride;\
    }\
}\
\
static void OPNAME ## mpeg4_qpel16_v_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){\
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;\
    int i;\
    const int w=16;\
    for(i=0; i<w; i++)\
    {\
        const int src0= src[0*srcStride];\
        const int src1= src[1*srcStride];\
        const int src2= src[2*srcStride];\
        const int src3= src[3*srcStride];\
        const int src4= src[4*srcStride];\
        const int src5= src[5*srcStride];\
        const int src6= src[6*srcStride];\
        const int src7= src[7*srcStride];\
        const int src8= src[8*srcStride];\
        const int src9= src[9*srcStride];\
        const int src10= src[10*srcStride];\
        const int src11= src[11*srcStride];\
        const int src12= src[12*srcStride];\
        const int src13= src[13*srcStride];\
        const int src14= src[14*srcStride];\
        const int src15= src[15*srcStride];\
        const int src16= src[16*srcStride];\
        OP(dst[ 0*dstStride], (src0 +src1 )*20 - (src0 +src2 )*6 + (src1 +src3 )*3 - (src2 +src4 ));\
        OP(dst[ 1*dstStride], (src1 +src2 )*20 - (src0 +src3 )*6 + (src0 +src4 )*3 - (src1 +src5 ));\
        OP(dst[ 2*dstStride], (src2 +src3 )*20 - (src1 +src4 )*6 + (src0 +src5 )*3 - (src0 +src6 ));\
        OP(dst[ 3*dstStride], (src3 +src4 )*20 - (src2 +src5 )*6 + (src1 +src6 )*3 - (src0 +src7 ));\
        OP(dst[ 4*dstStride], (src4 +src5 )*20 - (src3 +src6 )*6 + (src2 +src7 )*3 - (src1 +src8 ));\
        OP(dst[ 5*dstStride], (src5 +src6 )*20 - (src4 +src7 )*6 + (src3 +src8 )*3 - (src2 +src9 ));\
        OP(dst[ 6*dstStride], (src6 +src7 )*20 - (src5 +src8 )*6 + (src4 +src9 )*3 - (src3 +src10));\
        OP(dst[ 7*dstStride], (src7 +src8 )*20 - (src6 +src9 )*6 + (src5 +src10)*3 - (src4 +src11));\
        OP(dst[ 8*dstStride], (src8 +src9 )*20 - (src7 +src10)*6 + (src6 +src11)*3 - (src5 +src12));\
        OP(dst[ 9*dstStride], (src9 +src10)*20 - (src8 +src11)*6 + (src7 +src12)*3 - (src6 +src13));\
        OP(dst[10*dstStride], (src10+src11)*20 - (src9 +src12)*6 + (src8 +src13)*3 - (src7 +src14));\
        OP(dst[11*dstStride], (src11+src12)*20 - (src10+src13)*6 + (src9 +src14)*3 - (src8 +src15));\
        OP(dst[12*dstStride], (src12+src13)*20 - (src11+src14)*6 + (src10+src15)*3 - (src9 +src16));\
        OP(dst[13*dstStride], (src13+src14)*20 - (src12+src15)*6 + (src11+src16)*3 - (src10+src16));\
        OP(dst[14*dstStride], (src14+src15)*20 - (src13+src16)*6 + (src12+src16)*3 - (src11+src15));\
        OP(dst[15*dstStride], (src15+src16)*20 - (src14+src16)*6 + (src13+src15)*3 - (src12+src14));\
        dst++;\
        src++;\
    }\
}\
\
static void OPNAME ## qpel8_mc10_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t half[64];\
    put ## RND ## mpeg4_qpel8_h_lowpass(half, src, 8, stride, 8);\
    OPNAME ## pixels8_l2(dst, src, half, stride, stride, 8, 8);\
}\
\
static void OPNAME ## qpel8_mc20_c(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## mpeg4_qpel8_h_lowpass(dst, src, stride, stride, 8);\
}\
\
static void OPNAME ## qpel8_mc30_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t half[64];\
    put ## RND ## mpeg4_qpel8_h_lowpass(half, src, 8, stride, 8);\
    OPNAME ## pixels8_l2(dst, src+1, half, stride, stride, 8, 8);\
}\
\
static void OPNAME ## qpel8_mc01_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[16*9];\
    uint8_t half[64];\
    copy_block9(full, src, 16, stride, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass(half, full, 8, 16);\
    OPNAME ## pixels8_l2(dst, full, half, stride, 16, 8, 8);\
}\
\
static void OPNAME ## qpel8_mc02_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[16*9];\
    copy_block9(full, src, 16, stride, 9);\
    OPNAME ## mpeg4_qpel8_v_lowpass(dst, full, stride, 16);\
}\
\
static void OPNAME ## qpel8_mc03_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[16*9];\
    uint8_t half[64];\
    copy_block9(full, src, 16, stride, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass(half, full, 8, 16);\
    OPNAME ## pixels8_l2(dst, full+16, half, stride, 16, 8, 8);\
}\
void ff_ ## OPNAME ## qpel8_mc11_old_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[16*9];\
    uint8_t halfH[72];\
    uint8_t halfV[64];\
    uint8_t halfHV[64];\
    copy_block9(full, src, 16, stride, 9);\
    put ## RND ## mpeg4_qpel8_h_lowpass(halfH, full, 8, 16, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfV, full, 8, 16);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l4(dst, full, halfH, halfV, halfHV, stride, 16, 8, 8, 8, 8);\
}\
static void OPNAME ## qpel8_mc11_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[16*9];\
    uint8_t halfH[72];\
    uint8_t halfHV[64];\
    copy_block9(full, src, 16, stride, 9);\
    put ## RND ## mpeg4_qpel8_h_lowpass(halfH, full, 8, 16, 9);\
    put ## RND ## pixels8_l2(halfH, halfH, full, 8, 8, 16, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l2(dst, halfH, halfHV, stride, 8, 8, 8);\
}\
void ff_ ## OPNAME ## qpel8_mc31_old_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[16*9];\
    uint8_t halfH[72];\
    uint8_t halfV[64];\
    uint8_t halfHV[64];\
    copy_block9(full, src, 16, stride, 9);\
    put ## RND ## mpeg4_qpel8_h_lowpass(halfH, full, 8, 16, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfV, full+1, 8, 16);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l4(dst, full+1, halfH, halfV, halfHV, stride, 16, 8, 8, 8, 8);\
}\
static void OPNAME ## qpel8_mc31_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[16*9];\
    uint8_t halfH[72];\
    uint8_t halfHV[64];\
    copy_block9(full, src, 16, stride, 9);\
    put ## RND ## mpeg4_qpel8_h_lowpass(halfH, full, 8, 16, 9);\
    put ## RND ## pixels8_l2(halfH, halfH, full+1, 8, 8, 16, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l2(dst, halfH, halfHV, stride, 8, 8, 8);\
}\
void ff_ ## OPNAME ## qpel8_mc13_old_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[16*9];\
    uint8_t halfH[72];\
    uint8_t halfV[64];\
    uint8_t halfHV[64];\
    copy_block9(full, src, 16, stride, 9);\
    put ## RND ## mpeg4_qpel8_h_lowpass(halfH, full, 8, 16, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfV, full, 8, 16);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l4(dst, full+16, halfH+8, halfV, halfHV, stride, 16, 8, 8, 8, 8);\
}\
static void OPNAME ## qpel8_mc13_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[16*9];\
    uint8_t halfH[72];\
    uint8_t halfHV[64];\
    copy_block9(full, src, 16, stride, 9);\
    put ## RND ## mpeg4_qpel8_h_lowpass(halfH, full, 8, 16, 9);\
    put ## RND ## pixels8_l2(halfH, halfH, full, 8, 8, 16, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l2(dst, halfH+8, halfHV, stride, 8, 8, 8);\
}\
void ff_ ## OPNAME ## qpel8_mc33_old_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[16*9];\
    uint8_t halfH[72];\
    uint8_t halfV[64];\
    uint8_t halfHV[64];\
    copy_block9(full, src, 16, stride, 9);\
    put ## RND ## mpeg4_qpel8_h_lowpass(halfH, full  , 8, 16, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfV, full+1, 8, 16);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l4(dst, full+17, halfH+8, halfV, halfHV, stride, 16, 8, 8, 8, 8);\
}\
static void OPNAME ## qpel8_mc33_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[16*9];\
    uint8_t halfH[72];\
    uint8_t halfHV[64];\
    copy_block9(full, src, 16, stride, 9);\
    put ## RND ## mpeg4_qpel8_h_lowpass(halfH, full, 8, 16, 9);\
    put ## RND ## pixels8_l2(halfH, halfH, full+1, 8, 8, 16, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l2(dst, halfH+8, halfHV, stride, 8, 8, 8);\
}\
static void OPNAME ## qpel8_mc21_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t halfH[72];\
    uint8_t halfHV[64];\
    put ## RND ## mpeg4_qpel8_h_lowpass(halfH, src, 8, stride, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l2(dst, halfH, halfHV, stride, 8, 8, 8);\
}\
static void OPNAME ## qpel8_mc23_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t halfH[72];\
    uint8_t halfHV[64];\
    put ## RND ## mpeg4_qpel8_h_lowpass(halfH, src, 8, stride, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l2(dst, halfH+8, halfHV, stride, 8, 8, 8);\
}\
void ff_ ## OPNAME ## qpel8_mc12_old_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[16*9];\
    uint8_t halfH[72];\
    uint8_t halfV[64];\
    uint8_t halfHV[64];\
    copy_block9(full, src, 16, stride, 9);\
    put ## RND ## mpeg4_qpel8_h_lowpass(halfH, full, 8, 16, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfV, full, 8, 16);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l2(dst, halfV, halfHV, stride, 8, 8, 8);\
}\
static void OPNAME ## qpel8_mc12_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[16*9];\
    uint8_t halfH[72];\
    copy_block9(full, src, 16, stride, 9);\
    put ## RND ## mpeg4_qpel8_h_lowpass(halfH, full, 8, 16, 9);\
    put ## RND ## pixels8_l2(halfH, halfH, full, 8, 8, 16, 9);\
    OPNAME ## mpeg4_qpel8_v_lowpass(dst, halfH, stride, 8);\
}\
void ff_ ## OPNAME ## qpel8_mc32_old_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[16*9];\
    uint8_t halfH[72];\
    uint8_t halfV[64];\
    uint8_t halfHV[64];\
    copy_block9(full, src, 16, stride, 9);\
    put ## RND ## mpeg4_qpel8_h_lowpass(halfH, full, 8, 16, 9);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfV, full+1, 8, 16);\
    put ## RND ## mpeg4_qpel8_v_lowpass(halfHV, halfH, 8, 8);\
    OPNAME ## pixels8_l2(dst, halfV, halfHV, stride, 8, 8, 8);\
}\
static void OPNAME ## qpel8_mc32_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[16*9];\
    uint8_t halfH[72];\
    copy_block9(full, src, 16, stride, 9);\
    put ## RND ## mpeg4_qpel8_h_lowpass(halfH, full, 8, 16, 9);\
    put ## RND ## pixels8_l2(halfH, halfH, full+1, 8, 8, 16, 9);\
    OPNAME ## mpeg4_qpel8_v_lowpass(dst, halfH, stride, 8);\
}\
static void OPNAME ## qpel8_mc22_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t halfH[72];\
    put ## RND ## mpeg4_qpel8_h_lowpass(halfH, src, 8, stride, 9);\
    OPNAME ## mpeg4_qpel8_v_lowpass(dst, halfH, stride, 8);\
}\
\
static void OPNAME ## qpel16_mc10_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t half[256];\
    put ## RND ## mpeg4_qpel16_h_lowpass(half, src, 16, stride, 16);\
    OPNAME ## pixels16_l2(dst, src, half, stride, stride, 16, 16);\
}\
\
static void OPNAME ## qpel16_mc20_c(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## mpeg4_qpel16_h_lowpass(dst, src, stride, stride, 16);\
}\
\
static void OPNAME ## qpel16_mc30_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t half[256];\
    put ## RND ## mpeg4_qpel16_h_lowpass(half, src, 16, stride, 16);\
    OPNAME ## pixels16_l2(dst, src+1, half, stride, stride, 16, 16);\
}\
\
static void OPNAME ## qpel16_mc01_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[24*17];\
    uint8_t half[256];\
    copy_block17(full, src, 24, stride, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass(half, full, 16, 24);\
    OPNAME ## pixels16_l2(dst, full, half, stride, 24, 16, 16);\
}\
\
static void OPNAME ## qpel16_mc02_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[24*17];\
    copy_block17(full, src, 24, stride, 17);\
    OPNAME ## mpeg4_qpel16_v_lowpass(dst, full, stride, 24);\
}\
\
static void OPNAME ## qpel16_mc03_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[24*17];\
    uint8_t half[256];\
    copy_block17(full, src, 24, stride, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass(half, full, 16, 24);\
    OPNAME ## pixels16_l2(dst, full+24, half, stride, 24, 16, 16);\
}\
void ff_ ## OPNAME ## qpel16_mc11_old_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[24*17];\
    uint8_t halfH[272];\
    uint8_t halfV[256];\
    uint8_t halfHV[256];\
    copy_block17(full, src, 24, stride, 17);\
    put ## RND ## mpeg4_qpel16_h_lowpass(halfH, full, 16, 24, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfV, full, 16, 24);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l4(dst, full, halfH, halfV, halfHV, stride, 24, 16, 16, 16, 16);\
}\
static void OPNAME ## qpel16_mc11_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[24*17];\
    uint8_t halfH[272];\
    uint8_t halfHV[256];\
    copy_block17(full, src, 24, stride, 17);\
    put ## RND ## mpeg4_qpel16_h_lowpass(halfH, full, 16, 24, 17);\
    put ## RND ## pixels16_l2(halfH, halfH, full, 16, 16, 24, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l2(dst, halfH, halfHV, stride, 16, 16, 16);\
}\
void ff_ ## OPNAME ## qpel16_mc31_old_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[24*17];\
    uint8_t halfH[272];\
    uint8_t halfV[256];\
    uint8_t halfHV[256];\
    copy_block17(full, src, 24, stride, 17);\
    put ## RND ## mpeg4_qpel16_h_lowpass(halfH, full, 16, 24, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfV, full+1, 16, 24);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l4(dst, full+1, halfH, halfV, halfHV, stride, 24, 16, 16, 16, 16);\
}\
static void OPNAME ## qpel16_mc31_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[24*17];\
    uint8_t halfH[272];\
    uint8_t halfHV[256];\
    copy_block17(full, src, 24, stride, 17);\
    put ## RND ## mpeg4_qpel16_h_lowpass(halfH, full, 16, 24, 17);\
    put ## RND ## pixels16_l2(halfH, halfH, full+1, 16, 16, 24, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l2(dst, halfH, halfHV, stride, 16, 16, 16);\
}\
void ff_ ## OPNAME ## qpel16_mc13_old_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[24*17];\
    uint8_t halfH[272];\
    uint8_t halfV[256];\
    uint8_t halfHV[256];\
    copy_block17(full, src, 24, stride, 17);\
    put ## RND ## mpeg4_qpel16_h_lowpass(halfH, full, 16, 24, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfV, full, 16, 24);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l4(dst, full+24, halfH+16, halfV, halfHV, stride, 24, 16, 16, 16, 16);\
}\
static void OPNAME ## qpel16_mc13_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[24*17];\
    uint8_t halfH[272];\
    uint8_t halfHV[256];\
    copy_block17(full, src, 24, stride, 17);\
    put ## RND ## mpeg4_qpel16_h_lowpass(halfH, full, 16, 24, 17);\
    put ## RND ## pixels16_l2(halfH, halfH, full, 16, 16, 24, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l2(dst, halfH+16, halfHV, stride, 16, 16, 16);\
}\
void ff_ ## OPNAME ## qpel16_mc33_old_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[24*17];\
    uint8_t halfH[272];\
    uint8_t halfV[256];\
    uint8_t halfHV[256];\
    copy_block17(full, src, 24, stride, 17);\
    put ## RND ## mpeg4_qpel16_h_lowpass(halfH, full  , 16, 24, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfV, full+1, 16, 24);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l4(dst, full+25, halfH+16, halfV, halfHV, stride, 24, 16, 16, 16, 16);\
}\
static void OPNAME ## qpel16_mc33_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[24*17];\
    uint8_t halfH[272];\
    uint8_t halfHV[256];\
    copy_block17(full, src, 24, stride, 17);\
    put ## RND ## mpeg4_qpel16_h_lowpass(halfH, full, 16, 24, 17);\
    put ## RND ## pixels16_l2(halfH, halfH, full+1, 16, 16, 24, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l2(dst, halfH+16, halfHV, stride, 16, 16, 16);\
}\
static void OPNAME ## qpel16_mc21_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t halfH[272];\
    uint8_t halfHV[256];\
    put ## RND ## mpeg4_qpel16_h_lowpass(halfH, src, 16, stride, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l2(dst, halfH, halfHV, stride, 16, 16, 16);\
}\
static void OPNAME ## qpel16_mc23_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t halfH[272];\
    uint8_t halfHV[256];\
    put ## RND ## mpeg4_qpel16_h_lowpass(halfH, src, 16, stride, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l2(dst, halfH+16, halfHV, stride, 16, 16, 16);\
}\
void ff_ ## OPNAME ## qpel16_mc12_old_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[24*17];\
    uint8_t halfH[272];\
    uint8_t halfV[256];\
    uint8_t halfHV[256];\
    copy_block17(full, src, 24, stride, 17);\
    put ## RND ## mpeg4_qpel16_h_lowpass(halfH, full, 16, 24, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfV, full, 16, 24);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l2(dst, halfV, halfHV, stride, 16, 16, 16);\
}\
static void OPNAME ## qpel16_mc12_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[24*17];\
    uint8_t halfH[272];\
    copy_block17(full, src, 24, stride, 17);\
    put ## RND ## mpeg4_qpel16_h_lowpass(halfH, full, 16, 24, 17);\
    put ## RND ## pixels16_l2(halfH, halfH, full, 16, 16, 24, 17);\
    OPNAME ## mpeg4_qpel16_v_lowpass(dst, halfH, stride, 16);\
}\
void ff_ ## OPNAME ## qpel16_mc32_old_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[24*17];\
    uint8_t halfH[272];\
    uint8_t halfV[256];\
    uint8_t halfHV[256];\
    copy_block17(full, src, 24, stride, 17);\
    put ## RND ## mpeg4_qpel16_h_lowpass(halfH, full, 16, 24, 17);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfV, full+1, 16, 24);\
    put ## RND ## mpeg4_qpel16_v_lowpass(halfHV, halfH, 16, 16);\
    OPNAME ## pixels16_l2(dst, halfV, halfHV, stride, 16, 16, 16);\
}\
static void OPNAME ## qpel16_mc32_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[24*17];\
    uint8_t halfH[272];\
    copy_block17(full, src, 24, stride, 17);\
    put ## RND ## mpeg4_qpel16_h_lowpass(halfH, full, 16, 24, 17);\
    put ## RND ## pixels16_l2(halfH, halfH, full+1, 16, 16, 24, 17);\
    OPNAME ## mpeg4_qpel16_v_lowpass(dst, halfH, stride, 16);\
}\
static void OPNAME ## qpel16_mc22_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t halfH[272];\
    put ## RND ## mpeg4_qpel16_h_lowpass(halfH, src, 16, stride, 17);\
    OPNAME ## mpeg4_qpel16_v_lowpass(dst, halfH, stride, 16);\
}

#define op_avg(a, b) a = (((a)+cm[((b) + 16)>>5]+1)>>1)
#define op_avg_no_rnd(a, b) a = (((a)+cm[((b) + 15)>>5])>>1)
#define op_put(a, b) a = cm[((b) + 16)>>5]
#define op_put_no_rnd(a, b) a = cm[((b) + 15)>>5]

QPEL_MC(0, put_       , _       , op_put)
QPEL_MC(1, put_no_rnd_, _no_rnd_, op_put_no_rnd)
QPEL_MC(0, avg_       , _       , op_avg)
//QPEL_MC(1, avg_no_rnd , _       , op_avg)
#undef op_avg
#undef op_avg_no_rnd
#undef op_put
#undef op_put_no_rnd

#define put_qpel8_mc00_c  ff_put_pixels8x8_c
#define avg_qpel8_mc00_c  ff_avg_pixels8x8_c
#define put_qpel16_mc00_c ff_put_pixels16x16_c
#define avg_qpel16_mc00_c ff_avg_pixels16x16_c
#define put_no_rnd_qpel8_mc00_c  ff_put_pixels8x8_c
#define put_no_rnd_qpel16_mc00_c ff_put_pixels16x16_c

#if 1
#define H264_LOWPASS(OPNAME, OP, OP2) \
static av_unused void OPNAME ## h264_qpel2_h_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){\
    const int h=2;\
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;\
    int i;\
    for(i=0; i<h; i++)\
    {\
        OP(dst[0], (src[0]+src[1])*20 - (src[-1]+src[2])*5 + (src[-2]+src[3]));\
        OP(dst[1], (src[1]+src[2])*20 - (src[0 ]+src[3])*5 + (src[-1]+src[4]));\
        dst+=dstStride;\
        src+=srcStride;\
    }\
}\
\
static av_unused void OPNAME ## h264_qpel2_v_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){\
    const int w=2;\
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;\
    int i;\
    for(i=0; i<w; i++)\
    {\
        const int srcB= src[-2*srcStride];\
        const int srcA= src[-1*srcStride];\
        const int src0= src[0 *srcStride];\
        const int src1= src[1 *srcStride];\
        const int src2= src[2 *srcStride];\
        const int src3= src[3 *srcStride];\
        const int src4= src[4 *srcStride];\
        OP(dst[0*dstStride], (src0+src1)*20 - (srcA+src2)*5 + (srcB+src3));\
        OP(dst[1*dstStride], (src1+src2)*20 - (src0+src3)*5 + (srcA+src4));\
        dst++;\
        src++;\
    }\
}\
\
static av_unused void OPNAME ## h264_qpel2_hv_lowpass(uint8_t *dst, int16_t *tmp, uint8_t *src, int dstStride, int tmpStride, int srcStride){\
    const int h=2;\
    const int w=2;\
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;\
    int i;\
    src -= 2*srcStride;\
    for(i=0; i<h+5; i++)\
    {\
        tmp[0]= (src[0]+src[1])*20 - (src[-1]+src[2])*5 + (src[-2]+src[3]);\
        tmp[1]= (src[1]+src[2])*20 - (src[0 ]+src[3])*5 + (src[-1]+src[4]);\
        tmp+=tmpStride;\
        src+=srcStride;\
    }\
    tmp -= tmpStride*(h+5-2);\
    for(i=0; i<w; i++)\
    {\
        const int tmpB= tmp[-2*tmpStride];\
        const int tmpA= tmp[-1*tmpStride];\
        const int tmp0= tmp[0 *tmpStride];\
        const int tmp1= tmp[1 *tmpStride];\
        const int tmp2= tmp[2 *tmpStride];\
        const int tmp3= tmp[3 *tmpStride];\
        const int tmp4= tmp[4 *tmpStride];\
        OP2(dst[0*dstStride], (tmp0+tmp1)*20 - (tmpA+tmp2)*5 + (tmpB+tmp3));\
        OP2(dst[1*dstStride], (tmp1+tmp2)*20 - (tmp0+tmp3)*5 + (tmpA+tmp4));\
        dst++;\
        tmp++;\
    }\
}\
static void OPNAME ## h264_qpel4_h_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){\
    const int h=4;\
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;\
    int i;\
    for(i=0; i<h; i++)\
    {\
        OP(dst[0], (src[0]+src[1])*20 - (src[-1]+src[2])*5 + (src[-2]+src[3]));\
        OP(dst[1], (src[1]+src[2])*20 - (src[0 ]+src[3])*5 + (src[-1]+src[4]));\
        OP(dst[2], (src[2]+src[3])*20 - (src[1 ]+src[4])*5 + (src[0 ]+src[5]));\
        OP(dst[3], (src[3]+src[4])*20 - (src[2 ]+src[5])*5 + (src[1 ]+src[6]));\
        dst+=dstStride;\
        src+=srcStride;\
    }\
}\
\
static void OPNAME ## h264_qpel4_v_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){\
    const int w=4;\
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;\
    int i;\
    for(i=0; i<w; i++)\
    {\
        const int srcB= src[-2*srcStride];\
        const int srcA= src[-1*srcStride];\
        const int src0= src[0 *srcStride];\
        const int src1= src[1 *srcStride];\
        const int src2= src[2 *srcStride];\
        const int src3= src[3 *srcStride];\
        const int src4= src[4 *srcStride];\
        const int src5= src[5 *srcStride];\
        const int src6= src[6 *srcStride];\
        OP(dst[0*dstStride], (src0+src1)*20 - (srcA+src2)*5 + (srcB+src3));\
        OP(dst[1*dstStride], (src1+src2)*20 - (src0+src3)*5 + (srcA+src4));\
        OP(dst[2*dstStride], (src2+src3)*20 - (src1+src4)*5 + (src0+src5));\
        OP(dst[3*dstStride], (src3+src4)*20 - (src2+src5)*5 + (src1+src6));\
        dst++;\
        src++;\
    }\
}\
\
static void OPNAME ## h264_qpel4_hv_lowpass(uint8_t *dst, int16_t *tmp, uint8_t *src, int dstStride, int tmpStride, int srcStride){\
    const int h=4;\
    const int w=4;\
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;\
    int i;\
    src -= 2*srcStride;\
    for(i=0; i<h+5; i++)\
    {\
        tmp[0]= (src[0]+src[1])*20 - (src[-1]+src[2])*5 + (src[-2]+src[3]);\
        tmp[1]= (src[1]+src[2])*20 - (src[0 ]+src[3])*5 + (src[-1]+src[4]);\
        tmp[2]= (src[2]+src[3])*20 - (src[1 ]+src[4])*5 + (src[0 ]+src[5]);\
        tmp[3]= (src[3]+src[4])*20 - (src[2 ]+src[5])*5 + (src[1 ]+src[6]);\
        tmp+=tmpStride;\
        src+=srcStride;\
    }\
    tmp -= tmpStride*(h+5-2);\
    for(i=0; i<w; i++)\
    {\
        const int tmpB= tmp[-2*tmpStride];\
        const int tmpA= tmp[-1*tmpStride];\
        const int tmp0= tmp[0 *tmpStride];\
        const int tmp1= tmp[1 *tmpStride];\
        const int tmp2= tmp[2 *tmpStride];\
        const int tmp3= tmp[3 *tmpStride];\
        const int tmp4= tmp[4 *tmpStride];\
        const int tmp5= tmp[5 *tmpStride];\
        const int tmp6= tmp[6 *tmpStride];\
        OP2(dst[0*dstStride], (tmp0+tmp1)*20 - (tmpA+tmp2)*5 + (tmpB+tmp3));\
        OP2(dst[1*dstStride], (tmp1+tmp2)*20 - (tmp0+tmp3)*5 + (tmpA+tmp4));\
        OP2(dst[2*dstStride], (tmp2+tmp3)*20 - (tmp1+tmp4)*5 + (tmp0+tmp5));\
        OP2(dst[3*dstStride], (tmp3+tmp4)*20 - (tmp2+tmp5)*5 + (tmp1+tmp6));\
        dst++;\
        tmp++;\
    }\
}\
\
static void OPNAME ## h264_qpel8_h_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){\
    const int h=8;\
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;\
    int i;\
    for(i=0; i<h; i++)\
    {\
        OP(dst[0], (src[0]+src[1])*20 - (src[-1]+src[2])*5 + (src[-2]+src[3 ]));\
        OP(dst[1], (src[1]+src[2])*20 - (src[0 ]+src[3])*5 + (src[-1]+src[4 ]));\
        OP(dst[2], (src[2]+src[3])*20 - (src[1 ]+src[4])*5 + (src[0 ]+src[5 ]));\
        OP(dst[3], (src[3]+src[4])*20 - (src[2 ]+src[5])*5 + (src[1 ]+src[6 ]));\
        OP(dst[4], (src[4]+src[5])*20 - (src[3 ]+src[6])*5 + (src[2 ]+src[7 ]));\
        OP(dst[5], (src[5]+src[6])*20 - (src[4 ]+src[7])*5 + (src[3 ]+src[8 ]));\
        OP(dst[6], (src[6]+src[7])*20 - (src[5 ]+src[8])*5 + (src[4 ]+src[9 ]));\
        OP(dst[7], (src[7]+src[8])*20 - (src[6 ]+src[9])*5 + (src[5 ]+src[10]));\
        dst+=dstStride;\
        src+=srcStride;\
    }\
}\
\
static void OPNAME ## h264_qpel8_v_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){\
    const int w=8;\
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;\
    int i;\
    for(i=0; i<w; i++)\
    {\
        const int srcB= src[-2*srcStride];\
        const int srcA= src[-1*srcStride];\
        const int src0= src[0 *srcStride];\
        const int src1= src[1 *srcStride];\
        const int src2= src[2 *srcStride];\
        const int src3= src[3 *srcStride];\
        const int src4= src[4 *srcStride];\
        const int src5= src[5 *srcStride];\
        const int src6= src[6 *srcStride];\
        const int src7= src[7 *srcStride];\
        const int src8= src[8 *srcStride];\
        const int src9= src[9 *srcStride];\
        const int src10=src[10*srcStride];\
        OP(dst[0*dstStride], (src0+src1)*20 - (srcA+src2)*5 + (srcB+src3));\
        OP(dst[1*dstStride], (src1+src2)*20 - (src0+src3)*5 + (srcA+src4));\
        OP(dst[2*dstStride], (src2+src3)*20 - (src1+src4)*5 + (src0+src5));\
        OP(dst[3*dstStride], (src3+src4)*20 - (src2+src5)*5 + (src1+src6));\
        OP(dst[4*dstStride], (src4+src5)*20 - (src3+src6)*5 + (src2+src7));\
        OP(dst[5*dstStride], (src5+src6)*20 - (src4+src7)*5 + (src3+src8));\
        OP(dst[6*dstStride], (src6+src7)*20 - (src5+src8)*5 + (src4+src9));\
        OP(dst[7*dstStride], (src7+src8)*20 - (src6+src9)*5 + (src5+src10));\
        dst++;\
        src++;\
    }\
}\
\
static void OPNAME ## h264_qpel8_hv_lowpass(uint8_t *dst, int16_t *tmp, uint8_t *src, int dstStride, int tmpStride, int srcStride){\
    const int h=8;\
    const int w=8;\
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;\
    int i;\
    src -= 2*srcStride;\
    for(i=0; i<h+5; i++)\
    {\
        tmp[0]= (src[0]+src[1])*20 - (src[-1]+src[2])*5 + (src[-2]+src[3 ]);\
        tmp[1]= (src[1]+src[2])*20 - (src[0 ]+src[3])*5 + (src[-1]+src[4 ]);\
        tmp[2]= (src[2]+src[3])*20 - (src[1 ]+src[4])*5 + (src[0 ]+src[5 ]);\
        tmp[3]= (src[3]+src[4])*20 - (src[2 ]+src[5])*5 + (src[1 ]+src[6 ]);\
        tmp[4]= (src[4]+src[5])*20 - (src[3 ]+src[6])*5 + (src[2 ]+src[7 ]);\
        tmp[5]= (src[5]+src[6])*20 - (src[4 ]+src[7])*5 + (src[3 ]+src[8 ]);\
        tmp[6]= (src[6]+src[7])*20 - (src[5 ]+src[8])*5 + (src[4 ]+src[9 ]);\
        tmp[7]= (src[7]+src[8])*20 - (src[6 ]+src[9])*5 + (src[5 ]+src[10]);\
        tmp+=tmpStride;\
        src+=srcStride;\
    }\
    tmp -= tmpStride*(h+5-2);\
    for(i=0; i<w; i++)\
    {\
        const int tmpB= tmp[-2*tmpStride];\
        const int tmpA= tmp[-1*tmpStride];\
        const int tmp0= tmp[0 *tmpStride];\
        const int tmp1= tmp[1 *tmpStride];\
        const int tmp2= tmp[2 *tmpStride];\
        const int tmp3= tmp[3 *tmpStride];\
        const int tmp4= tmp[4 *tmpStride];\
        const int tmp5= tmp[5 *tmpStride];\
        const int tmp6= tmp[6 *tmpStride];\
        const int tmp7= tmp[7 *tmpStride];\
        const int tmp8= tmp[8 *tmpStride];\
        const int tmp9= tmp[9 *tmpStride];\
        const int tmp10=tmp[10*tmpStride];\
        OP2(dst[0*dstStride], (tmp0+tmp1)*20 - (tmpA+tmp2)*5 + (tmpB+tmp3));\
        OP2(dst[1*dstStride], (tmp1+tmp2)*20 - (tmp0+tmp3)*5 + (tmpA+tmp4));\
        OP2(dst[2*dstStride], (tmp2+tmp3)*20 - (tmp1+tmp4)*5 + (tmp0+tmp5));\
        OP2(dst[3*dstStride], (tmp3+tmp4)*20 - (tmp2+tmp5)*5 + (tmp1+tmp6));\
        OP2(dst[4*dstStride], (tmp4+tmp5)*20 - (tmp3+tmp6)*5 + (tmp2+tmp7));\
        OP2(dst[5*dstStride], (tmp5+tmp6)*20 - (tmp4+tmp7)*5 + (tmp3+tmp8));\
        OP2(dst[6*dstStride], (tmp6+tmp7)*20 - (tmp5+tmp8)*5 + (tmp4+tmp9));\
        OP2(dst[7*dstStride], (tmp7+tmp8)*20 - (tmp6+tmp9)*5 + (tmp5+tmp10));\
        dst++;\
        tmp++;\
    }\
}\
\
static void OPNAME ## h264_qpel16_v_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){\
    OPNAME ## h264_qpel8_v_lowpass(dst  , src  , dstStride, srcStride);\
    OPNAME ## h264_qpel8_v_lowpass(dst+8, src+8, dstStride, srcStride);\
    src += 8*srcStride;\
    dst += 8*dstStride;\
    OPNAME ## h264_qpel8_v_lowpass(dst  , src  , dstStride, srcStride);\
    OPNAME ## h264_qpel8_v_lowpass(dst+8, src+8, dstStride, srcStride);\
}\
\
static void OPNAME ## h264_qpel16_h_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){\
    OPNAME ## h264_qpel8_h_lowpass(dst  , src  , dstStride, srcStride);\
    OPNAME ## h264_qpel8_h_lowpass(dst+8, src+8, dstStride, srcStride);\
    src += 8*srcStride;\
    dst += 8*dstStride;\
    OPNAME ## h264_qpel8_h_lowpass(dst  , src  , dstStride, srcStride);\
    OPNAME ## h264_qpel8_h_lowpass(dst+8, src+8, dstStride, srcStride);\
}\
\
static void OPNAME ## h264_qpel16_hv_lowpass(uint8_t *dst, int16_t *tmp, uint8_t *src, int dstStride, int tmpStride, int srcStride){\
    OPNAME ## h264_qpel8_hv_lowpass(dst  , tmp  , src  , dstStride, tmpStride, srcStride);\
    OPNAME ## h264_qpel8_hv_lowpass(dst+8, tmp+8, src+8, dstStride, tmpStride, srcStride);\
    src += 8*srcStride;\
    dst += 8*dstStride;\
    OPNAME ## h264_qpel8_hv_lowpass(dst  , tmp  , src  , dstStride, tmpStride, srcStride);\
    OPNAME ## h264_qpel8_hv_lowpass(dst+8, tmp+8, src+8, dstStride, tmpStride, srcStride);\
}\

#define H264_MC(OPNAME, SIZE) \
static av_unused void OPNAME ## h264_qpel ## SIZE ## _mc00_c (uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## pixels ## SIZE ## _c(dst, src, stride, SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc10_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t half[SIZE*SIZE];\
    put_h264_qpel ## SIZE ## _h_lowpass(half, src, SIZE, stride);\
    OPNAME ## pixels ## SIZE ## _l2(dst, src, half, stride, stride, SIZE, SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc20_c(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## h264_qpel ## SIZE ## _h_lowpass(dst, src, stride, stride);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc30_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t half[SIZE*SIZE];\
    put_h264_qpel ## SIZE ## _h_lowpass(half, src, SIZE, stride);\
    OPNAME ## pixels ## SIZE ## _l2(dst, src+1, half, stride, stride, SIZE, SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc01_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[SIZE*(SIZE+5)];\
    uint8_t * const full_mid= full + SIZE*2;\
    uint8_t half[SIZE*SIZE];\
    copy_block ## SIZE (full, src - stride*2, SIZE,  stride, SIZE + 5);\
    put_h264_qpel ## SIZE ## _v_lowpass(half, full_mid, SIZE, SIZE);\
    OPNAME ## pixels ## SIZE ## _l2(dst, full_mid, half, stride, SIZE, SIZE, SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc02_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[SIZE*(SIZE+5)];\
    uint8_t * const full_mid= full + SIZE*2;\
    copy_block ## SIZE (full, src - stride*2, SIZE,  stride, SIZE + 5);\
    OPNAME ## h264_qpel ## SIZE ## _v_lowpass(dst, full_mid, stride, SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc03_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[SIZE*(SIZE+5)];\
    uint8_t * const full_mid= full + SIZE*2;\
    uint8_t half[SIZE*SIZE];\
    copy_block ## SIZE (full, src - stride*2, SIZE,  stride, SIZE + 5);\
    put_h264_qpel ## SIZE ## _v_lowpass(half, full_mid, SIZE, SIZE);\
    OPNAME ## pixels ## SIZE ## _l2(dst, full_mid+SIZE, half, stride, SIZE, SIZE, SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc11_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[SIZE*(SIZE+5)];\
    uint8_t * const full_mid= full + SIZE*2;\
    uint8_t halfH[SIZE*SIZE];\
    uint8_t halfV[SIZE*SIZE];\
    put_h264_qpel ## SIZE ## _h_lowpass(halfH, src, SIZE, stride);\
    copy_block ## SIZE (full, src - stride*2, SIZE,  stride, SIZE + 5);\
    put_h264_qpel ## SIZE ## _v_lowpass(halfV, full_mid, SIZE, SIZE);\
    OPNAME ## pixels ## SIZE ## _l2(dst, halfH, halfV, stride, SIZE, SIZE, SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc31_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[SIZE*(SIZE+5)];\
    uint8_t * const full_mid= full + SIZE*2;\
    uint8_t halfH[SIZE*SIZE];\
    uint8_t halfV[SIZE*SIZE];\
    put_h264_qpel ## SIZE ## _h_lowpass(halfH, src, SIZE, stride);\
    copy_block ## SIZE (full, src - stride*2 + 1, SIZE,  stride, SIZE + 5);\
    put_h264_qpel ## SIZE ## _v_lowpass(halfV, full_mid, SIZE, SIZE);\
    OPNAME ## pixels ## SIZE ## _l2(dst, halfH, halfV, stride, SIZE, SIZE, SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc13_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[SIZE*(SIZE+5)];\
    uint8_t * const full_mid= full + SIZE*2;\
    uint8_t halfH[SIZE*SIZE];\
    uint8_t halfV[SIZE*SIZE];\
    put_h264_qpel ## SIZE ## _h_lowpass(halfH, src + stride, SIZE, stride);\
    copy_block ## SIZE (full, src - stride*2, SIZE,  stride, SIZE + 5);\
    put_h264_qpel ## SIZE ## _v_lowpass(halfV, full_mid, SIZE, SIZE);\
    OPNAME ## pixels ## SIZE ## _l2(dst, halfH, halfV, stride, SIZE, SIZE, SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc33_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[SIZE*(SIZE+5)];\
    uint8_t * const full_mid= full + SIZE*2;\
    uint8_t halfH[SIZE*SIZE];\
    uint8_t halfV[SIZE*SIZE];\
    put_h264_qpel ## SIZE ## _h_lowpass(halfH, src + stride, SIZE, stride);\
    copy_block ## SIZE (full, src - stride*2 + 1, SIZE,  stride, SIZE + 5);\
    put_h264_qpel ## SIZE ## _v_lowpass(halfV, full_mid, SIZE, SIZE);\
    OPNAME ## pixels ## SIZE ## _l2(dst, halfH, halfV, stride, SIZE, SIZE, SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc22_c(uint8_t *dst, uint8_t *src, int stride){\
    int16_t tmp[SIZE*(SIZE+5)];\
    OPNAME ## h264_qpel ## SIZE ## _hv_lowpass(dst, tmp, src, stride, SIZE, stride);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc21_c(uint8_t *dst, uint8_t *src, int stride){\
    int16_t tmp[SIZE*(SIZE+5)];\
    uint8_t halfH[SIZE*SIZE];\
    uint8_t halfHV[SIZE*SIZE];\
    put_h264_qpel ## SIZE ## _h_lowpass(halfH, src, SIZE, stride);\
    put_h264_qpel ## SIZE ## _hv_lowpass(halfHV, tmp, src, SIZE, SIZE, stride);\
    OPNAME ## pixels ## SIZE ## _l2(dst, halfH, halfHV, stride, SIZE, SIZE, SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc23_c(uint8_t *dst, uint8_t *src, int stride){\
    int16_t tmp[SIZE*(SIZE+5)];\
    uint8_t halfH[SIZE*SIZE];\
    uint8_t halfHV[SIZE*SIZE];\
    put_h264_qpel ## SIZE ## _h_lowpass(halfH, src + stride, SIZE, stride);\
    put_h264_qpel ## SIZE ## _hv_lowpass(halfHV, tmp, src, SIZE, SIZE, stride);\
    OPNAME ## pixels ## SIZE ## _l2(dst, halfH, halfHV, stride, SIZE, SIZE, SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc12_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[SIZE*(SIZE+5)];\
    uint8_t * const full_mid= full + SIZE*2;\
    int16_t tmp[SIZE*(SIZE+5)];\
    uint8_t halfV[SIZE*SIZE];\
    uint8_t halfHV[SIZE*SIZE];\
    copy_block ## SIZE (full, src - stride*2, SIZE,  stride, SIZE + 5);\
    put_h264_qpel ## SIZE ## _v_lowpass(halfV, full_mid, SIZE, SIZE);\
    put_h264_qpel ## SIZE ## _hv_lowpass(halfHV, tmp, src, SIZE, SIZE, stride);\
    OPNAME ## pixels ## SIZE ## _l2(dst, halfV, halfHV, stride, SIZE, SIZE, SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc32_c(uint8_t *dst, uint8_t *src, int stride){\
    uint8_t full[SIZE*(SIZE+5)];\
    uint8_t * const full_mid= full + SIZE*2;\
    int16_t tmp[SIZE*(SIZE+5)];\
    uint8_t halfV[SIZE*SIZE];\
    uint8_t halfHV[SIZE*SIZE];\
    copy_block ## SIZE (full, src - stride*2 + 1, SIZE,  stride, SIZE + 5);\
    put_h264_qpel ## SIZE ## _v_lowpass(halfV, full_mid, SIZE, SIZE);\
    put_h264_qpel ## SIZE ## _hv_lowpass(halfHV, tmp, src, SIZE, SIZE, stride);\
    OPNAME ## pixels ## SIZE ## _l2(dst, halfV, halfHV, stride, SIZE, SIZE, SIZE);\
}\

#define op_avg(a, b)  a = (((a)+cm[((b) + 16)>>5]+1)>>1)
//#define op_avg2(a, b) a = (((a)*w1+cm[((b) + 16)>>5]*w2 + o + 64)>>7)
#define op_put(a, b)  a = cm[((b) + 16)>>5]
#define op2_avg(a, b)  a = (((a)+cm[((b) + 512)>>10]+1)>>1)
#define op2_put(a, b)  a = cm[((b) + 512)>>10]

H264_LOWPASS(put_       , op_put, op2_put)
H264_LOWPASS(avg_       , op_avg, op2_avg)
H264_MC(put_, 2)
H264_MC(put_, 4)
H264_MC(put_, 8)
H264_MC(put_, 16)
H264_MC(avg_, 4)
H264_MC(avg_, 8)
H264_MC(avg_, 16)

#undef op_avg
#undef op_put
#undef op2_avg
#undef op2_put
#endif

#define put_h264_qpel8_mc00_c  ff_put_pixels8x8_c
#define avg_h264_qpel8_mc00_c  ff_avg_pixels8x8_c
#define put_h264_qpel16_mc00_c ff_put_pixels16x16_c
#define avg_h264_qpel16_mc00_c ff_avg_pixels16x16_c

void ff_put_pixels8x8_c(uint8_t *dst, uint8_t *src, int stride) {
    put_pixels8_c(dst, src, stride, 8);
}
void ff_avg_pixels8x8_c(uint8_t *dst, uint8_t *src, int stride) {
    avg_pixels8_c(dst, src, stride, 8);
}
void ff_put_pixels16x16_c(uint8_t *dst, uint8_t *src, int stride) {
    put_pixels16_c(dst, src, stride, 16);
}
void ff_avg_pixels16x16_c(uint8_t *dst, uint8_t *src, int stride) {
    avg_pixels16_c(dst, src, stride, 16);
}

static inline int pix_abs16_c(void *v, uint8_t *pix1, uint8_t *pix2, int line_size, int h)
{
    int s, i;

    s = 0;
    for(i=0;i<h;i++) {
        s += abs(pix1[0] - pix2[0]);
        s += abs(pix1[1] - pix2[1]);
        s += abs(pix1[2] - pix2[2]);
        s += abs(pix1[3] - pix2[3]);
        s += abs(pix1[4] - pix2[4]);
        s += abs(pix1[5] - pix2[5]);
        s += abs(pix1[6] - pix2[6]);
        s += abs(pix1[7] - pix2[7]);
        s += abs(pix1[8] - pix2[8]);
        s += abs(pix1[9] - pix2[9]);
        s += abs(pix1[10] - pix2[10]);
        s += abs(pix1[11] - pix2[11]);
        s += abs(pix1[12] - pix2[12]);
        s += abs(pix1[13] - pix2[13]);
        s += abs(pix1[14] - pix2[14]);
        s += abs(pix1[15] - pix2[15]);
        pix1 += line_size;
        pix2 += line_size;
    }
    return s;
}

static inline int pix_abs8_c(void *v, uint8_t *pix1, uint8_t *pix2, int line_size, int h)
{
    int s, i;

    s = 0;
    for(i=0;i<h;i++) {
        s += abs(pix1[0] - pix2[0]);
        s += abs(pix1[1] - pix2[1]);
        s += abs(pix1[2] - pix2[2]);
        s += abs(pix1[3] - pix2[3]);
        s += abs(pix1[4] - pix2[4]);
        s += abs(pix1[5] - pix2[5]);
        s += abs(pix1[6] - pix2[6]);
        s += abs(pix1[7] - pix2[7]);
        pix1 += line_size;
        pix2 += line_size;
    }
    return s;
}

static int zero_cmp(void *s, uint8_t *a, uint8_t *b, int stride, int h){
    return 0;
}

static void clear_block_c(DCTELEM *block)
{
    memset(block, 0, sizeof(DCTELEM)*64);
}

/**
 * memset(blocks, 0, sizeof(DCTELEM)*6*64)
 */
static void clear_blocks_c(DCTELEM *blocks)
{
    memset(blocks, 0, sizeof(DCTELEM)*6*64);
}

static void add_bytes_c(uint8_t *dst, uint8_t *src, int w){
    long i;
    for(i=0; i<=w-sizeof(long); i+=sizeof(long)){
        long a = *(long*)(src+i);
        long b = *(long*)(dst+i);
        *(long*)(dst+i) = ((a&pb_7f) + (b&pb_7f)) ^ ((a^b)&pb_80);
    }
    for(; i<w; i++)
        dst[i+0] += src[i+0];
}

static void add_bytes_l2_c(uint8_t *dst, uint8_t *src1, uint8_t *src2, int w){
    long i;
    for(i=0; i<=w-sizeof(long); i+=sizeof(long)){
        long a = *(long*)(src1+i);
        long b = *(long*)(src2+i);
        *(long*)(dst+i) = ((a&pb_7f) + (b&pb_7f)) ^ ((a^b)&pb_80);
    }
    for(; i<w; i++)
        dst[i] = src1[i]+src2[i];
}

static void diff_bytes_c(uint8_t *dst, uint8_t *src1, uint8_t *src2, int w){
    long i;
#if !HAVE_FAST_UNALIGNED
    if((long)src2 & (sizeof(long)-1)){
        for(i=0; i+7<w; i+=8){
            dst[i+0] = src1[i+0]-src2[i+0];
            dst[i+1] = src1[i+1]-src2[i+1];
            dst[i+2] = src1[i+2]-src2[i+2];
            dst[i+3] = src1[i+3]-src2[i+3];
            dst[i+4] = src1[i+4]-src2[i+4];
            dst[i+5] = src1[i+5]-src2[i+5];
            dst[i+6] = src1[i+6]-src2[i+6];
            dst[i+7] = src1[i+7]-src2[i+7];
        }
    }else
#endif
    for(i=0; i<=w-sizeof(long); i+=sizeof(long)){
        long a = *(long*)(src1+i);
        long b = *(long*)(src2+i);
        *(long*)(dst+i) = ((a|pb_80) - (b&pb_7f)) ^ ((a^b^pb_80)&pb_80);
    }
    for(; i<w; i++)
        dst[i+0] = src1[i+0]-src2[i+0];
}

static void ff_jref_idct_put(uint8_t *dest, int line_size, DCTELEM *block)
{
    j_rev_dct (block);
    ff_put_pixels_clamped_c(block, dest, line_size);
}
static void ff_jref_idct_add(uint8_t *dest, int line_size, DCTELEM *block)
{
    j_rev_dct (block);
    ff_add_pixels_clamped_c(block, dest, line_size);
}

static void ff_jref_idct4_put(uint8_t *dest, int line_size, DCTELEM *block)
{
    j_rev_dct4 (block);
    put_pixels_clamped4_c(block, dest, line_size);
}
static void ff_jref_idct4_add(uint8_t *dest, int line_size, DCTELEM *block)
{
    j_rev_dct4 (block);
    add_pixels_clamped4_c(block, dest, line_size);
}

static void ff_jref_idct2_put(uint8_t *dest, int line_size, DCTELEM *block)
{
    j_rev_dct2 (block);
    put_pixels_clamped2_c(block, dest, line_size);
}
static void ff_jref_idct2_add(uint8_t *dest, int line_size, DCTELEM *block)
{
    j_rev_dct2 (block);
    add_pixels_clamped2_c(block, dest, line_size);
}

static void ff_jref_idct1_put(uint8_t *dest, int line_size, DCTELEM *block)
{
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    dest[0] = cm[(block[0] + 4)>>3];
}
static void ff_jref_idct1_add(uint8_t *dest, int line_size, DCTELEM *block)
{
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    dest[0] = cm[dest[0] + ((block[0] + 4)>>3)];
}

static void just_return(void *mem av_unused, int stride av_unused, int h av_unused) { return; }

/* init static data */
av_cold void dsputil_static_init(void)
{
    int i;

    for(i=0;i<256;i++) ff_cropTbl[i + MAX_NEG_CROP] = i;
    for(i=0;i<MAX_NEG_CROP;i++) {
        ff_cropTbl[i] = 0;
        ff_cropTbl[i + MAX_NEG_CROP + 256] = 255;
    }

    for(i=0;i<512;i++) {
        ff_squareTbl[i] = (i - 256) * (i - 256);
    }

    for(i=0; i<64; i++) inv_zigzag_direct16[ff_zigzag_direct[i]]= i+1;
}

int ff_check_alignment(void){
    static int did_fail=0;
    DECLARE_ALIGNED(16, int, aligned);

    if((intptr_t)&aligned & 15){
        if(!did_fail){
#if HAVE_MMX || HAVE_ALTIVEC
            av_log(NULL, AV_LOG_ERROR,
                "Compiler did not align stack variables. Libavcodec has been miscompiled\n"
                "and may be very slow or crash. This is not a bug in libavcodec,\n"
                "but in the compiler. You may try recompiling using gcc >= 4.2.\n"
                "Do not report crashes to FFmpeg developers.\n");
#endif
            did_fail=1;
        }
        return -1;
    }
    return 0;
}

av_cold void attribute_align_arg dsputil_init(DSPContext* c, AVCodecContext *avctx)
{
    int i;

    ff_check_alignment();

    if(avctx->lowres==1){
        if(avctx->idct_algo==FF_IDCT_INT || avctx->idct_algo==FF_IDCT_AUTO || !CONFIG_H264_DECODER){
            c->idct_put= ff_jref_idct4_put;
            c->idct_add= ff_jref_idct4_add;
        }else{
            c->idct_put= ff_h264_lowres_idct_put_c;
            c->idct_add= ff_h264_lowres_idct_add_c;
        }
        c->idct    = j_rev_dct4;
        c->idct_permutation_type= FF_NO_IDCT_PERM;
    }else if(avctx->lowres==2){
        c->idct_put= ff_jref_idct2_put;
        c->idct_add= ff_jref_idct2_add;
        c->idct    = j_rev_dct2;
        c->idct_permutation_type= FF_NO_IDCT_PERM;
    }else if(avctx->lowres==3){
        c->idct_put= ff_jref_idct1_put;
        c->idct_add= ff_jref_idct1_add;
        c->idct    = j_rev_dct1;
        c->idct_permutation_type= FF_NO_IDCT_PERM;
    }else{
        if(avctx->idct_algo==FF_IDCT_INT){
            c->idct_put= ff_jref_idct_put;
            c->idct_add= ff_jref_idct_add;
            c->idct    = j_rev_dct;
            c->idct_permutation_type= FF_LIBMPEG2_IDCT_PERM;
        }else if((CONFIG_VP3_DECODER || CONFIG_VP5_DECODER || CONFIG_VP6_DECODER ) &&
                avctx->idct_algo==FF_IDCT_VP3){
            c->idct_put= ff_vp3_idct_put_c;
            c->idct_add= ff_vp3_idct_add_c;
            c->idct    = ff_vp3_idct_c;
            c->idct_permutation_type= FF_NO_IDCT_PERM;
        }else{ //accurate/default
            c->idct_put= ff_simple_idct_put;
            c->idct_add= ff_simple_idct_add;
            c->idct    = ff_simple_idct;
            c->idct_permutation_type= FF_NO_IDCT_PERM;
        }
    }

    c->put_pixels_clamped = ff_put_pixels_clamped_c;
    c->put_signed_pixels_clamped = ff_put_signed_pixels_clamped_c;
    c->add_pixels_clamped = ff_add_pixels_clamped_c;
    c->add_pixels8 = add_pixels8_c;
    c->add_pixels4 = add_pixels4_c;
    c->emulated_edge_mc = ff_emulated_edge_mc;
    c->gmc1 = gmc1_c;
    c->gmc = ff_gmc_c;
    c->clear_block = clear_block_c;
    c->clear_blocks = clear_blocks_c;

#define dspfunc(PFX, IDX, NUM) \
    c->PFX ## _pixels_tab[IDX][0] = PFX ## _pixels ## NUM ## _c;     \
    c->PFX ## _pixels_tab[IDX][1] = PFX ## _pixels ## NUM ## _x2_c;  \
    c->PFX ## _pixels_tab[IDX][2] = PFX ## _pixels ## NUM ## _y2_c;  \
    c->PFX ## _pixels_tab[IDX][3] = PFX ## _pixels ## NUM ## _xy2_c

    dspfunc(put, 0, 16);
    dspfunc(put_no_rnd, 0, 16);
    dspfunc(put, 1, 8);
    dspfunc(put_no_rnd, 1, 8);
    dspfunc(put, 2, 4);
    dspfunc(put, 3, 2);

    dspfunc(avg, 0, 16);
    dspfunc(avg_no_rnd, 0, 16);
    dspfunc(avg, 1, 8);
    dspfunc(avg_no_rnd, 1, 8);
    dspfunc(avg, 2, 4);
    dspfunc(avg, 3, 2);
#undef dspfunc

    c->put_no_rnd_pixels_l2[0]= put_no_rnd_pixels16_l2_c;
    c->put_no_rnd_pixels_l2[1]= put_no_rnd_pixels8_l2_c;

#define dspfunc(PFX, IDX, NUM) \
    c->PFX ## _pixels_tab[IDX][ 0] = PFX ## NUM ## _mc00_c; \
    c->PFX ## _pixels_tab[IDX][ 1] = PFX ## NUM ## _mc10_c; \
    c->PFX ## _pixels_tab[IDX][ 2] = PFX ## NUM ## _mc20_c; \
    c->PFX ## _pixels_tab[IDX][ 3] = PFX ## NUM ## _mc30_c; \
    c->PFX ## _pixels_tab[IDX][ 4] = PFX ## NUM ## _mc01_c; \
    c->PFX ## _pixels_tab[IDX][ 5] = PFX ## NUM ## _mc11_c; \
    c->PFX ## _pixels_tab[IDX][ 6] = PFX ## NUM ## _mc21_c; \
    c->PFX ## _pixels_tab[IDX][ 7] = PFX ## NUM ## _mc31_c; \
    c->PFX ## _pixels_tab[IDX][ 8] = PFX ## NUM ## _mc02_c; \
    c->PFX ## _pixels_tab[IDX][ 9] = PFX ## NUM ## _mc12_c; \
    c->PFX ## _pixels_tab[IDX][10] = PFX ## NUM ## _mc22_c; \
    c->PFX ## _pixels_tab[IDX][11] = PFX ## NUM ## _mc32_c; \
    c->PFX ## _pixels_tab[IDX][12] = PFX ## NUM ## _mc03_c; \
    c->PFX ## _pixels_tab[IDX][13] = PFX ## NUM ## _mc13_c; \
    c->PFX ## _pixels_tab[IDX][14] = PFX ## NUM ## _mc23_c; \
    c->PFX ## _pixels_tab[IDX][15] = PFX ## NUM ## _mc33_c

    dspfunc(put_h264_qpel, 0, 16);
    dspfunc(put_h264_qpel, 1, 8);
    dspfunc(put_h264_qpel, 2, 4);
    dspfunc(put_h264_qpel, 3, 2);
    dspfunc(avg_h264_qpel, 0, 16);
    dspfunc(avg_h264_qpel, 1, 8);
    dspfunc(avg_h264_qpel, 2, 4);

#undef dspfunc
    c->put_h264_chroma_pixels_tab[0]= put_h264_chroma_mc8_c;
    c->put_h264_chroma_pixels_tab[1]= put_h264_chroma_mc4_c;
    c->put_h264_chroma_pixels_tab[2]= put_h264_chroma_mc2_c;
    c->avg_h264_chroma_pixels_tab[0]= avg_h264_chroma_mc8_c;
    c->avg_h264_chroma_pixels_tab[1]= avg_h264_chroma_mc4_c;
    c->avg_h264_chroma_pixels_tab[2]= avg_h264_chroma_mc2_c;

    c->draw_edges = draw_edges_c;

#define SET_CMP_FUNC(name) \
    c->name[0]= name ## 16_c;\
    c->name[1]= name ## 8x8_c;

    c->sad[0]= pix_abs16_c;
    c->sad[1]= pix_abs8_c;

    if (CONFIG_VP3_DECODER) {
        c->vp3_h_loop_filter= ff_vp3_h_loop_filter_c;
        c->vp3_v_loop_filter= ff_vp3_v_loop_filter_c;
        c->vp3_idct_dc_add= ff_vp3_idct_dc_add_c;
    }

    c->prefetch= just_return;

    memset(c->put_2tap_qpel_pixels_tab, 0, sizeof(c->put_2tap_qpel_pixels_tab));
    memset(c->avg_2tap_qpel_pixels_tab, 0, sizeof(c->avg_2tap_qpel_pixels_tab));

    if (HAVE_MMX)        dsputil_init_mmx   (c, avctx);

    for(i=0; i<64; i++){
        if(!c->put_2tap_qpel_pixels_tab[0][i])
            c->put_2tap_qpel_pixels_tab[0][i]= c->put_h264_qpel_pixels_tab[0][i];
        if(!c->avg_2tap_qpel_pixels_tab[0][i])
            c->avg_2tap_qpel_pixels_tab[0][i]= c->avg_h264_qpel_pixels_tab[0][i];
    }

    switch(c->idct_permutation_type){
    case FF_NO_IDCT_PERM:
        for(i=0; i<64; i++)
            c->idct_permutation[i]= i;
        break;
    case FF_LIBMPEG2_IDCT_PERM:
        for(i=0; i<64; i++)
            c->idct_permutation[i]= (i & 0x38) | ((i & 6) >> 1) | ((i & 1) << 2);
        break;
    case FF_SIMPLE_IDCT_PERM:
        for(i=0; i<64; i++)
            c->idct_permutation[i]= simple_mmx_permutation[i];
        break;
    case FF_TRANSPOSE_IDCT_PERM:
        for(i=0; i<64; i++)
            c->idct_permutation[i]= ((i&7)<<3) | (i>>3);
        break;
    case FF_PARTTRANS_IDCT_PERM:
        for(i=0; i<64; i++)
            c->idct_permutation[i]= (i&0x24) | ((i&3)<<3) | ((i>>3)&3);
        break;
    case FF_SSE2_IDCT_PERM:
        for(i=0; i<64; i++)
            c->idct_permutation[i]= (i&0x38) | idct_sse2_row_perm[i&7];
        break;
    default:
        av_log(avctx, AV_LOG_ERROR, "Internal error, IDCT permutation not set\n");
    }
}

// avcodec_get_current_idct,avcodec_get_encoder_info by h.yamagata
// It's caller's responsibility to check avctx->priv_data is MpegEncContext*.
const char* avcodec_get_current_idct(AVCodecContext *avctx)
{
    MpegEncContext *s = avctx->priv_data;
    DSPContext *c = &s->dsp;

    if (c->idct_put==ff_jref_idct_put)
        return "Integer (ff_jref_idct)";
    if (c->idct_put==ff_jref_idct1_put)
        return "Integer (ff_jref_idct1)";
    if (c->idct_put==ff_jref_idct1_put)
        return "Integer (ff_jref_idct2)";
    if (c->idct_put==ff_jref_idct4_put)
        return "Integer (ff_jref_idct4)";
    if (c->idct_put==ff_h264_lowres_idct_put_c)
        return "H.264 (ff_h264_lowres_idct_c)";
    if (c->idct_put==ff_vp3_idct_put_c)
        return "VP3 (ff_vp3_idct_c)";
    if (c->idct_put==ff_simple_idct_put)
        return "Simple IDCT (simple_idct)";
#if HAVE_MMX
    return avcodec_get_current_idct_mmx(avctx,c);
#else
    return "";
#endif
}

// It's caller's responsibility to check avctx->priv_data is MpegEncContext*.
void avcodec_get_encoder_info(AVCodecContext *avctx,int *xvid_build,int *divx_version,int *divx_build,int *lavc_build)
{
    MpegEncContext *s = avctx->priv_data;
    *xvid_build = s->xvid_build;
    *divx_version = s->divx_version;
    *divx_build = s->divx_build;
    *lavc_build = s->lavc_build;
}
