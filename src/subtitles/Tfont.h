#ifndef _TFONT_H_
#define _TFONT_H_

#include "interfaces.h"
#include "ffImgfmt.h"
#include "TsubtitleProps.h"
#include "rational.h"
#include "TfontSettings.h"
#include "CRect.h"
#include "ffdebug.h"

#define size_of_rgb32 4

enum
{
 ALIGN_FFDSHOW=0,
 ALIGN_LEFT=1,
 ALIGN_CENTER=2,
 ALIGN_RIGHT=3
};

class TrenderedSubtitleLine;
class TfontManager;
struct Tconfig;

struct TprintPrefs {
    TprintPrefs(IffdshowBase *Ideci,const TfontSettings *IfontSettings);

    TprintPrefs() {
        memset(this,0,sizeof(*this));
        memset(&fontSettings,0,sizeof(fontSettings));
    }

    bool operator != (const TprintPrefs &rt) const;
    bool operator == (const TprintPrefs &rt) const;

    void operator = (const TprintPrefs &rt) {
        memcpy(this, &rt, sizeof(*this));
    }
    
    unsigned int dx,dy,clipdy;
    bool isOSD;
    int xpos,ypos;
    int align;
    int linespacing;
    unsigned int sizeDx,sizeDy;
    int stereoScopicParallax;
    bool vobchangeposition;int vobscale,vobaamode,vobaagauss;
    bool fontchangesplit,fontsplit;
    int textBorderLR;
    int tabsize;
    bool dvd;
    int shadowMode, shadowAlpha; // Subtitles shadow
    double shadowSize;
    bool blur,outlineBlur;
    int csp;
    double outlineWidth;
    Rational sar;
    bool opaqueBox;
    int subformat;
    unsigned int xinput,yinput;
    TfontSettings fontSettings;
    YUVcolorA yuvcolor,outlineYUV,shadowYUV;

    // members that are not compared by operator == and !=
    REFERENCE_TIME rtStart;
    IffdshowBase *deci;
    const Tconfig *config;
};

class TrenderedSubtitleLines: public std::vector<TrenderedSubtitleLine*>
{
public:
    TrenderedSubtitleLines() {}
    TrenderedSubtitleLines(TrenderedSubtitleLine *ln) {push_back(ln);}

    /**
     * reset
     *  just clear pointers, do not delete objects.
     */
    void reset()
     {
      erase(begin(),end());
     }

    /**
     * clear
     *  clear pointers and delete objects.
     */
    void clear();

    using std::vector<value_type>::empty;

    void print(
       const TprintPrefs &prefs,
       unsigned char **dst,
       const stride_t *stride);

    size_t getMemorySize() const;

private:
    void printASS(
       const TprintPrefs &prefs,
       unsigned char **dst,
       const stride_t *stride);

    class ParagraphKey {
    public:
        int alignment;
        int marginTop,marginBottom;
        int marginL,marginR;
        int isPos;
        int isMove;
        CPoint pos;
        int layer;
        bool hasPrintedRect;
        CRect printedRect;
        int lineID;

        ParagraphKey(TrenderedSubtitleLine *line, unsigned int prefsdx, unsigned int prefsdy);
        bool operator != (const ParagraphKey &rt) const;
        bool operator == (const ParagraphKey &rt) const;
        bool operator < (const ParagraphKey &rt) const;
    };

    class ParagraphValue {
    public:
        double topOverhang;
        double bottomOverhang;
        double width,height,y;
        double linegap;
        double xmin,xmax,y0,xoffset,yoffset;
        bool firstuse;

        ParagraphValue():
            topOverhang(0),
            bottomOverhang(0),
            width(0),
            height(0),
            y(0),
            linegap(0),
            xmin(-1),
            xmax(-1),
            y0(0),
            xoffset(0),
            yoffset(0),
            firstuse(true)
            {};
    };
    class TlayerSort {
    public:
        bool operator() (TrenderedSubtitleLine *lt, TrenderedSubtitleLine *rt) const;
    };

    void handleCollision(TrenderedSubtitleLine *line, int x, ParagraphValue &pval, unsigned int prefsdy, int alignment);
};

class TrenderedSubtitleWordBase
{
private:
    bool own;
public:
    TrenderedSubtitleWordBase(bool Iown):
        own(Iown),
        dxChar(0),
        dyChar(0) 
    {
        for (int i=0;i<3;i++) {
            bmp[i]=NULL;msk[i]=NULL;
            outline[i]=NULL;shadow[i]=NULL;
            dx[i]=0;dy[i]=0;
        }
    }
    virtual ~TrenderedSubtitleWordBase();
    unsigned int dx[3],dy[3];
    unsigned int dxChar,dyChar;
    unsigned char *bmp[3],*msk[3];stride_t bmpmskstride[3];
    unsigned char *outline[3],*shadow[3];
    virtual void print(int startx, int starty, unsigned int dx[3],int dy[3],unsigned char *dstLn[3],const stride_t stride[3],const unsigned char *bmp[3],const unsigned char *msk[3],REFERENCE_TIME rtStart=REFTIME_INVALID) const =0;
    int csp;
    virtual double get_ascent() const {return dy[0];}
    virtual double get_descent() const {return 0;}
    virtual double get_baseline() const {return dy[0];}
    virtual double get_below_baseline() const {return 0;}
    virtual double get_linegap() const {return 0;}
    virtual CRect getOverhang() const {return CRect();}
    virtual size_t getMemorySize() const {return 0;}
    virtual int getPathOffsetX() const {return 0;}
    virtual int getPathOffsetY() const {return 0;}
};

class TrenderedVobsubWord : public TrenderedSubtitleWordBase
{
private:
    bool shiftChroma;
public:
    TrenderedVobsubWord(bool IshiftChroma=true):shiftChroma(IshiftChroma),TrenderedSubtitleWordBase(false) {}
    virtual void print(int startx, int starty, unsigned int dx[3],int dy[3],unsigned char *dstLn[3],const stride_t stride[3],const unsigned char *bmp[3],const unsigned char *msk[3],REFERENCE_TIME rtStart=REFTIME_INVALID) const;
};

class TrenderedTextSubtitleWord;

class TrenderedSubtitleLine : public std::vector<TrenderedSubtitleWordBase*>
{
    bool firstrun;
    TSubtitleProps props;
    double emptyHeight; // This is used as charHeight if empty.
    bool hasPrintedRect;
    CRect printedRect;
public:

    TrenderedSubtitleLine():firstrun(true),hasPrintedRect(false) {props.reset();}
    TrenderedSubtitleLine(TSubtitleProps p):firstrun(true),hasPrintedRect(false) {props=p;}
    TrenderedSubtitleLine(TSubtitleProps p, double IemptyHeight):firstrun(true),emptyHeight(IemptyHeight),hasPrintedRect(false) {props=p;}
    TrenderedSubtitleLine(TrenderedSubtitleWordBase *w):firstrun(true),hasPrintedRect(false) {push_back(w);props.reset();}

    TSubtitleProps& getPropsOfThisObject() {return props;}
    const TSubtitleProps& getProps() const;

    const CRect& getPrintedRect() const {return printedRect;}
    bool getHasPrintedRect() const {return hasPrintedRect;}
    bool checkCollision(const CRect &query, CRect &ans);

    unsigned int width() const;
    unsigned int height() const;
    double charHeight() const;
    double linegap(double prefsdy) const;
    double lineHeightWithGap(double prefsdy) const;
    unsigned int baselineHeight() const;
    int get_topOverhang() const;
    int get_bottomOverhang() const;
    int get_leftOverhang() const;
    int get_rightOverhang() const;
    void prepareKaraoke();
    using std::vector<value_type>::push_back;
    using std::vector<value_type>::empty;
    void clear();
    void print(
       int startx,
       int starty,
       const TprintPrefs &prefs,
       unsigned int prefsdx,
       unsigned int prefsdy,
       unsigned char **dst,
       const stride_t *stride);
    size_t getMemorySize() const;
};

struct TsubtitleWord;
struct TfontSettings;
struct Tsubtitle;
struct TfontSettings;
struct TsubtitleText;
class Tfont
{
private:
    IffdshowBase *deci;
    TfontManager *fontManager;
    HDC hdc;HGDIOBJ oldFont;
    TrenderedSubtitleLines lines;
    unsigned int height;
    int oldCsp;
    short matrix[5][5];
    void prepareC(TsubtitleText *sub,const TprintPrefs &prefs,bool forceChange);
public:
    friend struct TsubtitleText;
    //TfontSettings *fontSettings;
    Tfont(IffdshowBase *Ideci);
    ~Tfont();
    void init();
    /**
     * print (for OSD)
     * @return height
     */
    int print(
       TsubtitleText *sub,
       bool forceChange,
       const TprintPrefs &prefs,
       unsigned char **dst,
       const stride_t *stride);
    /**
     * printf(for subtitles)
     * lines must be filled before called
     */
    void print(
       const TprintPrefs &prefs,
       unsigned char **dst,
       const stride_t *stride);
    void reset()
    {
        lines.reset();
    }
    void done();
};

extern "C" {
 void* (__cdecl TtextSubtitlePrintY_mmx)(const unsigned char* bmp,const unsigned char* outline,const unsigned char* shadow,const unsigned short* colortbl,const unsigned char* dst,const unsigned char* msk);
 void* (__cdecl TtextSubtitlePrintUV_mmx)(const unsigned char* bmp,const unsigned char* outline,const unsigned char* shadow,const unsigned short* colortbl,const unsigned char* dstU,const unsigned char* dstV);
 void* (__cdecl TtextSubtitlePrintY_sse2)(const unsigned char* bmp,const unsigned char* outline,const unsigned char* shadow,const unsigned short* colortbl,const unsigned char* dst,const unsigned char* msk);
 void* (__cdecl TtextSubtitlePrintUV_sse2)(const unsigned char* bmp,const unsigned char* outline,const unsigned char* shadow,const unsigned short* colortbl,const unsigned char* dstU,const unsigned char* dstV);
 void* (__cdecl YV12_lum2chr_min_mmx)(const unsigned char* lum0,const unsigned char* lum1,unsigned char* chr);
 void* (__cdecl YV12_lum2chr_max_mmx)(const unsigned char* lum0,const unsigned char* lum1,unsigned char* chr);
 void* (__cdecl YV12_lum2chr_min_mmx2)(const unsigned char* lum0,const unsigned char* lum1,unsigned char* chr);
 void* (__cdecl YV12_lum2chr_max_mmx2)(const unsigned char* lum0,const unsigned char* lum1,unsigned char* chr);
 void  __cdecl storeXmmRegs(unsigned char* buf);
 void  __cdecl restoreXmmRegs(unsigned char* buf);
 void __cdecl fontRGB32toBW_mmx(size_t count,unsigned char *ptr);
 unsigned int __cdecl fontPrepareOutline_sse2(const unsigned char *src,size_t srcStrideGap,const short *matrix,size_t matrixSizeH,size_t matrixSizeV);
 unsigned int __cdecl fontPrepareOutline_mmx (const unsigned char *src,size_t srcStrideGap,const short *matrix,size_t matrixSizeH,size_t matrixSizeV,size_t matrixGap);
}

#endif
