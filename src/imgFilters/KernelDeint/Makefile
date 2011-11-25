### FILES and DIRECTORIES ###

SRCS=ff_kernelDeint.cpp
RESSRC=ff_kernelDeint.rc

ifeq ($(64BIT),yes)
    OBJ_DIR=../../../obj/MinGW64/kernelDeint/
    TARGET_LIB_DIR=../../../bin/
else
    OBJ_DIR=../../../obj/MinGW32/kernelDeint/
    TARGET_LIB_DIR=../../../bin/
endif

OBJ_DIRS=\
    $(OBJ_DIR)

TARGET_LIB=$(TARGET_LIB_DIR)ff_kernelDeint.dll

DEF=ff_kernelDeint.def


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
ifdef POSIX_COMPATIBLE_SHELL
	$(shell test -d $(@) || mkdir -p $(@))
else
	@IF NOT EXIST "$(@)" mkdir "$(@)"
endif


.PHONY: clean
clean:
ifdef POSIX_COMPATIBLE_SHELL
	rm -f $(TARGET_LIB)
	rm -r -f $(OBJ_DIR)
else
	IF EXIST "$(TARGET_LIB)" del "$(subst /,\,$(TARGET_LIB))"
	IF EXIST "$(OBJ_DIR)" rd /s /q "$(OBJ_DIR)"
endif
