/*
 * Copyright (c) 2011 h.yamagata
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "stdafx.h"
#include "TsubtitleMixedProps.h"
#include "Tfont.h"
#include "math.h"
#include "nmTextSubtitles.h"

TSubtitleMixedProps::TSubtitleMixedProps(const TSubtitleProps &props, const TprintPrefs &prefs):
    TSubtitleProps(props),
    opaqueBox(false),
    bodyBlurCount(0),
    outlineBlurCount(0),
    shadowMode(TfontSettings::ClassicShadow),
    autoSize(false),
    blurStrength(TfontSettings::Stronger),
    hqBorder(true)
{
    const TfontSettings &fontSettings = prefs.fontSettings;

    csp = prefs.csp & FF_CSPS_MASK;
    sar = prefs.sar;
    dx = prefs.dx;
    dy = prefs.dy;
    clipdy = prefs.clipdy;
    gdi_font_scale = fontSettings.gdi_font_scale;

    if (gdi_font_scale != 64 || !fontSettings.hqBorder)
        hqBorder = false;

    // non SSA/ASS, font settings over-ride or OSD
    if (shadowDepth == -1 || fontSettings.shadowOverride || prefs.shadowMode == TfontSettings::ShadowOSD) {
        shadowMode = fontSettings.shadowMode;
        shadowDepth = fontSettings.shadowSize;
    }

    // Outline width
    if (outlineWidth == -1 || fontSettings.outlineWidthOverride) {
        outlineWidth = fontSettings.outlineWidth;
    }

    if ((borderStyle == Not_specified && fontSettings.opaqueBox) ||
        (outlineWidth > 0 && fontSettings.opaqueBox && fontSettings.fontSettingsOverride) ||
        (outlineWidth > 0 && borderStyle == Opaquebox && !fontSettings.fontSettingsOverride)) {
        opaqueBox=true;
    }

    bodyYUV = (isColor && fontSettings.colorOverride == 0) ? YUVcolorA(color,colorA) : prefs.yuvcolor;
    outlineYUV = (isColor && fontSettings.colorOverride == 0) ? YUVcolorA(OutlineColour,OutlineColourA) : prefs.outlineYUV;
    shadowYUV = (isColor && fontSettings.colorOverride == 0) ? YUVcolorA(ShadowColour,ShadowColourA) : prefs.shadowYUV;

    // non SSA/ASS or shadow over-ride
    if (!isSSA() || fontSettings.shadowOverride) {
        if (prefs.shadowMode <= TfontSettings::GradientShadow) {
            shadowYUV.A = uint32_t(256*sqrt((double)shadowYUV.A/256.0));
        }
    }

    scaleY = get_yscale(fontSettings.yscale/100.0, prefs.sar, fontSettings.fontSettingsOverride);

    scaleX =  get_xscale(fontSettings.xscale/100.0,
                               Rational(1,1),      // To avoid calculation error, we don't mix aspect ratio and ScaleX here.
                               fontSettings.fontSettingsOverride);

    if (fontSettings.autosize && !isSSA())
        autoSize = true;

    if (!isSSA() || prefs.fontSettings.fontSettingsOverride)
        blurStrength = prefs.blurStrength;

    bodyBlurCount = getBlurCountBody(prefs);
    outlineBlurCount = getBlurCountOutline(prefs);

    if (!isSSA()) {
        refResX = prefs.xinput;
        refResY = prefs.yinput;
    }

    // Avoid division by zero.
    if (!refResX)
        refResX = 384;
    if (!refResY)
        refResY = 288;

    if (fontSettings.scaleBorderAndShadowOverride && isSSA() && !fontSettings.fontSettingsOverride)
        scaleBorderAndShadow = true;

    if (scaleBorderAndShadow) {
        shadowDepth *= (double)dy / refResY;
        outlineWidth *= (double)dy / refResY;
    }

    if (size > 0 && fontSettings.sizeOverride == 0) {
        size = size * (prefs.clipdy ? prefs.clipdy : dy) / refResY;
    } else {
        size = limit(fontSettings.getSize(dx,dy), 3U, 255U);
    }

    if (bold==-1 || fontSettings.fontSettingsOverride) {
        lfWeight=fontSettings.weight;
    } else if (bold==0) {
        lfWeight=0;
    } else {
        lfWeight=700;
    }

    if ((italic && !fontSettings.fontSettingsOverride) || (fontSettings.italic && isSSA() && fontSettings.fontSettingsOverride) || (fontSettings.italic && !isSSA())) {
        italic=1;
    } else {
        italic=0;
    }

    if ((underline && !fontSettings.fontSettingsOverride) || (fontSettings.underline && isSSA() && fontSettings.fontSettingsOverride) || (fontSettings.underline && !isSSA())) {
        underline = 1;
    } else {
        underline = 0;
    }

    if (encoding == -1 || fontSettings.fontSettingsOverride)
        encoding = fontSettings.charset;

    if (!fontname[0] || fontSettings.fontSettingsOverride)
        ff_strncpy(fontname, fontSettings.name, LF_FACESIZE);

    calculated_spacing = get_spacing(prefs);
}

TSubtitleMixedProps::TSubtitleMixedProps():
    opaqueBox(false),
    bodyBlurCount(0),
    outlineBlurCount(0),
    shadowMode(TfontSettings::ClassicShadow),
    autoSize(false),
    blurStrength(TfontSettings::Stronger),
    hqBorder(true)
{
}

// Compute blur count
// Before calling these functions, make sure m_outlineWidth is set.
int TSubtitleMixedProps::getBlurCountBody(const TprintPrefs &prefs) const
{
    // ASS and the settings from the stream is respected.
    if (blur_be > 0 && prefs.fontSettings.fontSettingsOverride == 0) {
        if (outlineWidth == 0)
            return blur_be;
        else
            return 0;
    }

    // non ASS or style over-ride by users. The settings from the dialog box is used.
    if (!isSSA() || prefs.fontSettings.fontSettingsOverride) {
        if (prefs.fontSettings.blur == 1)
            return 1;
        else
            return 0;
    }

    // Glowing shadow does not look nice unless the border is blured.
    if (prefs.shadowMode == TfontSettings::GlowingShadow && shadowDepth > 0) {
        if (outlineWidth == 0)
            return 1;
        else
            return 0;
    }

    return 0;
}

int TSubtitleMixedProps::getBlurCountOutline(const TprintPrefs &prefs) const
{
    // ASS and the settings from the stream is respected.
    if (blur_be > 0 && prefs.fontSettings.fontSettingsOverride == 0) {
        if (outlineWidth > 0)
            return blur_be;
        else
            return 0;
    }

    // non ASS or style over-ride by users. The settings from the dialog box is used.
    if (!isSSA() || prefs.fontSettings.fontSettingsOverride) {
        if (prefs.fontSettings.blur == 2)
            return 1;
        else
            return 0;
    }

    // Glowing shadow does not look nice unless the border is blured.
    if (prefs.shadowMode == TfontSettings::GlowingShadow && shadowDepth > 0) {
        if (outlineWidth > 0)
            return 1;
        else
            return 0;
    }

    return 0;
}

CPoint TSubtitleMixedProps::get_rotationAxis() const
{
    CPoint result;

    if (isMove || isOrg) {
        if (isOrg) {
            result = org;
        } else {
            result = pos;
        }
        result.x = result.x * dx / refResX;
        result.y = result.y * dy / refResY;
        return result;
    }

    switch (alignment) {
        case 5: // SSA top
        case 6:
        case 7:
            result.y = (LONG)get_marginTop();
            break;
        case 9: // SSA mid
        case 10:
        case 11:
            result.y = dy/2;
            break;
        case 1: // SSA bottom
        case 2:
        case 3:
        default:
            result.y = dy - (LONG)get_marginBottom();
            break;
    }

    switch (alignment) {
        case 1: // left(SSA)
        case 5:
        case 9:
            result.x = (LONG)get_marginL();
            break;
        case 3: // right(SSA)
        case 7:
        case 11:
            result.x = dx - (LONG)get_marginR();
            break;
        case 2: // center(SSA)
        case 6:
        case 10:
        default:
            // Center alignment : pos.x anchors to the center of the paragraph
            result.x = ((((LONG)((double)dx + get_marginL() - get_marginR()))) + 1) >> 1; // same as (dx - get_marginL() - get_marginR()) / 2 + get_marginL();
            break;
    }
    return result;
}

double TSubtitleMixedProps::get_marginR(double lineWidth) const
{
    // called only for SSA/ASS/ASS2
    double result;

    // Revert the line size to the input dimension for calculation
    lineWidth=lineWidth * refResX / dx;

    if (isMove) {
        switch (alignment) {
            case 1: // left(SSA)
            case 5:
            case 9:
                result = 0;
                break;
            case 3: // right(SSA)
            case 7:
            case 11:
                result=refResX - pos.x; // Right alignment : pos.x anchors to the right of paragraph
                break;
            case 2: // center(SSA)
            case 6:
            case 10:
            default:
                // Center alignment : pos.x anchors to the center of paragraph
                if (lineWidth == 0) {
                    result = 0;
                } else {
                    result = refResX - pos.x - lineWidth / 2.0;
                }
                break;
        }
    } else if (marginR >= 0) {
        result = marginR;
    } else {
        return 0;
    }

    if (result < 0) {
        result = 0;
    }
    return result * dx / refResX;
}

double TSubtitleMixedProps::get_marginL(double lineWidth) const
{
    // called only for SSA/ASS/ASS2
    double result;

    // Revert the line size to the input dimension for calculation
    lineWidth = lineWidth * refResX / dx;

    if (isMove) {
        switch (alignment) {
            case 1: // left(SSA)
            case 5:
            case 9:
                result = pos.x; // Left alignment : pos.x anchors to the left part of paragraph
                break;
            case 3: // right(SSA)
            case 7:
            case 11:
                result = 0;
                break;
            case 2: // center(SSA)
            case 6:
            case 10:
            default:
                // Center alignment : pos.x anchors to the center of the paragraph
                result = pos.x - lineWidth / 2.0;
                break;
        }
    } else if (marginL >= 0) {
        result = marginL;
    } else {
        return 0;
    }

    if (result < 0) {
        result = 0;
    }
    return result * dx / refResX;
}

double TSubtitleMixedProps::get_marginTop() const
{
    double result;

    if (isMove) {
        switch (alignment) {
            case 5: // SSA top
            case 6:
            case 7:
                result = pos.y;
                break;
            case 9: // SSA mid
            case 10:
            case 11:
                result = pos.y;
                break;
            case 1: // SSA bottom
            case 2:
            case 3:
            default:
                result = 0;
                break;
        }
    } else if (marginTop > 0) {
        result = marginTop;    //ASS
    } else if (marginV > 0) {
        result = marginV;    // SSA
    } else {
        return 0;
    }

    if (result < 0) {
        result = 0;
    }
    return result * dy / refResY;
}

double TSubtitleMixedProps::get_marginBottom() const
{
    double result;

    if (isMove) {
        switch (alignment) {
            case 5: // SSA top
            case 6:
            case 7:
                result = 0;
                break;
            case 9: // SSA mid
            case 10:
            case 11:
                result = 0;
                break;
            case 1: // SSA bottom
            case 2:
            case 3:
            default:
                result = refResY - pos.y;
                break;
        }
    } else if (marginBottom > 0) {
        result = marginBottom;    //ASS
    } else if (marginV > 0) {
        result = marginV;    // SSA
    } else {
        return 0;
    }

    if (result < 0) {
        result = 0;
    }
    return result * dy / refResY;
}

double TSubtitleMixedProps::get_movedistanceV() const
{
    if (!isMove) {
        return 0;
    }
    return ((double)(pos2.y - pos.y)) * ((double)dy / refResY);
}

double TSubtitleMixedProps::get_movedistanceH() const
{
    if (!isMove) {
        return 0;
    }
    return ((double)(pos2.x - pos.x)) * ((double)dx / refResX);
}

CRect TSubtitleMixedProps::get_clip() const
{
    if (!isClip)
        return CRect(0,0,dx >= 1 ? dx - 1 : 0, dy >= 1 ? dy -1 : 0);

    CRect result;

    result.left = clip.left * dx / refResX;
    result.top = clip.top * dy / refResY;
    result.right = clip.right * dx / refResX;
    result.bottom = clip.bottom * dy / refResY;
    if (result.left > result.right) std::swap(result.left, result.right);
    if (result.top > result.bottom) std::swap(result.top, result.bottom);
    if (result.left < 0) result.left = 0;
    if (result.top < 0) result.top = 0;
    if (result.right < 0) result.right = 0;
    if (result.bottom < 0) result.bottom = 0;
    if (result.right >= (int)dx) result.right = dx >= 1 ? dx - 1 : 0;
    if (result.bottom >= (int)dy) result.bottom = dy >= 1 ? dy -1 : 0;
    if (result.left >= (int)dx) result.left = dx >= 1 ? dx - 1 : 0;
    if (result.top >= (int)dy) result.top = dy >= 1 ? dy -1 : 0;
    return result;
}

void TSubtitleMixedProps::toLOGFONT(LOGFONT &lf) const
{
    memset(&lf,0,sizeof(lf));
    lf.lfHeight = LONG (size * gdi_font_scale);
    lf.lfWidth = 0;
    lf.lfWeight = lfWeight;
    lf.lfItalic = italic;
    lf.lfUnderline = underline;
    lf.lfStrikeOut = strikeout;
    lf.lfCharSet = BYTE(encoding);
    lf.lfOutPrecision = OUT_TT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = ANTIALIASED_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;
    ff_strncpy(lf.lfFaceName,fontname,LF_FACESIZE);
}
