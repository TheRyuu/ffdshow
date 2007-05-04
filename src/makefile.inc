all:.all

#if FFDSHOW_PREFIX is set, assume cross-compilation
ifeq ($(FFDSHOW_PREFIX),)
  NASM=yasm.exe
else
  NASM=yasm
endif

ifeq ($(CC),icl)
  OUTPUTFLAG=/Fo
  CFLAGS+=/nologo /MD /LD
  CFLAGS+=/D "WIN64" /D "_WIN64" /D "_WINDOWS" /D "_USERDLL" /D "NDEBUG" -DUCLIBCPP -D_WIN32_IE=0x0500
  OPTFLAGS=/O3 /G7 /GF /Oa /Zc:forScope /Qip /Zc:wchar_t
else
ifeq ($(CC),cl)
  OUTPUTFLAG=/Fo

  CFLAGS+=/nologo /MD /LD
  CFLAGS+=/D "WIN32" /D "_WIN32" /D "_WINDOWS" /D "_USERDLL" /D "NDEBUG" /D "_WIN32_IE=0x0500"
  OPTFLAGS=/Ox /G7 /GF /Zc:wchar_t
  ifeq ($(SSE),yes)
    OPTFLAGS+=/arch:SSE
  endif
else
  OUTPUTFLAG=-MMD -o\

  CFLAGS+=-mno-cygwin -mdll -fno-rtti -mthreads -pipe -D_WINGDI_ -DUCLIBCPP -D_GLIBCPP_HAVE_MBSTATE_T -D_WIN32_IE=0x0500
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

  OPTFLAGS=-O2 -march=pentium-mmx -mtune=i686 -fomit-frame-pointer -finline-functions -finline -frename-registers -fweb -funit-at-a-time
endif
endif

ifeq ($(UNICODE),yes)
  CFLAGS+=-DUNICODE -D_UNICODE
endif

CFLAGS+=-I. -I.. -Iuclibc++ -Ibaseclasses -I../baseclasses \
        -IimgFilters -I../imgFilters -Implayer -I../mplayer -Isettings -I../settings \
        -Isettings/filters -I../settings/filters -Icodecs -I../codecs \
        -Isubtitles -I../subtitles -Iconvert -I../convert -Idialog -I../dialog \
        -IaudioFilters -I../audioFilters -Icygwin -I../cygwin -Iffmpeg -I../ffmpeg \
        -Iacm -I../acm -Ixiph -I../xiph -Ifilters -I../filters -Imuxers -I../muxers -I/dx/Include -L/dx/MingLib -ldx9

OBJS=$(SRCS:.cpp=.o) $(SRCS_C:.c=.o) $(SRCS_NASM:.asm=.o) $(RESSRC:.rc:.res.o)

.c.o:
	$(FFDSHOW_PREFIX)$(CC) -c $(CFLAGS) $(OPTFLAGS) $(OUTPUTFLAG)$@ $<

.cpp.o:
	$(FFDSHOW_PREFIX)$(CC) -c $(CFLAGS) $(OPTFLAGS) $(OUTPUTFLAG)$@ $<

%.o: %.asm
	$(NASM) -f win32 -DWIN32 -DPREFIX $(NASMFLAGS) -I$(<D)/ -o $@ $<

ifeq ($(CC),icl)
%.res.o: %.rc
	rc /Fo$@ $<
else
ifeq ($(CC),cl)
%.res.o: %.rc
	rc /Fo$@ $<
else
%.res.o: %.rc
	$(FFDSHOW_PREFIX)windres $< -o $@ -D_WIN32_IE=0x0500 -DIDCLOSE=8
endif
endif

ifeq ($(CC),icl)
$(LIBNAME): $(OBJS)
	lib $(OBJS) /OUT:$(LIBNAME)
else
$(LIBNAME): $(OBJS)
	$(FFDSHOW_PREFIX)ar rcs $@ $^
endif

.distclean: clean

PHONY distclean: .distclean

TEMPFILES=*.i *.ii *.d *.s *.o
