#ifndef _TIMGFILTERAVISYNTH_H_
#define _TIMGFILTERAVISYNTH_H_

#include "TimgFilter.h"
#include "Tavisynth.h"
#include "TavisynthSettings.h"

class Tdll;
DECLARE_FILTER(TimgFilterAvisynth,public,TimgFilter)
 friend struct TavisynthSettings;

private:
 TavisynthSettings oldcfg;

public:
 struct TframeBuffer
  {
   int frameNo;
   int bytesPerPixel;
   REFERENCE_TIME start;
   REFERENCE_TIME stop;
   int fieldType;
   unsigned char* data[4];
   stride_t pitch[4];
   int width[4];
   int height[4];
  };

private:
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

     int numBuffers;
     TframeBuffer* buffers;

     int curFrame;
     int minAccessedFrame;
     int maxAccessedFrame;
     int numAccessedFrames;
     int accessedFrames[100]; // relative to curFrame
     int backLimit;
    };

  private:
   IScriptEnvironment *env;
   VideoInfo &vi;
   Tinput *input;
   Tffdshow_source(Tinput *Iinput,VideoInfo &Ivi,IScriptEnvironment *Ienv);
   static AVS_VideoFrame* AVSC_CC get_frame(AVS_FilterInfo *, int n);
   static int AVSC_CC get_parity(AVS_FilterInfo *, int n) {return 0;}
   static int AVSC_CC set_cache_hints(AVS_FilterInfo *, int cachehints, int frame_range) {return 0;}
   static void AVSC_CC free_filter(AVS_FilterInfo *);

  public:
   typedef std::pair<IScriptEnvironment*,Tffdshow_source::Tinput*> Tc_createStruct;
   static AVS_Value AVSC_CC Create(AVS_ScriptEnvironment *, AVS_Value args, void * user_data);
  };

 struct Tavisynth : public Tavisynth_c
  {
  public:
   Tavisynth():env(NULL),clip(NULL),restart(true),passFirstThrough(true),bufferData(NULL),buffers(NULL),frameScaleDen(1),frameScaleNum(1) {}
   ~Tavisynth() {done();}
   void skipAhead(bool passFirstThrough, bool clearLastOutStopTime);
   void done(void);
   PClip* createClip(const TavisynthSettings *cfg,Tffdshow_source::Tinput *input,TffPictBase& pict);
   typedef Tavisynth_c::PClip PClip;
   void setOutFmt(const TavisynthSettings *cfg,Tffdshow_source::Tinput *input,TffPictBase &pict);
   void init(const TavisynthSettings &oldcfg,Tffdshow_source::Tinput &input,int *outcsp,TffPictBase &pict);
   void process(TimgFilterAvisynth *self,TfilterQueue::iterator& it,TffPict &pict,const TavisynthSettings *cfg);
   PClip *clip;
   char infoBuf[1000];

  private:
   Trect outrect;
   IScriptEnvironment *env;

   int minAccessedFrame;
   int maxAccessedFrame;

   int curInFrameNo;
   int curOutFrameNo;
   int curOutScaledFrameNo;

   REFERENCE_TIME lastOutStopTime;

   REFERENCE_TIME frameScaleNum;
   REFERENCE_TIME frameScaleDen;

   Trect inputRect;
   Rational inputDar;

   Trect outputRect;
   Rational outputDar;

   bool enableBuffering;
   int bufferAhead;
   int bufferBack;

   int applyPulldown;
   bool hasPulldown;

   int numBuffers;

   int buffersFilled;
   int buffersNeeded;
   int curBufferNo;
   int backLimit;

   bool passFirstThrough;
   bool passLastThrough;
   bool restart;
   bool deleteBuffers;
   bool resetBuffers;
   bool ignoreAheadValue;

   unsigned char* bufferData;
   TframeBuffer* buffers;
  } *avisynth;

 int getWantedCsp(const TavisynthSettings *cfg) const;
 static const int NUM_FRAMES=10810800; // Divisible by everything up to 18, and by every even number up to 30, and then some.
 Tffdshow_source::Tinput input;
 static int findBuffer(TframeBuffer* buffers, int numBuffers, int n);
 int outcsp;

protected:
 //virtual bool is(const TffPictBase &pict,const TfilterSettingsVideo *cfg);
 virtual int getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const;
 virtual int getSupportedOutputColorspaces(const TfilterSettingsVideo *cfg) const;
 virtual void onSizeChange(void);
 virtual void onSeek(void);
 virtual void onStop(void);
 virtual void onFlush(void);
 void reset(void);

public:
 TimgFilterAvisynth(IffdshowBase *Ideci,Tfilters *Iparent);
 virtual ~TimgFilterAvisynth();
 virtual void done(void);
 virtual bool getOutputFmt(TffPictBase &pict,const TfilterSettingsVideo *cfg0);
 const char* getInfoBuffer(void);
 virtual HRESULT process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0);
 static int getMaxBufferAhead(void);
 static int getMaxBufferBack(void);
};

#endif
