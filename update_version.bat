@if not exist "%programfiles%\TortoiseSVN\bin\SubWCRev.exe" goto :x64

@"%programfiles%\TortoiseSVN\bin\SubWCRev.exe" .\ src\SubWCRev.conf src\svn_version.h -f
@if %ERRORLEVEL% neq 0 goto :NoSubWCRev
@goto :eof

:x64
@if not exist "%ProgramW6432%\TortoiseSVN\bin\SubWCRev.exe" goto :NoSubWCRev

@"%ProgramW6432%\TortoiseSVN\bin\SubWCRev.exe" .\ src\SubWCRev.conf src\svn_version.h -f
@if %ERRORLEVEL% neq 0 goto :NoSubWCRev
@goto :eof

:NoSubWCRev
@echo NoSubWCRev
@echo #define SVN_REVISION 0 > src\svn_version.h
@echo #define BUILD_YEAR 2010 >> src\svn_version.h
@echo #define BUILD_MONTH 1 >> src\svn_version.h
@echo #define BUILD_DAY 1 >> src\svn_version.h
