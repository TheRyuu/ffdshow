@if not exist "%programfiles%\TortoiseSVN\bin\SubWCRev.exe" goto :x64

@"%programfiles%\TortoiseSVN\bin\SubWCRev.exe" .\ src\SubWCRev.conf src\svn_version.h
@if %ERRORLEVEL%==6 goto :NoSubWCRev
@if %ERRORLEVEL%==10 goto :NoSubWCRev
@goto :eof

:x64
@if not exist "%ProgramW6432%\TortoiseSVN\bin\SubWCRev.exe" goto :NoSubWCRev

@"%ProgramW6432%\TortoiseSVN\bin\SubWCRev.exe" .\ src\SubWCRev.conf src\svn_version.h
@if %ERRORLEVEL%==6 goto :NoSubWCRev
@if %ERRORLEVEL%==10 goto :NoSubWCRev
@goto :eof

:NoSubWCRev
@echo NoSubWCRev
@if exist src\svn_version.h del src\svn_version.h
@echo #define SVN_REVISION 0 >> src\svn_version.h
@echo #define BUILD_YEAR 2010 >> src\svn_version.h
@echo #define BUILD_MONTH 1 >> src\svn_version.h
@echo #define BUILD_DAY 1 >> src\svn_version.h
