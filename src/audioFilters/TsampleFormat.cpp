/*
 * Copyright (c) 2004-2006 Milan Cutka
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
#include "TsampleFormat.h"
#include "ffdshow_mediaguids.h"
#include "codecs/ogg headers/vorbisformat.h"
#include "dsutil.h"

// OS Version
int OSMajorVersion = 0;
int OSMinorVersion = 0;

const TalternateSampleFormat TsampleFormat::alternateSampleFormats[] = {
    //Id            , originalwSubFormat                    , mediaSubtype                              , wFormatTag             , wSubFormat                            , nChannels, wBitsPerSample,nSamplesPerSec, dwChannelMask, isExtensible
    STANDARD_ONLY , KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP, MEDIASUBTYPE_PCM                          , WAVE_FORMAT_EXTENSIBLE , KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP, 0        ,  0            ,     0        , KSAUDIO_SPEAKER_7POINT1, true, // Standard #2
    STANDARD_ONLY , KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP, GUID_NULL                                 , 0                      , MEDIASUBTYPE_DOLBY_TRUEHD              , 0        ,  0            ,     0        , 0, true, // Standard #3
    XONAR         , KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP, KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL, WAVE_FORMAT_ESST_AC3, GUID_NULL                                , 0        ,  0            ,     0        , 0, false, // xonar
    AUZENTECH     , KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP, MEDIASUBTYPE_CYBERLINK_BITSTREAM         , WAVE_FORMAT_CYBERLINK_TRUEHD, GUID_NULL                         , 8        , 16            , 192000       , 0, false, // Auzentech

    STANDARD_ONLY , KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD   , MEDIASUBTYPE_PCM                          , WAVE_FORMAT_EXTENSIBLE , KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD   , 0        ,  0            , 96000        , KSAUDIO_SPEAKER_7POINT1, true, // Standard #2 (Vista)
    STANDARD_ONLY , KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD   , GUID_NULL                                 , 0                      , MEDIASUBTYPE_DTS_HD                    , 0        ,  0            ,     0        , 0, true, // Standard #3
    XONAR         , KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD,    KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL, WAVE_FORMAT_ESST_AC3   , GUID_NULL                              , 8        , 16            , 192000       , 0, false, // xonar
    AUZENTECH     , KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD   , MEDIASUBTYPE_CYBERLINK_BITSTREAM         , WAVE_FORMAT_CYBERLINK_DTS_HD, GUID_NULL                         , 2        , 16            , 96000        , 0, false, // Auzentech

    STANDARD_ONLY , KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS, GUID_NULL                        , 0                      , MEDIASUBTYPE_DOLBY_DDPLUS              , 0        ,  0            ,     0        , 0, true,
    XONAR, KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS, KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL, WAVE_FORMAT_ESST_AC3, GUID_NULL                       , 0        ,  0            ,     0        , 0, false,
    0
};
/*KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP, KSDATAFORMAT_SUBTYPE_WAVEFORMATEX       , WAVE_FORMAT_RAW_SPORT  ,MEDIASUBTYPE_DOLBY_TRUEHD, 2, 16, 48000, 0,  false, // WinDVD
 KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD,    KSDATAFORMAT_SUBTYPE_WAVEFORMATEX       , WAVE_FORMAT_RAW_SPORT  ,MEDIASUBTYPE_DTS_HD    ,   2, 16, 48000, 0, false, // WinDVD
 KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS, KSDATAFORMAT_SUBTYPE_WAVEFORMATEX, WAVE_FORMAT_RAW_SPORT,MEDIASUBTYPE_DOLBY_DDPLUS, 2, 16, 48000, 0, false, // WinDVD*/



const DWORD TsampleFormat::standardChannelMasks[] = {
    SPEAKER_FRONT_CENTER,
    SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT,
    SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_CENTER,
    SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
    SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
    SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
    SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_BACK_CENTER | SPEAKER_LOW_FREQUENCY,
    SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_LOW_FREQUENCY
};



TsampleFormat::TsampleFormat(const WAVEFORMATEX &wfex, bool wfextcheck, const GUID *subtype): pcm_be(false)
{
    init(wfex, wfextcheck, subtype);
}
void TsampleFormat::init(const WAVEFORMATEX &wfex, bool wfextcheck, const GUID *subtype)
{
    alternateSF = -1; // Used for audio renderers that do not accept official bitstream media types
    if (wfextcheck && wfex.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        init(*(const WAVEFORMATEXTENSIBLE*)&wfex, subtype);
    } else {
        freq = wfex.nSamplesPerSec;
        setChannels(wfex.nChannels, 0);
        if (wfex.wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
            sf = SF_FLOAT32;
        } else if (wfex.wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF) {
            sf = SF_AC3;
        } else if (subtype && (*subtype == MEDIASUBTYPE_twos || *subtype == MEDIASUBTYPE_TWOS)) {
            sf = SF_PCM16;
            pcm_be = true;
        } else if (subtype && (*subtype == MEDIASUBTYPE_sowt || *subtype == MEDIASUBTYPE_sowt)) {
            sf = SF_PCM16;
        } else if (subtype && (*subtype == MEDIASUBTYPE_IN32 || *subtype == MEDIASUBTYPE_in32 || *subtype == MEDIASUBTYPE_IN24 || *subtype == MEDIASUBTYPE_in24 || *subtype == MEDIASUBTYPE_FL32 || *subtype == MEDIASUBTYPE_fl32 || *subtype == MEDIASUBTYPE_FL64 || *subtype == MEDIASUBTYPE_fl64)) {
            if (*subtype == MEDIASUBTYPE_IN32 || *subtype == MEDIASUBTYPE_in32) {
                sf = SF_PCM32;
            } else if (*subtype == MEDIASUBTYPE_IN24 || *subtype == MEDIASUBTYPE_in24) {
                sf = SF_PCM24;
            } else if (*subtype == MEDIASUBTYPE_FL32 || *subtype == MEDIASUBTYPE_fl32) {
                sf = SF_FLOAT32;
            } else if (*subtype == MEDIASUBTYPE_FL64 || *subtype == MEDIASUBTYPE_fl64) {
                sf = SF_FLOAT64;
            }
            Textradata extradata(wfex);
            if (extradata.data) {
                const uint8_t *enda = (const uint8_t*)memnstr(extradata.data, extradata.size, "enda"); //TODO: properly parse headers
                if (enda && *(uint16_t*)(enda + 4) == 0) {
                    pcm_be = true;
                }
            }
        } else
            switch (wfex.wBitsPerSample) {
                case 8:
                    sf = SF_PCM8;
                    break;
                case 0:
                case 16:
                default:
                    sf = SF_PCM16;
                    break;
                case 20:
                    sf = SF_LPCM20;
                    break;
                case 24:
                    sf = SF_PCM24;
                    break;
                case 32:
                    sf = SF_PCM32;
                    break;
            }
        dolby = DOLBY_NO;
    }
}
TsampleFormat::TsampleFormat(const WAVEFORMATEXTENSIBLE &wfexten, const GUID *subtype): pcm_be(false)
{
    init(wfexten, subtype);
}
void TsampleFormat::init(const WAVEFORMATEXTENSIBLE &wfexten, const GUID *subtype)
{
    init(wfexten.Format, false, subtype);
    setChannels(wfexten.Format.nChannels, wfexten.dwChannelMask);
    if (wfexten.SubFormat == MEDIASUBTYPE_IEEE_FLOAT) {
        sf = SF_FLOAT32;
    }
}
TsampleFormat::TsampleFormat(const VORBISFORMAT &vf): pcm_be(false)
{
    init(vf);
}
void TsampleFormat::init(const VORBISFORMAT &vf)
{
    freq = vf.nSamplesPerSec;
    sf = SF_PCM16;
    setChannels(vf.nChannels, 0);
    dolby = DOLBY_NO;
    alternateSF = -1;
}
TsampleFormat::TsampleFormat(const VORBISFORMAT2 &vf2): pcm_be(false)
{
    init(vf2);
}
void TsampleFormat::init(const VORBISFORMAT2 &vf2)
{
    alternateSF = -1;
    freq = vf2.SamplesPerSec;
    switch (vf2.BitsPerSample) {
        case 0:
        case 16:
        default:
            sf = SF_PCM16;
            break;
        case 24:
            sf = SF_PCM24;
            break;
        case 32:
            sf = SF_PCM32;
            break;
    }
    setChannels(vf2.Channels, 0);
    dolby = DOLBY_NO;
}
TsampleFormat::TsampleFormat(const VORBISFORMATILL &vfIll): pcm_be(false)
{
    init(vfIll);
}
void TsampleFormat::init(const VORBISFORMATILL &vfIll)
{
    alternateSF = -1;
    freq = vfIll.samplesPerSec;
    sf = SF_PCM16;
    setChannels(vfIll.numChannels, 0);
    dolby = DOLBY_NO;
}
TsampleFormat::TsampleFormat(const AM_MEDIA_TYPE &mt): pcm_be(false)
{
    alternateSF = -1;
    if (mt.formattype == FORMAT_VorbisFormat) {
        init(*(const VORBISFORMAT*)mt.pbFormat);
    } else if (mt.formattype == FORMAT_VorbisFormat2) {
        init(*(const VORBISFORMAT2*)mt.pbFormat);
    } else if (mt.formattype == FORMAT_VorbisFormatIll) {
        init(*(const VORBISFORMATILL*)mt.pbFormat);
    } else if (mt.formattype == FORMAT_WaveFormatEx) {
        init(*(const WAVEFORMATEX*)mt.pbFormat, true, &mt.subtype);
    } else {
        nchannels = NULL;
        sf = SF_NULL;
    }
}

int TsampleFormat::sf_bestMatch(int sfIn, int wantedSFS)
{
    const int *bestsfs = NULL;
    switch (sfIn) {
        case SF_PCM16: {
            static const int best[] = {
                SF_PCM32,
                SF_PCM24,
                SF_FLOAT32,
                SF_NULL
            };
            bestsfs = best;
            break;
        }
        case SF_PCM24: {
            static const int best[] = {
                SF_PCM32,
                SF_PCM16,
                SF_FLOAT32,
                SF_NULL
            };
            bestsfs = best;
            break;
        }
        case SF_PCM32: {
            static const int best[] = {
                SF_PCM16,
                SF_PCM24,
                SF_FLOAT32,
                SF_NULL
            };
            bestsfs = best;
            break;
        }
        case SF_PCM8: {
            static const int best[] = {
                SF_PCM16,
                SF_PCM32,
                SF_NULL
            };
            bestsfs = best;
            break;
        }
        case SF_FLOAT32: {
            static const int best[] = {
                SF_PCM32,
                SF_PCM16,
                SF_PCM24,
                SF_NULL
            };
            bestsfs = best;
            break;
        }
        default:
            return SF_NULL;
    }
    while (*bestsfs) {
        if (*bestsfs & wantedSFS) {
            return *bestsfs;
        }
        bestsfs++;
    }
    return SF_NULL;
}

DWORD TsampleFormat::getPCMformat(const CMediaType &mtIn, DWORD def)
{
    if (*mtIn.FormatType() != FORMAT_WaveFormatEx) {
        return def;
    }
    const WAVEFORMATEX *wfex = (const WAVEFORMATEX*)mtIn.Format();
    if (wfex->wFormatTag != WAVE_FORMAT_EXTENSIBLE && wfex->wFormatTag != WAVE_FORMAT_PCM && wfex->wFormatTag != WAVE_FORMAT_IEEE_FLOAT && mtIn.subtype != MEDIASUBTYPE_RAW && mtIn.subtype != MEDIASUBTYPE_NONE) {
        return def;
    }
    TsampleFormat sf(mtIn);
    switch (sf.sf) {
        case SF_NULL:
            return def;
        case SF_PCM8:
            return WAVE_FORMAT_PCM8;
        case SF_PCM16:
            return WAVE_FORMAT_PCM16;
        case SF_PCM24:
            return WAVE_FORMAT_PCM24;
        case SF_PCM32:
            return WAVE_FORMAT_PCM32;
        case SF_FLOAT32:
            return WAVE_FORMAT_FLOAT32;
        case SF_FLOAT64:
            return WAVE_FORMAT_FLOAT64;
    }
    return def;
}



const char_t *TsampleFormat::getGuidName(GUID guid)
{
    if (guid == FORMAT_WaveFormatEx) {
        return _l("FORMAT_WaveFormatEx");
    }
    if (guid == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS) {
        return _l("KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS");
    }
    if (guid == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP) {
        return _l("KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP");
    }
    if (guid == KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD) {
        return _l("KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD");
    }
    if (guid == MEDIASUBTYPE_PCM) {
        return _l("MEDIASUBTYPE_PCM");
    }
    if (guid == MEDIASUBTYPE_DTS) {
        return _l("MEDIASUBTYPE_DTS");
    }
    if (guid == MEDIASUBTYPE_DOLBY_AC3) {
        return _l("MEDIASUBTYPE_DOLBY_AC3");
    }
    if (guid == MEDIASUBTYPE_DOLBY_AC3_SPDIF) {
        return _l("MEDIASUBTYPE_DOLBY_AC3_SPDIF");
    }
    if (guid == MEDIASUBTYPE_IEEE_FLOAT) {
        return _l("MEDIASUBTYPE_IEEE_FLOAT");
    }
    if (guid == MEDIASUBTYPE_DOLBY_TRUEHD) {
        return _l("MEDIASUBTYPE_DOLBY_TRUEHD");
    }
    if (guid == MEDIASUBTYPE_DTS_HD) {
        return _l("MEDIASUBTYPE_DTS_HD");
    }
    if (guid == MEDIASUBTYPE_EAC3) {
        return _l("MEDIASUBTYPE_EAC3");
    }
    char_t *tmp = (char_t *)malloc(sizeof(char_t) * 512);
    guid2str(guid, tmp, 512);
    return tmp;
}

/* This method is called by toWAVEFORMATEXTENSIBLE and toWAVEFORMATEXTENSIBLE_IEC61936 to
fill the whole WAVEFORMATEXTENSIBLE structure */
void TsampleFormat::fillCommonWAVEFORMATEX(WAVEFORMATEX *pWfe, WAVEFORMATEXTENSIBLE *pWfex, bool alwayextensible) const
{
    bool hdFormat = false;
    if (sf == SF_FLOAT32) {
        pWfe->wFormatTag = (WORD)WAVE_FORMAT_IEEE_FLOAT;
    } else if (sf == SF_LPCM16) {
        pWfe->wFormatTag = (WORD)WAVE_FORMAT_UNKNOWN;
    } else  if (sf == SF_TRUEHD || sf == SF_DTSHD || sf == SF_EAC3) {
        hdFormat = true;
        pWfe->wFormatTag = (WORD)WAVE_FORMAT_EXTENSIBLE;
        pWfe->wBitsPerSample = 16; // Always 16 bits for HDMI
        pWfe->nSamplesPerSec = 192000; // Always 192Khz for HDMI
        if (pWfex != NULL) {
            pWfex->Samples.wValidBitsPerSample = 16;
            pWfex->dwChannelMask = 0;
        }

        if (sf == SF_TRUEHD) {
            pWfe->nChannels = 8; // 4 IEC lines, different from the number of channels
            if (pWfex != NULL) {
                pWfex->SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP;
            }
        } else if (sf == SF_EAC3) {
            pWfe->nChannels = 2; // 1 IEC line, different from the number of channels
            if (pWfex != NULL) {
                pWfex->SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS;
                pWfex->dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
            }
        } else { //DTS HD
            // 96000 Hz for DTS HD which is the encoded sample rate
            pWfe->nChannels = 8; // 4 IEC lines, different from the number of channels
            if (pWfex != NULL) {
                pWfex->SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD;
            }

            // 96000 Hz (encoded sample rate) on vista instead of 196000 Hz on win7 ?
            /*if (OSMajorVersion==0)
            {
             OSVERSIONINFO osvi;
             ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
             osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
             GetVersionEx(&osvi);
             OSMajorVersion=osvi.dwMajorVersion;
             OSMinorVersion=osvi.dwMinorVersion;
            }

            if (OSMajorVersion < 6 || (OSMajorVersion==6 && OSMinorVersion==0)) // <= Vista
             pWfe->nSamplesPerSec=96000;*/

        }

        if (pWfex != NULL && pWfex->dwChannelMask == 0)
            switch (nchannels) {
                case 8:
                    pWfex->dwChannelMask = KSAUDIO_SPEAKER_7POINT1;
                    break;
                case 6:
                    pWfex->dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
                    break;
            }
    } else {
        pWfe->wFormatTag = (WORD)WAVE_FORMAT_PCM;
    }

    if (!hdFormat) { // channels and nSamplesPerSec different for HD formats (set before)
        pWfe->nChannels = WORD(nchannels);
        pWfe->nSamplesPerSec = freq;
        pWfe->wBitsPerSample = (WORD)bitsPerSample();
    }

    pWfe->nBlockAlign = WORD(pWfe->nChannels * pWfe->wBitsPerSample / 8);
    pWfe->nAvgBytesPerSec = pWfe->nSamplesPerSec * pWfe->nBlockAlign;

    // FIXME: 24/32 bit only seems to work with WAVE_FORMAT_EXTENSIBLE
    DWORD dwChannelMask;
    if (channelmask == 0 && (sf == TsampleFormat::SF_PCM24 || sf == TsampleFormat::SF_PCM32 || nchannels > 2)) {
        dwChannelMask = standardChannelMasks[nchannels - 1];
    } else {
        dwChannelMask = channelmask;
    }

    if (!alwayextensible && dwChannelMask == standardChannelMasks[nchannels - 1]) {
        dwChannelMask = 0;
    }

    if (!hdFormat) {
        if (dwChannelMask) {
            pWfe->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
            if (pWfex != NULL) {
                pWfex->dwChannelMask = dwChannelMask;
                pWfex->Samples.wValidBitsPerSample = pWfe->wBitsPerSample;
                pWfex->SubFormat = sf == SF_FLOAT32 ? MEDIASUBTYPE_IEEE_FLOAT : MEDIASUBTYPE_PCM;
            }
        } else {
            pWfe->cbSize = 0;
        }
    } else if (pWfex != NULL && pWfex->dwChannelMask == 0) {
        pWfex->dwChannelMask = dwChannelMask;
    }
}

// New structure : not compatible with all players and OS. May be compatible with Q4 2009 cards
WAVEFORMATEXTENSIBLE_IEC61937 TsampleFormat::toWAVEFORMATEXTENSIBLE_IEC61937(bool alwayextensible) const
{
    WAVEFORMATEXTENSIBLE_IEC61937 wfex_IEC61937;
    memset(&wfex_IEC61937, 0, sizeof(wfex_IEC61937));
    wfex_IEC61937.FormatExt.Format.cbSize = sizeof(wfex_IEC61937) - sizeof(wfex_IEC61937.FormatExt.Format);

    WAVEFORMATEXTENSIBLE *pWfex = &wfex_IEC61937.FormatExt;

    fillCommonWAVEFORMATEX(&(pWfex->Format), pWfex, alwayextensible);

    if (sf == SF_TRUEHD || sf == SF_DTSHD || sf == SF_EAC3) {
        wfex_IEC61937.dwAverageBytesPerSec = 0;
        wfex_IEC61937.dwEncodedSamplesPerSec = freq;
        wfex_IEC61937.dwEncodedChannelCount = WORD(nchannels);
        wfex_IEC61937.dwEncodedSamplesPerSec = freq;
    }
    return wfex_IEC61937;
}

WAVEFORMATEXTENSIBLE TsampleFormat::toWAVEFORMATEXTENSIBLE(bool alwayextensible) const
{
    WAVEFORMATEXTENSIBLE wfex;
    memset(&wfex, 0, sizeof(wfex));
    wfex.Format.cbSize = sizeof(wfex) - sizeof(wfex.Format);
    WAVEFORMATEX *pWfe = &wfex.Format;
    WAVEFORMATEXTENSIBLE *pWfex = &wfex;

    fillCommonWAVEFORMATEX(pWfe, pWfex, alwayextensible);
    return wfex;
}

WAVEFORMATEX TsampleFormat::toWAVEFORMATEX() const
{
    WAVEFORMATEX wfe;
    memset(&wfe, 0, sizeof(wfe));
    wfe.cbSize = 0;

    fillCommonWAVEFORMATEX(&wfe, NULL, false);
    return wfe;
}

CMediaType TsampleFormat::toCMediaType(bool alwaysextensible) const
{
    CMediaType mt;
    /*if (sf==SF_LPCM16)
     mt.majortype=MEDIATYPE_MPEG2_PES;
    else*/
    mt.majortype = MEDIATYPE_Audio;
    if (sf == SF_FLOAT32) {
        mt.subtype = MEDIASUBTYPE_IEEE_FLOAT;
    } else if (sf == SF_LPCM16) {
        mt.subtype = MEDIASUBTYPE_DVD_LPCM_AUDIO;
    } else {
        mt.subtype = MEDIASUBTYPE_PCM;
    }
    mt.formattype = FORMAT_WaveFormatEx;
    if (alternateSF != -1 && !alternateSampleFormats[alternateSF].isExtensible) {
        WAVEFORMATEX wfe = toWAVEFORMATEX();
        mt.SetFormat((BYTE*)&wfe, sizeof(wfe));
        updateAlternateMediaType(mt, alternateSF);
    } else {
        WAVEFORMATEXTENSIBLE wfex = toWAVEFORMATEXTENSIBLE(alwaysextensible);
        mt.SetFormat((BYTE*)&wfex, sizeof(wfex.Format) + wfex.Format.cbSize);
        updateAlternateMediaType(mt, alternateSF);
    }

    return mt;
}


CMediaType TsampleFormat::toCMediaTypeHD(bool alwaysextensible) const
{
    CMediaType mt;
    mt.majortype = MEDIATYPE_Audio;

    if (sf == SF_FLOAT32) {
        mt.subtype = MEDIASUBTYPE_IEEE_FLOAT;
    } else if (sf == SF_LPCM16) {
        mt.subtype = MEDIASUBTYPE_DVD_LPCM_AUDIO;
    } else {
        mt.subtype = MEDIASUBTYPE_PCM;
    }
    mt.formattype = FORMAT_WaveFormatEx;

    if (alternateSF != -1 && !alternateSampleFormats[alternateSF].isExtensible) {
        WAVEFORMATEX wfe = toWAVEFORMATEX();
        mt.SetFormat((BYTE*)&wfe, sizeof(wfe));
        updateAlternateMediaType(mt, alternateSF);
    } else {
        WAVEFORMATEXTENSIBLE_IEC61937 wfex_IEC61937 = toWAVEFORMATEXTENSIBLE_IEC61937(alwaysextensible);
        mt.SetFormat((BYTE*)&wfex_IEC61937, sizeof(wfex_IEC61937.FormatExt.Format) + wfex_IEC61937.FormatExt.Format.cbSize);
        updateAlternateMediaType(mt, alternateSF);
    }

    return mt;
}

void TsampleFormat::updateAlternateMediaType(CMediaType &mt, int newSF)
{
    if (newSF == -1 || mt.pbFormat == NULL) {
        return;
    }
    TalternateSampleFormat newSampleFormat = alternateSampleFormats[newSF];

    WAVEFORMATEX *pWfe = NULL;
    WAVEFORMATEXTENSIBLE *pWfex = NULL;

    if (newSampleFormat.isExtensible) {
        pWfex = (WAVEFORMATEXTENSIBLE*)mt.pbFormat;
        pWfe = &(pWfex->Format);
    } else {
        pWfe = (WAVEFORMATEX*)mt.pbFormat;
    }


    if (newSampleFormat.wSubFormat != GUID_NULL && pWfex != NULL) {
        pWfex->SubFormat = newSampleFormat.wSubFormat;
    }
    if (newSampleFormat.mediaSubtype != GUID_NULL) {
        mt.subtype = newSampleFormat.mediaSubtype;
    }
    if (newSampleFormat.wFormatTag != 0) {
        pWfe->wFormatTag = newSampleFormat.wFormatTag;
    }
    if (newSampleFormat.nChannels != 0) {
        pWfe->nChannels = newSampleFormat.nChannels;
    }
    if (newSampleFormat.wBitsPerSample != 0) {
        pWfe->wBitsPerSample = newSampleFormat.wBitsPerSample;
    }
    if (newSampleFormat.nSamplesPerSec != 0) {
        pWfe->nSamplesPerSec = newSampleFormat.nSamplesPerSec;
    }
    if (newSampleFormat.dwChannelMask != 0 && pWfex != NULL) {
        pWfex->dwChannelMask = newSampleFormat.dwChannelMask;
    }

    pWfe->nBlockAlign = WORD(pWfe->nChannels * pWfe->wBitsPerSample / 8);
    pWfe->nAvgBytesPerSec = pWfe->nSamplesPerSec * pWfe->nBlockAlign;
}

CMediaType TsampleFormat::createMediaTypeSPDIF(unsigned int frequency)
{
    // S/PDIF mode mandates 2 Channels and 16 bits per sample, regardless of the underlying format.
    // S/PDIF interface supports three standard sample rates: 48 kHz, 44.1 kHz and 32 kHz..
    // the underlying format can have different number of bits per sample or different sample rate, such in the case of DTS 96/24, (transferred over S/PDIF link at 48kHz, 16 bits per sample)
    // if possible, it's best to select S/PDIF sample rate based on the underlying sample rate.
    if (frequency != 48000 && frequency != 44100 && frequency != 32000 && frequency != 192000 && frequency != 96000) {
        frequency = 48000;
    }
    CMediaType mt = TsampleFormat(SF_PCM16, frequency, 2).toCMediaType();
    ((WAVEFORMATEX*)mt.pbFormat)->wFormatTag = WAVE_FORMAT_DOLBY_AC3_SPDIF;
    return mt;
}

const char_t* TsampleFormat::getSpeakerName(int speaker, bool shrt)
{
    switch (speaker) {
        case SPEAKER_FRONT_LEFT:
            return shrt ? _l("L") : _l("front left");
        case SPEAKER_FRONT_RIGHT:
            return shrt ? _l("R") : _l("front right");
        case SPEAKER_FRONT_CENTER:
            return shrt ? _l("C") : _l("front center");
        case SPEAKER_LOW_FREQUENCY:
            return _l("LFE");
        case SPEAKER_BACK_LEFT:
            return _l("back left");
        case SPEAKER_BACK_RIGHT:
            return _l("back right");
        case SPEAKER_FRONT_LEFT_OF_CENTER:
            return _l("front left of center");
        case SPEAKER_FRONT_RIGHT_OF_CENTER:
            return _l("front right of center");
        case SPEAKER_BACK_CENTER:
            return _l("back center");
        case SPEAKER_SIDE_LEFT:
            return _l("side left");
        case SPEAKER_SIDE_RIGHT:
            return _l("side right");
        case SPEAKER_TOP_CENTER:
            return _l("top center");
        case SPEAKER_TOP_FRONT_LEFT:
            return _l("top front left");
        case SPEAKER_TOP_FRONT_CENTER:
            return _l("top front center");
        case SPEAKER_TOP_FRONT_RIGHT:
            return _l("top front right");
        case SPEAKER_TOP_BACK_LEFT:
            return _l("top back left");
        case SPEAKER_TOP_BACK_CENTER:
            return _l("top back center");
        case SPEAKER_TOP_BACK_RIGHT:
            return _l("top back right");
        default:
            return _l("unknown");
    }
}
void TsampleFormat::getSpeakersDescr(char_t *buf, size_t buflen, bool shrt) const
{
    buf[0] = '\0';
    for (unsigned int i = 0; i < nchannels; i++) {
        strncatf(buf, buflen, _l("%s,"), getSpeakerName(speakers[i], shrt));
    }
    buf[buflen - 1] = '\0';
    size_t len = strlen(buf);
    if (len && buf[len - 1] == ',') {
        buf[len - 1] = '\0';
    }
}

// Static method that returns the output sample format according to the input stream to passthrough
int TsampleFormat::getSampleFormat(AVCodecID codecId)
{
    switch (codecId) {
        case CODEC_ID_BITSTREAM_TRUEHD:
            return TsampleFormat::SF_TRUEHD;
        case CODEC_ID_BITSTREAM_DTSHD:
            return TsampleFormat::SF_DTSHD;
        case CODEC_ID_BITSTREAM_EAC3:
            return TsampleFormat::SF_EAC3;
        case CODEC_ID_SPDIF_AC3:
        case CODEC_ID_SPDIF_DTS:
            return TsampleFormat::SF_AC3;
        default:
            return TsampleFormat::SF_PCM16;
    }
}


// Debug function that dumps the content of a CMediaType with WAVEFORMATEX(TENSIBLE(_IEC61937)) content
void TsampleFormat::DPRINTMediaTypeInfo(CMediaType mt)
{
    if (!allowDPRINTF) {
        return;
    }
    ffstring formatDescr = ffstring(_l(""));
    char_t tmp[1024] = _l("");
    tsprintf(tmp, _l("Media Type Structure\n Format type : %s\n "), getGuidName(mt.formattype));
    formatDescr.append(tmp);
    tsprintf(tmp, _l("Sub type : %s\n "), getGuidName(mt.subtype));
    formatDescr.append(tmp);

    if (mt.formattype == FORMAT_WaveFormatEx) {
        WAVEFORMATEX *wfex = (WAVEFORMATEX*)mt.pbFormat;
        if (wfex->cbSize >= 34) { // WAVEFORMATEXTENSIBLE_IEC61937
            WAVEFORMATEXTENSIBLE_IEC61937 *wfextensible_IEC61937 = (WAVEFORMATEXTENSIBLE_IEC61937*)mt.pbFormat;
            tsprintf(tmp, _l("WAVEFORMATEXTENSIBLE_IEC61937 :\n dwAverageBytesPerSec : %ld\n dwEncodedChannelCount : %ld\n dwEncodedSamplesPerSec : %ld\n "), wfextensible_IEC61937->dwAverageBytesPerSec, wfextensible_IEC61937->dwEncodedChannelCount, wfextensible_IEC61937->dwEncodedSamplesPerSec);
            formatDescr.append(tmp);
        }

        if (wfex->cbSize >= 22) { // WAVEFORMATEXTENSIBLE
            WAVEFORMATEXTENSIBLE *wfextensible = (WAVEFORMATEXTENSIBLE*)mt.pbFormat;
            tsprintf(tmp, _l("\n WAVEFORMATEXTENSIBLE :\n subFormat : %s\n "),  getGuidName(wfextensible->SubFormat));
            formatDescr.append(tmp);
            tsprintf(tmp, _l("\n wSamplesPerBlock : %ld \nValid bits per sample : %ld\n dwChannelMask : %ld\n "), wfextensible->Samples.wSamplesPerBlock, wfextensible->Samples.wValidBitsPerSample, wfextensible->dwChannelMask);
            formatDescr.append(tmp);
        }

        formatDescr.append(_l("\n WAVEFORMATEX :\n wFormatTag : "));

        switch (wfex->wFormatTag) {
            case WAVE_FORMAT_EXTENSIBLE:
                formatDescr.append(_l("WAVE_FORMAT_EXTENSIBLE"));
                break;
            case WAVE_FORMAT_MLP:
                formatDescr.append(_l("MLP"));
                break;
            case 8192:
                formatDescr.append(_l("Dolby AC3"));
                break;
            case WAVE_FORMAT_DOLBY_AC3_SPDIF:
                formatDescr.append(_l("Dolby AC3 SPDIF"));
                break;
            case WAVE_FORMAT_DTS:
                formatDescr.append(_l("DTS"));
                break;
            case WAVE_FORMAT_DTS_W:
                formatDescr.append(_l("DTS wave"));
                break;
            case WAVE_FORMAT_EAC3:
                formatDescr.append(_l("Dolby Digital Plus"));
                break;
            case WAVE_FORMAT_TRUEHD:
                formatDescr.append(_l("Dolby True HD"));
                break;
            case WAVE_FORMAT_DTS_HD:
                formatDescr.append(_l("DTS HD"));
                break;
            case WAVE_FORMAT_PCM:
                formatDescr.append(_l("PCM"));
                break;
            case WAVE_FORMAT_PCM16:
                formatDescr.append(_l("PCM 16"));
                break;
            case WAVE_FORMAT_PCM24:
                formatDescr.append(_l("PCM 24"));
                break;
            case WAVE_FORMAT_PCM32:
                formatDescr.append(_l("PCM 32"));
                break;
            case WAVE_FORMAT_LPCM:
                formatDescr.append(_l("LPCM"));
                break;
            default:
                tsprintf(tmp, _l("format %ld"), wfex->wFormatTag);
                formatDescr.append(tmp);
        }
        formatDescr.append(_l("\n "));
        tsprintf(tmp, _l("Channels : %d \nBits per sample : %d \n Samples per second : %ld \n nBlockAlign : %ld\n nAvgBytesPerSec : %ld\n "), wfex->nChannels, wfex->wBitsPerSample, wfex->nSamplesPerSec, wfex->nBlockAlign, wfex->nAvgBytesPerSec);
        formatDescr.append(tmp);
    }
    DPRINTF(formatDescr.c_str());
}

