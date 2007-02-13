/*
 * Buffered I/O for ffmpeg system
 * Copyright (c) 2000,2001 Fabrice Bellard
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
#include "avformat.h"
#include "avio.h"
#include <stdarg.h>

#define IO_BUFFER_SIZE 32768

static void fill_buffer(ByteIOContext *s);

int init_put_byte(ByteIOContext *s,
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                  offset_t (*seek)(void *opaque, offset_t offset, int whence))
{
    s->buffer = buffer;
    s->buffer_size = buffer_size;
    s->buf_ptr = buffer;
    s->write_flag = write_flag;
    if (!s->write_flag)
        s->buf_end = buffer;
    else
        s->buf_end = buffer + buffer_size;
    s->opaque = opaque;
    s->write_packet = write_packet;
    s->read_packet = read_packet;
    s->seek = seek;
    s->pos = 0;
    s->must_flush = 0;
    s->eof_reached = 0;
    s->error = 0;
    s->is_streamed = 0;
    s->max_packet_size = 0;
    s->update_checksum= NULL;
    if(!read_packet && !write_flag){
        s->pos = buffer_size;
        s->buf_end = s->buffer + buffer_size;
    }
    return 0;
}

static void flush_buffer(ByteIOContext *s)
{
    if (s->buf_ptr > s->buffer) {
        if (s->write_packet && !s->error){
            int ret= s->write_packet(s->opaque, s->buffer, s->buf_ptr - s->buffer);
            if(ret < 0){
                s->error = ret;
            }
        }
        if(s->update_checksum){
            s->checksum= s->update_checksum(s->checksum, s->checksum_ptr, s->buf_ptr - s->checksum_ptr);
            s->checksum_ptr= s->buffer;
        }
        s->pos += s->buf_ptr - s->buffer;
    }
    s->buf_ptr = s->buffer;
}

void put_byte(ByteIOContext *s, int b)
{
    *(s->buf_ptr)++ = b;
    if (s->buf_ptr >= s->buf_end)
        flush_buffer(s);
}

void put_buffer(ByteIOContext *s, const unsigned char *buf, int size)
{
    int len;

    while (size > 0) {
        len = (s->buf_end - s->buf_ptr);
        if (len > size)
            len = size;
        memcpy(s->buf_ptr, buf, len);
        s->buf_ptr += len;

        if (s->buf_ptr >= s->buf_end)
            flush_buffer(s);

        buf += len;
        size -= len;
    }
}

int url_ferror(ByteIOContext *s)
{
    return s->error;
}

/* Input stream */

static void fill_buffer(ByteIOContext *s)
{
    int len;

    /* no need to do anything if EOF already reached */
    if (s->eof_reached)
        return;

    if(s->update_checksum){
        if(s->buf_end > s->checksum_ptr)
            s->checksum= s->update_checksum(s->checksum, s->checksum_ptr, s->buf_end - s->checksum_ptr);
        s->checksum_ptr= s->buffer;
    }

    len = s->read_packet(s->opaque, s->buffer, s->buffer_size);
    if (len <= 0) {
        /* do not modify buffer if EOF reached so that a seek back can
           be done without rereading data */
        s->eof_reached = 1;
    if(len<0)
        s->error= len;
    } else {
        s->pos += len;
        s->buf_ptr = s->buffer;
        s->buf_end = s->buffer + len;
    }
}

/* NOTE: return 0 if EOF, so you cannot use it if EOF handling is
   necessary */
/* XXX: put an inline version */
int get_byte(ByteIOContext *s)
{
    if (s->buf_ptr < s->buf_end) {
        return *s->buf_ptr++;
    } else {
        fill_buffer(s);
        if (s->buf_ptr < s->buf_end)
            return *s->buf_ptr++;
        else
            return 0;
    }
}

int get_buffer(ByteIOContext *s, unsigned char *buf, int size)
{
    int len, size1;

    size1 = size;
    while (size > 0) {
        len = s->buf_end - s->buf_ptr;
        if (len > size)
            len = size;
        if (len == 0) {
            if(size > s->buffer_size && !s->update_checksum){
                len = s->read_packet(s->opaque, buf, size);
                if (len <= 0) {
                    /* do not modify buffer if EOF reached so that a seek back can
                    be done without rereading data */
                    s->eof_reached = 1;
                    if(len<0)
                        s->error= len;
                    break;
                } else {
                    s->pos += len;
                    size -= len;
                    buf += len;
                    s->buf_ptr = s->buffer;
                    s->buf_end = s->buffer/* + len*/;
                }
            }else{
                fill_buffer(s);
                len = s->buf_end - s->buf_ptr;
                if (len == 0)
                    break;
            }
        } else {
            memcpy(buf, s->buf_ptr, len);
            buf += len;
            s->buf_ptr += len;
            size -= len;
        }
    }
    return size1 - size;
}

#ifdef CONFIG_ENCODERS
static int url_write_packet(void *opaque, uint8_t *buf, int buf_size)
{
    URLContext *h = opaque;
    return url_write(h, buf, buf_size);
}
#else
#define         url_write_packet NULL
#endif //CONFIG_ENCODERS

static int url_read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    URLContext *h = opaque;
    return url_read(h, buf, buf_size);
}

static offset_t url_seek_packet(void *opaque, offset_t offset, int whence)
{
    URLContext *h = opaque;
    return url_seek(h, offset, whence);
    //return 0;
}

int url_fdopen(ByteIOContext *s, URLContext *h)
{
    uint8_t *buffer;
    int buffer_size, max_packet_size;


    max_packet_size = url_get_max_packet_size(h);
    if (max_packet_size) {
        buffer_size = max_packet_size; /* no need to bufferize more than one packet */
    } else {
        buffer_size = IO_BUFFER_SIZE;
    }
    buffer = av_malloc(buffer_size);
    if (!buffer)
        return AVERROR(ENOMEM);

    if (init_put_byte(s, buffer, buffer_size,
                      (h->flags & URL_WRONLY || h->flags & URL_RDWR), h,
                      url_read_packet, url_write_packet, url_seek_packet) < 0) {
        av_free(buffer);
        return AVERROR_IO;
    }
    s->is_streamed = h->is_streamed;
    s->max_packet_size = max_packet_size;
    return 0;
}

/* NOTE: when opened as read/write, the buffers are only used for
   reading */
int url_fopen(ByteIOContext *s, const char *filename, int flags)
{
    URLContext *h;
    int err;

    err = url_open(&h, filename, flags);
    if (err < 0)
        return err;
    err = url_fdopen(s, h);
    if (err < 0) {
        url_close(h);
        return err;
    }
    return 0;
}

int url_fclose(ByteIOContext *s)
{
    URLContext *h = s->opaque;

    av_free(s->buffer);
    memset(s, 0, sizeof(ByteIOContext));
    return url_close(h);
}
