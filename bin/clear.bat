@echo off
pushd %~dp0

for %%a in (
"makeAVIS.exe" ^
"ffavisynth.dll" ^
"ffmpeg.dll" ^
"ff_kernelDeint.dll" ^
"ff_liba52.dll" ^
"ff_libdts.dll" ^
"ff_libfaad2.dll" ^
"ff_libmad.dll" ^
"ff_samplerate.dll" ^
"ff_unrar.dll" ^
"ff_vfw.dll" ^
"ff_wmv9.dll" ^
"FLT_ffdshow.dll" ^
"IntelQuickSyncDecoder.dll" ^
"libmpeg2_ff.dll" ^
"TomsMoComp_ff.dll" ^
"ffdshow.ax" ^
"ffvdub.vdf" ^
"ff_acm.acm"
) do if exist %%a del /Q %%a

popd