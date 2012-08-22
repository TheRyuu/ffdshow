/*
 * Copyright (c) 2003-2006 Milan Cutka
 * based on CoreAAC - AAC DirectShow Decoder Filter  (C) 2003 Robert Cioch
 *      and ac3filter by Vigovsky Alexander
 *      and mpadecfilter by Gabest
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
#include "TffdshowDecAudio.h"
#include "ffdshow_mediaguids.h"
#include "TaudioCodec.h"
#include "TdialogSettings.h"
#include "TglobalSettings.h"
#include "Tpresets.h"
#include "TpresetSettingsAudio.h"
#include "ToutputAudioSettings.h"
#include "TaudioFilters.h"
#include "TtrayIcon.h"
#include "winamp2/Twinamp2.h"
#include "TffdshowDecAudioInputPin.h"
#include "dsutil.h"
#include "resource.h"
#include "Tinfo.h"
#include "ffcodecs.h"
#include <InitGuid.h>
#include <IffMmdevice.h> // Vista header import (MMDeviceAPI.h)

// This is the list of global filters (not part of the preset) to be displayed in the context menu
const TfilterIDFF TffdshowDecAudio::nextFilters[] = {
    NULL, 0
};

STDMETHODIMP_(int) TffdshowDecAudio::getVersion2(void)
{
    return VERSION;
}

CUnknown* WINAPI TffdshowDecAudio::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    TffdshowDecAudio *pNewObject = new TffdshowDecAudio(CLSID_FFDSHOWAUDIO, _l("TffAudioDecoder"), CLSID_TFFDSHOWAUDIOPAGE, IDS_FFDSHOWDECAUDIO, IDI_FFDSHOWAUDIO, punk, phr, IDFF_FILTERMODE_PLAYER | IDFF_FILTERMODE_AUDIO, defaultMerit, new TintStrColl);
    if (pNewObject == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return pNewObject;
}
CUnknown* WINAPI TffdshowDecAudioRaw::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    TffdshowDecAudioRaw *pNewObject = new TffdshowDecAudioRaw(punk, phr);
    if (pNewObject == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return pNewObject;
}

template<> interfaces<char_t>::IffdshowDecAudio* TffdshowDecAudio::getDecAudioInterface(void)
{
    return this;
}
template<> interfaces<tchar_traits<char_t>::other_char_t>::IffdshowDecAudio* TffdshowDecAudio::getDecAudioInterface(void)
{
    return &decAudio_char;
}

STDMETHODIMP TffdshowDecAudio::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    //char riidS[256];guid2str(riid,riidS,256);OutputDebugString(riidS);
    CheckPointer(ppv, E_POINTER);
    if (riid == IID_IffdshowDecAudioA) {
        return GetInterface<IffdshowDecAudioA>(getDecAudioInterface<IffdshowDecAudioA>(), ppv);
    } else if (riid == IID_IffdshowDecAudioW) {
        return GetInterface<IffdshowDecAudioW>(getDecAudioInterface<IffdshowDecAudioW>(), ppv);
    } else {
        return TffdshowDec::NonDelegatingQueryInterface(riid, ppv);
    }
}

TffdshowDecAudio::TffdshowDecAudio(CLSID Iclsid, const char_t *className, const CLSID &Iproppageid, int IcfgDlgCaptionId, int IiconId, LPUNKNOWN punk, HRESULT *phr, int Imode, int IdefaultMerit, TintStrColl *Ioptions):
    TffdshowDec(
        Ioptions,
        className, punk, Iclsid,
        globalSettings = Imode & IDFF_FILTERMODE_AUDIORAW ? new TglobalSettingsDecAudioRaw(&config, Imode, Ioptions) : new TglobalSettingsDecAudio(&config, Imode, Ioptions),
        dialogSettings = Imode & IDFF_FILTERMODE_AUDIORAW ? new TdialogSettingsDecAudioRaw(Ioptions) : new TdialogSettingsDecAudio(Ioptions),
        presets = Imode & IDFF_FILTERMODE_AUDIORAW ? new TpresetsAudioRaw : new TpresetsAudio, (Tpreset*&)presetSettings,
        this,
        (TinputPin*&)inpin,
        m_pOutput,
        m_pGraph,
        (Tfilters*&)audioFilters,
        Iproppageid, IcfgDlgCaptionId, IiconId,
        defaultMerit),
    decAudio_char(punk, this),
    isTmpgEnc(false),
    priorFrameMsgTime(0)
{
    setThreadName(DWORD(-1), "decAudio");

    setOnChange(IDFF_winamp2dir, this, &TffdshowDecAudio::onWinamp2dirChange);

    winamp2 = NULL;
    memset(&insf, 0, sizeof(insf));

    inpin = new TffdshowDecAudioInputPin(NAME("TffdshowDecAudioInputPin"), this, phr, L"In", 0);
    if (!inpin) {
        *phr = E_OUTOFMEMORY;
    }
    if (FAILED(*phr)) {
        return;
    }
    inpins.push_back(inpin);
    m_pInput = inpin;

    m_pOutput = new CTransformOutputPin(NAME("CTransformOutputPin"), this, phr, L"Out");
    if (!m_pOutput) {
        *phr = E_OUTOFMEMORY;
    }
    if (FAILED(*phr)) {
        delete m_pInput;
        m_pInput = NULL;
        return;
    }

    DPRINTF(_l("TffdshowDecAudio::Constructor"));
    trayIconStart = &TtrayIconBase::start<TtrayIconDecAudio>;
    audioFilters = NULL;
    isAudioSwitcher = !!globalSettings->isAudioSwitcher;
    alwaysextensible = !!globalSettings->alwaysextensible;
    allowOutStream = !!globalSettings->allowOutStream;
    m_rtStartDec = m_rtStartProc = REFTIME_INVALID;
    currentOutsf.reset();
    actual.cbBuffer = 0;

}

TffdshowDecAudioRaw::TffdshowDecAudioRaw(LPUNKNOWN punk, HRESULT *phr): TffdshowDecAudio(CLSID_FFDSHOWAUDIORAW, _l("TffdshowDecAudioRaw"), CLSID_TFFDSHOWAUDIORAWPAGE, IDS_FFDSHOWDECAUDIORAW, IDI_FFDSHOWAUDIO, punk, phr, IDFF_FILTERMODE_PLAYER | IDFF_FILTERMODE_AUDIORAW, defaultMerit, new TintStrColl)
{
}

TffdshowDecAudio::~TffdshowDecAudio()
{
    for (size_t i = 1; i < inpins.size(); i++) {
        delete inpins[i];
    }
    if (winamp2) {
        delete winamp2;
    }
}

int TffdshowDecAudio::GetPinCount(void)
{
    bool allconnect = isAudioSwitcher ? (inpins.size() == inpins.getNumConnectedInpins()) : false;
    return (int)inpins.size() + (allconnect ? 1 : 0) + 1;
}
CBasePin* TffdshowDecAudio::GetPin(int n)
{
    if (n == 0) {
        return m_pOutput;
    }
    n--;
    if (n < (int)inpins.size()) {
        return inpins[n];
    } else {
        wchar_t name[50];
        swprintf(name, L"In%i", n + 1);
        HRESULT phr = 0;
        TffdshowDecAudioInputPin *ipin = new TffdshowDecAudioInputPin(_l("CDECSSInputPin"), this, &phr, name, n + 1);
        if (FAILED(phr)) {
            return NULL;
        }
        inpins.push_back(ipin);
        return ipin;
    }
}

STDMETHODIMP TffdshowDecAudio::FindPin(LPCWSTR Id, IPin **ppPin)
{
    if (!ppPin) {
        return E_POINTER;
    }
    if (wcscmp(Id, m_pOutput->Name()) == 0) {
        *ppPin = m_pOutput;
    } else {
        *ppPin = inpins.find(Id);
    }

    if (*ppPin) {
        (*ppPin)->AddRef();
        return S_OK;
    } else {
        return VFW_E_NOT_FOUND;
    }
}

AVCodecID TffdshowDecAudio::getCodecId(const CMediaType &mt)
{
    DPRINTF(_l("TffdshowDecAudio::getCodecId"));
    //char typeS[256];DPRINTF("TffdshowDecAudio::getCodecId Type:%s",guid2str(type,typeS,256));
    if (mt.majortype != MEDIATYPE_Audio && mt.majortype != MEDIATYPE_MPEG2_PES && mt.majortype != MEDIATYPE_DVD_ENCRYPTED_PACK) {
        return AV_CODEC_ID_NONE;
    }
    //char subtypeS[256],formattypeS[256];DPRINTF("TffdshowDecAudio::getCodecId Subtype:%s, FormatType:%s",guid2str(subtype,subtypeS,256),guid2str(formattype,formattypeS,256));
    if (mt.formattype != FORMAT_WaveFormatEx &&
            mt.formattype != FORMAT_VorbisFormat &&
            mt.formattype != FORMAT_VorbisFormat2 &&
            mt.formattype != FORMAT_VorbisFormatIll) {
        return AV_CODEC_ID_NONE;
    }

    DWORD wFormatTag = 0;
    if (mt.subtype == MEDIASUBTYPE_Vorbis || mt.subtype == MEDIASUBTYPE_Vorbis2 || mt.subtype == MEDIASUBTYPE_VorbisIll) {
        wFormatTag = WAVE_FORMAT_VORBIS;
    } else if (mt.subtype == MEDIASUBTYPE_DOLBY_AC3) {
        wFormatTag = WAVE_FORMAT_AC3_W;
    } else if (mt.subtype == MEDIASUBTYPE_DTS) {
        wFormatTag = WAVE_FORMAT_DTS_W;
    } else if (mt.subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO || mt.subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO) {
        wFormatTag = WAVE_FORMAT_LPCM;
    } else if (mt.subtype == MEDIASUBTYPE_AAC3) {
        wFormatTag = WAVE_FORMAT_AAC3;
    } else if (mt.subtype == MEDIASUBTYPE_AAC5 || mt.subtype == KSDATAFORMAT_SUBTYPE_IEC61937_AAC) {
        wFormatTag = WAVE_FORMAT_AAC5;
    } else if (mt.subtype == MEDIASUBTYPE_MPEG2_AUDIO) {
        wFormatTag = WAVE_FORMAT_MPEG;
    } else if (mt.subtype == MEDIASUBTYPE_SAMR) {
        wFormatTag = WAVE_FORMAT_SAMR;
    } else if (mt.subtype == MEDIASUBTYPE_IMA_AMV) {
        wFormatTag = WAVE_FORMAT_IMA_AMV;
    } else if (mt.subtype == MEDIASUBTYPE_ADPCM_SWF) {
        wFormatTag = WAVE_FORMAT_ADPCM_SWF;
    } else if (mt.subtype == MEDIASUBTYPE_NELLYMOSER) {
        wFormatTag = WAVE_FORMAT_NELLYMOSER;
    } else if (mt.subtype == MEDIASUBTYPE_COOK || mt.subtype == MEDIASUBTYPE_cook || mt.subtype == MEDIASUBTYPE_COOK2) {
        wFormatTag = WAVE_FORMAT_COOK;
    } else if (mt.subtype == MEDIASUBTYPE_DOLBY_TRUEHD || mt.subtype == MEDIASUBTYPE_NERO_MLP
               || mt.subtype == MEDIASUBTYPE_ARCSOFT_MLP || mt.subtype == MEDIASUBTYPE_SONIC_MLP
               || mt.subtype == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP) {
        wFormatTag = WAVE_FORMAT_TRUEHD;
    } else if (mt.subtype == MEDIASUBTYPE_DOLBY_DDPLUS || mt.subtype == MEDIASUBTYPE_ARCSOFT_DDPLUS ||
               mt.subtype == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS) {
        wFormatTag = WAVE_FORMAT_EAC3;
    } else if (mt.subtype == MEDIASUBTYPE_DTS_HD || mt.subtype == MEDIASUBTYPE_ARCSOFT_DTSHD ||
               mt.subtype == KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD) {
        wFormatTag = WAVE_FORMAT_DTS_HD;  // TODO : define a separate codecId for DTSHD when available
    } else {
        const WAVEFORMATEX *wfex = (const WAVEFORMATEX*)mt.pbFormat;
        if ((mt.subtype == MEDIASUBTYPE_MPEG1AudioPayload || wfex->wFormatTag == WAVE_FORMAT_MPEG) && mt.cbFormat) {
            MPEG1WAVEFORMAT *m1wf = (MPEG1WAVEFORMAT*)mt.pbFormat;
            switch (m1wf->fwHeadLayer) {
                case ACM_MPEG_LAYER1:
                    wFormatTag = WAVE_FORMAT_MPEG;
                    break;
                case ACM_MPEG_LAYER2:
                    wFormatTag = WAVE_FORMAT_MPEG;
                    break;
                case ACM_MPEG_LAYER3:
                    wFormatTag = WAVE_FORMAT_MPEGLAYER3;
                    break;
            }
        } else if ((mt.subtype == MEDIASUBTYPE_QDM2 || mt.subtype == MEDIASUBTYPE_qdm2) && wfex->wFormatTag == 0 && wfex->cbSize > 8 && (*((FOURCC*)((uint8_t*)(wfex + 1) + 4)) == WAVE_FORMAT_QDM2 || *((FOURCC*)((uint8_t*)(wfex + 1) + 4)) == WAVE_FORMAT_qdm2)) {
            wFormatTag = WAVE_FORMAT_QDM2;
        } else if (mt.subtype == MEDIASUBTYPE_IMA4 && wfex->wFormatTag == 0 && wfex->cbSize > 8 && *((FOURCC*)((uint8_t*)(wfex + 1) + 4)) == mmioFOURCC('i', 'm', 'a', '4')) {
            wFormatTag = WAVE_FORMAT_IMA4;
        } else if (mt.subtype == MEDIASUBTYPE_MAC3) {
            wFormatTag = WAVE_FORMAT_MAC3;
        } else if (mt.subtype == MEDIASUBTYPE_MAC6) {
            wFormatTag = WAVE_FORMAT_MAC6;
        } else if (mt.subtype == MEDIASUBTYPE_14_4) {
            wFormatTag = WAVE_FORMAT_14_4;
        } else if (mt.subtype == MEDIASUBTYPE_28_8) {
            wFormatTag = WAVE_FORMAT_28_8;
        } else if (mt.subtype == MEDIASUBTYPE_IMC) {
            wFormatTag = WAVE_FORMAT_IMC;
        } else if (mt.subtype == MEDIASUBTYPE_ATRAC3 || mt.subtype == MEDIASUBTYPE_ATRC) {
            wFormatTag = WAVE_FORMAT_ATRAC3;
        } else if (mt.subtype == MEDIASUBTYPE_WAVPACK) {
            wFormatTag = WAVE_FORMAT_WAVPACK;
        } else if (mt.subtype == MEDIASUBTYPE_ULAW || mt.subtype == MEDIASUBTYPE_ulaw) {
            wFormatTag = WAVE_FORMAT_MULAW;
        } else if (mt.subtype == MEDIASUBTYPE_ALAW || mt.subtype == MEDIASUBTYPE_alaw) {
            wFormatTag = WAVE_FORMAT_ALAW;
        } else if (mt.subtype == MEDIASUBTYPE_SOWT || mt.subtype == MEDIASUBTYPE_sowt || mt.subtype == MEDIASUBTYPE_TWOS || mt.subtype == MEDIASUBTYPE_twos) {
            wFormatTag = WAVE_FORMAT_TWOS;
        } else if (mt.subtype == MEDIASUBTYPE_IN32 || mt.subtype == MEDIASUBTYPE_in32) {
            wFormatTag = WAVE_FORMAT_IN32;
        } else if (mt.subtype == MEDIASUBTYPE_IN24 || mt.subtype == MEDIASUBTYPE_in24) {
            wFormatTag = WAVE_FORMAT_IN24;
        } else if (mt.subtype == MEDIASUBTYPE_FL32 || mt.subtype == MEDIASUBTYPE_fl32) {
            wFormatTag = WAVE_FORMAT_FL32;
        } else if (mt.subtype == MEDIASUBTYPE_FL64 || mt.subtype == MEDIASUBTYPE_fl64) {
            wFormatTag = WAVE_FORMAT_FL64;
        }
        if (wFormatTag == 0) {
            wFormatTag = wfex->wFormatTag;
        }
        DPRINTF(_l("TffdshowDecAudio::getCodecId: %i Hz, %i channels"), (int)wfex->nSamplesPerSec, (int)wfex->nChannels);
        wFormatTag = TsampleFormat::getPCMformat(mt, wFormatTag);
    }
    AVCodecID codecId = globalSettings->getCodecId(wFormatTag, NULL);
    // Codec Id returned is CODEC_ID_MP3 when input format is MP1,MP2 and libavcodec is selected (because same config table mp123[])
    if (wFormatTag == WAVE_FORMAT_MPEG && codecId == AV_CODEC_ID_MP3) {
        MPEG1WAVEFORMAT *m1wf = (MPEG1WAVEFORMAT*)mt.pbFormat;
        switch (m1wf->fwHeadLayer) {
            case ACM_MPEG_LAYER1:
            case ACM_MPEG_LAYER2:
                codecId = AV_CODEC_ID_MP2;
                break;
        }
    }
    // use SPDIF/bistream pass-through where passthrough is checked for the corresponding format
    // but don't publish the bistream/SPDIF format if passthroughPCMConnection is enabled
    // (some cards don't accept the official SPDIF/bitstream media types)
    if (presetSettings && !presetSettings->output->passthroughPCMConnection && presetSettings->output) {
        DPRINTF(_l("TffdshowDecAudio::getCodecId Check if it is a SPDIF/bistream format"));
        switch (codecId) {
            case CODEC_ID_LIBA52:
            case AV_CODEC_ID_AC3:
                if (presetSettings->output->passthroughAC3) {
                    codecId = CODEC_ID_SPDIF_AC3;
                }
                break;
            case CODEC_ID_LIBDTS:
            case AV_CODEC_ID_DTS:
                if (presetSettings->output->passthroughDTS) {
                    codecId = CODEC_ID_SPDIF_DTS;
                }
                break;
            case AV_CODEC_ID_TRUEHD:
                if (presetSettings->output->passthroughTRUEHD) {
                    codecId = CODEC_ID_BITSTREAM_TRUEHD;
                }
                break;
            case AV_CODEC_ID_EAC3:
                if (presetSettings->output->passthroughEAC3) {
                    codecId = CODEC_ID_BITSTREAM_EAC3;
                }
                break;
                /*case CODEC_ID_DTS_HD: //TODO : no DTS-HD software decoder yet
                 if (presetSettings->output->passthroughDTSHD)  codecId = CODEC_ID_BITSTREAM_DTSHD;
                 break;*/
            default:
                break;
        }
        // Update codecID to bitstream format (but only if not already set as bitstream in the audio decoder)
        if ((spdif_codec(codecId) || bitstream_codec(codecId)) && inpin && inpin->audio && !bitstream_codec(inpin->audio->codecId)) {
            inpin->audio->codecId = codecId;
        }
    }

    DPRINTF(_l("TffdshowDecAudio::getCodecId: codecId=%s (%i)"), getCodecName(codecId), codecId);
    return codecId;
}

HRESULT TffdshowDecAudio::CheckInputType(const CMediaType *mtIn)
{
    DPRINTF(_l("TffdshowDecAudio::CheckInputType"));
    return getCodecId(*mtIn) == AV_CODEC_ID_NONE ? VFW_E_TYPE_NOT_ACCEPTED : S_OK;
}

HRESULT TffdshowDecAudio::CheckConnect(PIN_DIRECTION dir, IPin *pPin)
{
    DPRINTF(_l("TffdshowDecAudio::CheckConnect (%s)"), dir == PINDIR_INPUT ? _l("input") : _l("output"));
    HRESULT res = E_UNEXPECTED;
    switch (dir) {
        case PINDIR_INPUT:
            res = checkInputConnect(pPin);
            break;
        case PINDIR_OUTPUT:
            initPreset();
            TsampleFormat outsf = getOutsf();
            if (presetSettings->output->connectTo != 0) {
                if (presetSettings->output->connectToOnlySpdif) {
                    if (outsf.sf != TsampleFormat::SF_AC3) {
                        res = S_OK;
                        break;
                    }
                }
                CLSID clsid = GetCLSID(pPin);
                res = ((presetSettings->output->connectTo == 1 && clsid == CLSID_DSoundRender) || (presetSettings->output->connectTo == 2 && clsid == CLSID_AudioRender)) ? S_OK : E_FAIL;
            } else {
                res = S_OK;
            }
            break;
    };
    return res == S_OK ? CTransformFilter::CheckConnect(dir, pPin) : res;
}

TsampleFormat TffdshowDecAudio::getOutsf(void)
{
    TsampleFormat outsf;
    if (inpin->getsf(outsf)) { // SPDIF/HDMI
        return outsf;
    } else { // PCM
        return getOutsf(outsf);
    }
}

TsampleFormat TffdshowDecAudio::getOutsf(TsampleFormat &outsf)
{
    DPRINTF(_l("TffdshowDecAudio::getOutsf PCM %lx"), outsf.sf);
    if (!audioFilters) {
        audioFilters = new TaudioFiltersPlayer(this, this, presetSettings);
    }
    audioFilters->getOutputFmt(outsf, presetSettings);
    return outsf;
}


HRESULT TffdshowDecAudio::getMediaType(CMediaType *mtOut)
{
    if (inpin->is_spdif_codec()) {
        *mtOut = TsampleFormat::createMediaTypeSPDIF(inpin->audio->getInputSF().freq);
    } else {
        if (!presetSettings) {
            initPreset();
        }
        TsampleFormat outsf = getOutsf();
        //DPRINTF(_l("TffdshowDecAudio::getMediaType sample format %d"),outsf.sf);
        if (outsf.sf == TsampleFormat::SF_AC3) {
            *mtOut = TsampleFormat::createMediaTypeSPDIF(outsf.freq);  // BUG : if 96khz set, and SPDIF is set in output section then inpin->audio->getInputSF().freq should be set instead
        } else if (getParam2(IDFF_aoutUseIEC61937)) {
            *mtOut = outsf.toCMediaTypeHD(true);
        } else {
            *mtOut = outsf.toCMediaType(alwaysextensible);
        }
        char_t descS[256];
        outsf.descriptionPCM(descS, 256);
        //DPRINTF(_l("TffdshowDecAudio::getMediaType:%s"),descS);
    }

    return S_OK;
}

HRESULT TffdshowDecAudio::GetMediaType(int iPosition, CMediaType *mtOut)
{
    if (!inpin->IsConnected()) {
        return E_UNEXPECTED;
    }
    if (iPosition < 0) {
        return E_INVALIDARG;
    }

    // Work around Windows Media Encoder 9 crashes. I believe it's a bug in WME.
    if (iPosition > 0 && allowOutStream && config.is_WMEncEng_loaded()) {
        return VFW_S_NO_MORE_ITEMS;
    }

    if (iPosition > (allowOutStream ? 1 : 0)) {
        return VFW_S_NO_MORE_ITEMS;
    }

    switch (iPosition) {
        case 0:
            getMediaType(mtOut);
            break;
        case 1:
            mtOut->SetType(&MEDIATYPE_Stream);
            mtOut->SetSubtype(&MEDIASUBTYPE_None);
            mtOut->SetFormatType(&FORMAT_None);
            mtOut->SetSampleSize(48000 * 8 * 4 / 5);
            mtOut->SetVariableSize();
            mtOut->SetTemporalCompression(FALSE);
            break;
    }
    return S_OK;
}

HRESULT TffdshowDecAudio::DecideBufferSize(IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pProperties)
{
    DPRINTF(_l("TffdshowDecAudio::DecideBufferSize"));

    if (!presetSettings) {
        initPreset();
    }

    pProperties->cBuffers = 4;
    pProperties->cbBuffer = 48000 * 8 * 4 / 5;
    pProperties->cbAlign = 1;
    pProperties->cbPrefix = 0;

    DPRINTF(_l("TffAudioDecoder::DecideBufferSize %d"), pProperties->cbBuffer);

    HRESULT hr;
    if (FAILED(hr = pAllocator->SetProperties(pProperties, &actual))) {
        return hr;
    }

    return pProperties->cBuffers > actual.cBuffers || pProperties->cbBuffer > actual.cbBuffer ? E_FAIL : S_OK;
}

HRESULT TffdshowDecAudio::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
    DPRINTF(_l("TffdshowDecAudio::CheckTransform From :"));
    TsampleFormat::DPRINTMediaTypeInfo(*mtIn);
    DPRINTF(_l("TffdshowDecAudio::CheckTransform To :"));
    TsampleFormat::DPRINTMediaTypeInfo(*mtOut);

    HRESULT hr = S_OK;
#if 0
    hr = SUCCEEDED(CheckInputType(&inpin->CurrentMediaType())) && (mtOut->majortype == MEDIATYPE_Audio && (mtOut->subtype == MEDIASUBTYPE_PCM || mtOut->subtype == MEDIASUBTYPE_IEEE_FLOAT)) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
    DPRINTF(_l("TffdshowDecAudio::CheckTransform result %lx"), hr);
    return hr;
#else //more strict check for output sample type
    if (!SUCCEEDED(CheckInputType(&inpin->CurrentMediaType()))) {
        return VFW_E_TYPE_NOT_ACCEPTED;
    }
    if (allowOutStream && *mtOut->Type() == MEDIATYPE_Stream) {
        fileout = true;
        return S_OK;
    } else {
        fileout = false;
        if (!m_pInput->IsConnected() && m_pOutput->IsConnected()) {
            return S_OK;
        } else {
            AVCodecID codecId = AV_CODEC_ID_NONE;
            if (inpin != NULL) {
                this->inpin->getCodecId(&codecId);
            }
            if (spdif_codec(codecId) || bitstream_codec(codecId)) {
                DPRINTF(_l("TffdshowDecAudio::CheckTransform on bitstream format, accept the transformation"));
                return S_OK;
            }


            CMediaType ffOut;
            if (getMediaType(&ffOut) != S_OK) {
                DPRINTF(_l("TffdshowDecAudio::CheckTransform result VFW_E_TYPE_NOT_ACCEPTED"));
                return VFW_E_TYPE_NOT_ACCEPTED;
            }
            DPRINTF(_l("TffdshowDecAudio::CheckTransform To generated by FFDShow :"));
            TsampleFormat::DPRINTMediaTypeInfo(ffOut);
            if (ffOut != *mtOut) {
                DPRINTF(_l("TffdshowDecAudio::CheckTransform target format is different from FFDShow output format, refuse the transformation"));
                return VFW_E_TYPE_NOT_ACCEPTED;
            }
            DPRINTF(_l("TffdshowDecAudio::CheckTransform result %lx"), hr);
            return hr;
        }
    }
#endif
}

HRESULT TffdshowDecAudio::CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin)
{
    if (direction == PINDIR_OUTPUT) {
        const CLSID &out = GetCLSID(m_pOutput->GetConnected());
        static const GUID CLSID_TMPGencGetSample = {0x10AA1647, 0xCECE, 0x40D4, 0x8C, 0x35, 0xD2, 0x5D, 0xDA, 0x77, 0x5B, 0xEF};
        isTmpgEnc = !!(out == CLSID_TMPGencGetSample);
    }
    return CTransformFilter::CompleteConnect(direction, pReceivePin);
}

HRESULT TffdshowDecAudio::StartStreaming(void)
{
    DPRINTF(_l("TffdshowDecAudio::StartStreaming"));
    firsttransform = true;
    ft1 = ft2 = 0;
    return CTransformFilter::StartStreaming();
}
HRESULT TffdshowDecAudio::BeginFlush()
{
    DPRINTF(_l("TffdshowDecAudio::BeginFlush"));
    return CTransformFilter::BeginFlush();
}
HRESULT TffdshowDecAudio::EndFlush(void)
{
    DPRINTF(_l("TffdshowDecAudio::EndFlush"));
    CAutoLock cAutoLock(&m_csReceive);
    return CTransformFilter::EndFlush();
}

HRESULT TffdshowDecAudio::EndOfStream(void)
{
    DPRINTF(_l("TffdshowDecAudio::EndOfStream"));
    CAutoLock cAutoLock(&m_csReceive);
    return CTransformFilter::EndOfStream();
}

HRESULT TffdshowDecAudio::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    DPRINTF(_l("TffdshowDecAudio::NewSegment"));
    CAutoLock cAutoLock(&m_csReceive);
    m_rtStartDec = m_rtStartProc = REFTIME_INVALID;
    return TffdshowDec::NewSegment(tStart, tStop, dRate);
}

HRESULT TffdshowDecAudio::Receive(IMediaSample* pIn)
{
    if (firsttransform) {
        firsttransform = false;
        onTrayIconChange(0, 0);
    }
    if (m_dirtyStop) {
        m_dirtyStop = false;
        inpin->onSeek(0);
    }

    return S_OK;
}

HRESULT TffdshowDecAudio::onGraphRemove(void)
{
    //if (inpin->audio) delete inpin->audio;inpin->audio=NULL;
    if (audioFilters) {
        delete audioFilters;
    }
    audioFilters = NULL;
    currentOutsf.reset();
    return TffdshowDec::onGraphRemove();
}

STDMETHODIMP TffdshowDecAudio::deliverDecodedSample(const TffdshowDecAudioInputPin *pin, void *buf, size_t numsamples, const TsampleFormat &fmt)
{
    REFERENCE_TIME rtDurDec = REF_SECOND_MULT * numsamples / fmt.freq;
    m_rtStartDec += rtDurDec;
    if (!audioFilters) {
        audioFilters = new TaudioFiltersPlayer(this, this, presetSettings);
    }
    HRESULT res = audioFilters->process(fmt, buf, numsamples, presetSettings);
    return res;
}

HRESULT TffdshowDecAudio::flushDecodedSamples(const TffdshowDecAudioInputPin *pin)
{
    if (!audioFilters) {
        audioFilters = new TaudioFiltersPlayer(this, this, presetSettings);
    }
    return audioFilters->process(TsampleFormat(), NULL, 0, presetSettings);
}

STDMETHODIMP TffdshowDecAudio::deliverProcessedSample(const void *buf, size_t numsamples, const TsampleFormat &outsf0)
{
    if (m_pClock) {
        REFERENCE_TIME currentTime, diff;
        m_pClock->GetTime(&currentTime);
        diff = currentTime - priorFrameMsgTime;
        if (diff > 500000 || diff < 0) { // 50ms
            priorFrameMsgTime = currentTime;
            sendOnFrameMsg();
        }
    } else {
        sendOnFrameMsg();
    }
    if (numsamples == 0) {
        return S_OK;
    }
    currentOutsf = outsf0;

    CMediaType mt = currentOutsf.toCMediaType(alwaysextensible);
    WAVEFORMATEX *wfe = (WAVEFORMATEX*)mt.Format();
    if (inpin->is_spdif_codec()) {
        wfe->wFormatTag = WAVE_FORMAT_DOLBY_AC3_SPDIF;
    }

    HRESULT hr = S_OK;
    if (!fileout && FAILED(hr = ReconnectOutput(numsamples, mt))) {
        return hr;
    }

    comptr<IMediaSample> pOut;
    BYTE *dst = NULL;
    if (FAILED(getDeliveryBuffer(&pOut, &dst))) {
        return E_FAIL;
    }

    REFERENCE_TIME rtDurProc = REF_SECOND_MULT * numsamples / currentOutsf.freq;

    REFERENCE_TIME offset = 0;
    if (presetSettings && presetSettings->audioDelay != 0) {
        REFERENCE_TIME delay100ns = presetSettings->audioDelay * 10000LL;
        offset = delay100ns;
    }


    REFERENCE_TIME rtStart = m_rtStartProc + offset;
    REFERENCE_TIME rtStop = m_rtStartProc + offset + rtDurProc;
    m_rtStartProc += rtDurProc;
    if (rtStart < 0) {
        return S_OK;
    }

    if (!fileout && hr == S_OK) {
        m_pOutput->SetMediaType(&mt);
        pOut->SetMediaType(&mt);
    }
    pOut->SetTime(&rtStart, &rtStop);
    if (!isTmpgEnc) {
        pOut->SetMediaTime(NULL, NULL);   // this should cause subsequent calls to GetMediaType to return error, but actually they return S_OK and zero start and stop times
    }
    pOut->SetPreroll(FALSE);
    pOut->SetDiscontinuity(discontinuity);
    discontinuity = false;
    pOut->SetSyncPoint(TRUE);

    size_t dstlen = numsamples * currentOutsf.nchannels * currentOutsf.bitsPerSample() / 8;
    pOut->SetActualDataLength((long)dstlen);
    memcpy(dst, buf, dstlen);
    if (fileout) {
        ft2 += dstlen;
        pOut->SetTime(&ft1, &ft2);
        ft1 += dstlen;
    }
    return m_pOutput->Deliver(pOut);
}

STDMETHODIMP TffdshowDecAudio::deliverSampleBistream(void *buf, size_t size, int bit_rate, unsigned int sample_rate, int incRtDec, int frame_length, int iec_length)
{
    HRESULT hr = S_OK;
    CMediaType mt;
    TaudioParser *pAudioParser = NULL;
    TaudioParserData audioParserData;

    AVCodecID codecId = AV_CODEC_ID_NONE;
    if (inpin != NULL) {
        inpin->getAudioParser(&pAudioParser);
        this->inpin->getCodecId(&codecId);
        if (pAudioParser != NULL) {
            audioParserData = pAudioParser->getParserData();
        }
    }

    if (bitstream_codec(codecId)) {
        getMediaType(&mt);
        if (codecId == CODEC_ID_BITSTREAM_TRUEHD && buf == NULL) { // No data, just update the timestamps
            REFERENCE_TIME rtDur = inpin->insample_rtStop - inpin->insample_rtStart;
            m_rtStartProc += rtDur;
            if (incRtDec) {
                m_rtStartDec += rtDur;
            }
            return S_OK;
        }
    } else { // SPDIF
        currentOutsf.sf = TsampleFormat::SF_AC3;
        mt = TsampleFormat::createMediaTypeSPDIF(sample_rate);
    }

    WAVEFORMATEX *wfe = (WAVEFORMATEX*)mt.Format();

    size_t length = 0;
    size_t repetition_burst = 0x800; // 2048 = AC3
    if (!fileout) { // If we are delivering the samples to an audio device, encapsulate the stream into IEC structure (otherwise stream it with no modification)
        if (codecId == CODEC_ID_BITSTREAM_TRUEHD)
            // Repetition rate of TrueHD/MLP buffers is 61440
        {
            repetition_burst = 61440;  // max length of MAT data: 61424 bytes (total=61432+8 header bytes)
        } else if (codecId == CODEC_ID_BITSTREAM_EAC3) { // 6144 for DD Plus * 4 for IEC 60958 frames
            repetition_burst = 24576;
        } else if (codecId == CODEC_ID_BITSTREAM_DTSHD) {
            repetition_burst = 32768;
        } else {
            // AC3/DTS
            repetition_burst = 0x800; // 2048 = AC3 and DTS

            unsigned int size2;
            length = 0;
            // Add 4 more words (8 bytes) for AC3/DTS (for backward compatibility, should be *4 for other codecs)
            // AC3/DTS streams start with 8 blank bytes (why, don't know but let's going on with)
            while (length < odd2even(size) + sizeof(WORD) * 8) {
                length += repetition_burst;
            }

            // bit_rate is not always correct. This is a bug of some where else. Because I can't fix it now, work around for it...
            if (bit_rate <= 1 && inpin->insample_rtStart != REFTIME_INVALID && inpin->insample_rtStop != REFTIME_INVALID) {
                size2 = (unsigned int)(wfe->nBlockAlign * wfe->nSamplesPerSec * (inpin->insample_rtStop - inpin->insample_rtStart) / REF_SECOND_MULT);
            } else {
                size2 = (unsigned int)(int64_t(1) * wfe->nBlockAlign * wfe->nSamplesPerSec * size * 8 / bit_rate);
            }
            while (length < size2) {
                length += repetition_burst;
            }
        }
    } else {
        length = size;
    }

    if (length == 0) {
        length = repetition_burst;
    }


    if (!fileout && FAILED(hr = ReconnectOutput(length / wfe->nBlockAlign, mt))) {
        return hr;
    }

    comptr<IMediaSample> pOut;
    BYTE *pDataOut = NULL;
    if (FAILED(getDeliveryBuffer(&pOut, &pDataOut))) {
        return E_FAIL;
    }

    REFERENCE_TIME rtDur, rtStart = m_rtStartProc, rtStop = REFTIME_INVALID;
    if (bitstream_codec(codecId)) {
        if (inpin->insample_rtStart != REFTIME_INVALID && inpin->insample_rtStop != REFTIME_INVALID) {
            rtStart = inpin->insample_rtStart;
            rtDur = inpin->insample_rtStop - inpin->insample_rtStart;
            if (rtDur < 0) {
                rtDur = 1;
            }
        } else {
            rtDur = REF_SECOND_MULT * size / wfe->nBlockAlign / wfe->nSamplesPerSec;
        }
        rtStop = rtStart + rtDur;
    } else if (bit_rate <= 1 && inpin->insample_rtStart != REFTIME_INVALID && inpin->insample_rtStop != REFTIME_INVALID) {
        rtDur = inpin->insample_rtStop - inpin->insample_rtStart;
    } else {
        if (frame_length) {
            size_t blocks = (size + frame_length - 1) / frame_length;
            rtDur = REF_SECOND_MULT * blocks * frame_length * 8 / bit_rate;
        } else {
            rtDur = REF_SECOND_MULT * size * 8 / bit_rate;
        }
    }

    REFERENCE_TIME offset = 0;

    if (presetSettings && presetSettings->audioDelay != 0) {
        REFERENCE_TIME delay100ns = presetSettings->audioDelay * 10000LL;
        offset = delay100ns;
    }

    rtStart += offset;
    rtStop += offset;

    if (rtStop == REFTIME_INVALID) {
        rtStop = m_rtStartProc + offset + rtDur;
    }

    //DPRINTF(_l("pin:%I64i startDec:%I64i duration:%I64i"),rtStart,m_rtStartDec,rtDur);
    m_rtStartProc += rtDur;
    if (incRtDec) {
        m_rtStartDec += rtDur;
    }

    if (rtStart < 0) {
        return S_OK;
    }

    if (!fileout && hr == S_OK) {
        m_pOutput->SetMediaType(&mt);
        pOut->SetMediaType(&mt);
    }

    if (bit_rate <= 1 && inpin->insample_rtStart != REFTIME_INVALID && inpin->insample_rtStop != REFTIME_INVALID) {
        rtStart = inpin->insample_rtStart;
        rtStop = inpin->insample_rtStop;
    }
    pOut->SetTime(&rtStart, &rtStop);
    pOut->SetMediaTime(NULL, NULL);

    pOut->SetPreroll(FALSE);
    pOut->SetDiscontinuity(discontinuity);
    discontinuity = false;
    pOut->SetSyncPoint(TRUE);

    pOut->SetActualDataLength((long)length);

    if (!fileout) {
        // IEC 61936 structure writing (HDMI bitstream, SPDIF)
        DWORD type = 0x0001;
        short subDataType = 0; // TODO : 0 for all these formats (but different for AAC and WMA Pro)
        short errorFlag = 0;
        short datatypeInfo = 0;
        short bitstreamNumber = 0;
        switch (codecId) {
            case CODEC_ID_SPDIF_AC3:
                type = 1;
                break;
            case CODEC_ID_BITSTREAM_TRUEHD:
                type = 22;
                break;
            case CODEC_ID_BITSTREAM_EAC3:
                type = 21;
                break;
            case CODEC_ID_BITSTREAM_DTSHD:
                type = 17;
                datatypeInfo = 4;
                break; // = DTS Type IV : DTS-HD
            case CODEC_ID_SPDIF_DTS:
                if (pAudioParser == NULL || audioParserData.sample_blocks * 8 * 32 == 512) {
                    type = 0x0b;
                } else if (audioParserData.sample_blocks * 8 * 32 == 1024) {
                    type = 0x0c;
                } else {
                    type = 0x0d;
                }
                break;
            default:
                type = 1;
                break; // AC3 encode mode
        }

        DWORD Pc = type | (subDataType << 5) | (errorFlag << 7) | (datatypeInfo << 8) | (bitstreamNumber << 13);

        if (rtStart / REF_SECOND_MULT < 2) {
            DPRINTF(_l("TffdshowDecAudio::deliverSampleBistream Delivering IEC sample format type %ld - sample size %ld - buffer length %ld"), Pc, size, length);
            TsampleFormat::DPRINTMediaTypeInfo(mt);
        }

        WORD *pDataOutW = (WORD*)pDataOut; // Header is filled with words instead of bytes

        // Preamble : 16 bytes for AC3/DTS, 8 bytes for other formats
        int index = 0;
        pDataOutW[0] = pDataOutW[1] = pDataOutW[2] = pDataOutW[3] = 0; // Stuffing at the beginning, not sure if this is useful
        if (codecId == CODEC_ID_SPDIF_AC3 || codecId == CODEC_ID_SPDIF_DTS) {
            index = 4;  // First additional four words filled with 0 only for backward compatibility for AC3/DTS
        }

        // Fill after the input buffer with zeros if any extra bytes
        if (length > 8 + index * 2 + size) {
            memset(pDataOut + 8 + index * 2 + size, 0, length - 8 - index * 2 - size); // Fill the output buffer with zeros
        }

        // Fill the 8 bytes (4 words) of IEC header
        pDataOutW[index++] = 0xf872;
        pDataOutW[index++] = 0x4e1f;
        pDataOutW[index++] = (WORD)Pc;
        if (iec_length != 0) {
            pDataOutW[index++] = WORD(iec_length);
        } else if (bitstream_codec(codecId)) {
            pDataOutW[index++] = WORD(size);  // size in bytes for the others
        } else {
            pDataOutW[index++] = WORD(size * 8); // size in bits for AC3/DTS
        }

        // Apply different size for some formats
        /*switch (codecId)
        {
         case CODEC_ID_BITSTREAM_TRUEHD: pDataOutW[index-1]=WORD(61424);break;//61424 : repetition of MLP frames
         case CODEC_ID_BITSTREAM_EAC3:pDataOutW[index-1]=WORD(24576);break;//24 576 : why ? this is the full size including IEC header
         case CODEC_ID_BITSTREAM_DTSHD:pDataOutW[index-1]=WORD(((size-12) & ~0xf) + 0x18);break; // DTS-HD : (size without 12 extra bytes) & 0xF + 0x18
        }*/

        // Data : swap bytes from first byte of data on size length (input buffer lentgh)
        _swab((char*)buf, (char*)&pDataOutW[index], (int)(size & ~1));
        if (size & 1) { // _swab doesn't like odd number.
            pDataOut[index * 2 + size] = ((BYTE*)buf)[size - 1];
            pDataOut[index * 2 - 1 + size] = 0;
        }
    } else {
        memcpy(pDataOut, buf, length);
        ft2 += length;
        pOut->SetTime(&ft1, &ft2);
        ft1 += length;
    }

    HRESULT res = m_pOutput->Deliver(pOut);
    pOut = NULL;
    return res;
}

STDMETHODIMP TffdshowDecAudio::getMovieSource(const TaudioCodec* *moviePtr)
{
    return inpin->getMovieSource(moviePtr);
}

STDMETHODIMP TffdshowDecAudio::inputSampleFormatDescription(char_t *buf, size_t buflen)
{
    if (!buf || buflen == 0) {
        return E_POINTER;
    }
    if (!insf) {
        return E_UNEXPECTED;
    }
    insf.description(buf, buflen);
    return S_OK;
}
STDMETHODIMP TffdshowDecAudio::inputSampleFormat(unsigned int *nchannels, unsigned int *freq)
{
    if (!nchannels && !freq) {
        return E_POINTER;
    }
    if (!insf) {
        return E_UNEXPECTED;
    }
    if (nchannels) {
        *nchannels = insf.nchannels;
    }
    if (freq) {
        *freq = insf.freq;
    }
    return S_OK;
}
STDMETHODIMP TffdshowDecAudio::currentSampleFormat(unsigned int *nchannels, unsigned int *freq, int *sampleFormat)
{
    if (!nchannels && !freq && !sampleFormat) {
        return E_POINTER;
    }
    if (!currentOutsf) {
        return E_UNEXPECTED;
    }
    if (nchannels) {
        *nchannels = currentOutsf.nchannels;
    }
    if (freq) {
        *freq = currentOutsf.freq;
    }
    if (sampleFormat) {
        *sampleFormat = currentOutsf.sf;
    }
    return S_OK;
}
STDMETHODIMP_(int) TffdshowDecAudio::getJitter(void)
{
    return inpin ? inpin->getJitter() : 0;
}

STDMETHODIMP TffdshowDecAudio::getWinamp2(Twinamp2* *winamp2ptr)
{
    if (!winamp2ptr) {
        return E_POINTER;
    }
    if (!winamp2) {
        winamp2 = new Twinamp2(getParamStr2(IDFF_winamp2dir));
    }
    *winamp2ptr = winamp2;
    return S_OK;
}
void TffdshowDecAudio::onWinamp2dirChange(int paramID, const char_t *valname)
{
    // This is called when the dialog is opened during playback, even if winamp2 dir is not changed.
    // If we delete winamp2 during playback, ffdshow crashes.
    // Winamp dir cannot be changed during playback.
    // It's hard to code. I drop it.
    if (globalSettings->filtermode & IDFF_FILTERMODE_CONFIG) {
        if (winamp2) {
            delete winamp2;
        }
        winamp2 = NULL;
    }
}

STDMETHODIMP TffdshowDecAudio::getEncoderInfo(char_t *buf, size_t buflen)
{
    return inpin->getEncoderInfo(buf, buflen);
}
STDMETHODIMP TffdshowDecAudio::getInCodecString(char_t *buf, size_t buflen)
{
    return inpin->getInCodecString(buf, buflen);
}
STDMETHODIMP_(const char_t*) TffdshowDecAudio::getDecoderName(void)
{
    return inpin->getDecoderName();
}

STDMETHODIMP TffdshowDecAudio::getOutCodecString(char_t *buf, size_t buflen)
{
    if (!buf) {
        return E_POINTER;
    }
    AVCodecID codecId = AV_CODEC_ID_NONE;
    if (inpin) {
        inpin->getCodecId(&codecId);
    }

    if (bitstream_codec(codecId)) {
        tsnprintf_s(buf, buflen, _TRUNCATE, _l("HDMI bitstream (%ld)"), codecId);
        buf[buflen - 1] = '\0';
    } else if ((inpin && inpin->is_spdif_codec()) || currentOutsf.sf == TsampleFormat::SF_AC3) {
        tsnprintf_s(buf, buflen, _TRUNCATE, _l("S/PDIF (%ld)"), codecId);
        buf[buflen - 1] = '\0';
    } else {
        currentOutsf.descriptionPCM(buf, buflen);
    }
    return S_OK;
}
STDMETHODIMP TffdshowDecAudio::getOutSpeakersDescr(char_t *buf, size_t buflen, int shortcuts)
{
    if (!buf) {
        return E_POINTER;
    }
    currentOutsf.getSpeakersDescr(buf, buflen, !!shortcuts);
    return S_OK;
}

HRESULT TffdshowDecAudio::ReconnectOutput(size_t numsamples, CMediaType& mt)
{
    IPin *cpin = m_pOutput->GetConnected();
    if (!cpin) {
        return E_NOINTERFACE;
    }

    WAVEFORMATEX *wfe = (WAVEFORMATEX*)mt.Format();
    long cbBuffer = long(numsamples * wfe->nBlockAlign);
    if (mt != m_pOutput->CurrentMediaType() || cbBuffer > actual.cbBuffer) {
        DPRINTF(_l("TffdshowDecAudio::ReconnectOutput because output media type changed"));
        TsampleFormat::DPRINTMediaTypeInfo(mt);
        if (cbBuffer > actual.cbBuffer) {
            comptr<IMemInputPin> pPin;
            if (FAILED(cpin->QueryInterface(IID_IMemInputPin, (void**)&pPin))) {
                return E_NOINTERFACE;
            }

            HRESULT hr;
            comptr<IMemAllocator> pAllocator;
            if (FAILED(hr = pPin->GetAllocator(&pAllocator)) || !pAllocator) {
                return S_OK;    //hr; Avisynth GetSample filter can't handle format changes
            }

            ALLOCATOR_PROPERTIES props;
            if (FAILED(hr = pAllocator->GetProperties(&props))) {
                return hr;
            }

            props.cBuffers = 4;
            props.cbBuffer = cbBuffer * 3 / 2;

            if (FAILED(hr = m_pOutput->DeliverBeginFlush()) ||
                    FAILED(hr = m_pOutput->DeliverEndFlush()) ||
                    FAILED(hr = pAllocator->Decommit()) ||
                    FAILED(hr = pAllocator->SetProperties(&props, &actual)) ||
                    FAILED(hr = pAllocator->Commit())) {
                return hr;
            }

            if (props.cBuffers > actual.cBuffers || props.cbBuffer > actual.cbBuffer) {
                NotifyEvent(EC_ERRORABORT, hr, 0);
                return E_FAIL;
            }
        }
        return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP_(unsigned int) TffdshowDecAudio::getNumStreams2(void)
{
    return inpins.getNumConnectedInpins();
}
STDMETHODIMP TffdshowDecAudio::getStreamDescr(unsigned int i, char_t *buf, size_t buflen)
{
    if (i >= inpins.getNumConnectedInpins()) {
        return E_INVALIDARG;
    }
    if (!buf) {
        return E_POINTER;
    }
    TffdshowDecAudioInputPin *pin = inpins.getConnectedInpin(i);
    if (pin && pin->getStreamName(buf, buflen) == S_OK) {
        return S_OK;
    } else {
        return E_FAIL;
    }
}
STDMETHODIMP_(unsigned int) TffdshowDecAudio::getCurrentStream2(void)
{
    CAutoLock cs(&m_csReceive);
    int ii = 0;
    for (size_t i = 0; i < inpins.size(); i++)
        if (inpins[i]->IsConnected())
            if (inpin == inpins[i]) {
                return ii;
            } else {
                ii++;
            }
    return (unsigned int) - 1;
}
STDMETHODIMP TffdshowDecAudio::Enable(long lIndex, DWORD dwFlags)
{
    unsigned int i = (unsigned int)lIndex;
    return setCurrentStream(i);
}
STDMETHODIMP TffdshowDecAudio::Count(DWORD *pcStreams)
{
    if (pcStreams) {
        *pcStreams = (DWORD) inpins.getNumConnectedInpins();
    }
    return S_OK;
}
STDMETHODIMP TffdshowDecAudio::Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk)
{
    if (lIndex < 0 || lIndex >= (long)inpins.getNumConnectedInpins() || !presetSettings) {
        return E_INVALIDARG;
    }
    if (ppmt) {
        *ppmt = getInputMediaType(lIndex);
    }
    if (pdwFlags) {
        if (getCurrentStream2() == (unsigned int) lIndex) {
            *pdwFlags = AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE;
        } else {
            *pdwFlags = 0;
        }
    }
    if (plcid) {
        *plcid = 0;
    }
    if (pdwGroup) {
        *pdwGroup = 1;  //Audio
    }
    if (ppszName) {
        char_t descr[250];
        if (getStreamDescr((unsigned int) lIndex, descr, 250) == S_OK) {
            ffstring name = descr;
            size_t wlen = (name.size() + 1) * sizeof(WCHAR);
            *ppszName = (WCHAR*)CoTaskMemAlloc(wlen);
            memset(*ppszName, 0, wlen);
            nCopyAnsiToWideChar(*ppszName, name.c_str());
        }
    }
    if (ppObject) {
        *ppObject = NULL;
    }
    if (ppUnk) {
        *ppUnk = NULL;
    }
    return S_OK;
}
STDMETHODIMP TffdshowDecAudio::setCurrentStream(unsigned int i)
{
    if (i >= inpins.getNumConnectedInpins()) {
        return E_INVALIDARG;
    }
    return setCurrentStream2(inpins.getConnectedInpin(i));
}
STDMETHODIMP TffdshowDecAudio::setCurrentStream2(TffdshowDecAudioInputPin *newipin)
{
    if (newipin == inpin) {
        return S_OK;
    }

    // Set current input pin to none so that they flush themselves
    inpin = NULL;

    // Let the other pins deliver their last samples
    for (int i = 0; i < (int)inpins.getNumConnectedInpins(); i++) {
        if (newipin == inpins.getConnectedInpin(i)) {
            continue;
        }
        inpins.getConnectedInpin(i)->block(false);
        inpins.getConnectedInpin(i)->block(true);
    }

    // Set new pin
    inpin = newipin;

    //CAutoLock cs(&m_csReceive);
    if (m_pOutput) {
        m_pOutput->DeliverBeginFlush();
        m_pOutput->DeliverEndFlush();
    }
    comptrQ<IMediaControl> _pMC = m_pGraph;
    OAFilterState _fs = -1;
    if (_pMC) {
        _pMC->GetState(1000, &_fs);
    }
    if (_fs == State_Running) {
        _pMC->Pause();
    }
    HRESULT _hr = E_FAIL;
    comptrQ<IMediaSeeking> _pMS = m_pGraph;
    LONGLONG _rtNow = 0;
    if (_pMS) {
        _hr = _pMS->GetCurrentPosition(&_rtNow);
    }

    // Unblock the stream on the new pin
    inpin->block(false);

    if (SUCCEEDED(_hr) && _pMS) {
        _hr = _pMS->SetPositions(&_rtNow, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
    }
    if (_fs == State_Running && _pMS) {
        _pMC->Run();
    }
    return _hr;
}

STDMETHODIMP_(TffdshowDecAudioInputPin *) TffdshowDecAudio::GetCurrentPin(void)
{
    return inpin;
}


AM_MEDIA_TYPE* TffdshowDecAudio::getInputMediaType(int lIndex)
{
    TffdshowDecAudioInputPin *pPin = inpins.getConnectedInpin(lIndex);
    return pPin ?::CreateMediaType(&pPin->CurrentMediaType()) : ::CreateMediaType(&m_pInput->CurrentMediaType());
}
STDMETHODIMP_(int) TffdshowDecAudio::getInputBitrate2(void)
{
    return inpin->getInputBitrate();
}

bool TffdshowDecAudio::isStreamsMenu(void) const
{
    return isAudioSwitcher || globalSettings->streamsMenu;
}
void TffdshowDecAudio::addOwnStreams(void)
{
    unsigned int numstreams = inpins.getNumConnectedInpins();
    if (numstreams > 1)
        for (unsigned int i = 0; i < numstreams; i++) {
            streams.push_back(new TstreamAudio(this, 1000 + i, 200, inpins.getConnectedInpin(i)));
        }
}

DWORD TffdshowDecAudio::TstreamAudio::getFlags(void)
{
    return self->inpin == pPin ? AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE : 0;
}
const char_t* TffdshowDecAudio::TstreamAudio::getName(void)
{
    if (pPin->getStreamName(name, 256) != S_OK) {
        tsprintf(name, _l("In%i"), pPin->number);
    }
    return name;
}
bool TffdshowDecAudio::TstreamAudio::action(void)
{
    self->setCurrentStream2(pPin);
    return false;
}

STDMETHODIMP TffdshowDecAudio::setAudioFilters(TaudioFilters *audioFiltersPtr)
{
    audioFilters = audioFiltersPtr;
    return S_OK;
}

TinfoBase* TffdshowDecAudio::createInfo(void)
{
    return new TinfoDecAudio(this);
}

int TffdshowDecAudio::get_trayIconType(void)
{
    switch (globalSettings->trayIconType) {
        case 2:
            return IDI_FFDSHOWAUDIO;
        case 0:
        case 1:
        default:
            return IDI_MODERN_ICON_A;
    }
}

STDMETHODIMP_(TinputPin*) TffdshowDecAudio::getInputPin(void)
{
    return GetCurrentPin();
}

STDMETHODIMP_(CTransformOutputPin*) TffdshowDecAudio::getOutputPin(void)
{
    return m_pOutput;
}

STDMETHODIMP TffdshowDecAudio::getInputTime(REFERENCE_TIME &rtStart, REFERENCE_TIME &rtStop)
{
    if (inpin == NULL) {
        return E_FAIL;
    }
    rtStart = inpin->insample_rtStart;
    rtStop = inpin->insample_rtStop;
    return S_OK;
}
