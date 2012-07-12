#pragma once
#include "ffdshow_mediaguids.h"
#include "Crect.h"

class TffdshowDecVideoAllocator : public CMemAllocator
{
    BITMAPINFOHEADER* get_BITMAPINFOHEADER_ptr() const;
protected:
    CBaseFilter *filter;
    CMediaType mt;
public:
    TffdshowDecVideoAllocator(CBaseFilter* Ifilter, HRESULT* phr);
    STDMETHODIMP_(ULONG) NonDelegatingAddRef() {
        return filter->AddRef();
    }
    STDMETHODIMP_(ULONG) NonDelegatingRelease() {
        return filter->Release();
    }

    void NotifyMediaType(const CMediaType &Imt);
    bool mtChanged;

    STDMETHODIMP GetBuffer(IMediaSample** ppBuffer, REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime, DWORD dwFlags);
    int get_biWidth() const;
    int get_biHeight() const;
};
