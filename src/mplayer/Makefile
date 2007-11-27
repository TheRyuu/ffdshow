CFLAGS+=-I. -I.. -I../imgFilters -Ilibvo -w

SRCS_C=postproc/rgb2rgb.c postproc/yuv2rgb.c postproc/swscale.c mplayer.c \
       postproc/postprocess.c libvo/aclib.c  postproc/w32thread.c \
       mp3lib/sr1.c ../ffmpeg/libavutil/mem.c

ifneq ($(CC),icl)
SRCS_C+= mp3lib/decode_MMX.c mp3lib/dct64_MMX.c \
        mp3lib/tabinit_MMX.c mp3lib/dct64_3dnow.c mp3lib/dct64_sse.c \
        mp3lib/dct64_k7.c mp3lib/dct36_3dnow.c mp3lib/dct36_k7.c
CFLAGS+=-ffast-math
endif

ifeq ($(FFDEBUG),yes)
SRCS_C+= ../ffdebug.c 
CFLAGS+= -DFFDEBUG=1
else
CFLAGS+= -DFFDEBUG=0
endif

SLIB=../../bin/libmplayer.dll
DEF=libmplayer.def

NOINTRIN=1

include ../makefile_c.inc

.all: $(SLIB)

clean: 
	rm -f *.[od] postproc/*.o libvo/*.o mp3lib/*.o $(SLIB) *~ *.d postproc/*.d libvo/*.d mp3lib/*.d

#
# include dependency files if they exist
#
-include $(SRCS:.cpp=.d) $(SRCS_C:.c=.d)
