#ifndef _TIMGFILTERCROP_H_
#define _TIMGFILTERCROP_H_

#include "TimgFilter.h"
#include "TcropSettings.h"
#include "TimgFilterExpand.h"

struct TautoCrop;
DECLARE_FILTER(TimgFilterCrop, public, TimgFilter)
private:
Trect rectCrop, oldRect;
TcropSettings oldSettings;
long lastFrameMS, nextFrameMS;
long autoCropAnalysisDuration;
protected:
virtual uint64_t getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const
{
    return FF_CSPS_MASK&~(FF_CSP_NV12 | FF_CSP_CLJR | FF_CSPS_MASK_HIGH_BIT);
}
virtual void onSizeChange(void);
virtual bool computeAutoCropChange(long oldValue, long *highWaterp, long max, long *newValuep);
static TautoCrop autoCrop; // Can only be static because calcProp is static
public:
TimgFilterCrop(IffdshowBase *Ideci, Tfilters *Iparent);
virtual bool getOutputFmt(TffPictBase &pict, const TfilterSettingsVideo *cfg0);
virtual HRESULT process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0);
static Trect calcCrop(const Trect &pictRect, const TcropSettings *cfg);
virtual Trect calcCrop(const Trect &pictRect, TcropSettings *cfg, TffPict *ppict);
virtual void calcAutoCropVertical(TcropSettings *cfg, const unsigned char *src, unsigned int y0, int stepy, long *autoCrop);
virtual void calcAutoCropHorizontal(TcropSettings *cfg, const unsigned char *src, unsigned int x0, int stepx, long *autoCrop);
};

DECLARE_FILTER(TimgFilterCropExpand, public, TimgFilterExpand)
private:
TcropSettings oldSettings;
protected:
virtual bool is(const TffPictBase &pict, const TfilterSettingsVideo *cfg);
virtual void getDiffXY(const TffPict &pict, const TfilterSettingsVideo *cfg, int &diffx, int &diffy);
public:
TimgFilterCropExpand(IffdshowBase *Ideci, Tfilters *Iparent);
virtual bool getOutputFmt(TffPictBase &pict, const TfilterSettingsVideo *cfg0);
virtual HRESULT process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0);
};

#endif
