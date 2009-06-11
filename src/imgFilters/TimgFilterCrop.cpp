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
#include "IffdshowBase.h"
#include "IffdshowDecVideo.h"

int TimgFilterCrop::autoCropTop = 0,
    TimgFilterCrop::autoCropBottom = 0,
    TimgFilterCrop::autoCropLeft = 0,
    TimgFilterCrop::autoCropRight = 0;

TimgFilterCrop::TimgFilterCrop(IffdshowBase *Ideci,Tfilters *Iparent):TimgFilter(Ideci,Iparent)
{
 oldSettings.magnificationX=-1;
 autoCropTopStatus = 0;
 autoCropBottomStatus = 0;
 autoCropLeftStatus = 0;
 autoCropRightStatus = 0;
 lastFrameMS=0;

 TimgFilterCrop::autoCropBottom=0;
 TimgFilterCrop::autoCropTop=0;
 TimgFilterCrop::autoCropLeft=0;
 TimgFilterCrop::autoCropRight=0;
}

Trect TimgFilterCrop::calcCrop(const Trect &pictRect,const TcropSettings *cfg)
{
	int rcx,rcy,rcdx,rcdy;
 char init=0;
 unsigned char src0=0;
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
    if (rcdx<64) rcdx=64;
    if (rcdy<16) rcdy=16;
    break;
   case 3:
   case 4:
   case 5:
	   rcdx=pictRect.dx-(TimgFilterCrop::autoCropLeft+TimgFilterCrop::autoCropRight);
	   rcdy=pictRect.dy-(TimgFilterCrop::autoCropTop+TimgFilterCrop::autoCropBottom);
	   rcx=TimgFilterCrop::autoCropLeft;
	   rcy=TimgFilterCrop::autoCropTop;
	   if (rcdx<64) rcdx=64;
	   if (rcdy<16) rcdy=16;
   default:
    return pictRect;
  }
 rcdx&=~7;rcdy&=~7;if (rcdx<=0) rcdx=8;if (rcdy<=0) rcdy=8;
 if (rcx+rcdx>=(int)pictRect.dx) rcx=pictRect.dx-rcdx;if (rcy+rcdy>=(int)pictRect.dy) rcy=pictRect.dy-rcdy;
 if (rcx>=int(pictRect.dx-8)) rcx=pictRect.dx-8;if (rcy>=int(pictRect.dy-8)) rcy=pictRect.dy-8;
 rcx&=~1;rcy&=~1;
 return Trect(rcx,rcy,rcdx,rcdy);
}

Trect TimgFilterCrop::calcCrop(const Trect &pictRect,TcropSettings *cfg, TffPict *ppict)
{
 int rcx,rcy,rcdx,rcdy;
 const unsigned char *src;
 switch (cfg->mode)
  {
   case 0:
   case 1:
		return TimgFilterCrop::calcCrop(pictRect,cfg);
   case 3: // Vertical autocrop
   case 4: // Horizontal autocrop
   case 5: // Vertical and horizontal autocrop
	   if (ppict!=NULL)
	   {
		   unsigned int msec=0;
		   deciV->getCurrentFrameTimeMS(&msec);
		   if ((cfg->cropStopScan == 0 || msec <= (unsigned int)cfg->cropStopScan) &&
			   ((lastFrameMS >= msec && lastFrameMS-msec >= (unsigned int)cfg->cropRefreshDelay)
			   || (lastFrameMS < msec && msec-lastFrameMS >= (unsigned int)cfg->cropRefreshDelay)))
		   {
			   lastFrameMS=msec;
			   init(*ppict,cfg->full,cfg->half);
			   getCur(FF_CSPS_MASK_YUV_PLANAR,*ppict,COPYMODE_FULL,&src,NULL,NULL,NULL);
			   if (cfg->mode==3 || cfg->mode ==5)
			   {
				   calcAutoCropVertical(cfg, src, 0, 1, &(TimgFilterCrop::autoCropTop), &autoCropTopStatus); // Calculate autocrop from top to bottom
				   calcAutoCropVertical(cfg, src, dy1[0]-1, -1, &(TimgFilterCrop::autoCropBottom), &autoCropBottomStatus); // Calculate autocrop from bottom to top
			   }
			   if (cfg->mode==4 || cfg->mode ==5)
			   {
				   calcAutoCropHorizontal(cfg, src, 0, 1, &(TimgFilterCrop::autoCropLeft), &autoCropLeftStatus); // Calculate autocrop from left to right
				   calcAutoCropHorizontal(cfg, src, dx1[0]-1, -1, &(TimgFilterCrop::autoCropRight), &autoCropRightStatus); // Calculate autocrop from right to left
			   }
			   DPRINTF(_l("Ms : %i, Top %i, Bottom %i, Left %i, Right %i"),msec, TimgFilterCrop::autoCropTop, TimgFilterCrop::autoCropBottom,
                   TimgFilterCrop::autoCropLeft, TimgFilterCrop::autoCropRight);
		   }		   
	   }
	   rcdx=pictRect.dx-(TimgFilterCrop::autoCropLeft+TimgFilterCrop::autoCropRight);
	   rcdy=pictRect.dy-(TimgFilterCrop::autoCropTop+TimgFilterCrop::autoCropBottom);
	   rcx=TimgFilterCrop::autoCropLeft;
	   rcy=TimgFilterCrop::autoCropTop;
	   if (rcdx<64) rcdx=64;
	   if (rcdy<16) rcdy=16;
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

void TimgFilterCrop::calcAutoCropVertical(TcropSettings *cfg, const unsigned char *src, unsigned int y0, int stepy, int *autoCrop, int *autoCropStatus)
{
	bool first=true;
	int32_t avg0=0;
    int32_t stdev0=0;
	unsigned int x,y;
	int step=4; // Analyze every 4 pixels to gain speed
	for (y=y0;(y-y0)*stepy<dy1[0]*0.5;y+=stepy) // Top <-> bottom, scan limited to 40% of the screen
	{
     unsigned int nbPixelsAnalyzed=0; // Number of pixels analyzed per line (column actually)
     float s2=0, s=0; // Sum of levels^2, sum of levels
     for (x=0;x<stride1[0];x+=step) 
	 {
       unsigned int pos=x+stride1[0]*y;
       s2 += src[pos]*src[pos];
       s += src[pos];
       nbPixelsAnalyzed++;
     }

      float avg = s/nbPixelsAnalyzed; // average of the line
      float stdev = sqrt(s2/nbPixelsAnalyzed-avg*avg); // standard deviation of the line

      if (first) // First line = reference line
      {
       avg0=avg;
       stdev0=stdev;
       if (stdev > cfg->cropTolerance) // Too much variation of luminance for first line
       {
        if ((float)(y-y0)*stepy > (float)dy1[0]*0.15) // Look for a better reference line if we are below 15%
         break;
       }
       first=false;
       continue;
      }

      if ((float)abs(avg0-avg)*100/avg > (float) cfg->cropTolerance) // Luminance too different from reference line
       break;

      if (stdev > cfg->cropTolerance) // Too much variation of luminance for this line
        break;
	}
	// If crop result is more than 40% of the screen, give up (means that it is a blank frame)
	if ((float)abs((float)y-y0)/(float)dy1[0] < 0.4)
	{
		if (*autoCropStatus == 0) // Autocrop not done yet
		{
			*autoCrop=abs((float)y-y0);
			//*autoCropStatus = 1; // Now crop can only done in reduction not in growth
		}
		else if ((int)abs((float)y-y0) < *autoCrop)
			*autoCrop=abs((float)y-y0);
	}
}

void TimgFilterCrop::calcAutoCropHorizontal(TcropSettings *cfg, const unsigned char *src, unsigned int x0, int stepx, int *autoCrop, int *autoCropStatus)
{
	bool first=true;
	int32_t avg0=0;
    int32_t stdev0=0;
	unsigned int x,y;
	int step=1; // Analyze every 4 pixels to gain speed
	for (x=x0;(x-x0)*stepx<dx1[0]*0.5;x+=stepx) // Left <-> right, scan limited to 50% of the screen
	{
        unsigned int nbPixelsAnalyzed=0; // Number of pixels analyzed per line (column actually)
		float s2=0, s=0; // Sum of levels^2, sum of levels
		for (y=0;y<dy1[0];y+=step)
		{
         unsigned int pos=x+stride1[0]*y;
         s2 += src[pos]*src[pos];
         s += src[pos];
         nbPixelsAnalyzed++;
		}
		float avg = s/nbPixelsAnalyzed; // average of the line
        float stdev = sqrt(s2/nbPixelsAnalyzed-avg*avg); // standard deviation of the line

        if (first) // First line = reference line
        {
         avg0=avg;
         stdev0=stdev;
         first=false;
         if (stdev > cfg->cropTolerance) // Too much variation of luminance for first line
         {
          if ((float)(x-x0)*stepx<(float)dx1[0]*0.15) // Look for a better reference line if we are below 15%
           break;
         }
         continue;
        }
        if ((float)abs(avg0-avg)*100/avg > (float) cfg->cropTolerance) // Luminance too different from reference line
         break;

        if (stdev > cfg->cropTolerance) // Too much variation of luminance for this line
         break;
	}
	// If crop result is more than 40% of the screen, give up (means that it is a blank frame)cd 
	if ((float)abs((float)x-x0)/(float)dx1[0] < 0.4)
	{
		if (*autoCropStatus == 0) // Autocrop not done yet
		{
			*autoCrop=abs((float)x-x0);
			//*autoCropStatus = 1; // Now crop can only done in reduction not in growth
		}
		else if ((int)abs((float)x-x0) < *autoCrop)
			*autoCrop=abs((float)x-x0);
	}
}

void TimgFilterCrop::onSizeChange(void)
{
 oldSettings.magnificationX=-1;
}

bool TimgFilterCrop::getOutputFmt(TffPictBase &pict,const TfilterSettingsVideo *cfg0)
{
 if (is(pict,cfg0))
  {
   TcropSettings *cfg=(TcropSettings*)cfg0;
   pict.rectClip=Trect(calcCrop(pict.rectClip,cfg, NULL),pict.rectClip.sar);
   return true;
  }
 else
  return false;
}

HRESULT TimgFilterCrop::process(TfilterQueue::iterator it,TffPict &pict,const TfilterSettingsVideo *cfg0)
{
 TcropSettings *cfg=(TcropSettings*)cfg0;
 //init(pict,0,0);
 if (!cfg->equal(oldSettings) || pict.rectClip!=oldRect 
	 || (cfg->mode>=3 || cfg->mode<=5))
  {
   oldSettings=*cfg;
   oldRect=pict.rectClip;
   if (cfg->mode != oldSettings.mode) // If config has changed, reinitialize autoCrop settings
   {
		autoCropTopStatus = 0;
		autoCropBottomStatus = 0;
		autoCropLeftStatus = 0;
		autoCropRightStatus = 0;
		lastFrameMS=0;
		TimgFilterCrop::autoCropBottom=0;
		TimgFilterCrop::autoCropTop=0;
		TimgFilterCrop::autoCropLeft=0;
		TimgFilterCrop::autoCropRight=0;
   }
   rectCrop=calcCrop(pict.rectClip,cfg, &pict);
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
