@echo off
echo Windows Registry Editor Version 5.00 > ffdshow.reg
echo. >> ffdshow.reg

if "%PROCESSOR_ARCHITECTURE%"=="AMD64" goto os64bit
REM OS is 32bit
echo [HKEY_LOCAL_MACHINE\SOFTWARE\GNU\ffdshow] >> ffdshow.reg
set FFPATH=%~dp0%
echo "pthPriority"="%FFPATH:\=\\%" >> ffdshow.reg
regedit /s ffdshow.reg
start %WINDIR%\system32\regsvr32 %FFPATH%ffdshow.ax /s
goto endif

:os64bit
REM OS is 64bit
echo [HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\GNU\ffdshow] >> ffdshow.reg
set FFPATH=%~dp0%
echo "pthPriority"="%FFPATH:\=\\%" >> ffdshow.reg
%WINDIR%\SysWOW64\regedit /s ffdshow.reg
start %WINDIR%\SysWOW64\regsvr32 %FFPATH%ffdshow.ax /s

:endif
del ffdshow.reg
