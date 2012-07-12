#ifndef _CSUBTITLESTEXTPAGE_H_
#define _CSUBTITLESTEXTPAGE_H_

#include "TconfPageDecVideo.h"

class TsubtitlesTextPage : public TconfPageDecVideo
{
private:
    void linespacing2dlg(), min2dlg(), split2dlg();
    void memory2dlg();
protected:
    virtual INT_PTR msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
    TsubtitlesTextPage(TffdshowPageDec *Iparent, const TfilterIDFF *idff);
    virtual void init();
    virtual void cfg2dlg();
    virtual void translate();
};

#endif

