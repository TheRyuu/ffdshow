/*
 * Copyright (c) 2005,2006 Milan Cutka
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
#include "TspuImage.h"
#include "simd.h"
#include "TffPict.h"
#include "Tlibmplayer.h"
#include "Tconfig.h"
#include "IffdshowBase.h"

//=================================================== TspuPlane ===================================================
void TspuPlane::alloc(const CSize &sz, int div, int csp)
{
    size_t needed;
    if ((csp & FF_CSPS_MASK) == FF_CSP_420P) {
        stride = (((sz.cx+1)/div)/16+2)*16;
        needed = stride*(odd2even(sz.cy + 3) / div);
    } else {
        // RGB32
        stride = (sz.cx + 1) * 4;
        needed = stride * (sz.cy + 1);
    }
    if (needed>allocated) {
        c = (unsigned char*)aligned_realloc(c,needed);
        r = (unsigned char*)aligned_realloc(r,needed);
        allocated = needed;
    }
    setZero();
}

void TspuPlane::setZero()
{
    memset(r, 0, allocated);
}
//=================================================== TspuImage ===================================================
TspuImage::TspuImage(const TspuPlane src[3],const CRect &rcclip,const CRect &rectReal,const CRect &rectOrig,const TprintPrefs &prefs, int Icsp):TrenderedSubtitleWordBase(false)
{
    csp = prefs.csp & FF_CSPS_MASK;
    const TcspInfo *cspInfo = csp_getInfo(prefs.csp);
    if (rectReal.Width()<0) return;
    Tscaler *scaler=NULL;
    int scale=prefs.dvd?0x100:prefs.vobscale;
    if (prefs.pgs) scale=prefs.pgsscale;
    for (unsigned int i = 0 ; i < cspInfo->numPlanes ; i++) {
        rect[i]=CRect(rectReal.left >> cspInfo->shiftX[i],                                                           // left
                      rectReal.top  >> cspInfo->shiftY[i],                                                           // top
                      (rectReal.left >> cspInfo->shiftX[i]) + roundRshift(rectReal.Width(), cspInfo->shiftX[i]),     // right
                      (rectReal.top  >> cspInfo->shiftY[i]) + roundRshift(rectReal.Height(), cspInfo->shiftY[i]));   // bottom
        int dstdx = roundDiv(scale * (rectOrig.Width() ?
                            roundDiv(prefs.dx * (rect[i].Width() + 1), (unsigned int)rectOrig.Width() ) :
                            rect[i].Width() ),
                        0x100U);
        int dstdy = roundDiv(scale * (rectOrig.Height() ?
                            roundDiv(prefs.dy * (rect[i].Height() + 1), (unsigned int)rectOrig.Height()) :
                            rect[i].Height()),
                        0x100U);
        plane[i].alloc(CSize(dstdx,dstdy), 1, prefs.csp);
        if (!scaler || scaler->srcdx != rect[i].Width() + 1 || scaler->srcdy != rect[i].Height() + 1) {
            if (scaler)
                delete scaler;
            scaler = Tscaler::create(prefs, rect[i].Width() + 1, rect[i].Height() + 1, dstdx, dstdy, Icsp);
        }
        scaler->scale(
            src[i].c + rect[i].top * src[i].stride + rect[i].left * cspInfo->Bpp,
            src[i].r + rect[i].top * src[i].stride + rect[i].left * cspInfo->Bpp,
            src[i].stride,
            plane[i].c,
            plane[i].r,
            plane[i].stride);
        rect[i] += CPoint(roundRshift(rcclip.left,cspInfo->shiftX[i]),roundRshift(rcclip.top,cspInfo->shiftY[i]));
        rect[i].left = roundDiv(int(prefs.dx*rect[i].left),rectOrig.Width());rect[i].right=rect[i].left+dstdx;
        rect[i].top = roundDiv(int(prefs.dy*rect[i].top),rectOrig.Height());rect[i].bottom=rect[i].top+dstdy;
        dx[i] = rect[i].Width();dy[i]=rect[i].Height();
        bmp[i] = plane[i].c;msk[i]=plane[i].r;
        bmpmskstride[i] = plane[i].stride;
    }
    dxChar = dx[0];
    dyChar = dy[0];
    if (scaler)
        delete scaler;
}

//=============================================== TspuImage::Tscaler ==============================================
TspuImage::Tscaler* TspuImage::Tscaler::create(const TprintPrefs &prefs,int srcdx,int srcdy,int dstdx,int dstdy, int csp)
{
 switch (prefs.dvd?0:prefs.vobaamode)
  {
   case 1:return new TscalerApprox(prefs,srcdx,srcdy,dstdx,dstdy);
   case 2:return new TscalerFull(prefs,srcdx,srcdy,dstdx,dstdy);
   case 3:return new TscalerBilin(prefs,srcdx,srcdy,dstdx,dstdy);
   case 4:return new TscalerSw(prefs,srcdx,srcdy,dstdx,dstdy,csp);
   default:
   case 0:return new TscalerPoint(prefs,srcdx,srcdy,dstdx,dstdy);
  }
}

//============================================ TspuImage::TscalerPoint ============================================
TspuImage::TscalerPoint::TscalerPoint(const TprintPrefs &prefs,int srcdx,int srcdy,int dstdx,int dstdy):Tscaler(prefs,srcdx,srcdy,dstdx,dstdy)
{
}
void TspuImage::TscalerPoint::scale(const unsigned char *srci,const unsigned char *srca,stride_t srcStride,unsigned char *dsti,unsigned char *dsta,stride_t dstStride)
{
 const TcspInfo *cspInfo = csp_getInfo(csp);
 if (srcdx==dstdx && srcdy==dstdy)
  {
   TffPict::copy(dsti,dstStride,srci,srcStride,srcdx * cspInfo->Bpp,srcdy);
   TffPict::copy(dsta,dstStride,srca,srcStride,srcdx * cspInfo->Bpp,srcdy);
  }
 else
  {
   int scalex=0x100*dstdx/srcdx;
   int scaley=0x100*dstdy/srcdy;
   for (int y=0;y<dstdy;y++)
    {
     int unscaled_y=y*0x100/scaley;
     stride_t strides=srcStride*unscaled_y;
     stride_t scaled_strides=dstStride*y;
     for (int x=0;x<dstdx;x++)
      {
       int unscaled_x=x*0x100/scalex;
       dsti[scaled_strides+x]=srci[strides+unscaled_x];
       dsta[scaled_strides+x]=srca[strides+unscaled_x];
      }
    }
  }
}

//============================================ TspuImage::TscalerApprox ===========================================
TspuImage::TscalerApprox::TscalerApprox(const TprintPrefs &prefs,int srcdx,int srcdy,int dstdx,int dstdy):Tscaler(prefs,srcdx,srcdy,dstdx,dstdy)
{
 scalex=0x100*dstdx/srcdx;
 scaley=0x100*dstdy/srcdy;
}
void TspuImage::TscalerApprox::scale(const unsigned char *srci,const unsigned char *srca,stride_t srcStride,unsigned char *dsti,unsigned char *dsta,stride_t dstStride)
{
 unsigned int height=srcdy;
 for (int y=0;y<dstdy;y++,dsti+=dstStride,dsta+=dstStride)
  {
   const unsigned int unscaled_top=y*0x100/scaley;
   unsigned int unscaled_bottom=(y+1)*0x100/scaley;
   if (unscaled_bottom>=height)
    unscaled_bottom=height-1;
   unsigned char *dstiLn=dsti,*dstaLn=dsta;
   if ((csp & FF_CSPS_MASK) ==FF_CSP_420P)
   {
    for (int x=0;x<dstdx;x++,dstiLn++,dstaLn++)
     {
      const unsigned int unscaled_left=x*0x100/scalex;
      unsigned int unscaled_right=(x+1)*0x100/scalex;
      unsigned int width=srcdx;
      if (unscaled_right>=width)
       unscaled_right=width-1;
      unsigned int alpha=0,color=0;
      int cnt=0;
      for (unsigned int walky=unscaled_top;walky<=unscaled_bottom;walky++)
       for (unsigned int walkx=unscaled_left;walkx<=unscaled_right;walkx++)
        {
         cnt++;
         stride_t base=walky*srcStride+walkx;
         unsigned int tmp=srca[base];
         alpha+=tmp;
         color+=tmp*srci[base];
        }
      *dstiLn=(unsigned char)(alpha?color/alpha:0);
      *dstaLn=(unsigned char)(cnt?alpha/cnt:0);
     }
   }
   else //RGB32
   {
    uint32_t *dstiLn32 = (uint32_t*)dstiLn;
    uint32_t *dstaLn32 = (uint32_t*)dstaLn;
   for (int x=0;x<dstdx;x++,dstiLn32++,dstaLn32++)
    {
     const unsigned int unscaled_left=x*0x100/scalex;
     unsigned int unscaled_right=(x+1)*0x100/scalex;
     unsigned int width=srcdx;
     if (unscaled_right>=width)
      unscaled_right=width-1;
     uint32_t alpha=0,r=0,g=0,b=0;
     int cnt=0;
     for (unsigned int walky=unscaled_top;walky<=unscaled_bottom;walky++)
      for (unsigned int walkx=unscaled_left;walkx<=unscaled_right;walkx++)
       {
        //TODO : SSE optimizations but the operation "/alpha" may be problematic (no SSE instruction for division)
        cnt++;
        stride_t base=walky*srcStride+walkx*4;
        uint32_t *srci32=(uint32_t*)(srci+base);
        uint32_t *srca32=(uint32_t*)(srca+base);
        uint32_t tmp=(*srca32)&0xFF;
        alpha+=tmp;
        r+=tmp*(((*srci32)>>16)&0xFF);
        g+=tmp*(((*srci32)>>8)&0xFF);
        b+=tmp*(((*srci32))&0xFF);
       }
      *dstiLn32=(alpha?((r/alpha)<<16)|((g/alpha)<<8)|(b/alpha):0);
     alpha=alpha/cnt;
     *dstaLn32=(cnt?((alpha<<16)|(alpha<<8)|(alpha)):0);
    }
   }
  }
}

//============================================ TspuImage::TscalerFull =============================================
TspuImage::TscalerFull::TscalerFull(const TprintPrefs &prefs,int srcdx,int srcdy,int dstdx,int dstdy):Tscaler(prefs,srcdx,srcdy,dstdx,dstdy)
{
}
void TspuImage::TscalerFull::scale(const unsigned char *srci,const unsigned char *srca,stride_t srcStride,unsigned char *dsti,unsigned char *dsta,stride_t dstStride)
{
 const double scalex=(double)0x100*dstdx/srcdx;
 const double scaley=(double)0x100*dstdy/srcdy;
 const TcspInfo *cspInfo = csp_getInfo(csp);
 bool isRGB32 = ((csp & FF_CSPS_MASK) == FF_CSP_RGB32);

 /* Best antialiasing.  Very slow. */
 /* Any pixel (x, y) represents pixels from the original
    rectangular region comprised between the columns
    unscaled_y and unscaled_y + 0x100 / scaley and the rows
    unscaled_x and unscaled_x + 0x100 / scalex

    The original rectangular region that the scaled pixel
    represents is cut in 9 rectangular areas like this:

    +---+-----------------+---+
    | 1 |        2        | 3 |
    +---+-----------------+---+
    |   |                 |   |
    | 4 |        5        | 6 |
    |   |                 |   |
    +---+-----------------+---+
    | 7 |        8        | 9 |
    +---+-----------------+---+

    The width of the left column is at most one pixel and
    it is never null and its right column is at a pixel
    boundary.  The height of the top row is at most one
    pixel it is never null and its bottom row is at a
    pixel boundary. The width and height of region 5 are
    integral values.  The width of the right column is
    what remains and is less than one pixel.  The height
    of the bottom row is what remains and is less than
    one pixel.

    The row above 1, 2, 3 is unscaled_y.  The row between
    1, 2, 3 and 4, 5, 6 is top_low_row.  The row between 4,
    5, 6 and 7, 8, 9 is (unsigned int)unscaled_y_bottom.
    The row beneath 7, 8, 9 is unscaled_y_bottom.

    The column left of 1, 4, 7 is unscaled_x.  The column
    between 1, 4, 7 and 2, 5, 8 is left_right_column.  The
    column between 2, 5, 8 and 3, 6, 9 is (unsigned
    int)unscaled_x_right.  The column right of 3, 6, 9 is
    unscaled_x_right. */
 const double inv_scalex = (double) 0x100 / scalex;
 const double inv_scaley = (double) 0x100 / scaley;
 for (int y = 0; y < dstdy; ++y,dsti+=dstStride,dsta+=dstStride)
  {
   const double unscaled_y = y * inv_scaley;
   const double unscaled_y_bottom = unscaled_y + inv_scaley;
   const unsigned int top_low_row = (unsigned int)std::min(unscaled_y_bottom, unscaled_y + 1.0);
   const double top = top_low_row - unscaled_y;
   const unsigned int height = unscaled_y_bottom > top_low_row
     ? (unsigned int) unscaled_y_bottom - top_low_row
     : 0;
   const double bottom = unscaled_y_bottom > top_low_row
     ? unscaled_y_bottom - floor(unscaled_y_bottom)
     : 0.0;
   unsigned char *dstiLn=dsti,*dstaLn=dsta;
   for (int x = 0; x < dstdx; ++x,dstiLn+=cspInfo->Bpp,dstaLn+=cspInfo->Bpp)
    {
     const double unscaled_x = x * inv_scalex;
     const double unscaled_x_right = unscaled_x + inv_scalex;
     const unsigned int left_right_column = (unsigned int)std::min(unscaled_x_right, unscaled_x + 1.0);
     const double left = left_right_column - unscaled_x;
     const unsigned int width = unscaled_x_right > left_right_column
       ? (unsigned int) unscaled_x_right - left_right_column
       : 0;
     const double right = unscaled_x_right > left_right_column
       ? unscaled_x_right - floor(unscaled_x_right)
       : 0.0;
     double color = 0.0;
     double alpha = 0.0;
     double tmp;
     uint32_t rgb = 0;
     uint32_t r=0,g=0,b=0;
     stride_t base;
     /* Now use these informations to compute a good alpha,
        and lightness.  The sum is on each of the 9
        region's surface and alpha and lightness.

       transformed alpha = sum(surface * alpha) / sum(surface)
       transformed color = sum(surface * alpha * color) / sum(surface * alpha)
     */
     /* 1: top left part */
     base = srcStride * (unsigned int) unscaled_y;
     stride_t pos=base + unscaled_x*cspInfo->Bpp;
     if (isRGB32)
     {
      tmp = left * top * ((*(uint32_t*)(srca+ pos))&0xFF);
      alpha += tmp;
      rgb=*(uint32_t*)(srci+pos);
      r+=tmp*((rgb>>16)&0xFF);
      g+=tmp*((rgb>>8)&0xFF);
      b+=tmp*(rgb&0xFF);
     }
     else
     {
      tmp = left * top * (srca[(unsigned int) pos]);
      alpha += tmp;
      color += tmp * srci[(unsigned int) pos];
     }
     
     /* 2: top center part */
     if (width > 0) {
       unsigned int walkx;
       for (walkx = left_right_column; walkx < (unsigned int) unscaled_x_right; ++walkx) {
         base = srcStride * (unsigned int) unscaled_y + walkx*cspInfo->Bpp;
         if (isRGB32)
         {
          tmp = top * ((*(uint32_t*)(srca+base))&0xFF);
          alpha += tmp;
          rgb=*(uint32_t*)(srci+base);
          r+=tmp*((rgb>>16)&0xFF);
          g+=tmp*((rgb>>8)&0xFF);
          b+=tmp*(rgb&0xFF);
         }
         else
         {
          tmp = /* 1.0 * */ top * (srca[base]);
          alpha += tmp;
          color += tmp * srci[base];
         }
       }
     }
     /* 3: top right part */
     if (right > 0.0) {
       base = srcStride * (unsigned int) unscaled_y + (unsigned int) unscaled_x_right*cspInfo->Bpp;
       if (isRGB32)
       {
        tmp = right * top * ((*(uint32_t*)(srca+base))&0xFF);
        alpha += tmp;
        rgb=*(uint32_t*)(srci+base);
        r+=tmp*((rgb>>16)&0xFF);
        g+=tmp*((rgb>>8)&0xFF);
        b+=tmp*(rgb&0xFF);
       }
       else
       {
        tmp = right * top * (srca[base]);
        alpha += tmp;
        color += tmp * srci[base];
       }
     }
     /* 4: center left part */
     if (height > 0) 
     {
      unsigned int walky;
      for (walky = top_low_row; walky < (unsigned int) unscaled_y_bottom; ++walky) 
      {
       base = srcStride * walky + (unsigned int) unscaled_x*cspInfo->Bpp;
       if (isRGB32)
       {
        tmp = left * ((*(uint32_t*)(srca+base))&0xFF);
        alpha += tmp;
        rgb=*(uint32_t*)(srci+base);
        r+=tmp*((rgb>>16)&0xFF);
        g+=tmp*((rgb>>8)&0xFF);
        b+=tmp*(rgb&0xFF);
       }
       else
       {
         tmp = left /* * 1.0 */ * (srca[base]);
         alpha += tmp;
         color += tmp * srci[base];
       }
      }
     }
     /* 5: center part */
     if (width > 0 && height > 0) 
     {
      unsigned int walky;
      for (walky = top_low_row; walky < (unsigned int) unscaled_y_bottom; ++walky) 
      {
       unsigned int walkx;
       base = srcStride * walky;
       for (walkx = left_right_column; walkx < (unsigned int) unscaled_x_right; ++walkx) 
       {
        pos=base+ walkx*cspInfo->Bpp;
        if (isRGB32)
        {
         tmp = (*(uint32_t*)(srca+pos))&0xFF;
         alpha += tmp;
         rgb=*(uint32_t*)(srci+pos);
         r+=tmp*((rgb>>16)&0xFF);
         g+=tmp*((rgb>>8)&0xFF);
         b+=tmp*(rgb&0xFF);
        }
        else
        {
         tmp = /* 1.0 * 1.0 * */ (srca[pos]);
         alpha += tmp;
         color += tmp * srci[pos];
        }
       }
      }
     }
     /* 6: center right part */
     if (right > 0.0 && height > 0) 
     {
      unsigned int walky;
      for (walky = top_low_row; walky < (unsigned int) unscaled_y_bottom; ++walky) 
      {
       base = srcStride * walky + (unsigned int) unscaled_x_right*cspInfo->Bpp;
       if (isRGB32)
       {
        tmp = right*((*(uint32_t*)(srca+base))&0xFF);
        alpha += tmp;
        rgb=*(uint32_t*)(srci+base);
        r+=tmp*((rgb>>16)&0xFF);
        g+=tmp*((rgb>>8)&0xFF);
        b+=tmp*(rgb&0xFF);
       }
       else
       {
        tmp = right /* * 1.0 */ * (srca[base]);
        alpha += tmp;
        color += tmp * srci[base];
       }
      }
     }
     /* 7: bottom left part */
     if (bottom > 0.0)
     {
      base = srcStride * (unsigned int) unscaled_y_bottom + (unsigned int) unscaled_x*cspInfo->Bpp;
      if (isRGB32)
      {
       tmp = left*bottom*((*(uint32_t*)(srca+base))&0xFF);
       alpha += tmp;
       rgb=*(uint32_t*)(srci+base);
       r+=tmp*((rgb>>16)&0xFF);
       g+=tmp*((rgb>>8)&0xFF);
       b+=tmp*(rgb&0xFF);
      }
      else
      {
       tmp = left * bottom * (srca[base]);
       alpha += tmp;
       color += tmp * srci[base];
      }
     }
     /* 8: bottom center part */
     if (width > 0 && bottom > 0.0) 
     {
      unsigned int walkx;
      base = srcStride * (unsigned int) unscaled_y_bottom;
      for (walkx = left_right_column; walkx < (unsigned int) unscaled_x_right; ++walkx) 
      {
       pos=base+walkx*cspInfo->Bpp;
       if (isRGB32)
       {
        tmp = bottom*((*(uint32_t*)(srca+pos))&0xFF);
        alpha += tmp;
        rgb=*(uint32_t*)(srci+pos);
        r+=tmp*((rgb>>16)&0xFF);
        g+=tmp*((rgb>>8)&0xFF);
        b+=tmp*(rgb&0xFF);
       }
       else
       {
        tmp = /* 1.0 * */ bottom * (srca[pos]);
        alpha += tmp;
        color += tmp * srci[pos];
       }
      }
     }
     /* 9: bottom right part */
     if (right > 0.0 && bottom > 0.0) 
     {
       base = srcStride * (unsigned int) unscaled_y_bottom + (unsigned int) unscaled_x_right*cspInfo->Bpp;
       if (isRGB32)
       {
        tmp = right * bottom *((*(uint32_t*)(srca+base))&0xFF);
        alpha += tmp;
        rgb=*(uint32_t*)(srci+base);
        r+=tmp*((rgb>>16)&0xFF);
        g+=tmp*((rgb>>8)&0xFF);
        b+=tmp*(rgb&0xFF);
       }
       else
       {
        tmp = right * bottom * (srca[base]);
        alpha += tmp;
        color += tmp * srci[base];
       }
     }
     /* Finally mix these transparency and brightness information suitably */
     if (isRGB32)
     {
      uint32_t *dstiLn32 = (uint32_t*)dstiLn;
      uint32_t *dstaLn32 = (uint32_t*)dstaLn;
      uint32_t alpha32 = (uint32_t)alpha;
      *dstiLn32 = (alpha > 0 ? ((r/alpha32)<<16)|((g/alpha32)<<8)|(b/alpha32) : 0);
      uint32_t alpha2 = (uint32_t)(alpha * scalex * scaley / 0x10000);
      *dstaLn32 = ((alpha2<<16)|(alpha2<<8)|(alpha2));
     }
     else
     {
      *dstiLn = (unsigned char)(alpha > 0 ? color / alpha : 0);
      *dstaLn = (unsigned char)(alpha * scalex * scaley / 0x10000);
     }
    }
  }
}

//============================================ TspuImage::TscalerBilin =============================================
TspuImage::TscalerBilin::TscalerBilin(const TprintPrefs &prefs,int srcdx,int srcdy,int dstdx,int dstdy):Tscaler(prefs,srcdx,srcdy,dstdx,dstdy)
{
 table_x=(scale_pixel*)calloc(dstdx,sizeof(scale_pixel));
 table_y=(scale_pixel*)calloc(dstdy,sizeof(scale_pixel));
 scale_table(0,0,srcdx-1,dstdx-1,table_x);
 scale_table(0,0,srcdy-1,dstdy-1,table_y);
}
TspuImage::TscalerBilin::~TscalerBilin()
{
 free(table_x);
 free(table_y);
}
void TspuImage::TscalerBilin::scale_table(unsigned int start_src, unsigned int start_tar, unsigned int end_src, unsigned int end_tar, scale_pixel *table)
{
 unsigned int delta_src=end_src-start_src;
 unsigned int delta_tar=end_tar-start_tar;
 if (delta_src==0 || delta_tar==0)
  return;
 int src=0,src_step=(delta_src<<16)/delta_tar >>1;
 for (unsigned int t=0;t<=delta_tar;src+=(src_step<<1),t++)
  {
   table[t].position=std::min((unsigned int)(src>>16),end_src-1);
   table[t].right_down=src&0xffff;
   table[t].left_up=0x10000-table[t].right_down;
  }
}

void TspuImage::TscalerBilin::scale(const unsigned char *srci,const unsigned char *srca,stride_t srcStride,unsigned char *dsti,unsigned char *dsta,stride_t dstStride)
{
 const TcspInfo *cspInfo = csp_getInfo(csp);
 bool isRGB32 = ((csp & FF_CSPS_MASK) == FF_CSP_RGB32);
 if (isRGB32)
 {
  for (int y=0;y<dstdy;y++,dsti+=dstStride,dsta+=dstStride)
  {
   unsigned char *dstiLn=dsti,*dstaLn=dsta;
   for (int x=0;x<dstdx;x++,dstiLn+=cspInfo->Bpp,dstaLn+=cspInfo->Bpp)
   {
    stride_t base=table_y[y].position*srcStride+table_x[x].position*cspInfo->Bpp;
    uint32_t alpha[4];
    alpha[0]=canon_alpha32((*(uint32_t*)(srca+base))&0xFF);
    alpha[1]=canon_alpha32((*(uint32_t*)(srca+base+cspInfo->Bpp))&0xFF);
    alpha[2]=canon_alpha32((*(uint32_t*)(srca+base+srcStride))&0xFF);
    alpha[3]=canon_alpha32((*(uint32_t*)(srca+base+srcStride+cspInfo->Bpp))&0xFF);
    uint32_t color[4];
    color[0]=*(uint32_t*)(srci+base);
    color[1]=*(uint32_t*)(srci+base+cspInfo->Bpp);
    color[2]=*(uint32_t*)(srci+base+srcStride);
    color[3]=*(uint32_t*)(srci+base+srcStride+cspInfo->Bpp);
    
    uint32_t scale[4];
    scale[0]=(table_x[x].left_up*table_y[y].left_up>>16)*alpha[0];
    scale[1]=(table_x[x].right_down*table_y[y].left_up>>16)*alpha[1];
    scale[2]=(table_x[x].left_up*table_y[y].right_down>>16)*alpha[2];
    scale[3]=(table_x[x].right_down*table_y[y].right_down>>16)*alpha[3];
    
    uint32_t r=((((color[0]>>16)&0xFF) * scale[0] + ((color[1]>>16)&0xFF) * scale[1] + ((color[2]>>16)&0xFF) * scale[2] + ((color[3]>>16)&0xFF) * scale[3])>>24);
    uint32_t g=((((color[0]>>8)&0xFF) * scale[0] + ((color[1]>>8)&0xFF) * scale[1] + ((color[2]>>8)&0xFF) * scale[2] + ((color[3]>>8)&0xFF) * scale[3])>>24);
    uint32_t b=(((color[0]&0xFF) * scale[0] + (color[1]&0xFF) * scale[1] + (color[2]&0xFF) * scale[2] + (color[3]&0xFF) * scale[3])/0x40000);//>>24);

    *(uint32_t*)(dstiLn)=(r<<16)|(g<<8)|b;
    uint32_t alpha32=((scale[0] + scale[1] + scale[2] + scale[3])/0x40000);//>> 20);
    *(uint32_t*)(dstaLn)=(alpha32<<16)|(alpha32<<8)|(alpha32);
   }
  }
 }
 else
 {
  for (int y=0;y<dstdy;y++,dsti+=dstStride,dsta+=dstStride)
  {
   unsigned char *dstiLn=dsti,*dstaLn=dsta;
   for (int x=0;x<dstdx;x++,dstiLn++,dstaLn++)
   {
    stride_t base=table_y[y].position*srcStride+table_x[x].position;
    int alpha[4];
    alpha[0]=canon_alpha(srca[base]);
    alpha[1]=canon_alpha(srca[base+1]);
    alpha[2]=canon_alpha(srca[base+srcStride]);
    alpha[3]=canon_alpha(srca[base+srcStride+1]);
    int color[4];
    color[0]=srci[base];
    color[1]=srci[base+1];
    color[2]=srci[base+srcStride];
    color[3]=srci[base+srcStride+1];
    unsigned int scale[4];
    scale[0]=(table_x[x].left_up*table_y[y].left_up>>16)*alpha[0];
    scale[1]=(table_x[x].right_down*table_y[y].left_up>>16)*alpha[1];
    scale[2]=(table_x[x].left_up*table_y[y].right_down>>16)*alpha[2];
    scale[3]=(table_x[x].right_down*table_y[y].right_down>>16)*alpha[3];
    *dstiLn=(unsigned char)((color[0] * scale[0] + color[1] * scale[1] + color[2] * scale[2] + color[3] * scale[3])>>24);
    *dstaLn=(unsigned char)((scale[0] + scale[1] + scale[2] + scale[3]) >> 20);
   }
  }
 }
}

//============================================== TspuImage::TscalerSw ==============================================
TspuImage::TscalerSw::TscalerSw(const TprintPrefs &prefs,int srcdx,int srcdy,int dstdx,int dstdy, int csp):
 Tscaler(prefs,srcdx,srcdy,dstdx,dstdy),
 approx(prefs,srcdx,srcdy,dstdx,dstdy)
{
 prefs.deci->getPostproc(&libmplayer);
 int mplayercsp=csp_ffdshow2mplayer(csp);
 filter.lumH=filter.lumV=filter.chrH=filter.chrV=libmplayer->sws_getGaussianVec(prefs.vobaagauss/1000.0, 3.0);
 libmplayer->sws_normalizeVec(filter.lumH,1.0);
 SwsParams params;Tlibmplayer::swsInitParams(&params,SWS_GAUSS);
 ctx=libmplayer->sws_getContext(srcdx,srcdy,mplayercsp,dstdx,dstdy,mplayercsp,&params,&filter,NULL,NULL);
 alphactx=libmplayer->sws_getContext(srcdx,srcdy,mplayercsp,dstdx,dstdy,mplayercsp,&params,&filter,NULL,NULL);
 convert = new Tconvert(prefs.deci,dstdx,dstdy);
}
TspuImage::TscalerSw::~TscalerSw()
{
 libmplayer->sws_freeVec(filter.lumH);
 if (ctx) libmplayer->sws_freeContext(ctx);
 if (alphactx) libmplayer->sws_freeContext(alphactx);
 libmplayer->Release();
 if (convert) delete convert;
}
void TspuImage::TscalerSw::scale(const unsigned char *srci,const unsigned char *srca,stride_t srcStride,unsigned char *dsti,unsigned char *dsta,stride_t dstStride)
{
 if (!ctx)
  approx.scale(srci,srca,srcStride,dsti,dsta,dstStride);
 else
  {
   stride_t srcStride0[4]={srcStride, 0, 0, 0};
   stride_t dstStride0[4]={dstStride, 0, 0, 0};
   const unsigned char *srci0[4] = {srci, NULL, NULL, NULL};
   const unsigned char *srca0[4] = {srca, NULL, NULL, NULL};
   unsigned char *dsti0[4] = {dsti, NULL, NULL, NULL};
   unsigned char *dsta0[4] = {dsta, NULL, NULL, NULL};
   libmplayer->sws_scale_ordered(ctx,srci0,srcStride0,0,srcdy,dsti0,dstStride0);
   libmplayer->sws_scale_ordered(ctx,srca0,srcStride0,0,srcdy,dsta0,dstStride0);
   /*const Tpalette palette = Tpalette();
   convert->convert(FF_CSP_BGR32, src, dstStride1, FF_CSP_RGB32, dst, dstStride0, &palette);*/
  }
 
}

//================================================= TspuImageSimd =================================================
template<class _mm> void TspuImageSimd<_mm>::print(int startx, int starty, unsigned int dx[3],int dy[3],unsigned char *dstLn[3],const stride_t stride[3],const unsigned char *bmp[3],const unsigned char *msk[3],REFERENCE_TIME rtStart) const
{
 // dy[] is not used here.
 if (!plane[0].stride || !plane[0].c || !plane[0].r) return;
 typename _mm::__m m0=_mm::setzero_si64();
 for (int i=0;i<3;i++)
  {
   unsigned char *dst=dstLn[i];
   const unsigned char *c=bmp[i],*r=msk[i];
   for (int y=rect[i].top;y<rect[i].bottom;y++,dst+=stride[i],c+=plane[i].stride,r+=plane[i].stride)
    {
     int x=0;
     for (;x<int(dx[i]-_mm::size/2+1);x+=_mm::size/2)
      {
       typename _mm::__m p8=_mm::unpacklo_pi8(_mm::load2(dst+x),m0);
       typename _mm::__m c8=_mm::unpacklo_pi8(_mm::load2(c+x),m0);
       typename _mm::__m r8=_mm::unpacklo_pi8(_mm::load2(r+x),m0);
       _mm::store2(dst+x,_mm::packs_pu16(_mm::sub_pi16(p8,_mm::srai_pi16(_mm::mullo_pi16(_mm::sub_pi16(p8,c8),r8),4)),m0));
      }
     for (;x<int(dx[i]);x++)
      dst[x]=(unsigned char)(dst[x]-((dst[x]-c[x])*r[x]>>4));
    }
  }
 _mm::empty();
}
template<class _mm> void TspuImageSimd<_mm>::ownprint(
    const TprintPrefs &prefs,
    unsigned char **Idst,
    const stride_t *Istride)
{
    const TcspInfo *cspInfo = csp_getInfo(prefs.csp);
    if (!plane[0].stride || !plane[0].c || !plane[0].r) return;
    typename _mm::__m m0=_mm::setzero_si64();
    if ((prefs.csp & FF_CSPS_MASK) ==FF_CSP_420P) {
        typename _mm::__m m16=_mm::set1_pi16(16);
        typename _mm::__m m128=_mm::set1_pi16(128);
        unsigned int sizeDx=prefs.sizeDx?prefs.sizeDx:prefs.dx;
        unsigned char *dst=Idst[0]+rect[0].top*Istride[0]+rect[0].left;
        const unsigned char *c=plane[0].c,*r=plane[0].r;
        // Y plane
        for (int y=rect[0].top;y<rect[0].bottom;y++,dst+=Istride[0],c+=plane[0].stride,r+=plane[0].stride) {
            int x=0,dx=rect[0].Width();if (rect[0].left+dx>(int)sizeDx>>cspInfo->shiftX[0]) dx=(sizeDx>>cspInfo->shiftX[0])-rect[0].left;
            // for(;x<dx-32+1;x+=32)
            for (;x<int(dx-_mm::size/2+1);x+=_mm::size/2) {
                // Loads the dst, c and r values that are in 32 bits into a 64 bits register and interleaved with m0 (zero)
                // So a 32 bits lword 0xWWXXYYZZ is stored as 0xWW00XX00YY00ZZ00
                typename _mm::__m p8=_mm::unpacklo_pi8(_mm::load2(dst+x),m0);
                typename _mm::__m c8=_mm::unpacklo_pi8(_mm::load2(c+x),m0);
                typename _mm::__m r8=_mm::unpacklo_pi8(_mm::load2(r+x),m0);
                _mm::store2(dst+x,_mm::packs_pu16(_mm::sub_pi16(p8,_mm::srai_pi16(_mm::mullo_pi16(_mm::sub_pi16(p8,c8),r8),4)),m0));
            }
            // Equivalent unoptimized operands for writing the rest of this line
            // TODO : Why >>4 ? Normally r[x] is <= 0x40 (6 bits) so after multiplying by r[x] we should >>6 instead of 4
            for (;x<dx;x++)
                dst[x]=(unsigned char)(dst[x]-((dst[x]-c[x])*r[x]>>4));
        }
        // Other planes
        for (int i=1;i<3;i++) {
            unsigned char *dst=Idst[i]+rect[i].top*Istride[i]+rect[i].left;
            const unsigned char *c=plane[i].c,*r=plane[i].r;
            for (int y=rect[i].top;y<rect[i].bottom;y++,dst+=Istride[i],c+=plane[i].stride,r+=plane[i].stride) {
                int x=0,dx=rect[i].Width();
                if (rect[i].left+dx>(int)sizeDx>>cspInfo->shiftX[i])
                    dx=(sizeDx>>cspInfo->shiftX[i])-rect[i].left;
                for (;x<int(dx-_mm::size/2+1);x+=_mm::size/2) {
                    typename _mm::__m p8  = _mm::unpacklo_pi8(_mm::load2(dst+x),m0);
                    typename _mm::__m c8  = _mm::unpacklo_pi8(_mm::load2(c+x),m0);
                    typename _mm::__m r8  = _mm::unpacklo_pi8(_mm::load2(r+x),m0);
                    typename _mm::__m mm1 = _mm::sub_pi16(p8,m128);
                    typename _mm::__m mm2 = _mm::sub_pi16(c8,m128);
                    typename _mm::__m mm3 = _mm::sub_pi16(m16,r8);
                    mm1 = _mm::mullo_pi16(mm1, mm3);
                    mm1 = _mm::srai_pi16(mm1,4);
                    mm2 = _mm::mullo_pi16(mm2, r8);
                    mm2 = _mm::srai_pi16(mm2,4);
                    mm1 = _mm::add_pi16(mm1,mm2);
                    mm1 = _mm::add_pi16(mm1,m128);
                    mm1 = _mm::packs_pu16(mm1, m0);
                    _mm::store2(dst+x,mm1);
                }
                for (;x<dx;x++)
                    dst[x]=(unsigned char)(dst[x]-((dst[x]-c[x])*r[x]>>4));
            }
        }
    } else {
        // RGB32
        typename _mm::__m m128=_mm::set1_pi16(128);
        typename _mm::__m m255=_mm::set1_pi16(255);
        unsigned int sizeDx = prefs.sizeDx ? prefs.sizeDx : prefs.dx;
        uint8_t *dst = Idst[0] + rect[0].top * Istride[0] + rect[0].left * 4;
        const uint8_t *c = plane[0].c;
        const uint8_t *r = plane[0].r;
        for (int y = rect[0].top ; y < rect[0].bottom ; y++, dst += Istride[0], c+=plane[0].stride, r+=plane[0].stride) {
            uint32_t *dstLn = (uint32_t *)dst;
            const uint32_t *cLn = (const uint32_t *)c;
            const uint32_t *rLn = (const uint32_t *)r;
            int x=0, dx=rect[0].Width();
            if (rect[0].left+dx > (int)sizeDx)
                dx = (sizeDx - rect[0].left);
            int endx = dx -_mm::size/2 + 1;
            for (; x < dx ; x ++) {
                /* It would be better to use the alpha composant of cLn instead of using the r plane which is useless
                 but the background becomes white too (?) */
                //int alpha=(int)((*(cLn+x))>>24);

                int alpha=(int)((*(rLn+x))&0xFF);
                int invalpha=255-alpha;
                typename _mm::__m p8=_mm::unpacklo_pi8(_mm::load2(dstLn+x),m0);
                typename _mm::__m c8=_mm::unpacklo_pi8(_mm::load2(cLn+x), m0);
                typename _mm::__m strength64=_mm::set1_pi16(short(alpha));
                typename _mm::__m invstrength64=_mm::set1_pi16(short(invalpha));
                //typename _mm::__m mask=_mm::cmpgt_pi16(p8,c8); // Not needed : the destination buffer is empty
                
                // p8 : current destination value, c8 : color value to blend, r8 : alpha value to blend
                typename _mm::__m result=  _mm::srli_pi16(
                    _mm::add_pi16(
                     _mm::add_pi16(
                      _mm::mullo_pi16(invstrength64,p8),
                       _mm::mullo_pi16(strength64,c8)),
                       m255),
                       8); // foreach r,g,b : ( dstcolor*(255-alpha) + blendcolor*alpha + 255 )/255
                _mm::store2(dstLn + x, _mm::packs_pu16(result, m0));
            }
        }
    }
    _mm::empty();
}

template struct TspuImageSimd<Tmmx>;
 template struct TspuImageSimd<Tsse2>;

