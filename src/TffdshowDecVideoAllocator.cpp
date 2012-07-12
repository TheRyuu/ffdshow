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

TffdshowDecVideoAllocator::TffdshowDecVideoAllocator(CBaseFilter* Ifilter, HRESULT* phr):
    CMemAllocator(NAME("TffdshowDecVideoAllocator"), NULL, phr),
    filter(Ifilter),
    mtChanged(false)
{
}

STDMETHODIMP TffdshowDecVideoAllocator::GetBuffer(IMediaSample** ppBuffer, REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime, DWORD dwFlags)
{
    if (!m_bCommitted) {
        return VFW_E_NOT_COMMITTED;
    }

    if (mtChanged) {
        ALLOCATOR_PROPERTIES Properties, Actual;
        if (FAILED(GetProperties(&Properties))) {
            return E_FAIL;
        }

        BITMAPINFOHEADER bih;
        ExtractBIH(mt, &bih);
        unsigned int biSizeImage = (bih.biWidth * abs(bih.biHeight) * bih.biBitCount) >> 3;

        if (bih.biSizeImage < biSizeImage) {
            // bugus intervideo mpeg2 decoder doesn't seem to adjust biSizeImage to the really needed buffer size
            bih.biSizeImage = biSizeImage;
        }

        if ((DWORD)Properties.cbBuffer < bih.biSizeImage || !m_bCommitted) {
            Properties.cbBuffer = bih.biSizeImage;
            if (FAILED(Decommit())) {
                return E_FAIL;
            }
            if (FAILED(SetProperties(&Properties, &Actual))) {
                return E_FAIL;
            }
            if (FAILED(Commit())) {
                return E_FAIL;
            }
            ASSERT(Actual.cbBuffer >= Properties.cbBuffer);
            if (Actual.cbBuffer < Properties.cbBuffer) {
                return E_FAIL;
            }
        }
    }

    HRESULT hr = CMemAllocator::GetBuffer(ppBuffer, pStartTime, pEndTime, dwFlags);

    if (mtChanged && SUCCEEDED(hr)) {
        mtChanged = false;
    }
    return hr;
}

void TffdshowDecVideoAllocator::NotifyMediaType(const CMediaType &Imt)
{
    mt = Imt;
    mtChanged = true;
}

int TffdshowDecVideoAllocator::get_biWidth() const
{
    // make sure to check the return value is not zero.
    BITMAPINFOHEADER *bi = get_BITMAPINFOHEADER_ptr();
    if (bi) {
        return bi->biWidth;
    }
    return 0;
}

int TffdshowDecVideoAllocator::get_biHeight() const
{
    // make sure to check the return value is not zero.
    BITMAPINFOHEADER *bi = get_BITMAPINFOHEADER_ptr();
    if (bi) {
        return bi->biHeight;
    }
    return 0;
}

// Extract pointer to BITMAPINFOHEADER from raw formats.
BITMAPINFOHEADER* TffdshowDecVideoAllocator::get_BITMAPINFOHEADER_ptr() const
{
    // return non-NULL value for raw formats.
    if (mt.IsValid() && mt.bFixedSizeSamples) {
        BITMAPINFOHEADER *bi = NULL;
        if (mt.formattype == FORMAT_VideoInfo) {
            VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.pbFormat;
            bi = &vih->bmiHeader;
        } else if (mt.formattype == FORMAT_VideoInfo2) {
            VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)mt.pbFormat;
            bi = &vih->bmiHeader;
        }
        return bi;
    }
    return NULL;
}
