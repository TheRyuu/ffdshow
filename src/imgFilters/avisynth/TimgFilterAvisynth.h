#ifndef _TIMGFILTERAVISYNTH_H_
#define _TIMGFILTERAVISYNTH_H_

#include "TimgFilter.h"
#include "Tavisynth.h"
#include "TavisynthSettings.h"

class Tdll;
DECLARE_FILTER(TimgFilterAvisynth,public,TimgFilter)
private:
 TavisynthSettings oldcfg;

 class Tffdshow_source : Tavisynth_c
  {
  public:
   struct Tinput
    {
     unsigned int dx,dy;
     int fpsnum,fpsden;
     int csp,cspBpp;
     const unsigned char *src[4];
     stride_t *stride1;
    };
  private:
   IScriptEnvironment *env;
   VideoInfo &vi;
   const Tinput *input;
   Tffdshow_source(const Tinput *Iinput,VideoInfo &Ivi,IScriptEnvironment *Ienv);
   static AVS_VideoFrame* AVSC_CC get_frame(AVS_FilterInfo *, int n);
   static int AVSC_CC get_parity(AVS_FilterInfo *, int n) {return 0;}
   static int AVSC_CC set_cache_hints(AVS_FilterInfo *, int cachehints, int frame_range) {return 0;}
   static void AVSC_CC free_filter(AVS_FilterInfo *);
  public:
   typedef std::pair<IScriptEnvironment*,const Tffdshow_source::Tinput*> Tc_createStruct;
   static AVS_Value AVSC_CC Create(AVS_ScriptEnvironment *, AVS_Value args, void * user_data);
  };

 struct Tavisynth : public Tavisynth_c
  {
  public:
   Tavisynth():env(NULL),clip(NULL),isFirstError(true) {}
   ~Tavisynth() {done();}
   void done(void);
   PClip* createClip(const TavisynthSettings *cfg,const Tffdshow_source::Tinput *input);
   typedef Tavisynth_c::PClip PClip;
   void setOutFmt(const TavisynthSettings *cfg,const Tffdshow_source::Tinput *input,TffPictBase &pict);
   void init(const TavisynthSettings &oldcfg,const Tffdshow_source::Tinput &input,int *outcsp);
   void process(TimgFilterAvisynth *self,TffPict &pict,const TavisynthSettings *cfg);
   PClip *clip;

  private:
   Trect outrect;
   REFERENCE_TIME fpsscaleNum,fpsscaleDen;
   IScriptEnvironment *env;
   bool isFirstError;
  } *avisynth;

 int getWantedCsp(const TavisynthSettings *cfg) const;
 static const int NUM_FRAMES=1078920;
 Tffdshow_source::Tinput input;
 int outcsp;
protected:
 virtual bool is(const TffPictBase &pict,const TfilterSettingsVideo *cfg);
 virtual int getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const;
 virtual int getSupportedOutputColorspaces(const TfilterSettingsVideo *cfg) const;
 virtual void onSizeChange(void);
public:
 TimgFilterAvisynth(IffdshowBase *Ideci,Tfilters *Iparent);
 virtual ~TimgFilterAvisynth();
 virtual void done(void);
 virtual bool getOutputFmt(TffPictBase &pict,const TfilterSettingsVideo *cfg0);
 virtual HRESULT process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0);
};

#endif
