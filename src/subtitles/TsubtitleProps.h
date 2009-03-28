#ifndef _TSUBTITLEPROPS_H_
#define _TSUBTITLEPROPS_H_

#include "interfaces.h"
#include "Crect.h"

struct TfontSettings;
struct Rational;
class TfontManager;
struct TprintPrefs;

struct TSubtitleProps
{
 TSubtitleProps(void) {reset();}
 TSubtitleProps(unsigned int IrefResX,unsigned int IrefResY, int IwrapStyle) {reset();refResX=IrefResX;refResY=IrefResY;wrapStyle=IwrapStyle;}
 TSubtitleProps(bool Iitalic, bool Iunderline) {reset();italic=Iitalic;underline=Iunderline;}
 int bold;
 bool italic,underline,strikeout,blur;
 bool isColor;COLORREF color,SecondaryColour, TertiaryColour, OutlineColour, ShadowColour;
 int colorA,SecondaryColourA, TertiaryColourA, OutlineColourA, ShadowColourA;
 unsigned int refResX,refResY;
 bool isPos,isMove,isOrg;
 CPoint pos,pos2; // move from pos to pos2
 CPoint org;
 unsigned int t1,t2;
 int wrapStyle; // -1 = default
 int size;
 int scaleX,scaleY; //in percents, -1 = default
 char_t fontname[LF_FACESIZE];
 int encoding; // -1 = default
 int version;  // -1 = default
 int extendedTags; // 0 = default
 double spacing;  //INT_MIN = default
 double x; // Calculated x position
 double y; // Calculated y position
 int lineID;
 void reset(void);
 void toLOGFONT(LOGFONT &lf, const TfontSettings &fontSettings, unsigned int dx, unsigned int dy, unsigned int clipdy, const Rational& sar, unsigned int gdi_font_scale) const;

 // Alignment. This sets how text is "justified" within the Left/Right onscreen margins,
 // and also the vertical placing. Values may be 1=Left, 2=Centered, 3=Right.
 // Add 4 to the value for a "Toptitle". Add 8 to the value for a "Midtitle".
 // eg. 5 = left-justified toptitle]
 // -1 = default(center)
 int alignment;

 int marginR,marginL,marginV,marginTop,marginBottom; // -1 = default
 int borderStyle; // -1 = default
 double outlineWidth,shadowDepth; // -1 = default
 int layer; // 0 = default
 int get_spacing(unsigned int dy, unsigned int clipdy, unsigned int gdi_font_scale) const;
 int get_marginR(unsigned int screenWidth,unsigned int lineWidth=0) const;
 int get_marginL(unsigned int screenWidth,unsigned int lineWidth=0) const;
 int get_marginTop(unsigned int screenHeight) const;
 int get_marginBottom(unsigned int screenHeight) const;
 int get_xscale(int Ixscale,const Rational& sar,int aspectAuto,int overrideScale) const;
 int get_yscale(int Iyscale,const Rational& sar,int aspectAuto,int overrideScale) const;
 int get_movedistanceV(unsigned int screenHeight) const;
 int get_movedistanceH(unsigned int screenWidth) const;
 int get_maxWidth(unsigned int screenWidth, int subFormat, IffdshowBase *deci) const;
 REFERENCE_TIME get_moveStart(void) const;
 REFERENCE_TIME get_moveStop(void) const;
 static int alignASS2SSA(int align);
 int tmpFadT1,tmpFadT2;
 int isFad;
 int fadeA1,fadeA2,fadeA3;
 bool karaokeNewWord; // true if the word is top of karaoke sequence.
 REFERENCE_TIME tStart,tStop;
 REFERENCE_TIME fadeT1,fadeT2,fadeT3,fadeT4;
 REFERENCE_TIME karaokeDuration,karaokeStart;
 enum
  {
   KARAOKE_NONE,
   KARAOKE_k,
   KARAOKE_kf,
   KARAOKE_ko
  } karaokeMode;
};

#endif
