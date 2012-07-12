#ifndef _TIMGFILTERRESIZE_H_
#define _TIMGFILTERRESIZE_H_

#include "TimgFilter.h"
#include "TresizeAspectSettings.h"
#include "libswscale/swscale.h"

class SimpleResize;
struct Tlibavcodec;
DECLARE_FILTER(TimgFilterResize, public, TimgFilter)
private:
bool sizeChanged, inited;
TresizeAspectSettings oldSettings;
uint64_t oldcsp;
TffPict newpict;

bool oldinterlace, oldWarped;

Tlibavcodec *libavcodec;
SwsContext *swsc;
SwsFilter *swsf;
SwsParams *swsparams;

SimpleResize *simple;

unsigned int dxnone, xdif1none, xdif2none;
unsigned int dynone, ydif1none, ydif2none;
protected:
virtual uint64_t getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const;
virtual uint64_t getSupportedOutputColorspaces(const TfilterSettingsVideo *cfg) const;
virtual void onSizeChange(void);
virtual bool is(const TffPictBase &pict, const TfilterSettingsVideo *cfg);
public:
TimgFilterResize(IffdshowBase *Ideci, Tfilters *Iparent);
virtual ~TimgFilterResize();
virtual void done(void);
virtual bool getOutputFmt(TffPictBase &pict, const TfilterSettingsVideo *cfg0);
virtual HRESULT process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0);
};

#endif
