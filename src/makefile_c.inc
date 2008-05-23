all:.all

#if FFDSHOW_PREFIX is set, assume cross-compilation
ifeq ($(FFDSHOW_PREFIX),)
  NASM=yasm.exe
else
  NASM=yasm
endif

ifeq ($(64BIT),yes)
	TARGET_OS=i686-pc-mingw64
else
	TARGET_OS=i686-pc-mingw32
endif

OUTPUTFLAG=-MMD -o\

CFLAGS+=-mno-cygwin -mdll -mthreads -pipe
CFLAGS+=-DNDEBUG -UDEBUG

ifeq ($(64BIT),yes)
	CFLAGS+=-DWIN64 -D_WIN64
else
	CFLAGS+=-DWIN32 -D_WIN32
endif

CFLAGS+=-mmmx
ifeq ($(SSE),yes)
  CFLAGS+=-msse -mfpmath=sse
endif
ifeq ($(SSE2),yes)
  CFLAGS+=-msse2
endif

OPTFLAGS=-O3 -fomit-frame-pointer
ifeq ($(64BIT),yes)
	OPTFLAGS+=-m64
else
	OPTFLAGS+=-march=i686
endif

CPPFLAGS+=-fno-rtti

GCCDEF=$(DEF)


ifeq ($(UNICODE),yes)
  CFLAGS+=-DUNICODE -D_UNICODE
endif

OBJS+=$(SRCS:.cpp=.o) $(SRCS_C:.c=.o) $(SRCS_NASM:.asm=.o) $(RESSRC:.rc=.res.o)

ifeq ($(FORCECPP),1)
  CFLAGS+=-x c++
endif

ifeq ($(EXCEPTIONS),1)
  CPPFLAGS+=-fexceptions
else
  CPPFLAGS+=-fno-exceptions
endif

.c.o:
	$(FFDSHOW_PREFIX)$(CC) -c $(CFLAGS) $(OPTFLAGS) $(OUTPUTFLAG)$@ $<

.cpp.o:
	$(FFDSHOW_PREFIX)$(CC) -c $(CFLAGS) $(CPPFLAGS) $(OPTFLAGS) $(OUTPUTFLAG)$@ $<

%.o: %.asm
ifeq ($(64BIT),yes)
	$(NASM) -f win64 -DWIN64 -DPREFIX $(NASMFLAGS) -I$(<D)/ -o $@  $<
else
	$(NASM) -f win32 -DWIN32 -DPREFIX $(NASMFLAGS) -I$(<D)/ -o $@  $<
endif

%.res.o: %.rc
	$(FFDSHOW_PREFIX)windres $< -o $@ -D_WIN32_IE=0x0500 -DIDCLOSE=8

$(SLIB): $(OBJS) $(DEF)
	$(FFDSHOW_PREFIX)dllwrap -mno-cygwin --target=$(TARGET_OS) \
             -Wl,--enable-stdcall-fixup,--strip-all \
             --def $(GCCDEF) $(GCCDLLENTRY) -o $@ $(OBJS) $(GCCLIBS)

$(SEXE): $(OBJS)
	$(FFDSHOW_PREFIX)$(CC) -mno-cygwin --target=$(TARGET_OS) \
             -Wl,--enable-stdcall-fixup -o $@ $(OBJS) $(GCCLIBS)
	$(FFDSHOW_PREFIX)strip $@

distclean: clean

TEMPFILES=*.i *.ii *.d *.s *.o

-include $(SRCS:.cpp=.d) $(SRCS_C:.c=.d)
