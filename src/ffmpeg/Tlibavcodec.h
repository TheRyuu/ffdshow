#ifndef _TLIBAVCODEC_H_
#define _TLIBAVCODEC_H_

#include "../codecs/ffcodecs.h"
#include <dxva.h>
#include "TpostprocSettings.h"
#include "ffImgfmt.h"
#include "libavfilter/vf_yadif.h"
#include "libavfilter/gradfun.h"
#include "libswscale/swscale.h"

struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct AVPacket;
struct AVCodecParserContext;
struct SwsContext;
struct SwsParams;
struct PPMode;
struct AVDictionary;

struct Tconfig;
class Tdll;
struct DSPContext;
struct TlibavcodecExt;
struct Tlibavcodec {
private:
    int (*libswscale_sws_scale)(struct SwsContext *context, const uint8_t* const srcSlice[], const int srcStride[],
                                int srcSliceY, int srcSliceH, uint8_t* const dst[], const int dstStride[]);
    Tdll *dll;
    int refcount;
    static int get_buffer(AVCodecContext *c, AVFrame *pic);
    CCritSec csOpenClose;
public:
    Tlibavcodec(const Tconfig *config);
    ~Tlibavcodec();
    static void avlog(AVCodecContext*, int, const char*, va_list);
    static void avlogMsgBox(AVCodecContext*, int, const char*, va_list);
    void AddRef(void) {
        refcount++;
    }
    void Release(void) {
        if (--refcount < 0) {
            delete this;
        }
    }
    static bool getVersion(const Tconfig *config, ffstring &vers, ffstring &license);
    static bool check(const Tconfig *config);
    static int ppCpuCaps(uint64_t csp);
    static void pp_mode_defaults(PPMode &ppMode);
    static int getPPmode(const TpostprocSettings *cfg, int currentq);
    static void swsInitParams(SwsParams *params, int resizeMethod);
    static void swsInitParams(SwsParams *params, int resizeMethod, int flags);

    bool ok;
    AVCodecContext* avcodec_alloc_context(AVCodec *codec, TlibavcodecExt *ext = NULL);

    void (*avcodec_register_all)(void);
    AVCodecContext* (*avcodec_alloc_context0)(AVCodec *codec);
    AVCodec* (*avcodec_find_decoder)(AVCodecID codecId);
    AVCodec* (*avcodec_find_encoder)(AVCodecID id);
    int (*avcodec_open0)(AVCodecContext *avctx, AVCodec *codec, AVDictionary **options);
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
    int (*avcodec_close0)(AVCodecContext *avctx);
    int  avcodec_close(AVCodecContext *avctx);

    void (*av_log_set_callback)(void (*)(AVCodecContext*, int, const char*, va_list));
    void* (*av_log_get_callback)(void);
    int (*av_log_get_level)(void);
    void (*av_log_set_level)(int);

    void (*av_set_cpu_flags_mask)(int mask);

    int (*avcodec_default_get_buffer)(AVCodecContext *s, AVFrame *pic);
    void (*avcodec_default_release_buffer)(AVCodecContext *s, AVFrame *pic);
    int (*avcodec_default_reget_buffer)(AVCodecContext *s, AVFrame *pic);
    const char* (*avcodec_get_current_idct)(AVCodecContext *avctx);
    void (*avcodec_get_encoder_info)(AVCodecContext *avctx, int *xvid_build, int *divx_version, int *divx_build, int *lavc_build);

    void* (*av_mallocz)(size_t size);
    void (*av_free)(void *ptr);

    AVCodecParserContext* (*av_parser_init)(int codec_id);
    int (*av_parser_parse2)(AVCodecParserContext *s, AVCodecContext *avctx, uint8_t **poutbuf, int *poutbuf_size, const uint8_t *buf, int buf_size, int64_t pts, int64_t dts, int64_t pos);
    void (*av_parser_close)(AVCodecParserContext *s);

    void (*av_init_packet)(AVPacket *pkt);
    uint8_t* (*av_packet_new_side_data)(AVPacket *pkt, enum AVPacketSideDataType type, int size);

    int (*avcodec_h264_search_recovery_point)(AVCodecContext *avctx,
            const uint8_t *buf, int buf_size, int *recovery_frame_cnt);

    static const char_t *idctNames[], *errorRecognitions[], *errorConcealments[];
    struct Tdia_size {
        int size;
        const char_t *descr;
    };
    static const Tdia_size dia_sizes[];

    //libswscale imports
    SwsContext* (*sws_getCachedContext)(struct SwsContext *context, int srcW, int srcH, enum PixelFormat srcFormat,
                                        int dstW, int dstH, enum PixelFormat dstFormat, int flags,
                                        SwsFilter *srcFilter, SwsFilter *dstFilter, const double *param, SwsParams *ffdshow_params);

    void (*sws_freeContext)(SwsContext *c);
    SwsFilter* (*sws_getDefaultFilter)(float lumaGBlur, float chromaGBlur,
                                       float lumaSharpen, float chromaSharpen,
                                       float chromaHShift, float chromaVShift,
                                       int verbose);
    void (*sws_freeFilter)(SwsFilter *filter);
    int sws_scale(struct SwsContext *context, const uint8_t* const srcSlice[], const stride_t srcStride[],
                  int srcSliceY, int srcSliceH, uint8_t* const dst[], const stride_t dstStride[]);
    SwsVector *(*sws_getConstVec)(double c, int length);
    SwsVector *(*sws_getGaussianVec)(double variance, double quality);
    void (*sws_normalizeVec)(SwsVector *a, double height);
    void (*sws_freeVec)(SwsVector *a);
    int (*sws_setColorspaceDetails)(struct SwsContext *c, const int inv_table[4],
                                    int srcRange, const int table[4], int dstRange,
                                    int brightness, int contrast, int saturation);
    const int* (*sws_getCoefficients)(int colorspace);

    int (*GetCPUCount)(void);

    //libpostproc imports
    void (*pp_postprocess)(const uint8_t * src[3], const stride_t srcStride[3],
                           uint8_t * dst[3], const stride_t dstStride[3],
                           int horizontalSize, int verticalSize,
                           const /*QP_STORE_T*/int8_t *QP_store,  int QP_stride,
                           /*pp_mode*/void *mode, /*pp_context*/void *ppContext, int pict_type);
    /*pp_context*/
    void *(*pp_get_context)(int width, int height, int flags);
    void (*pp_free_context)(/*pp_context*/void *ppContext);
    void (*ff_simple_idct_mmx)(int16_t *block);

    // DXVA imports
    int (*av_h264_decode_frame)(struct AVCodecContext* avctx, uint8_t *buf, int buf_size);
    int (*av_vc1_decode_frame)(struct AVCodecContext* avctx, uint8_t *buf, int buf_size);

    // === H264 functions
    int (*FFH264CheckCompatibility)(int nWidth, int nHeight, struct AVCodecContext* pAVCtx, BYTE* pBuffer, UINT nSize, int nPCIVendor, int nPCIDevice, LARGE_INTEGER VideoDriverVersion);
    int (*FFH264DecodeBuffer)(struct AVCodecContext* pAVCtx, BYTE* pBuffer, UINT nSize, int* pFramePOC, int* pOutPOC, REFERENCE_TIME* pOutrtStart);
    HRESULT(*FFH264BuildPicParams)(DXVA_PicParams_H264* pDXVAPicParams, DXVA_Qmatrix_H264* pDXVAScalingMatrix, int* nFieldType, int* nSliceType, struct AVCodecContext* pAVCtx, int nPCIVendor);

    void (*FFH264SetCurrentPicture)(int nIndex, DXVA_PicParams_H264* pDXVAPicParams, struct AVCodecContext* pAVCtx);
    void (*FFH264UpdateRefFramesList)(DXVA_PicParams_H264* pDXVAPicParams, struct AVCodecContext* pAVCtx);
    BOOL (*FFH264IsRefFrameInUse)(int nFrameNum, struct AVCodecContext* pAVCtx);
    void (*FF264UpdateRefFrameSliceLong)(DXVA_PicParams_H264* pDXVAPicParams, DXVA_Slice_H264_Long* pSlice, struct AVCodecContext* pAVCtx);
    void (*FFH264SetDxvaSliceLong)(struct AVCodecContext* pAVCtx, void* pSliceLong);

    // === VC1 functions
    HRESULT(*FFVC1UpdatePictureParam)(DXVA_PictureParameters* pPicParams, struct AVCodecContext* pAVCtx, int* nFieldType, int* nSliceType, BYTE* pBuffer, UINT nSize, UINT* nFrameSize, BOOL b_SecondField, BOOL* b_repeat_pict);
    int (*FFIsSkipped)(struct AVCodecContext* pAVCtx);

    // === Common functions
    char*    (*GetFFMpegPictureType)(int nType);
    unsigned long(*FFGetMBNumber)(struct AVCodecContext* pAVCtx);

    // yadif
    void (*yadif_init)(YADIFContext *yadctx);
    void (*yadif_uninit)(YADIFContext *yadctx);
    void (*yadif_filter)(YADIFContext *yadctx, uint8_t *dst[3], stride_t dst_stride[3], int width, int height, int parity, int tff);

    // gradfun
    int (*gradfunInit)(GradFunContext *ctx, const char *args, void *opaque);
    void (*gradfunFilter)(GradFunContext *ctx, uint8_t *dst, uint8_t *src, int width, int height, int dst_linesize, int src_linesize, int r);
};

#endif
