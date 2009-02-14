#ifndef _TVIDEOCODEC_FFMPEG_MT_H_
#define _TVIDEOCODEC_FFMPEG_MT_H_

// Do not include avcodec.h in this file, ffmpeg and ffmpeg-mt may conflict.

#define COMPILE_AS_FFMPEG_MT 1
#define TvideoCodecLibavcodec TvideoCodecLibavcodec_mt
#define Tlibavcodec Tlibavcodec_mt
#define TlibavcodecExt TlibavcodecExt_mt
#undef _TVIDEOCODECLIBAVCODEC_H_
#undef _TLIBAVCODEC_H_
#include "TvideoCodecLibavcodec.h"
#undef TvideoCodecLibavcodec
#undef Tlibavcodec
#undef TlibavcodecExt
#undef COMPILE_AS_FFMPEG_MT

#endif // _TVIDEOCODEC_FFMPEG_MT_H_
