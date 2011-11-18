#pragma once

#include "Tfont.h"
#include "Rasterizer.h"
#include "TsubtitleMixedProps.h"

extern "C" {
    void __cdecl fontRendererFillBody_mmx(const unsigned char* bmp,const unsigned char* outline,const unsigned char* shadow,const unsigned short* colortbl,const unsigned char* dst,const unsigned char* msk);
    void __cdecl fontRendererFillBody_sse2(const unsigned char* bmp,const unsigned char* outline,const unsigned char* shadow,const unsigned short* colortbl,const unsigned char* dst,const unsigned char* msk);
    void __cdecl fontRenderer_mmx(const unsigned char* bmp,const unsigned char* outline,const unsigned char* shadow,const unsigned short* colortbl,const unsigned char* dst,const unsigned char* msk);
    void __cdecl fontRendererUV_mmx(const unsigned char* bmp,const unsigned char* outline,const unsigned char* shadow,const unsigned short* colortbl,const unsigned char* dstU,const unsigned char* dstV);
    void __cdecl fontRenderer_sse2(const unsigned char* bmp,const unsigned char* outline,const unsigned char* shadow,const unsigned short* colortbl,const unsigned char* dst,const unsigned char* msk);
    void __cdecl fontRendererUV_sse2(const unsigned char* bmp,const unsigned char* outline,const unsigned char* shadow,const unsigned short* colortbl,const unsigned char* dstU,const unsigned char* dstV);
    void __cdecl YV12_lum2chr_min_mmx(const unsigned char* lum0,const unsigned char* lum1,unsigned char* chr);
    void __cdecl YV12_lum2chr_max_mmx(const unsigned char* lum0,const unsigned char* lum1,unsigned char* chr);
    void __cdecl YV12_lum2chr_min_mmx2(const unsigned char* lum0,const unsigned char* lum1,unsigned char* chr);
    void __cdecl YV12_lum2chr_max_mmx2(const unsigned char* lum0,const unsigned char* lum1,unsigned char* chr);
    void __cdecl storeXmmRegs(unsigned char* buf);
    void __cdecl restoreXmmRegs(unsigned char* buf);
    unsigned int __cdecl fontPrepareOutline_sse2(const unsigned char *src,size_t srcStrideGap,const short *matrix,size_t matrixSizeH,size_t matrixSizeV);
    unsigned int __cdecl fontPrepareOutline_mmx (const unsigned char *src,size_t srcStrideGap,const short *matrix,size_t matrixSizeH,size_t matrixSizeV,size_t matrixGap);
}

class CPolygon;

class TrenderedTextSubtitleWord : public Rasterizer
{
private:
    TrenderedTextSubtitleWord *secondaryColoredWord;

    int m_outlineWidth;
    TfontSettings::TshadowMode m_shadowMode;
    double outlineWidth_double;
    mutable int oldFader;
    mutable unsigned int oldBodyYUVa,oldOutlineYUVa;
    unsigned int gdi_dx,gdi_dy;
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

    void updateMask(int fader = 1 << 16, int create = 1, bool isAlpha = false, int bodyA = 256, int outlineA = 256) const; // create: 0=update, 1=new, 2=update after copy (karaoke)
    void createShadow() const;
    unsigned char* blur(unsigned char *src,stride_t Idx,stride_t Idy,int startx,int starty,int endx, int endy, int blurStrength);
    void init();

    inline void RGBfontRenderer(int x, int y,
        int bodyYUVa, int outlineYUVa, int shadowYUVa,
        unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
        unsigned char *dst, stride_t dstStride) const;
    inline void RGBfontRendererFillBody(int x, int y,
        int bodyYUVa, int outlineYUVa, int shadowYUVa,
        unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
        unsigned char *dst, stride_t dstStride) const;
    inline void YV12_YfontRenderer(int x, int y,
        int bodyYUVa, int outlineYUVa, int shadowYUVa,
        unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
        unsigned char *dst, stride_t dstStride) const;
    inline void YV12_UVfontRenderer(int x, int y,
        int bodyYUVa, int outlineYUVa, int shadowYUVa,
        unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
        unsigned char *dstU, unsigned char *dstV, stride_t dstStride) const;

    class TexpandedGlyph
    {
        unsigned char *expBmp;
    public:
        int dx;
        int dy;
        TexpandedGlyph(const TrenderedTextSubtitleWord &word) {
            dx = word.dx[0] + 2 * word.m_outlineWidth;
            dy = word.dy[0] + 2 * word.m_outlineWidth;
            expBmp = aligned_calloc3<uint8_t>(dx, dy, 16);
            for (unsigned int y = 0 ; y < word.dy[0] ; y++) {
                memcpy(expBmp + dx * (y + word.m_outlineWidth)+ word.m_outlineWidth, word.bmp[0] + word.dx[0] * y, word.dx[0]);
            }
        }

        ~TexpandedGlyph() {
            _aligned_free(expBmp);
        }

        operator unsigned char*() const {
            return expBmp;
        }
    };

protected:
	CPolygon* m_pOpaqueBox;

    int m_shadowSize;
    double m_baseline;
    double m_ascent,m_descent;
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
    virtual void print(int startx, int starty, unsigned int dx[3],int dy[3],unsigned char *dstLn[3],const stride_t stride[3],const unsigned char *bmp[3],const unsigned char *msk[3],REFERENCE_TIME rtStart=REFTIME_INVALID) const {} // unused
    void paint(int startx, int starty, unsigned int dx[3],int dy[3],unsigned char *dstLn[3],const stride_t stride[3],ptrdiff_t srcOffset[3],REFERENCE_TIME rtStart=REFTIME_INVALID) const;
    unsigned int alignXsize;
    void (__cdecl *YV12_lum2chr_min)(const unsigned char* lum0,const unsigned char* lum1,unsigned char* chr);
    void (__cdecl *YV12_lum2chr_max)(const unsigned char* lum0,const unsigned char* lum1,unsigned char* chr);
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
