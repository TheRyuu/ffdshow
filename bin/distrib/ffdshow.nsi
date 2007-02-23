!packhdr "ffdshow.dat" "upx --best ffdshow.dat"

;based on MUI Welcome/Finish Page Example Script written by Joost Verburg

!verbose 3

Name "ffdshow"
!define FFDSHOW_VERSION "20060929-rev289"

; For those who use MSVC8 for their products,
; it is strongly recommended to define MSVCRT8
!define MSVCRT8

; If you want to include vcredist_x86.exe, enable PACK_VCREDIST_X86.
; vcredist_x86.exe is a large file, the installer's size is increased by 2.5MB.
; Stabe build should include vcredist_x86.exe while nightly build may dislike the size.
;!define PACK_VCREDIST_X86
!define PACKED_VCREDIST_VERSION 805072742

; If shared assembly "MICROSOFT.VC80.CRT" is not installed,
; and if vcredist_x86.exe is packed the installer try to install vcredist_x86.
;
; If vcredist_x86 is not packed or the installation of vcredist_x86 failed,
; and if MSVCR8 is defined,
; the installer try to install msvcr80.dll as private assembly.
;
; Installation of private assembly have version check, reference count controls, etc.
; The version of VC80.CRT is checked before executing vcredist_x86.exe.

!ifdef PACK_VCREDIST_X86
  !ifndef MSVCRT8
    !define MSVCRT8
  !endif
!endif

; Configure the following, if you are trying mix build including MSVC8.
!ifdef MSVCRT8
  !define AX_BY_VC8
  !define MAKEAVIS_BY_VC8
  ;!define FF_VFW_BY_VC8 # don't do this. should be compiled by GCC or MSVC7. private assembly loading fails
!endif

;!define AUDX

!define MUI_ICON  modern-wizard_icon.ico
!define MUI_UNICON  modern-wizard_icon.ico
!define MUI_WELCOMEFINISHPAGE_BITMAP modern-wizard_ff.bmp
!define MUI_UNWELCOMEFINISHPAGE_BITMAP modern-wizard_ff.bmp
!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_LANGDLL_ALWAYSSHOW

!include "MUI.nsh"
!include "Sections.nsh"
!include "LogicLib.nsh"
!include "StrFunc.nsh"
!include "Library.nsh"

${StrStr}
${UnStrStr}
!include "moviesources.nsh"

!ifdef X64
 !define FFDSHOW_X64 "64"
!else
 !define FFDSHOW_X64 ""
!endif

!ifdef CORE
 !define FFDSHOW_CORE "-core"
!else
 !define FFDSHOW_CORE ""
!endif

!ifdef ADDONS
 !define FFDSHOW_ADDONS "-addons"
!else
 !define FFDSHOW_ADDONS ""
!endif

!ifdef DEC
 !define FFDSHOW_DEC "-dec"
!else
 !define FFDSHOW_DEC ""
!endif

!ifdef SIMPLE
 !define FFDSHOW_SIMPLE "-simple"
!else
 !define FFDSHOW_SIMPLE ""
!endif

;General
!define FULLNAME ffdshow${FFDSHOW_X64}${FFDSHOW_CORE}${FFDSHOW_ADDONS}${FFDSHOW_DEC}${FFDSHOW_SIMPLE}-${FFDSHOW_VERSION}
OutFile "${FULLNAME}.exe"
Caption "${FULLNAME}"

!ifndef COMPRESSION
  !define COMPRESSION lzma
!endif
SetCompressor /SOLID ${COMPRESSION}

!define MUI_LANGDLL_REGISTRY_ROOT "HKLM"
!define MUI_LANGDLL_REGISTRY_KEY "SOFTWARE\GNU\ffdshow"
!define MUI_LANGDLL_REGISTRY_VALUENAME "lang"

;--------------------------------
;Configuration

!insertmacro MUI_PAGE_WELCOME
!ifdef SIMPLE
  ;Page custom CustomPageSimple
!else
  !insertmacro MUI_PAGE_LICENSE $(ffdshowLicense)
  !insertmacro MUI_PAGE_COMPONENTS
  Page custom CustomPageCodecVideo
  Page custom CustomPageCodecAudio
  Page custom CustomPageFiltersVideo
  Page custom CustomPageFiltersAudio
  !ifndef CORE
    Page custom CustomPageAvisynth
    Page custom CustomPageVirtualDub
    !ifndef X64
      Page custom CustomPageDscaler
    !endif
  !endif
  !insertmacro MUI_PAGE_DIRECTORY
!endif
Var SHORTCUTS_FOLDER
!define MUI_STARTMENUPAGE_REGISTRY_ROOT ${MUI_LANGDLL_REGISTRY_ROOT}
!define MUI_STARTMENUPAGE_REGISTRY_KEY ${MUI_LANGDLL_REGISTRY_KEY}
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "startmenufolder"
!ifndef ADDONS
 !ifndef SIMPLE
   !insertmacro MUI_PAGE_STARTMENU ffdshowApplication $SHORTCUTS_FOLDER
 !else
 !endif
!endif
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!define MUI_ABORTWARNING

;Language
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Czech"
!insertmacro MUI_LANGUAGE "Slovak"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "Italian"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "French"

!define NSH_LANG ${LANG_ENGLISH}
!include 'en.nsh'

!undef NSH_LANG
!define NSH_LANG ${LANG_SLOVAK}
!include 'sk.nsh'

!undef NSH_LANG
!define NSH_LANG ${LANG_CZECH}
!include 'cz.nsh'

!undef NSH_LANG
!define NSH_LANG ${LANG_GERMAN}
!include 'de.nsh'

!undef NSH_LANG
!define NSH_LANG ${LANG_KOREAN}
!include 'kor.nsh'

!undef NSH_LANG
!define NSH_LANG ${LANG_SIMPCHINESE}
!include 'cn.nsh'

!undef NSH_LANG
!define NSH_LANG ${LANG_RUSSIAN}
!include 'ru.nsh'

!undef NSH_LANG
!define NSH_LANG ${LANG_ITALIAN}
!include 'it.nsh'

!undef NSH_LANG
!define NSH_LANG ${LANG_JAPANESE}
!include 'jp.nsh'

!undef NSH_LANG
!define NSH_LANG ${LANG_FRENCH}
!include 'fr.nsh'

;without ffdshow specific translation, use English for now
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "Hungarian"
!insertmacro MUI_LANGUAGE "Polish"
!insertmacro MUI_LANGUAGE "Swedish"
!insertmacro MUI_LANGUAGE "TradChinese"
!insertmacro MUI_LANGUAGE "Bulgarian"

!undef NSH_LANG
!define NSH_LANG 1046
!include 'en.nsh'

!undef NSH_LANG
!define NSH_LANG ${LANG_SPANISH}
!include 'en.nsh'

!undef NSH_LANG
!define NSH_LANG ${LANG_HUNGARIAN}
!include 'en.nsh'

!undef NSH_LANG
!define NSH_LANG ${LANG_POLISH}
!include 'en.nsh'

!undef NSH_LANG
!define NSH_LANG ${LANG_SWEDISH}
!include 'en.nsh'

!undef NSH_LANG
!define NSH_LANG ${LANG_TRADCHINESE}
!include 'en.nsh'

!undef NSH_LANG
!define NSH_LANG ${LANG_BULGARIAN}
!include 'bg.nsh'

;Folder-selection page
InstallDir "$PROGRAMFILES\ffdshow"
; Registry key to check for directory (so if you install again, it will
; overwrite the old one automatically)
InstallDirRegKey HKLM SOFTWARE\GNU\ffdshow "pth"

AutoCloseWindow false

!ifdef SIMPLE
  ReserveFile "simple.ini"
!else
  ReserveFile "iodir.ini"
  ReserveFile "codecvideo.ini"
  ReserveFile "codecaudio.ini"
  ReserveFile "filtersvideo.ini"
  ReserveFile "filtersaudio.ini"
  ReserveFile "filtersaudio.ini"
!endif
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

;--------------------------------
;Installer Sections
!ifdef MSVCRT8
Var ALREADY_INSTALLED
Var PRIVATE_ASSEMBLY
Var WINDOWS_VERSION
!endif
!ifdef PACK_VCREDIST_X86
Var INSTALLED_VC80CRT_VERSION
!endif

!ifdef ADDONS
 Section $(TITLE_addons) Sec_addons
!else
 Section $(TITLE_ffdshow) Sec_ffdshow
!endif
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
;  UnRegDll $INSTDIR\ffdshow.ax
!ifndef ADDONS

  !ifdef MSVCRT8
    call GetWindowsVersion
    pop $WINDOWS_VERSION
    push $0
    StrCpy $0 $WINDOWS_VERSION 2 0
    ${If} $0 == "NT"
      StrCpy $0 $WINDOWS_VERSION 1 3
      ${If} $0 != "4"
        Abort "This build does not support Windows NT 3.x or older."
      ${EndIf}
    ${ElseIf} $0 == "95"
      Abort "This build does not support Windows95."
    ${EndIf}
    pop $0
  !endif

; Even ffdshow.ax by MSVC8 depends on msvcr71.dll to register itself
; without .NET frame work 2.0.
; So msvcr71.dll is included as default.
; Pure GCC build may not depend though, it may depend on it(not sure).
; Anyway msvcr71.dll is not a large file, it is safe to include.

; Usually we don't have to delete $WINDIR\system32\msvcr??.dll
; on uninstall (Should never be deleted).
; This installer always increment the reference count.
; This is not a bug.
; Just no need to implement conditional increment control.

  !insertmacro InstallLib DLL "" REBOOT_PROTECTED InnoSetup\msvcr71.dll $SYSDIR\msvcr71.dll $SYSDIR

  File "..\ffdshow.ax"

  !ifdef AX_BY_VC8
    ; ffdshow.ax.manifest seems to be old for MSVC8.
    ; ffdshow.ax has internal manifest and it is better.
    ; So, let's delete external manifest.
    Delete $INSTDIR\ffdshow.ax.manifest
  !else
    File "ffdshow.ax.manifest"
  !EndIf


  !ifdef MSVCRT8
  ; First check if msvcr80.dll is installed as shared assembly.
  ; It is always better to install vcredist_x86.exe.
  ; If vcredist is not installed, let's try to install private assembly.
  ; Private assembly should work fine too.
    !ifdef PACK_VCREDIST_X86
      ; vcredist_x86.exe can not install on Windows NT 4.0. private assembly.
      ${If} $WINDOWS_VERSION != "NT 4.0"
        IfFileExists $WINDIR\winsxs\Policies\x86_policy.8.0.Microsoft.VC80.CRT_1fc8b3b9a1e18e3b_x-ww_77c24773 0 msvcr80does_not_exist
          push $0
          push $1
          push $2
          push $3
          push $4
          push $5
          ; We need to know the version of VC80.CRT
          ; The the name of *.policy is the version number.
          FindFirst $0 $1 $WINDIR\winsxs\Policies\x86_policy.8.0.Microsoft.VC80.CRT_1fc8b3b9a1e18e3b_x-ww_77c24773\*.policy
          ${While} $1 != ""
            StrCpy $2 $1 1 0
            StrCpy $3 $1 1 2
            StrCpy $4 $1 5 4
            StrCpy $5 $1 2 10
            StrCpy $1 $2$3$4$5
            IntCmp $1 $INSTALLED_VC80CRT_VERSION 0 olderpolicy 0
            StrCpy $INSTALLED_VC80CRT_VERSION $1
           olderpolicy:
            FindNext $0 $1
          ${EndWhile}
          FindClose $0
          pop $5
          pop $4
          pop $3
          pop $2
          pop $1
          pop $0
          IntCmp $INSTALLED_VC80CRT_VERSION ${PACKED_VCREDIST_VERSION} msvcr80exists 0 msvcr80exists

        msvcr80does_not_exist:
          File "InnoSetup\vcredist_x86.exe"
          ExecWait "$INSTDIR\vcredist_x86.exe /Q"
          Delete "$INSTDIR\vcredist_x86.exe"
          IfFileExists $WINDIR\winsxs\Policies\x86_policy.8.0.Microsoft.VC80.CRT_1fc8b3b9a1e18e3b_x-ww_77c24773 msvcr80exists
      ${EndIf} # $WINDOWS_VERSION != "NT 4.0"

    !else  # !ifdef PACK_VCREDIST_X86
      IfFileExists $WINDIR\winsxs\Policies\x86_policy.8.0.Microsoft.VC80.CRT_1fc8b3b9a1e18e3b_x-ww_77c24773 msvcr80exists
    !endif # !ifdef PACK_VCREDIST_X86

    StrCpy $PRIVATE_ASSEMBLY '1'
    ${If} $WINDOWS_VERSION != "NT 4.0"
      File "msvcr80.dll"
      !insertmacro InstallLib DLL "" REBOOT_PROTECTED "msvcr80.dll" $SYSDIR\msvcr80.dll $SYSDIR
    ${Else}
      File "NT4.0\msvcr80.dll"
      IfFileExists $SYSDIR\msvcr80.dll.beforeffdshow msvcr80backExists
      Rename $SYSDIR\msvcr80.dll $SYSDIR\msvcr80.dll.beforeffdshow
     msvcr80backExists:
      !insertmacro InstallLib DLL "" REBOOT_PROTECTED "NT4.0\msvcr80.dll" $SYSDIR\msvcr80.dll $SYSDIR
    ${EndIf}
    File "microsoft.vc80.crt.manifest"
    SetOutPath $SYSDIR
    IfFileExists $SYSDIR\microsoft.vc80.crt.manifest msvcr80sysExists2
    File "microsoft.vc80.crt.manifest"
   msvcr80sysExists2:
    SetOutPath $INSTDIR
   msvcr80exists:
  !endif # !ifdef MSVCRT8

  !ifndef DEC
    File "..\libavcodec.dll"
    File "..\TomsMoComp_ff.dll"
  !else
    File "..\libavcodec_dec.dll"
  !endif
  File "..\libmplayer.dll"
  File "..\libmpeg2_ff.dll"
  File "..\ff_liba52.dll"
  Delete "$INSTDIR\unins000.exe"
  Delete "$INSTDIR\unins001.exe"
  Delete "$INSTDIR\unins000.dat"
  Delete "$INSTDIR\unins001.dat"
!endif
!ifdef AUDX
  File "audxlib.dll"
!endif
!ifndef CORE
  File "..\ff_wmv9.dll"
  File "..\ff_tremor.dll"
  File "..\ff_theora.dll"
  File "..\ff_libmad.dll"
  File "..\ff_libdts.dll"
  File "..\ff_libfaad2.dll"
  File "..\ff_realaac.dll"
  File "..\ff_samplerate.dll"
  File "..\ff_unrar.dll"
  File "..\ff_x264.dll"
  File "..\ff_kernelDeint.dll"
!endif
!ifndef ADDONS

  ClearErrors
  RegDll $INSTDIR\ffdshow.ax
  IfErrors errorCantReg

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\GNU\ffdshow "pth" "$INSTDIR"
  DeleteRegValue HKLM SOFTWARE\GNU\ffdshow "pthPriority"
  ; Write the uninstall keys for Windows
  WriteRegStr       HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ffdshow" "DisplayName" "ffdshow"
  WriteRegStr       HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ffdshow" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegExpandStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ffdshow" "InstallLocation" "$INSTDIR"
  WriteRegStr       HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ffdshow" "DisplayVersion" "${FFDSHOW_VERSION}"
  WriteRegStr       HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ffdshow" "URLInfoAbout" "http://ffdshow.sourceforge.net/tikiwiki/"
  WriteRegStr       HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ffdshow" "Publisher" "Milan Cutka"
  WriteRegDWORD     HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ffdshow" "NoModify" "1"
  WriteRegDWORD     HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ffdshow" "NoRepair" "1"

  WriteUninstaller "uninstall.exe"
  ; Shortcuts
  SetShellVarContext all
  !ifndef SIMPLE
    !insertmacro MUI_STARTMENU_WRITE_BEGIN ffdshowApplication
  !else
    StrCpy $SHORTCUTS_FOLDER "ffdshow"
  !endif
  CreateDirectory "$SMPROGRAMS\$SHORTCUTS_FOLDER"
  Delete "$SMPROGRAMS\ffdshow\Configuration.lnk"
  Delete "$SMPROGRAMS\ffdshow\Audio filter configuration.lnk"
  SetOutPath $INSTDIR
  CreateShortCut  "$SMPROGRAMS\$SHORTCUTS_FOLDER\Video decoder configuration.lnk" "rundll32.exe" "ffdshow.ax,configure" "regedit.exe" 0
  CreateShortCut  "$SMPROGRAMS\$SHORTCUTS_FOLDER\Audio decoder configuration.lnk" "rundll32.exe" "ffdshow.ax,configureAudio" "regedit.exe" 0
  WriteINIStr     "$SMPROGRAMS\$SHORTCUTS_FOLDER\ffdshow Homepage.url" "InternetShortcut" "URL" "http://ffdshow.sourceforge.net/tikiwiki/"
  WriteINIStr     "$SMPROGRAMS\$SHORTCUTS_FOLDER\ffdshow SourceForge.net Project Page.url" "InternetShortcut" "URL" "http://sourceforge.net/projects/ffdshow"
  Delete "$SMPROGRAMS\$SHORTCUTS_FOLDER\Uninstall ffdshow.lnk"
  CreateShortCut  "$SMPROGRAMS\$SHORTCUTS_FOLDER\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  !ifndef SIMPLE
  !insertmacro MUI_STARTMENU_WRITE_END
  !endif

  SetOutPath $INSTDIR\languages
  Delete "$INSTDIR\languages\ffdshow.cz"
  Delete "$INSTDIR\languages\ffdshow.en"
  Delete "$INSTDIR\languages\ffdshow.es"
  Delete "$INSTDIR\languages\ffdshow.fr"
  Delete "$INSTDIR\languages\ffdshow.hu"
  Delete "$INSTDIR\languages\ffdshow.it"
  Delete "$INSTDIR\languages\ffdshow.pl"
  Delete "$INSTDIR\languages\ffdshow.sc"
  Delete "$INSTDIR\languages\ffdshow.se"
  Delete "$INSTDIR\languages\ffdshow.sk"
  Delete "$INSTDIR\languages\ffdshow.br"
  Delete "$INSTDIR\languages\ffdshow.de"
  Delete "$INSTDIR\languages\ffdshow.ru"
  Delete "$INSTDIR\languages\ffdshow.tc"
  Delete "$INSTDIR\languages\ffdshow.jp"
  Delete "$INSTDIR\languages\ffdshow.ja"

  File "..\languages\ffdshow.1026.bg"
  File "..\languages\ffdshow.1028.tc"
  File "..\languages\ffdshow.1029.cz"
  File "..\languages\ffdshow.1031.de"
  File "..\languages\ffdshow.1033.en"
  File "..\languages\ffdshow.1034.es"
  File "..\languages\ffdshow.1036.fr"
  File "..\languages\ffdshow.1038.hu"
  File "..\languages\ffdshow.1040.it"
  File "..\languages\ffdshow.1041.ja"
  File "..\languages\ffdshow.1041.jp"
  File "..\languages\ffdshow.1045.pl"
  File "..\languages\ffdshow.1046.br"
  File "..\languages\ffdshow.1049.ru"
  File "..\languages\ffdshow.1051.sk"
  File "..\languages\ffdshow.1053.se"
  File "..\languages\ffdshow.2052.sc"
  SetOutPath "$INSTDIR\custom matrices"
  Delete "$INSTDIR\custom matrices\andreas_78er.matrix.txt"
  Delete "$INSTDIR\custom matrices\andreas_doppelte_99er.matrix.txt"
  Delete "$INSTDIR\custom matrices\andreas_einfache_99er.matrix.txt"
  Delete "$INSTDIR\custom matrices\Bulletproof's Heavy Compression Matrix.TXT"
  Delete "$INSTDIR\custom matrices\Bulletproof's High Quality Matrix.TXT"
  Delete "$INSTDIR\custom matrices\CG-Animation Matrix.txt"
  Delete "$INSTDIR\custom matrices\hvs-best-picture.txt"
  Delete "$INSTDIR\custom matrices\hvs-better-picture.txt"
  Delete "$INSTDIR\custom matrices\hvs-good-picture.txt"
  Delete "$INSTDIR\custom matrices\Low Bitrate Matrix.txt"
  Delete "$INSTDIR\custom matrices\MPEG.txt"
  Delete "$INSTDIR\custom matrices\pvcd.txt"
  Delete "$INSTDIR\custom matrices\Standard.txt"
  Delete "$INSTDIR\custom matrices\Ultimate Matrix.txt"
  Delete "$INSTDIR\custom matrices\Ultra Low Bitrate Matrix.txt"
  Delete "$INSTDIR\custom matrices\Very Low Bitrate Matrix.txt"
  ClearErrors
  File "..\custom matrices\andreas_78er.matrix.xcm"
  File "..\custom matrices\andreas_doppelte_99er.matrix.xcm"
  File "..\custom matrices\andreas_einfache_99er.matrix.xcm"
  File "..\custom matrices\Bulletproof's Heavy Compression Matrix.xcm"
  File "..\custom matrices\Bulletproof's High Quality Matrix.xcm"
  File "..\custom matrices\CG-Animation Matrix.xcm"
  File "..\custom matrices\hvs-best-picture.xcm"
  File "..\custom matrices\hvs-better-picture.xcm"
  File "..\custom matrices\hvs-good-picture.xcm"
  File "..\custom matrices\Low Bitrate Matrix.xcm"
  File "..\custom matrices\MPEG.xcm"
  File "..\custom matrices\pvcd.xcm"
  File "..\custom matrices\Standard.xcm"
  File "..\custom matrices\Ultimate Matrix.xcm"
  File "..\custom matrices\Ultra Low Bitrate Matrix.xcm"
  File "..\custom matrices\Very Low Bitrate Matrix.xcm"
  File "..\custom matrices\Soulhunters V3.xcm"
  File "..\custom matrices\Soulhunters V5.xcm"
  SetOutPath "$INSTDIR\dict"
  File "..\dict\Czech.dic"
  File "..\dict\dicts.txt"
  File "..\dict\Greek.dic"
  File "..\dict\Polski.dic"
  ;Write language to the registry (for the uninstaller)
  goto ffdshow_end
errorCantReg:
  MessageBox MB_OK|MB_ICONEXCLAMATION $(ERROR_regdll)
  Abort
!endif
ffdshow_end:
SectionEnd

!ifndef ADDONS
Section /o $(TITLE_doc) Sec_doc
  SectionIn 1
  SetOutPath $INSTDIR
  File "..\..\copying.txt"
  File "..\..\readme.txt"
  File "..\..\readmeAudio.txt"
  File "..\..\readmeEnc.txt"
  File "..\..\readmeMultiCPU.txt"
  SetOutPath $INSTDIR\help

  File "..\help\About+audio+decompressor.html"
  File "..\help\About+ffdshow.html"
  File "..\help\About+video+compressor.html"
  File "..\help\About+video+decompressor.html"
  File "..\help\ac3filter.html"
  File "..\help\avis.html"
  File "..\help\AviSynth.html"
  File "..\help\Credits.html"
  File "..\help\DirectShow.html"
  File "..\help\Encoding+library.html"
  File "..\help\ffavisynth.html"
  File "..\help\ffdshow+audio+decoder.html"
  File "..\help\ffdshow+video+decoder.html"
  File "..\help\ffdshow+video+encoder.html"
  File "..\help\ffmpeg.html"
  File "..\help\ffvdub.html"
  File "..\help\FOURCC.html"
  File "..\help\From+sources.html"
  File "..\help\HomePage.html"
  File "..\help\IDCT.html"
  File "..\help\Installing+ffdshow.html"
  File "..\help\Keyboard+and+remote+control.html"
  File "..\help\Levels.html"
  File "..\help\liba52.html"
  File "..\help\libavcodec.html"
  File "..\help\libdts.html"
  File "..\help\libFAAD2.html"
  File "..\help\libmad.html"
  File "..\help\libmpeg2.html"
  File "..\help\License.html"
  File "..\help\makeAVIS.html"
  File "..\help\Masking.html"
  File "..\help\Miscellaneus+settings.html"
  File "..\help\Motion+estimation.html"
  File "..\help\mpadecfilter.html"
  File "..\help\mplayer.html"
  File "..\help\Noise.html"
  File "..\help\Prepare+baseclasses+library.html"
  File "..\help\Quantization.html"
  File "..\help\Subtitles.html"
  File "..\help\tremor.html"
  File "..\help\unrar.dll.html"
  File "..\help\VFW.html"
  File "..\help\Video+decoder+features.html"
  File "..\help\Video+encoder+features.html"
  File "..\help\VirtualDub.html"
  File "..\help\Warpsharp.html"
  File "..\help\x264.html"
  File "..\help\XviD.html"
  SetOutPath $INSTDIR\help\styles
  File "..\help\styles\geo-light.css"
SectionEnd
!endif

#======================================== CODECS MACROS ========================================
Var ALLUSERS

!macro codecsection REG FOURCC DESC
  Var DECODER_${FOURCC}
  Var DECODER_${FOURCC}_DEFDEC
  !define DECODER_${FOURCC}_DESC "${DESC}"
  Section "-${desc}"
    IntCmp $ALLUSERS 0 ${FOURCC}codecsetHKCU
    WriteRegDWORD HKLM SOFTWARE\GNU\${REG} "${FOURCC}" $DECODER_${FOURCC}
    Goto ${FOURCC}codecsetEnd
   ${FOURCC}codecsetHKCU:
    WriteRegDWORD HKCU SOFTWARE\GNU\${REG} "${FOURCC}" $DECODER_${FOURCC}
   ${FOURCC}codecsetEnd:
  SectionEnd
!macroend

!macro testcodec REG FOURCC DEFDEC DEFON
  StrCpy $1 0
  ReadRegDWORD $0 HKCU SOFTWARE\GNU\${REG} "${FOURCC}"
  IfErrors ${FOURCC}defaultHKLM
 ${FOURCC}reg:
  StrCpy $2 $0
  IntCmp $2 0 ${FOURCC}def
  StrCpy $DECODER_${FOURCC}_DEFDEC $2
  Goto ${FOURCC}errend
 ${FOURCC}defaultHKLM:
  ReadRegDWORD $0 HKLM SOFTWARE\GNU\${REG} "${FOURCC}"
  IfErrors ${FOURCC}default
  Goto ${FOURCC}reg
 ${FOURCC}default:
  StrCpy $1 ${DEFON}
  StrCpy $2 ${DEFDEC}
 ${FOURCC}def:
  StrCpy $DECODER_${FOURCC}_DEFDEC ${DEFDEC}
 ${FOURCC}errend:

  IntOp $0 $0 || $1
  IntCmp $0 0 ${FOURCC}Equal ${FOURCC}Less
  StrCpy $DECODER_${FOURCC} $2
  Goto ${FOURCC}End
 ${FOURCC}Equal:
 ${FOURCC}Less:
  StrCpy $DECODER_${FOURCC} 0
 ${FOURCC}End:
!macroend

!macro codecpageset INIFILE FOURCC ENABLED
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field $R2" "Text" "${DECODER_${FOURCC}_desc}"
  IntCmp ${ENABLED} 1 codecpageset_${FOURCC}_show
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field $R2" "Flags" DISABLED
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field $R2" "Text" ""
codecpageset_${FOURCC}_show:
  IntCmp $DECODER_${FOURCC} 0 codecpageset_${FOURCC}_off
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field $R2" "State" 1
  goto codecpageset_${FOURCC}_on
codecpageset_${FOURCC}_off:
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field $R2" "State" 0
codecpageset_${FOURCC}_on:
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field $R2" "Left" $R3
  IntOp $R4 $R3 + 150
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field $R2" "Right" $R4
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field $R2" "Top" $R1
  IntOp $R1 $R1 + 11
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field $R2" "Bottom" $R1
  IntOp $R1 $R1 + 1
  IntCmp $R1 120 codecpageset_${FOURCC}_equ codecpageset_${FOURCC}_equ
  StrCpy $R3 185
  StrCpy $R1 2
codecpageset_${FOURCC}_equ:
  IntOp $R2 $R2 + 1
!macroend

!macro codecpageget INIFILE FOURCC
  !insertmacro MUI_INSTALLOPTIONS_READ $R0 ${INIFILE} "Field $R2" "State"
  IntCmp $R0 0 codecpageget_${FOURCC}_off
  StrCpy $DECODER_${FOURCC} $DECODER_${FOURCC}_DEFDEC
  Goto codecpageget_${FOURCC}_end
codecpageget_${FOURCC}_off:
  StrCpy $DECODER_${FOURCC} 0
codecpageget_${FOURCC}_end:
  IntOp $R2 $R2 + 1
!macroend
;======================================== CODECS MACROS ========================================
!ifndef ADDONS
  !insertmacro codecsection ffdshow xvid "XVID"
  !insertmacro codecsection ffdshow div3 "DivX 3"
  !insertmacro codecsection ffdshow divx "Generic MPEG4"
  !insertmacro codecsection ffdshow dx50 "DivX 5"
  !insertmacro codecsection ffdshow mp43 "MS MPEG4v3"
  !insertmacro codecsection ffdshow mp42 "MS MPEG4v2"
  !insertmacro codecsection ffdshow mp41 "MS MPEG4v1"
  !insertmacro codecsection ffdshow wmv1 "Windows Media Video 1/7"
  !insertmacro codecsection ffdshow wmv2 "Windows Media Video 2/8"
  !insertmacro codecsection ffdshow wmv3 "Windows Media Video 3/9"
  !insertmacro codecsection ffdshow _3iv "3IV2, 3IVX, RMP4, DM4V"
  !insertmacro codecsection ffdshow mpg1 "MPEG 1"
  !insertmacro codecsection ffdshow mpg2 "MPEG 2"
  !insertmacro codecsection ffdshow h263 "H.263(+)"
  !insertmacro codecsection ffdshow h264 "H.264, AVC"
  !insertmacro codecsection ffdshow mjpg "MJPEG"
  !insertmacro codecsection ffdshow dv   "DV"
  !insertmacro codecsection ffdshow hfyu "HuffYUV"
  !insertmacro codecsection ffdshow png1 "CorePNG, MPNG"
  !insertmacro codecsection ffdshow rawv "Uncompressed"
!endif

  !insertmacro codecsection ffdshow_audio mp2    "MP2"
  !insertmacro codecsection ffdshow_audio mp3    "MP3"
  !insertmacro codecsection ffdshow_audio ac3    "AC3"
  !insertmacro codecsection ffdshow_audio vorbis "Vorbis"
  !insertmacro codecsection ffdshow_audio wma1   "Windows Media Audio V1"
  !insertmacro codecsection ffdshow_audio wma2   "Windows Media Audio V2"
!ifndef CORE
  !insertmacro codecsection ffdshow_audio aac "AAC"
  !insertmacro codecsection ffdshow_audio dts "DTS"
!endif
  !insertmacro codecsection ffdshow_audio amr  "AMR"
  !insertmacro codecsection ffdshow_audio flac "Flac"
  !insertmacro codecsection ffdshow_audio rawa "Uncompressed"

;======================================== FILTERS MACROS ========================================
!macro filtersection REG FILTER REGNAME
  Var FILTER_${FILTER}
  Section "-${FILTER}"
    WriteRegDWORD HKCU SOFTWARE\GNU\${REG}\default ${REGNAME} $FILTER_${FILTER}
  SectionEnd
!macroend

!macro testfilter REG FILTER DEFON REGNAME
  ReadRegDWORD $0 HKCU SOFTWARE\GNU\${REG}\default ${REGNAME}
  IfErrors ${FILTER}default
  StrCpy $FILTER_${FILTER} $0
  goto ${FILTER}reg
 ${FILTER}default:
  StrCpy $FILTER_${FILTER} ${DEFON}
 ${FILTER}reg:
!macroend

!macro filterspageset INIFILE FILTER
  IntCmp $FILTER_${FILTER} 0 filterspageset_${FILTER}_off
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field $R2" "State" 1
  goto filterspageset_${FILTER}_on
filterspageset_${FILTER}_off:
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field $R2" "State" 0
filterspageset_${FILTER}_on:
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field $R2" "Text" "$(FILTER_${FILTER}_name)"
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field $R2" "Left" $R3
  IntOp $R4 $R3 + 150
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field $R2" "Right" $R4
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field $R2" "Top" $R1
  IntOp $R1 $R1 + $R5
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field $R2" "Bottom" $R1
  IntOp $R1 $R1 + 1
  IntCmp $R1 120 filterspageset_${FILTER}_equ filterspageset_${FILTER}_equ
  StrCpy $R3 185
  StrCpy $R1 2
filterspageset_${FILTER}_equ:
  IntOp $R2 $R2 + 1
!macroend

!macro filterspageget INIFILE FILTER
  !insertmacro MUI_INSTALLOPTIONS_READ $R0 ${INIFILE} "Field $R2" "State"
  IntCmp $R0 0 filterspageget_${FILTER}_off
  StrCpy $FILTER_${FILTER} 1
  Goto filterspageget_${FILTER}_end
filterspageget_${FILTER}_off:
  StrCpy $FILTER_${FILTER} 0
filterspageget_${FILTER}_end:
  IntOp $R2 $R2 + 1
!macroend

!ifndef ADDONS
  !insertmacro filtersection ffdshow oqueue    multiThread
  !insertmacro filtersection ffdshow postproc  ispostproc
  !insertmacro filtersection ffdshow noise     isnoise
  !insertmacro filtersection ffdshow sharpen   xsharpen
  !insertmacro filtersection ffdshow subtitles issubtitles

  !insertmacro filtersection ffdshow_audio volume    isvolume
  !insertmacro filtersection ffdshow_audio equalizer  iseq
  !insertmacro filtersection ffdshow_audio mixer     ismixer
  !insertmacro filtersection ffdshow_audio freeverb  isfreeverb
!endif

!ifndef ADDONS
SubSection /e $(TITLE_vfw) Sec_vfw
  Section $(TITLE_vfwVideo) Sec_vfwVideo
    SetOutPath $SYSDIR
    File "..\ff_vfw.dll"
    !ifdef FF_VFW_BY_VC8
     ; ff_vfw.dll.manifest seems to be old for MSVC8.
     ; ff_vfw.dll has internal manifest and it is better.
     ; So, let's delete external manifest.
     Delete $SYSDIR\ff_vfw.dll.manifest
    !else
      File "ff_vfw.dll.manifest"
    !endif
    Call VFWcleanup
    Call VFWRegSetup
    !ifndef SIMPLE
      !insertmacro MUI_STARTMENU_WRITE_BEGIN ffdshowApplication
    !endif
    CreateDirectory "$SMPROGRAMS\$SHORTCUTS_FOLDER"
    Delete "$SMPROGRAMS\$SHORTCUTS_FOLDER\VFW Configuration.lnk"
    CreateShortCut  "$SMPROGRAMS\$SHORTCUTS_FOLDER\VFW codec configuration.lnk" "rundll32.exe" "ff_vfw.dll,configureVFW" "regedit.exe" 0
    !ifndef SIMPLE
      !insertmacro MUI_STARTMENU_WRITE_END
    !endif
  SectionEnd

  Section /o $(TITLE_vfwAVIS) Sec_vfwAVIS
    SetOutPath $INSTDIR
    !ifdef MAKEAVIS_BY_VC8
      ; makeAVIS.exe.manifest seems to be old for MSVC8.
      ; makeAVIS.exe has internal manifest and it is better.
      ; So, let's delete external manifest.
      Delete $INSTDIR\makeAVIS.exe.manifest
    !else
      File "makeAVIS.exe.manifest"
    !endif
      File "..\makeAVIS.exe"
    !ifndef SIMPLE
      !insertmacro MUI_STARTMENU_WRITE_BEGIN ffdshowApplication
    !endif
    CreateDirectory "$SMPROGRAMS\$SHORTCUTS_FOLDER"
    CreateShortCut "$SMPROGRAMS\$SHORTCUTS_FOLDER\makeAVIS.lnk" "$INSTDIR\makeAVIS.exe"
    !ifndef SIMPLE
      !insertmacro MUI_STARTMENU_WRITE_END
    !endif
    SetOutPath $SYSDIR
    File "..\ff_acm.acm"
    Call VFWRegSetupAVIS
  SectionEnd
SubSectionEnd
!endif

Function .onSelChange
  SectionGetFlags ${Sec_vfwAVIS} $0
  SectionGetFlags ${Sec_vfwVideo} $1
  IntOp $0 $0 | $1
  SectionSetFlags ${Sec_vfwVideo} $0
FunctionEnd

Var REG_MSVCR_PLUGIN_INSTFLAG
Function install_msvcr80_as_private_assembly_for_plugins
!ifdef MSVCRT8
  ${If} $OUTDIR != $INSTDIR
    ${If} $PRIVATE_ASSEMBLY == '1'
      ReadRegDWORD $ALREADY_INSTALLED ${MUI_LANGDLL_REGISTRY_ROOT} ${MUI_LANGDLL_REGISTRY_KEY} $REG_MSVCR_PLUGIN_INSTFLAG
      ${If} $WINDOWS_VERSION != "NT 4.0"
        !insertmacro InstallLib DLL $ALREADY_INSTALLED REBOOT_PROTECTED "msvcr80.dll" $OUTDIR\msvcr80.dll $OUTDIR
      ${Else}
        !insertmacro InstallLib DLL $ALREADY_INSTALLED REBOOT_PROTECTED "NT4.0\msvcr80.dll" $OUTDIR\msvcr80.dll $OUTDIR
      ${EndIf}
      File "microsoft.vc80.crt.manifest"
      WriteRegDWORD ${MUI_LANGDLL_REGISTRY_ROOT} ${MUI_LANGDLL_REGISTRY_KEY} $REG_MSVCR_PLUGIN_INSTFLAG 1
    ${EndIf}
  ${EndIf}
!endif
FunctionEnd

!ifndef CORE
  Var AVISYNTHDIR
  Var VIRTUALDUBDIR
  !ifndef X64
    Var DSCALERDIR
  !endif

  SubSection /e $(TITLE_plugins) Sec_plugins
    Section $(TITLE_pluginAvisynth) Sec_pluginAvisynth
      SetOutPath $AVISYNTHDIR
      File "..\ffavisynth.dll"
      WriteRegStr ${MUI_LANGDLL_REGISTRY_ROOT} ${MUI_LANGDLL_REGISTRY_KEY} "pthAvisynth" $AVISYNTHDIR
      StrCpy $REG_MSVCR_PLUGIN_INSTFLAG "AvisynthMsvcr80Inst"
      call install_msvcr80_as_private_assembly_for_plugins
    SectionEnd

    Section $(TITLE_pluginVirtualDub) Sec_pluginVirtualDub
      SetOutPath $VIRTUALDUBDIR
      File "..\ffvdub.vdf"
      WriteRegStr ${MUI_LANGDLL_REGISTRY_ROOT} ${MUI_LANGDLL_REGISTRY_KEY} "pthVirtualDub" $VIRTUALDUBDIR
      StrCpy $REG_MSVCR_PLUGIN_INSTFLAG "VirtualDubMsvcr80Inst"
      call install_msvcr80_as_private_assembly_for_plugins
    SectionEnd

    !ifndef X64
      Section $(TITLE_pluginDScaler) Sec_pluginDScaler
        SetOutPath $DSCALERDIR
        File "..\FLT_ffdshow.dll"
        WriteRegStr ${MUI_LANGDLL_REGISTRY_ROOT} ${MUI_LANGDLL_REGISTRY_KEY} "dscalerPth" $DSCALERDIR
        StrCpy $REG_MSVCR_PLUGIN_INSTFLAG "DScalerMsvcr80Inst"
        call install_msvcr80_as_private_assembly_for_plugins
      SectionEnd
    !endif
  SubSectionEnd
!endif
;--------------------------------
;Descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${Sec_ffdshow} $(DESC_ffdshow)
  !insertmacro MUI_DESCRIPTION_TEXT ${Sec_doc} $(DESC_doc)
;  !insertmacro MUI_DESCRIPTION_TEXT ${Sec_codecsVideo} $(DESC_codecsVideo)
;  !insertmacro MUI_DESCRIPTION_TEXT ${Sec_codecsAudio} $(DESC_codecsAudio)
  !insertmacro MUI_DESCRIPTION_TEXT ${Sec_vfw} $(DESC_vfw)
  !insertmacro MUI_DESCRIPTION_TEXT ${Sec_vfwVideo} $(DESC_vfwVideo)
  !insertmacro MUI_DESCRIPTION_TEXT ${Sec_vfwAVIS} $(DESC_vfwAVIS)
  !insertmacro MUI_DESCRIPTION_TEXT ${Sec_plugins} $(DESC_plugins)
  !insertmacro MUI_DESCRIPTION_TEXT ${Sec_pluginAvisynth} $(DESC_pluginAvisynth)
  !insertmacro MUI_DESCRIPTION_TEXT ${Sec_pluginVirtualDub} $(DESC_pluginVirtualDub)
  !ifndef X64
    !insertmacro MUI_DESCRIPTION_TEXT ${Sec_pluginDScaler} $(DESC_pluginDScaler)
  !endif
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Var AVISYNTH_MSVCR80_UNINST
Var VDUB_MSVCR80_UNINST
Var DSCALER_MSVCR80_UNINST

Section "Uninstall"
  ; remove shortcuts, if any.
  SetShellVarContext all
  !ifndef SIMPLE
    !insertmacro MUI_STARTMENU_GETFOLDER ffdshowApplication $SHORTCUTS_FOLDER
  !else
     StrCpy $SHORTCUTS_FOLDER "ffdshow"
  !endif
  Delete "$SMPROGRAMS\$SHORTCUTS_FOLDER\*.*"
  RMDir  "$SMPROGRAMS\$SHORTCUTS_FOLDER"

  ReadRegStr $AVISYNTH_MSVCR80_UNINST ${MUI_LANGDLL_REGISTRY_ROOT} ${MUI_LANGDLL_REGISTRY_KEY} "AvisynthMsvcr80Inst"
  ReadRegStr $VDUB_MSVCR80_UNINST ${MUI_LANGDLL_REGISTRY_ROOT} ${MUI_LANGDLL_REGISTRY_KEY} "VirtualDubMsvcr80Inst"
  ReadRegStr $DSCALER_MSVCR80_UNINST ${MUI_LANGDLL_REGISTRY_ROOT} ${MUI_LANGDLL_REGISTRY_KEY} "DScalerMsvcr80Inst"

  ; remove registry keys
  Call un.VFWRegSetupAVIS
  Call un.VFWRegSetup
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ffdshow"
  DeleteRegKey HKLM SOFTWARE\GNU\ffdshow
  DeleteRegKey HKCU SOFTWARE\GNU\ffdshow
  DeleteRegKey HKLM SOFTWARE\GNU\ffdshow_audio
  DeleteRegKey HKCU SOFTWARE\GNU\ffdshow_audio
  DeleteRegKey HKLM SOFTWARE\GNU\ffdshow_enc
  DeleteRegKey HKCU SOFTWARE\GNU\ffdshow_enc
  DeleteRegKey HKLM SOFTWARE\GNU\ffdshow_vfw
  DeleteRegKey HKCU SOFTWARE\GNU\ffdshow_vfw
  DeleteRegKey HKCU SOFTWARE\GNU\makeAVIS
  UnRegDll $INSTDIR\ffdshow.ax
  ; remove files
!ifdef AUDX
  Delete "$INSTDIR\audxlib.dll"
!endif
!ifdef MSVCRT8
  Delete "$INSTDIR\Microsoft.VC80.CRT.manifest"
  Delete "$INSTDIR\msvcr80.dll"
!endif
  Delete "$SYSDIR\ff_vfw.dll"
  Delete "$SYSDIR\ff_vfw.dll.manifest"
  Delete "$SYSDIR\ff_acm.acm"
  Delete "$INSTDIR\ffdshow.ax"
  Delete "$INSTDIR\ffdshow.ax.manifest"
  Delete "$INSTDIR\libavcodec.dll"
  Delete "$INSTDIR\libpostproc.dll"
  Delete "$INSTDIR\libmpeg2_ff.dll"
  Delete "$INSTDIR\TomsMoComp_ff.dll"
  Delete "$INSTDIR\ff_kernelDeint.dll"
  Delete "$INSTDIR\libmplayer.dll"
  Delete "$INSTDIR\ff_libmad.dll"
  Delete "$INSTDIR\ff_mpeg2enc.dll"
  Delete "$INSTDIR\ff_theora.dll"
  Delete "$INSTDIR\ff_wmv9.dll"
  Delete "$INSTDIR\ff_x264.dll"
  Delete "$INSTDIR\ff_liba52.dll"
  Delete "$INSTDIR\ff_libdts.dll"
  Delete "$INSTDIR\ff_libfaad2.dll"
  Delete "$INSTDIR\ff_realaac.dll"
  Delete "$INSTDIR\ff_samplerate.dll"
  Delete "$INSTDIR\ff_tremor.dll"
  Delete "$INSTDIR\ff_unrar.dll"
  Delete "$INSTDIR\makeAVIS.exe"
  Delete "$INSTDIR\makeAVIS.exe.manifest"
  Delete "$INSTDIR\copying.txt"
  Delete "$INSTDIR\readme.txt"
  Delete "$INSTDIR\readmeAudio.txt"
  Delete "$INSTDIR\readmeEnc.txt"
  Delete "$INSTDIR\readmeMultiCPU.txt"
  Delete "$INSTDIR\languages\ffdshow.br"
  Delete "$INSTDIR\languages\ffdshow.cz"
  Delete "$INSTDIR\languages\ffdshow.de"
  Delete "$INSTDIR\languages\ffdshow.en"
  Delete "$INSTDIR\languages\ffdshow.es"
  Delete "$INSTDIR\languages\ffdshow.fr"
  Delete "$INSTDIR\languages\ffdshow.hu"
  Delete "$INSTDIR\languages\ffdshow.it"
  Delete "$INSTDIR\languages\ffdshow.pl"
  Delete "$INSTDIR\languages\ffdshow.ru"
  Delete "$INSTDIR\languages\ffdshow.sc"
  Delete "$INSTDIR\languages\ffdshow.se"
  Delete "$INSTDIR\languages\ffdshow.sk"
  Delete "$INSTDIR\languages\ffdshow.tc"
  Delete "$INSTDIR\languages\ffdshow.jp"
  Delete "$INSTDIR\languages\ffdshow.ja"
  Delete "$INSTDIR\languages\ffdshow.1026.bg"
  Delete "$INSTDIR\languages\ffdshow.1028.tc"
  Delete "$INSTDIR\languages\ffdshow.1029.cz"
  Delete "$INSTDIR\languages\ffdshow.1031.de"
  Delete "$INSTDIR\languages\ffdshow.1033.en"
  Delete "$INSTDIR\languages\ffdshow.1034.es"
  Delete "$INSTDIR\languages\ffdshow.1036.fr"
  Delete "$INSTDIR\languages\ffdshow.1038.hu"
  Delete "$INSTDIR\languages\ffdshow.1040.it"
  Delete "$INSTDIR\languages\ffdshow.1041.ja"
  Delete "$INSTDIR\languages\ffdshow.1041.jp"
  Delete "$INSTDIR\languages\ffdshow.1045.pl"
  Delete "$INSTDIR\languages\ffdshow.1046.br"
  Delete "$INSTDIR\languages\ffdshow.1049.ru"
  Delete "$INSTDIR\languages\ffdshow.1051.sk"
  Delete "$INSTDIR\languages\ffdshow.1053.se"
  Delete "$INSTDIR\languages\ffdshow.2052.sc"
  RMDir  "$INSTDIR\languages"
  Delete "$INSTDIR\custom matrices\andreas_78er.matrix.xcm"
  Delete "$INSTDIR\custom matrices\andreas_doppelte_99er.matrix.xcm"
  Delete "$INSTDIR\custom matrices\andreas_einfache_99er.matrix.xcm"
  Delete "$INSTDIR\custom matrices\Bulletproof's Heavy Compression Matrix.xcm"
  Delete "$INSTDIR\custom matrices\Bulletproof's High Quality Matrix.xcm"
  Delete "$INSTDIR\custom matrices\CG-Animation Matrix.xcm"
  Delete "$INSTDIR\custom matrices\hvs-best-picture.xcm"
  Delete "$INSTDIR\custom matrices\hvs-better-picture.xcm"
  Delete "$INSTDIR\custom matrices\hvs-good-picture.xcm"
  Delete "$INSTDIR\custom matrices\Low Bitrate Matrix.xcm"
  Delete "$INSTDIR\custom matrices\MPEG.xcm"
  Delete "$INSTDIR\custom matrices\pvcd.xcm"
  Delete "$INSTDIR\custom matrices\Standard.xcm"
  Delete "$INSTDIR\custom matrices\Ultimate Matrix.xcm"
  Delete "$INSTDIR\custom matrices\Ultra Low Bitrate Matrix.xcm"
  Delete "$INSTDIR\custom matrices\Very Low Bitrate Matrix.xcm"
  Delete "$INSTDIR\custom matrices\Soulhunters V3.xcm"
  Delete "$INSTDIR\custom matrices\Soulhunters V5.xcm"
  RMDir  "$INSTDIR\custom matrices"
  Delete "$INSTDIR\help\styles\geo-light.css"
  RMDir  "$INSTDIR\help\styles"
  Delete "$INSTDIR\help\About+audio+decompressor.html"
  Delete "$INSTDIR\help\About+ffdshow.html"
  Delete "$INSTDIR\help\About+video+compressor.html"
  Delete "$INSTDIR\help\About+video+decompressor.html"
  Delete "$INSTDIR\help\ac3filter.html"
  Delete "$INSTDIR\help\avis.html"
  Delete "$INSTDIR\help\AviSynth.html"
  Delete "$INSTDIR\help\Credits.html"
  Delete "$INSTDIR\help\DirectShow.html"
  Delete "$INSTDIR\help\Encoding+library.html"
  Delete "$INSTDIR\help\ffavisynth.html"
  Delete "$INSTDIR\help\ffdshow+audio+decoder.html"
  Delete "$INSTDIR\help\ffdshow+video+decoder.html"
  Delete "$INSTDIR\help\ffdshow+video+encoder.html"
  Delete "$INSTDIR\help\ffmpeg.html"
  Delete "$INSTDIR\help\ffvdub.html"
  Delete "$INSTDIR\help\FOURCC.html"
  Delete "$INSTDIR\help\From+sources.html"
  Delete "$INSTDIR\help\HomePage.html"
  Delete "$INSTDIR\help\IDCT.html"
  Delete "$INSTDIR\help\Installing+ffdshow.html"
  Delete "$INSTDIR\help\Keyboard+and+remote+control.html"
  Delete "$INSTDIR\help\Levels.html"
  Delete "$INSTDIR\help\liba52.html"
  Delete "$INSTDIR\help\libavcodec.html"
  Delete "$INSTDIR\help\libdts.html"
  Delete "$INSTDIR\help\libFAAD2.html"
  Delete "$INSTDIR\help\libmad.html"
  Delete "$INSTDIR\help\libmpeg2.html"
  Delete "$INSTDIR\help\License.html"
  Delete "$INSTDIR\help\makeAVIS.html"
  Delete "$INSTDIR\help\Masking.html"
  Delete "$INSTDIR\help\Miscellaneus+settings.html"
  Delete "$INSTDIR\help\Motion+estimation.html"
  Delete "$INSTDIR\help\mpadecfilter.html"
  Delete "$INSTDIR\help\mplayer.html"
  Delete "$INSTDIR\help\Noise.html"
  Delete "$INSTDIR\help\Prepare+baseclasses+library.html"
  Delete "$INSTDIR\help\Quantization.html"
  Delete "$INSTDIR\help\Subtitles.html"
  Delete "$INSTDIR\help\tremor.html"
  Delete "$INSTDIR\help\unrar.dll.html"
  Delete "$INSTDIR\help\VFW.html"
  Delete "$INSTDIR\help\Video+decoder+features.html"
  Delete "$INSTDIR\help\Video+encoder+features.html"
  Delete "$INSTDIR\help\VirtualDub.html"
  Delete "$INSTDIR\help\Warpsharp.html"
  Delete "$INSTDIR\help\x264.html"
  Delete "$INSTDIR\help\XviD.html"
  RMDir  "$INSTDIR\help"
  Delete "$INSTDIR\dict\Czech.dic"
  Delete "$INSTDIR\dict\dicts.txt"
  Delete "$INSTDIR\dict\Greek.dic"
  Delete "$INSTDIR\dict\Polski.dic"
  RMDir  "$INSTDIR\dict"
  Delete "$INSTDIR\ffavisynth.dll"
  Delete "$INSTDIR\ffvdub.vdf"
  Delete "$INSTDIR\FLT_ffdshow.dll"
  StrCmp $AVISYNTHDIR "" noDeleteAvisynth
  Delete "$AVISYNTHDIR\ffavisynth.dll"
    ${If} $AVISYNTHDIR != $INSTDIR
      ${If} $AVISYNTH_MSVCR80_UNINST == '1'
        !insertmacro UnInstallLib DLL SHARED REBOOT_PROTECTED $AVISYNTHDIR\msvcr80.dll
        IfFileExists $AVISYNTHDIR\msvcr80.dll nodeleteVC80manifestA
        Delete $AVISYNTHDIR\Microsoft.VC80.CRT.manifest
      nodeleteVC80manifestA:
      ${EndIf}
    ${EndIf}
 noDeleteAvisynth:
  StrCmp $VIRTUALDUBDIR "" noDeleteVirtualDub
  Delete "$VIRTUALDUBDIR\ffvdub.vdf"
    ${If} $VIRTUALDUBDIR != $INSTDIR
      ${If} $VDUB_MSVCR80_UNINST == '1'
        !insertmacro UnInstallLib DLL SHARED REBOOT_PROTECTED $VIRTUALDUBDIR\msvcr80.dll
        IfFileExists $VIRTUALDUBDIR\msvcr80.dll nodeleteVC80manifestV
        Delete $VIRTUALDUBDIR\Microsoft.VC80.CRT.manifest
      nodeleteVC80manifestV:
      ${EndIf}
    ${EndIf}
 noDeleteVirtualDub:
  !ifndef X64
    StrCmp $DSCALERDIR "" noDeleteDScaler
    Delete "$DSCALERDIR\FLT_ffdshow.dll"
      ${If} $DSCALERDIR != $INSTDIR
        ${If} $DSCALER_MSVCR80_UNINST == '1'
          !insertmacro UnInstallLib DLL SHARED REBOOT_PROTECTED $DSCALERDIR\msvcr80.dll
          IfFileExists $DSCALERDIR\msvcr80.dll nodeleteVC80manifestD
          Delete $DSCALERDIR\Microsoft.VC80.CRT.manifest
        nodeleteVC80manifestD:
        ${EndIf}
      ${EndIf}
   noDeleteDScaler:
  !endif
  ; MUST REMOVE UNINSTALLER, too
  Delete "$INSTDIR\uninstall.exe"
  RMDir  "$INSTDIR"
;  !insertmacro MUI_UNFINISHHEADER
SectionEnd

!ifdef ADDONS
Var ADD_INSTDIR
!endif

!ifndef X64
  !ifndef CORE
    Function GetDScalerDir
      StrCpy $R4 0
      StrCpy $0 -1
      StrCpy $3 "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"
      loopDScalerReg:
        IntOp $0 $0 + 1
        EnumRegKey $1 HKLM $3 $0
        StrCmp $1 "" doneDScalerReg
        ${StrStr} $2 $1 "DScaler"
        StrCmp $2 "" loopDscalerReg
        StrCpy $3 "$3\$2"
        ReadRegStr $DSCALERDIR "HKLM" $3 "Inno Setup: App Path"
        StrCpy $R4 1
      doneDScalerReg:
    FunctionEnd
  !endif
!endif

Function .onInit
  StrCpy $ALLUSERS 0

!ifdef ADDONS
  ReadRegStr $ADD_INSTDIR "HKLM" "SOFTWARE\GNU\ffdshow" "pth"
  StrCmp $ADD_INSTDIR "" add_nodir add_dir
  add_nodir:
   MessageBox MB_ICONSTOP  "ffdshow not installed."
    Abort
  add_dir:
!endif

 !ifdef SIMPLE
   !insertmacro MUI_INSTALLOPTIONS_EXTRACT "simple.ini"
 !else
   !insertmacro MUI_INSTALLOPTIONS_EXTRACT "codecvideo.ini"
   !insertmacro MUI_INSTALLOPTIONS_EXTRACT "codecaudio.ini"
   !insertmacro MUI_INSTALLOPTIONS_EXTRACT "filtersvideo.ini"
   !insertmacro MUI_INSTALLOPTIONS_EXTRACT "filtersaudio.ini"
 !endif

!ifndef CORE
  ReadRegStr $AVISYNTHDIR "HKLM" "SOFTWARE\Avisynth" "plugindir2_5"
  StrCmp $AVISYNTHDIR "" avis_nodir avis_dir
  avis_nodir:
   StrCpy $AVISYNTHDIR $INSTDIR
   SectionSetFlags ${Sec_pluginAvisynth} 0
  avis_dir:
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "iodir.ini"


  ClearErrors
  ReadRegStr $VIRTUALDUBDIR ${MUI_LANGDLL_REGISTRY_ROOT} ${MUI_LANGDLL_REGISTRY_KEY} "pthVirtualDub"
  SectionSetFlags ${Sec_pluginVirtualDub} 1
  IfErrors errorPthVirtualDub noErrorPthVirtualDub
 errorPthVirtualDub:
  StrCpy $VIRTUALDUBDIR $INSTDIR
  SectionSetFlags ${Sec_pluginVirtualDub} 0
 noErrorPthVirtualDub:
  ClearErrors

  !ifndef X64
    StrCpy $DSCALERDIR $INSTDIR
    SectionSetFlags ${Sec_pluginDScaler} 0
    call GetDScalerDir
    IntCmp $R4 0 dscaler_nodir
    SectionSetFlags ${Sec_pluginDScaler} 1
    dscaler_nodir:
  !endif
!endif

  ;Language selection
  !ifndef SIMPLE
    !insertmacro MUI_LANGDLL_DISPLAY
  !endif

  SectionSetFlags ${Sec_ffdshow} 17

  !ifdef SIMPLE
    StrCpy $R0 0
  !else
    StrCpy $R0 1
  !endif

  !insertmacro testcodec ffdshow XVID ${IDFF_MOVIE_LAVC} 1
  !insertmacro testcodec ffdshow DIV3 ${IDFF_MOVIE_LAVC} 1
  !insertmacro testcodec ffdshow DIVX ${IDFF_MOVIE_LAVC} 1
  !insertmacro testcodec ffdshow DX50 ${IDFF_MOVIE_LAVC} 1
  !insertmacro testcodec ffdshow MP43 ${IDFF_MOVIE_LAVC} 1
  !insertmacro testcodec ffdshow MP42 ${IDFF_MOVIE_LAVC} 1
  !insertmacro testcodec ffdshow MP41 ${IDFF_MOVIE_LAVC} 1
  !insertmacro testcodec ffdshow WMV1 ${IDFF_MOVIE_LAVC} $R0
  !insertmacro testcodec ffdshow wmv2 ${IDFF_MOVIE_LAVC} 0
  !insertmacro testcodec ffdshow wmv3 ${IDFF_MOVIE_LAVC} 0
  !insertmacro testcodec ffdshow _3iv ${IDFF_MOVIE_LAVC} $R0
  !insertmacro testcodec ffdshow mpg1 ${IDFF_MOVIE_LIBMPEG2} 0
  !insertmacro testcodec ffdshow mpg2 ${IDFF_MOVIE_LIBMPEG2} 0
  !insertmacro testcodec ffdshow h263 ${IDFF_MOVIE_LAVC} $R0
  !insertmacro testcodec ffdshow h264 ${IDFF_MOVIE_LAVC} 1
  !insertmacro testcodec ffdshow mjpg ${IDFF_MOVIE_LAVC} $R0
  !insertmacro testcodec ffdshow dv   ${IDFF_MOVIE_LAVC} $R0
  !insertmacro testcodec ffdshow hfyu ${IDFF_MOVIE_LAVC} $R0
  !insertmacro testcodec ffdshow png1 ${IDFF_MOVIE_LAVC} $R0
  !insertmacro testcodec ffdshow rawv 1                  0

  !insertmacro testcodec ffdshow_audio MP2     ${IDFF_MOVIE_MPLAYER} 1
  !insertmacro testcodec ffdshow_audio MP3     ${IDFF_MOVIE_MPLAYER} 1
  !insertmacro testcodec ffdshow_audio AC3     ${IDFF_MOVIE_LIBA52}  1
  !insertmacro testcodec ffdshow_audio AMR     ${IDFF_MOVIE_LAVC}    $R0
  !insertmacro testcodec ffdshow_audio VORBIS  ${IDFF_MOVIE_LAVC}    0
  !insertmacro testcodec ffdshow_audio FLAC    ${IDFF_MOVIE_LAVC}    0
  !insertmacro testcodec ffdshow_audio WMA1    ${IDFF_MOVIE_LAVC}    0
  !insertmacro testcodec ffdshow_audio WMA2    ${IDFF_MOVIE_LAVC}    0
  !ifndef CORE
    !insertmacro testcodec ffdshow_audio AAC ${IDFF_MOVIE_LIBFAAD} 1
    !insertmacro testcodec ffdshow_audio DTS ${IDFF_MOVIE_LIBDTS}  1
  !endif
  !insertmacro testcodec ffdshow_audio rawa    4                     0

  !insertmacro testfilter ffdshow oqueue    1 multiThread
  !insertmacro testfilter ffdshow postproc  0 ispostproc
  !insertmacro testfilter ffdshow noise     0 isnoise
  !insertmacro testfilter ffdshow sharpen   0 xsharpen
  !insertmacro testfilter ffdshow subtitles 0 issubtitles

  !insertmacro testfilter ffdshow_audio volume    0 isvolume
  !insertmacro testfilter ffdshow_audio equalizer 0 iseq
  !insertmacro testfilter ffdshow_audio mixer     0 ismixer
  !insertmacro testfilter ffdshow_audio freeverb  0 isfreeverb
FunctionEnd

Function VFWcleanup
  StrCpy $0 -1
  StrCpy $3 "SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers.desc"
  loopVFWcleanup:
    IntOp $0 $0 + 1
    EnumRegValue $1 HKLM $3 $0
    StrCmp $1 "" doneVFWcleanup
    ${StrStr} $2 $1 "ffdshow.ax"
    StrCmp $2 "" loopVFWcleanup
    DeleteRegValue "HKLM" $3 $1
    IntOp $0 $0 - 1
    Goto loopVFWcleanup
  doneVFWcleanup:
FunctionEnd

Function un.VFWcleanup
  StrCpy $0 -1
  StrCpy $3 "SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers.desc"
  un_loopVFWcleanup:
    IntOp $0 $0 + 1
    EnumRegValue $1 HKLM $3 $0
    StrCmp $1 "" un_doneVFWcleanup
    ${UnStrStr} $2 $1 "ffdshow.ax"
    StrCmp $2 "" un_loopVFWcleanup
    DeleteRegValue "HKLM" $3 $1
    IntOp $0 $0 - 1
    Goto un_loopVFWcleanup
  un_doneVFWcleanup:
FunctionEnd

Function un.onInit
  !insertmacro MUI_UNGETLANGUAGE
  !ifndef CORE
    ReadRegStr $AVISYNTHDIR   ${MUI_LANGDLL_REGISTRY_ROOT} ${MUI_LANGDLL_REGISTRY_KEY} "pthAvisynth"
    ReadRegStr $VIRTUALDUBDIR ${MUI_LANGDLL_REGISTRY_ROOT} ${MUI_LANGDLL_REGISTRY_KEY} "pthVirtualDub"
    !ifndef X64
      ReadRegStr $DSCALERDIR    ${MUI_LANGDLL_REGISTRY_ROOT} ${MUI_LANGDLL_REGISTRY_KEY} "dscalerPth"
    !endif
  !endif
FunctionEnd

Function VFWRegSetup
  Push $0
  Push $9
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  StrCmp $0 "" 0 winnt
  WriteRegStr HKLM SYSTEM\CurrentControlSet\Control\MediaResources\icm\vidc.ffds "Description" "ffdshow Video Codec"
  WriteRegStr HKLM SYSTEM\CurrentControlSet\Control\MediaResources\icm\vidc.ffds "Driver" ff_vfw.dll
  WriteRegStr HKLM SYSTEM\CurrentControlSet\Control\MediaResources\icm\vidc.ffds "FriendlyName" "ffdshow Video Codec"
  WriteINIStr $WINDIR\System.ini drivers32 vidc.ffds ff_vfw.dll
  Goto done
    winnt:
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers.desc" ff_vfw.dll "ffdshow Video Codec"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers32" vidc.ffds ff_vfw.dll
  done:
  Pop $9
  Exch $0
FunctionEnd

Function un.VFWRegSetup
  Push $0
  Push $9
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  StrCmp $0 "" 0 winnt1
  DeleteRegKey HKLM SYSTEM\CurrentControlSet\Control\MediaResources\icm\vidc.ffds
  DeleteINIStr $WINDIR\System.ini drivers32 vidc.ffds
  Goto end
    winnt1:
    DeleteRegValue HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers.desc" ff_vfw.dll
    DeleteRegValue HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers32" vidc.ffds
    Call un.vfwcleanup
  end:
  Pop $9
  Exch $0
FunctionEnd

Function VFWRegSetupAVIS
  Push $0
  Push $9
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  StrCmp $0 "" 0 winnt2
  WriteRegStr HKLM SYSTEM\CurrentControlSet\Control\MediaResources\acm\msacm.avis Description "ffdshow ACM codec"
  WriteRegStr HKLM SYSTEM\CurrentControlSet\Control\MediaResources\acm\msacm.avis Driver ff_acm.acm
  WriteRegStr HKLM SYSTEM\CurrentControlSet\Control\MediaResources\acm\msacm.avis FriendlyName "ffdshow ACM codec"
  WriteINIStr $WINDIR\System.ini drivers32 msacm.avis ff_acm.acm
  Goto done2
    winnt2:
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers32" msacm.avis ff_acm.acm
  done2:
  Pop $9
  Exch $0
FunctionEnd

Function un.VFWRegSetupAVIS
  Push $0
  Push $9
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  StrCmp $0 "" 0 winnt3
  DeleteRegKey HKLM SYSTEM\CurrentControlSet\Control\MediaResources\acm\msacm.avis
  DeleteINIStr $WINDIR\System.ini drivers32 msacm.avis
  Goto end2
    winnt3:
    DeleteRegValue HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers32" msacm.avis
    Call un.vfwcleanup
  end2:
  Pop $9
  Exch $0
FunctionEnd

!ifndef CORE
  !macro custPage PLUGIN
    Function CustomPage${PLUGIN}
      SectionGetFlags ${Sec_plugin${PLUGIN}} $R0
      IntOp $R0 $R0 & ${SF_SELECTED}
      IntCmp $R0 ${SF_SELECTED} is${PLUGIN} no${PLUGIN} no${PLUGIN}
      is${PLUGIN}:
       !insertmacro MUI_INSTALLOPTIONS_WRITE "iodir.ini" "Field 1" "Text" "$(DLG_${PLUGIN}_LABEL)"
       !insertmacro MUI_INSTALLOPTIONS_WRITE "iodir.ini" "Field 2" "Text" "$(DLG_${PLUGIN}_LABEL)"
       !insertmacro MUI_INSTALLOPTIONS_WRITE "iodir.ini" "Field 2" "State" $${PLUGIN}DIR
       !insertmacro MUI_HEADER_TEXT "$(DLG_${PLUGIN}_TITLE)" "$(DLG_${PLUGIN}_SUBTITLE)"
       !insertmacro MUI_INSTALLOPTIONS_DISPLAY "iodir.ini"
       !insertmacro MUI_INSTALLOPTIONS_READ $${PLUGIN}DIR "iodir.ini" "Field 2" "State"
      no${PLUGIN}:
    FunctionEnd
  !macroend

  !insertmacro custPage AVISYNTH
  !insertmacro custPage VIRTUALDUB
  !ifndef X64
    !insertmacro custPage DSCALER
  !endif
!endif

!macro codecpagesetALLUSERS INIFILE
  IntCmp $ALLUSERS 0 codecpagesetALLUSERS_off
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field 1" "State" 1
  goto codecpagesetALLUSERS_on
codecpagesetALLUSERS_off:
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field 1" "State" 0
codecpagesetALLUSERS_on:
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field 1" "Text" "$(DLG_CODEC_ALLUSERS)"
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field 1" "Left" 30
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field 1" "Right" 140
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field 1" "Top" 130
  !insertmacro MUI_INSTALLOPTIONS_WRITE ${INIFILE} "Field 1" "Bottom" 141
!macroend

!macro codecpagegetALLUSERS INIFILE
  !insertmacro MUI_INSTALLOPTIONS_READ $R0 ${INIFILE} "Field 1" "State"
  IntCmp $R0 0 codecpagegetALLUSERS_off
  StrCpy $ALLUSERS 1
  Goto codecpagegetALLUSERS_end
codecpagegetALLUSERS_off:
  StrCpy $ALLUSERS 0
codecpagegetALLUSERS_end:
!macroend

Function CustomPageCodecVideo
  StrCpy $R1 2
  StrCpy $R3 30
  StrCpy $R2 2
  !insertmacro codecpageset codecvideo.ini XVID 1
  !insertmacro codecpageset codecvideo.ini DIV3 1
  !insertmacro codecpageset codecvideo.ini DIVX 1
  !insertmacro codecpageset codecvideo.ini DX50 1
  !insertmacro codecpageset codecvideo.ini MP43 1
  !insertmacro codecpageset codecvideo.ini MP42 1
  !insertmacro codecpageset codecvideo.ini MP41 1
  !insertmacro codecpageset codecvideo.ini WMV1 1
  !insertmacro codecpageset codecvideo.ini wmv2 1
  !insertmacro codecpageset codecvideo.ini wmv3 1
  !insertmacro codecpageset codecvideo.ini _3iv 1
  !insertmacro codecpageset codecvideo.ini mpg1 1
  !insertmacro codecpageset codecvideo.ini mpg2 1
  !insertmacro codecpageset codecvideo.ini h263 1
  !insertmacro codecpageset codecvideo.ini h264 1
  !insertmacro codecpageset codecvideo.ini mjpg 1
  !insertmacro codecpageset codecvideo.ini dv   1
  !insertmacro codecpageset codecvideo.ini hfyu 1
  !insertmacro codecpageset codecvideo.ini png1 1
  !insertmacro codecpageset codecvideo.ini rawv 1
  !insertmacro codecpagesetALLUSERS codecvideo.ini

  !insertmacro MUI_HEADER_TEXT "$(DLG_CODECVIDEO_TITLE)" "$(DLG_CODECVIDEO_SUBTITLE)"
  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "codecvideo.ini"

  StrCpy $R2 2
  !insertmacro codecpageget codecvideo.ini XVID
  !insertmacro codecpageget codecvideo.ini DIV3
  !insertmacro codecpageget codecvideo.ini DIVX
  !insertmacro codecpageget codecvideo.ini DX50
  !insertmacro codecpageget codecvideo.ini MP43
  !insertmacro codecpageget codecvideo.ini MP42
  !insertmacro codecpageget codecvideo.ini MP41
  !insertmacro codecpageget codecvideo.ini WMV1
  !insertmacro codecpageget codecvideo.ini wmv2
  !insertmacro codecpageget codecvideo.ini wmv3
  !insertmacro codecpageget codecvideo.ini _3iv
  !insertmacro codecpageget codecvideo.ini mpg1
  !insertmacro codecpageget codecvideo.ini mpg2
  !insertmacro codecpageget codecvideo.ini h263
  !insertmacro codecpageget codecvideo.ini h264
  !insertmacro codecpageget codecvideo.ini mjpg
  !insertmacro codecpageget codecvideo.ini dv
  !insertmacro codecpageget codecvideo.ini hfyu
  !insertmacro codecpageget codecvideo.ini png1
  !insertmacro codecpageget codecvideo.ini rawv
  !insertmacro codecpagegetALLUSERS codecvideo.ini
FunctionEnd

Function CustomPageCodecAudio
  StrCpy $R1 2
  StrCpy $R3 30
  StrCpy $R2 2
  !insertmacro codecpageset codecaudio.ini MP2 1
  !insertmacro codecpageset codecaudio.ini MP3 1
  !insertmacro codecpageset codecaudio.ini VORBIS 1
  !insertmacro codecpageset codecaudio.ini WMA1 1
  !insertmacro codecpageset codecaudio.ini WMA2 1
  !insertmacro codecpageset codecaudio.ini AC3 1
  !ifndef CORE
    !insertmacro codecpageset codecaudio.ini DTS 1
    !insertmacro codecpageset codecaudio.ini AAC 1
  !else
    !insertmacro codecpageset codecaudio.ini DTS 0
    !insertmacro codecpageset codecaudio.ini AAC 0
  !endif
  !insertmacro codecpageset codecaudio.ini AMR 1
  !insertmacro codecpageset codecaudio.ini FLAC 1
  !insertmacro codecpageset codecaudio.ini RAWA 1
  !insertmacro codecpagesetALLUSERS codecaudio.ini

  !insertmacro MUI_HEADER_TEXT "$(DLG_CODECAUDIO_TITLE)" "$(DLG_CODECAUDIO_SUBTITLE)"
  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "codecaudio.ini"

  StrCpy $R2 2
  !insertmacro codecpageget codecaudio.ini MP2
  !insertmacro codecpageget codecaudio.ini MP3
  !insertmacro codecpageget codecaudio.ini VORBIS
  !insertmacro codecpageget codecaudio.ini WMA1
  !insertmacro codecpageget codecaudio.ini WMA2
  !insertmacro codecpageget codecaudio.ini AC3
  !ifndef CORE
    !insertmacro codecpageget codecaudio.ini DTS
    !insertmacro codecpageget codecaudio.ini AAC
  !endif
  !insertmacro codecpageget codecaudio.ini AMR
  !insertmacro codecpageget codecaudio.ini FLAC
  !insertmacro codecpageget codecaudio.ini RAWA
  !insertmacro codecpagegetALLUSERS codecaudio.ini
FunctionEnd

Function CustomPageFiltersVideo
  StrCpy $R1 2
  StrCpy $R3 30
  StrCpy $R2 1
  StrCpy $R5 11
  !insertmacro filterspageset filtersvideo.ini oqueue
  !insertmacro filterspageset filtersvideo.ini postproc
  !insertmacro filterspageset filtersvideo.ini noise
  !insertmacro filterspageset filtersvideo.ini sharpen
  !insertmacro filterspageset filtersvideo.ini subtitles

  !insertmacro MUI_HEADER_TEXT "$(DLG_FILTERSVIDEO_TITLE)" "$(DLG_FILTERSVIDEO_SUBTITLE)"
  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "filtersvideo.ini"

  StrCpy $R2 1
  !insertmacro filterspageget filtersvideo.ini oqueue
  !insertmacro filterspageget filtersvideo.ini postproc
  !insertmacro filterspageget filtersvideo.ini noise
  !insertmacro filterspageget filtersvideo.ini sharpen
  !insertmacro filterspageget filtersvideo.ini subtitles
FunctionEnd

Function CustomPageFiltersAudio
  StrCpy $R1 2
  StrCpy $R3 30
  StrCpy $R2 1
  StrCpy $R5 11
  !insertmacro filterspageset filtersaudio.ini volume
  !insertmacro filterspageset filtersaudio.ini equalizer
  !insertmacro filterspageset filtersaudio.ini mixer
  !insertmacro filterspageset filtersaudio.ini freeverb

  !insertmacro MUI_HEADER_TEXT "$(DLG_FILTERSAUDIO_TITLE)" "$(DLG_FILTERSAUDIO_SUBTITLE)"
  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "filtersaudio.ini"

  StrCpy $R2 1
  !insertmacro filterspageget filtersaudio.ini volume
  !insertmacro filterspageget filtersaudio.ini equalizer
  !insertmacro filterspageget filtersaudio.ini mixer
  !insertmacro filterspageget filtersaudio.ini freeverb
FunctionEnd

!ifdef SIMPLE
Function CustomPageSimple
  StrCpy $R0        $DECODER_XVID
  IntOp  $R0 $R0 || $DECODER_DIV3
  IntOp  $R0 $R0 || $DECODER_DIVX
  IntOp  $R0 $R0 || $DECODER_DX50
  IntOp  $R0 $R0 || $DECODER_MP43
  IntOp  $R0 $R0 || $DECODER_MP42
  IntOp  $R0 $R0 || $DECODER_MP41
  IntOp  $R0 $R0 || $DECODER_H264
  IntCmp $R0 0 customPageSimple_video_off
  !insertmacro MUI_INSTALLOPTIONS_WRITE "simple.ini" "Field 2" "State" 1
customPageSimple_video_off:

  StrCpy $R0        $DECODER_MP2
  IntOp  $R0 $R0 || $DECODER_MP3
  IntOp  $R0 $R0 || $DECODER_VORBIS
  IntOp  $R0 $R0 || $DECODER_AC3
  IntOp  $R0 $R0 || $DECODER_DTS
  IntOp  $R0 $R0 || $DECODER_AAC
  IntCmp $R0 0 customPageSimple_audio_off
  !insertmacro MUI_INSTALLOPTIONS_WRITE "simple.ini" "Field 3" "State" 1
customPageSimple_audio_off:

  StrCpy $R1 52
  StrCpy $R3 6
  StrCpy $R2 4
  StrCpy $R5 14
  !insertmacro filterspageset simple.ini subtitles
  !insertmacro filterspageset simple.ini volume

  !insertmacro MUI_HEADER_TEXT "$(DLG_SIMPLE_TITLE)" "$(DLG_SIMPLE_SUBTITLE)"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "simple.ini" "Field 2" "Text" "$(DLG_SIMPLE_VIDEO)"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "simple.ini" "Field 3" "Text" "$(DLG_SIMPLE_AUDIO)"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "simple.ini" "Field 4" "Text" "$(FILTER_subtitles_name)"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "simple.ini" "Field 5" "Text" "$(FILTER_volume_name)"
  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "simple.ini"

  !insertmacro MUI_INSTALLOPTIONS_READ $R0 "simple.ini" "Field 2" "State"
  IntCmp $R0 0 customPageSimple_video_set_off
  StrCpy $DECODER_XVID $DECODER_XVID_DEFDEC
  StrCpy $DECODER_DIV3 $DECODER_DIV3_DEFDEC
  StrCpy $DECODER_DIVX $DECODER_DIVX_DEFDEC
  StrCpy $DECODER_DX50 $DECODER_DX50_DEFDEC
  StrCpy $DECODER_MP43 $DECODER_MP43_DEFDEC
  StrCpy $DECODER_MP42 $DECODER_MP42_DEFDEC
  StrCpy $DECODER_MP41 $DECODER_MP41_DEFDEC
  StrCpy $DECODER_H264 $DECODER_H264_DEFDEC
  goto customPageSimple_video_set_end
customPageSimple_video_set_off:
  StrCpy $DECODER_XVID 0
  StrCpy $DECODER_DIV3 0
  StrCpy $DECODER_DIVX 0
  StrCpy $DECODER_DX50 0
  StrCpy $DECODER_MP43 0
  StrCpy $DECODER_MP42 0
  StrCpy $DECODER_MP41 0
  StrCpy $DECODER_H264 0
customPageSimple_video_set_end:

  !insertmacro MUI_INSTALLOPTIONS_READ $R0 "simple.ini" "Field 3" "State"
  IntCmp $R0 0 customPageSimple_audio_set_off
  StrCpy $DECODER_MP2    $DECODER_MP2_DEFDEC
  StrCpy $DECODER_MP3    $DECODER_MP3_DEFDEC
  StrCpy $DECODER_VORBIS $DECODER_VORBIS_DEFDEC
    StrCpy $DECODER_AC3    $DECODER_AC3_DEFDEC
  !ifndef CORE
    StrCpy $DECODER_DTS    $DECODER_DTS_DEFDEC
    StrCpy $DECODER_AAC    $DECODER_AAC_DEFDEC
  !endif
  goto customPageSimple_audio_set_end
customPageSimple_audio_set_off:
  StrCpy $DECODER_MP2    0
  StrCpy $DECODER_MP3    0
  StrCpy $DECODER_VORBIS 0
  StrCpy $DECODER_AC3    0
  !ifndef CORE
    StrCpy $DECODER_DTS    0
    StrCpy $DECODER_AAC    0
  !endif
customPageSimple_audio_set_end:

  StrCpy $R2 4
  !insertmacro filterspageget simple.ini subtitles
  !insertmacro filterspageget simple.ini volume
FunctionEnd
!endif

; GetWindowsVersion
;
; Based on Yazno's function, http://yazno.tripod.com/powerpimpit/
; Updated by Joost Verburg
;
; Returns on top of stack
;
; Windows Version (95, 98, ME, NT x.x, 2000, XP, 2003)
; or
; '' (Unknown Windows Version)
;
; Usage:
;   Call GetWindowsVersion
;   Pop $R0
;   ; at this point $R0 is "NT 4.0" or whatnot

!ifdef MSVCRT8
Function GetWindowsVersion

  Push $R0
  Push $R1

  ClearErrors

  ReadRegStr $R0 HKLM \
  "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion

  IfErrors 0 lbl_winnt

  ; we are not NT
  ReadRegStr $R0 HKLM \
  "SOFTWARE\Microsoft\Windows\CurrentVersion" VersionNumber

  StrCpy $R1 $R0 1
  StrCmp $R1 '4' 0 lbl_error

  StrCpy $R1 $R0 3

  StrCmp $R1 '4.0' lbl_win32_95
  StrCmp $R1 '4.9' lbl_win32_ME lbl_win32_98

  lbl_win32_95:
    StrCpy $R0 '95'
  Goto lbl_done

  lbl_win32_98:
    StrCpy $R0 '98'
  Goto lbl_done

  lbl_win32_ME:
    StrCpy $R0 'ME'
  Goto lbl_done

  lbl_winnt:

  StrCpy $R1 $R0 1

  StrCmp $R1 '3' lbl_winnt_x
  StrCmp $R1 '4' lbl_winnt_x

  StrCpy $R1 $R0 3

  StrCmp $R1 '5.0' lbl_winnt_2000
  StrCmp $R1 '5.1' lbl_winnt_XP
  StrCmp $R1 '5.2' lbl_winnt_2003 lbl_error

  lbl_winnt_x:
    StrCpy $R0 "NT $R0" 6
  Goto lbl_done

  lbl_winnt_2000:
    Strcpy $R0 '2000'
  Goto lbl_done

  lbl_winnt_XP:
    Strcpy $R0 'XP'
  Goto lbl_done

  lbl_winnt_2003:
    Strcpy $R0 '2003'
  Goto lbl_done

  lbl_error:
    Strcpy $R0 ''
  lbl_done:

  Pop $R1
  Exch $R0

FunctionEnd
!endif # MSVCRT8
