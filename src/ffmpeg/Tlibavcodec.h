#ifndef _TLIBAVCODEC_H_
#define _TLIBAVCODEC_H_

#include "../codecs/ffcodecs.h"
// Do not include avcodec.h in this file, ffmpeg and ffmpeg-mt may conflict.

struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct AVCodecParserContext;
struct AVPaletteControl;

struct Tconfig;
class Tdll;
struct DSPContext;
struct TlibavcodecExt;
struct Tlibavcodec
{
private:
 Tdll *dll;
 int refcount;
 static int get_buffer(AVCodecContext *c, AVFrame *pic);
 CCritSec csOpenClose;
public:
 Tlibavcodec(const Tconfig *config);
 ~Tlibavcodec();
 static void avlog(AVCodecContext*,int,const char*,va_list);
 static void avlogMsgBox(AVCodecContext*,int,const char*,va_list);
 void AddRef(void)
  {
   refcount++;
  }
 void Release(void)
  {
   if (--refcount<0)
    delete this;
  }
 static bool getVersion(const Tconfig *config,ffstring &vers,ffstring &license);
 static bool check(const Tconfig *config);
 static int lavcCpuFlags(void);

 bool ok,dec_only;
 AVCodecContext* avcodec_alloc_context(TlibavcodecExt *ext=NULL);

 void (*avcodec_init)(void);
 void (*avcodec_register_all)(void);
 AVCodecContext* (*avcodec_alloc_context0)(void);
 void (*dsputil_init)(DSPContext* p, AVCodecContext *avctx);
 AVCodec* (*avcodec_find_decoder)(CodecID codecId);
 AVCodec* (*avcodec_find_encoder)(CodecID id);
 int  (*avcodec_open0)(AVCodecContext *avctx, AVCodec *codec);
 int  avcodec_open(AVCodecContext *avctx, AVCodec *codec);
 AVFrame* (*avcodec_alloc_frame)(void);
 int (*avcodec_decode_video)(AVCodecContext *avctx, AVFrame *picture,
                             int *got_picture_ptr,
                             const uint8_t *buf, int buf_size);
int (*avcodec_decode_audio2)(AVCodecContext *avctx, int16_t *samples,
                         int *frame_size_ptr,
                         const uint8_t *buf, int buf_size);

 int (*avcodec_encode_video)(AVCodecContext *avctx, uint8_t *buf, int buf_size, const AVFrame *pict);
 int (*avcodec_encode_audio)(AVCodecContext *avctx, uint8_t *buf, int buf_size, const short *samples);
 void (*avcodec_flush_buffers)(AVCodecContext *avctx);
 int  (*avcodec_close0)(AVCodecContext *avctx);
 int  avcodec_close(AVCodecContext *avctx);
 //void (*av_free_static)(void);

 void (*av_log_set_callback)(void (*)(AVCodecContext*, int, const char*, va_list));
 void* (*av_log_get_callback)(void);
 int (*av_log_get_level)(void);
 void (*av_log_set_level)(int);

 int (*avcodec_thread_init)(AVCodecContext *s, int thread_count);
 void (*avcodec_thread_free)(AVCodecContext *s);

 int (*avcodec_default_get_buffer)(AVCodecContext *s, AVFrame *pic);
 void (*avcodec_default_release_buffer)(AVCodecContext *s, AVFrame *pic);
 int (*avcodec_default_reget_buffer)(AVCodecContext *s, AVFrame *pic);
 const char* (*avcodec_get_current_idct)(AVCodecContext *avctx);
 void (*avcodec_get_encoder_info)(AVCodecContext *avctx,int *xvid_build,int *divx_version,int *divx_build,int *lavc_build);

 void (*av_free)(void *ptr);
 
 AVCodecParserContext* (*av_parser_init)(int codec_id);
 int (*av_parser_parse)(AVCodecParserContext *s,AVCodecContext *avctx,uint8_t **poutbuf, int *poutbuf_size,const uint8_t *buf, int buf_size,int64_t pts, int64_t dts);
 void (*av_parser_close)(AVCodecParserContext *s);

 int (*avcodec_h264_search_recovery_point)(AVCodecContext *avctx,
                         const uint8_t *buf, int buf_size, int *recovery_frame_cnt);
 int (*avcodec_h264_decode_init_is_avc)(AVCodecContext *avctx);

 static const char_t *idctNames[],*errorRecognitions[],*errorConcealments[];
 struct Tdia_size
  {
   int size;
   const char_t *descr;
  };
 static const Tdia_size dia_sizes[];
};

#endif
