/*
 * Copyright (c) 2004-2006 Milan Cutka
 * based on CDirectVobSubInputPin by Gabest
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
#include "TffdshowDecVideoAllocator.h"
#include "dsutil.h"
#include "ffdshow_mediaguids.h"
#include "TffDecoderVideo.h"
#include "TffdshowVideoInputPin.h"

TffdshowDecVideoAllocator::TffdshowDecVideoAllocator(CBaseFilter* Ifilter,HRESULT* phr):
    CMemAllocator(NAME("TffdshowDecVideoAllocator"),NULL,phr),
    filter(Ifilter),
    mtChanged(false)
{
}

STDMETHODIMP TffdshowDecVideoAllocator::GetBuffer(IMediaSample** ppBuffer,REFERENCE_TIME *pStartTime,REFERENCE_TIME *pEndTime,DWORD dwFlags)
{
    if (!m_bCommitted) {
        return VFW_E_NOT_COMMITTED;
    }

    if (mtChanged) {
        ALLOCATOR_PROPERTIES Properties, Actual;
        if (FAILED(GetProperties(&Properties))) {
            return E_FAIL;
        }

        // force the upper stream filter to use 32 pixel alignment.
        // For Cb/Cr to be aligned 16, alignment 32 is needed.
        BITMAPINFOHEADER *bi = get_BITMAPINFOHEADER_ptr();
        TffdshowDecVideo *fv = dynamic_cast<TffdshowDecVideo*>(filter);
        if (bi && fv && fv->canUpperStreamHandleStrideChange()) {
            CRect zero;
            if (zero == get_rcTarget()) {
                CRect rcTarget(0,0,bi->biWidth, bi->biHeight);
                set_rcTarget(rcTarget);
            }
            bi->biWidth = ffalign(bi->biWidth, needed_align);
        }

        BITMAPINFOHEADER bih;
        ExtractBIH(mt,&bih);
        unsigned int biSizeImage=(bih.biWidth*abs(bih.biHeight)*bih.biBitCount)>>3;

        if (bih.biSizeImage<biSizeImage) {
            // bugus intervideo mpeg2 decoder doesn't seem to adjust biSizeImage to the really needed buffer size
            bih.biSizeImage=biSizeImage;
        }

        if ((DWORD)Properties.cbBuffer<bih.biSizeImage || !m_bCommitted) {
            Properties.cbBuffer=bih.biSizeImage;
            if (FAILED(Decommit())) {
                return E_FAIL;
            }
            if (FAILED(SetProperties(&Properties,&Actual))) {
                return E_FAIL;
            }
            if (FAILED(Commit())) {
                return E_FAIL;
            }
            ASSERT(Actual.cbBuffer>=Properties.cbBuffer);
            if (Actual.cbBuffer<Properties.cbBuffer) {
                return E_FAIL;
            }
        }
    }

    HRESULT hr=CMemAllocator::GetBuffer(ppBuffer,pStartTime,pEndTime,dwFlags);

    if (mtChanged && SUCCEEDED(hr)) {
        mtChanged=false;
        comptrQ<IffdshowMediaSample> iffmedia = *ppBuffer;
        if (iffmedia)
            iffmedia->SetMediaTypeNoFlag(&mt);
    }
    return hr;
}

void TffdshowDecVideoAllocator::NotifyMediaType(const CMediaType &Imt)
{
    mt=Imt;
    mtChanged=true;
}

STDMETHODIMP TffdshowDecVideoAllocator::SetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual)
{
    BITMAPINFOHEADER *bi = get_BITMAPINFOHEADER_ptr();
    // I don't want to change encoder's behavior because sufficient testing is nearly impossible.
    TffdshowDecVideo *fv = dynamic_cast<TffdshowDecVideo*>(filter);
    if (bi && fv) {
        // to make sure extra memory is allocated for alignment
        ALLOCATOR_PROPERTIES request = *pRequest;
        long newSize = 0;
        newSize = (ffalign(bi->biWidth, needed_align) * abs(bi->biHeight)*bi->biBitCount) >> 3;
        request.cbBuffer = std::max(newSize, request.cbBuffer);
        return CMemAllocator::SetProperties(&request, pActual);
    }
    return CMemAllocator::SetProperties(pRequest, pActual);
}

/* HRESULT TffdshowDecVideoAllocator::Alloc(void)                                              */
/* copied from the baseclass (amfilter.cpp) and replaced CMediaSample with CffdshowMediaSample */

// override this to allocate our resources when Commit is called.
//
// note that our resources may be already allocated when this is called,
// since we don't free them on Decommit. We will only be called when in
// decommit state with all buffers free.
//
// object locked by caller
HRESULT
TffdshowDecVideoAllocator::Alloc(void)
{
    CAutoLock lck(this);

    /* Check he has called SetProperties */
    HRESULT hr = CBaseAllocator::Alloc();
    if (FAILED(hr)) {
        return hr;
    }

    /* If the requirements haven't changed then don't reallocate */
    if (hr == S_FALSE) {
        ASSERT(m_pBuffer);
        return NOERROR;
    }
    ASSERT(hr == S_OK); // we use this fact in the loop below

    /* Free the old resources */
    if (m_pBuffer) {
        ReallyFree();
    }

    /* Make sure we've got reasonable values */
    if ( m_lSize < 0 || m_lPrefix < 0 || m_lCount < 0 ) {
        return E_OUTOFMEMORY;
    }

    /* Compute the aligned size */
    LONG lAlignedSize = m_lSize + m_lPrefix;

    /*  Check overflow */
    if (lAlignedSize < m_lSize) {
        return E_OUTOFMEMORY;
    }

    if (m_lAlignment > 1) {
        LONG lRemainder = lAlignedSize % m_lAlignment;
        if (lRemainder != 0) {
            LONG lNewSize = lAlignedSize + m_lAlignment - lRemainder;
            if (lNewSize < lAlignedSize) {
                return E_OUTOFMEMORY;
            }
            lAlignedSize = lNewSize;
        }
    }

    /* Create the contiguous memory block for the samples
       making sure it's properly aligned (64K should be enough!)
    */
    ASSERT(lAlignedSize % m_lAlignment == 0);

    LONGLONG lToAllocate = m_lCount * (LONGLONG)lAlignedSize;

    /*  Check overflow */
    if (lToAllocate > MAXLONG) {
        return E_OUTOFMEMORY;
    }

    m_pBuffer = (PBYTE)VirtualAlloc(NULL,
                    (LONG)lToAllocate,
                    MEM_COMMIT,
                    PAGE_READWRITE);

    if (m_pBuffer == NULL) {
        return E_OUTOFMEMORY;
    }

    LPBYTE pNext = m_pBuffer;
    CMediaSample *pSample;

    ASSERT(m_lAllocated == 0);

    // Create the new samples - we have allocated m_lSize bytes for each sample
    // plus m_lPrefix bytes per sample as a prefix. We set the pointer to
    // the memory after the prefix - so that GetPointer() will return a pointer
    // to m_lSize bytes.
    for (; m_lAllocated < m_lCount; m_lAllocated++, pNext += lAlignedSize) {


        pSample = new CffdshowMediaSample(
                            NAME("ffdshow memory media sample"),
                this,
                            &hr,
                            pNext + m_lPrefix,      // GetPointer() value
                            m_lSize);               // not including prefix

            ASSERT(SUCCEEDED(hr));
        if (pSample == NULL) {
            return E_OUTOFMEMORY;
        }

        // This CANNOT fail
        m_lFree.Add(pSample);
    }

    m_bChanged = FALSE;
    return NOERROR;
}

int TffdshowDecVideoAllocator::get_biWidth() const
{
    // If this allocator isn't used, biWidth is not changed for alignment.
    // make sure to check the return value is not zero.
    BITMAPINFOHEADER *bi = get_BITMAPINFOHEADER_ptr();
    if (bi)
        return bi->biWidth;
    return 0;
}

// Extract pointer to BITMAPINFOHEADER from raw formats.
BITMAPINFOHEADER* TffdshowDecVideoAllocator::get_BITMAPINFOHEADER_ptr() const
{
    // return non-NULL value for raw formats.
    if (mt.IsValid() && mt.bFixedSizeSamples) {
        BITMAPINFOHEADER *bi = NULL;
        if (mt.formattype==FORMAT_VideoInfo) {
            VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.pbFormat;
            bi = &vih->bmiHeader;
        } else if(mt.formattype == FORMAT_VideoInfo2) {
            VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)mt.pbFormat;
            bi = &vih->bmiHeader;
        }
        return bi;
    }
    return NULL;
}

CRect TffdshowDecVideoAllocator::get_rcTarget() const
{
    CRect result;
    if (mt.IsValid() && mt.bFixedSizeSamples) {
        if (mt.formattype==FORMAT_VideoInfo) {
            VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.pbFormat;
            result = vih->rcTarget;
        } else if(mt.formattype == FORMAT_VideoInfo2) {
            VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)mt.pbFormat;
            result = vih->rcTarget;
        }
    }
    return result;
}

void TffdshowDecVideoAllocator::set_rcTarget(const CRect &rcTarget) const
{
    if (mt.IsValid() && mt.bFixedSizeSamples) {
        if (mt.formattype==FORMAT_VideoInfo) {
            VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.pbFormat;
            vih->rcTarget = rcTarget;
        } else if(mt.formattype == FORMAT_VideoInfo2) {
            VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)mt.pbFormat;
            vih->rcTarget = rcTarget;
        }
    }
}

STDMETHODIMP CffdshowMediaSample::SetMediaType(AM_MEDIA_TYPE *pMediaType)
{
    if (pMediaType)
        m_MediaTypeSetExternallyFlag = true;
    return CMediaSample::SetMediaType(pMediaType);
}

// SetMediaType without setting m_MediaTypeSetExternallyFlag.
STDMETHODIMP CffdshowMediaSample::SetMediaTypeNoFlag(AM_MEDIA_TYPE *pMediaType)
{
    return CMediaSample::SetMediaType(pMediaType);
}

STDMETHODIMP CffdshowMediaSample::QueryInterface(REFIID riid, void **ppv)
{
    if (riid == IID_IffdshowMediaSample)
        return GetInterface((IffdshowMediaSample *) this, ppv);
    return CMediaSample::QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) CffdshowMediaSample::AddRef()
{
    return CMediaSample::AddRef();
}

STDMETHODIMP_(ULONG) CffdshowMediaSample::Release()
{
    return CMediaSample::Release();
}

STDMETHODIMP CffdshowMediaSample::clear_MediaTypeSetExternallyFlag()
{
    m_MediaTypeSetExternallyFlag = false;
    return S_OK;
}

STDMETHODIMP_(bool) CffdshowMediaSample::get_MediaTypeSetExternallyFlag()
{
    return m_MediaTypeSetExternallyFlag;
}
