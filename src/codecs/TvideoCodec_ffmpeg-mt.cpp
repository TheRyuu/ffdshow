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

#include "stdafx.h"
#include "ffmpeg-mt/libavcodec/avcodec.h"
#include "ffmpeg-mt/libavcodec/dvdata.h"

#define COMPILE_AS_FFMPEG_MT 1
#define TvideoCodecLibavcodec TvideoCodecLibavcodec_mt
#define Tlibavcodec Tlibavcodec_mt
#define TlibavcodecExt TlibavcodecExt_mt
#undef _TVIDEOCODECLIBAVCODEC_H_
#undef _TLIBAVCODEC_H_
#include "TvideoCodecLibavcodec.cpp"
#undef TvideoCodecLibavcodec
#undef Tlibavcodec
#undef TlibavcodecExt
#undef COMPILE_AS_FFMPEG_MT
