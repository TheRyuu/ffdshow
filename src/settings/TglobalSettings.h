#ifndef _TGLOBALSETTINGS_H_
#define _TGLOBALSETTINGS_H_

#include "TOSDsettings.h"
#include "ffcodecs.h"
#include "Toptions.h"

#define COMPATIBLE_EXE_FILENAME _l("aegisub.exe\r\nALLPlayer.exe\r\nALShow.exe\r\nass_help3r.exe\r\navipreview.exe\r\naviutl.exe\r\nbsplay.exe\r\nbsplayer.exe\r\nCorePlayer.exe\r\ncoreplayer.exe\r\ncrystalfree.exe\r\nDScaler.exe\r\ndv.exe\r\nDVDMaker.exe\r\nehshell.exe\r\nfirefox.exe\r\ngom.exe\r\ngraphedt.exe\r\ngspot.exe\r\niexplore.exe\r\nJetAudio.exe\r\nkmplayer.exe\r\nLA.exe\r\nLilith.exe\r\nmakeAVIS.exe\r\nMedia Center 12.exe\r\nMedia Jukebox.exe\r\nmegui.exe\r\nMovieMk.exe\r\nmplayer2.exe\r\nmplayerc.exe\r\nMunite.exe\r\nMuzikbrowzer.exe\r\nMv2PlayerPlus.exe\r\nnvplayer.exe\r\npowerdvd.exe\r\nprogdvb.exe\r\nQonoha.exe\r\nrealplay.exe\r\nrlkernel.exe\r\nSinkuHadouken.exe\r\nTheaterTek DVD.exe\r\ntimecodec.exe\r\nViPlay3.exe\r\nvirtualdub.exe\r\nvirtualdubmod.exe\r\nwinamp.exe\r\nwindvd.exe\r\nwmenc.exe\r\nwmplayer.exe\r\nzplayer.exe")
#define BLACKLIST_EXE_FILENAME _l("explorer.exe;oblivion.exe;morrowind.exe")

struct TregOp;
struct Tconfig;
struct TglobalSettingsBase :public Toptions
{
protected:
 const Tconfig *config;
 const char_t *reg_child;
 virtual void reg_op_codec(TregOp &t,TregOp *t2) {}
 void _reg_op_codec(short id,TregOp &tHKCU,TregOp *tHKLM,const char_t *name,int &val,int def);
 strings blacklistList,useonlyinList;bool firstBlacklist,firstUseonlyin;
public:
 TglobalSettingsBase(const Tconfig *Iconfig,int Imode,const char_t *Ireg_child,TintStrColl *Icoll);
 virtual ~TglobalSettingsBase() {}
 bool exportReg(bool all,const char_t *regflnm,bool unicode);
 int filtermode;
 int multipleInstances;
 int isBlacklist,isUseonlyin;char_t blacklist[MAX_COMPATIBILITYLIST_LENGTH],useonlyin[MAX_COMPATIBILITYLIST_LENGTH];
 virtual bool inBlacklist(const char_t *exe);
 virtual bool inUseonlyin(const char_t *exe);
 int addToROT;
 int allowedCPUflags;

 virtual void load(void);
 virtual void save(void);
 int trayIcon,trayIconExt;

 int outputdebug;
 int outputdebugfile;char_t debugfile[MAX_PATH];
 int errorbox;

 char_t dscalerPth[MAX_PATH];
};

struct TglobalSettingsDec :public TglobalSettingsBase
{
private:
 TglobalSettingsDec& operator =(const TglobalSettingsDec&);
protected:
 virtual void reg_op(TregOp &t);
 void fixMissing(int &codecId,int movie1,int movie2,int movie3);
 void fixMissing(int &codecId,int movie1,int movie2);
 void fixMissing(int &codecId,int movie);
 virtual int getDefault(int id);
 static void cleanupCodecsList(std::vector<CodecID> &ids,Tstrptrs &codecs);
 TglobalSettingsDec(const Tconfig *Iconfig,int Imode,const char_t *Ireg_child,TintStrColl *Icoll,TOSDsettings *Iosd);
public:
 char_t defaultPreset[MAX_PATH];
 int autoPreset,autoPresetFileFirst;

 int streamsMenu;
 TOSDsettings *osd;

 virtual void load(void);
 virtual void save(void);

 virtual CodecID getCodecId(DWORD fourCC,FOURCC *AVIfourCC) const =0;
 virtual const char_t** getFOURCClist(void) const=0;
 virtual void getCodecsList(Tstrptrs &codecs) const=0;
};

struct TglobalSettingsDecVideo :public TglobalSettingsDec
{
private:
 int forceInCSP;
 int needCodecFix;
 void fixNewCodecs(void);
 static const char_t *fourccs[];
 static const CodecID c_mpeg4[IDFF_MOVIE_MAX+1],c_mpeg1[IDFF_MOVIE_MAX+1],c_mpeg2[IDFF_MOVIE_MAX+1],c_theora[IDFF_MOVIE_MAX+1],c_wvc1[IDFF_MOVIE_MAX+1],c_wmv3[IDFF_MOVIE_MAX+1],c_wmv2[IDFF_MOVIE_MAX+1],c_wmv1[IDFF_MOVIE_MAX+1];
protected:
 virtual void reg_op(TregOp &t);
 virtual void reg_op_codec(TregOp &t,TregOp *t2);
 virtual int getDefault(int id);
public:
 int buildHistogram;
 int xvid,div3,mp4v,dx50,fvfw,mp43,mp42,mp41,h263,h264,vp5,vp6,vp6f,wmv1,wmv2,wvc1,wmv3,mss2,wvp2,cavs,rawv,mpg1,mpg2,mpegAVI,em2v,avrn,mjpg,dvsd,cdvc,hfyu,cyuv,theo,asv1,svq1,svq3,cram,rt21,iv32,cvid,rv10,ffv1,vp3,vcr1,rle,avis,mszh,zlib,flv1,_8bps,png1,qtrle,duck,tscc,qpeg,h261,loco,wnv1,cscd,zmbv,ulti,vixl,aasc,fps1,qtrpza,snow;
 int supdvddec;
 int fastMpeg2,fastH264,libtheoraPostproc;

 virtual void load(void);
 TOSDsettingsVideo osd;

 struct TsubtitlesSettings : Toptions
  {
  protected:
   virtual void getDefaultStr(int id,char_t *buf,size_t buflen);
  public:
   TsubtitlesSettings(TintStrColl *Icoll);
   char_t searchDir[2*MAX_PATH];char_t searchExt[2*MAX_PATH];
   int searchHeuristic;
   int watch;
   int textpin,textpinSSA;
  } sub;

 TglobalSettingsDecVideo(const Tconfig *Iconfig,int Imode,TintStrColl *Icoll);
 virtual CodecID getCodecId(DWORD fourCC,FOURCC *AVIfourCC) const;
 virtual const char_t** getFOURCClist(void) const;
 virtual void getCodecsList(Tstrptrs &codecs) const;
};

struct TglobalSettingsDecAudio :public TglobalSettingsDec
{
private:
 static const char_t *wave_formats[];
 static const CodecID c_mp123[IDFF_MOVIE_MAX+1],c_ac3[IDFF_MOVIE_MAX+1],c_dts[IDFF_MOVIE_MAX+1],c_aac[IDFF_MOVIE_MAX+1],c_vorbis[IDFF_MOVIE_MAX+1];
protected:
 virtual void reg_op_codec(TregOp &t,TregOp *t2);
 virtual int getDefault(int id);
 virtual void getDefaultStr(int id,char_t *buf,size_t buflen);
public:
 TglobalSettingsDecAudio(const Tconfig *Iconfig,int Imode,TintStrColl *Icoll,const char_t *Ireg_child=FFDSHOWDECAUDIO);
 TOSDsettingsAudio osd;
 int wma1,wma2,mp2,mp3,ac3,aac,amr,rawa,avis,iadpcm,msadpcm,flac,lpcm,dts,vorbis,law,gsm,tta,otherAdpcm,qdm2,truespeech,mace,ra,imc,atrac3;
 int dtsinwav;
 int ac3drc,dtsdrc;
 int showCurrentVolume;
 int showCurrentFFT;
 int firIsUserDisplayMaxFreq,firUserDisplayMaxFreq;
 int isAudioSwitcher;
 int alwaysextensible;
 int allowOutStream;
 int vorbisgain;
 char_t winamp2dir[MAX_PATH];
 virtual void load(void);
 virtual CodecID getCodecId(DWORD fourCC,FOURCC *AVIfourCC) const;
 virtual const char_t** getFOURCClist(void) const;
 virtual void getCodecsList(Tstrptrs &codecs) const;
};
struct TglobalSettingsDecAudioRaw :public TglobalSettingsDecAudio
{
 TglobalSettingsDecAudioRaw(const Tconfig *Iconfig,int Imode,TintStrColl *Icoll):TglobalSettingsDecAudio(Iconfig,Imode,Icoll,FFDSHOWDECAUDIORAW) {}
};

struct TglobalSettingsEnc :public TglobalSettingsBase
{
protected:
 virtual int getDefault(int id);
public:
 TglobalSettingsEnc(const Tconfig *Iconfig,int Imode,TintStrColl *Icoll);
 int psnr;
 int isDyInterlaced,dyInterlaced;
};

#endif
