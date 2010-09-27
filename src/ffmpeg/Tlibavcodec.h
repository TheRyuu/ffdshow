#ifndef _TLIBAVCODEC_H_
#define _TLIBAVCODEC_H_

#include "../codecs/ffcodecs.h"
#include <dxva.h>
#include "TpostprocSettings.h"
#include "ffImgfmt.h"
#include "libavfilter/vf_yadif.h"
// Do not include avcodec.h in this file, ffmpeg and ffmpeg-mt may conflict.

struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct AVPacket;
struct AVCodecParserContext;
struct AVPaletteControl;
struct SwsContext;
struct SwsFilter;
struct SwsVector;
struct SwsParams;
struct PPMode;

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
 static int ppCpuCaps(int csp);
 static int swsCpuCaps(void);
 static void pp_mode_defaults(PPMode &ppMode);
 static int getPPmode(const TpostprocSettings *cfg,int currentq);
 static void swsInitParams(SwsParams *params,int resizeMethod);
 static void swsInitParams(SwsParams *params,int resizeMethod,int flags);

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
 int (*avcodec_decode_video2)(AVCodecContext *avctx, AVFrame *picture,
                             int *got_picture_ptr,
                             AVPacket *avpkt);
 int (*avcodec_decode_audio3)(AVCodecContext *avctx, int16_t *samples,
                         int *frame_size_ptr,
                         AVPacket *avpkt);
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
 int (*av_parser_parse2)(AVCodecParserContext *s,AVCodecContext *avctx,uint8_t **poutbuf, int *poutbuf_size,const uint8_t *buf, int buf_size,int64_t pts, int64_t dts, int64_t pos);
 void (*av_parser_close)(AVCodecParserContext *s);

 void (*av_init_packet)(AVPacket *pkt);

 int (*avcodec_h264_search_recovery_point)(AVCodecContext *avctx,
                         const uint8_t *buf, int buf_size, int *recovery_frame_cnt);

 static const char_t *idctNames[],*errorRecognitions[],*errorConcealments[];
 struct Tdia_size
  {
   int size;
   const char_t *descr;
  };
 static const Tdia_size dia_sizes[];

 //libswscale imports
 SwsContext* (*sws_getContext)(int srcW, int srcH, enum PixelFormat srcFormat,
                           int dstW, int dstH, enum PixelFormat dstFormat, int flags,
                           SwsParams *params, //FFDShow structure
                           SwsFilter *srcFilter, SwsFilter *dstFilter, const double *param);

 void (*sws_freeContext)(SwsContext *c);
 SwsFilter* (*sws_getDefaultFilter)(float lumaGBlur, float chromaGBlur,
                                float lumaSharpen, float chromaSharpen,
                                float chromaHShift, float chromaVShift,
                                int verbose);
 void (*sws_freeFilter)(SwsFilter *filter);
 int (*sws_scale)(struct SwsContext *context, const uint8_t* const srcSlice[], const stride_t srcStride[],
              int srcSliceY, int srcSliceH, uint8_t* const dst[], const stride_t dstStride[]);
 SwsVector *(*sws_getConstVec)(double c, int length);
 SwsVector *(*sws_getGaussianVec)(double variance, double quality);
 void (*sws_normalizeVec)(SwsVector *a, double height);
 void (*sws_freeVec)(SwsVector *a);
 void (*sws_convertPalette8ToPacked32)(const uint8_t *src, uint8_t *dst, long num_pixels, const uint8_t *palette);
 void (*sws_convertPalette8ToPacked24)(const uint8_t *src, uint8_t *dst, long num_pixels, const uint8_t *palette);
 void (*palette8torgb32)(const uint8_t *src, uint8_t *dst, long num_pixels, const uint8_t *palette);
 void (*palette8tobgr32)(const uint8_t *src, uint8_t *dst, long num_pixels, const uint8_t *palette);
 void (*palette8torgb24)(const uint8_t *src, uint8_t *dst, long num_pixels, const uint8_t *palette);
 void (*palette8tobgr24)(const uint8_t *src, uint8_t *dst, long num_pixels, const uint8_t *palette);
 void (*palette8torgb16)(const uint8_t *src, uint8_t *dst, long num_pixels, const uint8_t *palette);
 void (*palette8tobgr16)(const uint8_t *src, uint8_t *dst, long num_pixels, const uint8_t *palette);
 void (*palette8torgb15)(const uint8_t *src, uint8_t *dst, long num_pixels, const uint8_t *palette);
 void (*palette8tobgr15)(const uint8_t *src, uint8_t *dst, long num_pixels, const uint8_t *palette);
 int (*GetCPUCount)(void);

 //libpostproc imports
 void  (*pp_postprocess)(const uint8_t * src[3], const stride_t srcStride[3],
                     uint8_t * dst[3], const stride_t dstStride[3],
                     int horizontalSize, int verticalSize,
                     const /*QP_STORE_T*/int8_t *QP_store,  int QP_stride,
                     /*pp_mode*/void *mode, /*pp_context*/void *ppContext, int pict_type);
 /*pp_context*/void *(*pp_get_context)(int width, int height, int flags);
 void (*pp_free_context)(/*pp_context*/void *ppContext);


 // DXVA imports
 int (*av_h264_decode_frame)(struct AVCodecContext* avctx, uint8_t *buf, int buf_size);
 int (*av_vc1_decode_frame)(struct AVCodecContext* avctx, uint8_t *buf, int buf_size);

 // === H264 functions
 int     (*FFH264CheckCompatibility)(int nWidth, int nHeight, struct AVCodecContext* pAVCtx, BYTE* pBuffer, UINT nSize, int nPCIVendor, int nPCIDevice, LARGE_INTEGER VideoDriverVersion);
 void    (*FFH264DecodeBuffer) (struct AVCodecContext* pAVCtx, BYTE* pBuffer, UINT nSize, int* pFramePOC, int* pOutPOC, REFERENCE_TIME* pOutrtStart);
 HRESULT (*FFH264BuildPicParams) (DXVA_PicParams_H264* pDXVAPicParams, DXVA_Qmatrix_H264* pDXVAScalingMatrix, int* nFieldType, int* nSliceType, struct AVCodecContext* pAVCtx, int nPCIVendor);
 
 void    (*FFH264SetCurrentPicture) (int nIndex, DXVA_PicParams_H264* pDXVAPicParams, struct AVCodecContext* pAVCtx);
 void    (*FFH264UpdateRefFramesList) (DXVA_PicParams_H264* pDXVAPicParams, struct AVCodecContext* pAVCtx);
 BOOL    (*FFH264IsRefFrameInUse) (int nFrameNum, struct AVCodecContext* pAVCtx);
 void    (*FF264UpdateRefFrameSliceLong) (DXVA_PicParams_H264* pDXVAPicParams, DXVA_Slice_H264_Long* pSlice, struct AVCodecContext* pAVCtx);
 void    (*FFH264SetDxvaSliceLong) (struct AVCodecContext* pAVCtx, void* pSliceLong);

 // === VC1 functions
 HRESULT (*FFVC1UpdatePictureParam) (DXVA_PictureParameters* pPicParams, struct AVCodecContext* pAVCtx, int* nFieldType, int* nSliceType, BYTE* pBuffer, UINT nSize);
 int     (*FFIsSkipped) (struct AVCodecContext* pAVCtx);

 // === Common functions
 char*    (*GetFFMpegPictureType) (int nType);
 int      (*FFIsInterlaced) (struct AVCodecContext* pAVCtx, int nHeight);
 unsigned long (*FFGetMBNumber) (struct AVCodecContext* pAVCtx);

 void (*yadif_init)(YadifContext *yadctx);
 void (*yadif_uninit)(YadifContext *yadctx);
 void (*yadif_filter)(YadifContext *yadctx, uint8_t *dst[3], stride_t dst_stride[3], int width, int height, int parity, int tff);
};

#endif
