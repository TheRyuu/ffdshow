/*
 * Copyright (c) 2003-2006 Milan Cutka
 *
 * Volume normalizing code from mplayer by pl <p_l@gmx.fr>,
 *  sources: some ideas from volnorm plugin for xmms
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
#include "TaudioFilterVolume.h"
#include "IffdshowBase.h"
#include "IffdshowDecAudio.h"

const unsigned int TaudioFilterVolume::NSAMPLES=128;

// If summing all the mem[].len is lower than MIN_SAMPLE_SIZE bytes, then we
// choose to ignore the computed value as it's not significant enough
// FIXME: should depend on the frequency of the data (0.5s maybe)
const unsigned int TaudioFilterVolume::MIN_SAMPLE_SIZE=32000;

const float TaudioFilterVolume::MUL_INIT=1.0f;
const float TaudioFilterVolume::MUL_MIN=0.1f;
//const float TaudioFilterVolume::MUL_MAX=5.0f;

//const int TaudioFilterVolume::MIN_S16=-32768;
//const int TaudioFilterVolume::MAX_S16= 32767;

const float TaudioFilterVolume::MID_S16=/*MAX_S16**/0.25f;
const float TaudioFilterVolume::SIL_S16=/*MAX_S16**/0.001f;

template<class sample_t> void TaudioFilterVolume::volume(sample_t* const samples,size_t numsamples,const TsampleFormat &fmt,const TvolumeSettings *cfg)
{
 if (cfg->is)
  if (!cfg->normalize)
   {
    if (isVol)
     {
      typedef void (*TprocessVol)(sample_t * const samples,size_t numsamples,const int *volumes);
      static const TprocessVol processVol[7]=
       {
        NULL,
        &Tmultiply<1>::processVol,
        &Tmultiply<2>::processVol,
        &Tmultiply<3>::processVol,
        &Tmultiply<4>::processVol,
        &Tmultiply<5>::processVol,
        &Tmultiply<6>::processVol
       };
      processVol[fmt.nchannels](samples,numsamples,volumes);
     } 
   }
  else
   {
    if (!mem)
     {
      mem=new Tmem[nSamples=oldcfg.nSamples];
      for (int i=0;i<nSamples;i++) 
       {
        mem[i].len=0;
        mem[i].avg=0;
       }
     }
  
    size_t len=numsamples*fmt.nchannels;

    // Evaluate current samples average level
    float curavg=0.0f;
    for (size_t i=0;i<len;i++)
     {
      float tmp=float(samples[i]);
      curavg+=tmp*tmp;
     }
    curavg=sqrtf(curavg/float(len));

    // Evaluate an adequate 'mul' coefficient based on previous state, current samples level, etc
    float avg=0.0f;
    size_t totallen=0;
    for (int i=0;i<nSamples;i++)
     {
      avg+=mem[i].avg*(float)mem[i].len;
      totallen+=mem[i].len;
     }

    if (totallen>MIN_SAMPLE_SIZE) 
     {
      avg/=(float)totallen;
      if (avg>=TsampleFormatInfo<sample_t>::max()*SIL_S16) 
       mul=limit((float)(TsampleFormatInfo<sample_t>::max()*MID_S16)/avg,MUL_MIN,cfg->normalizeMax/100.0f);
     }

    // Scale & clamp the samples
    typedef void (*TprocessMul)(sample_t * const samples,size_t numsamples,const int *volumes,float mul);
    static const TprocessMul processMul[7]=
     {
      NULL,
      &Tmultiply<1>::processMul,
      &Tmultiply<2>::processMul,
      &Tmultiply<3>::processMul,
      &Tmultiply<4>::processMul,
      &Tmultiply<5>::processMul,
      &Tmultiply<6>::processMul
     };
    processMul[fmt.nchannels](samples,numsamples,volumes,mul); 

    // Evaluation of newavg (not 100% accurate because of values clamping)
    float newavg=mul*curavg;
    // Stores computed values for future smoothing
    mem[idx].len=len;
    mem[idx].avg=newavg;
    idx=(idx+1)%nSamples;
   }
 if (numsamples>0 && deci->getParam2(IDFF_showCurrentVolume) && deci->getCfgDlgHwnd())
  {
   int64_t sum[6];memset(sum,0,sizeof(sum));
   for (size_t i=0;i<numsamples*fmt.nchannels;)
    for (unsigned int ch=0;ch<fmt.nchannels;ch++,i++)
     sum[ch]+=int64_t((int64_t)65536*ff_abs(samples[i]));
   CAutoLock lock(&csVolumes);  
   for (unsigned int i=0;i<fmt.nchannels;i++)
    storedvolumes.volumes[i]=int((sum[i]/numsamples)/int64_t(TsampleFormatInfo<sample_t>::max()));
   storedvolumes.have=true; 
  }  
}

TaudioFilterVolume::TaudioFilterVolume(IffdshowBase *Ideci,Tfilters *Iparent):TaudioFilter(Ideci,Iparent)
{
 mem=NULL;
 mul=MUL_INIT;
 idx=0;
 oldfmt.freq=0;
 oldcfg.vol=-100;
 storedvolumes.have=false;
}
TaudioFilterVolume::~TaudioFilterVolume()
{
 if (mem) delete []mem;
}

bool TaudioFilterVolume::is(const TsampleFormat &fmt,const TfilterSettingsAudio *cfg)
{
 return !!cfg->is;
}

HRESULT TaudioFilterVolume::process(TfilterQueue::iterator it,TsampleFormat &fmt,void *samples,size_t numsamples,const TfilterSettingsAudio *cfg0)
{
 const TvolumeSettings *cfg=(const TvolumeSettings*)cfg0;
 if (oldfmt!=fmt || !cfg->equal(oldcfg))
  {
   //Kurosu: Yeah, struct packing made that a bug
   memcpy(&oldfmt, &fmt, sizeof(TsampleFormat));
   oldcfg=*cfg;
   if (mem) delete []mem;mem=NULL;
   isVol=false;
   static const int speakers[]={SPEAKER_FRONT_LEFT,SPEAKER_FRONT_RIGHT,SPEAKER_FRONT_CENTER,SPEAKER_BACK_LEFT|SPEAKER_BACK_CENTER,SPEAKER_BACK_RIGHT,SPEAKER_LOW_FREQUENCY};
   static const int TvolumeSettings::*cfgvols[]={&TvolumeSettings::volL,&TvolumeSettings::volR,&TvolumeSettings::volC,&TvolumeSettings::volSL,&TvolumeSettings::volSR,&TvolumeSettings::volLFE};
   static const int TvolumeSettings::*cfgmutes[]={&TvolumeSettings::volLmute,&TvolumeSettings::volRmute,&TvolumeSettings::volCmute,&TvolumeSettings::volSLmute,&TvolumeSettings::volSRmute,&TvolumeSettings::volLFEmute};
   unsigned int solo=0;
   for (unsigned int i=0;i<countof(speakers);i++)
    if (cfg->*cfgmutes[i]==2)
     {
      solo=speakers[i];
      break;
     }
   for (unsigned int i=0;i<fmt.nchannels;i++)
    {
     int v=100;
     for (int ii=0;i<countof(speakers);ii++)
      if (fmt.speakers[i]&speakers[ii])
       {
        v=cfg->*cfgmutes[ii]!=1 && (solo==0 || (fmt.speakers[i]&solo))?cfg->*cfgvols[ii]:0;
        break;
       } 
     volumes[i]=v*cfg->vol/39; 
     if (volumes[i]!=256) isVol=true;
    }
  }
 if (is(fmt,cfg)) 
  samples=init(cfg,fmt,samples,numsamples);
 if (numsamples>0)
  {
   switch (fmt.sf)
    {
     case TsampleFormat::SF_PCM16  :volume((int16_t*)samples,numsamples,fmt,cfg);break;
     case TsampleFormat::SF_PCM32  :volume((int32_t*)samples,numsamples,fmt,cfg);break;
     case TsampleFormat::SF_FLOAT32:volume((float*)  samples,numsamples,fmt,cfg);break;
    }
  }
 return (it!=NULL)?parent->deliverSamples(++it,fmt,samples,numsamples):S_OK;
}

void TaudioFilterVolume::onSeek(void)
{
 if (oldcfg.normalizeResetOnSeek)
 if (mem)
  {
   mul=MUL_INIT;
   delete []mem;mem=NULL;
  }
}

HRESULT TaudioFilterVolume::queryInterface(const IID &iid,void **ptr) const
{
 if (iid==IID_IaudioFilterVolume) {*ptr=(IaudioFilterVolume*)this;return S_OK;}
 else return E_NOINTERFACE;
}

STDMETHODIMP TaudioFilterVolume::getVolumeData(unsigned int *nchannels,int channels[],int vol[])
{
 if (!storedvolumes.have) return E_UNEXPECTED;
 if (!nchannels || !channels || !vol)  return E_POINTER;
 CAutoLock lock(&csVolumes);
 *nchannels=oldfmt.nchannels;
 memcpy(channels,oldfmt.speakers,oldfmt.nchannels*sizeof(int));
 memcpy(vol,storedvolumes.volumes,oldfmt.nchannels*sizeof(int));
 return S_OK;
}
STDMETHODIMP_(int) TaudioFilterVolume::getCurrentNormalization(void)
{
 return int(mul*100);
}
