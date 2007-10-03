; Requires Inno Setup (http://www.innosetup.com) and ISPP (http://sourceforge.net/projects/ispp/)
; Place this script in directory: /bin/distrib/innosetup/

#define tryout_revision = 1500
#define buildyear = 2007
#define buildmonth = '10'
#define buildday = '03'

; Build specific options
#define unicode_required = True

#define include_cpu_detection = True
#define sse_required = False
#define sse2_required = False

#define MSVC80 = True

#define is64bit = False

#define localize = True

#define include_app_plugins = True
#define include_makeavis = True
#define include_x264 = True
#define include_xvidcore = True
#define include_audx = False
#define include_info_before = False
#define include_gnu_license = True
#define include_setup_icon = False

#define filename_suffix = ''
#define outputdir = '.'

; Custom builder preferences
#define PREF_CLSID = False
#define PREF_CLSID_ICL = False
#define PREF_YAMAGATA = False
#define PREF_XXL = False
#define PREF_X64 = False

#if PREF_CLSID
  #define MSVC80 = False
  #define unicode_required = False
  #define filename_suffix = '_clsid'
  #define outputdir = '..\..\..\..\'
#endif
#if PREF_CLSID_ICL
  #define MSVC80 = False
  #define unicode_required = True
  #define include_cpu_detection = True
  #define sse_required = True
  #define filename_suffix = '_clsid_sse_icl10'
  #define outputdir = '..\..\..\..\'
#endif
#if PREF_YAMAGATA
  #define MSVC80 = True
  #define unicode_required = False
  #define include_audx = True
  #define filename_suffix = '_Q'
#endif
#if PREF_XXL
  #define MSVC80 = False
  #define unicode_required = False
  #define localize = False
  #define include_audx = True
  #define include_info_before = True
  #define include_setup_icon = True
  #define filename_suffix = '_xxl'
#endif
#if PREF_X64
  #define MSVC80 = True
  #define is64bit = True
  #define unicode_required = True
  #define include_app_plugins = False
  #define include_makeavis = False
  #define filename_suffix = '_x64'
#endif

[Setup]
AllowCancelDuringInstall=no
AllowNoIcons=yes
AllowUNCPath=no
#if is64bit
AppId=ffdshow64
#else
AppId=ffdshow
#endif
AppName=ffdshow
AppVerName=ffdshow [rev {#= tryout_revision}] [{#= buildyear}-{#= buildmonth}-{#= buildday}]
AppVersion=1.0
#if is64bit
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
#endif
Compression=lzma/ultra
InternalCompressLevel=ultra
SolidCompression=true
DefaultDirName={code:GetDefaultInstallDir|}
#if is64bit
DefaultGroupName=ffdshow64
#else
DefaultGroupName=ffdshow
#endif
DirExistsWarning=no
#if include_info_before
InfoBeforeFile=infobefore.rtf
#endif
#if include_gnu_license
LicenseFile=gnu_license.txt
#endif
#if MSVC80
  #if unicode_required
MinVersion=0,5.0
  #else
MinVersion=4.1,5.0
  #endif
#else
  #if unicode_required
MinVersion=0,4
  #endif
#endif
OutputBaseFilename=ffdshow_rev{#= tryout_revision}_{#= buildyear}{#= buildmonth}{#= buildday}{#= filename_suffix}
OutputDir={#= outputdir}
PrivilegesRequired=admin
#if include_setup_icon
SetupIconFile=..\modern-wizard_icon.ico
#endif
#if localize
ShowLanguageDialog=yes
#else
ShowLanguageDialog=no
#endif
ShowTasksTreeLines=yes
ShowUndisplayableLanguages=no
UsePreviousTasks=no
VersionInfoCompany=ffdshow
VersionInfoCopyright=GNU
VersionInfoVersion=1.0.0.{#= tryout_revision}
WizardImageFile=MicrosoftModern01.bmp
WizardSmallImageFile=SetupModernSmall26.bmp

[Languages]
Name: en; MessagesFile: compiler:Default.isl
#if localize
Name: ba; MessagesFile: compiler:Languages\Basque.isl
Name: br; MessagesFile: compiler:Languages\BrazilianPortuguese.isl
Name: ca; MessagesFile: compiler:Languages\Catalan.isl
Name: cz; MessagesFile: compiler:Languages\Czech.isl; LicenseFile: ../../../copying.cz.txt
Name: da; MessagesFile: compiler:Languages\Danish.isl
Name: du; MessagesFile: compiler:Languages\Dutch.isl
Name: fi; MessagesFile: compiler:Languages\Finnish.isl
Name: fr; MessagesFile: compiler:Languages\French.isl
Name: de; MessagesFile: compiler:Languages\German.isl; LicenseFile: ../../../copying.de.txt; InfoBeforeFile: infobefore\infobefore.de.rtf
Name: hu; MessagesFile: compiler:Languages\Hungarian.isl
Name: it; MessagesFile: compiler:Languages\Italian.isl
Name: jp; MessagesFile: languages\Japanese.isl; LicenseFile: ../../../copying.jp.txt
Name: no; MessagesFile: compiler:Languages\Norwegian.isl
Name: pl; MessagesFile: compiler:Languages\Polish.isl; LicenseFile: ../../../copying.pl.txt; InfoBeforeFile: infobefore\infobefore.pl.rtf
Name: pr; MessagesFile: compiler:Languages\Portuguese.isl
Name: ru; MessagesFile: compiler:Languages\Russian.isl; LicenseFile: ../../../copying.ru.txt
Name: sk; MessagesFile: compiler:Languages\Slovak.isl; LicenseFile: ../../../copying.sk.txt
Name: sn; MessagesFile: compiler:Languages\Slovenian.isl
Name: sp; MessagesFile: compiler:Languages\Spanish.isl
#endif

[Types]
Name: Normal; Description: Normal; Flags: iscustom

[Components]
Name: ffdshow; Description: {cm:ffdshowds}; Types: Normal
Name: ffdshow\vfw; Description: {cm:vfwinterface}; Types: Normal
#if include_makeavis
Name: ffdshow\makeavis; Description: {cm:makeavis}; Flags: dontinheritcheck
#endif
#if include_app_plugins
Name: ffdshow\plugins; Description: {cm:appplugins}; Flags: dontinheritcheck
Name: ffdshow\plugins\avisynth; Description: AviSynth
Name: ffdshow\plugins\virtualdub; Description: VirtualDub
Name: ffdshow\plugins\dscaler; Description: DScaler
#endif

; CPU detection code
#if include_cpu_detection
#include "innosetup_cpu_detection.iss"
#endif

[Tasks]
Name: resetsettings; Description: {cm:resetsettings}; Flags: unchecked; Components: ffdshow
Name: video; Description: {cm:videoformats}; Flags: unchecked; Components: ffdshow
Name: video\h264; Description: H.264 / AVC; Check: CheckTaskVideo('h264', 1, True); Components: ffdshow
Name: video\h264; Description: H.264 / AVC; Check: NOT CheckTaskVideo('h264', 1, True); Components: ffdshow; Flags: unchecked
Name: video\divx; Description: DivX; Check: CheckTaskVideo2('dx50', True); Components: ffdshow
Name: video\divx; Description: DivX; Check: NOT CheckTaskVideo2('dx50', True); Components: ffdshow; Flags: unchecked
Name: video\xvid; Description: Xvid; Check: CheckTaskVideo2('xvid', True); Components: ffdshow
Name: video\xvid; Description: Xvid; Check: NOT CheckTaskVideo2('xvid', True); Components: ffdshow; Flags: unchecked
Name: video\mpeg4; Description: {cm:genericMpeg4}; Check: CheckTaskVideo2('mp4v', True); Components: ffdshow
Name: video\mpeg4; Description: {cm:genericMpeg4}; Check: NOT CheckTaskVideo2('mp4v', True); Components: ffdshow; Flags: unchecked
Name: video\flv1; Description: FLV1; Check: CheckTaskVideo('flv1', 1, True); Components: ffdshow
Name: video\flv1; Description: FLV1; Check: NOT CheckTaskVideo('flv1', 1, True); Components: ffdshow; Flags: unchecked
Name: video\h263; Description: H.263; Check: CheckTaskVideo('h263', 1, True); Components: ffdshow
Name: video\h263; Description: H.263; Check: NOT CheckTaskVideo('h263', 1, True); Components: ffdshow; Flags: unchecked
Name: video\mpeg1; Description: MPEG-1; Components: ffdshow; Flags: unchecked
Name: video\mpeg1\libmpeg2; Description: libmpeg2; Check: CheckTaskVideo('mpg1', 5, False); Components: ffdshow; Flags: exclusive
Name: video\mpeg1\libmpeg2; Description: libmpeg2; Check: NOT CheckTaskVideo('mpg1', 5, False); Components: ffdshow; Flags: exclusive unchecked
Name: video\mpeg1\libavcodec; Description: libavcodec; Check: CheckTaskVideo('mpg1', 1, False); Components: ffdshow; Flags: exclusive
Name: video\mpeg1\libavcodec; Description: libavcodec; Check: NOT CheckTaskVideo('mpg1', 1, False); Components: ffdshow; Flags: exclusive unchecked
Name: video\mpeg2; Description: MPEG-2; Components: ffdshow; Flags: unchecked
Name: video\mpeg2\libmpeg2; Description: libmpeg2; Check: CheckTaskVideo('mpg2', 5, False); Components: ffdshow; Flags: exclusive
Name: video\mpeg2\libmpeg2; Description: libmpeg2; Check: NOT CheckTaskVideo('mpg2', 5, False); Components: ffdshow; Flags: exclusive unchecked
Name: video\mpeg2\libavcodec; Description: libavcodec; Check: CheckTaskVideo('mpg2', 1, False); Components: ffdshow; Flags: exclusive
Name: video\mpeg2\libavcodec; Description: libavcodec; Check: NOT CheckTaskVideo('mpg2', 1, False); Components: ffdshow; Flags: exclusive unchecked
Name: video\huffyuv; Description: Huffyuv; Check: CheckTaskVideo('hfyu', 1, True); Components: ffdshow
Name: video\huffyuv; Description: Huffyuv; Check: NOT CheckTaskVideo('hfyu', 1, True); Components: ffdshow; Flags: unchecked
Name: video\qt; Description: SVQ1, SVQ3, Cinepak, RPZA, QTRLE; Check: CheckTaskVideo('svq3', 1, True); Components: ffdshow
Name: video\qt; Description: SVQ1, SVQ3, Cinepak, RPZA, QTRLE; Check: NOT CheckTaskVideo('svq3', 1, True); Components: ffdshow; Flags: unchecked
Name: video\vp56; Description: VP5, VP6; Check: CheckTaskVideo('vp6', 1, True); Components: ffdshow
Name: video\vp56; Description: VP5, VP6; Check: NOT CheckTaskVideo('vp6', 1, True); Components: ffdshow; Flags: unchecked
Name: video\wmv1; Description: WMV1; Check: CheckTaskVideo2('wmv1', False); Components: ffdshow
Name: video\wmv1; Description: WMV1; Check: NOT CheckTaskVideo2('wmv1', False); Flags: dontinheritcheck unchecked; Components: ffdshow
Name: video\wmv2; Description: WMV2; Check: CheckTaskVideo2('wmv2', False); Components: ffdshow
Name: video\wmv2; Description: WMV2; Check: NOT CheckTaskVideo2('wmv2', False); Flags: dontinheritcheck unchecked; Components: ffdshow
Name: video\wmv3; Description: WMV3; Check: CheckTaskVideo2('wmv3', False); Components: ffdshow
Name: video\wmv3; Description: WMV3; Check: NOT CheckTaskVideo2('wmv3', False); Flags: dontinheritcheck unchecked; Components: ffdshow
Name: video\wvc1; Description: WVC1; Check: CheckTaskVideo2('wvc1', False); Components: ffdshow
Name: video\wvc1; Description: WVC1; Check: NOT CheckTaskVideo2('wvc1', False); Flags: dontinheritcheck unchecked; Components: ffdshow
Name: video\wvp2; Description: WMVP, WVP2; Check: CheckTaskVideo('wvp2', 12, False); Components: ffdshow
Name: video\wvp2; Description: WMVP, WVP2; Check: NOT CheckTaskVideo('wvp2', 12, False); Flags: dontinheritcheck unchecked; Components: ffdshow
Name: video\mss2; Description: MSS1, MSS2; Check: CheckTaskVideo('mss2', 12, False); Components: ffdshow
Name: video\mss2; Description: MSS1, MSS2; Check: NOT CheckTaskVideo('mss2', 12, False); Flags: dontinheritcheck unchecked; Components: ffdshow
Name: video\dvsd; Description: DV; Check: CheckTaskVideo('dvsd', 1, False); Components: ffdshow
Name: video\dvsd; Description: DV; Check: NOT CheckTaskVideo('dvsd', 1, False); Flags: dontinheritcheck unchecked; Components: ffdshow
Name: video\other1; Description: H.261, MJPEG, Theora, VP3; Check: NOT IsUpdate; Components: ffdshow
Name: video\other2; Description: CorePNG, MS Video 1, MSRLE, Techsmith, Truemotion; Check: NOT IsUpdate; Components: ffdshow
Name: video\other3; Description: ASV1/2, CYUV, ZLIB, 8BPS, LOCO, MSZH, QPEG, WNV1, VCR1; Check: NOT IsUpdate; Flags: unchecked; Components: ffdshow
Name: video\other4; Description: CamStudio, ZMBV, Ultimotion, VIXL, AASC, IV32, FPS1, RT21; Check: NOT IsUpdate; Flags: unchecked; Components: ffdshow
Name: video\rawv; Description: {cm:rawvideo}; Check: CheckTaskVideo('rawv', 1, False); Flags: dontinheritcheck; Components: ffdshow
Name: video\rawv; Description: {cm:rawvideo}; Check: NOT CheckTaskVideo('rawv', 1, False); Flags: dontinheritcheck unchecked; Components: ffdshow
Name: audio; Description: {cm:audioformats}; Flags: unchecked; Components: ffdshow
Name: audio\mp3; Description: MP3; Check: CheckTaskAudio('mp3', 7, True); Components: ffdshow
Name: audio\mp3; Description: MP3; Check: NOT CheckTaskAudio('mp3', 7, True); Flags: unchecked; Components: ffdshow
Name: audio\aac; Description: AAC; Flags: unchecked; Components: ffdshow
Name: audio\aac\libfaad2; Description: libfaad2; Check: CheckTaskAudio('aac', 8, True); Flags: exclusive; Components: ffdshow
Name: audio\aac\libfaad2; Description: libfaad2; Check: NOT CheckTaskAudio('aac', 8, True); Flags: exclusive unchecked; Components: ffdshow
Name: audio\aac\realaac; Description: realaac; Check: CheckTaskAudio('aac', 19, False); Flags: exclusive; Components: ffdshow
Name: audio\aac\realaac; Description: realaac; Check: NOT CheckTaskAudio('aac', 19, False); Flags: exclusive unchecked; Components: ffdshow
Name: audio\ac3; Description: AC3; Check: CheckTaskAudio('ac3', 15, True); Components: ffdshow
Name: audio\ac3; Description: AC3; Check: NOT CheckTaskAudio('ac3', 15, True); Flags: unchecked; Components: ffdshow
Name: audio\dts; Description: DTS; Check: CheckTaskAudio('dts', 17, True); Components: ffdshow
Name: audio\dts; Description: DTS; Check: NOT CheckTaskAudio('dts', 17, True); Flags: unchecked; Components: ffdshow
Name: audio\lpcm; Description: LPCM; Check: CheckTaskAudio('lpcm', 4, True); Components: ffdshow
Name: audio\lpcm; Description: LPCM; Check: NOT CheckTaskAudio('lpcm', 4, True); Flags: unchecked; Components: ffdshow
Name: audio\mp2; Description: MP1, MP2; Check: CheckTaskAudio('mp2', 7, True); Components: ffdshow
Name: audio\mp2; Description: MP1, MP2; Check: NOT CheckTaskAudio('mp2', 7, True); Flags: unchecked; Components: ffdshow
Name: audio\vorbis; Description: Vorbis; Flags: unchecked; Components: ffdshow
Name: audio\vorbis\tremor; Description: tremor; Check: CheckTaskAudio('vorbis', 18, True); Flags: exclusive; Components: ffdshow
Name: audio\vorbis\tremor; Description: tremor; Check: NOT CheckTaskAudio('vorbis', 18, True); Flags: exclusive unchecked; Components: ffdshow
Name: audio\vorbis\libavcodec; Description: libavcodec; Check: CheckTaskAudio('vorbis', 1, False); Flags: exclusive; Components: ffdshow
Name: audio\vorbis\libavcodec; Description: libavcodec; Check: NOT CheckTaskAudio('vorbis', 1, False); Flags: exclusive unchecked; Components: ffdshow
Name: audio\flac; Description: FLAC; Check: CheckTaskAudio('flac', 1, False); Components: ffdshow
Name: audio\flac; Description: FLAC; Check: NOT CheckTaskAudio('flac', 1, False); Flags: unchecked; Components: ffdshow
Name: audio\tta; Description: True Audio; Check: CheckTaskAudio('tta', 1, True); Components: ffdshow
Name: audio\tta; Description: True Audio; Check: NOT CheckTaskAudio('tta', 1, True); Flags: unchecked; Components: ffdshow
Name: audio\amr; Description: AMR; Check: CheckTaskAudio('amr', 1, True); Components: ffdshow
Name: audio\amr; Description: AMR; Check: NOT CheckTaskAudio('amr', 1, True); Flags: unchecked; Components: ffdshow
Name: audio\qt; Description: QDM2, MACE; Check: CheckTaskAudio('qdm2', 1, True); Components: ffdshow
Name: audio\qt; Description: QDM2, MACE; Check: NOT CheckTaskAudio('qdm2', 1, True); Flags: unchecked; Components: ffdshow
Name: audio\adpcm; Description: ADPCM, MS GSM, Truespeech; Check: NOT IsUpdate; Flags: unchecked; Components: ffdshow
Name: audio\rawa; Description: {cm:rawaudio}; Check: CheckTaskAudio('rawa', 4, False); Flags: dontinheritcheck; Components: ffdshow
Name: audio\rawa; Description: {cm:rawaudio}; Check: NOT CheckTaskAudio('rawa', 4, False); Flags: dontinheritcheck unchecked; Components: ffdshow
Name: filter; Description: {cm:defaultfilters}; Flags: unchecked; Components: ffdshow
Name: filter\normalize; Description: {cm:volumenorm}; Check:     GetTaskVolNormalize(); Components: ffdshow
Name: filter\normalize; Description: {cm:volumenorm}; Check: NOT GetTaskVolNormalize(); Components: ffdshow; Flags: unchecked
Name: filter\subtitles; Description: {cm:subtitles};  Check:     CheckTaskVideoInpreset('issubtitles', 1, False); Components: ffdshow
Name: filter\subtitles; Description: {cm:subtitles};  Check: NOT CheckTaskVideoInpreset('issubtitles', 1, False); Components: ffdshow; Flags: unchecked;
#if !PREF_YAMAGATA
Name: tweaks; Description: {cm:tweaks}; Check: NOT IsUpdate; Flags: unchecked; Components: ffdshow
Name: tweaks\skipinloop; Description: {cm:skipinloop}; Check: NOT IsUpdate; Flags: unchecked; Components: ffdshow
#endif

[Icons]
Name: {group}\{cm:audioconfig}; Filename: rundll32.exe; Parameters: ffdshow.ax,configureAudio; WorkingDir: {app}; IconFilename: {app}\ffdshow.ax; Check: NOT CheckModernIcon ; IconIndex: 1; Components: ffdshow
Name: {group}\{cm:audioconfig}; Filename: rundll32.exe; Parameters: ffdshow.ax,configureAudio; WorkingDir: {app}; IconFilename: {app}\ffdshow.ax; Check:     CheckModernIcon ; IconIndex: 4; Components: ffdshow
Name: {group}\{cm:videoconfig}; Filename: rundll32.exe; Parameters: ffdshow.ax,configure;      WorkingDir: {app}; IconFilename: {app}\ffdshow.ax; Check: NOT CheckModernIcon ; IconIndex: 0; Components: ffdshow
Name: {group}\{cm:videoconfig}; Filename: rundll32.exe; Parameters: ffdshow.ax,configure;      WorkingDir: {app}; IconFilename: {app}\ffdshow.ax; Check:     CheckModernIcon ; IconIndex: 3; Components: ffdshow
#if is64bit
Name: {group}\{cm:vfwconfig}; Filename: rundll32.exe;   Parameters: ff_vfw.dll,configureVFW;   WorkingDir: {app}; IconFilename: {app}\ffdshow.ax; Check: NOT CheckModernIcon ; IconIndex: 2; Components: ffdshow\vfw
Name: {group}\{cm:vfwconfig}; Filename: rundll32.exe;   Parameters: ff_vfw.dll,configureVFW;   WorkingDir: {app}; IconFilename: {app}\ffdshow.ax; Check:     CheckModernIcon ; IconIndex: 5; Components: ffdshow\vfw
#else
Name: {group}\{cm:vfwconfig}; Filename: rundll32.exe;   Parameters: ff_vfw.dll,configureVFW;                      IconFilename: {app}\ffdshow.ax; Check: NOT CheckModernIcon ; IconIndex: 2; Components: ffdshow\vfw
Name: {group}\{cm:vfwconfig}; Filename: rundll32.exe;   Parameters: ff_vfw.dll,configureVFW;                      IconFilename: {app}\ffdshow.ax; Check:     CheckModernIcon ; IconIndex: 5; Components: ffdshow\vfw
#endif
#if include_makeavis
Name: {group}\makeAVIS; Filename: {app}\makeAVIS.exe; Components: ffdshow\makeavis
#endif
Name: {group}\{cm:uninstall}; Filename: {uninstallexe}

[Files]
; For speaker config
Source: msvc71\ffSpkCfg.dll; Flags: dontcopy

; MSVC71 runtimes are required for ffdshow components that are placed outside the ffdshow installation directory.
#if !is64bit
Source: Runtimes\msvc71\msvcp71.dll; DestDir: {sys}; Flags: onlyifdoesntexist sharedfile uninsnosharedfileprompt
Source: Runtimes\msvc71\msvcr71.dll; DestDir: {sys}; Flags: onlyifdoesntexist sharedfile uninsnosharedfileprompt
#endif

#if MSVC80
; Install MSVC80 runtime as private assembly (can only be used by components that are in the same directory).
  #if is64bit
Source: Runtimes\msvc80_x64\msvcr80.dll; DestDir: {app}; MinVersion: 0,5.02; Flags: ignoreversion restartreplace uninsrestartdelete
Source: Runtimes\msvc80_x64\microsoft.vc80.crt.manifest; DestDir: {app}; MinVersion: 0,5.02; Flags: ignoreversion restartreplace uninsrestartdelete
  #else
Source: Runtimes\msvc80\msvcr80.dll; DestDir: {app}; Flags: ignoreversion restartreplace uninsrestartdelete
Source: Runtimes\msvc80\microsoft.vc80.crt.manifest; DestDir: {app}; Flags: ignoreversion restartreplace uninsrestartdelete
  #endif
#endif

Source: ..\..\libavcodec.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
Source: ..\..\libmplayer.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
Source: ..\..\ff_realaac.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
Source: ..\..\ff_liba52.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
Source: ..\..\ff_libdts.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
Source: ..\..\ff_libfaad2.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
Source: ..\..\ff_libmad.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
Source: ..\..\ff_tremor.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
Source: ..\..\ff_unrar.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
Source: ..\..\ff_samplerate.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
Source: ..\..\ff_theora.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
#if include_x264
Source: ..\..\ff_x264.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow\vfw
Source: Runtimes\pthreadGC2.dll; DestDir: {sys}; Flags: sharedfile uninsnosharedfileprompt; Components: ffdshow\vfw
#endif
#if include_xvidcore
Source: ..\..\xvidcore.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
#endif
#if is64bit
Source: ..\..\ff_kernelDeint.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
Source: ..\..\TomsMoComp_ff.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
#else
Source: icl10\ff_kernelDeint.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
Source: icl10\TomsMoComp_ff.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
#endif
Source: ..\..\libmpeg2_ff.dll; DestDir: {app}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow

; Single build:
#if !PREF_CLSID && !PREF_CLSID_ICL
Source: ..\..\ffdshow.ax; DestDir: {app}; Flags: ignoreversion regserver restartreplace uninsrestartdelete noregerror; Components: ffdshow
#endif
; ANSI + Unicode:
#if PREF_CLSID
Source: ..\..\ffdshow_ansi.ax; DestName: ffdshow.ax; DestDir: {app}; Flags: ignoreversion regserver restartreplace uninsrestartdelete; MinVersion: 4,0; Components: ffdshow
Source: ..\..\ffdshow_unicode.ax; DestName: ffdshow.ax; DestDir: {app}; Flags: ignoreversion regserver restartreplace uninsrestartdelete noregerror; MinVersion: 0,4; Components: ffdshow
#endif
#if PREF_CLSID_ICL
Source: ..\..\ffdshow_icl.ax; DestName: ffdshow.ax; DestDir: {app}; Flags: ignoreversion regserver restartreplace uninsrestartdelete noregerror; MinVersion: 0,4; Components: ffdshow
#endif
; Multi build example (requires cpu detection to be enabled):
;Source: ..\..\ffdshow_generic.ax; DestName: ffdshow.ax; DestDir: {app}; Flags: ignoreversion regserver restartreplace uninsrestartdelete; Check: Is_MMX_Supported AND NOT Is_SSE_Supported; Components: ffdshow
;Source: ..\..\ffdshow_sse.ax; DestName: ffdshow.ax; DestDir: {app}; Flags: ignoreversion regserver restartreplace uninsrestartdelete; Check: Is_SSE_Supported AND NOT Is_SSE2_Supported; Components: ffdshow
;Source: ..\..\ffdshow_sse2.ax; DestName: ffdshow.ax; DestDir: {app}; Flags: ignoreversion regserver restartreplace uninsrestartdelete; Check: Is_SSE2_Supported; Components: ffdshow

#if MSVC80
	#if is64bit
Source: ..\..\manifest64\ffdshow.ax.manifest; DestDir: {app}; Flags: ignoreversion restartreplace uninsrestartdelete; MinVersion: 0,5.01; OnlyBelowVersion: 0,5.03
	#else
Source: ..\..\manifest32\msvc80\ffdshow.ax.manifest; DestDir: {app}; Flags: ignoreversion restartreplace uninsrestartdelete; MinVersion: 0,5.01; OnlyBelowVersion: 0,5.02
	#endif
#else
Source: ..\..\manifest32\ffdshow.ax.manifest; DestDir: {app}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow
#endif

; Single build:
#if !PREF_CLSID
Source: ..\..\ff_wmv9.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
#endif
; ANSI + Unicode:
#if PREF_CLSID
Source: ..\..\ff_wmv9_ansi.dll; DestName: ff_wmv9.dll; DestDir: {app}; Flags: ignoreversion; MinVersion: 4,0; Components: ffdshow
Source: ..\..\ff_wmv9_unicode.dll; DestName: ff_wmv9.dll; DestDir: {app}; Flags: ignoreversion; MinVersion: 0,4; Components: ffdshow
#endif

#if is64bit
Source: ..\..\ff_vfw.dll; DestDir: {app}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\vfw
#else
; If you use MSVC8 for ffdshow.ax, ff_vfw.dll should be compiled by GCC. Both MSVC7.1 and MSVC8 does not work in some environment such as Windows XP SP2 without shared assembly of MSVCR80.
Source: ..\..\ff_vfw.dll; DestDir: {sys}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\vfw
#endif

#if MSVC80
	#if is64bit
Source: ..\..\manifest64\ff_vfw.dll.manifest; DestDir: {app}; Flags: ignoreversion restartreplace uninsrestartdelete; MinVersion: 0,5.01; OnlyBelowVersion: 0,5.03	
	#else
Source: ..\..\manifest32\msvc80\ff_vfw.dll.manifest; DestDir: {app}; Flags: ignoreversion restartreplace uninsrestartdelete; MinVersion: 0,5.01; OnlyBelowVersion: 0,5.02	
	#endif
#else
Source: ..\..\manifest32\ff_vfw.dll.manifest; DestDir: {sys}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\vfw
#endif

#if include_audx
Source: audxlib.dll; DestDir: {app}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow
#endif

#if include_app_plugins
  #if MSVC80
Source: msvc71\ffavisynth.dll; DestDir: {code:GetAviSynthPluginDir}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\plugins\avisynth
  #else
Source: ..\..\ffavisynth.dll; DestDir: {code:GetAviSynthPluginDir}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\plugins\avisynth
  #endif
  #if MSVC80
Source: msvc71\ffvdub.vdf; DestDir: {code:GetVdubPluginDir}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\plugins\virtualdub
  #else
Source: ..\..\ffvdub.vdf; DestDir: {code:GetVdubPluginDir}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\plugins\virtualdub
  #endif
  #if MSVC80
Source: msvc71\FLT_ffdshow.dll; DestDir: {code:GetDScalerDir|}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\plugins\dscaler
  #else
Source: ..\..\FLT_ffdshow.dll; DestDir: {code:GetDScalerDir|}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\plugins\dscaler
  #endif
#endif

#if include_makeavis
Source: ..\..\makeAVIS.exe; DestDir: {app}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\makeavis
  #if !MSVC80
Source: ..\..\manifest32\makeAVIS.exe.manifest; DestDir: {app}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\makeavis
  #endif
  #if MSVC80
Source: msvc71\ff_acm.acm; DestDir: {sys}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\makeavis
  #else
Source: ..\..\ff_acm.acm; DestDir: {sys}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\makeavis
  #endif
#endif

Source: ..\..\languages\*.*; DestDir: {app}\languages; Flags: ignoreversion; Components: ffdshow
Source: ..\..\custom matrices\*.*; DestDir: {app}\custom matrices; Flags: ignoreversion; Components: ffdshow\vfw
Source: ..\..\openIE.js; DestDir: {app}; Flags: ignoreversion; Components: ffdshow

[InstallDelete]
#if MSVC80
Type: files; Name: {app}\ffdshow.ax.manifest; Components: ffdshow
	#if include_makeavis
Type: files; Name: {app}\makeAVIS.exe.manifest; Components: ffdshow\makeavis
	#endif
#endif
#if localize
; Localized shortcuts
Type: files; Name: {group}\Video decoder configuration.lnk; Components: ffdshow
Type: files; Name: {group}\Audio decoder configuration.lnk; Components: ffdshow
Type: files; Name: {group}\Uninstall ffdshow.lnk; Components: ffdshow
Type: files; Name: {group}\VFW configuration.lnk; Components: ffdshow\vfw
#endif
; Shortcuts belonging to old NSIS installers
Type: files; Name: {group}\VFW codec configuration.lnk; Components: ffdshow\vfw
Type: files; Name: {group}\Uninstall.lnk; Components: ffdshow

[Registry]
; Cleanup of settings
Root: HKCU; Subkey: Software\GNU; Flags: uninsdeletekeyifempty; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow; Flags: uninsdeletekey; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio; Flags: uninsdeletekey; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio_raw; Flags: uninsdeletekey; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_enc; Flags: uninsdeletekey; Components: ffdshow\vfw
Root: HKCU; Subkey: Software\GNU\ffdshow_vfw; Flags: uninsdeletekey; Components: ffdshow\vfw
Root: HKLM; Subkey: Software\GNU; Flags: uninsdeletekeyifempty; Components: ffdshow
Root: HKLM; Subkey: Software\GNU\ffdshow; Flags: uninsdeletekey; Components: ffdshow
Root: HKLM; Subkey: Software\GNU\ffdshow_audio; Flags: uninsdeletekey; Components: ffdshow
Root: HKLM; Subkey: Software\GNU\ffdshow_enc; Flags: uninsdeletekey; Components: ffdshow\vfw
Root: HKLM; Subkey: Software\GNU\ffdshow_vfw; Flags: uninsdeletekey; Components: ffdshow\vfw

; Reset settings
Root: HKCU; Subkey: Software\GNU\ffdshow; Flags: deletekey; Components: ffdshow; Tasks: resetsettings
Root: HKCU; Subkey: Software\GNU\ffdshow_audio; Flags: deletekey; Components: ffdshow; Tasks: resetsettings
Root: HKCU; Subkey: Software\GNU\ffdshow_audio_raw; Flags: deletekey; Components: ffdshow; Tasks: resetsettings
Root: HKCU; Subkey: Software\GNU\ffdshow_enc; Flags: deletekey; Components: ffdshow\vfw; Tasks: resetsettings
Root: HKCU; Subkey: Software\GNU\ffdshow_vfw; Flags: deletekey; Components: ffdshow\vfw; Tasks: resetsettings
Root: HKLM; Subkey: Software\GNU\ffdshow; Flags: deletekey; Components: ffdshow; Tasks: resetsettings
Root: HKLM; Subkey: Software\GNU\ffdshow_audio; Flags: deletekey; Components: ffdshow; Tasks: resetsettings
Root: HKLM; Subkey: Software\GNU\ffdshow_enc; Flags: deletekey; Components: ffdshow\vfw; Tasks: resetsettings
Root: HKLM; Subkey: Software\GNU\ffdshow_vfw; Flags: deletekey; Components: ffdshow\vfw; Tasks: resetsettings

; Path
Root: HKLM; Subkey: Software\GNU\ffdshow; ValueType: string; ValueName: pth; ValueData: {app}; Components: ffdshow
Root: HKLM; Subkey: Software\GNU\ffdshow; ValueName: pthPriority; Flags: deletevalue; Components: ffdshow
#if include_app_plugins
Root: HKLM; SubKey: Software\GNU\ffdshow; ValueType: string; ValueName: pthAvisynth; ValueData: {code:GetAviSynthPluginDir}; Flags: uninsclearvalue; Components: ffdshow\plugins\avisynth
Root: HKLM; SubKey: Software\GNU\ffdshow; ValueType: string; ValueName: pthVirtualDub; ValueData: {code:GetVdubPluginDir}; Flags: uninsclearvalue; Components: ffdshow\plugins\virtualdub
Root: HKLM; SubKey: Software\GNU\ffdshow; ValueType: string; ValueName: dscalerPth; ValueData: {code:GetDScalerDir|}; Flags: uninsclearvalue; Components: ffdshow\plugins\dscaler
#endif

; Version info
Root: HKLM; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: revision; ValueData: {#= tryout_revision}; Components: ffdshow
Root: HKLM; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: builddate; ValueData: {#= buildyear}{#= buildmonth}{#= buildday}; Components: ffdshow

; Language
#if localize
Root: HKLM; Subkey: Software\GNU\ffdshow; ValueType: string; ValueName: lang; ValueData: {cm:langid}; Components: ffdshow
#endif

; Register VFW interface
#if is64bit
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers.desc; ValueType: string; ValueName: {app}\ff_vfw.dll; ValueData: ffdshow video encoder; MinVersion: 0,4; Flags: uninsdeletevalue; Components: ffdshow\vfw
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Drivers32; ValueType: string; ValueName: VIDC.FFDS; ValueData: {code:GetVFWLocation|}; MinVersion: 0,4; Flags: uninsdeletevalue; Components: ffdshow\vfw
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.FFDS; Flags: uninsdeletekey; Components: ffdshow\vfw
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.FFDS; ValueType: string; ValueName: Description; ValueData: ffdshow video encoder; Components: ffdshow\vfw
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.FFDS; ValueType: string; ValueName: Driver; ValueData: {code:GetVFWLocation|}; Components: ffdshow\vfw
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.FFDS; ValueType: string; ValueName: FriendlyName; ValueData: ffdshow video encoder; Components: ffdshow\vfw
#else
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers.desc; ValueType: string; ValueName: ff_vfw.dll; ValueData: ffdshow video encoder; MinVersion: 0,4; Flags: uninsdeletevalue; Components: ffdshow\vfw
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Drivers32; ValueType: string; ValueName: VIDC.FFDS; ValueData: ff_vfw.dll; MinVersion: 0,4; Flags: uninsdeletevalue; Components: ffdshow\vfw
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.FFDS; Flags: uninsdeletekey; Components: ffdshow\vfw
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.FFDS; ValueType: string; ValueName: Description; ValueData: ffdshow video encoder; Components: ffdshow\vfw
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.FFDS; ValueType: string; ValueName: Driver; ValueData: ff_vfw.dll; Components: ffdshow\vfw
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.FFDS; ValueType: string; ValueName: FriendlyName; ValueData: ffdshow video encoder; Components: ffdshow\vfw
#endif

#if include_makeavis
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers.desc; ValueType: string; ValueName: ff_acm.acm; ValueData: ffdshow ACM codec; MinVersion: 0,4; Flags: uninsdeletevalue; Components: ffdshow\makeavis
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Drivers32; ValueType: string; ValueName: msacm.avis; ValueData: ff_acm.acm; MinVersion: 0,4; Flags: uninsdeletevalue; Components: ffdshow\makeavis
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\acm\msacm.avis; Flags: uninsdeletekey; Components: ffdshow\makeavis
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\acm\msacm.avis; ValueType: string; ValueName: Description; ValueData: ffdshow ACM codec; Components: ffdshow\makeavis
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\acm\msacm.avis; ValueType: string; ValueName: Driver; ValueData: ff_acm.acm; Components: ffdshow\makeavis
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\acm\msacm.avis; ValueType: string; ValueName: FriendlyName; ValueData: ffdshow ACM codec; Components: ffdshow\makeavis
#endif

; Recommended settings
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: allowOutChange; ValueData: 2; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: hwOverlay; ValueData: 2; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: idct; ValueData: 0; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: outChangeCompatOnly; ValueData: 1; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: postprocH264mode; ValueData: 0; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: resizeMethod; ValueData: 9; Flags: createvalueifdoesntexist; Components: ffdshow

#if !PREF_YAMAGATA
Root: HKCU; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: fastH264; ValueData: 0; Components: ffdshow; Tasks: NOT tweaks\skipinloop; Flags: createvalueifdoesntexist
Root: HKCU; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: fastH264; ValueData: 2; Components: ffdshow; Tasks:     tweaks\skipinloop;
#endif

Root: HKCU; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: subTextpin; ValueData: 1; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: subTextpinSSA; ValueData: 1; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: subIsExpand; ValueData: 0; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: isSubtitles; ValueData: 0; Components: ffdshow; Tasks: NOT filter\subtitles
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: isSubtitles; ValueData: 1; Components: ffdshow; Tasks: filter\subtitles

Root: HKCU; Subkey: Software\GNU\ffdshow_audio\default; ValueType: dword; ValueName: mixerNormalizeMatrix; ValueData: 0; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio\default; ValueType: dword; ValueName: volNormalize; ValueData: 0; Components: ffdshow; Tasks: NOT filter\normalize
Root: HKCU; Subkey: Software\GNU\ffdshow_audio\default; ValueType: dword; ValueName: isvolume;     ValueData: 1; Components: ffdshow; Tasks:     filter\normalize
Root: HKCU; Subkey: Software\GNU\ffdshow_audio\default; ValueType: dword; ValueName: volNormalize; ValueData: 1; Components: ffdshow; Tasks:     filter\normalize

Root: HKCU; Subkey: Software\GNU\ffdshow_audio\default; ValueType: dword; ValueName: ismixer; ValueData: {code:Get_ismixer}; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio\default; ValueType: dword; ValueName: mixerOut; ValueData: {code:Get_mixerOut}; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio\default; ValueType: dword; ValueName: mixerExpandStereo; ValueData: {code:Get_mixerExpandStereo}; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio\default; ValueType: dword; ValueName: mixerVoiceControl; ValueData: {code:Get_mixerVoiceControl}; Components: ffdshow

Root: HKLM; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: allowOutChange; ValueData: 2; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKLM; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: hwOverlay; ValueData: 2; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKLM; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: outChangeCompatOnly; ValueData: 1; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKLM; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: postprocH264mode; ValueData: 0; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKLM; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: idct; ValueData: 0; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKLM; SubKey: Software\GNU\ffdshow; ValueType: dword; ValueName: libtheoraPostproc; ValueData: 0; Flags: createvalueifdoesntexist; Components: ffdshow

; Blacklist
Root: HKCU; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: isBlacklist; ValueData: 1; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow; ValueType: String; ValueName: blacklist; ValueData: "oblivion.exe;morrowind.exe;"; Flags: createvalueifdoesntexist; OnlyBelowVersion: 0,6; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow; ValueType: String; ValueName: blacklist; ValueData: "oblivion.exe;morrowind.exe;"; Flags: createvalueifdoesntexist; MinVersion: 0,6; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio; ValueType: dword; ValueName: isBlacklist; ValueData: 1; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio; ValueType: String; ValueName: blacklist; ValueData: "oblivion.exe;morrowind.exe"; Flags: createvalueifdoesntexist; OnlyBelowVersion: 0,6; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio; ValueType: String; ValueName: blacklist; ValueData: "oblivion.exe;morrowind.exe"; Flags: createvalueifdoesntexist; MinVersion: 0,6; Components: ffdshow

; Compatibility list
Root: HKCU; Subkey: Software\GNU\ffdshow;       ValueType: dword;  ValueName: isUseonlyin; ValueData: {code:Get_isUseonlyinVideo}; Check: IsCompVValid; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow;       ValueType: String; ValueName: useonlyin;   ValueData: {code:Get_useonlyinVideo};                        Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio; ValueType: dword;  ValueName: isUseonlyin; ValueData: {code:Get_isUseonlyinAudio}; Check: IsCompAValid; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio; ValueType: String; ValueName: useonlyin;   ValueData: {code:Get_useonlyinAudio};                        Components: ffdshow

Root: HKCU; Subkey: Software\GNU\ffdshow;       ValueType: dword;  ValueName: dontaskComp; ValueData: {code:Get_isCompVdontask};   Check: IsCompVValid; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio; ValueType: dword;  ValueName: dontaskComp; ValueData: {code:Get_isCompAdontask};   Check: IsCompAValid; Components: ffdshow

; Registry keys for the audio/video formats:
#include "reg_formats.iss"

[INI]
#if !unicode_required
Filename: {win}\system.ini; Section: drivers32; Key: vidc.ffds; String: ff_vfw.dll; Flags: uninsdeleteentry; MinVersion: 4,0; Components: ffdshow\vfw
  #if include_makeavis
Filename: {win}\system.ini; Section: drivers32; Key: msacm.avis; String: ff_acm.acm; Flags: uninsdeleteentry; MinVersion: 4,0; Components: ffdshow\makeavis
  #endif
#endif

[Run]
Description: {cm:runaudioconfig}; Filename: rundll32.exe; Parameters: ffdshow.ax,configureAudio; WorkingDir: {app}; Flags: postinstall nowait unchecked; Components: ffdshow
Description: {cm:runvideoconfig}; Filename: rundll32.exe; Parameters: ffdshow.ax,configure; WorkingDir: {app}; Flags: postinstall nowait unchecked; Components: ffdshow
#if is64bit
Description: {cm:runvfwconfig}; Filename: rundll32.exe; Parameters: ff_vfw.dll,configureVFW; WorkingDir: {app}; Flags: postinstall nowait unchecked; Components: ffdshow\vfw
#else
Description: {cm:runvfwconfig}; Filename: rundll32.exe; Parameters: ff_vfw.dll,configureVFW; Flags: postinstall nowait unchecked; Components: ffdshow\vfw
#endif

; All custom strings in the installer:
#include "custom_messages.iss"

[Code]
const NUMBER_OF_COMPATIBLEAPPLICATIONS=250;
type
  TCompApp = record
    rev: Integer;  // The application (name) have been added to the compatibility list at this rev.
    name: String;
  end;

  TcomplistPage = record
    page: TInputOptionWizardPage;
    edt: TMemo;
    chbDontAsk: TCheckBox;
    skipped: Boolean;
    countAdded: Integer;
  end;

// Global vars
var
  reg_mixerOut: Cardinal;
  reg_ismixer: Cardinal;

  ComplistVideo: TcomplistPage;
  ComplistAudio: TcomplistPage;
  Complist_isMsgAddedShown: Boolean;

  SpeakerPage: TInputOptionWizardPage;
  chbVoicecontrol: TCheckBox;
  chbExpandStereo: TCheckBox;
  is8DisableMixer: Boolean;

  priorPageID: Integer;  // to be used "Don't ask me again" in compatibility list, so that audio and video config can link.
  compApps : array[1..NUMBER_OF_COMPATIBLEAPPLICATIONS] of TCompApp;

function CheckTaskVideo(name: String; value: Integer; showbydefault: Boolean): Boolean;
var
  regval: Cardinal;
begin
  Result := False;
  if RegQueryDwordValue(HKCU, 'Software\GNU\ffdshow', name, regval) then begin
    Result := (regval = value);
  end
  else begin
    if RegQueryDwordValue(HKLM, 'Software\GNU\ffdshow', name, regval) then begin
      Result := (regval = value);
    end
    else begin
      Result := showbydefault;
    end
  end
end;

function CheckTaskVideo2(name: String; showbydefault: Boolean): Boolean;
var
  regval: Cardinal;
begin
  Result := False;
  if RegQueryDwordValue(HKCU, 'Software\GNU\ffdshow', name, regval) then begin
    Result := (regval > 0);
  end
  else begin
    if RegQueryDwordValue(HKLM, 'Software\GNU\ffdshow', name, regval) then begin
      Result := (regval > 0);
    end
    else begin
      Result := showbydefault;
    end
  end
end;

function CheckTaskAudio(name: String; value: Integer; showbydefault: Boolean): Boolean;
var
  regval: Cardinal;
begin
  Result := False;
  if RegQueryDwordValue(HKCU, 'Software\GNU\ffdshow_audio', name, regval) then begin
    Result := (regval = value);
  end
  else begin
    if RegQueryDwordValue(HKLM, 'Software\GNU\ffdshow_audio', name, regval) then begin
      Result := (regval = value);
    end
    else begin
      Result := showbydefault;
    end
  end
end;

function CheckTaskVideoInpreset(name: String; value: Integer; showbydefault: Boolean): Boolean;
var
  regval: Cardinal;
begin
  Result := False;
  if RegQueryDwordValue(HKCU, 'Software\GNU\ffdshow\default', name, regval) then
    Result := (regval = value)
  else
    Result := showbydefault;
end;

function CheckTaskAudioInpreset(name: String; value: Integer; showbydefault: Boolean): Boolean;
var
  regval: Cardinal;
begin
  Result := False;
  if RegQueryDwordValue(HKCU, 'Software\GNU\ffdshow_audio\default', name, regval) then
    Result := (regval = value)
  else
    Result := showbydefault;
end;

function CheckModernIcon(): Boolean;
var
  regVal: Cardinal;
begin
  Result := True;
  if RegQueryDwordValue(HKCU, 'Software\GNU\ffdshow', 'trayIconType', regVal) AND (regVal = 2) then begin
    Result := False;
  end
end;

function GetTaskVolNormalize(): Boolean;
begin
  Result := False;
  if CheckTaskAudioInpreset('isvolume', 1, False) then
    if CheckTaskAudioInpreset('volNormalize', 1, False) then
     Result := True;
end;

#if is64bit
function GetVFWLocation(dummy: String): String;
begin
  Result := GetShortName(ExpandConstant('{app}\ff_vfw.dll'));
end;
#endif

#if include_app_plugins
// Global vars
var
  avisynthplugindir: String;
  dscalerdir: String;
  VdubDirPage: TInputDirWizardPage;

function GetAviSynthPluginDir(dummy: String): String;
begin
  if Length(avisynthplugindir) = 0 then begin
    if NOT RegQueryStringValue(HKLM, 'Software\AviSynth', 'plugindir2_5', avisynthplugindir) OR NOT DirExists(avisynthplugindir) then begin
      if NOT RegQueryStringValue(HKLM, 'Software\AviSynth', 'plugindir', avisynthplugindir) OR NOT DirExists(avisynthplugindir) then begin
        avisynthplugindir := ExpandConstant('{app}');
      end
    end
  end

  Result := avisynthplugindir;
end;

function GetDScalerDir(dummy: String): String;
var
  proglist: Array of String;
  i: Integer;
  temp : String;
begin
  if Length(dscalerdir) = 0 then begin
    dscalerdir := ExpandConstant('{app}');
    if RegGetSubkeyNames(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall', proglist) then begin
      for i:=0 to (GetArrayLength(proglist) - 1) do begin
        if Pos('dscaler', Lowercase(proglist[i])) > 0 then begin
          if RegQueryStringValue(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\' + proglist[i], 'Inno Setup: App Path', temp) AND DirExists(temp) then begin
            dscalerdir := temp;
            Break;
          end
        end
      end
    end
  end
  Result := dscalerdir;
end;

function GetVdubPluginDir(dummy: String): String;
begin
  Result := VdubDirPage.Values[0];
end;
#endif

function GetDefaultInstallDir(dummy: String): String;
begin
  if NOT RegQueryStringValue(HKLM, 'Software\GNU\ffdshow', 'pth', Result) OR (Length(Result) = 0) OR NOT DirExists(Result) then begin
    #if is64bit
    Result := ExpandConstant('{pf}\ffdshow64');
    #else
    Result := ExpandConstant('{pf}\ffdshow');
    #endif
  end
end;

var
  is_update: Boolean;

function IsUpdate(): Boolean;
begin
  Result := is_update;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if CurPageID = wpSelectComponents then begin
    if NOT IsComponentSelected('ffdshow') then begin
      msgbox('You must select at least one component.', mbInformation, MB_OK);
      Result := False;
    end
  end
  if CurPageID = ComplistVideo.page.ID then begin
    ComplistAudio.chbDontAsk.Checked := ComplistVideo.chbDontAsk.Checked;
  end
end;

function BackButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if CurPageID = ComplistAudio.page.ID then begin
    ComplistVideo.chbDontAsk.Checked := ComplistAudio.chbDontAsk.Checked;
  end
end;

procedure RemoveBuildUsingNSIS();
var
  regval: String;
  dword: Cardinal;
begin
  if RegKeyExists(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\ffdshow') then begin
    // Remove uninstall.exe
    if RegQueryStringValue(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\ffdshow', 'UninstallString', regval) then begin
      if FileExists(RemoveQuotes(regval)) then begin
        DeleteFile(RemoveQuotes(regval));
      end
      regval := ExtractFilePath(RemoveQuotes(regval));
      // ToDo: Also remove ffdshow files if new install path is different from old one.
    end
    // Remove MSVC80 runtimes
    if RegQueryDwordValue(HKLM, 'Software\GNU\ffdshow', 'AvisynthMsvcr80Inst', dword) AND (dword = 1) then begin
      if RegQueryStringValue(HKLM, 'Software\GNU\ffdshow', 'pthAvisynth', regval) AND DirExists(regval) then begin
        try
        if FileExists(regval + '\msvcr80.dll') AND DecrementSharedCount(False, regval + '\msvcr80.dll') then begin
          if NOT DeleteFile(regval + '\msvcr80.dll') then begin
            RestartReplace(regval + '\msvcr80.dll', '');
          end
        end
        if FileExists(regval + '\Microsoft.VC80.CRT.manifest') AND DecrementSharedCount(False, regval + '\Microsoft.VC80.CRT.manifest') then begin
          if NOT DeleteFile(regval + '\Microsoft.VC80.CRT.manifest') then begin
            RestartReplace(regval + '\Microsoft.VC80.CRT.manifest', '');
          end
        end
        except end;
      end
      RegDeleteValue(HKLM, 'Software\GNU\ffdshow', 'AvisynthMsvcr80Inst');
    end
    if RegQueryDwordValue(HKLM, 'Software\GNU\ffdshow', 'VirtualDubMsvcr80Inst', dword) AND (dword = 1) then begin
      if RegQueryStringValue(HKLM, 'Software\GNU\ffdshow', 'pthVirtualDub', regval) AND DirExists(regval) then begin
        try
        if FileExists(regval + '\msvcr80.dll') AND DecrementSharedCount(False, regval + '\msvcr80.dll') then begin
          if NOT DeleteFile(regval + '\msvcr80.dll') then begin
            RestartReplace(regval + '\msvcr80.dll', '');
          end
        end
        if FileExists(regval + '\Microsoft.VC80.CRT.manifest') AND DecrementSharedCount(False, regval + '\Microsoft.VC80.CRT.manifest') then begin
          if NOT DeleteFile(regval + '\Microsoft.VC80.CRT.manifest') then begin
            RestartReplace(regval + '\Microsoft.VC80.CRT.manifest', '');
          end
        end
        except end;
      end
      RegDeleteValue(HKLM, 'Software\GNU\ffdshow', 'VirtualDubMsvcr80Inst');
    end
    if RegQueryDwordValue(HKLM, 'Software\GNU\ffdshow', 'DScalerMsvcr80Inst', dword) AND (dword = 1) then begin
      if RegQueryStringValue(HKLM, 'Software\GNU\ffdshow', 'dscalerPth', regval) AND DirExists(regval) then begin
        try
        if FileExists(regval + '\msvcr80.dll') AND DecrementSharedCount(False, regval + '\msvcr80.dll') then begin
          if NOT DeleteFile(regval + '\msvcr80.dll') then begin
            RestartReplace(regval + '\msvcr80.dll', '');
          end
        end
        if FileExists(regval + '\Microsoft.VC80.CRT.manifest') AND DecrementSharedCount(False, regval + '\Microsoft.VC80.CRT.manifest') then begin
          if NOT DeleteFile(regval + '\Microsoft.VC80.CRT.manifest') then begin
            RestartReplace(regval + '\Microsoft.VC80.CRT.manifest', '');
          end
        end
        except end;
      end
      RegDeleteValue(HKLM, 'Software\GNU\ffdshow', 'DScalerMsvcr80Inst');
    end
    // Remove uninstall registry key
    RegDeleteKeyIncludingSubkeys(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\ffdshow');
  end
end;

function InitializeSetup(): Boolean;
begin
  Result := True;

  #if include_cpu_detection
    #if sse2_required
    if Result AND NOT Is_SSE2_Supported() then begin
      Result := False;
      msgbox('This build of ffdshow requires a CPU with SSE2 extension support. Your CPU does not have those capabilities.', mbError, MB_OK);
    end
    #endif
    #if sse_required
    if Result AND NOT Is_SSE_Supported() then begin
      Result := False;
      msgbox('This build of ffdshow requires a CPU with SSE extension support. Your CPU does not have those capabilities.', mbError, MB_OK);
    end
    #endif
  #endif
  
  is_update := RegKeyExists(HKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\ffdshow_is1');
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then begin
    RemoveBuildUsingNSIS;
  end
  
  #if include_cpu_detection
  if CurStep = ssPostInstall then begin
    RegWriteDwordValue(HKCU, 'Software\GNU\ffdshow\default', 'threadsnum', GetNumberOfCores);
  end
  #endif
end;

function InitializeUninstall(): Boolean;
var
  regval: String;
  ResultCode: Integer;
begin
  // Also uninstall NSIS build when uninstalling Inno Setup build.
  if RegQueryStringValue(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\ffdshow', 'UninstallString', regval) then begin
    if FileExists(RemoveQuotes(regval)) then begin
      if NOT Exec(RemoveQuotes(regval), '/S', '', SW_SHOW, ewWaitUntilTerminated, ResultCode) then begin
        MsgBox(SysErrorMessage(resultCode), mbError, MB_OK);
      end
    end
  end
  Result := True;
end;

// #define DSSPEAKER_DIRECTOUT         0x00000000
// #define DSSPEAKER_HEADPHONE         0x00000001
// #define DSSPEAKER_MONO              0x00000002
// #define DSSPEAKER_QUAD              0x00000003
// #define DSSPEAKER_STEREO            0x00000004
// #define DSSPEAKER_SURROUND          0x00000005
// #define DSSPEAKER_5POINT1           0x00000006
// #define DSSPEAKER_7POINT1           0x00000007

function ffSpkCfg(): Integer;
external 'getSpeakerConfig@files:ffSpkCfg.dll stdcall delayload';

function getSpeakerConfig(): Integer;
begin
  try
    Result := ffSpkCfg();
  except
    Result := 4; // DSSPEAKER_STEREO
  end;
end;

function Get_mixerOut(dummy: String): String;
begin
  if      SpeakerPage.Values[0] = True then
    Result := '0'
  else if SpeakerPage.Values[1] = True then
    Result := '17'
  else if SpeakerPage.Values[2] = True then
    Result := '1'
  else if SpeakerPage.Values[3] = True then
    Result := '2'
  else if SpeakerPage.Values[4] = True then
    Result := '5'
  else if SpeakerPage.Values[5] = True then
    Result := '12'
  else if SpeakerPage.Values[6] = True then
    Result := '6'
  else if SpeakerPage.Values[7] = True then
    Result := '13'
  else if SpeakerPage.Values[8] = True then
    Result := IntToStr(reg_mixerOut);
  RegWriteDWordValue(HKLM, 'Software\GNU\ffdshow_audio', 'isSpkCfg', 1);
end;

function Get_ismixer(dummy: String): String;
begin
  Result := '1';
  if (is8DisableMixer = True) and (SpeakerPage.Values[8] = True) then
    Result := '0';
end;

function Get_mixerExpandStereo(dummy: String): String;
begin
  Result := '0'
  if chbExpandStereo.Checked then
    Result := '1';
end;

function Get_mixerVoiceControl(dummy: String): String;
begin
  Result := '0'
  if chbVoicecontrol.Checked then
    Result := '1';
end;

function Get_isUseonlyinVideo(dummy: String): String;
begin
  Result := '1';
  if ComplistVideo.page.Values[0] then
    Result := '0';
end;

function Get_useonlyinVideo(dummy: String): String;
begin
  Result := ComplistVideo.edt.Text;
end;

function Get_isUseonlyinAudio(dummy: String): String;
begin
  Result := '1';
  if ComplistAudio.page.Values[0] then
    Result := '0';
end;

function Get_useonlyinAudio(dummy: String): String;
begin
  Result := ComplistAudio.edt.Text;
end;

function isCompVValid(): Boolean;
begin
  Result := not ComplistVideo.skipped;
end;

function isCompAValid(): Boolean;
begin
  Result := not ComplistAudio.skipped;
end;

function Get_isCompVdontask(dummy: String): String;
begin
  Result := '0';
  if ComplistVideo.chbDontAsk.Checked then
    Result := '1';
end;

function Get_isCompAdontask(dummy: String): String;
begin
  Result := '0';
  if ComplistAudio.chbDontAsk.Checked then
    Result := '1';
end;

function ffRegReadDWordHKCU(regKeyName: String; regValName: String; defaultValue: Cardinal): Cardinal;
begin
  if NOT RegQueryDwordValue(HKCU, regKeyName, regValName, Result) then
    Result := defaultValue;
end;

procedure initComplist(var complist: TcomplistPage; regKeyName: String);
var
  regstr: String;
  regstrUpper: String;
  i: Integer;
  revision: Cardinal;
  rev: Integer;
begin
  complist.countAdded := 0;
  complist.page.Add(ExpandConstant('{cm:comp_donotlimit}'));
  complist.page.Add(ExpandConstant('{cm:comp_useonlyin}'));
  if ffRegReadDWordHKCU(regKeyName, 'isUseonlyin', 1)=1 then
    complist.page.Values[1] := True
  else
    complist.page.Values[0] := True;

  // Edit control
  complist.edt := TMemo.Create(complist.page);
  complist.edt.Top := ScaleY(78);
  complist.edt.Width := complist.page.SurfaceWidth;
  complist.edt.Height := ScaleY(135);
  complist.edt.Parent := complist.page.Surface;
  complist.edt.MaxLength := 4000;
  complist.edt.ScrollBars := ssVertical;
  if RegQueryStringValue(HKCU, regKeyName, 'useonlyin', regstr) then begin
    if RegQueryDwordValue(HKLM, 'Software\GNU\ffdshow', 'revision', revision) then begin
      regstrUpper := AnsiUppercase(regstr);
      rev := 1077;
      for i:= 1 to NUMBER_OF_COMPATIBLEAPPLICATIONS do
      begin
        if compApps[i].rev = 0 then Break;
        if compApps[i].rev > 1 then
          rev := compApps[i].rev;
        if rev > revision then begin
          if Pos(AnsiUppercase(compApps[i].name),regstrUpper) = 0 then begin
            regstr := compApps[i].name + #13#10 + regstr;
            complist.countAdded := complist.countAdded + 1;
          end
        end
      end
    end
    complist.edt.text := regstr;
  end else begin
    complist.edt.Text :=
    '3wPlayer.exe'#13#10
    'ACDSee5.exe'#13#10
    'ACDSee6.exe'#13#10
    'ACDSee7.exe'#13#10
    'ACDSee8.exe'#13#10
    'ACDSee8pro.exe'#13#10
    'ACDSee9.exe'#13#10
    'Adobe Premiere Elements.exe'#13#10
    'Adobe Premiere Pro.exe'#13#10
    'aegisub.exe'#13#10
    'afreecaplayer.exe'#13#10
    'afreecastudio.exe'#13#10
    'aim6.exe'#13#10
    'ALLPlayer.exe'#13#10
    'ALShow.exe'#13#10
    'ALSong.exe'#13#10
    'AltDVB.exe'#13#10
    'amcap.exe'#13#10
    'amvtransform.exe'#13#10
    'Apollo DivX to DVD Creator.exe'#13#10
    'Apollo3GPVideoConverter.exe'#13#10
    'Ares.exe'#13#10
    'AsfTools.exe'#13#10
    'ass_help3r.exe'#13#10
    'ASUSDVD.exe'#13#10
    'Audition.exe'#13#10
    'AutoGK.exe'#13#10
    'autorun.exe'#13#10
    'avant.exe'#13#10
    'AVerTV.exe'#13#10
    'Avi2Dvd.exe'#13#10
    'avi2mpg.exe'#13#10
    'avicodec.exe'#13#10
    'avipreview.exe'#13#10
    'aviutl.exe'#13#10
    'avs2avi.exe'#13#10
    'Badak.exe'#13#10
    'BearShare.exe'#13#10
    'BePipe.exe'#13#10
    'bestplayer1.0.exe'#13#10
    'BitComet.exe'#13#10
    'BlazeDVD.exe'#13#10
    'bplay.exe'#13#10
    'bsplay.exe'#13#10
    'bsplayer.exe'#13#10
    'BTVD3DShell.exe'#13#10
    'CamRecorder.exe'#13#10
    'CamtasiaStudio.exe'#13#10
    'carom.exe'#13#10
    'christv.exe'#13#10
    'cinemaplayer.exe'#13#10
    'CinergyDVR.exe'#13#10
    'CodecInstaller.exe'#13#10
    'ConvertXtoDvd.exe'#13#10
    'coolpro2.exe'#13#10
    'CorePlayer.exe'#13#10
    'coreplayer.exe'#13#10
    'Crystal.exe'#13#10
    'crystalfree.exe'#13#10
    'CrystalPro.exe'#13#10
    'CTCMS.exe'#13#10
    'CTCMSU.exe'#13#10
    'CTWave.exe'#13#10
    'CTWave32.exe'#13#10
    'cut_assistant.exe'#13#10
    'cscript.exe'#13#10
    'dashboard.exe'#13#10
    'demo32.exe'#13#10
    'DivX Player.exe'#13#10
    'dllhost.exe'#13#10
    'dpgenc.exe'#13#10 
    'Dr.DivX.exe'#13#10
    'drdivx.exe'#13#10
    'drdivx2.exe'#13#10
    'DreamMaker.exe'#13#10
    'DSBrws.exe'#13#10
    'DScaler.exe'#13#10
    'dv.exe'#13#10
    'dvbdream.exe'#13#10
    'dvbviewer.exe'#13#10
    'DVDAuthor.exe'#13#10
    'DVDMF.exe'#13#10
    'dvdplay.exe'#13#10
    'DVDMaker.exe'#13#10
    'DVD Shrink 3.2.exe'#13#10
    'DXEnum.exe'#13#10
    'Easy RealMedia Tools.exe'#13#10
    'ehExtHost.exe'#13#10
    'ehshell.exe'#13#10
    'emule_TK4.exe'#13#10
    'Encode360.exe'#13#10
    'fenglei.exe'#13#10
    'ffmpeg.exe'#13#10
    'filtermanager.exe'#13#10
    'firefox.exe'#13#10
    'Flash.exe'#13#10
    'FMRadio.exe'#13#10
    'Fortius.exe'#13#10
    'FreeStyle.exe'#13#10
    'FSViewer.exe'#13#10
    'FusionHDTV.exe'#13#10
    'GDivX Player.exe'#13#10
    'gdsmux.exe'#13#10
    'GoldWave.exe'#13#10
    'gom.exe'#13#10
    'GomEnc.exe'#13#10
    'GoogleDesktop.exe'#13#10
    'GoogleDesktopCrawl.exe'#13#10
    'graphedit.exe'#13#10
    'graphedt.exe'#13#10
    'gspot.exe'#13#10
    'HBP.exe'#13#10
    'HDVSplit.exe'#13#10
    'honestechTV.exe'#13#10
    'HPWUCli.exe'#13#10
    'ICQ.exe'#13#10
    'ICQLite.exe'#13#10
    'iexplore.exe'#13#10
    'IHT.exe'#13#10
    'IncMail.exe'#13#10
    'InfoTool.exe'#13#10
    'infotv.exe'#13#10
    'iPlayer.exe'#13#10
    'i_view32.exe'#13#10
    'JetAudio.exe'#13#10
    'jwBrowser.exe'#13#10
    'kmplayer.exe'#13#10
    'LA.exe'#13#10
    'LifeCam.exe'#13#10
    'Lilith.exe'#13#10
    'makeAVIS.exe'#13#10
    'Maxthon.exe'#13#10
    'MDirect.exe'#13#10
    'Media Center 12.exe'#13#10
    'Media Jukebox.exe'#13#10
    'Media Player Classic.exe'#13#10
    'MediaLife.exe'#13#10
    'MediaPortal.exe'#13#10
    'MediaServer.exe'#13#10
    'megui.exe'#13#10
    'mencoder.exe'#13#10
    'Metacafe.exe'#13#10
    'MMPlayer.exe'#13#10
    'moviethumb.exe'#13#10
    'mpcstar.exe'#13#10
    'MpegVideoWizard.exe'#13#10
    'mplayer2.exe'#13#10
    'mplayerc.exe'#13#10
    'msoobe.exe'#13#10
    'MultimediaPlayer.exe'#13#10
    'Munite.exe'#13#10
    'MusicManager.exe'#13#10
    'Muzikbrowzer.exe'#13#10
    'Mv2PlayerPlus.exe'#13#10
    'myplayer.exe'#13#10
    'nero.exe'#13#10
    'NeroHome.exe'#13#10
    'NeroVision.exe'#13#10
    'NicoPlayer.exe'#13#10
    'NMSTranscoder.exe'#13#10
    'nvplayer.exe'#13#10
    'Omgjbox.exe'#13#10
    'OnlineTV.exe'#13#10
    'Opera.exe'#13#10
    'OrbStreamerClient.exe'#13#10
    'OUTLOOK.EXE'#13#10
    'PaintDotNet.exe'#13#10
    'paltalk.exe'#13#10
    'pcwmp.exe'#13#10
    'PhotoScreensaver.scr'#13#10
    'Photoshop.exe'#13#10
    'Picasa2.exe'#13#10
    'playwnd.exe'#13#10
    'PowerDirector.exe'#13#10
    'powerdvd.exe'#13#10
    'POWERPNT.EXE'#13#10
    'PPLive.exe'#13#10
    'ppmate.exe'#13#10
    'PPStream.exe'#13#10
    'Procoder2.exe'#13#10
    'Producer.exe'#13#10
    'progdvb.exe'#13#10
    'PVCR.exe'#13#10
    'Qonoha.exe'#13#10
    'QQPlayerSvr.exe'#13#10
    'RadLight.exe'#13#10
    'realplay.exe'#13#10
    'Recode.exe'#13#10
    'rlkernel.exe'#13#10
    'RoxMediaDB9.exe'#13#10
    'rundll32.exe'#13#10
    'SelfMV.exe'#13#10
    'Shareaza.exe'#13#10
    'sherlock2.exe'#13#10
    'ShowTime.exe'#13#10
    'sidebar.exe'#13#10
    'SinkuHadouken.exe'#13#10
    'Sleipnir.exe'#13#10
    'smartmovie.exe'#13#10
    'SopCast.exe'#13#10
    'START.EXE'#13#10
    'SubtitleEdit.exe'#13#10
    'SubtitleWorkshop.exe'#13#10
    'SplitCam.exe'#13#10
    'stillcap.exe'#13#10
    'Studio.exe'#13#10
    'subedit.exe'#13#10
    'SWFConverter.exe'#13#10
    'TheaterTek DVD.exe'#13#10
    'time_adjuster.exe'#13#10
    'timecodec.exe'#13#10
    'tmc.exe'#13#10
    'TMPGEnc.exe'#13#10
    'TMPGEnc4XP.exe'#13#10
    'TOTALCMD.EXE'#13#10
    'tvc.exe'#13#10
    'TVersity.exe'#13#10
    'TVPlayer.exe'#13#10
    'TVUPlayer.exe'#13#10
    'UCC.exe'#13#10
    'Ultra EDIT.exe'#13#10
    'VCD_PLAY.EXE'#13#10
    'VeohClient.exe'#13#10
    'VFAPIFrameServer.exe'#13#10
    'VideoSnapshot.exe'#13#10
    'VideoSplitter.exe'#13#10
    'VIDEOS~1.SCR'#13#10
    'ViPlay3.exe'#13#10
    'virtualdub.exe'#13#10
    'virtualdubmod.exe'#13#10
    'vplayer.exe'#13#10
    'WCreator.exe'#13#10
    'WFTV.exe'#13#10
    'winamp.exe'#13#10
    'WinAVI.exe'#13#10
    'WindowsPhotoGallery.exe'#13#10
    'windvd.exe'#13#10
    'WinMPGVideoConvert.exe'#13#10
    'WINWORD.EXE'#13#10
    'wmenc.exe'#13#10
    'wmplayer.exe'#13#10
    'wscript.exe'#13#10
    'x264.exe'#13#10
    'XNVIEW.EXE'#13#10
    'Xvid4PSP.exe'#13#10
    'YahooMusicEngine.exe'#13#10
    'YahooWidgetEngine.exe'#13#10
    'zplayer.exe'#13#10
    'Zune.exe'#13#10;
  end


  // Check box
  complist.chbDontAsk := TCheckBox.Create(complist.page);
  complist.chbDontAsk.Top := ScaleY(220);
  complist.chbDontAsk.Left := ScaleX(0);
  complist.chbDontAsk.Width := ScaleX(170);
  complist.chbDontAsk.Height := ScaleY(16);
  complist.chbDontAsk.Caption := ExpandConstant('{cm:dontaskmeagain}');
  complist.chbDontAsk.Parent := complist.page.Surface;
  complist.chbDontAsk.Checked := False;
end;

procedure InitializeWizard;
var
  systemSpeakerConfig: Integer;
  reg_isSpkCfg: Cardinal;
  isMajorType: Boolean;
  ii: Cardinal;
  i: Integer;
begin
  { Create the pages }

// new compatible applications should be written both here and edtTarget.Text := 'aegisub...
// FIXME more smart way of initializing compApps.
  for i := 1 to NUMBER_OF_COMPATIBLEAPPLICATIONS do
   compApps[i].rev := -1;

  compApps[1].rev   := 1077; //975;
  compApps[1].name  := 'Munite.exe';
  compApps[2].rev   := 1077; //975;
  compApps[2].name  := 'nvplayer.exe';
  compApps[3].rev   := 1077; //975;
  compApps[3].name  := 'Qonoha.exe';
  compApps[3].rev   := 1077; //976;
  compApps[3].name  := 'SinkuHadouken.exe';
  compApps[4].rev   := 1077; //985;
  compApps[4].name  := 'Lilith.exe';
  compApps[5].rev   := 1077; //1048;
  compApps[5].name  := 'megui.exe';
  compApps[6].rev   := 1116;
  compApps[6].name  := 'TheaterTek DVD.exe';
  compApps[7].rev   := 1125;
  compApps[7].name  := 'graphedit.exe';
  compApps[8].rev   := 1241;
  compApps[8].name  := 'BePipe.exe';

  compApps[9].rev   := 1245;
  compApps[9].name  := 'christv.exe';

  compApps[10].rev  := 1247;
  compApps[10].name := 'avi2mpg.exe';
  compApps[11].name := 'cut_assistant.exe';
  compApps[12].name := 'SplitCam.exe';

  compApps[13].rev  := 1253;
  compApps[13].name := 'Sleipnir.exe';
  compApps[14].name := 'fenglei.exe';
  compApps[15].name := 'MDirect.exe';
  compApps[16].name := 'SubtitleEdit.exe';
  compApps[17].name := 'sherlock2.exe';
  compApps[18].name := 'GoogleDesktop.exe';
  compApps[19].name := 'MediaServer.exe';
  compApps[20].name := 'MediaPortal.exe';
  compApps[21].name := 'honestechTV.exe';
  compApps[22].name := 'DVD Shrink 3.2.exe';
  compApps[23].name := 'stillcap.exe';
  compApps[24].name := 'carom.exe';
  compApps[25].name := 'WCreator.exe';
  compApps[26].name := 'ppmate.exe';

  compApps[27].rev  := 1259;
  compApps[27].name := 'ShowTime.exe';
  compApps[28].name := 'YahooWidgetEngine.exe';
  compApps[29].name := 'PowerDirector.exe';
  compApps[30].name := 'infotv.exe';
  compApps[31].name := 'rundll32.exe';
  compApps[32].name := 'smartmovie.exe';
  compApps[33].name := 'MpegVideoWizard.exe';
  compApps[34].name := 'SWFConverter.exe';
  compApps[35].name := 'FMRadio.exe';
  compApps[36].name := 'TMPGEnc.exe';
  compApps[37].name := 'HPWUCli.exe';

  compApps[38].rev  := 1267;
  compApps[38].name := 'pcwmp.exe';
  compApps[39].name := 'NeroVision.exe';
  compApps[40].name := 'ICQLite.exe';
  compApps[41].name := 'SubtitleWorkshop.exe';
  compApps[42].name := 'Adobe Premiere Pro.exe';
  compApps[43].name := 'Media Player Classic.exe';
  compApps[44].name := 'Opera.exe';
  compApps[45].name := 'amcap.exe';
  compApps[46].name := 'PaintDotNet.exe';
  compApps[47].name := 'GoogleDesktopCrawl.exe';
  compApps[48].name := 'WinAVI.exe';
  compApps[49].name := 'TVersity.exe';
  compApps[50].name := 'IHT.exe';
  compApps[51].name := 'START.EXE';
  compApps[52].name := 'mencoder.exe';
  compApps[53].name := 'dvdplay.exe';

  compApps[54].rev  := 1273;
  compApps[54].name := 'ehExtHost.exe';
  compApps[55].name := 'aim6.exe';
  compApps[56].name := 'CrystalPro.exe';
  compApps[57].name := 'PPStream.exe';
  compApps[58].name := 'Crystal.exe';
  compApps[59].name := 'TMPGEnc4XP.exe';
  compApps[60].name := 'subedit.exe';
  compApps[61].name := 'emule_TK4.exe';
  compApps[62].name := 'BTVD3DShell.exe';
  compApps[63].name := 'Xvid4PSP.exe';
  compApps[64].name := 'dashboard.exe';
  compApps[65].name := 'drdivx.exe';
  compApps[66].name := 'NMSTranscoder.exe';
  compApps[67].name := 'Fortius.exe';
  compApps[68].name := 'VideoSnapshot.exe';
  compApps[69].name := 'RadLight.exe';
  compApps[70].name := 'Procoder2.exe';
  compApps[71].name := 'DivX Player.exe';
  compApps[72].name := 'i_view32.exe';
  compApps[73].name := 'Recode.exe';
  compApps[74].name := 'Encode360.exe';
  compApps[75].name := 'ACDSee5.exe';
  compApps[76].name := 'filtermanager.exe';
  compApps[77].name := 'avicodec.exe';
  compApps[78].name := 'x264.exe';
  compApps[79].name := 'MediaLife.exe';
  compApps[80].name := 'cscript.exe';
  compApps[81].name := 'wscript.exe';
  compApps[82].name := 'SopCast.exe';
  compApps[83].name := 'DreamMaker.exe';
  compApps[84].name := 'Maxthon.exe';
  compApps[85].name := 'InfoTool.exe';
  compApps[86].name := 'Ultra EDIT.exe';
  compApps[87].name := 'moviethumb.exe';
  compApps[88].name := 'GDivX Player.exe';
  compApps[89].name := 'TOTALCMD.EXE';
  compApps[90].name := 'CodecInstaller.exe';

  compApps[91].rev  := 1283;
  compApps[91].name := 'OUTLOOK.EXE';
  compApps[92].name := 'NeroHome.exe';
  compApps[93].name := 'ALSong.exe';
  compApps[94].name := 'HBP.exe';
  compApps[95].name := 'Easy RealMedia Tools.exe';
  compApps[96].name := 'myplayer.exe';
  compApps[97].name := 'bplay.exe';
  compApps[98].name := 'nero.exe';
  compApps[99].name := 'ICQ.exe';
  compApps[100].name := 'XNVIEW.EXE';
  compApps[101].name := 'WINWORD.EXE';
  compApps[102].name := 'POWERPNT.EXE';
  compApps[103].name := 'Studio.exe';
  compApps[104].name := 'iPlayer.exe';
  compApps[105].name := 'Adobe Premiere Elements.exe';
  compApps[106].name := 'Photoshop.exe';
  compApps[107].name := 'VideoSplitter.exe';
  compApps[108].name := 'dvbviewer.exe';
  compApps[109].name := 'FSViewer.exe';
  compApps[110].name := 'DVDMF.exe';
  compApps[111].name := 'Flash.exe';
  compApps[112].name := 'VCD_PLAY.EXE';
  compApps[113].name := 'WinMPGVideoConvert.exe';
  compApps[114].name := 'Apollo DivX to DVD Creator.exe';
  compApps[115].name := 'Avi2Dvd.exe';
  compApps[116].name := 'PVCR.exe';
  compApps[117].name := 'HDVSplit.exe';
  compApps[118].name := 'time_adjuster.exe';
  compApps[119].name := 'playwnd.exe';

  compApps[120].rev  := 1296;
  compApps[120].name := 'vstudio.exe';
  compApps[121].name := 'msnmsgr.exe';
  compApps[122].name := 'DXEnum.exe';
  compApps[123].name := 'Shareaza.exe';
  compApps[124].name := 'YahooMessenger.exe';
  compApps[125].name := 'WebcamMax.exe';
  compApps[126].name := 'TMPGEncDVDAuthor3.exe';
  compApps[127].name := 'OrbStreamerClient.exe';
  compApps[128].name := 'SelfMV.exe';
  compApps[129].name := 'TVUPlayer.exe';
  compApps[130].name := 'amvtransform.exe';
  compApps[131].name := 'Badak.exe';
  compApps[132].name := 'CTCMSU.exe';
  compApps[133].name := 'CTWave.exe';
  compApps[134].name := 'IncMail.exe';
  compApps[135].name := 'MMPlayer.exe';
  compApps[136].name := 'mpcstar.exe';
  compApps[137].name := 'PhotoScreensaver.scr';
  compApps[138].name := 'QQPlayerSvr.exe';
  compApps[139].name := 'Shareaza.exe';

  compApps[140].rev  := 1322;
  compApps[140].name := 'afreecaplayer.exe';
  compApps[141].name := 'WFTV.exe';
  compApps[142].name := 'coolpro2.exe';
  compApps[143].name := 'PPLive.exe';
  compApps[144].name := 'Picasa2.exe';
  compApps[145].name := 'VeohClient.exe';
  compApps[146].name := 'ACDSee9.exe';
  compApps[147].name := 'BitComet.exe';
  compApps[148].name := 'jwBrowser.exe';
  compApps[149].name := 'DVDAuthor.exe';

  compApps[150].rev := 1355;
  compApps[150].name:= 'dllhost.exe';

  compApps[151].rev := 1378;
  compApps[151].name:= 'GomEnc.exe';
  compApps[152].name:= 'bestplayer1.0.exe';
  compApps[153].name:= 'msoobe.exe';
  compApps[154].name:= 'vplayer.exe';
  compApps[155].name:= 'paltalk.exe';
  compApps[156].name:= 'ffmpeg.exe';
  compApps[157].name:= 'demo32.exe';
  compApps[158].name:= 'Omgjbox.exe';
  compApps[159].name:= 'UCC.exe';
  compApps[160].name:= 'Metacafe.exe';
  compApps[161].name:= 'avant.exe';
  compApps[162].name:= 'CTWave32.exe';
  compApps[163].name:= 'tvc.exe';
  compApps[164].name:= 'GoldWave.exe';
  compApps[165].name:= 'WindowsPhotoGallery.exe';
  compApps[166].name:= 'Producer.exe';
  compApps[167].name:= 'MusicManager.exe';
  compApps[168].name:= 'cinemaplayer.exe';
  compApps[169].name:= 'CTCMS.exe';
  compApps[170].name:= 'sidebar.exe';
  compApps[171].name:= 'LifeCam.exe';
  compApps[172].name:= 'NicoPlayer.exe';
  compApps[172].name:= 'afreecastudio.exe';
  compApps[173].name:= 'AVerTV.exe';
  compApps[174].name:= 'FusionHDTV.exe';
  compApps[175].name:= 'VIDEOS~1.SCR';
  
  compApps[176].rev := 1410;
  compApps[176].name:= 'YahooMusicEngine.exe';
  compApps[177].name:= '3wPlayer.exe';
  compApps[178].name:= 'ACDSee6.exe';
  compApps[179].name:= 'ACDSee7.exe';
  compApps[180].name:= 'ACDSee8.exe';
  compApps[181].name:= 'ACDSee8Pro.exe';
  compApps[182].name:= 'AltDVB.exe';
  compApps[183].name:= 'Ares.exe';
  compApps[184].name:= 'AsfTools.exe';
  compApps[185].name:= 'Audition.exe';
  compApps[186].name:= 'AutoGK.exe';
  compApps[187].name:= 'autorun.exe';
  compApps[188].name:= 'avs2avi.exe';
  compApps[189].name:= 'BearShare.exe';
  compApps[190].name:= 'BlazeDVD.exe';
  compApps[191].name:= 'CamRecorder.exe';
  compApps[192].name:= 'CamtasiaStudio.exe';
  compApps[193].name:= 'CinergyDVR.exe';
  compApps[194].name:= 'ConvertXtoDvd.exe';
  compApps[195].name:= 'tmc.exe';
  compApps[196].name:= 'dpgenc.exe';
  compApps[197].name:= 'drdivx2.exe';
  compApps[198].name:= 'dvbdream.exe';
  compApps[199].name:= 'FreeStyle.exe';
  compApps[200].name:= 'MultimediaPlayer.exe';
  compApps[201].name:= 'RoxMediaDB9.exe';
  compApps[202].name:= 'TVPlayer.exe';
  compApps[203].name:= 'VFAPIFrameServer.exe';
  
  compApps[204].rev := 1500;
  compApps[204].name:= 'Apollo3GPVideoConverter.exe';
  compApps[205].name:= 'DXEnum.exe';
  compApps[206].name:= 'ASUSDVD.exe';
  compApps[207].name:= 'Dr.DivX.exe';
  compApps[208].name:= 'gdsmux.exe';
  compApps[209].name:= 'DSBrws.exe';
  compApps[210].name:= 'OnlineTV.exe';
  compApps[211].name:= 'Zune.exe';
  
  compApps[212].rev := 0;

// Compatibility list
  ComplistVideo.skipped := False;
  ComplistAudio.skipped := False;
  Complist_isMsgAddedShown := False;

  ComplistVideo.page := CreateInputOptionPage(wpSelectTasks,
    ExpandConstant('{cm:comp_SetupLabelV1}'),
    ExpandConstant('{cm:comp_SetupLabelV2}'),
    ExpandConstant('{cm:comp_SetupLabelV3}'),
    True, False);
  initComplist(ComplistVideo ,'Software\GNU\ffdshow');

  ComplistAudio.page := CreateInputOptionPage(ComplistVideo.page.ID,
    ExpandConstant('{cm:comp_SetupLabelA1}'),
    ExpandConstant('{cm:comp_SetupLabelA2}'),
    ExpandConstant('{cm:comp_SetupLabelA3}'),
    True, False);
  initComplist(ComplistAudio,'Software\GNU\ffdshow_audio');

// Speaker setup

  is8DisableMixer := False;
  SpeakerPage := CreateInputOptionPage(ComplistAudio.page.ID,
    ExpandConstant('{cm:speakersetup}'),
    ExpandConstant('{cm:SpeakerSetupLabel2}'),
    ExpandConstant('{cm:SpeakerSetupLabel3}'),
    True, False);
  SpeakerPage.Add(ExpandConstant('1.0 ({cm:spk_mono})'));                        // 0
  SpeakerPage.Add(ExpandConstant('2.0 ({cm:spk_headphone})'));                   // 17
  SpeakerPage.Add(ExpandConstant('2.0 ({cm:spk_stereo})'));                      // 1
  SpeakerPage.Add(ExpandConstant('3.0 ({cm:spk_front_3ch})'));                   // 2
  SpeakerPage.Add(ExpandConstant('4.0 ({cm:spk_quadro})'));                      // 5
  SpeakerPage.Add(ExpandConstant('4.1 ({cm:spk_quadro} + {cm:spk_Subwoofer})')); // 12
  SpeakerPage.Add(ExpandConstant('5.0 ({cm:spk_5ch})'));                         // 6
  SpeakerPage.Add(ExpandConstant('5.1 ({cm:spk_5ch} + {cm:spk_Subwoofer})'));    // 13
  if  RegQueryDWordValue(HKCU, 'Software\GNU\ffdshow_audio\default', 'mixerOut', reg_mixerOut)
  and RegQueryDWordValue(HKCU, 'Software\GNU\ffdshow_audio\default', 'ismixer' , reg_ismixer)
  and RegQueryDWordValue(HKLM, 'Software\GNU\ffdshow_audio'        , 'isSpkCfg', reg_isSpkCfg) then
  begin
    if reg_ismixer = 1 then begin
      isMajorType := True;
      if      reg_mixerOut = 0 then
        SpeakerPage.Values[0] := True
      else if reg_mixerOut = 17 then
        SpeakerPage.Values[1] := True
      else if reg_mixerOut = 1 then
        SpeakerPage.Values[2] := True
      else if reg_mixerOut = 2 then
        SpeakerPage.Values[3] := True
      else if reg_mixerOut = 5 then
        SpeakerPage.Values[4] := True
      else if reg_mixerOut = 12 then
        SpeakerPage.Values[5] := True
      else if reg_mixerOut = 6 then
        SpeakerPage.Values[6] := True
      else if reg_mixerOut = 13 then
        SpeakerPage.Values[7] := True
      else begin
        if reg_mixerOut = 3 then
          SpeakerPage.Add(ExpandConstant('2.1 ({cm:spk_front_2ch} + {cm:spk_Surround})'))
        else if reg_mixerOut = 4 then
          SpeakerPage.Add(ExpandConstant('3.1 ({cm:spk_front_3ch} + {cm:spk_Surround})'))
        else if reg_mixerOut = 7 then
          SpeakerPage.Add(ExpandConstant('1.1 ({cm:spk_mono} + {cm:spk_Subwoofer})'))
        else if reg_mixerOut = 8 then
          SpeakerPage.Add(ExpandConstant('2.1 ({cm:spk_front_2ch} + {cm:spk_Subwoofer})'))
        else if reg_mixerOut = 9 then
          SpeakerPage.Add(ExpandConstant('3.1 ({cm:spk_front_3ch} + {cm:spk_Subwoofer})'))
        else if reg_mixerOut = 10 then
          SpeakerPage.Add(ExpandConstant('2.1.1 ({cm:spk_front_2ch} + {cm:spk_Surround} + {cm:spk_Subwoofer})'))
        else if reg_mixerOut = 11 then
          SpeakerPage.Add(ExpandConstant('3.1.1 ({cm:spk_front_3ch} + {cm:spk_Surround} + {cm:spk_Subwoofer})'))
        else if reg_mixerOut = 14 then
          SpeakerPage.Add(ExpandConstant('{cm:spk_dolby1}'))
        else if reg_mixerOut = 19 then
          SpeakerPage.Add(ExpandConstant('{cm:spk_dolby1} + {cm:spk_Subwoofer}'))
        else if reg_mixerOut = 15 then
          SpeakerPage.Add(ExpandConstant('{cm:spk_dolby2}'))
        else if reg_mixerOut = 20 then
          SpeakerPage.Add(ExpandConstant('{cm:spk_dolby2} + {cm:spk_Subwoofer}'))
        else if reg_mixerOut = 16 then
          SpeakerPage.Add(ExpandConstant('{cm:spk_sameasinput}'))
        else if reg_mixerOut = 18 then
          SpeakerPage.Add(ExpandConstant('{cm:spk_HRTF}'));
        SpeakerPage.Values[8] := True;
        isMajorType := False;
      end
      if isMajorType then begin
       SpeakerPage.Add(ExpandConstant('{cm:spk_MixerDisable}'));
       is8DisableMixer := True;
      end
    end else begin
      SpeakerPage.Add(ExpandConstant('{cm:spk_MixerDisable}'));
      is8DisableMixer := True;
      SpeakerPage.Values[8] := True;
    end
  end else begin
    reg_ismixer := 1;
    reg_mixerOut := 1;
    SpeakerPage.Add(ExpandConstant('{cm:spk_MixerDisable}'));
    is8DisableMixer := True;
    systemSpeakerConfig := getSpeakerConfig(); // read the setting of control panel(requires directX 8)
    SpeakerPage.Values[2] := True;       // default 2.0 (Stereo)

    if      systemSpeakerConfig = 2 then // DSSPEAKER_MONO
      SpeakerPage.Values[0] := True
    else if systemSpeakerConfig = 1 then // DSSPEAKER_HEADPHONE
      SpeakerPage.Values[1] := True
    else if systemSpeakerConfig = 3 then // DSSPEAKER_QUAD
      SpeakerPage.Values[4] := True
    else if systemSpeakerConfig = 5 then // DSSPEAKER_SURROUND
      SpeakerPage.Values[6] := True
    else if systemSpeakerConfig = 6 then // DSSPEAKER_5POINT1
      SpeakerPage.Values[7] := True
    else if systemSpeakerConfig = 7 then // DSSPEAKER_7POINT1
      SpeakerPage.Values[7] := True
    else if systemSpeakerConfig = 8 then // 7.1ch hometheater
      SpeakerPage.Values[7] := True;
  end

  // Check boxes
  chbVoicecontrol := TCheckBox.Create(SpeakerPage);
  chbVoicecontrol.Top := ScaleY(222);
  chbVoicecontrol.Left := ScaleX(20);
  chbVoicecontrol.Width := ScaleX(170);
  chbVoicecontrol.Height := ScaleY(16);
  chbVoicecontrol.Caption := ExpandConstant('{cm:spk_VoiceControl}');
  ii := ffRegReadDWordHKCU('Software\GNU\ffdshow_audio\default', 'mixerVoiceControl',0);
  if ii = 0 then
    chbVoicecontrol.Checked := False
  else
    chbVoicecontrol.Checked := True;
  chbVoicecontrol.Parent := SpeakerPage.Surface;

  chbExpandStereo := TCheckBox.Create(SpeakerPage);
  chbExpandStereo.Top := ScaleY(222);
  chbExpandStereo.Left := ScaleX(200);
  chbExpandStereo.Width := ScaleX(200);
  chbExpandStereo.Height := ScaleY(16);
  chbExpandStereo.Caption := ExpandConstant('{cm:spk_ExpandStereo}');
  ii := ffRegReadDWordHKCU('Software\GNU\ffdshow_audio\default', 'mixerExpandStereo',0);
  if ii = 0 then
    chbExpandStereo.Checked := False
  else
    chbExpandStereo.Checked := True;
  chbExpandStereo.Parent := SpeakerPage.Surface;

// VirtualDub plugin install directory setting

#if include_app_plugins
  VdubDirPage := CreateInputDirPage(SpeakerPage.ID,
    ExpandConstant('{cm:SelectPluginDirLabel1,Virtual Dub}'),
    ExpandConstant('{cm:SelectPluginDirLabel2,Virtual Dub}'),
    ExpandConstant('{cm:SelectPluginDirLabel3,Virtual Dub}'),
    False, '');
  VdubDirPage.Add('');
#endif
end;

function ShouldSkipPage(PageID: Integer): Boolean;
#if include_app_plugins
var
  regval: String;
#endif
begin
  Result := False;
#if include_app_plugins
  if PageID = VdubDirPage.ID then begin
    if IsComponentSelected('ffdshow\plugins\virtualdub') then begin
      if VdubDirPage.Values[0] = '' then begin
        if RegQueryStringValue(HKLM, 'Software\GNU\ffdshow', 'pthVirtualDub', regval)
        and not (regval = ExpandConstant('{app}')) and not (regval = '') then
          VdubDirPage.Values[0] := regval
        else if FileOrDirExists(ExpandConstant('{pf}\virtualDub\plugins')) then
            VdubDirPage.Values[0] := ExpandConstant('{pf}\virtualDub\plugins')
        else if FileOrDirExists(ExpandConstant('{sd}\virtualDub\plugins')) then
            VdubDirPage.Values[0] := ExpandConstant('{sd}\virtualDub\plugins')
        else
          VdubDirPage.Values[0] := ExpandConstant('{app}');
      end
    end
    else begin
      Result := True;
    end
  end
#endif

  if PageID = ComplistVideo.page.ID then begin
    if ffRegReadDWordHKCU('Software\GNU\ffdshow','dontaskComp',0) = 1 then begin
      Result := True;
      ComplistVideo.skipped := True;
    end
  end
  if PageID = ComplistAudio.page.ID then begin
    if ffRegReadDWordHKCU('Software\GNU\ffdshow_audio','dontaskComp',0) = 1 then begin
      Result := True;
      ComplistAudio.Skipped := True;
    end
  end
  priorPageID := PageID;
end;

procedure showMsgAdded(PageID: Integer; complist: TcomplistPage);
begin
  if (PageID = complist.page.ID) AND NOT Complist_isMsgAddedShown AND NOT WizardSilent then begin
    if complist.countAdded = 1 then begin
      MsgBox(ExpandConstant('{cm:comp_oneCompAppAdded}'), mbInformation, MB_OK);
      Complist_isMsgAddedShown := True;
    end
    if complist.countAdded > 1 then begin
      MsgBox(ExpandConstant('{cm:comp_multiCompAppAdded}'), mbInformation, MB_OK);
      Complist_isMsgAddedShown := True;
    end
  end
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  showMsgAdded(CurPageID, ComplistVideo);
  showMsgAdded(CurPageID, ComplistAudio);
end;
