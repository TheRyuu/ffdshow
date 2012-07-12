/*
 * Copyright (c) 2003-2006 Milan Cutka
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
#include <windows.h>
#include "TffDecoderVideo.h"
#include "Tconfig.h"
#include "TglobalSettings.h"
#include "Tpresets.h"
#include "TpresetSettingsVideo.h"
#include "ToutputVideoSettings.h"
#include "TresizeAspectSettings.h"
#include "TdialogSettings.h"
#include "ffdshow_mediaguids.h"
#include "ffcodecs.h"

#include "TtrayIcon.h"
#include "TcpuUsage.h"
#include "TffPict.h"
#include "TimgFilters.h"
#include "TffdshowVideoInputPin.h"
#include "TtextInputPin.h"
#include "TfontManager.h"
#include "TvideoCodec.h"
#include "dsutil.h"
#include "TkeyboardDirect.h"
#include "ffdshowRemoteAPIimpl.h"
#include "IffdshowEnc.h"
#include "ThwOverlayControlOverlay.h"
#include "ThwOverlayControlVMR9.h"
#include "TffdshowDecVideoOutputPin.h"
#include "Tinfo.h"
#include "D3d9.h"
#include "Vmr9.h"
#include "IVMRffdshow9.h"
#include "IffdshowDecAudio.h"
#include "qnetwork.h"
#include "codecs\TvideoCodecLibavcodec.h"
#include "codecs\TvideoCodecLibavcodecDxva.h"


TffdshowDecVideoDXVA::TffdshowDecVideoDXVA(LPUNKNOWN punk, HRESULT *phr): TffdshowDecVideo(CLSID_FFDSHOWDXVA, NAME("TffdshowDecVideoDXVA"), CLSID_TFFDSHOWPAGEDXVA, IDS_FFDSHOWDECVIDEODXVA, IDI_FFDSHOW, punk, phr, IDFF_FILTERMODE_PLAYER | IDFF_FILTERMODE_VIDEODXVA, defaultMerit, new TintStrColl)
{
    DPRINTF(_l("TffdshowDecVideoDXVA constructor"));
}

// get list of supported output colorspaces
HRESULT TffdshowDecVideoDXVA::GetMediaType(int iPosition, CMediaType *mtOut)
{
    DPRINTF(_l("TffdshowDecVideoDXVA::GetMediaType"));
    CAutoLock lock(&inpin->m_csCodecs_and_imgFilters);

    if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }

    if (!presetSettings) {
        initPreset();
    }

    bool isVIH2;

    if (m_pOutput->IsConnected()) {
        const CLSID &ref = GetCLSID(m_pOutput->GetConnected());
        if (ref == CLSID_VideoMixingRenderer || ref == CLSID_VideoMixingRenderer9) {
            isVIH2 = true;
        }
    }

    isVIH2 = (iPosition & 1) == 0;

    iPosition /= 2;

    if (iPosition < 0) {
        return E_INVALIDARG;
    }

    TvideoCodecDec *pDecoder = NULL;
    getMovieSource((const TvideoCodecDec**)&pDecoder);
    if (!pDecoder->useDXVA()) {
        return VFW_S_NO_MORE_ITEMS;
    }

    TcspInfos ocsps;
    size_t osize;


    // DXVA mode : special output format
    TvideoCodecLibavcodecDxva *pDecoderDxva = (TvideoCodecLibavcodecDxva*)pDecoder;
    pDecoderDxva->getDXVAOutputFormats(ocsps);
    osize = ocsps.size();

    if ((size_t)iPosition >= osize) {
        return VFW_S_NO_MORE_ITEMS;
    }

    TffPictBase pictOut;
    if (inReconnect) {
        pictOut = reconnectRect;
    } else {
        pictOut = inpin->pictIn;
    }

    // Support mediatype with unknown dimension. This is necessary to support MEDIASUBTYPE_H264.
    // http://msdn.microsoft.com/en-us/library/dd757808(VS.85).aspx
    // The downstream filter has to support reconnecting after this.
    if (pictOut.rectFull.dx == 0) {
        pictOut.rectFull.dx = 320;
    }
    if (pictOut.rectFull.dy == 0) {
        pictOut.rectFull.dy = 160;
    }

    oldRect = pictOut.rectFull;

    const TcspInfo *c = ocsps[iPosition];
    BITMAPINFOHEADER bih;
    memset(&bih, 0, sizeof(bih));
    bih.biSize  = sizeof(BITMAPINFOHEADER);
    bih.biWidth = pDecoderDxva->pictWidthRounded();
    if (c->id == FF_CSP_420P) { // YV12 and odd number lines.
        pictOut.rectFull.dy = odd2even(pictOut.rectFull.dy);
    }
    bih.biHeight = pDecoderDxva->pictHeightRounded();
    bih.biPlanes = WORD(c->numPlanes);
    bih.biCompression = c->fcc;
    bih.biBitCount = WORD(c->bpp);
    bih.biSizeImage = DIBSIZE(bih); // bih.biWidth*bih.biHeight*bih.biBitCount>>3;

    mtOut->majortype = MEDIATYPE_Video;
    mtOut->subtype = *c->subtype;
    mtOut->formattype = isVIH2 ? FORMAT_VideoInfo2 : FORMAT_VideoInfo;
    mtOut->SetTemporalCompression(FALSE);
    mtOut->SetSampleSize(bih.biSizeImage);

    if (!isVIH2) {
        VIDEOINFOHEADER *vih = (VIDEOINFOHEADER*)mtOut->ReallocFormatBuffer(sizeof(VIDEOINFOHEADER));
        if (!vih) {
            return E_OUTOFMEMORY;
        }
        ZeroMemory(vih, sizeof(VIDEOINFOHEADER));

        vih->rcSource.left = 0;
        vih->rcSource.right = pictOut.rectFull.dx;
        vih->rcSource.top = 0;
        vih->rcSource.bottom = pictOut.rectFull.dy;
        vih->rcTarget = vih->rcSource;
        vih->AvgTimePerFrame = inpin->avgTimePerFrame;
        vih->bmiHeader = bih;
    } else {
        VIDEOINFOHEADER2 *vih2 = (VIDEOINFOHEADER2*)mtOut->ReallocFormatBuffer(sizeof(VIDEOINFOHEADER2));
        if (!vih2) {
            return E_OUTOFMEMORY;
        }
        ZeroMemory(vih2, sizeof(VIDEOINFOHEADER2));
        if ((presetSettings->resize && presetSettings->resize->is && presetSettings->resize->SARinternally && presetSettings->resize->mode == 0)) {
            pictOut.rectFull.sar.num = 1; //pictOut.rectFull.dx; // VMR9 behaves better when this is set to 1(SAR). But in reconnectOutput, it is different(DAR) in my system.
            pictOut.rectFull.sar.den = 1; //pictOut.rectFull.dy;
        }
        setVIH2aspect(vih2, pictOut.rectFull, presetSettings->output->hwOverlayAspect);

        //DPRINTF(_l("AR getMediaType: %i:%i"),vih2->dwPictAspectRatioX,vih2->dwPictAspectRatioY);

        vih2->rcSource.left = 0;
        vih2->rcSource.right = pictOut.rectFull.dx;
        vih2->rcSource.top = 0;
        vih2->rcSource.bottom = pictOut.rectFull.dy;
        vih2->rcTarget = vih2->rcSource;
        vih2->AvgTimePerFrame = inpin->avgTimePerFrame;
        vih2->bmiHeader = bih;
        //vih2->dwControlFlags=AMCONTROL_USED | AMCONTROL_COLORINFO_PRESENT | (DXVA_NominalRange_Wide << DXVA_NominalRangeShift) | (DXVA_VideoTransferMatrix_BT601 << DXVA_VideoTransferMatrixShift);
        hwDeinterlace = 1; // HW deinterlace for DXVA

        if (hwDeinterlace) {
            vih2->dwInterlaceFlags = AMINTERLACE_IsInterlaced | AMINTERLACE_DisplayModeBobOrWeave;
        }
    }
    return S_OK;
}


HRESULT TffdshowDecVideoDXVA::setOutputMediaType(const CMediaType &mt)
{
    DPRINTF(_l("TffdshowDecVideoDXVA::setOutputMediaType"));
    TvideoCodecDec *pDecoder = NULL;
    getMovieSource((const TvideoCodecDec**)&pDecoder);

    TcspInfos ocsps;
    // DXVA mode : special output format
    TvideoCodecLibavcodecDxva *pDecoderDxva = (TvideoCodecLibavcodecDxva*)pDecoder;
    pDecoderDxva->getDXVAOutputFormats(ocsps);


    for (int i = 0; cspFccs[i].name; i++) {
        const TcspInfo *cspInfo;
        // Look for the right DXVA colorspace
        bool ok = false;
        for (TcspInfos::const_iterator oc = ocsps.begin(); oc != ocsps.end(); oc++) {
            if (mt.subtype == *(*oc)->subtype) {
                cspInfo = (const TcspInfo *)(*oc);
                ok = true;
                break;
            }
        }
        if (!ok) {
            continue;
        }
        m_frame.dstColorspace = FF_CSP_NV12;

        int biWidth, outDy;
        BITMAPINFOHEADER *bih;
        if (mt.formattype == FORMAT_VideoInfo && mt.pbFormat) { // && mt.pbFormat = work around other filter's bug.
            VIDEOINFOHEADER *vih = (VIDEOINFOHEADER*)mt.pbFormat;
            m_frame.dstStride = calcBIstride(biWidth = vih->bmiHeader.biWidth, cspInfo->Bpp * 8);
            outDy = vih->bmiHeader.biHeight;
            bih = &vih->bmiHeader;
        } else if (mt.formattype == FORMAT_VideoInfo2 && mt.pbFormat) {
            VIDEOINFOHEADER2 *vih2 = (VIDEOINFOHEADER2*)mt.pbFormat;
            m_frame.dstStride = calcBIstride(biWidth = vih2->bmiHeader.biWidth, cspInfo->Bpp * 8);
            outDy = vih2->bmiHeader.biHeight;
            bih = &vih2->bmiHeader;
        } else {
            return VFW_E_TYPE_NOT_ACCEPTED;    //S_FALSE;
        }
        m_frame.dstSize = DIBSIZE(*bih);

        char_t s[256];
        DPRINTF(_l("TffdshowDecVideoDXVA::setOutputMediaType: colorspace:%s, biWidth:%i, dstStride:%i, Bpp:%i, dstSize:%i"), csp_getName(m_frame.dstColorspace, s, 256), biWidth, m_frame.dstStride, cspInfo->Bpp, m_frame.dstSize);
        if (csp_isRGB(m_frame.dstColorspace) && outDy > 0) {
            m_frame.dstColorspace |= FF_CSP_FLAGS_VFLIP;
        }
        //else if (biheight<0)
        // m_frame.colorspace|=FF_CSP_FLAGS_VFLIP;
        return S_OK;
    }
    m_frame.dstColorspace = FF_CSP_NULL;
    DPRINTF(_l("TffdshowDecVideoDXVA::setOutputMediaType Type not supported by FFDShow DXVA"));
    return VFW_E_TYPE_NOT_ACCEPTED; //S_FALSE;
}

HRESULT TffdshowDecVideoDXVA::CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin)
{
    if (direction == PINDIR_INPUT) {
        DPRINTF(_l("TffdshowDecVideoDXVA::CompleteConnect input"));
    } else if (direction == PINDIR_OUTPUT) {
        DPRINTF(_l("TffdshowDecVideoDXVA::CompleteConnect output"));
        TvideoCodecDec *pDecoder = NULL;
        getMovieSource((const TvideoCodecDec**)&pDecoder);
        if (pDecoder != NULL && pDecoder->useDXVA() != 0) {
            TvideoCodecLibavcodecDxva *pDecoderDxva = (TvideoCodecLibavcodecDxva*)pDecoder;
            if (!pDecoderDxva->checkDXVAMode(pReceivePin)) {
                DPRINTF(_l("TffdshowDecVideoDXVA::CompleteConnect output DXVA not supported"));
                return E_FAIL;
            }
        } else {
            DPRINTF(_l("TffdshowDecVideoDXVA::CompleteConnect output DXVA 1 & 2 not supported"));
            return E_FAIL;
        }
        const CLSID &out = GetCLSID(m_pOutput->GetConnected());
        outOverlayMixer = !!(out == CLSID_OverlayMixer);
    }
    return CTransformFilter::CompleteConnect(direction, pReceivePin);
}

// alloc output buffer
HRESULT TffdshowDecVideoDXVA::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    DPRINTF(_l("TffdshowDecVideoDXVA::DecideBufferSize"));
    if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }

    if (!presetSettings) {
        initPreset();
    }
    if (m_IsQueueListedApp == -1) { // Not initialized
        m_IsQueueListedApp = IsQueueListedApp(getExeflnm());
    }


    TvideoCodecDec *pDecoder = NULL;
    getMovieSource((const TvideoCodecDec**)&pDecoder);
    if (pDecoder != NULL && pDecoder->useDXVA() == 2) { // DXVA2 : allocator must be allocated inside the decoder (DXVA1 : allocator managed by the renderer)
        TvideoCodecLibavcodecDxva *pDecoderDxva = (TvideoCodecLibavcodecDxva*)pDecoder;
        HRESULT hr;
        ALLOCATOR_PROPERTIES Actual;

        ppropInputRequest->cBuffers = pDecoderDxva->getPicEntryNumber();

        if (FAILED(hr = pAlloc->SetProperties(ppropInputRequest, &Actual))) {
            return hr;
        }

        return ppropInputRequest->cBuffers > Actual.cBuffers || ppropInputRequest->cbBuffer > Actual.cbBuffer
               ? E_FAIL : NOERROR;
    }

    m_IsOldVideoRenderer = IsOldRenderer();
    const CLSID &ref = GetCLSID(m_pOutput->GetConnected());
    if (isQueue == -1) {
        isQueue = presetSettings->multiThread && m_IsQueueListedApp;
    }
    // Queue and Overlay Mixer works only in MPC and
    // when Overlay Mixer is not connected to old video renderer(rare, usually RGB out).
    // If queue can't work with Overlay Mixer, IsOldRenderer() returns true.
    isQueue = isQueue && !m_IsOldVideoRenderer &&
              (ref == CLSID_OverlayMixer || ref == CLSID_VideoMixingRenderer || ref == CLSID_VideoMixingRenderer9);
    m_IsOldVMR9RenderlessAndRGB = IsOldVMR9RenderlessAndRGB();
    isQueue = isQueue && !(m_IsOldVMR9RenderlessAndRGB); // inform MPC about queue only when queue is effective.
    // DPRINTF(_l("CLSID 0x%x,0x%x,0x%x"),ref.Data1,ref.Data2,ref.Data3);for(int i=0;i<8;i++) {DPRINTF(_l(",0x%2x"),ref.Data4[i]);}
    if (ref == CLSID_VideoRenderer || ref == CLSID_OverlayMixer) {
        return DecideBufferSizeOld(pAlloc, ppropInputRequest, ref);
    } else {
        return DecideBufferSizeVMR(pAlloc, ppropInputRequest, ref);
    }
}

int TffdshowDecVideoDXVA::get_trayIconType(void)
{
    return IDI_MODERN_ICON_VA;
}
