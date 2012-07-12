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
#include "TaudioFilterLFEcrossover.h"
#include "TlfeCrossoverSettings.h"
#include "firfilter.h"
#include "TfirSettings.h"

TaudioFilterLFEcrossover::TaudioFilterLFEcrossover(IffdshowBase *Ideci, Tfilters *Iparent): TaudioFilter(Ideci, Iparent)
{
    oldfmt.freq = 0;
    oldfreq = 0;
    filter_coefs_lfe = filter_coefs_lfeLR = NULL;
}

void TaudioFilterLFEcrossover::done(void)
{
    if (filter_coefs_lfe) {
        aligned_free(filter_coefs_lfe);
    }
    filter_coefs_lfe = NULL;
    if (filter_coefs_lfeLR) {
        aligned_free(filter_coefs_lfeLR);
    }
    filter_coefs_lfeLR = NULL;
    buf.clear();
}

HRESULT TaudioFilterLFEcrossover::process(TfilterQueue::iterator it, TsampleFormat &fmt, void *samples0, size_t numsamples, const TfilterSettingsAudio *cfg0)
{
    const TlfeCrossoverSettings *cfg = (const TlfeCrossoverSettings*)cfg0;
    if (fmt.channelmask == 0) {
        fmt.channelmask = fmt.makeChannelMask();
    }

    if (oldfmt != fmt || oldfreq != cfg->freq) {
        oldfmt = fmt;
        oldfreq = cfg->freq;
        done();
        li = ri = ci = -1;
        for (unsigned int i = 0; i < fmt.nchannels; i++) {
            if (fmt.speakers[i]&SPEAKER_FRONT_LEFT) {
                li = i;
            } else if (fmt.speakers[i]&SPEAKER_FRONT_RIGHT) {
                ri = i;
            } else if (fmt.speakers[i]&SPEAKER_FRONT_CENTER) {
                ci = i;
            }
        }
        TfirFilter::_ftype_t f = cfg->freq / TfirFilter::_ftype_t(fmt.freq / 2);
        lenLFE = 256;
        filter_coefs_lfe = TfirFilter::design_fir(&lenLFE, &f, TfirSettings::LOWPASS, TfirSettings::WINDOW_HAMMING, 0);
        lenLFElr = 256;
        filter_coefs_lfeLR = TfirFilter::design_fir(&lenLFElr, &f, TfirSettings::HIGHPASS, TfirSettings::WINDOW_HAMMING, 0);
        outfmt = fmt;
        if ((outfmt.channelmask & SPEAKER_LOW_FREQUENCY) == 0) {
            outfmt.setChannels(outfmt.nchannels + 1, outfmt.makeChannelMask() | SPEAKER_LOW_FREQUENCY);
            for (unsigned int i = 0; i < outfmt.nchannels; i++) {
                map[i] = oldfmt.findSpeaker(outfmt.speakers[i]);
            }
        }
        lfei = outfmt.findSpeaker(SPEAKER_LOW_FREQUENCY);
        lfe_pos = 0;
        memset(LFE_buf, 0, sizeof(LFE_buf));
        lfe_posL = 0;
        memset(LFE_bufL, 0, sizeof(LFE_bufL));
        lfe_posR = 0;
        memset(LFE_bufR, 0, sizeof(LFE_bufR));
        lfe_posC = 0;
        memset(LFE_bufC, 0, sizeof(LFE_bufC));
    }

    float *in = (float*)init(cfg, fmt, samples0, numsamples);
    outfmt.sf = fmt.sf;
    float gain = (float)db2value(cfg->gain / 100.0f);
    if ((fmt.channelmask & SPEAKER_LOW_FREQUENCY) == 0) {
        float *out = (float*)(samples0 = alloc_buffer(fmt = outfmt, numsamples, buf));
        for (size_t i = 0; i < numsamples; i++) {
            for (unsigned int ch = 0; ch < fmt.nchannels; ch++)
                if (ch == (unsigned int)lfei) {
                    if (ci != -1 && li != -1 && ri != -1) {
                        LFE_buf[lfe_pos] = (in[li] + in[ri] + in[ci]) * 0.5773502691896258; // sqrt(1/3)
                    } else if (li != -1 && ri != -1) {
                        LFE_buf[lfe_pos] = (in[li] + in[ri])       * 0.7071067811865476; // sqrt(1/2)
                    } else if (ci != -1) {
                        LFE_buf[lfe_pos] = in[ci];
                    } else {
                        break;
                    }

                    out[ch] = gain * TfirFilter::firfilter(LFE_buf, lfe_pos, lenLFE, lenLFE, filter_coefs_lfe);
                    lfe_pos++;
                    if (lfe_pos == lenLFE) {
                        lfe_pos = 0;
                    }
                } else if (cfg->cutLR)
                    if (li == map[ch]) {
                        LFE_bufL[lfe_posL] = in[li];
                        out[ch] = TfirFilter::firfilter(LFE_bufL, lfe_posL, lenLFElr, lenLFElr, filter_coefs_lfeLR);
                        lfe_posL++;
                        if (lfe_posL == lenLFElr) {
                            lfe_posL = 0;
                        }
                    } else if (ri == map[ch]) {
                        LFE_bufR[lfe_posR] = in[ri];
                        out[ch] = TfirFilter::firfilter(LFE_bufR, lfe_posR, lenLFElr, lenLFElr, filter_coefs_lfeLR);
                        lfe_posR++;
                        if (lfe_posR == lenLFElr) {
                            lfe_posR = 0;
                        }
                    } else if (ci == map[ch]) {
                        LFE_bufC[lfe_posC] = in[ci];
                        out[ch] = TfirFilter::firfilter(LFE_bufC, lfe_posC, lenLFElr, lenLFElr, filter_coefs_lfeLR);
                        lfe_posC++;
                        if (lfe_posC == lenLFElr) {
                            lfe_posC = 0;
                        }
                    } else {
                        out[ch] = in[map[ch]];
                    }
                else {
                    out[ch] = in[map[ch]];
                }
            in += oldfmt.nchannels;
            out += fmt.nchannels;
        }
    } else {
        float *out = (float*)(samples0 = in);
        for (size_t i = 0; i < numsamples; i++) {
            if (ci != -1 && li != -1 && ri != -1) {
                LFE_buf[lfe_pos] = (in[li] + in[ri] + in[ci]) * 0.5773502691896258; // sqrt(1/3)
            } else if (li != -1 && ri != -1) {
                LFE_buf[lfe_pos] = (in[li] + in[ri])       * 0.7071067811865476; // sqrt(1/2)
            } else if (ci != -1) {
                LFE_buf[lfe_pos] = in[ci];
            } else {
                break;
            }
            out[lfei] = (out[lfei] + gain * TfirFilter::firfilter(LFE_buf, lfe_pos, lenLFE, lenLFE, filter_coefs_lfe)) * 0.7071067811865476; // sqrt(1/2)
            if (cfg->cutLR) {
                if (li != -1) {
                    LFE_bufL[lfe_posL] = in[li];
                    out[li] = TfirFilter::firfilter(LFE_bufL, lfe_posL, lenLFElr, lenLFElr, filter_coefs_lfeLR);
                    lfe_posL++;
                    if (lfe_posL == lenLFElr) {
                        lfe_posL = 0;
                    }
                }
                if (ri != -1) {
                    LFE_bufR[lfe_posR] = in[ri];
                    out[ri] = TfirFilter::firfilter(LFE_bufR, lfe_posR, lenLFElr, lenLFElr, filter_coefs_lfeLR);
                    lfe_posR++;
                    if (lfe_posR == lenLFElr) {
                        lfe_posR = 0;
                    }
                }
                if (ci != -1) {
                    LFE_bufC[lfe_posC] = in[ci];
                    out[ci] = TfirFilter::firfilter(LFE_bufC, lfe_posC, lenLFElr, lenLFElr, filter_coefs_lfeLR);
                    lfe_posC++;
                    if (lfe_posC == lenLFElr) {
                        lfe_posC = 0;
                    }
                }
            }
            lfe_pos++;
            if (lfe_pos == lenLFE) {
                lfe_pos = 0;
            }
            in += oldfmt.nchannels;
            out += fmt.nchannels;
        }
    }

    return parent->deliverSamples(++it, fmt, samples0, numsamples);
}

void TaudioFilterLFEcrossover::onSeek(void)
{
    lfe_pos = 0;
    memset(LFE_buf, 0, sizeof(LFE_buf));
    lfe_posL = 0;
    memset(LFE_bufL, 0, sizeof(LFE_bufL));
    lfe_posR = 0;
    memset(LFE_bufR, 0, sizeof(LFE_bufR));
    lfe_posC = 0;
    memset(LFE_bufC, 0, sizeof(LFE_bufC));
}
