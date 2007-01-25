CFLAGS+= -I. -I../.. -DHAVE_PTHREAD -D__WIN32__ -D__X264__

ifneq ($(CC),icl)
CFLAGS+=-DARCH_X86 -DHAVE_MMXEXT -DHAVE_SSE2 
endif
       
SRCS_C=common/mc.c common/predict.c common/pixel.c common/macroblock.c \
       common/frame.c common/dct.c common/cpu.c common/cabac.c \
       common/common.c common/csp.c common/set.c common/quant.c \
       encoder/analyse.c encoder/me.c encoder/ratecontrol.c \
       encoder/set.c encoder/macroblock.c encoder/cabac.c encoder/cavlc.c \
       encoder/encoder.c encoder/eval.c \
       ff_x264.c

ifneq ($(CC),icl)
SRCS_C+=common/i386/mc-c.c common/i386/predict-c.c

SRCS_NASM=common/i386/dct-a.asm common/i386/pixel-a.asm \
          common/i386/mc-a.asm common/i386/mc-a2.asm common/i386/predict-a.asm \
          common/i386/pixel-sse2.asm common/i386/quant-a.asm \
          common/i386/deblock-a.asm
endif

SLIB=../../../bin/ff_x264.dll
DEF=ff_x264.def

include ../../makefile_c.inc

.all: $(SLIB)

clean: 
	rm -f *.o encoder/*.o common/*.o common/i386/*.o *~ $(SLIB)

#
# include dependency files if they exist
#
-include $(SRCS:.cpp=.d) $(SRCS_C:.c=.d)
