#pragma once

#include "TconfPageDecVideo.h"

class ToutcspsPage : public TconfPageDecVideo
{
private:
    static const int idcs[], idffs[];

    void overlay2dlg(), csp2dlg();
    void dlg2primaryCsp();
    virtual INT_PTR msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
    int *backupDV;
public:
    ToutcspsPage(TffdshowPageDec *Iparent);
    ~ToutcspsPage() {
        if (backupDV) {
            free(backupDV);
        }
    }
    virtual void init();
    virtual void cfg2dlg();
    virtual void getTip(char_t *tipS, size_t len);
    virtual void translate();
};
