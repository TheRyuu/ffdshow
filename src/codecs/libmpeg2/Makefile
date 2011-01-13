### FILES and DIRECTORIES ###

SRCS_C=\
    libmpeg2/alloc.c \
		libmpeg2/decode.c \
		libmpeg2/header.c \
		libmpeg2/idct.c \
		libmpeg2/idct_mmx.c \
		libmpeg2/idct_sse2.c \
		libmpeg2/motion_comp.c \
		libmpeg2/motion_comp_mmx.c \
		libmpeg2/mc_sse2.c \
		libmpeg2/slice.c \
		libmpeg2/cpu_accel.c \
		version.c

ifeq ($(64BIT),yes)
    OBJ_DIR=../../../obj/MinGW64/libmpeg2/
    TARGET_LIB_DIR=../../../bin/
else
    OBJ_DIR=../../../obj/MinGW32/libmpeg2/
    TARGET_LIB_DIR=../../../bin/
endif

OBJ_DIRS=\
    $(OBJ_DIR) \
    $(OBJ_DIR)/libmpeg2

TARGET_LIB=$(TARGET_LIB_DIR)libmpeg2_ff.dll

DEF=libmpeg2_ff.def


### SETTINGS ###

CFLAGS+= -I. -Iinclude
FORCECPP=1

#CFLAGS+=-fverbose-asm -save-temps

# enable SSE because compilation fails without it
OPTFLAGS+=-msse -mfpmath=sse


### INCLUDES ###

include ../../makefile_c.inc


### TARGETS ###

.all: outdirs $(TARGET_LIB)

.PHONY: outdirs $(OBJ_DIRS)
outdirs: $(OBJ_DIRS)
$(OBJ_DIRS):
	$(shell test -d $(@) || mkdir -p $(@))


.PHONY: clean
clean:
	rm -f $(TARGET_LIB)
	rm -r -f $(OBJ_DIR)
