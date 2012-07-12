#pragma once

#include "Tfont.h"
#include "libswscale/swscale.h"
#include "Crect.h"
#include "Tconvert.h"

struct TspuPlane {
    /*private:
     size_t allocated;*/
public:
    size_t allocated;
    unsigned char *c, *r; // c[pos] : color[pos], r[pos] = alpha[pos]
    stride_t stride;
    TspuPlane(): c(NULL), r(NULL), allocated(0) {}
    ~TspuPlane() {
        if (c) {
            aligned_free(c);
        }
        if (r) {
            aligned_free(r);
        }
    }
    void alloc(const CSize &sz, int div, uint64_t csp);
    void setZero();
};

struct Tlibavcodec;
struct TspuImage : TrenderedSubtitleWordBase {
protected:
    uint64_t csp;
    TspuPlane plane[3];
    CRect rect[3];
    CRect originalRect[3];
    class Tscaler
    {
    protected:
        uint64_t csp;
    public:
        int srcdx, srcdy, dstdx, dstdy;
        static Tscaler *create(const TprintPrefs &prefs, int srcdx, int srcdy, int dstdx, int dstdy, uint64_t csp = FF_CSP_Y800);
        Tscaler(const TprintPrefs &prefs, int Isrcdx, int Isrcdy, int Idstdx, int Idstdy): srcdx(Isrcdx), srcdy(Isrcdy), dstdx(Idstdx), dstdy(Idstdy), csp(prefs.csp & FF_CSPS_MASK) {}
        virtual ~Tscaler() {}
        virtual void scale(const unsigned char *srci, const unsigned char *srca, stride_t srcStride, unsigned char *dsti, unsigned char *dsta, stride_t dstStride) = 0;
    };
    class TscalerPoint : public Tscaler
    {
    public:
        TscalerPoint(const TprintPrefs &prefs, int srcdx, int srcdy, int dstdx, int dstdy);
        virtual void scale(const unsigned char *srci, const unsigned char *srca, stride_t srcStride, unsigned char *dsti, unsigned char *dsta, stride_t dstStride);
    };
    class TscalerApprox : public Tscaler
    {
    private:
        unsigned int scalex, scaley;
    public:
        TscalerApprox(const TprintPrefs &prefs, int srcdx, int srcdy, int dstdx, int dstdy);
        virtual void scale(const unsigned char *srci, const unsigned char *srca, stride_t srcStride, unsigned char *dsti, unsigned char *dsta, stride_t dstStride);
    };
    class TscalerFull : public Tscaler
    {
    public:
        TscalerFull(const TprintPrefs &prefs, int srcdx, int srcdy, int dstdx, int dstdy);
        virtual void scale(const unsigned char *srci, const unsigned char *srca, stride_t srcStride, unsigned char *dsti, unsigned char *dsta, stride_t dstStride);
    };
    class TscalerBilin : public Tscaler
    {
    private:
        struct scale_pixel {
            unsigned position;
            unsigned left_up;
            unsigned right_down;
        } *table_x, *table_y;
        static void scale_table(unsigned int start_src, unsigned int start_tar, unsigned int end_src, unsigned int end_tar, scale_pixel *table);
        static int canon_alpha(int alpha) {
            return alpha ? 256 - alpha : 0;
        }
        static uint32_t canon_alpha32(uint32_t alpha) {
            return alpha ? 256 - alpha : 0;
        }
    public:
        TscalerBilin(const TprintPrefs &prefs, int srcdx, int srcdy, int dstdx, int dstdy);
        virtual ~TscalerBilin();
        virtual void scale(const unsigned char *srci, const unsigned char *srca, stride_t srcStride, unsigned char *dsti, unsigned char *dsta, stride_t dstStride);
    };
    class TscalerSw : public Tscaler
    {
    private:
        SwsFilter filter;
        SwsContext *ctx, *alphactx;
        //Tconvert *convert;
        Tlibavcodec *libavcodec;
        TscalerApprox approx;
    public:
        TscalerSw(const TprintPrefs &prefs, int srcdx, int srcdy, int dstdx, int dstdy, uint64_t csp);
        virtual ~TscalerSw();
        virtual void scale(const unsigned char *srci, const unsigned char *srca, stride_t srcStride, unsigned char *dsti, unsigned char *dsta, stride_t dstStride);
    };
public:
    TspuImage(const TspuPlane src[3], const CRect &rcclip, const CRect &rectReal, const CRect &rectOrig, const CRect &IfinalRect, const TprintPrefs &prefs, uint64_t Icsp = FF_CSP_Y800);
    virtual void ownprint(
        const TprintPrefs &prefs,
        unsigned char **dst,
        const stride_t *stride) = 0;
};

template<class _mm> struct TspuImageSimd : public TspuImage {
public:
    TspuImageSimd(const TspuPlane src[3], const CRect &rcclip, const CRect &rectReal, const CRect &rectOrig, const TprintPrefs &prefs, uint64_t csp): TspuImage(src, rcclip, rectReal, rectOrig, CRect(), prefs, csp) {}
    TspuImageSimd(const TspuPlane src[3], const CRect &rcclip, const CRect &rectReal, const CRect &rectOrig, const CRect &finalRect, const TprintPrefs &prefs, uint64_t csp): TspuImage(src, rcclip, rectReal, rectOrig, finalRect, prefs, csp) {}
    virtual void ownprint(
        const TprintPrefs &prefs,
        unsigned char **dst,
        const stride_t *stride);
    virtual void print(int startx, int starty, unsigned int dx[3], int dy[3], unsigned char *dstLn[3], const stride_t stride[3], const unsigned char *bmp[3], const unsigned char *msk[3], REFERENCE_TIME rtStart = REFTIME_INVALID) const;
};
