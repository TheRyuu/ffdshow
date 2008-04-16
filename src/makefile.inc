all:.all

#if FFDSHOW_PREFIX is set, assume cross-compilation
ifeq ($(FFDSHOW_PREFIX),)
  NASM=yasm.exe
else
  NASM=yasm
endif


OUTPUTFLAG=-MMD -o\

CFLAGS+=-mno-cygwin -mdll -fno-rtti -mthreads -pipe -D_WINGDI_ -DUCLIBCPP -D_GLIBCPP_HAVE_MBSTATE_T -D_WIN32_IE=0x0500
CFLAGS+=-DARCH_IS_IA32 -DARCH_IS_32BIT
CFLAGS+=-DHAVE_MMX
CFLAGS+=-mmmx
ifeq ($(SSE),yes)
  CFLAGS+=-msse -mfpmath=sse
endif
ifeq ($(SSE2),yes)
  CFLAGS+=-msse2
endif

CFLAGS+=-w
CFLAGS+=-DNDEBUG -UDEBUG -DFFDEBUG=0
#CFLAGS+=-DDEBUG -D_DEBUG

OPTFLAGS=-O3 -march=i686 -fomit-frame-pointer


ifeq ($(UNICODE),yes)
  CFLAGS+=-DUNICODE -D_UNICODE
endif

CFLAGS+=-I. -I.. -Iuclibc++ -Ibaseclasses -I../baseclasses \
        -IimgFilters -I../imgFilters -Implayer -I../mplayer -Isettings -I../settings \
        -Isettings/filters -I../settings/filters -Icodecs -I../codecs \
        -Isubtitles -I../subtitles -Iconvert -I../convert -Idialog -I../dialog \
        -IaudioFilters -I../audioFilters -Icygwin -I../cygwin -Iffmpeg -I../ffmpeg \
        -Iacm -I../acm -Ifilters -I../filters -Imuxers -I../muxers -I/dx/Include -L/dx/MingLib -ldx9

OBJS=$(SRCS:.cpp=.o) $(SRCS_C:.c=.o) $(SRCS_NASM:.asm=.o) $(RESSRC:.rc:.res.o)

.c.o:
	$(FFDSHOW_PREFIX)$(CC) -c $(CFLAGS) $(OPTFLAGS) $(OUTPUTFLAG)$@ $<

.cpp.o:
	$(FFDSHOW_PREFIX)$(CC) -c $(CFLAGS) $(OPTFLAGS) $(OUTPUTFLAG)$@ $<

%.o: %.asm
	$(NASM) -f win32 -DWIN32 -DPREFIX $(NASMFLAGS) -I$(<D)/ -o $@ $<

%.res.o: %.rc
	$(FFDSHOW_PREFIX)windres $< -o $@ -D_WIN32_IE=0x0500 -DIDCLOSE=8

$(LIBNAME): $(OBJS)
	$(FFDSHOW_PREFIX)ar rcs $@ $^

.distclean: clean

PHONY distclean: .distclean

TEMPFILES=*.i *.ii *.d *.s *.o
