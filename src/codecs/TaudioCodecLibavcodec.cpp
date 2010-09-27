/*
 * Copyright (c) 2003-2006 Milan Cutka
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
#include "TaudioCodecLibavcodec.h"
#include "IffdshowBase.h"
#include "IffdshowDec.h"
#include "IffdshowDecAudio.h"
#include "dsutil.h"
#include "ffdshow_mediaguids.h"
#include "xiph/vorbis/vorbisformat.h"
#include "ffmpeg/libavcodec/avcodec.h"
#include "reorder_ch.h"

TaudioCodecLibavcodec::TaudioCodecLibavcodec(IffdshowBase *deci,IdecAudioSink *Isink):
 Tcodec(deci),
 TaudioCodec(deci,Isink)
{
 avctx=NULL;avcodec=NULL;codecinited=false;contextinited=false;parser=NULL;src_ch_layout = AF_CHANNEL_LAYOUT_MPLAYER_DEFAULT;
}

bool TaudioCodecLibavcodec::init(const CMediaType &mt)
{
 deci->getLibavcodec(&libavcodec);
 if (libavcodec->ok)
  {
   avcodec=libavcodec->avcodec_find_decoder(codecId);
   if (!avcodec) return false;
   if (codecId==CODEC_ID_AMR_NB) //HACK: 3ivx splitter doesn't report correct frequency/number of channels
    {
     fmt.setChannels(1);
     fmt.freq=8000;
    }
   avctx=libavcodec->avcodec_alloc_context();
   avctx->sample_rate=fmt.freq;
   avctx->channels=fmt.nchannels;
   avctx->codec_id=codecId;

   if (parser)
   {
       libavcodec->av_parser_close(parser);
       parser = NULL;
   }
   parser=libavcodec->av_parser_init(codecId);

   if (mt.formattype==FORMAT_WaveFormatEx)
    {
     const WAVEFORMATEX *wfex=(const WAVEFORMATEX*)mt.pbFormat;
     avctx->bit_rate=wfex->nAvgBytesPerSec*8;
     avctx->bits_per_coded_sample=wfex->wBitsPerSample;
     if (wfex->wBitsPerSample == 0 && codecId==CODEC_ID_COOK)
      avctx->bits_per_coded_sample = 16;
     avctx->block_align=wfex->nBlockAlign;
    }
   else
    {
     avctx->bit_rate=fmt.avgBytesPerSec()*8;
     avctx->bits_per_coded_sample=fmt.bitsPerSample();
     avctx->block_align=fmt.blockAlign();
    }

   bpssum=lastbps=avctx->bit_rate/1000;

   if (codecId==CODEC_ID_WMAV1 || codecId==CODEC_ID_WMAV2)
    {
     numframes=1;
    }
   Textradata extradata(mt,FF_INPUT_BUFFER_PADDING_SIZE);
   if (codecId==CODEC_ID_FLAC && extradata.size>=4 && *(FOURCC*)extradata.data==mmioFOURCC('f','L','a','C')) // HACK
    {
     avctx->extradata=(uint8_t*)extradata.data+8;
     avctx->extradata_size=34;
    }
   else if (codecId==CODEC_ID_COOK && mt.formattype==FORMAT_WaveFormatEx && mt.pbFormat)
    {
     /* Cook specifications : extradata is located after the real audio info, 4 or 5 depending on the version
        @See http://wiki.multimedia.cx/index.php?title=RealMedia the audio block information
        TODO : add support for header version 3
     */

     DWORD cbSize = ((const WAVEFORMATEX*)mt.pbFormat)->cbSize;
     BYTE* fmt = mt.Format() + sizeof(WAVEFORMATEX) + cbSize;
     
     for(int i = 0, len = mt.FormatLength() - (sizeof(WAVEFORMATEX) + cbSize); i < len-4; i++, fmt++)
		   {
			   if(fmt[0] == '.' || fmt[1] == 'r' || fmt[2] == 'a')
				   break;
		   }

     m_realAudioInfo = *(TrealAudioInfo*) fmt;
     m_realAudioInfo.bswap();
     
     BYTE* p = NULL;
     if(m_realAudioInfo.version2 == 4)
		   {
      // Skip the TrealAudioInfo4 data
      p = (BYTE*)((TrealAudioInfo4*)fmt+1);
			   int len = *p++; p += len; len = *p++; p += len; 
			   ASSERT(len == 4);
      //DPRINTF(_l("TaudioCodecLibavcodec cook version 4"));
		   }
		   else if(m_realAudioInfo.version2 == 5)
		   {
      // Skip the TrealAudioInfo5 data
			   p = (BYTE*)((TrealAudioInfo5*)fmt+1);
      //DPRINTF(_l("TaudioCodecLibavcodec cook version 5"));
		   }
		   else
		   {
			   return VFW_E_TYPE_NOT_ACCEPTED;
		   }

     /* Cook specifications : after the end of TrealAudio4 or TrealAudio5 structure, we have this :
       byte[3]  Unknown
       #if version == 5
        byte     Unknown
       #endif
        dword    Codec extradata length
        byte[]   Codec extradata
     */
     
     // Skip the 3 unknown bytes
     p += 3;
		   // + skip 1 byte if version 5
     if(m_realAudioInfo.version2 == 5) p++;

     // Extradata size : next 4 bytes
     avctx->extradata_size = std::min((DWORD)((mt.Format() + mt.FormatLength()) - (p + 4)), *(DWORD*)p);
     // Extradata starts after the 4 bytes size
		   avctx->extradata = (uint8_t*)(p + 4);
     avctx->block_align = m_realAudioInfo.coded_frame_size;
    }
   else
    {
     avctx->extradata=(uint8_t*)extradata.data;
     avctx->extradata_size=(int)extradata.size;
    }
   if (codecId==CODEC_ID_VORBIS && mt.formattype==FORMAT_VorbisFormat2)
    {
     const VORBISFORMAT2 *vf2=(const VORBISFORMAT2*)mt.pbFormat;
     avctx->vorbis_header_size[0]=vf2->HeaderSize[0];
     avctx->vorbis_header_size[1]=vf2->HeaderSize[1];
     avctx->vorbis_header_size[2]=vf2->HeaderSize[2];
    }
   if (libavcodec->avcodec_open(avctx,avcodec)<0) return false;
   codecinited=true;
   switch (avctx->sample_fmt)
    {
     case SAMPLE_FMT_S16:fmt.sf=TsampleFormat::SF_PCM16;break;
     case SAMPLE_FMT_S32:fmt.sf=TsampleFormat::SF_PCM32;break;
     case SAMPLE_FMT_FLT:fmt.sf=TsampleFormat::SF_FLOAT32;break;
     case SAMPLE_FMT_DBL:fmt.sf=TsampleFormat::SF_FLOAT64;break;
    }
   isGain=deci->getParam2(IDFF_vorbisgain);
   updateChannelMapping();

   libavcodec->av_log_set_level(AV_LOG_QUIET);

   // Handle truncated streams
   if(avctx->codec->capabilities & CODEC_CAP_TRUNCATED)
        avctx->flags|=CODEC_FLAG_TRUNCATED;

   return true;
  }
 else
  return false;
}
TaudioCodecLibavcodec::~TaudioCodecLibavcodec()
{ 
 if (parser)
 {
   libavcodec->av_parser_close(parser);
   parser = NULL;
 }
 if (avctx)
  {
   if (codecinited) libavcodec->avcodec_close(avctx);codecinited=false;
   libavcodec->av_free(avctx);
  }
 if (libavcodec) libavcodec->Release();
}
const char_t* TaudioCodecLibavcodec::getName(void) const
{
 return _l("libavcodec");
}
void TaudioCodecLibavcodec::getInputDescr1(char_t *buf,size_t buflen) const
{

if (avcodec)
{
    if (!strcmp(text<char_t>(avcodec->name), _l("mp3")))
        ff_strncpy(buf,_l("MP3"),buflen);
    else if (!strcmp(text<char_t>(avcodec->name), _l("mp2")))
        ff_strncpy(buf,_l("MP2"),buflen);
    else if (!strcmp(text<char_t>(avcodec->name), _l("mp1")))
        ff_strncpy(buf,_l("MP1"),buflen);
    else if (!strcmp(text<char_t>(avcodec->name), _l("mp3float")))
        ff_strncpy(buf,_l("MP3"),buflen);
    else if (!strcmp(text<char_t>(avcodec->name), _l("mp2float")))
        ff_strncpy(buf,_l("MP2"),buflen);
    else if (!strcmp(text<char_t>(avcodec->name), _l("mp1float")))
        ff_strncpy(buf,_l("MP1"),buflen);
    else if (!strcmp(text<char_t>(avcodec->name), _l("ac3")))
        ff_strncpy(buf,_l("AC3"),buflen);
    else if (!strcmp(text<char_t>(avcodec->name), _l("eac3")))
        ff_strncpy(buf,_l("E-AC3"),buflen);
    else if (!strcmp(text<char_t>(avcodec->name), _l("truehd")))
        ff_strncpy(buf,_l("TrueHD"),buflen);
    else if (!strcmp(text<char_t>(avcodec->name), _l("mlp")))
        ff_strncpy(buf,_l("MLP"),buflen);
    else if (!strcmp(text<char_t>(avcodec->name), _l("dca")))
        ff_strncpy(buf,_l("DTS"),buflen);
    else if (!strcmp(text<char_t>(avcodec->name), _l("aac")))
        ff_strncpy(buf,_l("AAC"),buflen);
    else if (!strcmp(text<char_t>(avcodec->name), _l("vorbis")))
        ff_strncpy(buf,_l("Vorbis"),buflen);
    else if (!strcmp(text<char_t>(avcodec->name), _l("flac")))
        ff_strncpy(buf,_l("FLAC"),buflen);
    else if (!strcmp(text<char_t>(avcodec->name), _l("tta")))
        ff_strncpy(buf,_l("TTA"),buflen);
    else if (!strcmp(text<char_t>(avcodec->name), _l("wmav2")))
        ff_strncpy(buf,_l("WMAV2"),buflen);
    else if (!strcmp(text<char_t>(avcodec->name), _l("cook")))
        ff_strncpy(buf,_l("COOK"),buflen);
    else
        ff_strncpy(buf,(const char_t *)text<char_t>(avcodec->name),buflen);
}
 buf[buflen-1]='\0';
}

HRESULT TaudioCodecLibavcodec::decode(TbyteBuffer &src0)
{
 /* A few explanations about the audio fields
  dstLength = size (in bytes) of the decoded audio data
  Block : audio data for all audio channels (=sample), so number of channels x sample_size
  Bits per sample = number of bits in one channel sample (6, 16, 24, 32, 64). The highest gives the best quality. Usually 16
  Sample size : bits per sample in bytes so bps/8
  Number of samples = number of blocks in the audio data = dstLength/block size, except for channels reordering
 */

 int size=(int)src0.size();
 unsigned char *src = size ? &src0[0] : NULL;
 int step_size = 0;

 // Dynamic range compression for AC3/DTS formats
 if (codecId == CODEC_ID_AC3 || codecId == CODEC_ID_EAC3 || codecId == CODEC_ID_DTS || codecId == CODEC_ID_MLP || codecId == CODEC_ID_TRUEHD)
 {
  if (deci->getParam2(IDFF_audio_decoder_DRC))
   {
    float drcLevel=(float)deci->getParam2(IDFF_audio_decoder_DRC_Level) / 100;
    avctx->drc_scale=drcLevel;
   }
  else
   {
    avctx->drc_scale=0.0;
   }
 }
 else if (codecId == CODEC_ID_COOK)  //Special behaviour for real audio cook decoder
 {
  int w = m_realAudioInfo.coded_frame_size;
  int h = m_realAudioInfo.sub_packet_h;
  int sps = m_realAudioInfo.sub_packet_size;
  step_size = w;
  size_t len = w*h;
  avctx->block_align = m_realAudioInfo.sub_packet_size;

  BYTE *pBuf = (BYTE*)srcBuf.resize(len*2);
  memcpy(pBuf+buflen,&*src0.begin(), src0.size());

  buflen += src0.size();

  if (buflen >= len)
  {
   src =(BYTE*) pBuf;
   BYTE *src_end = pBuf+len;

   if(sps > 0)
   {
    for(int y = 0; y < h; y++)
    {
     for(int x = 0, w2 = w / sps; x < w2; x++)
     {
      memcpy(src_end + sps*(h*x+((h+1)/2)*(y&1)+(y>>1)), src, sps);
      src += sps;
     }
    }
    src = pBuf + len;
    src_end = pBuf + len*2;
   }
   size = src_end - src;
   buflen = 0;
  }
  else
  {
   src0.clear();
   return S_OK;
  }
 }

 int maxLength=AVCODEC_MAX_AUDIO_FRAME_SIZE;

 while (size > 0)
  {
   if ((codecId == CODEC_ID_MLP || codecId == CODEC_ID_TRUEHD) && size == 1)
       break;  // workaround when skipping TrueHD in MPC?
   int dstLength=AVCODEC_MAX_AUDIO_FRAME_SIZE;
   void *dst=(void*)getDst(dstLength);
   AVPacket avpkt;
   libavcodec->av_init_packet(&avpkt);
   int dstLength2=AVCODEC_MAX_AUDIO_FRAME_SIZE;
   void *dst2=buf2.alloc(dstLength);

   int ret=0,ret2=0;
   // Use parser if available and do not use it for MLP/TrueHD stream
   if (parser && codecId != CODEC_ID_MLP && codecId != CODEC_ID_TRUEHD)
   {
       // Parse the input buffer src(size) and returned parsed data into dst2(dstLength2)
       ret=libavcodec->av_parser_parse2(parser, avctx, (uint8_t**)&dst2, &dstLength2,
           (const uint8_t*)src, size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
       // dstLength2==0 : nothing parsed
       if (ret<0 || (ret==0 && dstLength2==0))
           break;
       size-=ret;
       src+=ret;
       avpkt.data=(uint8_t*)dst2;
       avpkt.size=dstLength2;

       if (dstLength2 > 0) // This block could be parsed
       {
          // Decode the parsed buffer avpkt.data(avpkt.size) into dst(dstLength)
          ret2=libavcodec->avcodec_decode_audio3(avctx,(int16_t*)dst,&dstLength,&avpkt);
          // If nothing could be decoded, skip this data and continue
          if (ret2<0 || (ret2==0 &&dstLength==0))
            continue;
       }
       else // The buffer could not be parsed
            continue;
   }
   else
   {
       avpkt.data = src;
       avpkt.size = size;
       ret=libavcodec->avcodec_decode_audio3(avctx,(int16_t*)dst,&dstLength,&avpkt);
       if (ret<0 || (ret==0 && dstLength==0))
       {
           DPRINTF(_l("Unable to decode this frame"));
           TaudioParser *pAudioParser=NULL;
           this->sinkA->getAudioParser(&pAudioParser);
           if (pAudioParser!=NULL)
            pAudioParser->NewSegment();
          break;
       }
       
       size-=ret;
       src+=ret;
   }

   bpssum=lastbps=avctx->bit_rate/1000;

   // Correct the input media type from what has been parsed
   if (dstLength > 0)
   {
       fmt.setChannels(avctx->channels);
       fmt.freq=avctx->sample_rate;
       switch (avctx->sample_fmt)
        {
         case SAMPLE_FMT_S16:fmt.sf=TsampleFormat::SF_PCM16;break;
         case SAMPLE_FMT_S32:fmt.sf=TsampleFormat::SF_PCM32;break;
         case SAMPLE_FMT_FLT:fmt.sf=TsampleFormat::SF_FLOAT32;break;
         case SAMPLE_FMT_DBL:fmt.sf=TsampleFormat::SF_FLOAT64;break;
        }
   }

  // Correct channel mapping
  if (dstLength > 0 && fmt.nchannels >= 5)
  {
      reorder_channel_nch(dst,
         src_ch_layout,AF_CHANNEL_LAYOUT_FFDSHOW_DEFAULT,
         fmt.nchannels, dstLength * 8 /fmt.blockAlign(), fmt.bitsPerSample()/8);
  }

  if (dstLength > 0)
  {
      HRESULT hr=sinkA->deliverDecodedSample(dst, dstLength/fmt.blockAlign(),fmt,isGain?avctx->postgain:1);
      if (hr!=S_OK) 
         return hr;
  }
 }
 src0.clear();
 return S_OK;
}

void TaudioCodecLibavcodec::updateChannelMapping()
{
    src_ch_layout = AF_CHANNEL_LAYOUT_FFDSHOW_DEFAULT;
    if (!avctx->codec->name) return;
    char_t codec[255];
    ff_strncpy(codec, (const char_t *)text<char_t>(avctx->codec->name), countof(codec));
    codec[254]='\0';

    if (!stricmp(codec, _l("ac3")) || !stricmp(codec, _l("eac3")))
      src_ch_layout = AF_CHANNEL_LAYOUT_LAVC_AC3_DEFAULT;
    else if (!stricmp(codec, _l("dca")))
      src_ch_layout = AF_CHANNEL_LAYOUT_LAVC_DCA_DEFAULT;
    else if (!stricmp(codec, _l("libfaad")) || !stricmp(codec, _l("mpeg4aac")))
      src_ch_layout = AF_CHANNEL_LAYOUT_AAC_DEFAULT;
    else if (!stricmp(codec, _l("liba52")))
      src_ch_layout = AF_CHANNEL_LAYOUT_LAVC_LIBA52_DEFAULT;
    else if (!stricmp(codec, _l("vorbis")))
      src_ch_layout = AF_CHANNEL_LAYOUT_VORBIS_DEFAULT;
    else if (!stricmp(codec, _l("mlp")) || !stricmp(codec, _l("truehd")))
      src_ch_layout = AF_CHANNEL_LAYOUT_MLP_DEFAULT;
    else
      src_ch_layout = AF_CHANNEL_LAYOUT_FFDSHOW_DEFAULT;
}

bool TaudioCodecLibavcodec::onSeek(REFERENCE_TIME segmentStart)
{
 buflen = 0;
 return avctx?(libavcodec->avcodec_flush_buffers(avctx),true):false;
}
