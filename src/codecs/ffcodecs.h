#ifndef _FFCODECS_H_
#define _FFCODECS_H_

// When adding a new CodecID, don't forget
// to add it to getCodecName in ffcodecs.cpp too.
enum CodecID {

 CODEC_ID_UNSUPPORTED       = -1,
 CODEC_ID_NONE              = 0,

 /* Well-known video codecs */
 CODEC_ID_H261              = 1,
 CODEC_ID_H263              = 2,
 CODEC_ID_H263I             = 3,
 CODEC_ID_H263P             = 4,
 CODEC_ID_H264              = 5,
 CODEC_ID_MPEG1VIDEO        = 6,
 CODEC_ID_MPEG2VIDEO        = 7,
 CODEC_ID_MPEG4             = 8,
 CODEC_ID_MSMPEG4V1         = 9,
 CODEC_ID_MSMPEG4V2         = 10,
 CODEC_ID_MSMPEG4V3         = 11,
 CODEC_ID_WMV1              = 12,
 CODEC_ID_WMV2              = 13,
 CODEC_ID_WMV3              = 14,
 CODEC_ID_VC1               = 15,
 CODEC_ID_SVQ1              = 16,
 CODEC_ID_SVQ3              = 17,
 CODEC_ID_FLV1              = 18,
 CODEC_ID_VP3               = 19,
 CODEC_ID_VP5               = 20,
 CODEC_ID_VP6               = 21,
 CODEC_ID_VP6F              = 22,
 CODEC_ID_RV10              = 23,
 CODEC_ID_RV20              = 24,
 CODEC_ID_DVVIDEO           = 25,
 CODEC_ID_MJPEG             = 26,
 CODEC_ID_MJPEGB            = 27,
 CODEC_ID_INDEO2            = 28,
 CODEC_ID_INDEO3            = 29,
 CODEC_ID_MSVIDEO1          = 30,
 CODEC_ID_CINEPAK           = 31,
 CODEC_ID_THEORA            = 32,

 /* Lossless video codecs */
 CODEC_ID_HUFFYUV           = 33,
 CODEC_ID_FFVHUFF           = 34,
 CODEC_ID_FFV1              = 35,
 CODEC_ID_ZMBV              = 36,
 CODEC_ID_PNG               = 37,
 CODEC_ID_LJPEG             = 39,
 CODEC_ID_JPEGLS            = 40,
 CODEC_ID_CSCD              = 41,
 CODEC_ID_QPEG              = 42,
 CODEC_ID_LOCO              = 43,
 CODEC_ID_MSZH              = 44,
 CODEC_ID_ZLIB              = 45,
 CODEC_ID_SP5X              = 46,

 /* Other video codecs */ 
 CODEC_ID_AVS               = 47,
 CODEC_ID_8BPS              = 48,
 CODEC_ID_QTRLE             = 49,
 CODEC_ID_RPZA              = 50,
 CODEC_ID_TRUEMOTION1       = 51,
 CODEC_ID_TRUEMOTION2       = 52,
 CODEC_ID_TSCC              = 53,
 CODEC_ID_CYUV              = 55,
 CODEC_ID_VCR1              = 56,
 CODEC_ID_MSRLE             = 57,
 CODEC_ID_ASV1              = 58,
 CODEC_ID_ASV2              = 59,
 CODEC_ID_VIXL              = 60,
 CODEC_ID_WNV1              = 61,
 CODEC_ID_FRAPS             = 62,
 CODEC_ID_MPEG2TS           = 63,
 CODEC_ID_AASC              = 64,
 CODEC_ID_ULTI              = 65,
 CODEC_ID_CAVS              = 66,
 CODEC_ID_SNOW              = 67,
 CODEC_ID_VP6A              = 68,
 CODEC_ID_RV30              = 69,
 CODEC_ID_RV40              = 70,
 CODEC_ID_AMV               = 71,
 CODEC_ID_VP8               = 72,
 CODEC_ID_INDEO5            = 73,
 CODEC_ID_WMV3IMAGE         = 74,
 CODEC_ID_VC1IMAGE          = 75,

 /* Well-known audio codecs */
 CODEC_ID_MP2               = 100,
 CODEC_ID_MP3               = 101,
 CODEC_ID_VORBIS            = 102,
 CODEC_ID_AC3               = 103,
 CODEC_ID_WMAV1             = 104,
 CODEC_ID_WMAV2             = 105,
 CODEC_ID_AAC               = 106,
 CODEC_ID_DTS               = 107,
 CODEC_ID_IMC               = 108,
 CODEC_ID_PCM_U16LE         = 109,
 CODEC_ID_PCM_U16BE         = 110,
 CODEC_ID_PCM_S8            = 111,
 CODEC_ID_PCM_U8            = 112,
 CODEC_ID_PCM_MULAW         = 113,
 CODEC_ID_PCM_ALAW          = 114,
 CODEC_ID_ADPCM_IMA_QT      = 115,
 CODEC_ID_ADPCM_IMA_WAV     = 116,
 CODEC_ID_ADPCM_MS          = 117,
 CODEC_ID_ADPCM_IMA_DK3     = 118,
 CODEC_ID_ADPCM_IMA_DK4     = 119,
 CODEC_ID_ADPCM_IMA_WS      = 120,
 CODEC_ID_ADPCM_IMA_SMJPEG  = 121,
 CODEC_ID_ADPCM_4XM         = 122,
 CODEC_ID_ADPCM_XA          = 123,
 CODEC_ID_ADPCM_EA          = 124,
 CODEC_ID_ADPCM_G726        = 125,
 CODEC_ID_ADPCM_CT          = 126,
 CODEC_ID_ADPCM_SWF         = 127,
 CODEC_ID_ADPCM_YAMAHA      = 128,
 CODEC_ID_ADPCM_SBPRO_2     = 129,
 CODEC_ID_ADPCM_SBPRO_3     = 130,
 CODEC_ID_ADPCM_SBPRO_4     = 131,
 CODEC_ID_ADPCM_IMA_AMV     = 132,
 CODEC_ID_FLAC              = 133,
 CODEC_ID_AMR_NB            = 134,
 CODEC_ID_GSM_MS            = 135,
 CODEC_ID_TTA               = 136,
 CODEC_ID_MACE3             = 137,
 CODEC_ID_MACE6             = 138,
 CODEC_ID_QDM2              = 139,
 CODEC_ID_COOK              = 140,
 CODEC_ID_TRUESPEECH        = 141,
 CODEC_ID_RA_144            = 142,
 CODEC_ID_RA_288            = 143,
 CODEC_ID_ATRAC3            = 144,
 CODEC_ID_NELLYMOSER        = 145,
 CODEC_ID_EAC3              = 146,
 CODEC_ID_MP3ADU            = 147,
 CODEC_ID_MLP               = 148,
 CODEC_ID_MP1               = 149,
 CODEC_ID_TRUEHD            = 150,
 CODEC_ID_WAVPACK           = 151,
 CODEC_ID_GSM               = 152,
 CODEC_ID_AAC_LATM          = 153,
 CODEC_ID_THP               = 154,

 /* Raw formats */
 CODEC_ID_RAW               = 300,
 CODEC_ID_YUY2              = 301,
 CODEC_ID_RGB2              = 302,
 CODEC_ID_RGB3              = 303,
 CODEC_ID_RGB5              = 304,
 CODEC_ID_RGB6              = 305,
 CODEC_ID_BGR2              = 306,
 CODEC_ID_BGR3              = 307,
 CODEC_ID_BGR5              = 308,
 CODEC_ID_BGR6              = 309,
 CODEC_ID_YV12              = 310,
 CODEC_ID_YVYU              = 311,
 CODEC_ID_UYVY              = 312,
 CODEC_ID_VYUY              = 313,
 CODEC_ID_I420              = 314,
 CODEC_ID_CLJR              = 315,
 CODEC_ID_Y800              = 316,
 CODEC_ID_444P              = 317,
 CODEC_ID_422P              = 318,
 CODEC_ID_411P              = 319,
 CODEC_ID_410P              = 320,
 CODEC_ID_NV12              = 321,
 CODEC_ID_NV21              = 322,
 CODEC_ID_YV16              = 326,
 CODEC_ID_LPCM              = 398,
 CODEC_ID_PCM               = 399,
 CODEC_ID_PCM_S32LE         = 200,
 CODEC_ID_PCM_S32BE         = 201,
 CODEC_ID_PCM_U32LE         = 202,
 CODEC_ID_PCM_U32BE         = 203,
 CODEC_ID_PCM_S24LE         = 204,
 CODEC_ID_PCM_S24BE         = 205,
 CODEC_ID_PCM_U24LE         = 206,
 CODEC_ID_PCM_U24BE         = 207,
 CODEC_ID_PCM_S16LE         = 208,
 CODEC_ID_PCM_S16BE         = 209,
 CODEC_ID_PCM_S24DAUD       = 210,

 /* Decoders and other stuff */
 CODEC_ID_XVID4             = 400,
 CODEC_ID_LIBMPEG2          = 500,
 CODEC_ID_LIBMAD            = 800,
 CODEC_ID_LIBFAAD           = 900,
 CODEC_ID_WMV9_LIB          = 1000,
 CODEC_ID_AVISYNTH          = 1100,
 CODEC_ID_LIBA52            = 1300,
 CODEC_ID_SPDIF_AC3         = 1400,
 CODEC_ID_SPDIF_DTS         = 1401,
 CODEC_ID_BITSTREAM_TRUEHD  = 1402,
 CODEC_ID_BITSTREAM_DTSHD   = 1403,
 CODEC_ID_BITSTREAM_EAC3    = 1404,
 CODEC_ID_LIBDTS            = 1500,
 CODEC_ID_H264_DXVA         = 2100,
 CODEC_ID_VC1_DXVA          = 2101,
 CODEC_ID_HDMV_PGS_SUBTITLE = 2200,
 CODEC_ID_H264_QUICK_SYNC   = 2300,
 CODEC_ID_VC1_QUICK_SYNC    = 2301,
 CODEC_ID_MPEG2_QUICK_SYNC  = 2302
};

#ifdef __cplusplus

const FOURCC* getCodecFOURCCs(CodecID codecId);
const char_t* getCodecName(CodecID codecId);

static __inline bool lavc_codec(int x)
{
    return x>0 && x<200;
}
static __inline bool raw_codec(int x)
{
    return x>=300 && x<400;
}
static __inline bool xvid_codec(int x)
{
    return x==CODEC_ID_XVID4;
}
static __inline bool wmv9_codec(int x)
{
    return x>=1000 && x<1100 || x==CODEC_ID_VC1_QUICK_SYNC;
}
static __inline bool mpeg12_codec(int x)
{
    return x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_MPEG2VIDEO || x==CODEC_ID_LIBMPEG2 || x==CODEC_ID_MPEG2_QUICK_SYNC;
}
static __inline bool mpeg1_codec(int x)
{
    return x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_LIBMPEG2;
}
static __inline bool mpeg2_codec(int x)
{
    return x==CODEC_ID_MPEG2VIDEO || x==CODEC_ID_LIBMPEG2 || x==CODEC_ID_MPEG2_QUICK_SYNC;
}
static __inline bool mpeg4_codec(int x)
{
    return x==CODEC_ID_MPEG4 || xvid_codec(x);
}
static __inline bool spdif_codec(int x)
{
    return x==CODEC_ID_SPDIF_AC3 || x==CODEC_ID_SPDIF_DTS;
}
static __inline bool bitstream_codec(int x)
{
    return x==CODEC_ID_BITSTREAM_TRUEHD || x==CODEC_ID_BITSTREAM_DTSHD || x==CODEC_ID_BITSTREAM_EAC3;
}
static __inline bool huffyuv_codec(int x)
{
    return x==CODEC_ID_HUFFYUV || x==CODEC_ID_FFVHUFF;
}
static __inline bool lossless_codec(int x)
{
    return huffyuv_codec(x) || x==CODEC_ID_LJPEG || x==CODEC_ID_FFV1 || x==CODEC_ID_DVVIDEO;
}
static __inline bool h264_codec(int x)
{
    return x == CODEC_ID_H264 || x == CODEC_ID_H264_QUICK_SYNC;
}
static __inline bool vc1_codec(int x)
{
    return x == CODEC_ID_VC1 || x == CODEC_ID_WMV9_LIB || x == CODEC_ID_VC1_QUICK_SYNC;
}
static __inline bool dxva_codec(int x)
{
    return x == CODEC_ID_H264_DXVA || x == CODEC_ID_VC1_DXVA;
}

static __inline bool is_quicksync_codec(int x)
{
    return x == CODEC_ID_H264_QUICK_SYNC || x == CODEC_ID_VC1_QUICK_SYNC || x == CODEC_ID_MPEG2_QUICK_SYNC;
}

//I'm not sure of all these
static __inline bool sup_CBR(int x)
{
    return !lossless_codec(x) && !raw_codec(x);
}
static __inline bool sup_VBR_QUAL(int x)
{
    return !lossless_codec(x) && !raw_codec(x);
}
static __inline bool sup_VBR_QUANT(int x)
{
    return (lavc_codec(x) || xvid_codec(x)) && !lossless_codec(x);
}
static __inline bool sup_XVID2PASS(int x)
{
    return sup_VBR_QUANT(x);
}
static __inline bool sup_LAVC2PASS(int x)
{
    return (lavc_codec(x) && !lossless_codec(x) && x!=CODEC_ID_MJPEG && !raw_codec(x));
}

static __inline bool sup_interlace(int x)
{
    return x==CODEC_ID_MPEG4 || x==CODEC_ID_MPEG2VIDEO || xvid_codec(x);
}
static __inline bool sup_gray(int x)
{
    return x!=CODEC_ID_LJPEG && x!=CODEC_ID_FFV1 && !wmv9_codec(x) && !raw_codec(x) && x!=CODEC_ID_DVVIDEO;
}
static __inline bool sup_globalheader(int x)
{
    return x==CODEC_ID_MPEG4;
}
static __inline bool sup_part(int x)
{
    return x==CODEC_ID_MPEG4;
}
static __inline bool sup_packedBitstream(int x)
{
    return xvid_codec(x);
}
static __inline bool sup_minKeySet(int x)
{
    return x!=CODEC_ID_MJPEG && !lossless_codec(x) && !wmv9_codec(x) && !raw_codec(x);
}
static __inline bool sup_maxKeySet(int x)
{
    return x!=CODEC_ID_MJPEG && !lossless_codec(x) && !raw_codec(x);
}
static __inline bool sup_bframes(int x)
{
    return x==CODEC_ID_MPEG4 || x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_MPEG2VIDEO || xvid_codec(x);
}
static __inline bool sup_adaptiveBframes(int x)
{
    return lavc_codec(x);
}
static __inline bool sup_closedGop(int x)
{
    return sup_bframes(x);
}
static __inline bool sup_lavcme(int x)
{
    return lavc_codec(x) && x!=CODEC_ID_MJPEG && !lossless_codec(x);
}
static __inline bool sup_quantProps(int x)
{
    return !lossless_codec(x) && !wmv9_codec(x) && !raw_codec(x);
}
static __inline bool sup_trellisQuant(int x)
{
    return x==CODEC_ID_MPEG4 || x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_MPEG2VIDEO || x==CODEC_ID_XVID4 || x==CODEC_ID_H263 || x==CODEC_ID_H263P;
}
static __inline bool sup_masking(int x)
{
    return x==CODEC_ID_MPEG4 || x==CODEC_ID_H263 || x==CODEC_ID_H263P || x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_MPEG2VIDEO || xvid_codec(x);
}
static __inline bool sup_lavcOnePass(int x)
{
    return (lavc_codec(x) && !lossless_codec(x));
}
static __inline bool sup_perFrameQuant(int x)
{
    return !lossless_codec(x) && !wmv9_codec(x) && !raw_codec(x);
}
static __inline bool sup_4mv(int x)
{
    return x==CODEC_ID_MPEG4 || x==CODEC_ID_H263 || x==CODEC_ID_H263P;
}
static __inline bool sup_aspect(int x)
{
    return x==CODEC_ID_MPEG4 || x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_MPEG2VIDEO;
}
static __inline bool sup_PSNR(int x)
{
    return (lavc_codec(x) && !lossless_codec(x)) || xvid_codec(x);
}
static __inline bool sup_quantBias(int x)
{
    return lavc_codec(x) && !lossless_codec(x);
}
static __inline bool sup_MPEGquant(int x)
{
    return x==CODEC_ID_MPEG4 || x==CODEC_ID_MSMPEG4V3 || x==CODEC_ID_MPEG2VIDEO || xvid_codec(x);
}
static __inline bool sup_lavcQuant(int x)
{
    return lavc_codec(x) && sup_quantProps(x);
}
static __inline bool sup_customQuantTables(int x)
{
    return x==CODEC_ID_MPEG4 || xvid_codec(x) || x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_MPEG2VIDEO;
}
static __inline bool sup_qpel(int x)
{
    return x==CODEC_ID_MPEG4 || xvid_codec(x);
}
static __inline bool sup_gmc(int x)
{
    return xvid_codec(x);
}
static __inline bool sup_me_mv0(int x)
{
    return sup_lavcme(x);
}
static __inline bool sup_cbp_rd(int x)
{
    return x==CODEC_ID_MPEG4;
}
static __inline bool sup_qns(int x)
{
    return lavc_codec(x) && sup_quantProps(x) && x!=CODEC_ID_MSMPEG4V3 && x!=CODEC_ID_MSMPEG4V2 && x!=CODEC_ID_MSMPEG4V1 && x!=CODEC_ID_WMV1 && x!=CODEC_ID_WMV2 && x!=CODEC_ID_MJPEG;
}
static __inline bool sup_threads_enc(int x)
{
    return x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_MPEG2VIDEO || x==CODEC_ID_XVID4;
}
static __inline bool sup_threads_dec_slice(int x)
{
    return x==CODEC_ID_MPEG1VIDEO || x==CODEC_ID_MPEG2VIDEO || x==CODEC_ID_FFV1 || x==CODEC_ID_DVVIDEO;
}
static __inline bool sup_threads_dec_frame(int x)
{
    return x==CODEC_ID_H264;
}
static __inline bool sup_palette(int x)
{
    return x==CODEC_ID_MSVIDEO1 || x==CODEC_ID_8BPS || x==CODEC_ID_QTRLE || x==CODEC_ID_TSCC || x==CODEC_ID_QPEG || x==CODEC_ID_PNG;
}

#endif

#endif
