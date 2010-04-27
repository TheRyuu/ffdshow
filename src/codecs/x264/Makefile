CFLAGS+= -I. -I../../pthreads -Icommon -Icommon/x86/ -Iencoder

CFLAGS+=-DSYS_MINGW -DHAVE_MMX -DHAVE_PTHREAD -DPTW32_STATIC_LIB

SRCS_C=ff_x264.c \
		common/mc.c \
		common/predict.c \
		common/pixel.c \
		common/macroblock.c \
		common/frame.c \
		common/dct.c \
		common/cpu.c \
		common/cabac.c \
		common/common.c \
		common/set.c \
		common/quant.c \
		common/vlc.c \
		encoder/analyse.c \
		encoder/me.c \
		encoder/ratecontrol.c \
		encoder/set.c \
		encoder/macroblock.c encoder/cabac.c \
		encoder/cavlc.c \
		encoder/encoder.c \
		encoder/lookahead.c

SRCS_C+=common/x86/mc-c.c \
		common/x86/predict-c.c

SRCS_YASM=common/x86/cabac-a.asm \
			common/x86/dct-a.asm \
			common/x86/deblock-a.asm \
			common/x86/mc-a.asm \
			common/x86/mc-a2.asm \
			common/x86/pixel-a.asm \
			common/x86/predict-a.asm \
			common/x86/quant-a.asm \
			common/x86/sad-a.asm \
			common/x86/cpu-a.asm \
			common/x86/x86inc.asm \
			common/x86/x86inc.asm \
			common/x86/x86util.asm

YASMFLAGS=-DHAVE_MMX

ifneq ($(64BIT),yes)
	CFLAGS+=-DARCH_X86
	YASMFLAGS+=-DARCH_X86
	SRCS_YASM+=common/x86/dct-32.asm common/x86/pixel-32.asm
	GCCLIBS=../../../bin/lib_x86/libpthreadGC2.a -lws2_32
else
	CFLAGS+=-DARCH_X86_64
	SRCS_YASM+=common/x86/dct-64.asm
	GCCLIBS=../../../bin/lib_x64/libpthreadGC2.a -lws2_32
endif

SLIB=../../../bin/ff_x264.dll
DEF=ff_x264.def

include ../../makefile_c.inc

.all: $(SLIB)

clean:
	rm -f *.o encoder/*.o common/*.o common/x86/*.o *~ $(SLIB) *.d encoder/*.d common/*.d common/x86/*.d
