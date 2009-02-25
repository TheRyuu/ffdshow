#ifndef _TFONT_H_
#define _TFONT_H_

#include "interfaces.h"
#include "ffImgfmt.h"
#include "TsubtitleProps.h"
#include "rational.h"
#include "TfontSettings.h"

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
class TrenderedSubtitleLines: public std::vector<TrenderedSubtitleLine*>
{
public:
 struct TprintPrefs
  {
   TprintPrefs(IffdshowBase *Ideci,const TfontSettings *IfontSettings)
    {
     memset(this,0,sizeof(this));
     if (IfontSettings)
      fontSettings = *IfontSettings;
     deci=Ideci;
     config=NULL;
     sizeDx=0;
     sizeDy=0;
     posXpix=0;
     fontchangesplit=false;
     textBorderLR=0;
     tabsize=8;
     dvd=false;
     blur=false;
     outlineBlur=false;
     clipdy=0;
     sar=Rational(1,1);
     opaqueBox=false;
     subformat=-1;
     isOSD=false;
     xinput=0;
     yinput=0;
     rtStart=REFTIME_INVALID;
    }
   TprintPrefs()
    {
     memset(this,0,sizeof(*this));
    }
   bool operator != (const TrenderedSubtitleLines::TprintPrefs &rt)
    {
     // compare except rtStart
     REFERENCE_TIME rtStart0 = rtStart;
     rtStart = rt.rtStart;
     bool ret= !!memcmp(this, &rt, sizeof(*this));
     rtStart = rtStart0;
     return ret;
    }
   bool operator == (const TrenderedSubtitleLines::TprintPrefs &rt)
    {
     REFERENCE_TIME rtStart0 = rtStart;
     rtStart = rt.rtStart;
     bool ret= !memcmp(this, &rt, sizeof(*this));
     rtStart = rtStart0;
     return ret;
    }
   void operator = (const TrenderedSubtitleLines::TprintPrefs &rt)
    {
     memcpy(this, &rt, sizeof(*this));
    }
   
   unsigned char **dst;
   const stride_t *stride;
   const unsigned int *shiftX,*shiftY;
   unsigned int dx,dy,clipdy;
   bool isOSD;
   int xpos,ypos;
   int align,alignSSA;
   int linespacing;
   unsigned int sizeDx,sizeDy;
   int posXpix;
   bool vobchangeposition;int vobscale,vobaamode,vobaagauss;
   bool fontchangesplit,fontsplit;
   int textBorderLR;
   int tabsize;
   bool dvd;
   IffdshowBase *deci;
   const Tconfig *config;
   int shadowMode, shadowAlpha; // Subtitles shadow
   double shadowSize;
   bool blur,outlineBlur;
   int csp,cspBpp;
   double outlineWidth;
   Rational sar;
   bool opaqueBox;
   int subformat;
   REFERENCE_TIME rtStart;
   unsigned int xinput,yinput;
   TfontSettings fontSettings;
  };
 TrenderedSubtitleLines(void) {}
 TrenderedSubtitleLines(TrenderedSubtitleLine *ln) {push_back(ln);}
 /**
  * reset
  *  just clear pointers, do not delete objects.
  */
 void reset(void)
  {
   erase(begin(),end());
  }
 /**
  * clear
  *  clear pointers and delete objects.
  */
 void clear(void);
 using std::vector<value_type>::empty;
 void print(const TprintPrefs &prefs);
 size_t getMemorySize() const;

private:
 void printASS(const TprintPrefs &prefs);
 class ParagraphKey
  {
   public:
    short alignment;
    short marginTop,marginBottom;
    short marginL,marginR;
    short isPos;
    short isMove;
    short posx,posy;
    short layer;

    ParagraphKey(): layer(0),alignment(-1), marginTop(-1), marginBottom(-1), marginL(-1), marginR(-1), isPos(false), posx(-1),posy(-1){};
  };
 friend bool operator < (const TrenderedSubtitleLines::ParagraphKey &a, const TrenderedSubtitleLines::ParagraphKey &b);

 class ParagraphValue
  {
   public:
    double topOverhang,bottomOverhang;
    double width,height,y;
    double xmin,xmax,y0,xoffset,yoffset;
    bool firstuse;

    bool checkCollision(ParagraphValue pVal)
    {
        if (((y0+yoffset >= pVal.y0 && y0+yoffset < pVal.y0+pVal.height)
               || (y0+yoffset<pVal.y0 &&  y0+yoffset+height>pVal.y0)) &&
               ((xmin+xoffset >= pVal.xmin && xmin+xoffset < pVal.xmax)
               || (xmin+xoffset<pVal.xmin && xmax+xoffset>pVal.xmin)))
               return true;
        return false;
    }

    ParagraphValue(): topOverhang(0), bottomOverhang(0),width(0),height(0), y(0), xmin(-1),xmax(-1),y0(0),xoffset(0),yoffset(0), firstuse(true) {};
  };
 void prepareKey(const_iterator i,ParagraphKey &pkey,unsigned int prefsdx,unsigned int prefsdy);
};

bool operator < (const TrenderedSubtitleLines::ParagraphKey &a, const TrenderedSubtitleLines::ParagraphKey &b);

class TrenderedTextSubtitleWord;

class TcharsChache
{
private:
 typedef stdext::hash_map<int,TrenderedTextSubtitleWord*> Tchars;
 Tchars chars;
 HDC hdc;
 YUVcolorA yuv,outlineYUV,shadowYUV;
 int xscale,yscale;
 IffdshowBase *deci;
public:
 const YUVcolorA& getBodyYUV(void) const {return yuv;}
 const YUVcolorA& getOutlineYUV(void) const {return outlineYUV;}
 const YUVcolorA& getShadowYUV(void) const {return shadowYUV;}
 TcharsChache(HDC Ihdc,const YUVcolorA &Iyuv,const YUVcolorA &Ioutline,const YUVcolorA &Ishadow,int Ixscale,int Iyscale,IffdshowBase *Ideci);
 const TrenderedTextSubtitleWord *getChar(const wchar_t *s,const TrenderedSubtitleLines::TprintPrefs &prefs,const LOGFONT &lf,TSubtitleProps props,unsigned int gdi_font_scale,unsigned int GDI_rendering_window);
 ~TcharsChache();
};

class TrenderedSubtitleWordBase
{
private:
 bool own;
public:
 TrenderedSubtitleWordBase(bool Iown):own(Iown) 
  {
   for (int i=0;i<3;i++)
    {
     bmp[i]=NULL;msk[i]=NULL;
     outline[i]=NULL;shadow[i]=NULL;
    }
  }
 ~TrenderedSubtitleWordBase();
 unsigned int dx[3],dy[3];
 unsigned int dxCharY,dyCharY;
 unsigned char *bmp[3],*msk[3];stride_t bmpmskstride[3];
 unsigned char *outline[3],*shadow[3];
 virtual void print(int startx, int starty, unsigned int dx[3],int dy[3],unsigned char *dstLn[3],const stride_t stride[3],const unsigned char *bmp[3],const unsigned char *msk[3],REFERENCE_TIME rtStart=REFTIME_INVALID) const =0;
 int csp;
 virtual int get_ascent64() const {return dy[0]*8;}
 virtual int get_descent64() const {return 0;}
 virtual int get_baseline() const {return dy[0];}
 virtual int get_topOverhang() const {return 0;}
 virtual int get_bottomOverhang() const {return 0;}
 virtual int get_leftOverhang() const {return 0;}
 virtual int get_rightOverhang() const {return 0;}
 virtual size_t getMemorySize() const {return 0;}
};

class TrenderedVobsubWord : public TrenderedSubtitleWordBase
{
private:
 bool shiftChroma;
public:
 TrenderedVobsubWord(bool IshiftChroma=true):shiftChroma(IshiftChroma),TrenderedSubtitleWordBase(false) {}
 virtual void print(int startx, int starty, unsigned int dx[3],int dy[3],unsigned char *dstLn[3],const stride_t stride[3],const unsigned char *bmp[3],const unsigned char *msk[3],REFERENCE_TIME rtStart=REFTIME_INVALID) const;
};

class TrenderedTextSubtitleWord : public TrenderedSubtitleWordBase
{
private:
 TrenderedTextSubtitleWord *secondaryColoredWord;
 TrenderedSubtitleLines::TprintPrefs prefs;
 YUVcolorA m_bodyYUV,m_outlineYUV,m_shadowYUV;
 int baseline;
 int topOverhang,bottomOverhang,leftOverhang,rightOverhang;
 int m_outlineWidth,m_shadowSize,m_shadowMode;
 int dstOffset;
 mutable int oldFader;
 mutable unsigned int oldBodyYUVa,oldOutlineYUVa;
 template<int GDI_rendering_window> void drawShadow(
                        HDC hdc,
                        HBITMAP hbmp,
                        unsigned char *bmp16,
                        HGDIOBJ old,
                        double xscale,
                        const SIZE &sz,
                        unsigned int gdi_font_scale
                        );
 void updateMask(int fader = 1 << 16, int create = 1) const; // create: 0=update, 1=new, 2=update after copy (karaoke)
 unsigned char* blur(unsigned char *src,stride_t Idx,stride_t Idy,int startx,int starty,int endx, int endy, bool mild);
 unsigned int getShadowSize(LONG fontHeight, unsigned int gdi_font_scale);
 unsigned int getBottomOverhang(void);
 unsigned int getRightOverhang(void);
 unsigned int getTopOverhang(void);
 unsigned int getLeftOverhang(void);
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
                        const TrenderedSubtitleLines::TprintPrefs &prefs,
                        LOGFONT lf,
                        double xscale,
                        TSubtitleProps Iprops,
                        unsigned int gdi_font_scale,
                        unsigned int GDI_rendering_window                  // Only 4 and 6 are supported (easy to add).
                        );
 // secondary (for karaoke)
 TrenderedTextSubtitleWord(
                        const TrenderedTextSubtitleWord &parent,
                        bool senondaryColor                       // to distinguish from default copy constructor
                        );
 // fast rendering
 TrenderedTextSubtitleWord(
                        TcharsChache *chars,
                        const wchar_t *s,size_t strlens,
                        const TrenderedSubtitleLines::TprintPrefs &prefs,
                        const LOGFONT &lf,
                        TSubtitleProps Iprops,
                        unsigned int gdi_font_scale,
                        unsigned int GDI_rendering_window);
 ~TrenderedTextSubtitleWord();
 virtual void print(int startx, int starty, unsigned int dx[3],int dy[3],unsigned char *dstLn[3],const stride_t stride[3],const unsigned char *bmp[3],const unsigned char *msk[3],REFERENCE_TIME rtStart=REFTIME_INVALID) const;
 unsigned int alignXsize;
 void* (__cdecl *TtextSubtitlePrintY)  (const unsigned char* bmp,const unsigned char* outline,const unsigned char* shadow,const unsigned short *colortbl,const unsigned char* dst,const unsigned char* msk);
 void* (__cdecl *TtextSubtitlePrintUV) (const unsigned char* bmp,const unsigned char* outline,const unsigned char* shadow,const unsigned short *colortbl,const unsigned char* dstU,const unsigned char* dstV);
 void* (__cdecl *YV12_lum2chr_min)(const unsigned char* lum0,const unsigned char* lum1,unsigned char* chr);
 void* (__cdecl *YV12_lum2chr_max)(const unsigned char* lum0,const unsigned char* lum1,unsigned char* chr);
 virtual int get_ascent64() const;
 virtual int get_descent64() const;
 virtual int get_baseline() const;
 virtual int get_topOverhang() const {return topOverhang;}
 virtual int get_bottomOverhang() const {return bottomOverhang;}
 virtual int get_leftOverhang() const {return leftOverhang;}
 virtual int get_rightOverhang() const {return rightOverhang;}
 virtual size_t getMemorySize() const;
};

class TrenderedSubtitleLine : protected std::vector<TrenderedSubtitleWordBase*>
{
 bool firstrun;
public:
 TrenderedSubtitleLine(void):firstrun(true) {props.reset();}
 TrenderedSubtitleLine(TSubtitleProps p):firstrun(true) {props=p;}
 TrenderedSubtitleLine(TrenderedSubtitleWordBase *w):firstrun(true) {push_back(w);props.reset();}
 unsigned int width(void) const;
 unsigned int height(void) const;
 double charHeight(void) const;
 unsigned int baselineHeight(void) const;
 int get_topOverhang(void) const;
 int get_bottomOverhang(void) const;
 int get_leftOverhang(void) const;
 int get_rightOverhang(void) const;
 void prepareKaraoke(void);
 using std::vector<value_type>::push_back;
 using std::vector<value_type>::empty;
 void clear(void);
 void print(int startx,int starty,const TrenderedSubtitleLines::TprintPrefs &prefs,unsigned int prefsdx,unsigned int prefsdy) const;
 TSubtitleProps props;
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
 unsigned int gdi_font_scale;
 TfontManager *fontManager;
 HDC hdc;HGDIOBJ oldFont;
 TrenderedSubtitleLines lines;
 unsigned int height;
 const Tsubtitle *oldsub;
 int oldCsp;
 YUVcolorA yuvcolor,outlineYUV,shadowYUV;
 short matrix[5][5];
 void prepareC(TsubtitleText *sub,const TrenderedSubtitleLines::TprintPrefs &prefs,bool forceChange);
 TcharsChache *charsCache;
 TrenderedTextSubtitleWord* newWord(const wchar_t *s,size_t slen,TrenderedSubtitleLines::TprintPrefs prefs,const TsubtitleWord *w,const LOGFONT &lf,bool trimRightSpaces=false);
public:
 friend struct TsubtitleText;
 TfontSettings *fontSettings;
 // gdi_font_scale: 4: for OSD. rendering_window is 4x5.
 //                 8-16: for subtitles. 16:very sharp (slow), 12:soft & sharp, (moderately slow) 8:blurry (fast)
 Tfont(IffdshowBase *Ideci, unsigned int Igdi_font_scale);
 ~Tfont();
 void init(const TfontSettings *IfontSettings);
 /**
  * print (for OSD)
  * @return height
  */
 int print(TsubtitleText *sub,bool forceChange,const TrenderedSubtitleLines::TprintPrefs &prefs);
 /**
  * printf(for subtitles)
  * lines must be filled before called
  */
 void print(const TrenderedSubtitleLines::TprintPrefs &prefs);
 void reset(void)
  {
   lines.reset();
  }
 void done(void);
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
