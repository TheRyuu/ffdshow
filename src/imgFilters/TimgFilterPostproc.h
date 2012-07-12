#ifndef _TIMGFILTERPOSTPROC_H_
#define _TIMGFILTERPOSTPROC_H_

#include "TimgFilter.h"
#include "libpostproc/postprocess_internal.h"
#include "nic/nic_postProcess.h"
#include "simd.h"
#include "TPerformanceCounter.h"

#ifndef MAX_THREADS
#define MAX_THREADS 32
#endif

#define USE_LIBAVCODEC_SIMPLE_IDCT 1

struct TpostprocSettings;
DECLARE_FILTER(TimgFilterPostprocBase, public, TimgFilter)
private:
static const int max_cpu_usage_history = 4;
int cpu_usage_history[max_cpu_usage_history];
int cpu_usage_history_pos;
bool h264, testh264, playingH264;
int oldh264mode;
unsigned int oldDeblockStrength;
bool wasIP;
bool pp_codec(int);
protected:
TPerformanceCounter timer;
int currentq;
int8_t *quants;
static const unsigned int quantsDx, quantsDy;
int prepare(const TpostprocSettings *cfg, int maxquant, int frametype);
public:
TimgFilterPostprocBase(IffdshowBase *Ideci, Tfilters *Iparent, bool Ih264);
virtual ~TimgFilterPostprocBase();
virtual void onSeek(void);
};

struct Tlibavcodec;
DECLARE_FILTER(TimgFilterPostprocAvcodec, public, TimgFilterPostprocBase)
private:
Tlibavcodec *libavcodec;
void *pp_ctx;
PPMode pp_mode;
protected:
virtual bool is(const TffPictBase &pict, const TfilterSettingsVideo *cfg);
virtual uint64_t getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const
{
    return FF_CSPS_MASK_YUV_PLANAR;
}
virtual void onSizeChange(void);
public:
TimgFilterPostprocAvcodec(IffdshowBase *Ideci, Tfilters *Iparent);
virtual ~TimgFilterPostprocAvcodec();
virtual void done(void);
virtual HRESULT process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0);
};

struct Tlibavcodec;
DECLARE_FILTER(TimgFilterPostprocSpp, public, TimgFilterPostprocBase)
private:
static const int BITS_FRW_ACC = 3; //; 2 or 3 for accuracy
                                static const int SHIFT_FRW_COL = BITS_FRW_ACC;
                                        static const int SHIFT_FRW_ROW = (BITS_FRW_ACC + 17 - 3);
                                                static const int RND_FRW_ROW = (1 << (SHIFT_FRW_ROW - 1));
                                                        static __align16(const int32_t, fdct_r_row[4]);

                                                        Tlibavcodec *libavcodec;
                                                        unsigned int old_dx, old_dy;
                                                        size_t CPUCount;
                                                        int old_sppMode;
                                                        typedef short DCTELEM;
                                                        void (*store_slice)(uint8_t *dst, const int16_t *src, int dst_stride, int src_stride, unsigned int width, unsigned int height, int log2_scale);
                                                        void (*requantize)(DCTELEM dst[64], const DCTELEM src[64], int qp);
                                                        int temp_stride;
                                                        int16_t *temp[MAX_THREADS];
                                                        uint8_t *src[MAX_THREADS];
                                                        static const uint8_t offset[127][2];
                                                        static void store_slice_c(uint8_t *dst, const int16_t *src, int dst_stride, int src_stride, unsigned int width, unsigned int height, int log2_scale);
                                                        friend class TimgFilterPostprocFspp;
template<class _mm> struct TstoreSlice {
    static __align16(const uint8_t, dither[8][_mm::size]);
    static void store_slice(uint8_t *dst, const int16_t *src, int dst_stride, int src_stride, unsigned int width, unsigned int height, int log2_scale);
};
static void softthresh_mmx(DCTELEM dst[64], const DCTELEM src[64], int qp);
static __forceinline void requant_core_soft(unsigned char *dst0, unsigned char *dst1, unsigned char *dst2, unsigned char *dst3, const unsigned char *src0, const unsigned char *src1, const unsigned char *src2, const unsigned char *src3, const __m64 &mm4, const __m64 &mm5);
static void hardthresh_mmx(DCTELEM dst[64], const DCTELEM src[64], int qp);
static void hardthresh_sse2(DCTELEM dst[64], const DCTELEM src[64], int qp);
static __forceinline void requant_core_hard_mmx(unsigned char *dst0, unsigned char *dst1, unsigned char *dst2, unsigned char *dst3, const unsigned char *src0, const unsigned char *src1, const unsigned char *src2, const unsigned char *src3, const __m64 &mm4, const __m64 &mm5, const __m64 &mm6);
static __forceinline void requant_core_hard_sse2(unsigned char *dst0, unsigned char *dst2, const unsigned char *src0, const unsigned char *src2, const __m128i &xmm4, const __m128i &xmm5, const __m128i &xmm6);
void filter(uint8_t *dst, const uint8_t *src, int dst_stride, int src_stride, unsigned int width, unsigned int height, const int8_t *qp_store, int qp_stride, bool is_luma, size_t thread_number, size_t thread_count);
template <class _mm> __forceinline void filter_simd(uint8_t *dst, const uint8_t *src, int dst_stride, int src_stride, unsigned int width, unsigned int height, const int8_t *qp_store, int qp_stride, bool is_luma, size_t thread_number, size_t thread_count);
template <class _mm> static __forceinline void add_block(int16_t *dst, int stride, DCTELEM block[64]);
template <class _mm> static __forceinline void get_pixels(DCTELEM *block, const uint8_t *pixels, int line_size);
template <class _mm> static __forceinline void fdct_row(const int16_t *in, int16_t *out, const int16_t *table);
template <class _mm> static void ff_fdct_simd(int16_t *block);

static __forceinline void store(int pos, const int16_t *src, int x, int y, int dst_stride, int src_stride, int log2_scale, const uint8_t *d, uint8_t *dst)
{
    int temp = ((src[x + y * src_stride + pos] << log2_scale) + d[pos]) >> 6;
    if (temp & 0x100) {
        temp = ~(temp >> 31);
    }
    dst[x + y * dst_stride + pos] = (uint8_t)temp;
}
#if !USE_LIBAVCODEC_SIMPLE_IDCT
static void simple_idct(int16_t *block);
#endif
protected:
virtual bool is(const TffPictBase &pict, const TfilterSettingsVideo *cfg);
virtual uint64_t getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const
{
    return FF_CSPS_MASK_YUV_PLANAR;
}
virtual void onSizeChange(void);
public:
TimgFilterPostprocSpp(IffdshowBase *Ideci, Tfilters *Iparent);
virtual void done(void);
virtual HRESULT process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0);
~TimgFilterPostprocSpp();
};

DECLARE_FILTER(TimgFilterPostprocNic, public, TimgFilterPostprocBase)
private:
Tnic_deringFc nic_dering;
protected:
virtual bool is(const TffPictBase &pict, const TfilterSettingsVideo *cfg);
virtual uint64_t getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const
{
    return FF_CSP_420P;
}
public:
TimgFilterPostprocNic(IffdshowBase *Ideci, Tfilters *Iparent);
virtual HRESULT process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0);
};

DECLARE_FILTER(TimgFilterPostprocFspp, public, TimgFilterPostprocBase)
private:
void filter(uint8_t *dst, const uint8_t *src,
            stride_t dst_stride, stride_t src_stride,
            int width, int height,
            int8_t *qp_store, int qp_stride, int is_luma);
uint8_t *src;
int temp_stride;
int16_t *temp;
int prev_q;
static const int qp = 0;
                      static const int log2_count = 4;
                              static const int BLOCKSZ = 12;
                                      typedef short DCTELEM;
                                      uint64_t threshold_mtx_noq[8 * 2];
                                      uint64_t threshold_mtx[8 * 2];
                                      void mul_thrmat_s(int q);
                                      static void row_fdct_s(DCTELEM *data, const uint8_t *pixels, int line_size, int cnt);
                                      static void column_fidct_s(int16_t* thr_adr, DCTELEM *data, DCTELEM *output, int cnt);
                                      static void row_idct_s(DCTELEM* workspace, int16_t* output_adr, int output_stride, int cnt);
                                      static void store_slice_s(uint8_t *dst, int16_t *src, stride_t dst_stride, stride_t src_stride, long width, long height, long log2_scale);
                                      static void store_slice2_s(uint8_t *dst, int16_t *src, stride_t dst_stride, stride_t src_stride, long width, long height, long log2_scale);
                                      static const int DCTSIZE_S = 8;
                                              static const __int64 MM_FIX_0_382683433, MM_FIX_0_541196100, MM_FIX_0_707106781, MM_FIX_1_306562965, MM_FIX_1_414213562_A, MM_FIX_1_847759065, MM_FIX_2_613125930, MM_FIX_1_414213562, MM_FIX_1_082392200, MM_FIX_0_847759065, MM_FIX_0_566454497, MM_FIX_0_198912367, MM_DESCALE_RND, MM_2;
                                              protected:
                                              virtual bool is(const TffPictBase &pict, const TfilterSettingsVideo *cfg);
                                              virtual uint64_t getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const
{
    return FF_CSPS_MASK_YUV_PLANAR;
}
virtual void onSizeChange(void);
public:
TimgFilterPostprocFspp(IffdshowBase *Ideci, Tfilters *Iparent);
virtual void done(void);
virtual HRESULT process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0);
};

#endif
