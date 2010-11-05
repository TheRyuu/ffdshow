@ECHO OFF
REM Add MinGW\bin directory to PATH (Control Panel > System > Advanced > Environment Variables)
pushd src\ffmpeg
make clean && make -j4
popd

pushd src\ffmpeg-mt
make clean && make -j4
popd

pushd src\imgFilters\KernelDeint
make clean && make -j4
popd

pushd src\codecs\x264
make clean && make -j4
popd

pushd src\codecs\xvidcore
make clean && make -j4
popd