@ECHO OFF
REM Add MinGW\bin directory to PATH (Control Panel > System > Advanced > Environment Variables)

FOR %%A IN (ffmpeg imgFilters\KernelDeint
) DO (
  PUSHD "src\%%A"
  make clean
  make -j%NUMBER_OF_PROCESSORS%
  POPD
)
