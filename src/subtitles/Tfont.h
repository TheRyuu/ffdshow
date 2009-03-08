#ifndef _TFONT_H_
#define _TFONT_H_

#include "interfaces.h"
#include "ffImgfmt.h"
#include "TsubtitleProps.h"
#include "rational.h"
#include "TfontSettings.h"
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
class TrenderedSubtitleLines: public std::vector<TrenderedSubtitleLine*>
{
public:
 struct TprintPrefs {
     TprintPrefs(IffdshowBase *Ideci,const TfontSettings *IfontSettings);

     TprintPrefs() {
         memset(this,0,sizeof(*this));
         memset(&fontSettings,0,sizeof(fontSettings));
     }

     bool operator != (const TrenderedSubtitleLines::TprintPrefs &rt) const;
     bool operator == (const TrenderedSubtitleLines::TprintPrefs &rt) const;
     void debug_print() const;

     void operator = (const TrenderedSubtitleLines::TprintPrefs &rt) {
         memcpy(this, &rt, sizeof(*this));
     }
     
     unsigned int dx,dy,clipdy;
     bool isOSD;
     int xpos,ypos;
     int align;
     int linespacing;
     unsigned int sizeDx,sizeDy;
     int posXpix;
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
 virtual ~TrenderedSubtitleWordBase();
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

class TrenderedTextSubtitleWord;

class TrenderedSubtitleLine : public std::vector<TrenderedSubtitleWordBase*>
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
 void print(
    int startx,
    int starty,
    const TrenderedSubtitleLines::TprintPrefs &prefs,
    unsigned int prefsdx,
    unsigned int prefsdy,
    unsigned char **dst,
    const stride_t *stride) const;
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
 TfontManager *fontManager;
 HDC hdc;HGDIOBJ oldFont;
 TrenderedSubtitleLines lines;
 unsigned int height;
 const Tsubtitle *oldsub;
 int oldCsp;
 short matrix[5][5];
 void prepareC(TsubtitleText *sub,const TrenderedSubtitleLines::TprintPrefs &prefs,bool forceChange);
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
    const TrenderedSubtitleLines::TprintPrefs &prefs,
    unsigned char **dst,
    const stride_t *stride);
 /**
  * printf(for subtitles)
  * lines must be filled before called
  */
 void print(
    const TrenderedSubtitleLines::TprintPrefs &prefs,
    unsigned char **dst,
    const stride_t *stride);
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
