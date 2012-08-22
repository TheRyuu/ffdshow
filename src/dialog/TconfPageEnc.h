#ifndef _TCONFPAGEENC_H_
#define _TCONFPAGEENC_H_

#include "TconfPageBase.h"
#include "TffdshowPageEnc.h"
#include "IffdshowEnc.h"
#include "ffcodecs.h"

struct TglobalSettingsEnc;
struct IffdshowEnc;
class TconfPageEnc : public TconfPageBase
{
protected:
    comptrQ<IffdshowEnc> deciE;
    TffdshowPageEnc *parent;
    AVCodecID &codecId;
public:
    TconfPageEnc(TffdshowPageEnc *Iparent): TconfPageBase(Iparent), codecId(Iparent->codecId) {
        parent = Iparent;
        deciE = deci;
    }
};

#endif
