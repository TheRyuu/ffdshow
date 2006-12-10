#ifndef _TFFDSHOWDECVIDEOOUTPUTPIN_H_
#define _TFFDSHOWDECVIDEOOUTPUTPIN_H_

#include "transfrm.h"
#include "TffdecoderVideo.h"
#include "outputq.h"

#ifdef DEBUG
#define MAX_SAMPLES_OPIN 30  // This may cause out of memory.
#else
#define MAX_SAMPLES_OPIN 10
#endif
class TffdshowDecVideo;
class TffdshowDecVideoOutputPin : public CTransformOutputPin
{
protected:
 HANDLE hEvent;
 TffdshowDecVideo* fdv;
 int oldSettingOfMultiThread;     // -1: first run, 0: false, 1: true
 void waitUntillQueueCleanedUp(void);
 void freeQueue(void);
 COutputQueue* queue;
 strings queuelistList;
 int m_IsQueueListedApp;        // -1: first run, 0: false, 1: true
 bool isFirstFrame;

public:
 TffdshowDecVideoOutputPin(
        TCHAR *pObjectName,
        TffdshowDecVideo *Ifdv,
        HRESULT * phr,
        LPCWSTR pName);
 virtual ~TffdshowDecVideoOutputPin();
 virtual HRESULT Deliver(IMediaSample *pSample);
 virtual HRESULT BreakConnect();
 virtual HRESULT CompleteConnect(IPin *pReceivePin);
 virtual STDMETHODIMP Disconnect(void);
 virtual STDMETHODIMP Connect(IPin * pReceivePin, const AM_MEDIA_TYPE *pmt);
 virtual HRESULT DeliverBeginFlush(void);
 virtual HRESULT DeliverEndFlush(void);
 virtual HRESULT DeliverEndOfStream(void);
 virtual HRESULT Inactive(void);
 virtual HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
 void waitForPopEvent(void);
 void resetPopEvent(void);
 IMemAllocator* GetAllocator(void) {return m_pAllocator;}
 void SendAnyway(void);
 int IsQueueListedApp(const char_t *exe);
 friend class TffdshowDecVideo;
};

#endif
