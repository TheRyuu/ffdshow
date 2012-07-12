/*
 * $Id: DXVADecoderH264.cpp 4276 2012-04-07 08:48:47Z Aleksoid $
 *
 * (C) 2006-2011 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include "TDXVADecoderH264.h"
#include "TvideoCodecLibavcodec.h"
#include "TvideoCodecLibavcodecDxva.h"
#include "PODtypes.h"
#include "ffmpeg/libavcodec/avcodec.h"
#include "H264Nalu.h"

TDXVADecoderH264::TDXVADecoderH264(IffdshowDecVideo*   IdeciV, IAMVideoAccelerator*    pAMVideoAccelerator, DXVAMode   nMode, int nPicEntryNumber)
    :   TDXVADecoder(IdeciV,   pAMVideoAccelerator, nMode, nPicEntryNumber)
{
    m_bUseLongSlice = (GetDXVA1Config()->bConfigBitstreamRaw != 2);
    Init();
}

TDXVADecoderH264::TDXVADecoderH264(IffdshowDecVideo*   IdeciV, IDirectXVideoDecoder*   pDirectXVideoDec,   DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config)
    :   TDXVADecoder(IdeciV,   pDirectXVideoDec,   nMode, nPicEntryNumber, pDXVA2Config)
{
    m_bUseLongSlice = (m_pCodec->getDXVA2Config()->ConfigBitstreamRaw   != 2);
    Init();
}

TDXVADecoderH264::~TDXVADecoderH264()
{
}

void TDXVADecoderH264::Init()
{
    memset(&m_DXVAPicParams,   0, sizeof(m_DXVAPicParams));
    memset(&m_DXVAPicParams,   0, sizeof(DXVA_PicParams_H264));
    memset(&m_pSliceLong,      0, sizeof(DXVA_Slice_H264_Long) *MAX_SLICES);
    memset(&m_pSliceShort,     0, sizeof(DXVA_Slice_H264_Short)*MAX_SLICES);

    m_DXVAPicParams.MbsConsecutiveFlag                  = 1;
    if (m_pCodec->getPCIVendor() == PCIV_Intel) {
        m_DXVAPicParams.Reserved16Bits                  = 0x534c;
    } else {
        m_DXVAPicParams.Reserved16Bits                  = 0;
    }
    m_DXVAPicParams.ContinuationFlag                    = 1;
    m_DXVAPicParams.Reserved8BitsA                      = 0;
    m_DXVAPicParams.Reserved8BitsB                      = 0;
    m_DXVAPicParams.MinLumaBipredSize8x8Flag            = 1;    // Improve accelerator performances
    m_DXVAPicParams.StatusReportFeedbackNumber          = 0;    // Use to report status

    for (int i = 0; i < 16; i++) {
        m_DXVAPicParams.RefFrameList[i].AssociatedFlag  = 1;
        m_DXVAPicParams.RefFrameList[i].bPicEntry       = 255;
        m_DXVAPicParams.RefFrameList[i].Index7Bits      = 127;
    }


    m_nNALLength        = 4;
    m_nMaxSlices        = 0;

    switch (GetMode()) {
        case H264_VLD :
            AllocExecuteParams(3);
            break;
        default :
            ASSERT(FALSE);
    }
}

void TDXVADecoderH264::CopyBitstream(BYTE* pDXVABuffer, BYTE*   pBuffer, UINT& nSize)
{
    CH264Nalu   Nalu;
    int         nDummy;
    int         nSlices     = 0;
    UINT        m_nSize     = nSize;
    int         slice_step  = 1;
    int         nDxvaNalLength;

    Nalu.SetBuffer(pBuffer, nSize, m_nNALLength);

slice_again:
    nSize = 0;
    while (Nalu.ReadNext()) {
        switch (Nalu.GetType()) {
            case NALU_TYPE_SLICE:
            case NALU_TYPE_IDR:
                // Skip the NALU if the data length is below 0
                if (Nalu.GetDataLength() < 0) {
                    break;
                }

                // For AVC1, put startcode 0x000001
                pDXVABuffer[0] = pDXVABuffer[1] = 0;
                pDXVABuffer[2] = 1;
                if (Nalu.GetDataLength() < 0) {
                    break;
                }

                // Copy NALU
                __try {
                    memcpy(pDXVABuffer + 3, Nalu.GetDataBuffer(), Nalu.GetDataLength());
                } __except (EXCEPTION_EXECUTE_HANDLER) { break; }

                // Update slice control buffer
                nDxvaNalLength                                  = Nalu.GetDataLength() + 3;
                m_pSliceShort[nSlices].BSNALunitDataLocation    = nSize;
                m_pSliceShort[nSlices].SliceBytesInBuffer       = nDxvaNalLength;

                nSize                                          += nDxvaNalLength;
                pDXVABuffer                                    += nDxvaNalLength;
                nSlices++;
                break;
        }
    }

    if (!nSlices && slice_step == 1) {
        slice_step++;
        Nalu.SetBuffer(pBuffer, m_nSize, 0);
        goto slice_again;
    }

    // Complete with zero padding (buffer size should be a multiple of 128)
    nDummy  = 128 - (nSize % 128);

    memset(pDXVABuffer, 0, nDummy);
    m_pSliceShort[nSlices - 1].SliceBytesInBuffer += nDummy;
    nSize                                       += nDummy;
}

void TDXVADecoderH264::Flush()
{
    ClearRefFramesList();
    m_DXVAPicParams.UsedForReferenceFlags   = 0;
    m_nOutPOC                               = INT_MIN;
    m_rtLastFrameDisplayed      =   0;

    __super::Flush();
}

HRESULT TDXVADecoderH264::DecodeFrame(BYTE* pDataIn, UINT   nSize, REFERENCE_TIME   rtStart, REFERENCE_TIME rtStop)
{
    HRESULT                     hr              = S_FALSE;
    UINT                        nSlices         = 0;
    int                         nSurfaceIndex   = -1;
    int                         nFieldType      = -1;
    int                         nSliceType      = -1;
    int                         nFramePOC       = INT_MIN;
    int                         nOutPOC         = INT_MIN;
    REFERENCE_TIME              rtOutStart      = _I64_MIN;
    CH264Nalu                   Nalu;
    UINT                        nNalOffset      = 0;
    CComPtr<IMediaSample>       pSampleToDeliver;
    //CComQIPtr<IMPCDXVA2Sample>    pDXVA2Sample;
    int                         slice_step      = 1;

    if (m_pCodec->libavcodec->FFH264DecodeBuffer(m_pCodec->avctx,   pDataIn, nSize, &nFramePOC, &nOutPOC,   &rtOutStart) == -1) {
        return S_FALSE;
    }

    DPRINTF(_l("CDXVADecoderH264::DecodeFrame() : nFramePOC = %d, nOutPOC = %d[%d], rtOutStart = %I64d\n"), nFramePOC, nOutPOC, m_nOutPOC, rtOutStart);

    Nalu.SetBuffer(pDataIn, nSize, m_nNALLength);

slice_again:
    while (Nalu.ReadNext()) {
        switch (Nalu.GetType()) {
            case NALU_TYPE_SLICE:
            case NALU_TYPE_IDR:
                if (m_bUseLongSlice) {
                    m_pSliceLong[nSlices].BSNALunitDataLocation = nNalOffset;
                    m_pSliceLong[nSlices].SliceBytesInBuffer    = Nalu.GetDataLength() + 3; //.GetRoundedDataLength();
                    m_pSliceLong[nSlices].slice_id              = nSlices;
                    m_pCodec->libavcodec->FF264UpdateRefFrameSliceLong(&m_DXVAPicParams, &m_pSliceLong[nSlices], m_pCodec->avctx);

                    if (nSlices > 0) {
                        m_pSliceLong[nSlices - 1].NumMbsForSlice = m_pSliceLong[nSlices].NumMbsForSlice = m_pSliceLong[nSlices].first_mb_in_slice - m_pSliceLong[nSlices - 1].first_mb_in_slice;
                    }
                }
                nSlices++;
                nNalOffset += (UINT)(Nalu.GetDataLength() + 3);
                if (nSlices > MAX_SLICES) {
                    break;
                }
                break;
        }
    }

    if (!nSlices) {
        if (slice_step == 1) {
            slice_step++;
            Nalu.SetBuffer(pDataIn, nSize, 0);
            goto slice_again;
        }

        return S_FALSE;
    }

    m_nMaxWaiting = std::min(std::max((int)m_DXVAPicParams.num_ref_frames, 3), 8);

    // If parsing fail (probably no PPS/SPS), continue anyway it may arrived later (happen on truncated streams)
    if (FAILED(m_pCodec->libavcodec->FFH264BuildPicParams(&m_DXVAPicParams,   &m_DXVAScalingMatrix,   &nFieldType, &nSliceType,   m_pCodec->avctx, m_pCodec->getPCIVendor()))) {
        return S_FALSE;
    }

    // Wait I frame after a flush
    if (m_bFlushed && !m_DXVAPicParams.IntraPicFlag) {
        return S_FALSE;
    }

    CHECK_HR(GetFreeSurfaceIndex(nSurfaceIndex, &pSampleToDeliver, rtStart, rtStop));
    m_pCodec->libavcodec->FFH264SetCurrentPicture(nSurfaceIndex, &m_DXVAPicParams,   m_pCodec->avctx);

    CHECK_HR(BeginFrame(nSurfaceIndex, pSampleToDeliver));

    m_DXVAPicParams.StatusReportFeedbackNumber++;

    // Send picture parameters
    CHECK_HR(AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(m_DXVAPicParams), &m_DXVAPicParams));
    CHECK_HR(Execute());

    // Add bitstream, slice control and quantization matrix
    CHECK_HR(AddExecuteBuffer(DXVA2_BitStreamDateBufferType, nSize, pDataIn, &nSize));

    if (m_bUseLongSlice) {
        CHECK_HR(AddExecuteBuffer(DXVA2_SliceControlBufferType,  sizeof(DXVA_Slice_H264_Long)*nSlices, m_pSliceLong));
    } else {
        CHECK_HR(AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_Slice_H264_Short)*nSlices, m_pSliceShort));
    }

    CHECK_HR(AddExecuteBuffer(DXVA2_InverseQuantizationMatrixBufferType, sizeof(DXVA_Qmatrix_H264), (void*)&m_DXVAScalingMatrix));

    // Decode bitstream
    CHECK_HR(Execute());

    CHECK_HR(EndFrame(nSurfaceIndex));

#if defined(_DEBUG) && 0
    DisplayStatus();
#endif

    bool bAdded = AddToStore(nSurfaceIndex, pSampleToDeliver, m_DXVAPicParams.RefPicFlag, rtStart, rtStop,
                             m_DXVAPicParams.field_pic_flag, (FF_FIELD_TYPE)nFieldType,
                             (FF_SLICE_TYPE)nSliceType, nFramePOC);

    m_pCodec->libavcodec->FFH264UpdateRefFramesList(&m_DXVAPicParams, m_pCodec->avctx);
    ClearUnusedRefFrames();

    if (bAdded) {
        hr = DisplayNextFrame();
    }

    if (nOutPOC != INT_MIN) {
        m_nOutPOC       = nOutPOC;
        m_rtOutStart    = rtOutStart;
    }

    m_bFlushed = false;
    return hr;
}

void TDXVADecoderH264::RemoveUndisplayedFrame(int   nPOC)
{
    // Find frame with given POC, and free the slot
    for (int i = 0; i < m_nPicEntryNumber; i++) {
        if (m_pPictureStore[i].bInUse && m_pPictureStore[i].nCodecSpecific == nPOC) {
            m_pPictureStore[i].bDisplayed = true;
            RemoveRefFrame(i);
            return;
        }
    }
}

void TDXVADecoderH264::ClearUnusedRefFrames()
{
    // Remove old reference frames (not anymore a short or long ref frame)
    for (int i = 0; i < m_nPicEntryNumber; i++) {
        if (m_pPictureStore[i].bRefPicture && m_pPictureStore[i].bDisplayed) {
            if (!m_pCodec->libavcodec->FFH264IsRefFrameInUse(i, m_pCodec->avctx)) {
                RemoveRefFrame(i);
            }
        }
    }
}

void TDXVADecoderH264::SetExtraData(BYTE* pDataIn, UINT nSize)
{
    AVCodecContext* pAVCtx = m_pCodec->avctx;
    m_nNALLength            = pAVCtx->nal_length_size;

    m_pCodec->libavcodec->FFH264DecodeBuffer(pAVCtx,   pDataIn, nSize, NULL,   NULL,   NULL);
    m_pCodec->libavcodec->FFH264SetDxvaSliceLong(pAVCtx,   m_pSliceLong);
}

void TDXVADecoderH264::ClearRefFramesList()
{
    for (int i = 0; i < m_nPicEntryNumber; i++) {
        if (m_pPictureStore[i].bInUse) {
            m_pPictureStore[i].bDisplayed = true;
            RemoveRefFrame(i);
        }
    }
}

HRESULT TDXVADecoderH264::DisplayStatus()
{
    HRESULT             hr = E_INVALIDARG;
    DXVA_Status_H264    Status;

    memset(&Status, 0, sizeof(Status));
    CHECK_HR(hr = TDXVADecoder::QueryStatus(&Status,   sizeof(Status)));

    DPRINTF(_l("TDXVADecoderH264::DisplayStatus	Status for the frame %u	:	bBufType = %u, bStatus = %u, wNumMbsAffected = %u\n"),
            Status.StatusReportFeedbackNumber,
            Status.bBufType,
            Status.bStatus,
            Status.wNumMbsAffected);

    return hr;
}

int TDXVADecoderH264::FindOldestFrame()
{
    int             nPos  = -1;
    REFERENCE_TIME  rtPos = _I64_MAX;

    for (int i = 0; i < m_nPicEntryNumber; i++) {
        if (m_pPictureStore[i].bInUse && !m_pPictureStore[i].bDisplayed) {
            if ((m_pPictureStore[i].nCodecSpecific == m_nOutPOC) && (m_pPictureStore[i].rtStart < rtPos) && (m_pPictureStore[i].rtStart >= m_rtOutStart)) {
                nPos  = i;
                rtPos = m_pPictureStore[i].rtStart;
            }
        }
    }

    if (nPos != -1) {
        if (m_rtOutStart == _I64_MIN)   {
            // If   start   time not set (no PTS for example), guess presentation   time!
            m_rtOutStart = m_rtLastFrameDisplayed;
        }
        m_pPictureStore[nPos].rtStart   = m_rtOutStart;
        m_pPictureStore[nPos].rtStop = m_rtOutStart + m_pCodec->getAvrTimePerFrame();
        m_rtLastFrameDisplayed = m_pPictureStore[nPos].rtStop;
        m_pCodec->reorderBFrames(m_pPictureStore[nPos].rtStart, m_pPictureStore[nPos].rtStop);
    }

    return nPos;
}
