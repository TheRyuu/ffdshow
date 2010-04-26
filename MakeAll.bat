REM Add MinGW\bin directory to PATH (Control Panel > System > Advanced > Environment Variables)
cd src\ffmpeg
make -j4
cd ..\..\src\ffmpeg-mt
make -j4
cd ..\..\src\mplayer
make -j4
cd ..\..\src\codecs\x264
make -j4
cd ..\..\src\codecs\xvidcore
make -j4
REM uncomment the following 2 lines to build theora
REM cd ..\..\..\src\codecs\theora
REM make -j4
cd ..\..\..