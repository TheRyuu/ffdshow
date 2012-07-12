#ifndef _CSUBTITLESPOSPAGE_H_
#define _CSUBTITLESPOSPAGE_H_

#include "TconfPageDecVideo.h"

class TsubtitlesPosPage : public TconfPageDecVideo
{
private:
    void expand2dlg(void);
    void stereo2dlg(void);
    void onExpandClick(void);
protected:
    virtual INT_PTR msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
    TsubtitlesPosPage(TffdshowPageDec *Iparent, const TfilterIDFF *idff);
    virtual void init(void);
    virtual void cfg2dlg(void);
    virtual void translate(void);
};

#endif

