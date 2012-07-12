#ifndef _TIMGFILTERSUBTITLES_H_
#define _TIMGFILTERSUBTITLES_H_

#include "TimgFilter.h"
#include "Tfont.h"
#include "TsubtitlesFile.h"
#include "TsubtitlesSettings.h"
#include "TexpandSettings.h"
#include "TsubtitlesTextpin.h"

class TsubtitlesTextpin;
class TimgFilterSubtitleExpand;
struct TfontSettingsSub;
DECLARE_FILTER(TimgFilterSubtitles, public, TimgFilter)

public:
typedef enum {
    ADHOC_NORMAL,
    ADHOC_ADHOC_DRAW_DVD_SUB_ONLY,
    ADHOC_SECOND_DONT_DRAW_DVD_SUB
} AdhocMode;

private:
Trect oldRectClip;
int isdvdproc;
bool wasDiscontinuity;
bool expandSizeChanged, fontSizeChanged;
TsubtitlesFile subs;
typedef stdext::hash_map<int, TsubtitlesTextpin*> Tembedded;
Tembedded embedded;
Tfont font, fontCC;
int oldstereo, oldsplitborder;
double AVIfps;
TimgFilterSubtitleExpand *expand;
TexpandSettings expandSettings;
Trect oldExpandRect;
int oldExpandCode;
unsigned int oldSizeDx, oldSizeDy;
boost::recursive_mutex csEmbedded, csCC;
TfilterQueue::iterator prevIt;
REFERENCE_TIME prevTime;
TffPict prevPict;
const TfilterSettingsVideo *prevCfg;
bool again;
Tbuffer prevbuf;
AdhocMode prevAdhocMode;
int subFlnmChanged;
const char_t* findAutoSubFlnm(const TsubtitlesSettings *cfg);
bool first;

bool sequenceEnded;
TprintPrefs oldprefs;

struct TsubPrintPrefs : TprintPrefs {
    TsubPrintPrefs(
        unsigned int Idx[4],
        unsigned int Idy[4],
        IffdshowBase *Ideci,
        const TsubtitlesSettings *cfg,
        const TffPict &pict,
        int Iclipdy,
        const Tconfig *Iconfig,
        bool Idvd,
        const TfontSettings *fontSettings);
};

TsubtitleText *cc;
bool wasCCchange;
bool everRGB;
AdhocMode adhocMode; // 0: normal, 1: adhoc! process only DVD sub/menu, 2: after adhoc, second call. process none DVD sub (cc decoder, etc).

// Prepares glph in background.
class TglyphThread
{
    // Subtitles preparation requires a lot of memory to store prepared image.
    // Please note that subitles filter uses a lot of memory not only for storing prepared images.
    // If you set 20MB here, subitles filter would use 40MB or so.
    // Please understand it's natural and not wasting memory.
    size_t max_memory_usage;
    size_t used_memory;

    TimgFilterSubtitles *parent;
    boost::thread *thread;
    TprintPrefs copied_prefs;
    size_t current_pos;
    Tsubtitles *oldpin;
    HANDLE platform_specific_thread;

    int threadCmd; // 0:end 1: continue
    TprintPrefs shared_prefs;
    boost::mutex mutex_prefs;
    boost::condition_variable condv_prefs;

    void glyphThreadFunc();
    static void glyphThreadFunc0(TimgFilterSubtitles::TglyphThread *self) {
        self->glyphThreadFunc();
        boost::unique_lock<boost::mutex> lock(self->mutex_terminate);
        self->terminated = true;
        self->condv_terminate.notify_one();
    }

    // get reference to Tsubreader object
    Tsubreader* get_subreader();

    void onSeek();
    void clean_past();
    void slow();
    void hustle();

    // get next subtitle to render
    TsubtitleText* getNext();
    Tfont font;
    bool firstrun;
    bool terminated;
    boost::mutex mutex_terminate;
    boost::condition_variable condv_terminate;

public:
    HANDLE get_platform_specific_thread();
    TglyphThread(TimgFilterSubtitles *Iparent, IffdshowBase *deci);
    ~TglyphThread() {
        delete thread;
    };
    void done();
    friend class TimgFilterSubtitles;
} glyphThread;

public:
HANDLE getGlyphThreadHandle();

protected:
virtual bool is(const TffPictBase &pict, const TfilterSettingsVideo *cfg);
virtual uint64_t getSupportedInputColorspaces(const TfilterSettingsVideo *cfg) const
{
    return FF_CSP_420P | FF_CSP_RGB32;
}
virtual void onSizeChange();
TsubtitlesTextpin* getTextpin();

public:
TimgFilterSubtitles(IffdshowBase *Ideci, Tfilters *Iparent);
virtual ~TimgFilterSubtitles();
virtual bool getOutputFmt(TffPictBase &pict, const TfilterSettingsVideo *cfg0);
virtual HRESULT process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0);
virtual void onSeek();
void onSubFlnmChange(int id, int), onSubFlnmChangeStr(int id, const char_t*);

bool initSubtitles(int id, int type, const unsigned char *extradata, unsigned int extradatalen);
void addSubtitle(int id, REFERENCE_TIME start, REFERENCE_TIME stop, const unsigned char *data, unsigned int datalen, const TsubtitlesSettings *cfg, bool utf8);
void resetSubtitles(int id);
bool ctlSubtitles(int id, int type, unsigned int ctl_id, const void *ctl_data, unsigned int ctl_datalen);
const char_t *getCurrentFlnm() const;

void addClosedCaption(const wchar_t *line), hideClosedCaptions();
virtual int getImgFilterID()
{
    return IMGFILTER_SUBTITLES;
}
bool enterAdhocMode();
};


#endif
