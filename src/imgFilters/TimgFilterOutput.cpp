/*
 * Copyright (c) 2005,2006 Milan Cutka
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
#include "TimgFilterOutput.h"
#include "Tconvert.h"
#include "TffPict.h"
#include "ToutputVideoSettings.h"
#include "IffdshowBase.h"
#include "Tlibavcodec.h"
#include "IffdshowDecVideo.h"

TimgFilterOutput::TimgFilterOutput(IffdshowBase *Ideci, Tfilters *Iparent): TimgFilter(Ideci, Iparent),
    convert(NULL),
    libavcodec(NULL), avctx(NULL), frame(NULL), dvpict(NULL),
    vramBenchmark(this)
{
}
TimgFilterOutput::~TimgFilterOutput()
{
    if (convert) {
        delete convert;
    }
    if (frame) {
        libavcodec->av_free(frame);
    }
    if (avctx) {
        libavcodec->av_free(avctx);
    }
    if (libavcodec) {
        libavcodec->Release();
    }
    if (dvpict) {
        delete dvpict;
    }
}

HRESULT TimgFilterOutput::process(TffPict &pict, uint64_t dstcsp, unsigned char *dst[4], int dstStride[4], LONG &dstSize, const ToutputVideoSettings *cfg)
{
    checkBorder(pict);
    if (!convert
            || convert->dx != pict.rectFull.dx
            || convert->dy != pict.rectFull.dy
            || old_cspOptionsRgbInterlaceMode != cfg->cspOptionsRgbInterlaceMode
            || old_highQualityRGB != cfg->highQualityRGB
            || old_dithering != cfg->dithering
            || old_outputLevelsMode != cfg->cspOptionsOutputLevelsMode
            || old_inputLevelsMode != cfg->cspOptionsInputLevelsMode
            || old_IturBt != cfg->cspOptionsIturBt) {
        old_highQualityRGB = cfg->highQualityRGB;
        old_dithering = cfg->dithering;
        old_cspOptionsRgbInterlaceMode = cfg->cspOptionsRgbInterlaceMode;
        old_outputLevelsMode = cfg->cspOptionsOutputLevelsMode;
        old_inputLevelsMode = cfg->cspOptionsInputLevelsMode;
        old_IturBt = cfg->cspOptionsIturBt;
        if (convert) {
            delete convert;
        }
        vramBenchmark.init();
        convert = new Tconvert(deci, pict.rectFull.dx, pict.rectFull.dy, dstSize);
    }
    stride_t cspstride[4] = {0, 0, 0, 0};
    unsigned char *cspdst[4] = {0, 0, 0, 0};

    const TcspInfo *outcspInfo = csp_getInfo(dstcsp);
    for (int i = 0; i < 4; i++) {
        cspstride[i] = dstStride[0] >> outcspInfo->shiftX[i];
        if (i == 0) {
            cspdst[i] = dst[i];
        } else {
            cspdst[i] = cspdst[i - 1] + cspstride[i - 1] * (pict.rectFull.dy >> outcspInfo->shiftY[i - 1]);
        }
    }

    if (convert->m_wasChange) {
        vramBenchmark.onChange();
    }
    uint64_t cspret = convert->convert(pict,
                                       (dstcsp ^ (cfg->flip ? FF_CSP_FLAGS_VFLIP : 0)) | ((!pict.film && (pict.fieldtype & FIELD_TYPE::MASK_INT)) ? FF_CSP_FLAGS_INTERLACED : 0),
                                       cspdst,
                                       cspstride,
                                       vramBenchmark.get_vram_indirect());
    vramBenchmark.update();

    return cspret ? S_OK : E_FAIL;
}

void TimgFilterOutput::TvramBenchmark::init(void)
{
    vram_indirect = false;
    frame_count = 0;
}

TimgFilterOutput::TvramBenchmark::TvramBenchmark(TimgFilterOutput *Iparent) : parent(Iparent)
{
    init();
}

bool TimgFilterOutput::TvramBenchmark::get_vram_indirect(void)
{
    if (frame_count < BENCHMARK_FRAMES * 2) {
        parent->deciV->get_CurrentTime(&t1);
        return !(frame_count & 1);
    }
    return vram_indirect;
}

void TimgFilterOutput::TvramBenchmark::update(void)
{
    if (frame_count < BENCHMARK_FRAMES * 2) {
        REFERENCE_TIME t_indirect = 0, t_direct = 0;
        unsigned int frame_count_half = frame_count >> 1;
        parent->deciV->get_CurrentTime(&t2);

        if (frame_count & 1) {
            time_on_convert_direct[frame_count_half] = t2 - t1;
        } else {
            time_on_convert_indirect[frame_count_half] = t2 - t1;
        }

        if (frame_count & 1) {
            for (unsigned int i = 0 ; i <= frame_count_half ; i++) {
                t_indirect += time_on_convert_indirect[i];
                t_direct   += time_on_convert_direct[i];
            }

            // Judge the benchmark result.
            // Indirect mode has penalty : 1ms per frame.
            // For the 1st pair, if indirect mode is 5x faster, indirect wins.
            // For the average upto 2nd pair, if indirect mode is 4x faster, indirect wins.
            // ...
            // For the average upto 5th pair, if indirect mode is 1x faster, indirect wins.
            if ((t_indirect + (frame_count_half + 1) * 10000 /* 1ms */) * (BENCHMARK_FRAMES - frame_count_half) < t_direct) {
                frame_count = BENCHMARK_FRAMES * 2; // stop updating, as result is known.
                vram_indirect = true;
            } else {
                vram_indirect = false;
            }
        }
        frame_count++;
        if (frame_count == BENCHMARK_FRAMES * 2) {
            DPRINTF(_l("TimgFilterOutput::process V-RAM access is %s, t_indirect = %I64i, t_direct = %I64i"), vram_indirect ? _l("Indirect") : _l("Direct"), t_indirect, t_direct);
        }
    }
}

HRESULT TimgFilterOutputConvert::process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0)
{
    const ToutputVideoConvertSettings *cfg = (const ToutputVideoConvertSettings*)cfg0;
    init(pict, true, false);
    const unsigned char *src[4];
    getCur(cfg->csp, pict, true, src);
    return parent->processSample(++it, pict);
}
