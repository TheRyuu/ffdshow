#ifndef _TIMGFILTERPOSTPROC_H_
#define _TIMGFILTERPOSTPROC_H_

#include "TimgFilter.h"
#include "postproc/postprocess.h"
#include "nic/nic_postProcess.h"
#include "simd.h"

struct TpostprocSettings;
DECLARE_FILTER(TimgFilterPostprocBase,public,TimgFilter)
private:
 bool h264,testh264,playingH264;
 int oldh264mode;
 unsigned int oldDeblockStrength;
 bool wasIP;
 bool pp_codec(int);
protected:
 int currentq;
 int8_t *quants;
 static const unsigned int quantsDx,quantsDy;
 int prepare(const TpostprocSettings *cfg,int maxquant,int frametype);
public:
 TimgFilterPostprocBase(IffdshowBase *Ideci,Tfilters *Iparent,bool Ih264);
 virtual ~TimgFilterPostprocBase();
 virtual void onSeek(void);
};

struct Tlibmplayer;
DECLARE_FILTER(TimgFilterPostprocMplayer,public,TimgFilterPostprocBase)
private:
 Tlibmplayer *libmplayer;
 void *pp_ctx;PPMode pp_mode;
protected:
 virtual bool is(const TffPictBase &pict,const TfilterSettingsVideo *cfg);
 virtual int getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const {return FF_CSPS_MASK_YUV_PLANAR;}
 virtual void onSizeChange(void);
public:
 TimgFilterPostprocMplayer(IffdshowBase *Ideci,Tfilters *Iparent);
 virtual ~TimgFilterPostprocMplayer();
 virtual void done(void);
 virtual HRESULT process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0);
};

struct Tlibavcodec;
DECLARE_FILTER(TimgFilterPostprocSpp,public,TimgFilterPostprocBase)
private:
 int old_sppMode;
 typedef short DCTELEM;
 void (*store_slice)(uint8_t *dst, const int16_t *src, int dst_stride, int src_stride, unsigned int width, unsigned int height, int log2_scale);
 void (*requantize)(DCTELEM dst[64], const DCTELEM src[64], int qp);
 int temp_stride;
 int16_t *temp;
 uint8_t *src;
 static const uint8_t offset[127][2];
 static void store_slice_c(uint8_t *dst, const int16_t *src, int dst_stride, int src_stride, unsigned int width, unsigned int height, int log2_scale);
 friend class TimgFilterPostprocFspp;
 template<class _mm> struct TstoreSlice
  {
   static __align16(const uint8_t,dither[8][_mm::size]);
   static void store_slice(uint8_t *dst, const int16_t *src, int dst_stride, int src_stride, unsigned int width, unsigned int height, int log2_scale);
  }; 
 static void softthresh_mmx(DCTELEM dst[64], const DCTELEM src[64], int qp);
 static __forceinline void requant_core_soft(unsigned char *dst0,unsigned char *dst1,unsigned char *dst2,unsigned char *dst3,const unsigned char *src0,const unsigned char *src1,const unsigned char *src2,const unsigned char *src3,const __m64 &mm4,const __m64 &mm5);
 static void hardthresh_mmx(DCTELEM dst[64], const DCTELEM src[64], int qp);
 static __forceinline void requant_core_hard(unsigned char *dst0,unsigned char *dst1,unsigned char *dst2,unsigned char *dst3,const unsigned char *src0,const unsigned char *src1,const unsigned char *src2,const unsigned char *src3,const __m64 &mm4,const __m64 &mm5,const __m64 &mm6);
 void filter(uint8_t *dst, const uint8_t *src, int dst_stride, int src_stride, unsigned int width, unsigned int height, const int8_t *qp_store, int qp_stride, bool is_luma);
 static inline void add_block(int16_t *dst,int stride,DCTELEM block[64]);
 static inline void get_pixels(DCTELEM *block, const uint8_t *pixels, int line_size);
 static __forceinline void store(int pos,const int16_t *src,int x,int y,int dst_stride,int src_stride,int log2_scale,const uint8_t *d,uint8_t *dst)
  {
   int temp= ((src[x + y*src_stride + pos]<<log2_scale) + d[pos])>>6;
   if (temp & 0x100) temp= ~(temp>>31);
   dst[x + y*dst_stride + pos]= (uint8_t)temp;
  } 
 static void ff_fdct_mmx2(int16_t *block);
protected: 
 virtual bool is(const TffPictBase &pict,const TfilterSettingsVideo *cfg);
 virtual int getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const {return FF_CSPS_MASK_YUV_PLANAR;}
 virtual void onSizeChange(void);
public:
 TimgFilterPostprocSpp(IffdshowBase *Ideci,Tfilters *Iparent);
 virtual void done(void);
 virtual HRESULT process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0);
};

DECLARE_FILTER(TimgFilterPostprocNic,public,TimgFilterPostprocBase)
private:
 Tnic_deringFc nic_dering;
protected:
 virtual bool is(const TffPictBase &pict,const TfilterSettingsVideo *cfg);
 virtual int getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const {return FF_CSP_420P;}
public:
 TimgFilterPostprocNic(IffdshowBase *Ideci,Tfilters *Iparent);
 virtual HRESULT process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0);
};

#if H264PP
DECLARE_FILTER(TimgFilterPostprocH264,public,TimgFilterPostprocBase)
private:
 static const int alphas[52],betas[52],cs[52];
 typedef void (Tfilter)(uint8_t *pix, stride_t stride, int alpha, int beta, const int *tc0);
 Tfilter *h264_h_loop_filter_luma,*h264_v_loop_filter_luma;
 Tfilter *h264_h_loop_filter_chroma,*h264_v_loop_filter_chroma;
 static __forceinline void h264_loop_filter_luma_c(uint8_t *pix, stride_t xstride, stride_t ystride, int alpha, int beta, const int *tc0);
 static void h264_h_loop_filter_luma_c(uint8_t *pix, stride_t stride, int alpha, int beta, const int *tc0);
 static void h264_v_loop_filter_luma_c(uint8_t *pix, stride_t stride, int alpha, int beta, const int *tc0);
 static __forceinline void h264_loop_filter_chroma_c(uint8_t *pix, stride_t xstride, stride_t ystride, int alpha, int beta, const int *tc0);
 static void h264_v_loop_filter_chroma_c(uint8_t *pix, stride_t stride, int alpha, int beta, const int *tc0);
 static void h264_h_loop_filter_chroma_c(uint8_t *pix, stride_t stride, int alpha, int beta, const int *tc0);
 static __forceinline void h264_loop_filter_chroma_intra_c(uint8_t *pix, stride_t xstride, stride_t ystride, int alpha, int beta);
 static void h264_v_loop_filter_chroma_intra_c(uint8_t *pix, stride_t stride, int alpha, int beta);
 static void h264_h_loop_filter_chroma_intra_c(uint8_t *pix, stride_t stride, int alpha, int beta);

 static __forceinline void transpose4x4(uint8_t *dst, uint8_t *src, stride_t dst_stride, stride_t src_stride);
 static __forceinline void H264_DEBLOCK_P0_Q0(const __m64 &pb_01, const __m64 &pb_3f,__m64 &mm0,__m64 &mm1,__m64 &mm2,__m64 &mm3,__m64 &mm4,__m64 &mm5,__m64 &mm6,__m64 &mm7);
 static __forceinline void DIFF_GT_MMX(__m64 &x,__m64 &y,__m64 &a,__m64 &o,__m64 &t);
 static __forceinline void H264_DEBLOCK_Q1(__m64 &p1, __m64 &q2, uint8_t *q2addr, uint8_t *q1addr, __m64 &tc0, __m64 &tmp,__m64 &mm1,__m64 &mm2,const __m64 &mm_bone);
 template<class Tsimd> struct TloopFilter
  {
  private:
   static __forceinline void H264_DEBLOCK_MASK(int alpha1,int beta1,__m64 &mm0,__m64 &mm1,__m64 &mm2,__m64 &mm3,__m64 &mm4,__m64 &mm5,__m64 &mm6,__m64 &mm7);
   static __forceinline void h264_loop_filter_luma_mmx2(uint8_t *pix, stride_t stride, int alpha1, int beta1, const int *tc0);
   static __forceinline void h264_loop_filter_chroma_mmx2(uint8_t *pix, stride_t stride, int alpha1, int beta1, const int *tc0);
  public: 
   static void h264_v_loop_filter_luma(uint8_t *pix, stride_t stride, int alpha, int beta, const int *tc0);
   static void h264_h_loop_filter_luma(uint8_t *pix, stride_t stride, int alpha, int beta, const int *tc0);
   static void h264_v_loop_filter_chroma(uint8_t *pix, stride_t stride, int alpha, int beta, const int *tc0);
   static void h264_h_loop_filter_chroma(uint8_t *pix, stride_t stride, int alpha, int beta, const int *tc0);
  };
protected:
 virtual int getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const {return FF_CSP_420P;}
public:
 TimgFilterPostprocH264(IffdshowBase *Ideci,Tfilters *Iparent);
 virtual HRESULT process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0);
};
#endif

DECLARE_FILTER(TimgFilterPostprocFspp,public,TimgFilterPostprocBase)
private:
 void filter(uint8_t *dst, const uint8_t *src,
            stride_t dst_stride, stride_t src_stride,
            int width, int height,
            int8_t *qp_store, int qp_stride, int is_luma);
 uint8_t *src;
 int temp_stride;int16_t *temp;
 int prev_q;static const int qp=0;
 static const int log2_count=4;
 static const int BLOCKSZ=12;
 typedef short DCTELEM;
 uint64_t threshold_mtx_noq[8*2];
 uint64_t threshold_mtx[8*2];
 void mul_thrmat_s(int q);
 static void row_fdct_s(DCTELEM *data, const uint8_t *pixels, int line_size, int cnt);
 static void column_fidct_s(int16_t* thr_adr, DCTELEM *data, DCTELEM *output, int cnt);
 static void row_idct_s(DCTELEM* workspace, int16_t* output_adr, int output_stride, int cnt);
 static void store_slice_s(uint8_t *dst, int16_t *src, stride_t dst_stride, stride_t src_stride, long width, long height, long log2_scale);
 static void store_slice2_s(uint8_t *dst, int16_t *src, stride_t dst_stride, stride_t src_stride, long width, long height, long log2_scale);
 static const int DCTSIZE_S=8;
 static const __int64 MM_FIX_0_382683433,MM_FIX_0_541196100,MM_FIX_0_707106781,MM_FIX_1_306562965,MM_FIX_1_414213562_A,MM_FIX_1_847759065,MM_FIX_2_613125930,MM_FIX_1_414213562,MM_FIX_1_082392200,MM_FIX_0_847759065,MM_FIX_0_566454497,MM_FIX_0_198912367,MM_DESCALE_RND,MM_2;
protected: 
 virtual bool is(const TffPictBase &pict,const TfilterSettingsVideo *cfg);
 virtual int getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const {return FF_CSPS_MASK_YUV_PLANAR;}
 virtual void onSizeChange(void);
public:
 TimgFilterPostprocFspp(IffdshowBase *Ideci,Tfilters *Iparent);
 virtual void done(void);
 virtual HRESULT process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0);
};

#endif
