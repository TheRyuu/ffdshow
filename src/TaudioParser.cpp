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
#include "TaudioParser.h"
#include "IffdshowBase.h"
#include "IffdshowDec.h"
#include "IffdshowDecAudio.h"
#include "dsutil.h"
#include "ffdshow_mediaguids.h"
#include "Tpresets.h"
#include "MMDeviceAPI.h"
#include "TffdshowDecAudioInputPin.h"

TaudioParser::TaudioParser(IffdshowBase *Ideci, IdecAudioSink *Isink):
    deci(Ideci),
    deciA(Ideci),
    sinkA(Isink),
    firstFrame(true)
{
    includeBytes = 0;
    skipBytes = 0;
    frame_size = 0;
    usableAC3Passthrough = true;
    usableDTSPassthrough = true;
    usableTrueHDPassthrough = true;
    usableDTSHDPassthrough = true;
    usableEAC3Passthrough = true;
    deci->getGlobalSettings((TglobalSettingsBase **)&globalSettings);
    init();
}

void TaudioParser::init(void)
{
    streamformat = UNDEFINED;
    codecId = AV_CODEC_ID_NONE;
    hasMLPFrames = false;
    searchSync = true;
    initConfig();
    backupbuf.reserve(61440);
}

TaudioParser::~TaudioParser()
{
}

// For debug
#if 0
void TaudioParser::printbitssimple(uint32_t n)
{
    uint32_t i;
    ffstring string;
    i = 1 << (sizeof(n) * 8 - 1);

    while (i > 0) {
        if (n & i) {
            string.append(_l("1"));
        } else {
            string.append(_l("0"));
        }
        i >>= 1;
    }
    DPRINTF(string.c_str());
}
#endif


TaudioParserData TaudioParser::getParserData(void)
{
    return audioParserData;
}

/* This method is used by other decoders to parse AC3/DTS streams
 Returns the stream format and parsed buffer
 */
AVCodecID TaudioParser::parseStream(unsigned char *src, int size,
                                  TbyteBuffer *newsrcBuffer)
{
#if 0
    // DEBUG 1 : dump input buffer
    /*char_t string[10000] = _l("");
    int pos=0;
    Tbitdata bitdata;
    bitdata.bitindex=0;
    bitdata.bitsleft=size*8;
    bitdata.wordpointer=src;

    while(bitdata.bitsleft >= 4)
    {
        pos+=tsprintf(&string[pos], _l("%01X"), bitdata.getBits(4));
        if (pos%992==0)
        {
            DPRINTF(string);
            strcpy(string, _l(""));
            pos=0;
        }

    }
    DPRINTF(string);*/

    //DEBUG 2 : dump parsed stream to file
    if (codecId == CODEC_ID_MLP || codecId == CODEC_ID_TRUEHD) {
        if (!dumpfile) {
            dumpfile = fopen(_l("c:\\temp\\dump.thd"), _l("wb"));
        }
        if (dumpfile) {
            fwrite(src, sizeof(uint8_t), size, dumpfile);
        }
    }
#endif
    audioParserData.frames.clear();

    if (codecId == AV_CODEC_ID_NONE) {
        sinkA->getCodecId(&codecId);
    }

    // For DTS streams, DTS-HD blocks must be removed, libavcodec does not know how to handle them yet
    if ((codecId == AV_CODEC_ID_DTS || codecId == CODEC_ID_LIBDTS || codecId == CODEC_ID_SPDIF_DTS || codecId == CODEC_ID_BITSTREAM_DTSHD
            || codecId == CODEC_ID_PCM) && size > 0) {
        parseDTS(src, size, newsrcBuffer);
        codecId = getCodecIdFromStream();
    } else if ((codecId == AV_CODEC_ID_AC3 || codecId == AV_CODEC_ID_EAC3 || codecId == AV_CODEC_ID_MLP  || codecId == AV_CODEC_ID_TRUEHD
                || codecId == CODEC_ID_LIBA52 || codecId == CODEC_ID_SPDIF_AC3 || codecId == CODEC_ID_BITSTREAM_EAC3 || codecId == CODEC_ID_BITSTREAM_TRUEHD)
               && size > 0) {
        parseAC3(src, size, newsrcBuffer);
        codecId = getCodecIdFromStream();
    }
    return codecId;
}

// This method is called for bitstream formats to check if the output renderer is compatible
bool TaudioParser::checkOutputFormat(AVCodecID codecId)
{
    TsampleFormat fmt = TsampleFormat((audioParserData.sample_format != 0) ? audioParserData.sample_format : TsampleFormat::SF_PCM16,
                                      audioParserData.sample_rate, audioParserData.channels);
    fmt.sf = TsampleFormat::getSampleFormat(codecId);
    fmt.alternateSF = -1;
    audioParserData.alternateSampleFormat = -1;

    DPRINTF(_l("TaudioParser::checkOutputFormat for codec %s with sample format %ld"), getCodecName(codecId), fmt.sf);

    IPin *outConnectedPin = NULL;
    CTransformOutputPin *m_pOutput = deciA->getOutputPin();
    if (m_pOutput == NULL) {
        return true;
    }
    m_pOutput->ConnectedTo(&outConnectedPin);
    if (outConnectedPin == NULL) {
        return true;
    }

    CMediaType mt;
    if (spdif_codec(codecId)) {
        mt = TsampleFormat::createMediaTypeSPDIF(audioParserData.sample_rate);
    } else {
        /* If PCM (uncompressed) format in output, then process it through the FFDShow filters :
         because the filters will eventually modify it (mixer and resample for example) */
        if (!spdif_codec(codecId) && ! bitstream_codec(codecId)) {
            deciA->getOutsf(fmt);
        }

        if (deci->getParam2(IDFF_aoutUseIEC61937)) {
            mt = fmt.toCMediaTypeHD();
        } else {
            mt = fmt.toCMediaType();
        }
    }

    TsampleFormat::DPRINTMediaTypeInfo(mt);

    if (!bitstream_codec(codecId)) {
        DPRINTF(_l("TaudioParser::checkOutputFormat Accept the format without testing it"));
        return true;
    }


    HRESULT hr = S_OK;
    int deviceId = deci->getParam2(IDFF_aoutpassthroughDeviceId);
    if (deviceId <= 1 || !bitstream_codec(codecId)) { // Use the standard media types if set to default or standard
        hr = outConnectedPin->QueryAccept(&mt);
        if (hr == S_OK) {
            FreeMediaType(mt);
            DPRINTF(_l("TaudioParser::checkOutputFormat accepted for codec %s with sample format %ld"), getCodecName(codecId), fmt.sf);
            return true;
        } else {
            DPRINTF(_l("TaudioParser::checkOutputFormat refused for codec %s with sample format %ld"), getCodecName(codecId), fmt.sf);
        }
    }

    if (mt.formattype != FORMAT_WaveFormatEx || mt.pbFormat == NULL) {
        FreeMediaType(mt);
        return false;
    }
    WAVEFORMATEX *wf = (WAVEFORMATEX*)mt.pbFormat;
    if (wf->wFormatTag != WAVE_FORMAT_EXTENSIBLE) {
        FreeMediaType(mt);
        return false;
    }

    WAVEFORMATEXTENSIBLE *wfex = (WAVEFORMATEXTENSIBLE*)mt.pbFormat;
    if (wfex == NULL) {
        return false;
    }

    hr = VFW_E_NO_ACCEPTABLE_TYPES;

    GUID originalSubtype = wfex->SubFormat;
    for (int i = 0; TsampleFormat::alternateSampleFormats[i].originalSubType != GUID_NULL; i++) {
        if (TsampleFormat::alternateSampleFormats[i].originalSubType != originalSubtype) {
            continue;
        }
        // deviceId : 0 (try all media types), 1 (standard media types), 2 (xonar media types)
        if (deviceId != 0 && deviceId != TsampleFormat::alternateSampleFormats[i].alternateFormatId) {
            continue;
        }
        FreeMediaType(mt);
        fmt.alternateSF = i;
        if (deci->getParam2(IDFF_aoutUseIEC61937)) {
            mt = fmt.toCMediaTypeHD();
        } else {
            mt = fmt.toCMediaType();
        }
        if (deviceId == 0) {
            DPRINTF(_l("TaudioParser::getSupportedFormat format not supported, trying another one"));
            hr = outConnectedPin->QueryAccept(&mt);
        } else {
            DPRINTF(_l("TaudioParser::getSupportedFormat We are in compatibility mode. The following format will be used without checking if it is supported"));
            hr = S_OK;
        }

        TsampleFormat::DPRINTMediaTypeInfo(mt);

        if (hr == S_OK) {
            // Store the new format for filters negociation
            audioParserData.alternateSampleFormat = i;
            break;
        }
    }
    FreeMediaType(mt);

    if (hr != S_OK) {
        DPRINTF(_l("TaudioParser::getSupportedFormat no compatible format supported"));
        return false;
    }
    DPRINTF(_l("TaudioParser::getSupportedFormat this format is supported"));
    return true;
}

void TaudioParser::initConfig(void)
{
    // Apply Dobly True-HD passthrough if decoding if checkbox is selected
    useTrueHDPassthrough = usableTrueHDPassthrough && deci->getParam2(IDFF_aoutpassthroughTRUEHD) == 1;
    // Apply DTS-HD passthrough if checkbox is selected
    useDTSHDPassthrough = usableDTSHDPassthrough && (deci->getParam2(IDFF_aoutpassthroughDTSHD) == 1);
    // Apply EAC3 passthrough if checkbox is selected
    useEAC3Passthrough = usableEAC3Passthrough && (deci->getParam2(IDFF_aoutpassthroughEAC3) == 1);

    // Use AC3 core from TrueHD only if : an AC3 decoder is enabled & TrueHD decoder is disabled &
    // TrueHD passthrough is disabled
    AVCodecID codecIDAC3 = globalSettings->getCodecId(WAVE_FORMAT_AC3_W, NULL);

    useAC3CoreOnly = (codecIDAC3 != AV_CODEC_ID_NONE)
                     && (globalSettings->getCodecId(WAVE_FORMAT_TRUEHD, NULL) == AV_CODEC_ID_NONE)
                     &&  !useTrueHDPassthrough;

    useAC3Passthrough = usableAC3Passthrough && (codecIDAC3 != AV_CODEC_ID_NONE) && (deci->getParam2(IDFF_aoutpassthroughAC3) == 1);

    AVCodecID codecIDDTS = globalSettings->getCodecId(WAVE_FORMAT_DTS_W, NULL);
    useDTSPassthrough = usableDTSPassthrough && (codecIDDTS != AV_CODEC_ID_NONE) && (deci->getParam2(IDFF_aoutpassthroughDTS) == 1);
}

AVCodecID TaudioParser::getCodecIdFromStream()
{
    initConfig();

    REFERENCE_TIME m_tStart = deciA->GetCurrentPin()->CurrentStartTime();
    // Don't allow more than 1 format change and only if the last seek occurred during the last 3 seconds
    //if (audioParserData.nbFormatChanges>=4 || (m_tStart / 10000 / 1000 > 3))
    //return codecId;

    audioParserData.wFormatTag = 0;

    switch (streamformat) {
        case REGULAR_AC3:
            if (useAC3Passthrough) {
                if (codecId == CODEC_ID_SPDIF_AC3) {
                    return codecId;
                }
                audioParserData.nbFormatChanges++;
                codecId = CODEC_ID_SPDIF_AC3;
                if (checkOutputFormat(codecId)) {
                    return codecId;
                } else {
                    useAC3Passthrough = false;
                }
            }
            //DPRINTF(_l("TaudioParser::getCodecIdFromStream change to AC3"));
            audioParserData.wFormatTag = WAVE_FORMAT_AC3_W;
            break;
        case EAC3:
            if (useEAC3Passthrough) {
                if (codecId == CODEC_ID_BITSTREAM_EAC3) {
                    return codecId;
                }
                audioParserData.nbFormatChanges++;
                codecId = CODEC_ID_BITSTREAM_EAC3;
                if (checkOutputFormat(codecId)) {
                    return codecId;
                } else {
                    usableEAC3Passthrough = false;
                }
            }
            audioParserData.wFormatTag = WAVE_FORMAT_EAC3;
            // TODO : if EAC3 decoder disabled, find a compatible EAC3 decoder and pull FFDShow Audio out of the graph
            break;
        case MLP:
            if (useTrueHDPassthrough) {
                if (codecId == CODEC_ID_BITSTREAM_TRUEHD) {
                    return codecId;
                }
                audioParserData.nbFormatChanges++;
                codecId = CODEC_ID_BITSTREAM_TRUEHD;
                if (checkOutputFormat(codecId)) {
                    return codecId;
                } else {
                    usableTrueHDPassthrough = false;
                }
            }
            audioParserData.wFormatTag = WAVE_FORMAT_MLP;
            break;
        case TRUEHD:
            if (useTrueHDPassthrough) {
                if (codecId == CODEC_ID_BITSTREAM_TRUEHD) {
                    return codecId;
                }
                audioParserData.nbFormatChanges++;
                codecId = CODEC_ID_BITSTREAM_TRUEHD;
                if (checkOutputFormat(codecId)) {
                    return codecId;
                } else {
                    usableTrueHDPassthrough = false;
                }
            }
            audioParserData.wFormatTag = WAVE_FORMAT_TRUEHD;
            // TODO : if MLP decoder disabled, find a compatible MLP decoder and pull FFDShow Audio out of the graph
            // Problem : no MLP mediaguid exist
            break;
        case AC3_TRUEHD:
            if (useTrueHDPassthrough) {
                if (codecId == CODEC_ID_BITSTREAM_TRUEHD) {
                    return codecId;
                }
                audioParserData.nbFormatChanges++;
                codecId = CODEC_ID_BITSTREAM_TRUEHD;
                // Check if output is compatible
                if (checkOutputFormat(codecId)) {
                    return codecId;
                } else {
                    usableTrueHDPassthrough = false;
                }
            }
            // If AC3 codec is set to SPDIF and MLP decoder disabled and TRUEHD passthrough is disabled,
            // then send AC3 frams in passthrough and throw away TrueHD frames
            if (useAC3CoreOnly) {
                if (useAC3Passthrough) {
                    if (codecId == CODEC_ID_SPDIF_AC3) {
                        return codecId;
                    }
                    audioParserData.nbFormatChanges++;
                    codecId = CODEC_ID_SPDIF_AC3;
                    if (checkOutputFormat(codecId)) {
                        return codecId;
                    } else {
                        usableAC3Passthrough = false;
                    }
                    codecId = AV_CODEC_ID_AC3;
                    audioParserData.nbFormatChanges++;
                    return codecId;
                } else { //MLP decoder is disabled and AC3 pass-through is disabled
                    // TODO : find a compatible MLP decoder and pull FFDShow Audio out of the graph
                    // Problem : no MLP mediaguid exist
                    audioParserData.wFormatTag = WAVE_FORMAT_AC3_W;
                }
            } else {
                audioParserData.wFormatTag = WAVE_FORMAT_TRUEHD;
            }
            break;
        case DTS_HD:
            if (useDTSHDPassthrough) {
                if (codecId == CODEC_ID_BITSTREAM_DTSHD) {
                    return codecId;
                }
                audioParserData.nbFormatChanges++;
                codecId = CODEC_ID_BITSTREAM_DTSHD;
                if (checkOutputFormat(codecId)) {
                    return codecId;
                } else {
                    usableDTSHDPassthrough = false;
                }
            }
            // Else jump to case DTS :
        case DTS:
            // If DTS Pass-through is enabled, then send DTS frames (or DTS core frames for DTS-HD stream)
            // in passthrough (DTS-HD frames are thrown away for DTS-HD stream)
            if (useDTSPassthrough) {
                if (codecId == CODEC_ID_SPDIF_DTS) {
                    return codecId;
                }
                DPRINTF(_l("TaudioParser::getCodecIdFromStream DTS stream detected and DTS passthrough selected"));
                audioParserData.nbFormatChanges++;
                codecId = CODEC_ID_SPDIF_DTS;
                if (checkOutputFormat(codecId)) {
                    return codecId;
                } else {
                    usableDTSPassthrough = false;
                }
                DPRINTF(_l("TaudioParser::getCodecIdFromStream DTS passthrough not usable"));
            }
            audioParserData.wFormatTag = WAVE_FORMAT_DTS_W;
            break;
        case UNDEFINED:
            return AV_CODEC_ID_NONE;
        default:
            break;
    }
    codecId = globalSettings->getCodecId(audioParserData.wFormatTag, NULL);
    //DPRINTF(_l("TaudioParser::getCodecIdFromStream %s"), getCodecName(codecId));
    return codecId;
}

void TaudioParser::NewSegment(void)
{
    DPRINTF(_l("TaudioParser::NewSegment"));
    // A new segment has arrived (occurs when there is some skipping or a change of stream) :
    // Solution 1 : reset the parser to detect the stream format again. This could result in codec switch
    init();

    // Solution 2: reset only the context
    /*audioParserData.channels=0;audioParserData.bit_rate=0;audioParserData.sample_rate=0;audioParserData.nbFormatChanges=0;
    audioParserData.sample_format=0;audioParserData.frames.clear();
    includeBytes=0;skipBytes=0;firstFrame=true;*/
    SearchSync();
}

HRESULT TaudioParser::parseDTS(unsigned char *src, int size, TbyteBuffer *newsrcBuffer)
{

    TbyteBuffer tmpBuffer;

    // For DTS streams, DTS-HD blocks must be removed, libavcodec does not know how to handle them yet
    Tbitdata bitdata;
    bitdata.bitsleft = size * 8;
    bitdata.wordpointer = src;
    // Include the remaining bytes from the previous frame
    if (!backupbuf.empty()) {
        tmpBuffer.clear();
        tmpBuffer.append(&*backupbuf.begin(), backupbuf.size());
        tmpBuffer.append(src, size);
        bitdata.bitsleft = (long)tmpBuffer.size() * 8;
        bitdata.wordpointer = &*tmpBuffer.begin();
        backupbuf.clear();
    }

    if (includeBytes > 0) {
        if (bitdata.bitsleft < includeBytes * 8) {
            includeBytes -= bitdata.bitsleft / 8;
            newsrcBuffer->append(bitdata.wordpointer, bitdata.bitsleft / 8);
            return S_OK;
        }
        newsrcBuffer->append(bitdata.wordpointer, includeBytes);
        bitdata.bitsleft -= includeBytes * 8;
        bitdata.wordpointer += includeBytes;
        includeBytes = 0;
    }

    if (skipBytes > 0) {
        if (bitdata.bitsleft < skipBytes * 8) {
            skipBytes -= bitdata.bitsleft / 8;
            return S_OK;
        }
        bitdata.bitsleft -= skipBytes * 8;
        bitdata.wordpointer += skipBytes;
        skipBytes = 0;
    }

    while (bitdata.bitsleft > 64) { // 64 : necessary size to have the DTS block header
        if (bitdata.showBits(32) == 0x64582025) { //DTS-HD block
            if (streamformat != DTS_HD) {
                streamformat = DTS_HD;
                // Ooops, we have written the DTS frame in the source buffer whereas for bitstream we need a complete
                // couple of DTS+DTSHD frame
                if (useDTSHDPassthrough && newsrcBuffer->size() > 0) {
                    backupbuf.append(&*newsrcBuffer->begin(), newsrcBuffer->size());
                    newsrcBuffer->clear();
                    audioParserData.frames.clear();
                }
            }

            // Save the start position and left length of the DTS HD block
            unsigned char *backuppointer = bitdata.wordpointer;
            int backupBitsLeft = bitdata.bitsleft;

            bitdata.getBits(32);

            uint32_t dummy = bitdata.getBits(8);
            bitdata.getBits(2);
            bool blownupHeader = bitdata.getBits(1) == 1;
            frame_size = 0;
            if (blownupHeader) {
                dummy = bitdata.getBits(12) + 1; // header size
                frame_size = bitdata.getBits(20) + 1; // full dts-hd block size
            } else {
                dummy = bitdata.getBits(8) + 1; // header size
                frame_size = bitdata.getBits(16) + 1; // full dts-hd block size
            }

            bitdata.wordpointer = backuppointer;
            bitdata.bitsleft = backupBitsLeft;
            bitdata.bitindex = 0;

            if (useDTSHDPassthrough) {
                audioParserData.sample_rate = 96000;
                audioParserData.channels = 8;
                audioParserData.sample_format = TsampleFormat::SF_PCM24;
            }


            if (frame_size > (uint32_t)bitdata.bitsleft / 8) {
                backupbuf.append(bitdata.wordpointer, bitdata.bitsleft / 8);
                bitdata.bitsleft = 0;
            } else {
                if (useDTSHDPassthrough) { // DTS-HD will be taken only if in passthrough mode (no decoder yet)
                    if (backupbuf.size() != 0) {
                        newsrcBuffer->append(&*backupbuf.begin(), backupbuf.size());
                    }
                    newsrcBuffer->append(bitdata.wordpointer, frame_size);
                    bitdata.bitsleft -= frame_size * 8;
                    bitdata.wordpointer += frame_size;
                    audioParserData.frames.push_back(TframeData((uint32_t)backupbuf.size() + frame_size));
                    backupbuf.clear();
                } else { // strip off HD blocks (DTS core only)
                    bitdata.bitindex = 0;
                    bitdata.bitsleft = backupBitsLeft - frame_size * 8;
                    bitdata.wordpointer = backuppointer + frame_size;
                }
            }
        } else if ( // DTS block
            /* 14 bits and little endian bitstream */
            (bitdata.showBits(32) == 0xFF1F00E8
             && (bitdata.wordpointer[4] & 0xf0) == 0xf0 && bitdata.wordpointer[5] == 0x07) ||
            /* 14 bits and big endian bitstream */
            (bitdata.showBits(32) == 0x1FFFE800
             && bitdata.wordpointer[4] == 0x07 && (bitdata.wordpointer[5] & 0xf0) == 0xf0) ||
            /* 16 bits and little endian bitstream */
            bitdata.showBits(32) == 0xFE7F0180 ||
            /* 16 bits and big endian bitstream */
            bitdata.showBits(32) == 0x7FFE8001) {
            if (streamformat == UNDEFINED) {
                streamformat = DTS;
            }

            unsigned char *backuppointer = bitdata.wordpointer;
            int backupBitsLeft = bitdata.bitsleft;

            bool bigendianMode = (bitdata.wordpointer[0] == 0x1F || bitdata.wordpointer[0] == 0x7F);
            // word mode for 16 bits stream
            bool wordMode = (bitdata.wordpointer[0] == 0xFE || bitdata.wordpointer[0] == 0x7F);
            bitdata.bigEndian = bigendianMode;
            bitdata.wordMode = wordMode;
            bitdata.align();

            bitdata.getBits2(32); /* Sync code */
            bitdata.getBits2(1); /* Frame type */
            bitdata.getBits2(5); /* Samples deficit */
            int crcpresent = bitdata.getBits2(1); /* CRC present */
            uint32_t sample_blocks = (bitdata.getBits2(7) + 1) / 8; /* sample blocks */
            // update the stream context with DTS core info only if we have a DTS stream or if we have a DTS-HD stream but we are in DTS core mode (HD blocks stripped off)
            if (streamformat == DTS || !useDTSHDPassthrough) {
                audioParserData.sample_blocks = sample_blocks;
            }


            frame_size = bitdata.getBits2(14) + 1;

            int amode = bitdata.getBits2(6);

            if (streamformat == DTS || !useDTSHDPassthrough) {
                audioParserData.sample_rate = dca_sample_rates[bitdata.getBits2(4)];
                audioParserData.bit_rate = dca_bit_rates[bitdata.getBits2(5)];
            } else {
                bitdata.getBits2(4);
                bitdata.getBits2(5);
            }


            bitdata.getBits2(10);
            int lfe = !!bitdata.getBits2(2);
            bitdata.getBits2(1);

            uint32_t header_crc = 0;
            if (crcpresent) {
                header_crc = bitdata.getBits2(16);
            }

            bitdata.getBits2(16);
            int output = amode;
            if (lfe) {
                output |= 0x80;    //DCA LFE
            }

            int subframes = bitdata.getBits2(4) + 1;
            int primchannels = bitdata.getBits2(3) + 1;
            int ffmpegchannels = primchannels;
            if (ffmpegchannels > 5) {
                ffmpegchannels = 5;
            }

            int channels = primchannels + lfe;

            if (streamformat == DTS || !useDTSHDPassthrough) {
                audioParserData.channels = channels;
            }

            int datasize = (sample_blocks / 8) * 256 * sizeof(int16_t) * channels;

            bitdata.wordpointer = backuppointer;
            bitdata.bitsleft = backupBitsLeft;
            bitdata.bitindex = 0;

            if (!wordMode) {
                frame_size = frame_size * 8 / 14 * 2;
            }

            // DTS-HD bistream
            if (streamformat == DTS_HD && useDTSHDPassthrough) {
                if (frame_size > (uint32_t)bitdata.bitsleft / 8) {
                    backupbuf.append(bitdata.wordpointer, bitdata.bitsleft / 8);
                    bitdata.bitsleft = 0;
                } else { // store the block into the source buffer
                    backupbuf.append(bitdata.wordpointer, frame_size);
                    bitdata.bitsleft -= frame_size * 8;
                    bitdata.wordpointer += frame_size;
                }
            } // DTS or DTS-HD in core mode
            // DTS frame not complete in this buffer. Backup it for next pass
            else if (frame_size > (uint32_t)bitdata.bitsleft / 8) {
                backupbuf.append(bitdata.wordpointer, bitdata.bitsleft / 8);
                bitdata.bitsleft = 0;
            } else if (firstFrame) { // Frame complete and store first frame in case it is an DTSHD stream (DTS frame---DTSHD frame)
                firstFrame = false;
                backupbuf.clear();
                backupbuf.append(bitdata.wordpointer, frame_size);
                bitdata.bitsleft -= frame_size * 8;
                bitdata.wordpointer += frame_size;
            } else { // store the block into the source buffer
                newsrcBuffer->append(bitdata.wordpointer, frame_size);
                bitdata.bitsleft -= frame_size * 8;
                bitdata.wordpointer += frame_size;
                audioParserData.frames.push_back(TframeData(frame_size));
            }
        } else { // Ignore the byte
            bitdata.bitsleft -= 8;
            bitdata.wordpointer++;
        }
    }

    // Copy remaining bytes into a backup buffer for next frame
    if (bitdata.bitsleft > 0) {
        backupbuf.clear();
        backupbuf.append(bitdata.wordpointer, bitdata.bitsleft / 8);
    }
    return S_OK;
}

HRESULT TaudioParser::parseAC3(unsigned char *src, int size, TbyteBuffer *newsrcBuffer)
{

    TbyteBuffer tmpBuffer;
    Tbitdata bitdata;
    bitdata.bitindex = 0;
    bitdata.bitsleft = size * 8;
    bitdata.wordpointer = src;

    // Include the remaining bytes from the previous frame
    if (!backupbuf.empty()) {
        tmpBuffer.clear();
        tmpBuffer.append(&*backupbuf.begin(), backupbuf.size());
        tmpBuffer.append(src, size);
        bitdata.bitsleft = (long)tmpBuffer.size() * 8;
        bitdata.wordpointer = &*tmpBuffer.begin();
        backupbuf.clear();
    }

    if (includeBytes > 0) {
        if (bitdata.bitsleft < includeBytes * 8) {
            includeBytes -= bitdata.bitsleft / 8;
            newsrcBuffer->append(bitdata.wordpointer, bitdata.bitsleft / 8);
            return S_OK;
        }
        newsrcBuffer->append(bitdata.wordpointer, includeBytes);
        bitdata.bitsleft -= includeBytes * 8;
        bitdata.wordpointer += includeBytes;
        includeBytes = 0;
    }

    if (skipBytes > 0) {
        if (bitdata.bitsleft < skipBytes * 8) {
            skipBytes -= bitdata.bitsleft / 8;
            return S_OK;
        }
        bitdata.bitsleft -= skipBytes * 8;
        bitdata.wordpointer += skipBytes;
        skipBytes = 0;
    }

    while (bitdata.bitsleft > 128) { // 64 : necessary size to have the AC3 block header
        if (bitdata.showBits(16) == 0x0B77) { // AC3 stream
            //searchSync=false;
            unsigned char *backuppointer = bitdata.wordpointer;
            int backupBitsLeft = bitdata.bitsleft;
            frame_size = 0;
            bool isEAC3 = false;

            bitdata.getBits(16);
            uint32_t bitstream_id = bitdata.showBits(29) & 0x1F;
            if (bitstream_id <= 10) { // AC3
                if (streamformat == UNDEFINED) {
                    streamformat = REGULAR_AC3;
                }

                bitdata.getBits(16); // CRC
                int sr_code = bitdata.getBits(2); // Sample rate code
                int frame_size_code = bitdata.getBits(6); // Frame size code
                if (frame_size_code > 37) { // Wrong size
                    // Restore pointer to sync word position
                    bitdata.wordpointer = backuppointer;
                    bitdata.bitsleft = backupBitsLeft;
                    bitdata.bitindex = 0;
                    bitdata.getBits(8); // Jump to next byte
                    continue;
                }
                frame_size = ff_ac3_frame_size_tab[frame_size_code][sr_code] * 2;
                bitdata.getBits(8); // Skip bsid and bitstream mode
                int channel_mode = bitdata.getBits(3);
                if (channel_mode == 2) { // stereo
                    bitdata.getBits(2);    // Skip dsurmod
                } else {
                    if ((channel_mode & 1) && channel_mode != 1) { // Not mono
                        bitdata.getBits(2);    // Center mix level
                    }
                    if (channel_mode & 4) {
                        bitdata.getBits(2);    // Surround mix levels
                    }
                }
                int lfe = bitdata.getBits(1);
                int sr_shift = bitstream_id - 8;
                if (sr_shift < 0) {
                    sr_shift = 0;
                }

                if (streamformat == REGULAR_AC3 || useAC3CoreOnly) {
                    audioParserData.sample_rate = ff_ac3_sample_rate_tab[sr_code] >> sr_shift;
                    audioParserData.bit_rate = (ff_ac3_bitrate_tab[frame_size_code >> 1] * 1000) >> sr_shift;
                    audioParserData.channels = ff_ac3_channels_tab[channel_mode] + lfe;
                }
            } else { // EAC3
                isEAC3 = true;
                if (streamformat == UNDEFINED || streamformat == REGULAR_AC3) {
                    streamformat = EAC3;
                    // Ooops, we have written the AC3 frame in the source buffer whereas for bitstream we need a complete
                    // couple of AC3+EAC3 frame
                    if (useEAC3Passthrough && newsrcBuffer->size() > 0) {
                        backupbuf.append(&*newsrcBuffer->begin(), newsrcBuffer->size());
                        newsrcBuffer->clear();
                        audioParserData.frames.clear();
                    }
                }

                bitdata.getBits(2); // Frame type
                bitdata.getBits(3); // Substream id
                frame_size = (bitdata.getBits(11) + 1) << 1;
                int sr_code = bitdata.getBits(2); // Sample rate code
                audioParserData.sample_rate = 0;
                int num_blocks = 6;
                int sr_shift = 0;
                if (sr_code == 3) {
                    int sr_code2 = bitdata.getBits(2);
                    audioParserData.sample_rate = ff_ac3_sample_rate_tab[sr_code2] / 2;
                    sr_shift = 1;
                } else {
                    num_blocks = eac3_blocks[bitdata.getBits(2)];
                    audioParserData.sample_rate = ff_ac3_sample_rate_tab[sr_code];
                }

                int channel_mode = bitdata.getBits(3);
                int lfe = bitdata.getBits(1);
                audioParserData.channels = ff_ac3_channels_tab[channel_mode] + lfe;

                if (streamformat == EAC3) {
                    audioParserData.bit_rate = (uint32_t)(8.0 * frame_size * audioParserData.sample_rate /
                                                          (num_blocks * 256.0));
                }

            }

            // Restore pointer to sync word position
            bitdata.wordpointer = backuppointer;
            bitdata.bitsleft = backupBitsLeft;
            bitdata.bitindex = 0;

            // If AC3 codec is SPDIF or MLP decoder disabled, we keep AC3/EAC3 frames otherwise (MLP) we throw them avay
            if (useAC3CoreOnly || streamformat != AC3_TRUEHD) {
                // AC3 frame not complete in this buffer.
                // Back it up for next pass
                if (frame_size > (uint32_t)bitdata.bitsleft / 8) {
                    backupbuf.clear();
                    backupbuf.append(bitdata.wordpointer, bitdata.bitsleft / 8);
                    bitdata.bitsleft = 0;
                } else if (firstFrame && streamformat != EAC3) { // Frame complete and store first frame in case it is an TrueHD stream (AC3 frame---TrueHD frame)
                    firstFrame = false;
                    backupbuf.clear();
                    backupbuf.append(bitdata.wordpointer, frame_size);
                    bitdata.bitsleft -= frame_size * 8;
                    bitdata.wordpointer += frame_size;
                } else if (streamformat == EAC3 && useEAC3Passthrough) {
                    if (isEAC3) { // This is an EAC3 frame that follows an AC3 frame (backed up) so store the AC3+EAC3 frame in the source buffer
                        if (backupbuf.size() != 0) {
                            newsrcBuffer->append(&*backupbuf.begin(), backupbuf.size());
                        }
                        newsrcBuffer->append(bitdata.wordpointer, frame_size);
                        bitdata.bitsleft -= frame_size * 8;
                        bitdata.wordpointer += frame_size;
                        audioParserData.frames.push_back(TframeData((uint32_t)backupbuf.size() + frame_size));
                        backupbuf.clear();
                    } else { // This is an AC3 frame, back it up because they have to be sent with an EAC3 frame
                        backupbuf.clear();
                        backupbuf.append(bitdata.wordpointer, frame_size);
                        bitdata.bitsleft -= frame_size * 8;
                        bitdata.wordpointer += frame_size;
                    }
                } else { // store the AC3 block into the source buffer
                    // Recover the first frame we backed up (to check if the stream was different from regular AC3
                    if (backupbuf.size() > 0) {
                        newsrcBuffer->append(&*backupbuf.begin(), backupbuf.size());
                        audioParserData.frames.push_back(TframeData((uint32_t)backupbuf.size()));
                        backupbuf.clear();
                    }

                    newsrcBuffer->append(bitdata.wordpointer, frame_size);
                    bitdata.bitsleft -= frame_size * 8;
                    bitdata.wordpointer += frame_size;
                    audioParserData.frames.push_back(TframeData(frame_size));
                }
            } else { // MLP parser does not know how to handle AC3 frames interweaved
                bitdata.bitindex = 0;
                bitdata.bitsleft = backupBitsLeft - frame_size * 8;
                if (bitdata.bitsleft < 0) {
                    skipBytes = -bitdata.bitsleft / 8;
                    bitdata.bitsleft = 0;
                    //bitdata.wordpointer+=skipStartBitsLeft/8; // EOF
                } else {
                    bitdata.wordpointer = backuppointer + frame_size;
                }
            }
        } else if (bitdata.showBits(32, 32) == 0xf8726fba || // True HD major sync frame
                   bitdata.showBits(32, 32) == 0xf8726fbb) { // MLP
            hasMLPFrames = true;

            uint32_t frame_size1 = bitdata.showBits(32, 0);
            WORD frame_time = (WORD)frame_size1;
            //uint32_t frame_size = (((bitdata.wordpointer[0] << 8) | bitdata.wordpointer[1]) & 0xfff) *2;
            frame_size = ((frame_size1 >> 16) & 0xfff) * 2;

            // Oops... current decoder is AC3 whereas we have MLP/TrueHD data =>
            // and AC3 codec is not set to SPDIF and MLP decoder is enabled. So drop AC3 frames and keep MLP/TrueHD only
            // Also if searchSync is true we throw away the first buffers
            if (searchSync || ((streamformat == REGULAR_AC3 || streamformat == EAC3)
                               && !useAC3CoreOnly)) {
                newsrcBuffer->clear();
                audioParserData.frames.clear();
            }
            searchSync = false;


            // Save the start position and left length of the MLP/TrueHD block
            unsigned char *backuppointer = bitdata.wordpointer;
            int backupBitsLeft = bitdata.bitsleft;

            bitdata.getBits(32); // Jump frame size (16) and ignored bits (16)

            // Identify the stream : if AC3 frames found (REGULAR_AC3 or EAC3) and we are
            // in a MLP frame, then this is an AC3_TRUEHD stream.
            // Otherwise this is either a TRUEHD or MLP stream (basing on the header sync)
            if (streamformat != AC3_TRUEHD) {
                if (streamformat == REGULAR_AC3 || streamformat == EAC3) {
                    streamformat = AC3_TRUEHD;
                } else {
                    bitdata.wordpointer[3] == 0xba ? streamformat = TRUEHD : streamformat = MLP;
                }
            }

            uint32_t ratebits = 0;

            bool isTrueHD = false;
            uint32_t group1_samplerate = 0, group2_samplerate = 0;
            if (bitdata.wordpointer[3] == 0xbb) { // MLP (0xbb)
                bitdata.getBits(32);
                int group1_bits = bitdata.getBits(4); // group1 bits
                if (!useAC3CoreOnly) { // Don't update stream format with MLP config if we are in AC3 mode
                    switch (group1_bits) {
                        case 8:
                            audioParserData.sample_format = TsampleFormat::SF_PCM8;
                            break;
                        case 0:
                        case 16:
                        default:
                            audioParserData.sample_format = TsampleFormat::SF_PCM16;
                            break;
                        case 20:
                            audioParserData.sample_format = TsampleFormat::SF_LPCM20;
                            break;
                        case 24:
                            audioParserData.sample_format = TsampleFormat::SF_PCM24;
                            break;
                        case 32:
                            audioParserData.sample_format = TsampleFormat::SF_PCM32;
                            break;
                    }
                }
                int group2_bits = bitdata.getBits(4); // group2 bits
                ratebits = bitdata.getBits(4);
                group1_samplerate = (ratebits == 0xF) ? 0 : (ratebits & 8 ? 44100 : 48000) << (ratebits & 7);
                if (!useAC3CoreOnly) { // Don't update stream format with MLP config if we are in AC3 mode
                    audioParserData.sample_rate = group1_samplerate;
                }

                uint32_t ratebits2 = bitdata.getBits(4);
                group2_samplerate = (ratebits2 == 0xF) ? 0 : (ratebits2 & 8 ? 44100 : 48000) << (ratebits2 & 7);
                bitdata.getBits(11); // Skip
                int channels_mlp = bitdata.getBits(5);
                if (!useAC3CoreOnly) {
                    audioParserData.channels = mlp_channels[channels_mlp];
                }
            } else { // Dolby TrueHD (0xba)
                isTrueHD = true;
                bitdata.getBits(32);
                int group1_bits = 24; // group1 bits
                audioParserData.sample_format = TsampleFormat::SF_PCM24;
                int group2_bits = 0; // group2 bits
                ratebits = bitdata.getBits(4);
                group1_samplerate = (ratebits == 0xF) ? 0 : (ratebits & 8 ? 44100 : 48000) << (ratebits & 7);
                if (!useAC3CoreOnly) { // Don't update stream format with MLP config if we are in AC3 mode
                    audioParserData.sample_rate = group1_samplerate;
                }

                group2_samplerate = 0;
                bitdata.getBits(8); // Skip
                int channels_thd_stream1 = bitdata.getBits(5);
                bitdata.getBits(2); // Skip
                int channels_thd_stream2 = bitdata.getBits(13);

                if (!useAC3CoreOnly) { // Don't update stream format with MLP config if we are in AC3 mode
                    if (channels_thd_stream2) {
                        audioParserData.channels = truehd_channels(channels_thd_stream2);
                    } else {
                        audioParserData.channels = truehd_channels(channels_thd_stream1);
                    }
                }
            }

            bitdata.getBits(48); // Skip

            audioParserData.ratebits = ratebits;

            int isVbr = bitdata.getBits(1);
            uint32_t peak_bitrate = (bitdata.getBits(15) * group1_samplerate + 8) >> 4;

            if (!useAC3CoreOnly) { // Don't update stream format with MLP config if we are in AC3 mode
                audioParserData.bit_rate = peak_bitrate;
            }

            int num_substreams = bitdata.getBits(4);


            // Restore pointer to (sync word - 4 bytes) position
            bitdata.bitindex = 0;
            bitdata.bitsleft = backupBitsLeft;
            bitdata.wordpointer = backuppointer;

            // If codecId is AC3, strip off the TrueHD blocks (AC3 Core)
            if (streamformat == AC3_TRUEHD && useAC3CoreOnly) {
                bitdata.bitsleft -= frame_size * 8;
                if (bitdata.bitsleft < 0) {
                    skipBytes = -bitdata.bitsleft / 8;
                    bitdata.bitsleft = 0;
                    //bitdata.wordpointer+=skipStartBitsLeft/8; // EOF
                } else {
                    bitdata.wordpointer += frame_size;
                }
            } else {
                // Behaviour : feed frame by frame (one major frame or one non major frame)
                // MLP frame not complete in this buffer.
                // Back it up until having complete frame
                if (frame_size > (uint32_t)bitdata.bitsleft / 8) {
                    backupbuf.append(bitdata.wordpointer, bitdata.bitsleft / 8);
                    bitdata.bitsleft = 0;
                } else { // store the block into the source buffer
                    newsrcBuffer->append(bitdata.wordpointer, frame_size);
                    bitdata.bitsleft -= frame_size * 8;
                    bitdata.wordpointer += frame_size;

                    if (searchSync) {
                        continue;
                    }

                    TframeData frameData = TframeData(frame_size);

                    uint32_t rate = 64 >> (audioParserData.ratebits & 7); // Used to calculate the number of zeros
                    if (audioParserData.isFirst) {
                        audioParserData.isFirst = false;
                    } else {
                        frameData.space_size = ((frame_time - audioParserData.lastFrameTime) & 0xFF) * rate;
                    }

                    // Store the frame time for later
                    audioParserData.lastFrameTime = frame_time;

                    audioParserData.frames.push_back(frameData);
                }
            }
        } else { // If just AC3, skip byte, or if has MLP frams this is a non major sync frame
            // searchSync flag is enabled at the initialization and when doing a skip in the file
            // In those cases we should look after a sync frame first
            if (hasMLPFrames && !searchSync) {
                uint32_t frame_size1 = bitdata.showBits(32, 0);
                //uint32_t frame_size = (((bitdata.wordpointer[0] << 8) | bitdata.wordpointer[1]) & 0xfff) *2;
                WORD frame_time = (WORD)frame_size1;
                frame_size = ((frame_size1 >> 16) & 0xfff) * 2;


                if (frame_size < 3) {
                    bitdata.bitsleft -= 8;
                    bitdata.wordpointer++;
                }

                // If the stream contains AC3 frames and AC3 is set to SPDIF throw avay the MLP/TrueHD block
                if (streamformat == AC3_TRUEHD && useAC3CoreOnly) {
                    bitdata.bitsleft -= frame_size * 8;
                    if (bitdata.bitsleft < 0) {
                        skipBytes = -bitdata.bitsleft / 8;
                        bitdata.bitsleft = 0;
                        //bitdata.wordpointer+=skipStartBitsLeft/8; // EOF
                    } else {
                        bitdata.wordpointer += frame_size;
                    }
                } else {
                    // MLP/TrueHD frame not complete in this buffer.
                    // Back it up until having complete frame
                    if (frame_size > (uint32_t)bitdata.bitsleft / 8) {
                        backupbuf.append(bitdata.wordpointer, bitdata.bitsleft / 8);
                        bitdata.bitsleft = 0;
                    } else { // store the block into the source buffer
                        // Additional check : in case we are in a midle of a frame, check that there is
                        // no AC3 or major MLP frame inside this supposed non major frame
                        // In that case we were wrong : this is not a non major frame,
                        // so we throw the bytes between
                        unsigned char *ptr = bitdata.wordpointer;
                        bool skip = false;
                        while (ptr < bitdata.wordpointer + frame_size) {
                            if (bitdata.showBits(16) == 0x0B77 || // AC3 stream
                                    bitdata.showBits(32, 32) == 0xf8726fba || // True HD major sync frame
                                    bitdata.showBits(32, 32) == 0xf8726fbb) { // MLP major sync frame
                                bitdata.bitsleft -= (long)(ptr - bitdata.wordpointer) * 8;
                                bitdata.wordpointer += (ptr - bitdata.wordpointer);
                                skip = true;
                                break;
                            }
                            ptr++;
                        }
                        if (skip) {
                            backupbuf.clear();
                            continue;
                        }

                        /* Behaviour : feed frame by frame ((non)major by (non)major frame)*/
                        newsrcBuffer->append(bitdata.wordpointer, frame_size);
                        bitdata.bitsleft -= frame_size * 8;
                        bitdata.wordpointer += frame_size;
                        TframeData frameData = TframeData(frame_size);

                        uint32_t rate = 64 >> (audioParserData.ratebits & 7); // Used to calculate the number of zeros
                        if (audioParserData.isFirst) {
                            audioParserData.isFirst = false;
                        } else {
                            frameData.space_size = ((frame_time - audioParserData.lastFrameTime) & 0xFF) * rate;
                        }

                        // Store the frame time for later
                        audioParserData.lastFrameTime = frame_time;

                        audioParserData.frames.push_back(frameData);
                    }
                }
            } else { // This is neither AC3/major MLP/non-major MLP frame so we throw away the byte
                bitdata.bitsleft -= 8;
                bitdata.wordpointer++;
            }
        }
    }

    // Copy remaining bytes into a backup buffer for next frame
    if (bitdata.bitsleft > 0) {
        //newsrcBuffer->append(bitdata.wordpointer, bitdata.bitsleft/8);
        backupbuf.clear();
        backupbuf.append(bitdata.wordpointer, bitdata.bitsleft / 8);
    }
    return S_OK;
}

static int truehd_channels(int chanmap)
{
    int channels = 0, i;

    for (i = 0; i < 13; i++) {
        channels += thd_chancount[i] * ((chanmap >> i) & 1);
    }

    return channels;
}
