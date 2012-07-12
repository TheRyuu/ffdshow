#ifndef _COSDPAGE_H_
#define _COSDPAGE_H_

#include "TconfPageDec.h"

class TOSDpageDec : public TconfPageDec
{
private:
    HWND hlv;
    int lvx, lvy;
    ints osds;
    void pos2dlg(void), osds2dlg(void), osd2dlg(void), save2dlg(void);
    void checkOSDline(int idff, bool check);
    static int CALLBACK osdsSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
    void lv2osdFormat(void);
    bool nostate, user;
    strings osdslabels;
    void onLineUp(void), onLineDown(void), onPresets(void), onSave(void);
    int dragitem;
protected:
    virtual INT_PTR msgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
    TOSDpageDec(TffdshowPageDec *Iparent, const TfilterIDFF *idff);
    virtual void init(void);
    virtual void cfg2dlg(void);
    virtual void applySettings(void);
    virtual void translate(void);
};

class TOSDpageVideo : public TOSDpageDec
{
public:
    TOSDpageVideo(TffdshowPageDec *Iparent, const TfilterIDFF *idff);
    virtual bool reset(bool testonly = false);
};

class TOSDpageAudio : public TOSDpageDec
{
public:
    TOSDpageAudio(TffdshowPageDec *Iparent, const TfilterIDFF *idff);
    virtual void init(void);
};

#endif

