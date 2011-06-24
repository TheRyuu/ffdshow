@ECHO OFF

IF "%PROGRAMFILES(x86)%zzz"=="zzz" (
  SET FF_PROGRAMFILES=%PROGRAMFILES%
) ELSE (
  SET FF_PROGRAMFILES=%PROGRAMFILES(x86)%
)

IF "%FF_TARGET%"=="x64" (
  SET FF_MAKE_PARAM=64BIT=yes
  SET ISCC_PARAM=/dis64bit=yes
) ELSE (
  SET FF_MAKE_PARAM=
  SET ISCC_PARAM=
)

SET ISCC="%FF_PROGRAMFILES%\Inno Setup 5\ISCC.exe"


echo [Removing files]
cd bin
del /Q *.dll *.exe
cd ..\


echo [Compiling with MSVC]

call "%VS100COMNTOOLS%vsvars32.bat"
devenv %SOLUTIONFILE% %BUILDTYPE% %BUILDTARGET%
IF %ERRORLEVEL% NEQ 0 GOTO :GotError


echo [Compiling with GCC]

cd src\ffmpeg
IF NOT "%BUILDTYPE%"=="/build" (
  make clean
  IF %ERRORLEVEL% NEQ 0 GOTO :GotError
)
IF NOT "%BUILDTYPE%"=="/clean" (
  make %FF_MAKE_PARAM%
  IF %ERRORLEVEL% NEQ 0 GOTO :GotError
)
cd ..\..

IF "%FF_TARGET%"=="x86" (
  cd src\imgFilters\KernelDeint
  IF NOT "%BUILDTYPE%"=="/build" (
    make clean
    IF %ERRORLEVEL% NEQ 0 GOTO :GotError
  )
  IF NOT "%BUILDTYPE%"=="/clean" (
    make %FF_MAKE_PARAM%
    IF %ERRORLEVEL% NEQ 0 GOTO :GotError
  )
  cd ..\..\..
)


echo [Building installer]

IF EXIST %ISCC% (
  cd bin\distrib
  %ISCC% ffdshow_installer.iss %FF_MAKE_PARAM%
  IF %ERRORLEVEL% NEQ 0 GOTO :GotError
  cd ..\..
) else (
  echo InnoSetup not found
  pause
)


goto :EOF


:GotError
echo There was an error!
pause