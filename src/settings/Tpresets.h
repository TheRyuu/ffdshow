#ifndef _TPRESETS_H_
#define _TPRESETS_H_

#include "interfaces.h"

struct Tpreset;
struct TautoPresetProps;
class Tpresets :public std::vector<Tpreset*>
{
private:
 const char_t *presetext;
 iterator findPreset(const char_t *presetName);
 void listRegKeys(strings &l);
protected:
 const char_t *reg_child;
 Tpresets(const char_t *Ireg_child,const char_t *Ipresetext):reg_child(Ireg_child),presetext(Ipresetext) {}
 virtual Tpreset* getAutoPreset0(TautoPresetProps &aprops,bool filefirst);
public:
 virtual ~Tpresets();
 virtual Tpresets* newPresets(void)=0;
 void init(void);
 void done(void);
 virtual Tpreset* newPreset(const char_t *presetName=NULL)=0;
 virtual Tpreset* getPreset(const char_t *presetName,bool create);
 virtual Tpreset* getAutoPreset(IffdshowBase *deci,bool filefirst);
 void savePreset(Tpreset *preset,const char_t *presetName);
 bool savePresetFile(Tpreset *preset,const char_t *flnm);
 void storePreset(Tpreset *preset);
 bool removePreset(const char_t *presetName);
 void saveRegAll(void);
 void nextUniqueName(Tpreset *preset),nextUniqueName(char_t *presetName); 
};

struct TpresetVideo;
struct TvideoAutoPresetProps;
class TpresetsVideo :public Tpresets
{
protected:
 virtual Tpreset* getAutoPreset0(TautoPresetProps &aprops,bool filefirst);
 TpresetsVideo(const char_t *Ireg_child):Tpresets(Ireg_child,_l("ffpreset")) {}
public:
 virtual Tpresets* newPresets(void) {return new TpresetsVideo(reg_child);}
 virtual Tpreset* getAutoPreset(IffdshowBase *deci,bool filefirst);
 virtual Tpreset* newPreset(const char_t *presetName=NULL);
};

class TpresetsVideoProc :public TpresetsVideo
{
public:
 TpresetsVideoProc(void):TpresetsVideo(FFDSHOWDECVIDEO) {}
 virtual Tpresets* newPresets(void) {return new TpresetsVideoProc;}
};

class TpresetsVideoPlayer :public TpresetsVideo
{
public:
 TpresetsVideoPlayer(void):TpresetsVideo(FFDSHOWDECVIDEO) {}
 virtual Tpresets* newPresets(void) {return new TpresetsVideoPlayer;}
 virtual Tpreset* newPreset(const char_t *presetName=NULL);
};


class TpresetsVideoVFW :public TpresetsVideo
{
public:
 TpresetsVideoVFW(void):TpresetsVideo(FFDSHOWDECVIDEOVFW) {}
 virtual Tpresets* newPresets(void) {return new TpresetsVideoVFW;}
};

struct TaudioAutoPresetProps;
class TpresetsAudio :public Tpresets
{
public:
 TpresetsAudio(const char_t *IregChild=FFDSHOWDECAUDIO):Tpresets(IregChild,_l("ffApreset")) {}
 virtual Tpreset* getAutoPreset(IffdshowBase *deci,bool filefirst);
 virtual Tpresets* newPresets(void) {return new TpresetsAudio;}
 virtual Tpreset* newPreset(const char_t *presetName=NULL);
};

class TpresetsAudioRaw :public TpresetsAudio
{
public:
 TpresetsAudioRaw(void):TpresetsAudio(FFDSHOWDECAUDIORAW) {}
};

#endif
