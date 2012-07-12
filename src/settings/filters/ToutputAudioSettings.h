#ifndef _TOUTPUTAUDIOSETTINGS_H_
#define _TOUTPUTAUDIOSETTINGS_H_

#include "TfilterSettings.h"

struct ToutputAudioSettings : TfilterSettingsAudio {
private:
    static const TfilterIDFF idffs;
protected:
    const int* getResets(unsigned int pageId);
public:
    ToutputAudioSettings(TintStrColl *Icoll = NULL, TfilterIDFFs *filters = NULL);
    int passthroughAC3;
    int passthroughDTS;
    int passthroughTRUEHD;
    int passthroughDTSHD;
    int passthroughEAC3;
    int passthroughPCMConnection;
    int passthroughDeviceId;
    int useIEC61937;
    int outsfs;
    int outAC3bitrate;
    int connectTo, connectToOnlySpdif;
    static const char_t *connetTos[];
    int outAC3EncodeMode;
    virtual void createFilters(size_t filtersorder, Tfilters *filters, TfilterQueue &queue) const;
    virtual void createPages(TffdshowPageDec *parent) const {}
};

#endif
