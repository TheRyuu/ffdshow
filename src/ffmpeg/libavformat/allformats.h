/*
 * Register all the formats and protocols.
 * copyright (c) 2000, 2001, 2002 Fabrice Bellard
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

#ifndef ALLFORMATS_H
#define ALLFORMATS_H

/* mpeg.c */
extern AVInputFormat mpegps_demux;
int mpegps_init(void);

/* mpegts.c */
extern AVInputFormat mpegts_demux;
int mpegts_init(void);

/* rm.c */
int rm_init(void);

/* crc.c */
int crc_init(void);

/* img.c */
int img_init(void);

/* img2.c */
int img2_init(void);

/* asf.c */
int asf_init(void);

/* avienc.c */
int avienc_init(void);

/* avidec.c */
int avidec_init(void);

/* swf.c */
int swf_init(void);

/* mov.c */
int mov_init(void);

/* movenc.c */
int movenc_init(void);

/* flvdec.c */
int flvdec_init(void);

extern AVOutputFormat flv_muxer;

/* jpeg.c */
int jpeg_init(void);

/* gif.c */
int gif_init(void);

/* au.c */
int au_init(void);

/* amr.c */
int amr_init(void);

/* wav.c */
int ff_wav_init(void);

/* mmf.c */
int ff_mmf_init(void);

/* raw.c */
int pcm_read_seek(AVFormatContext *s, 
                  int stream_index, int64_t timestamp, int flags);
int raw_init(void);

/* mp3.c */
int mp3_init(void);

/* yuv4mpeg.c */
int yuv4mpeg_init(void);

/* ogg2.c */
int ogg_init(void);

/* ogg.c */
int libogg_init(void);

/* dv.c */
int ff_dv_init(void);

/* ffm.c */
int ffm_init(void);

/* rtsp.c */
extern AVInputFormat redir_demux;
int redir_open(AVFormatContext **ic_ptr, ByteIOContext *f);

/* 4xm.c */
int fourxm_init(void);

/* psxstr.c */
int str_init(void);

/* idroq.c */
int roq_init(void);

/* ipmovie.c */
int ipmovie_init(void);

/* nut.c */
int nut_init(void);

/* wc3movie.c */
int wc3_init(void);

/* westwood.c */
int westwood_init(void);

/* segafilm.c */
int film_init(void);

/* idcin.c */
int idcin_init(void);

/* flic.c */
int flic_init(void);

/* sierravmd.c */
int vmd_init(void);

/* matroska.c */
int matroska_init(void);

/* sol.c */
int sol_init(void);

/* electronicarts.c */
int ea_init(void);

/* nsvdec.c */
int nsvdec_init(void);
/* yuv4mpeg.c */
extern AVOutputFormat yuv4mpegpipe_oformat;

#endif
