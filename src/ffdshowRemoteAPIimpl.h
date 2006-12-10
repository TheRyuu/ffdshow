#ifndef _FFDSHOWREMOTEAPIIMPL_H_
#define _FFDSHOWREMOTEAPIIMPL_H_

#include "Toptions.h"
#include "ffdshowRemoteAPI.h"
#include "interfaces.h"

class Tkeyboard;
class Tremote : public Toptions
{
private:
 int is,messageMode,messageUser,acceptKeys;
 IffdshowBase *deci;comptrQ<IffdshowDec> deciD;comptrQ<IffdshowDecVideo> deciV;
 UINT remotemsg;
 static unsigned int __stdcall threadProc(void*);
 HWND h;
 LRESULT CALLBACK remoteWndProc(HWND wnd, UINT msg, WPARAM wprm, LPARAM lprm);
 static LRESULT CALLBACK remoteWndProc0(HWND hwnd, UINT msg, WPARAM wprm, LPARAM lprm);
 int paramid;
 HANDLE hThread;
 Tkeyboard *keys;
 unsigned int subtitleIdx;
 void start(void),stop(void); 
 bool inExplorer;
public:
 Tremote(TintStrColl *Icoll,IffdshowBase *Ideci);
 ~Tremote();
 void load(void),save(void);
 void onChange(int id,int val);
};

#endif
