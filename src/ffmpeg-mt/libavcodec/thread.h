/*
 * Multithreading support
 * Copyright (c) 2008 Alexander Strange <astrange@ithinksw.com>
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
 * @file thread.h
 * Multithreading support header.
 * @author Alexander Strange <astrange@ithinksw.com>
 */

#ifndef AVCODEC_THREAD_H
#define AVCODEC_THREAD_H

#include "config.h"
#include "avcodec.h"

/**
 * Waits for decoding threads to finish and resets the internal
 * state. Called by avcodec_flush_buffers().
 */
void ff_thread_flush(AVCodecContext *avctx);

/**
 * Submits a new frame to a decoding thread. Parameters are the
 * same as avcodec_decode_video2(). Returns the earliest available
 * decoded picture.
 *
 * NULL AVFrames returned from the codec will be dropped if
 * the client passes NULL data in.
 */
int ff_thread_decode_frame(AVCodecContext *avctx,
                        void *data, int *data_size,
                        AVPacket *avpkt);

/**
 * For codecs which define update_thread_context.
 * Call this when the context is set up for the next frame to be
 * decoded. The next decoding thread will start afterwards.
 * The codec must not modify parts of the context read by
 * update_thread_context after calling this or it will cause a race
 * condition.
 */
void ff_thread_finish_setup(AVCodecContext *avctx);

/**
 * Call this when some part of the picture is finished decoding.
 * Later calls with lower progress values will be ignored.
 *
 * @param f The AVFrame containing the current field or frame
 * @param progress The highest-numbered part finished so far
 * @param field The field being decoded, for field pictures.
 * 0 for top field or progressive, 1 for bottom.
 */
void ff_thread_report_progress(AVFrame *f, int progress, int field);

/**
 * Call this before accessing some part of a previous field or frame.
 * Returns after the previous decoding thread has called ff_thread_report_progress()
 * with sufficiently high progress.
 *
 * @param f The AVFrame containing the reference field or frame
 * @param progress The highest-numbered part of the reference picture to wait for
 * @param field The field being referenced, for field pictures.
 * 0 for top field or progressive, 1 for bottom.
 */
void ff_thread_await_progress(AVFrame *f, int progress, int field);

/**
 * Convenience function to set progress for both fields to INT_MAX.
 * Can be used to prevent deadlocks in later threads when a decoder
 * exits early due to errors.
 *
 * @param f The frame or field picture being decoded.
 */
void ff_thread_finish_frame(AVFrame *f);

/**
 * Replacement for get_buffer() for frame-level threading.
 *
 * Codecs with CODEC_CAP_FRAME_THREADS must call this instead
 * of calling get_buffer() directly.
 */
int ff_thread_get_buffer(AVCodecContext *avctx, AVFrame *f);

/**
 * Replacement for release_buffer() for frame-level threading.
 *
 * Codecs with CODEC_CAP_FRAME_THREADS must call this instead
 * of calling release_buffer() directly.
 *
 * On return, \p f->data will be cleared.
 */
void ff_thread_release_buffer(AVCodecContext *avctx, AVFrame *f);

// ffdshow custom code. return pointer to the copied AVCodecContext for thread 0.
AVCodecContext* get_thread0_avctx(AVCodecContext *avctx);

#endif /* AVCODEC_THREAD_H */
