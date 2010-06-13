LAVC_DIR=libavcodec
LAVU_DIR=libavutil
LAVS_DIR=libswscale
LAVP_DIR=libpostproc
ZLIB_DIR=../zlib

CFLAGS+= -I. -I$(LAVC_DIR) -I$(LAVU_DIR) -I../imgFilters -I$(ZLIB_DIR) \
         -DHAVE_AV_CONFIG_H -D_ISOC99_SOURCE -D_POSIX_C_SOURCE=200112 -std=gnu99

#CFLAGS+= -save-temps
#CFLAGS+= -fverbose-asm

SRCS_C=\
      DllEntry.c \
      $(LAVC_DIR)/8bps.c \
      $(LAVC_DIR)/aac_ac3_parser.c \
      $(LAVC_DIR)/aactab.c \
      $(LAVC_DIR)/aandcttab.c \
      $(LAVC_DIR)/aasc.c \
      $(LAVC_DIR)/ac3.c \
      $(LAVC_DIR)/ac3_parser.c \
      $(LAVC_DIR)/ac3dec.c \
      $(LAVC_DIR)/ac3dec_data.c \
      $(LAVC_DIR)/ac3enc.c \
      $(LAVC_DIR)/ac3tab.c \
      $(LAVC_DIR)/adpcm.c \
      $(LAVC_DIR)/allcodecs.c \
      $(LAVC_DIR)/asv1.c \
      $(LAVC_DIR)/atrac.c \
      $(LAVC_DIR)/atrac3.c \
      $(LAVC_DIR)/audioconvert.c \
      $(LAVC_DIR)/avfft.c \
      $(LAVC_DIR)/avs.c \
      $(LAVC_DIR)/bitstream.c \
      $(LAVC_DIR)/cabac.c \
      $(LAVC_DIR)/cavs.c \
      $(LAVC_DIR)/cavsdec.c \
      $(LAVC_DIR)/cavsdsp.c \
      $(LAVC_DIR)/celp_filters.c \
      $(LAVC_DIR)/celp_math.c \
      $(LAVC_DIR)/cinepak.c \
      $(LAVC_DIR)/cook.c \
      $(LAVC_DIR)/corepng.c \
      $(LAVC_DIR)/cscd.c \
      $(LAVC_DIR)/cyuv.c \
      $(LAVC_DIR)/dca.c \
      $(LAVC_DIR)/dcadsp.c \
      $(LAVC_DIR)/dca_parser.c \
      $(LAVC_DIR)/dct.c \
      $(LAVC_DIR)/dsputil.c \
      $(LAVC_DIR)/dv.c \
      $(LAVC_DIR)/eac3dec.c \
      $(LAVC_DIR)/eac3dec_data.c \
      $(LAVC_DIR)/error_resilience.c \
      $(LAVC_DIR)/faandct.c \
      $(LAVC_DIR)/faanidct.c \
      $(LAVC_DIR)/ffmpegContext.c \
      $(LAVC_DIR)/fft.c \
      $(LAVC_DIR)/ffv1.c \
      $(LAVC_DIR)/flac.c \
      $(LAVC_DIR)/flacdata.c \
      $(LAVC_DIR)/flacdec.c \
      $(LAVC_DIR)/flvdec.c \
      $(LAVC_DIR)/flvenc.c \
      $(LAVC_DIR)/fraps.c \
      $(LAVC_DIR)/g726.c \
      $(LAVC_DIR)/golomb.c \
      $(LAVC_DIR)/gsm.c \
      $(LAVC_DIR)/h261.c \
      $(LAVC_DIR)/h261dec.c \
      $(LAVC_DIR)/h261enc.c \
      $(LAVC_DIR)/h263.c \
      $(LAVC_DIR)/h263_parser.c \
      $(LAVC_DIR)/h263dec.c \
      $(LAVC_DIR)/h264.c \
      $(LAVC_DIR)/h264_cabac.c \
      $(LAVC_DIR)/h264_cavlc.c \
      $(LAVC_DIR)/h264_direct.c \
      $(LAVC_DIR)/h264_loopfilter.c \
      $(LAVC_DIR)/h264_parser.c \
      $(LAVC_DIR)/h264_ps.c \
      $(LAVC_DIR)/h264_refs.c \
      $(LAVC_DIR)/h264_sei.c \
      $(LAVC_DIR)/h264dsp.c \
      $(LAVC_DIR)/h264idct.c \
      $(LAVC_DIR)/h264pred.c \
      $(LAVC_DIR)/huffman.c \
      $(LAVC_DIR)/huffyuv.c \
      $(LAVC_DIR)/imc.c \
      $(LAVC_DIR)/imgconvert.c \
      $(LAVC_DIR)/indeo2.c \
      $(LAVC_DIR)/indeo3.c \
      $(LAVC_DIR)/intelh263dec.c \
      $(LAVC_DIR)/intrax8.c \
      $(LAVC_DIR)/intrax8dsp.c \
      $(LAVC_DIR)/ituh263dec.c \
      $(LAVC_DIR)/ituh263enc.c \
      $(LAVC_DIR)/jfdctfst.c \
      $(LAVC_DIR)/jfdctint.c \
      $(LAVC_DIR)/jpegls.c \
      $(LAVC_DIR)/jpeglsdec.c \
      $(LAVC_DIR)/jrevdct.c \
      $(LAVC_DIR)/lcldec.c \
      $(LAVC_DIR)/libamr.c \
      $(LAVC_DIR)/ljpegenc.c \
      $(LAVC_DIR)/loco.c \
      $(LAVC_DIR)/mace.c \
      $(LAVC_DIR)/mdct.c \
      $(LAVC_DIR)/mjpeg.c \
      $(LAVC_DIR)/mjpeg_parser.c \
      $(LAVC_DIR)/mjpegbdec.c \
      $(LAVC_DIR)/mjpegdec.c \
      $(LAVC_DIR)/mjpegenc.c \
      $(LAVC_DIR)/mlp.c \
      $(LAVC_DIR)/mlp_parser.c \
      $(LAVC_DIR)/mlpdec.c \
      $(LAVC_DIR)/mlpdsp.c \
      $(LAVC_DIR)/motion_est.c \
      $(LAVC_DIR)/mpeg12.c \
      $(LAVC_DIR)/mpeg12data.c \
      $(LAVC_DIR)/mpeg12enc.c \
      $(LAVC_DIR)/mpeg4video.c \
      $(LAVC_DIR)/mpeg4video_parser.c \
      $(LAVC_DIR)/mpeg4videodec.c \
      $(LAVC_DIR)/mpeg4videoenc.c \
      $(LAVC_DIR)/mpegaudio.c \
      $(LAVC_DIR)/mpegaudiodata.c \
      $(LAVC_DIR)/mpegaudiodec.c \
      $(LAVC_DIR)/mpegaudiodecheader.c \
      $(LAVC_DIR)/mpegaudio_parser.c \
      $(LAVC_DIR)/mpegvideo.c \
      $(LAVC_DIR)/mpegvideo_enc.c \
      $(LAVC_DIR)/msmpeg4.c \
      $(LAVC_DIR)/msmpeg4data.c \
      $(LAVC_DIR)/msrle.c \
      $(LAVC_DIR)/msrledec.c \
      $(LAVC_DIR)/msvideo1.c \
      $(LAVC_DIR)/nellymoser.c \
      $(LAVC_DIR)/nellymoserdec.c \
      $(LAVC_DIR)/options.c \
      $(LAVC_DIR)/parser.c \
      $(LAVC_DIR)/pcm.c \
      $(LAVC_DIR)/png.c \
      $(LAVC_DIR)/pngdec.c \
      $(LAVC_DIR)/pngenc.c \
      $(LAVC_DIR)/qdm2.c \
      $(LAVC_DIR)/qpeg.c \
      $(LAVC_DIR)/qtrle.c \
      $(LAVC_DIR)/ra144.c \
      $(LAVC_DIR)/ra144dec.c \
      $(LAVC_DIR)/ra288.c \
      $(LAVC_DIR)/rangecoder.c \
      $(LAVC_DIR)/ratecontrol.c \
      $(LAVC_DIR)/rdft.c \
      $(LAVC_DIR)/rpza.c \
      $(LAVC_DIR)/rtjpeg.c \
      $(LAVC_DIR)/rv10.c \
      $(LAVC_DIR)/rv30.c \
      $(LAVC_DIR)/rv30dsp.c \
      $(LAVC_DIR)/rv34.c \
      $(LAVC_DIR)/rv40.c \
      $(LAVC_DIR)/rv40dsp.c \
      $(LAVC_DIR)/simple_idct.c \
      $(LAVC_DIR)/sp5xdec.c \
      $(LAVC_DIR)/svq1.c \
      $(LAVC_DIR)/svq1dec.c \
      $(LAVC_DIR)/svq3.c \
      $(LAVC_DIR)/synth_filter.c \
      $(LAVC_DIR)/truemotion1.c \
      $(LAVC_DIR)/truemotion2.c \
      $(LAVC_DIR)/truespeech.c \
      $(LAVC_DIR)/tscc.c \
      $(LAVC_DIR)/tta.c \
      $(LAVC_DIR)/ulti.c \
      $(LAVC_DIR)/utils.c \
      $(LAVC_DIR)/vc1.c \
      $(LAVC_DIR)/vc1data.c \
      $(LAVC_DIR)/vc1dec.c \
      $(LAVC_DIR)/vc1dsp.c \
      $(LAVC_DIR)/vcr1.c \
      $(LAVC_DIR)/vorbis.c \
      $(LAVC_DIR)/vorbis_data.c \
      $(LAVC_DIR)/vorbis_dec.c \
      $(LAVC_DIR)/vp3.c \
      $(LAVC_DIR)/vp3dsp.c \
      $(LAVC_DIR)/vp5.c \
      $(LAVC_DIR)/vp56.c \
      $(LAVC_DIR)/vp56data.c \
      $(LAVC_DIR)/vp56dsp.c \
      $(LAVC_DIR)/vp6.c \
      $(LAVC_DIR)/vp6dsp.c \
      $(LAVC_DIR)/w32thread.c \
      $(LAVC_DIR)/wavpack.c \
      $(LAVC_DIR)/wma.c \
      $(LAVC_DIR)/wmadec.c \
      $(LAVC_DIR)/wmv2.c \
      $(LAVC_DIR)/wmv2dec.c \
      $(LAVC_DIR)/wmv2enc.c \
      $(LAVC_DIR)/wnv1.c \
      $(LAVC_DIR)/xiph.c \
      $(LAVC_DIR)/xl.c \
      $(LAVC_DIR)/zmbv.c \
      $(LAVC_DIR)/amr_float/interf_dec.c \
      $(LAVC_DIR)/amr_float/sp_dec.c \
    \
      $(LAVU_DIR)/avstring.c \
      $(LAVU_DIR)/crc.c \
      $(LAVU_DIR)/eval.c \
      $(LAVU_DIR)/intfloat_readwrite.c \
      $(LAVU_DIR)/lfg.c \
      $(LAVU_DIR)/lls.c \
      $(LAVU_DIR)/log.c \
      $(LAVU_DIR)/lzo.c \
      $(LAVU_DIR)/mathematics.c \
      $(LAVU_DIR)/md5.c \
      $(LAVU_DIR)/mem.c \
      $(LAVU_DIR)/pixdesc.c \
      $(LAVU_DIR)/random_seed.c \
      $(LAVU_DIR)/rational.c \
    \
      $(ZLIB_DIR)/adler32.c \
      $(ZLIB_DIR)/compress.c \
      $(ZLIB_DIR)/crc32.c \
      $(ZLIB_DIR)/deflate.c \
      $(ZLIB_DIR)/gzclose.c \
      $(ZLIB_DIR)/gzlib.c \
      $(ZLIB_DIR)/gzread.c \
      $(ZLIB_DIR)/gzwrite.c \
      $(ZLIB_DIR)/infback.c \
      $(ZLIB_DIR)/inffast.c \
      $(ZLIB_DIR)/inflate.c \
      $(ZLIB_DIR)/inftrees.c \
      $(ZLIB_DIR)/trees.c \
      $(ZLIB_DIR)/uncompr.c \
      $(ZLIB_DIR)/zutil.c \
    \
      $(LAVC_DIR)/x86/cpuid.c \
      $(LAVC_DIR)/x86/cavsdsp_mmx.c \
      $(LAVC_DIR)/x86/dsputil_mmx.c \
      $(LAVC_DIR)/x86/dsputilenc_mmx.c \
      $(LAVC_DIR)/x86/fdct_mmx.c \
      $(LAVC_DIR)/x86/idct_mmx.c \
      $(LAVC_DIR)/x86/idct_mmx_xvid.c \
      $(LAVC_DIR)/x86/idct_sse2_xvid.c \
      $(LAVC_DIR)/x86/mlpdsp.c \
      $(LAVC_DIR)/x86/motion_est_mmx.c \
      $(LAVC_DIR)/x86/mpegvideo_mmx.c \
      $(LAVC_DIR)/x86/simple_idct_mmx.c \
      $(LAVC_DIR)/x86/vc1dsp_mmx.c \
      $(LAVC_DIR)/x86/vp3dsp_mmx.c \
      $(LAVC_DIR)/x86/vp3dsp_sse2.c \
      $(LAVC_DIR)/x86/vp6dsp_mmx.c \
      $(LAVC_DIR)/x86/vp6dsp_sse2.c \
   \
      $(LAVS_DIR)/rgb2rgb.c \
      $(LAVS_DIR)/swscale.c \
      $(LAVS_DIR)/swutils.c \
      $(LAVS_DIR)/yuv2rgb.c \
      $(LAVS_DIR)/sww32thread.c \
   \
      $(LAVS_DIR)/x86/yuv2rgb_mmx.c \
   \
      $(LAVP_DIR)/postprocess.c
      

ifneq ($(64BIT),yes)
SRCS_C+=\
      $(LAVC_DIR)/x86/fft.c \
      $(LAVC_DIR)/x86/fft_3dn.c \
      $(LAVC_DIR)/x86/fft_3dn2.c \
      $(LAVC_DIR)/x86/fft_sse.c
endif

ifneq ($(64BIT),yes)
SRCS_YASM=\
      $(LAVC_DIR)/x86/dsputil_yasm.asm \
      $(LAVC_DIR)/x86/fft_mmx.asm \
      $(LAVC_DIR)/x86/h264_deblock_sse2.asm \
      $(LAVC_DIR)/x86/h264_idct_sse2.asm \
      $(LAVC_DIR)/x86/x86util.asm
endif

ifeq ($(64BIT),yes)
    CFLAGS+=-DARCH_X86_64
else
    CFLAGS+=-DARCH_X86_32
endif

SLIB=../../bin/libavcodec.dll
DEF=libavcodec.def

include ../makefile_c.inc

.all: $(SLIB)

clean:
	rm -f *.o *.d $(LAVC_DIR)/*.o $(LAVC_DIR)/*.d $(LAVC_DIR)/x86/*.o $(LAVC_DIR)/x86/*.d \
	$(LAVC_DIR)/amr_float/*.o $(LAVC_DIR)/amr_float/*.d $(LAVU_DIR)/*.o $(LAVU_DIR)/*.d \
	$(LAVS_DIR)/*.o $(LAVS_DIR)/*.d $(LAVS_DIR)/x86/*.o $(LAVS_DIR)/x86/*.d \
	$(LAVP_DIR)/*.o $(LAVP_DIR)/*.d $(ZLIB_DIR)/*.o $(ZLIB_DIR)/*.d $(SLIB)
