/*
 * Copyright (c) 2003-2006 Milan Cutka
 *               2007-2011 h.yamagata
 * subtitles fixing code from SubRip by MJQ (subrip@divx.pl)
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

/*
 * Subtitle reader with format autodetection
 *
 * Written by laaz
 * Some code cleanup & realloc() by A'rpi/ESP-team
 * dunnowhat sub format by szabi
 *
 * Orginal code is from mplayer project, file subreader.c
 * convert from c to c++ by Milan Cutka. If you add another
 * subtitle format plese add it also to subreader.c (mplayer project).
 *
 * 21.04.2009 fix vplayer format, crash before. Kamil Dziobek
 */

#include "stdafx.h"
#include "TSSAstyles.h"
#include "TsubtitleProps.h"
#include "TsubreaderMplayer.h"

bool TSSAstyle::toCOLORREF(const ffstring& colourStr, COLORREF &colour, int &alpha)
{
    if (colourStr.empty()) {
        return false;
    }
    int radix;
    ffstring s1, s2;
    s1 = colourStr;
    s1.ConvertToUpperCase();
    if (s1.compare(0, 2, L"&H", 2) == 0) {
        s1.erase(0, 2);
        radix = 16;
    } else {
        if (s1.compare(0, 1, L"-", 1) == 0) {
            colour = 0x000000;
            alpha = 256;
            return true;
        }
        radix = 10;
    }
    s2 = s1;
    if (s1.size() > 6) {
        s1.erase(s1.size() - 6, 6);
        s2.erase(0, s2.size() - 6);
    } else {
        s1.clear();
    }

    int msb = 0;
    if (!s1.empty()) {
        const wchar_t *alphaS = s1.c_str();
        wchar_t *endalpha;
        long a = strtol(alphaS, &endalpha, radix);
        if (*endalpha == '\0') {
            msb = a;
        }
    }
    if (s2.empty()) {
        return false;
    }
    const wchar_t *colorS = s2.c_str();
    wchar_t *endcolor;
    COLORREF c = strtol(colorS, &endcolor, radix);
    if (*endcolor == '\0') {
        DWORD result = msb * (radix == 16 ? 0x1000000 : 1000000) + c;
        colour = result & 0xffffff;
        alpha = 256 - (result >> 24);
        return true;
    }
    return false;
}

void TSSAstyle::toProps(void)
{
    if (fontname) {
        text<char_t>(fontname.c_str(), -1, props.fontname, countof(props.fontname));
    }
    if (int size = atoi(fontsize.c_str())) {
        props.size = size;
    }
    bool isColor = toCOLORREF(primaryColour, props.color, props.colorA);
    isColor |= toCOLORREF(secondaryColour, props.SecondaryColour, props.SecondaryColourA);
    isColor |= toCOLORREF(tertiaryColour, props.TertiaryColour, props.TertiaryColourA);
    isColor |= toCOLORREF(outlineColour, props.OutlineColour, props.OutlineColourA);
    if (version == nmTextSubtitles::SSA) {
        isColor |= toCOLORREF(backgroundColour, props.OutlineColour, props.OutlineColourA);
        props.ShadowColour = props.OutlineColour;
        props.ShadowColourA = 128;
    } else {
        isColor |= toCOLORREF(backgroundColour, props.ShadowColour, props.ShadowColourA);
    }
    props.isColor = isColor;
    if (bold == L"0") {
        props.bold = 0;
    } else if (bold.size()) {
        props.bold = 1;
    }
    if (italic != L"0" && italic.size()) {
        props.italic = true;
    }
    if (underline != L"0" && underline.size()) {
        props.underline = true;
    }
    if (strikeout != L"0" && strikeout.size()) {
        props.strikeout = true;
    }
    nmTextSubtitles::strToInt(encoding, &props.encoding);
    nmTextSubtitles::strToDouble(spacing, &props.spacing);
    nmTextSubtitles::strToDouble(fontScaleX, &props.scaleX);
    nmTextSubtitles::strToDouble(fontScaleY, &props.scaleY);
    if (props.scaleX > 0) {
        props.scaleX /= 100.0;
    }
    if (props.scaleY > 0) {
        props.scaleY /= 100.0;
    }
    nmTextSubtitles::strToInt(alignment, &props.alignment);
    nmTextSubtitles::strToDouble(angleZ, &props.angleZ);
    nmTextSubtitles::strToInt(marginLeft, &props.marginL);
    nmTextSubtitles::strToInt(marginRight, &props.marginR);
    nmTextSubtitles::strToInt(marginV, &props.marginV);
    nmTextSubtitles::strToInt(marginTop, &props.marginTop);
    nmTextSubtitles::strToInt(marginBottom, &props.marginBottom);
    nmTextSubtitles::strToInt(borderStyle, (int *)&props.borderStyle);
    nmTextSubtitles::strToDouble(outlineWidth, &props.outlineWidth);
    nmTextSubtitles::strToDouble(shadowDepth, &props.shadowDepth);
    if (alignment && this->version != nmTextSubtitles::SSA) {
        props.alignment = TSubtitleProps::alignASS2SSA(props.alignment);
    }
}

void TSSAstyles::add(TSSAstyle &s)
{
    s.toProps();
    insert(std::make_pair(s.name, s));
}

const TSubtitleProps* TSSAstyles::getProps(const ffstring &style) const
{
    std::map<ffstring, TSSAstyle, ffstring_iless>::const_iterator si = this->find(style);
    if (si != this->end()) {
        return &si->second.props;
    }

    std::map<ffstring, TSSAstyle, ffstring_iless>::const_iterator iDefault = this->find(ffstring(L"Default"));
    if (iDefault != this->end()) {
        return &iDefault->second.props;
    }

    iDefault = this->find(ffstring(L"*Default"));

    if (iDefault != this->end()) {
        return &iDefault->second.props;
    } else {
        return NULL;
    }
}
