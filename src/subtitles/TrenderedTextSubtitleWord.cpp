/*
 * Copyright (c) 2002-2006 Milan Cutka
 *               2007-2011 h.yamagata
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
#include "Tconfig.h"
#include "CPolygon.h"
#include "TfontSettings.h"
#include "simd.h"
#include "Tsubreader.h"
#include "SeparableFilter.h"
#include "nmTextSubtitles.h"
#pragma warning(disable:4244)

// custom copy constructor for karaoke
TrenderedTextSubtitleWord::TrenderedTextSubtitleWord(
    const TrenderedTextSubtitleWord &parent,
    secondaryColor_t):
    Rasterizer(parent)
{
    *this = parent;
    secondaryColoredWord = NULL;
    m_pOpaqueBox = NULL;
    bmp[0]     = aligned_calloc3<uint8_t>(dx[0], dy[0], 16);
    bmp[1]     = aligned_calloc3<uint8_t>(dx[1], dy[1], 16);
    outline[0] = aligned_calloc3<uint8_t>(dx[0], dy[0], 16);
    outline[1] = aligned_calloc3<uint8_t>(dx[1], dy[1], 16);
    shadow[0]  = aligned_calloc3<uint8_t>(dx[0], dy[0], 16);
    shadow[1]  = aligned_calloc3<uint8_t>(dx[1], dy[1], 16);
    msk[0]     = aligned_calloc3<uint8_t>(dx[0], dy[0], 16);
    memcpy(msk[0], parent.msk[0], dx[0] * dy[0]);

    if (mprops.karaokeMode == TSubtitleProps::KARAOKE_ko_opaquebox) {
        memcpy(shadow[0] , parent.shadow[0] , dx[0] * dy[0]);
        memcpy(shadow[1] , parent.shadow[1] , dx[1] * dy[1]);
        mprops.outlineYUV.A = 0;
    } else if (mprops.karaokeMode == TSubtitleProps::KARAOKE_ko) {
        memcpy(bmp[0], parent.bmp[0], dx[0] * dy[0]);
        memcpy(bmp[1], parent.bmp[1], dx[1] * dy[1]);
        m_outlineWidth = 0;
        mprops.outlineWidth = 0;
        mprops.outlineYUV.A = 0;
        createShadow();
    } else {
        memcpy(bmp[0], parent.bmp[0], dx[0] * dy[0]);
        memcpy(bmp[1], parent.bmp[1], dx[1] * dy[1]);
        memcpy(outline[0], parent.outline[0], dx[0] * dy[0]);
        memcpy(outline[1], parent.outline[1], dx[1] * dy[1]);
        memcpy(shadow[0] , parent.shadow[0] , dx[0] * dy[0]);
        memcpy(shadow[1] , parent.shadow[1] , dx[1] * dy[1]);
    }

    if (parent.msk[1]) {
        msk[1] = aligned_calloc3<uint8_t>(dx[1], dy[1], 16);
        memcpy(msk[1], parent.msk[1], dx[1] * dy[1]);
    }

    mprops.bodyYUV = YUVcolorA(mprops.SecondaryColour, mprops.SecondaryColourA);
    m_bitmapReady = true;
}

// full rendering
TrenderedTextSubtitleWord::TrenderedTextSubtitleWord(
    HDC hdc,
    const wchar_t *s0,
    size_t strlens,
    const TprintPrefs &prefs,
    LOGFONT lf,
    TSubtitleProps props):
    secondaryColoredWord(NULL),
    m_pOpaqueBox(NULL),
    oldBodyYUVa(256),
    oldOutlineYUVa(256),
    overhang(0, 0, 0, 0),
    m_outlineWidth(0),
    m_baseline(0),
    m_ascent(0),
    m_descent(0),
    m_bitmapReady(false),
    mprops(props, prefs)
{
    if (!*s0) {
        return;
    }

    int gdi_font_scale = mprops.gdi_font_scale;

    init();

    csp = prefs.csp & FF_CSPS_MASK;

    m_outlineWidth = 1;
    outlineWidth_double = mprops.outlineWidth;

    if (!mprops.opaqueBox) {
        // Improve YV12 rendering and remove these ugly workaround.
        if (csp == FF_CSP_420P && outlineWidth_double < 0.6 && !mprops.bodyYUV.isGray()) {
            m_outlineWidth = 1;
            outlineWidth_double = 0.6;
            mprops.outlineYUV = 0;
        } else {
            m_outlineWidth = ceil(outlineWidth_double);
        }
    } else {
        m_outlineWidth = 0;
    }

    strings tab_parsed_string;
    strtok(ffstring(s0, strlens).c_str(), L"\t", tab_parsed_string);
    SIZE sz;
    sz.cx = sz.cy = 0;
    ints cxs;
    for (strings::iterator s = tab_parsed_string.begin(); s != tab_parsed_string.end(); s++) {
        SIZE sz0;
        GetTextExtentPoint32W(hdc, s->c_str(), (int)s->size(), &sz0);
        sz.cx += sz0.cx + mprops.calculated_spacing * int(s->size());
        if (s + 1 != tab_parsed_string.end()) {
            int tabsize = prefs.tabsize * sz0.cy;
            int newpos = (sz.cx / tabsize + 1) * tabsize;
            sz0.cx += newpos - sz.cx;
            sz.cx = newpos;
        }
        cxs.push_back(sz0.cx);
        sz.cy = std::max(sz.cy, sz0.cy);
    }

    // mix ScaleX and aspect ratio
    double xscale = mprops.scaleX;
    if (mprops.sar.num > mprops.sar.den) {
        xscale = xscale * mprops.sar.den / mprops.sar.num;
    }

    dxChar  = xscale * sz.cx / gdi_font_scale;
    dyChar  = mprops.scaleY * sz.cy / gdi_font_scale;
    calcOutlineTextMetric(hdc, sz, lf);

    if (gdi_font_scale == 4) {
        drawGlyphOSD(hdc, tab_parsed_string, cxs, xscale); // sharp and fast, good for OSD.
        postRasterisation();
        m_bitmapReady = true;
    } else {
        getPath(hdc, tab_parsed_string, cxs);  // anti aliased, good for subtitles.
        m_sar = (double)mprops.sar.den / mprops.sar.num;
        // Bitmaps are not ready.
        // We return here because the axis of rotation is unknown at this time.
        // Let the caller call again when the axix is known.
    }
}

// As a base class of CPolygon
TrenderedTextSubtitleWord::TrenderedTextSubtitleWord(const TSubtitleMixedProps &Improps):
    secondaryColoredWord(NULL),
    oldBodyYUVa(256),
    oldOutlineYUVa(256),
    overhang(0, 0, 0, 0),
    m_outlineWidth(0),
    m_baseline(0),
    m_ascent(0),
    m_descent(0),
    m_bitmapReady(false),
    mprops(Improps),
    m_pOpaqueBox(NULL)
{
    csp = mprops.csp;

    outlineWidth_double = mprops.outlineWidth;

    m_outlineWidth = ceil(mprops.outlineWidth);

    dyChar = 0;
    m_sar = (double)mprops.sar.den / mprops.sar.num;

    init();
}

void TrenderedTextSubtitleWord::init()
{
    if (Tconfig::cpu_flags & FF_CPU_MMXEXT) {
        YV12_lum2chr_min = YV12_lum2chr_min_mmx2;
        YV12_lum2chr_max = YV12_lum2chr_max_mmx2;
    } else {
        YV12_lum2chr_min = YV12_lum2chr_min_mmx;
        YV12_lum2chr_max = YV12_lum2chr_max_mmx;
    }
#ifndef WIN64
    if (Tconfig::cpu_flags & FF_CPU_SSE2) {
#endif
        alignXsize = 16;
#ifndef WIN64
    } else {
        alignXsize = 8;
    }
#endif
}

void TrenderedTextSubtitleWord::calcOutlineTextMetric(HDC hdc,
        SIZE italic_fixed_sz,
        const LOGFONT &lf)
{
    double gdi_font_scale = mprops.gdi_font_scale;
    OUTLINETEXTMETRIC otm;
    if (GetOutlineTextMetrics(hdc, sizeof(otm), &otm)) {
        m_shadowSize = getShadowSize(otm.otmTextMetrics.tmHeight);
        overhang = computeOverhang();
        m_ascent  = otm.otmTextMetrics.tmAscent / gdi_font_scale;
        m_descent = otm.otmTextMetrics.tmDescent / gdi_font_scale;
        if (otm.otmItalicAngle) {
            italic_fixed_sz.cx += ff_abs(LONG(italic_fixed_sz.cy * sin(otm.otmItalicAngle * M_PI / 1800)));
        } else if (otm.otmTextMetrics.tmItalic) {
            italic_fixed_sz.cx += italic_fixed_sz.cy * 0.35;
        }
    } else {
        // non true-type
        m_shadowSize = getShadowSize(lf.lfHeight);
        overhang = computeOverhang();
        m_ascent   = italic_fixed_sz.cy * 0.8 / gdi_font_scale;
        m_descent  = italic_fixed_sz.cy * 0.2 / gdi_font_scale;
        if (lf.lfItalic) {
            italic_fixed_sz.cx += italic_fixed_sz.cy * 0.35;
        }
    }

    m_ascent *= mprops.scaleY;
    m_descent *= mprops.scaleY;
    m_baseline = m_ascent;

    gdi_dx = ((italic_fixed_sz.cx + gdi_font_scale) / gdi_font_scale + 1) * gdi_font_scale;
    gdi_dy = italic_fixed_sz.cy + gdi_font_scale;
}

void TrenderedTextSubtitleWord::getPath(
    HDC hdc,
    const strings &tab_parsed_string,
    const ints &cxs)
{
    bool bFirstPath = true;
    int x = 0;
    ints::const_iterator cx = cxs.begin();
    foreach(const ffstring & s, tab_parsed_string) {
        if (mprops.calculated_spacing == 0) {
            PartialBeginPath(hdc, bFirstPath);
            bFirstPath = false;
            TextOutW(hdc, 0, 0, s.c_str(), (int)s.size());
            PartialEndPath(hdc, x, 0);
            x += *cx;
            cx++;
        } else {
            for (unsigned int i = 0; i < s.size(); i++) {
                CSize extent;
                GetTextExtentPoint32W(hdc, s.c_str() + i, 1, &extent);

                PartialBeginPath(hdc, bFirstPath);
                bFirstPath = false;
                TextOutW(hdc, 0, 0, s.c_str() + i, 1);
                PartialEndPath(hdc, x, 0);
                x += extent.cx + mprops.calculated_spacing;
            }
        }
    }
}

void TrenderedTextSubtitleWord::Transform(CPoint org)
{
    /*
     *  Copied and modified from guliverkli, RTS.cpp
     *
     *  Copyright (C) 2003-2006 Gabest
     *  http://www.gabest.org
     */

    if (mprops.angleX == 0 && mprops.angleY == 0 && mprops.angleZ == 0 && mprops.scaleX == 1.0 && m_sar == 1.0 && mprops.scaleY == 1.0) {
        return;
    }

    double caz = cos((3.1415 / 180) * mprops.angleZ);
    double saz = sin((3.1415 / 180) * mprops.angleZ);
    double cax = cos((3.1415 / 180) * mprops.angleX);
    double sax = sin((3.1415 / 180) * mprops.angleX);
    double cay = cos((3.1415 / 180) * mprops.angleY);
    double say = sin((3.1415 / 180) * mprops.angleY);
    double ScaleX = mprops.scaleX;
    double ScaleY = mprops.scaleY;

    for (int i = 0; i < mPathPoints; i++) {
        double x, y, z, xx, yy, zz;

        x = ScaleX * (mpPathPoints[i].x /*+ m_style.fontShiftX * mpPathPoints[i].y*/) - org.x;
        y = ScaleY * (mpPathPoints[i].y /*+ m_style.fontShiftY * mpPathPoints[i].x*/) - org.y;
        z = 0;

        xx = x * caz + y * saz;
        yy = -(x * saz - y * caz);
        zz = z;

        x = xx;
        y = yy * cax + zz * sax;
        z = yy * sax - zz * cax;

        xx = x * cay + z * say;
        yy = y;
        zz = x * say - z * cay;

        zz = std::max<double>(zz, -19000);

        x = (xx * 20000) / (zz + 20000) * m_sar;
        y = (yy * 20000) / (zz + 20000);

        mpPathPoints[i].x = (LONG)(x + org.x * m_sar + 0.5);
        mpPathPoints[i].y = (LONG)(y + org.y + 0.5);
    }
}

void TrenderedTextSubtitleWord::drawGlyphOSD(
    HDC hdc,
    const strings &tab_parsed_string,
    const ints &cxs,
    double xscale)
{
    RECT r = {0, 0, gdi_dx, gdi_dy};
    uint8_t *bmp16 = aligned_calloc3<uint8_t>(gdi_dx * size_of_rgb32, gdi_dy, 32);
    HBITMAP hbmp = CreateCompatibleBitmap(hdc, gdi_dx, gdi_dy);
    HGDIOBJ old = SelectObject(hdc, hbmp);
    FillRect(hdc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkColor(hdc, RGB(0, 0, 0));

    int x = 2;
    const int gdi_font_scale = 4;
    ints::const_iterator cx = cxs.begin();
    foreach(const ffstring & s, tab_parsed_string) {
        if (mprops.calculated_spacing == 0) {
            TextOutW(hdc, x, 2, s.c_str(), (int)s.size());
            x += *cx;
            cx++;
        } else {
            for (unsigned int i = 0; i < s.size(); i++) {
                CSize extent;
                GetTextExtentPoint32W(hdc, s.c_str() + i, 1, &extent);

                TextOutW(hdc, x, 2, s.c_str() + i, 1);
                PartialEndPath(hdc, x, 0);
                x += extent.cx + mprops.calculated_spacing;
            }
        }
    }

    BITMAPINFO bmi;
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = gdi_dx;
    bmi.bmiHeader.biHeight = -1 * gdi_dy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = gdi_dx * gdi_dy;
    bmi.bmiHeader.biXPelsPerMeter = 75;
    bmi.bmiHeader.biYPelsPerMeter = 75;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;
    GetDIBits(hdc, hbmp, 0, gdi_dy, bmp16, &bmi, DIB_RGB_COLORS); // copy bitmap, get it in bmp16 (RGB32).
    SelectObject(hdc, old);
    DeleteObject(hbmp);

    unsigned int tmpdx = xscale * gdi_dx / gdi_font_scale + overhang.left + overhang.right;
    dx[0] = ((tmpdx + 15) / 16) * 16;
    dy[0]    = gdi_dy / gdi_font_scale + overhang.top + overhang.bottom;
    m_baseline = m_baseline / gdi_font_scale + 2;

    overhang.right += dx[0] - tmpdx;
    if (dx[0] < 16) {
        dx[0] = 16;
    }
    if (csp == FF_CSP_420P) {
        dy[0] = ((dy[0] + 1) / 2) * 2;
    }
    mGlyphBmpWidth = dx[0] + m_outlineWidth * 2; // add margin to simplify the outline drawing process.
    mGlyphBmpHeight = dy[0] + m_outlineWidth * 2;
    mGlyphBmpWidth = ((mGlyphBmpWidth + 7) / 8) * 8;
    bmp[0] = aligned_calloc3<uint8_t>(dx[0], dy[0], 16);

    // Here we scale to 1/gdi_font_scale.
    // average 4x5 pixels and store it in 6bit (max 64, not 63)
    unsigned int xstep = xscale == 1 ?
                         gdi_font_scale * 65536 :
                         gdi_font_scale * 65536 / xscale;
    unsigned int gdi_rendering_window_width = std::max<unsigned int>(
                xscale == 1 ?
                4 :
                4 * 1 / xscale
                , 1);
    // coeff calculation
    // To averave gdi_rendering_window_width * 5 pixels
    unsigned int coeff;
    if (gdi_rendering_window_width == 4) {
        coeff = 824;    // 65536/ 4 / (4 * 5) plus bit to make 63.xxx->64;
    } else {
        coeff = 65536 / 4 / (gdi_rendering_window_width * 5);
    }
    int dx0_mult_4 = gdi_dx * size_of_rgb32;
    unsigned int xstep_sse2 = xstep * 8;
    unsigned int startx = (2 << 16) + xstep;
    unsigned int endx = (gdi_dx - 2) << 16;
    for (unsigned int y = 2 ; y < gdi_dy - 3 ; y += gdi_font_scale) {
        unsigned char *dstBmpY = bmp[0] + (y / gdi_font_scale + overhang.top) * dx[0] + overhang.left;
        unsigned int x = startx;
        const unsigned char *bmp16srcLineStart = bmp16 + ((y - 2) * gdi_dx) * size_of_rgb32;
        const unsigned char *bmp16srcEnd;
        bmp16srcEnd = bmp16srcLineStart + 5 * dx0_mult_4;
        for (; x < endx ; x += xstep, dstBmpY++) {
            unsigned int sum;
            const unsigned char *bmp16src = bmp16srcLineStart + ((x >> 16) - 2) * size_of_rgb32;

            if (xscale == 1) {
                sum = 0;
                for (; bmp16src < bmp16srcEnd ; bmp16src += dx0_mult_4) {
                    // a bit of optimization: Only one if block will be compiled. Loops are unrolled.
                    sum += bmp16src[0] + bmp16src[4] + bmp16src[8] + bmp16src[12];
                }

                sum = (sum * coeff) >> 16;
                *dstBmpY = (unsigned char)std::min<unsigned int>(sum, 255);
            } else {
                sum = 0;
                for (; bmp16src < bmp16srcEnd ; bmp16src += dx0_mult_4) {
                    for (unsigned int i = 0 ; i < size_of_rgb32 * gdi_rendering_window_width ; i += size_of_rgb32) {
                        sum += bmp16src[i];
                    }
                }
                sum = (sum * coeff) >> 16;
                *dstBmpY = (unsigned char)std::min<unsigned int>(sum, 255);
            }
        }
    }
    aligned_free(bmp16);
}

void TrenderedTextSubtitleWord::createOpaquBox(const CPointDouble &bodysLeftTop)
{
    TSubtitleMixedProps boxProps = mprops;
    boxProps.polygon = 4;
    boxProps.opaqueBox = 0;
    boxProps.outlineWidth = 0;
    boxProps.bodyYUV = mprops.outlineYUV;
    boxProps.colorA = mprops.OutlineColourA;
    if (boxProps.karaokeMode == TSubtitleProps::KARAOKE_ko) {
        boxProps.karaokeMode = TSubtitleProps::KARAOKE_ko_opaquebox;
    } else {
        boxProps.karaokeMode = TSubtitleProps::KARAOKE_NONE;
    }
    wchar_t str[256];
    double w = mprops.outlineWidth * 8;
    double h = w;

    tsnprintf_s(str, countof(str), _TRUNCATE, L"m %d %d l %d %d %d %d %d %d",
                int(-w), int(-h),
                int(dxChar * 8 + w), int(-h),
                int(dxChar * 8 + w), int(dyChar * 8 + h),
                int(-w),  int(dyChar * 8 + h));
    m_pOpaqueBox = new CPolygon(boxProps, str);
    m_pOpaqueBox->rasterize(bodysLeftTop);
    m_pOpaqueBox->m_ascent = m_ascent;
    m_pOpaqueBox->m_descent = m_descent;
    m_pOpaqueBox->m_baseline = m_pOpaqueBox->m_ascent;
}

void TrenderedTextSubtitleWord::rasterize(const CPointDouble &bodysLeftTop)
{
    if (m_bitmapReady) { return; }

    CPoint axis = mprops.get_rotationAxis();
    axis.x = (axis.x - bodysLeftTop.x - overhang.left) * mprops.gdi_font_scale / m_sar;
    axis.y = (axis.y - bodysLeftTop.y - overhang.top) * mprops.gdi_font_scale;
    Transform(axis);
    ScanConvert();
    if (!mprops.opaqueBox) {
        if (mprops.hqBorder) {
            CreateWidenedRegion(int(mprops.outlineWidth * mprops.scaleX * 8 + 0.5), int(mprops.outlineWidth * mprops.scaleY * 8 + 0.5));
        }
    } else {
        createOpaquBox(bodysLeftTop);
    }

    bool fCheckRange = (mprops.angleX != 0.0 || mprops.angleY != 0.0 || mprops.angleZ != 0.0)
                       && !mprops.isMove;
    int xsub = bodysLeftTop.x * 8.0 - int(bodysLeftTop.x) * 8;
    int ysub = bodysLeftTop.y * 8.0 - int(bodysLeftTop.y) * 8;
    Rasterizer::Rasterize(xsub, ysub, overhang, mprops.hqBorder, fCheckRange, CPoint(int(bodysLeftTop.x), int(bodysLeftTop.y)), mprops.dx, mprops.dy, csp == FF_CSP_420P);

    dx[0] = mGlyphBmpWidth;
    dy[0] = mGlyphBmpHeight;
    if (bmp[0] == NULL) {
        return;
    }
    postRasterisation();
    m_bitmapReady = true;
}

// \blur
void TrenderedTextSubtitleWord::applyGaussianBlur(unsigned char *src)
{
    // Do some gaussian blur magic
    // code copied from MPC-HC
    if (mprops.gauss > 0.0) {
        GaussianKernel filter(mprops.gauss);
        if ((int)dx[0] >= filter.width && (int)dy[0] >= filter.width) {
            size_t pitch = dx[0];

            byte *tmp = new byte[pitch * dy[0]];
            if (!tmp) {
                return;
            }

            SeparableFilterX<1>(src, tmp, dx[0], dy[0], pitch, filter.kernel, filter.width, filter.divisor);
            SeparableFilterY<1>(tmp, src, dx[0], dy[0], pitch, filter.kernel, filter.width, filter.divisor);

            delete[] tmp;
        }
    }
}

// image filter for \be
unsigned char* TrenderedTextSubtitleWord::blur(unsigned char *src, stride_t Idx, stride_t Idy, int startx, int starty, int endx, int endy, int blurStrength)
{
    /*
     *  Copied and modified from guliverkli, Rasterizer.cpp
     *
     *  Copyright (C) 2003-2006 Gabest
     *  http://www.gabest.org
     */
    unsigned char *dst = aligned_calloc3<uint8_t>(Idx, Idy, 16);
    int sx = startx <= 0 ? 1 : startx;
    int sy = starty <= 0 ? 1 : starty;
    int ex = endx >= Idx ? Idx - 1 : endx;
    int ey = endy >= Idy ? Idy - 1 : endy;

    // 3x3 box blur, with different strength based on the user settings.
    // 1           2           1
    // 2     (2^factor)-12     2
    // 1           2           1 , then divide by 2^factor
    // After applying the filter, the result has to be divided by the sum
    // of all pixel weights. Keep this in mind when adding extra options!

    if (blurStrength < TfontSettings::Extreme) {
        int factor = 0;
        switch (blurStrength) {
            case TfontSettings::Softest:
                factor = 9;
                break;
            case TfontSettings::Softer:
                factor = 8;
                break;
            case TfontSettings::Soft:
                factor = 7;
                break;
            case TfontSettings::Normal:
                factor = 6;
                break;
            case TfontSettings::Strong:
                factor = 5;
                break;
            case TfontSettings::Stronger:
                factor = 4;
                break;
        }

        for (int y = sy ; y < ey ; y++)
            for (int x = sx ; x < ex ; x++) {
                int pos = Idx * y + x;
                unsigned char *srcpos = src + pos;
                dst[pos] = (srcpos[-1 - Idx]   + (srcpos[-Idx] << 1)            +  srcpos[+1 - Idx]
                            + (srcpos[-1] << 1) + (srcpos[0] * ((1 << factor) - 12)) + (srcpos[+1] << 1)
                            +  srcpos[-1 + Idx]   + (srcpos[+Idx] << 1)            +  srcpos[+1 + Idx])  >> factor;
            }
    }

    // The last option (extreme) applies a 3x3 box blur without any pixel weighting
    else if (blurStrength == TfontSettings::Extreme) {
        for (int y = sy ; y < ey ; y++)
            for (int x = sx ; x < ex ; x++) {
                int pos = Idx * y + x;
                unsigned char *srcpos = src + pos;
                dst[pos] = (srcpos[-1 - Idx]  +  srcpos[-Idx]  +  srcpos[+1 - Idx]
                            +  srcpos[-1]      +  srcpos[0]     +  srcpos[+1]
                            +  srcpos[-1 + Idx]  +  srcpos[+Idx]  +  srcpos[+1 + Idx])  / 9;
            }
    }

    if (startx == 0)
        for (int y = starty ; y < endy ; y++) {
            dst[Idx * y] = src[Idx * y];
        }
    if (endx == Idx - 1)
        for (int y = starty ; y < endy ; y++) {
            dst[Idx * y + endx] = src[Idx * y + endx];
        }
    if (starty == 0) {
        memcpy(dst, src, Idx);
    }
    if (endy == Idy - 1) {
        memcpy(dst + Idx * endy, src + Idx * endy, Idx);
    }
    aligned_free(src);
    return dst;
}

// \be main
void TrenderedTextSubtitleWord::apply_beBlur(unsigned char* &src, int blurCount)
{
    if (blurCount) {
        while (blurCount--) {
            src = blur(src, dx[0], dy[0], 0, 0, dx[0], dy[0], mprops.blurStrength);
        }
    }
}

CRect TrenderedTextSubtitleWord::computeOverhang()
{
    int filterWidth = 0;
    if (mprops.gauss > 0) {
        GaussianKernel filter(mprops.gauss);
        filterWidth = filter.width / 2;
    }
    double ow = mprops.outlineWidth;
    if (mprops.opaqueBox) {
        ow = 0;
    }
    int blurCount = mprops.bodyBlurCount + mprops.outlineBlurCount;
    int top = ceil(ow * mprops.scaleY) + filterWidth + blurCount;
    int left = ceil(ow * mprops.scaleX) + filterWidth + blurCount;
    int bottom = top + m_shadowSize;
    int right = left + m_shadowSize;

    if (mprops.shadowMode == TfontSettings::GlowingShadow) {
        top += m_shadowSize;
        left += m_shadowSize;
    }

    return CRect(left, top, right, bottom);
}

void TrenderedTextSubtitleWord::ffCreateWidenedRegion()
{
    // Prepare outline by h.yamagata
    // Faster than CreateWidenedRegion of MPC/MPC-HC with acceptable quality.

    // Prepare matrix for outline calculation
    short *matrix = NULL;
    int owx = std::max<int>(outlineWidth_double * mprops.scaleX, overhang.left);
    unsigned int matrixSizeH = ffalign(owx * 2, 8); // 2 bytes for one.
    unsigned int matrixSizeV = m_outlineWidth * 2 + 1;

    double r_cutoff = 1.5;
    if (outlineWidth_double < 4.5) {
        r_cutoff = outlineWidth_double / 3.0;
    }
    double xc = 1 / mprops.scaleX / mprops.scaleX;
    double r_mul = 512.0 / r_cutoff;
    matrix = (short*)aligned_calloc(matrixSizeH * 2, matrixSizeV, 16);
    for (int y = -m_outlineWidth ; y <= m_outlineWidth ; y++)
        for (int x = -owx ; x <= owx ; x++) {
            int pos = (y + m_outlineWidth) * matrixSizeH + x + owx;
            double r = 0.5 + outlineWidth_double - sqrt(double(x * x * xc + y * y));
            if (r > r_cutoff) {
                matrix[pos] = 512;
            } else if (r > 0) {
                matrix[pos] = r * r_mul;
            }
        }

    unsigned int max_outline_pos_x  = dx[0] - overhang.right + owx;
    unsigned int max_outline_pos_y  = dy[0] - overhang.bottom + m_outlineWidth;
    unsigned int startx = overhang.left - owx;
    unsigned int starty = overhang.top - m_outlineWidth;

    TexpandedGlyph expanded(*this);
    if (Tconfig::cpu_flags & FF_CPU_SSE2
#ifndef WIN64
            && m_outlineWidth >= 2
#endif
       ) {
        size_t matrixSizeH_sse2 = matrixSizeH >> 3;
        size_t srcStrideGap = expanded.dx - matrixSizeH;
        for (unsigned int y = starty ; y < max_outline_pos_y ; y++)
            for (unsigned int x = startx ; x < max_outline_pos_x ; x++) {
                unsigned int sum = fontPrepareOutline_sse2((unsigned char*)expanded + expanded.dx * y + x , srcStrideGap, matrix, matrixSizeH_sse2, matrixSizeV) >> 9;
                msk[0][dx[0]*y + x] = std::min<unsigned int>(sum, 64);
            }
    }
#ifndef WIN64
    else if (Tconfig::cpu_flags & FF_CPU_MMX) {
        size_t matrixSizeH_mmx = (matrixSizeV + 3) / 4;
        size_t srcStrideGap = expanded.dx - matrixSizeH_mmx * 4;
        size_t matrixGap = matrixSizeH_mmx & 1 ? 8 : 0;
        for (unsigned int y = starty ; y < max_outline_pos_y ; y++) {
            for (unsigned int x = startx ; x < max_outline_pos_x ; x++) {
                unsigned int sum = fontPrepareOutline_mmx((unsigned char*)expanded + expanded.dx * y + x, srcStrideGap, matrix, matrixSizeH_mmx, matrixSizeV, matrixGap) >> 9;
                msk[0][dx[0]*y + x] = std::min<unsigned int>(sum, 64);
            }
        }
    }
#endif
    else {
        for (unsigned int y = starty ; y < max_outline_pos_y ; y++)
            for (unsigned int x = startx ; x < max_outline_pos_x ; x++) {
                unsigned char *srcPos = (unsigned char*)expanded + expanded.dx * y + x;
                unsigned int sum = 0;
                for (unsigned int yy = 0; yy < matrixSizeV; yy++, srcPos += mGlyphBmpWidth - matrixSizeV)
                    for (unsigned int xx = 0; xx < matrixSizeV; xx++, srcPos++) {
                        sum += (*srcPos) * matrix[matrixSizeH * yy + xx];
                    }
                sum >>= 9;
                msk[0][dx[0]*y + x] = std::min<unsigned int>(sum, 64);
            }
    }
    if (matrix) {
        aligned_free(matrix);
    }

    _mm_empty();
}

void TrenderedTextSubtitleWord::postRasterisation()
{
    if (!msk[0]) {
        msk[0] = aligned_calloc3<uint8_t>(dx[0], dy[0], 16);
    }
    outline[0] = aligned_calloc3<uint8_t>(dx[0], dy[0], 16);
    shadow[0]  = aligned_calloc3<uint8_t>(dx[0], dy[0], 16);

    if (mprops.opaqueBox || m_outlineWidth == 0) {
        applyGaussianBlur(bmp[0]);
        apply_beBlur(bmp[0], mprops.bodyBlurCount);
        memcpy(msk[0], bmp[0], dx[0] * dy[0]);
    } else {
        // non ASS or style over-ride may blur BODY even if there is outline.
        apply_beBlur(bmp[0], mprops.bodyBlurCount);

        if (!mprops.hqBorder) {
            ffCreateWidenedRegion();
        }

        // blur outlines
        applyGaussianBlur(msk[0]);
        apply_beBlur(msk[0], mprops.outlineBlurCount);
    }

    // Draw outline.
    unsigned int count = dx[0] * dy[0];
    for (unsigned int c = 0; c < count; c++) {
        int b = bmp[0][c];
        int o = msk[0][c] - b;
        if (o > 0) {
            outline[0][c] = o;
        }
    }

    m_shadowMode = mprops.shadowMode;
    // Opaque box's shadow is drawn if this is executed as a base class of CPolygon, when mprops.opaqueBox is set to 0.
    if (mprops.opaqueBox) {
        m_shadowMode = TfontSettings::ShadowDisabled;
    }

    if (csp == FF_CSP_420P) {
        dx[1] = dx[0] >> 1;
        dy[1] = dy[0] >> 1;
        dx[1] = (dx[1] / alignXsize + 1) * alignXsize;
        bmp[1]     = aligned_calloc3<uint8_t>(dx[1], dy[1]);
        outline[1] = aligned_calloc3<uint8_t>(dx[1], dy[1]);
        shadow[1]  = aligned_calloc3<uint8_t>(dx[1], dy[1]);

        dx[2] = dx[0] >> 1;
        dy[2] = dy[0] >> 1;
        dx[2] = (dx[2] / alignXsize + 1) * alignXsize;
    } else {
        //RGB32
        dx[1] = dx[0] * size_of_rgb32;
        dy[1] = dy[0];
        bmp[1]     = aligned_calloc3<uint8_t>(dx[1], dy[1], 16);
        outline[1] = aligned_calloc3<uint8_t>(dx[1], dy[1], 16);
        shadow[1]  = aligned_calloc3<uint8_t>(dx[1], dy[1], 16);
        msk[1]     = aligned_calloc3<uint8_t>(dx[1], dy[1], 16);
    }

    createShadow();
    prepareForColorSpace();

    if (mprops.karaokeMode != TSubtitleProps::KARAOKE_NONE) {
        secondaryColoredWord = new TrenderedTextSubtitleWord(*this, secondaryColor_t());
    }
}

void TrenderedTextSubtitleWord::Y2RGB(unsigned char *const bitmap[3], unsigned int size) const
{
    DWORD *bmpRGB = (DWORD *)bitmap[1];
    const unsigned char *bmpY = bitmap[0];
    for (unsigned int i = size ; i ; bmpRGB += 4, bmpY += 4, i--) {
        *(bmpRGB)      = *bmpY << 16         | *bmpY << 8         | *bmpY;
        *(bmpRGB + 1)    = *(bmpY + 1) << 16     | *(bmpY + 1) << 8     | *(bmpY + 1);
        *(bmpRGB + 2)    = *(bmpY + 2) << 16     | *(bmpY + 2) << 8     | *(bmpY + 2);
        *(bmpRGB + 3)    = *(bmpY + 3) << 16     | *(bmpY + 3) << 8     | *(bmpY + 3);
    }
}

void TrenderedTextSubtitleWord::prepareForColorSpace() const
{
    unsigned int _dx = dx[0];
    unsigned int _dy = dy[0];

    // Preparation for each color space
    if (csp == FF_CSP_420P) {
        int isColorOutline = (mprops.outlineYUV.U != 128 || mprops.outlineYUV.V != 128);
        if (0 && Tconfig::cpu_flags & FF_CPU_MMX || Tconfig::cpu_flags & FF_CPU_MMXEXT) {
            unsigned int edxAlign = (_dx & ~0xf) >> 1;
            unsigned int edx = _dx >> 1;
            for (unsigned int y = 0 ; y < dy[1] ; y++)
                for (unsigned int x = 0 ; x < edx ; x += 8) {
                    if (x >= edxAlign) {
                        x = edx - 8;
                    }
                    unsigned int lum0 = 2 * y * _dx + x * 2;
                    unsigned int lum1 = (2 * y + 1) * _dx + x * 2;
                    unsigned int chr = y * dx[1] + x;
                    YV12_lum2chr_max(&bmp[0][lum0], &bmp[0][lum1], &bmp[1][chr]);
                    if (isColorOutline) {
                        YV12_lum2chr_max(&outline[0][lum0], &outline[0][lum1], &outline[1][chr]);
                    } else {
                        YV12_lum2chr_min(&outline[0][lum0], &outline[0][lum1], &outline[1][chr]);
                    }
                    YV12_lum2chr_min(&shadow[0][lum0], &shadow [0][lum1], &shadow [1][chr]);
                }
        } else {
            unsigned int _dx1 = _dx / 2;
            for (unsigned int y = 0; y < dy[1]; y++)
                for (unsigned int x = 0; x < _dx1; x++) {
                    unsigned int lum0 = 2 * y * _dx + x * 2;
                    unsigned int lum1 = (2 * y + 1) * _dx + x * 2;
                    unsigned int chr = y * dx[1] + x;
                    bmp[1][chr] = std::max(std::max(std::max(bmp[0][lum0], bmp[0][lum0 + 1]), bmp[0][lum1]), bmp[0][lum1 + 1]);
                    if (isColorOutline) {
                        outline[1][chr] = std::max(std::max(std::max(outline[0][lum0], outline[0][lum0 + 1]), outline[0][lum1]), outline[0][lum1 + 1]);
                    } else {
                        outline[1][chr] = std::min(std::min(std::min(outline[0][lum0], outline[0][lum0 + 1]), outline[0][lum1]), outline[0][lum1 + 1]);
                    }
                    shadow[1][chr] = std::min(std::min(std::min(shadow[0][lum0], shadow[0][lum0 + 1]), shadow[0][lum1]), shadow[0][lum1 + 1]);
                }
        }
    } else {
        //RGB32
        unsigned int xy = (_dx * _dy) >> 2;

        Y2RGB(bmp, xy);
        Y2RGB(outline, xy);
        Y2RGB(shadow, xy);
        Y2RGB(msk, xy);
    }
    _mm_empty();
}

void TrenderedTextSubtitleWord::createShadow() const
{
    if (m_shadowSize == 0 || m_shadowMode == TfontSettings::ShadowDisabled) {
        return;
    }

    unsigned char* mskptr;
    if (m_outlineWidth) {
        if (mprops.bodyYUV.A > 1) {
            mskptr = msk[0];
        } else {
            mskptr = outline[0];
        }
    } else {
        mskptr = bmp[0];
    }

    unsigned int _dx = dx[0];
    unsigned int _dy = dy[0];
    unsigned int shadowSize = m_shadowSize;
    unsigned int shadowAlpha = 255;
    if (m_shadowMode == TfontSettings::GlowingShadow) { //Gradient glowing shadow (most complex)
        _mm_empty();
        if (_dx < shadowSize) {
            shadowSize = _dx;
        }
        if (_dy < shadowSize) {
            shadowSize = _dy;
        }
        unsigned int circle[1089]; // 1089=(16*2+1)^2
        if (shadowSize > 16) {
            shadowSize = 16;
        }
        int circleSize = shadowSize * 2 + 1;
        for (int y = 0; y < circleSize; y++) {
            for (int x = 0; x < circleSize; x++) {
                unsigned int rx = ff_abs(x - (int)shadowSize);
                unsigned int ry = ff_abs(y - (int)shadowSize);
                unsigned int r = (unsigned int)sqrt((double)(rx * rx + ry * ry));
                if (r > shadowSize) {
                    circle[circleSize * y + x] = 0;
                } else {
                    circle[circleSize * y + x] = shadowAlpha * (shadowSize + 1 - r) / (shadowSize + 1);
                }
            }
        }
        for (unsigned int y = 0; y < _dy; y++) {
            int starty = y >= shadowSize ? 0 : shadowSize - y;
            int endy = y + shadowSize < _dy ? circleSize : _dy - y + shadowSize;
            for (unsigned int x = 0; x < _dx; x++) {
                unsigned int pos = _dx * y + x;
                int startx = x >= shadowSize ? 0 : shadowSize - x;
                int endx = x + shadowSize < _dx ? circleSize : _dx - x + shadowSize;
                if (mskptr[pos] == 0) {
                    continue;
                }
                for (int ry = starty; ry < endy; ry++) {
                    for (int rx = startx; rx < endx; rx++) {
                        unsigned int alpha = circle[circleSize * ry + rx];
                        if (alpha) {
                            unsigned int dstpos = _dx * (y + ry - shadowSize) + x + rx - shadowSize;
                            unsigned int s = mskptr[pos] * alpha >> 8;
                            if (shadow[0][dstpos] < s) {
                                shadow[0][dstpos] = (unsigned char)s;
                            }
                        }
                    }
                }
            }
        }
    } else if (m_shadowMode == TfontSettings::GradientShadow) { //Gradient classic shadow
        unsigned int shadowStep = shadowAlpha / shadowSize;
        for (unsigned int y = 0; y < _dy; y++) {
            for (unsigned int x = 0; x < _dx; x++) {
                unsigned int pos = _dx * y + x;
                if (mskptr[pos] == 0) {
                    continue;
                }

                unsigned int shadowAlphaGradient = shadowAlpha;
                for (unsigned int xx = 1; xx <= shadowSize; xx++) {
                    unsigned int s = mskptr[pos] * shadowAlphaGradient >> 8;
                    if (x + xx < _dx) {
                        if (y + xx < _dy && shadow[0][_dx * (y + xx) + x + xx] < s) {
                            shadow[0][_dx * (y + xx) + x + xx] = s;
                        }
                    }
                    shadowAlphaGradient -= shadowStep;
                }
            }
        }
    } else if (m_shadowMode == TfontSettings::ClassicShadow) { //Classic shadow
        for (unsigned int y = shadowSize; y < _dy; y++) {
            memcpy(shadow[0] + _dx * y + shadowSize, mskptr + _dx * (y - shadowSize), _dx - shadowSize);
        }
    }
}

TrenderedTextSubtitleWord::~TrenderedTextSubtitleWord()
{
    if (secondaryColoredWord) {
        delete secondaryColoredWord;
    }
    if (m_pOpaqueBox) {
        delete m_pOpaqueBox;
    }
}

unsigned int TrenderedTextSubtitleWord::getShadowSize(LONG fontHeight)
{
    if (mprops.shadowDepth == 0 || mprops.shadowMode == TfontSettings::ShadowDisabled) {
        return 0;
    }

    if (!mprops.isSSA() && mprops.autoSize && mprops.shadowDepth > 0) { // non SSA/ASS/ASS2 and autosize enabled
        return mprops.shadowDepth * fontHeight / (mprops.gdi_font_scale * 250) + 2.6;
    }

    return mprops.shadowDepth;;
}

size_t TrenderedTextSubtitleWord::getMemorySize() const
{
    int c = csp == FF_CSP_420P ? 2 : 1;
    size_t used_memory = sizeof(*this);
    used_memory += 4 * ((dx[0] * dy[0] + 256) + c * (dx[1] * dy[1] + 256));
    if (secondaryColoredWord) {
        used_memory += secondaryColoredWord->getMemorySize();
    }
    return used_memory;
}

double TrenderedTextSubtitleWord::get_baseline() const
{
    return m_baseline;
}

double TrenderedTextSubtitleWord::get_ascent() const
{
    return m_ascent;
}

double TrenderedTextSubtitleWord::get_descent() const
{
    return m_descent;
}

double TrenderedTextSubtitleWord::aboveBaseLinePlusOutline() const
{
    return m_ascent + mprops.outlineWidth;
}

double TrenderedTextSubtitleWord::belowBaseLinePlusOutline() const
{
    return m_descent + mprops.outlineWidth;
}

// printText
// Calculate destination address (dstLn), width (local_dx) and height (local_dy).
// clip is handled here. \kf is translated into two words with clip.
// If dst is NULL, prepare the bitmaps (do not print on video planes).
void TrenderedTextSubtitleWord::printText(
    double startx,
    double starty,
    double lineBaseline,
    REFERENCE_TIME rtStart,
    unsigned int prefsdx,
    unsigned int prefsdy,
    unsigned char **dst,
    const stride_t *stride)
{
    double sx0 = startx + mPathOffsetX / 8.0 - overhang.left;
    double sy0 = starty + lineBaseline - m_baseline + mPathOffsetY / 8.0 - overhang.top;
    if (mprops.gdi_font_scale == 4 && csp == FF_CSP_420P) {
        // YV12 OSD workaround solution. Improve renderer and remove this ugly workaround.
        sx0 = ffalign(int(sx0),  2);
        sy0 = ffalign(int(sy0), 2);
    }

    if (dst) {
        if (bmp[0] == NULL) {
            return;
        }
        unsigned char *dstLn[3];
        int local_dx[3];
        int local_dy[3];
        int x1[3], y1[3];
        ptrdiff_t srcOffset[] = {0, 0, 0};
        CRect clip = mprops.get_clip();
        const TcspInfo *cspInfo = csp_getInfo(csp);

        if (m_pOpaqueBox) {
            m_pOpaqueBox->printText(startx, starty, lineBaseline, rtStart, prefsdx, prefsdy, dst, stride);
        }

        // Translate \kf into two words with clip.
        if (mprops.karaokeFillStart != mprops.karaokeFillEnd) {
            if (rtStart < mprops.karaokeFillStart && secondaryColoredWord) {
                return secondaryColoredWord->printText(startx, starty, lineBaseline, rtStart, prefsdx, prefsdy, dst, stride);
            }
            if (mprops.karaokeFillStart < rtStart && rtStart < mprops.karaokeFillEnd) {
                int kx = sx0 + (double)(rtStart - mprops.karaokeFillStart) / (double)(mprops.karaokeFillEnd - mprops.karaokeFillStart) * dx[0];
                if (secondaryColoredWord) {
                    // We are doing highlighted left half.
                    if (kx < clip.right && clip.left <= kx) {
                        clip.right = kx;
                    }
                    secondaryColoredWord->printText(startx, starty, lineBaseline, rtStart, prefsdx, prefsdy, dst, stride);
                    if (kx < clip.left) {
                        return;
                    }
                } else {
                    // We are doing secondary colored right half.
                    kx++;
                    if (clip.left < kx) {
                        clip.left = kx;
                        if (clip.right < kx) {
                            return;
                        }
                    }
                }
            }
        }

        if (sx0 > clip.right) { return; }
        if (sy0 > clip.bottom) { return; }
        for (int i = 0; i < 3 ; i++) {
            int sy = int(sy0) >> cspInfo->shiftY[i];
            x1[i] = int(sx0) >> cspInfo->shiftX[i];
            int x2 = x1[i] + dx[i];
            y1[i] = sy;
            int y2 = y1[i] + dy[i];
            int left = clip.left >> cspInfo->shiftX[i];
            int top = clip.top >> cspInfo->shiftY[i];
            int right = clip.right >> cspInfo->shiftX[i];
            int bottom = clip.bottom >> cspInfo->shiftY[i];

            if (x1[i] < left) {
                srcOffset[i] = (left - x1[i]) * cspInfo->Bpp;
                x1[i] = left;
            }
            if (right < x2) {
                x2 = right + 1;
            }
            local_dx[i] = x2 - x1[i];
            if (local_dx[i] < 0) {
                local_dx[i] = 0;
            }

            if (y1[i] < top) {
                srcOffset[i] += (top - y1[i]) * dx[i];
                y1[i] = top;
            }
            if (bottom < y2) {
                y2 = bottom + 1;
            }
            local_dy[i] = y2 - y1[i];
            if (local_dy[i] < 0) {
                local_dy[i] = 0;
            }

            dstLn[i] = dst[i] + int(y1[i] * stride[i]);
            dstLn[i] += x1[i] * cspInfo->Bpp;
        }
        if (local_dx[0] > 0 && local_dy[0] > 0 && local_dx[1] > 0 && local_dy[1] > 0) {
#ifndef WIN64
            if (Tconfig::cpu_flags & FF_CPU_SSE2) {
#endif
                paint<Tsse2>(x1[0], y1[0], local_dx, local_dy, dstLn, stride, srcOffset, rtStart);
#ifndef WIN64
            } else {
                paint<Tmmx>(x1[0], y1[0], local_dx, local_dy, dstLn, stride, srcOffset, rtStart);
            }
#endif
        }
    } else {
        rasterize(CPointDouble(sx0, sy0));
    }
}

inline void TrenderedTextSubtitleWord::YV12_YfontRenderer(int x, int y,
        int bodyA, int outlineA, int shadowA,
        unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
        unsigned char *dst, stride_t dstStride) const
{
    int srcPos = y * dx[0] + x;
    int dstPos = y * dstStride + x;
    int s = shadowA * shadow[srcPos] >> 6;
    int d = ((256 - s) * dst[dstPos] >> 8) + (s * mprops.shadowYUV.Y >> 8);
    int o = outlineA * outline[srcPos] >> 6;
    int b = bodyA * bmp[srcPos] >> 6;
    int m = b + o;
    d = ((256 - m) * d >> 8) + (o * mprops.outlineYUV.Y >> 8);
    dst[dstPos] = std::min<int>(d + (b * mprops.bodyYUV.Y >> 8), 255);
}

inline void TrenderedTextSubtitleWord::YV12_UVfontRenderer(int x, int y,
        int bodyA, int outlineA, int shadowA,
        unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
        unsigned char *dstU, unsigned char *dstV, stride_t dstStride) const
{
    int srcPos = y * dx[1] + x;
    int dstPos = y * dstStride + x;
    /* U */
    int s = shadowA * shadow[srcPos] >> 6;
    int d = ((256 - s) * dstU[dstPos] >> 8) + (s * mprops.shadowYUV.U >> 8);
    int o = outlineA * outline[srcPos] >> 6;
    int b = bodyA * bmp[srcPos] >> 6;
    d = ((256 - o) * d >> 8) + (o * mprops.outlineYUV.U >> 8);
    dstU[dstPos] = std::min<int>(((256 - b) * d >> 8) + (b * mprops.bodyYUV.U >> 8), 255);
    /* V */
    d = ((256 - s) * dstV[dstPos] >> 8) + (s * mprops.shadowYUV.V >> 8);
    d = ((256 - o) * d >> 8) + (o * mprops.outlineYUV.V >> 8);
    dstV[dstPos] = std::min<int>(((256 - b) * d >> 8) + (b * mprops.bodyYUV.V >> 8), 255);
}

inline void TrenderedTextSubtitleWord::paintC_YV12(int startx, int startxUV, int endx, int endxUV,
        int starty, int startyUV, int dy1, int dy1UV, int dstStartx,
        int bodyA, int outlineA, int shadowA,
        unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
        unsigned char *bmpUV, unsigned char *outlineUV, unsigned char *shadowUV, unsigned char *mskUV,
        unsigned char *dst, unsigned char *dstU, unsigned char *dstV,
        stride_t dstStride, stride_t dstStrideUV) const
{
    for (int y = 0 ; y < dy1 ; y++) {
        if (y + starty >= 0) {
            int bodyA1 = bodyA, outlineA1 = outlineA, shadowA1 = shadowA;
            if (mprops.scroll.directionV && mprops.scroll.fadeaway) {
                double fader = mprops.get_scrollFader(y + starty);
                bodyA1 = bodyA * fader + 0.5;
                outlineA1 = outlineA * fader + 0.5;
                shadowA1 = shadowA * fader + 0.5;
            }
            for (int x = startx ; x < endx ; x++) {
                if (mprops.scroll.directionH && mprops.scroll.fadeaway) {
                    double fader = mprops.get_scrollFader(x + dstStartx);
                    bodyA1 = bodyA * fader + 0.5;
                    outlineA1 = outlineA * fader + 0.5;
                    shadowA1 = shadowA * fader + 0.5;
                }
                //bool fBody = (bodyA1 == 256 && m_outlineWidth > 0);
                YV12_YfontRenderer(x, y, bodyA1, outlineA1, shadowA1, bmp, outline, shadow, msk, dst, dstStride);
            }
        }
    }
    for (int y = 0 ; y < dy1UV ; y++) {
        if (y + startyUV >= 0) {
            int bodyA1 = bodyA, outlineA1 = outlineA, shadowA1 = shadowA;
            if (mprops.scroll.directionV && mprops.scroll.fadeaway) {
                double fader = mprops.get_scrollFader(y * 2 + starty);
                bodyA1 = bodyA * fader + 0.5;
                outlineA1 = outlineA * fader + 0.5;
                shadowA1 = shadowA * fader + 0.5;
            }
            for (int x = startxUV ; x < endxUV ; x++) {
                if (mprops.scroll.directionH && mprops.scroll.fadeaway) {
                    double fader = mprops.get_scrollFader(x * 2 + dstStartx);
                    bodyA1 = bodyA * fader + 0.5;
                    outlineA1 = outlineA * fader + 0.5;
                    shadowA1 = shadowA * fader + 0.5;
                }
                //bool fBody = (bodyA1 == 256 && m_outlineWidth > 0);
                YV12_UVfontRenderer(x, y, bodyA1, outlineA1, shadowA1, bmpUV, outlineUV, shadowUV, mskUV, dstU, dstV, dstStrideUV);
            }
        }
    }
}

inline void TrenderedTextSubtitleWord::RGBfontRenderer(int x, int y,
        int bodyA, int outlineA, int shadowA,
        unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
        unsigned char *dst, stride_t dstStride) const
{
    int srcPos = y * dx[1] + x * 4;
    int dstPos = y * dstStride + x * 4;
    /* B */
    int s = shadowA * shadow[srcPos] >> 6;
    int d = ((256 - s) * dst[dstPos] >> 8) + (s * mprops.shadowYUV.b >> 8);
    int o = outlineA * outline[srcPos] >> 6;
    int b = bodyA * bmp[srcPos] >> 6;
    int m = b + o;
    d = ((256 - m) * d >> 8) + (o * mprops.outlineYUV.b >> 8);
    dst[dstPos] = std::min<int>(d + (b * mprops.bodyYUV.b >> 8), 255);
    /* G */
    d = ((256 - s) * dst[dstPos + 1] >> 8) + (s * mprops.shadowYUV.g >> 8);
    d = ((256 - m) * d >> 8) + (o * mprops.outlineYUV.g >> 8);
    dst[dstPos + 1] = std::min<int>(d + (b * mprops.bodyYUV.g >> 8), 255);
    /* R */
    d = ((256 - s) * dst[dstPos + 2] >> 8) + (s * mprops.shadowYUV.r >> 8);
    d = ((256 - m) * d >> 8) + (o * mprops.outlineYUV.r >> 8);
    dst[dstPos + 2] = std::min<int>(d + (b * mprops.bodyYUV.r >> 8), 255);
}

// RGBfontRendererFillBody
// Can be used for RGB32 and Planar YUV-Y.
// Uses mask instead of outline (outline is created by subtracting bmp from mask), and after that print bmp ignoring alpha settings.
// This produces better quality if alpha for bmp is 100%.
inline void TrenderedTextSubtitleWord::RGBfontRendererFillBody(int x, int y,
        int bodyA, int outlineA, int shadowA,
        unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
        unsigned char *dst, stride_t dstStride) const
{
    int srcPos = y * dx[1] + x * 4;
    int dstPos = y * dstStride + x * 4;
    /* B */
    int s = shadowA * shadow[srcPos] >> 6;
    int d = ((256 - s) * dst[dstPos] >> 8) + (s * mprops.shadowYUV.b >> 8);
    int b = bmp[srcPos];
    int m;

    if (outlineA == 256) {
        m = msk[srcPos] << 2;
    } else {
        m = (b << 2) + (outlineA * outline[srcPos] >> 6);
    }

    d = ((256 - m) * d >> 8) + (m * mprops.outlineYUV.b >> 8);
    dst[dstPos] = std::min<int>(((64 - b) * d >> 6) + (b * mprops.bodyYUV.b >> 6), 255);
    /* G */
    d = ((256 - s) * dst[dstPos + 1] >> 8) + (s * mprops.shadowYUV.g >> 8);
    d = ((256 - m) * d >> 8) + (m * mprops.outlineYUV.g >> 8);
    dst[dstPos + 1] = std::min<int>(((64 - b) * d >> 6) + (b * mprops.bodyYUV.g >> 6), 255);
    /* R */
    d = ((256 - s) * dst[dstPos + 2] >> 8) + (s * mprops.shadowYUV.r >> 8);
    d = ((256 - m) * d >> 8) + (m * mprops.outlineYUV.r >> 8);
    dst[dstPos + 2] = std::min<int>(((64 - b) * d >> 6) + (b * mprops.bodyYUV.r >> 6), 255);
}

void TrenderedTextSubtitleWord::paintC_RGB(int startx, int endx, int starty, int dy1, int dstStartx,
        int bodyA, int outlineA, int shadowA,
        unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
        unsigned char *dst, stride_t dstStride) const
{
    for (int y = 0 ; y < dy1 ; y++) {
        if (y + starty >= 0) {
            int bodyA1 = bodyA, outlineA1 = outlineA, shadowA1 = shadowA;
            if (mprops.scroll.directionV && mprops.scroll.fadeaway) {
                double fader = mprops.get_scrollFader(y + starty);
                bodyA1 = bodyA * fader + 0.5;
                outlineA1 = outlineA * fader + 0.5;
                shadowA1 = shadowA * fader + 0.5;
            }
            for (int x = startx ; x < endx ; x++) {
                if (mprops.scroll.directionH && mprops.scroll.fadeaway) {
                    double fader = mprops.get_scrollFader(x + dstStartx);
                    bodyA1 = bodyA * fader + 0.5;
                    outlineA1 = outlineA * fader + 0.5;
                    shadowA1 = shadowA * fader + 0.5;
                }
                bool fBody = (bodyA1 == 256 && m_outlineWidth > 0);
                if (fBody) {
                    RGBfontRendererFillBody(x, y, bodyA1, outlineA1, shadowA1, bmp, outline, shadow, msk, dst, dstStride);
                } else {
                    RGBfontRenderer(x, y, bodyA1, outlineA1, shadowA1, bmp, outline, shadow, msk, dst, dstStride);
                }
            }
        }
    }
}

template<class _mm, bool fillBody, bool fillOutline> __forceinline void TrenderedTextSubtitleWord::fontRenderer_simd(
    const Tcctbl<_mm> &cctbl,
    unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
    unsigned char *dst) const
{
    const int srlcount = (fillBody && fillOutline) ? 6 : 8;
    _mm::__m _mm0, _mm1, _mm2, _mm3, _mm4, _mm5, _mm6, _mm7, _mm8, _mm9, _mm10, _mm11, _mm12, _mm13;
    _mm0 =    cctbl.shadow_A;
    movVqu(_mm1, shadow);
    _mm2 =    _mm1;
    pxor(_mm3, _mm3);
    punpckhbw(_mm1, _mm3);
    punpcklbw(_mm2, _mm3);
    pmullw(_mm1, _mm0);
    psrlw(_mm1, 6);
    pmullw(_mm2, _mm0);
    psrlw(_mm2, 6);                 // _mm1:_mm2 = s = m_shadowYUV.A * shadow[srcPos] >> 6;

    _mm4 =    cctbl.mask256;
    _mm5 =    _mm4;
    psubw(_mm4, _mm1);
    psubw(_mm5, _mm2);              // _mm4:_mm5 = (256-s)

    movVqu(_mm6, dst);
    _mm7 =    _mm6;
    punpckhbw(_mm6, _mm3);
    punpcklbw(_mm7, _mm3);          // _mm6;_mm7 = dst[dstPos]
    pmullw(_mm4, _mm6);
    psrlw(_mm4, 8);
    pmullw(_mm5, _mm7);
    psrlw(_mm5, 8);                 // _mm4:_mm5 = ((256-s) * dst[dstPos] >> 8)

    _mm0 =    cctbl.shadow_Y;
    pmullw(_mm1, _mm0);
    psrlw(_mm1, 8);
    pmullw(_mm2, _mm0);
    psrlw(_mm2, 8);                 // _mm1:_mm2 = (s * m_shadowYUV.Y >> 8)

    paddusb(_mm1, _mm4);
    paddusb(_mm2, _mm5);            // _mm1:_mm2 = d =((256-s) * dst[dstPos] >> 8) + (s * m_shadowYUV.Y >> 8);

    if (fillBody && fillOutline) {
        movVqu(_mm4, msk);
        _mm5 =    _mm4;
        punpckhbw(_mm4, _mm3);
        punpcklbw(_mm5, _mm3);
        _mm6 =    cctbl.mask64;
    } else {
        _mm0 =    cctbl.outline_A;
        movdqu(_mm8, outline);      // mm4 = outline[srcPos]
        _mm9 =    _mm8;
        punpckhbw(_mm8, _mm3);
        punpcklbw(_mm9, _mm3);
        pmullw(_mm8, _mm0);
        psrlw(_mm8, 6);
        pmullw(_mm9, _mm0);
        psrlw(_mm9, 6);             // _mm8:_mm9 = o= m_outlineYUV.A * outline[srcPos] >> 6;
        _mm4 =    _mm8;
        _mm5 =    _mm9;

        _mm0 = cctbl.body_A;
        movVqu(_mm10, bmp);
        _mm11      = _mm10;
        punpckhbw(_mm10, _mm3);
        punpcklbw(_mm11, _mm3);      // _mm10:_mm11 = b = bmp[srcPos];
        pmullw(_mm10, _mm0);
        psrlw(_mm10, 6);
        pmullw(_mm11, _mm0);
        psrlw(_mm11, 6);             // _mm10:_mm11 = b = m_bodyYUV.A * bmp[0][srcPos] >> 6;
        paddusw(_mm4, _mm10);
        paddusw(_mm5, _mm11);
        if (fillBody) {
            _mm12 = _mm4;
            _mm13 = _mm5;
        }
        _mm6 =    cctbl.mask256;
    }
    _mm7 =    _mm6;
    psubw(_mm6, _mm4);
    psubw(_mm7, _mm5);              // _mm6:_mm7 = (64-m) or (256-m)

    pmullw(_mm6, _mm1);
    psrlw(_mm6, srlcount);
    pmullw(_mm7, _mm2);
    psrlw(_mm7, srlcount);          // _mm6:_mm7 = ((64-m) * d >> 6) or ((256-m) * d >> 8)

    if (fillBody && fillOutline) {
        movdqu(_mm4, msk);          // mm4 = mask[srcPos]
        _mm5 =    _mm4;
        punpckhbw(_mm4, _mm3);
        punpcklbw(_mm5, _mm3);
    } else if (fillBody) {
        _mm4 =    _mm12;
        _mm5 =    _mm13;
    } else {
        _mm4 =    _mm8;
        _mm5 =    _mm9;
    }

    _mm0 =    cctbl.outline_Y;
    pmullw(_mm4, _mm0);

    psrlw(_mm4, srlcount);
    pmullw(_mm5, _mm0);
    psrlw(_mm5, srlcount);
    // BODY == FILL                   _mm4:_mm5 = (m * m_outlineYUV.Y >> 6);
    // BODY != FILL                   _mm4:_mm5 = (o * m_outlineYUV.Y >> 8);

    paddusb(_mm4, _mm6);
    paddusb(_mm5, _mm7);            // _mm4:_mm5 = d = ((64-m) * d >> 6) + (o * m_outlineYUV.Y >> 8); ( or ... + (m * m_outlineYUV.Y >> 6) )

    if (fillBody) {
        movVqu(_mm1, bmp);
        _mm2      = _mm1;
        punpckhbw(_mm1, _mm3);
        punpcklbw(_mm2, _mm3);      // _mm1:_mm2 = b = bmp[srcPos];
        _mm6 =    cctbl.mask64;
        _mm7 =    _mm6;
        psubw(_mm6, _mm1);
        psubw(_mm7, _mm2);          // _mm6:_mm7 = (64-b)

        pmullw(_mm6, _mm4);
        psrlw(_mm6, 6);
        pmullw(_mm7, _mm5);
        psrlw(_mm7, 6);             // _mm6:_mm7 = (64-b) * d >> 6
        _mm0 =    cctbl.body_Y;
        pmullw(_mm1, _mm0);
        psrlw(_mm1, 6);
        pmullw(_mm2, _mm0);
        psrlw(_mm2, 6);             // _mm1:_mm2 = (b * m_bodyYUV.Y >> 6)

        paddusb(_mm1, _mm6);
        paddusb(_mm2, _mm7);        // _mm1:_mm2 = ((64-b) * d >> 6) + (o * m_bodyYUV.Y >> 6);
    } else {
        _mm0 = cctbl.body_A;
        _mm1 =    _mm10;
        _mm2 =    _mm11;            // _mm1:_mm2 = b = m_bodyYUV.A * bmp[0][srcPos] >> 6;

        _mm0 =    cctbl.body_Y;
        pmullw(_mm1, _mm0);
        psrlw(_mm1, 8);
        pmullw(_mm2, _mm0);
        psrlw(_mm2, 8);             // _mm1:_mm2 = (b * m_bodyYUV.Y >> 8)

        paddusb(_mm1, _mm4);
        paddusb(_mm2, _mm5);        // _mm1:_mm2 = d + (o * m_bodyYUV.Y >> 8);
    }

    packuswb(_mm2, _mm1);
    movVqu(dst, _mm2);
}

template<class _mm> __forceinline void TrenderedTextSubtitleWord::fontRendererUV_simd(
    const Tcctbl<_mm> &cctbl,
    unsigned char *bmp, unsigned char *outline, unsigned char *shadow,
    unsigned char *dstU, unsigned char* dstV) const
{
    _mm::__m _mm0, _mm1, _mm2, _mm3, _mm4, _mm5, _mm6, _mm7;
    _mm::__m b1, b2, o1, o2, s1, s2;

    // U
    movVqu(_mm1, shadow);
    _mm0 =    cctbl.shadow_A;
    _mm2 =    _mm1;
    pxor(_mm3, _mm3);
    punpckhbw(_mm1, _mm3);
    punpcklbw(_mm2, _mm3);
    pmullw(_mm1, _mm0);
    psrlw(_mm1, 6);
    s1 =      _mm1;
    pmullw(_mm2, _mm0);
    psrlw(_mm2, 6);                           // _mm1:_mm2 = s =  = m_shadowYUV.A *shadow [1][srcPos1]>>6;
    s2 =      _mm2;

    _mm4 =    cctbl.mask256;
    _mm5 =    _mm4;
    psubw(_mm4, _mm1);
    psubw(_mm5, _mm2);                        // _mm4:_mm5 = (256-s)

    movVqu(_mm6, dstU);
    _mm7 =    _mm6;
    punpckhbw(_mm6, _mm3);
    punpcklbw(_mm7, _mm3);                    // _mm6;_mm7 = dstLn[1][dstPos]
    pmullw(_mm4, _mm6);
    psrlw(_mm4, 8);
    pmullw(_mm5, _mm7);
    psrlw(_mm5, 8);                           // _mm4:_mm5 = ((256-s)*dstLn[1][dstPos]>>8)

    _mm0 =    cctbl.shadow_U;
    pmullw(_mm1, _mm0);
    psrlw(_mm1, 8);
    pmullw(_mm2, _mm0);
    psrlw(_mm2, 8);                           // _mm1:_mm2 = (s*m_shadowYUV.U>>8)

    paddusb(_mm1, _mm4);
    paddusb(_mm2, _mm5);                      // _mm1:_mm2 = d =((256-s)*dstLn[1][dstPos]>>8)+(s*m_shadowYUV.U>>8);

    _mm0 =    cctbl.outline_A;
    movVqu(_mm4, outline);
    _mm5 =    _mm4;
    punpckhbw(_mm4, _mm3);
    punpcklbw(_mm5, _mm3);
    pmullw(_mm4, _mm0);
    psrlw(_mm4, 6);
    o1 =      _mm4;
    pmullw(_mm5, _mm0);
    psrlw(_mm5, 6);                           // _mm4:_mm5 = o = m_outlineYUV.A * mask1[srcPos]>>6;
    o2 =      _mm5;

    _mm6 =    cctbl.mask256;
    _mm7 =    _mm6;
    psubw(_mm6, _mm4);
    psubw(_mm7, _mm5);                        // _mm6:_mm7 = (256-o)

    pmullw(_mm6, _mm1);
    psrlw(_mm6, 8);
    pmullw(_mm7, _mm2);
    psrlw(_mm7, 8);                           // _mm6:_mm7 = ((256-o)*d>>8)

    _mm0 =    cctbl.outline_U;
    pmullw(_mm4, _mm0);
    psrlw(_mm4, 8);
    pmullw(_mm5, _mm0);
    psrlw(_mm5, 8);                           // _mm4:_mm5 = (o*m_outlineYUV.U>>8)

    paddusb(_mm4, _mm6);
    paddusb(_mm5, _mm7);                      // _mm4:_mm5 = d = ((256-o)*d>>8)+(o*m_outlineYUV.U>>8);

    _mm0 =    cctbl.body_A;

    movVqu(_mm1, bmp);
    _mm2 =    _mm1;
    punpckhbw(_mm1, _mm3);
    punpcklbw(_mm2, _mm3);
    pmullw(_mm1, _mm0);
    psrlw(_mm1, 6);
    b1 =      _mm1;
    pmullw(_mm2, _mm0);
    psrlw(_mm2, 6);                           // _mm1:_mm2 = b = m_bodyYUV.A * body[1][srcPos]>>6;
    b2 =      _mm2;

    _mm6 =    cctbl.mask256;
    _mm7 =    _mm6;
    psubw(_mm6, _mm1);
    psubw(_mm7, _mm2);                        // _mm6:_mm7 = (256-b)

    pmullw(_mm6, _mm4);
    psrlw(_mm6, 8);
    pmullw(_mm7, _mm5);
    psrlw(_mm7, 8);                           // _mm6:_mm7 = ((256-b)*d>>8)

    _mm0 =    cctbl.body_U;
    pmullw(_mm1, _mm0);
    psrlw(_mm1, 8);
    pmullw(_mm2, _mm0);
    psrlw(_mm2, 8);                           // _mm1:_mm2 = (b*m_bodyYUV.U>>8)

    paddusb(_mm1, _mm6);
    paddusb(_mm2, _mm7);                      // _mm1:_mm2 = ((256-b)*d>>8)+(b*m_bodyYUV.U>>8);

    packuswb(_mm2, _mm1);
    movVqu(dstU, _mm2);

    // V

    _mm1 =    s1;
    _mm2 =    s2;                             // _mm1:_mm2 = s = m_shadowYUV.A *shadow [1][srcPos1]>>8;

    _mm4 =    cctbl.mask256;
    _mm5 =    _mm4;
    psubw(_mm4, _mm1);
    psubw(_mm5, _mm2);                        // _mm4:_mm5 = (256-s)

    movVqu(_mm6, dstV);
    _mm7 =    _mm6;
    punpckhbw(_mm6, _mm3);
    punpcklbw(_mm7, _mm3);                    // _mm6;_mm7 = dstLn[2][dstPos]
    pmullw(_mm4, _mm6);
    psrlw(_mm4, 8);
    pmullw(_mm5, _mm7);
    psrlw(_mm5, 8);                           // _mm4:_mm5 = ((256-s)*dstLn[2][dstPos]>>8)

    _mm0 =    cctbl.shadow_V;
    pmullw(_mm1, _mm0);
    psrlw(_mm1, 8);
    pmullw(_mm2, _mm0);
    psrlw(_mm2, 8);                           // _mm1:_mm2 = (s*m_shadowYUV.V>>8)

    paddusb(_mm1, _mm4);
    paddusb(_mm2, _mm5);                      // _mm1:_mm2 = d =((256-s)*dstLn[2][dstPos]>>8)+(s*m_shadowYUV.V>>8);

    _mm4 =    o1;
    _mm5 =    o2;                             // _mm4:_mm5 = o = m_outlineYUV.A * mask1[srcPos]>>8;

    _mm6 =    cctbl.mask256;
    _mm7 =    _mm6;
    psubw(_mm6, _mm4);
    psubw(_mm7, _mm5);                        // _mm6:_mm7 = (256-o)

    pmullw(_mm6, _mm1);
    psrlw(_mm6, 8);
    pmullw(_mm7, _mm2);
    psrlw(_mm7, 8);                           // _mm6:_mm7 = ((256-o)*d>>8)

    _mm0 =    cctbl.outline_V;
    pmullw(_mm4, _mm0);
    psrlw(_mm4, 8);
    pmullw(_mm5, _mm0);
    psrlw(_mm5, 8);                           // _mm4:_mm5 = (o*m_outlineYUV.V>>8)

    paddusb(_mm4, _mm6);
    paddusb(_mm5, _mm7);                      // _mm4:_mm5 = d = ((256-o)*d>>8)+(o*m_outlineYUV.V>>8);

    _mm1 =    b1;
    _mm2 =    b2;                             // _mm1:_mm2 = b = m_bodyYUV.A * bmp[1][srcPos]>>8;

    _mm6 =    cctbl.mask256;
    _mm7 =    _mm6;
    psubw(_mm6, _mm1);
    psubw(_mm7, _mm2);                        // _mm6:_mm7 = (256-b)

    pmullw(_mm6, _mm4);
    psrlw(_mm6, 8);
    pmullw(_mm7, _mm5);
    psrlw(_mm7, 8);                           // _mm6:_mm7 = ((256-b)*d>>8)

    _mm0 =    cctbl.body_V;
    pmullw(_mm1, _mm0);
    psrlw(_mm1, 8);
    pmullw(_mm2, _mm0);
    psrlw(_mm2, 8);                           // _mm1:_mm2 = (b*m_bodyYUV.V>>8)

    paddusb(_mm1, _mm6);
    paddusb(_mm2, _mm7);                      // _mm1:_mm2 = ((256-b)*d>>8)+(b*m_bodyYUV.V>>8);

    packuswb(_mm2, _mm1);
    movVqu(dstV, _mm2);
}

template<class _mm> __forceinline void TrenderedTextSubtitleWord::fontRenderer_simd_funcs(bool fBody, bool fOutline,
        const Tcctbl<_mm> &cctbl,
        unsigned char *bmp, unsigned char *outline, unsigned char *shadow, unsigned char *msk,
        unsigned char *dst) const
{
    if (fBody && fOutline) {
        fontRenderer_simd<_mm, 1, 1>(cctbl, bmp, outline, shadow, msk, dst);
    } else if (fBody) {
        fontRenderer_simd<_mm, 1, 0>(cctbl, bmp, outline, shadow, msk, dst);
    } else {
        fontRenderer_simd<_mm, 0, 0>(cctbl, bmp, outline, shadow, msk, dst);
    }
}

template<class _mm> void TrenderedTextSubtitleWord::paint(int startx, int starty, int sdx[3], int sdy[3], unsigned char *dstLn[3], const stride_t stride[3], ptrdiff_t srcClipOffset[3], REFERENCE_TIME rtStart) const
{
    if (sdy[0] <= 0 || sdy[1] < 0 || dx[0] <= 0 || dy[0] <= 0) {
        return;
    }

    // karaoke: use secondaryColoredWord if not highlighted.
    if ((mprops.karaokeMode == TSubtitleProps::KARAOKE_k || mprops.karaokeMode == TSubtitleProps::KARAOKE_ko || mprops.karaokeMode == TSubtitleProps::KARAOKE_ko_opaquebox)
            && rtStart < mprops.karaokeStart && secondaryColoredWord) {
        return secondaryColoredWord->paint<_mm>(startx, starty, sdx, sdy, dstLn, stride, srcClipOffset, rtStart);
    }

    const TcspInfo *cspInfo = csp_getInfo(csp);

    unsigned char *cBmp[3], *cOutline[3], *cShadow[3], *cMsk[3];
    for (int i = 0 ; i < 3 ; i++) {
        cBmp[i]     = bmp[i]     + srcClipOffset[i];
        cOutline[i] = outline[i] + srcClipOffset[i];
        cShadow[i]  = shadow[i]  + srcClipOffset[i];
        cMsk[i]     = msk[i]     + srcClipOffset[i];
    }

    int startyUV = starty >> 1;
    int bodyA = mprops.bodyYUV.A;
    int outlineA = mprops.outlineYUV.A;
    int shadowA = mprops.shadowYUV.A;
    if (mprops.isFad) {
        double fader = mprops.get_fader(rtStart);
        bodyA *= fader;
        outlineA *= fader;
        shadowA *= fader;
    }

    bool fBody = (bodyA == 256 && m_outlineWidth > 0);

#ifdef WIN64
    if (Tconfig::cpu_flags & FF_CPU_SSE2) {
#else
    if (Tconfig::cpu_flags & (FF_CPU_SSE2 | FF_CPU_MMX)) {
#endif
        Tcctbl<_mm> cctbl(csp, mprops, bodyA, outlineA, shadowA, m_outlineWidth);

        if (csp == FF_CSP_420P) {
            // Y
            int endx = sdx[0] & ~(alignXsize - 1);
            int endxUV = sdx[1] & ~(alignXsize - 1);

            if (mprops.scroll.directionH && mprops.scroll.fadeaway) {
                // no SIMD here
                endx = 0;
                endxUV = 0;
            } else {
                for (int y = 0 ; y < sdy[0] ; y++) {
                    if (y + starty >= 0) {
                        if (mprops.scroll.directionV && mprops.scroll.fadeaway) {
                            fBody = set_scrollFader(cctbl, mprops.get_scrollFader(y + starty), bodyA, outlineA, shadowA);
                        }
                        for (int x = 0 ; x < endx ; x += alignXsize) {
                            int srcPos = y * dx[0] + x;
                            int dstPos = y * stride[0] + x;
                            fontRenderer_simd<_mm, 0, 0>(cctbl, &cBmp[0][srcPos], &cOutline[0][srcPos], &cShadow[0][srcPos], &cMsk[0][srcPos], &dstLn[0][dstPos]);
                        }
                    }
                }
                // UV
                for (int y = 0; y < sdy[1]; y++) {
                    if (y + startyUV >= 0) {
                        if (mprops.scroll.directionV && mprops.scroll.fadeaway) {
                            fBody = set_scrollFader(cctbl, mprops.get_scrollFader(y + starty), bodyA, outlineA, shadowA);
                        }
                        for (int x = 0 ; x < endxUV ; x += alignXsize) {
                            int srcPos = y * dx[1] + x;
                            int dstPos = y * stride[1] + x;
                            fontRendererUV_simd<_mm>(cctbl, &cBmp[1][srcPos], &cOutline[1][srcPos], &cShadow[1][srcPos], &dstLn[1][dstPos], &dstLn[2][dstPos]);
                        }
                    }
                }
            }

            paintC_YV12(endx, endxUV, sdx[0], sdx[1], starty, startyUV, sdy[0], sdy[1], startx, bodyA, outlineA, shadowA,
                        cBmp[0], cOutline[0], cShadow[0], cMsk[0], cBmp[1], cOutline[1], cShadow[1], cMsk[1], dstLn[0], dstLn[1], dstLn[2], stride[0], stride[1]);
        } else {
            //RGB32
            int simd_startx = 0;
            int endx2 = sdx[0] * 4;
            int simd_endx = endx2;
            if (mprops.scroll.directionH && mprops.scroll.fadeaway) {
                int fadeawayWidth = mprops.get_fadeawayWidth();
                simd_startx = fadeawayWidth > startx ? (fadeawayWidth - startx) * 4 : 0;
                if (mprops.dx - fadeawayWidth < startx + sdx[0]) {
                    simd_endx = (mprops.dx - fadeawayWidth - startx) * 4;
                }
            }
            int simd_dx = simd_endx > simd_startx ? (simd_endx - simd_startx) & ~(alignXsize - 1) : 0;
            simd_endx = simd_startx + simd_dx;
            if (simd_startx) {
                paintC_RGB(0, std::min(simd_startx / 4, sdx[0]), starty, sdy[1], startx, bodyA, outlineA, shadowA, cBmp[1], cOutline[1], cShadow[1], cMsk[1], dstLn[0], stride[0]);
            }
            if (simd_startx > endx2) {
                return;
            }
            bool fOutline = outlineA == 256;
            for (int y = 0; y < sdy[0]; y++) {
                if (y + starty >= 0) {
                    if (mprops.scroll.directionV && mprops.scroll.fadeaway) {
                        fBody = set_scrollFader(cctbl, mprops.get_scrollFader(y + starty), bodyA, outlineA, shadowA);
                        fOutline = outlineA == 256;
                    }
                    for (int x = simd_startx ; x < simd_endx ; x += alignXsize) {
                        int srcPos = y * dx[1] + x;
                        int dstPos = y * stride[0] + x;
                        fontRenderer_simd_funcs<_mm>(fBody, fOutline, cctbl, &cBmp[1][srcPos], &cOutline[1][srcPos], &cShadow[1][srcPos], &cMsk[1][srcPos], &dstLn[0][dstPos]);
                    }
                }
            }
            if (simd_endx < endx2) {
                paintC_RGB(std::max(simd_endx / 4, 0), sdx[0], starty, sdy[1], startx, bodyA, outlineA, shadowA, cBmp[1], cOutline[1], cShadow[1], cMsk[1], dstLn[0], stride[0]);
            }
        }
    } else {
        if (csp == FF_CSP_420P) {
            // YV12
            paintC_YV12(0, 0, sdx[0], sdx[1], starty, startyUV, sdy[0], sdy[1], startx, bodyA, outlineA, shadowA,
                        cBmp[0], cOutline[0], cShadow[0], cMsk[0], cBmp[1], cOutline[1], cShadow[1], cMsk[1], dstLn[0], dstLn[1], dstLn[2], stride[0], stride[1]);
        } else {
            //RGB32
            paintC_RGB(0, sdx[0], starty, sdy[1], startx, bodyA, outlineA, shadowA, cBmp[1], cOutline[1], cShadow[1], cMsk[1], dstLn[0], stride[0]);
        }
    }
    _mm_empty();
}
