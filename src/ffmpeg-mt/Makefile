### FILES and DIRECTORIES ###

SRCS_C=\
    DllEntry.c \
    libavcodec/allcodecs.c \
    libavcodec/avpacket.c \
    libavcodec/bitstream.c \
    libavcodec/cabac.c \
    libavcodec/dsputil.c \
    libavcodec/error_resilience.c \
    libavcodec/golomb.c \
    libavcodec/h264.c \
    libavcodec/h264_cabac.c \
    libavcodec/h264_cavlc.c \
    libavcodec/h264_direct.c \
    libavcodec/h264_loopfilter.c \
    libavcodec/h264_parser.c \
    libavcodec/h264_ps.c \
    libavcodec/h264_refs.c \
    libavcodec/h264_sei.c \
    libavcodec/h264dsp.c \
    libavcodec/h264idct.c \
    libavcodec/h264pred.c \
    libavcodec/huffyuv.c \
    libavcodec/imgconvert.c \
    libavcodec/jrevdct.c \
    libavcodec/mpegvideo.c \
    libavcodec/options.c \
    libavcodec/parser.c \
    libavcodec/pthread.c \
    libavcodec/simple_idct.c \
    libavcodec/utils.c \
    \
    libavcodec/x86/dsputil_mmx.c \
    libavcodec/x86/fdct_mmx.c \
    libavcodec/x86/h264_intrapred_init.c \
    libavcodec/x86/h264dsp_mmx.c \
    libavcodec/x86/idct_mmx.c \
    libavcodec/x86/idct_mmx_xvid.c \
    libavcodec/x86/idct_sse2_xvid.c \
    libavcodec/x86/mpegvideo_mmx.c \
    libavcodec/x86/simple_idct_mmx.c \
    \
    libavutil/avstring.c \
    libavutil/cpu.c \
    libavutil/dict.c \
    libavutil/eval.c \
    libavutil/imgutils.c \
    libavutil/intfloat_readwrite.c \
    libavutil/inverse.c \
    libavutil/log.c \
    libavutil/mathematics.c \
    libavutil/mem.c \
    libavutil/opt.c \
    libavutil/pixdesc.c \
    libavutil/rational.c \
    libavutil/utils.c \
    \
    libavutil/x86/cpu.c \


SRCS_YASM=\
    libavcodec/x86/dsputil_yasm.asm \
    libavcodec/x86/h264_chromamc.asm \
    libavcodec/x86/h264_deblock.asm \
    libavcodec/x86/h264_deblock_10bit.asm \
    libavcodec/x86/h264_idct.asm \
    libavcodec/x86/h264_idct_10bit.asm \
    libavcodec/x86/h264_intrapred.asm \
    libavcodec/x86/h264_intrapred_10bit.asm \
    libavcodec/x86/h264_weight.asm \
    libavcodec/x86/vp3dsp.asm \
    libavcodec/x86/x86util.asm

ifeq ($(64BIT),yes)
    OBJ_DIR=../../obj/MinGW64/ffmpegmt/
    TARGET_LIB_DIR=../../bin/
else
    OBJ_DIR=../../obj/MinGW32/ffmpegmt/
    TARGET_LIB_DIR=../../bin/
endif

OBJ_DIRS=\
    $(OBJ_DIR) \
    $(OBJ_DIR)libavcodec \
    $(OBJ_DIR)libavcodec/x86 \
    $(OBJ_DIR)libavutil \
    $(OBJ_DIR)libavutil/x86

TARGET_LIB=$(TARGET_LIB_DIR)ffmpegmt.dll

DEF=ffmpegmt.def


### SETTINGS ###

CFLAGS=-I. -Ilibavcodec -Ilibavutil -I../pthreads -I../imgFilters -DHAVE_AV_CONFIG_H -D_ISOC99_SOURCE -std=c99 \
       -fomit-frame-pointer -fno-math-errno -fno-signed-zeros -Wdeclaration-after-statement -Wno-parentheses -Wno-switch \
       -Wdisabled-optimization -Wpointer-arith -Wredundant-decls -Wno-pointer-sign -Wcast-qual -Wwrite-strings -Wtype-limits
       
#CFLAGS+=-Wall -Wundef

ifeq ($(64BIT),yes)
    CFLAGS+=-DARCH_X86_64
    GCCLIBS=../../bin/lib_x64/libpthreadGC2.a -lws2_32
else
    CFLAGS+=-DARCH_X86_32
    GCCLIBS=../../bin/lib_x86/libpthreadGC2.a -lws2_32
endif

#CFLAGS+=-save-temps
#CFLAGS+=-fverbose-asm

YASMFLAGS+=-Pconfig.asm


### INCLUDES ###

include ../makefile_c.inc


### TARGETS ###

.all: outdirs $(TARGET_LIB)

.PHONY: outdirs $(OBJ_DIRS)
outdirs: $(OBJ_DIRS)
$(OBJ_DIRS):
ifdef POSIX_COMPATIBLE_SHELL
	$(shell test -d $(@) || mkdir -p $(@))
else
	@IF NOT EXIST "$(@)" mkdir "$(@)"
endif


.PHONY: clean
clean:
ifdef POSIX_COMPATIBLE_SHELL
	rm -f $(TARGET_LIB)
	rm -r -f $(OBJ_DIR)
else
	IF EXIST "$(TARGET_LIB)" del "$(subst /,\,$(TARGET_LIB))"
	IF EXIST "$(OBJ_DIR)" rd /s /q "$(OBJ_DIR)"
endif
