#pragma once

#include "nmTextSubtitles.h"
#include "Tsubreader.h"
#include "TsubtitleText.h"
#include "rational.h"
#include "TSSAstyles.h"

class TsubtitleParserBase
{
private:
protected:
    static const int LINE_LEN = MAX_SUBTITLE_LENGTH; // Maximum length of subtitle lines
    int format;
    double fps;
    REFERENCE_TIME frameToTime(int frame) {
        return hmsToTime(0, 0, int(frame / fps), int(100 * frame / fps) % 100);
    }
    static REFERENCE_TIME hmsToTime(int h, int m, int s, int hunsec = 0) {
        return REF_SECOND_MULT * h * 60 * 60 + REF_SECOND_MULT * m * 60 + REF_SECOND_MULT * s + REF_SECOND_MULT * hunsec / 100;
    }
public:
    static TsubtitleParserBase* getParser(int format, double fps, const TsubtitlesSettings *cfg, const Tconfig *ffcfg, Tsubreader *Isubreader, bool utf8, bool isEmbedded);
    TsubtitleParserBase(int Iformat, double Ifps): format(Iformat), fps(Ifps) {}
    enum FLAGS {PARSETIME = 1, SSA_NODIALOGUE = 2};
    virtual Tsubtitle* parse(Tstream &fd, int flags = PARSETIME, REFERENCE_TIME start = REFTIME_INVALID, REFERENCE_TIME stop = REFTIME_INVALID) = 0;
};

class TtextFix;
class TsubtitleParser : public TsubtitleParserBase
{
protected:
    Tsubreader *subreader;
    TsubtitlesSettings cfg;
    const Tconfig *ffcfg;

    TsubtitleFormat textformat;
    TsubtitleParser(int Iformat, double Ifps, const TsubtitlesSettings *Icfg, const Tconfig *Iffcfg, Tsubreader *Isubreader);
    static const wchar_t* sub_readtext(const wchar_t *source, TsubtitleText &sub);
    Tsubtitle* store(TsubtitleText &sub);
    static int eol(wchar_t p);
    static void trail_space(wchar_t *s);
    int lineID;
public:
    virtual ~TsubtitleParser() {}
};

class TsubtitleParserMicrodvd : public TsubtitleParser
{
public:
    TsubtitleParserMicrodvd(int Iformat, double Ifps, const TsubtitlesSettings *Icfg, const Tconfig *Iffcfg, Tsubreader *Isubreader): TsubtitleParser(Iformat, Ifps, Icfg, Iffcfg, Isubreader) {}
    virtual Tsubtitle* parse(Tstream &fd, int flags = TsubtitleParser::PARSETIME, REFERENCE_TIME start = REFTIME_INVALID, REFERENCE_TIME stop = REFTIME_INVALID);
};
class TsubtitleParserSubrip : public TsubtitleParser
{
public:
    TsubtitleParserSubrip(int Iformat, double Ifps, const TsubtitlesSettings *Icfg, const Tconfig *Iffcfg, Tsubreader *Isubreader): TsubtitleParser(Iformat, Ifps, Icfg, Iffcfg, Isubreader) {}
    virtual Tsubtitle* parse(Tstream &fd, int flags = TsubtitleParser::PARSETIME, REFERENCE_TIME start = REFTIME_INVALID, REFERENCE_TIME stop = REFTIME_INVALID);
};

class TsubtitleParserSubviewer : public TsubtitleParser
{
public:
    TsubtitleParserSubviewer(int Iformat, double Ifps, const TsubtitlesSettings *Icfg, const Tconfig *Iffcfg, Tsubreader *Isubreader): TsubtitleParser(Iformat, Ifps, Icfg, Iffcfg, Isubreader) {}
    virtual Tsubtitle* parse(Tstream &fd, int flags = TsubtitleParser::PARSETIME, REFERENCE_TIME start = REFTIME_INVALID, REFERENCE_TIME stop = REFTIME_INVALID);
};

class TsubtitleParserSami : public TsubtitleParser
{
private:
    int sub_slacktime;
    bool sub_no_text_pp;
    wchar_t line[TsubtitleParser::LINE_LEN + 1], *s;
    const wchar_t *slacktime_s;
public:
    TsubtitleParserSami(int Iformat, double Ifps, const TsubtitlesSettings *Icfg, const Tconfig *Iffcfg, Tsubreader *Isubreader): TsubtitleParser(Iformat, Ifps, Icfg, Iffcfg, Isubreader), s(NULL), sub_slacktime(2000), sub_no_text_pp(false) {} // 20 seconds
    virtual Tsubtitle* parse(Tstream &fd, int flags = TsubtitleParser::PARSETIME, REFERENCE_TIME start = REFTIME_INVALID, REFERENCE_TIME stop = REFTIME_INVALID);
};

class TsubtitleParserVplayer : public TsubtitleParser
{
public:
    TsubtitleParserVplayer(int Iformat, double Ifps, const TsubtitlesSettings *Icfg, const Tconfig *Iffcfg, Tsubreader *Isubreader): TsubtitleParser(Iformat, Ifps, Icfg, Iffcfg, Isubreader) {}
    virtual Tsubtitle* parse(Tstream &fd, int flags = TsubtitleParser::PARSETIME, REFERENCE_TIME start = REFTIME_INVALID, REFERENCE_TIME stop = REFTIME_INVALID);
};

class TsubtitleParserRt : public TsubtitleParser
{
public:
    TsubtitleParserRt(int Iformat, double Ifps, const TsubtitlesSettings *Icfg, const Tconfig *Iffcfg, Tsubreader *Isubreader): TsubtitleParser(Iformat, Ifps, Icfg, Iffcfg, Isubreader) {}
    virtual Tsubtitle* parse(Tstream &fd, int flags = TsubtitleParser::PARSETIME, REFERENCE_TIME start = REFTIME_INVALID, REFERENCE_TIME stop = REFTIME_INVALID);
};

class TsubtitleParserSSA : public TsubtitleParser
{
private:
    uint8_t inV4styles, inEvents, inInfo;
    int playResX, playResY, wrapStyle, scaleBorderAndShadow;
    Rational timer;
    bool isEmbedded;
    TSSAstyles styles;
    typedef std::vector<ffstring TSSAstyle::*> TstyleFormat;
    TstyleFormat styleFormat;

    struct Tevent {
        ffstring dummy;
        ffstring start, end, style, text;
        ffstring marked, readorder, layer, actor, marginT, marginB, name, marginL, marginR, marginV, effect;
    };
    typedef std::vector<ffstring Tevent::*> TeventFormat;
    TeventFormat eventFormat;
    TSubtitleProps defprops;
public:
    TsubtitleParserSSA(int Iformat, double Ifps, const TsubtitlesSettings *Icfg, const Tconfig *Iffcfg, Tsubreader *Isubreader, bool isEmbedded0);
    virtual Tsubtitle* parse(Tstream &fd, int flags = TsubtitleParser::PARSETIME, REFERENCE_TIME start = REFTIME_INVALID, REFERENCE_TIME stop = REFTIME_INVALID);
    nmTextSubtitles::TSSAversion version;
};

class TsubtitleParserDunnowhat : public TsubtitleParser
{
public:
    TsubtitleParserDunnowhat(int Iformat, double Ifps, const TsubtitlesSettings *Icfg, const Tconfig *Iffcfg, Tsubreader *Isubreader): TsubtitleParser(Iformat, Ifps, Icfg, Iffcfg, Isubreader) {}
    virtual Tsubtitle* parse(Tstream &fd, int flags = TsubtitleParser::PARSETIME, REFERENCE_TIME start = REFTIME_INVALID, REFERENCE_TIME stop = REFTIME_INVALID);
};

class TsubtitleParserMPsub : public TsubtitleParser
{
private:
    double mpsub_position;
    bool sub_uses_time;
public:
    TsubtitleParserMPsub(int Iformat, double Ifps, const TsubtitlesSettings *Icfg, const Tconfig *Iffcfg, Tsubreader *Isubreader): TsubtitleParser(Iformat, Ifps, Icfg, Iffcfg, Isubreader), mpsub_position(0), sub_uses_time(Iformat & Tsubreader::SUB_USESTIME ? true : false) {}
    virtual Tsubtitle* parse(Tstream &fd, int flags = TsubtitleParser::PARSETIME, REFERENCE_TIME start = REFTIME_INVALID, REFERENCE_TIME stop = REFTIME_INVALID);
};

class TsubtitleParserAqt : public TsubtitleParser
{
private:
    Tsubtitle *previous;
public:
    TsubtitleParserAqt(int Iformat, double Ifps, const TsubtitlesSettings *Icfg, const Tconfig *Iffcfg, Tsubreader *Isubreader): TsubtitleParser(Iformat, Ifps, Icfg, Iffcfg, Isubreader), previous(NULL) {}
    virtual Tsubtitle* parse(Tstream &fd, int flags = TsubtitleParser::PARSETIME, REFERENCE_TIME start = REFTIME_INVALID, REFERENCE_TIME stop = REFTIME_INVALID);
};

class TsubtitleParserSubviewer2 : public TsubtitleParser
{
public:
    TsubtitleParserSubviewer2(int Iformat, double Ifps, const TsubtitlesSettings *Icfg, const Tconfig *Iffcfg, Tsubreader *Isubreader): TsubtitleParser(Iformat, Ifps, Icfg, Iffcfg, Isubreader) {}
    virtual Tsubtitle* parse(Tstream &fd, int flags = TsubtitleParser::PARSETIME, REFERENCE_TIME start = REFTIME_INVALID, REFERENCE_TIME stop = REFTIME_INVALID);
};

class TsubtitleParserSubrip09 : public TsubtitleParser
{
private:
    Tsubtitle *previous;
public:
    TsubtitleParserSubrip09(int Iformat, double Ifps, const TsubtitlesSettings *Icfg, const Tconfig *Iffcfg, Tsubreader *Isubreader): TsubtitleParser(Iformat, Ifps, Icfg, Iffcfg, Isubreader), previous(NULL) {}
    virtual Tsubtitle* parse(Tstream &fd, int flags = TsubtitleParser::PARSETIME, REFERENCE_TIME start = REFTIME_INVALID, REFERENCE_TIME stop = REFTIME_INVALID);
};

class TsubtitleParserMPL2 : public TsubtitleParser
{
public:
    TsubtitleParserMPL2(int Iformat, double Ifps, const TsubtitlesSettings *Icfg, const Tconfig *Iffcfg, Tsubreader *Isubreader): TsubtitleParser(Iformat, Ifps, Icfg, Iffcfg, Isubreader) {}
    virtual Tsubtitle* parse(Tstream &fd, int flags = TsubtitleParser::PARSETIME, REFERENCE_TIME start = REFTIME_INVALID, REFERENCE_TIME stop = REFTIME_INVALID);
};

struct TsubreaderMplayer : Tsubreader {
public:
    TsubreaderMplayer(Tstream &f, int Isub_format, double Ifps, const TsubtitlesSettings *Icfg, const Tconfig *Iffcfg, bool isEmbedded);
};
