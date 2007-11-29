//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
//
//--------------------------------------------------------------------------


/****************************************************************************
*
*   Module Title :     loopf_asm.c
*
*   Description  :     Optimized version of the loop filter.
*
*
*****************************************************************************
*/

/****************************************************************************
*  Header Frames
*****************************************************************************
*/


#pragma warning (disable:4799)
#pragma warning (disable:4731)


#define STRICT              /* Strict type checking. */
#include <memory.h>

#include "codecs/theora/ogg.h"
#include "codec_internal.h"
#include "inttypes.h"
#include "simd.h"

#define LIMIT_OFFSET        0
#define FOURONES_OFFSET     8
#define LFABS_OFFSET        16
#define TRANS_OFFSET        24

/****************************************************************************
*  Module constants.
*****************************************************************************
*/

/****************************************************************************
*  Explicit Imports
*****************************************************************************
*/

/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/

/****************************************************************************
*  Exported Functions
*****************************************************************************
*/

/****************************************************************************
*  Module Statics
*****************************************************************************
*/

/****************************************************************************
 *
 *  ROUTINE       :     SetupBoundingValueArray_ForMMX
 *
 *  INPUTS        :
 *
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Applies a loop filter to the edge pixels of coded blocks.
 *
 *  SPECIAL NOTES :
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
extern "C" void SetupBoundingValueArray_ForMMX(PB_INSTANCE *pbi, ogg_int32_t FLimit)
{
    ogg_int32_t * BoundingValuePtr;


    //    Since the FiltBoundingValue array is currently only used in the generic version, we are going
    //    to reuse this memory for our own purposes.
    //    2 longs for limit, 2 longs for _4ONES, 2 longs for LFABS_MMX, and 8 longs for temp work storage

    BoundingValuePtr = (ogg_int32_t *)((ogg_uint32_t)(&pbi->FiltBoundingValue[256]) & 0xffffffe0);

    //expand for mmx code
    BoundingValuePtr[0] = BoundingValuePtr[1] = FLimit * 0x00010001;
    BoundingValuePtr[2] = BoundingValuePtr[3] = 0x00010001;
    BoundingValuePtr[4] = BoundingValuePtr[5] = 0x00040004;

    pbi->BoundingValuePtr=BoundingValuePtr;
}
/****************************************************************************
 *
 *  ROUTINE       :     FilterHoriz_MMX
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Applies a loop filter to the vertical edge horizontally
 *
 *  SPECIAL NOTES :
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
// this version attempts to fix the DC_misalign stalls
extern "C" void FilterHoriz_MMX(unsigned char * PixelPtr, ogg_int32_t LineLength, ogg_int32_t *BoundingValuePtr)
{
    ogg_int32_t ms = -LineLength;
    ogg_int32_t ms2 = ms + ms;

    /* A somewhat optimized MMX version of the left edge filter.*/
        unsigned char *eax=(unsigned char*)BoundingValuePtr;
        int         edx=LineLength;            //stride

        unsigned char *ebx=PixelPtr;
        int         ecx=LineLength;            //stride

        __m64 mm0,mm1,mm2,mm3,mm4,mm5,mm6,mm7;

        movd        (mm0,ebx + -2              );//xx xx xx xx 01 00 xx xx
    //-

        movd        (mm4,ebx + 2               );//xx xx xx xx xx xx 03 02
        psrld       (mm0,16                      );//xx xx xx xx 00 00 01 00

        movd        (mm1,ebx + ecx + -2        );//xx xx xx xx 11 10 xx xx
        punpcklwd   (mm0,mm4                     );//xx xx xx xx 03 02 01 00

        movd        (mm4,ebx + ecx + 2         );//xx xx xx xx xx xx 13 12
        psrld       (mm1,16                      );//xx xx xx xx 00 00 11 10

        punpcklwd   (mm1,mm4                     );//xx xx xx xx 13 12 11 10
        edx=edx*3           ;//stride * 3

        movd        (mm2,ebx + ecx*2 + -2      );//xx xx xx xx 21 20 xx xx
        punpcklbw   (mm0,mm1                     );//13 03 12 02 11 01 10 00

        movd        (mm4,ebx + ecx*2 + 2       );//xx xx xx xx xx xx 23 22
        psrld       (mm2,16                      );//xx xx xx xx 00 00 21 20

        movd        (mm1,ebx + edx + -2        );//xx xx xx xx 31 30 xx xx
        punpcklwd   (mm2,mm4                     );//xx xx xx xx 23 22 21 20

        movd        (mm4,ebx + edx + 2         );//xx xx xx xx xx xx 33 32
        psrld       (mm1,16                      );//xx xx xx xx 00 00 31 30

        punpcklwd   (mm1,mm4                     );//xx xx xx xx 33 32 31 30
        pxor        (mm4,mm4);

        punpcklbw   (mm2,mm1                     );//33 23 32 22 31 21 30 20
        movq        (mm1,mm0);

        punpcklwd   (mm0,mm2                     );//31 21 11 01 30 20 10 00
        ebx=ebx + ecx*4;//base + (stride * 4)

        punpckhwd   (mm1,mm2                     );//33 23 13 03 32 22 12 02
        movq        (mm6,mm0                     );//xx xx xx xx 30 20 10 00

        movq        (eax + TRANS_OFFSET + 0,mm0);
        movq        (mm2,mm1);

        movq        (eax + TRANS_OFFSET + 8,mm1);
        psrlq       (mm0,32                      );//xx xx xx xx 31 21 11 01

//-----------
        movd        (mm7,ebx + -2              );//xx xx xx xx 41 40 xx xx
        punpcklbw   (mm1,mm4                     );//convert to words

        movd        (mm4,ebx + 2               );//xx xx xx xx xx xx 43 42
        psrld       (mm7,16                      );//xx xx xx xx 00 00 41 40

        movd        (mm5,ebx + ecx + -2        );//xx xx xx xx 51 50 xx xx
        punpcklwd   (mm7,mm4                     );//xx xx xx xx 43 42 41 40

        movd        (mm4,ebx + ecx + 2         );//xx xx xx xx xx xx 53 52
        psrld       (mm5,16);

        punpcklwd   (mm5,mm4);
        pxor        (mm4,mm4);

        punpcklbw   (mm0,mm4);
//-

        psrlq       (mm2,32                      );//xx xx xx xx 33 23 13 03
        psubw       (mm1,mm0                     );//x = p0 - pms

        punpcklbw   (mm7,mm5                     );//53 43 52 42 51 41 50 40
        movq        (mm3,mm1);
//-------------------
        punpcklbw   (mm6,mm4);
        paddw       (mm3,mm1);

        punpcklbw   (mm2,mm4);
        paddw       (mm1,mm3);

        paddw       (mm1,eax + LFABS_OFFSET    );//x += LoopFilterAdjustBeforeShift
        psubw       (mm6,mm2);

        movd        (mm2,ebx + ecx*2 + -2      );//xx xx xx xx 61 60 xx xx
        paddw       (mm6,mm1);

        movd        (mm4,ebx + ecx*2 + 2       );//xx xx xx xx xx xx 63 62
        psrld       (mm2,16);

        movd        (mm5,ebx + edx + -2        );//xx xx xx xx 71 70 xx xx
        punpcklwd   (mm2,mm4                     );//xx xx xx xx 63 62 61 60

        movd        (mm4,ebx + edx + 2         );//xx xx xx xx xx xx 73 72
        psrld       (mm5,16                      );//xx xx xx xx 00 00 71 70

        ebx=(unsigned char*)PixelPtr;              //restore PixelPtr
        punpcklwd   (mm5,mm4                     );//xx xx xx xx 73 72 71 70

        psraw       (mm6,3                       );//values to be clipped
        pxor        (mm4,mm4);

        punpcklbw   (mm2,mm5                     );//73 63 72 62 71 61 70 60
        movq        (mm5,mm7                     );//53 43 52 42 51 41 50 40

        movq        (mm1,mm6);
        punpckhwd   (mm5,mm2                     );//73 63 53 43 72 62 52 42


        movq        (eax + TRANS_OFFSET + 24,mm5   );//save for later
        punpcklwd   (mm7,mm2                     );//71 61 51 41 70 60 50 40

        movq        (eax + TRANS_OFFSET + 16,mm7   );//save for later
        psraw       (mm6,15);

        movq        (mm2,eax + LIMIT_OFFSET        );//get the limit value
        movq        (mm0,mm7                         );//xx xx xx xx 70 60 50 41

        psrlq       (mm7,32                          );//xx xx xx xx 71 61 51 41
        pxor        (mm1,mm6);

        psubsw      (mm1,mm6                         );//abs(i)
        punpcklbw   (mm5,mm4);

        por         (mm6,eax + FOURONES_OFFSET     );//now have -1 or 1
        movq        (mm3,mm2);

        punpcklbw   (mm7,mm4);
        psubw       (mm3,mm1                         );//limit - abs(i)

        movq        (mm4,mm3);
        psraw       (mm3,15);

        //push        ebp
    //-

        psubw       (mm5,mm7                         );//x = p0 - pms
        pxor        (mm4,mm3);

        psubsw      (mm4,mm3                         );//abs(limit - abs(i))
        pxor        (mm3,mm3);

//        movd        mm1,eax + TRANS_OFFSET + 28  );//xx xx xx xx 73 63 53 43
        movq        (mm1,eax + TRANS_OFFSET + 28  );//xx xx xx xx 73 63 53 43
        psubusw     (mm2,mm4                     );//limit - abs(limit - abs(i))

        punpcklbw   (mm0,mm3);
        movq        (mm7,mm5);

        paddw       (mm7,mm5);
        pmullw      (mm2,mm6                     );//new y -- wait 3 cycles

        punpcklbw   (mm1,mm3);
        paddw       (mm5,mm7);

        paddw       (mm5,eax + LFABS_OFFSET             );//x += LoopFilterAdjustBeforeShift
        psubw       (mm0,mm1);

        paddw       (mm0,mm5);
        pxor        (mm6,mm6     );

        movd        (mm7,eax + TRANS_OFFSET + 8  );//xx xx xx xx 32 22 12 02
        psraw       (mm0,3                       );//values to be clipped

        movd        (mm3,eax + TRANS_OFFSET + 4  );//xx xx xx xx 31 21 11 01
        punpcklbw   (mm7,mm6);

        psubw       (mm7,mm2                     );//pms + y
        punpcklbw   (mm3,mm6);

        paddw       (mm3,mm2                     );//p0 - y
        packuswb    (mm7,mm7                     );//clamp pms + y

        packuswb    (mm3,mm3                     );//clamp p0 - y
        movq        (mm1,mm0);

        movq        (mm2,eax + LIMIT_OFFSET                 );//get the limit value
        psraw       (mm0,15);

//values to write out
        punpcklbw   (mm3,mm7                     );//32 31 22 21 12 11 02 01
        movq        (mm7,mm0                     );//save sign

        unsigned int ebp;
        movd        ((int&)ebp,mm3                     );//12 11 02 01
        pxor        (mm1,mm0);

//xor bp,bp

        *(uint16_t*)(ebx + 1)=ebp               ;//02 01
        psubsw      (mm1,mm0                     );//abs(i)

        ebp>>=16;//shr         ebp,16
        movq        (mm5,mm2);

        *(uint16_t*)(ebx + ecx + 1)=ebp;
        psrlq       (mm3,32                      );//xx xx xx xx 32 31 22 21

        por         (mm7,eax + FOURONES_OFFSET                );//now have -1 or 1
        psubw       (mm5,mm1                     );//limit - abs(i)

        movd        ((int&)ebp,mm3                     );//32 31 22 21
        movq        (mm4,mm5);

        *(uint16_t*)(ebx + ecx*2 + 1)=ebp;
        psraw       (mm5,15);

        ebp>>=16;// shr         ebp,16
        pxor        (mm4,mm5);

        *(uint16_t*)(ebx + edx + 1)=ebp;
        psubsw      (mm4,mm5                     );//abs(limit - abs(i))

        movd        (mm5,eax + TRANS_OFFSET + 24  );//xx xx xx xx 72 62 52 42
        psubusw     (mm2,mm4                     );//limit - abs(limit - abs(i))

        pmullw      (mm2,mm7                     );//new y
        pxor        (mm6,mm6);

        movd        (mm3,eax + TRANS_OFFSET + 20  );//xx xx xx xx 71 61 51 41
        punpcklbw   (mm5,mm6);

        ebx=ebx + ecx*4;
        punpcklbw   (mm3,mm6);

        paddw       (mm3,mm2                     );//pms + y
        psubw       (mm5,mm2                     );//p0 - y

        packuswb    (mm3,mm3                     );//clamp pms + y
        //pop         ebp
    //-

//
//NOTE: optimize the following crap somehow
//

        packuswb    (mm5,mm5                     );//clamp p0 - y
    //-
        punpcklbw   (mm3,mm5                     );//72 71 62 61 52 51 42 41
    //-
        unsigned int ax;
        movd        ((int&)ax,mm3                     );//52 51 42 41
        psrlq       (mm3,32                      );//xx xx xx xx 72 71 62 61

        *(uint16_t*)(ebx + 1)=ax;
    //-
        ax>>=16;//shr         eax,16
    //-

        *(uint16_t*)(ebx + ecx + 1)=ax;
    //-


        movd        ((int&)ax,mm3);
    //-

        *(uint16_t*)(ebx + ecx*2 + 1)=ax;
    //-

        ax>>=16;//shr         eax,16
    //-

        *(uint16_t*)(ebx + edx + 1)=ax;
    //-
}
/****************************************************************************
 *
 *  ROUTINE       :     FilterVert_MMX
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Applies a loop filter to a horizontal edge vertically
 *
 *  SPECIAL NOTES :
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
extern "C" void FilterVert_MMX(unsigned char * PixelPtr, ogg_int32_t LineLength, ogg_int32_t *BoundingValuePtr)
{
    ogg_int32_t ms = -LineLength;
    ogg_int32_t ms2 = ms + ms;

    /* A somewhat optimized MMX version of the top edge filter.*/
    /* Originally written for Tim's VP30 code. */
        unsigned char         *eax=(unsigned char*)BoundingValuePtr;
    //-

        unsigned char *ebx=(unsigned char*)PixelPtr;
        int         ecx=ms;                    //negative stride
        __m64 mm0,mm1,mm2,mm3,mm4,mm5,mm6,mm7;
        movd        (mm1,ebx + 0               );//p0
        pxor        (mm4,mm4);

        movd        (mm0,ebx + ecx             );//get row above -- pms
        punpcklbw   (mm1,mm4                     );//convert to words

        int         edx=LineLength;
        punpcklbw   (mm0,mm4);

        movd        (mm6,ebx + ecx*2           );//pms2
        psubw       (mm1,mm0                     );//x = p0 - pms

        movq        (mm2,ebx + edx             );//pstride
        movq        (mm3,mm1);

        punpcklbw   (mm6,mm4);
        paddw       (mm3,mm1);

        punpcklbw   (mm2,mm4);
        paddw       (mm1,mm3);

        paddw       (mm1,eax + LFABS_OFFSET             );//x += LoopFilterAdjustBeforeShift
        psubw       (mm6,mm2);

        movq        (mm2,eax + LIMIT_OFFSET                 );//get the limit value
        paddw       (mm6,mm1);

        movd        (mm5,ebx + 4               );//p0
        psraw       (mm6,3                       );//values to be clipped

        movq        (mm1,mm6  );
        psraw       (mm6,15);

        movd        (mm7,ebx + ecx + 4         );//pms
        pxor        (mm1,mm6);

        psubsw      (mm1,mm6                     );//abs(i)
        pxor        (mm0,mm0);

        punpcklbw   (mm5,mm0);
        movq        (mm3,mm2);

        por         (mm6,eax + FOURONES_OFFSET                );//now have -1 or 1
        punpcklbw   (mm7,mm0);

        psubw       (mm3,mm1                     );//limit - abs(i)
        psubw       (mm5,mm7                     );//x = p0 - pms

        movq        (mm4,mm3);
        psraw       (mm3,15);

        movd        (mm0,ebx + ecx*2 + 4       );//pms2
        pxor        (mm4,mm3);

        movd        (mm1,ebx + edx +4          );//pstride
        psubsw      (mm4,mm3                     );//abs(limit - abs(i))

        pxor        (mm3,mm3);
        psubusw     (mm2,mm4                     );//limit - abs(limit - abs(i))

        punpcklbw   (mm0,mm3);
        movq        (mm7,mm5);

        paddw       (mm7,mm5);
        pmullw      (mm2,mm6                     );//new y -- wait 3 cycles

        punpcklbw   (mm1,mm3);
        paddw       (mm5,mm7);

        paddw       (mm5,eax + LFABS_OFFSET             );//x += LoopFilterAdjustBeforeShift
        psubw       (mm0,mm1);

        paddw       (mm0,mm5);
        pxor        (mm6,mm6     );

        movd        (mm7,ebx + 0               );//p0
        psraw       (mm0,3                       );//values to be clipped

        movd        (mm3,ebx + ecx             );//get row above -- pms
        punpcklbw   (mm7,mm6);

        psubw       (mm7,mm2                     );//pms + y
        punpcklbw   (mm3,mm6);

        paddw       (mm3,mm2                     );//p0 - y
        packuswb    (mm7,mm7                     );//clamp pms + y

        packuswb    (mm3,mm3                     );//clamp p0 - y
        movq        (mm1,mm0);

        movd        (ebx + 0,mm7               );//write p0
        psraw       (mm0,15);

        movq        (mm7,mm0                     );//save sign
        pxor        (mm1,mm0);

//
//
        movq        (mm2,eax + LIMIT_OFFSET                 );//get the limit value
//
//

        psubsw      (mm1,mm0                     );//abs(i)
        movq        (mm5,mm2);

        por         (mm7,eax + FOURONES_OFFSET                );//now have -1 or 1
        psubw       (mm5,mm1                     );//limit - abs(i)

        movq        (mm4,mm5);
        psraw       (mm5,15);

        movd        (ebx + ecx,mm3             );//write pms
        pxor        (mm4,mm5);

        psubsw      (mm4,mm5                     );//abs(limit - abs(i))
        pxor        (mm6,mm6);

        movd        (mm5,ebx + 4               );//p0
        psubusw     (mm2,mm4                     );//limit - abs(limit - abs(i))

        movd        (mm3,ebx + ecx + 4         );//pms
        pmullw      (mm2,mm7                     );//new y

        punpcklbw   (mm5,mm6);
    //-

        punpcklbw   (mm3,mm6);
    //-

        paddw       (mm3,mm2                     );//pms + y
        psubw       (mm5,mm2                     );//p0 - y

        packuswb    (mm3,mm3                     );//clamp pms + y
    //-

        packuswb    (mm5,mm5                     );//clamp p0 - y
    //-

        movd        (ebx + ecx + 4,mm3         );//write pms
//

        movd        (ebx + 4,mm5               );//write p0
}
