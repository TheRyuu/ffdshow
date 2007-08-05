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
#include "TsubtitleProps.h"
#include "TsubtitlesSettings.h"

void TSubtitleProps::reset(void)
{
 wrapStyle=-1;
 refResX=refResY=0;
 bold=-1;
 italic=underline=strikeout=false;
 isColor=false;
 isPos=false;
 size=0;
 fontname[0]='\0';
 encoding=spacing=-1;
 scaleX=scaleY=-1;
 alignment=-1;
 marginR=marginL=marginV=marginTop=marginBottom=-1;
 borderStyle=-1;
 outlineWidth=shadowDepth=-1;
 SecondaryColour=TertiaryColour=0xffffff;
 OutlineColour=ShadowColour=0;
 colorA=SecondaryColourA=TertiaryColourA=OutlineColourA=256;
 ShadowColourA=128;
 blur=0;
}

void TSubtitleProps::toLOGFONT(LOGFONT &lf,const TfontSettings &fontSettings,unsigned int dx,unsigned int dy,unsigned int clipdy) const
{
 memset(&lf,0,sizeof(lf));
 lf.lfHeight=(LONG)limit(size?size:fontSettings.getSize(dx,dy),3U,255U)*4;
 if (scaleY!=-1)
  lf.lfHeight=scaleY*lf.lfHeight/100;
 if (refResY && dy)
  lf.lfHeight=(clipdy ? clipdy : dy)*lf.lfHeight/refResY;
 lf.lfWidth=0;
 if (bold==-1)
  lf.lfWeight=fontSettings.weight;
 else if (bold==0)
  lf.lfWeight=0;
 else
  lf.lfWeight=700;
 lf.lfItalic=italic;
 lf.lfUnderline=underline;
 lf.lfStrikeOut=strikeout;
 lf.lfCharSet=BYTE(encoding!=-1?encoding:fontSettings.charset);
 lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
 lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
 lf.lfQuality=DEFAULT_QUALITY;
 lf.lfPitchAndFamily=DEFAULT_PITCH|FF_DONTCARE;
 strncpy(lf.lfFaceName,fontname[0]?fontname:fontSettings.name,LF_FACESIZE);
}

unsigned int TSubtitleProps::get_marginR(unsigned int screenWidth) const
{
 unsigned int result;
 if (marginR>0)
  result=marginR;
 else return 0;

 if (refResX>0)
  return result*screenWidth/refResX;
 else
  return result;
}
unsigned int TSubtitleProps::get_marginL(unsigned int screenWidth) const
{
 unsigned int result;
 if (marginL>0)
  result=marginL;
 else return 0;

 if (refResX>0)
  return result*screenWidth/refResX;
 else
  return result;
}
unsigned int TSubtitleProps::get_marginTop(unsigned int screenHeight) const
{
 unsigned int result;
 if (marginTop>0)
  result=marginTop; //ASS
 else if (marginV>0)
  result=marginV; // SSA
 else return 0;

 if (refResY>0)
  return result*screenHeight/refResY;
 else
  return result;
}
unsigned int TSubtitleProps::get_marginBottom(unsigned int screenHeight) const
{
 unsigned int result;
 if (marginBottom>0)
  result=marginBottom; //ASS
 else if (marginV>0)
  result=marginV; // SSA
 else return 0;

 if (refResY>0)
  return result*screenHeight/refResY;
 else
  return result;
}

int TSubtitleProps::alignASS2SSA(int align)
{
 switch (align)
  {
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
