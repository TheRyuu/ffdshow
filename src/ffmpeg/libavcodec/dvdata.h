/*
 * Constants for DV codec
 * Copyright (c) 2002 Fabrice Bellard
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
 * Constants for DV codec.
 */

#ifndef AVCODEC_DVDATA_H
#define AVCODEC_DVDATA_H

#include "libavutil/rational.h"
#include "avcodec.h"

#ifndef FF_ARRAY_ELEMS
#define FF_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))
#endif

/* ffdshow custom code */
enum  {
  DV_PROFILE_AUTO=-1
};

typedef struct DVwork_chunk {
    uint16_t  buf_offset;
    uint16_t  mb_coordinates[5];
} DVwork_chunk;

/*
 * DVprofile is used to express the differences between various
 * DV flavors. For now it's primarily used for differentiating
 * 525/60 and 625/50, but the plans are to use it for various
 * DV specs as well (e.g. SMPTE314M vs. IEC 61834).
 */
typedef struct DVprofile {
    int              dsf;                   /* value of the dsf in the DV header */
    int              video_stype;           /* stype for VAUX source pack */
    int              frame_size;            /* total size of one frame in bytes */
    int              difseg_size;           /* number of DIF segments per DIF channel */
    int              n_difchan;             /* number of DIF channels per frame */
    AVRational       time_base;             /* 1/framerate */
    int              ltc_divisor;           /* FPS from the LTS standpoint */
    int              height;                /* picture height in pixels */
    int              width;                 /* picture width in pixels */
    AVRational       sar[2];                /* sample aspect ratios for 4:3 and 16:9 */
    DVwork_chunk    *work_chunks;           /* each thread gets its own chunk of frame to work on */
    uint32_t        *idct_factor;           /* set of iDCT factor tables */
    enum PixelFormat pix_fmt;               /* picture pixel format */
    int              bpm;                   /* blocks per macroblock */
    const uint8_t   *block_sizes;           /* AC block sizes, in bits */
    int              audio_stride;          /* size of audio_shuffle table */
    int              audio_min_samples[3];  /* min amount of audio samples */
                                            /* for 48kHz, 44.1kHz and 32kHz */
    int              audio_samples_dist[5]; /* how many samples are supposed to be */
                                            /* in each frame in a 5 frames window */
    const uint8_t  (*audio_shuffle)[9];     /* PCM shuffling table */
    const char *name; /* ffdshow custom code */
} DVprofile;

static DVwork_chunk work_chunks_dv25pal   [1*12*27];
static DVwork_chunk work_chunks_dv25pal411[1*12*27];
static DVwork_chunk work_chunks_dv25ntsc  [1*10*27];
static DVwork_chunk work_chunks_dv50pal   [2*12*27];
static DVwork_chunk work_chunks_dv50ntsc  [2*10*27];
static DVwork_chunk work_chunks_dv100palp [2*12*27];
static DVwork_chunk work_chunks_dv100ntscp[2*10*27];
static DVwork_chunk work_chunks_dv100pali [4*12*27];
static DVwork_chunk work_chunks_dv100ntsci[4*10*27];

static uint32_t dv_idct_factor_sd    [2*2*22*64];
static uint32_t dv_idct_factor_hd1080[2*4*16*64];
static uint32_t dv_idct_factor_hd720 [2*4*16*64];

static const uint8_t dv_audio_shuffle525[10][9] = {
  {  0, 30, 60, 20, 50, 80, 10, 40, 70 }, /* 1st channel */
  {  6, 36, 66, 26, 56, 86, 16, 46, 76 },
  { 12, 42, 72,  2, 32, 62, 22, 52, 82 },
  { 18, 48, 78,  8, 38, 68, 28, 58, 88 },
  { 24, 54, 84, 14, 44, 74,  4, 34, 64 },

  {  1, 31, 61, 21, 51, 81, 11, 41, 71 }, /* 2nd channel */
  {  7, 37, 67, 27, 57, 87, 17, 47, 77 },
  { 13, 43, 73,  3, 33, 63, 23, 53, 83 },
  { 19, 49, 79,  9, 39, 69, 29, 59, 89 },
  { 25, 55, 85, 15, 45, 75,  5, 35, 65 },
};

static const uint8_t dv_audio_shuffle625[12][9] = {
  {   0,  36,  72,  26,  62,  98,  16,  52,  88}, /* 1st channel */
  {   6,  42,  78,  32,  68, 104,  22,  58,  94},
  {  12,  48,  84,   2,  38,  74,  28,  64, 100},
  {  18,  54,  90,   8,  44,  80,  34,  70, 106},
  {  24,  60,  96,  14,  50,  86,   4,  40,  76},
  {  30,  66, 102,  20,  56,  92,  10,  46,  82},

  {   1,  37,  73,  27,  63,  99,  17,  53,  89}, /* 2nd channel */
  {   7,  43,  79,  33,  69, 105,  23,  59,  95},
  {  13,  49,  85,   3,  39,  75,  29,  65, 101},
  {  19,  55,  91,   9,  45,  81,  35,  71, 107},
  {  25,  61,  97,  15,  51,  87,   5,  41,  77},
  {  31,  67, 103,  21,  57,  93,  11,  47,  83},
};

/* macroblock bit budgets */
static const uint8_t block_sizes_dv2550[8] = {
    112, 112, 112, 112, 80, 80, 0, 0,
};

static const DVprofile dv_profiles[] = {
    { /*.dsf = */0,
      /*.video_stype = */0x0,
      /*.frame_size = */120000,        /* IEC 61834, SMPTE-314M - 525/60 (NTSC) */
      /*.difseg_size = */10,
      /*.n_difchan = */1,
      /*.time_base = */{ 1001, 30000 },
      /*.ltc_divisor = */30,
      /*.height = */480,
      /*.width = */720,
      /*.sar = */{{8, 9}, {32, 27}},
      /*.work_chunks = */&work_chunks_dv25ntsc[0],
      /*.idct_factor = */&dv_idct_factor_sd[0],
      /*.pix_fmt = */PIX_FMT_YUV411P,
      /*.bpm = */6,
      /*.block_sizes = */block_sizes_dv2550,
      /*.audio_stride = */90,
      /*.audio_min_samples  = */{ 1580, 1452, 1053 }, /* for 48, 44.1 and 32kHz */
      /*.audio_samples_dist = */{ 1600, 1602, 1602, 1602, 1602 }, /* per SMPTE-314M */
      /*.audio_shuffle = */dv_audio_shuffle525,
      "IEC 61834, SMPTE-314M - 525/60 (NTSC)",
    },
    { /*.dsf = */1,
      /*.video_stype = */0x0,
      /*.frame_size = */144000,        /* IEC 61834 - 625/50 (PAL) */
      /*.difseg_size = */12,
      /*.n_difchan = */1,
      /*.time_base = */{ 1, 25 },
      /*.ltc_divisor = */25,
      /*.height = */576,
      /*.width = */720,
      /*.sar = */{{16, 15}, {64, 45}},
      /*.work_chunks = */&work_chunks_dv25pal[0],
      /*.idct_factor = */&dv_idct_factor_sd[0],
      /*.pix_fmt = */PIX_FMT_YUV420P,
      /*.bpm = */6,
      /*.block_sizes = */block_sizes_dv2550,
      /*.audio_stride = */108,
      /*.audio_min_samples  = */{ 1896, 1742, 1264 }, /* for 48, 44.1 and 32kHz */
      /*.audio_samples_dist = */{ 1920, 1920, 1920, 1920, 1920 },
      /*.audio_shuffle = */dv_audio_shuffle625,
      "IEC 61834 - 625/50 (PAL)",
    },
    { /*.dsf = */1,
      /*.video_stype = */0x0,
      /*.frame_size = */144000,        /* SMPTE-314M - 625/50 (PAL) */
      /*.difseg_size = */12,
      /*.n_difchan = */1,
      /*.time_base = */{ 1, 25 },
      /*.ltc_divisor = */25,
      /*.height = */576,
      /*.width = */720,
      /*.sar = */{{16, 15}, {64, 45}},
      /*.work_chunks = */&work_chunks_dv25pal411[0],
      /*.idct_factor = */&dv_idct_factor_sd[0],
      /*.pix_fmt = */PIX_FMT_YUV411P,
      /*.bpm = */6,
      /*.block_sizes = */block_sizes_dv2550,
      /*.audio_stride = */108,
      /*.audio_min_samples  = */{ 1896, 1742, 1264 }, /* for 48, 44.1 and 32kHz */
      /*.audio_samples_dist = */{ 1920, 1920, 1920, 1920, 1920 },
      /*.audio_shuffle = */dv_audio_shuffle625,
      "SMPTE-314M - 625/50 (PAL)",
    },
    { /*.dsf = */0,
      /*.video_stype = */0x4,
      /*.frame_size = */240000,        /* SMPTE-314M - 525/60 (NTSC) 50 Mbps */
      /*.difseg_size = */10,           /* also known as "DVCPRO50" */
      /*.n_difchan = */2,
      /*.time_base = */{ 1001, 30000 },
      /*.ltc_divisor = */30,
      /*.height = */480,
      /*.width = */720,
      /*.sar = */{{8, 9}, {32, 27}},
      /*.work_chunks = */&work_chunks_dv50ntsc[0],
      /*.idct_factor = */&dv_idct_factor_sd[0],
      /*.pix_fmt = */PIX_FMT_YUV422P,
      /*.bpm = */6,
      /*.block_sizes = */block_sizes_dv2550,
      /*.audio_stride = */90,
      /*.audio_min_samples  = */{ 1580, 1452, 1053 }, /* for 48, 44.1 and 32kHz */
      /*.audio_samples_dist = */{ 1600, 1602, 1602, 1602, 1602 }, /* per SMPTE-314M */
      /*.audio_shuffle = */dv_audio_shuffle525,
      "SMPTE-314M - 525/60 (NTSC) 50 Mbps",
    },
    { /*.dsf = */1,
      /*.video_stype = */0x4,
      /*.frame_size = */288000,        /* SMPTE-314M - 625/50 (PAL) 50 Mbps */
      /*.difseg_size = */12,           /* also known as "DVCPRO50" */
      /*.n_difchan = */2,
      /*.time_base = */{ 1, 25 },
      /*.ltc_divisor = */25,
      /*.height = */576,
      /*.width = */720,
      /*.sar = */{{16, 15}, {64, 45}},
      /*.work_chunks = */&work_chunks_dv50pal[0],
      /*.idct_factor = */&dv_idct_factor_sd[0],
      /*.pix_fmt = */PIX_FMT_YUV422P,
      /*.bpm = */6,
      /*.block_sizes = */block_sizes_dv2550,
      /*.audio_stride = */108,
      /*.audio_min_samples  = */{ 1896, 1742, 1264 }, /* for 48, 44.1 and 32kHz */
      /*.audio_samples_dist = */{ 1920, 1920, 1920, 1920, 1920 },
      /*.audio_shuffle = */dv_audio_shuffle625,
      "SMPTE-314M - 625/50 (PAL) 50 Mbps",
    },
#if 0
    { /*.dsf = */0,
      /*.video_stype = */0x14,
      /*.frame_size = */480000,        /* SMPTE-370M - 1080i60 100 Mbps */
      /*.difseg_size = */10,           /* also known as "DVCPRO HD" */
      /*.n_difchan = */4,
      /*.time_base = */{ 1001, 30000 },
      /*.ltc_divisor = */30,
      /*.height = */1080,
      /*.width = */1280,
      /*.sar = */{{1, 1}, {3, 2}},
      /*.work_chunks = */&work_chunks_dv100ntsci[0],
      /*.idct_factor = */&dv_idct_factor_hd1080[0],
      /*.pix_fmt = */PIX_FMT_YUV422P,
      /*.bpm = */8,
      /*.block_sizes = */block_sizes_dv100,
      /*.audio_stride = */90,
      /*.audio_min_samples  = */{ 1580, 1452, 1053 }, /* for 48, 44.1 and 32kHz */
      /*.audio_samples_dist = */{ 1600, 1602, 1602, 1602, 1602 }, /* per SMPTE-314M */
      /*.audio_shuffle = */dv_audio_shuffle525,
      "SMPTE-370M - 1080i60 100 Mbps",
    },
    { /*.dsf = */1,
      /*.video_stype = */0x14,
      /*.frame_size = */576000,        /* SMPTE-370M - 1080i50 100 Mbps */
      /*.difseg_size = */12,           /* also known as "DVCPRO HD" */
      /*.n_difchan = */4,
      /*.time_base = */{ 1, 25 },
      /*.ltc_divisor = */25,
      /*.height = */1080,
      /*.width = */1440,
      /*.sar = */{{1, 1}, {4, 3}},
      /*.work_chunks = */&work_chunks_dv100pali[0],
      /*.idct_factor = */&dv_idct_factor_hd1080[0],
      /*.pix_fmt = */PIX_FMT_YUV422P,
      /*.bpm = */8,
      /*.block_sizes = */block_sizes_dv100,
      /*.audio_stride = */108,
      /*.audio_min_samples  = */{ 1896, 1742, 1264 }, /* for 48, 44.1 and 32kHz */
      /*.audio_samples_dist = */{ 1920, 1920, 1920, 1920, 1920 },
      /*.audio_shuffle = */dv_audio_shuffle625,
      "SMPTE-370M - 1080i50 100 Mbps",
    },
    { /*.dsf = */0,
      /*.video_stype = */0x18,
      /*.frame_size = */240000,        /* SMPTE-370M - 720p60 100 Mbps */
      /*.difseg_size = */10,           /* also known as "DVCPRO HD" */
      /*.n_difchan = */2,
      /*.time_base = */{ 1001, 60000 },
      /*.ltc_divisor = */60,
      /*.height = */720,
      /*.width = */960,
      /*.sar = */{{1, 1}, {4, 3}},
      /*.work_chunks = */&work_chunks_dv100ntscp[0],
      /*.idct_factor = */&dv_idct_factor_hd720[0],
      /*.pix_fmt = */PIX_FMT_YUV422P,
      /*.bpm = */8,
      /*.block_sizes = */block_sizes_dv100,
      /*.audio_stride = */90,
      /*.audio_min_samples  = */{ 1580, 1452, 1053 }, /* for 48, 44.1 and 32kHz */
      /*.audio_samples_dist = */{ 1600, 1602, 1602, 1602, 1602 }, /* per SMPTE-314M */
      /*.audio_shuffle = */dv_audio_shuffle525,
      "SMPTE-370M - 720p60 100 Mbps",
    },
    { /*.dsf = */1,
      /*.video_stype = */0x18,
      /*.frame_size = */288000,        /* SMPTE-370M - 720p50 100 Mbps */
      /*.difseg_size = */12,           /* also known as "DVCPRO HD" */
      /*.n_difchan = */2,
      /*.time_base = */{ 1, 50 },
      /*.ltc_divisor = */50,
      /*.height = */720,
      /*.width = */960,
      /*.sar = */{{1, 1}, {4, 3}},
      /*.work_chunks = */&work_chunks_dv100palp[0],
      /*.idct_factor = */&dv_idct_factor_hd720[0],
      /*.pix_fmt = */PIX_FMT_YUV422P,
      /*.bpm = */8,
      /*.block_sizes = */block_sizes_dv100,
      /*.audio_stride = */90,
      /*.audio_min_samples  = */{ 1896, 1742, 1264 }, /* for 48, 44.1 and 32kHz */
      /*.audio_samples_dist = */{ 1920, 1920, 1920, 1920, 1920 },
      /*.audio_shuffle = */dv_audio_shuffle625,
      "SMPTE-370M - 720p50 100 Mbps",
    },
#endif
    { /*.dsf = */1,
      /*.video_stype = */0x1,
      /*.frame_size = */144000,        /* IEC 61883-5 - 625/50 (PAL) */
      /*.difseg_size = */12,
      /*.n_difchan = */1,
      /*.time_base = */{ 1, 25 },
      /*.ltc_divisor = */25,
      /*.height = */576,
      /*.width = */720,
      /*.sar = */{{16, 15}, {64, 45}},
      /*.work_chunks = */&work_chunks_dv25pal[0],
      /*.idct_factor = */&dv_idct_factor_sd[0],
      /*.pix_fmt = */PIX_FMT_YUV420P,
      /*.bpm = */6,
      /*.block_sizes = */block_sizes_dv2550,
      /*.audio_stride = */108,
      /*.audio_min_samples  = */{ 1896, 1742, 1264 }, /* for 48, 44.1 and 32kHz */
      /*.audio_samples_dist = */{ 1920, 1920, 1920, 1920, 1920 },
      /*.audio_shuffle = */dv_audio_shuffle625,
      "IEC 61883-5 - 625/50 (PAL)",
    }
};

enum dv_section_type {
     dv_sect_header  = 0x1f,
     dv_sect_subcode = 0x3f,
     dv_sect_vaux    = 0x56,
     dv_sect_audio   = 0x76,
     dv_sect_video   = 0x96,
};

enum dv_pack_type {
     dv_header525     = 0x3f, /* see dv_write_pack for important details on */
     dv_header625     = 0xbf, /* these two packs */
     dv_timecode      = 0x13,
     dv_audio_source  = 0x50,
     dv_audio_control = 0x51,
     dv_audio_recdate = 0x52,
     dv_audio_rectime = 0x53,
     dv_video_source  = 0x60,
     dv_video_control = 0x61,
     dv_video_recdate = 0x62,
     dv_video_rectime = 0x63,
     dv_unknown_pack  = 0xff,
};

#define DV_PROFILE_IS_HD(p) ((p)->video_stype & 0x10)
#define DV_PROFILE_IS_1080i50(p) (((p)->video_stype == 0x14) && ((p)->dsf == 1))
#define DV_PROFILE_IS_720p50(p)  (((p)->video_stype == 0x18) && ((p)->dsf == 1))

/* minimum number of bytes to read from a DV stream in order to
   determine the profile */
#define DV_PROFILE_BYTES (6*80) /* 6 DIF blocks */

/**
 * largest possible DV frame, in bytes (1080i50)
 */
#define DV_MAX_FRAME_SIZE 576000

/**
 * maximum number of blocks per macroblock in any DV format
 */
#define DV_MAX_BPM 8

static const DVprofile* avpriv_dv_frame_profile(const DVprofile *sys,
                                  const uint8_t* frame, unsigned buf_size)
{
    int i, dsf, stype;

    if (buf_size < 80 * 5 + 48 + 4)
        return NULL;

    dsf = (frame[3] & 0x80) >> 7;
    stype = frame[80 * 5 + 48 + 3] & 0x1f;

    /* 576i50 25Mbps 4:1:1 is a special case */
    if (dsf == 1 && stype == 0 && frame[4] & 0x07 /* the APT field */) {
        return &dv_profiles[2];
    }

    for (i = 0; i < FF_ARRAY_ELEMS(dv_profiles); i++)
        if (dsf == dv_profiles[i].dsf && stype == dv_profiles[i].video_stype)
            return &dv_profiles[i];

    /* check if old sys matches and assumes corrupted input */
    if (sys && buf_size == sys->frame_size)
        return sys;

    return NULL;
}

static const DVprofile* avpriv_dv_codec_profile(AVCodecContext* codec)
{
    int i;

    for (i=0; i<FF_ARRAY_ELEMS(dv_profiles); i++)
       if (codec->height  == dv_profiles[i].height  &&
           codec->pix_fmt == dv_profiles[i].pix_fmt &&
           codec->width   == dv_profiles[i].width)
               return &dv_profiles[i];

    return NULL;
}

#endif /* AVCODEC_DVDATA_H */
