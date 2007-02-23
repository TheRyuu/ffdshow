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

static void sub8x8__mmx (unsigned char *FiltPtr, unsigned char *ReconPtr,
                  ogg_int16_t *DctInputPtr, ogg_uint32_t PixelsPerLine,
                  ogg_uint32_t ReconPixelsPerLine)
{
    __m64 mm7=_mm_setzero_si64();

    for (int i=0;i<8;i++)
     {
      __m64 mm0,mm1,mm2,mm3;
      movq        (FiltPtr, mm0);       /* mm0 = FiltPtr */
      movq        (ReconPtr, mm1);       /* mm1 = ReconPtr */
      movq        (mm0, mm2);      /* dup to prepare for up conversion */
      movq        (mm1, mm3);      /* dup to prepare for up conversion */
    /* convert from UINT8 to INT16 */
      punpcklbw   (mm7, mm0);      /* mm0 = INT16(FiltPtr) */
      punpcklbw   (mm7, mm1);      /* mm1 = INT16(ReconPtr) */
      punpckhbw   (mm7, mm2);      /* mm2 = INT16(FiltPtr) */
      punpckhbw   (mm7, mm3);      /* mm3 = INT16(ReconPtr) */
    /* start calculation */
      psubw       (mm1, mm0);      /* mm0 = FiltPtr - ReconPtr */
      psubw       (mm3, mm2);      /* mm2 = FiltPtr - ReconPtr */
      movq        (mm0,  DctInputPtr);      /* write answer out */
      movq        (mm2, 4+DctInputPtr);     /* write answer out */
    /* Increment pointers */
      DctInputPtr+=8;
      FiltPtr+=PixelsPerLine;
      ReconPtr+=ReconPixelsPerLine;
     }
}

static void sub8x8_128__mmx (unsigned char *FiltPtr, ogg_int16_t *DctInputPtr,
                      ogg_uint32_t PixelsPerLine)
{
 __m64 mm1 = _mm_set1_pi16(0x00080);

 __m64 mm7=_mm_setzero_si64();
 for (int i=0;i<8;i++)
  {
   __m64 mm0,mm2;
      movq        (FiltPtr, mm0);       /* mm0 = FiltPtr */
      movq        (mm0, mm2);      /* dup to prepare for up conversion */
    /* convert from UINT8 to INT16 */
      punpcklbw   (mm7, mm0);      /* mm0 = INT16(FiltPtr) */
      punpckhbw   (mm7, mm2);      /* mm2 = INT16(FiltPtr) */
    /* start calculation */
      psubw       (mm1, mm0);      /* mm0 = FiltPtr - 128 */
      psubw       (mm1, mm2);      /* mm2 = FiltPtr - 128 */
      movq        (mm0,  DctInputPtr);      /* write answer out */
      movq        (mm2, 4+DctInputPtr);      /* write answer out */
    /* Increment pointers */
      DctInputPtr+=8;
      FiltPtr+=PixelsPerLine;
   }
}

static void sub8x8avg2__mmx (unsigned char *FiltPtr, unsigned char *ReconPtr1,
                     unsigned char *ReconPtr2, ogg_int16_t *DctInputPtr,
                     ogg_uint32_t PixelsPerLine,
                     ogg_uint32_t ReconPixelsPerLine)
{
 __m64 mm7=_mm_setzero_si64();

 for (int i=0;i<8;i++)
  {
   __m64 mm0,mm1,mm2,mm3,mm4,mm5;
      movq        (FiltPtr, mm0);       /* mm0 = FiltPtr */
      movq        (ReconPtr1, mm1);       /* mm1 = ReconPtr1 */
      movq        (ReconPtr2, mm4);       /* mm1 = ReconPtr2 */
      movq        (mm0, mm2);      /* dup to prepare for up conversion */
      movq        (mm1, mm3);      /* dup to prepare for up conversion */
      movq        (mm4, mm5);      /* dup to prepare for up conversion */
    /* convert from UINT8 to INT16 */
      punpcklbw   (mm7, mm0);      /* mm0 = INT16(FiltPtr) */
      punpcklbw   (mm7, mm1);      /* mm1 = INT16(ReconPtr1) */
      punpcklbw   (mm7, mm4);      /* mm1 = INT16(ReconPtr2) */
      punpckhbw   (mm7, mm2);      /* mm2 = INT16(FiltPtr) */
      punpckhbw   (mm7, mm3);      /* mm3 = INT16(ReconPtr1) */
      punpckhbw   (mm7, mm5);     /* mm3 = INT16(ReconPtr2) */
    /* average ReconPtr1 and ReconPtr2 */
      paddw       (mm4, mm1);      /* mm1 = ReconPtr1 + ReconPtr2 */
      paddw       (mm5, mm3);      /* mm3 = ReconPtr1 + ReconPtr2 */
      psrlw       (1, mm1);         /* mm1 = (ReconPtr1 + ReconPtr2) / 2 */
      psrlw       (1, mm3);         /* mm3 = (ReconPtr1 + ReconPtr2) / 2 */
      psubw       (mm1, mm0);      /* mm0 = FiltPtr - ((ReconPtr1 + ReconPtr2) / 2) */
      psubw       (mm3, mm2);      /* mm2 = FiltPtr - ((ReconPtr1 + ReconPtr2) / 2) */
      movq        (mm0,  DctInputPtr);      /* write answer out */
      movq        (mm2, 4+DctInputPtr);      /* write answer out */
    /* Increment pointers */
      DctInputPtr+=8;
      FiltPtr+=PixelsPerLine;
      ReconPtr1+=ReconPixelsPerLine;
      ReconPtr2+=ReconPixelsPerLine;
   }
}

static ogg_uint32_t row_sad8__mmx (unsigned char *Src1, unsigned char *Src2)
{
  ogg_uint32_t MaxSad;
  __m64 mm6=_mm_setzero_si64(),mm7=mm6,mm0,mm1,mm2,mm3;
      movq        (Src1, mm0);      	/* take 8 bytes */
      movq        (Src2, mm1);

      movq        (mm0, mm2);
      psubusb     (mm1, mm0);      	/* A - B */
      psubusb     (mm2, mm1);     	/* B - A */
      por         (mm1, mm0);           	/* and or gives abs difference */

      movq        (mm0, mm1);

      punpcklbw   (mm6, mm0);            /* ; unpack low four bytes to higher precision */
      punpckhbw   (mm7, mm1);            /* ; unpack high four bytes to higher precision */

      movq        (mm0, mm2);
      movq        (mm1, mm3);
      psrlq       (32, mm2 );      	/* fold and add */
      psrlq       (32, mm3 );
      paddw       (mm2, mm0);
      paddw       (mm3, mm1);
      movq        (mm0, mm2);
      movq        (mm1, mm3);
      psrlq       (16, mm2 );
      psrlq       (16, mm3 );
      paddw       (mm2, mm0);
      paddw       (mm3, mm1);

      psubusw     (mm0, mm1);
      paddw       (mm0, mm1);      	/* mm1 = max(mm1, mm0) */
      movd        (mm1, (int&)MaxSad);

  return MaxSad&0xffff;
}

static ogg_uint32_t col_sad8x8__mmx (unsigned char *Src1, unsigned char *Src2,
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

      psubusw     (mm6, mm7);
      paddw       (mm6, mm7);      	/* mm7 = max(mm7, mm6) */
      psubusw     (mm4, mm5);
      paddw       (mm4, mm5);      	/* mm5 = max(mm5, mm4) */
      psubusw     (mm5, mm7);
      paddw       (mm5, mm7);      	/* mm7 = max(mm5, mm7) */
      movq        (mm7, mm6);
      psrlq       (32, mm6 );
      psubusw     (mm6, mm7);
      paddw       (mm6, mm7);      	/* mm7 = max(mm5, mm7) */
      movq        (mm7, mm6);
      psrlq       (16, mm6 );
      psubusw     (mm6, mm7);
      paddw       (mm6, mm7);      	/* mm7 = max(mm5, mm7) */
      movd        (mm7, (int&)MaxSad);

  return MaxSad&0xffff;
}

static ogg_uint32_t sad8x8__mmx (unsigned char *ptr1, ogg_uint32_t stride1,
		       	    unsigned char *ptr2, ogg_uint32_t stride2)
{
  ogg_uint32_t  DiffVal;

  __m64 mm6=_mm_setzero_si64(),mm7=mm6,mm0,mm1,mm2;
  for (int i=0;i<8;i++,ptr1+=stride1,ptr2+=stride2)
   {
      movq        (ptr1, mm0);      	/* take 8 bytes */
      movq        (ptr2, mm1);
      movq        (mm0, mm2);

      psubusb     (mm1, mm0);      	/* A - B */
      psubusb     (mm2, mm1);     	/* B - A */
      por         (mm1, mm0);           	/* and or gives abs difference */
      movq        (mm0, mm1);

      punpcklbw   (mm6, mm0);     	/* unpack to higher precision for accumulation */
      paddw       (mm0, mm7);     	/* accumulate difference... */
      punpckhbw   (mm6, mm1);     	/* unpack high four bytes to higher precision */
      paddw       (mm1, mm7);     	/* accumulate difference... */
    }

      movq        (mm7, mm0     );
      psrlq       (32, mm7       );
      paddw       (mm0, mm7     );
      movq        (mm7, mm0     );
      psrlq       (16, mm7       );
      paddw       (mm0, mm7     );
      movd        (mm7, (int&)DiffVal        );
  return DiffVal&0xffff;;
}

static ogg_uint32_t sad8x8_thres__mmx (unsigned char *ptr1, ogg_uint32_t stride1,
		       		  unsigned char *ptr2, ogg_uint32_t stride2,
			   	  ogg_uint32_t thres)
{
  return sad8x8__mmx (ptr1, stride1, ptr2, stride2);
}

static ogg_uint32_t sad8x8_xy2_thres__mmx (unsigned char *SrcData, ogg_uint32_t SrcStride,
		                      unsigned char *RefDataPtr1,
			              unsigned char *RefDataPtr2, ogg_uint32_t RefStride,
			              ogg_uint32_t thres)
{
  ogg_uint32_t  DiffVal;
  __m64 mm5;
  pcmpeqd     (mm5, mm5);     	/* fefefefefefefefe in mm5 */
  paddb       (mm5, mm5);
  __m64 mm6=_mm_setzero_si64(),mm7=mm6,mm0,mm1,mm2,mm3;
   for (int i=0;i<8;i++,SrcData+=SrcStride,RefDataPtr1+=RefStride,RefDataPtr2+=RefStride)
    {
      movq        (SrcData, mm0);      	/* take 8 bytes */

      movq        (RefDataPtr1, mm2);
      movq        (RefDataPtr2, mm3);      	/* take average of mm2 and mm3 */
      movq        (mm2, mm1);
      pand        (mm3, mm1);
      pxor        (mm2, mm3);
      pand        (mm5, mm3);
      psrlq       (1, mm3 );
      paddb       (mm3, mm1);

      movq        (mm0, mm2);

      psubusb     (mm1, mm0);      	/* A - B */
      psubusb     (mm2, mm1);     	/* B - A */
      por         (mm1, mm0);         	/* and or gives abs difference */
      movq        (mm0, mm1);

      punpcklbw   (mm6, mm0);     	/* unpack to higher precision for accumulation */
      paddw       (mm0, mm7);     	/* accumulate difference... */
      punpckhbw   (mm6, mm1);     	/* unpack high four bytes to higher precision */
      paddw       (mm1, mm7);     	/* accumulate difference... */
     }

      movq        (mm7, mm0);
      psrlq       (32, mm7 );
      paddw       (mm0, mm7);
      movq        (mm7, mm0);
      psrlq       (16, mm7 );
      paddw       (mm0, mm7);
      movd        (mm7, (int&)DiffVal);

  return DiffVal&0xffff;
}

static ogg_uint32_t intra8x8_err__mmx (unsigned char *DataPtr, ogg_uint32_t Stride)
{
  ogg_int16_t  XSum;
  ogg_uint32_t  XXSum;

  __m64 mm5=_mm_setzero_si64(),mm6=mm5,mm7=mm5,mm0,mm2;
  for (int i=0;i<8;i++,DataPtr+=Stride)
   {
      movq        (DataPtr, mm0);      	/* take 8 bytes */
      movq        (mm0, mm2);

      punpcklbw   (mm6, mm0);
      punpckhbw   (mm6, mm2);

      paddw       (mm0, mm5);
      paddw       (mm2, mm5);

      pmaddwd     (mm0, mm0);
      pmaddwd     (mm2, mm2);

      paddd       (mm0, mm7);
      paddd       (mm2, mm7);

    }

      movq        (mm5, mm0);
      psrlq       (32, mm5);
      paddw       (mm0, mm5);
      movq        (mm5, mm0);
      psrlq       (16, mm5);
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

  /* Compute population variance as mis-match metric. */
  return (( (XXSum<<6) - XSum*XSum ) );
}

static ogg_uint32_t inter8x8_err__mmx (unsigned char *SrcData, ogg_uint32_t SrcStride,
		                 unsigned char *RefDataPtr, ogg_uint32_t RefStride)
{
  ogg_int16_t  XSum;
  ogg_uint32_t  XXSum;

  __m64 mm5=_mm_setzero_si64(),mm6=mm5,mm7=mm5,mm0,mm1,mm2,mm3;

  for (int i=0;i<8;i++,SrcData+=SrcStride,RefDataPtr+=RefStride)
   {
      movq        (SrcData, mm0);      	/* take 8 bytes */
      movq        (RefDataPtr, mm1);
      movq        (mm0, mm2);
      movq        (mm1, mm3);

      punpcklbw   (mm6, mm0);
      punpcklbw   (mm6, mm1);
      punpckhbw   (mm6, mm2);
      punpckhbw   (mm6, mm3);

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
      psrlq       (32, mm7);
      paddd       (mm0, mm7);
      movd        (mm7, (int&)XXSum);

  /* Compute and return population variance as mis-match metric. */
  return (( (XXSum<<6) - XSum*XSum ));
}

static ogg_uint32_t inter8x8_err_xy2__mmx (unsigned char *SrcData, ogg_uint32_t SrcStride,
		                     unsigned char *RefDataPtr1,
				     unsigned char *RefDataPtr2, ogg_uint32_t RefStride)
{
  ogg_int16_t XSum;
  ogg_uint32_t XXSum;

  __m64 mm4;
  pcmpeqd     (mm4, mm4);     	/* fefefefefefefefe in mm4 */
  paddb       (mm4, mm4);
  __m64 mm5=_mm_setzero_si64(),mm6=mm5,mm7=mm5,mm0,mm1,mm2,mm3;
  for (int i=0;i<8;i++,SrcData+=SrcStride,RefDataPtr1+=RefStride,RefDataPtr2+=RefStride)
   {
      movq        (SrcData, mm0);      	/* take 8 bytes */

      movq        (RefDataPtr1, mm2);
      movq        (RefDataPtr2, mm3);      	/* take average of mm2 and mm3 */
      movq        (mm2, mm1);
      pand        (mm3, mm1);
      pxor        (mm2, mm3);
      pand        (mm4, mm3);
      psrlq       (1, mm3 );
      paddb       (mm3, mm1);

      movq        (mm0, mm2);
      movq        (mm1, mm3);

      punpcklbw   (mm6, mm0);
      punpcklbw   (mm6, mm1);
      punpckhbw   (mm6, mm2);
      punpckhbw   (mm6, mm3);

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

static void restore_fpu (void)
{
 _mm_empty();
}

extern "C"
void dsp_i386_mmx_init(DspFunctions *funcs)
{
  funcs->restore_fpu = restore_fpu;
  funcs->sub8x8 = sub8x8__mmx;
  funcs->sub8x8_128 = sub8x8_128__mmx;
  funcs->sub8x8avg2 = sub8x8avg2__mmx;
  funcs->row_sad8 = row_sad8__mmx;
  funcs->col_sad8x8 = col_sad8x8__mmx;
  funcs->sad8x8 = sad8x8__mmx;
  funcs->sad8x8_thres = sad8x8_thres__mmx;
  funcs->sad8x8_xy2_thres = sad8x8_xy2_thres__mmx;
  funcs->intra8x8_err = intra8x8_err__mmx;
  funcs->inter8x8_err = inter8x8_err__mmx;
  funcs->inter8x8_err_xy2 = inter8x8_err_xy2__mmx;
}





