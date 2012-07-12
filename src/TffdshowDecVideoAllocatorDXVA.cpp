/*
 * $Id: TffdshowVideoDecAllocatorDXVA.cpp
 *
 * (C) 2006-2009
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include "TffdshowDecVideoAllocatorDXVA.h"
#include "TvideoCodecLibavcodec.h"
#include "TvideoCodecLibavcodecDxva.h"
#include "dsutil.h"
#include "ffdshow_mediaguids.h"
#include "Tpresets.h"



TDXVA2Sample::TDXVA2Sample(TffdshowDecVideoAllocatorDXVA *pAlloc, HRESULT *phr)
    : CMediaSample(NAME("TDXVA2Sample"), (CBaseAllocator*)pAlloc, phr, NULL, 0)
    , m_dwSurfaceId(0)
{
}

// Note: CMediaSample does not derive from CUnknown, so we cannot use the
//       DECLARE_IUNKNOWN macro that is used by most of the filter classes.

STDMETHODIMP TDXVA2Sample::QueryInterface(REFIID riid, __deref_out void **ppv)
{
    CheckPointer(ppv, E_POINTER);
    ValidateReadWritePtr(ppv, sizeof(PVOID));

    if (riid == __uuidof(IMFGetService)) {
        return GetInterface((IMFGetService*) this, ppv);
    }
    if (riid == IID_IFFDSDXVA2Sample) {
        return GetInterface<IFFDSDXVA2Sample>(this, ppv);
    } else {
        return CMediaSample::QueryInterface(riid, ppv);
    }
}


STDMETHODIMP_(ULONG) TDXVA2Sample::AddRef()
{
    return CMediaSample::AddRef();
}

STDMETHODIMP_(ULONG) TDXVA2Sample::Release()
{
    // Return a temporary variable for thread safety.
    ULONG cRef = CMediaSample::Release();
    return cRef;
}

// IMFGetService::GetService
STDMETHODIMP TDXVA2Sample::GetService(REFGUID guidService, REFIID riid, LPVOID *ppv)
{
    if (guidService != MR_BUFFER_SERVICE) {
        return MF_E_UNSUPPORTED_SERVICE;
    } else if (m_pSurface == NULL) {
        return E_NOINTERFACE;
    } else {
        return m_pSurface->QueryInterface(riid, ppv);
    }
}

// Override GetPointer because this class does not manage a system memory buffer.
// The EVR uses the MR_BUFFER_SERVICE service to get the Direct3D surface.
STDMETHODIMP TDXVA2Sample::GetPointer(BYTE ** ppBuffer)
{
    return E_NOTIMPL;
}



// Sets the pointer to the Direct3D surface.
void TDXVA2Sample::SetSurface(DWORD surfaceId, IDirect3DSurface9 *pSurf)
{
    m_pSurface = pSurf;
    m_dwSurfaceId = surfaceId;
}

STDMETHODIMP_(int) TDXVA2Sample::GetDXSurfaceId()
{
    return m_dwSurfaceId;
}



//------------------------- TffdshowDecVideoAllocatorDXVA ----------------------------------------
TffdshowDecVideoAllocatorDXVA::TffdshowDecVideoAllocatorDXVA(IffdshowDecVideo* IdeciV,  HRESULT* phr)
    : CBaseAllocator(NAME("TffdshowDecVideoAllocatorDXVA"), NULL, phr),
      deciV(IdeciV)
{
    ppRTSurfaceArray    = NULL;
}

TffdshowDecVideoAllocatorDXVA::~TffdshowDecVideoAllocatorDXVA()
{
    Free();
}

HRESULT TffdshowDecVideoAllocatorDXVA::Alloc()
{
    HRESULT hr;
    CComPtr<IDirectXVideoAccelerationService> pDXVA2Service;
    TvideoCodecLibavcodecDxva *dxvaCodec = NULL;
    deciV->getMovieSource((const TvideoCodecDec**)&dxvaCodec);

    CheckPointer(dxvaCodec->m_pDeviceManager, E_UNEXPECTED);
    hr = dxvaCodec->m_pDeviceManager->GetVideoService(dxvaCodec->hDevice, IID_IDirectXVideoAccelerationService, (void**)&pDXVA2Service);
    CheckPointer(pDXVA2Service, E_UNEXPECTED);
    CAutoLock lock(this);

    hr = __super::Alloc();

    if (SUCCEEDED(hr)) {
        // Free the old resources.
        Free();
        nSurfaceArrayCount = m_lCount;

        // Allocate a new array of pointers.
        ppRTSurfaceArray = new IDirect3DSurface9*[m_lCount];
        if (ppRTSurfaceArray == NULL) {
            hr = E_OUTOFMEMORY;
        } else {
            ZeroMemory(ppRTSurfaceArray, sizeof(IDirect3DSurface9*) * m_lCount);
        }
    }

    // Allocate the surfaces.
    D3DFORMAT m_dwFormat = dxvaCodec->videoDesc.Format;
    if (SUCCEEDED(hr))
        hr = pDXVA2Service->CreateSurface(
                 dxvaCodec->pictWidthRounded(),
                 dxvaCodec->pictHeightRounded(),
                 m_lCount - 1,
                 (D3DFORMAT)m_dwFormat,
                 D3DPOOL_DEFAULT,
                 0,
                 DXVA2_VideoDecoderRenderTarget,
                 ppRTSurfaceArray,
                 NULL
             );

    if (SUCCEEDED(hr)) {
        // Important : create samples in reverse order !
        for (m_lAllocated = m_lCount - 1; m_lAllocated >= 0; m_lAllocated--) {
            TDXVA2Sample *pSample = new TDXVA2Sample(this, &hr);
            if (pSample == NULL) {
                hr = E_OUTOFMEMORY;
                break;
            }
            if (FAILED(hr)) {
                break;
            }

            // Assign the Direct3D surface pointer and the index.
            pSample->SetSurface(m_lAllocated, ppRTSurfaceArray[m_lAllocated]);

            // Add to the sample list.
            m_lFree.Add(pSample);
        }

        hr = dxvaCodec->createDXVA2Decoder(m_lCount, ppRTSurfaceArray);
        if (FAILED(hr)) {
            Free();
        }
    }

    if (SUCCEEDED(hr)) {
        m_bChanged = FALSE;
    }

    return hr;
}

void TffdshowDecVideoAllocatorDXVA::Free()
{
    CMediaSample *pSample = NULL;

    TvideoCodecLibavcodecDxva *dxvaCodec = NULL;
    deciV->getMovieSource((const TvideoCodecDec**)&dxvaCodec);

    dxvaCodec->flushDXVADecoder();
    do {
        pSample = m_lFree.RemoveHead();
        if (pSample) {
            delete pSample;
        }
    } while (pSample);

    if (ppRTSurfaceArray) {
        for (long i = 0; i < (long)nSurfaceArrayCount; i++)
            if (ppRTSurfaceArray[i] != NULL) {
                ppRTSurfaceArray[i]->Release();
            }

        delete [] ppRTSurfaceArray;
        ppRTSurfaceArray = NULL;
    }
    m_lAllocated       = 0;
    nSurfaceArrayCount = 0;
}
