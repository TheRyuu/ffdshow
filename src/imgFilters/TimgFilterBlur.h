#ifndef _TIMGFILTERBLUR_H_
#define _TIMGFILTERBLUR_H_

#include "TimgFilter.h"
#include "libpostproc/postprocess_internal.h"

class T3x3blurSWS;
struct Tlibavcodec;
struct TblurSettings;
DECLARE_FILTER(TimgFilterBlur, public, TimgFilter)
private:
unsigned char *bluredPict;
unsigned int bluredStride;
T3x3blurSWS *blur;
void (TimgFilterBlur::*mergeFc)(const TblurSettings *cfg, const unsigned char *srcY, unsigned char *dstY);
template<class _mm> void merge(const TblurSettings *cfg, const unsigned char *srcY, unsigned char *dstY);
protected:
virtual bool is(const TffPictBase &pict, const TfilterSettingsVideo *cfg);
virtual uint64_t getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const
{
    return FF_CSPS_MASK_YUV_PLANAR;
}
public:
TimgFilterBlur(IffdshowBase *Ideci, Tfilters *Iparent);
virtual void onSizeChange(void);
virtual void done(void);
virtual HRESULT process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0);
};

struct SwsContext;
DECLARE_FILTER(TimgFilterAvcodecBlur, public, TimgFilter)
private:
SwsContext *swsc;
int oldradius, oldluma, oldchroma;
Tlibavcodec *libavcodec;
protected:
virtual bool is(const TffPictBase &pict, const TfilterSettingsVideo *cfg);
virtual uint64_t getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const
{
    return SWS_IN_CSPS;
}
virtual uint64_t getSupportedOutputColorspaces(const TfilterSettingsVideo *cfg) const
{
    return SWS_OUT_CSPS;
}
virtual void onSizeChange(void);
public:
TimgFilterAvcodecBlur(IffdshowBase *Ideci, Tfilters *Iparent);
virtual ~TimgFilterAvcodecBlur();
virtual void done(void);
virtual HRESULT process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0);
};

DECLARE_FILTER(TimgFilterAvcodecTNR, public, TimgFilter)
private:
void *pp_ctx;
PPMode pp_mode;
Tlibavcodec *libavcodec;
protected:
virtual bool is(const TffPictBase &pict, const TfilterSettingsVideo *cfg);
virtual uint64_t getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const
{
    return FF_CSPS_MASK_YUV_PLANAR;
}
virtual void onSizeChange(void);
public:
TimgFilterAvcodecTNR(IffdshowBase *Ideci, Tfilters *Iparent);
virtual ~TimgFilterAvcodecTNR();
virtual void done(void);
virtual HRESULT process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0);
virtual void onSeek(void);
};

#endif
