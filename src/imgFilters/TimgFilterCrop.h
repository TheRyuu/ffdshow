#ifndef _TIMGFILTERCROP_H_
#define _TIMGFILTERCROP_H_

#include "TimgFilter.h"
#include "TcropSettings.h"
#include "TimgFilterExpand.h"

DECLARE_FILTER(TimgFilterCrop,public,TimgFilter)
private:
 Trect rectCrop,oldRect;
 TcropSettings oldSettings;
protected:
 virtual int getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const {return FF_CSPS_MASK&~(FF_CSP_NV12|FF_CSP_CLJR);}
 virtual void onSizeChange(void);
public:
 TimgFilterCrop(IffdshowBase *Ideci,Tfilters *Iparent);
 virtual bool getOutputFmt(TffPictBase &pict,const TfilterSettingsVideo *cfg0);
 virtual HRESULT process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0);
 static Trect calcCrop(const Trect &pictRect,const TcropSettings *cfg);
};

DECLARE_FILTER(TimgFilterCropExpand,public,TimgFilterExpand)
private:
 TcropSettings oldSettings;
protected:
 virtual bool is(const TffPictBase &pict,const TfilterSettingsVideo *cfg);
 virtual void getDiffXY(const TffPict &pict,const TfilterSettingsVideo *cfg,int &diffx,int &diffy);
public:
 TimgFilterCropExpand(IffdshowBase *Ideci,Tfilters *Iparent);
 virtual bool getOutputFmt(TffPictBase &pict,const TfilterSettingsVideo *cfg0);
 virtual HRESULT process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0);
};

#endif
