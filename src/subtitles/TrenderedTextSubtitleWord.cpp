/*
 * Copyright (c) 2002-2006 Milan Cutka
 *               2007-2009 h.yamagata
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
#include "TrenderedTextSubtitleWord.h"
#include "TfontSettings.h"
#include "simd.h"
#include "ffdebug.h"
#pragma warning(disable:4244)

// custom copy constructor for karaoke
TrenderedTextSubtitleWord::TrenderedTextSubtitleWord(
        const TrenderedTextSubtitleWord &parent,
        secondaryColor_t):
    prefs(parent.prefs),
    Rasterizer(parent)
{
    *this = parent;
    secondaryColoredWord = NULL;
    bmp[0]     = aligned_calloc3<uint8_t>(dx[0], dy[0], 16);
    bmp[1]     = aligned_calloc3<uint8_t>(dx[1], dy[1], 16);
    outline[0] = aligned_calloc3<uint8_t>(dx[0], dy[0], 16);
    outline[1] = aligned_calloc3<uint8_t>(dx[1], dy[1], 16);
    shadow[0]  = aligned_calloc3<uint8_t>(dx[0], dy[0], 16);
    shadow[1]  = aligned_calloc3<uint8_t>(dx[1], dy[1], 16);
    msk[0]     = aligned_calloc3<uint8_t>(dx[0], dy[0], 16);

    memcpy(bmp[0], parent.bmp[0], dx[0] * dy[0]);
    memcpy(bmp[1], parent.bmp[1], dx[1] * dy[1]);
    if (props.karaokeMode == TSubtitleProps::KARAOKE_ko) {
        memset(outline[0], 0, dx[0] * dy[0]);
        memset(outline[1], 0, dx[1] * dy[1]);
        memset(shadow[0] , 0, dx[0] * dy[0]);
        memset(shadow[1] , 0, dx[1] * dy[1]);
        m_outlineYUV.A = 0;
    } else {
        memcpy(outline[0], parent.outline[0], dx[0] * dy[0]);
        memcpy(outline[1], parent.outline[1], dx[1] * dy[1]);
        memcpy(shadow[0] , parent.shadow[0] , dx[0] * dy[0]);
        memcpy(shadow[1] , parent.shadow[1] , dx[1] * dy[1]);
    }

    if (parent.msk[1]) {
        msk[1] = aligned_calloc3<uint8_t>(dx[1], dy[1], 16);
        memset(msk[1], 0, dx[1] * dy[1]);
    }
    m_bodyYUV = YUVcolorA(props.SecondaryColour,props.SecondaryColourA);
    oldFader = 0;
    updateMask(1 << 16, 2);
}

// full rendering
TrenderedTextSubtitleWord::TrenderedTextSubtitleWord(
        HDC hdc,
        const wchar_t *s0,
        size_t strlens,
        const YUVcolorA &YUV,
        const YUVcolorA &outlineYUV,
        const YUVcolorA &shadowYUV,
        const TprintPrefs &Iprefs,
        LOGFONT lf,
        double xscale,
        TSubtitleProps Iprops):
    props(Iprops),
    m_bodyYUV(YUV),
    m_outlineYUV(outlineYUV),
    m_shadowYUV(shadowYUV),
    prefs(Iprefs),
    secondaryColoredWord(NULL),
    dstOffset(0),
    oldBodyYUVa(256),
    oldOutlineYUVa(256),
    overhang(0,0,0,0),
    m_outlineWidth(0),
    baseline(0),
    m_ascent(0),
    m_descent(0),
    m_linegap(0)
{
    if (!*s0)
        return;

    int gdi_font_scale = prefs.fontSettings.gdi_font_scale;

    if (Tconfig::cpu_flags&FF_CPU_MMXEXT) {
        YV12_lum2chr_min=YV12_lum2chr_min_mmx2;
        YV12_lum2chr_max=YV12_lum2chr_max_mmx2;
    } else {
        YV12_lum2chr_min=YV12_lum2chr_min_mmx;
        YV12_lum2chr_max=YV12_lum2chr_max_mmx;
    }
#ifndef WIN64
    if (Tconfig::cpu_flags&FF_CPU_SSE2) {
#endif
        alignXsize=16;
        TtextSubtitlePrintY=TtextSubtitlePrintY_sse2;
        TtextSubtitlePrintUV=TtextSubtitlePrintUV_sse2;
#ifndef WIN64
    } else {
        alignXsize=8;
        TtextSubtitlePrintY=TtextSubtitlePrintY_mmx;
        TtextSubtitlePrintUV=TtextSubtitlePrintUV_mmx;
    }
#endif

    csp = prefs.csp & FF_CSPS_MASK;

    m_outlineWidth=1;
    outlineWidth_double = prefs.outlineWidth;
    //if (props.refResY && prefs.clipdy)
    // outlineWidth_double = outlineWidth_double * prefs.clipdy / props.refResY;

    if (!prefs.opaqueBox) {
        if (csp==FF_CSP_420P && outlineWidth_double < 0.6 && !m_bodyYUV.isGray()) {
            m_outlineWidth=1;
            outlineWidth_double=0.6;
            m_outlineYUV=0;
        } else {
            m_outlineWidth=int(outlineWidth_double);
            if ((double)m_outlineWidth < outlineWidth_double)
                m_outlineWidth++;
        }
    } else
        m_outlineWidth = int(outlineWidth_double);

    if (outlineWidth_double < 1.0 && outlineWidth_double > 0)
        outlineWidth_double = 0.5 + outlineWidth_double/2.0;

    strings tab_parsed_string;
    strtok(ffstring(s0,strlens).c_str(),L"\t",tab_parsed_string);
    SIZE sz;
    sz.cx=sz.cy=0;
    ints cxs;
    for (strings::iterator s=tab_parsed_string.begin();s!=tab_parsed_string.end();s++) {
        SIZE sz0;
        GetTextExtentPoint32W(hdc,s->c_str(),(int)s->size(),&sz0);
        sz.cx+=sz0.cx;
        if (s+1!=tab_parsed_string.end()) {
            int tabsize=prefs.tabsize*sz0.cy;
            int newpos=(sz.cx/tabsize+1)*tabsize;
            sz0.cx+=newpos-sz.cx;
            sz.cx=newpos;
        }
        cxs.push_back(sz0.cx);
        sz.cy=std::max(sz.cy,sz0.cy);
    }
    dxChar  = xscale * sz.cx / (gdi_font_scale * 100);
    dyChar  = sz.cy / gdi_font_scale;
    getGlyph(hdc, tab_parsed_string, xscale, sz, cxs, lf);
    drawShadow();
}

void TrenderedTextSubtitleWord::getGlyph(HDC hdc,
    const strings &tab_parsed_string,
    double xscale,
    SIZE italic_fixed_sz,
    const ints &cxs,
    const LOGFONT &lf)
{
    int gdi_font_scale = prefs.fontSettings.gdi_font_scale;
    OUTLINETEXTMETRIC otm;
    if (GetOutlineTextMetrics(hdc,sizeof(otm),&otm)) {
        baseline=otm.otmTextMetrics.tmAscent / gdi_font_scale + m_outlineWidth;
        m_ascent = otm.otmTextMetrics.tmAscent / gdi_font_scale;
        m_descent = otm.otmTextMetrics.tmDescent / gdi_font_scale;
        m_linegap = (lf.lfHeight - otm.otmTextMetrics.tmAscent - otm.otmTextMetrics.tmDescent) / gdi_font_scale;
        if (otm.otmItalicAngle)
            italic_fixed_sz.cx += ff_abs(LONG(italic_fixed_sz.cy*sin(otm.otmItalicAngle*M_PI/1800)));
        else
            if (otm.otmTextMetrics.tmItalic)
                italic_fixed_sz.cx+=italic_fixed_sz.cy*0.35;
            m_shadowSize = getShadowSize(otm.otmTextMetrics.tmHeight, gdi_font_scale);
    } else {
        // non true-type
        baseline  = italic_fixed_sz.cy * 0.8 / gdi_font_scale + m_outlineWidth;;
        m_ascent  = italic_fixed_sz.cy * 0.8 / gdi_font_scale;
        m_descent = italic_fixed_sz.cy * 0.2 / gdi_font_scale;
        m_linegap = italic_fixed_sz.cy * 0.03 / gdi_font_scale;
        m_shadowSize = getShadowSize(lf.lfHeight, gdi_font_scale);
        if (lf.lfItalic)
            italic_fixed_sz.cx+=italic_fixed_sz.cy*0.35;
    }

    overhang = getOverhangPrivate();

    gdi_dx = ((italic_fixed_sz.cx + gdi_font_scale) / gdi_font_scale + 1) * gdi_font_scale;
    gdi_dy = italic_fixed_sz.cy + gdi_font_scale;

    if (gdi_font_scale == 4)
         drawGlyphOSD(hdc,tab_parsed_string,cxs,xscale);  // sharp and fast, good for OSD.
    else
         drawGlyphSubtitles(hdc,tab_parsed_string,cxs,xscale); // anti aliased, good for subtitles.
}

void TrenderedTextSubtitleWord::drawGlyphSubtitles(
      HDC hdc,
      const strings &tab_parsed_string,
      const ints &cxs,
      double xscale)
{
    int gdi_font_scale = prefs.fontSettings.gdi_font_scale;
    bool bFirstPath = true;
    int x = 0;
    ints::const_iterator cx=cxs.begin();
    foreach (const ffstring &s, tab_parsed_string) {
        PartialBeginPath(hdc, bFirstPath);
        bFirstPath = false;
        TextOutW(hdc,0,0,s.c_str(),(int)s.size());
        PartialEndPath(hdc, x, 0);
        x+=*cx;
        cx++;
    }

    Transform(CPoint(0, 0), xscale/100);
    ScanConvert();
    Rasterize(0, 0, overhang);
    dx[0] = mGlyphBmpWidth;
    dy[0] = mGlyphBmpHeight;
}

void TrenderedTextSubtitleWord::Transform(CPoint org, double scalex)
{
 /*
  *  Copied and modified from guliverkli, RTS.cpp
  *
  *  Copyright (C) 2003-2006 Gabest
  *  http://www.gabest.org
  */
    double scaley = 1;

    double caz = cos((3.1415/180)*0/*m_style.fontAngleZ*/);
    double saz = sin((3.1415/180)*0/*m_style.fontAngleZ*/);
    double cax = cos((3.1415/180)*0/*m_style.fontAngleX*/);
    double sax = sin((3.1415/180)*0/*m_style.fontAngleX*/);
    double cay = cos((3.1415/180)*0/*m_style.fontAngleY*/);
    double say = sin((3.1415/180)*0/*m_style.fontAngleY*/);

    for(int i = 0; i < mPathPoints; i++) {
        double x, y, z, xx, yy, zz;

        x = scalex * (mpPathPoints[i].x /*+ m_style.fontShiftX * mpPathPoints[i].y*/) - org.x;
        y = scaley * (mpPathPoints[i].y /*+ m_style.fontShiftY * mpPathPoints[i].x*/) - org.y;
        z = 0;

        xx = x*caz + y*saz;
        yy = -(x*saz - y*caz);
        zz = z;

        x = xx;
        y = yy*cax + zz*sax;
        z = yy*sax - zz*cax;

        xx = x*cay + z*say;
        yy = y;
        zz = x*say - z*cay;

        zz = std::max<double>(zz, -19000);

        x = (xx * 20000) / (zz + 20000);
        y = (yy * 20000) / (zz + 20000);

        mpPathPoints[i].x = (LONG)(x + org.x + 0.5);
        mpPathPoints[i].y = (LONG)(y + org.y + 0.5);
    }
}

void TrenderedTextSubtitleWord::drawGlyphOSD(
      HDC hdc,
      const strings &tab_parsed_string,
      const ints &cxs,
      double xscale)
{
    RECT r={0,0,gdi_dx,gdi_dy};
    uint8_t *bmp16 = aligned_calloc3<uint8_t>(gdi_dx * size_of_rgb32,gdi_dy, 32);
    HBITMAP hbmp=CreateCompatibleBitmap(hdc,gdi_dx,gdi_dy);
    HGDIOBJ old=SelectObject(hdc,hbmp);
    FillRect(hdc,&r,(HBRUSH)GetStockObject(BLACK_BRUSH));
    SetTextColor(hdc,RGB(255,255,255));
    SetBkColor(hdc,RGB(0,0,0));

    int x = 2;
    const int gdi_font_scale = 4;
    ints::const_iterator cx=cxs.begin();
    foreach (const ffstring &s, tab_parsed_string) {
        TextOutW(hdc, x, 2, s.c_str(), (int)s.size());
        x+=*cx;
        cx++;
    }

    BITMAPINFO bmi;
    bmi.bmiHeader.biSize=sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth=gdi_dx;
    bmi.bmiHeader.biHeight=-1*gdi_dy;
    bmi.bmiHeader.biPlanes=1;
    bmi.bmiHeader.biBitCount=32;
    bmi.bmiHeader.biCompression=BI_RGB;
    bmi.bmiHeader.biSizeImage=gdi_dx*gdi_dy;
    bmi.bmiHeader.biXPelsPerMeter=75;
    bmi.bmiHeader.biYPelsPerMeter=75;
    bmi.bmiHeader.biClrUsed=0;
    bmi.bmiHeader.biClrImportant=0;
    GetDIBits(hdc,hbmp,0,gdi_dy,bmp16,&bmi,DIB_RGB_COLORS);  // copy bitmap, get it in bmp16 (RGB32).
    SelectObject(hdc,old);
    DeleteObject(hbmp);

    unsigned int tmpdx = xscale * gdi_dx / (gdi_font_scale * 100) + overhang.left + overhang.right;
    dx[0]=((tmpdx + 15) / 16) * 16;
    dy[0]    = gdi_dy / gdi_font_scale + overhang.top + overhang.bottom;
    baseline = baseline / gdi_font_scale + 2;

    overhang.right += dx[0] - tmpdx;
    if (dx[0] < 16) dx[0]=16;
    if (csp==FF_CSP_420P)
     dy[0]=((dy[0]+1)/2)*2;
    mGlyphBmpWidth = dx[0] + m_outlineWidth*2; // add margin to simplify the outline drawing process.
    mGlyphBmpHeight = dy[0] + m_outlineWidth*2;
    mGlyphBmpWidth = ((mGlyphBmpWidth+7)/8)*8;
    bmp[0]=aligned_calloc3<uint8_t>(dx[0],dy[0],16);

    // Here we scale to 1/gdi_font_scale.
    // average 4x5 pixels and store it in 6bit (max 64, not 63)
    unsigned int xstep = xscale == 100 ?
                             gdi_font_scale * 65536 :
                             gdi_font_scale * 100 * 65536 / xscale;
    unsigned int gdi_rendering_window_width = std::max<unsigned int>(
                         xscale == 100 ?
                             4 :
                             4 * 100 / xscale
                             , 1);
    // coeff calculation
    // To averave gdi_rendering_window_width * 5 pixels
    unsigned int coeff;
    if (gdi_rendering_window_width == 4)
        coeff = 824; // 65536/ 4 / (4 * 5) plus bit to make 63.xxx->64;
    else
        coeff = 65536/ 4 / (gdi_rendering_window_width * 5);
    int dx0_mult_4 = gdi_dx * size_of_rgb32;
    unsigned int xstep_sse2 = xstep * 8;
    unsigned int startx = (2 << 16) + xstep;
    unsigned int endx = (gdi_dx - 2) << 16;
    for (unsigned int y = 2 ; y < gdi_dy - 3 ; y += gdi_font_scale) {
        unsigned char *dstBmpY = bmp[0] + (y/gdi_font_scale + overhang.top) * dx[0] + overhang.left;
        unsigned int x = startx;
        const unsigned char *bmp16srcLineStart = bmp16 + ((y - 2) * gdi_dx) * size_of_rgb32;
        const unsigned char *bmp16srcEnd;
        bmp16srcEnd = bmp16srcLineStart + 5 * dx0_mult_4;
        for (; x < endx ; x += xstep, dstBmpY++) {
            unsigned int sum;
            const unsigned char *bmp16src = bmp16srcLineStart + ((x >> 16) - 2) * size_of_rgb32;

            if (xscale == 100) {
                sum = 0;
                for (; bmp16src < bmp16srcEnd ; bmp16src += dx0_mult_4) {
                    // a bit of optimization: Only one if block will be compiled. Loops are unrolled.
                    sum += bmp16src[0] + bmp16src[4] + bmp16src[8] + bmp16src[12];
                }

                sum = (sum * coeff) >> 16;
                *dstBmpY = (unsigned char)std::min<unsigned int>(sum,255);
            } else {
                sum = 0;
                for (; bmp16src < bmp16srcEnd ; bmp16src += dx0_mult_4) {
                    for (unsigned int i = 0 ; i < size_of_rgb32 * gdi_rendering_window_width ; i += size_of_rgb32)
                        sum += bmp16src[i];
                }
                sum = (sum * coeff) >> 16;
                *dstBmpY = (unsigned char)std::min<unsigned int>(sum,255);
            }
        }
    }
    aligned_free(bmp16);
}

void TrenderedTextSubtitleWord::drawShadow()
{
    if (prefs.opaqueBox && (mPathOffsetX > 0 || mPathOffsetY > 0)) {
        unsigned int offx = (mPathOffsetX > 0) ?
                                mPathOffsetX >> 3:
                                0;
        unsigned int offy = (mPathOffsetY > 0) ?
                                mPathOffsetY >> 3:
                                0;
        unsigned int newdx0 = dx[0] + offx;
        unsigned int newdx = ((newdx0 + 15) / 16) * 16;
        unsigned int newdy = dy[0] + offy;
        uint8_t *newbmp = aligned_calloc3<uint8_t>(newdx,newdy,16);
        for (unsigned int y = 0 ; y < dy[0] ; y++) {
            memcpy(newbmp + (y + offy) * newdx + offx, bmp[0] + y * dx[0], dx[0]);
        }
        dx[0] = newdx;
        dy[0] = newdy;
        _aligned_free(bmp[0]);
        bmp[0] = newbmp;
        overhang.right += newdx - newdx0;
        if (mPathOffsetX > 0) mPathOffsetX = 0;
        if (mPathOffsetY > 0) mPathOffsetY = 0;
    }

    msk[0]     = aligned_calloc3<uint8_t>(dx[0],dy[0],16);
    outline[0] = aligned_calloc3<uint8_t>(dx[0],dy[0],16);
    shadow[0]  = aligned_calloc3<uint8_t>(dx[0],dy[0],16);

    if (prefs.blur) {
        int startx = overhang.left - 1;
        int starty = overhang.top - 1;
        int endx = dx[0] - overhang.right + 1;
        int endy = dy[0] - overhang.bottom + 1;
        bmp[0]=blur(bmp[0], dx[0], dy[0], startx, starty, endx, endy, true);
    }

    if (prefs.opaqueBox) {
        if (m_outlineWidth > 0) {
            CRect opaqueBox = overhang - CRect(m_outlineWidth,m_outlineWidth,m_outlineWidth,m_outlineWidth);
            for (unsigned int y = opaqueBox.top ; y < dy[0] - opaqueBox.bottom ; y++) {
                memset(msk[0] + y * dx[0] + opaqueBox.left, 64, dx[0] - opaqueBox.left - opaqueBox.right);
            }
        } else {
            memcpy(msk[0], bmp[0], dx[0] * dy[0]);
        }
    } else if (m_outlineWidth) {
        // Prepare outline
        // Prepare matrix for outline calculation
        short *matrix=NULL;
        unsigned int matrixSizeH = ((m_outlineWidth*2+8)/8)*8; // 2 bytes for one.
        unsigned int matrixSizeV = m_outlineWidth*2+1;
        if (m_outlineWidth>0) {
            double r_cutoff=1.5;
            if (outlineWidth_double<4.5)
             r_cutoff=outlineWidth_double/3.0;
            double r_mul=512.0/r_cutoff;
            matrix=(short*)aligned_calloc(matrixSizeH*2,matrixSizeV,16);
            for (int y = -m_outlineWidth ; y <= m_outlineWidth ; y++)
                for (int x = -m_outlineWidth ; x <= m_outlineWidth ; x++) {
                    int pos=(y + m_outlineWidth)*matrixSizeH+x+m_outlineWidth;
                    double r=0.5+outlineWidth_double-sqrt(double(x*x+y*y));
                    if (r>r_cutoff)
                       matrix[pos]=512;
                    else if (r>0)
                       matrix[pos]=r*r_mul;
                }
        }

        unsigned int max_outline_pos_x  = dx[0] - overhang.right + m_outlineWidth;
        unsigned int max_outline_pos_y  = dy[0] - overhang.bottom + m_outlineWidth;

        TexpandedGlyph expanded(*this);
        if (Tconfig::cpu_flags&FF_CPU_SSE2
#ifndef WIN64
            && m_outlineWidth>=2
#endif
           )
        {
            size_t matrixSizeH_sse2 = matrixSizeH >> 3;
            size_t srcStrideGap = expanded.dx - matrixSizeH;
            for (unsigned int y = overhang.top - m_outlineWidth ; y < max_outline_pos_y ; y++)
                for (unsigned int x = overhang.left - m_outlineWidth ; x < max_outline_pos_x ; x++) {
                    unsigned int sum=fontPrepareOutline_sse2((unsigned char*)expanded + expanded.dx * y + x , srcStrideGap, matrix, matrixSizeH_sse2, matrixSizeV) >> 9;
                    msk[0][dx[0]*y+x]=sum>64 ? 64 : sum;
                }
        }
#ifndef WIN64
        else if (Tconfig::cpu_flags&FF_CPU_MMX) {
            size_t matrixSizeH_mmx=(matrixSizeV+3)/4;
            size_t srcStrideGap = expanded.dx - matrixSizeH_mmx * 4;
            size_t matrixGap=matrixSizeH_mmx & 1 ? 8 : 0;
            for (unsigned int y = overhang.top - m_outlineWidth ; y < max_outline_pos_y ; y++) {
                for (unsigned int x = overhang.left - m_outlineWidth ; x < max_outline_pos_x ; x++) {
                    unsigned int sum=fontPrepareOutline_mmx((unsigned char*)expanded + expanded.dx * y + x, srcStrideGap, matrix, matrixSizeH_mmx, matrixSizeV, matrixGap) >> 9;
                    msk[0][dx[0]*y+x]=sum>64 ? 64 : sum;
                }
            }
        }
#endif
        else {
            for (unsigned int y = overhang.top - m_outlineWidth ; y < max_outline_pos_y ; y++)
               for (unsigned int x = overhang.left - m_outlineWidth ; x < max_outline_pos_x ; x++) {
                   unsigned char *srcPos=(unsigned char*)expanded + expanded.dx * y + x;
                   unsigned int sum=0;
                   for (unsigned int yy=0;yy<matrixSizeV;yy++,srcPos+=mGlyphBmpWidth-matrixSizeV)
                      for (unsigned int xx=0;xx<matrixSizeV;xx++,srcPos++)
                          sum+=(*srcPos)*matrix[matrixSizeH*yy+xx];
                     sum>>=9;
                     msk[0][dx[0]*y+x]=sum>64 ? 64 : sum;
               }
        }

        if (prefs.outlineBlur || (prefs.shadowMode==0 && m_shadowSize>0)) // blur outline and msk
            msk[0]=blur(msk[0], dx[0], dy[0], 0, 0, dx[0], dy[0], false);
        if (matrix)
            aligned_free(matrix);
    }  else {
        // m_outlineWidth==0
        memcpy(msk[0],bmp[0],dx[0]*dy[0]);
    }

    // Draw outline.
    unsigned int count=dx[0]*dy[0];
    for (unsigned int c=0;c<count;c++) {
        int b=bmp[0][c];
        int o=msk[0][c]-b;
        if (o>0)
            outline[0][c]=o;
    }
    m_shadowMode=prefs.shadowMode;

    if (csp==FF_CSP_420P) {
        dx[1]=dx[0]>>1;
        dy[1]=dy[0]>>1;
        dx[1]=(dx[1]/alignXsize+1)*alignXsize;
        bmp[1]     = aligned_calloc3<uint8_t>(dx[1],dy[1]);
        outline[1] = aligned_calloc3<uint8_t>(dx[1],dy[1]);
        shadow[1]  = aligned_calloc3<uint8_t>(dx[1],dy[1]);

        dx[2]=dx[0]>>1;
        dy[2]=dy[0]>>1;
        dx[2]=(dx[2]/alignXsize+1)*alignXsize;
    } else {
        //RGB32
        dx[1]=dx[0] * size_of_rgb32;
        dy[1]=dy[0];
        bmp[1]     = aligned_calloc3<uint8_t>(dx[1],dy[1],16);
        outline[1] = aligned_calloc3<uint8_t>(dx[1],dy[1],16);
        shadow[1]  = aligned_calloc3<uint8_t>(dx[1],dy[1],16);
        msk[1]     = aligned_calloc3<uint8_t>(dx[1],dy[1],16);
    }
    updateMask();

    if (props.karaokeMode != TSubtitleProps::KARAOKE_NONE)
        secondaryColoredWord = new TrenderedTextSubtitleWord(*this, secondaryColor_t());
}

void TrenderedTextSubtitleWord::updateMask(int fader, int create) const
{
 // shadowMode 0: glowing, 1:classic with gradient, 2: classic with no gradient, >=3: no shadow
 if (create == 0 && oldFader == fader)
  return;
 oldFader = fader;
 unsigned int _dx=dx[0];
 unsigned int _dy=dy[0];
 unsigned int count=_dx*_dy;
 unsigned int shadowSize = m_shadowSize;

 unsigned int bodyYUVa = m_bodyYUV.A * fader >> 16;
 unsigned int outlineYUVa = m_outlineYUV.A * fader >> 16;
 unsigned int shadowYUVa = m_shadowYUV.A * fader >> 16;
 if (bodyYUVa != 256 || outlineYUVa != 256 || create == 2 || bodyYUVa != oldBodyYUVa || outlineYUVa != oldOutlineYUVa)
  {
   oldBodyYUVa = bodyYUVa;
   oldOutlineYUVa = outlineYUVa;
   for (unsigned int c=0;c<count;c++)
    {
     msk[0][c]=(bmp[0][c] * bodyYUVa >> 8) + (outline[0][c] * outlineYUVa >> 8);
    }
  }

 unsigned char* mskptr;
 if (m_outlineWidth)
  mskptr=msk[0];
 else
  mskptr=bmp[0];

 unsigned int shadowAlpha = 255;
 if (shadowSize > 0 && (create || mskptr != bmp[0]))
  if (m_shadowMode == 0) //Gradient glowing shadow (most complex)
   {
    _mm_empty();
    if (_dx<shadowSize) shadowSize=_dx;
    if (_dy<shadowSize) shadowSize=_dy;
    unsigned int circle[1089]; // 1089=(16*2+1)^2
    if (shadowSize>16) shadowSize=16;
    int circleSize=shadowSize*2+1;
    for (int y=0;y<circleSize;y++)
     {
      for (int x=0;x<circleSize;x++)
       {
        unsigned int rx=ff_abs(x-(int)shadowSize);
        unsigned int ry=ff_abs(y-(int)shadowSize);
        unsigned int r=(unsigned int)sqrt((double)(rx*rx+ry*ry));
        if (r>shadowSize)
         circle[circleSize*y+x] = 0;
        else
         circle[circleSize*y+x] = shadowAlpha*(shadowSize+1-r)/(shadowSize+1);
       }
     }
    for (unsigned int y=0; y<_dy;y++)
     {
      int starty = y>=shadowSize ? 0 : shadowSize-y;
      int endy = y+shadowSize<_dy ? circleSize : _dy-y+shadowSize;
      for (unsigned int x=0; x<_dx;x++)
       {
        unsigned int pos = _dx*y+x;
        int startx = x>=shadowSize ? 0 : shadowSize-x;
        int endx = x+shadowSize<_dx ? circleSize : _dx-x+shadowSize;
        if (mskptr[pos] == 0) continue;
        for (int ry=starty; ry<endy;ry++)
         {
          for (int rx=startx; rx<endx;rx++)
           {
             unsigned int alpha = circle[circleSize*ry+rx];
             if (alpha)
              {
               unsigned int dstpos = _dx*(y+ry-shadowSize)+x+rx-shadowSize;
               unsigned int s = mskptr[pos] * alpha >> 8;
               if (shadow[0][dstpos]<s)
                shadow[0][dstpos] = (unsigned char)s;
              }
           }
         }
       }
     }
   }
  else if (m_shadowMode == 1) //Gradient classic shadow
   {
    unsigned int shadowStep = shadowAlpha/shadowSize;
    for (unsigned int y=0; y<_dy;y++)
     {
      for (unsigned int x=0; x<_dx;x++)
       {
        unsigned int pos = _dx*y+x;
        if (mskptr[pos] == 0) continue;

        unsigned int shadowAlphaGradient = shadowAlpha;
        for (unsigned int xx=1; xx<=shadowSize; xx++)
         {
          unsigned int s = mskptr[pos]*shadowAlphaGradient>>8;
          if (x + xx < _dx)
           {
            if (y+xx < _dy && shadow[0][_dx*(y+xx)+x+xx] <s)
             shadow[0][_dx*(y+xx)+x+xx] = s;
           }
          shadowAlphaGradient -= shadowStep;
         }
       }
     }
   }
  else if (m_shadowMode == 2) //Classic shadow
   {
    for (unsigned int y=shadowSize; y<_dy;y++)
     memcpy(shadow[0]+_dx*y+shadowSize,mskptr+_dx*(y-shadowSize),_dx-shadowSize);
   }

 // Preparation for each color space
 if (csp==FF_CSP_420P)
  {
   if (create == 1)
    {
     int isColorOutline=(m_outlineYUV.U!=128 || m_outlineYUV.V!=128);
     if (Tconfig::cpu_flags&FF_CPU_MMX || Tconfig::cpu_flags&FF_CPU_MMXEXT)
      {
       unsigned int edxAlign=(_dx & ~0xf) >> 1;
       unsigned int edx=_dx >> 1;
       for (unsigned int y=0 ; y<dy[1] ; y++)
        for (unsigned int x=0 ; x<edx ; x+=8)
         {
          if (x>=edxAlign)
           x=edx - 8;
          unsigned int lum0=2*y*_dx+x*2;
          unsigned int lum1=(2*y+1)*_dx+x*2;
          unsigned int chr=y*dx[1]+x;
          YV12_lum2chr_max(&bmp[0][lum0],&bmp[0][lum1],&bmp[1][chr]);
          if (isColorOutline)
           YV12_lum2chr_max(&outline[0][lum0],&outline[0][lum1],&outline[1][chr]);
          else
           YV12_lum2chr_min(&outline[0][lum0],&outline[0][lum1],&outline[1][chr]);
          YV12_lum2chr_min(&shadow[0][lum0],&shadow [0][lum1],&shadow [1][chr]);
         }
      }
     else
      {
       unsigned int _dx1=_dx/2;
       for (unsigned int y=0;y<dy[1];y++)
        for (unsigned int x=0;x<_dx1;x++)
         {
          unsigned int lum0=2*y*_dx+x*2;
          unsigned int lum1=(2*y+1)*_dx+x*2;
          unsigned int chr=y*dx[1]+x;
          bmp[1][chr]=std::max(std::max(std::max(bmp[0][lum0],bmp[0][lum0+1]),bmp[0][lum1]),bmp[0][lum1+1]);
          if (isColorOutline)
           outline[1][chr]=std::max(std::max(std::max(outline[0][lum0],outline[0][lum0+1]),outline[0][lum1]),outline[0][lum1+1]);
          else
           outline[1][chr]=std::min(std::min(std::min(outline[0][lum0],outline[0][lum0+1]),outline[0][lum1]),outline[0][lum1+1]);
          shadow[1][chr]=std::min(std::min(std::min(shadow[0][lum0],shadow[0][lum0+1]),shadow[0][lum1]),shadow[0][lum1+1]);
         }
      }
    }
  }
 else
  {
   //RGB32
   unsigned int xy=(_dx*_dy)>>2;

   #define Y2RGB(bmp)                                                              \
    DWORD *bmp##RGB=(DWORD *)bmp[1];                                               \
    unsigned char *bmp##Y=bmp[0];                                                  \
    for (unsigned int i=xy;i;bmp##RGB+=4,bmp ## Y+=4,i--)                          \
     {                                                                             \
      *(bmp##RGB)      =*bmp##Y<<16         | *bmp##Y<<8         | *bmp##Y;        \
      *(bmp##RGB+1)    =*(bmp##Y+1)<<16     | *(bmp##Y+1)<<8     | *(bmp##Y+1);    \
      *(bmp##RGB+2)    =*(bmp##Y+2)<<16     | *(bmp##Y+2)<<8     | *(bmp##Y+2);    \
      *(bmp##RGB+3)    =*(bmp##Y+3)<<16     | *(bmp##Y+3)<<8     | *(bmp##Y+3);    \
     }
   if (create == 1)
    {
     Y2RGB(bmp)
     Y2RGB(outline)
    }
   if (create)
    {
     Y2RGB(shadow)
    }
   Y2RGB(msk)
  }
 _mm_empty();
}

size_t TrenderedTextSubtitleWord::getMemorySize() const
{
    return 4 * (((dx[0] +  m_outlineWidth*2) * (dy[0] +  m_outlineWidth*2) + 32) + (dx[1] * dy[1] +32));
}

TrenderedTextSubtitleWord::~TrenderedTextSubtitleWord()
{
 if (secondaryColoredWord)
  delete secondaryColoredWord;
}

unsigned int TrenderedTextSubtitleWord::getShadowSize(LONG fontHeight, unsigned int gdi_font_scale)
{
 if (prefs.shadowSize==0 || prefs.shadowMode==3)
  return 0;
 if (prefs.shadowSize < 0) // SSA/ASS/ASS2
  {
   //if (prefs.clipdy && props.refResY)
   // return -1 * prefs.shadowSize * prefs.clipdy / props.refResY;
   //else
    return -1 * prefs.shadowSize;
  }

 unsigned int shadowSize = prefs.shadowSize*fontHeight/(gdi_font_scale * 45)+2.6;
 if (prefs.shadowMode==0)
  shadowSize*=0.6;
 else if (prefs.shadowMode==1)
  shadowSize/=1.4142;  // 1.4142 = sqrt(2.0)
 else if (prefs.shadowMode==2)
  shadowSize*=0.4;

 if (shadowSize==0)
  shadowSize = 1;
 if (shadowSize>16)
  shadowSize = 16;
 return shadowSize;
}

unsigned char* TrenderedTextSubtitleWord::blur(unsigned char *src,stride_t Idx,stride_t Idy,int startx,int starty,int endx, int endy, bool mild)
{
 /*
  *  Copied and modified from guliverkli, Rasterizer.cpp
  *
  *  Copyright (C) 2003-2006 Gabest
  *  http://www.gabest.org
  */
 unsigned char *dst = aligned_calloc3<uint8_t>(Idx,Idy,16);
 int sx=startx <= 0 ? 1 : startx;
 int sy=starty <= 0 ? 1 : starty;
 int ex=endx >= Idx ? Idx-1 : endx;
 int ey=endy >= Idy ? Idy-1 : endy;

 if (mild)
  {
   for (int y=sy ; y < ey ; y++)
    for (int x=sx ; x < ex ; x++)
     {
      int pos=Idx*y+x;
      unsigned char *srcpos=src+pos;
      dst[pos] =   (srcpos[-1-Idx]   + (srcpos[-Idx] << 1) +  srcpos[+1-Idx]
                 + (srcpos[-1] << 1) + (srcpos[0]*20)      + (srcpos[+1] << 1)
                 +  srcpos[-1+Idx]   + (srcpos[+Idx] << 1) +  srcpos[+1+Idx]) >> 5;
     }
  }
 else
  {
   for (int y=sy ; y < ey ; y++)
    for (int x=sx ; x < ex ; x++)
     {
      int pos=Idx*y+x;
      unsigned char *srcpos=src+pos;
      dst[pos] =   (srcpos[-1-Idx]   + (srcpos[-Idx] << 1) +  srcpos[+1-Idx]
                 + (srcpos[-1] << 1) + (srcpos[0] << 2)    + (srcpos[+1] << 1)
                 +  srcpos[-1+Idx]   + (srcpos[+Idx] << 1) +  srcpos[+1+Idx]) >> 4;
     }
  }
 if (startx==0)
  for (int y=starty ; y<endy ; y++)
   dst[Idx*y]=src[Idx*y];
 if (endx==Idx-1)
  for (int y=starty ; y<endy ; y++)
   dst[Idx*y+endx]=src[Idx*y+endx];
 if (starty==0)
  memcpy(dst,src,Idx);
 if (endy==Idy-1)
  memcpy(dst+Idx*endy,src+Idx*endy,Idx);
 aligned_free(src);
 return dst;
}

CRect TrenderedTextSubtitleWord::getOverhangPrivate()
{
    int top_left;

    if (prefs.shadowMode==0)
        top_left = m_shadowSize + m_outlineWidth;
    else
        top_left = m_outlineWidth;

    return CRect(top_left, top_left, m_shadowSize + m_outlineWidth, m_shadowSize + m_outlineWidth);
}

double TrenderedTextSubtitleWord::get_baseline() const
{
 return baseline;
}

double TrenderedTextSubtitleWord::get_below_baseline() const
{
 return m_descent + m_linegap;
}

double TrenderedTextSubtitleWord::get_linegap() const
{
 return m_linegap;
}

double TrenderedTextSubtitleWord::get_ascent() const
{
 return m_ascent;
}

double TrenderedTextSubtitleWord::get_descent() const
{
 return m_descent;
}

void TrenderedTextSubtitleWord::print(int startx, int starty, unsigned int sdx[3], int sdy[3], unsigned char *dstLn[3], const stride_t stride[3], const unsigned char *Ibmp[3], const unsigned char *Imsk[3],REFERENCE_TIME rtStart) const
{
 if (sdy[0] <= 0 || sdy[1] < 0 || dx[0] <= 0 || dy[0] <= 0)
  return;

 // karaoke: use secondaryColoredWord if not highlighted.
 if ((props.karaokeMode == TSubtitleProps::KARAOKE_k || props.karaokeMode == TSubtitleProps::KARAOKE_ko) && rtStart < props.karaokeStart && secondaryColoredWord)
  return secondaryColoredWord->print(startx, starty, sdx, sdy, dstLn, stride, Ibmp, Imsk, rtStart);

 const TcspInfo *cspInfo = csp_getInfo(prefs.csp);
 int srcOffset = startx < 0 ? -startx : 0;
 if (props.karaokeMode == TSubtitleProps::KARAOKE_kf && secondaryColoredWord)
  {
   if (rtStart < props.karaokeStart)
    {
     secondaryColoredWord->dstOffset = 0;
     return secondaryColoredWord->print(startx, starty, sdx, sdy, dstLn, stride, Ibmp, Imsk, rtStart);
    }
   if (rtStart < props.karaokeStart + props.karaokeDuration && props.karaokeDuration)
    {
     unsigned int sdx2[3];
     int offset = (double)(rtStart - props.karaokeStart) / props.karaokeDuration * dx[0];
     if (offset < srcOffset)
      {
       sdx2[0] = sdx[0];
       sdx2[1] = sdx[1];
      }
     else
      {
       if ((int)sdx[0] > offset - srcOffset)
        {
         sdx2[0] = sdx[0] - offset + srcOffset;
         sdx2[1] = sdx2[0] >> cspInfo->shiftX[1];
        }
       else
        {
         sdx2[0] = 0;
         sdx2[1] = 0;
        }
      }
     int startx2 = -std::max(srcOffset, offset);
     secondaryColoredWord->dstOffset = std::max(offset - srcOffset, 0);
     secondaryColoredWord->print(startx2, starty, sdx2, sdy, dstLn, stride, Ibmp, Imsk, rtStart);
     sdx[0] -= sdx2[0];
     sdx[1] = sdx[0] >> cspInfo->shiftX[1];
    }
  }

 int srcOffsetUV = srcOffset >> 1;
 int srcOffsetRGB = srcOffset << 2;
 int dstOffsetUV = dstOffset >> 1;
 int dstOffsetRGB = dstOffset << 2;
 int startyUV = starty >> 1;
 int bodyYUVa = m_bodyYUV.A;
 int outlineYUVa = m_outlineYUV.A;
 int shadowYUVa = m_shadowYUV.A;
 if (props.isFad)
  {
   double fader = 1.0;
   if      (rtStart < props.fadeT1)
    fader = props.fadeA1 / 255.0;
   else if (rtStart < props.fadeT2)
    fader = (double)(rtStart - props.fadeT1) / (props.fadeT2 - props.fadeT1) * (props.fadeA2 - props.fadeA1) / 255.0 + props.fadeA1 / 255.0; 
   else if (rtStart < props.fadeT3)
    fader = props.fadeA2 / 255.0;
   else if (rtStart < props.fadeT4)
    fader = (double)(props.fadeT4 - rtStart) / (props.fadeT4 - props.fadeT3) * (props.fadeA2 - props.fadeA3) / 255.0 + props.fadeA3 / 255.0;
   else
    fader = props.fadeA3 / 255.0;
   bodyYUVa = bodyYUVa * fader;
   outlineYUVa = outlineYUVa * fader;
   shadowYUVa = shadowYUVa * fader;
   updateMask(int(fader * (1 << 16)), false);  // updateMask doesn't accept floating point because it use MMX.
  }
#ifdef WIN64
 if (Tconfig::cpu_flags&FF_CPU_SSE2)
  {
   unsigned char xmmregs[16*16];
   storeXmmRegs(xmmregs);
#else
 if (Tconfig::cpu_flags&(FF_CPU_SSE2|FF_CPU_MMX))
  {
#endif
   if (csp==FF_CSP_420P)
    {
     //YV12
     unsigned int halfAlingXsize=alignXsize>>1;
     unsigned short* colortbl=(unsigned short *)aligned_malloc(192,16);
     for (unsigned int i=0;i<halfAlingXsize;i++)
      {
       colortbl[i]   =(short)m_bodyYUV.Y;
       colortbl[i+8] =(short)m_bodyYUV.U;
       colortbl[i+16]=(short)m_bodyYUV.V;
       colortbl[i+24]=(short)bodyYUVa;
       colortbl[i+32]=(short)m_outlineYUV.Y;
       colortbl[i+40]=(short)m_outlineYUV.U;
       colortbl[i+48]=(short)m_outlineYUV.V;
       colortbl[i+56]=(short)outlineYUVa;
       colortbl[i+64]=(short)m_shadowYUV.Y;
       colortbl[i+72]=(short)m_shadowYUV.U;
       colortbl[i+80]=(short)m_shadowYUV.V;
       colortbl[i+88]=(short)shadowYUVa;
      }
     // Y
     const unsigned char *mask0=msk[0];
     //if (bodyYUVa!=256)
     // mask0=outline[0];

     int endx=sdx[0] & ~(alignXsize-1);
     for (int y=0 ; y < sdy[0] ; y++)
      if (y + starty >=0)
       for (int x=0 ; x < endx ; x+=alignXsize)
        {
         int srcPos=y * dx[0] + x + srcOffset;
         int dstPos=y * stride[0] + x + dstOffset;
         TtextSubtitlePrintY(&bmp[0][srcPos],&outline[0][srcPos],&shadow[0][srcPos],colortbl,&dstLn[0][dstPos],&msk[0][srcPos]);
        }
     if (endx < (int)sdx[0])
      {
       for (int y=0;y<sdy[0];y++)
        if (y + starty >=0)
         for (unsigned int x=endx;x<sdx[0];x++)
          {
           #define YV12_Y_FONTRENDERER                                              \
           int srcPos=y * dx[0] + x + srcOffset;                                    \
           int dstPos=y * stride[0] + x + dstOffset;                                \
           int s=shadowYUVa * shadow[0][srcPos] >> 6;                               \
           int d=((256-s) * dstLn[0][dstPos] >> 8) + (s * m_shadowYUV.Y >> 8);      \
           int o=outlineYUVa * outline[0][srcPos] >> 6;                             \
           int b=bodyYUVa * bmp[0][srcPos] >> 6;                                    \
           int m=msk[0][srcPos];                                                    \
               d=((64-m) * d >> 6) + (o * m_outlineYUV.Y >> 8);                     \
               dstLn[0][dstPos]=d + (b * m_bodyYUV.Y >> 8);

           YV12_Y_FONTRENDERER
          }
       }
     // UV
     const unsigned char *mask1=msk[1];
     //if (bodyYUVa!=256)
     // mask1=outline[1];

     endx=sdx[1] & ~(alignXsize-1);
     for (int y=0;y<sdy[1];y++)
      if (y + startyUV >= 0)
       for (int x=0 ; x < endx ; x+=alignXsize)
        {
         int srcPos=y * dx[1] + x + srcOffsetUV;
         int dstPos=y * stride[1] + x + dstOffsetUV;
         TtextSubtitlePrintUV(&bmp[1][srcPos],&outline[1][srcPos],&shadow[1][srcPos],colortbl,&dstLn[1][dstPos],&dstLn[2][dstPos]);
        }
     if (endx < (int)sdx[1])
      {
       for (int y=0;y<sdy[1];y++)
        if (y + startyUV >= 0)
         for (int x=0 ; x < (int)sdx[1] ; x++)
          {
           #define YV12_UV_FONTRENDERER                                             \
           int srcPos=y * dx[1] + x + srcOffsetUV;                                  \
           int dstPos=y * stride[1] + x + dstOffsetUV;                              \
           /* U */                                                                  \
           int s=shadowYUVa * shadow[1][srcPos] >> 6;                               \
           int d=((256-s) * dstLn[1][dstPos] >> 8) + (s * m_shadowYUV.U >> 8);      \
           int o=outlineYUVa * outline[1][srcPos] >> 6;                             \
           int b=bodyYUVa * bmp[1][srcPos] >> 6;                                    \
               d=((256-o) * d >> 8) + (o * m_outlineYUV.U >> 8);                    \
               dstLn[1][dstPos]=((256-b) * d >> 8) + (b * m_bodyYUV.U >> 8);        \
           /* V */                                                                  \
               d=((256-s) * dstLn[2][dstPos] >> 8) + (s * m_shadowYUV.V >> 8);      \
               d=((256-o) * d >> 8) + (o * m_outlineYUV.V >> 8);                    \
               dstLn[2][dstPos]=((256-b) * d >> 8) + (b * m_bodyYUV.V >> 8);

           YV12_UV_FONTRENDERER
          }
      }
     aligned_free(colortbl);
    }
   else
    {
     //RGB32
     unsigned int halfAlingXsize=alignXsize>>1;
     unsigned short* colortbl=(unsigned short *)aligned_malloc(192,16);
     colortbl[ 0]=colortbl[ 4]=(short)m_bodyYUV.b;
     colortbl[ 1]=colortbl[ 5]=(short)m_bodyYUV.g;
     colortbl[ 2]=colortbl[ 6]=(short)m_bodyYUV.r;
     colortbl[ 3]=colortbl[ 7]=0;
     colortbl[32]=colortbl[36]=(short)m_outlineYUV.b;
     colortbl[33]=colortbl[37]=(short)m_outlineYUV.g;
     colortbl[34]=colortbl[38]=(short)m_outlineYUV.r;
     colortbl[35]=colortbl[39]=0;
     colortbl[64]=colortbl[68]=(short)m_shadowYUV.b;
     colortbl[65]=colortbl[69]=(short)m_shadowYUV.g;
     colortbl[66]=colortbl[70]=(short)m_shadowYUV.r;
     colortbl[67]=colortbl[71]=0;
     for (unsigned int i=0;i<halfAlingXsize;i++)
      {
       colortbl[i+24]=(short)bodyYUVa;
       colortbl[i+56]=(short)outlineYUVa;
       colortbl[i+88]=(short)shadowYUVa;
      }

     int endx2=sdx[0]*4;
     int endx=endx2 & ~(alignXsize-1);
     for (int y=0;y<sdy[0];y++)
      if (y + starty >=0)
       for (int x=0 ; x < endx ; x+=alignXsize)
        {
         int srcPos=y * dx[1] + x + srcOffsetRGB;
         int dstPos=y * stride[0] + x + dstOffsetRGB;
         TtextSubtitlePrintY(&bmp[1][srcPos],&outline[1][srcPos],&shadow[1][srcPos],colortbl,&dstLn[0][dstPos],&msk[1][srcPos]);
        }
     if (endx<endx2)
      {
       for (int y=0 ; y < sdy[1] ; y++)
        if (y + starty >=0)
         for (int x=endx ; x < endx2 ; x+=4)
          {
           #define RGBFONTRENDERER \
           int srcPos=y * dx[1] + x + srcOffsetRGB;                               \
           int dstPos=y * stride[0] + x + dstOffsetRGB;                           \
           /* B */                                                                \
           int s=shadowYUVa * shadow[1][srcPos] >> 6;                             \
           int d=((256-s) * dstLn[0][dstPos] >> 8) + (s * m_shadowYUV.b >> 8);    \
           int o=outlineYUVa * outline[1][srcPos] >> 6;                           \
           int b=bodyYUVa * bmp[1][srcPos] >> 6;                                  \
           int m=msk[1][srcPos];                                                  \
               d=((64-m) * d >> 6)+(o * m_outlineYUV.b >> 8);                     \
               dstLn[0][dstPos]=d + (b * m_bodyYUV.b >> 8);                       \
           /* G */                                                                \
               d=((256-s) * dstLn[0][dstPos+1] >> 8)+(s * m_shadowYUV.g >> 8);    \
               d=((64-m) * d >> 6)+(o * m_outlineYUV.g >> 8);                     \
               dstLn[0][dstPos + 1]=d + (b * m_bodyYUV.g >> 8);                   \
           /* R */                                                                \
               d=((256-s) * dstLn[0][dstPos+2] >> 8)+(s * m_shadowYUV.r >> 8);    \
               d=((64-m) * d >> 6)+(o * m_outlineYUV.r >> 8);                     \
               dstLn[0][dstPos + 2]=d + (b * m_bodyYUV.r >> 8);

           RGBFONTRENDERER
          }
      }
     aligned_free(colortbl);
    }
#ifdef WIN64
   restoreXmmRegs(xmmregs);
#endif
  }
 else
  {
   if (csp==FF_CSP_420P)
    {
     // YV12-Y
     const unsigned char *mask0=msk[0];
     //if (bodyYUVa!=256)
     // mask0=outline[0];

     for (int y=0 ; y < sdy[0] ; y++)
      if (y + starty >=0)
       for (int x=0 ; x < (int)sdx[0] ; x++)
        {
         YV12_Y_FONTRENDERER
        }
     const unsigned char *mask1=msk[1];
     if (bodyYUVa!=256)
      mask1=outline[1];

     for (int y=0 ; y < sdy[1] ; y++)
      if (y + startyUV >=0)
       for (int x=0 ; x < (int)sdx[1] ; x++)
        {
         YV12_UV_FONTRENDERER
        }
    }
   else
    {
     //RGB32
     for (int y=0 ; y < sdy[1] ; y++)
      if (y + starty >=0)
       for (int x=0 ; x < (int)sdx[0]*4 ; x+=4)
        {
         RGBFONTRENDERER
        }
    }
  }
 _mm_empty();
}
