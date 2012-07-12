#ifndef _TIMGFILTERYADIF_H_
#define _TIMGFILTERYADIF_H_

#include "TimgFilter.h"
#include "TdeinterlaceSettings.h"
#include "Tlibavcodec.h"

DECLARE_FILTER(TimgFilterYadif, public, TimgFilter)
private:
Tlibavcodec *libavcodec;
TdeinterlaceSettings oldcfg;
YADIFContext* yadctx;
const TdeinterlaceSettings *cfg;
TfilterQueue::iterator it;
bool hasImageInBuffer;
REFERENCE_TIME last_rtStop;
bool dllok;

virtual bool is(const TffPictBase &pict, const TfilterSettingsVideo *cfg);
virtual uint64_t getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const
{
    return FF_CSP_420P | FF_CSP_422P;
}
virtual void onSizeChange(void);
HRESULT onDiscontinuity(const TffPict &pict);

YADIFContext* getContext(int mode, int parity);
HRESULT put_image(TffPict &pict, const unsigned char *src[4], int full);
void store_ref(const uint8_t *src[3], stride_t src_stride[3], int width, int height);
int config(TffPict &pict);

TffPict oldpict;

public:
TimgFilterYadif(IffdshowBase *Ideci, Tfilters *Iparent, bool Ibob = false);
~TimgFilterYadif();
virtual void done(void);
virtual HRESULT process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0);
virtual void onSeek(void);
virtual HRESULT onEndOfStream(void);
virtual bool onPullImageFromSubtitlesFilter(void);
};

#endif
