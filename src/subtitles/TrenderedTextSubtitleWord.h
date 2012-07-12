#pragma once

#include "Tfont.h"
#include "Rasterizer.h"
#include "TsubtitleMixedProps.h"

extern "C" {
    void __cdecl YV12_lum2chr_min_mmx(const unsigned char* lum0, const unsigned char* lum1, unsigned char* chr);
    void __cdecl YV12_lum2chr_max_mmx(const unsigned char* lum0, const unsigned char* lum1, unsigned char* chr);
    void __cdecl YV12_lum2chr_min_mmx2(const unsigned char* lum0, const unsigned char* lum1, unsigned char* chr);
    void __cdecl YV12_lum2chr_max_mmx2(const unsigned char* lum0, const unsigned char* lum1, unsigned char* chr);
    unsigned int __cdecl fontPrepareOutline_sse2(const unsigned char *src, size_t srcStrideGap, const short *matrix, size_t matrixSizeH, size_t matrixSizeV);
    unsigned int __cdecl fontPrepareOutline_mmx(const unsigned char *src, size_t srcStrideGap, const short *matrix, size_t matrixSizeH, size_t matrixSizeV, size_t matrixGap);
}

class CPolygon;

class TrenderedTextSubtitleWord : public Rasterizer
{
private:
    TrenderedTextSubtitleWord *secondaryColoredWord;

    int m_outlineWidth;
    TfontSettings::TshadowMode m_shadowMode;
    double outlineWidth_double;
    mutable unsigned int oldBodyYUVa, oldOutlineYUVa;
    unsigned int gdi_dx, gdi_dy;
    bool m_bitmapReady;

    void calcOutlineTextMetric(
        HDC hdc,
        SIZE italic_fixed_sz,
        const LOGFONT &lf);

    void Transform(CPoint org);

    void getPath(
        HDC hdc,
        const strings &tab_parsed_string,
        const ints &cxs);

    void drawGlyphOSD(
        HDC hdc,
        const strings &tab_parsed_string,
        const ints &cxs,
        double xscale);

    void rasterize(const CPointDouble &bodysLeftTop);
    void createOpaquBox(const CPointDouble &bodysLeftTop);
    void applyGaussianBlur(unsigned char *src);
    void apply_beBlur(unsigned char* &src, int blurCount);
    void postRasterisation();
    void ffCreateWidenedRegion();

    void prepareForColorSpace() const;
    void createShadow() const;
    unsigned char* blur(unsigned char *src, stride_t Idx, stride_t Idy, int startx, int starty, int endx, int endy, int blurStrength);
    void init();

    // color and constants table for SIMD
    template<class _mm> class Tcctbl
    {
    public:
        typename _mm::__m mask256, mask00ff, maskffff, mask8080, mask64;
        typename _mm::__m body_Y, body_U, body_V, body_A;
        typename _mm::__m outline_Y, outline_U, outline_V, outline_A;
        typename _mm::__m shadow_Y, shadow_U, shadow_V, shadow_A;
        void _mm_set1_16(__m128i &dst, short w) {
            dst = _mm_set1_epi16(w);
        }
        void _mm_set1_16(__m64 &dst, short w) {
            dst = _mm_set1_pi16(w);
        }
        void _mm_set4_16(__m128i &dst, short w3, short w2, short w1, short w0) {
            dst = _mm_set_epi16(w3, w2, w1, w0, w3, w2, w1, w0);
        }
        void _mm_set4_16(__m64 &dst, short w3, short w2, short w1, short w0) {
            dst = _mm_set_pi16(w3, w2, w1, w0);
        }
        Tcctbl(uint64_t csp, const TSubtitleMixedProps &mprops, int bodyA, int outlineA, int shadowA, int outlineWidth) {
            _mm_set1_16(mask256, 0x100);
            _mm_set1_16(mask00ff, 0xff);
            _mm_set1_16(maskffff, -1);
            _mm_set1_16(mask8080, -32640/*0x8080*/);
            _mm_set1_16(mask64, 0x40);
            _mm_set1_16(body_A, bodyA);
            _mm_set1_16(outline_A, outlineA);
            _mm_set1_16(shadow_A, shadowA);
            if (csp == FF_CSP_420P) {
                _mm_set1_16(body_Y, mprops.bodyYUV.Y);
                _mm_set1_16(body_U, mprops.bodyYUV.U);
                _mm_set1_16(body_V, mprops.bodyYUV.V);
                _mm_set1_16(outline_Y, mprops.outlineYUV.Y);
                _mm_set1_16(outline_U, mprops.outlineYUV.U);
                _mm_set1_16(outline_V, mprops.outlineYUV.V);
                _mm_set1_16(shadow_Y, mprops.shadowYUV.Y);
                _mm_set1_16(shadow_U, mprops.shadowYUV.U);
                _mm_set1_16(shadow_V, mprops.shadowYUV.V);
            } else {
                _mm_set4_16(body_Y, 0, mprops.bodyYUV.r, mprops.bodyYUV.g, mprops.bodyYUV.b);
                _mm_set4_16(outline_Y, 0, mprops.outlineYUV.r, mprops.outlineYUV.g, mprops.outlineYUV.b);
                _mm_set4_16(shadow_Y, 0, mprops.shadowYUV.r, mprops.shadowYUV.g, mprops.shadowYUV.b);
            }
        }
        void set_alpha(int body, int outline, int shadow) {
            _mm_set1_16(body_A, body);
            _mm_set1_16(outline_A, outline);
            _mm_set1_16(shadow_A, shadow);
        }
    };

    template<class _mm> bool set_scrollFader(Tcctbl<_mm> &cctbl, double fader, int bodyA, int outlineA, int shadowA) const {
        bodyA = int(bodyA * fader + 0.5);
        outlineA = int(outlineA * fader + 0.5);
        shadowA = int(shadowA * fader + 0.5);
        cctbl.set_alpha(bodyA, outlineA, shadowA);
        return (bodyA == 256 && m_outlineWidth > 0);
    }

    inline void RGBfontRenderer(int x, int y,
                                int bodyA, int outlineA, int shadowA,
                                unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
                                unsigned char *dst, stride_t dstStride) const;
    inline void RGBfontRendererFillBody(int x, int y,
                                        int bodyA, int outlineA, int shadowA,
                                        unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
                                        unsigned char *dst, stride_t dstStride) const;
    inline void YV12_YfontRenderer(int x, int y,
                                   int bodyA, int outlineA, int shadowA,
                                   unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
                                   unsigned char *dst, stride_t dstStride) const;
    inline void YV12_UVfontRenderer(int x, int y,
                                    int bodyA, int outlineA, int shadowA,
                                    unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
                                    unsigned char *dstU, unsigned char *dstV, stride_t dstStride) const;

    template<class _mm, bool fillBody, bool fillOutline> __forceinline void fontRenderer_simd(
        const Tcctbl<_mm> &cctbl,
        unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
        unsigned char *dst) const;
    template<class _mm> __forceinline void fontRenderer_simd_funcs(bool fBody, bool fOutline,
            const Tcctbl<_mm> &cctbl,
            unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
            unsigned char *dst) const;
    template<class _mm> __forceinline void fontRendererUV_simd(
        const Tcctbl<_mm> &cctbl,
        unsigned char *bmp, unsigned char *outline, unsigned char *shadow,
        unsigned char *dstU, unsigned char* dstV) const;
    void paintC_RGB(int startx, int endx, int starty, int dy1, int dstStartx,
                    int bodyA, int outlineA, int shadowA,
                    unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
                    unsigned char *dst, stride_t dstStride) const;
    void paintC_YV12(int startx, int startxUV, int endx, int endxUV,
                     int starty, int startyUV, int dy1, int dy1UV, int dstStartx,
                     int bodyA, int outlineA, int shadowA,
                     unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
                     unsigned char *bmpUV, unsigned char *outlineUV, unsigned char *shadowUV, unsigned char *mskUV,
                     unsigned char *dst, unsigned char *dstU, unsigned char *dstV,
                     stride_t dstStride, stride_t dstStrideUV) const;

    class TexpandedGlyph
    {
        unsigned char *expBmp;
    public:
        int dx;
        int dy;
        TexpandedGlyph(const TrenderedTextSubtitleWord &word) {
            int owx = ceil(word.m_outlineWidth * word.mprops.scaleX);
            dx = word.dx[0] + 2 * owx;
            dy = word.dy[0] + 2 * word.m_outlineWidth;
            expBmp = aligned_calloc3<uint8_t>(dx, dy, 16);
            for (unsigned int y = 0 ; y < word.dy[0] ; y++) {
                memcpy(expBmp + dx * (y + word.m_outlineWidth) + owx, word.bmp[0] + word.dx[0] * y, word.dx[0]);
            }
        }

        ~TexpandedGlyph() {
            _aligned_free(expBmp);
        }

        operator unsigned char*() const {
            return expBmp;
        }
    };

    void Y2RGB(unsigned char *const bitmap[3], unsigned int size) const;

protected:
    CPolygon* m_pOpaqueBox;

    int m_shadowSize;
    double m_baseline;
    double m_ascent, m_descent;
    double m_sar;
    CRect overhang;

    CRect computeOverhang();
    unsigned int getShadowSize(LONG fontHeight);

public:
    TSubtitleMixedProps mprops;
    // full rendering
    TrenderedTextSubtitleWord(
        HDC hdc,
        const wchar_t *s,
        size_t strlens,
        const TprintPrefs &prefs,
        LOGFONT lf,
        TSubtitleProps Iprops);

    // As a base class of CPolygon
    TrenderedTextSubtitleWord(const TSubtitleMixedProps &Improps);

    // secondary (for karaoke)
    struct secondaryColor_t {};
    TrenderedTextSubtitleWord(
        const TrenderedTextSubtitleWord &parent,
        struct secondaryColor_t);
    virtual ~TrenderedTextSubtitleWord();
    virtual void print(int startx, int starty, unsigned int dx[3], int dy[3], unsigned char *dstLn[3], const stride_t stride[3], const unsigned char *bmp[3], const unsigned char *msk[3], REFERENCE_TIME rtStart = REFTIME_INVALID) const {} // unused
    template<class _mm> void paint(int startx, int starty, int dx[3], int dy[3], unsigned char *dstLn[3], const stride_t stride[3], ptrdiff_t srcOffset[3], REFERENCE_TIME rtStart = REFTIME_INVALID) const;
    unsigned int alignXsize;
    void (__cdecl *YV12_lum2chr_min)(const unsigned char* lum0, const unsigned char* lum1, unsigned char* chr);
    void (__cdecl *YV12_lum2chr_max)(const unsigned char* lum0, const unsigned char* lum1, unsigned char* chr);
    virtual double get_ascent() const;
    virtual double get_descent() const;
    virtual double get_baseline() const;

    // for collisions
    double aboveBaseLinePlusOutline() const;
    double belowBaseLinePlusOutline() const;

    virtual size_t getMemorySize() const;
    void TrenderedTextSubtitleWord::printText(
        double startx,
        double starty,
        double lineBaseline,
        REFERENCE_TIME rtStart,
        unsigned int prefsdx,
        unsigned int prefsdy,
        unsigned char **dst,
        const stride_t *stride);
    friend class TexpandedGlyph;
};
