/*
 * Copyright (c) 2004-2006 Milan Cutka
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "stdafx.h"
#include "TsampleFormat.h"
#include "ffdshow_mediaguids.h"
#include "xiph/vorbis/vorbisformat.h"
#include "dsutil.h"


float TsampleFormat::os_version=0;

const int TsampleFormat::standardChannelMasks[]=
{
 SPEAKER_FRONT_CENTER,
 SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT,
 SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_CENTER,
 SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT,
 SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT,
 SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT,
 SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT|SPEAKER_BACK_CENTER|SPEAKER_LOW_FREQUENCY,
 SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT|SPEAKER_LOW_FREQUENCY
};

TsampleFormat::TsampleFormat(const WAVEFORMATEX &wfex,bool wfextcheck,const GUID *subtype):pcm_be(false)
{
 init(wfex,wfextcheck,subtype);
}
void TsampleFormat::init(const WAVEFORMATEX &wfex,bool wfextcheck,const GUID *subtype)
{
 if (wfextcheck && wfex.wFormatTag==WAVE_FORMAT_EXTENSIBLE)
  init(*(const WAVEFORMATEXTENSIBLE*)&wfex,subtype);
 else
  {
   freq=wfex.nSamplesPerSec;
   setChannels(wfex.nChannels,0);
   if (wfex.wFormatTag==WAVE_FORMAT_IEEE_FLOAT)
    sf=SF_FLOAT32;
   else if (wfex.wFormatTag==WAVE_FORMAT_DOLBY_AC3_SPDIF)
    sf=SF_AC3;
   else if (subtype && (*subtype==MEDIASUBTYPE_IN32 || *subtype==MEDIASUBTYPE_in32 || *subtype==MEDIASUBTYPE_IN24 || *subtype==MEDIASUBTYPE_in24 || *subtype==MEDIASUBTYPE_FL32 || *subtype==MEDIASUBTYPE_fl32 || *subtype==MEDIASUBTYPE_FL64 || *subtype==MEDIASUBTYPE_fl64 || *subtype==MEDIASUBTYPE_twos || *subtype==MEDIASUBTYPE_TWOS))
    {
     if (*subtype==MEDIASUBTYPE_IN32 || *subtype==MEDIASUBTYPE_in32 || *subtype==MEDIASUBTYPE_IN24 || *subtype==MEDIASUBTYPE_in24 || *subtype==MEDIASUBTYPE_FL32 || *subtype==MEDIASUBTYPE_fl32 || *subtype==MEDIASUBTYPE_FL64 || *subtype==MEDIASUBTYPE_fl64)
      {
       if (*subtype==MEDIASUBTYPE_IN32 || *subtype==MEDIASUBTYPE_in32)
        sf=SF_PCM32;
       else if (*subtype==MEDIASUBTYPE_IN24 || *subtype==MEDIASUBTYPE_in24)
        sf=SF_PCM24;
       else if (*subtype==MEDIASUBTYPE_FL32 || *subtype==MEDIASUBTYPE_fl32)
        sf=SF_FLOAT32;
       else if (*subtype==MEDIASUBTYPE_FL64 || *subtype==MEDIASUBTYPE_fl64)
        sf=SF_FLOAT64;
       Textradata extradata(wfex);
       if (extradata.data)
        {
         const uint8_t *enda=(const uint8_t*)memnstr(extradata.data,extradata.size,"enda"); //TODO: properly parse headers
         if (enda && *(uint16_t*)(enda+4)==0)
          pcm_be=true;
        }
      }
     else if (*subtype==MEDIASUBTYPE_twos || *subtype==MEDIASUBTYPE_TWOS)
      {
       sf=SF_PCM16;
       pcm_be=true;
      }
    }
   else
    switch (wfex.wBitsPerSample)
     {
      case 8:sf=SF_PCM8;break;
      case 0:
      case 16:
      default:sf=SF_PCM16;break;
      case 20:sf=SF_LPCM20;break;
      case 24:sf=SF_PCM24;break;
      case 32:sf=SF_PCM32;break;
     }
    dolby=DOLBY_NO;
  }
}
TsampleFormat::TsampleFormat(const WAVEFORMATEXTENSIBLE &wfexten,const GUID *subtype):pcm_be(false)
{
 init(wfexten,subtype);
}
void TsampleFormat::init(const WAVEFORMATEXTENSIBLE &wfexten,const GUID *subtype)
{
 init(wfexten.Format,false,subtype);
 setChannels(wfexten.Format.nChannels,wfexten.dwChannelMask);
 if (wfexten.SubFormat==MEDIASUBTYPE_IEEE_FLOAT)
  sf=SF_FLOAT32;
}
void TsampleFormat::init(const WAVEFORMATEXTENSIBLE_IEC61937 &wfexten_iec61937,const GUID *subtype)
{
 init(wfexten_iec61937.FormatExt,subtype);
 /*wfexten_iec61937.dwAverageBytesPerSec=0;
 wfexten_iec61937.dwEncodedSamplesPerSec=0;
 wfexten_iec61937.dwEncodedChannelCount=(wfexten_iec61937.FormatExt.Format.nChannels;*/
}
TsampleFormat::TsampleFormat(const VORBISFORMAT &vf):pcm_be(false)
{
 init(vf);
}
void TsampleFormat::init(const VORBISFORMAT &vf)
{
 freq=vf.nSamplesPerSec;
 sf=SF_PCM16;
 setChannels(vf.nChannels,0);
 dolby=DOLBY_NO;
}
TsampleFormat::TsampleFormat(const VORBISFORMAT2 &vf2):pcm_be(false)
{
 init(vf2);
}
void TsampleFormat::init(const VORBISFORMAT2 &vf2)
{
 freq=vf2.SamplesPerSec;
 switch (vf2.BitsPerSample)
  {
   case 0:
   case 16:
   default:sf=SF_PCM16;break;
   case 24:sf=SF_PCM24;break;
   case 32:sf=SF_PCM32;break;
  }
 setChannels(vf2.Channels,0);
 dolby=DOLBY_NO;
}
TsampleFormat::TsampleFormat(const VORBISFORMATILL &vfIll):pcm_be(false)
{
 init(vfIll);
}
void TsampleFormat::init(const VORBISFORMATILL &vfIll)
{
 freq=vfIll.samplesPerSec;
 sf=SF_PCM16;
 setChannels(vfIll.numChannels,0);
 dolby=DOLBY_NO;
}
TsampleFormat::TsampleFormat(const AM_MEDIA_TYPE &mt):pcm_be(false)
{
 if (mt.formattype==FORMAT_VorbisFormat)
  init(*(const VORBISFORMAT*)mt.pbFormat);
 else if (mt.formattype==FORMAT_VorbisFormat2)
  init(*(const VORBISFORMAT2*)mt.pbFormat);
 else if (mt.formattype==FORMAT_VorbisFormatIll)
  init(*(const VORBISFORMATILL*)mt.pbFormat);
 else if (mt.formattype==FORMAT_WaveFormatEx)
  /* commented by albain : the new structure does not work properly. Under investigation
  init(*(const WAVEFORMATEXTENSIBLE_IEC61937*)mt.pbFormat,&mt.subtype);*/
  init(*(const WAVEFORMATEX*)mt.pbFormat,true,&mt.subtype);
 else
  {
   nchannels=NULL;
   sf=SF_NULL;
  }
}

int TsampleFormat::sf_bestMatch(int sfIn,int wantedSFS)
{
 const int *bestsfs=NULL;
 switch (sfIn)
  {
   case SF_PCM16:
    {
     static const int best[]=
      {
       SF_PCM32,
       SF_PCM24,
       SF_FLOAT32,
       SF_NULL
      };
     bestsfs=best;
     break;
    }
   case SF_PCM24:
    {
     static const int best[]=
      {
       SF_PCM32,
       SF_PCM16,
       SF_FLOAT32,
       SF_NULL
      };
     bestsfs=best;
     break;
    }
   case SF_PCM32:
    {
     static const int best[]=
      {
       SF_PCM16,
       SF_PCM24,
       SF_FLOAT32,
       SF_NULL
      };
     bestsfs=best;
     break;
    }
   case SF_PCM8:
    {
     static const int best[]=
      {
       SF_PCM16,
       SF_PCM32,
       SF_NULL
      };
     bestsfs=best;
     break;
    }
   case SF_FLOAT32:
    {
     static const int best[]=
      {
       SF_PCM32,
       SF_PCM16,
       SF_PCM24,
       SF_NULL
      };
     bestsfs=best;
     break;
    }
   default:return SF_NULL;
  }
 while (*bestsfs)
  {
   if (*bestsfs&wantedSFS)
    return *bestsfs;
   bestsfs++;
  }
 return SF_NULL;
}

DWORD TsampleFormat::getPCMformat(const CMediaType &mtIn,DWORD def)
{
 if (*mtIn.FormatType()!=FORMAT_WaveFormatEx)
  return def;
 const WAVEFORMATEX *wfex=(const WAVEFORMATEX*)mtIn.Format();
 if (wfex->wFormatTag!=WAVE_FORMAT_EXTENSIBLE && wfex->wFormatTag!=WAVE_FORMAT_PCM && wfex->wFormatTag!=WAVE_FORMAT_IEEE_FLOAT && mtIn.subtype!=MEDIASUBTYPE_RAW)
  return def;
 TsampleFormat sf(mtIn);
 switch (sf.sf)
  {
   case SF_NULL:return def;
   case SF_PCM8:return WAVE_FORMAT_PCM8;
   case SF_PCM16:return WAVE_FORMAT_PCM16;
   case SF_PCM24:return WAVE_FORMAT_PCM24;
   case SF_PCM32:return WAVE_FORMAT_PCM32;
   case SF_FLOAT32:return WAVE_FORMAT_FLOAT32;
   case SF_FLOAT64:return WAVE_FORMAT_FLOAT64;
  }
 return def;
}

WAVEFORMATEXTENSIBLE TsampleFormat::toWAVEFORMATEXTENSIBLE(bool alwayextensible) const
{
 WAVEFORMATEXTENSIBLE wfex;
 memset(&wfex,0,sizeof(wfex));
 WAVEFORMATEX *wfe=&wfex.Format;
 if (sf==SF_FLOAT32)
  wfe->wFormatTag=(WORD)WAVE_FORMAT_IEEE_FLOAT;
 else if (sf==SF_LPCM16)
  wfe->wFormatTag=(WORD)WAVE_FORMAT_UNKNOWN;
 else if (sf==SF_TRUEHD || sf==SF_DTSHD || sf==SF_EAC3)
  wfe->wFormatTag=(WORD)WAVE_FORMAT_EXTENSIBLE;
 else
  wfe->wFormatTag=(WORD)WAVE_FORMAT_PCM;
 wfe->nChannels=WORD(nchannels);
 wfe->nSamplesPerSec=freq;
 wfe->wBitsPerSample=(WORD)bitsPerSample();
 wfe->nBlockAlign=WORD(wfe->nChannels*wfe->wBitsPerSample/8);
 wfe->nAvgBytesPerSec=wfe->nSamplesPerSec*wfe->nBlockAlign;

 // FIXME: 24/32 bit only seems to work with WAVE_FORMAT_EXTENSIBLE
 int dwChannelMask;
 if (channelmask==0 && (sf==TsampleFormat::SF_PCM24 || sf==TsampleFormat::SF_PCM32 || nchannels>2))
  dwChannelMask=makeChannelMask2();
 else
  dwChannelMask=channelmask;

 if (!alwayextensible && dwChannelMask==standardChannelMasks[nchannels-1])
  dwChannelMask=0;

 if (dwChannelMask)
  {
   wfex.Format.wFormatTag=WAVE_FORMAT_EXTENSIBLE;
   wfex.Format.cbSize=sizeof(wfex)-sizeof(wfex.Format);
   wfex.dwChannelMask=dwChannelMask;
   wfex.Samples.wValidBitsPerSample=wfex.Format.wBitsPerSample;
   wfex.SubFormat=sf==SF_FLOAT32?MEDIASUBTYPE_IEEE_FLOAT:MEDIASUBTYPE_PCM;
  }
 return wfex;
}

// New Windows 7 structure
WAVEFORMATEXTENSIBLE_IEC61937 TsampleFormat::toWAVEFORMATEXTENSIBLE_IEC61937(bool alwayextensible) const
{
 WAVEFORMATEXTENSIBLE_IEC61937 wfex_iec61937;
 memset(&wfex_iec61937,0,sizeof(wfex_iec61937));
 wfex_iec61937.FormatExt=toWAVEFORMATEXTENSIBLE(alwayextensible);
 wfex_iec61937.dwEncodedSamplesPerSec=freq;
 wfex_iec61937.dwEncodedChannelCount=nchannels;
 wfex_iec61937.dwAverageBytesPerSec=0;

 switch (sf)
 {
  case SF_TRUEHD:
   wfex_iec61937.FormatExt.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP;
   wfex_iec61937.FormatExt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
   wfex_iec61937.FormatExt.Format.wBitsPerSample = 16;
   wfex_iec61937.FormatExt.Format.cbSize = 34;
   break;
  case SF_DTSHD:
   wfex_iec61937.FormatExt.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD;
   wfex_iec61937.FormatExt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
   wfex_iec61937.FormatExt.Format.wBitsPerSample = 16;
   wfex_iec61937.FormatExt.Format.cbSize = 34;
   break;
  case SF_EAC3:
   wfex_iec61937.FormatExt.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS;
   wfex_iec61937.FormatExt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
   wfex_iec61937.FormatExt.Format.wBitsPerSample = 16;
   wfex_iec61937.FormatExt.Format.cbSize = 34;
   break;
 }
 return wfex_iec61937;
}

float TsampleFormat::getOSVersion(void)
{
 if (os_version==0)
 {
   OSVERSIONINFO osvi;
   BOOL bIsWindowsXPorLater;

   ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

   GetVersionEx(&osvi);
   os_version=(float)osvi.dwMajorVersion+(float)osvi.dwMajorVersion/10;
 }
 return os_version;
}

CMediaType TsampleFormat::toCMediaType(bool alwaysextensible) const
{
 CMediaType mt;
 /*if (sf==SF_LPCM16)
  mt.majortype=MEDIATYPE_MPEG2_PES;
 else*/
  mt.majortype=MEDIATYPE_Audio;
 if (sf==SF_FLOAT32)
  mt.subtype=MEDIASUBTYPE_IEEE_FLOAT;
 else if (sf==SF_LPCM16)
  mt.subtype=MEDIASUBTYPE_DVD_LPCM_AUDIO;
 else if (sf==SF_TRUEHD)
  mt.subtype=MEDIASUBTYPE_DOLBY_TRUEHD;
 else if (sf==SF_DTSHD)
  mt.subtype=MEDIASUBTYPE_DTS_HD;
 else if (sf==SF_EAC3)
  mt.subtype=MEDIASUBTYPE_DOLBY_DDPLUS;
 else
  mt.subtype=MEDIASUBTYPE_PCM;

 mt.formattype=FORMAT_WaveFormatEx;
 WAVEFORMATEXTENSIBLE_IEC61937 wfex_iec61937=toWAVEFORMATEXTENSIBLE_IEC61937(alwaysextensible);
 // cbSize = 54
 mt.SetFormat((BYTE*)&wfex_iec61937,
    sizeof(wfex_iec61937.FormatExt.Format)+sizeof(wfex_iec61937.FormatExt)+wfex_iec61937.FormatExt.Format.cbSize); 
 return mt;
}

CMediaType TsampleFormat::createMediaTypeSPDIF(unsigned int frequency)
{
 // S/PDIF mode mandates 2 Channels and 16 bits per sample, regardless of the underlying format.
 // S/PDIF interface supports three standard sample rates: 48 kHz, 44.1 kHz and 32 kHz..
 // the underlying format can have different number of bits per sample or different sample rate, such in the case of DTS 96/24, (transferred over S/PDIF link at 48kHz, 16 bits per sample)
 // if possible, it's best to select S/PDIF sample rate based on the underlying sample rate.
 CMediaType mt=TsampleFormat(SF_PCM16,frequency,2).toCMediaType();
 ((WAVEFORMATEX*)mt.pbFormat)->wFormatTag=WAVE_FORMAT_DOLBY_AC3_SPDIF;
 return mt;
}

const char_t* TsampleFormat::getSpeakerName(int speaker,bool shrt)
{
 switch (speaker)
  {
    case SPEAKER_FRONT_LEFT:return shrt?_l("L"):_l("front left");
    case SPEAKER_FRONT_RIGHT:return shrt?_l("R"):_l("front right");
    case SPEAKER_FRONT_CENTER:return shrt?_l("C"):_l("front center");
    case SPEAKER_LOW_FREQUENCY:return _l("LFE");
    case SPEAKER_BACK_LEFT:return _l("back left");
    case SPEAKER_BACK_RIGHT:return _l("back right");
    case SPEAKER_FRONT_LEFT_OF_CENTER:return _l("front left of center");
    case SPEAKER_FRONT_RIGHT_OF_CENTER:return _l("front right of center");
    case SPEAKER_BACK_CENTER:return _l("back center");
    case SPEAKER_SIDE_LEFT:return _l("side left");
    case SPEAKER_SIDE_RIGHT:return _l("side right");
    case SPEAKER_TOP_CENTER:return _l("top center");
    case SPEAKER_TOP_FRONT_LEFT:return _l("top front left");
    case SPEAKER_TOP_FRONT_CENTER:return _l("top front center");
    case SPEAKER_TOP_FRONT_RIGHT:return _l("top front right");
    case SPEAKER_TOP_BACK_LEFT:return _l("top back left");
    case SPEAKER_TOP_BACK_CENTER:return _l("top back center");
    case SPEAKER_TOP_BACK_RIGHT:return _l("top back right");
    default:return _l("unknown");
  }
}
void TsampleFormat::getSpeakersDescr(char_t *buf,size_t buflen,bool shrt) const
{
 buf[0]='\0';
 for (unsigned int i=0;i<nchannels;i++)
  strncatf(buf,buflen,_l("%s,"),getSpeakerName(speakers[i],shrt));
 buf[buflen-1]='\0';
 size_t len=strlen(buf);
 if (len && buf[len-1]==',') buf[len-1]='\0';
}

// Static method that returns the output sample format according to the input stream to passthrough
int TsampleFormat::getSampleFormat(CodecID codecId)
{
 switch(codecId)
 {
  case CODEC_ID_BITSTREAM_TRUEHD: return TsampleFormat::SF_TRUEHD;
  case CODEC_ID_BITSTREAM_DTSHD: return TsampleFormat::SF_DTSHD;
  case CODEC_ID_SPDIF_EAC3: //return TsampleFormat::SF_EAC3; Try AC3
  case CODEC_ID_SPDIF_AC3:
  case CODEC_ID_SPDIF_DTS:return TsampleFormat::SF_AC3;
  default:return TsampleFormat::SF_PCM16;
 }
}

