all:.all
distclean: clean

### SETTINGS ###

#DEBUG=yes

ifeq ($(64BIT),yes)
    FFDSHOW_PREFIX=x86_64-w64-mingw32-
    TARGET_OS=x86_64-w64-mingw32
    CFLAGS+=-DWIN64 -D_WIN64 -D__MINGW64__
    #CFLAGS+=-fno-leading-underscore
    #LDFLAGS+=-fno-leading-underscore
    OPTFLAGS+=-O2 -m64
    YASMFLAGS+=-f win64 -m amd64 -DWIN64 -DARCH_X86_64 -DPREFIX
else
    TARGET_OS=i686-pc-mingw32
    CFLAGS+=-DWIN32 -D_WIN32 -D__MINGW32__
    OPTFLAGS+=-O2 -march=i686 -mmmx
    YASMFLAGS+=-f win32 -m x86 -DWIN32 -DARCH_X86_32 -DPREFIX
endif

CFLAGS+=-mno-cygwin -mdll -mthreads -pipe
OPTFLAGS+=-fno-common -fno-tree-vectorize
CPPFLAGS+=-fno-rtti
OUTPUTFLAG=-MMD -o\

ifeq ($(DEBUG),yes)
    CFLAGS+=-DDEBUG -D_DEBUG -g
    YASMFLAGS+=-g cv8
else
    CFLAGS+=-DNDEBUG -UDEBUG
    OPTFLAGS+=-fomit-frame-pointer
    LDFLAGS+=--strip-all
endif

ifeq ($(EXCEPTIONS),1)
    CPPFLAGS+=-fexceptions
else
    CPPFLAGS+=-fno-exceptions
endif


### COMMANDS ###

OBJS+=$(SRCS:%.cpp=$(OBJ_DIR)%.o) $(SRCS_C:%.c=$(OBJ_DIR)%.o) $(SRCS_YASM:%.asm=$(OBJ_DIR)%.o) $(RESSRC:%.rc=$(OBJ_DIR)%.res.o)

$(OBJ_DIR)%.o: %.c
	@echo $<
	@$(FFDSHOW_PREFIX)$(CC) -c $(CFLAGS) $(OPTFLAGS) $(OUTPUTFLAG)$@ $<

$(OBJ_DIR)%.o: %.cpp
	@echo $<
	@$(FFDSHOW_PREFIX)$(CC) -c $(CFLAGS) $(CPPFLAGS) $(OPTFLAGS) $(OUTPUTFLAG)$@ $<

$(OBJ_DIR)%.o: %.asm
	@echo $<
	@yasm $(YASMFLAGS) -I$(<D)/ -o $@ $<

$(OBJ_DIR)%.res.o: %.rc
	@echo $<
  @$(FFDSHOW_PREFIX)windres $< -o $@ -D_WIN32_IE=0x0500 -DIDCLOSE=8

$(TARGET_LIB): $(OBJS) $(DEF)
	@echo $@
	@$(FFDSHOW_PREFIX)dllwrap -mno-cygwin --target=$(TARGET_OS) \
             -Wl,--enable-stdcall-fixup $(LDFLAGS) \
             --def $(DEF) $(GCCDLLENTRY) -o $@ $(OBJS) $(GCCLIBS)

$(TARGET_EXE): $(OBJS)
	@echo $@
	@$(FFDSHOW_PREFIX)$(CC) -mno-cygwin --target=$(TARGET_OS) \
             -Wl,--enable-stdcall-fixup -o $@ $(OBJS) $(GCCLIBS)
	@$(FFDSHOW_PREFIX)strip $@

-include $(SRCS:.cpp=.d) $(SRCS_C:.c=.d)
