CFLAGS+= -I. -I../.. -Icommon/x86/

CFLAGS+=-DHAVE_MMX -DHAVE_PTHREAD

ifeq ($(64BIT),yes)
    CFLAGS+=-DARCH_X86_64
else
    CFLAGS+=-DARCH_X86
endif

SRCS_C=common/mc.c common/predict.c common/pixel.c common/macroblock.c \
       common/frame.c common/dct.c common/cpu.c common/cabac.c \
       common/common.c common/set.c common/quant.c \
       encoder/analyse.c encoder/me.c encoder/ratecontrol.c \
       encoder/set.c encoder/macroblock.c encoder/cabac.c \
       encoder/cavlc.c encoder/encoder.c encoder/eval.c \
       ff_x264.c

ifneq ($(CC),icl)
SRCS_C+=common/x86/mc-c.c common/x86/predict-c.c

ifeq ($(64BIT),yes)
SRCS_NASM=common/x86/cabac-a.asm common/x86/dct-a.asm common/x86/deblock-a.asm \
          common/x86/mc-a.asm common/x86/mc-a2.asm \
          common/x86/pixel-a.asm common/x86/predict-a.asm \
          common/x86/quant-a.asm common/x86/sad-a.asm \
          common/x86/cpu-64.asm common/x86/dct-64.asm \
          common/x86/x86inc.asm common/x86/x86inc-64.asm
else
SRCS_NASM=common/x86/cabac-a.asm common/x86/dct-a.asm common/x86/deblock-a.asm \
          common/x86/mc-a.asm common/x86/mc-a2.asm \
          common/x86/pixel-a.asm common/x86/predict-a.asm \
          common/x86/quant-a.asm common/x86/sad-a.asm \
          common/x86/cpu-32.asm common/x86/dct-32.asm common/x86/pixel-32.asm \
          common/x86/x86inc.asm common/x86/x86inc-32.asm
endif
endif

ifeq ($(64BIT),yes)
    NASMFLAGS+=-DHAVE_MMX -DARCH_X86_64
else
    NASMFLAGS+=-DHAVE_MMX
endif

GCCLIBS=-lpthread

SLIB=../../../bin/ff_x264.dll
DEF=ff_x264.def

include ../../makefile_c.inc

.all: $(SLIB)

clean:
	rm -f *.o encoder/*.o common/*.o common/x86/*.o *~ $(SLIB) *.d encoder/*.d common/*.d common/x86/*.d

#
# include dependency files if they exist
#
-include $(SRCS:.cpp=.d) $(SRCS_C:.c=.d)
