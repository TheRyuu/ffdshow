/*
 * Copyright (c) 2002-2006 Milan Cutka
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "stdafx.h"
#include "TvideoCodecQuickSync.h"
#include "Tdll.h"
#include "ffcodecs.h"
#include "TcodecSettings.h"
#include "IffdshowBase.h"
#include "IffdshowDecVideo.h"
#include "dsutil.h"
#include "IntelQuickSyncDecoder/src/IQuickSyncDecoder.h"

const char_t* TvideoCodecQuickSync::dllname=_l(QS_DEC_DLL_NAME);

HRESULT TvideoCodecQuickSync::DeliverSurfaceCallback(void* obj, QsFrameData* frameData)
{
    if (!obj) return E_UNEXPECTED;

    return ((TvideoCodecQuickSync*)obj)->DeliverSurface(frameData);
}

TvideoCodecQuickSync::TvideoCodecQuickSync(IffdshowBase *Ideci,IdecVideoSink *IsinkD, int codecID) :
    Tcodec(Ideci),
    TcodecDec(Ideci,IsinkD),
    TvideoCodec(Ideci),
    TvideoCodecDec(Ideci,IsinkD),
    createQuickSync(NULL),
    destroyQuickSync(NULL),
    m_QuickSync(NULL),
    m_Dll(NULL)
{
    ok = false;
    m_Dll = new Tdll(dllname,config);
    m_Dll->loadFunction(createQuickSync, "createQuickSync");
    m_Dll->loadFunction(destroyQuickSync, "destroyQuickSync");
    if (NULL != createQuickSync)
    {
        m_QuickSync = createQuickSync();
        ok = (m_QuickSync) && m_QuickSync->getOK();
        m_QuickSync->SetDeliverSurfaceCallback(this, DeliverSurfaceCallback);
    }
}

TvideoCodecQuickSync::~TvideoCodecQuickSync()
{
    CAutoLock lock(&m_csLock);
    if (destroyQuickSync)
    {
        try
        {
            destroyQuickSync(m_QuickSync);
            m_QuickSync = NULL;
        }
        catch (...)
        {
            m_QuickSync = NULL;
        }
    }
    delete m_Dll;
}

const char_t* TvideoCodecQuickSync::getName(void) const
{
    return _l("Intel\xae QuickSync");
}

bool TvideoCodecQuickSync::beginDecompress(TffPictBase &pict,FOURCC infcc,const CMediaType &mt,int sourceFlags)
{
    if (!ok) return false;

    CAutoLock lock(&m_csLock);
    HRESULT hr = m_QuickSync->InitDecoder(&mt, infcc);
    pict.csp = FF_CSP_NV12;
    pict.rectFull.dx =  ((pict.rectFull.dx + 15) & (~15)); // 16 alignment
    return hr == S_OK;
}

HRESULT TvideoCodecQuickSync::decompress(const unsigned char *src,size_t srcLen,IMediaSample *pIn)
{
    if (!ok) return E_UNEXPECTED;

    CAutoLock lock(&m_csLock);
    bool isSyncPoint = pIn->IsSyncPoint() == S_OK;
    HRESULT hr = m_QuickSync->Decode(pIn);

    if (pIn->IsPreroll() == S_OK)
    {
        return sinkD->deliverPreroll((isSyncPoint) ? FRAME_TYPE::I : FRAME_TYPE::P);
    }
    
    return hr;
}

bool TvideoCodecQuickSync::onSeek(REFERENCE_TIME segmentStart)
{
    if (!ok)  return false;

    CAutoLock lock(&m_csLock);
    return S_OK == m_QuickSync->OnSeek(segmentStart);
}

HRESULT TvideoCodecQuickSync::DeliverSurface(QsFrameData* frameData)
{
    if (NULL == frameData) return E_POINTER;

    unsigned char* dstBuffer[4]= {frameData->y, frameData->u, 0, 0};
    ptrdiff_t strides[4]= {frameData->dwStride, frameData->dwStride, 0, 0};
    int frametype, fieldtype;

    // set frame type - Not curently available!
    switch (frameData->frameType)
    {
    case QsFrameData::P:
        frametype = FRAME_TYPE::P; break;
    case QsFrameData::B:
        frametype = FRAME_TYPE::B; break;
    case QsFrameData::I:
    default:
        frametype = FRAME_TYPE::I;
    }

    // interlacing info
    // progressive
    if (frameData->dwInterlaceFlags & AM_VIDEO_FLAG_WEAVE)
    {
        fieldtype = FIELD_TYPE::PROGRESSIVE_FRAME;
    }
    // interlaced
    else
    {
        fieldtype = (frameData->dwInterlaceFlags & AM_VIDEO_FLAG_FIELD1FIRST) ? 
            FIELD_TYPE::INT_TFF : FIELD_TYPE::INT_BFF;
    }

    Trect r(0, 0, frameData->dwWidth, frameData->dwHeight);
    TffPict pict(FF_CSP_NV12, dstBuffer, strides, r, false, frametype, fieldtype, 0, NULL); //TODO: src frame size

    // set times stamps
    pict.rtStart = frameData->rtStart;
    pict.rtStop  = frameData->rtStop;

    // aspect ratio
    if (frameData->dwPictAspectRatioX * frameData->dwPictAspectRatioY != 0)
    {
        Rational dar(frameData->dwPictAspectRatioX, frameData->dwPictAspectRatioY);
        pict.setDar(dar);
    }

   // pict.film = 0 != (frameData->dwInterlaceFlags & AM_VIDEO_FLAG_REPEAT_FIELD);

    return sinkD->deliverDecodedSample(pict);
}

HRESULT TvideoCodecQuickSync::BeginFlush()
{
    if (!ok) return E_UNEXPECTED;

    // No lock!
    return m_QuickSync->BeginFlush();
}

HRESULT TvideoCodecQuickSync::EndFlush()
{
    if (!ok) return E_UNEXPECTED;

    // No lock!
    return m_QuickSync->EndFlush();
}

bool TvideoCodecQuickSync::onDiscontinuity()
{
    if (!ok) return false;

    return __super::onDiscontinuity();
}

// called on "end of stream" event
HRESULT TvideoCodecQuickSync::onEndOfStream()
{
    if (!ok) return E_UNEXPECTED;

    CAutoLock lock(&m_csLock);
    HRESULT hr = m_QuickSync->Flush(true);
    return hr;
}

bool TvideoCodecQuickSync::testMediaType(FOURCC fcc,const CMediaType &mt)
{
    return true;
}
