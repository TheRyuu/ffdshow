ALLDLLS=LIBMPEG2 LIBMAD LIBFAAD2 THEORA X264 XVIDCORE LIBDTS SAMPLERATE TREMOR \
		TOMSMOCOMP KDINT UNRAR LIBA52 FFMPEG FFMPEGMT LIBMP

FFDLLS=FFMPEG FFMPEGMT LIBMP \
		X264 XVIDCORE

ALLS=../bin/libavcodec.dll \
     ../bin/ffmpegmt.dll \
     ../bin/libmplayer.dll \
     ../bin/libmpeg2_ff.dll \
     ../bin/ff_libmad.dll \
     ../bin/ff_libfaad2.dll \
     ../bin/ff_theora.dll \
     ../bin/ff_x264.dll \
     ../bin/ff_xvidcore.dll \
     ../bin/ff_libdts.dll \
     ../bin/ff_samplerate.dll \
     ../bin/ff_tremor.dll \
     ../bin/TomsMoComp_ff.dll \
     ../bin/ff_kernelDeint.dll \
     ../bin/ff_unrar.dll \
     ../bin/ff_liba52.dll

.all: $(ALLDLLS)

FF: $(FFDLLS)

ALL: $(ALLDLLS)

../bin/libavcodec.dll:
	$(MAKE) -C ffmpeg
FFMPEG:
	$(MAKE) -C ffmpeg

../bin/ffmpegmt.dll:
	$(MAKE) -C ffmpeg-mt
FFMPEGMT:
	$(MAKE) -C ffmpeg-mt

../bin/libmplayer.dll:
	$(MAKE) -C mplayer
LIBMP:
	$(MAKE) -C mplayer

../bin/libmpeg2_ff.dll:
	$(MAKE) -C codecs/libmpeg2
LIBMPEG2:
	$(MAKE) -C codecs/libmpeg2

../bin/ff_libmad.dll:
	$(MAKE) -C codecs/libmad
LIBMAD:
	$(MAKE) -C codecs/libmad

../bin/ff_libfaad2.dll:
	$(MAKE) -C codecs/faad
LIBFAAD2:
	$(MAKE) -C codecs/faad

../bin/ff_theora.dll:
	$(MAKE) -C codecs/theora
THEORA:
	$(MAKE) -C codecs/theora

../bin/ff_x264.dll:
	$(MAKE) -C codecs/x264
X264:
	$(MAKE) -C codecs/x264

../bin/ff_xvidcore.dll:
	$(MAKE) -C codecs/xvidcore
XVIDCORE:
	$(MAKE) -C codecs/xvidcore

../bin/ff_liba52.dll:
	$(MAKE) -C codecs/liba52
LIBA52:
	$(MAKE) -C codecs/liba52

../bin/ff_libdts.dll:
	$(MAKE) -C codecs/libdts
LIBDTS:
	$(MAKE) -C codecs/libdts

../bin/ff_samplerate.dll:
	$(MAKE) -C audioFilters/resample/libsamplerate
SAMPLERATE:
	$(MAKE) -C audioFilters/resample/libsamplerate

../bin/ff_tremor.dll:
	$(MAKE) -C codecs/tremor
TREMOR:
	$(MAKE) -C codecs/tremor

../bin/TomsMoComp_ff.dll:
	$(MAKE) -C imgFilters/TomsMoComp
TOMSMOCOMP:
	$(MAKE) -C imgFilters/TomsMoComp

../bin/ff_kernelDeint.dll:
	$(MAKE) -C imgFilters/KernelDeint
KDINT:
	$(MAKE) -C imgFilters/KernelDeint

../bin/ff_unrar.dll:
	$(MAKE) -C ../unrar
UNRAR:
	$(MAKE) -C ../unrar

clean:	
	$(MAKE) -C ffmpeg clean
	$(MAKE) -C ffmpeg-mt clean
	$(MAKE) -C mplayer clean
	$(MAKE) -C codecs/libmpeg2 clean
	$(MAKE) -C codecs/libmad clean
	$(MAKE) -C codecs/faad clean
	$(MAKE) -C codecs/theora clean
	$(MAKE) -C codecs/x264 clean
	$(MAKE) -C codecs/xvidcore clean
	$(MAKE) -C codecs/liba52 clean
	$(MAKE) -C codecs/libdts clean
	$(MAKE) -C audioFilters/resample/libsamplerate clean
	$(MAKE) -C codecs/tremor clean
	$(MAKE) -C imgFilters/TomsMoComp clean
	$(MAKE) -C imgFilters/KernelDeint clean
	$(MAKE) -C ../unrar clean

distclean:
	$(MAKE) -C ffmpeg distclean
	$(MAKE) -C ffmpeg-mt distclean
	$(MAKE) -C mplayer distclean
	$(MAKE) -C codecs/libmpeg2 distclean
	$(MAKE) -C codecs/libmad distclean
	$(MAKE) -C codecs/faad distclean
	$(MAKE) -C codecs/theora distclean
	$(MAKE) -C codecs/x264 distclean
	$(MAKE) -C codecs/xvidcore distclean
	$(MAKE) -C codecs/liba52 distclean
	$(MAKE) -C codecs/libdts distclean
	$(MAKE) -C audioFilters/resample/libsamplerate distclean
	$(MAKE) -C codecs/tremor distclean
	$(MAKE) -C imgFilters/TomsMoComp distclean
	$(MAKE) -C imgFilters/KernelDeint distclean
	$(MAKE) -C ../unrar distclean

.PHONY: $(FFDLLS)
