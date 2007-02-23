#ifndef _TFONTSETTINGS_H_
#define _TFONTSETTINGS_H_

#include "Toptions.h"

struct TfontSettings :Toptions
{
public:
 TfontSettings(TintStrColl *Icoll=NULL);
 TfontSettings& operator =(const TfontSettings &src)
  {
   memcpy(((uint8_t*)this)+sizeof(Toptions),((uint8_t*)&src)+sizeof(Toptions),sizeof(*this)-sizeof(Toptions));
   return *this;
  }
 char_t name[260];
 int charset;
 int autosize,autosizeVideoWindow;
 int sizeP,sizeA;
 int xscale;
 unsigned int getSize(unsigned int AVIdx,unsigned int AVIdy) const
  {
   if (autosize && AVIdx && AVIdy)
    return limit(sizeA*ff_sqrt(AVIdx*AVIdx+AVIdy*AVIdy)/1000,3U,255U);
   else
    return sizeP;
  }
 struct Tweigth
  {
   const char_t *name;
   int id;
  };
 static const Tweigth weights[];
 static const int charsets[];
 static const char_t *getCharset(int i);
 static int getCharset(const char_t *name);
 bool getTip(char_t *buf,size_t len)
  {
   tsnprintf(buf,len,_l("Font: %s, %s charset, %ssize:%i, %s, spacing:%i\nShadow intensity: %i, shadow radius:%i"),name,getCharset(charset),autosize?_l("auto"):_l(""),autosize?sizeA:sizeP,weights[weight/100-1].name,spacing,shadowStrength,shadowRadius);
   return true;
  }
 int spacing,weight,color,shadowStrength,shadowRadius;
 int split;
 int fast;
 virtual void reg_op(TregOp &t);
};

struct TfontSettingsOSD :TfontSettings
{
 TfontSettingsOSD(TintStrColl *Icoll=NULL);
};

struct TfontSettingsSub :TfontSettings
{
 TfontSettingsSub(TintStrColl *Icoll=NULL);
 virtual void reg_op(TregOp &t);
};

#endif
