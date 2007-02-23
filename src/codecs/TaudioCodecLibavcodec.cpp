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
#include "vorbis/vorbisformat.h"

TaudioCodecLibavcodec::TaudioCodecLibavcodec(IffdshowBase *deci,IdecAudioSink *Isink):
 Tcodec(deci),
 TaudioCodec(deci,Isink)
{
 avctx=NULL;avcodec=NULL;codecinited=false;
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
   if (mt.formattype==FORMAT_WaveFormatEx)
    {
     const WAVEFORMATEX *wfex=(const WAVEFORMATEX*)mt.pbFormat;
     avctx->bit_rate=wfex->nAvgBytesPerSec*8;
     avctx->bits_per_sample=wfex->wBitsPerSample;
     avctx->block_align=wfex->nBlockAlign;
    }
   else
    {
     avctx->bit_rate=fmt.avgBytesPerSec()*8;
     avctx->bits_per_sample=fmt.bitsPerSample();
     avctx->block_align=fmt.blockAlign();
    }
   if (codecId==CODEC_ID_WMAV1 || codecId==CODEC_ID_WMAV2)
    {
     bpssum=lastbps=avctx->bit_rate/1000;
     numframes=1;
    }
   Textradata extradata(mt,FF_INPUT_BUFFER_PADDING_SIZE);
   if (codecId==CODEC_ID_FLAC && extradata.size>=4 && *(FOURCC*)extradata.data==mmioFOURCC('f','L','a','C')) // HACK
    {
     avctx->extradata=extradata.data+8;
     avctx->extradata_size=34;
    }
   else if (codecId==CODEC_ID_COOK && mt.formattype==FORMAT_WaveFormatEx && mt.pbFormat)
    {
     avctx->extradata=mt.pbFormat+sizeof(WAVEFORMATEX);
     avctx->extradata_size=mt.cbFormat-sizeof(WAVEFORMATEX);
     for (;avctx->extradata_size;avctx->extradata=(uint8_t*)avctx->extradata+1,avctx->extradata_size--)
      if (memcmp(avctx->extradata,"cook",4)==0)
       {
        avctx->extradata=(uint8_t*)avctx->extradata+12;
        avctx->extradata_size-=12;
        break;
       }
    }
   else
    {
     avctx->extradata=extradata.data;
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
     case SAMPLE_FMT_FLT:fmt.sf=TsampleFormat::SF_FLOAT32;break;
    }
   isGain=deci->getParam2(IDFF_vorbisgain);
   return true;
  }
 else
  return false;
}
TaudioCodecLibavcodec::~TaudioCodecLibavcodec()
{
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
  strncpy(buf,text<char_t>(avcodec->name),buflen);
 buf[buflen-1]='\0';
}

HRESULT TaudioCodecLibavcodec::decode(TbyteBuffer &src0)
{
 int size=(int)src0.size();
 unsigned char *src=&*src0.begin();
 while (size>0)
  {
   int dstLength=AVCODEC_MAX_AUDIO_FRAME_SIZE;
   void *dst=(void*)getDst(dstLength);
   int ret=libavcodec->avcodec_decode_audio(avctx,dst,&dstLength,src,size);
   if (ret<0 || (ret==0 && dstLength==0))
    break;
   HRESULT hr=sinkA->deliverDecodedSample(dst,dstLength/fmt.blockAlign(),fmt,isGain?avctx->postgain:1);
   if (hr!=S_OK) return hr;
   size-=ret;
   src+=ret;
  }
 src0.clear();
 return S_OK;
}

bool TaudioCodecLibavcodec::onSeek(REFERENCE_TIME segmentStart)
{
 return avctx?(libavcodec->avcodec_flush_buffers(avctx),true):false;
}
