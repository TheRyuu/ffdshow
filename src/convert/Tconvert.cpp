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
#include "IffdshowBase.h"
#include "IffdshowDecVideo.h"
#include "ffdshow_constants.h"
#include "Tconfig.h"
#include "Tconvert.h"
#include "ffImgfmt.h"
#include "convert_yv12.h"
#include "convert_yuy2.h"
#include "Tswscale.h"
#include "image.h"
#include "TffPict.h"
#include "Tlibavcodec.h"
#include "libswscale/swscale.h"
#include "libavcodec/get_bits.h"
#include "ToutputVideoSettings.h"
#include "ffdshow_converters.h"
#include "ffdshow_converters2.h"

//======================================= Tconvert =======================================
Tconvert::Tconvert(IffdshowBase *deci, unsigned int Idx, unsigned int Idy, LONG dstSize) :
    TrgbPrimaries(deci),
    m_wasChange(false),
    m_dstSize(dstSize),
    tmpcsp(0),
    timer(L"color space conversion cost:")
{
    Tlibavcodec *libavcodec;
    deci->getLibavcodec(&libavcodec);
    bool highQualityRGB = !!deci->getParam2(IDFF_highQualityRGB);
    bool dithering = !!deci->getParam2(IDFF_RGB_dithering);
    IffdshowDecVideo *deciV = comptrQ<IffdshowDecVideo>(deci);
    bool isMPEG1 = false;
    if (deciV) {
        isMPEG1 = deciV->getMovieFOURCC() == mmioFOURCC('M', 'P', 'G', '1');
    }
    int rgbInterlaceMode = deci->getParam2(IDFF_cspOptionsRgbInterlaceMode);
    init(libavcodec, highQualityRGB, Idx, Idy, rgbInterlaceMode, dithering, isMPEG1);
}

Tconvert::Tconvert(Tlibavcodec *Ilibavcodec, bool highQualityRGB, unsigned int Idx, unsigned int Idy, const TrgbPrimaries &IrgbPrimaries, int rgbInterlaceMode, bool dithering, bool isMPEG1):
    TrgbPrimaries(IrgbPrimaries),
    m_wasChange(false),
    m_dstSize(0),
    tmpcsp(0),
    timer(L"color space conversion cost:")
{
    Ilibavcodec->AddRef();
    init(Ilibavcodec, highQualityRGB, Idx, Idy, rgbInterlaceMode, dithering, isMPEG1);
}

void Tconvert::init(Tlibavcodec *Ilibavcodec, bool highQualityRGB, unsigned int Idx, unsigned int Idy, int IrgbInterlaceMode, bool dithering, bool isMPEG1)
{
    libavcodec = Ilibavcodec;
    m_isMPEG1 = isMPEG1;
    dx = Idx;
    dy = Idy&~1;
    outdy = Idy;
    swscale = NULL;
    oldincsp = oldoutcsp = -1;
    incspInfo = outcspInfo = NULL;
    initsws = true;
    tmp[0] = tmp[1] = tmp[2] = NULL;
    tmpConvert1 = tmpConvert2 = NULL;
    m_highQualityRGB = highQualityRGB;
    m_dithering = dithering;
    rgbInterlaceMode = IrgbInterlaceMode;
    ffdshow_converters = NULL;
}

Tconvert::~Tconvert()
{
    freeTmpConvert();
    if (swscale) {
        delete swscale;
    }
    libavcodec->Release();
    if (ffdshow_converters) {
        delete ffdshow_converters;
    }
}

const char_t* Tconvert::getModeName(int mode)
{
    switch (mode) {
        case MODE_none:
            return _l("none");
        case MODE_avisynth_yv12_to_yuy2:
            return _l("avisynth_yv12_to_yuy2");
        case MODE_xvidImage_output:
            return _l("xvidImage_output");
        case MODE_avisynth_yuy2_to_yv12:
            return _l("avisynth_yuy2_to_yv12");
        case MODE_mmx_ConvertRGB32toYUY2:
            return _l("mmx_ConvertRGB32toYUY2");
        case MODE_mmx_ConvertRGB24toYUY2:
            return _l("mmx_ConvertRGB24toYUY2");
        case MODE_mmx_ConvertYUY2toRGB32:
            return _l("mmx_ConvertYUY2toRGB32");
        case MODE_mmx_ConvertYUY2toRGB24:
            return _l("mmx_ConvertYUY2toRGB24");
        case MODE_mmx_ConvertUYVYtoRGB32:
            return _l("mmx_ConvertUYVYtoRGB32");
        case MODE_mmx_ConvertUYVYtoRGB24:
            return _l("mmx_ConvertUYVYtoRGB24");
        case MODE_ffdshow_converters:
            return _l("ffdshow converters");
        case MODE_ffdshow_converters2:
            return _l("ffdshow converters2");
        case MODE_CLJR:
            return _l("CLJR");
        case MODE_xvidImage_input:
            return _l("xvidImage_input");
        case MODE_swscale:
            return _l("swscale");
        case MODE_avisynth_bitblt:
            return _l("avisynth_bitblt");
        case MODE_fallback:
            return _l("fallback");
        case MODE_fast_copy:
            return _l("fast copy");
        default:
            return _l("unknown");
    }
}

void Tconvert::freeTmpConvert(void)
{
    if (tmp[0]) {
        aligned_free(tmp[0]);
    }
    tmp[0] = NULL;
    if (tmp[1]) {
        aligned_free(tmp[1]);
    }
    tmp[1] = NULL;
    if (tmp[2]) {
        aligned_free(tmp[2]);
    }
    tmp[2] = NULL;
    if (tmpConvert1) {
        delete tmpConvert1;
    }
    tmpConvert1 = NULL;
    if (tmpConvert2) {
        delete tmpConvert2;
    }
    tmpConvert2 = NULL;
}

int Tconvert::convert(uint64_t incsp0,
                      const uint8_t*const src0[],
                      const stride_t srcStride0[],
                      uint64_t outcsp0, uint8_t* dst0[],
                      stride_t dstStride0[],
                      const Tpalette *srcpal,
                      enum AVColorRange &video_full_range_flag,
                      enum AVColorSpace YCbCr_RGB_matrix_coefficients,
                      bool vram_indirect)
{
    bool wasChange;
    m_wasChange = false;
    uint64_t incsp = incsp0, outcsp = outcsp0;
    if (rgbInterlaceMode == 1) { // Force Interlace
        incsp |= FF_CSP_FLAGS_INTERLACED;
        outcsp0 |= FF_CSP_FLAGS_INTERLACED;
        outcsp |= FF_CSP_FLAGS_INTERLACED;
    } else if (rgbInterlaceMode == 2) { // Force progressive
        incsp &= ~FF_CSP_FLAGS_INTERLACED;
        outcsp &= ~FF_CSP_FLAGS_INTERLACED;
        outcsp0 &= ~FF_CSP_FLAGS_INTERLACED;
    }

    if (!incspInfo || incspInfo->id != (incsp & FF_CSPS_MASK) || !outcspInfo || outcspInfo->id != (outcsp & FF_CSPS_MASK)) {
        incspInfo = csp_getInfo(incsp);
        outcspInfo = csp_getInfo(outcsp);
        wasChange = true;
    } else {
        wasChange = false;
    }

    const unsigned char *src[] = {src0[0], src0[1], src0[2], src0[3]};
    stride_t srcStride[] = {srcStride0[0], srcStride0[1], srcStride0[2], srcStride0[3]};
    csp_yuv_adj_to_plane(incsp , incspInfo , dy, (unsigned char**)src, srcStride); // YV12 and YV16, FF_CSP_FLAGS_YUV_ADJ: Cr and Cb is swapped here.
    csp_yuv_order(incsp , (unsigned char**)src, srcStride);
    csp_vflip(incsp , incspInfo, (unsigned char**)src, srcStride, dy);

    unsigned char *dst[] = {dst0[0], dst0[1], dst0[2], dst0[3]};
    stride_t dstStride[] = {dstStride0[0], dstStride0[1], dstStride0[2], dstStride0[3]};
    if (outcspInfo->id == FF_CSP_420P) {
        csp_yuv_adj_to_plane(outcsp, outcspInfo, odd2even(outdy), (unsigned char**)dst, dstStride);
    } else {
        csp_yuv_adj_to_plane(outcsp, outcspInfo, dy, (unsigned char**)dst, dstStride);
    }
    csp_yuv_order(outcsp, (unsigned char**)dst, dstStride);
    csp_vflip(outcsp, outcspInfo, (unsigned char**)dst, dstStride, dy);

    // check if the dstination buffer is big enough
    if (m_dstSize) {
        const TcspInfo *cspInfo = csp_getInfo(outcsp);
        LONG size = 0;
        for (unsigned int i = 0 ; i < cspInfo->numPlanes ; i++) {
            size += dstStride[i] * dy >> cspInfo->shiftY[i];
        }
        if (m_dstSize < size) {
            DPRINTF(L"ffdshow error: the down-stream filter prepared insufficient buffer. This can be a bug of the down-stream filter or ffdshow.");
            if (cspInfo->numPlanes > 1 || !dstStride[0]) {
                return 0;
            }
            dy = m_dstSize / dstStride[0];
        }
        m_dstSize = 0;
    }

    if (wasChange || oldincsp != incsp || oldoutcsp != outcsp) {
        m_wasChange = true;
        oldincsp = incsp;
        oldoutcsp = outcsp;
        freeTmpConvert();
        incsp1 = incsp & FF_CSPS_MASK;
        outcsp1 = outcsp & FF_CSPS_MASK;
        mode = MODE_none;

        if (incsp1 == outcsp1) {
            rowsize = dx * incspInfo->Bpp;
            mode = MODE_fast_copy;
        } else

#ifdef AVISYNTH_BITBLT
            if (incsp1 == outcsp1) {
                rowsize = dx * incspInfo->Bpp;
                mode = MODE_avisynth_bitblt;
            } else
#endif
                if (m_highQualityRGB
                        && (Tconfig::cpu_flags & FF_CPU_SSE2)
                        && !((outcsp | incsp) & FF_CSP_FLAGS_INTERLACED)
                        && incsp_sup_ffdshow_converter(incsp1)
                        && outcsp_sup_ffdshow_converter(outcsp1)) {
                    mode = MODE_ffdshow_converters;
                } else if (TffdshowConverters2::csp_sup_ffdshow_converter2(incsp1, outcsp1)) {
                    mode = MODE_ffdshow_converters2;
                } else {
                    switch (incsp1) {
                        case FF_CSP_420P:
                            switch (outcsp1) {
                                case FF_CSP_YUY2: //YV12 -> YUY2
                                    mode = MODE_avisynth_yv12_to_yuy2;
                                    if (incsp & FF_CSP_FLAGS_INTERLACED)
                                        if (Tconfig::cpu_flags & FF_CPU_SSE2) {
                                            avisynth_yv12_to_yuy2 = TconvertYV12<Tsse2>::yv12_i_to_yuy2;
                                        } else if (Tconfig::cpu_flags & FF_CPU_MMXEXT) {
                                            avisynth_yv12_to_yuy2 = TconvertYV12<Tmmxext>::yv12_i_to_yuy2;
                                        } else {
                                            avisynth_yv12_to_yuy2 = TconvertYV12<Tmmx>::yv12_i_to_yuy2;
                                        }
                                    else if (Tconfig::cpu_flags & FF_CPU_SSE2) {
                                        avisynth_yv12_to_yuy2 = TconvertYV12<Tsse2>::yv12_to_yuy2;
                                    } else if (Tconfig::cpu_flags & FF_CPU_MMXEXT) {
                                        avisynth_yv12_to_yuy2 = TconvertYV12<Tmmxext>::yv12_to_yuy2;
                                    } else {
                                        avisynth_yv12_to_yuy2 = TconvertYV12<Tmmx>::yv12_to_yuy2;
                                    }
                                    break;
                                default:
                                    // Xvid converter is slow for interlaced color spaces. Use AviSynth converter in this case.
                                    if (((outcsp & FF_CSP_FLAGS_INTERLACED) || m_highQualityRGB) && (outcsp1 == FF_CSP_RGB24 || outcsp1 == FF_CSP_RGB32)) {
                                        mode = MODE_fallback;
                                        tmpcsp = FF_CSP_YUY2;
                                        tmpStride[0] = 2 * (dx / 16 + 2) * 16;
                                        tmp[0] = (unsigned char*)aligned_malloc(tmpStride[0] * dy);
                                        tmpConvert1 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                                        tmpConvert2 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                                        if (incsp & FF_CSP_FLAGS_INTERLACED || outcsp & FF_CSP_FLAGS_INTERLACED) {
                                            tmpcsp |= FF_CSP_FLAGS_INTERLACED;
                                        }
                                        if ((incsp | outcsp) & FF_CSP_FLAGS_YUV_JPEG) {
                                            tmpcsp |= FF_CSP_FLAGS_YUV_JPEG;
                                        }
                                    } else if (csp_supXvid(outcsp1)
#ifndef XVID_BITBLT
                                               && outcsp1 != FF_CSP_420P
#endif
                                              ) {
                                        mode = MODE_xvidImage_output;
                                    }
                                    break;
                            } //switch (outcsp1)
                            break;
                        case FF_CSP_YUY2:
                            switch (outcsp1) {
                                case FF_CSP_420P: // YUY2 -> YV12
                                    mode = MODE_avisynth_yuy2_to_yv12;
                                    if (incsp & FF_CSP_FLAGS_INTERLACED)
                                        if (Tconfig::cpu_flags & FF_CPU_SSE2) {
                                            avisynth_yuy2_to_yv12 = TconvertYV12<Tsse2>::yuy2_i_to_yv12;
                                        } else if (Tconfig::cpu_flags & FF_CPU_MMXEXT) {
                                            avisynth_yuy2_to_yv12 = TconvertYV12<Tmmxext>::yuy2_i_to_yv12;
                                        } else {
                                            avisynth_yuy2_to_yv12 = TconvertYV12<Tmmx>::yuy2_i_to_yv12;
                                        }
                                    else if (Tconfig::cpu_flags & FF_CPU_SSE2) {
                                        avisynth_yuy2_to_yv12 = TconvertYV12<Tsse2>::yuy2_to_yv12;
                                    } else if (Tconfig::cpu_flags & FF_CPU_MMXEXT) {
                                        avisynth_yuy2_to_yv12 = TconvertYV12<Tmmxext>::yuy2_to_yv12;
                                    } else {
                                        avisynth_yuy2_to_yv12 = TconvertYV12<Tmmx>::yuy2_to_yv12;
                                    }
                                    break;
                                case FF_CSP_RGB24:
                                    mode = MODE_mmx_ConvertYUY2toRGB24; // YUY2 -> RGB24
                                    break;
                                case FF_CSP_RGB32:
                                    mode = MODE_mmx_ConvertYUY2toRGB32; // YUY2 -> RGB32
                                    break;
                            } //switch (outcsp1)
                            break;
                        case FF_CSP_UYVY:
                            switch (outcsp1) {
                                case FF_CSP_RGB24:
                                    mode = MODE_mmx_ConvertUYVYtoRGB24; // UYVY -> RGB24
                                    break;
                                case FF_CSP_RGB32:
                                    mode = MODE_mmx_ConvertUYVYtoRGB32; // UYVY -> RGB32
                                    break;
                            }
                            break;
                        case FF_CSP_RGB32:
                            if (outcsp1 == FF_CSP_YUY2) { // RGB32 -> YUY2
                                mode = MODE_mmx_ConvertRGB32toYUY2;
                                break;
                            }
                            if (outcsp1 == FF_CSP_NV12) { // RGB32 -> YV12 -> NV12
                                mode = MODE_fallback;
                                tmpcsp = FF_CSP_420P;
                                tmpStride[1] = tmpStride[2] = (tmpStride[0] = (dx / 16 + 2) * 16) / 2;
                                tmp[0] = (unsigned char*)aligned_malloc(tmpStride[0] * dy);
                                tmp[1] = (unsigned char*)aligned_malloc(tmpStride[1] * dy / 2);
                                tmp[2] = (unsigned char*)aligned_malloc(tmpStride[2] * dy / 2);
                                tmpConvert1 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                                tmpConvert2 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                                break;
                            }
                            if (outcsp1 == FF_CSP_UYVY || outcsp1 == FF_CSP_YVYU) { // RGB32 -> YUY2 -> UYVY/YVYU
                                mode = MODE_fallback;
                                tmpcsp = FF_CSP_YUY2;
                                tmpStride[0] = 2 * (dx / 16 + 2) * 16;
                                tmp[0] = (unsigned char*)aligned_malloc(tmpStride[0] * dy);
                                tmpConvert1 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                                tmpConvert2 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                                break;
                            }
                            break;
                        case FF_CSP_BGR24:
                            if (outcsp1 == FF_CSP_RGB24) {
                                // workaround to get correct colors when grabbing to BMP
                                incsp = incsp0 = FF_CSP_RGB24;
                            }
                            break;
                        case FF_CSP_RGB24:
                            if (!(outcsp1 == FF_CSP_BGR32 || outcsp1 == FF_CSP_BGR24)) {
                                mode = MODE_fallback;
                                tmpcsp = FF_CSP_BGR32; // FF_CSP_RGB32 doesn't work (libswscale's limitation).
                                tmpStride[0] = 4 * ffalign(dx, 16);
                                tmp[0] = (unsigned char*)aligned_malloc(tmpStride[0] * dy);
                                tmpConvert1 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                                tmpConvert2 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                                break;
                            }
                            break;
                        case FF_CSP_422P:
                            break;
                        case FF_CSP_CLJR:
                            if (outcsp1 == FF_CSP_420P) {
                                mode = MODE_CLJR;
                            }
                            break;
                        case FF_CSP_PAL8:
                            if (outcsp1 == FF_CSP_RGB32) {
                                if (!swscale) {
                                    swscale = new Tswscale(libavcodec);
                                }
                                swscale->init(dx, dy, incsp, outcsp);
                                mode = MODE_MODE_palette8torgb;
                            }
                    } // switch (incsp1)
                }

        if (mode == MODE_none)
            if (incsp1 != FF_CSP_420P && outcsp1 == FF_CSP_420P && csp_supXvid(incsp1) && incsp1 != FF_CSP_RGB24 && incsp1 != FF_CSP_BGR24) { // x -> YV12
                mode = MODE_xvidImage_input;
            } else if ((csp_supSWSin(incsp1) || csp_isRGBplanar(incsp1)) && csp_supSWSout(outcsp1)) {
                if (!swscale) {
                    swscale = new Tswscale(libavcodec);
                }
                swscale->init(dx, dy, incsp, outcsp);
                mode = MODE_swscale;
            } else if (outcsp1 == FF_CSP_P010 || outcsp1 == FF_CSP_P016) {
                mode = MODE_fallback;
                tmpcsp = FF_CSP_420P10;
                tmpStride[0] = ffalign(dx, 8) * 2;
                tmpStride[1] = ffalign(dx / 2, 8) * 2;
                tmpStride[2] = tmpStride[1];
                tmp[0] = (unsigned char*)aligned_malloc(tmpStride[0] * dy);
                tmp[1] = (unsigned char*)aligned_malloc(tmpStride[1] * dy / 2);
                tmp[2] = (unsigned char*)aligned_malloc(tmpStride[2] * dy / 2);
                tmpConvert1 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                tmpConvert2 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                if (incsp & FF_CSP_FLAGS_INTERLACED || outcsp & FF_CSP_FLAGS_INTERLACED) {
                    tmpcsp |= FF_CSP_FLAGS_INTERLACED;
                }
            } else if (outcsp1 == FF_CSP_P210 || outcsp1 == FF_CSP_P216) {
                mode = MODE_fallback;
                tmpcsp = FF_CSP_422P10;
                tmpStride[0] = ffalign(dx, 8) * 2;
                tmpStride[1] = ffalign(dx / 2, 8) * 2;
                tmpStride[2] = tmpStride[1];
                tmp[0] = (unsigned char*)aligned_malloc(tmpStride[0] * dy);
                tmp[1] = (unsigned char*)aligned_malloc(tmpStride[1] * dy);
                tmp[2] = (unsigned char*)aligned_malloc(tmpStride[2] * dy);
                tmpConvert1 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                tmpConvert2 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                if (incsp & FF_CSP_FLAGS_INTERLACED || outcsp & FF_CSP_FLAGS_INTERLACED) {
                    tmpcsp |= FF_CSP_FLAGS_INTERLACED;
                }
            } else if (outcsp1 == FF_CSP_AYUV) {
                mode = MODE_fallback;
                tmpcsp = FF_CSP_444P;
                tmpStride[0] = ffalign(dx, 16);
                tmpStride[1] = tmpStride[0];
                tmpStride[2] = tmpStride[0];
                tmp[0] = (unsigned char*)aligned_malloc(tmpStride[0] * dy);
                tmp[1] = (unsigned char*)aligned_malloc(tmpStride[1] * dy);
                tmp[2] = (unsigned char*)aligned_malloc(tmpStride[2] * dy);
                tmpConvert1 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                tmpConvert2 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                if (incsp & FF_CSP_FLAGS_INTERLACED || outcsp & FF_CSP_FLAGS_INTERLACED) {
                    tmpcsp |= FF_CSP_FLAGS_INTERLACED;
                }
            } else if (outcsp1 == FF_CSP_Y416) {
                mode = MODE_fallback;
                tmpcsp = FF_CSP_444P10;
                tmpStride[0] = ffalign(dx, 8) * 2;
                tmpStride[1] = tmpStride[0];
                tmpStride[2] = tmpStride[0];
                tmp[0] = (unsigned char*)aligned_malloc(tmpStride[0] * dy);
                tmp[1] = (unsigned char*)aligned_malloc(tmpStride[1] * dy);
                tmp[2] = (unsigned char*)aligned_malloc(tmpStride[2] * dy);
                tmpConvert1 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                tmpConvert2 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                if (incsp & FF_CSP_FLAGS_INTERLACED || outcsp & FF_CSP_FLAGS_INTERLACED) {
                    tmpcsp |= FF_CSP_FLAGS_INTERLACED;
                }
            } else {
                mode = MODE_fallback;
                if (incsp1 == FF_CSP_PAL8) {
                    tmpcsp = FF_CSP_RGB32;
                } else {
                    tmpcsp = FF_CSP_420P;
                }
                if (tmpcsp == FF_CSP_RGB32) {
                    tmpStride[0] = 4 * (dx / 16 + 2) * 16;
                    tmp[0] = (unsigned char*)aligned_malloc(tmpStride[0] * dy);
                } else {
                    ASSERT(tmpcsp);
                    tmpStride[1] = tmpStride[2] = (tmpStride[0] = (dx / 16 + 2) * 16) / 2;
                    tmp[0] = (unsigned char*)aligned_malloc(tmpStride[0] * dy);
                    tmp[1] = (unsigned char*)aligned_malloc(tmpStride[1] * dy / 2);
                    tmp[2] = (unsigned char*)aligned_malloc(tmpStride[2] * dy / 2);
                }
                tmpConvert1 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                tmpConvert2 = new Tconvert(libavcodec, m_highQualityRGB, dx, dy, *this, rgbInterlaceMode, m_dithering, m_isMPEG1);
                if (incsp & FF_CSP_FLAGS_INTERLACED || outcsp & FF_CSP_FLAGS_INTERLACED) {
                    tmpcsp |= FF_CSP_FLAGS_INTERLACED;
                }
            }

#ifdef DEBUG
        char_t incspS[256], outcspS[256];
        DPRINTF(_l("colorspace conversion: %s -> %s (%s)"), csp_getName(incsp0, incspS, 256), csp_getName(outcsp0, outcspS, 256), getModeName(mode));
#endif

        if (mode == MODE_xvidImage_input || mode == MODE_xvidImage_output) {
            int rgb_add = UpdateSettings(video_full_range_flag, YCbCr_RGB_matrix_coefficients);
            setJpeg(!!((incsp | outcsp) & FF_CSP_FLAGS_YUV_JPEG), rgb_add);
            initXvid(rgb_add);
        }
    }
    if (outcsp == FF_CSP_NULL) {
        return 0;
    }

    TautoPerformanceCounter autoTimer(&timer);
    int ret = dy;
    switch (mode) {
        case MODE_fast_copy: {
            for (unsigned int i = 0; i < incspInfo->numPlanes; i++) {
                copyPlane(dst[i], dstStride[i], src[i], srcStride[i], rowsize >> incspInfo->shiftX[i], dy >> incspInfo->shiftY[i]);
            }
            break;
        }

        case MODE_avisynth_bitblt: {
            for (unsigned int i = 0; i < incspInfo->numPlanes; i++) {
                TffPict::copy(dst[i], dstStride[i], src[i], srcStride[i], rowsize >> incspInfo->shiftX[i], dy >> incspInfo->shiftY[i]);
            }
            break;
        }
        case MODE_ffdshow_converters: {
            if (!ffdshow_converters) {
                ffdshow_converters = new TffdshowConverters(libavcodec->GetCPUCount()/* avoid multithreading on P4HT */);
            }
            UpdateSettings(video_full_range_flag, YCbCr_RGB_matrix_coefficients);
            setJpeg(!!((incsp | outcsp) & FF_CSP_FLAGS_YUV_JPEG));
            ffdshow_converters->init(incsp1, outcsp1, (ffYCbCr_RGB_MatrixCoefficientsType)cspOptionsIturBt, cspOptionsWhiteCutoff, cspOptionsBlackCutoff, cspOptionsChromaCutoff, cspOptionsRGB_WhiteLevel, cspOptionsRGB_BlackLevel, m_dithering, m_isMPEG1);
            ffdshow_converters->convert(src[0], src[1], src[2], dst[0], dx, dy, srcStride[0], srcStride[1], dstStride[0]);
            if (cspOptionsRGB_BlackLevel == 0 && cspOptionsRGB_WhiteLevel == 255) {
                video_full_range_flag = AVCOL_RANGE_JPEG;
            } else {
                video_full_range_flag = AVCOL_RANGE_MPEG;
            }
            break;
        }
        case MODE_ffdshow_converters2: {
            TffdshowConverters2::convert(
                incsp0 & FF_CSPS_MASK, outcsp0 & FF_CSPS_MASK,
                src[0], src[1], src[2],
                dst[0], dst[1], dst[2],
                dx, dy,
                srcStride[0], srcStride[1],
                dstStride[0], dstStride[1]);
            break;
        }
        case MODE_avisynth_yv12_to_yuy2:
            avisynth_yv12_to_yuy2(src[0], src[1], src[2], dx, srcStride[0], srcStride[1],
                                  dst[0], dstStride[0],
                                  dy);
            break;
        case MODE_xvidImage_output: {
            int rgb_add = UpdateSettings(video_full_range_flag, YCbCr_RGB_matrix_coefficients);
            setJpeg(!!((incsp | outcsp) & FF_CSP_FLAGS_YUV_JPEG), rgb_add);
            writeToXvidYCbCr2RgbMatrix(&yv12_to_bgr_mmx_data);
            IMAGE srcPict;
            srcPict.y = (unsigned char*)src[0];
            srcPict.u = (unsigned char*)src[1];
            srcPict.v = (unsigned char*)src[2];
            image_output(&srcPict,
                         dx,
                         outdy,
                         srcStride,
                         dst,
                         dstStride,
                         outcsp & ~FF_CSP_FLAGS_INTERLACED,
                         outcsp & FF_CSP_FLAGS_INTERLACED,
                         (incsp | outcsp) & FF_CSP_FLAGS_YUV_JPEG,
                         rgb_add,
                         vram_indirect);
            ret = outdy;
            break;
        }
        case MODE_avisynth_yuy2_to_yv12:
            avisynth_yuy2_to_yv12(src[0], dx * 2, srcStride[0],
                                  dst[0], dst[1], dst[2], dstStride[0], dstStride[1],
                                  dy);
            break;
        case MODE_mmx_ConvertRGB32toYUY2: {
            UpdateSettings(video_full_range_flag, YCbCr_RGB_matrix_coefficients);
            setJpeg(!!((incsp | outcsp) & FF_CSP_FLAGS_YUV_JPEG));
            const Tmmx_ConvertRGBtoYUY2matrix *matrix = getAvisynthRgb2YuvMatrix();
            Tmmx_ConvertRGBtoYUY2<false, false>::mmx_ConvertRGBtoYUY2(src[0], dst[0], srcStride[0], dstStride[0], dx, dy, matrix);
            break;
        }
        case MODE_mmx_ConvertRGB24toYUY2: {
            UpdateSettings(video_full_range_flag, YCbCr_RGB_matrix_coefficients);
            setJpeg(!!((incsp | outcsp) & FF_CSP_FLAGS_YUV_JPEG));
            const Tmmx_ConvertRGBtoYUY2matrix *matrix = getAvisynthRgb2YuvMatrix();
            Tmmx_ConvertRGBtoYUY2<true , false>::mmx_ConvertRGBtoYUY2(src[0], dst[0], srcStride[0], dstStride[0], dx, dy, matrix);
            break;
        }
        case MODE_mmx_ConvertYUY2toRGB24: {
            int rgb_add;
            UpdateSettings(video_full_range_flag, YCbCr_RGB_matrix_coefficients);
            setJpeg(!!((incsp | outcsp) & FF_CSP_FLAGS_YUV_JPEG));
            const unsigned char *matrix = getAvisynthYCbCr2RgbMatrix(rgb_add);
            if (rgb_add) {
                Tmmx_ConvertYUY2toRGB<0, 0, TvRGB>::mmx_ConvertYUY2toRGB(src[0], dst[0], src[0] + dy * srcStride[0], srcStride[0], dstStride[0], dx * 2, matrix);
            } else {
                Tmmx_ConvertYUY2toRGB<0, 0, PcRGB>::mmx_ConvertYUY2toRGB(src[0], dst[0], src[0] + dy * srcStride[0], srcStride[0], dstStride[0], dx * 2, matrix);
            }
            break;
        }
        case MODE_mmx_ConvertYUY2toRGB32: {
            int rgb_add;
            UpdateSettings(video_full_range_flag, YCbCr_RGB_matrix_coefficients);
            setJpeg(!!((incsp | outcsp) & FF_CSP_FLAGS_YUV_JPEG));
            const unsigned char *matrix = getAvisynthYCbCr2RgbMatrix(rgb_add);
            if (rgb_add) {
                Tmmx_ConvertYUY2toRGB<0, 1, TvRGB>::mmx_ConvertYUY2toRGB(src[0], dst[0], src[0] + dy * srcStride[0], srcStride[0], dstStride[0], dx * 2, matrix);
            } else {
                Tmmx_ConvertYUY2toRGB<0, 1, PcRGB>::mmx_ConvertYUY2toRGB(src[0], dst[0], src[0] + dy * srcStride[0], srcStride[0], dstStride[0], dx * 2, matrix);
            }
            break;
        }
        case MODE_mmx_ConvertUYVYtoRGB24: {
            int rgb_add;
            UpdateSettings(video_full_range_flag, YCbCr_RGB_matrix_coefficients);
            setJpeg(!!((incsp | outcsp) & FF_CSP_FLAGS_YUV_JPEG));
            const unsigned char *matrix = getAvisynthYCbCr2RgbMatrix(rgb_add);
            if (rgb_add) {
                Tmmx_ConvertYUY2toRGB<1, 0, TvRGB>::mmx_ConvertYUY2toRGB(src[0], dst[0], src[0] + dy * srcStride[0], srcStride[0], dstStride[0], dx * 2, matrix);
            } else {
                Tmmx_ConvertYUY2toRGB<1, 0, PcRGB>::mmx_ConvertYUY2toRGB(src[0], dst[0], src[0] + dy * srcStride[0], srcStride[0], dstStride[0], dx * 2, matrix);
            }
            break;
        }
        case MODE_mmx_ConvertUYVYtoRGB32: {
            int rgb_add;
            UpdateSettings(video_full_range_flag, YCbCr_RGB_matrix_coefficients);
            setJpeg(!!((incsp | outcsp) & FF_CSP_FLAGS_YUV_JPEG));
            const unsigned char *matrix = getAvisynthYCbCr2RgbMatrix(rgb_add);
            if (rgb_add) {
                Tmmx_ConvertYUY2toRGB<1, 1, TvRGB>::mmx_ConvertYUY2toRGB(src[0], dst[0], src[0] + dy * srcStride[0], srcStride[0], dstStride[0], dx * 2, matrix);
            } else {
                Tmmx_ConvertYUY2toRGB<1, 1, PcRGB>::mmx_ConvertYUY2toRGB(src[0], dst[0], src[0] + dy * srcStride[0], srcStride[0], dstStride[0], dx * 2, matrix);
            }
            break;
        }
        case MODE_CLJR: {
            // Copyright (c) 2003 Alex Beregszaszi
            GetBitContext gb;
            init_get_bits(&gb, src[0], int(srcStride[0]*dy * 8));
            for (unsigned int y = 0; y < dy; y++) {
                uint8_t *luma = &dst[0][y * dstStride[0]];
                uint8_t *cb  = &dst[1][(y / 2) * dstStride[1]];
                uint8_t *cr  = &dst[2][(y / 2) * dstStride[2]];
                for (unsigned int x = 0; x < dx; x += 4) {
                    luma[3] = uint8_t(get_bits(&gb, 5) << 3);
                    luma[2] = uint8_t(get_bits(&gb, 5) << 3);
                    luma[1] = uint8_t(get_bits(&gb, 5) << 3);
                    luma[0] = uint8_t(get_bits(&gb, 5) << 3);
                    luma += 4;
                    cb[0] = cb[1] = uint8_t(get_bits(&gb, 6) << 2);
                    cb += 2;
                    cr[0] = cr[1] = uint8_t(get_bits(&gb, 6) << 2);
                    cr += 2;
                }
            }
            break;
        }
        case MODE_xvidImage_input: {
            int rgb_add = UpdateSettings(video_full_range_flag, YCbCr_RGB_matrix_coefficients);
            setJpeg(!!((incsp | outcsp) & FF_CSP_FLAGS_YUV_JPEG), rgb_add);
            writeToXvidRgb2YCbCrMatrix(&bgr_to_yv12_mmx_data);
            IMAGE dstPict = {dst[0], dst[1], dst[2]};
            image_input(&dstPict, dx, dy, dstStride[0], dstStride[1], src[0], srcStride[0], incsp, incsp & FF_CSP_FLAGS_INTERLACED, (incsp | outcsp)&FF_CSP_FLAGS_YUV_JPEG, PcRGB);
            break;
        }
        case MODE_swscale:
            // egur: this crashes on NV12->NV12 copy!
            UpdateSettings(video_full_range_flag, YCbCr_RGB_matrix_coefficients);
            setJpeg(!!((incsp | outcsp) & FF_CSP_FLAGS_YUV_JPEG));
            swscale->convert(src, srcStride, dst, dstStride, *this);
            break;
        case MODE_MODE_palette8torgb: {
            const unsigned char *src1[4] = {src[0], srcpal->pal, NULL, NULL};
            UpdateSettings(video_full_range_flag, YCbCr_RGB_matrix_coefficients);
            setJpeg(!!((incsp | outcsp) & FF_CSP_FLAGS_YUV_JPEG));
            swscale->convert(src1, srcStride, dst, dstStride, *this);
            break;
        }
        case MODE_fallback:
            tmpConvert1->convert(incsp, src, srcStride, tmpcsp, tmp, tmpStride, srcpal, video_full_range_flag, YCbCr_RGB_matrix_coefficients);
            tmpConvert2->convert(tmpcsp, (const uint8_t**)tmp, tmpStride, outcsp, dst, dstStride, NULL, video_full_range_flag, YCbCr_RGB_matrix_coefficients);
            break;
        default:
            return 0;
    }
    return ret;
}
int Tconvert::convert(TffPict &pict, uint64_t outcsp, uint8_t* dst[], stride_t dstStride[], bool vram_indirect)
{
    return convert(pict.csp | ((!pict.film && (pict.fieldtype & FIELD_TYPE::MASK_INT)) ? FF_CSP_FLAGS_INTERLACED : 0),
                   pict.data,
                   pict.stride,
                   outcsp,
                   dst,
                   dstStride,
                   &pict.palette,
                   pict.video_full_range_flag,
                   pict.YCbCr_RGB_matrix_coefficients,
                   vram_indirect);
}

#define MIN_BUFF_SIZE (1 << 18)
typedef void* (*Tmemcpy)(void*, const void*, size_t);

static void* mt_copy(void* d, const void* s, size_t size, Tmemcpy memcpyFunc)
{
    if (!d || !s) { return NULL; }

    // Buffer is very small and not worth the effort
    if (size < MIN_BUFF_SIZE) {
        return memcpyFunc(d, s, size);
    }

    size_t blockSize = (size / 2) & ~0xf; // Make size a multiple of 16 bytes
    std::array<size_t, 3> offsets = { 0, blockSize, size };
    std::array<size_t, 2> indexes = { 0, 1 };

    Concurrency::parallel_for_each(indexes.begin(), indexes.end(), [d, s, offsets, memcpyFunc](size_t & i) {
        memcpyFunc((char*)d + offsets[i], (const char*)s + offsets[i], offsets[i + 1] - offsets[i]);
    });

    return d;
}

inline void* mt_memcpy(void* d, const void* s, size_t size)
{
    return mt_copy(d, s, size, memcpy);
}

void Tconvert::copyPlane(BYTE *dstp, stride_t dst_pitch, const BYTE *srcp, stride_t src_pitch, int row_size, int height, bool flip)
{
    if (dst_pitch == src_pitch && src_pitch == row_size && !flip) {
        mt_memcpy(dstp, srcp, src_pitch * height);
    } else {
        if (!flip) {
            for (int y = height; y > 0; --y) {
                memcpy(dstp, srcp, row_size);
                dstp += dst_pitch;
                srcp += src_pitch;
            }
        } else {
            dstp += dst_pitch * (height - 1);
            for (int y = height; y > 0; --y) {
                memcpy(dstp, srcp, row_size);
                dstp -= dst_pitch;
                srcp += src_pitch;
            }
        }
    }
}

//================================= TffColorspaceConvert =================================
CUnknown* WINAPI TffColorspaceConvert::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    TffColorspaceConvert *pNewObject = new TffColorspaceConvert(punk, phr);
    if (pNewObject == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return pNewObject;
}
STDMETHODIMP TffColorspaceConvert::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv, E_POINTER);
    if (riid == IID_IffColorspaceConvert) {
        return GetInterface<IffColorspaceConvert>(this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}

TffColorspaceConvert::TffColorspaceConvert(LPUNKNOWN punk, HRESULT *phr):
    CUnknown(NAME("TffColorspaceConvert"), punk, phr),
    c(NULL),
    config(new Tconfig(g_hInst))
{
    libavcodec = new Tlibavcodec(config);
    libavcodec->AddRef();
}
TffColorspaceConvert::~TffColorspaceConvert()
{
    if (c) {
        delete c;
    }
    libavcodec->Release();
    delete config;
}

STDMETHODIMP TffColorspaceConvert::allocPicture(uint64_t csp, unsigned int dx, unsigned int dy, uint8_t *data[], stride_t stride[])
{
    if (!dx || !dy || csp == FF_CSP_NULL) {
        return E_INVALIDARG;
    }
    if (!data || !stride) {
        return E_POINTER;
    }
    Tbuffer buf;
    buf.free = false;
    TffPict pict;
    pict.alloc(dx, dy, csp, buf);
    for (unsigned int i = 0; i < pict.cspInfo.numPlanes; i++) {
        data[i] = pict.data[i];
        stride[i] = pict.stride[i];
    }
    return S_OK;
}
STDMETHODIMP TffColorspaceConvert::freePicture(uint8_t *data[])
{
    if (!data) {
        return E_POINTER;
    }
    if (data[0]) {
        aligned_free(data[0]);
    }
    return S_OK;
}

STDMETHODIMP TffColorspaceConvert::convert(unsigned int dx, unsigned int dy, uint64_t incsp, uint8_t *src[], const stride_t srcStride[], uint64_t outcsp, uint8_t *dst[], stride_t dstStride[])
{
    return convertPalette(dx, dy, incsp, src, srcStride, outcsp, dst, dstStride, NULL, 0);
}
STDMETHODIMP TffColorspaceConvert::convertPalette(unsigned int dx, unsigned int dy, uint64_t incsp, uint8_t *src[], const stride_t srcStride[], uint64_t outcsp, uint8_t *dst[], stride_t dstStride[], const unsigned char *pal, unsigned int numcolors)
{
    if (!c || c->dx != dx || c->dy != dy) {
        if (c) {
            delete c;
        }
        c = new Tconvert(libavcodec, false, dx, dy, TrgbPrimaries(), 0, 0, false);
    }
    Tpalette p(pal, numcolors);
    enum AVColorRange range = AVCOL_RANGE_UNSPECIFIED; // dumy
    c->convert(incsp, src, srcStride, outcsp, dst, dstStride, &p, range);
    return S_OK;
}
