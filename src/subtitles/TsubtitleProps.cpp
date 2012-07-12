/*
 * Copyright (c) 2003-2006 Milan Cutka
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

#include "stdafx.h"
#include "IffdshowBase.h"
#include "TsubtitleProps.h"
#include "TsubtitlesSettings.h"
#include "rational.h"
#include "TfontManager.h"
#include "Tsubreader.h"
#include <locale.h>
#include "Tfont.h"
#include "nmTextSubtitles.h"

void TSubtitleProps::reset(void)
{
    wrapStyle = -1;
    scaleBorderAndShadow = 0;
    refResX = 384; // Default VSFilter reference
    refResY = 288; // dimensions
    bold = -1;
    italic = underline = strikeout = false;
    isColor = false;
    isClip = false;
    isMove = false;
    isOrg = false;
    transform.isTransform = false;
    transform.isAlpha = false;
    transform.alpha = 0;
    transform.alphaT1 = transform.alphaT2 = REFTIME_INVALID;
    karaokeFillStart = karaokeFillEnd = REFTIME_INVALID;
    transform.accel = 1.0;
    size = 0;
    polygon = 0;
    fontname[0] = '\0';
    encoding = -1;
    spacing = INT_MIN;
    scaleX = scaleY = -1;
    alignment = -1;
    isAlignment = false;
    marginR = marginL = marginV = marginTop = marginBottom = -1;
    angleX = 0;
    angleY = 0;
    angleZ = 0;
    borderStyle = Not_specified;
    layer = 0;
    outlineWidth = shadowDepth = -1;
    color = SecondaryColour = TertiaryColour = 0xffffff;
    OutlineColour = ShadowColour = 0;
    colorA = SecondaryColourA = TertiaryColourA = OutlineColourA = 256;
    ShadowColourA = 128;
    blur_be = 0;
    gauss = 0;
    version = -1;
    tStart = tStop = fadeT1 = fadeT2 = fadeT3 = fadeT4 = REFTIME_INVALID;
    isFad = 0;
    karaokeMode = KARAOKE_NONE;
    karaokeStart = 0;
    karaokeDuration = 0;
    karaokeNewWord = false;
    extendedTags = 1;
    x = 0;
    y = 0;
    lineID = 0;
}

int TSubtitleProps::get_spacing(const TprintPrefs &prefs) const
{
    if (spacing == INT_MIN || prefs.fontSettings.fontSettingsOverride) {
        return prefs.fontSettings.spacing;
    }

    if (isSSA() && refResX) {
        return int(spacing * prefs.fontSettings.gdi_font_scale * prefs.dx / refResX);
    } else {
        return int(spacing * prefs.fontSettings.gdi_font_scale);
    }
}

double TSubtitleProps::get_maxWidth(unsigned int screenWidth, int textMarginLR, int subFormat, IffdshowBase *deci) const
{
    double result = 0;
    int resX;

    if (isSSA()) {
        resX = refResX;
    }
    // SRT subtitles. VSFilter assumes 384x288 input dimensions, and does
    // positioning calculations based on that, so we assume that too if
    // position is set (through position tags) so our results are equivalent.
    // If not, use video dimensions.
    else {
        if (isMove && !deci->getParam2(IDFF_subSSAUseMovieDimensions)) {
            resX = 384;
        } else {
            resX = screenWidth;
        }
    }
    int mL = marginL == -1 ? textMarginLR / 2 : marginL;
    int mR = marginR == -1 ? textMarginLR / 2 : marginR;


    /* Calculate the maximum width of line according to the position to be set
     Take the position into account for calculation if :
      - A position is set (through position tag) and :
       - The option "Maintain outside text inside picture" is set
       - Or if the subtitles are SRT
     */
    if (isMove && (deci->getParam2(IDFF_subSSAMaintainInside)
                   || (subFormat & Tsubreader::SUB_FORMATMASK) == Tsubreader::SUB_SUBVIEWER)) {
        switch (alignment) {
            case 1: // left(SSA) : left alignment, left margin is ignored
            case 5:
            case 9:
                // If option to maintain text inside is set then calculate max width according to the position
                if (pos.x < 0) {
                    break;
                } else {
                    result = resX - pos.x - mR;
                }
                break;
            case 3: // right(SSA) : right alignment, right margin is ignored
            case 7:
            case 11:
                // If option to maintain text inside is set then calculate max width according to the position
                if (pos.x < 0) {
                    break;
                } else {
                    result = pos.x - mL;
                }
                break;
            case 2: // center(SSA)
            case 6:
            case 10:
            default:
                // If option to maintain text inside is set then calculate max width according to the position
                // Center alignment : pos.x anchors to the center of the paragraph
                if (pos.x > 0 && pos.x < resX) {
                    // We try to calculate the maximum margin around the anchor point (at center)
                    if (mL > pos.x) {
                        mL = pos.x;    // MarginL should not exceed posX
                    }
                    if (mR > resX - pos.x) {
                        mR = resX - pos.x;  // MarginR should not exceed posX
                    }
                    int margin = std::min(pos.x - mL, resX - mR - pos.x);
                    // Calculated margin should not go out of the screen
                    if (pos.x - margin < 0) {
                        margin = pos.x;
                    }
                    if (pos.x + margin > resX) {
                        margin = resX - pos.x;
                    }
                    result = margin * 2;
                }
                break;
        }
    } else { // If no position tag is set let's take the margins into account
        result = resX - mL - mR;
    }

    if (result < 0) {
        result = 0;
    }
    if (screenWidth > result * screenWidth / resX) {
        return (result * screenWidth / resX);
    } else {
        return screenWidth;
    }
}

REFERENCE_TIME TSubtitleProps::get_moveStart() const
{
    return moveT1 * 10000 + tStart;
}

REFERENCE_TIME TSubtitleProps::get_moveStop() const
{
    if (moveT2 == 0) {
        return tStop;
    }
    return moveT2 * 10000 + tStart;
}

int TSubtitleProps::alignASS2SSA(int align)
{
    switch (align) {
        case 1:
        case 2:
        case 3:
            return align;
        case 4:
        case 5:
        case 6:
            return align + 5;
        case 7:
        case 8:
        case 9:
            return align - 2;
    }
    return align;
}

double TSubtitleProps::get_xscale(double Ixscale, const Rational& sar, int fontSettingsOverride) const
{
    double result;
    if (isSSA() && !fontSettingsOverride && scaleX == -1) {
        // SSA often comes here.
        result = 1.0;
    } else if (scaleX == -1 || fontSettingsOverride) {
        result = Ixscale;
    } else {
        result = scaleX;
    }

    if (sar.num > sar.den) {
        result = result * sar.den / sar.num;
    }
    return result;
}

double TSubtitleProps::get_yscale(double Iyscale, const Rational& sar, int fontSettingsOverride) const
{
    double result;
    if (isSSA() && !fontSettingsOverride && scaleY == -1) {
        // SSA often comes here.
        result = 1.0;
    } else if (scaleY == -1 || fontSettingsOverride) {
        result = Iyscale;
    } else {
        result = scaleY;
    }
    if (sar.num < sar.den) {
        result = result * sar.num / sar.den;
    }
    return result;
}

bool TSubtitleProps::isSSA() const
{
    if (version < nmTextSubtitles::SSA) {
        return false;
    }
    if (version > nmTextSubtitles::ASS2) {
        return false;
    }
    return true;
}
