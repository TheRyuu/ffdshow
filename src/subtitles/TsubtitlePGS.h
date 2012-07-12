#pragma once

#include "Tsubtitle.h"
#include "autoptr.h"
#include "Crect.h"
#include "TffRect.h"
#include "TimgFilter.h"
#include "TsubtitleDVD.h"
#include "TsubtitlePGSParser.h"


struct TsubtitlePGS : public TsubtitleDVD {
    TsubtitlePGS(IffdshowBase *Ideci, REFERENCE_TIME Istart, REFERENCE_TIME Istop, TcompositionObject *pCompositionObject,
                 TwindowDefinition *IpWindow, TsubtitleDVDparent *Iparent);
    virtual ~TsubtitlePGS();
    virtual void print(
        REFERENCE_TIME time,
        bool wasseek,
        Tfont &f,
        bool forceChange,
        TprintPrefs &prefs,
        unsigned char **dst,
        const stride_t *stride);

    virtual void updateTimestamps(void);
    virtual Tsubtitle* copy(void) {
        return new TsubtitlePGS(*this);
    }
    Tconvert *convert;
    IffdshowBase *deci;
    TcompositionObject *m_pCompositionObject;
    TwindowDefinition *m_pWindow;
    int videoWidth, videoHeight;
};
