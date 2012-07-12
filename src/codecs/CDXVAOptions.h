#ifndef _CDXVAOPTIONSPAGE_H_
#define _CDXVAOPTIONSPAGE_H_

#include "TconfPageDecVideo.h"

class TDXVAOptionsPage : public TconfPageDecVideo
{
private:
    bool islavc;
protected:
    virtual INT_PTR msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
    TDXVAOptionsPage(TffdshowPageDec *Iparent);
    virtual void init(void);
    virtual void cfg2dlg(void);
    virtual void translate(void);
};

#endif
