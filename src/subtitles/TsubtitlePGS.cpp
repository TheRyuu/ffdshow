/*
 * Copyright (c) 2004-2010 Damien Bain-Thouverez
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
#include "Tstream.h"
#include "Tconfig.h"
#include "TglobalSettings.h"
#include "ffdshow_mediaguids.h"
#include "TcodecSettings.h"
#include "rational.h"
#include "line.h"
#include "simd.h"
#include "TimgFilterSubtitles.h"
#include "TsubtitlePGS.h"
#include "TffPict.h"

TsubtitlePGS::TsubtitlePGS(IffdshowBase *Ideci, REFERENCE_TIME Istart, REFERENCE_TIME Istop, TcompositionObject *IpCompositionObject,
                           TwindowDefinition *IpWindow,
                           TsubtitleDVDparent *Iparent):
    TsubtitleDVD(Istart, Iparent),
    deci(Ideci),
    m_pCompositionObject(IpCompositionObject),
    m_pWindow(IpWindow)
{
    assert(m_pCompositionObject != NULL);
    m_pCompositionObject->m_pSubtitlePGS = this;
    updateTimestamps();
    videoWidth = m_pCompositionObject->m_pVideoDescriptor->nVideoWidth;
    videoHeight = m_pCompositionObject->m_pVideoDescriptor->nVideoHeight;
}

void TsubtitlePGS::updateTimestamps(void)
{
    start = _I64_MIN;
    stop = _I64_MAX;
    if (m_pWindow->m_rtStart != INVALID_TIME) {
        start = m_pWindow->m_rtStart;
    }
    if (m_pWindow->m_rtStop != INVALID_TIME) {
        stop = m_pWindow->m_rtStop;
    }
}


TsubtitlePGS::~TsubtitlePGS()
{
    //ownimage is freed by TsubtitleLines
}

void TsubtitlePGS::print(
    REFERENCE_TIME time,
    bool wasseek,
    Tfont &f,
    bool forceChange,
    TprintPrefs &prefs,
    unsigned char **dst,
    const stride_t *stride)
{
    //DPRINTF(_l("TsubtitlePGS::print %I64i"),start);
    //TsubPrintPrefs *subPrefs = (TsubPrintPrefs*)&prefs;
    csp = prefs.csp & FF_CSPS_MASK;
    updateTimestamps();

    if (m_pWindow->data[0].size() == 0 ||
            m_pWindow->m_rtStart > time || (m_pWindow->m_rtStop != INVALID_TIME && m_pWindow->m_rtStop <= time)) {
        return;
    }

    if (m_pWindow->ownimage == NULL || forceChange) {
#if DEBUG_PGS
        char_t rtString[32], rtString2[32];
        rt2Str(m_pWindow->m_rtStart, rtString);
        rt2Str(m_pWindow->m_rtStop, rtString2);
        DPRINTF(_l("TsubtitlePGS::print Object [%d] (%d x %d) at (%d,%d) %s -> %s "),
                m_pCompositionObject->m_compositionNumber, m_pWindow->m_width, m_pWindow->m_height,
                m_pWindow->m_horizontal_position, m_pWindow->m_vertical_position,
                rtString, rtString2);
#endif
        parent->rectOrig = Trect(0, 0, prefs.dx, prefs.dy);
        if (videoWidth == 0) {
            videoWidth = prefs.dx;
        }
        if (videoHeight == 0) {
            videoHeight = prefs.dy;
        }

        int scale100 = (int)(100 * ((float)prefs.dx / videoWidth));
        scale100 = (int)((float)scale100 * prefs.subimgscale / 256);

        // size : original size, newSize : size after scaling
        CSize size = CSize(m_pWindow->m_width, m_pWindow->m_height);

        // Rectangle of our subtitles with the original size and top left position
        CRect rc(CPoint(m_pWindow->m_horizontal_position, m_pWindow->m_vertical_position), size);

        // Real size of out subtitles rectangle after it has been reduced (due to transparent aeras) = cropping rectangle
        CRect rectReal(INT_MAX / 2, INT_MAX / 2, INT_MIN / 2, INT_MIN / 2);

        // Allocate the planes with original size and position
        TspuPlane *planes = parent->allocPlanes(rc, prefs.csp);

        Tbitdata bitdata = Tbitdata(&m_pWindow->data[0][0], m_pWindow->data[0].size());
        uint32_t bTemp;
        uint32_t bSwitch;
        int nPaletteIndex = 0; //Index of RGBA color
        int nCount = 0; // Repetition count of the pixel
        int pictureIndex = 0;

        CPoint pt = CPoint(rc.left, rc.top);

        while (pt.y < rc.bottom) {
            if (bitdata.bitsleft <= 0) {
                pictureIndex++;
                if (pictureIndex < RLE_ARRAY_SIZE && m_pWindow->data[pictureIndex].size() > 0) {
                    bitdata = Tbitdata(&m_pWindow->data[pictureIndex][0], m_pWindow->data[pictureIndex].size());
                } else {
                    break;
                }
            }

            bTemp = bitdata.readByte();
            if (bTemp != 0) {
                nPaletteIndex = bTemp;
                nCount = 1;
            } else {
                bSwitch = bitdata.readByte();
                if (!(bSwitch & 0x80)) {
                    if (!(bSwitch & 0x40)) {
                        nCount = bSwitch & 0x3F;
                        if (nCount > 0) {
                            nPaletteIndex = 0;
                        }
                    } else {
                        nCount = (bSwitch & 0x3F) << 8 | (SHORT)bitdata.readByte();
                        nPaletteIndex = 0;
                    }
                } else {
                    if (!(bSwitch & 0x40)) {
                        nCount = bSwitch & 0x3F;
                        nPaletteIndex = bitdata.readByte();
                    } else {
                        nCount = (bSwitch & 0x3F) << 8 | (SHORT)bitdata.readByte();
                        nPaletteIndex = bitdata.readByte();
                    }
                }
            } // if bTemp !=0

            // 1 or more pixels to fill with the palette index
            if (nCount > 0) { // Fill this series of pixels on this line
                bool nextline = false;
                if (pt.x + nCount > rc.right) { // Beyond the line (error)
#if DEBUG_PGS
                    DPRINTF(_l("TsubtitlePGS::print RLE data beyond line width starting at (%d,%d) on %d pixels (width=%d), changing to %d pixels"),
                            pt.x, pt.y, nCount, rc.Width(), (rc.right - pt.x));
#endif
                    nCount = rc.right - pt.x;
                    nextline = true;
                }

                if (nPaletteIndex != 0xFF && nCount > 0) { // 255 = Fully transparent
                    uint32_t color = m_pCompositionObject->m_Colors[nPaletteIndex];
                    // There is bug somewhere (in the PGS parsing) : R and B are inversed. Probably due to memory read/write order
                    unsigned char alpha = (color >> 24) & 0xFF;
                    YUVcolorA c(RGB((color) & 0xFF,
                                    (color >> 8) & 0xFF,
                                    (color >> 16) & 0xFF), alpha);
                    drawPixels(pt, nCount, c, rc, rectReal, planes, true);
                }
                pt.x += nCount;

                if (nextline) {
                    pt.y++;
                    pt.x = rc.left;
                }
            } else { // This line is fully filled
                pt.y++;
                pt.x = rc.left;
            }
        } //While

        // Recover the skipped edges
        rectReal.left--;

        // First garbage line
        rectReal.top++;

        // Apply crop area
        if (m_pWindow->m_object_cropped_flag) {
#if DEBUG_PGS
            DPRINTF(_l("Crop (%d,%d) on (%d x %d) inside (%d,%d) on (%d x %d)"), m_pWindow->m_cropping_horizontal_position, m_pWindow->m_cropping_vertical_position,
                    m_pWindow->m_cropping_width, m_pWindow->m_cropping_height,
                    m_pWindow->m_horizontal_position, m_pWindow->m_vertical_position,
                    m_pWindow->m_width, m_pWindow->m_height);
#endif
            int leftCrop = abs(m_pWindow->m_cropping_horizontal_position - m_pWindow->m_horizontal_position);
            int rightCrop = abs(m_pWindow->m_horizontal_position + m_pWindow->m_width - (m_pWindow->m_cropping_horizontal_position + m_pWindow->m_cropping_width));
            int topCrop = abs(m_pWindow->m_cropping_vertical_position - m_pWindow->m_vertical_position);
            int bottomCrop = abs(m_pWindow->m_vertical_position + m_pWindow->m_height - (m_pWindow->m_cropping_vertical_position + m_pWindow->m_cropping_height));

            if (rectReal.left < leftCrop) {
                rectReal.left = leftCrop;
            }
            if (rectReal.right > m_pWindow->m_width - rightCrop) {
                rectReal.right = m_pWindow->m_width - rightCrop;
            }
            if (rectReal.top < topCrop) {
                rectReal.top = topCrop;
            }
            if (rectReal.bottom > m_pWindow->m_height - bottomCrop) {
                rectReal.bottom = m_pWindow->m_height - bottomCrop;
            }
        }

        // Now reposition the center of the real rectangle (rcclip) proportionnaly to original size % new size
        CPoint centerPoint = CPoint(rc.left + rectReal.left + rectReal.Width() / 2, rc.top + rectReal.top + rectReal.Height() / 2);

        // Recalculate the coordinates proportionally
        centerPoint.x = (int)((float)centerPoint.x * scale100 / 100);
        centerPoint.y = (int)((float)centerPoint.y * scale100 / 100);

        CSize newSize((int)((float)rectReal.Width()*scale100 / 100), (int)((float)rectReal.Height()*scale100 / 100));
        CPoint newTopLeft(centerPoint.x - newSize.cx / 2, centerPoint.y - newSize.cy / 2);
        /*if (newSize.cx > (long)prefs.dx) newSize.cx= (long)prefs.dx-1;
        if (newSize.cy > (long)prefs.dy) newSize.cy= (long)prefs.dy-1;*/
        if (newTopLeft.x + newSize.cx > (long)prefs.dx) {
            newTopLeft.x = (long)prefs.dx - newSize.cx;
        }
        if (newTopLeft.y + newSize.cy > (long)prefs.dy) {
            newTopLeft.y = (long)prefs.dy - newSize.cy;
        }
        if (newTopLeft.x < 0) {
            newTopLeft.x = 0;
        }
        if (newTopLeft.y < 0) {
            newTopLeft.y = 0;
        }

        // Rectangle of our subtitles with the final size and position
        CRect rcclip(newTopLeft, newSize);

        //DPRINTF(_l("TsubtitlePGS::print Build image original(left,right,top,bottom)=(%ld,%ld,%ld,%ld) resized(%ld,%ld,%ld,%ld)"),rectReal.left, rectReal.right, rectReal.top, rectReal.bottom, rcclip.left, rcclip.right, rcclip.top, rcclip.bottom);
        m_pWindow->ownimage = createNewImage(planes, rc, rectReal, rcclip, prefs);
    }
    //DPRINTF(_l("TsubtitlePGS::print Print image"));
    m_pWindow->ownimage->ownprint(prefs, dst, stride);
}
