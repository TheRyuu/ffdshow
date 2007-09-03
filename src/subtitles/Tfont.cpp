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
#include "Tsubreader.h"
#include "TsubtitleText.h"
#include "Tfont.h"
#include "TfontSettings.h"
#include "IffdshowBase.h"
#include "IffdshowDecVideo.h"
#include "TfontManager.h"
#include "simd.h"
#include "Tconfig.h"
#include "ffdebug.h"
#include "postproc/swscale.h"
#include "Tlibmplayer.h"
#include "TwordWrap.h"
#include <mbstring.h>
#pragma warning(disable:4244)

//============================ The matrix for outline =============================
align16(static const short matrix0_YV12[]) ={
 100,200,100,000,000,000,000,000,
 200,800,200,000,000,000,000,000,
 100,200,100,000,000,000,000,000
};

align16(static const short matrix1[]) ={
 200,500,200,000,000,000,000,000,
 500,800,500,000,000,000,000,000,
 200,500,200,000,000,000,000,000
};

align16(static const short matrix2[]) ={
 000,200,300,200,000,000,000,000,
 200,400,500,400,200,000,000,000,
 300,500,800,500,300,000,000,000,
 200,400,500,400,200,000,000,000,
 000,200,300,200,000,000,000,000
};

//============================ TrenderedSubtitleWordBase =============================
TrenderedSubtitleWordBase::~TrenderedSubtitleWordBase()
{
 if (own)
  for (int i=0;i<3;i++)
   {
    aligned_free(bmp[i]);
    aligned_free(msk[i]);
    aligned_free(outline[i]);
    aligned_free(shadow[i]);
   }
}

//============================== TrenderedSubtitleWord ===============================

// full rendering
template<class tchar> TrenderedSubtitleWord::TrenderedSubtitleWord(
                       HDC hdc,
                       const tchar *s0,
                       size_t strlens,
                       const YUVcolorA &YUV,
                       const YUVcolorA &outlineYUV,
                       const YUVcolorA &shadowYUV,
                       const TrenderedSubtitleLines::TprintPrefs &prefs,
                       const LOGFONT &lf,
                       int xscale,
                       int yscale):
 TrenderedSubtitleWordBase(true),
 shiftChroma(true)
{
 csp=prefs.csp;
 typedef typename tchar_traits<tchar>::ffstring ffstring;
 typedef typename tchar_traits<tchar>::strings strings;
 strings s1;
 strtok(ffstring(s0,strlens).c_str(),_L("\t"),s1);
 SIZE sz;sz.cx=sz.cy=0;ints cxs;
 for (typename strings::iterator s=s1.begin();s!=s1.end();s++)
  {
   SIZE sz0;
   prefs.config->getGDI<tchar>().getTextExtentPoint32(hdc,s->c_str(),(int)s->size(),&sz0);
   sz.cx+=sz0.cx;
   if (s+1!=s1.end())
    {
     int tabsize=prefs.tabsize*sz0.cy;
     int newpos=(sz.cx/tabsize+1)*tabsize;
     sz0.cx+=newpos-sz.cx;
     sz.cx=newpos;
    }
   cxs.push_back(sz0.cx);
   sz.cy=std::max(sz.cy,sz0.cy);
  }
 OUTLINETEXTMETRIC otm;
 unsigned int shadowSize;
 SIZE sz1=sz;
 if (GetOutlineTextMetrics(hdc,sizeof(otm),&otm))
  {
   baseline=otm.otmTextMetrics.tmAscent;
   if (otm.otmItalicAngle)
    sz1.cx-=LONG(sz1.cy*sin(otm.otmItalicAngle*M_PI/1800));
   else
    if (otm.otmTextMetrics.tmItalic)
     sz1.cx+=sz1.cy*0.35;
   shadowSize = getShadowSize(prefs,otm.otmTextMetrics.tmHeight);
  }
 else
  { // non true-type
   baseline=sz1.cy*0.8;
   shadowSize = getShadowSize(prefs,lf.lfHeight);
   if (lf.lfItalic)
    sz1.cx+=sz1.cy*0.35;
  }
 dx[0]=(sz1.cx/4+2)*4;dy[0]=sz1.cy+4;
 unsigned char *bmp16=(unsigned char*)aligned_calloc3(dx[0]*4,dy[0],32,16);
 HBITMAP hbmp=CreateCompatibleBitmap(hdc,dx[0],dy[0]);
 HGDIOBJ old=SelectObject(hdc,hbmp);
 RECT r={0,0,dx[0],dy[0]};
 FillRect(hdc,&r,(HBRUSH)GetStockObject(BLACK_BRUSH));
 SetTextColor(hdc,RGB(255,255,255));
 SetBkColor(hdc,RGB(0,0,0));
 int x=2;
 ints::const_iterator cx=cxs.begin();
 for (typename strings::const_iterator s=s1.begin();s!=s1.end();s++,cx++)
  {
   const char *t=(const char *)s->c_str();
   int sz=(int)s->size();
   prefs.config->getGDI<tchar>().textOut(hdc,x,2,s->c_str(),sz/*(int)s->size()*/);
   x+=*cx;
  }
 drawShadow(hdc,hbmp,bmp16,old,xscale,yscale,sz,prefs,YUV,outlineYUV,shadowYUV,shadowSize);
}
void TrenderedSubtitleWord::drawShadow(
      HDC hdc,
      HBITMAP hbmp,
      unsigned char *bmp16,
      HGDIOBJ old,
      int xscale,
      int yscale,
      const SIZE &sz,
      const TrenderedSubtitleLines::TprintPrefs &prefs,
      const YUVcolorA &yuv,
      const YUVcolorA &outlineYUV,
      const YUVcolorA &shadowYUV,
      unsigned int shadowSize)
{
 int outlineWidth=0;
 if (!prefs.opaqueBox)
  {
   if (prefs.outlineWidth==0 && csp==FF_CSP_420P)
    outlineWidth=1;
   else
    outlineWidth=prefs.outlineWidth;
  }
 m_bodyYUV=yuv;
 m_outlineYUV=outlineYUV;
 m_shadowYUV=shadowYUV;
 BITMAPINFO bmi;
 bmi.bmiHeader.biSize=sizeof(bmi.bmiHeader);
 bmi.bmiHeader.biWidth=dx[0];
 bmi.bmiHeader.biHeight=-1*dy[0];
 bmi.bmiHeader.biPlanes=1;
 bmi.bmiHeader.biBitCount=32;
 bmi.bmiHeader.biCompression=BI_RGB;
 bmi.bmiHeader.biSizeImage=dx[0]*dy[0];
 bmi.bmiHeader.biXPelsPerMeter=75;
 bmi.bmiHeader.biYPelsPerMeter=75;
 bmi.bmiHeader.biClrUsed=0;
 bmi.bmiHeader.biClrImportant=0;
 GetDIBits(hdc,hbmp,0,dy[0],bmp16,&bmi,DIB_RGB_COLORS);  // copy bitmap, get it in bmp16 (RGB32).
 SelectObject(hdc,old);
 DeleteObject(hbmp);

 if (Tconfig::cpu_flags&FF_CPU_MMXEXT)
  {
   YV12_lum2chr_min=YV12_lum2chr_min_mmx2;
   YV12_lum2chr_max=YV12_lum2chr_max_mmx2;
  }
 else
  {
   YV12_lum2chr_min=YV12_lum2chr_min_mmx;
   YV12_lum2chr_max=YV12_lum2chr_max_mmx;
  }
#ifndef WIN64
 if (Tconfig::cpu_flags&FF_CPU_SSE2)
  {
#endif
   alignXsize=16;
   TrenderedSubtitleWord_printY=TrenderedSubtitleWord_printY_sse2;
   TrenderedSubtitleWord_printUV=TrenderedSubtitleWord_printUV_sse2;
#ifndef WIN64
  }
 else
  {
   alignXsize=8;
   TrenderedSubtitleWord_printY=TrenderedSubtitleWord_printY_mmx;
   TrenderedSubtitleWord_printUV=TrenderedSubtitleWord_printUV_mmx;
  }
#endif
 unsigned int _dx,_dy;
 _dx=xscale*dx[0]/400+4+getLeftOverhang(shadowSize,prefs)+getRightOverhang(shadowSize,prefs);
 _dy=yscale*dy[0]/400+4+getTopOverhang(shadowSize,prefs)+getBottomOverhang(shadowSize,prefs);
 dxCharY=xscale*sz.cx/400;dyCharY=yscale*sz.cy/400;
 baseline=yscale*baseline/400+2;

 unsigned int al=csp==FF_CSP_420P ? alignXsize : 8; // swscaler requires multiple of 8.
 _dx=((_dx+al-1)/al)*al;
 if (csp==FF_CSP_420P)
  _dy=((_dy+1)/2)*2;
 stride_t extra_dx=_dx+outlineWidth*2; // add margin to simplify the outline drawing process.
 stride_t extra_dy=_dy+outlineWidth*2;
 extra_dx=((extra_dx+7)/8)*8;     // align for swscaler
 bmp[0]=(unsigned char*)aligned_calloc3(extra_dx,extra_dy,16,16);
 msk[0]=(unsigned char*)aligned_calloc3(_dx,_dy,16,16);
 outline[0]=(unsigned char*)aligned_calloc3(_dx,_dy,16,16);
 shadow[0]=(unsigned char*)aligned_calloc3(_dx,_dy,16,16);
 int dxCharYstart=0;

 // bmp16 RGB32->IMGFMT_Y800 (full range) conversion.
 if (Tconfig::cpu_flags&FF_CPU_MMX)
  {
   fontRGB32toBW_mmx((dx[0]*dy[0]+7)/8,bmp16);
   _mm_empty();
  }
 else
  {
   unsigned char *bmp16_wb=bmp16;
   unsigned char *bmp16_rgb32=bmp16;
   int cxy=dx[0]*dy[0];
   while(cxy)
    {
     *bmp16_wb=*bmp16_rgb32;
     bmp16_wb++;
     bmp16_rgb32+=4;
     cxy--;
    }
  }

 // Scaling. GDI has drawn the font in 4x big size. So we have to shrink to 25% by default.
 unsigned int scaledDx=xscale*dx[0]/400;
 unsigned int scaledDy=yscale*dy[0]/400;
 if (scaledDx<8) scaledDx=8; // swscaler returns error if scaledDx<8
 Tlibmplayer *libmplayer;
 SwsParams params;
 prefs.deci->getPostproc(&libmplayer);
 float lumaGBlur=0.0;
 int resizeMethod=SWS_LANCZOS;
 if (prefs.blur || (prefs.shadowMode==0 && shadowSize>0))
  {
   lumaGBlur=1.9f;
   resizeMethod=SWS_GAUSS;
  }
 SwsFilter *filter=libmplayer->sws_getDefaultFilter(lumaGBlur,0,0,0,0,0,0);
 Tlibmplayer::swsInitParams(&params,resizeMethod);
 SwsContext *ctx=libmplayer->sws_getContext(dx[0], dy[0], IMGFMT_Y800, scaledDx, scaledDy, IMGFMT_Y800, &params, filter, NULL);
 if (ctx)
  {
   stride_t sdx=dx[0];
   uint8_t *scaledBmp=bmp[0]+outlineWidth+getLeftOverhang(shadowSize,prefs)+extra_dx*(outlineWidth+getTopOverhang(shadowSize,prefs)); // FIXME for thicker outlines.
   libmplayer->sws_scale_ordered(ctx,(const uint8_t**)&bmp16,&sdx,0,dy[0],&scaledBmp,(stride_t *)&extra_dx);
   libmplayer->sws_freeContext(ctx);
  }
 libmplayer->Release();
 libmplayer=NULL;
 aligned_free(bmp16);

 dx[0]=_dx;dy[0]=_dy;

 short *matrix=NULL;
 unsigned int matrixSizeH=((outlineWidth*2+8)/8)*8; // 2 bytes for one.
 unsigned int matrixSizeV=outlineWidth*2+1;
 if (outlineWidth>2)
  {
   double r_cutoff=(double)outlineWidth/3;
   double r_mul=800.0/r_cutoff;
   matrix=(short*)aligned_calloc(matrixSizeH*2,matrixSizeV,16);
   for (int y=-outlineWidth;y<=outlineWidth;y++)
    for (int x=-outlineWidth;x<=outlineWidth;x++)
     {
      int pos=(y+outlineWidth)*matrixSizeH+x+outlineWidth;
      double r=0.1+outlineWidth-sqrt(double(x*x+y*y));
      if (r>r_cutoff)
       matrix[pos]=800;
      else if (r>0)
       matrix[pos]=r*r_mul;
     }
  }
 else if (prefs.outlineWidth==0 && csp==FF_CSP_420P)
  matrix=(short *)matrix0_YV12;
 else if (outlineWidth==1)
  matrix=(short *)matrix1;
 else if (outlineWidth==2)
  matrix=(short *)matrix2;

 if (prefs.opaqueBox)
  memset(msk[0],255,dx[0]*dy[0]);
 else if (outlineWidth)
  {
   // Prepare outline
   if (Tconfig::cpu_flags&FF_CPU_SSE2)
    {
     size_t matrixSizeH_sse2=matrixSizeH>>3;
     size_t srcStrideGap=extra_dx-matrixSizeH;
     for (unsigned int y=0;y<_dy;y++)
      for (unsigned int x=0;x<_dx;x++)
       {
        unsigned int sum=fontPrepareOutline_sse2(bmp[0]+extra_dx*y+x,srcStrideGap,matrix,matrixSizeH_sse2,matrixSizeV)/800;
        msk[0][_dx*y+x]=sum>255 ? 255 : sum;
       }
    }
#ifndef WIN64
   else if (Tconfig::cpu_flags&FF_CPU_MMX)
    {
     size_t matrixSizeH_mmx=(matrixSizeV+3)/4;
     size_t srcStrideGap=extra_dx-matrixSizeH_mmx*4;
     size_t matrixGap=matrixSizeH_mmx & 1 ? 8 : 0;
     for (unsigned int y=0;y<_dy;y++)
      for (unsigned int x=0;x<_dx;x++)
       {
        unsigned int sum=fontPrepareOutline_mmx(bmp[0]+extra_dx*y+x,srcStrideGap,matrix,matrixSizeH_mmx,matrixSizeV,matrixGap)/800;
        msk[0][_dx*y+x]=sum>255 ? 255 : sum;
       }
    }
#endif
   else
    {
     for (unsigned int y=0;y<_dy;y++)
      for (unsigned int x=0;x<_dx;x++)
       {
        unsigned char *srcPos=bmp[0]+extra_dx*y+x; // (x-outlineWidth,y-outlineWidth)
        unsigned int sum=0;
        for (unsigned int yy=0;yy<matrixSizeV;yy++,srcPos+=extra_dx-matrixSizeV)
         for (unsigned int xx=0;xx<matrixSizeV;xx++,srcPos++)
          {
           sum+=(*srcPos)*matrix[matrixSizeH*yy+xx];
          }
        sum/=800;
        msk[0][_dx*y+x]=sum>255 ? 255 : sum;
       }
    }

   // remove the margin that we have just added.
   for (unsigned int y=0;y<_dy;y++)
    memcpy(bmp[0]+_dx*y,bmp[0]+extra_dx*(y+outlineWidth)+outlineWidth,_dx);
  }
 // Draw outline, keeping the body bright.
 unsigned int count=_dx*_dy;
 for (unsigned int c=0;c<count;c++)
  {
   int b=bmp[0][c];
   int o=msk[0][c]-b;
   if (o>0)
    outline[0][c]=o*(255-b)>>8;
  }

 unsigned int _dxUV;
 if (csp==FF_CSP_420P)
  {
   dx[1]=dx[0]>>prefs.shiftX[1];
   dy[1]=dy[0]>>prefs.shiftY[1];
   _dxUV=dx[1];
   dx[1]=(dx[1]/alignXsize+1)*alignXsize;
   bmp[1]=(unsigned char*)aligned_calloc(dx[1],dy[1],16);
   outline[1]=(unsigned char*)aligned_calloc(dx[1],dy[1],16);
   shadow[1]=(unsigned char*)aligned_calloc(dx[1],dy[1],16);

   dx[2]=dx[0]>>prefs.shiftX[2];
   dy[2]=dy[0]>>prefs.shiftY[2];
   dx[2]=(dx[2]/alignXsize+1)*alignXsize;

   bmpmskstride[0]=dx[0];bmpmskstride[1]=dx[1];bmpmskstride[2]=dx[2];
  }
 else
  {
   dx[1]=dx[0]*4;
   dy[1]=dy[0];
   bmp[1]=(unsigned char*)aligned_malloc(dx[1]*dy[1]+16,16);
   outline[1]=(unsigned char*)aligned_malloc(dx[1]*dy[1]+16,16);
   shadow[1]=(unsigned char*)aligned_malloc(dx[1]*dy[1]+16,16);

   bmpmskstride[0]=dx[0];bmpmskstride[1]=dx[1];
  }

    unsigned int shadowAlpha = 255; //prefs.shadowAlpha;
    unsigned int shadowMode = prefs.shadowMode; // 0: glowing, 1:classic with gradient, 2: classic with no gradient, >=3: no shadow
    if (shadowSize > 0)
    if (shadowMode == 0) //Gradient glowing shadow (most complex)
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
                if (msk[0][pos] == 0) continue;
                for (int ry=starty; ry<endy;ry++)
                {
                    for (int rx=startx; rx<endx;rx++)
                    {
                        unsigned int alpha = circle[circleSize*ry+rx];
                        if (alpha)
                        {
                            unsigned int dstpos = _dx*(y+ry-shadowSize)+x+rx-shadowSize;
                            unsigned int s = msk[0][pos] * alpha >> 8;
                            if (shadow[0][dstpos]<s)
                                shadow[0][dstpos] = (unsigned char)s;
                        }
                    }
                }
            }
        }
    }
    else if (shadowMode == 1) //Gradient classic shadow
    {
        unsigned int shadowStep = shadowAlpha/shadowSize;
        for (unsigned int y=0; y<_dy;y++)
        {
            for (unsigned int x=0; x<_dx;x++)
            {
                unsigned int pos = _dx*y+x;
                if (msk[0][pos] == 0) continue;

                unsigned int shadowAlphaGradient = shadowAlpha;
                for (unsigned int xx=1; xx<=shadowSize; xx++)
                {
                    unsigned int s = msk[0][pos]*shadowAlphaGradient>>8;
                    if (x + xx < _dx)
                    {
                        if (y+xx < _dy && 
                            shadow[0][_dx*(y+xx)+x+xx] <s)
                        {
                            shadow[0][_dx*(y+xx)+x+xx] = s;
                        }
                    }
                    shadowAlphaGradient -= shadowStep;
                }
            }
        }
    }
    else if (shadowMode == 2) //Classic shadow
    {
        for (unsigned int y=shadowSize; y<_dy;y++)
            memcpy(shadow[0]+_dx*y+shadowSize,msk[0]+_dx*(y-shadowSize),_dx-shadowSize);
    }

 if (csp==FF_CSP_420P)
  {
   int isColorOutline=(outlineYUV.U!=128 || outlineYUV.V!=128);
   if (Tconfig::cpu_flags&FF_CPU_MMX)
    {
     unsigned int e_dx0=_dx & ~0xf;
     unsigned int e_dx1=e_dx0/2;
     for (unsigned int y=0;y<dy[1];y++)
      for (unsigned int x=0;x<e_dx1;x+=8)
       {
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
     unsigned int e_dx2=_dx/2;
     if (e_dx2>e_dx1)
      {
       if (_dx>=16)
        {
         unsigned int ldx=_dx-16;
         unsigned int cdx=ldx/2;
         for (unsigned int y=0;y<dy[1];y++)
          {
           unsigned int lum0=2*y*_dx+ldx;
           unsigned int lum1=(2*y+1)*_dx+ldx;
           unsigned int chr=y*dx[1]+cdx;
           YV12_lum2chr_max(&bmp[0][lum0],&bmp[0][lum1],&bmp[1][chr]);
           if (isColorOutline)
            YV12_lum2chr_max(&outline[0][lum0],&outline[0][lum1],&outline[1][chr]);
           else
            YV12_lum2chr_min(&outline[0][lum0],&outline[0][lum1],&outline[1][chr]);
           YV12_lum2chr_min(&shadow[0][lum0],&shadow[0][lum1],&shadow[1][chr]);
          }
        }
       else
        {
          for (unsigned int y=0;y<dy[1];y++)
           for (unsigned int x=e_dx1;x<e_dx2;x++)
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
 else
  {
   //RGB32
   unsigned int xy=(_dx*_dy)>>2;
   DWORD *bmpRGB=(DWORD *)bmp[1];
   unsigned char *bmpY=bmp[0];
   for (unsigned int i=xy;i;bmpRGB+=4,bmpY+=4,i--)
    {
     *(bmpRGB)      =*bmpY<<16         | *bmpY<<8         | *bmpY;
     *(bmpRGB+1)    =*(bmpY+1)<<16     | *(bmpY+1)<<8     | *(bmpY+1);
     *(bmpRGB+2)    =*(bmpY+2)<<16     | *(bmpY+2)<<8     | *(bmpY+2);
     *(bmpRGB+3)    =*(bmpY+3)<<16     | *(bmpY+3)<<8     | *(bmpY+3);
    }
   DWORD *outlineRGB=(DWORD *)outline[1];
   unsigned char *outlineY=outline[0];
   for (unsigned int i=xy;i;outlineRGB+=4,outlineY+=4,i--)
    {
     *(outlineRGB)  =*outlineY<<16     | *outlineY<<8     | *outlineY;
     *(outlineRGB+1)=*(outlineY+1)<<16 | *(outlineY+1)<<8 | *(outlineY+1);
     *(outlineRGB+2)=*(outlineY+2)<<16 | *(outlineY+2)<<8 | *(outlineY+2);
     *(outlineRGB+3)=*(outlineY+3)<<16 | *(outlineY+3)<<8 | *(outlineY+3);
    }
   DWORD *shadowRGB=(DWORD *)shadow[1];
   unsigned char *shadowY=shadow[0];
   for (unsigned int i=xy;i;shadowRGB+=4,shadowY+=4,i--)
    {
     *(shadowRGB)   =*shadowY<<16      | *shadowY<<8      | *shadowY;
     *(shadowRGB+1) =*(shadowY+1)<<16  | *(shadowY+1)<<8  | *(shadowY+1);
     *(shadowRGB+2) =*(shadowY+2)<<16  | *(shadowY+2)<<8  | *(shadowY+2);
     *(shadowRGB+3) =*(shadowY+3)<<16  | *(shadowY+3)<<8  | *(shadowY+3);
    }
  }
 _mm_empty();
 aligned_free(msk[0]);
 if (prefs.outlineWidth>2 && matrix)
  aligned_free(matrix);
 msk[0]=NULL;
}
unsigned int TrenderedSubtitleWord::getShadowSize(const TrenderedSubtitleLines::TprintPrefs &prefs,LONG fontHeight)
{
 if (prefs.shadowSize==0 || prefs.shadowMode==3)
  return 0;
 if (prefs.shadowSize < 0) // SSA/ASS/ASS2
 return -1 * prefs.shadowSize;
 unsigned int shadowSize = prefs.shadowSize*fontHeight/180.0+2.6;
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
unsigned int TrenderedSubtitleWord::getBottomOverhang(unsigned int shadowSize,const TrenderedSubtitleLines::TprintPrefs &prefs)
{
 return shadowSize+prefs.outlineWidth;
}
unsigned int TrenderedSubtitleWord::getRightOverhang(unsigned int shadowSize,const TrenderedSubtitleLines::TprintPrefs &prefs)
{
 return shadowSize+prefs.outlineWidth;
}
unsigned int TrenderedSubtitleWord::getTopOverhang(unsigned int shadowSize,const TrenderedSubtitleLines::TprintPrefs &prefs)
{
 if (prefs.shadowMode==0)
  return shadowSize+prefs.outlineWidth;
 else
  return prefs.outlineWidth;
}
unsigned int TrenderedSubtitleWord::getLeftOverhang(unsigned int shadowSize,const TrenderedSubtitleLines::TprintPrefs &prefs)
{
 return getTopOverhang(shadowSize,prefs);
}

// fast rendering
// YV12 only. RGB32 is not supported.
template<class tchar> TrenderedSubtitleWord::TrenderedSubtitleWord(TcharsChache *charsChache,const tchar *s,size_t strlens,const TrenderedSubtitleLines::TprintPrefs &prefs,const LOGFONT &lf):TrenderedSubtitleWordBase(true),shiftChroma(true)
{
 csp=prefs.csp;
 m_bodyYUV=(charsChache->getBodyYUV());
 m_outlineYUV=(charsChache->getOutlineYUV());
 m_shadowYUV=(charsChache->getShadowYUV());
 const TrenderedSubtitleWord **chars=(const TrenderedSubtitleWord**)_alloca(strlens*sizeof(TrenderedSubtitleLine*));
 for (int i=0;i<3;i++)
  dx[i]=dy[i]=0;
 for (size_t i=0;i<strlens;i++)
  {
   chars[i]=charsChache->getChar(&s[i],prefs,lf);
   dx[0]+=chars[i]->dxCharY+1;
   if (s[i]=='\t')
    {
     int tabsize=prefs.tabsize*std::max(chars[0]->dyCharY,1U);
     dx[0]=(dx[0]/tabsize+1)*tabsize;
    }
   for (int p=0;p<3;p++)
    dy[p]=std::max(dy[p],chars[i]->dy[p]);
   if(sizeof(tchar)==sizeof(char))
    if(_mbclen((const unsigned char *)&s[i])==2)
     i++;
  }
 alignXsize=chars[0]->alignXsize;
 TrenderedSubtitleWord_printY=chars[0]->TrenderedSubtitleWord_printY;
 TrenderedSubtitleWord_printUV=chars[0]->TrenderedSubtitleWord_printUV;
 dx[0]=(dx[0]/alignXsize+2)*alignXsize;dx[1]=dx[2]=(dx[0]/alignXsize/2+1)*alignXsize;
 dxCharY=dx[0];
 dyCharY=chars[0]->dyCharY;
 for (int i=0;i<2;i++)
  {
   bmp[i]=(unsigned char*)aligned_calloc(dx[i],dy[i],16);
   shadow[i]=(unsigned char*)aligned_calloc(dx[i],dy[i],16);
   outline[i]=(unsigned char*)aligned_calloc(dx[i],dy[i],16);
   bmpmskstride[i]=dx[i];
  }
 unsigned int x=0;
 for (size_t i=0;i<strlens;i++)
  {
   if (s[i]=='\t')
    {
     int tabsize=prefs.tabsize*std::max(chars[0]->dyCharY,1U);
     x=(x/tabsize+1)*tabsize;
    }
   for (unsigned int p=0;p<2;p++)
    {
     const unsigned char *charbmpptr=chars[i]->bmp[p];
     const unsigned char *charshadowptr=chars[i]->shadow[p],*charoutlineptr=chars[i]->outline[p];
     unsigned char *bmpptr=bmp[p]+roundRshift(x,prefs.shiftX[p]);
     unsigned char *shadowptr=shadow[p]+roundRshift(x,prefs.shiftX[p]),*outlineptr=outline[p]+roundRshift(x,prefs.shiftX[p]);
     for (unsigned int y=0;y<chars[i]->dy[p];y++,
       bmpptr+=bmpmskstride[p],
       shadowptr+=bmpmskstride[p],outlineptr+=bmpmskstride[p],
       charbmpptr+=chars[i]->bmpmskstride[p],
       charshadowptr+=chars[i]->bmpmskstride[p],charoutlineptr+=chars[i]->bmpmskstride[p])
      {
       memadd(bmpptr,charbmpptr,chars[i]->dx[p]);
       memadd(shadowptr,charshadowptr,chars[i]->dx[p]);
       memadd(outlineptr,charoutlineptr,chars[i]->dx[p]);
      }
    }
   x+=chars[i]->dxCharY+1;
   if(sizeof(tchar)==sizeof(char))
    if(_mbclen((const unsigned char *)&s[i])==2)
     i++;
  }
 _mm_empty();
}
void TrenderedSubtitleWord::print(unsigned int sdx[3],unsigned char *dstLn[3],const stride_t stride[3],const unsigned char *bmp[3],const unsigned char *msk[3]) const
{
 if (shadow[0])
  {
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
         colortbl[i+24]=(short)m_bodyYUV.A;
         colortbl[i+32]=(short)m_outlineYUV.Y;
         colortbl[i+40]=(short)m_outlineYUV.U;
         colortbl[i+48]=(short)m_outlineYUV.V;
         colortbl[i+56]=(short)m_outlineYUV.A;
         colortbl[i+64]=(short)m_shadowYUV.Y;
         colortbl[i+72]=(short)m_shadowYUV.U;
         colortbl[i+80]=(short)m_shadowYUV.V;
         colortbl[i+88]=(short)m_shadowYUV.A;
        }
       // Y
       unsigned int endx=sdx[0] & ~(alignXsize-1);
       for (unsigned int y=0;y<dy[0];y++)
        for (unsigned int x=0;x<endx;x+=alignXsize)
         {
          int srcPos=y*dx[0]+x;
          int dstPos=y*stride[0]+x;
          TrenderedSubtitleWord_printY(&bmp[0][srcPos],&outline[0][srcPos],&shadow[0][srcPos],colortbl,&dstLn[0][dstPos]);
         }
       if (endx<sdx[0])
        {
         for (unsigned int y=0;y<dy[0];y++)
          for (unsigned int x=endx;x<sdx[0];x++)
           {
            int srcPos=y*dx[0]+x;
            int dstPos=y*stride[0]+x;
            int s=m_shadowYUV.A *shadow [0][srcPos]>>8;
            int d=((256-s)*dstLn[0][dstPos]>>8)+(s*m_shadowYUV.Y>>8);
            int b=m_bodyYUV.A   *bmp    [0][srcPos]>>8;
                d=((256-b)*d>>8)+(b*m_bodyYUV.Y>>8);
            int o=m_outlineYUV.A*outline[0][srcPos]>>8;
                dstLn[0][dstPos]=((256-o)*d>>8)+(o*m_outlineYUV.Y>>8);
           }
         }
       // UV
       endx=sdx[1] & ~(alignXsize-1);
       for (unsigned int y=0;y<dy[1];y++)
        for (unsigned int x=0;x<endx;x+=alignXsize)
         {
          int srcPos=y*dx[1]+x;
          int dstPos=y*stride[1]+x;
          TrenderedSubtitleWord_printUV(&bmp[1][srcPos],&outline[1][srcPos],&shadow[1][srcPos],colortbl,&dstLn[1][dstPos],&dstLn[2][dstPos]);
         }
       if (endx<sdx[1])
        {
         for (unsigned int y=0;y<dy[1];y++)
          for (unsigned int x=0;x<sdx[1];x++)
           {
            int srcPos=y*dx[1]+x;
            int dstPos=y*stride[1]+x;
            // U
            int s=m_shadowYUV.A *shadow [1][srcPos]>>8;
            int d=((256-s)*dstLn[1][dstPos]>>8)+(s*m_shadowYUV.U>>8);
            int b=m_bodyYUV.A   *bmp    [1][srcPos]>>8;
                d=((256-b)*d>>8)+(b*m_bodyYUV.U>>8);
            int o=m_outlineYUV.A*outline[1][srcPos]>>8;
                dstLn[1][dstPos]=((256-o)*d>>8)+(o*m_outlineYUV.U>>8);
            // V
                d=((256-s)*dstLn[2][dstPos]>>8)+(s*m_shadowYUV.V>>8);
                d=((256-b)*d>>8)+(b*m_bodyYUV.V>>8);
                dstLn[2][dstPos]=((256-o)*d>>8)+(o*m_outlineYUV.V>>8);
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
         colortbl[i+24]=(short)m_bodyYUV.A;
         colortbl[i+56]=(short)m_outlineYUV.A;
         colortbl[i+88]=(short)m_shadowYUV.A;
        }

       unsigned int endx2=sdx[0]*4;
       unsigned int endx=endx2 & ~(alignXsize-1);
       for (unsigned int y=0;y<dy[0];y++)
        for (unsigned int x=0;x<endx;x+=alignXsize)
         {
          int srcPos=y*dx[1]+x;
          int dstPos=y*stride[0]+x;
          TrenderedSubtitleWord_printY(&bmp[1][srcPos],&outline[1][srcPos],&shadow[1][srcPos],colortbl,&dstLn[0][dstPos]);
         }
       if (endx<endx2)
        {
         for (unsigned int y=0;y<dy[1];y++)
          for (unsigned int x=endx;x<endx2;x+=4)
           {
            int srcPos=y*dx[1]+x;
            int dstPos=y*stride[0]+x;
            // B
            int s=m_shadowYUV.A *shadow [1][srcPos]>>8;
            int d=((256-s)*dstLn[0][dstPos]>>8)+(s*m_shadowYUV.b>>8);
            int b=m_bodyYUV.A   *bmp    [1][srcPos]>>8;
                d=((256-b)*d>>8)+(b*m_bodyYUV.b>>8);
            int o=m_outlineYUV.A*outline[1][srcPos]>>8;
                dstLn[0][dstPos]=((256-o)*d>>8)+(o*m_outlineYUV.b>>8);
            // G
                d=((256-s)*dstLn[0][dstPos+1]>>8)+(s*m_shadowYUV.g>>8);
                d=((256-b)*d>>8)+(b*m_bodyYUV.g>>8);
                dstLn[0][dstPos+1]=((256-o)*d>>8)+(o*m_outlineYUV.g>>8);
            // R
                d=((256-s)*dstLn[0][dstPos+2]>>8)+(s*m_shadowYUV.r>>8);
                d=((256-b)*d>>8)+(b*m_bodyYUV.r>>8);
                dstLn[0][dstPos+2]=((256-o)*d>>8)+(o*m_outlineYUV.r>>8);
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
       for (unsigned int y=0;y<dy[0];y++)
        for (unsigned int x=0;x<sdx[0];x++)
         {
          int srcPos=y*dx[0]+x;
          int dstPos=y*stride[0]+x;
          int s=m_shadowYUV.A *shadow [0][srcPos]>>8;
          int d=((256-s)*dstLn[0][dstPos]>>8)+(s*m_shadowYUV.Y>>8);
          int b=m_bodyYUV.A   *bmp    [0][srcPos]>>8;
              d=((256-b)*d>>8)+(b*m_bodyYUV.Y>>8);
          int o=m_outlineYUV.A*outline[0][srcPos]>>8;
              dstLn[0][dstPos]=((256-o)*d>>8)+(o*m_outlineYUV.Y>>8);
         }
       for (unsigned int y=0;y<dy[1];y++)
        for (unsigned int x=0;x<sdx[1];x++)
         {
          int srcPos=y*dx[1]+x;
          int dstPos=y*stride[1]+x;
          // U
          int s=m_shadowYUV.A *shadow [1][srcPos]>>8;
          int d=((256-s)*dstLn[1][dstPos]>>8)+(s*m_shadowYUV.U>>8);
          int b=m_bodyYUV.A   *bmp    [1][srcPos]>>8;
              d=((256-b)*d>>8)+(b*m_bodyYUV.U>>8);
          int o=m_outlineYUV.A*outline[1][srcPos]>>8;
              dstLn[1][dstPos]=((256-o)*d>>8)+(o*m_outlineYUV.U>>8);
          // V
              d=((256-s)*dstLn[2][dstPos]>>8)+(s*m_shadowYUV.V>>8);
              d=((256-b)*d>>8)+(b*m_bodyYUV.V>>8);
              dstLn[2][dstPos]=((256-o)*d>>8)+(o*m_outlineYUV.V>>8);
         }
      }
     else
      {
       //RGB32
       for (unsigned int y=0;y<dy[1];y++)
        for (unsigned int x=0;x<sdx[0]*4;x+=4)
         {
          int srcPos=y*dx[1]+x;
          int dstPos=y*stride[0]+x;
          // B
          int s=m_shadowYUV.A *shadow [1][srcPos]>>8;
          int d=((256-s)*dstLn[0][dstPos]>>8)+(s*m_shadowYUV.b>>8);
          int b=m_bodyYUV.A   *bmp    [1][srcPos]>>8;
              d=((256-b)*d>>8)+(b*m_bodyYUV.b>>8);
          int o=m_outlineYUV.A*outline[1][srcPos]>>8;
              dstLn[0][dstPos]=((256-o)*d>>8)+(o*m_outlineYUV.b>>8);
          // G
              d=((256-s)*dstLn[0][dstPos+1]>>8)+(s*m_shadowYUV.g>>8);
              d=((256-b)*d>>8)+(b*m_bodyYUV.g>>8);
              dstLn[0][dstPos+1]=((256-o)*d>>8)+(o*m_outlineYUV.g>>8);
          // R
              d=((256-s)*dstLn[0][dstPos+2]>>8)+(s*m_shadowYUV.r>>8);
              d=((256-b)*d>>8)+(b*m_bodyYUV.r>>8);
              dstLn[0][dstPos+2]=((256-o)*d>>8)+(o*m_outlineYUV.r>>8);
         }
      }
    }
  }
 else
  {
   int sdx15=sdx[0]-15;
   for (unsigned int y=0;y<dy[0];y++,dstLn[0]+=stride[0],msk[0]+=bmpmskstride[0],bmp[0]+=bmpmskstride[0])
    {
     int x=0;
     for (;x<sdx15;x+=16)
      {
       __m64 mm0=*(__m64*)(dstLn[0]+x),mm1=*(__m64*)(dstLn[0]+x+8);
       mm0=_mm_subs_pu8(mm0,*(__m64*)(msk[0]+x));mm1=_mm_subs_pu8(mm1,*(__m64*)(msk[0]+x+8));
       mm0=_mm_adds_pu8(mm0,*(__m64*)(bmp[0]+x));mm1=_mm_adds_pu8(mm1,*(__m64*)(bmp[0]+x+8));
       *(__m64*)(dstLn[0]+x)=mm0;*(__m64*)(dstLn[0]+x+8)=mm1;
      }
     for (;x<int(sdx[0]);x++)
      {
       int c=dstLn[0][x];
       c-=msk[0][x];if (c<0) c=0;
       c+=bmp[0][x];if (c>255) c=255;
       dstLn[0][x]=(unsigned char)c;
      }
    }
   __m64 m128=_mm_set1_pi8((char)128),m0=_mm_setzero_si64(),mAdd=shiftChroma?m128:m0;
   int add=shiftChroma?128:0;
   int sdx7=sdx[1]-7;
   for (unsigned int y=0;y<dy[1];y++,dstLn[1]+=stride[1],dstLn[2]+=stride[2],msk[1]+=bmpmskstride[1],bmp[1]+=bmpmskstride[1],bmp[2]+=bmpmskstride[2])
    {
     int x=0;
     for (;x<sdx7;x+=8)
      {
       __m64 mm0=*(__m64*)(dstLn[1]+x);
       __m64 mm1=*(__m64*)(dstLn[2]+x);

       psubb(mm0,m128);
       psubb(mm1,m128);

       const __m64 msk8=*(const __m64*)(msk[1]+x);

       __m64 mskU=_mm_cmpgt_pi8(m0,mm0); //what to be negated
       mm0=_mm_or_si64(_mm_and_si64(mskU,_mm_adds_pu8(mm0,msk8)),_mm_andnot_si64(mskU,_mm_subs_pu8(mm0,msk8)));

       __m64 mskV=_mm_cmpgt_pi8(m0,mm1);
       mm1=_mm_or_si64(_mm_and_si64(mskV,_mm_adds_pu8(mm1,msk8)),_mm_andnot_si64(mskV,_mm_subs_pu8(mm1,msk8)));

       mm0=_mm_add_pi8(_mm_add_pi8(mm0,*(__m64*)(bmp[1]+x)),mAdd);
       mm1=_mm_add_pi8(_mm_add_pi8(mm1,*(__m64*)(bmp[2]+x)),mAdd);

       *(__m64*)(dstLn[1]+x)=mm0;
       *(__m64*)(dstLn[2]+x)=mm1;
      }
     for (;x<int(sdx[1]);x++)
      {
       int m=msk[1][x],c;
       c=dstLn[1][x];
       c-=128;
       if (c<0) {c+=m;if (c>0) c=0;}
       else     {c-=m;if (c<0) c=0;}
       c+=bmp[1][x];
       c+=add;
       dstLn[1][x]=c;//(unsigned char)limit(c,0,255);

       c=dstLn[2][x];
       c-=128;
       if (c<0) {c+=m;if (c>0) c=0;}
       else     {c-=m;if (c<0) c=0;};
       c+=bmp[2][x];
       c+=add;
       dstLn[2][x]=c;//(unsigned char)limit(c,0,255);
      }
    }
  }

/* for (int x=0;x<dx[0];x++)
     for (int y=0;y<dy[0];y++)
     {
         if (bmp[0][dy[0]*y+x] !=0)
         {
             dstLn[0][x]=c
         }
     }*/
 _mm_empty();
}

//============================== TrenderedSubtitleLine ===============================
unsigned int TrenderedSubtitleLine::width(void) const
{
 unsigned int dx=0;
 for (const_iterator w=begin();w!=end();w++)
  dx+=(*w)->dxCharY;
 return dx;
}
unsigned int TrenderedSubtitleLine::height(void) const
{
 int aboveBaseline=0,belowBaseline=0;
 for (const_iterator w=begin();w!=end();w++)
  {
   aboveBaseline=std::max<int>(aboveBaseline,(*w)->get_baseline());
   belowBaseline=std::max<int>(belowBaseline,(*w)->dy[0]-(*w)->get_baseline());
  }
 return aboveBaseline+belowBaseline;
}
unsigned int TrenderedSubtitleLine::charHeight(void) const
{
 int aboveBaseline=0,belowBaseline=0;
 for (const_iterator w=begin();w!=end();w++)
  {
   aboveBaseline=std::max<int>(aboveBaseline,(*w)->get_baseline());
   belowBaseline=std::max<int>(belowBaseline,(*w)->dyCharY-(*w)->get_baseline());
  }
 return aboveBaseline+belowBaseline;
}
unsigned int TrenderedSubtitleLine::baselineHeight(void) const
{
 unsigned int aboveBaseline=0;
 for (const_iterator w=begin();w!=end();w++)
  {
   aboveBaseline=std::max<unsigned int>(aboveBaseline,(*w)->get_baseline());
  }
 return aboveBaseline;
}
void TrenderedSubtitleLine::print(int startx,int starty,const TrenderedSubtitleLines::TprintPrefs &prefs,unsigned int prefsdx,unsigned int prefsdy) const
{
 int baseline=baselineHeight();
 for (const_iterator w=begin();w!=end() && startx<(int)prefsdx;startx+=(*w)->dxCharY,w++)
  {
   const unsigned char *msk[3],*bmp[3];
   unsigned char *dstLn[3];
   int x[3];
   unsigned int dx[3];
   for (int i=0;i<3;i++)
    {
     x[i]=startx>>prefs.shiftX[i];
     msk[i]=(*w)->msk[i];
     bmp[i]=(*w)->bmp[i];
     if (prefs.align!=ALIGN_FFDSHOW && x[i]<0)
      {
       msk[i]+=-x[i];
       bmp[i]+=-x[i];
      }
     dstLn[i]=prefs.dst[i]+prefs.stride[i]*((starty+baseline-(*w)->get_baseline())>>prefs.shiftY[i]);
     if (x[i]>0) dstLn[i]+=x[i]*prefs.cspBpp;

     if (x[i]+(*w)->dx[i]>(prefsdx>>prefs.shiftX[i])) dx[i]=(prefsdx>>prefs.shiftX[i])-x[i];
     else if (x[i]<0) dx[i]=(*w)->dx[i]+x[i];
     else dx[i]=(*w)->dx[i];
     dx[i]=std::min(dx[i],prefsdx>>prefs.shiftX[i]);
    }
   (*w)->print(dx,dstLn,prefs.stride,bmp,msk);
  }
}
void TrenderedSubtitleLine::clear(void)
{
 for (iterator w=begin();w!=end();w++)
  delete *w;
 std::vector<value_type>::clear();
}

//============================== TrenderedSubtitleLines ==============================
void TrenderedSubtitleLines::print(const TprintPrefs &prefs)
{
 if (empty()) return;
 unsigned int prefsdx,prefsdy;
 if (prefs.sizeDx && prefs.sizeDy)
  {
   prefsdx=prefs.sizeDx;
   prefsdy=prefs.sizeDy;
  }
 else
  {
   prefsdx=prefs.dx;
   prefsdy=prefs.dy;
  }
 double h=0;
 for (const_iterator i=begin();i!=end();i++)
  h+=(double)prefs.linespacing*(*i)->charHeight()/100.0;

 const_reverse_iterator ri=rbegin();
 unsigned int h1 = h - (double)prefs.linespacing*(*ri)->charHeight()/100.0 + (*ri)->height();

 double y=(prefs.ypos<0) ? -(double)prefs.ypos : ((double)prefs.ypos*prefsdy)/100.0-h/2;
 if (y+h1 >= (double)prefsdy) y=(double)prefsdy-h1-1;
 if (y<0) y=0;

 int old_alignment=-1;
 int old_marginTop=-1,old_marginL=-1;
 bool old_isPos=false;
 int old_posx=-1,old_posy=-1;

 for (const_iterator i=begin();i!=end();y+=(double)prefs.linespacing*(*i)->charHeight()/100,i++)
  {
   if (y<0) continue;

   unsigned int marginTop=(*i)->props.get_marginTop(prefsdy);
   unsigned int marginBottom=(*i)->props.get_marginBottom(prefsdy);

   // When the alignment or marginTop or marginL changes, it's a new paragraph.
   if ((*i)->props.alignment>0 && ((*i)->props.alignment!=old_alignment || old_marginTop!=(*i)->props.marginTop || old_marginL!=(*i)->props.marginL || old_isPos!=(*i)->props.isPos || ((*i)->props.isPos && ((*i)->props.posx!=old_posx || (*i)->props.posy!=old_posy))))
    {
     old_alignment=(*i)->props.alignment;
     old_marginTop=(*i)->props.marginTop;
     old_marginL=(*i)->props.marginL;
     old_isPos=(*i)->props.isPos;
     old_posx=(*i)->props.posx;
     old_posy=(*i)->props.posy;
     // calculate the height of the paragraph
     double paragraphHeight=0,h1=0;
     for (const_iterator pi=i;pi!=end();pi++)
      {
       double h2;
       if ((*pi)->props.alignment!=old_alignment || (*pi)->props.marginTop!=old_marginTop || (*pi)->props.marginL!=old_marginL || (*pi)->props.isPos!=old_isPos || ((*pi)->props.isPos && ((*pi)->props.posx!=old_posx || (*pi)->props.posy!=old_posy)))
        break;
       h2=h1+(*pi)->height();
       h1+=(double)prefs.linespacing*(*pi)->charHeight()/100;
       if (h2>paragraphHeight) paragraphHeight=h2;
      }
     h1+=2.0; // FIXME: should plus the outline width of the top line + that of the botom line.

     switch ((*i)->props.alignment)
      {
       case 1: // SSA bottom
       case 2:
       case 3:
        y=(double)prefsdy-h1-marginBottom;
        break;
       case 9: // SSA mid
       case 10:
       case 11:
        y=((double)prefsdy-marginTop-marginBottom-h1)/2.0+marginTop;
        break;
       case 5: // SSA top
       case 6:
       case 7:
        y=marginTop;
        break;
       default:
        break;
      }

     if (y+paragraphHeight>=(double)prefsdy)
      y=(double)prefsdy-paragraphHeight-1;
     if (y<0) y=0;
    }

   if ((int)y+(*i)->height()>prefsdy) break;
   //TODO: cleanup
   int x;
   unsigned int cdx=(*i)->width();
   if (prefs.xpos<0) x=-prefs.xpos;
   else
    {
     switch ((*i)->props.alignment)
      {
       case 1: // left(SSA)
       case 5:
       case 9:
        x=(*i)->props.get_marginL(prefsdx);
        break;
       case 2: // center(SSA)
       case 6:
       case 10:
        x=(prefsdx-(*i)->props.get_marginL(prefsdx)-(*i)->props.get_marginR(prefsdx)-cdx)/2+(*i)->props.get_marginL(prefsdx);
        if (x<0) x=0;
        if (x+cdx>=prefsdx) x=prefsdx-cdx;
        break;
       case 3: // right(SSA)
       case 7:
       case 11:
        x=prefsdx-cdx-(*i)->props.get_marginR(prefsdx)+2;
        break;
       default: // -1 (non SSA)
        x=(prefs.xpos*prefsdx)/100+prefs.posXpix;
        switch (prefs.align)
         {
          case ALIGN_FFDSHOW:x=x-cdx/2;if (x<0) x=0;if (x+cdx>=prefsdx) x=prefsdx-cdx;break;
          case ALIGN_LEFT:break;
          case ALIGN_CENTER:x=x-cdx/2;break;
          case ALIGN_RIGHT:x=x-cdx;break;
         }
        break;
      }
    }
   if (x+cdx>=prefsdx) x=prefsdx-cdx-1;
   if (x<0) x=0;
   (*i)->print(x,y,prefs,prefsdx,prefsdy); // print a line (=print words).
  }
}
void TrenderedSubtitleLines::add(TrenderedSubtitleLine *ln,unsigned int *height)
{
 push_back(ln);
 if (height)
  *height+=ln->height();
}
void TrenderedSubtitleLines::clear(void)
{
 for (iterator l=begin();l!=end();l++)
  {
   (*l)->clear();
   delete *l;
  }
 std::vector<value_type>::clear();
}

//================================= TcharsChache =================================
TcharsChache::TcharsChache(HDC Ihdc,const YUVcolorA &Iyuv,const YUVcolorA &Ioutline,const YUVcolorA &Ishadow,int Ixscale,int Iyscale,IffdshowBase *Ideci):
 hdc(Ihdc),
 yuv(Iyuv),
 outlineYUV(Ioutline),
 shadowYUV(Ishadow),
 xscale(Ixscale),
 yscale(Iyscale),
 deci(Ideci)
{
}
TcharsChache::~TcharsChache()
{
 for (Tchars::iterator c=chars.begin();c!=chars.end();c++)
  delete c->second;
}
template<> const TrenderedSubtitleWord* TcharsChache::getChar(const wchar_t *s,const TrenderedSubtitleLines::TprintPrefs &prefs,const LOGFONT &lf)
{
 int key=(int)*s;
 Tchars::iterator l=chars.find(key);
 if (l!=chars.end()) return l->second;
 TrenderedSubtitleWord *ln=new TrenderedSubtitleWord(hdc,s,1,yuv,outlineYUV,shadowYUV,prefs,lf,xscale,yscale);
 chars[key]=ln;
 return ln;
}

template<> const TrenderedSubtitleWord* TcharsChache::getChar(const char *s,const TrenderedSubtitleLines::TprintPrefs &prefs,const LOGFONT &lf)
{
 if(_mbclen((unsigned char *)s)==1)
  {
   int key=(int)*s;
   Tchars::iterator l=chars.find(key);
   if (l!=chars.end()) return l->second;
   TrenderedSubtitleWord *ln=new TrenderedSubtitleWord(hdc,s,1,yuv,outlineYUV,shadowYUV,prefs,lf,xscale,yscale);
   chars[key]=ln;
   return ln;
  }
 else
  {
   const wchar_t *mbcs=(wchar_t *)s; // ANSI-MBCS
   int key=(int)*mbcs;
   Tchars::iterator l=chars.find(key);
   if (l!=chars.end()) return l->second;
   TrenderedSubtitleWord *ln=new TrenderedSubtitleWord(hdc,s,2,yuv,outlineYUV,shadowYUV,prefs,lf,xscale,yscale);
   chars[key]=ln;
   return ln;
  }
}

//==================================== Tfont ====================================
Tfont::Tfont(IffdshowBase *Ideci):
 fontManager(NULL),
 deci(Ideci),
 oldsub(NULL),
 hdc(NULL),oldFont(NULL),
 charsCache(NULL),
 height(0),
 fontSettings((TfontSettings*)malloc(sizeof(TfontSettings)))
{
}
Tfont::~Tfont()
{
 done();
 free(fontSettings);
}
void Tfont::init(const TfontSettings *IfontSettings)
{
 done();
 memcpy(fontSettings,IfontSettings,sizeof(TfontSettings));
 hdc=CreateCompatibleDC(NULL);
 if (!hdc) return;
 yuvcolor=YUVcolorA(fontSettings->color,fontSettings->bodyAlpha);
 outlineYUV=YUVcolorA(fontSettings->outlineColor,fontSettings->outlineAlpha);
 shadowYUV=YUVcolorA(fontSettings->shadowColor,fontSettings->shadowAlpha);
 if (fontSettings->fast)
  charsCache=new TcharsChache(hdc,yuvcolor,outlineYUV,shadowYUV,fontSettings->xscale,fontSettings->yscale,deci);
}
void Tfont::done(void)
{
 lines.clear();
 if (hdc)
  {
   if (oldFont) SelectObject(hdc,oldFont);oldFont=NULL;
   DeleteDC(hdc);hdc=NULL;
  }
 oldsub=NULL;
 if (charsCache) delete charsCache;charsCache=NULL;
}

template<class tchar> TrenderedSubtitleWord* Tfont::newWord(const tchar *s,size_t slen,TrenderedSubtitleLines::TprintPrefs prefs,const TsubtitleWord<tchar> *w,const LOGFONT &lf,bool trimRightSpaces)
{
 typedef typename tchar_traits<tchar>::ffstring ffstring;

 ffstring s1(s);
 if (trimRightSpaces)
  {
   while (s1.size() && s1.at(s1.size()-1)==' ')
    s1.erase(s1.size()-1,1);
  }

 if (w->props.shadowDepth != -1) // SSA/ASS/ASS2
  {
   if (w->props.shadowDepth == 0)
    {
     prefs.shadowMode = 3;
     prefs.shadowSize = 0;
    }
   else
    {
     prefs.shadowMode = 2;
     prefs.shadowSize = -1 * w->props.shadowDepth;
    }
  }

 prefs.alignSSA=w->props.alignment;
 prefs.outlineWidth=w->props.outlineWidth==-1 ? fontSettings->outlineWidth : w->props.outlineWidth;

 if (prefs.shadowMode==-1) // OSD
  {
   prefs.shadowMode=fontSettings->shadowMode;
   prefs.shadowSize=fontSettings->shadowSize;
  }

 YUVcolorA shadowYUV1;
 if (!w->props.isColor)
  {
   shadowYUV1=shadowYUV;
   if (prefs.shadowMode<=1)
    shadowYUV1.A=256*sqrt((double)shadowYUV1.A/256.0);
  }
 if (fontSettings->blur || w->props.blur)
  prefs.blur=true;
 else
  prefs.blur=false;

 if (w->props.outlineWidth==-1 && fontSettings->opaqueBox)
  {
   prefs.outlineWidth=0;
   prefs.opaqueBox=true;
  }

 TrenderedSubtitleWord *rw;
 if (!w->props.isColor && fontSettings->fast && !lf.lfItalic && !(prefs.shadowSize!=0 && prefs.shadowMode!=3) && prefs.csp==FF_CSP_420P && !prefs.opaqueBox)
  rw=new TrenderedSubtitleWord(charsCache,s1.c_str(),slen,prefs,lf); // fast rendering
 else
  rw=new TrenderedSubtitleWord(hdc,                      // full rendering
                                   s1.c_str(),
                                   slen,
                                   w->props.isColor ? YUVcolorA(w->props.color,w->props.colorA) : yuvcolor,
                                   w->props.isColor ? YUVcolorA(w->props.OutlineColour,w->props.OutlineColourA) : outlineYUV,
                                   w->props.isColor ? YUVcolorA(w->props.ShadowColour,w->props.ShadowColourA) : shadowYUV1,
                                   prefs,
                                   lf,
                                   w->props.get_xscale(fontSettings->xscale,prefs.sar,fontSettings->aspectAuto,fontSettings->overrideScale),
                                   w->props.get_yscale(fontSettings->yscale,prefs.sar,fontSettings->aspectAuto,fontSettings->overrideScale)
                                   );
 if (rw->dxCharY && rw->dyCharY)
  return rw;
 else
  {
   delete rw;
   return NULL;
  }
}

template<class tchar> int Tfont::get_splitdx_for_new_line(const TsubtitleWord<tchar> &w,int splitdx,int dx) const
{
 if (w.props.marginR!=-1 || w.props.marginL!=-1)
  return (dx- w.props.get_marginR(dx) - w.props.get_marginL(dx))*4;
 else
  return splitdx;
}

template<class tchar> void Tfont::prepareC(const TsubtitleTextBase<tchar> *sub,const TrenderedSubtitleLines::TprintPrefs &prefs,bool forceChange)
{
 typedef typename tchar_traits<tchar>::ffstring ffstring;
 if (oldsub!=sub || forceChange || oldCsp!=prefs.csp)
  {
   oldsub=sub;
   oldCsp=prefs.csp;

   unsigned int dx,dy;
   if (prefs.sizeDx && prefs.sizeDy)
    {
     dx=prefs.sizeDx;
     dy=prefs.sizeDy;
    }
   else
    {
     dx=prefs.dx;
     dy=prefs.dy;
    }

   lines.clear();height=0;
   if (!sub) return;
   if (!fontManager)
    comptrQ<IffdshowDecVideo>(deci)->getFontManager(&fontManager);
   bool nosplit=!fontSettings->split && !(prefs.fontchangesplit && prefs.fontsplit);
   int splitdx0=nosplit ? 0 : ((int)dx-prefs.textBorderLR<1 ? 1 : dx-prefs.textBorderLR)*4;

   int *pwidths=NULL;
   Tbuffer width;

   for (typename TsubtitleTextBase<tchar>::const_iterator l=sub->begin();l!=sub->end();l++)
    {
     int charCount=0;
     ffstring allStr;
     Tbuffer tempwidth;
     double left=0.0,nextleft=0.0;
     int wordWrapMode=-1;
     int splitdxMax=splitdx0;
     for (typename TsubtitleLine<tchar>::const_iterator w=l->begin();w!=l->end();w++)
      {
       LOGFONT lf;
       w->props.toLOGFONT(lf,*fontSettings,dx,dy,prefs.clipdy);
       HFONT font=fontManager->getFont(lf);
       HGDIOBJ old=SelectObject(hdc,font);
       SetTextCharacterExtra(hdc,w->props.spacing==INT_MIN ? fontSettings->spacing : w->props.get_spacing(dy,prefs.clipdy));
       const tchar *p=*w;
       if (*p) // drop empty words
        {
         int xscale=w->props.get_xscale(fontSettings->xscale,prefs.sar,fontSettings->aspectAuto,fontSettings->overrideScale);
         wordWrapMode=w->props.wrapStyle;
         splitdxMax=get_splitdx_for_new_line(*w,splitdx0,dx);
         allStr+=p;
         pwidths=(int*)width.resize((allStr.size()+1)*sizeof(int));
         left=nextleft;
         int nfit;
         SIZE sz;
         size_t strlenp=strlen(p);
         int *ptempwidths=(int*)tempwidth.alloc((strlenp+1)*sizeof(int));
         prefs.config->getGDI<tchar>().getTextExtentExPoint(hdc,p,(int)strlenp,INT_MAX,&nfit,ptempwidths,&sz);
         for (size_t x=0;x<strlenp;x++)
          {
           pwidths[charCount]=nextleft=(double)ptempwidths[x]*xscale/100+left;
           charCount++;
          }
        }
      }
     if (allStr.empty()) continue;
     if (wordWrapMode==-1) // non SSA/ASS/ASS2
      {
       if (nosplit)
        wordWrapMode=2;
       else
        {
         deci->getParam(IDFF_subWordWrap,&wordWrapMode);
         if (wordWrapMode>=2) wordWrapMode++;
        }
      }
     TwordWrap<tchar> wordWrap(wordWrapMode,allStr.c_str(),pwidths,splitdxMax);
     //wordWrap.debugprint();

     TrenderedSubtitleLine *line=NULL;
     int cx=0,cy=0;
     for (typename TsubtitleLine<tchar>::const_iterator w=l->begin();w!=l->end();w++)
      {
       LOGFONT lf;
       w->props.toLOGFONT(lf,*fontSettings,dx,dy,prefs.clipdy);
       HFONT font=fontManager->getFont(lf);
       HGDIOBJ old=SelectObject(hdc,font);
       if (!oldFont) oldFont=old;
       SetTextCharacterExtra(hdc,w->props.spacing==INT_MIN ? fontSettings->spacing : w->props.get_spacing(dy,prefs.clipdy));
       if (!line)
        {
         line=new TrenderedSubtitleLine(w->props);
        }
       const tchar *p=*w;
       if (*p) // drop empty words
        {
         #ifdef DEBUG
         text<char_t> dbgstr(p);
         DPRINTF(_l("%s"),dbgstr);
         #endif
         int linesInWord=0;
         do
          {
           if (linesInWord>0)
            {
             while (*p && tchar_traits<tchar>::isspace((typename tchar_traits<tchar>::uchar_t)*p))
              {
               cx++;
               p++;
              }
            }
           int strlenp=strlen(p);
           if (cx+strlenp-1<=wordWrap.getRightOfTheLine(cy))
            {
             if (*p)
              {
               TrenderedSubtitleWord *rw=newWord(p,strlenp,prefs,w,lf,w+1==l->end());
               if (rw) line->push_back(rw);
               cx+=strlenp;
              }
             break;
            }
           else
            {
             int n=wordWrap.getRightOfTheLine(cy)-cx+1;
             if (n<=0)
              {
               cy++;
               linesInWord++;
               n=wordWrap.getRightOfTheLine(cy)-cx+1;
               if (!line->empty())
                {
                 lines.add(line,&height);
                 line=new TrenderedSubtitleLine(w->props);
                }
               if (cy>=wordWrap.getLineCount()) break;
              }
             if (*p)
              {
               TrenderedSubtitleWord *rw=newWord(p,n,prefs,w,lf,true);
               if (rw) line->push_back(rw);
              }
             if (!line->empty())
              {
               lines.add(line,&height);
               line=new TrenderedSubtitleLine(w->props);
              }
             p+=wordWrap.getRightOfTheLine(cy)-cx+1;
             cx=wordWrap.getRightOfTheLine(cy)+1;
             cy++;
             linesInWord++;
            }
          } while(cy<wordWrap.getLineCount());
        }
      }
     if (line)
      if (!line->empty())
       lines.add(line,&height);
      else
       delete line;
    }
  }
}

template<class tchar> void Tfont::print(const TsubtitleTextBase<tchar> *sub,bool forceChange,const TrenderedSubtitleLines::TprintPrefs &prefs,unsigned int *y)
{
 if (!sub) return;
 prepareC(sub,prefs,forceChange);if (y) *y+=height;
 lines.print(prefs);
}

template void Tfont::print(const TsubtitleTextBase<char> *sub,bool forceChange,const TrenderedSubtitleLines::TprintPrefs &prefs,unsigned int *y);
template void Tfont::print(const TsubtitleTextBase<wchar_t> *sub,bool forceChange,const TrenderedSubtitleLines::TprintPrefs &prefs,unsigned int *y);
