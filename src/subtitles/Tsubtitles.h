#pragma once

#include "interfaces.h"
#include "Tsubreader.h"

struct Tsubtitle;
struct TsubtitlesSettings;
struct Tconfig;
class Tsubtitles : public safe_bool<Tsubtitles>
{
private:
    Tsubtitle *oldsub;
    unsigned int current_sub;
    REFERENCE_TIME nosub_range_start, nosub_range_end;
protected:
    IffdshowBase *deci;
    const Tconfig *ffcfg;
    Tsubreader *subs;
    virtual void checkChange(const TsubtitlesSettings *cfg, bool *forceChange) {}
public:
    int sub_format;

    void init(void);
    Tsubtitles(IffdshowBase *Ideci);
    virtual ~Tsubtitles();
    virtual void done(void);
    bool boolean_test() const {
        return subs != NULL;
    }
    virtual Tsubtitle* getSubtitle(const TsubtitlesSettings *cfg, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, bool *forceChange = NULL);
    void setModified(void) {
        subs->IsProcessOverlapDone = false;
    };
    bool IsProcessOverlapDone(void) {
        return subs->IsProcessOverlapDone;
    };
    void processOverlap(void);
    void onSeek(void) {
        if (subs) {
            subs->onSeek();
        }
    }
    bool isText() {
        if (!subs) {
            return false;
        }
        return subs->isText(sub_format);
    }
    friend class TimgFilterSubtitles; // let TimgFilterSubtitles take care of back ground rendering
};
