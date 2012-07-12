#ifndef _TPRESETSETTINGSVIDEO_H_
#define _TPRESETSETTINGSVIDEO_H_

#include "TpresetSettings.h"

struct TvideoAutoPresetProps : TautoPresetProps {
private:
    comptrQ<IffdshowDecVideo> deciV;
    bool wasResolution;
    unsigned int dx, dy;
    char_t fourcc[5];
    char_t previousfourcc[5];
    double SAR, DAR;
    double fps;
    static const char_t aspectSAR, aspectDAR;
public:
    TvideoAutoPresetProps(IffdshowBase *Ideci);
    virtual void getSourceResolution(unsigned int *dx, unsigned int *dy);
    const char_t* getFOURCC(void);
    const char_t* getPreviousFOURCC(void);
    static const char_t* getFOURCCitem(IffdshowDec *deciD, unsigned int index);
    const char_t *getSAR(void), *getDAR(void);
    const char_t *getFps(void);
    bool aspectMatch(const char_t *mask, const char_t *flnm);
    bool fpsMatch(const char_t *mask, const char_t *flnm);
};

struct TpostprocSettings;
struct TsubtitlesSettings;
struct TlevelsSettings;
struct TvisSettings;
struct TgrabSettings;
struct ToutputVideoSettings;
struct TresizeAspectSettings;
struct TdeinterlaceSettings;
struct TQSSettings;

struct TpresetVideo : public Tpreset {
private:
    int needOutcspsFix, needGlobalFix;
protected:
    virtual void reg_op(TregOp &t);
    virtual int getDefault(int id);
public:
    virtual Tpreset& operator =(const Tpreset &src);
    TpresetVideo(const char_t *Ireg_child, const char_t *IpresetName, int filtermode);
    virtual ~TpresetVideo() {}

    virtual Tpreset* copy(void) const {
        return new_copy(this);
    }
    virtual void loadReg(void);

    int autoloadSize, autoloadSizeXmin, autoloadSizeXmax, autoloadSizeCond, autoloadSizeYmin, autoloadSizeYmax;
    virtual bool autoloadSizeMatch(int AVIdx, int AVIdy) const;
    virtual bool is_autoloadSize(void) const {
        return !!autoloadSize;
    }

    int videoDelay, isVideoDelayEnd, videoDelayEnd;
    int idct;
    int softTelecine;
    int workaroundBugs;
    int lavcDecThreads;
    int grayscale;
    int multiThread;
    int dontQueueInWMP, useQueueOnlyIn, queueCount, queueVMR9YV12;
    int dropOnDelay, dropDelayTime, dropDelayTimeReal;
    int h264skipOnDelay, h264skipDelayTime;
    char_t useQueueOnlyInList[256];

    int isDyInterlaced, dyInterlaced;
    int bordersBrightness;
    int dec_dxva_h264;
    int dec_dxva_vc1;
    int dec_dxva_compatibilityMode;
    int dec_dxva_postProcessingMode;

    // QuickSync params:
    int qs_enable_ts_corr, qs_enable_mt, qs_field_order, qs_enable_sw_emulation, qs_force_field_order,
        qs_enable_dvd_decode, qs_enable_di, qs_force_di, qs_enable_full_rate, qs_detail,
        qs_denoise;

    TpostprocSettings *postproc;
    TsubtitlesSettings *subtitles;
    TlevelsSettings *levels;
    TresizeAspectSettings *resize;
    TvisSettings *vis;
    TdeinterlaceSettings *deinterlace;
    TgrabSettings *grab;
    ToutputVideoSettings *output;
};

class TpresetVideoPlayer : public TpresetVideo
{
public:
    virtual Tpreset& operator =(const Tpreset &src) {
        TpresetVideo::operator =(src);
        return *this;
    }
    TpresetVideoPlayer(const char_t *Ireg_child, const char_t *IpresetName, int filtermode);
    virtual Tpreset* copy(void) const {
        return new_copy(this);
    }
};

#endif
