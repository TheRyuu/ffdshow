/*
 * Added to support multi-thread related features
 * by Haruhiko Yamagata <h.yamagata@nifty.com> in 2006.
 */

#include "stdafx.h"
#include "TffdshowDecVideoOutputPin.h"
#include "ffdebug.h"
#include "TpresetSettingsVideo.h"
#include "Tlibmplayer.h"

TffdshowDecVideoOutputPin::TffdshowDecVideoOutputPin(
        TCHAR *pObjectName,
        TffdshowDecVideo *Ifdv,
        HRESULT * phr,
        LPCWSTR pName)
    :CTransformOutputPin(pObjectName, Ifdv, phr, pName), fdv(Ifdv), queue(NULL), oldSettingOfMultiThread(-1), m_IsQueueListedApp(-1), isFirstFrame(true)
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
}

HRESULT TffdshowDecVideoOutputPin::Deliver(IMediaSample * pSample)
{
 if(m_pInputPin == NULL)
  return VFW_E_NOT_CONNECTED;
 if(!isFirstFrame && fdv->m_aboutToFlash == true)
  return S_FALSE;

 isFirstFrame= false;
 if(fdv->m_IsOldVideoRenderer==false && fdv->presetSettings->multiThread && m_IsQueueListedApp && !fdv->m_IsOldVMR9RenderlessAndRGB)
  {
   oldSettingOfMultiThread= fdv->presetSettings->multiThread;
   //oldSettingOfDontQueueInWMP= fdv->presetSettings->dontQueueInWMP;
   ASSERT(queue);
   pSample->AddRef();
   return queue->Receive(pSample);
  }
 else
  {
   if(fdv->presetSettings->multiThread != oldSettingOfMultiThread)
    {
     DPRINTF(_l("Setting of presetSettings->multiThread have been changed"));
     SendAnyway();
     oldSettingOfMultiThread= fdv->presetSettings->multiThread;
     //oldSettingOfDontQueueInWMP= fdv->presetSettings->dontQueueInWMP;
    }
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
 if(fdv->m_IsOldVideoRenderer==false && oldSettingOfMultiThread && m_IsQueueListedApp && !fdv->m_IsOldVMR9RenderlessAndRGB)
  {
   fdv->m_aboutToFlash= true;
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
 if(fdv->m_IsOldVideoRenderer==false && oldSettingOfMultiThread && m_IsQueueListedApp && !fdv->m_IsOldVMR9RenderlessAndRGB)
  {
   queue->EndFlush();
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
 if(fdv->m_IsOldVideoRenderer==false && oldSettingOfMultiThread && m_IsQueueListedApp && !fdv->m_IsOldVMR9RenderlessAndRGB)
  {
   DPRINTF(_l("queue->NewSegment"));
   queue->NewSegment(tStart, tStop, dRate);
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
 if(fdv->m_IsOldVideoRenderer==false && oldSettingOfMultiThread && m_IsQueueListedApp && !fdv->m_IsOldVMR9RenderlessAndRGB)
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

HRESULT TffdshowDecVideoOutputPin::Inactive(void)
{
 if (m_Connected==NULL)
  return VFW_E_NOT_CONNECTED;

 if(fdv->m_IsOldVideoRenderer==false)
  {
   waitUntillQueueCleanedUp();
   DPRINTF(_l("queue->Reset()"));
   queue->Reset();
  }
 return CBaseOutputPin::Inactive();
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
   queue= new COutputQueue(pReceivePin, &phr, false, true, 1, false);
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

int TffdshowDecVideoOutputPin::IsQueueListedApp(const char_t *exe)
{
 if (!fdv->presetSettings) fdv->initPreset();
 if(fdv->presetSettings->useQueueOnlyIn)
  {
   strtok(fdv->presetSettings->useQueueOnlyInList,_l(";"),queuelistList);
   for (strings::const_iterator b=queuelistList.begin();b!=queuelistList.end();b++)
    if (DwStrcasecmp(*b,exe)==0)
     return 1;
   return 0;
  }
 else
  {
   return 1;
  }
}