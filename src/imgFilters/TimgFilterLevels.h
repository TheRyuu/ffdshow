#ifndef _TIMGFILTERLEVELS_H_
#define _TIMGFILTERLEVELS_H_

#include "TimgFilter.h"
#include "TlevelsSettings.h"
#include "IimgFilterLevels.h"
#include "TPerformanceCounter.h"

class TimgFilterLevels : public TimgFilter, public IimgFilterLevels
    _DECLARE_FILTER(TimgFilterLevels, TimgFilter)
    private:
        TlevelsSettings oldSettings;
TPerformanceCounter timer;
int flag_resetHistory, oldMode;
static const int HISTORY = 32;
int inMins[HISTORY], inMinSum, inMin, inMaxs[HISTORY], inMaxSum, inMax;
unsigned int minMaxPos;
void resetHistory(void);
unsigned int map[256];
int mapc[256];
unsigned int histogram[256];
CCritSec csHistogram;
template <uint64_t incsp> void filter(const uint8_t *srcY, const uint8_t *srcU, const uint8_t *srcV,
                                      uint8_t *dstY, uint8_t *dstU, uint8_t *dstV);
void filterRGB32(const uint8_t *srcY, const uint8_t *srcU, const uint8_t *srcV,
                 uint8_t *dstY, uint8_t *dstU, uint8_t *dstV);
inline uint8_t TimgFilterLevels::getuv(int u, int lumav);

protected:
virtual bool is(const TffPictBase &pict, const TfilterSettingsVideo *cfg);
static const int supportedcsps = FF_CSP_RGB32 | FF_CSP_420P | FF_CSP_422P | FF_CSP_444P;
virtual uint64_t getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const
{
    const TlevelsSettings *cfg1 = dynamic_cast<const TlevelsSettings*>(cfg);
    if (cfg1) {
        if (cfg1->forceRGB) {
            return FF_CSP_RGB32;
        }
    }
    return supportedcsps;
}
public:
TimgFilterLevels(IffdshowBase *Ideci, Tfilters *Iparent);
virtual HRESULT process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0);
virtual void onSeek(void);

virtual HRESULT queryInterface(const IID &iid, void **ptr) const;
STDMETHODIMP getHistogram(unsigned int dst[256]);
STDMETHODIMP getInAuto(int *min, int *max);
};

#endif
