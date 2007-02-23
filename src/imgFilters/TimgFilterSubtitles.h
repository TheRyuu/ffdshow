#ifndef _TIMGFILTERSUBTITLES_H_
#define _TIMGFILTERSUBTITLES_H_

#include "TimgFilter.h"
#include "Tfont.h"
#include "TsubtitlesFile.h"
#include "TsubtitlesSettings.h"
#include "TexpandSettings.h"

class TsubtitlesTextpin;
class TimgFilterExpand;
struct TfontSettingsSub;
template<class tchar> struct TsubtitleTextBase;
DECLARE_FILTER(TimgFilterSubtitles,public,TimgFilter)
private:
 int isdvdproc;
 bool wasDiscontinuity;
 bool expandSizeChanged,fontSizeChanged;
 TsubtitlesFile subs;
 typedef std::hash_map<int,TsubtitlesTextpin*> Tembedded;
 Tembedded embedded;
 Tfont font,fontCC;
 TfontSettingsSub *oldFontCfg,*oldFontCCcfg;
 int oldstereo,oldsplitborder;
 double AVIfps;
 TimgFilterExpand *expand;TexpandSettings expandSettings;Trect oldExpandRect;
 int oldExpandCode;
 unsigned int oldSizeDx,oldSizeDy;
 CCritSec csEmbedded,csCC;
 TfilterQueue::iterator prevIt;TffPict prevPict;const TfilterSettingsVideo *prevCfg;bool again;Tbuffer prevbuf;
 int subFlnmChanged;
 const char_t* findAutoSubFlnm(const TsubtitlesSettings *cfg);

 struct TsubPrintPrefs : TrenderedSubtitleLines::TprintPrefs
  {
   TsubPrintPrefs(unsigned char *Idst[4],stride_t Istride[4],unsigned int Idx[4],unsigned int Idy[4],IffdshowBase *Ideci,const TsubtitlesSettings *cfg,const TffPict &pict,const Tconfig *Iconfig,bool Idvd);
  };

 TsubtitleTextBase<char> *cc;
 bool wasCCchange;
protected:
 virtual bool is(const TffPictBase &pict,const TfilterSettingsVideo *cfg);
 virtual int getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const {return FF_CSP_420P;}
 virtual void onSizeChange(void);
public:
 TimgFilterSubtitles(IffdshowBase *Ideci,Tfilters *Iparent);
 virtual ~TimgFilterSubtitles();
 virtual bool getOutputFmt(TffPictBase &pict,const TfilterSettingsVideo *cfg0);
 virtual HRESULT process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0);
 virtual void onSeek(void);
 void onSubFlnmChange(int id,int),onSubFlnmChangeStr(int id,const char_t*);

 bool initSubtitles(int id,int type,const unsigned char *extradata,unsigned int extradatalen);
 void addSubtitle(int id,REFERENCE_TIME start,REFERENCE_TIME stop,const unsigned char *data,unsigned int datalen,const TsubtitlesSettings *cfg,bool utf8);
 void resetSubtitles(int id);
 bool ctlSubtitles(int id,int type,unsigned int ctl_id,const void *ctl_data,unsigned int ctl_datalen);

 const char_t *getCurrentFlnm(void) const;

 void addClosedCaption(const char *line),hideClosedCaptions(void);
};


#endif
