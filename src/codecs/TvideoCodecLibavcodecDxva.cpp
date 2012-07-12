/*
 * Copyright (c) 2009 Damien Bain-Thouverez
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

#include "ffmpeg/libavcodec/avcodec.h"
#include "ffmpeg/Tlibavcodec.h"
#include "IffdshowBase.h"
#include "IffdshowDecVideo.h"
#include "IffdshowEnc.h"
#include "cc_decoder.h"
#include "TvideoCodecLibavcodec.h"
#include "TvideoCodecLibavcodecDxva.h"
#include "TDXVADecoder.h"
#include "TglobalSettings.h"
#include "ffdshow_mediaguids.h"
#include "TcodecSettings.h"
#include "rational.h"
#include "qtpalette.h"
#include "line.h"
#include "simd.h"
#include <mmreg.h>
#include "TffdshowVideoInputPin.h"

inline int LNKO(int a, int b)
{
    if (a == 0 || b == 0) {
        return(1);
    }
    while (a != b) {
        if (a < b) {
            b -= a;
        } else if (a > b) {
            a -= b;
        }
    }
    return(a);
}

// DXVA modes supported for Mpeg2 TODO
DXVA_PARAMS DXVA_Mpeg2 = {
    14, // PicEntryNumber
    1,  // PreferedConfigBitstream
    { &DXVA_ModeMPEG2_A,            &DXVA_ModeMPEG2_C,                    &GUID_NULL },
    { DXVA_RESTRICTED_MODE_MPEG2_A,  DXVA_RESTRICTED_MODE_MPEG2_C,     0 }
};

// Note regarding PicEntryNumber (H.264):
// During H.264 decoding process, there is a maximum of 16 frames in the DPB, in addition to the current frame (17 total)
// num_reorder_frame <= MaxDpbFrames <= 16
// [ num_reorder_frame - the maximum number of frames that precede any frame in decoding order and follow it in output order ]
// [ L4.1@High: MaxDpbFrames =  ((12,288 / 1.5) * 1024) / (width * height) ]
//
// We want to have enough DX surfaces to hold decoded frames until it's time to output them,
// and we need num_reorder_frame + 1 surfaces, so we need a maximum of 17 DX surfaces
// unfortunately, VMR 9 (Windows XP) supports up to 16 surfaces.
// this may only affect low-resolution videos (~960x540 and below) where num_reorder_frame = 16, and those are extremely rare.
//
// for more info regarding frame re-ordering: see http://lists.mplayerhq.hu/pipermail/ffmpeg-devel/2006-August/013793.html
//
// DXVA modes supported for H264
DXVA_PARAMS DXVA_H264 = {
    16, // PicEntryNumber
    2,  // PreferedConfigBitstream
    { &DXVA2_ModeH264_E, &DXVA2_ModeH264_F, &DXVA_Intel_H264_ClearVideo, &GUID_NULL },
    { DXVA_RESTRICTED_MODE_H264_E, 0}
};

DXVA_PARAMS DXVA_H264_VISTA = {
    17, // PicEntryNumber
    2,  // PreferedConfigBitstream
    { &DXVA2_ModeH264_E, &DXVA2_ModeH264_F, &DXVA_Intel_H264_ClearVideo, &GUID_NULL },
    { DXVA_RESTRICTED_MODE_H264_E, 0}
};

// DXVA modes supported for VC1
DXVA_PARAMS DXVA_VC1 = {
    14, // PicEntryNumber
    1,  // PreferedConfigBitstream
    { &DXVA2_ModeVC1_D, &GUID_NULL },
    { DXVA_RESTRICTED_MODE_VC1_D, 0}
};

// Static list for DXVA2
TcspInfo dxva2List[] = {
    {FF_CSP_NV12, _l("avxd"), 1, 12, 1, {0, 0, 0, 0}, {0, 1, 1, 0}, {0, 128, 128, 0}, 'avxd', 'avxd', &MEDIASUBTYPE_NV12},
    {FF_CSP_NV12, _l("AVXD"), 1, 12, 1, {0, 0, 0, 0}, {0, 1, 1, 0}, {0, 128, 128, 0}, 'AVXD', 'AVXD', &MEDIASUBTYPE_NV12},
    {FF_CSP_NV12, _l("AVxD"), 1, 12, 1, {0, 0, 0, 0}, {0, 1, 1, 0}, {0, 128, 128, 0}, 'AVxD', 'AVxD', &MEDIASUBTYPE_NV12},
    {FF_CSP_NV12, _l("AvXD"), 1, 12, 1, {0, 0, 0, 0}, {0, 1, 1, 0}, {0, 128, 128, 0}, 'AvXD', 'AvXD', &MEDIASUBTYPE_NV12}
};


TvideoCodecLibavcodecDxva::TvideoCodecLibavcodecDxva(IffdshowBase *Ideci, IdecVideoSink *IsinkD, CodecID IcodecId):
    Tcodec(Ideci), TcodecDec(Ideci, IsinkD),
    TvideoCodec(Ideci),
    TvideoCodecLibavcodec(Ideci, IsinkD),
    isDXVACompatible(true),
    hDevice(INVALID_HANDLE_VALUE),
    initDXVAMode(false),
    pDXVADecoder(NULL)
{
    TvideoCodecLibavcodec::create();
    dxvaCodecId = codecId = IcodecId;
    create();
    ok = libavcodec ? libavcodec->ok : false;
}


//----------------------- TvideoCodecLivavcodec overload ---------------------------------------------

void TvideoCodecLibavcodecDxva::create(void)
{
    nARMode = 1;
    inPosB = 1;
    nDXVAMode = MODE_SOFTWARE;
    pDXVADecoder = NULL;
    sar = AVRational();
    switch (dxvaCodecId) {
        case CODEC_ID_H264_DXVA:
            dxvaParamsp = &DXVA_H264;
            if (isVista()) {
                dxvaParamsp = &DXVA_H264_VISTA;
            }
            break;
        case CODEC_ID_VC1_DXVA:
            dxvaParamsp = &DXVA_VC1;
            break;
            /*case CODEC_ID_MPEG2_DXVA:dxvaParamsp=&DXVA_Mpeg2;break;
            */
    }
}

TvideoCodecLibavcodecDxva::~TvideoCodecLibavcodecDxva()
{
    cleanup();
}

bool TvideoCodecLibavcodecDxva::checkDXVAMode(IPin *pReceivePin)
{
    DPRINTF(_l("TvideoCodecLibavcodecDxva::checkDXVAMode Checking for DXVA compatibility"));
    if (isDXVASupported()) {
        if (pReceivePin == NULL) {
            return true;
        }

        if (connectedSplitter == TffdshowVideoInputPin::MPC_mpegSplitters) {
            bReorderBFrame = false;
        }

        if (nDXVAMode == MODE_DXVA1) {
            if (SUCCEEDED(pDXVADecoder->ConfigureDXVA1())) {
                DPRINTF(_l("TvideoCodecLibavcodecDxva::checkDXVAMode DXVA1 configured successfully"));
                return true;
            }
        } else if (SUCCEEDED(configureDXVA2(pReceivePin))) {
            DPRINTF(_l("TvideoCodecLibavcodecDxva::checkDXVAMode DXVA2 configured successfully"));
            if SUCCEEDED(setEVRForDXVA2(pReceivePin)) {
                nDXVAMode  = MODE_DXVA2;
            }
            return true;
        }
        // We had a receivePin to check after DXVA and the mode didn't change : this means that it is not compatible
        if (nDXVAMode == MODE_SOFTWARE) {
            return false;    // DXVA not supported
        }
    }
    return false;

    /*TODO MPC
     if ( ((m_pOutput->CurrentMediaType().subtype == MEDIASUBTYPE_NV12) && (m_nDXVAMode == MODE_SOFTWARE)) ||
            ((m_pOutput->CurrentMediaType().subtype == MEDIASUBTYPE_YUY2) && (m_pAVCtx->width&1 || m_pAVCtx->height&1)) )
           return VFW_E_INVALIDMEDIATYPE;
      */
}

bool TvideoCodecLibavcodecDxva::onSeek(REFERENCE_TIME segmentStart)
{
    if (pDXVADecoder) {
        pDXVADecoder->Flush();
    }
    return TvideoCodecLibavcodec::onSeek(segmentStart);
}

#pragma region TvideoCodecLibavcodecDxva methods

bool  TvideoCodecLibavcodecDxva::isVista()
{
    OSVERSIONINFO osver;
    osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&osver) && osver.dwPlatformId == VER_PLATFORM_WIN32_NT &&
            (osver.dwMajorVersion >= 6)) {
        return true;
    }

    return false;
}

void TvideoCodecLibavcodecDxva::cleanup()
{
    SAFE_DELETE(pDXVADecoder);

    // Release DXVA ressources
    if (hDevice != INVALID_HANDLE_VALUE) {
        m_pDeviceManager->CloseDeviceHandle(hDevice);
        hDevice = INVALID_HANDLE_VALUE;
    }

    m_pDeviceManager     = NULL;
    m_pDecoderService    = NULL;
    pDecoderRenderTarget = NULL;
}

int TvideoCodecLibavcodecDxva::pictWidthRounded()
{
    // Picture height should be rounded to 16 for DXVA
    return ((avctx->width + 15) / 16) * 16;
}

int TvideoCodecLibavcodecDxva::pictHeightRounded()
{
    // Picture height should be rounded to 16 for DXVA
    return ((avctx->height + 15) / 16) * 16;
}


void TvideoCodecLibavcodecDxva::detectVideoCard(HWND hwnd)
{
    IDirect3D9* pD3D9;
    nPCIVendor = 0;
    nPCIDevice = 0;
    videoDriverVersion.HighPart = 0;
    videoDriverVersion.LowPart = 0;

    if (pD3D9 = Direct3DCreate9(D3D_SDK_VERSION)) {
        D3DADAPTER_IDENTIFIER9 adapterIdentifier;
        if (pD3D9->GetAdapterIdentifier(getAdapter(pD3D9, hwnd), 0, &adapterIdentifier) == S_OK) {
            nPCIVendor = adapterIdentifier.VendorId;
            nPCIDevice = adapterIdentifier.DeviceId;
            videoDriverVersion = adapterIdentifier.DriverVersion;
            strDeviceDescription = text<char_t>(adapterIdentifier.Description);
            strDeviceDescription.append(ffstring(_l(" (")) + ffstring::intToStr(nPCIVendor) + ffstring(_l(")")));
        }
        pD3D9->Release();
    }
}

BOOL CALLBACK EnumFindProcessWnd(HWND hwnd, LPARAM lParam)
{
    DWORD procid = 0;
    TCHAR WindowClass [40];
    GetWindowThreadProcessId(hwnd, &procid);
    GetClassName(hwnd, WindowClass, countof(WindowClass));

    if (procid == GetCurrentProcessId() &&
            (_tcscmp(WindowClass, _l("MediaPlayerClassicW")) == 0 || // MPC-HC window
             _tcscmp(WindowClass, _l("WMPlayerApp")) == 0 || // WMPlayer window
             _tcscmp(WindowClass, _l("eHome Render Window")) == 0 // WMC window
            )
       ) {
        HWND* pWnd = (HWND*) lParam;
        *pWnd = hwnd;
        return FALSE;
    }
    return TRUE;
}

bool TvideoCodecLibavcodecDxva::isDXVASupported(void)
{
    HWND hWnd = NULL;
    EnumWindows(EnumFindProcessWnd, (LPARAM)&hWnd);
    detectVideoCard(hWnd);

    bool isDXVACompatible = true;

    if (dxvaCodecId == CODEC_ID_H264_DXVA) {
        /* a non-zero value indicates that an incompatibility was detected */
        int nCompat = 0;
        if (pictWidthRounded() > 1920 || pictHeightRounded() > 1440) {
            isDXVACompatible = false;
        } else {
            nCompat = libavcodec->FFH264CheckCompatibility(pictWidthRounded(), pictHeightRounded(), avctx, (BYTE*)avctx->extradata, avctx->extradata_size, nPCIVendor, nPCIDevice, videoDriverVersion);
        }

        if (nCompat > 0) {
            int nCompatibilityMode = deci->getParam2(IDFF_dec_DXVA_CompatibilityMode);

            // debug output
            if (nCompat & DXVA_UNSUPPORTED_LEVEL) {
                DPRINTF(_l("TvideoCodecLibavcodecDxva::isDXVASupported : unsupported level"));
            }
            if (nCompat & DXVA_TOO_MUCH_REF_FRAMES) {
                DPRINTF(_l("TvideoCodecLibavcodecDxva::isDXVASupported : too much reference frames"));
            }

            switch (nCompatibilityMode) {
                case 0:
                    // full check
                    isDXVACompatible = false;
                    break;
                case 1 :
                    // skip level check
                    if (nCompat != DXVA_UNSUPPORTED_LEVEL) {
                        isDXVACompatible = false;
                    }
                    break;
                case 2 :
                    // skip reference frame check
                    if (nCompat != DXVA_TOO_MUCH_REF_FRAMES) {
                        isDXVACompatible = false;
                    }
                    break;
                case 3 :
                    // skip all checks
                    //if(nCompat != (DXVA_UNSUPPORTED_LEVEL | DXVA_TOO_MUCH_REF_FRAMES)) m_bDXVACompatible = false; // example of how a combination of two ignored checks can be done
                    break;
            }
        }
    }

    return isDXVACompatible;
}

void TvideoCodecLibavcodecDxva::getDXVAOutputFormats(TcspInfos &ocsps)
{
    int nVideoOutputCount = 0;
    for (nVideoOutputCount = 0; nVideoOutputCount < MAX_SUPPORTED_MODE; nVideoOutputCount++)
        if (dxvaParamsp->Decoder[nVideoOutputCount] == &GUID_NULL) {
            break;
        }
    ocsps.clear();
    TcspInfo csp = {
        FF_CSP_NV12, _l("avxd"),
        1, 12, //Bpp
        1, //numplanes
        {0, 0, 0, 0}, //shiftX
        {0, 1, 1, 0}, //shiftY
        {0, 128, 128, 0}, //black
        'avxd', 'avxd', &GUID_NULL
    };
    // Dynamic DXVA media types for DXVA1
    for (int nPos = 0; nPos < nVideoOutputCount; nPos++) {
        TcspInfo *pCsp = new TcspInfo(csp);
        pCsp->subtype = dxvaParamsp->Decoder[nPos];
        ocsps.push_back(pCsp);
    }
    // Static list for DXVA2
    nVideoOutputCount = SIZEOF_ARRAY(dxva2List);
    for (int nPos = 0; nPos < nVideoOutputCount; nPos++) {
        ocsps.push_back((TcspInfo *)&dxva2List[nPos]);
    }
}

int TvideoCodecLibavcodecDxva::getPicEntryNumber(void)
{
    if (isDXVASupported()) {
        return dxvaParamsp->PicEntryNumber;
    } else {
        return 0;
    }
}

BOOL TvideoCodecLibavcodecDxva::isSupportedDecoderConfig(const D3DFORMAT nD3DFormat, const DXVA2_ConfigPictureDecode& config, bool& bIsPrefered)
{
    bool    bRet = false;

    // TODO : not finished
    bRet = (nD3DFormat == MAKEFOURCC('N', 'V', '1', '2'));

    bIsPrefered = (config.ConfigBitstreamRaw == dxvaParamsp->PreferedConfigBitstream);
    DPRINTF(_l("TvideoCodecLibavcodecDxva::isSupportedDecoderConfig  0x%08x  %d"), nD3DFormat, bRet);
    return bRet;
}

void TvideoCodecLibavcodecDxva::fillInVideoDescription(DXVA2_VideoDesc *pDesc)
{
    memset(pDesc, 0, sizeof(DXVA2_VideoDesc));
    pDesc->SampleWidth        = pictWidthRounded();
    pDesc->SampleHeight       = pictHeightRounded();
    pDesc->Format             = D3DFMT_A8R8G8B8;
    pDesc->UABProtectionLevel = 1;
}

HRESULT TvideoCodecLibavcodecDxva::findDXVA2DecoderConfiguration(IDirectXVideoDecoderService *pDecoderService,
        const GUID& guidDecoder,
        DXVA2_ConfigPictureDecode *pSelectedConfig,
        BOOL *pbFoundDXVA2Configuration)
{
    HRESULT hr = S_OK;
    UINT cFormats = 0;
    UINT cConfigurations = 0;
    bool bIsPrefered = false;

    D3DFORMAT                   *pFormats = NULL;           // size = cFormats
    DXVA2_ConfigPictureDecode   *pConfig = NULL;            // size = cConfigurations

    // Find the valid render target formats for this decoder GUID.
    hr = pDecoderService->GetDecoderRenderTargets(guidDecoder, &cFormats, &pFormats);
    DPRINTF(_l("TvideoCodecLibavcodecDxva::findDXVA2DecoderConfiguration GetDecoderRenderTargets => %d"), cFormats);

    if (SUCCEEDED(hr)) {
        // Look for a format that matches our output format.
        for (UINT iFormat = 0; iFormat < cFormats;  iFormat++) {
            DPRINTF(_l("TvideoCodecLibavcodecDxva::findDXVA2DecoderConfiguration Try to negociate => 0x%08x"), pFormats[iFormat]);

            // Fill in the video description. Set the width, height, format, and frame rate.
            fillInVideoDescription(&videoDesc); // Private helper function.
            videoDesc.Format = pFormats[iFormat];

            // Get the available configurations.
            hr = pDecoderService->GetDecoderConfigurations(guidDecoder, &videoDesc, NULL, &cConfigurations, &pConfig);

            if (FAILED(hr)) {
                continue;
            }

            // Find a supported configuration.
            for (UINT iConfig = 0; iConfig < cConfigurations; iConfig++) {
                if (isSupportedDecoderConfig(pFormats[iFormat], pConfig[iConfig], bIsPrefered)) {
                    // This configuration is good.
                    if (bIsPrefered || !*pbFoundDXVA2Configuration) {
                        *pbFoundDXVA2Configuration = TRUE;
                        *pSelectedConfig = pConfig[iConfig];
                    }

                    if (bIsPrefered) {
                        break;
                    }
                }
            }

            CoTaskMemFree(pConfig);
        } // End of formats loop.
    }

    CoTaskMemFree(pFormats);

    // Note: It is possible to return S_OK without finding a configuration.
    return hr;
}

HRESULT TvideoCodecLibavcodecDxva::configureDXVA2(IPin *pPin)
{
    HRESULT hr                       = S_OK;
    UINT    cDecoderGuids            = 0;
    BOOL    bFoundDXVA2Configuration = FALSE;
    GUID    guidDecoder              = GUID_NULL;

    DXVA2_ConfigPictureDecode config;
    ZeroMemory(&config, sizeof(config));

    CComPtr<IMFGetService>               pGetService;
    CComPtr<IDirect3DDeviceManager9>     pDeviceManager;
    CComPtr<IDirectXVideoDecoderService> pDecoderService;
    GUID*                                pDecoderGuids = NULL;
    HANDLE                               hdevice = INVALID_HANDLE_VALUE;

    // Query the pin for IMFGetService.
    hr = pPin->QueryInterface(__uuidof(IMFGetService), (void**)&pGetService);

    // Get the Direct3D device manager.
    if (SUCCEEDED(hr))
        hr = pGetService->GetService(
                 MR_VIDEO_ACCELERATION_SERVICE,
                 __uuidof(IDirect3DDeviceManager9),
                 (void**)&pDeviceManager);

    // Open a new device handle.
    if (SUCCEEDED(hr)) {
        hr = pDeviceManager->OpenDeviceHandle(&hdevice);
    }

    // Get the video decoder service.
    if (SUCCEEDED(hr))
        hr = pDeviceManager->GetVideoService(
                 hdevice,
                 __uuidof(IDirectXVideoDecoderService),
                 (void**)&pDecoderService);

    // Get the decoder GUIDs.
    if (SUCCEEDED(hr)) {
        hr = pDecoderService->GetDecoderDeviceGuids(&cDecoderGuids, &pDecoderGuids);
    }

    if (SUCCEEDED(hr))
        // Look for the decoder GUIDs we want.
        for (UINT iGuid = 0; iGuid < cDecoderGuids; iGuid++) {
            bool found = false;
            // Do we support this mode?
            for (int i = 0; i < MAX_SUPPORTED_MODE; i++) {
                if (*dxvaParamsp->Decoder[i] == GUID_NULL) {
                    break;
                }
                if (*dxvaParamsp->Decoder[i] == pDecoderGuids[iGuid]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                continue;
            }
            // Find a configuration that we support.
            hr = findDXVA2DecoderConfiguration(pDecoderService, pDecoderGuids[iGuid], &config, &bFoundDXVA2Configuration);
            if (FAILED(hr)) {
                break;
            }

            // Sandy Bridge crashes with Mode_E on current code, so ignore it
            // known device IDs for SB integrated graphics are: 258, 274, 278, 290, 294
            if (nPCIVendor == PCIV_Intel && nPCIDevice >= 258 && nPCIDevice <= 294 && pDecoderGuids[iGuid] == DXVA2_ModeH264_E) {
                bFoundDXVA2Configuration = false;
            }

            if (bFoundDXVA2Configuration) { // Found a good configuration. Save the GUID.
                guidDecoder = pDecoderGuids[iGuid];
                break;
            }
        }

    if (pDecoderGuids) {
        CoTaskMemFree(pDecoderGuids);
    }
    if (!bFoundDXVA2Configuration) {
        hr = E_FAIL;    // Unable to find a configuration.
    }

    if (SUCCEEDED(hr)) {
        // Store the things we will need later.
        m_pDeviceManager  = pDeviceManager;
        m_pDecoderService = pDecoderService;
        dxva2Config       = config;
        dxvaDecoderGUID   = guidDecoder;
        hDevice           = hdevice;
    }
    if (FAILED(hr))
        if (hDevice != INVALID_HANDLE_VALUE) {
            pDeviceManager->CloseDeviceHandle(hdevice);
        }

    return hr;
}

HRESULT TvideoCodecLibavcodecDxva::setEVRForDXVA2(IPin *pPin)
{
    HRESULT hr = S_OK;

    CComPtr<IMFGetService>                    pGetService;
    CComPtr<IDirectXVideoMemoryConfiguration> pVideoConfig;
    CComPtr<IMFVideoDisplayControl>           pVdc;

    // Query the pin for IMFGetService.
    hr = pPin->QueryInterface(__uuidof(IMFGetService), (void**)&pGetService);

    // Get the IDirectXVideoMemoryConfiguration interface.
    if (SUCCEEDED(hr)) {
        hr = pGetService->GetService(
                 MR_VIDEO_ACCELERATION_SERVICE,
                 __uuidof(IDirectXVideoMemoryConfiguration),
                 (void**)&pVideoConfig);

        if (SUCCEEDED(pGetService->GetService(MR_VIDEO_RENDER_SERVICE, __uuidof(IMFVideoDisplayControl), (void**)&pVdc))) {
            HWND hWnd;
            if (SUCCEEDED(pVdc->GetVideoWindow(&hWnd))) {
                detectVideoCard(hWnd);
            }
        }
    }

    // Notify the EVR.
    if (SUCCEEDED(hr)) {
        DXVA2_SurfaceType surfaceType;

        for (DWORD iTypeIndex = 0; ; iTypeIndex++) {
            hr = pVideoConfig->GetAvailableSurfaceTypeByIndex(iTypeIndex, &surfaceType);

            if (FAILED(hr)) {
                break;
            }

            if (surfaceType == DXVA2_SurfaceType_DecoderRenderTarget) {
                hr = pVideoConfig->SetSurfaceType(DXVA2_SurfaceType_DecoderRenderTarget);
                break;
            }
        }
    }
    return hr;
}

HRESULT TvideoCodecLibavcodecDxva::createDXVA2Decoder(UINT nNumRenderTargets, IDirect3DSurface9** pDecoderRenderTargets)
{
    HRESULT hr;
    CComPtr<IDirectXVideoDecoder> pDirectXVideoDec;

    pDecoderRenderTarget    = NULL;

    if (pDXVADecoder) {
        pDXVADecoder->SetDirectXVideoDec(NULL);
    }

    hr = m_pDecoderService->CreateVideoDecoder(dxvaDecoderGUID, &videoDesc, &dxva2Config,
            pDecoderRenderTargets, nNumRenderTargets, &pDirectXVideoDec);

    if (SUCCEEDED(hr)) {
        if (!pDXVADecoder) {
            pDXVADecoder    = TDXVADecoder::CreateDecoder(deciV, pDirectXVideoDec, &dxvaDecoderGUID, getPicEntryNumber(), &dxva2Config);
            if (pDXVADecoder) {
                pDXVADecoder->SetExtraData((BYTE*)avctx->extradata, avctx->extradata_size);
            }
        }

        pDXVADecoder->SetDirectXVideoDec(pDirectXVideoDec);
    }

    return hr;
}

HRESULT TvideoCodecLibavcodecDxva::findDXVA1DecoderConfiguration(IAMVideoAccelerator* pAMVideoAccelerator, const GUID* guidDecoder, DDPIXELFORMAT* pPixelFormat)
{
    HRESULT        hr            = E_FAIL;
    DWORD          dwFormats     = 0;
    DDPIXELFORMAT* pPixelFormats = NULL;


    pAMVideoAccelerator->GetUncompFormatsSupported(guidDecoder, &dwFormats, NULL);
    if (dwFormats > 0) {
        // Find the valid render target formats for this decoder GUID.
        pPixelFormats = new DDPIXELFORMAT[dwFormats];
        hr = pAMVideoAccelerator->GetUncompFormatsSupported(guidDecoder, &dwFormats, pPixelFormats);
        if (SUCCEEDED(hr)) {
            // Look for a format that matches our output format.
            for (DWORD iFormat = 0; iFormat < dwFormats; iFormat++) {
                if (pPixelFormats[iFormat].dwFourCC == MAKEFOURCC('N', 'V', '1', '2')) {
                    memcpy(pPixelFormat, &pPixelFormats[iFormat], sizeof(DDPIXELFORMAT));
                    SAFE_ARRAYDELETE(pPixelFormats)
                    return S_OK;
                }
            }

            SAFE_ARRAYDELETE(pPixelFormats);
            hr = E_FAIL;
        }
    }
    return hr;
}

HRESULT TvideoCodecLibavcodecDxva::checkDXVA1Decoder(const GUID *pGuid)
{
    for (int i = 0; i < MAX_SUPPORTED_MODE; i++)
        if (*dxvaParamsp->Decoder[i] == *pGuid) {
            return S_OK;
        }
    return E_INVALIDARG;
}

void TvideoCodecLibavcodecDxva::setDXVA1Params(const GUID* pGuid, DDPIXELFORMAT* pPixelFormat)
{
    dxvaDecoderGUID = *pGuid;
    memcpy(&pixelFormat, pPixelFormat, sizeof(DDPIXELFORMAT));
}

WORD TvideoCodecLibavcodecDxva::getDXVA1RestrictedMode()
{
    for (int i = 0; i < MAX_SUPPORTED_MODE; i++)
        if (*dxvaParamsp->Decoder[i] == dxvaDecoderGUID) {
            return dxvaParamsp->RestrictedMode[i];
        }
    return DXVA_RESTRICTED_MODE_UNRESTRICTED;
}

HRESULT TvideoCodecLibavcodecDxva::createDXVA1Decoder(IAMVideoAccelerator*  pAMVideoAccelerator, const GUID* pDecoderGuid, DWORD dwSurfaceCount)
{
    // commented because we may want to connect VMR9 after disconnecting overlay mixer (for example), and we have to make sure m_pAMVideoAccelerator is being updated.
    /*if (pDXVADecoder && dxvaDecoderGUID == *pDecoderGuid)
        return S_OK;*/
    SAFE_DELETE(pDXVADecoder);

    nDXVAMode       = MODE_DXVA1;
    dxvaDecoderGUID = *pDecoderGuid;
    pDXVADecoder    = TDXVADecoder::CreateDecoder(deciV, pAMVideoAccelerator, &dxvaDecoderGUID, dwSurfaceCount);
    if (pDXVADecoder) {
        pDXVADecoder->SetExtraData((BYTE*)avctx->extradata, avctx->extradata_size);
    }

    return S_OK;
}

STDMETHODIMP_(GUID*) TvideoCodecLibavcodecDxva::getDXVADecoderGUID(void)
{
    IFilterGraph* pGraph = NULL;
    deci->getGraph(&pGraph);
    if (pGraph == NULL) {
        return NULL;
    } else {
        return &dxvaDecoderGUID;
    }
}

void TvideoCodecLibavcodecDxva::updateAspectRatio(void)
{
    if (((nARMode) && (avctx)) && ((avctx->sample_aspect_ratio.num > 0) && (avctx->sample_aspect_ratio.den > 0))) {
        sar = avctx->sample_aspect_ratio;
        /*TODO : imported from MPCHC, update of aspect ratio
        if(sar != SAR)
        {
            sar = SAR;
            CSize aspect(mb_width * SAR.cx, mb_height * SAR.cy);
            int lnko = LNKO(aspect.cx, aspect.cy);
            if(lnko > 1) aspect.cx /= lnko, aspect.cy /= lnko;
            //SetAspect(aspect);  TODO : update TffPict ratio
        } */
    }
}
REFERENCE_TIME TvideoCodecLibavcodecDxva::getAvrTimePerFrame(void)
{
    if (avgTimePerFrame == -1) {
        deciV->getAverageTimePerFrame(&avgTimePerFrame);
    }
    return avgTimePerFrame;
}

bool TvideoCodecLibavcodecDxva::isPostProcessingEnabled(void)
{
    int nPostProcessingMode = deci->getParam2(IDFF_dec_DXVA_PostProcessingMode);
    return (nPostProcessingMode > 0);
}

void TvideoCodecLibavcodecDxva::PostProcessUSWCFrame(void * pDXVABuffer, UINT pitch)
{
    UINT width = pictWidthRounded();
    UINT height = pictHeightRounded();

    int nPostProcessingMode = deci->getParam2(IDFF_dec_DXVA_PostProcessingMode);

    switch (nPostProcessingMode) {
        case 1: { //overlay subs / OSD on top of the decoded buffer
            Tbuffer processedFrame;
            GetProcessedFrame(processedFrame, width, height);
            OverlayYV12OnUSWCFrame(processedFrame, (unsigned char*)pDXVABuffer, width, height, pitch);
            processedFrame.clear();
            break;
        }
        default:
            break;
    }
}

void TvideoCodecLibavcodecDxva::GetProcessedFrame(Tbuffer &processedFrame, UINT width, UINT height)
{
    int size = width * height * 3 / 2;
    uint64_t csp = FF_CSP_420P | FF_CSP_FLAGS_YUV_ADJ; // we use YV12. there is no native FF_CSP_NV12 processing
    processedFrame.free = false;
    // process frame over black background:
    TffPict pict;
    pict.alloc(width, height, csp, processedFrame, 0);
    ((TffdshowDecVideo*)sinkD)->processDecodedSample(pict);
}

// performance might be improved by using _mm_stream_si32
// to improve performance, we're doing here the conversion from YV12 to NV12 (USWC frame is NV12)
// YV12 - Y,V,U - Y plane, Cr plane, Cb plane
// NV12 - Y,U,V - Y plane, packed Cb, Cr samples
void TvideoCodecLibavcodecDxva::OverlayYV12OnUSWCFrame(unsigned char * pSrc, unsigned char * pDest, UINT width, UINT height, UINT pitch)
{
    unsigned int x, y;

    for (y = 0; y < height; y += 2) {
        for (x = 0; x < width; x += 2) {
            int srcY0Offset = y * width + x;
            int srcY1Offset = srcY0Offset + width;
            int srcCrOffset = width * height + y / 2 * width / 2 + x / 2;
            int srcCbOffset = srcCrOffset + width * height / 4;
            unsigned char srcY00 = pSrc[srcY0Offset + 0];
            unsigned char srcY01 = pSrc[srcY0Offset + 1];
            unsigned char srcY10 = pSrc[srcY1Offset + 0];
            unsigned char srcY11 = pSrc[srcY1Offset + 1];
            unsigned char srcCr = pSrc[srcCrOffset];
            unsigned char srcCb = pSrc[srcCbOffset];

            // Black in yuv is 0,128,128
            int diffCountY = 0;
            if (srcY00 != 0) {
                int destY00Offset = y * pitch + x;
                pDest[destY00Offset + 0] = srcY00;
                diffCountY++;
            }
            if (srcY01 != 0) {
                int destY01Offset = y * pitch + x + 1;
                pDest[destY01Offset] = srcY01;
                diffCountY++;
            }
            if (srcY10 != 0) {
                int destY10Offset = (y + 1) * pitch + x;
                pDest[destY10Offset + 0] = srcY10;
                diffCountY++;
            }
            if (srcY11 != 0) {
                int destY11Offset = (y + 1) * pitch + x + 1;
                pDest[destY11Offset] = srcY11;
                diffCountY++;
            }

            if (diffCountY > 2 || srcCb != 128 || srcCr != 128) {
                int destCbOffset = pitch * height + y / 2 * pitch + x;
                int destCrOffset = destCbOffset + 1;
                pDest[destCbOffset] = srcCb;
                pDest[destCrOffset] = srcCr;
            }
        }
    }
}
#pragma endregion

//------------------------- TvideoCodecLibavcodec overloaded methods -----------------------------
bool TvideoCodecLibavcodecDxva::beginDecompress(TffPictBase &pict, FOURCC fcc, const CMediaType &mt, int sourceFlags)
{
    dxvaCodecId = codecId;

    bool result = TvideoCodecLibavcodec::beginDecompress(pict, fcc, mt, sourceFlags);
    if (!result) {
        return false;
    }
    if (codecId == CODEC_ID_VC1) {
        bReorderBFrame = true;
    }

    isDXVACompatible = isDXVASupported();

    mb_height = avctx->height;
    mb_width = avctx->width;
    sar = avctx->sample_aspect_ratio;
    return true;
}

int TvideoCodecLibavcodecDxva::useDXVA(void)
{
    return (nDXVAMode == MODE_DXVA2) ? MODE_DXVA2 : MODE_DXVA1;
}

HRESULT TvideoCodecLibavcodecDxva::decompress(const unsigned char *src, size_t srcLen0, IMediaSample *pIn)
{
    HRESULT hr = S_OK;
    TffdshowVideoInputPin::TrateAndFlush *rateInfo = (TffdshowVideoInputPin::TrateAndFlush*)deciV->getRateInfo();

    if (pIn && pIn->IsDiscontinuity() == S_OK) {
        rateInfo->isDiscontinuity = true;
    }

    if (pIn) {
        pIn->GetTime(&rtStart, &rtStop);
    }

    b[inPosB].rtStart = rtStart;
    b[inPosB].rtStop = rtStop;
    b[inPosB].srcSize = (unsigned)srcLen0;
    inPosB++;
    if (inPosB >= countof(b)) {
        inPosB = 0;
    }

    avctx->reordered_opaque = rtStart;
    avctx->reordered_opaque2 = rtStop;
    avctx->reordered_opaque3 = srcLen0;

    if (avctx->sample_aspect_ratio.num
            && !(connectedSplitter == TffdshowVideoInputPin::MPC_matroska_splitter && avctx->sample_aspect_ratio.num == 1 && avctx->sample_aspect_ratio.den == 1)
       ) { // With MPC's internal matroska splitter, AR is not reliable.
        sar = avctx->sample_aspect_ratio;
    } else {
        sar = containerSar;
    }

    switch (nDXVAMode) {
        case MODE_SOFTWARE :
            return TvideoCodecLibavcodec::decompress(src, srcLen0, pIn);
        case MODE_DXVA1 :
        case MODE_DXVA2 :
            CheckPointer(pDXVADecoder, E_UNEXPECTED);
            updateAspectRatio();

            /* TODO : update picture size
            TffPict pict;
            pict.setSize(m_pCodec->mb_width,m_pCodec->mb_height);
            pict.setCSP(FF_CSP_NV12);
            pict.setSar(Rational(m_pCodec->sar.cx,m_pCodec->sar.cy));
            pict.rectFull.dx=m_pCodec->mb_width;
            pict.rectFull.dy=m_pCodec->mb_height;
            deciV->reconnectOutput(pict);

            // Change aspect ratio for DXVA1
            if ((nDXVAMode == MODE_DXVA1) &&
            ReconnectOutput(pictWidthRounded(), pictHeightRounded(), true, pictWidth(), pictHeight()) == S_OK)
            {
            pDXVADecoder->ConfigureDXVA1();
            }*/

            if (src) { //FIXME: this is a quick fix, when src == NULL we should deliver the frames that are still present in the simulated DPB
                hr = pDXVADecoder->DecodeFrame((BYTE*)src, (UINT)srcLen0, rtStart, rtStop);
            } else {
                hr = S_FALSE;
            }
            break;
        default :
            ASSERT(FALSE);
            hr = E_UNEXPECTED;
    }
    return hr;
}

const char_t* TvideoCodecLibavcodecDxva::getName(void) const
{
    if (avcodec) {
        static const char_t *libname = _l("libavcodec");
        switch (nDXVAMode) {
            case MODE_SOFTWARE :
                tsnprintf_s(codecName, countof(codecName), _TRUNCATE, _l("%s (no DXVA) %s"), libname, (const char_t*)text<char_t>(avcodec->name));
                break;
            case MODE_DXVA1 :
                tsnprintf_s(codecName, countof(codecName), _TRUNCATE, _l("DXVA1 %s"), (const char_t*)text<char_t>(avcodec->name));
                break;
            case MODE_DXVA2 :
                tsnprintf_s(codecName, countof(codecName), _TRUNCATE, _l("DXVA2 %s"), (const char_t*)text<char_t>(avcodec->name));
                break;
        }
        return codecName;
    } else {
        return _l("libavcodec");
    }
}

#pragma region Not used
// TODO (if possible) : find a way to retrieve the window handle of the player
UINT TvideoCodecLibavcodecDxva::getAdapter(IDirect3D9* pD3D, HWND hWnd)
{
    if (hWnd == NULL || pD3D == NULL) {
        return D3DADAPTER_DEFAULT;
    }

    HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    if (hMonitor == NULL) {
        return D3DADAPTER_DEFAULT;
    }

    for (UINT adp = 0, num_adp = pD3D->GetAdapterCount(); adp < num_adp; ++adp) {
        HMONITOR hAdpMon = pD3D->GetAdapterMonitor(adp);
        if (hAdpMon == hMonitor) {
            return adp;
        }
    }

    return D3DADAPTER_DEFAULT;
}

#pragma endregion
