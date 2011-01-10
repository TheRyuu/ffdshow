CFLAGS+= -I.
       
SRCS=TomsMoComp.cpp

SLIB=../../../bin/TomsMoComp_ff.dll
DEF=TomsMoComp_ff.def

GCCLIBS=-lstdc++

# enable SSE because compilation fails without it
OPTFLAGS+=-msse -mfpmath=sse

include ../../makefile_c.inc

.all: $(SLIB)

clean: 
	rm -f *.o *~ $(SLIB) *.d
