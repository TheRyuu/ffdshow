/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2003                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function:

 ********************************************************************/

#include <stdlib.h>
#include "dsp.h"
#include "csimd.h"

using namespace csimd;

static ogg_uint32_t sad8x8__mmxext (unsigned char *ptr1, ogg_uint32_t stride1,
		       	    unsigned char *ptr2, ogg_uint32_t stride2)
{
  ogg_uint32_t  DiffVal;
  __m64 mm7=_mm_setzero_si64();
  for (int i=0;i<8;i++,ptr1+=stride1,ptr2+=stride2)
   {
    __m64 mm0,mm1;
    movq (ptr1, mm0);             	/* take 8 bytes */
    movq (ptr2, mm1);             
    psadbw (mm1, mm0);          
    paddw (mm0, mm7);           	/* accumulate difference... */
   }
  movd (mm7, (int&)DiffVal);               
  return DiffVal;
}

static ogg_uint32_t sad8x8_thres__mmxext (unsigned char *ptr1, ogg_uint32_t stride1,
		       		  unsigned char *ptr2, ogg_uint32_t stride2, 
			   	  ogg_uint32_t thres)
{
  ogg_uint32_t  DiffVal;

  __m64 mm7=_mm_setzero_si64();             	/* mm7 contains the result */

  for (int i=0;i<8;i++,ptr1+=stride1,ptr2+=stride2)
   {
    __m64 mm0,mm1;
    movq (ptr1, mm0);             	/* take 8 bytes */
    movq (ptr2, mm1);             
    psadbw (mm1, mm0);          
    paddw (mm0, mm7);           	/* accumulate difference... */
   }

  movd (mm7, (int&)DiffVal);               

  return DiffVal;
}

static ogg_uint32_t sad8x8_xy2_thres__mmxext (unsigned char *SrcData, ogg_uint32_t SrcStride,
		                      unsigned char *RefDataPtr1,
			              unsigned char *RefDataPtr2, ogg_uint32_t RefStride,
			              ogg_uint32_t thres)
{
  ogg_uint32_t  DiffVal;
  __m64 mm7=_mm_setzero_si64();
  for (int i=0;i<8;i++,SrcData+=SrcStride,RefDataPtr1+=RefStride,RefDataPtr2+=RefStride)
   {
      __m64 mm0,mm1,mm2;
      movq (SrcData, mm0);             	/* take 8 bytes */
      movq (RefDataPtr1, mm1);             
      movq (RefDataPtr2, mm2);             
      pavgb (mm2, mm1);           
      psadbw (mm1, mm0);          
      paddw (mm0, mm7);           	/* accumulate difference... */
   }

  movd (mm7, (int&)DiffVal);
  return DiffVal;
}
		
static ogg_uint32_t row_sad8__mmxext (unsigned char *Src1, unsigned char *Src2)
{
  ogg_uint32_t MaxSad;
  __m64 mm0,mm1,mm2,mm3;
      movd        (Src1, mm0);
      movd        (Src2, mm1);      
      psadbw      (mm0, mm1);     
      movd        (4+Src1, mm2);     
      movd        (4+Src2, mm3);     
      psadbw      (mm2, mm3);     

      pmaxsw      (mm1, mm3);     
      movd        (mm3, (int&)MaxSad);        
  return MaxSad&0xffff;
}

static ogg_uint32_t col_sad8x8__mmxext (unsigned char *Src1, unsigned char *Src2,
		                    ogg_uint32_t stride)
{
  ogg_uint32_t MaxSad;

  __m64 mm3=_mm_setzero_si64(),mm4=mm3,mm5=mm3,mm6=mm3,mm7=mm3,mm0,mm1,mm2;
  int i;
  for (i=0;i<4;i++,Src1+=stride,Src2+=stride)
   {
      movq        (Src1, mm0);      	/* take 8 bytes */
      movq        (Src2, mm1);      	/* take 8 bytes */

      movq        (mm0, mm2);     
      psubusb     (mm1, mm0);      	/* A - B */
      psubusb     (mm2, mm1);     	/* B - A */
      por         (mm1, mm0);           	/* and or gives abs difference */
      movq        (mm0, mm1);     

      punpcklbw   (mm3, mm0);     	/* unpack to higher precision for accumulation */
      paddw       (mm0, mm4);     	/* accumulate difference... */
      punpckhbw   (mm3, mm1);     	/* unpack high four bytes to higher precision */
      paddw       (mm1, mm5);     	/* accumulate difference... */
   }
  for (i=0;i<4;i++,Src1+=stride,Src2+=stride)
   {
      movq        (Src1, mm0);      	/* take 8 bytes */
      movq        (Src2, mm1);      	/* take 8 bytes */

      movq        (mm0, mm2);     
      psubusb     (mm1, mm0);      	/* A - B */
      psubusb     (mm2, mm1);     	/* B - A */
      por         (mm1, mm0);           	/* and or gives abs difference */
      movq        (mm0, mm1);     

      punpcklbw   (mm3, mm0);     	/* unpack to higher precision for accumulation */
      paddw       (mm0, mm6);     	/* accumulate difference... */
      punpckhbw   (mm3, mm1);     	/* unpack high four bytes to higher precision */
      paddw       (mm1, mm7);     	/* accumulate difference... */
   }
      pmaxsw      (mm6, mm7);     
      pmaxsw      (mm4, mm5);     
      pmaxsw      (mm5, mm7);     
      movq        (mm7, mm6);     
      psrlq       (32, mm6 );      
      pmaxsw      (mm6, mm7);     
      movq        (mm7, mm6);     
      psrlq       (16, mm6 );      
      pmaxsw      (mm6, mm7);     
      movd        (mm7, (int&)MaxSad);        
  return MaxSad&0xffff;
}

static ogg_uint32_t inter8x8_err_xy2__mmxext (unsigned char *SrcData, ogg_uint32_t SrcStride,
		                     unsigned char *RefDataPtr1,
				     unsigned char *RefDataPtr2, ogg_uint32_t RefStride)
{
  ogg_int16_t XSum;
  ogg_uint32_t XXSum;

  __m64 mm4=_mm_setzero_si64(),mm5=mm4,mm6=mm4,mm7=mm4,mm0,mm1,mm2,mm3;
  for (int i=0;i<8;i++,SrcData+=SrcStride,RefDataPtr1+=RefStride,RefDataPtr2+=RefStride)
   {
      movq        (SrcData, mm0);      	/* take 8 bytes */

      movq        (RefDataPtr1, mm2);      
      movq        (RefDataPtr2, mm1);      	/* take average of mm2 and mm1 */
      pavgb       (mm2, mm1);     

      movq        (mm0, mm2);     
      movq        (mm1, mm3);     

      punpcklbw   (mm6, mm0);     
      punpcklbw   (mm4, mm1);     
      punpckhbw   (mm6, mm2);     
      punpckhbw   (mm4, mm3);     

      psubsw      (mm1, mm0);     
      psubsw      (mm3, mm2);     

      paddw       (mm0, mm5);     
      paddw       (mm2, mm5);     

      pmaddwd     (mm0, mm0);     
      pmaddwd     (mm2, mm2);     
    
      paddd       (mm0, mm7);     
      paddd       (mm2, mm7);     
   }

      movq        (mm5, mm0);     
      psrlq       (32, mm5 );      
      paddw       (mm0, mm5);     
      movq        (mm5, mm0);     
      psrlq       (16, mm5 );      
      paddw       (mm0, mm5);     
      int edi;
      movd        (mm5, edi);     
      //movsx       di, edi      
      //movl        edi, XSum        
      XSum=edi;

      movq        (mm7, mm0);     
      psrlq       (32, mm7 );      
      paddd       (mm0, mm7);     
      movd        (mm7, (int&)XXSum);        
  /* Compute and return population variance as mis-match metric. */
  return (( (XXSum<<6) - XSum*XSum ));
}

void dsp_i386_mmxext_init(DspFunctions *funcs)
{
  funcs->row_sad8 = row_sad8__mmxext;
  funcs->col_sad8x8 = col_sad8x8__mmxext;
  funcs->sad8x8 = sad8x8__mmxext;
  funcs->sad8x8_thres = sad8x8_thres__mmxext;
  funcs->sad8x8_xy2_thres = sad8x8_xy2_thres__mmxext;
  funcs->inter8x8_err_xy2 = inter8x8_err_xy2__mmxext;
}

