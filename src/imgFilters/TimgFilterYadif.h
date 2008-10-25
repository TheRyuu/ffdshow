#ifndef _TIMGFILTERYADIF_H_
#define _TIMGFILTERYADIF_H_

#include "TimgFilter.h"
#include "TdeinterlaceSettings.h"
#include "Tlibmplayer.h"
#include "yadif/mp_image.h"

DECLARE_FILTER(TimgFilterYadif,public,TimgFilter)
private:
 Tlibmplayer *libmplayer;
 TdeinterlaceSettings oldcfg;
 YadifContext* yadctx;
 const TdeinterlaceSettings *cfg;
 TfilterQueue::iterator it;

 virtual bool is(const TffPictBase &pict,const TfilterSettingsVideo *cfg);
 virtual int getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const {return FF_CSP_420P | FF_CSP_YUY2;}
 virtual void onSizeChange(void);

 YadifContext* TimgFilterYadif::getContext(int mode, int parity);
 HRESULT put_image(mp_image_t *mpi, TffPict &pict);
 HRESULT continue_buffered_image(TffPict &pict);
 void store_ref(uint8_t *src[3], int src_stride[3], int width, int height);
 void vf_clone_mpi_attributes(mp_image_t* dst, mp_image_t* src);
 int config(TffPict &pict);

 TffPict oldpict;
public:
 TimgFilterYadif(IffdshowBase *Ideci,Tfilters *Iparent,bool Ibob=false);
 ~TimgFilterYadif();
 virtual void done(void);
 virtual HRESULT process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0);
 virtual void onSeek(void);
 virtual HRESULT onEndOfStream(void);
};

#endif
