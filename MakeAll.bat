@ECHO OFF
REM Add MinGW\bin directory to PATH (Control Panel > System > Advanced > Environment Variables)

FOR %%A IN (ffmpeg ffmpeg-mt KernelDeint xvidcore
) DO (
  PUSHD "src\%%A"
  make clean
  make -j4
  POPD
)
