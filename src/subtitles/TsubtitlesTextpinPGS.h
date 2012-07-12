#pragma once

#include "TsubtitlesTextpin.h"
#include "TsubtitleDVD.h"
#include "TsubtitlePGSParser.h"

class TsubtitlesTextpinPGS : public TsubtitlesTextpin, public TsubtitleDVDparent
{
protected:
    TsubtitlePGSParser *pSubtitlePGSParser;
    TcompositionObjects m_compositionObjects;
    struct Tsubtitles : public Tsubtitle, public std::vector<Tsubtitle*> {
        virtual void print(REFERENCE_TIME time,
                           bool wasseek,
                           Tfont &f,
                           bool forceChange,
                           TprintPrefs &prefs,
                           unsigned char **dst,
                           const stride_t *stride);
        virtual Tsubtitle* copy(void);
        virtual Tsubtitle* create(void) {
            return new Tsubtitles;
        }
    } subtitles;
public:
    TsubtitlesTextpinPGS(int Itype, IffdshowBase *Ideci);
    virtual ~TsubtitlesTextpinPGS(void);
    virtual bool ctlSubtitles(unsigned int id, const void *data, unsigned int datalen);
    virtual void addSubtitle(REFERENCE_TIME start, REFERENCE_TIME stop, const unsigned char *data, unsigned int datalen, const TsubtitlesSettings *cfg, bool utf8);
    virtual void resetSubtitles(void);
    virtual Tsubtitle* getSubtitle(const TsubtitlesSettings *cfg, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, bool *forceChange = NULL);
};
