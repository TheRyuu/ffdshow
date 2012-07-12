#ifndef _TIMGFILTEROSD_H_
#define _TIMGFILTEROSD_H_

#include "TimgFilter.h"
#include "Tfont.h"
#include "Tsubreader.h"
#include "TsubtitleText.h"
#include "TOSDprovider.h"
#include "Tconfig.h"

struct TfontSettingsOSD;
struct TOSDsettings;
class Ttranslate;
struct TOSDsettingsVideo;
class TimgFilterOSD : public TimgFilter, public TOSDprovider
    _DECLARE_FILTER(TimgFilterOSD, TimgFilter)
    private:
        TimgFiltersPlayer *parent;
Ttranslate *trans;
struct TosdLine {
private:
    IOSDprovider *provider;
    const Tconfig *config;
    TsubtitleText sub;
    bool firsttime;
    struct TosdValue {
    private:
        char_t s[512], olds[512];
        int oldVal;
        Rational oldSar, oldDar;
        double oldDuration;
        IOSDprovider *provider;
        int type;
        mutable const char_t *name;
    public:
        TosdValue(void): name(NULL), provider(NULL) {}
        TosdValue(int Itype, IOSDprovider *Iprovider): type(Itype), provider(Iprovider), oldDuration(-1), name(NULL) {
            s[0] = olds[0] = '\0';
            oldVal = -1;
        }
        const char_t* getName(void) const;
        const char_t* getVal(bool &wasChange, bool &splitline, const TffPict &pict);
    };
    struct TosdToken {
    public:
        enum {
            TOKEN_STRING,
            TOKEN_VALUE,
        } type;
    private:
        ffstring string;
        TosdValue val;
    public:
        TosdToken(const char_t *s): type(TOKEN_STRING), string(s) {}
        TosdToken(int Itype, IOSDprovider *Iprovider): type(TOKEN_VALUE), val(Itype, Iprovider) {}
        const char_t *getName(void) const;
        const char_t *getStr(bool &wasChange, bool &splitline, const TffPict &pict) {
            switch (type) {
                case TOKEN_VALUE:
                    return val.getVal(wasChange, splitline, pict);
                default:
                case TOKEN_STRING:
                    wasChange = splitline = false;
                    return string.c_str();
            }
        }
    };
    typedef std::vector<TosdToken> TosdTokens;
    TosdTokens tokens;
public:
    TosdLine(IffdshowBase *Ideci, IffdshowDec *IdeciB, IffdshowDecVideo *IdeciV, const Tconfig *Iconfig, const ffstring &Iformat, unsigned int Iduration, IOSDprovider *Iprovider, bool Iitalic = false);
    int duration;
    unsigned int posX;
    unsigned int posY;
    Tfont font;
    unsigned int print(
        IffdshowBase *deci,
        const TffPict &pict,
        unsigned char *dst[4],
        stride_t stride[4],
        unsigned int dxY,
        unsigned int dyY,
        int linespace,
        FILE *f,
        bool fileonly,
        const TfontSettings &fontSettings);
    unsigned int print(
        IffdshowBase *deci,
        const TffPict &pict,
        unsigned char *dst[4],
        stride_t stride[4],
        unsigned int dxY,
        unsigned int dyY,
        unsigned int x,
        unsigned int y,
        int linespace,
        FILE *f,
        bool fileonly,
        const TfontSettings &fontSettings);
    const char_t *getName(unsigned int i) const;
};

struct Tosds : public std::vector<TosdLine*> {
private:
    char_t name[MAX_PATH];
    FILE *f;
    int oldSave;
    char_t oldSaveFlnm[MAX_PATH];
    char_t oldFormat[1024];
public:
    Tosds(IOSDprovider *Iprovider = NULL, const char_t *Iname = NULL);
    ~Tosds();
    bool is;
    IOSDprovider *provider;
    void init(bool allowSave, IffdshowBase *deci, IffdshowDec *deciD, IffdshowDecVideo *deciV, const Tconfig *config, const TOSDsettingsVideo *cfg, int framecnt);
    unsigned int print(
        IffdshowBase *deci,
        const TffPict &pict,
        unsigned char *dst[4],
        stride_t stride[4],
        unsigned int dxY,
        unsigned int dyY,
        int linespace,
        bool fileonly,
        const TfontSettings &fontSettings);
    unsigned int print(
        IffdshowBase *deci,
        const TffPict &pict,
        unsigned char *dst[4],
        stride_t stride[4],
        unsigned int dxY,
        unsigned int dyY,
        unsigned int x,
        unsigned int y,
        int linespace,
        bool fileonly,
        const TfontSettings &fontSettings);
    void done(void);
    void freeOsds(void);
};
Tosds shortOsdRelative;
Tosds shortOsdAbsolute;

// IOSDprovider
STDMETHODIMP_(const char_t*) getInfoItemName(int type);

struct TprovOSDs : std::vector<Tosds*> {
    bool empty(void) const;
};
TprovOSDs provOSDs;
CCritSec csProvider;

unsigned int framecnt;

struct TshortOsdParameters {
    unsigned int duration;
    unsigned int posX;
    unsigned int posY;
};

CCritSec cs;
CCritSec csClean;
typedef std::pair<ffstring, TshortOsdParameters> TshortOsdTemp;
std::vector<TshortOsdTemp> shortOsdRelativeTemp;
std::vector<TshortOsdTemp> shortOsdAbsoluteTemp;
protected:
virtual bool is(const TffPictBase &pict, const TfilterSettingsVideo *cfg);
virtual uint64_t getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const
{
    return FF_CSP_420P | FF_CSP_RGB32;
}
public:
TimgFilterOSD(IffdshowBase *Ideci, Tfilters *Iparent);
virtual ~TimgFilterOSD(void);
virtual HRESULT process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg);
virtual void done(void);
bool shortOSDmessage(const char_t *msg, unsigned int duration);
bool shortOSDmessageAbsolute(const char_t *msg, unsigned int duration, unsigned int posX, unsigned int posY);
bool cleanShortOSDmessages(void);
HRESULT registerOSDprovider(IOSDprovider *provider, const char *name);
HRESULT unregisterOSDprovider(IOSDprovider *provider);
virtual bool acceptRandomYV12andRGB32(void)
{
    return true;
}
};

#endif
