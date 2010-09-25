@IF "%PROGRAMFILES(x86)%zzz"=="zzz" (
  SET FF_PROG_FILES=%PROGRAMFILES%
) ELSE (
  SET FF_PROG_FILES=%PROGRAMFILES(x86)%
)

@SET ISCC="%FF_PROG_FILES%\Inno Setup 5\ISCC.exe"
@SET MSVC9="%FF_PROG_FILES%\Microsoft Visual Studio 9.0\Common7\IDE\devenv.exe"

@echo [Removing files]
@cd bin
@del /Q *.dll *.exe
@cd ..\

@echo [Compiling with MSVC]
%MSVC9% %SOLUTIONFILE1% %BUILDTYPE% %BUILDTARGET1% /project rebase
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
%MSVC9% %SOLUTIONFILE1% %BUILDTYPE% %BUILDTARGET1% /project makeAVIS
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
%MSVC9% %SOLUTIONFILE1% %BUILDTYPE% %BUILDTARGET1% /project ffavisynth
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
%MSVC9% %SOLUTIONFILE1% %BUILDTYPE% %BUILDTARGET1% /project ffvdub
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
%MSVC9% %SOLUTIONFILE1% %BUILDTYPE% %BUILDTARGET1% /project FLT_ffdshow
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
%MSVC9% %SOLUTIONFILE1% %BUILDTYPE% %BUILDTARGET1% /project ff_vfw
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
%MSVC9% %SOLUTIONFILE1% %BUILDTYPE% %BUILDTARGET1% /project ff_acm
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
%MSVC9% %SOLUTIONFILE1% %BUILDTYPE% %BUILDTARGET1% /project ff_wmv9
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
%MSVC9% %SOLUTIONFILE1% %BUILDTYPE% %BUILDTARGET1% /project TomsMoComp_ff
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
%MSVC9% %SOLUTIONFILE1% %BUILDTYPE% %BUILDTARGET1% /project ff_kernelDeint
IF %ERRORLEVEL% NEQ 0 GOTO :GotError

%MSVC9% %SOLUTIONFILE2% %BUILDTYPE% %BUILDTARGET2% /project ff_unrar
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
%MSVC9% %SOLUTIONFILE2% %BUILDTYPE% %BUILDTARGET2% /project ff_libmad
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
%MSVC9% %SOLUTIONFILE2% %BUILDTYPE% %BUILDTARGET2% /project ff_liba52
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
%MSVC9% %SOLUTIONFILE2% %BUILDTYPE% %BUILDTARGET2% /project ff_samplerate
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
%MSVC9% %SOLUTIONFILE2% %BUILDTYPE% %BUILDTARGET2% /project ff_tremor
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
%MSVC9% %SOLUTIONFILE2% %BUILDTYPE% %BUILDTARGET2% /project ff_libdts
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
%MSVC9% %SOLUTIONFILE2% %BUILDTYPE% %BUILDTARGET2% /project ff_libfaad2
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
%MSVC9% %SOLUTIONFILE2% %BUILDTYPE% %BUILDTARGET2% /project libmpeg2_ff
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
%MSVC9% %SOLUTIONFILE2% %BUILDTYPE% %BUILDTARGET2% /project ffdshow
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError

@echo [make clean]
@cd src\ffmpeg
@make clean
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
@cd ..\..
@cd src\ffmpeg-mt
@make clean
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
@cd ..\..
@cd src\mplayer
@make clean
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
@cd ..\..
@cd src\codecs\x264
@make clean
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
@cd ..\..\..
@cd src\codecs\xvidcore
@make clean
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
@cd ..\..\..

@IF "%FF_BUILD_NAME%"=="x64" (
  SET FF_MAKE_PARAM=64BIT=yes
) ELSE (
  SET FF_MAKE_PARAM=
)

@echo [Compiling with GCC]
@cd src\ffmpeg
@make %FF_MAKE_PARAM%
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
@cd ..\..
@cd src\ffmpeg-mt
@make %FF_MAKE_PARAM%
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
@cd ..\..
@cd src\mplayer
@make %FF_MAKE_PARAM%
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
@cd ..\..
@cd src\codecs\x264
@make %FF_MAKE_PARAM%
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
@cd ..\..\..
@cd src\codecs\xvidcore
@make %FF_MAKE_PARAM%
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
@cd ..\..\..

@echo [Building installer]
@cd bin\distrib
@%ISCC% ffdshow_installer.iss
@IF %ERRORLEVEL% NEQ 0 GOTO :GotError
@cd ..\..\..

@goto :EOF

:GotError
@echo There was an error!
@pause