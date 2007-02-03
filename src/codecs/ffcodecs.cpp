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
 switch (codecId)
  {
   case CODEC_ID_MPEG4:
   case CODEC_ID_SKAL:
   case CODEC_ID_XVID4:
    {
     static const FOURCC fccs[]={FOURCC_XVID,FOURCC_FFDS,FOURCC_FVFW,FOURCC_DX50,FOURCC_DIVX,FOURCC_MP4V,FOURCC_3IV2,FOURCC_3IVX,FOURCC_MP4S,FOURCC_M4S2,FOURCC_RMP4,FOURCC_DM4V,FOURCC_DXGM,FOURCC_WV1F,FOURCC_FMP4,FOURCC_HDX4,0};
     return fccs;
    }
   case CODEC_ID_MSMPEG4V3:
    {
     static const FOURCC fccs[]={FOURCC_DIV3,FOURCC_MP43,FOURCC_COL1,FOURCC_DVX3,FOURCC_DIV4,FOURCC_DIV5,FOURCC_DIV6,FOURCC_AP41,FOURCC_MPG3,0};
     return fccs;
    }
   case CODEC_ID_MSMPEG4V2:
    {
     static const FOURCC fccs[]={FOURCC_MP42,FOURCC_DIV2,0};
     return fccs;
    } 
   case CODEC_ID_MSMPEG4V1:
    {
     static const FOURCC fccs[]={FOURCC_MP41,FOURCC_DIV1,0};
     return fccs;
    } 
   case CODEC_ID_MPEG1VIDEO:
    {
     static const FOURCC fccs[]={FOURCC_MPEG,FOURCC_MPG1,0};
     return fccs;
    } 
   case CODEC_ID_MPEG2VIDEO:
    {
     static const FOURCC fccs[]={FOURCC_MPEG,FOURCC_MPG2,0};
     return fccs;
    } 
   case CODEC_ID_H263:
   case CODEC_ID_H263P:
   case CODEC_ID_H263I:
    {
     static const FOURCC fccs[]={FOURCC_H263,FOURCC_S263,0};
     return fccs;
    }
   case CODEC_ID_H261:
    {
     static const FOURCC fccs[]={FOURCC_H261,0};
     return fccs;
    } 
   case CODEC_ID_WMV1:
    {
     static const FOURCC fccs[]={FOURCC_WMV1,0};
     return fccs;
    }
   case CODEC_ID_WMV2:
    {
     static const FOURCC fccs[]={FOURCC_WMV2,0};
     return fccs;
    }
   case CODEC_ID_WMV9_LIB:
    {
     static const FOURCC fccs[]={FOURCC_WVC1,FOURCC_WMV3,FOURCC_MSS1,FOURCC_MSS2,FOURCC_WMVP,FOURCC_WVP2,0};
     return fccs;
    }
   case CODEC_ID_VP5:
    {
     static const FOURCC fccs[]={FOURCC_VP50,0};
     return fccs;
    }
   case CODEC_ID_VP6:
    {
     static const FOURCC fccs[]={FOURCC_VP60,FOURCC_VP61,FOURCC_VP62,0};
     return fccs;
    }
   case CODEC_ID_VP6F:
    {
     static const FOURCC fccs[]={FOURCC_VP6F,FOURCC_FLV4,0};
     return fccs;
    }
   case CODEC_ID_CAVS:
    {
     static const FOURCC fccs[]={FOURCC_CAVS,0};
     return fccs;
    }
   case CODEC_ID_VMNC:
    {
     static const FOURCC fccs[]={FOURCC_VMNC,0};
     return fccs;
    }
   case CODEC_ID_MJPEG:
    {
     static const FOURCC fccs[]={FOURCC_MJPG,0};
     return fccs;
    }
   case CODEC_ID_LJPEG:
    {
     static const FOURCC fccs[]={FOURCC_LJPG,0};
     return fccs;
    }
   case CODEC_ID_HUFFYUV:
    {
     static const FOURCC fccs[]={FOURCC_HFYU,FOURCC_FFVH,0};
     return fccs;
    }
   case CODEC_ID_FFV1:
    {
     static const FOURCC fccs[]={FOURCC_FFV1,0};
     return fccs;
    }
   case CODEC_ID_DVVIDEO:
    {
     /* lowercase FourCC 'dvsd' for compatibility with MS DV decoder */
     static const FOURCC fccs[]={mmioFOURCC('d','v','s','d'),FOURCC_DVSD,mmioFOURCC('d','v','2','5'),FOURCC_DV25,mmioFOURCC('d','v','5','0'),FOURCC_DV50,0};
     return fccs;
    }
   case CODEC_ID_VP3:
    {
     static const FOURCC fccs[]={FOURCC_VP31,0};
     return fccs;
    }
   case CODEC_ID_THEORA:
   case CODEC_ID_THEORA_LIB:
    {
     static const FOURCC fccs[]={FOURCC_THEO,0};
     return fccs;
    }
   case CODEC_ID_H264:
   case CODEC_ID_X264:
   case CODEC_ID_X264_LOSSLESS:
    {
     static const FOURCC fccs[]={FOURCC_H264,FOURCC_X264,FOURCC_VSSH,FOURCC_DAVC,FOURCC_AVC1,0};
     return fccs;
    }
   case CODEC_ID_RV10:
    {
     static const FOURCC fccs[]={FOURCC_RV10,0};
     return fccs;
    }
   case CODEC_ID_RV20:
    {
     static const FOURCC fccs[]={FOURCC_RV10,0};
     return fccs;
    }
   case CODEC_ID_SVQ1:
    {
     static const FOURCC fccs[]={FOURCC_SVQ1,0};
     return fccs;
    }
   case CODEC_ID_SVQ3:
    {
     static const FOURCC fccs[]={FOURCC_SVQ3,0};
     return fccs;
    }
   case CODEC_ID_RPZA:
    {
     static const FOURCC fccs[]={FOURCC_RPZA,0};
     return fccs;
    }
   case CODEC_ID_FRAPS:
    {
     static const FOURCC fccs[]={FOURCC_FPS1,0};
     return fccs;
    }
   case CODEC_ID_AASC:
    {
     static const FOURCC fccs[]={FOURCC_AASC,0};
     return fccs;
    }
   case CODEC_ID_VIXL:
    {
     static const FOURCC fccs[]={FOURCC_VIXL,0};
     return fccs;
    }
   case CODEC_ID_ASV1:
    {
     static const FOURCC fccs[]={FOURCC_ASV1,0};
     return fccs;
    }
   case CODEC_ID_ASV2:
    {
     static const FOURCC fccs[]={FOURCC_ASV2,0};
     return fccs;
    }
   case CODEC_ID_CYUV:
    {
     static const FOURCC fccs[]={FOURCC_CYUV,0};
     return fccs;
    }
    case CODEC_ID_INDEO2:
    {
     static const FOURCC fccs[]={FOURCC_RT21,0};
     return fccs;
    }
   case CODEC_ID_INDEO3:
    {
     static const FOURCC fccs[]={FOURCC_IV31,FOURCC_IV32,0};
     return fccs;
    }
   case CODEC_ID_MSVIDEO1:
    {
     static const FOURCC fccs[]={FOURCC_CRAM,0};
     return fccs;
    }
   case CODEC_ID_CINEPAK:
    {
     static const FOURCC fccs[]={FOURCC_CVID,0};
     return fccs;
    }
   case CODEC_ID_VCR1:
    {
     static const FOURCC fccs[]={FOURCC_VCR1,0};
     return fccs;
    }
   case CODEC_ID_MSZH:
    {
     static const FOURCC fccs[]={FOURCC_MSZH,0};
     return fccs;
    }
   case CODEC_ID_ZLIB:
    {
     static const FOURCC fccs[]={FOURCC_ZLIB,0};
     return fccs;
    }
   case CODEC_ID_CLJR:
    {
     static const FOURCC fccs[]={FOURCC_CLJR,0};
     return fccs;
    }
   case CODEC_ID_Y800:
    {
     static const FOURCC fccs[]={FOURCC_Y800,0};
     return fccs;
    }
   case CODEC_ID_SNOW:
    {
     static const FOURCC fccs[]={FOURCC_SNOW,0};
     return fccs;
    }
   case CODEC_ID_FLV1:
    {
     static const FOURCC fccs[]={FOURCC_FLV1,0};
     return fccs;
    }
   case CODEC_ID_444P:
    {
     static const FOURCC fccs[]={FOURCC_444P,FOURCC_YV24,0};
     return fccs;
    }
   case CODEC_ID_422P:
    {
     static const FOURCC fccs[]={FOURCC_422P,FOURCC_YV16,0};
     return fccs;
    }
   case CODEC_ID_411P:
    {
     static const FOURCC fccs[]={FOURCC_411P,FOURCC_Y41B,0};
     return fccs;
    }
   case CODEC_ID_410P:
    {
     static const FOURCC fccs[]={FOURCC_410P,0};
     return fccs;
    }
   default:
    {
     static const FOURCC fccs[]={0,0};
     return fccs;
    }
  }
}

const char_t *getCodecName(CodecID codecId)
{
 switch (codecId)
  {
   #define CODEC_OP(codecEnum,codecId,codecName) case codecEnum:return _l(codecName);
   FFDSHOW_CODECS
   #undef CODEC_OP
   default:return _l("unknown");
  }
}
