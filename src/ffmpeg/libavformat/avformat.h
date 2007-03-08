/*
 * copyright (c) 2001 Fabrice Bellard
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

#ifndef AVFORMAT_H
#define AVFORMAT_H

#ifdef __cplusplus
extern "C" {
#endif

#define LIBAVFORMAT_VERSION_INT ((51<<16)+(10<<8)+0)
#define LIBAVFORMAT_VERSION     51.10.0
#define LIBAVFORMAT_BUILD       LIBAVFORMAT_VERSION_INT

#define LIBAVFORMAT_IDENT       "Lavf" AV_STRINGIFY(LIBAVFORMAT_VERSION)

#include <time.h>
#include <stdio.h>  /* FILE */
#include "libavcodec/avcodec.h"

#ifdef HAVE_AV_CONFIG_H

void pstrcpy(char *buf, int buf_size, const char *str);
char *pstrcat(char *buf, int buf_size, const char *s);

#endif /* HAVE_AV_CONFIG_H */

#ifdef __cplusplus
}
#endif

#endif /* AVFORMAT_H */





