CFLAGS+= -I. -I.. -I../.. -I../../xiph -Idec -Ienc -DUSE_ASM

SRCS_C=\
  blockmap.c \
  comment.c \
  dct.c \
  dct_decode.c \
	dct_encode.c \
	decode.c \
	encode.c \
	frarray.c \
	frinit.c \
	huffman.c \
	idct.c \
	mcomp.c \
	misc_common.c \
	pb.c \
	pp.c \
	quant.c \
	reconstruct.c \
	scan.c \
	toplevel.c \
	encoder_toplevel.c \
	cpu.c \
	dsp.c \
	../../xiph/ogg/bitwise.c \
	../../xiph/ogg/framing.c
	
ifeq ($(64BIT),yes)
SRCS_C+=\
	enc/x86_64/recon_mmx.c \
	enc/x86_64/dsp_mmx.c \
	enc/x86_64/dsp_mmxext.c \
	enc/x86_64/fdct_mmx.c \
	enc/x86_64/idct_mmx.c \
	enc/x86_64/dct_decode_mmx.c
else
SRCS_C+=\
	enc/x86_32/recon_mmx.c \
	enc/x86_32/dsp_mmx.c \
	enc/x86_32/dsp_mmxext.c \
	enc/x86_32/fdct_mmx.c \
	enc/x86_32/idct_mmx.c \
	enc/x86_32/dct_decode_mmx.c
endif


SLIB=../../../bin/ff_theora.dll

DEF=ff_theora.def

include ../../makefile_c.inc

.all: $(SLIB)

clean: 
	rm -f $(SLIB) *.o *.d enc/x86_32/*.o enc/x86_32/*.d enc/x86_64/*.o *.d enc/x86_64/*.d ../../xiph/ogg/*.o ../../xiph/ogg/*.d

#
# include dependency files if they exist
#
-include $(SRCS:.cpp=.d) $(SRCS_C:.c=.d)
