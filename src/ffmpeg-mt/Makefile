LAVC_DIR=libavcodec
LAVU_DIR=libavutil
LAVCORE_DIR=libavcore
ZLIB_DIR=../zlib

CFLAGS+= -I. -I$(LAVC_DIR) -I$(LAVU_DIR) -I$(LAVCORE_DIR) -I../pthreads -I../imgFilters -I$(ZLIB_DIR) \
         -DHAVE_AV_CONFIG_H -D_ISOC99_SOURCE -std=gnu99

#CFLAGS+= -save-temps
#CFLAGS+= -fverbose-asm

YASMFLAGS+= -Pconfig.asm

SRCS_C=\
      DllEntry.c \
      $(LAVC_DIR)/allcodecs.c \
      $(LAVC_DIR)/avpacket.c \
      $(LAVC_DIR)/bitstream.c \
      $(LAVC_DIR)/cabac.c \
      $(LAVC_DIR)/dsputil.c \
      $(LAVC_DIR)/error_resilience.c \
      $(LAVC_DIR)/faanidct.c \
      $(LAVC_DIR)/golomb.c \
      $(LAVC_DIR)/h264.c \
      $(LAVC_DIR)/h264_cabac.c \
      $(LAVC_DIR)/h264_cavlc.c \
      $(LAVC_DIR)/h264_direct.c \
      $(LAVC_DIR)/h264_loopfilter.c \
      $(LAVC_DIR)/h264_parser.c \
      $(LAVC_DIR)/h264_ps.c \
      $(LAVC_DIR)/h264_refs.c \
      $(LAVC_DIR)/h264_sei.c \
      $(LAVC_DIR)/h264dsp.c \
      $(LAVC_DIR)/h264idct.c \
      $(LAVC_DIR)/h264pred.c \
      $(LAVC_DIR)/imgconvert.c \
      $(LAVC_DIR)/jrevdct.c \
      $(LAVC_DIR)/mpegvideo.c \
      $(LAVC_DIR)/options.c \
      $(LAVC_DIR)/parser.c \
      $(LAVC_DIR)/pthread.c \
      $(LAVC_DIR)/simple_idct.c \
      $(LAVC_DIR)/utils.c \
      $(LAVC_DIR)/vp3dsp.c \
    \
      $(LAVU_DIR)/intfloat_readwrite.c \
      $(LAVU_DIR)/inverse.c \
      $(LAVU_DIR)/log.c \
      $(LAVU_DIR)/mathematics.c \
      $(LAVU_DIR)/mem.c \
      $(LAVU_DIR)/pixdesc.c \
      $(LAVU_DIR)/rational.c \
    \
      $(LAVCORE_DIR)/imgutils.c \
      $(LAVCORE_DIR)/utils.c \
    \
      $(LAVC_DIR)/x86/cpuid.c \
      $(LAVC_DIR)/x86/dsputil_mmx.c \
      $(LAVC_DIR)/x86/fdct_mmx.c \
      $(LAVC_DIR)/x86/h264_intrapred_init.c \
      $(LAVC_DIR)/x86/h264dsp_mmx.c \
      $(LAVC_DIR)/x86/idct_mmx.c \
      $(LAVC_DIR)/x86/idct_mmx_xvid.c \
      $(LAVC_DIR)/x86/idct_sse2_xvid.c \
      $(LAVC_DIR)/x86/mpegvideo_mmx.c \
      $(LAVC_DIR)/x86/simple_idct_mmx.c

SRCS_YASM=\
      $(LAVC_DIR)/x86/dsputil_yasm.asm \
      $(LAVC_DIR)/x86/h264_chromamc.asm \
      $(LAVC_DIR)/x86/h264_deblock.asm \
      $(LAVC_DIR)/x86/h264_idct_sse2.asm \
      $(LAVC_DIR)/x86/h264_intrapred.asm \
      $(LAVC_DIR)/x86/h264_weight.asm \
      $(LAVC_DIR)/x86/vp3dsp.asm \
      $(LAVC_DIR)/x86/x86util.asm

ifeq ($(64BIT),yes)
    CFLAGS+=-DARCH_X86_64
    GCCLIBS=../../bin/lib_x64/libpthreadGC2.a -lws2_32
else
    CFLAGS+=-DARCH_X86_32
    GCCLIBS=../../bin/lib_x86/libpthreadGC2.a -lws2_32
endif

SLIB=../../bin/ffmpegmt.dll
DEF=libavcodec.def

include ../makefile_c.inc

.all: $(SLIB)

clean:
	rm -f *.o *.d $(LAVC_DIR)/*.o $(LAVC_DIR)/*.d $(LAVC_DIR)/x86/*.o $(LAVC_DIR)/x86/*.d \
	$(LAVCORE_DIR)/*.o $(LAVCORE_DIR)/*.d \
  $(LAVU_DIR)/*.o $(LAVU_DIR)/*.d $(ZLIB_DIR)/*.o $(ZLIB_DIR)/*.d $(SLIB)
