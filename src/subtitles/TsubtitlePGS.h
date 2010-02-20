#ifndef _TSUBTITLEPGS_H_
#define _TSUBTITLEPGS_H_

#include "Tsubtitle.h"
#include "autoptr.h"
#include "Crect.h"
#include "TffRect.h"
#include "TimgFilter.h"
#include "TsubtitleDVD.h"
#include "TsubtitlePGSParser.h"


struct TsubtitlePGS :public TsubtitleDVD 
{
 TsubtitlePGS(IffdshowBase *Ideci,REFERENCE_TIME Istart, REFERENCE_TIME Istop, TcompositionObject *pCompositionObject, TsubtitleDVDparent *Iparent);
 virtual ~TsubtitlePGS();
 virtual void readContext(void);
 virtual void print(
    REFERENCE_TIME time,
    bool wasseek,
    Tfont &f,
    bool forceChange,
    TprintPrefs &prefs,
    unsigned char **dst,
    const stride_t *stride);

 virtual Tsubtitle* copy(void) {return new TsubtitlePGS(*this);}
 Trect rect;
 CPoint centerPoint;
 CSize size;
 stride_t stride;
 TffPict *bitmap;
 Tconvert *convert;
 IffdshowBase *deci;
 TspuImage *ownimage;
 TcompositionObject *m_pCompositionObject;
 int videoWidth, videoHeight;
};

#endif
