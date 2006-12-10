#ifndef _TMIXERSETTINGS_H_
#define _TMIXERSETTINGS_H_

#include "TfilterSettings.h"

struct TsampleFormat;
struct TmixerSettings :TfilterSettingsAudio
{
private:
 static const TfilterIDFF idffs;
protected:
 virtual const int *getResets(unsigned int pageId);
public: 
 TmixerSettings(TintStrColl *Icoll=NULL,TfilterIDFFs *filters=NULL);
 struct TchConfig
  {
   int id;
   const char_t *name;int nameIndex;
   unsigned int nchannels,channelmask;
   int dolby;
  };
 static const TchConfig chConfigs[];
 
 int out;
 void setFormatOut(TsampleFormat &fmt) const;
 void setFormatOut(TsampleFormat &outfmt,const TsampleFormat &infmt) const;
 int normalizeMatrix,expandStereo,voiceControl;
 int customMatrix;
 int matrix00,matrix01,matrix02,matrix03,matrix04,matrix05,
     matrix10,matrix11,matrix12,matrix13,matrix14,matrix15,
     matrix20,matrix21,matrix22,matrix23,matrix24,matrix25,
     matrix30,matrix31,matrix32,matrix33,matrix34,matrix35,
     matrix40,matrix41,matrix42,matrix43,matrix44,matrix45,
     matrix50,matrix51,matrix52,matrix53,matrix54,matrix55;
 int clev,slev,lfelev;
 int autogain;
 int headphone_dim;
 virtual void createFilters(size_t filtersorder,Tfilters *filters,TfilterQueue &queue) const;
 virtual void createPages(TffdshowPageDec *parent) const;
 virtual bool getTip(unsigned int pageId,char_t *buf,size_t buflen);
};

#endif
