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
struct Tinput;

struct TframeBuffer {
    int frameNo;
    int bytesPerPixel;
    REFERENCE_TIME start;
    REFERENCE_TIME stop;
    int fieldType;
    Tinput* input;
    AVS_VideoFrame* frame;

    TframeBuffer() :
        input(0),
        frame(0)
    {}

    void CreateFrame(Tinput* input);
    void CreateField(Tinput* input, bool topField);
    void CombineFrame(Tinput* input, bool inTopField, bool outTopField, AVS_VideoFrame* otherField);

    //void CopyFrame(TframeBuffer& frame);
    void ReleaseFrame();
    ~TframeBuffer();
};

struct Tinput : Tavisynth_c {
    unsigned int dx,dy;
    int fpsnum,fpsden;
    uint64_t csp;
    int cspBpp;
    const unsigned char *src[4];
    stride_t *stride1;

    IScriptEnvironment* env;
    PClip* clip;

    int numBuffers;
    TframeBuffer* buffers;

    int curFrame;
    volatile int minAccessedFrame;
    volatile int maxAccessedFrame;
    volatile int numAccessedFrames;
    volatile int accessedFrames[100]; // relative to curFrame
    int backLimit;

    Rational outputDar;
    Rational outputSar;

    void InitVideoInfo(AVS_VideoInfo& vi) {
        memset(&vi,0,sizeof(VideoInfo));
        vi.width=dx;
        vi.height=dy;
        vi.fps_numerator=fpsnum;
        vi.fps_denominator=fpsden;
        vi.num_frames=NUM_FRAMES;
        // RGB values: avisynth refers to the write order, FF_CSP_ enum refers to the "memory byte order",
        // which under x86 is reversed, see the comment above the FF_CSP_ enum definition.
        if      (csp & FF_CSP_420P) {
            vi.pixel_type=AVS_CS_YV12;
        } else if (csp & FF_CSP_YUY2) {
            vi.pixel_type=AVS_CS_YUY2;
        } else if (csp & FF_CSP_RGB32) {
            vi.pixel_type=AVS_CS_BGR32;
        } else if (csp & FF_CSP_RGB24) {
            vi.pixel_type=AVS_CS_BGR24;
        }
    }

    Tinput() : env(NULL),clip(NULL), buffers(NULL) {}

    ~Tinput() {
        if (clip) {
            delete clip;
        }
        if (env) {
            delete env;
        }
    }
};

private:
class Tffdshow_source : Tavisynth_c
{
private:
    VideoInfo &vi;
    Tinput *input;

    Tffdshow_source(Tinput *Iinput,VideoInfo &Ivi);

    static AVS_VideoFrame* AVSC_CC get_frame(AVS_FilterInfo *, int n);
    static int AVSC_CC get_parity(AVS_FilterInfo *, int n) {
        return 0;
    }
    static int AVSC_CC set_cache_hints(AVS_FilterInfo *, int cachehints, int frame_range) {
        return 0;
    }
    static void AVSC_CC free_filter(AVS_FilterInfo *);

public:
    static AVS_Value AVSC_CC Create(AVS_ScriptEnvironment *env, AVS_Value args, void * user_data);
};

class Tffdshow_setAR : Tavisynth_c
{
private:
    Tinput *input;
    bool setDAR;

    static AVS_Value AVSC_CC Create(AVS_ScriptEnvironment *env, AVS_Value args, void * user_data, bool setDAR);

public:
    static AVS_Value AVSC_CC Create_SetSAR(AVS_ScriptEnvironment *env, AVS_Value args, void * user_data);
    static AVS_Value AVSC_CC Create_SetDAR(AVS_ScriptEnvironment *env, AVS_Value args, void * user_data);
};

struct Tavisynth : public Tavisynth_c {
public:
    Tavisynth():
        restart(true),
        passFirstThrough(true),
        buffers(NULL),
        frameScaleDen(1),
        frameScaleNum(1)
    {}

    ~Tavisynth() {
        done();
    }

    void skipAhead(bool passFirstThrough, bool clearLastOutStopTime);
    void done(void);
    bool createClip(const TavisynthSettings *cfg,Tinput *input,TffPictBase& pict);
    void setOutFmt(const TavisynthSettings *cfg,Tinput *input,TffPictBase &pict);
    void init(const TavisynthSettings &oldcfg,Tinput *input,uint64_t *outcsp,TffPictBase &pict);
    HRESULT process(TimgFilterAvisynth *self,TfilterQueue::iterator& it,TffPict &pict,const TavisynthSettings *cfg);
    char infoBuf[1000];

private:
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
    Rational inputSar;

    Trect outputRect;

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

    TframeBuffer* buffers;
} *avisynth;

uint64_t getWantedCsp(const TavisynthSettings *cfg) const;
static const int NUM_FRAMES=10810800; // Divisible by everything up to 18, and by every even number up to 30, and then some.
Tinput* input;
Tinput* outFmtInput;
static int findBuffer(TframeBuffer* buffers, int numBuffers, int n);
uint64_t outcsp;

protected:
//virtual bool is(const TffPictBase &pict,const TfilterSettingsVideo *cfg);
virtual uint64_t getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const;
virtual uint64_t getSupportedOutputColorspaces(const TfilterSettingsVideo *cfg) const;
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
