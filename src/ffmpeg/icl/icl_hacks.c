#include "libavcodec/avcodec.h"
#include "libavcodec/ac3enc.h"
#include "libavcodec/cavsdsp.h"
#include "libavcodec/dcadsp.h"
#include "libavcodec/fft.h"
#include "libavcodec/fmtconvert.h"
#include "libavcodec/h264.h"
#include "libavcodec/mpegaudiodsp.h"
#include "libavcodec/rv34dsp.h"
#include "libavcodec/synth_filter.h"
#include "libavcodec/sbrdsp.h"
#include "libavcodec/vc1.h"
#include "libavcodec/vp8.h"
#include "libavcodec/vp56dsp.h"
#include "libswscale/swscale.h"
#include "libswscale/swscale_internal.h"

//
// ICL debug build compiles if (0) {} blocks which GCC does not compile.
// It results in a lot of linker errors.
// This file does the work around.
//

// ARCH__xxx
void ff_dsputil_init_alpha(DSPContext* c, AVCodecContext *avctx) {}
void ff_dsputil_init_arm(DSPContext* c, AVCodecContext *avctx) {}
void ff_dsputil_init_bfin(DSPContext* c, AVCodecContext *avctx) {}
void ff_dsputil_init_mmi(DSPContext* c, AVCodecContext *avctx) {}
void ff_dsputil_init_mmx(DSPContext* c, AVCodecContext *avctx) {}
void ff_dsputil_init_ppc(DSPContext* c, AVCodecContext *avctx) {}
void ff_dsputil_init_sh4(DSPContext* c, AVCodecContext *avctx) {}
void ff_dsputil_init_vis(DSPContext* c, AVCodecContext *avctx) {}
void ff_ac3dsp_init_arm(AC3DSPContext *c, int bit_exact) {}
void ff_cavsdsp_init_mmx(CAVSDSPContext* c, AVCodecContext *avctx) {}
void ff_dcadsp_init_arm(DCADSPContext *s) {}
void ff_fft_permute_sse(FFTContext *s, FFTComplex *z) {}
void ff_fft_calc_avx(FFTContext *s, FFTComplex *z) {}
void ff_fft_calc_sse(FFTContext *s, FFTComplex *z) {}
void ff_fft_calc_3dn(FFTContext *s, FFTComplex *z) {}
void ff_fft_calc_3dn2(FFTContext *s, FFTComplex *z) {}
void ff_imdct_calc_3dn(FFTContext *s, FFTSample *output, const FFTSample *input) {}
void ff_imdct_half_3dn(FFTContext *s, FFTSample *output, const FFTSample *input) {}
void ff_imdct_calc_3dn2(FFTContext *s, FFTSample *output, const FFTSample *input) {}
void ff_imdct_half_3dn2(FFTContext *s, FFTSample *output, const FFTSample *input) {}
void ff_imdct_calc_sse(FFTContext *s, FFTSample *output, const FFTSample *input) {}
void ff_imdct_half_avx(FFTContext *s, FFTSample *output, const FFTSample *input) {}
void ff_dct32_float_avx(FFTSample *out, const FFTSample *in) {}
void ff_fft_fixed_init_arm(FFTContext *s) {}
void ff_fft_init_altivec(FFTContext *s) {}
void ff_fft_init_arm(FFTContext *s) {}
void ff_fmt_convert_init_arm(FmtConvertContext *c, AVCodecContext *avctx) {}
void ff_fmt_convert_init_altivec(FmtConvertContext *c, AVCodecContext *avctx) {}
void ff_h264dsp_init_arm(H264DSPContext *c, const int bit_depth, const int chroma_format_idc) {}
void ff_h264dsp_init_ppc(H264DSPContext *c, const int bit_depth, const int chroma_format_idc) {}
void ff_h264dsp_init_x86(H264DSPContext *c, const int bit_depth, const int chroma_format_idc) {}
void ff_h264_pred_init_arm(H264PredContext *h, int codec_id, const int bit_depth, const int chroma_format_idc) {}
void ff_mlp_init_x86(DSPContext* c, AVCodecContext *avctx) {}
void ff_mpadsp_init_arm(MPADSPContext *s) {}
void ff_mpadsp_init_altivec(MPADSPContext *s) {}
void ff_rdft_init_arm(RDFTContext *s) {}
void ff_rv34dsp_init_neon(RV34DSPContext *c, DSPContext *dsp) {}
void ff_rv40dsp_init_neon(RV34DSPContext *c, DSPContext *dsp) {}
void ff_synth_filter_init_arm(SynthFilterContext *c) {}
void ff_sbrdsp_init_arm(SBRDSPContext *s) {}
void ff_vc1dsp_init_altivec(VC1DSPContext* c) {}
void ff_vc1dsp_init_mmx(VC1DSPContext* dsp) {}
void ff_vp56dsp_init_arm(VP56DSPContext *s, enum CodecID codec) {}
void sws_init_swScale_MMX(struct SwsContext *c) {}
void ff_vp8dsp_init_altivec(VP8DSPContext *c) {}
void ff_vp8dsp_init_arm(VP8DSPContext *c) {}
void ff_put_signed_pixels_clamped_mmx(const DCTELEM *block, uint8_t *pixels, int line_size) {}
void ff_add_pixels_clamped_mmx(const DCTELEM *block, uint8_t *pixels, int line_size) {}
void ff_yadif_filter_line_mmx(uint8_t *dst,
                              uint8_t *prev, uint8_t *cur, uint8_t *next,
                              int w, int prefs, int mrefs, int parity, int mode) {}
void ff_yadif_filter_line_sse2(uint8_t *dst,
                               uint8_t *prev, uint8_t *cur, uint8_t *next,
                               int w, int prefs, int mrefs, int parity, int mode) {}
void ff_yadif_filter_line_ssse3(uint8_t *dst,
                                uint8_t *prev, uint8_t *cur, uint8_t *next,
                                int w, int prefs, int mrefs, int parity, int mode) {}
int ff_get_cpu_flags_arm(void) {}
int ff_get_cpu_flags_ppc(void) {}
int ff_get_cpu_flags_x86(void) {}
void rgb2rgb_init_x86(void) {}
void ff_sws_init_swScale_altivec(struct SwsContext *c) {}
void ff_bfin_get_unscaled_swscale(struct SwsContext *c) {}
void ff_swscale_get_unscaled_altivec(struct SwsContext *c) {}
SwsFunc ff_yuv2rgb_init_mmx(struct SwsContext *c) {}
SwsFunc ff_yuv2rgb_init_vis(struct SwsContext *c) {}
SwsFunc ff_yuv2rgb_init_altivec(struct SwsContext *c) {}
SwsFunc ff_yuv2rgb_get_func_ptr_bfin(SwsContext *c) {}
void ff_yuv2rgb_init_tables_altivec(SwsContext *c, const int inv_table[4],
                                    int brightness, int contrast, int saturation) {}
void ff_mpadsp_init_mmx(MPADSPContext *s) {}

// CONFIG_VC1_VDPAU_DECODER
void ff_vdpau_add_data_chunk(MpegEncContext *s, const uint8_t *buf, int buf_size) {}
void ff_vdpau_mpeg_picture_complete(MpegEncContext *s, const uint8_t *buf, int buf_size, int slice_count) {}
void ff_vdpau_h264_picture_start(MpegEncContext *s) {}
void ff_vdpau_h264_set_reference_frames(MpegEncContext *s) {}
void ff_vdpau_h264_picture_complete(MpegEncContext *s) {}
void ff_vdpau_vc1_decode_picture(MpegEncContext *s, const uint8_t *buf, int buf_size) {}
void ff_vdpau_mpeg4_decode_picture(MpegEncContext *s, const uint8_t *buf, int buf_size) {}

// CONFIG_EAC3_ENCODER || CONFIG_EAC3_ENCODER
void ff_eac3_get_frame_exp_strategy(AC3EncodeContext *s) {}
void ff_eac3_exponent_init(void) {}
void ff_ac3_float_mdct_end(AC3EncodeContext *s) {}
int ff_ac3_float_mdct_init(AC3EncodeContext *s) {return 0;}
int ff_ac3_float_allocate_sample_buffers(AC3EncodeContext *s) {return 0;}
void ff_eac3_set_cpl_states(AC3EncodeContext *s) {}
void ff_eac3_output_frame_header(AC3EncodeContext *s) {}

// CONFIG_H261_ENCODER
int ff_h261_get_picture_format(int width, int height) {return 0;}
void ff_h261_encode_init(MpegEncContext *s) {}
void ff_h261_encode_mb(MpegEncContext *s,
                       DCTELEM block[6][64],
                       int motion_x, int motion_y) {}
void ff_h261_encode_picture_header(MpegEncContext * s, int picture_number) {}
void ff_h261_reorder_mb_index(MpegEncContext* s) {}

// CONFIG_H263_ENCODER
void ff_h263_encode_init(MpegEncContext *s) {}
void ff_h263_encode_mb(MpegEncContext *s,
                       DCTELEM block[6][64],
                       int motion_x, int motion_y) {}
void ff_h263_encode_picture_header(MpegEncContext *s, int picture_number) {}
void ff_h263_encode_gob_header(MpegEncContext * s, int mb_line) {}
void ff_h263_encode_mba(MpegEncContext *s) {}
void ff_clean_h263_qscales(MpegEncContext *s) {}

// CONFIG_MSMPEG4_ENCODER
void ff_msmpeg4_encode_init(MpegEncContext *s) {}
void ff_msmpeg4_encode_ext_header(MpegEncContext * s) {}
void ff_msmpeg4_encode_mb(MpegEncContext * s,
                          DCTELEM block[6][64],
                          int motion_x, int motion_y) {}
int ff_wmv2_encode_picture_header(MpegEncContext * s, int picture_number) {}
void ff_wmv2_encode_mb(MpegEncContext * s,
                       DCTELEM block[6][64],
                       int motion_x, int motion_y) {}
void ff_msmpeg4_encode_picture_header(MpegEncContext * s, int picture_number) {}

// CONFIG_MPEG1VIDEO_ENCODER || CONFIG_MPEG2VIDEO_ENCODER
void ff_mpeg1_encode_init(MpegEncContext *s) {}
void ff_mpeg1_encode_mb(MpegEncContext *s,
                        DCTELEM block[6][64],
                        int motion_x, int motion_y) {}
void ff_mpeg1_encode_slice_header(MpegEncContext *s) {}
void ff_mpeg1_encode_picture_header(MpegEncContext *s, int picture_number) {}

// CONFIG_MPEG4_ENCODER
void ff_mpeg4_encode_mb(MpegEncContext *s,
                        DCTELEM block[6][64],
                        int motion_x, int motion_y) {}
void ff_mpeg4_merge_partitions(MpegEncContext *s) {}
void ff_mpeg4_stuffing(PutBitContext * pbc) {}
void ff_mpeg4_init_partitions(MpegEncContext *s) {}
void ff_mpeg4_encode_video_packet_header(MpegEncContext *s) {}
void ff_clean_mpeg4_qscales(MpegEncContext *s) {}
void ff_set_mpeg4_time(MpegEncContext * s) {}
void ff_mpeg4_encode_picture_header(MpegEncContext *s, int picture_number) {}

// CONFIG_RV10_ENCODER
void ff_rv10_encode_picture_header(MpegEncContext *s, int picture_number) {}

// CONFIG_RV20_ENCODER
void ff_rv20_encode_picture_header(MpegEncContext *s, int picture_number) {}

// CONFIG_FLV_ENCODER
void ff_flv_encode_picture_header(MpegEncContext * s, int picture_number) {}
void ff_flv2_encode_ac_esc(PutBitContext *pb, int slevel, int level, int run, int last) {}

// avcodec_register_all
AVCodec ff_ac3_encoder;
AVCodec ff_mp3_decoder;
AVCodec ff_mp2_decoder;
AVCodec ff_mp1_decoder;
AVCodec ff_ljpeg_encoder;
AVCodec ff_indeo3_decoder;
AVCodec ff_huffyuv_encoder;

// SPP deblocking
void ff_simple_idct_mmx(int16_t *block)
{
    // Not implemented.
}
