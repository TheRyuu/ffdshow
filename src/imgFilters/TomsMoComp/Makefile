### FILES and DIRECTORIES ###

SRCS=TomsMoComp.cpp

ifeq ($(64BIT),yes)
    OBJ_DIR=../../../obj/MinGW64/TomsMoComp/
    TARGET_LIB_DIR=../../../bin/
else
    OBJ_DIR=../../../obj/MinGW32/TomsMoComp/
    TARGET_LIB_DIR=../../../bin/
endif

OBJ_DIRS=\
    $(OBJ_DIR)

TARGET_LIB=$(TARGET_LIB_DIR)TomsMoComp_ff.dll

DEF=TomsMoComp_ff.def


### SETTINGS ###

CFLAGS+= -I.

GCCLIBS=-lstdc++

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
