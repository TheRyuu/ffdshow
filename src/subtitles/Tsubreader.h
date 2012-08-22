#pragma once

#include "Tsubtitle.h"
#include "TsubtitleText.h"
#include "Tstream.h"

#ifndef SAFE_FREE
#define SAFE_FREE(x) {if (x) free(x); (x) = 0;} /* helper macro */
#endif

struct TsubtitlesSettings;
struct Tsubreader : public std::vector<Tsubtitle*> {
private:
    static bool subComp(const Tsubtitle *s1, const Tsubtitle *s2);
    void timesort(void);
protected:
    void processDuration(const TsubtitlesSettings *cfg);
    std::vector<TsubtitleTexts> processedSubtitleTexts;
public:
    virtual void getSubtitle(const TsubtitlesSettings *cfg, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, bool *forceChange = NULL) {};
    void push_back(Tsubtitle* const &sub) {
        IsProcessOverlapDone = false;
        std::vector<Tsubtitle*>::push_back(sub);
    }

    enum SUB_FORMAT {
        SUB_INVALID   = 0,

        // text
        SUB_MICRODVD  = 1,
        SUB_SUBRIP    = 2,
        SUB_SUBVIEWER = 3,
        SUB_SAMI      = 4,
        SUB_VPLAYER   = 5,
        SUB_RT        = 6,
        SUB_SSA       = 7,
        SUB_DUNNOWHAT = 8, // FIXME what format is it ?
        SUB_MPSUB     = 9,
        SUB_AQTITLE   = 10,
        SUB_SUBVIEWER2 = 11,
        SUB_SUBRIP09  = 12,

        // bitmap
        SUB_VOBSUB    = 14,
        SUB_DVD       = 15,
        SUB_CVD       = 16,
        SUB_SVCD      = 17,

        // text
        SUB_MPL2      = 18,

        //BluRay, HDDVD subtitles (PNG)
        SUB_PGS       = 19,

        SUB_USESTIME  = 1024,
        SUB_FORMATMASK = SUB_USESTIME - 1,
        SUB_ENC_UTF8  = 2048,
        SUB_ENC_UNILE = 4096,
        SUB_ENC_UNIBE = 8192,
        SUB_ENC_MASK  = SUB_ENC_UTF8 + SUB_ENC_UNILE + SUB_ENC_UNIBE,
        SUB_KEEP_FILE_OPENED = 16384
    };
    virtual int getFormat(void) {
        return SUB_INVALID;
    }
    static bool isBitmapsub(int format) {
        format &= SUB_FORMATMASK;
        return (isDVDsub(format) || format == SUB_PGS);
    }
    static bool isDVDsub(int format) {
        format &= SUB_FORMATMASK;
        return format == SUB_DVD || format == SUB_CVD || format == SUB_SVCD;
    }
    static bool isText(int format) {
        format &= SUB_FORMATMASK;
        if (format >= SUB_MICRODVD && format <= SUB_SUBRIP09) {
            return true;
        }
        if (format == SUB_MPL2) {
            return true;
        }
        return false;
    }
    static Tstream::ENCODING getSubEnc(int format);
    static void setSubEnc(int &format, const Tstream &fs);
    static int sub_autodetect(Tstream &fd, const Tconfig *config);
    Tsubreader(void): langid(-1), IsProcessOverlapDone(false) {}
    virtual ~Tsubreader();
    virtual void clear();
    int langid;
    virtual void setLang(int langid) {};
    virtual int findlang(int langname) {
        return 0;
    }
    void adjust_subs_time(float subtime);
    void processOverlap(void);
    bool IsProcessOverlapDone;
    Tsubtitle* operator[](size_t pos) const;
    size_t count() const; // overridding size makes trouble as some calls to size() in this class expect the size of the base class.
    virtual void onSeek();
    void dropRendered();
    size_t getMemorySize() const;
};
