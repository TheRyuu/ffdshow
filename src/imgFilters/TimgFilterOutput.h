#ifndef _TIMGFILTEROUTPUT_H_
#define _TIMGFILTEROUTPUT_H_

#include "TimgFilter.h"

struct ToutputVideoSettings;
struct Tlibavcodec;
struct AVCodecContext;
struct AVFrame;
DECLARE_FILTER(TimgFilterOutput, public, TimgFilter)
private:
Tconvert *convert;
Tlibavcodec *libavcodec;
AVCodecContext *avctx;
AVFrame *frame;
TffPict *dvpict;
Tbuffer dvpictbuf;
protected:
int old_cspOptionsRgbInterlaceMode, old_highQualityRGB, old_outputLevelsMode, old_inputLevelsMode, old_IturBt, old_dithering;
virtual uint64_t getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const
{
    return FF_CSPS_MASK;
}
public:
TimgFilterOutput(IffdshowBase *Ideci, Tfilters *Iparent);
virtual ~TimgFilterOutput();
virtual HRESULT process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0)
{
    return E_NOTIMPL;
}
HRESULT process(TffPict &pict, uint64_t dstcsp, unsigned char *dst[4], int dstStride[4], LONG &dstSize, const ToutputVideoSettings *cfg); //S_FALSE = dv

protected:
class TvramBenchmark
{
    // In some combination of video card and video renderer, video RAM access gets very slow if it is accessed randomly.
    // In such case, V-RAM must be accessed sequentially.
    // Here, we do automatic benchmark to detect slow V-RAM.
private:
    static const int BENCHMARK_FRAMES = 5;
    TimgFilterOutput *parent;
    REFERENCE_TIME time_on_convert_direct[BENCHMARK_FRAMES];
    REFERENCE_TIME time_on_convert_indirect[BENCHMARK_FRAMES];
    unsigned int frame_count;
    bool vram_indirect;
    REFERENCE_TIME t1, t2;
public:
    void init(void);
    TvramBenchmark(TimgFilterOutput *Iparent);
    bool get_vram_indirect(void);
    void update(void);
    void onChange(void) {
        if (frame_count > 1) {
            init();
        }
    }
} vramBenchmark;
};

DECLARE_FILTER(TimgFilterOutputConvert, public, TimgFilter)
protected:
virtual uint64_t getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const
{
    return FF_CSPS_MASK;
}
public:
TimgFilterOutputConvert(IffdshowBase *Ideci, Tfilters *Iparent): TimgFilter(Ideci, Iparent) {}
virtual HRESULT process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0);
};

#endif
