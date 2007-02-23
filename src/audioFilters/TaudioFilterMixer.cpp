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
#include "TaudioFilterMixer.h"
#include "TmixerSettings.h"
#include "mixer.h"

TaudioFilterMixer::TaudioFilterMixer(IffdshowBase *Ideci,Tfilters *Iparent):TaudioFilter(Ideci,Iparent)
{
 mixerPCM16.deciA=mixerPCM32.deciA=mixerFloat.deciA=deciA;
 matrixPtr=NULL;
}

bool TaudioFilterMixer::getOutputFmt(TsampleFormat &fmt,const TfilterSettingsAudio *cfg0)
{
 if (super::getOutputFmt(fmt,cfg0))
  {
   ((const TmixerSettings*)cfg0)->setFormatOut(fmt);
   return true;
  }
 else
  return false;
}

HRESULT TaudioFilterMixer::process(TfilterQueue::iterator it,TsampleFormat &fmt,void *samples,size_t numsamples,const TfilterSettingsAudio *cfg0)
{
 const TmixerSettings *cfg=(const TmixerSettings*)cfg0;

 samples=init(cfg,fmt,samples,numsamples);
 switch (fmt.sf)
  {
   case TsampleFormat::SF_PCM16  :mixerPCM16.process(fmt,(int16_t*&)samples,numsamples,cfg,&matrixPtr);break;
   case TsampleFormat::SF_PCM32  :mixerPCM32.process(fmt,(int32_t*&)samples,numsamples,cfg,&matrixPtr);break;
   case TsampleFormat::SF_FLOAT32:mixerFloat.process(fmt,(float*&)  samples,numsamples,cfg,&matrixPtr);break;
  }
 return parent->deliverSamples(++it,fmt,samples,numsamples);
}

HRESULT TaudioFilterMixer::queryInterface(const IID &iid,void **ptr) const
{
 if (iid==IID_IaudioFilterMixer) {*ptr=(IaudioFilterMixer*)this;return S_OK;}
 else return E_NOINTERFACE;
}
STDMETHODIMP TaudioFilterMixer::getMixerMatrixData(double Imatrix[6][6])
{
 if (!matrixPtr) return E_UNEXPECTED;
 if (!Imatrix) return E_POINTER;
 CAutoLock lock(&csMatrix);
 memcpy(Imatrix,*matrixPtr,sizeof(double)*6*6);
 return S_OK;
}
