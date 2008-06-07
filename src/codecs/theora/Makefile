CFLAGS+= -I. -I.. -I../.. -DUSE_ASM

ifeq ($(64BIT),yes)
	CFLAGS+=-Ix86_64
else
	CFLAGS+=-Ix86_32
endif

ifeq ($(64BIT),yes)
SRCS_C=blockmap.c comment.c dct.c dct_decode.c \
	dct_encode.c decode.c encode.c frarray.c frinit.c \
	huffman.c idct.c mcomp.c misc_common.c pb.c pp.c \
	quant.c reconstruct.c scan.c toplevel.c encoder_toplevel.c \
	cpu.c dsp.c bitwise.c framing.c \
	x86_64/recon_mmx.c x86_64/dsp_mmx.c x86_64/dsp_mmxext.c x86_64/fdct_mmx.c \
	x86_64/idct_mmx.c x86_64/dct_decode_mmx.c
else
SRCS_C=blockmap.c comment.c dct.c dct_decode.c \
	dct_encode.c decode.c encode.c frarray.c frinit.c \
	huffman.c idct.c mcomp.c misc_common.c pb.c pp.c \
	quant.c reconstruct.c scan.c toplevel.c encoder_toplevel.c \
	cpu.c dsp.c bitwise.c framing.c \
	x86_32/recon_mmx.c x86_32/dsp_mmx.c x86_32/dsp_mmxext.c x86_32/fdct_mmx.c \
	x86_32/idct_mmx.c x86_32/dct_decode_mmx.c
endif

SLIB=../../../bin/ff_theora.dll
DEF=ff_theora.def

include ../../makefile_c.inc

.all: $(SLIB)

clean: 
	rm -f *.o *~ $(SLIB) x86_32*.o *.d x86_32*.d x86_64*.o *.d x86_64*.d

#
# include dependency files if they exist
#
-include $(SRCS:.cpp=.d) $(SRCS_C:.c=.d)
