LAVC_DIR=libavcodec
LAVU_DIR=libavutil
ZLIB_DIR=../zlib

CFLAGS+= -I. -I$(LAVC_DIR) -I$(LAVU_DIR) -I../imgFilters -I$(ZLIB_DIR) \
         -DHAVE_AV_CONFIG_H -D_ISOC99_SOURCE -std=gnu99

#CFLAGS+= -save-temps
#CFLAGS+= -fverbose-asm

SRCS_C=\
      DllEntry.c \
      $(LAVC_DIR)/allcodecs.c \
      $(LAVC_DIR)/bitstream.c \
      $(LAVC_DIR)/cabac.c \
      $(LAVC_DIR)/dsputil.c \
      $(LAVC_DIR)/error_resilience.c \
      $(LAVC_DIR)/faanidct.c \
      $(LAVC_DIR)/golomb.c \
      $(LAVC_DIR)/h264.c \
      $(LAVC_DIR)/h264_parser.c \
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
      $(LAVU_DIR)/log.c \
      $(LAVU_DIR)/mathematics.c \
      $(LAVU_DIR)/mem.c \
      $(LAVU_DIR)/rational.c \
    \
      $(LAVC_DIR)/x86/cpuid.c \
      $(LAVC_DIR)/x86/dsputil_mmx.c \
      $(LAVC_DIR)/x86/fdct_mmx.c \
      $(LAVC_DIR)/x86/idct_mmx.c \
      $(LAVC_DIR)/x86/idct_mmx_xvid.c \
      $(LAVC_DIR)/x86/idct_sse2_xvid.c \
      $(LAVC_DIR)/x86/mpegvideo_mmx.c \
      $(LAVC_DIR)/x86/simple_idct_mmx.c \
      $(LAVC_DIR)/x86/vp3dsp_mmx.c \
      $(LAVC_DIR)/x86/vp3dsp_sse2.c

ifneq ($(64BIT),yes)
SRCS_YASM=\
      $(LAVC_DIR)/x86/dsputil_yasm.asm \
      $(LAVC_DIR)/x86/h264_deblock_sse2.asm \
      $(LAVC_DIR)/x86/h264_idct_sse2.asm \
      $(LAVC_DIR)/x86/x86util.asm
endif

ifeq ($(64BIT),yes)
    CFLAGS+=-DARCH_X86_64
else
    CFLAGS+=-DARCH_X86_32
endif

GCCLIBS=-lpthread

SLIB=../../bin/ffmpegmt.dll
DEF=libavcodec.def

NOINTRIN=1

include ../makefile_c.inc

.all: $(SLIB)

clean:
	rm -f *.o *.d $(LAVC_DIR)/*.o $(LAVC_DIR)/*.d $(LAVC_DIR)/x86/*.o $(LAVC_DIR)/x86/*.d \
    $(LAVC_DIR)/amr_float/*.o $(LAVC_DIR)/amr_float/*.d $(LAVU_DIR)/*.o $(LAVU_DIR)/*.d \
    $(ZLIB_DIR)/*.o $(ZLIB_DIR)/*.d $(SLIB)

#
# include dependency files if they exist
#
-include $(SRCS:.cpp=.d) $(SRCS_C:.c=.d)
