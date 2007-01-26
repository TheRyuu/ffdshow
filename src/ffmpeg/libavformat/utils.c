/*
 * Various utilities for ffmpeg system
 * Copyright (c) 2000, 2001, 2002 Fabrice Bellard
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
#include "allformats.h"
#include "opt.h"

#undef NDEBUG
#include <assert.h>

/**
 * @file libavformat/utils.c
 * Various utility functions for using ffmpeg library.
 */

static void av_frac_init(AVFrac *f, int64_t val, int64_t num, int64_t den);
static void av_frac_add(AVFrac *f, int64_t incr);
static void av_frac_set(AVFrac *f, int64_t val);

/** head of registered input format linked list. */
AVInputFormat *first_iformat = NULL;
/** head of registered output format linked list. */
AVOutputFormat *first_oformat = NULL;

void av_register_input_format(AVInputFormat *format)
{
    AVInputFormat **p;
    p = &first_iformat;
    while (*p != NULL) p = &(*p)->next;
    *p = format;
    format->next = NULL;
}

void av_register_output_format(AVOutputFormat *format)
{
    AVOutputFormat **p;
    p = &first_oformat;
    while (*p != NULL) p = &(*p)->next;
    *p = format;
    format->next = NULL;
}

int match_ext(const char *filename, const char *extensions)
{
    const char *ext, *p;
    char ext1[32], *q;

    if(!filename)
        return 0;

    ext = strrchr(filename, '.');
    if (ext) {
        ext++;
        p = extensions;
        for(;;) {
            q = ext1;
            while (*p != '\0' && *p != ',' && q-ext1<sizeof(ext1)-1)
                *q++ = *p++;
            *q = '\0';
            if (!strcasecmp(ext1, ext))
                return 1;
            if (*p == '\0')
                break;
            p++;
        }
    }
    return 0;
}

AVOutputFormat *guess_format(const char *short_name, const char *filename,
                             const char *mime_type)
{
    AVOutputFormat *fmt, *fmt_found;
    int score_max, score;

    /* find the proper file type */
    fmt_found = NULL;
    score_max = 0;
    fmt = first_oformat;
    while (fmt != NULL) {
        score = 0;
        if (fmt->name && short_name && !strcmp(fmt->name, short_name))
            score += 100;
        if (fmt->mime_type && mime_type && !strcmp(fmt->mime_type, mime_type))
            score += 10;
        if (filename && fmt->extensions &&
            match_ext(filename, fmt->extensions)) {
            score += 5;
        }
        if (score > score_max) {
            score_max = score;
            fmt_found = fmt;
        }
        fmt = fmt->next;
    }
    return fmt_found;
}

/* memory handling */

/**
 * Default packet destructor.
 */
void av_destruct_packet(AVPacket *pkt)
{
    av_free(pkt->data);
    pkt->data = NULL; pkt->size = 0;
}

/**
 * Allocate the payload of a packet and intialized its fields to default values.
 *
 * @param pkt packet
 * @param size wanted payload size
 * @return 0 if OK. AVERROR_xxx otherwise.
 */
int av_new_packet(AVPacket *pkt, int size)
{
    uint8_t *data;
    if((unsigned)size > (unsigned)size + FF_INPUT_BUFFER_PADDING_SIZE)
        return AVERROR_NOMEM;
    data = av_malloc(size + FF_INPUT_BUFFER_PADDING_SIZE);
    if (!data)
        return AVERROR_NOMEM;
    memset(data + size, 0, FF_INPUT_BUFFER_PADDING_SIZE);

    av_init_packet(pkt);
    pkt->data = data;
    pkt->size = size;
    pkt->destruct = av_destruct_packet;
    return 0;
}

/* This is a hack - the packet memory allocation stuff is broken. The
   packet is allocated if it was not really allocated */
int av_dup_packet(AVPacket *pkt)
{
    if (pkt->destruct != av_destruct_packet) {
        uint8_t *data;
        /* we duplicate the packet and don't forget to put the padding
           again */
        if((unsigned)pkt->size > (unsigned)pkt->size + FF_INPUT_BUFFER_PADDING_SIZE)
            return AVERROR_NOMEM;
        data = av_malloc(pkt->size + FF_INPUT_BUFFER_PADDING_SIZE);
        if (!data) {
            return AVERROR_NOMEM;
        }
        memcpy(data, pkt->data, pkt->size);
        memset(data + pkt->size, 0, FF_INPUT_BUFFER_PADDING_SIZE);
        pkt->data = data;
        pkt->destruct = av_destruct_packet;
    }
    return 0;
}

/************************************************************/
/* input media file */

/**
 * Open a media file from an IO stream. 'fmt' must be specified.
 */
static const char* format_to_name(void* ptr)
{
    AVFormatContext* fc = (AVFormatContext*) ptr;
    if(fc->iformat) return fc->iformat->name;
    else if(fc->oformat) return fc->oformat->name;
    else return "NULL";
}

#define OFFSET(x) offsetof(AVFormatContext,x)
#define DEFAULT 0 //should be NAN but it doesnt work as its not a constant in glibc as required by ANSI/ISO C
//these names are too long to be readable
#define E AV_OPT_FLAG_ENCODING_PARAM
#define D AV_OPT_FLAG_DECODING_PARAM

static const AVOption options[]={
{"probesize", NULL, OFFSET(probesize), FF_OPT_TYPE_INT, 32000, 32, INT_MAX, D}, /* 32000 from mpegts.c: 1.0 second at 24Mbit/s */
{"muxrate", "set mux rate", OFFSET(mux_rate), FF_OPT_TYPE_INT, DEFAULT, 0, INT_MAX, E},
{"packetsize", "set packet size", OFFSET(packet_size), FF_OPT_TYPE_INT, DEFAULT, 0, INT_MAX, E},
{"fflags", NULL, OFFSET(flags), FF_OPT_TYPE_FLAGS, DEFAULT, INT_MIN, INT_MAX, D|E, "fflags"},
{"ignidx", "ignore index", 0, FF_OPT_TYPE_CONST, AVFMT_FLAG_IGNIDX, INT_MIN, INT_MAX, D, "fflags"},
{"genpts", "generate pts", 0, FF_OPT_TYPE_CONST, AVFMT_FLAG_GENPTS, INT_MIN, INT_MAX, D, "fflags"},
{"track", " set the track number", OFFSET(track), FF_OPT_TYPE_INT, DEFAULT, 0, INT_MAX, E},
{"year", "set the year", OFFSET(year), FF_OPT_TYPE_INT, DEFAULT, INT_MIN, INT_MAX, E},
{NULL},
};

#undef E
#undef D
#undef DEFAULT

static const AVClass av_format_context_class = { "AVFormatContext", format_to_name, options };

#if LIBAVFORMAT_VERSION_INT >= ((51<<16)+(0<<8)+0)
static
#endif
void avformat_get_context_defaults(AVFormatContext *s){
    memset(s, 0, sizeof(AVFormatContext));

    s->av_class = &av_format_context_class;

    av_opt_set_defaults(s);
}

/*******************************************************/

/**
 * Read a transport packet from a media file.
 *
 * This function is absolete and should never be used.
 * Use av_read_frame() instead.
 *
 * @param s media file handle
 * @param pkt is filled
 * @return 0 if OK. AVERROR_xxx if error.
 */
int av_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    return s->iformat->read_packet(s, pkt);
}

/**********************************************************/

/**
 * Get the number of samples of an audio frame. Return (-1) if error.
 */
static int get_audio_frame_size(AVCodecContext *enc, int size)
{
    int frame_size;

    if (enc->frame_size <= 1) {
        int bits_per_sample = av_get_bits_per_sample(enc->codec_id);

        if (bits_per_sample) {
            if (enc->channels == 0)
                return -1;
            frame_size = (size << 3) / (bits_per_sample * enc->channels);
        } else {
            /* used for example by ADPCM codecs */
            if (enc->bit_rate == 0)
                return -1;
            frame_size = (size * 8 * enc->sample_rate) / enc->bit_rate;
        }
    } else {
        frame_size = enc->frame_size;
    }
    return frame_size;
}


/**
 * Return the frame duration in seconds, return 0 if not available.
 */
static void compute_frame_duration(int *pnum, int *pden, AVStream *st,
                                   AVCodecParserContext *pc, AVPacket *pkt)
{
    int frame_size;

    *pnum = 0;
    *pden = 0;
    switch(st->codec->codec_type) {
    case CODEC_TYPE_VIDEO:
        if(st->time_base.num*1000LL > st->time_base.den){
            *pnum = st->time_base.num;
            *pden = st->time_base.den;
        }else if(st->codec->time_base.num*1000LL > st->codec->time_base.den){
            *pnum = st->codec->time_base.num;
            *pden = st->codec->time_base.den;
            if (pc && pc->repeat_pict) {
                *pden *= 2;
                *pnum = (*pnum) * (2 + pc->repeat_pict);
            }
        }
        break;
    case CODEC_TYPE_AUDIO:
        frame_size = get_audio_frame_size(st->codec, pkt->size);
        if (frame_size < 0)
            break;
        *pnum = frame_size;
        *pden = st->codec->sample_rate;
        break;
    default:
        break;
    }
}

static int is_intra_only(AVCodecContext *enc){
    if(enc->codec_type == CODEC_TYPE_AUDIO){
        return 1;
    }else if(enc->codec_type == CODEC_TYPE_VIDEO){
        switch(enc->codec_id){
        case CODEC_ID_MJPEG:
        case CODEC_ID_MJPEGB:
        case CODEC_ID_LJPEG:
//        case CODEC_ID_RAWVIDEO:
        case CODEC_ID_DVVIDEO:
        case CODEC_ID_HUFFYUV:
        case CODEC_ID_FFVHUFF:
        case CODEC_ID_ASV1:
        case CODEC_ID_ASV2:
        case CODEC_ID_VCR1:
            return 1;
        default: break;
        }
    }
    return 0;
}

static int64_t lsb2full(int64_t lsb, int64_t last_ts, int lsb_bits){
    int64_t mask = lsb_bits < 64 ? (1LL<<lsb_bits)-1 : -1LL;
    int64_t delta= last_ts - mask/2;
    return  ((lsb - delta)&mask) + delta;
}

void av_destruct_packet_nofree(AVPacket *pkt)
{
    pkt->data = NULL; pkt->size = 0;
}

/* XXX: suppress the packet queue */
static void flush_packet_queue(AVFormatContext *s)
{
    AVPacketList *pktl;

    for(;;) {
        pktl = s->packet_buffer;
        if (!pktl)
            break;
        s->packet_buffer = pktl->next;
        av_free_packet(&pktl->pkt);
        av_free(pktl);
    }
}

/*******************************************************/

/**
 * Updates cur_dts of all streams based on given timestamp and AVStream.
 *
 * Stream ref_st unchanged, others set cur_dts in their native timebase
 * only needed for timestamp wrapping or if (dts not set and pts!=dts)
 * @param timestamp new dts expressed in time_base of param ref_st
 * @param ref_st reference stream giving time_base of param timestamp
 */
void av_update_cur_dts(AVFormatContext *s, AVStream *ref_st, int64_t timestamp){
    int i;

    for(i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];

        st->cur_dts = av_rescale(timestamp,
                                 st->time_base.den * (int64_t)ref_st->time_base.num,
                                 st->time_base.num * (int64_t)ref_st->time_base.den);
    }
}

/**
 * Add a index entry into a sorted list updateing if it is already there.
 *
 * @param timestamp timestamp in the timebase of the given stream
 */
int av_add_index_entry(AVStream *st,
                            int64_t pos, int64_t timestamp, int size, int distance, int flags)
{
    AVIndexEntry *entries, *ie;
    int index;

    if((unsigned)st->nb_index_entries + 1 >= UINT_MAX / sizeof(AVIndexEntry))
        return -1;

    entries = av_fast_realloc(st->index_entries,
                              &st->index_entries_allocated_size,
                              (st->nb_index_entries + 1) *
                              sizeof(AVIndexEntry));
    if(!entries)
        return -1;

    st->index_entries= entries;

    index= av_index_search_timestamp(st, timestamp, AVSEEK_FLAG_ANY);

    if(index<0){
        index= st->nb_index_entries++;
        ie= &entries[index];
        assert(index==0 || ie[-1].timestamp < timestamp);
    }else{
        ie= &entries[index];
        if(ie->timestamp != timestamp){
            if(ie->timestamp <= timestamp)
                return -1;
            memmove(entries + index + 1, entries + index, sizeof(AVIndexEntry)*(st->nb_index_entries - index));
            st->nb_index_entries++;
        }else if(ie->pos == pos && distance < ie->min_distance) //dont reduce the distance
            distance= ie->min_distance;
    }

    ie->pos = pos;
    ie->timestamp = timestamp;
    ie->min_distance= distance;
    ie->size= size;
    ie->flags = flags;

    return index;
}

/**
 * Returns TRUE if we deal with a raw stream.
 *
 * Raw codec data and parsing needed.
 */
static int is_raw_stream(AVFormatContext *s)
{
    AVStream *st;

    if (s->nb_streams != 1)
        return 0;
    st = s->streams[0];
    if (!st->need_parsing)
        return 0;
    return 1;
}

/**
 * Gets the index for a specific timestamp.
 * @param flags if AVSEEK_FLAG_BACKWARD then the returned index will correspond to
 *                 the timestamp which is <= the requested one, if backward is 0
 *                 then it will be >=
 *              if AVSEEK_FLAG_ANY seek to any frame, only keyframes otherwise
 * @return < 0 if no such timestamp could be found
 */
int av_index_search_timestamp(AVStream *st, int64_t wanted_timestamp,
                              int flags)
{
    AVIndexEntry *entries= st->index_entries;
    int nb_entries= st->nb_index_entries;
    int a, b, m;
    int64_t timestamp;

    a = - 1;
    b = nb_entries;

    while (b - a > 1) {
        m = (a + b) >> 1;
        timestamp = entries[m].timestamp;
        if(timestamp >= wanted_timestamp)
            b = m;
        if(timestamp <= wanted_timestamp)
            a = m;
    }
    m= (flags & AVSEEK_FLAG_BACKWARD) ? a : b;

    if(!(flags & AVSEEK_FLAG_ANY)){
        while(m>=0 && m<nb_entries && !(entries[m].flags & AVINDEX_KEYFRAME)){
            m += (flags & AVSEEK_FLAG_BACKWARD) ? -1 : 1;
        }
    }

    if(m == nb_entries)
        return -1;
    return  m;
}

/*******************************************************/

/**
 * Returns TRUE if the stream has accurate timings in any stream.
 *
 * @return TRUE if the stream has accurate timings for at least one component.
 */
static int av_has_timings(AVFormatContext *ic)
{
    int i;
    AVStream *st;

    for(i = 0;i < ic->nb_streams; i++) {
        st = ic->streams[i];
        if (st->start_time != AV_NOPTS_VALUE &&
            st->duration != AV_NOPTS_VALUE)
            return 1;
    }
    return 0;
}

/**
 * Estimate the stream timings from the one of each components.
 *
 * Also computes the global bitrate if possible.
 */
static void av_update_stream_timings(AVFormatContext *ic)
{
    int64_t start_time, start_time1, end_time, end_time1;
    int i;
    AVStream *st;

    start_time = INT64_MAX;
    end_time = INT64_MIN;
    for(i = 0;i < ic->nb_streams; i++) {
        st = ic->streams[i];
        if (st->start_time != AV_NOPTS_VALUE) {
            start_time1= av_rescale_q(st->start_time, st->time_base, AV_TIME_BASE_Q);
            if (start_time1 < start_time)
                start_time = start_time1;
            if (st->duration != AV_NOPTS_VALUE) {
                end_time1 = start_time1
                          + av_rescale_q(st->duration, st->time_base, AV_TIME_BASE_Q);
                if (end_time1 > end_time)
                    end_time = end_time1;
            }
        }
    }
    if (start_time != INT64_MAX) {
        ic->start_time = start_time;
        if (end_time != INT64_MIN) {
            ic->duration = end_time - start_time;
            if (ic->file_size > 0) {
                /* compute the bit rate */
                ic->bit_rate = (double)ic->file_size * 8.0 * AV_TIME_BASE /
                    (double)ic->duration;
            }
        }
    }

}

static void fill_all_stream_timings(AVFormatContext *ic)
{
    int i;
    AVStream *st;

    av_update_stream_timings(ic);
    for(i = 0;i < ic->nb_streams; i++) {
        st = ic->streams[i];
        if (st->start_time == AV_NOPTS_VALUE) {
            if(ic->start_time != AV_NOPTS_VALUE)
                st->start_time = av_rescale_q(ic->start_time, AV_TIME_BASE_Q, st->time_base);
            if(ic->duration != AV_NOPTS_VALUE)
                st->duration = av_rescale_q(ic->duration, AV_TIME_BASE_Q, st->time_base);
        }
    }
}

static int has_codec_parameters(AVCodecContext *enc)
{
    int val;
    switch(enc->codec_type) {
    case CODEC_TYPE_AUDIO:
        val = enc->sample_rate;
        break;
    case CODEC_TYPE_VIDEO:
        val = enc->width && enc->pix_fmt != PIX_FMT_NONE;
        break;
    default:
        val = 1;
        break;
    }
    return (val != 0);
}

/*******************************************************/

/**
 * Add a new stream to a media file.
 *
 * Can only be called in the read_header() function. If the flag
 * AVFMTCTX_NOHEADER is in the format context, then new streams
 * can be added in read_packet too.
 *
 * @param s media file handle
 * @param id file format dependent stream id
 */
AVStream *av_new_stream(AVFormatContext *s, int id)
{
    AVStream *st;
    int i;

    if (s->nb_streams >= MAX_STREAMS)
        return NULL;

    st = av_mallocz(sizeof(AVStream));
    if (!st)
        return NULL;

    st->codec= avcodec_alloc_context();
    if (s->iformat) {
        /* no default bitrate if decoding */
        st->codec->bit_rate = 0;
    }
    st->index = s->nb_streams;
    st->id = id;
    st->start_time = AV_NOPTS_VALUE;
    st->duration = AV_NOPTS_VALUE;
    st->cur_dts = AV_NOPTS_VALUE;

    /* default pts settings is MPEG like */
    av_set_pts_info(st, 33, 1, 90000);
    st->last_IP_pts = AV_NOPTS_VALUE;
    for(i=0; i<MAX_REORDER_DELAY+1; i++)
        st->pts_buffer[i]= AV_NOPTS_VALUE;

    s->streams[s->nb_streams++] = st;
    return st;
}

/************************************************************/
/* output media file */

int av_set_parameters(AVFormatContext *s, AVFormatParameters *ap)
{
    int ret;

    if (s->oformat->priv_data_size > 0) {
        s->priv_data = av_mallocz(s->oformat->priv_data_size);
        if (!s->priv_data)
            return AVERROR_NOMEM;
    } else
        s->priv_data = NULL;

    if (s->oformat->set_parameters) {
        ret = s->oformat->set_parameters(s, ap);
        if (ret < 0)
            return ret;
    }
    return 0;
}

/**
 * allocate the stream private data and write the stream header to an
 * output media file
 *
 * @param s media file handle
 * @return 0 if OK. AVERROR_xxx if error.
 */
int av_write_header(AVFormatContext *s)
{
    int ret, i;
    AVStream *st;

    // some sanity checks
    for(i=0;i<s->nb_streams;i++) {
        st = s->streams[i];

        switch (st->codec->codec_type) {
        case CODEC_TYPE_AUDIO:
            if(st->codec->sample_rate<=0){
                av_log(s, AV_LOG_ERROR, "sample rate not set\n");
                return -1;
            }
            break;
        case CODEC_TYPE_VIDEO:
            if(st->codec->time_base.num<=0 || st->codec->time_base.den<=0){ //FIXME audio too?
                av_log(s, AV_LOG_ERROR, "time base not set\n");
                return -1;
            }
            if(st->codec->width<=0 || st->codec->height<=0){
                av_log(s, AV_LOG_ERROR, "dimensions not set\n");
                return -1;
            }
            break;
        }
    }

    if(s->oformat->write_header){
        ret = s->oformat->write_header(s);
        if (ret < 0)
            return ret;
    }

    /* init PTS generation */
    for(i=0;i<s->nb_streams;i++) {
        int64_t den = AV_NOPTS_VALUE;
        st = s->streams[i];

        switch (st->codec->codec_type) {
        case CODEC_TYPE_AUDIO:
            den = (int64_t)st->time_base.num * st->codec->sample_rate;
            break;
        case CODEC_TYPE_VIDEO:
            den = (int64_t)st->time_base.num * st->codec->time_base.den;
            break;
        default:
            break;
        }
        if (den != AV_NOPTS_VALUE) {
            if (den <= 0)
                return AVERROR_INVALIDDATA;
            av_frac_init(&st->pts, 0, 0, den);
        }
    }
    return 0;
}

//FIXME merge with compute_pkt_fields
static int compute_pkt_fields2(AVStream *st, AVPacket *pkt){
    int delay = FFMAX(st->codec->has_b_frames, !!st->codec->max_b_frames);
    int num, den, frame_size, i;

//    av_log(st->codec, AV_LOG_DEBUG, "av_write_frame: pts:%"PRId64" dts:%"PRId64" cur_dts:%"PRId64" b:%d size:%d st:%d\n", pkt->pts, pkt->dts, st->cur_dts, delay, pkt->size, pkt->stream_index);

/*    if(pkt->pts == AV_NOPTS_VALUE && pkt->dts == AV_NOPTS_VALUE)
        return -1;*/

    /* duration field */
    if (pkt->duration == 0) {
        compute_frame_duration(&num, &den, st, NULL, pkt);
        if (den && num) {
            pkt->duration = av_rescale(1, num * (int64_t)st->time_base.den, den * (int64_t)st->time_base.num);
        }
    }

    //XXX/FIXME this is a temporary hack until all encoders output pts
    if((pkt->pts == 0 || pkt->pts == AV_NOPTS_VALUE) && pkt->dts == AV_NOPTS_VALUE && !delay){
        pkt->dts=
//        pkt->pts= st->cur_dts;
        pkt->pts= st->pts.val;
    }

    //calculate dts from pts
    if(pkt->pts != AV_NOPTS_VALUE && pkt->dts == AV_NOPTS_VALUE){
        st->pts_buffer[0]= pkt->pts;
        for(i=1; i<delay+1 && st->pts_buffer[i] == AV_NOPTS_VALUE; i++)
            st->pts_buffer[i]= (i-delay-1) * pkt->duration;
        for(i=0; i<delay && st->pts_buffer[i] > st->pts_buffer[i+1]; i++)
            FFSWAP(int64_t, st->pts_buffer[i], st->pts_buffer[i+1]);

        pkt->dts= st->pts_buffer[0];
    }

    if(st->cur_dts && st->cur_dts != AV_NOPTS_VALUE && st->cur_dts >= pkt->dts){
        av_log(NULL, AV_LOG_ERROR, "error, non monotone timestamps %"PRId64" >= %"PRId64"\n", st->cur_dts, pkt->dts);
        return -1;
    }
    if(pkt->dts != AV_NOPTS_VALUE && pkt->pts != AV_NOPTS_VALUE && pkt->pts < pkt->dts){
        av_log(NULL, AV_LOG_ERROR, "error, pts < dts\n");
        return -1;
    }

//    av_log(NULL, AV_LOG_DEBUG, "av_write_frame: pts2:%"PRId64" dts2:%"PRId64"\n", pkt->pts, pkt->dts);
    st->cur_dts= pkt->dts;
    st->pts.val= pkt->dts;

    /* update pts */
    switch (st->codec->codec_type) {
    case CODEC_TYPE_AUDIO:
        frame_size = get_audio_frame_size(st->codec, pkt->size);

        /* HACK/FIXME, we skip the initial 0-size packets as they are most likely equal to the encoder delay,
           but it would be better if we had the real timestamps from the encoder */
        if (frame_size >= 0 && (pkt->size || st->pts.num!=st->pts.den>>1 || st->pts.val)) {
            av_frac_add(&st->pts, (int64_t)st->time_base.den * frame_size);
        }
        break;
    case CODEC_TYPE_VIDEO:
        av_frac_add(&st->pts, (int64_t)st->time_base.den * st->codec->time_base.num);
        break;
    default:
        break;
    }
    return 0;
}

static void truncate_ts(AVStream *st, AVPacket *pkt){
    int64_t pts_mask = (2LL << (st->pts_wrap_bits-1)) - 1;

//    if(pkt->dts < 0)
//        pkt->dts= 0;  //this happens for low_delay=0 and b frames, FIXME, needs further invstigation about what we should do here

    if (pkt->pts != AV_NOPTS_VALUE)
    pkt->pts &= pts_mask;
    if (pkt->dts != AV_NOPTS_VALUE)
    pkt->dts &= pts_mask;
}

/**
 * Write a packet to an output media file.
 *
 * The packet shall contain one audio or video frame.
 *
 * @param s media file handle
 * @param pkt the packet, which contains the stream_index, buf/buf_size, dts/pts, ...
 * @return < 0 if error, = 0 if OK, 1 if end of stream wanted.
 */
int av_write_frame(AVFormatContext *s, AVPacket *pkt)
{
    int ret;

    ret=compute_pkt_fields2(s->streams[pkt->stream_index], pkt);
    if(ret<0 && !(s->oformat->flags & AVFMT_NOTIMESTAMPS))
        return ret;

    truncate_ts(s->streams[pkt->stream_index], pkt);

    ret= s->oformat->write_packet(s, pkt);
    if(!ret)
        ret= url_ferror(&s->pb);
    return ret;
}

/**
 * Interleave a packet per DTS in an output media file.
 *
 * Packets with pkt->destruct == av_destruct_packet will be freed inside this function,
 * so they cannot be used after it, note calling av_free_packet() on them is still safe.
 *
 * @param s media file handle
 * @param out the interleaved packet will be output here
 * @param in the input packet
 * @param flush 1 if no further packets are available as input and all
 *              remaining packets should be output
 * @return 1 if a packet was output, 0 if no packet could be output,
 *         < 0 if an error occured
 */
int av_interleave_packet_per_dts(AVFormatContext *s, AVPacket *out, AVPacket *pkt, int flush){
    AVPacketList *pktl, **next_point, *this_pktl;
    int stream_count=0;
    int streams[MAX_STREAMS];

    if(pkt){
        AVStream *st= s->streams[ pkt->stream_index];

//        assert(pkt->destruct != av_destruct_packet); //FIXME

        this_pktl = av_mallocz(sizeof(AVPacketList));
        this_pktl->pkt= *pkt;
        if(pkt->destruct == av_destruct_packet)
            pkt->destruct= NULL; // non shared -> must keep original from being freed
        else
            av_dup_packet(&this_pktl->pkt);  //shared -> must dup

        next_point = &s->packet_buffer;
        while(*next_point){
            AVStream *st2= s->streams[ (*next_point)->pkt.stream_index];
            int64_t left=  st2->time_base.num * (int64_t)st ->time_base.den;
            int64_t right= st ->time_base.num * (int64_t)st2->time_base.den;
            if((*next_point)->pkt.dts * left > pkt->dts * right) //FIXME this can overflow
                break;
            next_point= &(*next_point)->next;
        }
        this_pktl->next= *next_point;
        *next_point= this_pktl;
    }

    memset(streams, 0, sizeof(streams));
    pktl= s->packet_buffer;
    while(pktl){
//av_log(s, AV_LOG_DEBUG, "show st:%d dts:%"PRId64"\n", pktl->pkt.stream_index, pktl->pkt.dts);
        if(streams[ pktl->pkt.stream_index ] == 0)
            stream_count++;
        streams[ pktl->pkt.stream_index ]++;
        pktl= pktl->next;
    }

    if(s->nb_streams == stream_count || (flush && stream_count)){
        pktl= s->packet_buffer;
        *out= pktl->pkt;

        s->packet_buffer= pktl->next;
        av_freep(&pktl);
        return 1;
    }else{
        av_init_packet(out);
        return 0;
    }
}

/**
 * Interleaves a AVPacket correctly so it can be muxed.
 * @param out the interleaved packet will be output here
 * @param in the input packet
 * @param flush 1 if no further packets are available as input and all
 *              remaining packets should be output
 * @return 1 if a packet was output, 0 if no packet could be output,
 *         < 0 if an error occured
 */
static int av_interleave_packet(AVFormatContext *s, AVPacket *out, AVPacket *in, int flush){
    if(s->oformat->interleave_packet)
        return s->oformat->interleave_packet(s, out, in, flush);
    else
        return av_interleave_packet_per_dts(s, out, in, flush);
}

/**
 * Writes a packet to an output media file ensuring correct interleaving.
 *
 * The packet must contain one audio or video frame.
 * If the packets are already correctly interleaved the application should
 * call av_write_frame() instead as its slightly faster, its also important
 * to keep in mind that completly non interleaved input will need huge amounts
 * of memory to interleave with this, so its prefereable to interleave at the
 * demuxer level
 *
 * @param s media file handle
 * @param pkt the packet, which contains the stream_index, buf/buf_size, dts/pts, ...
 * @return < 0 if error, = 0 if OK, 1 if end of stream wanted.
 */
int av_interleaved_write_frame(AVFormatContext *s, AVPacket *pkt){
    AVStream *st= s->streams[ pkt->stream_index];

    //FIXME/XXX/HACK drop zero sized packets
    if(st->codec->codec_type == CODEC_TYPE_AUDIO && pkt->size==0)
        return 0;

//av_log(NULL, AV_LOG_DEBUG, "av_interleaved_write_frame %d %"PRId64" %"PRId64"\n", pkt->size, pkt->dts, pkt->pts);
    if(compute_pkt_fields2(st, pkt) < 0 && !(s->oformat->flags & AVFMT_NOTIMESTAMPS))
        return -1;

    if(pkt->dts == AV_NOPTS_VALUE)
        return -1;

    for(;;){
        AVPacket opkt;
        int ret= av_interleave_packet(s, &opkt, pkt, 0);
        if(ret<=0) //FIXME cleanup needed for ret<0 ?
            return ret;

        truncate_ts(s->streams[opkt.stream_index], &opkt);
        ret= s->oformat->write_packet(s, &opkt);

        av_free_packet(&opkt);
        pkt= NULL;

        if(ret<0)
            return ret;
        if(url_ferror(&s->pb))
            return url_ferror(&s->pb);
    }
}

/**
 * @brief Write the stream trailer to an output media file and
 *        free the file private data.
 *
 * @param s media file handle
 * @return 0 if OK. AVERROR_xxx if error.
 */
int av_write_trailer(AVFormatContext *s)
{
    int ret, i;

    for(;;){
        AVPacket pkt;
        ret= av_interleave_packet(s, &pkt, NULL, 1);
        if(ret<0) //FIXME cleanup needed for ret<0 ?
            goto fail;
        if(!ret)
            break;

        truncate_ts(s->streams[pkt.stream_index], &pkt);
        ret= s->oformat->write_packet(s, &pkt);

        av_free_packet(&pkt);

        if(ret<0)
            goto fail;
        if(url_ferror(&s->pb))
            goto fail;
    }

    if(s->oformat->write_trailer)
        ret = s->oformat->write_trailer(s);
fail:
    if(ret == 0)
       ret=url_ferror(&s->pb);
    for(i=0;i<s->nb_streams;i++)
        av_freep(&s->streams[i]->priv_data);
    av_freep(&s->priv_data);
    return ret;
}

/**
 * Set the pts for a given stream.
 *
 * @param s stream
 * @param pts_wrap_bits number of bits effectively used by the pts
 *        (used for wrap control, 33 is the value for MPEG)
 * @param pts_num numerator to convert to seconds (MPEG: 1)
 * @param pts_den denominator to convert to seconds (MPEG: 90000)
 */
void av_set_pts_info(AVStream *s, int pts_wrap_bits,
                     int pts_num, int pts_den)
{
    s->pts_wrap_bits = pts_wrap_bits;
    s->time_base.num = pts_num;
    s->time_base.den = pts_den;
}

/* fraction handling */

/**
 * f = val + (num / den) + 0.5.
 *
 * 'num' is normalized so that it is such as 0 <= num < den.
 *
 * @param f fractional number
 * @param val integer value
 * @param num must be >= 0
 * @param den must be >= 1
 */
static void av_frac_init(AVFrac *f, int64_t val, int64_t num, int64_t den)
{
    num += (den >> 1);
    if (num >= den) {
        val += num / den;
        num = num % den;
    }
    f->val = val;
    f->num = num;
    f->den = den;
}

/**
 * Set f to (val + 0.5).
 */
static void av_frac_set(AVFrac *f, int64_t val)
{
    f->val = val;
    f->num = f->den >> 1;
}

/**
 * Fractionnal addition to f: f = f + (incr / f->den).
 *
 * @param f fractional number
 * @param incr increment, can be positive or negative
 */
static void av_frac_add(AVFrac *f, int64_t incr)
{
    int64_t num, den;

    num = f->num + incr;
    den = f->den;
    if (num < 0) {
        f->val += num / den;
        num = num % den;
        if (num < 0) {
            num += den;
            f->val--;
        }
    } else if (num >= den) {
        f->val += num / den;
        num = num % den;
    }
    f->num = num;
}
