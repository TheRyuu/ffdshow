#pragma once

#include "TconfPageDecVideo.h"

class TQSdecoderOptionsPage : public TconfPageDecVideo
{
public:
    TQSdecoderOptionsPage(TffdshowPageDec *Iparent);
    virtual void init(void);
    virtual void cfg2dlg(void);
    virtual void detail2dlg(void);
    virtual void denoise2dlg(void);
    virtual void getTip(char_t *tipS, size_t len);
    virtual bool reset(bool testonly = false);
    virtual void translate(void);
};
