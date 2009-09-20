include makefile_c.inc

ALLS=lib \
	../bin/libmpeg2_ff.dll \
	../bin/ff_x264.dll \
	../bin/xvidcore.dll \
	../bin/libmplayer.dll \
	../bin/libavcodec.dll \
	../bin/ffmpegmt.dll

ALLS+=$(LIBSONAME)

.all: $(ALLS)

lib:
../bin/libavcodec.dll:
	$(MAKE) -C ffmpeg

../bin/ffmpegmt.dll:
	$(MAKE) -C ffmpeg-mt

../bin/libmplayer.dll:
	$(MAKE) -C mplayer

../bin/libmpeg2_ff.dll:
	$(MAKE) -C codecs/libmpeg2

../bin/ff_x264.dll:
	$(MAKE) -C codecs/x264

../bin/xvidcore.dll:
	$(MAKE) -C codecs/xvidcore
	
clean:
	$(MAKE) -C ffmpeg clean
	$(MAKE) -C ffmpeg-mt clean
	$(MAKE) -C mplayer clean
	$(MAKE) -C codecs/libmpeg2 clean
	$(MAKE) -C codecs/x264 clean
	$(MAKE) -C codecs/xvidcore clean

dep:
	$(MAKE) .depend
	$(MAKE) -C ffmpeg .depend
	$(MAKE) -C ffmpeg-mt .depend
	$(MAKE) -C mplayer .depend
	$(MAKE) -C codecs/libmpeg2 .depend
	$(MAKE) -C codecs/x264 .depend
	$(MAKE) -C codecs/xvidcore .depend

distclean:
	$(MAKE) -C ffmpeg distclean
	$(MAKE) -C ffmpeg-mt distclean
	$(MAKE) -C mplayer distclean
	$(MAKE) -C codecs/libmpeg2 distclean
	$(MAKE) -C codecs/x264 distclean
	$(MAKE) -C codecs/xvidcore distclean

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
