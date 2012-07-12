#ifndef _TOSDSETTINGS_H_
#define _TOSDSETTINGS_H_

#include "TfilterSettings.h"
#include "TfontSettings.h"

struct TOSDsettings : TfilterSettingsVideo {
public:
    static const TfilterIDFF idffs;
private:
    TOSDsettings& operator =(const TOSDsettings&);
    mutable bool changed;
public:
    TOSDsettings(size_t IsizeofthisAll, TintStrColl *Icoll = NULL, TfilterIDFFs *filters = NULL);
    const char_t* getFormat(void) const;
    char_t format[1024];
    int isAutoHide;
    int durationVisible;
    int isSave, saveOnly;
    char_t saveFlnm[MAX_PATH];
};

struct TOSDsettingsVideo : TOSDsettings {
public:
    TOSDsettingsVideo(TintStrColl *Icoll = NULL, TfilterIDFFs *filters = NULL);
    virtual void copy(const TfilterSettings *src);
    virtual void reg_op(TregOp &t);
    virtual void createFilters(size_t filtersorder, Tfilters *filters, TfilterQueue &queue) const;
    virtual void createPages(TffdshowPageDec *parent) const;
    void resetLook(void);

    int linespace;
    int posX, posY;
    int userFormat;
    TfontSettingsOSD font; //must be last, the implementation of the method 'copy' depends on that.
};

struct TOSDsettingsAudio : TOSDsettings {
public:
    TOSDsettingsAudio(TintStrColl *Icoll = NULL, TfilterIDFFs *filters = NULL);

    virtual void createFilters(size_t filtersorder, Tfilters *filters, TfilterQueue &queue) const;
    virtual void createPages(TffdshowPageDec *parent) const;
};

#endif
