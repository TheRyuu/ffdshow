CFLAGS+= -I. -I../.. -I../../xiph/ogg -I../../xiph/vorbis -I../../xiph -D__i386__ -Dinline=__inline -U_LOW_ACCURACY_
       
SRCS_C=block.c codebook.c floor0.c floor1.c info.c mapping0.c mdct.c registry.c \
       res012.c sharedbook.c synthesis.c window.c \
       ../../xiph/ogg/bitwise.c ../../xiph/ogg/framing.c

SLIB=../../../bin/ff_tremor.dll
DEF=ff_tremor.def

include ../../makefile_c.inc

.all: $(SLIB)

clean: 
	rm -f *.o *~ $(SLIB) *.d

#
# include dependency files if they exist
#
-include $(SRCS:.cpp=.d) $(SRCS_C:.c=.d)
