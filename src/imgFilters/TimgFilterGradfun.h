#ifndef _TIMGFILTERGRADFUN_H_
#define _TIMGFILTERGRADFUN_H_

#include "TimgFilter.h"
#include "Tlibavcodec.h"
#include "libavfilter/gradfun.h"

DECLARE_FILTER(TimgFilterGradfun,public,TimgFilter)

private:
Tlibavcodec *ffmpeg;
GradFunContext *gradFunContext;
bool dllok;
int oldThreshold;
int oldRadius;
unsigned int oldSizeX;
unsigned int oldSizeY;
int reconfigure;

virtual GradFunContext *configure (float threshold, int radius, TffPict &pict);
virtual void filter (GradFunContext *gradFunContext, TffPict &pict);

protected:
virtual int getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const
{
    return FF_CSP_420P;
}

public:
TimgFilterGradfun(IffdshowBase *Ideci,Tfilters *Iparent);
TimgFilterGradfun::~TimgFilterGradfun();
virtual HRESULT process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0);
virtual void onSizeChange(void);
virtual void done(void);
};

#endif
