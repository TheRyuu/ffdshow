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
#include <mbstring.h>
#pragma warning(disable:4244)

//============================ TrenderedSubtitleWordBase =============================
TrenderedSubtitleWordBase::~TrenderedSubtitleWordBase()
{
 if (own)
  for (int i=0;i<3;i++)
   {
    aligned_free(bmp[i]);
    aligned_free(msk[i]);
   }
}

//============================== TrenderedSubtitleWord ===============================

// not fast rendering
template<class tchar> TrenderedSubtitleWord::TrenderedSubtitleWord(HDC hdc,const tchar *s0,size_t strlens,const short (*matrix)[5],const YUVcolor &yuv,const TrenderedSubtitleLines::TprintPrefs &prefs,int xscale):TrenderedSubtitleWordBase(true),shiftChroma(true)
{
 typedef typename tchar_traits<tchar>::ffstring ffstring;
 typedef typename tchar_traits<tchar>::strings strings;
 strings s1;
 strtok(ffstring(s0,strlens).c_str(),_L("\t"),s1);
 SIZE sz;sz.cx=sz.cy=0;ints cxs;
 for (typename strings::const_iterator s=s1.begin();s!=s1.end();s++)
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
 GetOutlineTextMetrics(hdc,sizeof(otm),&otm);
 if (otm.otmItalicAngle)
  sz.cx-=LONG(sz.cy*sin(otm.otmItalicAngle*M_PI/1800));
 else
  if (otm.otmTextMetrics.tmItalic)
   sz.cx+=sz.cy*0.35;
 dx[0]=(sz.cx/4+2)*4;dy[0]=sz.cy+4;
 unsigned char *bmp16=(unsigned char*)calloc(dx[0]*4,dy[0]);
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
   int sz=s->size();
   prefs.config->getGDI<tchar>().textOut(hdc,x,2,s->c_str(),sz/*(int)s->size()*/);
   x+=*cx;
  }
 drawShadow(hdc,hbmp,bmp16,old,xscale,sz,prefs,matrix,yuv);
}
void TrenderedSubtitleWord::drawShadow(HDC hdc,HBITMAP hbmp,unsigned char *bmp16,HGDIOBJ old,int xscale,const SIZE &sz,const TrenderedSubtitleLines::TprintPrefs &prefs,const short (*matrix)[5],const YUVcolor &yuv)
{
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
 GetDIBits(hdc,hbmp,0,dy[0],bmp16,&bmi,DIB_RGB_COLORS);
 SelectObject(hdc,old);
 DeleteObject(hbmp);


 unsigned int _dx,_dy;
 _dx=(xscale*dx[0]/100)/4+4;_dy=dy[0]/4+4;
 dxCharY=xscale*sz.cx/400;dyCharY=sz.cy/4;
 _dx=(_dx/8+1)*8;
 bmp[0]=(unsigned char*)aligned_calloc(_dx,_dy);
 msk[0]=(unsigned char*)aligned_calloc(_dx,_dy);
 int dxCharYstart=((xscale==100 && prefs.align!=ALIGN_LEFT)?(_dx-dx[0]/4)/2:0);
 for (unsigned int y=2;y<dy[0]-2;y+=4)
  {
   unsigned char *dstBmpY=bmp[0]+(y/4+2)*_dx+2+dxCharYstart;
   for (unsigned int xstep=xscale==100?4*65536:400*65536/xscale,x=(2<<16)+xstep;x<((dx[0]-2)<<16);x+=xstep,dstBmpY++)
    {
     unsigned int sum=0;
     for (const unsigned char *bmp16src=bmp16+((y-2)*dx[0]+((x>>16)-2))*4,*bmp16srcEnd=bmp16src+5*dx[0]*4;bmp16src!=bmp16srcEnd;bmp16src+=dx[0]*4)
	 {
      for (int i=0;i<=12;i+=4)
       sum+=bmp16src[i];
	 }
     sum/=25;
	 if (sum >= 180) sum = 255; // Fixed the gray issue. 180 seems fine. Lower : more blur but more gray. Higher : sharper and lighter
     *dstBmpY=(unsigned char)sum;
    }
  }
 free(bmp16);
 dx[0]=_dx;dy[0]=_dy;
 if (!matrix)
  memset(msk[0],255,dx[0]*dy[0]);
 else
  {
   memcpy(msk[0],bmp[0],dx[0]*dy[0]);

	#define MAKE_SHADOW(x,y)                                   \
    {                                                         \
     unsigned int s=0,cnt=0;                                  \
     for (int yy=-2;yy<=+2;yy++)                              \
      {                                                       \
       if (y+yy<0 || (unsigned int)(y+yy)>=dy[0]) continue;   \
       for (int xx=-2;xx<=+2;xx++)                            \
        {                                                     \
         if (x+xx<0 || (unsigned int)(x+xx)>=dx[0]) continue; \
         s+=bmp[0][dx[0]*(y+yy)+(x+xx)]*matrix[yy+2][xx+2];   \
         cnt++;                                               \
        }                                                     \
      }                                                       \
     s/=cnt*32;                                               \
     if (s>255) s=255;                                        \
     msk[0][dx[0]*y+x]=(unsigned char)s;                      \
    }
   int dyY1=dy[0]-1,dyY2=dy[0]-2;
   for (unsigned int x=0;x<dx[0];x++)
    {
     MAKE_SHADOW(x,0);
     MAKE_SHADOW(x,1);
     MAKE_SHADOW(x,dyY2);
     MAKE_SHADOW(x,dyY1);
    }
   int dxY1=dx[0]-1,dxY2=dx[0]-2;
   for (unsigned int y0=2;y0<dy[0]-2;y0++)
    {
     MAKE_SHADOW(0,y0);
     MAKE_SHADOW(1,y0);
     MAKE_SHADOW(dxY2,y0);
     MAKE_SHADOW(dxY1,y0);
    }



   __m64 matrix8_0=*(__m64*)matrix[0],matrix8_1=*(__m64*)matrix[1],matrix8_2=*(__m64*)matrix[2];
   const int matrix4[3]={matrix[0][4],matrix[1][4],matrix[2][4]};
   const unsigned char *bmpYsrc_2=bmp[0],*bmpYsrc_1=bmp[0]+1*dx[0],*bmpYsrc=bmp[0]+2*dx[0],*bmpYsrc1=bmp[0]+3*dx[0],*bmpYsrc2=bmp[0]+4*dx[0];
   unsigned char *mskYdst=msk[0]+dx[0]*2;
   __m64 m0=_mm_setzero_si64();
   for (unsigned int y=2;y<dy[0]-2;y++,bmpYsrc_2+=dx[0],bmpYsrc_1+=dx[0],bmpYsrc+=dx[0],bmpYsrc1+=dx[0],bmpYsrc2+=dx[0],mskYdst+=dx[0])
    for (unsigned int x=2;x<dx[0]-2;x++)
     {
      __m64 r_2=_mm_madd_pi16(_mm_unpacklo_pi8(*(__m64*)(bmpYsrc_2+x-2),m0),matrix8_0);
      __m64 r_1=_mm_madd_pi16(_mm_unpacklo_pi8(*(__m64*)(bmpYsrc_1+x-2),m0),matrix8_1);
      __m64 r  =_mm_madd_pi16(_mm_unpacklo_pi8(*(__m64*)(bmpYsrc  +x-2),m0),matrix8_2);
      __m64 r1 =_mm_madd_pi16(_mm_unpacklo_pi8(*(__m64*)(bmpYsrc1 +x-2),m0),matrix8_1);
      __m64 r2 =_mm_madd_pi16(_mm_unpacklo_pi8(*(__m64*)(bmpYsrc2 +x-2),m0),matrix8_0);
      r=_mm_add_pi32(_mm_add_pi32(_mm_add_pi32(_mm_add_pi32(r_2,r_1),r),r1),r2);
      r=_mm_add_pi32(_mm_srli_si64(r,32),r);
      int s=bmpYsrc_2[x+2]*matrix4[0];
      s+=   bmpYsrc_1[x+2]*matrix4[1];
      s+=   bmpYsrc  [x+2]*matrix4[2];
      s+=   bmpYsrc1 [x+2]*matrix4[1];
      s+=   bmpYsrc2 [x+2]*matrix4[0];
      mskYdst[x]=limit_uint8((_mm_cvtsi64_si32(r)+s)/(25*32));
     }
  }
 dx[1]=dx[0]>>prefs.shiftX[1];dx[2]=dx[0]>>prefs.shiftX[2];
 dy[1]=dy[0]>>prefs.shiftY[1];dy[2]=dy[0]>>prefs.shiftY[2];
 unsigned int _dxUV=dx[1];
 dx[1]=(dx[1]/8+1)*8;
 bmp[1]=(unsigned char*)aligned_calloc(dx[1],dy[1]);msk[1]=(unsigned char*)aligned_calloc(dx[1],dy[1]);
 dx[2]=(dx[2]/8+1)*8;
 bmp[2]=(unsigned char*)aligned_calloc(dx[2],dy[2]);msk[2]=(unsigned char*)aligned_calloc(dx[2],dy[2]);
 unsigned char *bmpptr[3]={NULL,bmp[1],bmp[2]};
 unsigned char *mskptr[3]={NULL,msk[1],msk[2]};
 for (unsigned int y=0;y<dy[1];y++,bmpptr[1]+=dx[1],bmpptr[2]+=dx[2],mskptr[1]+=dx[1],mskptr[2]+=dx[2])
  {
   const unsigned char *bmpYptr=bmp[0]+dx[0]*(y*2),*mskYptr=msk[0]+dx[0]*(y*2);
   for (unsigned int x=0;x<_dxUV;x++,bmpYptr+=2,mskYptr+=2)
    {
     unsigned int s;
     s =bmpYptr[0];
     s+=bmpYptr[1];
     s+=bmpYptr[dx[0]];
     s+=bmpYptr[dx[0]+1];
     bmpptr[1][x]=(unsigned char)((yuv.U*s)>>9);
     bmpptr[2][x]=(unsigned char)((yuv.V*s)>>9);
     s =mskYptr[0];
     s+=mskYptr[1];
     s+=mskYptr[dx[0]];
     s+=mskYptr[dx[0]+1];
     mskptr[1][x]=
     mskptr[2][x]=(unsigned char)(s/8);
    }
  }
 unsigned int cnt=dx[0]*dy[0],i=0;
 __m64 m0=_mm_setzero_si64(),yuvY=_mm_set1_pi16(yuv.Y);
 for (;i<cnt-3;i+=4)
  {
   __m64 bmp8=_mm_unpacklo_pi8(_mm_cvtsi32_si64(*(int*)(bmp[0]+i)),m0);
   bmp8=_mm_srli_pi16(_mm_mullo_pi16(bmp8,yuvY),8);
   *(int*)(bmp[0]+i)=_mm_cvtsi64_si32(_mm_packs_pu16(bmp8,m0));
  }
 for (;i<cnt;i++)
 {
  bmp[0][i]=(unsigned char)((bmp[0][i]*yuv.Y)>>8);
 }
 bmpmskstride[0]=dx[0];bmpmskstride[1]=dx[1];bmpmskstride[2]=dx[2];
 _mm_empty();

   unsigned int shadowSize = prefs.shadowSize;
   unsigned int shadowAlpha = prefs.shadowAlpha;
   unsigned int shadowMode = prefs.shadowMode; // 0: glowing, 1:classic with gradient, 2: classic with no gradient, >=3: no shadow
   if (shadowSize > 0)
   if (shadowMode == 0) //Gradient glowing shadow (most complex)
   {
	   for (unsigned int y=0; y<dy[0];y++)
	   {
		   for (unsigned int x=0; x<dx[0];x++)
		   {
			   unsigned int pos = dx[0]*y+x;
			   if (bmp[0][pos] == 0) continue;

			   unsigned int shadowAlphaGradient = shadowAlpha;
			   for (unsigned int circleSize=1; circleSize<=shadowSize; circleSize++)
			   {
				   for (unsigned int i=0; i<circleSize; i++)
				   {
						unsigned int yy = (unsigned int)sqrt((double)(circleSize*circleSize) - (double)(i*i));
						if (x - i > 0)
						{
							
							if (y+yy < dy[0] && 
								bmp[0][dx[0]*(y+yy)+x-i]==0 && msk[0][dx[0]*(y+yy)+x-i] <shadowAlphaGradient)
							{
								msk[0][dx[0]*(y+yy)+x-i] = shadowAlphaGradient;
							}
							if (y-yy > 0 && 
								bmp[0][dx[0]*(y-yy)+x-i]==0 && msk[0][dx[0]*(y-yy)+x-i] <shadowAlphaGradient)
							{
								msk[0][dx[0]*(y-yy)+x-i] = shadowAlphaGradient;
							}
						}
						if (x + i < dx[0])
						{
							if (y+yy < dy[0] && 
								bmp[0][dx[0]*(y+yy)+x+i]==0 && msk[0][dx[0]*(y+yy)+x+i] <shadowAlphaGradient)
							{
								msk[0][dx[0]*(y+yy)+x+i] = shadowAlphaGradient;
							}
							if (y-yy > 0 && 
								bmp[0][dx[0]*(y-yy)+x+i]==0 && msk[0][dx[0]*(y-yy)+x+i] <shadowAlphaGradient)
							{
								msk[0][dx[0]*(y-yy)+x+i] = shadowAlphaGradient;
							}
						}
				   }
				   shadowAlphaGradient -= shadowAlpha/shadowSize;
			   }
		   }
	   }
   }
   else if (shadowMode == 1) //Gradient classic shadow
   {
	   for (unsigned int y=0; y<dy[0];y++)
	   {
		   for (unsigned int x=0; x<dx[0];x++)
		   {
			   unsigned int pos = dx[0]*y+x;
			   if (bmp[0][pos] == 0) continue;
			   /*else
				bmp[0][pos] = 255;*/

			   unsigned int shadowAlphaGradient = shadowAlpha;
			   for (unsigned int circleSize=1; circleSize<=shadowSize; circleSize++)
			   {
					unsigned int xx = (unsigned int)((double)circleSize/sqrt((double)2));
					if (x + xx < dx[0])
					{
						if (y+xx < dy[0] && 
							bmp[0][dx[0]*(y+xx)+x+xx]==0 && msk[0][dx[0]*(y+xx)+x+xx] <shadowAlphaGradient)
						{
							msk[0][dx[0]*(y+xx)+x+xx] = shadowAlphaGradient;
						}
					}
				   shadowAlphaGradient -= shadowAlpha/shadowSize;
			   }
		   }
	   }
   }
   else if (shadowMode == 2) //Classic shadow
   {
		for (unsigned int y=dy[0]-1; y>=shadowSize;y--)
		{
			for (unsigned int x=dx[0]-1; x>=shadowSize;x--)
			{
				unsigned int pos = dx[0]*y+x;
				if (bmp[0][dx[0]*(y-shadowSize)+x-shadowSize] != 0 && bmp[0][pos] == 0)
					msk[0][pos] = shadowAlpha;
			}
		}
   }
}

// fast rendering
template<class tchar> TrenderedSubtitleWord::TrenderedSubtitleWord(TcharsChache *charsChache,const tchar *s,size_t strlens,const TrenderedSubtitleLines::TprintPrefs &prefs):TrenderedSubtitleWordBase(true),shiftChroma(true)
{
 const TrenderedSubtitleWord **chars=(const TrenderedSubtitleWord**)_alloca(strlens*sizeof(TrenderedSubtitleLine*));
 for (int i=0;i<3;i++)
  dx[i]=dy[i]=0;
 for (size_t i=0;i<strlens;i++)
  {
   chars[i]=charsChache->getChar(&s[i],prefs);
   dx[0]+=chars[i]->dxCharY;
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
 dx[0]=(dx[0]/8+2)*8;dx[1]=dx[2]=(dx[0]/16+1)*8;
 dxCharY=dx[0];
 dyCharY=chars[0]->dyCharY;
 for (int i=0;i<3;i++)
  {
   bmp[i]=(unsigned char*)aligned_calloc(dx[i],dy[i]);//bmp[1]=(unsigned char*)aligned_calloc(dxUV,dyUV);bmp[2]=(unsigned char*)aligned_calloc(dxUV,dyUV);
   msk[i]=(unsigned char*)aligned_calloc(dx[i],dy[i]);//msk[1]=(unsigned char*)aligned_calloc(dxUV,dyUV);msk[2]=(unsigned char*)aligned_calloc(dxUV,dyUV);
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
   for (unsigned int p=0;p<3;p++)
    {
     const unsigned char *charbmpptr=chars[i]->bmp[p],*charmskptr=chars[i]->msk[p];
     unsigned char *bmpptr=bmp[p]+roundRshift(x,prefs.shiftX[p]),*mskptr=msk[p]+roundRshift(x,prefs.shiftX[p]);
     for (unsigned int y=0;y<chars[i]->dy[p];y++,bmpptr+=bmpmskstride[p],mskptr+=bmpmskstride[p],charbmpptr+=chars[i]->bmpmskstride[p],charmskptr+=chars[i]->bmpmskstride[p])
      {
       memadd(bmpptr,charbmpptr,chars[i]->dx[p]);
       memadd(mskptr,charmskptr,chars[i]->dx[p]);
      }
    }
   x+=chars[i]->dxCharY;
   if(sizeof(tchar)==sizeof(char))
    if(_mbclen((const unsigned char *)&s[i])==2)
     i++;
  }
 _mm_empty();
}

void TrenderedSubtitleWord::print(unsigned int sdx[3],unsigned char *dstLn[3],const stride_t stride[3],const unsigned char *bmp[3],const unsigned char *msk[3]) const
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
  dx+=(*w)->dx[0];
 return dx;
}
unsigned int TrenderedSubtitleLine::height(void) const
{
 unsigned int dy=0;
 for (const_iterator w=begin();w!=end();w++)
  dy=std::max(dy,(*w)->dy[0]);
 return dy;
}
void TrenderedSubtitleLine::print(int startx,int starty,const TrenderedSubtitleLines::TprintPrefs &prefs,unsigned int prefsdx,unsigned int prefsdy) const
{
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
     dstLn[i]=prefs.dst[i]+prefs.stride[i]*(starty>>prefs.shiftY[i]);if (x[i]>0) dstLn[i]+=x[i];

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
 unsigned int h=0;
 for (const_iterator i=begin();i!=end();i++)
  h+=prefs.linespacing*(*i)->height()/100;

 const_reverse_iterator ri=rbegin();
 unsigned int h1 = h - prefs.linespacing*(*ri)->height()/100 + (*ri)->height();

 int y=(prefs.ypos<0) ? -prefs.ypos : (prefs.ypos*prefsdy)/100-h/2;
 if (y+h1 >= prefsdy) y=prefsdy-h1-1;
 if (y<0) y=0;

 int old_alignment=-1;
 int old_marginTop=-1,old_marginL=-1;

 for (const_iterator i=begin();i!=end();y+=prefs.linespacing*(*i)->height()/100-2,i++)
  {
   if (y<0) continue;

   unsigned int marginTop=(*i)->props.get_marginTop(prefsdy);
   unsigned int marginBottom=(*i)->props.get_marginBottom(prefsdy);

   // When the alignment or marginTop or marginL changes, it's a new paragraph.
   if ((*i)->props.alignment>0 && ((*i)->props.alignment!=old_alignment || old_marginTop!=(*i)->props.marginTop || old_marginL!=(*i)->props.marginL))
    {
     old_alignment=(*i)->props.alignment;
     old_marginTop=(*i)->props.marginTop;
     old_marginL=(*i)->props.marginL;
     // calculate the height of the paragraph
     unsigned int hParagraph=0;
     for (const_iterator pi=i;pi!=end();pi++)
      {
       if ((*pi)->props.alignment!=old_alignment || (*pi)->props.marginTop!=old_marginTop || (*pi)->props.marginL!=old_marginL)
        break;
       if (pi+1!=end() && (*(pi+1))->props.alignment==old_alignment && (*(pi+1))->props.marginTop==old_marginTop && (*(pi+1))->props.marginL==old_marginL)
        hParagraph+=prefs.linespacing*(*pi)->height()/100;
       else
        hParagraph+=(*pi)->height();
      }

     switch ((*i)->props.alignment)
      {
       case 1: // SSA bottom
       case 2:
       case 3:
        y=prefsdy-hParagraph-marginBottom;
        break;
       case 9: // SSA mid
       case 10:
       case 11:
        y=(prefsdy-hParagraph)/2;
        break;
       case 5: // SSA top
       case 6:
       case 7:
        y=marginTop;
        break;
       default:
        break;
      }

     if (y<0) y=0;
     if (y+hParagraph>=prefsdy)
      y=prefsdy-hParagraph-1;
    }

   if (y+(*i)->height()>=prefsdy) break;
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
        x=(prefsdx-cdx)/2;
        if (x<0) x=0;
        if (x+cdx>=prefsdx) x=prefsdx-cdx;
        break;
       case 3: // right(SSA)
       case 7:
       case 11:
        x=prefsdx-cdx-(*i)->props.get_marginR(prefsdx);
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
   // if (x+cdx>prefsdx) x=prefsdx-cdx-1;
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
TcharsChache::TcharsChache(HDC Ihdc,const short (*Imatrix)[5],const YUVcolor &Iyuv,int Ixscale):hdc(Ihdc),matrix(Imatrix),yuv(Iyuv),xscale(Ixscale)
{
}
TcharsChache::~TcharsChache()
{
 for (Tchars::iterator c=chars.begin();c!=chars.end();c++)
  delete c->second;
}
template<> const TrenderedSubtitleWord* TcharsChache::getChar(const wchar_t *s,const TrenderedSubtitleLines::TprintPrefs &prefs)
{
 int key=(int)*s;
 Tchars::iterator l=chars.find(key);
 if (l!=chars.end()) return l->second;
 TrenderedSubtitleWord *ln=new TrenderedSubtitleWord(hdc,s,1,matrix,yuv,prefs,xscale);
 chars[key]=ln;
 return ln;
}

template<> const TrenderedSubtitleWord* TcharsChache::getChar(const char *s,const TrenderedSubtitleLines::TprintPrefs &prefs)
{
 if(_mbclen((unsigned char *)s)==1)
  {
   int key=(int)*s;
   Tchars::iterator l=chars.find(key);
   if (l!=chars.end()) return l->second;
   TrenderedSubtitleWord *ln=new TrenderedSubtitleWord(hdc,s,1,matrix,yuv,prefs,xscale);
   chars[key]=ln;
   return ln;
  }
 else
  {
   const wchar_t *mbcs=(wchar_t *)s; // ANSI-MBCS
   int key=(int)*mbcs;
   Tchars::iterator l=chars.find(key);
   if (l!=chars.end()) return l->second;
   TrenderedSubtitleWord *ln=new TrenderedSubtitleWord(hdc,s,2,matrix,yuv,prefs,xscale);
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
 yuvcolor=YUVcolor(fontSettings->color);
 if (fontSettings->shadowStrength<100)
  for (int y=-2;y<=2;y++)
   for (int x=-2;x<=2;x++)
    {
     double d=8-(x*x+y*y);
     matrix[y+2][x+2]=short(2.55*fontSettings->shadowStrength*pow(d/8,2-fontSettings->shadowRadius/50.0)+0.5);
    }
 if (fontSettings->fast)
  charsCache=new TcharsChache(hdc,fontSettings->shadowStrength==100?NULL:matrix,yuvcolor,fontSettings->xscale);
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

template<class tchar> TrenderedSubtitleWord* Tfont::newWord(const tchar *s,size_t slen,const TrenderedSubtitleLines::TprintPrefs &prefs,const TsubtitleWord<tchar> *w)
{
 OUTLINETEXTMETRIC otm;
 GetOutlineTextMetrics(hdc,sizeof(otm),&otm);

 if (!w->props.isColor && fontSettings->fast && !otm.otmItalicAngle && !otm.otmTextMetrics.tmItalic)
  return new TrenderedSubtitleWord(charsCache,s,slen,prefs);
 else
  return new TrenderedSubtitleWord(hdc,s,slen,fontSettings->shadowStrength==100?NULL:matrix,w->props.isColor?w->props.color:yuvcolor,prefs,w->props.scaleX!=-1?w->props.scaleX:fontSettings->xscale);
}

template<class tchar> void Tfont::prepareC(const TsubtitleTextBase<tchar> *sub,const TrenderedSubtitleLines::TprintPrefs &prefs,bool forceChange)
{
 if (oldsub!=sub || forceChange)
  {
   oldsub=sub;

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
   int splitdx0=nosplit?0:(prefs.textBorderLR-40>(int)dx?dx:dx-prefs.textBorderLR)*4;
   for (typename TsubtitleTextBase<tchar>::const_iterator l=sub->begin();l!=sub->end();l++)
    {
     TrenderedSubtitleLine *line=NULL;
     int splitdx=splitdx0;
     for (typename TsubtitleLine<tchar>::const_iterator w=l->begin();w!=l->end();w++)
      {
       LOGFONT lf;
       w->props.toLOGFONT(lf,*fontSettings,dx,dy);
       HFONT font=fontManager->getFont(lf);
       HGDIOBJ old=SelectObject(hdc,font);
       if (!oldFont) oldFont=old;
       SetTextCharacterExtra(hdc,w->props.spacing>=0?w->props.spacing:fontSettings->spacing);
       if (!line) line=new TrenderedSubtitleLine(w->props);
       if (nosplit)
        line->push_back(newWord<tchar>(*w,strlen(*w),prefs,&*w));
       else
        {
         const tchar *p=*w;
         while (*p)
          {
           int nfit;
           SIZE sz;
           size_t strlenp=strlenp=strlen(p);
           int *pwidths=(int*)_alloca(sizeof(int)*(strlenp+1));
           int splitdx1;
           if (w->props.get_marginR(dx) || w->props.get_marginL(dx))
            splitdx1=(dx- w->props.get_marginR(dx) - w->props.get_marginL(dx))*4;
           else
            splitdx1=splitdx;
           if (!prefs.config->getGDI<tchar>().getTextExtentExPoint(hdc,p,(int)strlenp,splitdx1,&nfit,pwidths,&sz) || nfit>=(int)strlen(p))
            {
             TrenderedSubtitleWord *rw=newWord(p,strlen(p),prefs,&*w);
             line->push_back(rw);
             splitdx-=rw->dxCharY*4;
             break;
            }

           for (int j=nfit;j>0;j--)
            if (tchar_traits<tchar>::isspace((typename tchar_traits<tchar>::uchar_t)p[j]))
             {
              nfit=j;
              break;
             }

           TrenderedSubtitleWord *rw=newWord(p,nfit,prefs,&*w);
           line->push_back(rw);
           lines.add(line,&height);
           line=new TrenderedSubtitleLine(w->props);
           splitdx=splitdx0;
           p+=nfit;
           while (*p && tchar_traits<tchar>::isspace((typename tchar_traits<tchar>::uchar_t)*p))
            p++;
          }
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
