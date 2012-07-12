/*
 * Copyright (c) 2009 h.yamagata
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

#pragma once

#include "TfontManager.h"
#include "TsubtitleMixedProps.h"

class TtoGdiFont
{
    HGDIOBJ old;
    HDC hdc;
    int gdi_font_scale;
    TSubtitleMixedProps mprops;
public:
    LOGFONT lf;
    TtoGdiFont(const TSubtitleProps &props, HDC Ihdc, const TprintPrefs &prefs, unsigned int dx, unsigned int dy, TfontManager *fontManager):
        old(NULL),
        hdc(Ihdc),
        mprops(props, prefs),
        gdi_font_scale(prefs.fontSettings.gdi_font_scale) {
        if (!hdc || !fontManager) {
            return;
        }
        mprops.toLOGFONT(lf);
        HFONT font = fontManager->getFont(lf);
        old = SelectObject(hdc, font);
    }

    ~TtoGdiFont() {
        if (hdc && old) {
            SelectObject(hdc, old);
        }
    }

    double getHeight() {
        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);
        return (double)(tm.tmAscent + tm.tmDescent) / gdi_font_scale;
    }
};
