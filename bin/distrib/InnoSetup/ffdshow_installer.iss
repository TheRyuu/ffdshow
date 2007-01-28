; Written by CLSID
; Requires Inno Setup (http://www.innosetup.com) and ISPP (http://sourceforge.net/projects/ispp/)
; Place this script in directory: /bin/distrib/innosetup/

#define tryout_revision = 796
#define buildyear = 2007
#define buildmonth = '01'
#define buildday = '18'

; Build specific options
#define unicode_required = True

#define include_cpu_detection = False
#define mmx_required = True
#define sse_required = False
#define sse2_required = False
#define _3dnow_required = False
#define ext3dnow_required = False

#define MSVC80 = True

#define localize = True

#define include_app_plugins = True
#define include_makeavis = True
#define include_audx = False
#define include_info_before = False
#define include_gnu_license = True
#define include_setup_icon = False

#define filename_prefix = ''
#define outputdir = '.'

; Custom builder preferences
#define PREF_CLSID = False
#define PREF_YAMAGATA = False
#define PREF_XXL = False

#if PREF_CLSID
  #define MSVC80 = False
  #define unicode_required = False
  #define include_audx = True
  #define filename_prefix = '_clsid'
  #define outputdir = '..\..\..\..\'
#endif
#if PREF_YAMAGATA
  #define MSVC80 = True
  #define unicode_required = False
  #define include_audx = True
  #define filename_prefix = '_Q'
#endif
#if PREF_XXL
  #define MSVC80 = False
  #define unicode_required = False
  #define localize = False
  #define include_audx = True
  #define include_info_before = True
  #define include_setup_icon = True
  #define include_cpu_detection = True
  #define sse_required = False
  #define sse2_required = False
  #define filename_prefix = '_xxl'
#endif

[Setup]
AllowCancelDuringInstall=no
AllowNoIcons=yes
AllowUNCPath=no
AppId=ffdshow
AppName=ffdshow
AppVerName=ffdshow [rev {#= tryout_revision}] [{#= buildyear}-{#= buildmonth}-{#= buildday}]
AppVersion=1.0
Compression=lzma/ultra
InternalCompressLevel=ultra
SolidCompression=true
DefaultDirName={code:GetDefaultInstallDir|}
DefaultGroupName=ffdshow
DirExistsWarning=no
#if include_info_before
InfoBeforeFile=infobefore.rtf
#endif
#if include_gnu_license
LicenseFile=gnu_license.txt
#endif
#if MSVC80
  #if unicode_required
MinVersion=0,4.0.1381
  #else
MinVersion=4.1,4.0.1381
  #endif
#else
  #if unicode_required
MinVersion=0,4
  #endif
#endif
OutputBaseFilename=ffdshow_rev{#= tryout_revision}_{#= buildyear}{#= buildmonth}{#= buildday}{#= filename_prefix}
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
Name: video\divx; Description: DivX; Check: CheckTaskVideoXvid('dx50'); Components: ffdshow
Name: video\divx; Description: DivX; Check: NOT CheckTaskVideoXvid('dx50'); Components: ffdshow; Flags: unchecked
Name: video\xvid; Description: Xvid; Check: CheckTaskVideoXvid('xvid'); Components: ffdshow
Name: video\xvid; Description: Xvid; Check: NOT CheckTaskVideoXvid('xvid'); Components: ffdshow; Flags: unchecked
Name: video\mp4v; Description: MP4V; Check: CheckTaskVideoXvid('divx'); Components: ffdshow
Name: video\mp4v; Description: MP4V; Check: NOT CheckTaskVideoXvid('divx'); Components: ffdshow; Flags: unchecked
Name: video\mpeg4; Description: {cm:genericMpeg4}; Check: CheckTaskVideoXvid('_3iv'); Components: ffdshow
Name: video\mpeg4; Description: {cm:genericMpeg4}; Check: NOT CheckTaskVideoXvid('_3iv'); Components: ffdshow; Flags: unchecked
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
Name: video\wmv1; Description: WMV1; Check: CheckTaskVideo('wmv1', 1, False); Components: ffdshow
Name: video\wmv1; Description: WMV1; Check: NOT CheckTaskVideo('wmv1', 1, False); Flags: dontinheritcheck unchecked; Components: ffdshow
Name: video\wmv2; Description: WMV2; Check: CheckTaskVideo('wmv2', 1, False); Components: ffdshow
Name: video\wmv2; Description: WMV2; Check: NOT CheckTaskVideo('wmv2', 1, False); Flags: dontinheritcheck unchecked; Components: ffdshow
Name: video\wmv3; Description: WMV3; Check: CheckTaskVideo('wmv3', 1, False); Components: ffdshow
Name: video\wmv3; Description: WMV3; Check: NOT CheckTaskVideo('wmv3', 1, False); Flags: dontinheritcheck unchecked; Components: ffdshow
Name: video\wvc1; Description: WVC1; Check: CheckTaskVideo('wvc1', 12, False); Components: ffdshow
Name: video\wvc1; Description: WVC1; Check: NOT CheckTaskVideo('wvc1', 12, False); Flags: dontinheritcheck unchecked; Components: ffdshow
Name: video\wvp2; Description: WMVP, WVP2; Check: CheckTaskVideo('wvp2', 12, False); Components: ffdshow
Name: video\wvp2; Description: WMVP, WVP2; Check: NOT CheckTaskVideo('wvp2', 12, False); Flags: dontinheritcheck unchecked; Components: ffdshow
Name: video\mss2; Description: MSS1, MSS2; Check: CheckTaskVideo('mss2', 12, False); Components: ffdshow
Name: video\mss2; Description: MSS1, MSS2; Check: NOT CheckTaskVideo('mss2', 12, False); Flags: dontinheritcheck unchecked; Components: ffdshow
Name: video\dvsd; Description: DV; Check: CheckTaskVideo('dvsd', 1, False); Components: ffdshow
Name: video\dvsd; Description: DV; Check: NOT CheckTaskVideo('dvsd', 1, False); Flags: dontinheritcheck unchecked; Components: ffdshow
Name: video\other1; Description: H.261, MJPEG, Theora, VP3; Check: CheckTask('video\other1', True); Components: ffdshow
Name: video\other1; Description: H.261, MJPEG, Theora, VP3; Check: NOT CheckTask('video\other1', True); Flags: unchecked; Components: ffdshow
Name: video\other2; Description: CorePNG, MS Video 1, MSRLE, Techsmith, Truemotion; Check: CheckTask('video\other2', True); Components: ffdshow
Name: video\other2; Description: CorePNG, MS Video 1, MSRLE, Techsmith, Truemotion; Check: NOT CheckTask('video\other2', True); Flags: unchecked; Components: ffdshow
Name: video\other3; Description: ASV1/2, CYUV, ZLIB, 8BPS, LOCO, MSZH, QPEG, WNV1, VCR1; Check: CheckTask('video\other3', False); Components: ffdshow
Name: video\other3; Description: ASV1/2, CYUV, ZLIB, 8BPS, LOCO, MSZH, QPEG, WNV1, VCR1; Check: NOT CheckTask('video\other3', False); Flags: unchecked; Components: ffdshow
Name: video\other4; Description: CamStudio, ZMBV, Ultimotion, VIXL, AASC, IV32, FPS1, RT21; Check: CheckTask('video\other4', False); Components: ffdshow
Name: video\other4; Description: CamStudio, ZMBV, Ultimotion, VIXL, AASC, IV32, FPS1, RT21; Check: NOT CheckTask('video\other4', False); Flags: unchecked; Components: ffdshow
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
Name: audio\adpcm; Description: ADPCM, MS GSM, Truespeech; Check: CheckTask('audio\adpcm', False); Components: ffdshow
Name: audio\adpcm; Description: ADPCM, MS GSM, Truespeech; Check: NOT CheckTask('audio\adpcm', False); Flags: unchecked; Components: ffdshow
Name: audio\rawa; Description: {cm:rawaudio}; Check: CheckTaskAudio('rawa', 4, False); Flags: dontinheritcheck; Components: ffdshow
Name: audio\rawa; Description: {cm:rawaudio}; Check: NOT CheckTaskAudio('rawa', 4, False); Flags: dontinheritcheck unchecked; Components: ffdshow
Name: filter; Description: {cm:defaultfilters}; Flags: unchecked; Components: ffdshow
Name: filter\normalize; Description: {cm:volumenorm}; Check: CheckTask('filter\normalize', False); Components: ffdshow
Name: filter\normalize; Description: {cm:volumenorm}; Check: NOT CheckTask('filter\normalize', False); Flags: unchecked; Components: ffdshow
Name: filter\subtitles; Description: {cm:subtitles}; Check: CheckTask('filter\subtitles', False); Components: ffdshow
Name: filter\subtitles; Description: {cm:subtitles}; Check: NOT CheckTask('filter\subtitles', False); Flags: unchecked; Components: ffdshow
#if !PREF_YAMAGATA
Name: tweaks; Description: {cm:tweaks}; Flags: unchecked; Components: ffdshow
Name: tweaks\skipinloop; Description: {cm:skipinloop}; Check: CheckTask('tweaks\skipinloop', False); Components: ffdshow
Name: tweaks\skipinloop; Description: {cm:skipinloop}; Check: NOT CheckTask('tweaks\skipinloop', False); Flags: unchecked; Components: ffdshow
#endif

[Icons]
Name: {group}\{cm:audioconfig}; Filename: rundll32.exe; Parameters: ffdshow.ax,configureAudio; WorkingDir: {app}; IconFilename: {app}\ffdshow.ax; IconIndex: 1; Components: ffdshow
Name: {group}\{cm:videoconfig}; Filename: rundll32.exe; Parameters: ffdshow.ax,configure; WorkingDir: {app}; IconFilename: {app}\ffdshow.ax; IconIndex: 0; Components: ffdshow
Name: {group}\{cm:vfwconfig}; Filename: rundll32.exe; Parameters: ff_vfw.dll,configureVFW; IconFilename: {app}\ffdshow.ax; IconIndex: 2; Components: ffdshow\vfw
#if include_makeavis
Name: {group}\makeAVIS; Filename: {app}\makeAVIS.exe; Components: ffdshow\makeavis
#endif
Name: {group}\{cm:uninstall}; Filename: {uninstallexe}

[Files]
; For speaker config
Source: msvc71\ffSpkCfg.dll; Flags: dontcopy

; MSVC71 runtimes are required for ffdshow components that are placed outside the ffdshow installation directory.
Source: msvcp71.dll; DestDir: {sys}; Flags: onlyifdoesntexist
Source: msvcr71.dll; DestDir: {sys}; Flags: onlyifdoesntexist
#if MSVC80
; Install MSVC80 runtime as private assembly (can only be used by components that are in the same directory).
Source: ..\msvcr80.dll; DestDir: {app}; MinVersion: 4.1,5; Flags: ignoreversion restartreplace uninsrestartdelete
Source: ..\NT4.0\msvcr80.dll; DestDir: {app}; MinVersion: 0,4; OnlyBelowVersion: 0,5; Flags: ignoreversion restartreplace uninsrestartdelete
Source: ..\microsoft.vc80.crt.manifest; DestDir: {app}; Flags: ignoreversion restartreplace uninsrestartdelete
Source: msvc80\ffdshow.ax.manifest; DestDir: {app}; Flags: ignoreversion restartreplace uninsrestartdelete; MinVersion: 0,5.01; OnlyBelowVersion: 0,5.02
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
Source: ..\..\ff_x264.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
Source: icl9\ff_kernelDeint.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
Source: icl9\TomsMoComp_ff.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow
Source: ..\..\libmpeg2_ff.dll; DestDir: {app}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow

; Single build:
#if !PREF_CLSID
Source: ..\..\ffdshow.ax; DestDir: {app}; Flags: ignoreversion regserver restartreplace uninsrestartdelete; Components: ffdshow
#endif
; ANSI + Unicode:
#if PREF_CLSID
Source: ..\..\ffdshow_ansi.ax; DestName: ffdshow.ax; DestDir: {app}; Flags: ignoreversion regserver restartreplace uninsrestartdelete; MinVersion: 4,0; Components: ffdshow
Source: ..\..\ffdshow_unicode.ax; DestName: ffdshow.ax; DestDir: {app}; Flags: ignoreversion regserver restartreplace uninsrestartdelete; MinVersion: 0,4; Components: ffdshow
#endif
; Multi build example (requires cpu detection to be enabled):
;Source: ..\..\ffdshow_generic.ax; DestName: ffdshow.ax; DestDir: {app}; Flags: ignoreversion regserver restartreplace uninsrestartdelete; Check: Is_MMX_Supported AND NOT Is_SSE_Supported; Components: ffdshow
;Source: ..\..\ffdshow_sse.ax; DestName: ffdshow.ax; DestDir: {app}; Flags: ignoreversion regserver restartreplace uninsrestartdelete; Check: Is_SSE_Supported AND NOT Is_SSE2_Supported; Components: ffdshow
;Source: ..\..\ffdshow_sse2.ax; DestName: ffdshow.ax; DestDir: {app}; Flags: ignoreversion regserver restartreplace uninsrestartdelete; Check: Is_SSE2_Supported; Components: ffdshow

#if !MSVC80
Source: ..\ffdshow.ax.manifest; DestDir: {app}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow
#endif

; Single build:
#if !PREF_CLSID
Source: ..\..\ff_wmv9.dll; DestDir: {app}; Flags: ignoreversion; Components: ffdshow\vfw
#endif
; ANSI + Unicode:
#if PREF_CLSID
Source: ..\..\ff_wmv9_ansi.dll; DestName: ff_wmv9.dll; DestDir: {app}; Flags: ignoreversion; MinVersion: 4,0; Components: ffdshow\vfw
Source: ..\..\ff_wmv9_unicode.dll; DestName: ff_wmv9.dll; DestDir: {app}; Flags: ignoreversion; MinVersion: 0,4; Components: ffdshow\vfw
#endif

; If you want to use MSVC8 for ffdshow.ax, ff_vfw.dll should be compiled by GCC. Both MSVC7.1 and MSVC8 does not work in some environment such as Windows XP SP2 without shared assembly of MSVCR80.
Source: ..\..\ff_vfw.dll; DestDir: {sys}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\vfw
Source: ..\ff_vfw.dll.manifest; DestDir: {sys}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\vfw

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
Source: ..\makeAVIS.exe.manifest; DestDir: {app}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\makeavis
  #endif
  #if MSVC80
Source: msvc71\ff_acm.acm; DestDir: {sys}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\makeavis
  #else
Source: ..\..\ff_acm.acm; DestDir: {sys}; Flags: ignoreversion restartreplace uninsrestartdelete; Components: ffdshow\makeavis
  #endif
#endif
Source: ..\..\languages\*.*; DestDir: {app}\languages; Flags: ignoreversion; Components: ffdshow
Source: ..\..\custom matrices\*.*; DestDir: {app}\custom matrices; Flags: ignoreversion; Components: ffdshow\vfw

[InstallDelete]
#if MSVC80
Type: files; Name: {app}\ffdshow.ax.manifest; Components: ffdshow
Type: files; Name: {app}\makeAVIS.exe.manifest; Components: ffdshow\makeavis
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
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers.desc; ValueType: string; ValueName: ff_vfw.dll; ValueData: ffdshow video encoder; MinVersion: 0,4; Flags: uninsdeletevalue; Components: ffdshow\vfw
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Drivers32; ValueType: string; ValueName: VIDC.FFDS; ValueData: ff_vfw.dll; MinVersion: 0,4; Flags: uninsdeletevalue; Components: ffdshow\vfw
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.FFDS; Flags: uninsdeletekey; Components: ffdshow\vfw
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.FFDS; ValueType: string; ValueName: Description; ValueData: ffdshow video encoder; Components: ffdshow\vfw
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.FFDS; ValueType: string; ValueName: Driver; ValueData: ff_vfw.dll; Components: ffdshow\vfw
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.FFDS; ValueType: string; ValueName: FriendlyName; ValueData: ffdshow video encoder; Components: ffdshow\vfw
#if include_makeavis
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers.desc; ValueType: string; ValueName: ff_acm.acm; ValueData: ffdshow ACM codec; MinVersion: 0,4; Flags: uninsdeletevalue; Components: ffdshow\makeavis
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Drivers32; ValueType: string; ValueName: msacm.avis; ValueData: ff_acm.acm; MinVersion: 0,4; Flags: uninsdeletevalue; Components: ffdshow\makeavis
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\acm\msacm.avis; Flags: uninsdeletekey; Components: ffdshow\makeavis
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\acm\msacm.avis; ValueType: string; ValueName: Description; ValueData: ffdshow ACM codec; Components: ffdshow\makeavis
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\acm\msacm.avis; ValueType: string; ValueName: Driver; ValueData: ff_acm.acm; Components: ffdshow\makeavis
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\MediaResources\acm\msacm.avis; ValueType: string; ValueName: FriendlyName; ValueData: ffdshow ACM codec; Components: ffdshow\makeavis
;Root: HKLM; SubKey: SOFTWARE\Microsoft\AudioCompressionManager\DriverCache\msacm.avis; Flags: uninsdeletekey; Components: ffdshow\makeavis
;Root: HKLM; SubKey: SOFTWARE\Microsoft\AudioCompressionManager\DriverCache\msacm.avis; ValueType: binary; ValueName: aFormatTagCache; ValueData: 01 00 00 00 10 00 00 00 13 33 00 00 12 00 00 00; Components: ffdshow\makeavis
;Root: HKLM; SubKey: SOFTWARE\Microsoft\AudioCompressionManager\DriverCache\msacm.avis; ValueType: dword; ValueName: cFilterTags; ValueData: 0; Components: ffdshow\makeavis
;Root: HKLM; SubKey: SOFTWARE\Microsoft\AudioCompressionManager\DriverCache\msacm.avis; ValueType: dword; ValueName: cFormatTags; ValueData: 2; Components: ffdshow\makeavis
;Root: HKLM; SubKey: SOFTWARE\Microsoft\AudioCompressionManager\DriverCache\msacm.avis; ValueType: dword; ValueName: fdwSupport; ValueData: 1; Components: ffdshow\makeavis
#endif

; Recommended settings
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: allowOutChange; ValueData: 2; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: hwOverlay; ValueData: 2; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: idct; ValueData: 1; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: outChangeCompatOnly; ValueData: 1; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: postprocH264mode; ValueData: 0; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: resizeMethod; ValueData: 9; Flags: createvalueifdoesntexist; Components: ffdshow

#if !PREF_YAMAGATA
Root: HKCU; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: fastH264; ValueData: 0; Components: ffdshow; Tasks: NOT tweaks\skipinloop
Root: HKCU; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: fastH264; ValueData: 2; Components: ffdshow; Tasks: tweaks\skipinloop
#endif

Root: HKCU; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: subTextpin; ValueData: 1; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: subTextpinSSA; ValueData: 1; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: subIsExpand; ValueData: 0; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: isSubtitles; ValueData: 0; Components: ffdshow; Tasks: NOT filter\subtitles
Root: HKCU; Subkey: Software\GNU\ffdshow\default; ValueType: dword; ValueName: isSubtitles; ValueData: 1; Components: ffdshow; Tasks: filter\subtitles

Root: HKCU; Subkey: Software\GNU\ffdshow_audio\default; ValueType: dword; ValueName: mixerNormalizeMatrix; ValueData: 0; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio\default; ValueType: dword; ValueName: isvolume; ValueData: 0; Components: ffdshow; Tasks: NOT filter\normalize
Root: HKCU; Subkey: Software\GNU\ffdshow_audio\default; ValueType: dword; ValueName: isvolume; ValueData: 1; Components: ffdshow; Tasks: filter\normalize
Root: HKCU; Subkey: Software\GNU\ffdshow_audio\default; ValueType: dword; ValueName: ismixer; ValueData: {code:Get_ismixer}; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio\default; ValueType: dword; ValueName: mixerOut; ValueData: {code:Get_mixerOut}; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio\default; ValueType: dword; ValueName: mixerExpandStereo; ValueData: {code:Get_mixerExpandStereo}; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio\default; ValueType: dword; ValueName: mixerVoiceControl; ValueData: {code:Get_mixerVoiceControl}; Components: ffdshow

Root: HKLM; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: allowOutChange; ValueData: 2; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKLM; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: hwOverlay; ValueData: 2; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKLM; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: outChangeCompatOnly; ValueData: 1; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKLM; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: postprocH264mode; ValueData: 0; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKLM; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: idct; ValueData: 1; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKLM; SubKey: Software\GNU\ffdshow; ValueType: dword; ValueName: libtheoraPostproc; ValueData: 0; Flags: createvalueifdoesntexist; Components: ffdshow

; Blacklist
Root: HKCU; Subkey: Software\GNU\ffdshow; ValueType: dword; ValueName: isBlacklist; ValueData: 1; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow; ValueType: String; ValueName: blacklist; ValueData: "explorer.exe;oblivion.exe;morrowind.exe;powerdvd.exe"; Flags: createvalueifdoesntexist; OnlyBelowVersion: 0,6; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow; ValueType: String; ValueName: blacklist; ValueData: "oblivion.exe;morrowind.exe;powerdvd.exe"; Flags: createvalueifdoesntexist; MinVersion: 0,6; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio; ValueType: dword; ValueName: isBlacklist; ValueData: 1; Flags: createvalueifdoesntexist; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio; ValueType: String; ValueName: blacklist; ValueData: "explorer.exe;oblivion.exe;morrowind.exe"; Flags: createvalueifdoesntexist; OnlyBelowVersion: 0,6; Components: ffdshow
Root: HKCU; Subkey: Software\GNU\ffdshow_audio; ValueType: String; ValueName: blacklist; ValueData: "oblivion.exe;morrowind.exe"; Flags: createvalueifdoesntexist; MinVersion: 0,6; Components: ffdshow

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
Description: {cm:runvfwconfig}; Filename: rundll32.exe; Parameters: ff_vfw.dll,configureVFW; WorkingDir: {app}; Flags: postinstall nowait unchecked; Components: ffdshow\vfw

; All custom strings in the installer:
#include "custom_messages.iss"

[Code]
// Global vars
var
  reg_mixerOut: Cardinal;
  reg_ismixer: Cardinal;
  SpeakerPage: TInputOptionWizardPage;
  chbVoicecontrol: TCheckBox;
  chbExpandStereo: TCheckBox;
  is8DisableMixer: Boolean;

function CheckTask(name: String; defaultvalue: Boolean): Boolean;
var
  regval: String;
begin
  if RegQueryStringValue(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\ffdshow_is1', 'Inno Setup: Selected Tasks', regval) then begin
    Result := (Pos(name, regval) > 0);
  end
  else begin
    Result := defaultvalue;
  end
end;

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

function CheckTaskVideoXvid(name: String): Boolean;
var
  regval: Cardinal;
begin
  Result := False;
  if RegQueryDwordValue(HKCU, 'Software\GNU\ffdshow', name, regval) then begin
    Result := NOT(regval = 0);
  end
  else begin
    if RegQueryDwordValue(HKLM, 'Software\GNU\ffdshow', name, regval) then begin
      Result := NOT(regval = 0);
    end
    else begin
      Result := TRUE;
    end
  end
end;

function GetTaskVideoI(name: String; defaultvalue: Cardinal): String;
var
  regval: Cardinal;
begin
  if NOT RegQueryDwordValue(HKCU, 'Software\GNU\ffdshow', name, regval) then
  if NOT RegQueryDwordValue(HKLM, 'Software\GNU\ffdshow', name, regval) then
    regval :=defaultvalue;
  if regval = 0 then
    regval :=defaultvalue;
  Result := IntToStr(regval);
end;

function GetTaskVideoXvid(name: String): String;
begin
  Result := GetTaskVideoI(name, 1);
end;

function GetTaskVideoTheora(name: String): String;
begin
  Result := GetTaskVideoI(name, 3);
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
  if NOT RegQueryStringValue(HKLM, 'Software\GNU\ffdshow', 'pth', Result) OR (Length(Result) = 0) then begin
    Result := ExpandConstant('{pf}\ffdshow');
  end
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
end;

#if false
procedure UninstallBuildUsingNSIS();
var
  regval: String;
  ResultCode: Integer;
begin
  if RegQueryStringValue(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\ffdshow', 'UninstallString', regval) then begin
    if FileExists(RemoveQuotes(regval)) then begin
      if msgbox('Setup has detected that you already have another build of ffdshow installed.'#13#10'It is recommended to uninstall that build first because it uses a different installer.'#13#10#13#10'Uninstall it now?', mbConfirmation, MB_YESNO) = IDYES then begin
        if msgbox('Uninstalling your current build will also reset your ffdshow settings.'#13#10#13#10'Are you sure you want to uninstall your current build?', mbConfirmation, MB_YESNO) = IDYES then begin
          if NOT Exec(RemoveQuotes(regval), '/S', '', SW_SHOW, ewWaitUntilTerminated, ResultCode) then begin
            MsgBox(SysErrorMessage(resultCode), mbError, MB_OK);
          end
        end
      end
    end
  end
end;
#endif

procedure RemoveBuildUsingNSIS();
var
  regval: String;
  ResultCode: Integer;
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
    #if mmx_required
    if NOT Is_MMX_Supported() then begin
      Result := False;
      msgbox('This build of ffdshow requires a CPU with MMX extension support. Your CPU does not have those capabilities.', mbError, MB_OK);
    end
    #endif
    #if sse_required
    if NOT Is_SSE_Supported() then begin
      Result := False;
      msgbox('This build of ffdshow requires a CPU with SSE extension support. Your CPU does not have those capabilities.', mbError, MB_OK);
    end
    #endif
    #if sse2_required
    if NOT Is_SSE2_Supported() then begin
      Result := False;
      msgbox('This build of ffdshow requires a CPU with SSE2 extension support. Your CPU does not have those capabilities.', mbError, MB_OK);
    end
    #endif
    #if _3dnow_required
    if NOT Is_3dnow_Supported() then begin
      Result := False;
      msgbox('This build of ffdshow requires a CPU with 3dnow extension support. Your CPU does not have those capabilities.', mbError, MB_OK);
    end
    #endif
    #if ext3dnow_required
    if NOT Is_ext3dnow_Supported() then begin
      Result := False;
      msgbox('This build of ffdshow requires a CPU with 3dnowext extension support. Your CPU does not have those capabilities.', mbError, MB_OK);
    end
    #endif
  #endif
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep=ssInstall then begin
    RemoveBuildUsingNSIS;
  end
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

function ffRegReadDWordHKCU(regKeyName: String; regValName: String; defaultValue: Cardinal): Cardinal;
var
  regval: Cardinal;
begin
  if NOT RegQueryDwordValue(HKCU, regKeyName, regValName, Result) then
    Result :=defaultValue;
end;

procedure InitializeWizard;
var
  systemSpeakerConfig: Integer;
  reg_isSpkCfg: Cardinal;
  isMajorType: Boolean;
  ii: Cardinal;
begin

  { Create the pages }

// Speaker setup

  is8DisableMixer := False;
  SpeakerPage := CreateInputOptionPage(wpSelectTasks,
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

#if include_app_plugins
function ShouldSkipPage(PageID: Integer): Boolean;
var
  regval: String;
begin
  Result := False;
  if PageID = VdubDirPage.ID then begin
    if IsComponentSelected('ffdshow\plugins\virtualdub') then begin
      if VdubDirPage.Values[0] = '' then begin
        if RegQueryStringValue(HKLM, 'Software\GNU\ffdshow', 'pthVirtualDub', regval)
        and not (regval = ExpandConstant('{app}')) then
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
end;
#endif
