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

#include "codec_internal.h"
#include "csimd.h"

using namespace csimd;

static void copy8x8__mmx (unsigned char *src,
	                unsigned char *dest,
	                ogg_uint32_t stride)
{
      __m64 mm0,mm1,mm2,mm3;

      movq        (src, mm0          );
      movq        (src+ stride, mm1  );
      movq        (src+ stride*2, mm2);
      movq        (src+ stride*3, mm3);


      movq        (mm0, dest          );
      movq        (mm1, dest+ stride  );
      movq        (mm2, dest+ stride*2);
      movq        (mm3, dest+ stride*3);

      src+=stride*4;
      dest+=stride*4;

      movq        (src, mm0          );
      movq        (src+ stride, mm1  );
      movq        (src+ stride*2, mm2);
      movq        (src+ stride*3, mm3);

      movq        (mm0, dest          );
      movq        (mm1, dest+ stride  );
      movq        (mm2, dest+ stride*2);
      movq        (mm3, dest+ stride*3);
}

static void recon_intra8x8__mmx (unsigned char *ReconPtr, ogg_int16_t *ChangePtr,
		      ogg_uint32_t LineStep)
{
 __m64 V128=_mm_set1_pi8((char)0x80),mm0,mm2;

      movq     (V128, mm0);        /* Set mm0 to 0x8080808080808080 */

      for (ogg_int16_t *edi=ChangePtr+64;ChangePtr<edi;) // lea         128(ChangePtr), edi       /* Endpoint in input buffer */
      {
       movq         (ChangePtr, mm2);         /* First four input values */

       packsswb    (4+ChangePtr, mm2);         /* pack with next(high) four values */
       por         (mm0, mm0);
       pxor        (mm0, mm2);         /* Convert result to unsigned (same as add 128) */
       ChangePtr+=8;// lea         16(ChangePtr), ChangePtr           /* Step source buffer */
       //cmp         edi, ChangePtr            /* are we done */

       movq        (mm2, ReconPtr);          /* store results */

       ReconPtr+=LineStep;//         (ReconPtr, LineStep), ReconPtr         /* Step output buffer */
      }                   /* Loop back if we are not done */
}

static void recon_inter8x8__mmx (unsigned char *ReconPtr, unsigned char *RefPtr,
		      ogg_int16_t *ChangePtr, ogg_uint32_t LineStep)
{
 __m64 mm0=_mm_setzero_si64(),mm2,mm3,mm4,mm5;
 for (ogg_int16_t *edi=ChangePtr+64;ChangePtr<edi;RefPtr+=LineStep,ReconPtr+=LineStep,ChangePtr+=8)
  {
   movq        (RefPtr, mm2);          /* (+3 misaligned) 8 reference pixels */
   movq        (ChangePtr, mm4);          /* first 4 changes */
   movq        (mm2, mm3);
   movq        (4+ChangePtr, mm5);         /* last 4 changes */
   punpcklbw   (mm0, mm2);         /* turn first 4 refs into positive 16-bit #s */
   paddsw      (mm4, mm2);         /* add in first 4 changes */
   punpckhbw   (mm0, mm3);         /* turn last 4 refs into positive 16-bit #s */
   paddsw      (mm5, mm3);         /* add in last 4 changes */
   packuswb    (mm3, mm2);         /* pack result to unsigned 8-bit values */
   movq        (mm2, ReconPtr);          /* store result */
  }
}

static void recon_inter8x8_half__mmx (unsigned char *ReconPtr, unsigned char *RefPtr1,
		           unsigned char *RefPtr2, ogg_int16_t *ChangePtr,
			   ogg_uint32_t LineStep)
{
 __m64 mm0,mm2,mm3,mm4,mm5,mm6,mm7;
      pxor        (mm0, mm0);
      for (ogg_int16_t *edi=ChangePtr+64;ChangePtr<edi;)
       {
        movq        (RefPtr1, mm2);          /* (+3 misaligned) 8 reference pixels */
        movq        (RefPtr2, mm4);          /* (+3 misaligned) 8 reference pixels */

        movq        (mm2, mm3);
        punpcklbw   (mm0, mm2);         /* mm2 = start ref1 as positive 16-bit #s */
        movq        (mm4, mm5);
        movq        (ChangePtr, mm6);          /* first 4 changes */
        punpckhbw   (mm0, mm3);         /* mm3 = end ref1 as positive 16-bit #s */
        movq        (4+ChangePtr, mm7);         /* last 4 changes */
        punpcklbw   (mm0, mm4);         /* mm4 = start ref2 as positive 16-bit #s */
        punpckhbw   (mm0, mm5);         /* mm5 = end ref2 as positive 16-bit #s */
        paddw       (mm4, mm2);         /* mm2 = start (ref1 + ref2) */
        paddw       (mm5, mm3);         /* mm3 = end (ref1 + ref2) */
        psrlw       (1, mm2 );           /* mm2 = start (ref1 + ref2)/2 */
        psrlw       (1, mm3 );           /* mm3 = end (ref1 + ref2)/2 */
        paddw       (mm6, mm2);         /* add changes to start */
        paddw       (mm7, mm3);         /* add changes to end */
        ChangePtr+=8;                                    /* next row of changes */
        packuswb    (mm3, mm2);         /* pack start|end to unsigned 8-bit */
        RefPtr1+=LineStep;               /* next row of reference pixels */
        RefPtr2+=LineStep;               /* next row of reference pixels */
        movq        (mm2, ReconPtr);          /* store result */
        ReconPtr+=LineStep;               /* next row of output */
       }
}

void dsp_i386_mmx_recon_init(DspFunctions *funcs)
{
  funcs->copy8x8 = copy8x8__mmx;
  funcs->recon_intra8x8 = recon_intra8x8__mmx;
  funcs->recon_inter8x8 = recon_inter8x8__mmx;
  funcs->recon_inter8x8_half = recon_inter8x8_half__mmx;
}





