#pragma once

#include "Tsubreader.h"
#include "TsubtitleText.h"
#include "TsubreaderMplayer.h"
#include "TsubtitlePGS.h"
#include "TsubtitlePGSParser.h"

//struct Tstream;

class TsubreaderPGS : public Tsubreader, public TsubtitleDVDparent
{
private:
    static const size_t MAX_LEN = 4194304; // Maximal length of a PGS segment : 4Mb (this is random, I don't know the real limit)
    uint8_t data[MAX_LEN];
    const Tconfig *ffcfg;
    TsubtitlesSettings cfg;
    Tstream *pStream;
    IffdshowBase *deci;
    TsubtitlePGSParser *pSubtitlePGSParser;
    REFERENCE_TIME rtPos;
public:
    TsubreaderPGS(IffdshowBase *Ideci, Tstream &Ifd, double Ifps, const TsubtitlesSettings *Icfg, const Tconfig *Iffcfg);
    virtual ~TsubreaderPGS();
    virtual void onSeek();
    virtual void parse(int flags = TsubtitleParser::PARSETIME, REFERENCE_TIME rtStart = REFTIME_INVALID, REFERENCE_TIME rtStop = REFTIME_INVALID);
    virtual void getSubtitle(const TsubtitlesSettings *cfg, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, bool *forceChange = NULL);
    virtual int getFormat() {
        return SUB_PGS | SUB_KEEP_FILE_OPENED;
    };
};
