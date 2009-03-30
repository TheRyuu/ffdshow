@echo off

rem After building x64 version of ffdshow,
rem double click this manually to register.
rem Because MSVC is 32bit process, it cannot call x64 version of regedit.
rem It calls 32bit version of regedit, ending up writing to 
rem "HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\GNU\ffdshow64"

echo Windows Registry Editor Version 5.00 > ffdshow.reg
echo. >> ffdshow.reg
echo [HKEY_LOCAL_MACHINE\SOFTWARE\GNU\ffdshow64] >> ffdshow.reg
set FFPATH=%~dp0%
echo "pthPriority"="%FFPATH:\=\\%" >> ffdshow.reg
regedit /s ffdshow.reg
start regsvr32 %FFPATH%ffdshow.ax /s
del ffdshow.reg
