ALLDLLS=LIBMPEG2 XVIDCORE TOMSMOCOMP KDINT FFMPEG FFMPEGMT

FFDLLS=FFMPEG FFMPEGMT XVIDCORE

ALLS=../bin/ffmpeg.dll \
     ../bin/ffmpegmt.dll \
     ../bin/libmpeg2_ff.dll \
     ../bin/ff_xvidcore.dll \
     ../bin/TomsMoComp_ff.dll \
     ../bin/ff_kernelDeint.dll

.all: $(ALLDLLS)

FF: $(FFDLLS)

ALL: $(ALLDLLS)

../bin/ffmpeg.dll:
	$(MAKE) -C ffmpeg
FFMPEG:
	$(MAKE) -C ffmpeg

../bin/ffmpegmt.dll:
	$(MAKE) -C ffmpeg-mt
FFMPEGMT:
	$(MAKE) -C ffmpeg-mt

../bin/libmpeg2_ff.dll:
	$(MAKE) -C codecs/libmpeg2
LIBMPEG2:
	$(MAKE) -C codecs/libmpeg2

../bin/ff_xvidcore.dll:
	$(MAKE) -C codecs/xvidcore
XVIDCORE:
	$(MAKE) -C codecs/xvidcore

../bin/TomsMoComp_ff.dll:
	$(MAKE) -C imgFilters/TomsMoComp
TOMSMOCOMP:
	$(MAKE) -C imgFilters/TomsMoComp

../bin/ff_kernelDeint.dll:
	$(MAKE) -C imgFilters/KernelDeint
KDINT:
	$(MAKE) -C imgFilters/KernelDeint

clean:	
	$(MAKE) -C ffmpeg clean
	$(MAKE) -C ffmpeg-mt clean
	$(MAKE) -C codecs/libmpeg2 clean
	$(MAKE) -C codecs/xvidcore clean
	$(MAKE) -C imgFilters/TomsMoComp clean
	$(MAKE) -C imgFilters/KernelDeint clean

distclean:
	$(MAKE) -C ffmpeg distclean
	$(MAKE) -C ffmpeg-mt distclean
	$(MAKE) -C codecs/libmpeg2 distclean
	$(MAKE) -C codecs/xvidcore distclean
	$(MAKE) -C imgFilters/TomsMoComp distclean
	$(MAKE) -C imgFilters/KernelDeint distclean

.PHONY: $(FFDLLS)
