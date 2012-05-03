@ECHO OFF

IF DEFINED PROGRAMFILES(x86) (
  SET "SUBWCREV=%ProgramW6432%\TortoiseSVN\bin\SubWCRev.exe"
) ELSE (
  SET "SUBWCREV=%ProgramFiles%\TortoiseSVN\bin\SubWCRev.exe"
)

PUSHD %~dp0
"%SUBWCREV%" . "src\SubWCRev.conf" "src\svn_version.h" -f
IF %ERRORLEVEL% NEQ 0 GOTO NoSubWCRev
POPD
EXIT /B


:NoSubWCRev
ECHO. & ECHO SubWCRev, which is part of TortoiseSVN, wasn't found!
ECHO You should (re)install TortoiseSVN.
ECHO I'll use SVN_REVISION=0.

ECHO #define SVN_REVISION 0 > "src\svn_version.h"
ECHO #define BUILD_YEAR 2012 >> "src\svn_version.h"
ECHO #define BUILD_MONTH 1 >> "src\svn_version.h"
ECHO #define BUILD_DAY 1 >> "src\svn_version.h"

POPD
EXIT /B
