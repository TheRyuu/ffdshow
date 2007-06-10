CFLAGS+= -I. -I../.. -I../../xiph
 
SRCS_C=blockmap.c comment.c dct.c dct_decode.c \
     dct_encode.c decode.c encode.c frarray.c frinit.c \
     huffman.c idct.c mcomp.c misc_common.c pb.c pp.c \
     quant.c reconstruct.c scan.c toplevel.c encoder_toplevel.c \
     cpu.c dsp.c ../../xiph/ogg/bitwise.c ../../xiph/ogg/framing.c

SRCS=i386/recon_mmx.cpp i386/dsp_mmx.cpp i386/dsp_mmxext.cpp i386/fdct_mmx.cpp \
     i386/mmxIdct.cpp i386/loopf_asm.cpp i386/DeblockOpt.cpp i386/DeRingOpt.cpp

SLIB=../../../bin/ff_theora.dll
DEF=ff_theora.def

# enable SSE because compilation fails without it
SSE=yes

include ../../makefile_c.inc

.all: $(SLIB)

clean: 
	rm -f *.o *~ $(SLIB) i386/*.o *.d i386/*.d

#
# include dependency files if they exist
#
-include $(SRCS:.cpp=.d) $(SRCS_C:.c=.d)
