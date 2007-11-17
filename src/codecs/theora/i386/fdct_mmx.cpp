;//==========================================================================
;//
;//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
;//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
;//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
;//  PURPOSE.
;//
;//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
;//
;//--------------------------------------------------------------------------

#include "theora.h"
#include "dsp.h"
#include "csimd.h"

using namespace csimd;

/***********************************************************************
 *	File:			fdct_m.asm
 *
 *	Description:
 *					This function perform 2-D Forward DCT on a 8x8 block
 *
 *
 *	Input:			Pointers to input source data buffer and destination
 *					buffer.
 *
 *	Note:			none
 *
 *	Special Notes:	We try to do the truncation right to match the result
 *					of the c version.
 *
 ************************************************************************/

/* execute stage 1 of forward DCT */
static __forceinline void Fdct_mmx(ogg_int16_t *ip0,ogg_int16_t *ip1,ogg_int16_t *ip2,ogg_int16_t *ip3,ogg_int16_t *ip4,ogg_int16_t *ip5,ogg_int16_t *ip6,ogg_int16_t *ip7,ogg_int16_t *temp)
{
 const __m64  xC1S7 = _mm_set1_pi16(0xfb15);
 const __m64  xC2S6 = _mm_set1_pi16(0xec83);
 const __m64  xC3S5 = _mm_set1_pi16(0xd4db);
 const __m64  xC4S4 = _mm_set1_pi16(0xb505);
 const __m64  xC5S3 = _mm_set1_pi16(0x8e3a);
 const __m64  xC6S2 = _mm_set1_pi16(0x61f8);
 const __m64  xC7S1 = _mm_set1_pi16(0x31f1);

 __m64 mm0,mm1,mm2,mm3,mm4,mm5,mm6,mm7;

    movq      ( ip0 , mm0);
    movq      ( ip1 , mm1);
    movq      ( ip3 , mm2);
    movq      ( ip5 , mm3);
    movq      (  mm0, mm4);
    movq      (  mm1, mm5);
    movq      (  mm2, mm6);
    movq      (  mm3, mm7);

    paddsw     (ip7 , mm0);       /* mm0 = ip0 + ip7 = is07 */
    paddsw     (ip2 , mm1);       /* mm1 = ip1 + ip2 = is12 */
    paddsw     (ip4 , mm2);       /* mm2 = ip3 + ip4 = is34 */
    paddsw     (ip6 , mm3);       /* mm3 = ip5 + ip6 = is56 */
    psubsw     (ip7 , mm4);       /* mm4 = ip0 - ip7 = id07 */
    psubsw     (ip2 , mm5);       /* mm5 = ip1 - ip2 = id12 */

    psubsw     ( mm2, mm0);        /* mm0 = is07 - is34 */

    paddsw     ( mm2, mm2);

    psubsw     (ip4 , mm6);       /* mm6 = ip3 - ip4 = id34 */

    paddsw     ( mm0, mm2 );       /* mm2 = is07 + is34 = is0734 */
    psubsw     ( mm3, mm1 );       /* mm1 = is12 - is56 */
    movq       ( mm0, temp);      /* Save is07 - is34 to free mm0; */
    paddsw     ( mm3, mm3 );
    paddsw     ( mm1, mm3 );       /* mm3 = is12 + 1s56	= is1256 */

    psubsw     (ip6 , mm7 );      /* mm7 = ip5 - ip6 = id56 */
  /* ------------------------------------------------------------------- */
    psubsw     ( mm7, mm5 );       /* mm5 = id12 - id56 */
    paddsw     ( mm7, mm7 );
    paddsw     ( mm5, mm7 );       /* mm7 = id12 + id56 */
  /* ------------------------------------------------------------------- */
    psubsw     ( mm3, mm2 );       /* mm2 = is0734 - is1256 */
    paddsw     ( mm3, mm3 );

    movq       ( mm2, mm0 );       /* make a copy */
    paddsw     ( mm2, mm3);        /* mm3 = is0734 + is1256 */

    pmulhw   (xC4S4, mm0);      /* mm0 = xC4S4 * ( is0734 - is1256 ) - ( is0734 - is1256 ) */
    paddw       (mm2, mm0  );      /* mm0 = xC4S4 * ( is0734 - is1256 ) */
    psrlw       (15, mm2  );
    paddw       (mm2, mm0  );      /* Truncate mm0, now it is op[4] */

    movq        (mm3, mm2  );
    movq        (mm0, ip4  );     /* save ip4, now mm0,mm2 are free */

    movq        (mm3, mm0  );
    pmulhw   (xC4S4, mm3);      /* mm3 = xC4S4 * ( is0734 +is1256 ) - ( is0734 +is1256 ) */

    psrlw       (15, mm2  );
    paddw       (mm0, mm3  );      /* mm3 = xC4S4 * ( is0734 +is1256 )	 */
    paddw       (mm2, mm3  );      /* Truncate mm3, now it is op[0] */

    movq        (mm3, ip0  );
  /* ------------------------------------------------------------------- */
    movq       (temp , mm3 );     /* mm3 = irot_input_y */
    pmulhw   (xC2S6, mm3);      /* mm3 = xC2S6 * irot_input_y - irot_input_y */

    movq       (temp , mm2 );
    movq       ( mm2, mm0  );

    psrlw      ( 15, mm2  );        /* mm3 = xC2S6 * irot_input_y */
    paddw      ( mm0, mm3  );

    paddw      ( mm2, mm3  );      /* Truncated */
    movq       ( mm5, mm0  );

    movq       ( mm5, mm2  );
    pmulhw   (xC6S2, mm0);      /* mm0 = xC6S2 * irot_input_x */

    psrlw      ( 15, mm2  );
    paddw      ( mm2, mm0  );      /* Truncated */

    paddsw     ( mm0, mm3  );      /* ip[2] */
    movq       ( mm3, ip2  );     /* Save ip2 */

    movq       ( mm5, mm0  );
    movq       ( mm5, mm2  );

    pmulhw   (xC2S6, mm5);      /* mm5 = xC2S6 * irot_input_x - irot_input_x */
    psrlw      ( 15, mm2  );

    movq       (temp , mm3 );
    paddw      ( mm0, mm5  );      /* mm5 = xC2S6 * irot_input_x */

    paddw      ( mm2, mm5  );      /* Truncated */
    movq       ( mm3, mm2  );

    pmulhw   (xC6S2, mm3);      /* mm3 = xC6S2 * irot_input_y */
    psrlw      ( 15, mm2  );

    paddw      ( mm2, mm3  );      /* Truncated */
    psubsw     ( mm5, mm3  );

    movq       ( mm3, ip6  );
  /* ------------------------------------------------------------------- */
    movq     (xC4S4, mm0);
    movq       ( mm1, mm2  );
    movq       ( mm1, mm3  );

    pmulhw     ( mm0, mm1  );      /* mm0 = xC4S4 * ( is12 - is56 ) - ( is12 - is56 ) */
    psrlw      ( 15, mm2  );

    paddw      ( mm3, mm1  );      /* mm0 = xC4S4 * ( is12 - is56 ) */
    paddw      ( mm2, mm1  );      /* Truncate mm1, now it is icommon_product1 */

    movq       ( mm7, mm2  );
    movq       ( mm7, mm3  );

    pmulhw     ( mm0, mm7  );      /* mm7 = xC4S4 * ( id12 + id56 ) - ( id12 + id56 ) */
    psrlw      ( 15, mm2  );

    paddw      ( mm3, mm7  );      /* mm7 = xC4S4 * ( id12 + id56 ) */
    paddw      ( mm2, mm7  );      /* Truncate mm7, now it is icommon_product2 */
  /* ------------------------------------------------------------------- */
    pxor       ( mm0, mm0  );      /* Clear mm0 */
    psubsw     ( mm6, mm0  );      /* mm0 = - id34 */

    psubsw     ( mm7, mm0  );      /* mm0 = - ( id34 + idcommon_product2 ) */
    paddsw     ( mm6, mm6  );
    paddsw     ( mm0, mm6  );      /* mm6 = id34 - icommon_product2 */

    psubsw     ( mm1, mm4  );      /* mm4 = id07 - icommon_product1 */
    paddsw     ( mm1, mm1  );
    paddsw     ( mm4, mm1  );      /* mm1 = id07 + icommon_product1 */
  /* ------------------------------------------------------------------- */
    movq     (xC1S7, mm7);
    movq       ( mm1, mm2  );

    movq       ( mm1, mm3  );
    pmulhw     ( mm7, mm1  );      /* mm1 = xC1S7 * irot_input_x - irot_input_x */

    movq     (xC7S1, mm7);
    psrlw      ( 15, mm2  );

    paddw      ( mm3, mm1  );      /* mm1 = xC1S7 * irot_input_x */
    paddw      ( mm2, mm1  );      /* Trucated */

    pmulhw     ( mm7, mm3  );      /* mm3 = xC7S1 * irot_input_x */
    paddw      ( mm2, mm3  );      /* Truncated */

    movq       ( mm0, mm5  );
    movq       ( mm0, mm2  );

    movq     (xC1S7, mm7);
    pmulhw     ( mm7, mm0  );      /* mm0 = xC1S7 * irot_input_y - irot_input_y */

    movq     (xC7S1, mm7);
    psrlw      ( 15, mm2  );

    paddw      ( mm5, mm0  );      /* mm0 = xC1S7 * irot_input_y */
    paddw      ( mm2, mm0  );      /* Truncated */

    pmulhw     ( mm7, mm5  );      /* mm5 = xC7S1 * irot_input_y */
    paddw      ( mm2, mm5  );      /* Truncated */

    psubsw     ( mm5, mm1  );      /* mm1 = xC1S7 * irot_input_x - xC7S1 * irot_input_y = ip1 */
    paddsw     ( mm0, mm3  );      /* mm3 = xC7S1 * irot_input_x - xC1S7 * irot_input_y = ip7 */

    movq       ( mm1, ip1  );
    movq       ( mm3, ip7  );
  /* ------------------------------------------------------------------- */
    movq     (xC3S5, mm0);
    movq     (xC5S3, mm1);

    movq       ( mm6, mm5  );
    movq       ( mm6, mm7  );

    movq       ( mm4, mm2  );
    movq       ( mm4, mm3  );

    pmulhw     ( mm0, mm4  );      /* mm4 = xC3S5 * irot_input_x - irot_input_x */
    pmulhw     ( mm1, mm6  );      /* mm6 = xC5S3 * irot_input_y - irot_input_y */

    psrlw      ( 15, mm2  );
    psrlw      ( 15, mm5  );

    paddw      ( mm3, mm4  );      /* mm4 = xC3S5 * irot_input_x */
    paddw      ( mm7, mm6  );      /* mm6 = xC5S3 * irot_input_y */

    paddw      ( mm2, mm4  );      /* Truncated */
    paddw      ( mm5, mm6  );      /* Truncated */

    psubsw     ( mm6, mm4  );      /* ip3 */
    movq       ( mm4, ip3  );

    movq       ( mm3, mm4  );
    movq       ( mm7, mm6  );

    pmulhw     ( mm1, mm3  );      /* mm3 = xC5S3 * irot_input_x - irot_input_x */
    pmulhw     ( mm0, mm7  );      /* mm7 = xC3S5 * irot_input_y - irot_input_y */

    paddw      ( mm2, mm4  );
    paddw      ( mm5, mm6  );

    paddw      ( mm4, mm3  );      /* mm3 = xC5S3 * irot_input_x */
    paddw      ( mm6, mm7  );      /* mm7 = xC3S5 * irot_input_y */

    paddw      ( mm7, mm3  );      /* ip5 */
    movq       ( mm3, ip5  );
}
static __forceinline void Transpose_mmx(ogg_int16_t *ip0,ogg_int16_t *ip1,ogg_int16_t *ip2,ogg_int16_t *ip3,ogg_int16_t *ip4,ogg_int16_t *ip5,ogg_int16_t *ip6,ogg_int16_t *ip7,ogg_int16_t *op0,ogg_int16_t *op1,ogg_int16_t *op2,ogg_int16_t *op3,ogg_int16_t *op4,ogg_int16_t *op5,ogg_int16_t *op6,ogg_int16_t *op7)
{
 __m64 mm0,mm1,mm2,mm3,mm4,mm5,mm6,mm7;
    movq        (ip0, mm0);       /* mm0 = a0 a1 a2 a3 */
    movq        (ip4, mm4);       /* mm4 = e4 e5 e6 e7 */
    movq        (ip1, mm1);       /* mm1 = b0 b1 b2 b3 */
    movq        (ip5, mm5);       /* mm5 = f4 f5 f6 f7 */
    movq        (ip2, mm2);       /* mm2 = c0 c1 c2 c3 */
    movq        (ip6, mm6);       /* mm6 = g4 g5 g6 g7 */
    movq        (ip3, mm3);       /* mm3 = d0 d1 d2 d3 */
    movq        (mm1, op1);       /* save  b0 b1 b2 b3 */
    movq        (ip7, mm7);       /* mm7 = h0 h1 h2 h3 */
   /* Transpose 2x8 block */
    movq        (mm4, mm1);        /* mm1 = e3 e2 e1 e0 */
    punpcklwd   (mm5, mm4);        /* mm4 = f1 e1 f0 e0 */
    movq        (mm0, op0);       /* save a3 a2 a1 a0  */
    punpckhwd	(mm5, mm1);        /* mm1 = f3 e3 f2 e2 */
    movq        (mm6, mm0);        /* mm0 = g3 g2 g1 g0 */
    punpcklwd	(mm7, mm6);        /* mm6 = h1 g1 h0 g0 */
    movq        (mm4, mm5);        /* mm5 = f1 e1 f0 e0 */
    punpckldq   (mm6, mm4);        /* mm4 = h0 g0 f0 e0 = MM4 */
    punpckhdq   (mm6, mm5);        /* mm5 = h1 g1 f1 e1 = MM5 */
    movq        (mm1, mm6);        /* mm6 = f3 e3 f2 e2 */
    movq        (mm4, op4);
    punpckhwd   (mm7, mm0);        /* mm0 = h3 g3 h2 g2 */
    movq        (mm5, op5);
    punpckhdq   (mm0, mm6);        /* mm6 = h3 g3 f3 e3 = MM7 */
    movq        (op0, mm4);       /* mm4 = a3 a2 a1 a0 */
    punpckldq   (mm0, mm1);        /* mm1 = h2 g2 f2 e2 = MM6 */
    movq        (op1, mm5);       /* mm5 = b3 b2 b1 b0 */
    movq        (mm4, mm0);        /* mm0 = a3 a2 a1 a0 */
    movq        (mm6, op7);
    punpcklwd   (mm5, mm0);        /* mm0 = b1 a1 b0 a0 */
    movq        (mm1, op6);
    punpckhwd   (mm5, mm4);        /* mm4 = b3 a3 b2 a2 */
    movq        (mm2, mm5);        /* mm5 = c3 c2 c1 c0 */
    punpcklwd   (mm3, mm2);        /* mm2 = d1 c1 d0 c0 */
    movq        (mm0, mm1);        /* mm1 = b1 a1 b0 a0 */
    punpckldq   (mm2, mm0);        /* mm0 = d0 c0 b0 a0 = MM0 */
    punpckhdq   (mm2, mm1);        /* mm1 = d1 c1 b1 a1 = MM1 */
    movq        (mm4, mm2);        /* mm2 = b3 a3 b2 a2 */
    movq        (mm0, op0);
    punpckhwd   (mm3, mm5);        /* mm5 = d3 c3 d2 c2 */
    movq        (mm1, op1);
    punpckhdq   (mm5, mm4);        /* mm4 = d3 c3 b3 a3 = MM3 */
    punpckldq   (mm5, mm2);        /* mm2 = d2 c2 b2 a2 = MM2 */
    movq        (mm4, op3);
    movq        (mm2, op2);
}

static void fdct_short__mmx ( ogg_int16_t *InputData, ogg_int16_t *OutputData)
{
  __align8(ogg_int64_t,align_tmp[16]);
  ogg_int16_t *const temp= (ogg_int16_t*)align_tmp;

    /*
     * Input data is an 8x8 block.  To make processing of the data more efficent
     * we will transpose the block of data to two 4x8 blocks???
     */
    Transpose_mmx (  InputData, 16/2+InputData, 32/2+InputData, 48/2+InputData,  8/2+InputData, 24/2+InputData, 40/2+InputData, 56/2+InputData,OutputData, 16/2+OutputData, 32/2+OutputData, 48/2+OutputData,  8/2+OutputData, 24/2+OutputData, 40/2+OutputData, 56/2+OutputData);
    Fdct_mmx      (  OutputData, 16/2+OutputData, 32/2+OutputData, 48/2+OutputData,  8/2+OutputData, 24/2+OutputData, 40/2+OutputData, 56/2+OutputData, temp);

    Transpose_mmx (64/2+InputData, 80/2+InputData, 96/2+InputData,112/2+InputData, 72/2+InputData, 88/2+InputData,104/2+InputData,120/2+InputData, 64/2+OutputData, 80/2+OutputData, 96/2+OutputData,112/2+OutputData, 72/2+OutputData, 88/2+OutputData,104/2+OutputData,120/2+OutputData);
    Fdct_mmx      (64/2+OutputData, 80/2+OutputData, 96/2+OutputData,112/2+OutputData, 72/2+OutputData, 88/2+OutputData,104/2+OutputData,120/2+OutputData, temp);

    Transpose_mmx ( 0/2+OutputData, 16/2+OutputData, 32/2+OutputData, 48/2+OutputData, 64/2+OutputData, 80/2+OutputData, 96/2+OutputData,112/2+OutputData, 0/2+OutputData, 16/2+OutputData, 32/2+OutputData, 48/2+OutputData, 64/2+OutputData, 80/2+OutputData, 96/2+OutputData,112/2+OutputData);
    Fdct_mmx      ( 0/2+OutputData, 16/2+OutputData, 32/2+OutputData, 48/2+OutputData, 64/2+OutputData, 80/2+OutputData, 96/2+OutputData,112/2+OutputData, temp);

    Transpose_mmx ( 8/2+OutputData, 24/2+OutputData, 40/2+OutputData, 56/2+OutputData, 72/2+OutputData, 88/2+OutputData,104/2+OutputData,120/2+OutputData,  8/2+OutputData, 24/2+OutputData, 40/2+OutputData, 56/2+OutputData, 72/2+OutputData, 88/2+OutputData,104/2+OutputData,120/2+OutputData);
    Fdct_mmx      ( 8/2+OutputData, 24/2+OutputData, 40/2+OutputData, 56/2+OutputData, 72/2+OutputData, 88/2+OutputData,104/2+OutputData,120/2+OutputData, temp);

    _mm_empty();
}

void dsp_i386_mmx_fdct_init(DspFunctions *funcs)
{
  funcs->fdct_short = fdct_short__mmx;
}
