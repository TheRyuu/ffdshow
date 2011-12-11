#pragma once
#include "ffdshow_mediaguids.h"

class TffdshowDecVideoAllocator :public CMemAllocator
{
    static const int needed_align = 32;
    BITMAPINFOHEADER* get_BITMAPINFOHEADER_ptr() const;
protected:
    CBaseFilter *filter;
    CMediaType mt;
public:
    TffdshowDecVideoAllocator(CBaseFilter* Ifilter,HRESULT* phr);
    STDMETHODIMP_(ULONG) NonDelegatingAddRef() {
        return filter->AddRef();
    }
    STDMETHODIMP_(ULONG) NonDelegatingRelease() {
        return filter->Release();
    }

    void NotifyMediaType(const CMediaType &Imt);
    bool mtChanged;

    STDMETHODIMP GetBuffer(IMediaSample** ppBuffer,REFERENCE_TIME* pStartTime,REFERENCE_TIME* pEndTime,DWORD dwFlags);
    STDMETHODIMP SetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual);
    HRESULT Alloc();
    int get_biWidth() const;
};

DECLARE_INTERFACE_(IffdshowMediaSample,IUnknown)
{
    STDMETHOD (clear_MediaTypeSetExternallyFlag)() PURE;
    STDMETHOD_(bool,get_MediaTypeSetExternallyFlag)() PURE;
    STDMETHOD (SetMediaTypeNoFlag)(AM_MEDIA_TYPE *pMediaType) PURE;
};

template<> inline const GUID& getGUID<IffdshowMediaSample>(void)
{
    return IID_IffdshowMediaSample;
}

class CffdshowMediaSample :public CMediaSample,IffdshowMediaSample
{
    bool m_MediaTypeSetExternallyFlag;
public:
    CffdshowMediaSample(wchar_t *pName,
               CBaseAllocator *pAllocator,
               HRESULT *phr,
               LPBYTE pBuffer,
               LONG length) :
        m_MediaTypeSetExternallyFlag(false),
        CMediaSample(pName, pAllocator, phr, pBuffer, length){}

    CffdshowMediaSample(char *pName,
               CBaseAllocator *pAllocator,
               HRESULT *phr,
               LPBYTE pBuffer,
               LONG length) :
        m_MediaTypeSetExternallyFlag(false),
        CMediaSample(pName, pAllocator, phr, pBuffer, length){}
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP SetMediaType(AM_MEDIA_TYPE *pMediaType);
    STDMETHODIMP SetMediaTypeNoFlag(AM_MEDIA_TYPE *pMediaType);
    STDMETHODIMP clear_MediaTypeSetExternallyFlag();
    STDMETHODIMP_(bool) get_MediaTypeSetExternallyFlag();
};
