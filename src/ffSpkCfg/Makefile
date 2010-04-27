CFLAGS+=-I. -w

SRCS=ffSpkCfg.cpp

SLIB=../../bin/ffSpkCfg.dll
DEF=ffSpkCfg.def

GCCLIBS=-ldsound
CLLIBS=dsound.lib

include ../makefile_c.inc

.all: $(SLIB)

clean: 
	rm -f *.o $(SLIB) *~ *.d
