CFLAGS+= -I.
       
SRCS=ff_kernelDeint.cpp

SLIB=../../../bin/ff_kernelDeint.dll
DEF=ff_kernelDeint.def

GCCLIBS=-lstdc++

# enable SSE because compilation fails without it
OPTFLAGS+=-msse -mfpmath=sse

include ../../makefile_c.inc

.all: $(SLIB)

clean: 
	rm -f *.o *~ $(SLIB) *.d
