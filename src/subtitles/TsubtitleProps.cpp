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

void TSubtitleProps::reset(void)
{
    wrapStyle=-1;
    scaleBorderAndShadow=0;
    refResX=384; // Default VSFilter reference
    refResY=288; // dimensions
    bold=-1;
    italic=underline=strikeout=false;
    isColor=false;
    isPos=false;
    isMove=false;
    isOrg=false;
    transform.isTransform=false;
    transform.isAlpha=false;
    transform.alpha=0;
    transform.alphaT1=transform.alphaT2=REFTIME_INVALID;
    transform.accel=1.0;
    size=0;
    fontname[0]='\0';
    encoding=-1;
    spacing=INT_MIN;
    scaleX=scaleY=-1;
    alignment=-1;
    marginR=marginL=marginV=marginTop=marginBottom=-1;
    angleX=0;
    angleY=0;
    angleZ=0;
    borderStyle=-1;
    layer=0;
    outlineWidth=shadowDepth=-1;
    color=SecondaryColour=TertiaryColour=0xffffff;
    OutlineColour=ShadowColour=0;
    colorA=SecondaryColourA=TertiaryColourA=OutlineColourA=256;
    ShadowColourA=128;
    blur=0;
    version=-1;
    tStart=tStop=fadeT1=fadeT2=fadeT3=fadeT4=REFTIME_INVALID;
    isFad=0;
    karaokeMode = KARAOKE_NONE;
    karaokeStart = 0;
    karaokeDuration = 0;
    karaokeNewWord = false;
    extendedTags=1;
    x=0;
    y=0;
    lineID=0;
}

void TSubtitleProps::toLOGFONT(LOGFONT &lf,const TfontSettings &fontSettings,unsigned int dx,unsigned int dy,unsigned int clipdy,const Rational& sar,unsigned int gdi_font_scale) const
{
    memset(&lf,0,sizeof(lf));
    if (size && fontSettings.sizeOverride == 0) {
        lf.lfHeight=(LONG) size * gdi_font_scale;
        lf.lfHeight=(clipdy ? clipdy : dy)*lf.lfHeight/refResY;
    } else {
        lf.lfHeight=(LONG)limit(fontSettings.getSize(dx,dy),3U,255U) * gdi_font_scale;
    }
    int yscale=get_yscale(fontSettings.yscale,sar,fontSettings.aspectAuto,fontSettings.fontSettingsOverride);
    lf.lfHeight=lf.lfHeight*yscale/100;
    lf.lfWidth=0;
    if (bold==-1 || fontSettings.fontSettingsOverride) {
        lf.lfWeight=fontSettings.weight;
    } else if (bold==0) {
        lf.lfWeight=0;
    } else {
        lf.lfWeight=700;
    }
    if ((italic && !fontSettings.fontSettingsOverride) || (fontSettings.italic && this->version >= 4 && fontSettings.fontSettingsOverride) || (fontSettings.italic && this->version < 4)) {
        lf.lfItalic=1;
    } else {
        lf.lfItalic=0;
    }
    if ((underline && !fontSettings.fontSettingsOverride) || (fontSettings.underline && this->version >= 4 && fontSettings.fontSettingsOverride) || (fontSettings.underline && this->version < 4)) {
        lf.lfUnderline=1;
    } else {
        lf.lfUnderline=0;
    }
    lf.lfStrikeOut=strikeout;
    if (encoding != -1 && fontSettings.fontSettingsOverride == 0) {
        lf.lfCharSet=BYTE(encoding);
    } else {
        lf.lfCharSet=BYTE(fontSettings.charset);
    }
    lf.lfOutPrecision=OUT_TT_PRECIS;
    lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
    lf.lfQuality=ANTIALIASED_QUALITY;
    lf.lfPitchAndFamily=DEFAULT_PITCH|FF_DONTCARE;
    if (fontname[0] && fontSettings.fontSettingsOverride == 0) {
        ff_strncpy(lf.lfFaceName,fontname,LF_FACESIZE);
    } else {
        ff_strncpy(lf.lfFaceName,fontSettings.name,LF_FACESIZE);
    }
}

int TSubtitleProps::get_spacing(unsigned int dy, unsigned int clipdy, unsigned int gdi_font_scale) const
{
    if (refResY) {
        return int(spacing * gdi_font_scale * (clipdy ? clipdy : dy) / refResY);
    } else {
        return int(spacing * gdi_font_scale);
    }
}

int TSubtitleProps::get_marginR(unsigned int screenWidth,unsigned int lineWidth) const
{
    // called only for SSA/ASS/ASS2
    int result;
    int resX = (refResX>0) ? refResX:screenWidth;
    // Revert the line size to the input dimension for calculation
    lineWidth=lineWidth*resX/screenWidth;

    if (isPos||isMove) {
        switch (alignment) {
            case 1: // left(SSA)
            case 5:
            case 9:
                result=0;
                break;
            case 3: // right(SSA)
            case 7:
            case 11:
                result=resX-pos.x; // Right alignment : pos.x anchors to the right of paragraph
                break;
            case 2: // center(SSA)
            case 6:
            case 10:
            default:
                // Center alignment : pos.x anchors to the center of paragraph
                if (lineWidth == 0) {
                    result=0;
                } else {
                    result=resX-pos.x-(lineWidth)/2;
                }
                break;
        }
    } else if (marginR>=0) {
        result=marginR;
    } else {
        return 0;
    }

    if (result<0) {
        result=0;
    }
    return result*screenWidth/resX;
}
int TSubtitleProps::get_marginL(unsigned int screenWidth, unsigned int lineWidth) const
{
    // called only for SSA/ASS/ASS2
    int result;
    int resX = (refResX>0) ? refResX:screenWidth;
    // Revert the line size to the input dimension for calculation
    lineWidth=lineWidth*resX/screenWidth;

    if (isPos||isMove) {
        switch (alignment) {
            case 1: // left(SSA)
            case 5:
            case 9:
                result=pos.x; // Left alignment : pos.x anchors to the left part of paragraph
                break;
            case 3: // right(SSA)
            case 7:
            case 11:
                result=0;
                break;
            case 2: // center(SSA)
            case 6:
            case 10:
            default:
                // Center alignment : pos.x anchors to the center of the paragraph
                result=pos.x-(lineWidth)/2;
                break;
        }
    } else if (marginL>=0) {
        result=marginL;
    } else {
        return 0;
    }

    if (result<0) {
        result=0;
    }
    return result*screenWidth/resX;
}

int TSubtitleProps::get_marginTop(unsigned int screenHeight) const
{
    int result;

    int resY = (refResY>0) ? refResY:screenHeight;

    if (isPos||isMove) {
        switch (alignment) {
            case 5: // SSA top
            case 6:
            case 7:
                result=pos.y;
                break;
            case 9: // SSA mid
            case 10:
            case 11:
                result=pos.y;
                break;
            case 1: // SSA bottom
            case 2:
            case 3:
            default:
                result=0;
                break;
        }
    } else if (marginTop>0) {
        result=marginTop;    //ASS
    } else if (marginV>0) {
        result=marginV;    // SSA
    } else {
        return 0;
    }

    if (result<0) {
        result=0;
    }
    return result*screenHeight/resY;
}
int TSubtitleProps::get_marginBottom(unsigned int screenHeight) const
{
    int result;
    int resY = (refResY>0) ? refResY:screenHeight;

    if (isPos||isMove) {
        switch (alignment) {
            case 5: // SSA top
            case 6:
            case 7:
                result=0;
                break;
            case 9: // SSA mid
            case 10:
            case 11:
                result=0;
                break;
            case 1: // SSA bottom
            case 2:
            case 3:
            default:
                result=resY-pos.y;
                break;
        }
    } else if (marginBottom>0) {
        result=marginBottom;    //ASS
    } else if (marginV>0) {
        result=marginV;    // SSA
    } else {
        return 0;
    }

    if (result<0) {
        result=0;
    }
    return result*screenHeight/resY;
}

int TSubtitleProps::get_maxWidth(unsigned int screenWidth, int textBorderLR, int subFormat, IffdshowBase *deci) const
{
    int result = 0;
    int resX;
    // SSA/ASS subtitles. refResX is always >0.
    if (refResX>0) {
        resX = refResX;
    }
    // SRT subtitles. VSFilter assumes 384x288 input dimensions, and does
    // positioning calculations based on that, so we assume that too if
    // position is set (through position tags) so our results are equivalent.
    // If not, use video dimensions.
    else {
        if (isPos && !deci->getParam2(IDFF_subSSAUseMovieDimensions)) {
            resX = 384;
        } else {
            resX = screenWidth;
        }
    }
    int mL = marginL == -1 ? textBorderLR/2 : marginL;
    int mR = marginR == -1 ? textBorderLR/2 : marginR;


    /* Calculate the maximum width of line according to the position to be set
     Take the position into account for calculation if :
      - A position is set (through position tag) and :
       - The option "Maintain outside text inside picture" is set
       - Or if the subtitles are SRT
     */
    if (isPos && (deci->getParam2(IDFF_subSSAMaintainInside)
                  || (subFormat & Tsubreader::SUB_FORMATMASK) == Tsubreader::SUB_SUBVIEWER)) {
        switch (alignment) {
            case 1: // left(SSA) : left alignment, left margin is ignored
            case 5:
            case 9:
                // If option to maintain text inside is set then calculate max width according to the position
                if (pos.x<0) {
                    break;
                } else {
                    result=resX-pos.x-mR;
                }
                break;
            case 3: // right(SSA) : right alignment, right margin is ignored
            case 7:
            case 11:
                // If option to maintain text inside is set then calculate max width according to the position
                if (pos.x<0) {
                    break;
                } else {
                    result=pos.x-mL;
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
                    if (mR > resX-pos.x) {
                        mR = resX-pos.x;    // MarginR should not exceed posX
                    }
                    int margin = std::min(pos.x-mL, resX-mR-pos.x);
                    // Calculated margin should not go out of the screen
                    if (pos.x-margin < 0) {
                        margin = pos.x;
                    }
                    if (pos.x+margin > resX) {
                        margin = resX-pos.x;
                    }
                    result = margin*2;
                }
                break;
        }
    } else { // If no position tag is set let's take the margins into account
        result=resX-mL-mR;
    }

    if (result<0) {
        result=0;
    }
    if (screenWidth>result*screenWidth/resX) {
        return (result*screenWidth/resX);
    } else {
        return screenWidth;
    }
}

int TSubtitleProps::get_movedistanceV(unsigned int screenHeight) const
{
    if (!isMove || !refResY) {
        return 0;
    }
    return (int)(((float)(pos2.y-pos.y))*((float)screenHeight/refResY));
}

int TSubtitleProps::get_movedistanceH(unsigned int screenWidth) const
{
    if (!isMove || !refResX) {
        return 0;
    }
    return (int)(((float)(pos2.x-pos.x))*((float)screenWidth/refResX));
}

REFERENCE_TIME TSubtitleProps::get_moveStart() const
{
    return moveT1*10000+tStart;
}

REFERENCE_TIME TSubtitleProps::get_moveStop() const
{
    if (moveT2==0) {
        return tStop;
    }
    return moveT2*10000+tStart;
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
            return align+5;
        case 7:
        case 8:
        case 9:
            return align-2;
    }
    return align;
}

int TSubtitleProps::get_xscale(int Ixscale,const Rational& sar,int aspectAuto,int fontSettingsOverride) const
{
    int result;
    if (scaleX==-1 || fontSettingsOverride) {
        result=Ixscale;
    } else {
        result=scaleX;
    }
    if ((aspectAuto) && sar.num>sar.den) {
        result=result*sar.den/sar.num;
    }
    return result;
}

int TSubtitleProps::get_yscale(int Iyscale,const Rational& sar,int aspectAuto,int fontSettingsOverride) const
{
    int result;
    if (scaleY==-1 || fontSettingsOverride) {
        result=Iyscale;
    } else {
        result=scaleY;
    }
    if ((aspectAuto) && sar.num<sar.den) {
        result=result*sar.num/sar.den;
    }
    return result;
}
