/*
 * Copyright (c) 2002-2006 Milan Cutka
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
#include "TimgFilterCrop.h"

TimgFilterCrop::TimgFilterCrop(IffdshowBase *Ideci,Tfilters *Iparent):TimgFilter(Ideci,Iparent)
{
 oldSettings.magnificationX=-1;
}

Trect TimgFilterCrop::calcCrop(const Trect &pictRect,const TcropSettings *cfg)
{
 int rcx,rcy,rcdx,rcdy;
 switch (cfg->mode)
  {
   case 0:
    {
     int magX=cfg->magnificationX;
     int magY=(cfg->magnificationLocked)?cfg->magnificationX:cfg->magnificationY;
     rcdx=(magX<0)?pictRect.dx:((100-magX)*pictRect.dx)/100;
     rcx =(pictRect.dx-rcdx)/2;
     rcdy=(magY<0)?pictRect.dy:((100-magY)*pictRect.dy)/100;
     rcy =(pictRect.dy-rcdy)/2;
     break;
    }
   case 1:
    rcdx=pictRect.dx-(cfg->cropLeft+cfg->cropRight );
    rcdy=pictRect.dy-(cfg->cropTop +cfg->cropBottom);
    rcx=cfg->cropLeft;rcy=cfg->cropTop;
    break;
   default:
    return pictRect;
  }
 rcdx&=~7;rcdy&=~7;if (rcdx<=0) rcdx=8;if (rcdy<=0) rcdy=8;
 if (rcx+rcdx>=(int)pictRect.dx) rcx=pictRect.dx-rcdx;if (rcy+rcdy>=(int)pictRect.dy) rcy=pictRect.dy-rcdy;
 if (rcx>=int(pictRect.dx-8)) rcx=pictRect.dx-8;if (rcy>=int(pictRect.dy-8)) rcy=pictRect.dy-8;
 rcx&=~1;rcy&=~1;
 return Trect(rcx,rcy,rcdx,rcdy);
}
void TimgFilterCrop::onSizeChange(void)
{
 oldSettings.magnificationX=-1;
}

bool TimgFilterCrop::getOutputFmt(TffPictBase &pict,const TfilterSettingsVideo *cfg0)
{
 if (is(pict,cfg0))
  {
   const TcropSettings *cfg=(const TcropSettings*)cfg0;
   pict.rectClip=Trect(calcCrop(pict.rectClip,cfg),pict.rectClip.sar);
   return true;
  }
 else
  return false;
}

HRESULT TimgFilterCrop::process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0)
{
 const TcropSettings *cfg=(const TcropSettings*)cfg0;
 //init(pict,0,0);
 if (!cfg->equal(oldSettings) || pict.rectClip!=oldRect)
  {
   oldSettings=*cfg;
   oldRect=pict.rectClip;
   rectCrop=calcCrop(pict.rectClip,cfg);
  }
 csp_yuv_adj_to_plane(pict.csp,&pict.cspInfo,pict.rectFull.dy,pict.data,pict.stride);
 pict.rectClip=Trect(rectCrop,pict.rectClip.sar);pict.calcDiff();
 return parent->deliverSample(++it,pict);
}

//========================= TimgFilterCropExpand ==============================
TimgFilterCropExpand::TimgFilterCropExpand(IffdshowBase *Ideci,Tfilters *Iparent):TimgFilterExpand(Ideci,Iparent)
{
 oldSettings.magnificationX=-1;
}
bool TimgFilterCropExpand::is(const TffPictBase &pict,const TfilterSettingsVideo *cfg0)
{
 if (TimgFilter::is(pict,cfg0))
  {
   const TcropSettings *cfg=(const TcropSettings*)cfg0;
   Trect newrect=TimgFilterCrop::calcCrop(pict.rectFull,cfg);
   return pict.rectFull.dx!=newrect.dx || pict.rectFull.dy!=newrect.dy;
  }
 else
  return false;
}

bool TimgFilterCropExpand::getOutputFmt(TffPictBase &pict,const TfilterSettingsVideo *cfg0)
{
 if (TimgFilter::getOutputFmt(pict,cfg0))
  {
   const TcropSettings *cfg=(const TcropSettings*)cfg0;
   calcNewRect(TimgFilterCrop::calcCrop(pict.rectClip,cfg),pict.rectFull,pict.rectClip);
   return true;
  }
 else
  return false;
}

void TimgFilterCropExpand::getDiffXY(const TffPict &pict,const TfilterSettingsVideo *cfg,int &diffx,int &diffy)
{
 if (diffy==INT_MIN)
  {
   Trect newrect=TimgFilterCrop::calcCrop(pict.rectClip,(TcropSettings*)cfg);
   diffx=newrect.x;diffy=newrect.y;
  }
}

HRESULT TimgFilterCropExpand::process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0)
{
 const TcropSettings *cfg=(const TcropSettings*)cfg0;
 if (is(pict,cfg))
  {
   if (!cfg->equal(oldSettings))
    {
     oldSettings=*cfg;
     onSizeChange();
    }
   expand(pict,cfg,true);
  }
 return parent->deliverSample(++it,pict);
}
