#ifndef _FFDSHOWREMOTEAPIIMPL_H_
#define _FFDSHOWREMOTEAPIIMPL_H_

#include "Toptions.h"
#include "ffdshowRemoteAPI.h"
#include "interfaces.h"
#include "TffdshowDec.h"


enum ffrwMode {
    FAST_FORWARD_MODE = 1,
    REWIND_MODE = -1
};

enum streamType {
    AUDIO_STREAM_TYPE = 1,
    SUBTITLES_STREAM_TYPE = 2
};
class Ttranslate;
class Tkeyboard;
class Tremote : public Toptions
{
private:
    int is, messageMode, messageUser, acceptKeys;
    IffdshowBase *deci;
    comptrQ<IffdshowDec> deciD;
    comptrQ<IffdshowDecVideo> deciV;
    UINT remotemsg;
    static unsigned int __stdcall threadProc(void*);
    static unsigned int __stdcall ffwdThreadProc(void*); // Thread method for fast forward/rewind
    HWND h;
    LRESULT CALLBACK remoteWndProc(HWND wnd, UINT msg, WPARAM wprm, LPARAM lprm);
    static LRESULT CALLBACK remoteWndProc0(HWND hwnd, UINT msg, WPARAM wprm, LPARAM lprm);
    int paramid;
    HANDLE hThread;
    HANDLE fThread; // Thread used to perform fast forward/rewind
    HANDLE fEvent; // Manual reset event for terminating the thread
    int fSeconds; // Step in seconds to perform fast forward/rewind
    int fMode; // Fast forward/rewind mode
    Tkeyboard *keys;
    unsigned int subtitleIdx;
    void start(void), stop(void);
    bool inExplorer;
    DWORD pdwROT;
    int OSDDuration;
    int OSDPositionX, OSDPositionY;
    TexternalStreams *pAudioStreams, *pSubtitleStreams, *pEditionStreams;
    bool streamsLoaded;
    bool foundHaali;
    bool noFFRWOSD;
    CCritSec m_csRemote;
    Ttranslate *tr;
    int currentSubStream, currentAudioStream;
public:
    Tremote(TintStrColl *Icoll, IffdshowBase *Ideci);
    ~Tremote();
    void load(void), save(void);
    void onChange(int id, int val);
    void getStreams(bool reload);
    void fastForward(ffrwMode mode, int seconds);
    void onSubStreamChange(int id, int val);
    void onAudioStreamChange(int id, int val);
    void onFastForwardChange(int id, int val);
};

#endif
