/*
 * Copyright (c) 2004 Roman Shaposhnik
 * Copyright (c) 2008 Alexander Strange (astrange@ithinksw.com)
 *
 * Many thanks to Steven M. Schultz for providing clever ideas and
 * to Michael Niedermayer <michaelni@gmx.at> for writing initial
 * implementation.
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

#include "avcodec.h"
#include "../../pthreads/pthread.h"
#include "thread.h"

/// Max number of frame buffers that can be allocated when using frame threads.
#define MAX_BUFFERS 32

typedef int (action_func)(AVCodecContext *c, void *arg);
typedef int (action_func2)(AVCodecContext *c, void *arg, int jobnr, int threadnr);

typedef struct ThreadContext {
    pthread_t *workers;
    action_func *func;
    action_func2 *func2;
    void *args;
    int *rets;
    int rets_count;
    int job_count;
    int job_size;

    pthread_cond_t last_job_cond;
    pthread_cond_t current_job_cond;
    pthread_mutex_t current_job_lock;
    int current_job;
    int done;
} ThreadContext;

typedef struct PerThreadContext {
    pthread_t thread;
    pthread_cond_t input_cond;      ///< Used to wait for a new frame from the main thread.
    pthread_cond_t progress_cond;   ///< Used by child threads to wait for decoding/encoding progress.
    pthread_cond_t output_cond;     ///< Used by the main thread to wait for frames to finish.

    pthread_mutex_t mutex;          ///< Mutex used to protect the contents of the PerThreadContext.
    pthread_mutex_t progress_mutex; ///< Mutex used to protect frame progress values and progress_cond.

    AVCodecContext *avctx;          ///< Context used to decode frames passed to this thread.

    uint8_t *buf;                   ///< Input frame (for decoding) or output (for encoding).
    int buf_size;
    int allocated_buf_size;

    AVFrame picture;                ///< Output frame (for decoding) or input (for encoding).
    int got_picture;                ///< The output of got_picture_ptr from the last avcodec_decode_video() call (for decoding).
    int result;                     ///< The result of the last codec decode/encode() call.

    struct FrameThreadContext *parent;

    enum {
        STATE_INPUT_READY,          ///< Set when the thread is sleeping.
        STATE_SETTING_UP,           ///< Set before the codec has called ff_thread_finish_setup().
        STATE_SETUP_FINISHED        /**<
                                     * Set after the codec has called ff_thread_finish_setup().
                                     * At this point it is safe to start the next thread.
                                     */
    } state;

    /**
     * Array of frames passed to ff_thread_release_buffer(),
     * to be released later.
     */
    AVFrame released_buffers[MAX_BUFFERS];
    int num_released_buffers;

    /**
     * Array of progress values for ff_thread_get_buffer().
     */
    int progress[MAX_BUFFERS][2];
    uint8_t used_progress[MAX_BUFFERS];
} PerThreadContext;

typedef struct FrameThreadContext {
    PerThreadContext *threads;     ///< The contexts for frame decoding threads.
    PerThreadContext *prev_thread; ///< The last thread submit_frame() was called on.

    int next_decoding;             ///< The next context to submit frames to.
    int next_finished;             ///< The next context to return output from.

    int delaying;                  /**
                                    * Set for the first N frames, where N is the number of threads.
                                    * While it is set, ff_en/decode_frame_threaded won't return any results.
                                    */

    pthread_mutex_t buffer_mutex;  ///< Mutex used to protect get/release_buffer().

    int die;                       ///< Set to cause threads to exit.
} FrameThreadContext;

static void* attribute_align_arg worker(void *v)
{
    AVCodecContext *avctx = v;
    ThreadContext *c = avctx->thread_opaque;
    int our_job = c->job_count;
    int thread_count = avctx->thread_count;
    int self_id;

    pthread_mutex_lock(&c->current_job_lock);
    self_id = c->current_job++;
    for (;;){
        while (our_job >= c->job_count) {
            if (c->current_job == thread_count + c->job_count)
                pthread_cond_signal(&c->last_job_cond);

            pthread_cond_wait(&c->current_job_cond, &c->current_job_lock);
            our_job = self_id;

            if (c->done) {
                pthread_mutex_unlock(&c->current_job_lock);
                return NULL;
            }
        }
        pthread_mutex_unlock(&c->current_job_lock);

        c->rets[our_job%c->rets_count] = c->func ? c->func(avctx, (char*)c->args + our_job*c->job_size):
                                                   c->func2(avctx, c->args, our_job, self_id);

        pthread_mutex_lock(&c->current_job_lock);
        our_job = c->current_job++;
    }
}

static av_always_inline void avcodec_thread_park_workers(ThreadContext *c, int thread_count)
{
    pthread_cond_wait(&c->last_job_cond, &c->current_job_lock);
    pthread_mutex_unlock(&c->current_job_lock);
}

static void thread_free(AVCodecContext *avctx)
{
    ThreadContext *c = avctx->thread_opaque;
    int i;

    pthread_mutex_lock(&c->current_job_lock);
    c->done = 1;
    pthread_cond_broadcast(&c->current_job_cond);
    pthread_mutex_unlock(&c->current_job_lock);

    for (i=0; i<avctx->thread_count; i++)
         pthread_join(c->workers[i], NULL);

    pthread_mutex_destroy(&c->current_job_lock);
    pthread_cond_destroy(&c->current_job_cond);
    pthread_cond_destroy(&c->last_job_cond);
    av_free(c->workers);
    av_freep(&avctx->thread_opaque);
}

static int avcodec_thread_execute(AVCodecContext *avctx, action_func* func, void *arg, int *ret, int job_count, int job_size)
{
    ThreadContext *c= avctx->thread_opaque;
    int dummy_ret;

    if (!(avctx->active_thread_type&FF_THREAD_SLICE) || avctx->thread_count <= 1)
        return avcodec_default_execute(avctx, func, arg, ret, job_count, job_size);

    if (job_count <= 0)
        return 0;

    pthread_mutex_lock(&c->current_job_lock);

    c->current_job = avctx->thread_count;
    c->job_count = job_count;
    c->job_size = job_size;
    c->args = arg;
    c->func = func;
    if (ret) {
        c->rets = ret;
        c->rets_count = job_count;
    } else {
        c->rets = &dummy_ret;
        c->rets_count = 1;
    }
    pthread_cond_broadcast(&c->current_job_cond);

    avcodec_thread_park_workers(c, avctx->thread_count);

    return 0;
}

static int avcodec_thread_execute2(AVCodecContext *avctx, action_func2* func2, void *arg, int *ret, int job_count)
{
    ThreadContext *c= avctx->thread_opaque;
    c->func2 = func2;
    return avcodec_thread_execute(avctx, NULL, arg, ret, job_count, 0);
}

static int thread_init(AVCodecContext *avctx)
{
    int i;
    ThreadContext *c;
    int thread_count = avctx->thread_count;

    avctx->thread_count = thread_count;

    if (thread_count <= 1)
        return 0;

    c = av_mallocz(sizeof(ThreadContext));
    if (!c)
        return -1;

    c->workers = av_mallocz(sizeof(pthread_t)*thread_count);
    if (!c->workers) {
        av_free(c);
        return -1;
    }

    avctx->thread_opaque = c;
    c->current_job = 0;
    c->job_count = 0;
    c->job_size = 0;
    c->done = 0;
    pthread_cond_init(&c->current_job_cond, NULL);
    pthread_cond_init(&c->last_job_cond, NULL);
    pthread_mutex_init(&c->current_job_lock, NULL);
    pthread_mutex_lock(&c->current_job_lock);
    for (i=0; i<thread_count; i++) {
        if(pthread_create(&c->workers[i], NULL, worker, avctx)) {
           avctx->thread_count = i;
           pthread_mutex_unlock(&c->current_job_lock);
           avcodec_thread_free(avctx);
           return -1;
        }
    }

    avcodec_thread_park_workers(c, thread_count);

    avctx->execute = avcodec_thread_execute;
    avctx->execute2 = avcodec_thread_execute2;
    return 0;
}

/**
 * Read and decode frames from the main thread until fctx->die is set.
 * ff_thread_finish_setup() is called before decoding if the codec
 * doesn't define update_thread_context(), and afterwards if the codec errors
 * before calling it.
 */
static attribute_align_arg void *frame_worker_thread(void *arg)
{
    PerThreadContext * p = arg;
    AVCodecContext *avctx = p->avctx;
    FrameThreadContext * fctx = p->parent;
    AVCodec *codec = avctx->codec;
    AVPacket avpkt;

    while (1) {
        if (p->state == STATE_INPUT_READY && !fctx->die) {
            pthread_mutex_lock(&p->mutex);
            while (p->state == STATE_INPUT_READY && !fctx->die)
                pthread_cond_wait(&p->input_cond, &p->mutex);
            pthread_mutex_unlock(&p->mutex);
        }

        if (fctx->die) break;

        if (!codec->update_thread_context) ff_thread_finish_setup(avctx);

        pthread_mutex_lock(&p->mutex);
        av_init_packet(&avpkt);
        avpkt.data = p->buf;
        avpkt.size = p->buf_size;
        p->result = codec->decode(avctx, &p->picture, &p->got_picture, &avpkt);

        if (p->state == STATE_SETTING_UP) ff_thread_finish_setup(avctx);

        p->buf_size = 0;
        p->state = STATE_INPUT_READY;

        pthread_mutex_lock(&p->progress_mutex);
        pthread_cond_signal(&p->output_cond);
        pthread_mutex_unlock(&p->progress_mutex);
        pthread_mutex_unlock(&p->mutex);
    };

    return NULL;
}

/**
 * Update a thread's context from the last thread. This is used for returning
 * frames and for starting new decoding jobs after the previous one finishes
 * predecoding.
 *
 * @param dst The destination context.
 * @param src The source context.
 * @param for_user Whether or not dst is the user-visible context. update_thread_context won't be called and some pointers will be copied.
 */
static int update_thread_context_from_copy(AVCodecContext *dst, AVCodecContext *src, int for_user)
{
    int err = 0;
#define COPY(f) dst->f = src->f;
#define COPY_FIELDS(s, e) memcpy(&dst->s, &src->s, (char*)&dst->e - (char*)&dst->s);

    //coded_width/height are not copied here, so that codecs' update_thread_context can see when they change
    //many encoding parameters could be theoretically changed during encode, but aren't copied ATM

    if (dst != src) {
        COPY(sub_id);
        COPY(width);
        COPY(height);
        COPY(pix_fmt);
        COPY(real_pict_num); //necessary?
        COPY(delay);
        COPY(max_b_frames);

        COPY_FIELDS(mv_bits, opaque);

        COPY(has_b_frames);
        COPY(bits_per_coded_sample);
        COPY(sample_aspect_ratio);
        COPY(idct_algo);
        memcpy(dst->error, src->error, sizeof(src->error));
        COPY(last_predictor_count); //necessary?
        COPY(dtg_active_format);
        COPY(color_table_id);
        COPY(profile);
        COPY(level);
        COPY(bits_per_raw_sample);
        COPY(ticks_per_frame);
        COPY(color_primaries);
        COPY(color_trc);
        COPY(colorspace);
        COPY(color_range);
    }

    if (for_user) {
        COPY(coded_frame);
        dst->has_b_frames += src->thread_count - 1;
    } else {
        if (dst->codec->update_thread_context)
            err = dst->codec->update_thread_context(dst, src);
    }

    return err;
}

///Update the next decoding thread with values set by the user
static void update_thread_context_from_user(AVCodecContext *dst, AVCodecContext *src)
{
    COPY(hurry_up);
    COPY_FIELDS(skip_loop_filter, bidir_refine);
    COPY(frame_number);
    /* ffdshow custom code (begin) */
    COPY(reordered_opaque);
    COPY(reordered_opaque2);
    COPY(reordered_opaque3);
    COPY(h264_has_to_drop_first_non_ref);
    /* ffdshow custom code (end) */
}

static void free_progress(AVFrame *f)
{
    PerThreadContext *p = f->owner->thread_opaque;
    int *progress = f->thread_opaque;

    p->used_progress[(progress - p->progress[0]) / 2] = 0;
}

/// Release all frames passed to ff_thread_release_buffer()
static void handle_delayed_releases(PerThreadContext *p)
{
    FrameThreadContext *fctx = p->parent;

    while (p->num_released_buffers > 0) {
        AVFrame *f = &p->released_buffers[--p->num_released_buffers];

        pthread_mutex_lock(&fctx->buffer_mutex);
        free_progress(f);
        f->thread_opaque = NULL;

        f->owner->release_buffer(f->owner, f);
        pthread_mutex_unlock(&fctx->buffer_mutex);
    }
}

/// Submit a frame to the next decoding thread
static int submit_frame(PerThreadContext * p, AVPacket *avpkt)
{
    FrameThreadContext *fctx = p->parent;
    PerThreadContext *prev_thread = fctx->prev_thread;
    AVCodec *codec = p->avctx->codec;
    int err = 0;

    if (!avpkt->size && !(codec->capabilities & CODEC_CAP_DELAY)) return 0;

    pthread_mutex_lock(&p->mutex);

    handle_delayed_releases(p);

    if (prev_thread) {
        if (prev_thread->state == STATE_SETTING_UP) {
            pthread_mutex_lock(&prev_thread->progress_mutex);
            while (prev_thread->state == STATE_SETTING_UP)
                pthread_cond_wait(&prev_thread->progress_cond, &prev_thread->progress_mutex);
            pthread_mutex_unlock(&prev_thread->progress_mutex);
        }

        err = update_thread_context_from_copy(p->avctx, prev_thread->avctx, 0);
        if (err) return err;
    }

    //FIXME: try to reuse the avpkt data instead of copying it
    p->buf = av_fast_realloc(p->buf, &p->allocated_buf_size, avpkt->size + FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(p->buf, avpkt->data, avpkt->size);
    memset(p->buf + avpkt->size, 0, FF_INPUT_BUFFER_PADDING_SIZE);
    p->buf_size = avpkt->size;

    p->state = STATE_SETTING_UP;
    pthread_cond_signal(&p->input_cond);
    pthread_mutex_unlock(&p->mutex);

    fctx->prev_thread = p;

    return err;
}

int ff_thread_decode_frame(AVCodecContext *avctx,
                             void *data, int *data_size,
                             AVPacket *avpkt)
{
    FrameThreadContext *fctx = avctx->thread_opaque;
    PerThreadContext * p;
    int thread_count = avctx->thread_count, err = 0;
    int returning_thread = fctx->next_finished;

    p = &fctx->threads[fctx->next_decoding];
    update_thread_context_from_user(p->avctx, avctx);
    err = submit_frame(p, avpkt);
    if (err) return err;

    // ffdshow custom code
    // if (avctx->got_first_frame) {}
    // DirectShow workaround for delivering the first frame without delay
    // delay delaying until the first frame is delivered.

    if (avctx->got_first_frame) {
        fctx->next_decoding++;

        if (fctx->delaying) {
            if (fctx->next_decoding >= (thread_count-1)) fctx->delaying = 0;

            *data_size=0;
            return 0;
        }
    }

    //If it's draining frames at EOF, ignore null frames from the codec.
    //Only return one when we've run out of codec frames to return.
    do {
#if 0
        p = &fctx->threads[returning_thread++];
#else
        // ffdshow custom code block
        p = &fctx->threads[returning_thread];
        if (avctx->got_first_frame)
            returning_thread++;
#endif
        if (p->state != STATE_INPUT_READY) {
            pthread_mutex_lock(&p->progress_mutex);
            while (p->state != STATE_INPUT_READY)
                pthread_cond_wait(&p->output_cond, &p->progress_mutex);
            pthread_mutex_unlock(&p->progress_mutex);
        }

        *(AVFrame*)data = p->picture;
        *data_size = p->got_picture;

        avcodec_get_frame_defaults(&p->picture);
        p->got_picture = 0;

        if (returning_thread >= thread_count) returning_thread = 0;
    } while (!avpkt->size && !*data_size && returning_thread != fctx->next_finished);

    update_thread_context_from_copy(avctx, p->avctx, 1);

    if (fctx->next_decoding >= thread_count) fctx->next_decoding = 0;
    fctx->next_finished = returning_thread;

    return p->result;
}

void ff_thread_report_progress(AVFrame *f, int n, int field)
{
    PerThreadContext *p;
    int *progress = f->thread_opaque;

    if (!progress || progress[field] >= n) return;

    p = f->owner->thread_opaque;

    if (f->owner->debug&FF_DEBUG_THREADS)
        av_log(f->owner, AV_LOG_DEBUG, "%p finished %d field %d\n", progress, n, field);

    pthread_mutex_lock(&p->progress_mutex);
    progress[field] = n;
    pthread_cond_broadcast(&p->progress_cond);
    pthread_mutex_unlock(&p->progress_mutex);
}

void ff_thread_await_progress(AVFrame *f, int n, int field)
{
    PerThreadContext *p;
    int *progress = f->thread_opaque;

    if (!progress || progress[field] >= n) return;

    p = f->owner->thread_opaque;

    if (f->owner->debug&FF_DEBUG_THREADS)
        av_log(f->owner, AV_LOG_DEBUG, "thread awaiting %d field %d from %p\n", n, field, progress);

    pthread_mutex_lock(&p->progress_mutex);
    while (progress[field] < n)
        pthread_cond_wait(&p->progress_cond, &p->progress_mutex);
    pthread_mutex_unlock(&p->progress_mutex);
}

void ff_thread_finish_frame(AVFrame *f)
{
    ff_thread_report_progress(f, INT_MAX, 0);
    ff_thread_report_progress(f, INT_MAX, 1);
}

void ff_thread_finish_setup(AVCodecContext *avctx) {
    PerThreadContext *p = avctx->thread_opaque;

    if (!(avctx->active_thread_type&FF_THREAD_FRAME)) return;

    pthread_mutex_lock(&p->progress_mutex);
    p->state = STATE_SETUP_FINISHED;
    pthread_cond_broadcast(&p->progress_cond);
    pthread_mutex_unlock(&p->progress_mutex);
}

/// Wait for all threads to finish decoding
static void park_frame_worker_threads(FrameThreadContext *fctx, int thread_count)
{
    int i;

    for (i = 0; i < thread_count; i++) {
        PerThreadContext *p = &fctx->threads[i];

        if (p->state != STATE_INPUT_READY) {
            pthread_mutex_lock(&p->progress_mutex);
            while (p->state != STATE_INPUT_READY)
                pthread_cond_wait(&p->output_cond, &p->progress_mutex);
            pthread_mutex_unlock(&p->progress_mutex);
        }
    }
}

static void frame_thread_free(AVCodecContext *avctx, int thread_count)
{
    FrameThreadContext *fctx = avctx->thread_opaque;
    AVCodec *codec = avctx->codec;
    int i;

    park_frame_worker_threads(fctx, thread_count);

    if (fctx->prev_thread && fctx->prev_thread != fctx->threads)
        update_thread_context_from_copy(fctx->threads->avctx, fctx->prev_thread->avctx, 0);

    fctx->die = 1;

    for (i = 0; i < thread_count; i++) {
        PerThreadContext *p = &fctx->threads[i];

        pthread_mutex_lock(&p->mutex);
        pthread_cond_signal(&p->input_cond);
        pthread_mutex_unlock(&p->mutex);

        if (p->thread.p) /* ffdshow custom code */
            pthread_join(p->thread, NULL);

        if (codec->close)
            codec->close(p->avctx);

        handle_delayed_releases(p);
    }

    for (i = 0; i < thread_count; i++) {
        PerThreadContext *p = &fctx->threads[i];

        avcodec_default_free_buffers(p->avctx);

        pthread_mutex_destroy(&p->mutex);
        pthread_mutex_destroy(&p->progress_mutex);
        pthread_cond_destroy(&p->input_cond);
        pthread_cond_destroy(&p->progress_cond);
        pthread_cond_destroy(&p->output_cond);
        av_freep(&p->buf);

        if (i)
            av_freep(&p->avctx->priv_data);

        av_freep(&p->avctx);
    }

    av_freep(&fctx->threads);
    pthread_mutex_destroy(&fctx->buffer_mutex);
    av_freep(&avctx->thread_opaque);
}

static int frame_thread_init(AVCodecContext *avctx)
{
    FrameThreadContext *fctx;
    AVCodecContext *src = avctx;
    AVCodec *codec = avctx->codec;
    int i, thread_count = avctx->thread_count, err = 0;

    avctx->thread_opaque = fctx = av_mallocz(sizeof(FrameThreadContext));
    fctx->delaying = 1;
    pthread_mutex_init(&fctx->buffer_mutex, NULL);

    fctx->threads = av_mallocz(sizeof(PerThreadContext) * thread_count);

    for (i = 0; i < thread_count; i++) {
        AVCodecContext *copy = av_malloc(sizeof(AVCodecContext));
        PerThreadContext *p  = &fctx->threads[i];

        pthread_mutex_init(&p->mutex, NULL);
        pthread_mutex_init(&p->progress_mutex, NULL);
        pthread_cond_init(&p->input_cond, NULL);
        pthread_cond_init(&p->progress_cond, NULL);
        pthread_cond_init(&p->output_cond, NULL);

        p->parent = fctx;
        p->avctx  = copy;

        *copy = *src;
        copy->thread_opaque = p;

        if (!i) {
            src = copy;

            if (codec->init)
                err = codec->init(copy);
        } else {
            copy->is_copy   = 1;
            copy->priv_data = av_malloc(codec->priv_data_size);
            memcpy(copy->priv_data, src->priv_data, codec->priv_data_size);

            if (codec->init_thread_copy)
                err = codec->init_thread_copy(copy);
        }

        if (err) goto error;

        pthread_create(&p->thread, NULL, frame_worker_thread, p);
    }

    update_thread_context_from_copy(avctx, src, 1);

    return 0;

error:
    // the failed thread isn't completed but must be freed
    frame_thread_free(avctx, i+1);

    return err;
}

void ff_thread_flush(AVCodecContext *avctx)
{
    FrameThreadContext *fctx = avctx->thread_opaque;

    if (!fctx || !fctx->prev_thread) return; // ffdshow custom code

    park_frame_worker_threads(fctx, avctx->thread_count);

    if (fctx->prev_thread && fctx->prev_thread != fctx->threads)
        update_thread_context_from_copy(fctx->threads->avctx, fctx->prev_thread->avctx, 0);

    fctx->next_decoding = fctx->next_finished = 0;
    fctx->delaying = 1;
    fctx->prev_thread = NULL;
}

static int *allocate_progress(PerThreadContext *p)
{
    int i;

    for (i = 0; i < MAX_BUFFERS; i++)
        if (!p->used_progress[i]) break;

    if (i == MAX_BUFFERS) {
        av_log(p->avctx, AV_LOG_ERROR, "allocate_progress() overflow\n");
        return NULL;
    }

    p->used_progress[i] = 1;

    return p->progress[i];
}

int ff_thread_get_buffer(AVCodecContext *avctx, AVFrame *f)
{
    int ret, *progress;
    PerThreadContext *p = avctx->thread_opaque;

    f->owner = avctx;

    if (!(avctx->active_thread_type&FF_THREAD_FRAME)) {
        f->thread_opaque = NULL;
        return avctx->get_buffer(avctx, f);
    }

    pthread_mutex_lock(&p->parent->buffer_mutex);
    f->thread_opaque = progress = allocate_progress(p);

    if (!progress) {
        pthread_mutex_unlock(&p->parent->buffer_mutex);
        return -1;
    }

    progress[0] =
    progress[1] = -1;

    ret = avctx->get_buffer(avctx, f);
    pthread_mutex_unlock(&p->parent->buffer_mutex);

    /*
     * The buffer list isn't shared between threads,
     * so age doesn't mean what codecs expect it to mean.
     * Disable it for now.
     */
    f->age = INT_MAX;

    return ret;
}

void ff_thread_release_buffer(AVCodecContext *avctx, AVFrame *f)
{
    PerThreadContext *p = avctx->thread_opaque;

    if (!(avctx->active_thread_type&FF_THREAD_FRAME)) {
        avctx->release_buffer(avctx, f);
        return;
    }

    if (p->num_released_buffers >= MAX_BUFFERS) {
        av_log(p->avctx, AV_LOG_ERROR, "too many delayed release_buffer calls!\n");
        return;
    }

    if(avctx->debug & FF_DEBUG_BUFFERS)
        av_log(avctx, AV_LOG_DEBUG, "delayed_release_buffer called on pic %p, %d buffers used\n",
                                    f, f->owner->internal_buffer_count);

    p->released_buffers[p->num_released_buffers++] = *f;
    memset(f->data, 0, sizeof(f->data));
}

/// Set the threading algorithm used, or none if an algorithm was set but no thread count.
static void validate_thread_parameters(AVCodecContext *avctx)
{
    int frame_threading_supported = (avctx->codec->capabilities & CODEC_CAP_FRAME_THREADS)
                                && !(avctx->flags & CODEC_FLAG_TRUNCATED)
                                && !(avctx->flags & CODEC_FLAG_LOW_DELAY)
                                && !(avctx->flags2 & CODEC_FLAG2_CHUNKS);
    if (avctx->thread_count <= 1)
        avctx->active_thread_type = 0;
    else if (frame_threading_supported && (avctx->thread_type & FF_THREAD_FRAME))
        avctx->active_thread_type = FF_THREAD_FRAME;
    else
        avctx->active_thread_type = FF_THREAD_SLICE;
}

int avcodec_thread_init(AVCodecContext *avctx, int thread_count)
{
    if (avctx->thread_opaque) {
        av_log(avctx, AV_LOG_ERROR, "avcodec_thread_init called after avcodec_open, this does nothing in ffmpeg-mt\n");
        return -1;
    }

    avctx->thread_count = thread_count;

    if (avctx->codec) {
        validate_thread_parameters(avctx);

        if (avctx->active_thread_type&FF_THREAD_SLICE)
            return thread_init(avctx);
        else if (avctx->active_thread_type&FF_THREAD_FRAME)
            return frame_thread_init(avctx);
    }

    return 0;
}

void avcodec_thread_free(AVCodecContext *avctx)
{
    if (avctx->active_thread_type&FF_THREAD_FRAME)
        frame_thread_free(avctx, avctx->thread_count);
    else
        thread_free(avctx);
}

// ffdshow custom code
AVCodecContext* get_thread0_avctx(AVCodecContext *avctx)
{
    FrameThreadContext *fctx;
    PerThreadContext *p;

    if (avctx->active_thread_type&FF_THREAD_FRAME && avctx->thread_opaque){
        fctx = avctx->thread_opaque;
        p = &fctx->threads[0];
        return p->avctx;
    } else {
        return avctx;
    }
}
