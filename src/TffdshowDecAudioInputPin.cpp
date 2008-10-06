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
#include "TffdshowDecAudioInputPin.h"
#include "TffdshowDecAudio.h"
#include "ffdshow_mediaguids.h"
#include "TglobalSettings.h"
#include "ffdebug.h"

TffdshowDecAudioInputPin::TffdshowDecAudioInputPin(const char_t* pObjectName, TffdshowDecAudio* pFilter, HRESULT* phr, LPWSTR pName, int Inumber)
 :TinputPin(pObjectName,pFilter,phr,pName),
  filter(pFilter),
  number(Inumber),
  audio(NULL),
  jitter(0),
  prevpostgain(1.0f),
  insample_rtStart(REFTIME_INVALID),
  insample_rtStop(REFTIME_INVALID)
{
}
TffdshowDecAudioInputPin::~TffdshowDecAudioInputPin()
{
}

bool TffdshowDecAudioInputPin::init(const CMediaType &mt)
{
 DPRINTF(_l("TffdshowDecAudioInputPin::initAudio"));
 codecId=filter->getCodecId(mt);
 if (codecId==CODEC_ID_NONE) return false;
 strippacket=(mt.majortype==MEDIATYPE_DVD_ENCRYPTED_PACK || mt.majortype==MEDIATYPE_MPEG2_PES);
 if (mt.formattype==FORMAT_WaveFormatEx)
  {
   const WAVEFORMATEX *wfex=(const WAVEFORMATEX*)mt.pbFormat;
   searchdts=(filter->globalSettings->dts && filter->globalSettings->dtsinwav && wfex->wFormatTag==1 && wfex->nChannels==2 && wfex->nSamplesPerSec==44100);
  }
 filter->insf=mt;
 if (audio) {delete audio;codec=audio=NULL;}
 codec=audio=TaudioCodec::initSource(filter,this,codecId,filter->insf,mt);
 if (!audio) return false;
 filter->insf=audio->getInputSF();
 return true;
}

void TffdshowDecAudioInputPin::done(void)
{
 if (audio)
  {
   delete audio;
   codec=audio=NULL;
  }
 codecId=CODEC_ID_NONE;
}

STDMETHODIMP TffdshowDecAudioInputPin::EndFlush(void)
{
 DPRINTF(_l("TffdshowDecAudioInputPin::EndFlush"));
 CAutoLock cAutoLock(&m_csReceive);
 buf.clear();
 return TinputPin::EndFlush();
}

STDMETHODIMP TffdshowDecAudioInputPin::NewSegment(REFERENCE_TIME tStart,REFERENCE_TIME tStop,double dRate)
{
 DPRINTF(_l("TffdshowDecAudioInputPin::NewSegment"));
 CAutoLock cAutoLock(&m_csReceive);
 buf.clear();
 jitter=0;
 insample_rtStart = REFTIME_INVALID;
 insample_rtStop = REFTIME_INVALID;
 return TinputPin::NewSegment(tStart,tStop,dRate);
}

STDMETHODIMP TffdshowDecAudioInputPin::Receive(IMediaSample* pIn)
{
 if (this!=filter->inpin)
  return S_FALSE;

 CAutoLock cAutoLock(&m_csReceive);

 if (filter->IsStopped())
  return S_FALSE;

 HRESULT hr=TinputPin::Receive(pIn);
 if (hr!=S_OK)
  return hr;

 AM_SAMPLE2_PROPERTIES* const pProps=SampleProps();
 if (pProps->dwStreamId!=AM_STREAM_MEDIA)
  return filter->m_pOutput->Deliver(pIn);

 AM_MEDIA_TYPE *pmt=NULL;
 if (SUCCEEDED(pIn->GetMediaType(&pmt)) && pmt)
  {
   CMediaType mt(*pmt);
   SetMediaType(&mt);
   DeleteMediaType(pmt);
   pmt=NULL;
  }

 BYTE *src=NULL;
 if (FAILED(hr=pIn->GetPointer(&src))) return hr;
 long srclen=pIn->GetActualDataLength();
 if (searchdts)
  {
   searchdts=false;
   BYTE *buf=src;
   for (int i=0;i<srclen-5;i+=2)
    //from mplayer DTS support patch by Roberto Togni
    if((buf[i]==0xff && buf[i+1]==0x1f && buf[i+2]==0x00 && buf[i+3]== 0xe8 && (buf[i+4]&0xfe)==0xf0 && buf[i+5]==0x07) || // DTS, 14 bit, LE
       (buf[i]==0x1f && buf[i+1]==0xff && buf[i+2]==0xe8 && buf[i+3]== 0x00 && buf[i+4]==0x07 && (buf[i+5]&0xfe)==0xf0) || // DTS, 14 bit, BE
       (buf[i]==0x7f && buf[i+1]==0xfe && buf[i+2]==0x80 && buf[i+3]== 0x01) ||    // DTS, 16 bit, BE
       (buf[i]==0xfe && buf[i+1]==0x7f && buf[i+2]==0x01 && buf[i+3]== 0x80))      // DTS, 16 bit, LE
     {
      CMediaType mt=TsampleFormat(TsampleFormat::SF_PCM16,44100,2).toCMediaType();
      ((WAVEFORMATEX*)mt.Format())->wFormatTag=WAVE_FORMAT_DTS_W;
      init(mt);
      break;
     }
  }

 if (strippacket) StripPacket(src,srclen);

 REFERENCE_TIME rtStart=_I64_MIN,rtStop=_I64_MIN;
 hr=pIn->GetTime(&rtStart,&rtStop);

 if (hr == S_OK)
  {
   insample_rtStart = rtStart;
   insample_rtStop = rtStop;
   //DPRINTF(_l("audio sample start duration %I64i %I64i"),rtStart,rtStop-rtStart);
  }

 if (pIn->IsDiscontinuity()==S_OK)
  {
   filter->discontinuity=true;
   buf.clear();
   if (FAILED(hr)) return S_OK;
   filter->m_rtStartDec=filter->m_rtStartProc=rtStart;
  }

 if (SUCCEEDED(hr))
  {
   REFERENCE_TIME j=filter->m_rtStartDec-rtStart;
   jitter=int(j);
   if ((uint64_t)ff_abs(j) > 100 * (REF_SECOND_MULT / 1000) // +-100ms jitter is allowed for now
       && codecId != CODEC_ID_FLAC
       && codecId != CODEC_ID_TTA)
    {
     DPRINTF(_l("jitter correction"));
     buf.clear();
     filter->m_rtStartDec=filter->m_rtStartProc=rtStart;
    }
  }

 buf.append(src,srclen);
 buf.reserve(buf.size()+32);

 // Decode data
 return audio->decode(buf);
}

STDMETHODIMP TffdshowDecAudioInputPin::deliverDecodedSample(void *buf,size_t numsamples,const TsampleFormat &fmt,float postgain)
{
 if (numsamples==0) return S_OK;
 return filter->deliverDecodedSample(this,buf,numsamples,fmt,prevpostgain=postgain);
}

STDMETHODIMP TffdshowDecAudioInputPin::flushDecodedSamples(void)
{
 return filter->flushDecodedSamples(this,prevpostgain);
}

HRESULT TffdshowDecAudioInputPin::getMovieSource(const TaudioCodec* *moviePtr)
{
 if (!moviePtr) return S_FALSE;
 *moviePtr=audio;
 return S_OK;
}
HRESULT TffdshowDecAudioInputPin::getInCodecString(char_t *buf,size_t buflen)
{
 if (!buf) return E_POINTER;
 if (audio)
  {
   audio->getInputDescr(buf,buflen);
   return S_OK;
  }
 else
  {
   buf[0]='\0';
   return S_FALSE;
  }
}

int TffdshowDecAudioInputPin::getInputBitrate(void) const
{
 return audio?audio->getLastbps():-1;
}

bool TffdshowDecAudioInputPin::getsf(TsampleFormat &outsf)
{
 if (!audio)
  outsf=CurrentMediaType();
 else
  if (spdif_codec(audio->codecId))
   {
    outsf=TsampleFormat::createMediaTypeSPDIF();
    return true;
   }
  else
   outsf=audio->getInputSF();
 return false;
}
int TffdshowDecAudioInputPin::getJitter(void) const
{
 if (codecId != CODEC_ID_FLAC && codecId != CODEC_ID_TTA)
  return jitter/int(REF_SECOND_MULT/1000);
 else
  return 0;
}
