/*
 * Copyright (c) 2002-2006 Milan Cutka
 * Levels table calculation from Avisynth v1.0 beta. Copyright 2000 Ben Rudiak-Gould
 * Ylevels by Didée
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
#include "TimgFilterLevels.h"
#include "IffdshowBase.h"
#include "TrgbPrimaries.h"

TimgFilterLevels::TimgFilterLevels(IffdshowBase *Ideci, Tfilters *Iparent):
    TimgFilter(Ideci, Iparent),
    timer(L"Level filter cost :")
{
    oldSettings.inMin = -1;
    oldMode = 0;
    resetHistory();
}

bool TimgFilterLevels::is(const TffPictBase &pict, const TfilterSettingsVideo *cfg0)
{
    const TlevelsSettings *cfg = (const TlevelsSettings*)cfg0;
    return super::is(pict, cfg) && (cfg->mode == 5 || cfg->inAuto || cfg->inMin != 0 || cfg->inMax != 255 || cfg->outMin != 0 || cfg->outMax != 255 || cfg->gamma != 100);
}

HRESULT TimgFilterLevels::process(TfilterQueue::iterator it, TffPict &pict, const TfilterSettingsVideo *cfg0)
{
    const TlevelsSettings *cfg = (const TlevelsSettings*)cfg0;
    const unsigned char *srcY = NULL, *srcU = NULL, *srcV = NULL;
    int incsps = supportedcsps;
    int outputLevelsMode = TrgbPrimaries::Invalid_RGB_range;
    if (cfg->forceRGB) {
        incsps = FF_CSP_RGB32;
        outputLevelsMode = deci->getParam2(IDFF_cspOptionsOutputLevelsMode);
    }

    if (flag_resetHistory || cfg->mode != oldMode) {
        flag_resetHistory = false;
        oldMode = cfg->mode;
        if (cfg->inAuto) {
            if (cfg->mode != 6) {
                inMin = 0;
                memsetd(inMins, inMin, sizeof(inMins));
                inMinSum = inMin;
                inMax = 255;
                memsetd(inMaxs, inMax, sizeof(inMaxs));
                inMaxSum = inMax * HISTORY;
                minMaxPos = 0;
            } else {
                inMin = cfg->inMin;
                inMax = cfg->inMax;
                inMinSum = 0;
                inMaxSum = 0;
            }
        } else {
            inMin = 0;
            memsetd(inMins, inMin, sizeof(inMins));
            inMinSum = inMin;
            inMax = 255;
            memsetd(inMaxs, inMax, sizeof(inMaxs));
            inMaxSum = inMax * HISTORY;
            minMaxPos = 0;
        }
    }

    if (cfg->inAuto || (deci->getParam2(IDFF_buildHistogram) && deci->getCfgDlgHwnd())) {
        if (cfg->inAuto) {
            init(pict, cfg->full, cfg->half);
            getCur(incsps, pict, cfg->full, &srcY, &srcU, &srcV, NULL);
        }
        CAutoLock lock(&csHistogram);
        pict.histogram(histogram, cfg->full, cfg->half);
    }
    if (cfg->inAuto) {
        if (cfg->mode != 6) {
            int newInMin, newInMax;

            unsigned int l = pictRect.dx * pictRect.dy / 7000;
            for (newInMin = 0; newInMin < 250 && histogram[newInMin] < l; newInMin++) {
                ;
            }
            for (newInMax = 255; newInMax > newInMin + 1 && histogram[newInMax] < l; newInMax--) {
                ;
            }
            int diff = newInMax - newInMin;
            if (diff < 40) {
                newInMin = limit(newInMin - 20, 0, 254);
                newInMax = limit(newInMax + 20, 1, 255);
            }
            inMinSum -= inMins[minMaxPos];
            inMins[minMaxPos] = newInMin;
            inMinSum += newInMin;
            inMaxSum -= inMaxs[minMaxPos];
            inMaxs[minMaxPos] = newInMax;
            inMaxSum += newInMax;
            minMaxPos++;
            if (minMaxPos == HISTORY) {
                minMaxPos = 0;
            }
            inMin = inMinSum / HISTORY;
            inMax = inMaxSum / HISTORY;
        } else {
            // mode==6
            unsigned int threshold = pictRect.dx * pictRect.dy * (cfg->Ythreshold) / 1000;

            unsigned int countMin, countMax;
            int x;

            for (countMin = 0, x = 0; x < inMin; countMin += histogram[x], x++) {
                ;
            }
            for (countMax = 0, x = 255; x > inMax; countMax += histogram[x], x--) {
                ;
            }

            if (countMin > threshold) {
                if (inMinSum > cfg->Ytemporal) {
                    if (inMin > cfg->inMin - cfg->YmaxDelta) {
                        inMin--;
                    }
                    inMinSum = 0;
                } else {
                    inMinSum++;
                }
            } else {
                inMinSum = 0;
            }

            if (countMax > threshold) {
                if (inMaxSum > cfg->Ytemporal) {
                    if (inMax < cfg->inMax + cfg->YmaxDelta) {
                        inMax++;
                    }
                    inMaxSum = 0;
                } else {
                    inMaxSum++;
                }
            } else {
                inMaxSum = 0;
            }
        }

        //DPRINTF("min:%i max:%i\n",inMin,inMax);
    } else {
        inMin = cfg->inMin;
        inMax = cfg->inMax;
    }
    if (cfg->is && (cfg->mode == 5 || cfg->inAuto || inMin != 0 || inMax != 255 || cfg->outMin != 0 || cfg->outMax != 255 || cfg->gamma != 100 || cfg->posterize != 255)) {
        TautoPerformanceCounter autoTimer(&timer);
        bool equal = cfg->equal(oldSettings);
        if (cfg->inAuto || !equal) {
            oldSettings = *cfg;
            oldSettings.calcMap(map, mapc, inMin, inMax, outputLevelsMode);
        }
        if (!srcY) {
            init(pict, cfg->full, cfg->half);
            getCur(incsps, pict, cfg->full, &srcY, &srcU, &srcV, NULL);
        }
        unsigned char *dstY;
        unsigned char *dstU, *dstV;
        getCurNext(incsps, pict, cfg->full, COPYMODE_NO, &dstY, &dstU, &dstV, NULL);
        if (oldSettings.onlyLuma) {
            for (unsigned int y = 0; y < dy1[0]; y++) {
                const unsigned char *src;
                unsigned char *dst, *dstEnd;
                for (src = srcY + stride1[0] * y, dst = dstY + stride2[0] * y, dstEnd = dst + dx1[0]; dst < dstEnd - 3; src += 4, dst += 4) {
                    *(unsigned int*)dst = (map[src[3]] << 24) | (map[src[2]] << 16) | (map[src[1]] << 8) | map[src[0]];
                }
                for (; dst != dstEnd; src++, dst++) {
                    *dst = uint8_t(map[*src]);
                }
            }
        } else {
            if ((pict.csp & FF_CSPS_MASK) == FF_CSP_420P) {
                filter<FF_CSP_420P>(srcY, srcU, srcV, dstY, dstU, dstV);
            } else if ((pict.csp & FF_CSPS_MASK) == FF_CSP_422P) {
                filter<FF_CSP_422P>(srcY, srcU, srcV, dstY, dstU, dstV);
            } else if ((pict.csp & FF_CSPS_MASK) == FF_CSP_444P) {
                filter<FF_CSP_444P>(srcY, srcU, srcV, dstY, dstU, dstV);
            } else {
                filterRGB32(srcY, srcU, srcV, dstY, dstU, dstV);
            }
        }
    }
    return parent->processSample(++it, pict);
}

inline uint8_t TimgFilterLevels::getuv(int u, int lumav)
{
    return uint8_t(limit(((u - 128) * mapc[lumav] >> 14) + 128, 16, 240));
}

void TimgFilterLevels::filterRGB32(const uint8_t *srcY, const uint8_t *srcU, const uint8_t *srcV,
                                   uint8_t *dstY, uint8_t *dstU, uint8_t *dstV)
{
    for (unsigned int y = 0 ; y < dy1[0] ; y ++) {
        const uint8_t *srcy = srcY + stride1[0] * y;
        uint32_t *dsty = (uint32_t*)(dstY + stride2[0] * y);
        int xcount = dx1[0];
        while (xcount--) {
            int r = srcy[0];
            int g = srcy[1];
            int b = srcy[2];
            srcy += 4;
            r = map[r];
            g = map[g];
            b = map[b];
            uint32_t rgb = 0xff000000 | (b << 16) | (g << 8) | r;
            *dsty = rgb;
            dsty++;
        }
    }
}

template <uint64_t incsp> void TimgFilterLevels::filter(
    const uint8_t *srcY, const uint8_t *srcU, const uint8_t *srcV,
    uint8_t *dstY, uint8_t *dstU, uint8_t *dstV)
{
    for (unsigned int y = 0 ; y < dy1[0] ; y += 2) {
        int halfy = y;
        if (incsp == FF_CSP_420P) {
            halfy >>= 1;
        }
        const uint8_t *srcy = srcY + stride1[0] * y;
        const uint8_t *srcu = srcU + stride1[1] * halfy;
        const uint8_t *srcv = srcV + stride1[1] * halfy;
        uint8_t *dsty = dstY + stride2[0] * y;
        uint8_t *dstu = dstU + stride2[1] * halfy;
        uint8_t *dstv = dstV + stride2[1] * halfy;
        int xcount = dx1[1];
        while (xcount--) {
            int u = *srcu;
            int v = *srcv;
            int lum00 = *srcy, lum01;
            if (incsp != FF_CSP_444P) {
                lum01 = *(srcy + 1);
            }
            int lum10 = *(srcy + stride1[0]), lum11;
            if (incsp != FF_CSP_444P) {
                lum11 = *(srcy + stride1[0] + 1);
            }

            *dsty = uint8_t(map[lum00]);

            if (incsp != FF_CSP_444P) {
                *(dsty + 1) = uint8_t(map[lum01]);
            }

            *(dsty + stride2[0]) = uint8_t(map[lum10]);

            if (incsp != FF_CSP_444P) {
                *(dsty + stride2[0] + 1) = uint8_t(map[lum11]);
            }

            if (incsp == FF_CSP_420P) {
                int lumav = (lum00 + lum01 + lum10 + lum11) >> 2;
                *dstu++ = getuv(u, lumav);
                *dstv++ = getuv(v, lumav);
                srcu++;
                srcv++;
            } else if (incsp == FF_CSP_422P) {
                int lumav = (lum00 + lum01) >> 1;
                *dstu = getuv(u, lumav);
                *dstv = getuv(v, lumav);
                lumav = (lum10 + lum11) >> 1;
                u = *(srcu + stride1[1]);
                v = *(srcv + stride1[1]);
                *(dstu++ + stride2[1]) = getuv(u, lumav);
                *(dstv++ + stride2[1]) = getuv(v, lumav);
                srcu++;
                srcv++;
            } else if (incsp == FF_CSP_444P) {
                *dstu = getuv(u, lum00);
                *dstv = getuv(v, lum00);
                u = *(srcu + stride1[1]);
                v = *(srcv + stride1[1]);
                *(dstu++ + stride2[1]) = getuv(u, lum10);
                *(dstv++ + stride2[1]) = getuv(v, lum10);
                srcu++;
                srcv++;
            }
            if (incsp == FF_CSP_444P) {
                srcy++;
                dsty++;
            } else {
                srcy += 2;
                dsty += 2;
            }
        }
    }
}

void TimgFilterLevels::resetHistory(void)
{
    flag_resetHistory = true;
}
void TimgFilterLevels::onSeek(void)
{
    resetHistory();
}

STDMETHODIMP TimgFilterLevels::getHistogram(unsigned int dst[256])
{
    CAutoLock lock(&csHistogram);
    memcpy(dst, histogram, sizeof(histogram));
    return S_OK;
}
STDMETHODIMP TimgFilterLevels::getInAuto(int *min, int *max)
{
    *min = inMin;
    *max = inMax;
    return S_OK;
}

HRESULT TimgFilterLevels::queryInterface(const IID &iid, void **ptr) const
{
    if (iid == IID_IimgFilterLevels) {
        *ptr = (IimgFilterLevels*)this;
        return S_OK;
    } else {
        return E_NOINTERFACE;
    }
}
