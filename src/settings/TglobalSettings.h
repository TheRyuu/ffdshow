#ifndef _TGLOBALSETTINGS_H_
#define _TGLOBALSETTINGS_H_

#include "TOSDsettings.h"
#include "ffcodecs.h"
#include "Toptions.h"

#define MAX_COMPATIBILITYLIST_LENGTH 5000

#define WHITELIST_EXE_FILENAME \
	_l("3wPlayer.exe;ACDSee5.exe;ACDSee6.exe;ACDSee7.exe;ACDSee8.exe;ACDSee8Pro.exe;ACDSee9.exe;Adobe Premiere Elements.exe;Adobe Premiere Pro.exe;aegisub.exe;afreecaplayer.exe;afreecastudio.exe;aim6.exe;ALLPlayer.exe;ALShow.exe;ALSong.exe;AltDVB.exe;amcap.exe;amvtransform.exe;Apollo DivX to DVD Creator.exe;Apollo3GPVideoConverter.exe;Ares.exe;AsfTools.exe;ass_help3r.exe;ASUSDVD.exe;Audition.exe;AutoGK.exe;autorun.exe;avant.exe;AVerTV.exe;Avi2Dvd.exe;avi2mpg.exe;avicodec.exe;avipreview.exe;aviutl.exe;avs2avi.exe;Badak.exe;BearShare.exe;BePipe.exe;bestplayer1.0.exe;BitComet.exe;BlazeDVD.exe;bplay.exe;bsplay.exe;bsplayer.exe;BTVD3DShell.exe;CamRecorder.exe;CamtasiaStudio.exe;carom.exe;christv.exe;cinemaplayer.exe;CinergyDVR.exe;CodecInstaller.exe;ConvertXtoDvd.exe;coolpro2.exe;CorePlayer.exe;coreplayer.exe;Crystal.exe;crystalfree.exe;CrystalPro.exe;")\
	_l("cscript.exe;CTCMS.exe;CTCMSU.exe;CTWave.exe;CTWave32.exe;cut_assistant.exe;dashboard.exe;demo32.exe;DivX Player.exe;dllhost.exe;dpgenc.exe;Dr.DivX.exe;drdivx.exe;drdivx2.exe;DreamMaker.exe;DSBrws.exe;DScaler.exe;dv.exe;dvbdream.exe;dvbviewer.exe;DVD Shrink 3.2.exe;DVDAuthor.exe;DVDMaker.exe;DVDMF.exe;dvdplay.exe;DXEnum.exe;Easy RealMedia Tools.exe;ehExtHost.exe;ehshell.exe;emule_TK4.exe;Encode360.exe;fenglei.exe;ffmpeg.exe;filtermanager.exe;firefox.exe;Flash.exe;FMRadio.exe;Fortius.exe;FreeStyle.exe;FSViewer.exe;FusionHDTV.exe;GDivX Player.exe;gdsmux.exe;GoldWave.exe;gom.exe;GomEnc.exe;GoogleDesktop.exe;GoogleDesktopCrawl.exe;graphedit.exe;graphedt.exe;gspot.exe;HBP.exe;HDVSplit.exe;honestechTV.exe;HPWUCli.exe;i_view32.exe;ICQ.exe;ICQLite.exe;iexplore.exe;IHT.exe;IncMail.exe;InfoTool.exe;infotv.exe;iPlayer.exe;JetAudio.exe;jwBrowser.exe;")\
	_l("kmplayer.exe;LA.exe;LifeCam.exe;Lilith.exe;makeAVIS.exe;Maxthon.exe;MDirect.exe;Media Center 12.exe;Media Jukebox.exe;Media Player Classic.exe;MediaLife.exe;MediaPortal.exe;MEDIAREVOLUTION.EXE;MediaServer.exe;megui.exe;mencoder.exe;Metacafe.exe;MMPlayer.exe;moviethumb.exe;mpcstar.exe;MpegVideoWizard.exe;mplayer2.exe;mplayerc.exe;msoobe.exe;MultimediaPlayer.exe;Munite.exe;MusicManager.exe;Muzikbrowzer.exe;Mv2PlayerPlus.exe;myplayer.exe;nero.exe;NeroHome.exe;NeroVision.exe;NicoPlayer.exe;NMSTranscoder.exe;nvplayer.exe;Omgjbox.exe;OnlineTV.exe;Opera.exe;OrbStreamerClient.exe;OUTLOOK.EXE;PaintDotNet.exe;paltalk.exe;pcwmp.exe;PhotoScreensaver.scr;Photoshop.exe;Picasa2.exe;playwnd.exe;PowerDirector.exe;powerdvd.exe;POWERPNT.EXE;PPLive.exe;ppmate.exe;PPStream.exe;Procoder2.exe;Producer.exe;progdvb.exe;PVCR.exe;Qonoha.exe;QQPlayerSvr.exe;RadLight.exe;realplay.exe;")\
	_l("Recode.exe;rlkernel.exe;RoxMediaDB9.exe;rundll32.exe;SelfMV.exe;Shareaza.exe;sherlock2.exe;ShowTime.exe;sidebar.exe;SinkuHadouken.exe;Sleipnir.exe;smartmovie.exe;SopCast.exe;SplitCam.exe;START.EXE;stillcap.exe;Studio.exe;subedit.exe;SubtitleEdit.exe;SubtitleWorkshop.exe;SWFConverter.exe;TheaterTek DVD.exe;time_adjuster.exe;timecodec.exe;tmc.exe;TMPGEnc.exe;TMPGEnc4XP.exe;TOTALCMD.EXE;tvc.exe;TVersity.exe;TVPlayer.exe;TVUPlayer.exe;UCC.exe;Ultra EDIT.exe;VCD_PLAY.EXE;VeohClient.exe;VFAPIFrameServer.exe;VideoSnapshot.exe;VideoSplitter.exe;VIDEOS~1.SCR;ViPlay.exe;ViPlay3.exe;ViPlay4.exe;virtualdub.exe;virtualdubmod.exe;vplayer.exe;WCreator.exe;WFTV.exe;winamp.exe;WinAVI.exe;WindowsPhotoGallery.exe;windvd.exe;WinMPGVideoConvert.exe;WINWORD.EXE;wmenc.exe;wmplayer.exe;wscript.exe;x264.exe;XNVIEW.EXE;Xvid4PSP.exe;YahooMusicEngine.exe;YahooWidgetEngine.exe;zplayer.exe;")\
	_l("Zune.exe;moviemk.exe;ACDSee10.exe;BoonPlayer.exe;CEC_MAIN.exe;msnmsgr.exe;QzoneMusic.exe;ReClockHelper.dll;YahooMessenger.exe;ACDSeePro2.exe;AlltoaviV4.exe;Funshion.exe;Internet TV.exe;MatroskaDiag.exe;QQ.exe;Tvants.exe")

#define BLACKLIST_EXE_FILENAME _l("age3.exe;cm2008.exe;cmr5.exe;conquer.exe;dmc3se.exe;em3.exe;em4.exe;fable.exe;game.exe;gta-vc.exe;gta3.exe;gta_sa.exe;gta-underground.exe;hl.exe;hl2.exe;juiced2_hin.exe;morrowind.exe;oblivion.exe;pes2008.exe;pes4.exe;pes5.exe;pes6.exe;residentevil3.exe;rf_online.bin;sacred.exe;sega rally.exe;sh3.exe;sh4.exe;tw2008.exe;twoworlds.exe;war3.exe;worms 4 mayhem.exe")

struct TregOp;
struct Tconfig;
struct TglobalSettingsBase :public Toptions
{
private:
 void addToCompatiblityList(char_t *list, const char_t *exe, const char_t *delimit);
protected:
 const Tconfig *config;
 const char_t *reg_child;
 virtual void reg_op_codec(TregOp &t,TregOp *t2) {}
 void _reg_op_codec(short id,TregOp &tHKCU,TregOp *tHKLM,const char_t *name,int &val,int def);
 strings blacklistList,whitelistList;bool firstBlacklist,firstWhitelist;
public:
 TglobalSettingsBase(const Tconfig *Iconfig,int Imode,const char_t *Ireg_child,TintStrColl *Icoll);
 virtual ~TglobalSettingsBase() {}
 bool exportReg(bool all,const char_t *regflnm,bool unicode);
 int filtermode;
 int multipleInstances;
 int isBlacklist,isWhitelist;char_t blacklist[MAX_COMPATIBILITYLIST_LENGTH],whitelist[MAX_COMPATIBILITYLIST_LENGTH];
 virtual bool inBlacklist(const char_t *exe);
 virtual bool inWhitelist(const char_t *exe,IffdshowBase *Ideci);
 int addToROT;
 int allowedCPUflags;
 int compOnLoadMode;

 virtual void load(void);
 virtual void save(void);
 int trayIcon,trayIconType,trayIconExt,trayIconChanged;
 int isCompMgr,isCompMgrChanged;
 int allowDPRINTF,allowDPRINTFchanged;

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
 int alternateUncompressed;
 int autodetect24P;

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
 int wma1,wma2,mp2,mp3,ac3,aac,amr,rawa,avis,iadpcm,msadpcm,flac,lpcm,dts,vorbis,law,gsm,tta,otherAdpcm,qdm2,truespeech,mace,ra,imc,atrac3,nellymoser;
 int dtsinwav;
 int ac3drc,dtsdrc;
 int ac3SPDIF;
 int SPDIFCompatibility;
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
