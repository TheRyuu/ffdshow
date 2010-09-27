CFLAGS+=-I. -Imp3lib -I../ffmpeg -std=gnu99

SRCS_C=\
		mplayer.c \
		../ffmpeg/libavutil/mem.c \
		mp3lib/sr1.c
		
ifneq ($(64BIT),yes)
SRCS_C+=\
		mp3lib/decode_MMX.c \
		mp3lib/dct64_MMX.c \
		mp3lib/tabinit_MMX.c \
		mp3lib/dct64_3dnow.c \
		mp3lib/dct64_sse.c \
		mp3lib/dct64_k7.c \
		mp3lib/dct36_3dnow.c \
		mp3lib/dct36_k7.c
endif

ifeq ($(64BIT),yes)
    CFLAGS+=-DARCH_X86_64
else
    CFLAGS+=-DARCH_X86_32
endif

SLIB=../../bin/libmplayer.dll
DEF=libmplayer.def

include ../makefile_c.inc

.all: $(SLIB)

clean:
	rm -f *.o *.d mp3lib/*.o mp3lib/*.d ../ffmpeg/libavutil/mem.o ../ffmpeg/libavutil/mem.d $(SLIB)
