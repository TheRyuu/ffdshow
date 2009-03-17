#ifndef _TRENDEREDTEXTSUBTITLEWORD_H_
#define _TRENDEREDTEXTSUBTITLEWORD_H_

#include "Tfont.h"
#include "Rasterizer.h"

class TrenderedTextSubtitleWord : public Rasterizer
{
private:
    TrenderedTextSubtitleWord *secondaryColoredWord;
    TprintPrefs prefs;
    YUVcolorA m_bodyYUV,m_outlineYUV,m_shadowYUV;
    double baseline;
    double m_ascent,m_descent,m_linegap;
    CRect overhang;
    int m_outlineWidth,m_shadowSize,m_shadowMode;
    double outlineWidth_double;
    int dstOffset;
    mutable int oldFader;
    mutable unsigned int oldBodyYUVa,oldOutlineYUVa;
    unsigned int gdi_dx,gdi_dy;

    void getGlyph(
        HDC hdc,
        const strings &s1,
        double xscale,
        SIZE italic_fixed_sz,
        const ints &cxs,
        const LOGFONT &lf);

    void Transform(CPoint org, double scalex);

    void drawGlyphSubtitles(
        HDC hdc,
        const strings &tab_parsed_string,
        const ints &cxs,
        double xscale);

    void drawGlyphOSD(
        HDC hdc,
        const strings &tab_parsed_string,
        const ints &cxs,
        double xscale);

    void drawShadow();

    void updateMask(int fader = 1 << 16, int create = 1) const; // create: 0=update, 1=new, 2=update after copy (karaoke)
    unsigned char* blur(unsigned char *src,stride_t Idx,stride_t Idy,int startx,int starty,int endx, int endy, bool mild);
    unsigned int getShadowSize(LONG fontHeight, unsigned int gdi_font_scale);
    CRect getOverhangPrivate();

    class TexpandedGlyph {
        unsigned char *expBmp;
    public:
        int dx;
        int dy;
        TexpandedGlyph(const TrenderedTextSubtitleWord &word)
        {
            dx = word.dx[0] + 2 * word.m_outlineWidth;
            dy = word.dy[0] + 2 * word.m_outlineWidth;
            expBmp = aligned_calloc3<uint8_t>(dx, dy, 16);
            for (unsigned int y = 0 ; y < word.dy[0] ; y++) {
                memcpy(expBmp + dx * (y + word.m_outlineWidth)+ word.m_outlineWidth, word.bmp[0] + word.dx[0] * y, word.dx[0]);
            }
        }

        ~TexpandedGlyph()
        {
            _aligned_free(expBmp);
        }

        operator unsigned char*() const {return expBmp;}
    };

public:
    TSubtitleProps props;
    // full rendering
    TrenderedTextSubtitleWord(
        HDC hdc,
        const wchar_t *s,
        size_t strlens,
        const YUVcolorA &YUV,
        const YUVcolorA &outlineYUV,
        const YUVcolorA &shadowYUV,
        const TprintPrefs &prefs,
        LOGFONT lf,
        double xscale,
        TSubtitleProps Iprops);

    // secondary (for karaoke)
    struct secondaryColor_t {};
    TrenderedTextSubtitleWord(
        const TrenderedTextSubtitleWord &parent,
        struct secondaryColor_t);
    virtual ~TrenderedTextSubtitleWord();
    virtual void print(int startx, int starty, unsigned int dx[3],int dy[3],unsigned char *dstLn[3],const stride_t stride[3],const unsigned char *bmp[3],const unsigned char *msk[3],REFERENCE_TIME rtStart=REFTIME_INVALID) const;
    unsigned int alignXsize;
    void* (__cdecl *TtextSubtitlePrintY)  (const unsigned char* bmp,const unsigned char* outline,const unsigned char* shadow,const unsigned short *colortbl,const unsigned char* dst,const unsigned char* msk);
    void* (__cdecl *TtextSubtitlePrintUV) (const unsigned char* bmp,const unsigned char* outline,const unsigned char* shadow,const unsigned short *colortbl,const unsigned char* dstU,const unsigned char* dstV);
    void* (__cdecl *YV12_lum2chr_min)(const unsigned char* lum0,const unsigned char* lum1,unsigned char* chr);
    void* (__cdecl *YV12_lum2chr_max)(const unsigned char* lum0,const unsigned char* lum1,unsigned char* chr);
    virtual double get_ascent() const;
    virtual double get_descent() const;
    virtual double get_below_baseline() const;
    virtual double get_linegap() const;
    virtual double get_baseline() const;
    virtual int getPathOffsetX() const {return mPathOffsetX >> 3;}
    virtual int getPathOffsetY() const {return mPathOffsetY >> 3;}
    virtual CRect getOverhang() const {return overhang;}
    virtual size_t getMemorySize() const;
    friend class TexpandedGlyph;
};


#endif //_TRENDEREDTEXTSUBTITLEWORD_H_