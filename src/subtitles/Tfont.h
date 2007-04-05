#ifndef _TFONT_H_
#define _TFONT_H_

#include "interfaces.h"
#include "ffImgfmt.h"
#include "TsubtitleProps.h"

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
class TrenderedSubtitleLines: protected std::vector<TrenderedSubtitleLine*>
{
public:
 struct TprintPrefs
  {
   TprintPrefs(IffdshowBase *Ideci):deci(Ideci),config(NULL),sizeDx(0),sizeDy(0),posXpix(0),fontchangesplit(false),textBorderLR(0),tabsize(8),dvd(false) {}
   unsigned char **dst;
   const stride_t *stride;
   const unsigned int *shiftX,*shiftY;
   unsigned int dx,dy;
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
   IffdshowBase *deci;
   const Tconfig *config;
   int shadowMode, shadowAlpha; // Subtitles shadow
   double shadowSize;
  };
 TrenderedSubtitleLines(void) {}
 TrenderedSubtitleLines(TrenderedSubtitleLine *ln) {push_back(ln);}
 void add(TrenderedSubtitleLine *ln,unsigned int *height);
 void clear(void);
 using std::vector<value_type>::empty;
 void print(const TprintPrefs &prefs);
};

class TrenderedSubtitleWord;
class TcharsChache
{
private:
 typedef std::hash_map<int,TrenderedSubtitleWord*> Tchars;
 Tchars chars;
 HDC hdc;
 const short (*matrix)[5];
 const YUVcolor &yuv;
 int xscale;
 IffdshowBase *deci;
public:
 TcharsChache(HDC Ihdc,const short (*Imatrix)[5],const YUVcolor &Iyuv,int Ixscale,IffdshowBase *Ideci);
 template<class tchar> const TrenderedSubtitleWord *getChar(tchar *s,const TrenderedSubtitleLines::TprintPrefs &prefs);
 ~TcharsChache();
};

class TrenderedSubtitleWordBase
{
private:
 bool own;
public:
 TrenderedSubtitleWordBase(bool Iown):own(Iown) {}
 ~TrenderedSubtitleWordBase();
 unsigned int dx[3],dy[3];
 unsigned int dxCharY,dyCharY;
 unsigned char *bmp[3],*msk[3];stride_t bmpmskstride[3];
 virtual void print(unsigned int dx[3],unsigned char *dstLn[3],const stride_t stride[3],const unsigned char *bmp[3],const unsigned char *msk[3]) const =0;
};

class TrenderedSubtitleWord : public TrenderedSubtitleWordBase
{
private:
 bool shiftChroma;
 void drawShadow(HDC hdc,HBITMAP hbmp,unsigned char *bmp16,HGDIOBJ old,int xscale,const SIZE &sz,const TrenderedSubtitleLines::TprintPrefs &prefs,const short (*matrix)[5],const YUVcolor &yuv,unsigned int shadowSize);
 unsigned int getShadowSize(const TrenderedSubtitleLines::TprintPrefs &prefs,LONG fontHeight);
public:
 template<class tchar> TrenderedSubtitleWord(HDC hdc,const tchar *s,size_t strlens,const short (*matrix)[5],const YUVcolor &yuv,const TrenderedSubtitleLines::TprintPrefs &prefs,int xscale);
 template<class tchar> TrenderedSubtitleWord(TcharsChache *chars,const tchar *s,size_t strlens,const TrenderedSubtitleLines::TprintPrefs &prefs);
 TrenderedSubtitleWord(bool IshiftChroma=true):shiftChroma(IshiftChroma),TrenderedSubtitleWordBase(false) {}
 virtual void print(unsigned int dx[3],unsigned char *dstLn[3],const stride_t stride[3],const unsigned char *bmp[3],const unsigned char *msk[3]) const;
};

class TrenderedSubtitleLine : protected std::vector<TrenderedSubtitleWordBase*>
{
public:
 TrenderedSubtitleLine(void) {props.reset();}
 TrenderedSubtitleLine(TSubtitleProps p) {props=p;}
 TrenderedSubtitleLine(TrenderedSubtitleWordBase *w) {push_back(w);props.reset();}
 unsigned int width(void) const,height(void) const;
 using std::vector<value_type>::push_back;
 using std::vector<value_type>::empty;
 void clear(void);
 void print(int startx,int starty,const TrenderedSubtitleLines::TprintPrefs &prefs,unsigned int prefsdx,unsigned int prefsdy) const;
 TSubtitleProps props;
};

template<class tchar> struct TsubtitleWord;
struct TfontSettings;
struct Tsubtitle;
struct TfontSettings;
template<class tchar> struct TsubtitleTextBase;
class Tfont
{
private:
 IffdshowBase *deci;
 TfontManager *fontManager;
 TfontSettings *fontSettings;
 HDC hdc;HGDIOBJ oldFont;
 TrenderedSubtitleLines lines;
 unsigned int height;
 const Tsubtitle *oldsub;
 YUVcolor yuvcolor;
 short matrix[5][5];
 template<class tchar> void prepareC(const TsubtitleTextBase<tchar> *sub,const TrenderedSubtitleLines::TprintPrefs &prefs,bool forceChange);
 TcharsChache *charsCache;
 template<class tchar> TrenderedSubtitleWord* newWord(const tchar *s,size_t slen,TrenderedSubtitleLines::TprintPrefs prefs,const TsubtitleWord<tchar> *w);
public:
 Tfont(IffdshowBase *Ideci);
 ~Tfont();
 void init(const TfontSettings *IfontSettings);
 template<class tchar> void print(const TsubtitleTextBase<tchar> *sub,bool forceChange,const TrenderedSubtitleLines::TprintPrefs &prefs,unsigned int *y=NULL);
 void done(void);
};

#endif
