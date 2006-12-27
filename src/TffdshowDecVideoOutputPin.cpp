/*
 * Copyright (c) 2006 h.yamagata
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
#include "ffdebug.h"
#include "TffdshowDecVideoOutputPin.h"
#include "TpresetSettingsVideo.h"
#include "Tlibmplayer.h"
#include "TListEmptyIMediaSamples.h"

TffdshowDecVideoOutputPin::TffdshowDecVideoOutputPin(
        TCHAR *pObjectName,
        TffdshowDecVideo *Ifdv,
        HRESULT * phr,
        LPCWSTR pName)
    :CTransformOutputPin(pObjectName, Ifdv, phr, pName),
    fdv(Ifdv),
    queue(NULL),
    oldSettingOfMultiThread(-1),
    isFirstFrame(true),
    buffers(NULL)
{
 DPRINTF(_l("TffdshowDecVideoOutputPin::Constructor"));
}

TffdshowDecVideoOutputPin::~TffdshowDecVideoOutputPin()
{
 DPRINTF(_l("TffdshowDecVideoOutputPin::Destructor"));
 freeQueue();
}

void TffdshowDecVideoOutputPin::freeQueue(void)
{
 if(queue)
  {
   queue->SetPopEvent(NULL);
   CloseHandle(hEvent);
   delete queue;
   queue=  NULL;
   hEvent= NULL;
  }
 if(buffers)
  {
   delete buffers;
   buffers=NULL;
  }
}

HRESULT TffdshowDecVideoOutputPin::Deliver(IMediaSample * pSample)
{
 if(m_pInputPin == NULL)
  return VFW_E_NOT_CONNECTED;
 if(!isFirstFrame && fdv->m_aboutToFlash == true)
  return S_FALSE;

 isFirstFrame= false;
 if(fdv->isQueue==1)
  {
   ASSERT(queue);
   ASSERT(buffers);
   pSample->AddRef();
   return queue->Receive(pSample);
  }
 else
  {
   return m_pInputPin->Receive(pSample);
  }
}

void TffdshowDecVideoOutputPin::waitUntillQueueCleanedUp(void)
{
 if (queue==NULL)
  return;

 ResetEvent(hEvent);
 while(!queue->IsIdle())
  {
   WaitForSingleObject(hEvent, INFINITE);
  }
 if(buffers)
  buffers->freeAll();
}

void TffdshowDecVideoOutputPin::waitForPopEvent(void)
{
 if (queue==NULL)
  return;

 if(!queue->IsIdle())
  WaitForSingleObject(hEvent, INFINITE);
}

void TffdshowDecVideoOutputPin::resetPopEvent(void)
{
 if (queue==NULL)
  return;

 ResetEvent(hEvent);
}

HRESULT TffdshowDecVideoOutputPin::DeliverBeginFlush(void)
{
 DPRINTF(_l("TffdshowDecVideoOutputPin::DeliverBeginFlush"));
 if (m_Connected == NULL)
  {
   return VFW_E_NOT_CONNECTED;
  }
 fdv->m_aboutToFlash= true;
 if(fdv->isQueue==1)
  {
   ASSERT(buffers);
   buffers->BeginFlush();
   queue->BeginFlush();
  }
 else
  {
   return m_Connected->BeginFlush();
  }
 return S_OK;
}

HRESULT TffdshowDecVideoOutputPin::DeliverEndFlush(void)
{
 DPRINTF(_l("TffdshowDecVideoOutputPin::DeliverEndFlush"));
 if (m_Connected == NULL)
  {
   return VFW_E_NOT_CONNECTED;
  }
 if(fdv->isQueue==1)
  {
   ASSERT(buffers);
   queue->EndFlush();
   buffers->EndFlush();
  }
 else
  {
   return m_Connected->EndFlush();
  }
 return S_OK;
}

HRESULT TffdshowDecVideoOutputPin::DeliverNewSegment( REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
 DPRINTF(_l("TffdshowDecVideoOutputPin::DeliverNewSegment tStart %7.0f, tStop %7.0f"), tStart/10000.0, tStop/10000.0);
 if (m_Connected == NULL)
  return VFW_E_NOT_CONNECTED;

 isFirstFrame= true;
 if(fdv->isQueue==1)
  {
   DPRINTF(_l("queue->NewSegment"));
   queue->NewSegment(tStart, tStop, dRate);
   ASSERT(buffers);
   buffers->NewSegment();
  }
 else
  {
   DPRINTF(_l("m_Connected->NewSegment"));
   return m_Connected->NewSegment(tStart, tStop, dRate);
  }
 return S_OK;
}

HRESULT TffdshowDecVideoOutputPin::DeliverEndOfStream(void)
{
 DPRINTF(_l("TffdshowDecVideoOutputPin::DeliverEndOfStream"));
 if (m_Connected == NULL)
  {
   return VFW_E_NOT_CONNECTED;
  }
 if(fdv->isQueue==1)
  {
   queue->EOS();
   waitUntillQueueCleanedUp();
  }
 else
  {
   return m_Connected->EndOfStream();
  }
 return S_OK;
}

void TffdshowDecVideoOutputPin::SendAnyway(void)
{
 if (queue==NULL)
  return;

 queue->SendAnyway();
 waitUntillQueueCleanedUp();
}

void TffdshowDecVideoOutputPin::BeginStop(void)
{
 if(buffers)
  buffers->BeginStop();
}

HRESULT TffdshowDecVideoOutputPin::Inactive(void)
{
 DPRINTF(_l("TffdshowDecVideoOutputPin::Inactive"));
 if (m_Connected==NULL)
  return VFW_E_NOT_CONNECTED;

 if(fdv->m_IsOldVideoRenderer==false)
  {
   waitUntillQueueCleanedUp();
   DPRINTF(_l("queue->Reset()"));
   queue->Reset();
  }
 HRESULT hr=CBaseOutputPin::Inactive();
 if(buffers)
  buffers->EndStop();
 return hr;
}

IMediaSample* TffdshowDecVideoOutputPin::GetBuffer(void)
{
 ASSERT(buffers);
 return buffers->GetBuffer();
}

void TffdshowDecVideoOutputPin::addOne(void)
{
 ASSERT(buffers);
 return buffers->addOne();
}

HRESULT TffdshowDecVideoOutputPin::CompleteConnect(IPin *pReceivePin)
{
 HRESULT phr= S_OK;
 HRESULT hr= CTransformOutputPin::CompleteConnect(pReceivePin);
 DPRINTF(_l("TffdshowDecVideoOutputPin::CompleteConnect"));
 if(SUCCEEDED(hr))
  {
   if(queue)
    freeQueue();
   queue= new TffOutputQueue(this,pReceivePin, &phr, false, true, 1, false);
   buffers= new ListEmptyIMediaSamples(this,fdv->ppropActual.cBuffers);
   hEvent= CreateEvent(NULL, false, false, NULL);
   queue->SetPopEvent(hEvent);
  }
 return hr;
}

HRESULT TffdshowDecVideoOutputPin::BreakConnect(void)
{
 // Omit checking connected or not to support dynamic reconnect.
 if(queue)
  freeQueue();
 m_pTransformFilter->BreakConnect(PINDIR_OUTPUT);
 return CBaseOutputPin::BreakConnect();
}

STDMETHODIMP TffdshowDecVideoOutputPin::Disconnect(void)
{
 // Omit checking connected or not to support dynamic reconnect.
 CAutoLock cObjectLock(m_pLock);
 return DisconnectInternal();
}

/* Asked to connect to a pin. A pin is always attached to an owning filter
   object so we always delegate our locking to that object. We first of all
   retrieve a media type enumerator for the input pin and see if we accept
   any of the formats that it would ideally like, failing that we retrieve
   our enumerator and see if it will accept any of our preferred types */

STDMETHODIMP TffdshowDecVideoOutputPin::Connect(
    IPin * pReceivePin,
    const AM_MEDIA_TYPE *pmt   // optional media type
)
{
 CheckPointer(pReceivePin,E_POINTER);
 ValidateReadPtr(pReceivePin,sizeof(IPin));
 CAutoLock cObjectLock(m_pLock);
 DisplayPinInfo(pReceivePin);

 // Omit checking connected or not to support dynamic reconnect.

 // Find a mutually agreeable media type -
 // Pass in the template media type. If this is partially specified,
 // each of the enumerated media types will need to be checked against
 // it. If it is non-null and fully specified, we will just try to connect
 // with this.

 DPRINTF(_l("TffdshowDecVideoOutputPin::Connect"));
 const CMediaType * ptype = (CMediaType*)pmt;
 HRESULT hr = AgreeMediaType(pReceivePin, ptype);
 if (FAILED(hr))
  {
   DPRINTF(_l("Failed to agree type"));

   // Since the procedure is already returning an error code, there
   // is nothing else this function can do to report the error.
   EXECUTE_ASSERT( SUCCEEDED( BreakConnect() ) );
   return hr;
  }
 DPRINTF(_l("Connection succeeded"));
 return NOERROR;
}
