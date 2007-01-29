#ifndef _TSUBTITLEPROPS_H_
#define _TSUBTITLEPROPS_H_
struct TfontSettings;

struct TSubtitleProps
{
 TSubtitleProps(void) {reset();}
 TSubtitleProps(unsigned int IrefResX,unsigned int IrefResY) {reset();refResX=IrefResX;refResY=IrefResY;}
 TSubtitleProps(bool Ibold,bool Iitalic, bool Iunderline) {reset();bold=Ibold;italic=Iitalic;underline=Iunderline;}
 bool bold,italic,underline,strikeout;
 bool isColor;COLORREF color;
 bool isPos;int posx,posy;
 unsigned int refResX,refResY;
 int size;
 int scaleX,scaleY; //in percents, -1 default
 char_t fontname[LF_FACESIZE*2]; // LF_FACESIZE chars max because of LF_FACESIZE.
 int encoding; //-1 = default
 int spacing;  //-1 = default
 void reset(void);
 void toLOGFONT(LOGFONT &lf,const TfontSettings &fontSettings,unsigned int dx,unsigned int dy) const;

 // Alignment. This sets how text is "justified" within the Left/Right onscreen margins,
 // and also the vertical placing. Values may be 1=Left, 2=Centered, 3=Right.
 // Add 4 to the value for a "Toptitle". Add 8 to the value for a "Midtitle".
 // eg. 5 = left-justified toptitle
 int alignment;  // -1 = default(center)

 int marginR,marginL,marginV,marginTop,marginBottom; // -1 = default 
};

#endif
