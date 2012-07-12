#ifndef _TFFDSHOWDECVIDEOALLOCATORDXVA_H_
#define _TFFDSHOWDECVIDEOALLOCATORDXVA_H_

#include "stdafx.h"
#include <atlbase.h>
#include <dxva.h>
#include <dxva2api.h>
#include <streams.h>
#include <dvdmedia.h>
#include <d3dx9.h>
#include <evr.h>
#include <mfapi.h>
#include <Mferror.h>
#include "IffdshowBase.h"
#include "IffdshowDec.h"
#include "IffdshowDecVideo.h"
#include "TffDecoderVideo.h"
#include "dsutil.h"
#include "strmif.h"


class TffdshowDecVideoAllocatorDXVA;



DECLARE_INTERFACE_(IFFDSDXVA2Sample, IUnknown)
{
    STDMETHOD_(int, GetDXSurfaceId()) = 0;
};

class TDXVA2Sample : public CMediaSample, public IMFGetService, public IFFDSDXVA2Sample
{
    friend class TffdshowDecVideoAllocatorDXVA;

public:
    TDXVA2Sample(TffdshowDecVideoAllocatorDXVA *pAlloc, HRESULT *phr);

    // Note: CMediaSample does not derive from CUnknown, so we cannot use the
    //       DECLARE_IUNKNOWN macro that is used by most of the filter classes.

    STDMETHODIMP         QueryInterface(REFIID riid, __deref_out void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IMFGetService::GetService
    STDMETHODIMP GetService(REFGUID guidService, REFIID riid, LPVOID *ppv);

    // IMPCDXVA2Sample
    STDMETHODIMP_(int) GetDXSurfaceId();

    // Override GetPointer because this class does not manage a system memory buffer.
    // The EVR uses the MR_BUFFER_SERVICE service to get the Direct3D surface.
    STDMETHODIMP GetPointer(BYTE ** ppBuffer);

private:
    CodecID dxvaCodecId;
    // Sets the pointer to the Direct3D surface.
    void SetSurface(DWORD surfaceId, IDirect3DSurface9 *pSurf);
    CComPtr<IDirect3DSurface9> m_pSurface;
    DWORD                      m_dwSurfaceId;
};


class TffdshowDecVideoAllocatorDXVA :  public CBaseAllocator
{
public:
    TffdshowDecVideoAllocatorDXVA(IffdshowDecVideo *IdeciV, HRESULT* phr);
    virtual ~TffdshowDecVideoAllocatorDXVA();

protected:
    HRESULT Alloc(void);
    void    Free(void);


private :
    IffdshowDecVideo *deciV;

    IDirect3DSurface9** ppRTSurfaceArray;
    UINT                nSurfaceArrayCount;
};

#endif
