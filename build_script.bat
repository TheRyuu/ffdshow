@ECHO OFF

if "%FF_TARGET%"=="x64" (
  set FF_MAKE_PARAM=64BIT=yes
  set ISCC_PARAM=/dis64bit
) else (
  set FF_MAKE_PARAM=
  set ISCC_PARAM=
)


echo [Removing files]
pushd bin
call clear.bat
popd

echo [Compiling with MSVC]

call "%VS100COMNTOOLS%vsvars32.bat"
title %SOLUTIONFILE% %BUILDTYPE% %BUILDTARGET%
devenv %SOLUTIONFILE% %BUILDTYPE% %BUILDTARGET%
if %ERRORLEVEL% neq 0 goto GotError


echo [Compiling with GCC]

pushd src\ffmpeg
if not "%BUILDTYPE%"=="/build" (
  make clean
  if %ERRORLEVEL% neq 0 goto GotError
)
if not "%BUILDTYPE%"=="/clean" (
  make -j%NUMBER_OF_PROCESSORS% %FF_MAKE_PARAM%
  if %ERRORLEVEL% neq 0 goto GotError
)
popd

if "%FF_TARGET%"=="x86" (
  pushd src\imgFilters\KernelDeint
  if not "%BUILDTYPE%"=="/build" (
    make clean
    if %ERRORLEVEL% neq 0 goto GotError
  )
  if not "%BUILDTYPE%"=="/clean" (
    make -j%NUMBER_OF_PROCESSORS% %FF_MAKE_PARAM%
    if %ERRORLEVEL% neq 0 goto GotError
  )
  popd
)


if "%BUILDTYPE%"=="/clean" exit /b
echo [Building installer]

call :SubDetectInnoSetup
set ISCC="%InnoSetupPath%\ISCC.exe"

if exist %ISCC% (
  cd bin\distrib
  %ISCC% ffdshow_installer.iss %ISCC_PARAM%
  if %ERRORLEVEL% neq 0 goto GotError
  cd ..\..
) else (
  echo InnoSetup not found
  pause
)


exit /b

:SubDetectInnoSetup
if defined PROGRAMFILES(x86) (
  set "U_=HKLM\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall"
) else (
  set "U_=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"
)

for /F "delims=" %%a in (
  'REG QUERY "%U_%\Inno Setup 5_is1" /v "Inno Setup: App Path"2^>Nul^|FIND "REG_"') do (
  set "InnoSetupPath=%%a" & CALL :SubInnoSetup %%InnoSetupPath:*Z=%%)
exit /b

:SubInnoSetup
set InnoSetupPath=%*
exit /b

:GotError
echo There was an error!
pause