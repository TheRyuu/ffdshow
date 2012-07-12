/*
 * Copyright (c) 2004-2006 Milan Cutka
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "stdafx.h"
#include "ffcodecs.h"
#include "ffdshow_mediaguids.h"

const FOURCC* getCodecFOURCCs(CodecID codecId)
{
    switch (codecId) {
        case CODEC_ID_MJPEG: {
            static const FOURCC fccs[] = {FOURCC_MJPG, 0};
            return fccs;
        }
        case CODEC_ID_FFVHUFF: {
            static const FOURCC fccs[] = {FOURCC_FFVH, 0};
            return fccs;
        }
        case CODEC_ID_FFV1: {
            static const FOURCC fccs[] = {FOURCC_FFV1, 0};
            return fccs;
        }
        case CODEC_ID_DVVIDEO: {
            /* lowercase FourCC 'dvsd' for compatibility with MS DV decoder */
            static const FOURCC fccs[] = {mmioFOURCC('d', 'v', 's', 'd'), FOURCC_DVSD, mmioFOURCC('d', 'v', '2', '5'), FOURCC_DV25, mmioFOURCC('d', 'v', '5', '0'), FOURCC_DV50, 0};
            return fccs;
        }
        case CODEC_ID_CYUV: {
            static const FOURCC fccs[] = {FOURCC_CYUV, 0};
            return fccs;
        }
        case CODEC_ID_CLJR: {
            static const FOURCC fccs[] = {FOURCC_CLJR, 0};
            return fccs;
        }
        case CODEC_ID_Y800: {
            static const FOURCC fccs[] = {FOURCC_Y800, 0};
            return fccs;
        }
        case CODEC_ID_444P: {
            static const FOURCC fccs[] = {FOURCC_444P, FOURCC_YV24, 0};
            return fccs;
        }
        case CODEC_ID_422P: {
            static const FOURCC fccs[] = {FOURCC_422P, FOURCC_YV16, 0};
            return fccs;
        }
        case CODEC_ID_411P: {
            static const FOURCC fccs[] = {FOURCC_411P, FOURCC_Y41B, 0};
            return fccs;
        }
        case CODEC_ID_410P: {
            static const FOURCC fccs[] = {FOURCC_410P, 0};
            return fccs;
        }
        default: {
            static const FOURCC fccs[] = {0, 0};
            return fccs;
        }
    }
}

const char_t *getCodecName(CodecID codecId)
{
    switch (codecId) {
        case  CODEC_ID_UNSUPPORTED :
            return _l("unsupported");
        case  CODEC_ID_NONE :
            return _l("none");

            /* Well-known video codecs */
        case  CODEC_ID_H261 :
            return _l("h261");
        case  CODEC_ID_H263 :
            return _l("h263");
        case  CODEC_ID_H263I :
            return _l("h263");
        case  CODEC_ID_H263P :
            return _l("h263");
        case  CODEC_ID_H264 :
            return _l("h264");
        case  CODEC_ID_MPEG1VIDEO :
            return _l("mpeg1video");
        case  CODEC_ID_MPEG2VIDEO :
            return _l("mpeg2video");
        case  CODEC_ID_MPEG4 :
            return _l("mpeg4");
        case  CODEC_ID_MSMPEG4V1 :
            return _l("msmpeg4v1");
        case  CODEC_ID_MSMPEG4V2 :
            return _l("msmpeg4v2");
        case  CODEC_ID_MSMPEG4V3 :
            return _l("msmpeg4v3");
        case  CODEC_ID_WMV1 :
            return _l("wmv1");
        case  CODEC_ID_WMV2 :
            return _l("wmv2");
        case  CODEC_ID_WMV3 :
            return _l("wmv3");
        case  CODEC_ID_VC1 :
            return _l("vc1");
        case  CODEC_ID_SVQ1 :
            return _l("svq1");
        case  CODEC_ID_SVQ3 :
            return _l("svq3");
        case  CODEC_ID_FLV1 :
            return _l("flv1");
        case  CODEC_ID_VP3 :
            return _l("vp3");
        case  CODEC_ID_VP5 :
            return _l("vp5");
        case  CODEC_ID_VP6 :
            return _l("vp6");
        case  CODEC_ID_VP6F :
            return _l("vp6f");
        case  CODEC_ID_RV10 :
            return _l("rv10");
        case  CODEC_ID_RV20 :
            return _l("rv20");
        case  CODEC_ID_DVVIDEO :
            return _l("dvvideo");
        case  CODEC_ID_MJPEG :
            return _l("mjpeg");
        case  CODEC_ID_MJPEGB :
            return _l("mjpegb");
        case  CODEC_ID_INDEO2 :
            return _l("indeo2");
        case  CODEC_ID_INDEO3 :
            return _l("indeo3");
        case  CODEC_ID_MSVIDEO1 :
            return _l("msvideo1");
        case  CODEC_ID_CINEPAK :
            return _l("cinepak");
        case  CODEC_ID_THEORA :
            return _l("theora");

            /* Lossless video codecs */
        case  CODEC_ID_HUFFYUV :
            return _l("huffyuv");
        case  CODEC_ID_FFVHUFF :
            return _l("ffvhuff");
        case  CODEC_ID_FFV1 :
            return _l("ffv1");
        case  CODEC_ID_ZMBV :
            return _l("zmbv");
        case  CODEC_ID_PNG :
            return _l("png");
        case  CODEC_ID_LJPEG :
            return _l("ljpeg");
        case  CODEC_ID_JPEGLS :
            return _l("jpegls");
        case  CODEC_ID_CSCD :
            return _l("camstudio");
        case  CODEC_ID_QPEG :
            return _l("qpeg");
        case  CODEC_ID_LOCO :
            return _l("loco");
        case  CODEC_ID_MSZH :
            return _l("mszh");
        case  CODEC_ID_ZLIB :
            return _l("zlib");
        case  CODEC_ID_SP5X :
            return _l("sp5x");

            /* Other video codecs */
        case  CODEC_ID_AVS :
            return _l("avs");
        case  CODEC_ID_8BPS :
            return _l("8bps");
        case  CODEC_ID_QTRLE :
            return _l("qtrle");
        case  CODEC_ID_RPZA :
            return _l("qtrpza");
        case  CODEC_ID_TRUEMOTION1 :
            return _l("truemotion");
        case  CODEC_ID_TRUEMOTION2 :
            return _l("truemotion2");
        case  CODEC_ID_TSCC :
            return _l("tscc");
        case  CODEC_ID_CYUV :
            return _l("cyuv");
        case  CODEC_ID_VCR1 :
            return _l("vcr1");
        case  CODEC_ID_MSRLE :
            return _l("msrle");
        case  CODEC_ID_ASV1 :
            return _l("asv1");
        case  CODEC_ID_ASV2 :
            return _l("asv2");
        case  CODEC_ID_VIXL :
            return _l("vixl");
        case  CODEC_ID_WNV1 :
            return _l("wnv1");
        case  CODEC_ID_FRAPS :
            return _l("fraps");
        case  CODEC_ID_MPEG2TS :
            return _l("mpeg2ts");
        case  CODEC_ID_AASC :
            return _l("aasc");
        case  CODEC_ID_ULTI :
            return _l("ulti");
        case  CODEC_ID_CAVS :
            return _l("cavs");
        case  CODEC_ID_SNOW :
            return _l("snow");
        case  CODEC_ID_VP6A :
            return _l("vp6a");
        case  CODEC_ID_RV30 :
            return _l("rv30");
        case  CODEC_ID_RV40 :
            return _l("rv40");
        case  CODEC_ID_AMV :
            return _l("amv");
        case  CODEC_ID_VP8 :
            return _l("vp8");
        case  CODEC_ID_INDEO5 :
            return _l("indeo5");
        case  CODEC_ID_WMV3IMAGE :
            return _l("wmvp");
        case  CODEC_ID_VC1IMAGE :
            return _l("wvp2");
        case  CODEC_ID_INDEO4 :
            return _l("indeo4");

            /* Well-known audio codecs */
        case  CODEC_ID_MP2 :
            return _l("mp2");
        case  CODEC_ID_MP3 :
            return _l("mp3");
        case  CODEC_ID_VORBIS :
            return _l("vorbis");
        case  CODEC_ID_AC3 :
            return _l("ac3");
        case  CODEC_ID_WMAV1 :
            return _l("wmav1");
        case  CODEC_ID_WMAV2 :
            return _l("wmav2");
        case  CODEC_ID_AAC :
            return _l("aac");
        case  CODEC_ID_DTS :
            return _l("dts");
        case  CODEC_ID_IMC :
            return _l("imc");
        case  CODEC_ID_PCM_U16LE :
            return _l("pcm u16le");
        case  CODEC_ID_PCM_U16BE :
            return _l("pcm u16be");
        case  CODEC_ID_PCM_S8 :
            return _l("pcm s8");
        case  CODEC_ID_PCM_U8 :
            return _l("pcm u8");
        case  CODEC_ID_PCM_MULAW :
            return _l("mulaw");
        case  CODEC_ID_PCM_ALAW :
            return _l("alaw");
        case  CODEC_ID_ADPCM_IMA_QT :
            return _l("adpcm ima qt");
        case  CODEC_ID_ADPCM_IMA_WAV :
            return _l("adpcm ima wav");
        case  CODEC_ID_ADPCM_MS :
            return _l("adpcm ms");
        case  CODEC_ID_ADPCM_IMA_DK3 :
            return _l("adpcm ima dk3");
        case  CODEC_ID_ADPCM_IMA_DK4 :
            return _l("adpcm ima dk4");
        case  CODEC_ID_ADPCM_IMA_WS :
            return _l("adpcm ima ws");
        case  CODEC_ID_ADPCM_IMA_SMJPEG :
            return _l("adpcm ima smjpeg");
        case  CODEC_ID_ADPCM_4XM :
            return _l("adpcm 4xm");
        case  CODEC_ID_ADPCM_XA :
            return _l("adpcm xa");
        case  CODEC_ID_ADPCM_EA :
            return _l("adpcm ea");
        case  CODEC_ID_ADPCM_G726 :
            return _l("adpcm g726");
        case  CODEC_ID_ADPCM_CT :
            return _l("adpcm ct");
        case  CODEC_ID_ADPCM_SWF :
            return _l("adpcm swf");
        case  CODEC_ID_ADPCM_YAMAHA :
            return _l("adpcm yamaha");
        case  CODEC_ID_ADPCM_SBPRO_2 :
            return _l("adpcm sbpro2");
        case  CODEC_ID_ADPCM_SBPRO_3 :
            return _l("adpcm sbpro3");
        case  CODEC_ID_ADPCM_SBPRO_4 :
            return _l("adpcm sbpro4");
        case  CODEC_ID_ADPCM_IMA_AMV :
            return _l("adpcm ima amv");
        case  CODEC_ID_FLAC :
            return _l("flac");
        case  CODEC_ID_AMR_NB :
            return _l("amr nb");
        case  CODEC_ID_GSM_MS :
            return _l("gsm ms");
        case  CODEC_ID_TTA :
            return _l("tta");
        case  CODEC_ID_MACE3 :
            return _l("mace3");
        case  CODEC_ID_MACE6 :
            return _l("mace6");
        case  CODEC_ID_QDM2 :
            return _l("qdm2");
        case  CODEC_ID_COOK :
            return _l("cook");
        case  CODEC_ID_TRUESPEECH :
            return _l("truespeech");
        case  CODEC_ID_RA_144 :
            return _l("14_4");
        case  CODEC_ID_RA_288 :
            return _l("28_8");
        case  CODEC_ID_ATRAC3 :
            return _l("atrac 3");
        case  CODEC_ID_NELLYMOSER :
            return _l("nellymoser");
        case  CODEC_ID_EAC3 :
            return _l("eac3");
        case  CODEC_ID_MP3ADU :
            return _l("mp3adu");
        case  CODEC_ID_MLP :
            return _l("mlp");
        case  CODEC_ID_MP1 :
            return _l("mp1");
        case  CODEC_ID_TRUEHD :
            return _l("truehd");
        case  CODEC_ID_WAVPACK :
            return _l("wavpack");
        case  CODEC_ID_GSM :
            return _l("gsm");
        case  CODEC_ID_AAC_LATM :
            return _l("aac latm");
        case  CODEC_ID_THP :
            return _l("thp");
        case  CODEC_ID_AMR_WB :
            return _l("amr wb");
        case  CODEC_ID_IAC :
            return _l("iac");

            /* Raw formats */
        case  CODEC_ID_RAW :
            return _l("raw");
        case  CODEC_ID_YUY2 :
            return _l("raw");
        case  CODEC_ID_RGB2 :
            return _l("raw");
        case  CODEC_ID_RGB3 :
            return _l("raw");
        case  CODEC_ID_RGB5 :
            return _l("raw");
        case  CODEC_ID_RGB6 :
            return _l("raw");
        case  CODEC_ID_BGR2 :
            return _l("raw");
        case  CODEC_ID_BGR3 :
            return _l("raw");
        case  CODEC_ID_BGR5 :
            return _l("raw");
        case  CODEC_ID_BGR6 :
            return _l("raw");
        case  CODEC_ID_YV12 :
            return _l("raw");
        case  CODEC_ID_YVYU :
            return _l("raw");
        case  CODEC_ID_UYVY :
            return _l("raw");
        case  CODEC_ID_VYUY :
            return _l("raw");
        case  CODEC_ID_I420 :
            return _l("raw");
        case  CODEC_ID_CLJR :
            return _l("raw");
        case  CODEC_ID_Y800 :
            return _l("raw");
        case  CODEC_ID_444P :
            return _l("raw");
        case  CODEC_ID_422P :
            return _l("raw");
        case  CODEC_ID_411P :
            return _l("raw");
        case  CODEC_ID_410P :
            return _l("raw");
        case  CODEC_ID_NV12 :
            return _l("raw");
        case  CODEC_ID_NV21 :
            return _l("raw");
        case  CODEC_ID_YV16 :
            return _l("raw");
        case  CODEC_ID_LPCM :
            return _l("raw LPCM");
        case  CODEC_ID_PCM :
            return _l("raw PCM");
        case  CODEC_ID_PCM_S32LE :
            return _l("raw PCM 32");
        case  CODEC_ID_PCM_S32BE :
            return _l("raw PCM 32");
        case  CODEC_ID_PCM_U32LE :
            return _l("raw PCM 32");
        case  CODEC_ID_PCM_U32BE :
            return _l("raw PCM 32");
        case  CODEC_ID_PCM_S24LE :
            return _l("raw PCM 24");
        case  CODEC_ID_PCM_S24BE :
            return _l("raw PCM 24");
        case  CODEC_ID_PCM_U24LE :
            return _l("raw PCM 24");
        case  CODEC_ID_PCM_U24BE :
            return _l("raw PCM 24");
        case  CODEC_ID_PCM_S16LE :
            return _l("raw PCM 16");
        case  CODEC_ID_PCM_S16BE :
            return _l("raw PCM 16");
        case  CODEC_ID_PCM_S24DAUD :
            return _l("raw PCM 24");

            /* Decoders and other stuff */
        case  CODEC_ID_XVID4 :
            return _l("xvid");
        case  CODEC_ID_LIBMPEG2 :
            return _l("libmpeg2");
        case  CODEC_ID_LIBMAD :
            return _l("libmad");
        case  CODEC_ID_LIBFAAD :
            return _l("faad2");
        case  CODEC_ID_WMV9_LIB :
            return _l("wmv9codec");
        case  CODEC_ID_AVISYNTH :
            return _l("avisynth");
        case  CODEC_ID_LIBA52 :
            return _l("liba52");
        case  CODEC_ID_SPDIF_AC3 :
            return _l("AC3 s/pdif");
        case  CODEC_ID_SPDIF_DTS :
            return _l("DTS s/pdif");
        case  CODEC_ID_BITSTREAM_TRUEHD :
            return _l("bitstream Dolby True HD");
        case  CODEC_ID_BITSTREAM_DTSHD :
            return _l("bitstream DTS-HD");
        case  CODEC_ID_BITSTREAM_EAC3 :
            return _l("bitstream EAC3");
        case  CODEC_ID_LIBDTS :
            return _l("libdts");
        case  CODEC_ID_H264_DXVA :
            return _l("h264 DXVA");
        case  CODEC_ID_VC1_DXVA :
            return _l("vc1 DXVA");
        case  CODEC_ID_HDMV_PGS_SUBTITLE :
            return _l("PGS subtitles");
        case  CODEC_ID_MPEG2_QUICK_SYNC :
            return _l("mpeg2 QuickSync");
        case  CODEC_ID_H264_QUICK_SYNC :
            return _l("h264 QuickSync");
        case  CODEC_ID_VC1_QUICK_SYNC :
            return _l("vc1 QuickSync");

        default:
            return _l("unknown");
    }
}
