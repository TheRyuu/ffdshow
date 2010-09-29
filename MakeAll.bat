@ECHO OFF
REM Add MinGW\bin directory to PATH (Control Panel > System > Advanced > Environment Variables)
pushd src\ffmpeg
make -j4
popd

pushd src\ffmpeg-mt
make -j4
popd

pushd src\imgFilters\KernelDeint
make -j4
popd

pushd src\codecs\x264
make -j4
popd

pushd src\codecs\xvidcore
make -j4
popd