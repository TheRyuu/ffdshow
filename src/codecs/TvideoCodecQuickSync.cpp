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
#include <Mfidl.h>
#include <evr.h>
#include <dxva2api.h>

#include "TvideoCodecQuickSync.h"
#include "Tdll.h"
#include "ffcodecs.h"
#include "TcodecSettings.h"
#include "IffdshowBase.h"
#include "IffdshowDecVideo.h"
#include "dsutil.h"
#include "TfakeImediaSample.h"
#include "IntelQuickSyncDecoder/src/IQuickSyncDecoder.h"

const char_t* TvideoCodecQuickSync::dllname = _l(QS_DEC_DLL_NAME);
static bool CheckForSSE41()
{
    int CPUInfo[4];
    __cpuid(CPUInfo, 1);

    return 0 != (CPUInfo[2] & (1 << 19)); //19th bit of 2nd reg means sse4.1 is enabled
}

static const bool s_SSE4_1_enabled = CheckForSSE41();

HRESULT TvideoCodecQuickSync::DeliverSurfaceCallback(void* obj, QsFrameData* frameData)
{
    if (!obj) { return E_UNEXPECTED; }

    return ((TvideoCodecQuickSync*)obj)->DeliverSurface(frameData);
}

TvideoCodecQuickSync::TvideoCodecQuickSync(IffdshowBase *Ideci, IdecVideoSink *IsinkD, int codecID) :
    Tcodec(Ideci),
    TcodecDec(Ideci, IsinkD),
    TvideoCodec(Ideci),
    TvideoCodecDec(Ideci, IsinkD),
    createQuickSync(NULL),
    destroyQuickSync(NULL),
    m_QuickSync(NULL),
    m_Dll(NULL),
    m_FakeAllocator("", NULL, NULL),
    m_MediaSample(_l("Fake Media Sample"), &m_FakeAllocator, NULL, NULL, 0)
{
    ok = false;
    m_Dll = new Tdll(dllname, config);
    m_Dll->loadFunction(createQuickSync, "createQuickSync");
    m_Dll->loadFunction(destroyQuickSync, "destroyQuickSync");
    if (NULL != createQuickSync && NULL != destroyQuickSync) {
        m_QuickSync = createQuickSync();
        ok = (m_QuickSync) && m_QuickSync->getOK();
        if (ok) {
            m_QuickSync->SetDeliverSurfaceCallback(this, DeliverSurfaceCallback);

            // workaround for WMC so it doesn't fail.
            if (0 == _tcscmp(Ideci->getExeflnm(), _l("ehshell.exe"))) {
                CQsConfig cfg;
                m_QuickSync->GetConfig(&cfg);
                cfg.bEnableSwEmulation = true;
                m_QuickSync->SetConfig(&cfg);
            }
        } else {
            destroyQuickSync(m_QuickSync);
            m_QuickSync = NULL;
        }
    }
}

TvideoCodecQuickSync::~TvideoCodecQuickSync()
{
    CAutoLock lock(&m_csLock);
    if (destroyQuickSync) {
        try {
            destroyQuickSync(m_QuickSync);
            m_QuickSync = NULL;
        } catch (...) {
            m_QuickSync = NULL;
        }
    }
    delete m_Dll;
}

const char_t* TvideoCodecQuickSync::getName(void) const
{
    return _l("Intel\xae QuickSync");
}

bool TvideoCodecQuickSync::beginDecompress(TffPictBase &pict, FOURCC infcc, const CMediaType &mt, int sourceFlags)
{
    if (!ok) { return false; }

    CAutoLock lock(&m_csLock);
    CQsConfig cfg;
    m_QuickSync->GetConfig(&cfg);

    // force ffdshow defaults/options
    cfg.bTimeStampCorrection          = deci->getParam2(IDFF_QS_ENABLE_TS_CORR) != 0;
    cfg.bEnableMultithreading         = deci->getParam2(IDFF_QS_ENABLE_MT) != 0;
    cfg.eFieldOrder                   = deci->getParam2(IDFF_QS_FIELD_ORDER);
    cfg.bEnableSwEmulation            = deci->getParam2(IDFF_QS_ENABLE_SW_EMULATION) != 0;
    cfg.bForceFieldOrder              = deci->getParam2(IDFF_QS_FORCE_FIELD_ORDER) != 0;
    cfg.bEnableDvdDecoding            = deci->getParam2(IDFF_QS_ENABLE_DVD_DECODE) != 0;
    cfg.bVppEnableDeinterlacing       = deci->getParam2(IDFF_QS_ENABLE_DI) != 0;
    cfg.bVppEnableForcedDeinterlacing = deci->getParam2(IDFF_QS_FORCE_DI) != 0;
    cfg.bVppEnableFullRateDI          = deci->getParam2(IDFF_QS_ENABLE_FULL_RATE) != 0;
    cfg.nVppDetailStrength            = deci->getParam2(IDFF_QS_DETAIL);
    cfg.nVppDenoiseStrength           = deci->getParam2(IDFF_QS_DENOISE);

    if (cfg.bVppEnableForcedDeinterlacing) {
        cfg.bVppEnableDeinterlacing = true;
    }

    // Auto enable VPP
    cfg.bEnableVideoProcessing = (cfg.nVppDenoiseStrength || cfg.nVppDetailStrength || cfg.bVppEnableDeinterlacing);

    m_QuickSync->SetConfig(&cfg);

    // Init decoder
    HRESULT hr = m_QuickSync->InitDecoder(&mt, infcc);

    pict.csp = FF_CSP_NV12;
    return hr == S_OK;
}

HRESULT TvideoCodecQuickSync::decompress(const unsigned char *src, size_t srcLen, IMediaSample *pIn)
{
    if (!ok) { return E_UNEXPECTED; }

    CAutoLock lock(&m_csLock);

    IMediaSample* pMediaSample = pIn;
    TfakeMediaSample* pFakeMediaSample = (E_NOTIMPL == pIn->GetActualDataLength()) ? static_cast<TfakeMediaSample*>(pIn) : NULL;

    // VFW flow
    if (NULL != pFakeMediaSample) {
        m_MediaSample.SetDiscontinuity(pFakeMediaSample->IsDiscontinuity() == S_OK ? TRUE : FALSE);
        m_MediaSample.SetSyncPoint(pFakeMediaSample->IsSyncPoint() == S_OK ? TRUE : FALSE);
        m_MediaSample.SetPointer(const_cast<BYTE*>(src), (LONG)srcLen);
        m_MediaSample.SetActualDataLength((LONG)srcLen);
        pMediaSample = &m_MediaSample;
    }

    bool isSyncPoint = pMediaSample->IsSyncPoint() == S_OK;
    HRESULT hr = m_QuickSync->Decode(pMediaSample);

    if (pMediaSample->IsPreroll() == S_OK) {
        return sinkD->deliverPreroll((isSyncPoint) ? FRAME_TYPE::I : FRAME_TYPE::P);
    }

    return hr;
}

bool TvideoCodecQuickSync::onSeek(REFERENCE_TIME segmentStart)
{
    if (!ok) { return false; }

    CAutoLock lock(&m_csLock);
    telecineManager.onSeek();
    return S_OK == m_QuickSync->OnSeek(segmentStart);
}

HRESULT TvideoCodecQuickSync::DeliverSurface(QsFrameData* frameData)
{
    if (NULL == frameData) { return E_POINTER; }

    unsigned char* dstBuffer[4] = {frameData->y, frameData->u, 0, 0};
    ptrdiff_t strides[4] = {frameData->dwStride, frameData->dwStride, 0, 0};
    int frametype, fieldtype;

    // set frame type - Not curently available!
    switch (frameData->frameType) {
        case QsFrameData::P:
            frametype = FRAME_TYPE::P;
            break;
        case QsFrameData::B:
            frametype = FRAME_TYPE::B;
            break;
        case QsFrameData::I:
        default:
            frametype = FRAME_TYPE::I;
    }

    // interlacing info
    // progressive
    if (frameData->dwInterlaceFlags & AM_VIDEO_FLAG_WEAVE) {
        fieldtype = FIELD_TYPE::PROGRESSIVE_FRAME;
    }
    // interlaced
    else {
        fieldtype = (frameData->dwInterlaceFlags & AM_VIDEO_FLAG_FIELD1FIRST) ?
                    (FIELD_TYPE::INT_TFF) :
                    (FIELD_TYPE::INT_BFF);
    }

    Trect r(frameData->rcClip);
    TffPict pict(FF_CSP_NV12, dstBuffer, strides, r, frameData->bReadOnly, frametype, fieldtype, 0, NULL); //TODO: src frame size
    pict.rectClip = frameData->rcClip;

    // set times stamps
    pict.rtStart = frameData->rtStart;
    pict.rtStop  = frameData->rtStop;

    // aspect ratio
    if (frameData->dwPictAspectRatioX * frameData->dwPictAspectRatioY != 0) {
        Rational dar(frameData->dwPictAspectRatioX, frameData->dwPictAspectRatioY);
        pict.setDar(dar);
    }

    // soft telecine detection
    // if "Detect soft telecine and average frame durations" is enabled,
    // flames are flagged as progressive, frame durations are averaged.
    // pict.film is valid even if the setting is disabled.
    telecineManager.new_frame(
        0 != (frameData->dwInterlaceFlags & AM_VIDEO_FLAG_FIELD1FIRST),
        0 != (frameData->dwInterlaceFlags & AM_VIDEO_FLAG_REPEAT_FIELD),
        pict.rtStart,
        pict.rtStop);
    telecineManager.get_fieldtype(pict);
    telecineManager.get_timestamps(pict);

    return sinkD->deliverDecodedSample(pict);
}

HRESULT TvideoCodecQuickSync::BeginFlush()
{
    if (!ok) { return E_UNEXPECTED; }

    // No lock!
    HRESULT hr =  m_QuickSync->BeginFlush();
    telecineManager.onSeek();
    return hr;
}

HRESULT TvideoCodecQuickSync::EndFlush()
{
    if (!ok) { return E_UNEXPECTED; }

    // No lock!
    return m_QuickSync->EndFlush();
}

bool TvideoCodecQuickSync::onDiscontinuity()
{
    if (!ok) { return false; }
    return __super::onDiscontinuity();
}

// called on "end of stream" event
HRESULT TvideoCodecQuickSync::onEndOfStream()
{
    if (!ok) { return E_UNEXPECTED; }

    CAutoLock lock(&m_csLock);
    HRESULT hr = m_QuickSync->Flush(true);
    return hr;
}

bool TvideoCodecQuickSync::testMediaType(FOURCC fcc, const CMediaType &mt)
{
    if (!ok) { return false; }
    return S_OK == m_QuickSync->TestMediaType(&mt, fcc);
}

void TvideoCodecQuickSync::setOutputPin(IPin *pPin)
{
    if (!ok) { return; }

    if (NULL == pPin) {
        m_QuickSync->SetD3DDeviceManager(NULL);
    }

    IDirect3DDeviceManager9 *pDeviceManager = NULL;
    IMFGetService *pGetService = NULL;
    HRESULT hr = pPin->QueryInterface(__uuidof(IMFGetService), (void**)&pGetService);
    if (SUCCEEDED(hr)) {
        hr = pGetService->GetService(MR_VIDEO_ACCELERATION_SERVICE, IID_IDirect3DDeviceManager9, (void**)&pDeviceManager);
    }

    m_QuickSync->SetD3DDeviceManager((SUCCEEDED(hr)) ? pDeviceManager : NULL);

    if (pDeviceManager) { pDeviceManager->Release(); }
    if (pGetService) { pGetService->Release(); }
}

bool TvideoCodecQuickSync::check(Tconfig* config)
{
    // Check for SSE4.1 so old CPUs will not be bothered
    if (!s_SSE4_1_enabled) {
        return false;
    }
    static bool checkResult = Tdll::check(dllname, config); //no need to do this more than once
    return checkResult;
}

