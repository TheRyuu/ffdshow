@echo off
setlocal
REM Add MinGW\bin directory to PATH (Control Panel > System > Advanced > Environment Variables)
REM or uncomment the next line
rem set PATH=%MSYS%\bin;%MINGW32%\bin;%PATH%

rem if /I "%1"=="x64" set MAKE_PARAMS=64BIT=yes

for %%A in (ffmpeg imgFilters\KernelDeint
) do (
  pushd "src\%%A"
  make clean
  title [%%A] make -j%NUMBER_OF_PROCESSORS%
  make -j%NUMBER_OF_PROCESSORS%
  popd
)

endlocal