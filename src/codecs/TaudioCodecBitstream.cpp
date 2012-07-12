/*
 * Copyright (c) 2009
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
#include "TaudioCodecBitstream.h"
#include "IffdshowDec.h"
#include "IffdshowDecAudio.h"
#include "ffdshow_mediaguids.h"
#include "IffdshowBase.h"
#include "streams.h"
#include "TsampleFormat.h"



//0100 0000 0000 0000 FEFE : 10 bytes
WORD additional_DTSHD_start_bytesW[5] = {0x0001, 0x0000, 0x0000, 0x0000, 0xFEFE};
//079E0003 84010101 800056A5 3BF48183 (498077E0) : 20 bytes
WORD additional_MAT_start_bytesW[10] = {0x9E07, 0x0300, 0x0184, 0x0101, 0x0080, 0xA556, 0xF43B, 0x8381, 0x8049, 0xE077};
//C1C3 4942 FA3B 8382 8049 E077 : 12 bytes
WORD additional_MAT_middle_bytesW[6] = {0xC1C3, 0x4942, 0xFA3B, 0x8382, 0x8049, 0xE077};
//C2C3 C4C0 0000 0000 0000 0000 0000 1197 0000 0000 0000 0000 : 24 bytes
WORD additional_MAT_end_bytesW[12] = {0xC2C3, 0xC4C0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1197, 0x0000, 0x0000, 0x0000, 0x0000};

TaudioCodecBitstream::TaudioCodecBitstream(IffdshowBase *deci, IdecAudioSink *Isink):
    Tcodec(deci),
    TaudioCodec(deci, Isink),
    additional_blank_size(0),
    isDTSHDMA(false),
    filledBytes(0),
    fullMATFrameSize(0)
{
    bitstreamBuffer.clear();
}


bool TaudioCodecBitstream::init(const CMediaType &mt)
{
    fmt = TsampleFormat(mt);
    fmt.sf = TsampleFormat::getSampleFormat(codecId);
    DPRINTF(_l("TaudioCodecBitstream::init"));
    return true;
}

TaudioCodecBitstream::~TaudioCodecBitstream()
{
}

void TaudioCodecBitstream::getInputDescr1(char_t *buf, size_t buflen) const
{
    if (codecId == CODEC_ID_BITSTREAM_TRUEHD) {
        ff_strncpy(buf, _l("Dolby TrueHD"), buflen);
    } else if (codecId == CODEC_ID_BITSTREAM_DTSHD) {
        ff_strncpy(buf, _l("DTS-HD"), buflen);
    } else if (codecId == CODEC_ID_BITSTREAM_EAC3) {
        ff_strncpy(buf, _l("Dolby Digital Plus"), buflen);
    } else if (codecId == CODEC_ID_SPDIF_AC3) {
        ff_strncpy(buf, _l("Dolby Digital"), buflen);
    } else if (codecId == CODEC_ID_SPDIF_DTS) {
        ff_strncpy(buf, _l("DTS"), buflen);
    } else {
        ff_strncpy(buf, _l("Compressed format"), buflen);
    }
    buf[buflen - 1] = '\0';
}

template<class sample_t> void TaudioCodecBitstream::swapbe(sample_t *dst, size_t size)
{
    for (size_t i = 0; i < size / sizeof(sample_t); i++) {
        bswap(dst[i]);
    }
}

void TaudioCodecBitstream::fillAdditionalBytes(void)
{
    // Additional header data
    // DTS-HD : add 0001000000000000FEFEA80A after size and before DTS syncword ??
    // TrueHD : add 9E070300018401010080A556F43B83818049E077 before data burst
    if (bitstreamBuffer.empty()) {
        if (codecId == CODEC_ID_BITSTREAM_DTSHD) {
            size_t additional_bytes_size = SIZEOF_ARRAY(additional_DTSHD_start_bytesW) * 2;
            bitstreamBuffer.append((char*)additional_DTSHD_start_bytesW, additional_bytes_size);
        } else if (codecId == CODEC_ID_BITSTREAM_TRUEHD) {
            size_t additional_bytes_size = SIZEOF_ARRAY(additional_MAT_start_bytesW) * 2;
            appendMATBuffer((char*)additional_MAT_start_bytesW, additional_bytes_size);
            additional_bytes_size += 8; // For IEC size
            if (additional_blank_size > 0) {
                if (additional_blank_size > additional_bytes_size) {
                    additional_blank_size -= additional_bytes_size;
                    filledBytes = 0;
                } else {
                    filledBytes = additional_bytes_size - additional_blank_size;
                    additional_blank_size = 0;
                }
            }
        }
    }
}

void TaudioCodecBitstream::appendMATBuffer(char *src, size_t length)
{
    if (bitstreamBuffer.size() == 0) {
        filledBytes += 8;  // IEC header (8 bytes at the beginning of each bitstream frames) must be counted
    }
    bitstreamBuffer.append(src, length);
    filledBytes += length;
}

void TaudioCodecBitstream::fillAdditionalMiddleBytes(void)
{
    if (codecId != CODEC_ID_BITSTREAM_TRUEHD) {
        return;
    }
    appendMATBuffer((char*)additional_MAT_middle_bytesW, SIZEOF_ARRAY(additional_MAT_middle_bytesW) * 2);
}

void TaudioCodecBitstream::fillAdditionalEndBytes(void)
{
    if (codecId != CODEC_ID_BITSTREAM_TRUEHD) {
        return;
    }
    appendMATBuffer((char*)additional_MAT_end_bytesW, SIZEOF_ARRAY(additional_MAT_end_bytesW) * 2);
}



int TaudioCodecBitstream::fillMATBuffer(BYTE *src, size_t length, bool checkLength)
{
    if (bitstreamBuffer.size() >= buffer_limit) {
        return (int)length;
    }
    int remainedLength = (int)length;

    // Pos 30708 = middle of the MAT frame and we need to write the additinal middle bytes even if there
    // is a buffer overlapped. So if the buffer will overlap, we will cut it into 2 pieces
    // (we must not overwrite the bytes but shift the bytes)
    if (bitstreamBuffer.size() <= 30708 && bitstreamBuffer.size() + length > 30708) {
        size_t writeBefore = 30708 - bitstreamBuffer.size();
        appendMATBuffer((char *)src, writeBefore);
        remainedLength -= (int)writeBefore;
        fillAdditionalMiddleBytes();
        if (checkLength) { // We are writing zero bytes with fixed length (including the additional bytes)
            size_t addedSize = SIZEOF_ARRAY(additional_MAT_middle_bytesW) * 2;
            remainedLength -= (int)addedSize;
        }
        if (remainedLength > 0) {
            appendMATBuffer((char *)(src + writeBefore), remainedLength);
            remainedLength = 0;
        }
        return remainedLength;
    }

    // Buffer ends after the buffer limit
    if (bitstreamBuffer.size() + length > buffer_limit) {
        size_t writeBefore = buffer_limit - bitstreamBuffer.size();
        appendMATBuffer((char *)src, writeBefore);
        remainedLength -= (int)writeBefore;
        fillAdditionalEndBytes();
        if (checkLength) { // We are writing zero bytes with fixed length (including the additional bytes)
            size_t addedSize = SIZEOF_ARRAY(additional_MAT_end_bytesW) * 2;
            remainedLength -= (int)addedSize;
        }
        return remainedLength;
    }

    appendMATBuffer((char *)src, length);

    return 0;
}


void TaudioCodecBitstream::fillBlankBytes(size_t bufferSize)
{
    // Additional 0 data bytes to add
    if (additional_blank_size > 0) { // For TrueHD there is a special treatment
        if (codecId == CODEC_ID_BITSTREAM_TRUEHD) {
            BYTE *blank_data = (BYTE*)alloca(sizeof(BYTE) * additional_blank_size);
            memset(blank_data, 0, additional_blank_size);
            int remainedBytes = fillMATBuffer(blank_data, additional_blank_size, true);
            // We couldn't write all the blank bytes in this buffer
            if (remainedBytes >= 0) {
                additional_blank_size = remainedBytes;
                filledBytes = 0; // bytes of data filled (include also extra bytes)
            } else { // We filled more than needed zero bytes (including extra bytes)
                additional_blank_size = 0;
                filledBytes = (size_t)((-1) * remainedBytes);
            }
        } else {
            size_t blank_size = additional_blank_size;
            if (bitstreamBuffer.size() + blank_size > bufferSize) {
                blank_size = bufferSize - bitstreamBuffer.size();
            }
            BYTE *blank_data = (BYTE*)alloca(sizeof(BYTE) * blank_size);
            memset(blank_data, 0, blank_size);
            bitstreamBuffer.append(blank_data, blank_size);
            additional_blank_size -= blank_size;
        }
    }
}

HRESULT TaudioCodecBitstream::decodeMAT(TbyteBuffer &src, TaudioParserData audioParserData)
{
    unsigned char *ptr = &src[0];
    unsigned char *base = ptr;
    unsigned char *end = ptr + src.size();
    buffer_limit = 61440/*total size*/ - 8/*IEC header*/ - 24/*Ending extra bytes*/;

    for (long l = 0; l < (long) audioParserData.frames.size() && ptr < end; l++) {
        numframes++;
        TframeData frameData = audioParserData.frames[l];
        uint32_t frame_size = frameData.frame_size;
        uint32_t nbZeros = (frameData.space_size - fullMATFrameSize) & 0xFFF;

        // Number of zeros to be added
        additional_blank_size += nbZeros;

        // Limit case : the previous buffer ended just before the buffer limit
        if (bitstreamBuffer.size() >= buffer_limit) {
            fillAdditionalEndBytes();
            size_t addedSize = SIZEOF_ARRAY(additional_MAT_end_bytesW) * 2;
            additional_blank_size -= addedSize;
            HRESULT hr = deciA->deliverSampleBistream((void*)&*bitstreamBuffer.begin(), (size_t)bitstreamBuffer.size(), audioParserData.bit_rate, audioParserData.sample_rate, true, 0, 61424);
            bitstreamBuffer.clear();
            if (hr != S_OK) {
                DPRINTF(_l("TaudioCodecBitstream::decode failed (%lld)"), hr);
                src.clear();
                return hr;
            }
            fillAdditionalBytes();
            // Fill last zero bytes if any
            fillBlankBytes(buffer_limit);
        }

        if (ptr + frame_size > end) {
            DPRINTF(_l("TaudioCodecBitstream::decode ERROR frame beyond the end of the buffer (size %ld, left %ld)"), frame_size, (end - ptr));
            return S_OK;
        }

        // Add additional bytes if empty buffer
        if (bitstreamBuffer.size() == 0) {
            fillAdditionalBytes();
        }

        // Fill the additional zero bytes
        fillBlankBytes(buffer_limit);

        // The buffer is full
        if (bitstreamBuffer.size() >= 61440 - 8) {
            HRESULT hr = deciA->deliverSampleBistream((void*)&*bitstreamBuffer.begin(), (size_t)bitstreamBuffer.size(), audioParserData.bit_rate, audioParserData.sample_rate, true, 0, 61424);
            bitstreamBuffer.clear();
            if (hr != S_OK) {
                DPRINTF(_l("TaudioCodecBitstream::decode failed (%lld)"), hr);
                src.clear();
                return hr;
            }
            fillAdditionalBytes();
            // Fill last zero bytes if any
            fillBlankBytes(buffer_limit);

        }

        // Fill the MAT frame into the output buffer
        int remainMATBytes = fillMATBuffer(ptr, frame_size);
        ptr += (frame_size - remainMATBytes);

        // Not all the bytes from the MAT frame could be filled, so submit the buffer
        if (remainMATBytes > 0) {
            HRESULT hr = deciA->deliverSampleBistream((void*)&*bitstreamBuffer.begin(), (size_t)bitstreamBuffer.size(), audioParserData.bit_rate, audioParserData.sample_rate, true, 0, 61424);
            bitstreamBuffer.clear();
            if (hr != S_OK) {
                DPRINTF(_l("TaudioCodecBitstream::decode failed (%lld)"), hr);
                src.clear();
                return hr;
            }
            fillAdditionalBytes();
            // Fill the last mat bytes
            fillMATBuffer(ptr, remainMATBytes);
            ptr += remainMATBytes;
        }

        // Now update the MAT size : it include the size of the MAT frame + the size of the extra bytes around and inside
        fullMATFrameSize = filledBytes;
        filledBytes = 0;
    }
    src.clear();
    return S_OK;
}



HRESULT TaudioCodecBitstream::decode(TbyteBuffer &src)
{
    if (src.size() == 0) {
        return S_OK;
    }
    size_t numsamples = src.size() / fmt.blockAlign();
    void *samples = numsamples ? &src[0] : NULL;

    unsigned char *ptr = &src[0];
    unsigned char *base = ptr;
    unsigned char *end = ptr + src.size();
    buffer_limit = 2048;

    TaudioParser *pAudioParser = NULL;
    sinkA->getAudioParser(&pAudioParser);


    if (pAudioParser == NULL) {
        DPRINTF(_l("TaudioCodecBitstream::decode No Audio parser !!"));
        src.clear();
        return S_OK;
    }

    TaudioParserData audioParserData = pAudioParser->getParserData();

    bpssum += (audioParserData.bit_rate / 1000);

    if (spdif_codec(codecId) || bitstream_codec(codecId)) {
        if (audioParserData.frames.size() == 0) {
            DPRINTF(_l("TaudioCodecBitstream::decode No frame, skip"));
            src.clear();
            return S_OK;
        }

        if (codecId == CODEC_ID_BITSTREAM_TRUEHD) {
            TinputPin *inpin = deciA->getInputPin();
            REFERENCE_TIME rtStart, rtStop;
            deciA->getInputTime(rtStart, rtStop);
            int audioDelay = deci->getParam2(IDFF_audio_decoder_delay);
            REFERENCE_TIME delay100ns = audioDelay * 10000LL;

            // Not a good idea : causes audio cuts (audio desync)
            if (deci->getParam2(IDFF_aoutpassthroughDeviceId) == TsampleFormat::XONAR && (rtStart + delay100ns) < 0) {
                DPRINTF(_l("TaudioCodecBitstream::decode drop frame (timestamp negative)"));
                deciA->deliverSampleBistream(NULL, bitstreamBuffer.size(), audioParserData.bit_rate, audioParserData.sample_rate, true, 0, 0);
                bitstreamBuffer.clear();
                src.clear();
                pAudioParser->SearchSync();
                return S_OK;
            }
            return decodeMAT(src, audioParserData);
        }

        switch (codecId) {
            case CODEC_ID_BITSTREAM_DTSHD:
                buffer_limit = 32768 - 16;
                break;
            default:
                buffer_limit = 2048;
                break;//For now we don't reformat the bistream for other codecs including EAC3. TODO maybe
        }
        buffer_limit -= 48; // Don't write in the last 48 bytes (for safety or if the IEC header is longer)
        if (bitstreamBuffer.empty()) {
            bitstreamBuffer.reserve(buffer_limit);
        }

        for (long l = 0; l < (long) audioParserData.frames.size() && ptr < end; l++) {
            numframes++;
            TframeData frameData = audioParserData.frames[l];
            uint32_t frame_size = frameData.frame_size;
            if (ptr + frame_size > end) {
                DPRINTF(_l("TaudioCodecBitstream::decode ERROR frame beyond the end of the buffer (size %ld, left %ld)"), frame_size, (end - ptr));
                src.clear();
                return S_OK;
            }

            // Fill the additional zero bytes if some remain
            fillBlankBytes(buffer_limit);

            // If buffer is empty, add the additional bytes if any(between the IEC header and the data)
            if (bitstreamBuffer.empty()) {
                fillAdditionalBytes();
            }

            if (codecId == CODEC_ID_BITSTREAM_DTSHD) {
                WORD frame_size_swab = frame_size;
                WORD iec_size = WORD((frame_size & ~0xf) + 0x18);
                swapbe(&frame_size_swab, 2);
                bitstreamBuffer.append((void*)&frame_size_swab, 2);
                bitstreamBuffer.append(ptr, frame_size);
                HRESULT hr = deciA->deliverSampleBistream((void*)&*bitstreamBuffer.begin(), bitstreamBuffer.size(), audioParserData.bit_rate, audioParserData.sample_rate, true, 0, iec_size);
                bitstreamBuffer.clear();
                if (hr != S_OK) {
                    DPRINTF(_l("TaudioCodecBitstream::decode failed (%lld)"), hr);
                    src.clear();
                    return hr;
                }
                ptr += frame_size;
            } else {
                WORD iec_length = 0;
                if (codecId == CODEC_ID_BITSTREAM_EAC3) {
                    iec_length = 24576;
                }
                HRESULT hr = deciA->deliverSampleBistream((void*)ptr, frame_size, audioParserData.bit_rate, audioParserData.sample_rate, true, (codecId == CODEC_ID_SPDIF_DTS) ? (audioParserData.sample_blocks * 8 * 32) : 0, iec_length);
                ptr += frame_size;

                if (hr != S_OK) {
                    DPRINTF(_l("TaudioCodecBitstream::decode failed (%lld)"), hr);
                    src.clear();
                    return hr;
                }
            }
        }
        src.clear();
        return S_OK;
    } else { // Not a bitstream codec (but why would we be here ?)
        numframes++;
        HRESULT hr = sinkA->deliverProcessedSample(samples, numsamples, fmt);
        src.clear();
        return hr;
    }
}

bool TaudioCodecBitstream::onSeek(REFERENCE_TIME segmentStart)
{
    DPRINTF(_l("TaudioCodecBitstream::onSeek"));
    bitstreamBuffer.clear();
    additional_blank_size = 0;
    filledBytes = 0;
    fullMATFrameSize = 0;
    //pAudioParser->SearchSync();
    return false;
}