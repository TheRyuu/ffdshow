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
 *   Module Title :     DeblockOpt.c
 *
 *   Description  :     Optimized functions for deblocking
 *
 *****************************************************************************
 */


/****************************************************************************
 *  Header Frames
 *****************************************************************************
 */

#ifdef _MSC_VER
#pragma warning(disable:4799)
#pragma warning(disable:4731)
#endif

#include <stdio.h>
#include <stdlib.h>
#include "inttypes.h"
#include "codec_internal.h"
#include "simd.h"

/****************************************************************************
 *  Module constants.
 *****************************************************************************
 */

/****************************************************************************
 *  Explicit Imports
 *****************************************************************************
 */

extern "C" uint32_t LoopFilterLimitValuesV2[];
extern unsigned char FragDeblockingFlag[];
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
 *  ROUTINE       :     DeblockLoopFilteredBand_MMX
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Filter both horizontal and vertical edge in a band
 *
 *  SPECIAL NOTES :
 *
 *      REFERENCE         :
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
 extern "C" void DeblockLoopFilteredBand_MMX(
                                          PB_INSTANCE *pbi,
                                          uint8_t *SrcPtr,
                                          uint8_t *DesPtr,
                                          uint32_t PlaneLineStep,
                                          uint32_t FragAcross,
                                          uint32_t StartFrag,
                                          uint32_t *QuantScale
                                          )
{
        uint32_t j;
        uint32_t CurrentFrag=StartFrag;
        uint32_t QStep;
    uint32_t LoopFLimit;
        uint8_t *Src, *Des;
    uint32_t Var1, Var2;

 __m64 QStepMmx;
 __m64 FLimitMmx;
 __align16(short,Rows[80]);
 __align16(short,NewRows[64]);

 __align16(unsigned char,Variance11[8]);
 __align16(unsigned char,Variance21[8]);

 static __align16(const short ,FourFours[])= {4, 4, 4, 4};
 static __align16(const unsigned char,  Eight128c[]) = {128, 128, 128, 128,128, 128, 128, 128 };



        while(CurrentFrag < StartFrag + FragAcross )
        {

                Src=SrcPtr+8*(CurrentFrag-StartFrag);
                Des=DesPtr+8*(CurrentFrag-StartFrag);


                QStep = QuantScale[ pbi->FragQIndex[CurrentFrag+FragAcross]];


                        /* Calculate the FLimit and store FLimit and QStep */
                        /* Copy the data to the intermediate buffer */
                        uint32_t eax1=QStep;
                        int edx=0;;/* clear edx */
                        __m64 mm5,mm6,mm7,mm0,mm1,mm4,mm2,mm3;
                        uint32_t ecx=PlaneLineStep              ;/* ecx = Pitch */
                        movd            (mm5,   eax1);

                        uint8_t *eax=     Src;/* eax = Src */
                        punpcklwd       (mm5,   mm5                                     );

                        uint8_t *esi=   (uint8_t*)NewRows;/* esi = NewRows */
                        punpckldq       (mm5,   mm5);

            edx-=       ecx                                     ;/* edx = - Pitch */
            movq        (mm6,    mm5                 );/*  Q Q Q Q */

            paddw       (mm6,    mm5                 );
            paddw       (mm6,    mm5                 );/* 3Q3Q3Q3Q */

            packuswb    (mm5,    mm5                 );/* QQQQQQQQ */
                movq            (QStepMmx,      mm5);

            psraw       (mm6,    2                   );/*  F F F F */
            packuswb    (mm6,    mm6                 );/* FFFFFFFF */

                        uint8_t *edi=   (uint8_t*)Rows;                         /* edi = Rows */
            pxor                (mm7,   mm7                                     );/* Clear mm7 */

            psubb       (mm6,    Eight128c           );/* Eight (F-128)s */

                        eax=    eax + edx * 4   ;/* eax = Src - 4*Pitch */
                        movq            (mm0,   eax + edx                       );/* mm0 = Src-5*Pitch */

                        movq            (mm1,   mm0                                     );/* mm1 = mm0 */
                        punpcklbw       (mm0,   mm7                                     );/* Lower Four -5 */

            movq        (mm4,    mm1                 );/* mm4 = Src-5*Pitch */
            movq        (       FLimitMmx, mm6            );/* FFFF FFFF */

                        movq(           mm2,    eax                             );/* mm2 = Src-4*Pitch */
                        punpckhbw(      mm1,    mm7                                     );/* Higher Four -5 */

                        movq    (       edi,    mm0                                     );/* Write Lower Four of -5 */
            movq        (mm5,    mm2                 );/* mm5 = S_4 */

            movq                (mm3,   mm2                                     );/* mm3 = S_4 */
                        movq    (       edi+8, mm1                              );/* Write Higher Four of -5 */

            movq                (mm0,   eax + ecx                       );/* mm0 = Src-3*Pitch */
            psubusb     (mm5,    mm4                 );/* S_4 - S_5 */

            psubusb     (mm4,    mm2                 );/* S_5 - S_4 */
            punpcklbw   (mm2,   mm7                                     );/* Lower Four -4 */

            por         (mm4,    mm5                 );/* abs(S_4-S_5) */
            movq                (edi+16, mm2                            );/* Write Lower -4 */

            movq        (mm6,    mm3                 );/* mm6 = S_4 */
                        punpckhbw       (mm3,   mm7                                     );/* higher Four -4 */

            movq                (edi+24, mm3                            );/* write hight -4 */
            movq                (mm1,   mm0                                     );/* mm1 = S_3 */

                        punpcklbw       (mm0,   mm7                                     );/* lower four -3 */
                        movq            (edi+32, mm0                            );/* write Lower -3 */

                        movq            (mm2,   eax + ecx *2            );/* mm2 = Src-2*Pitch */
            movq        (mm5,    mm1                 );/* mm5 = S_3 */

            psubusb     (mm5,    mm6                 );/* S_3 - S_4 */
            psubusb     (mm6,    mm1                 );/* S_4 - S_3 */

            por         (mm5,    mm6                 );/* abs(S_4-S_3) */
            movq        (mm6,    mm1                 );/* mm6 = S_3 */

                        punpckhbw       (mm1,   mm7                                     );/* higher four -3 */
                        movq            (mm3,   mm2                                     );/* mm3 = S_2 */

                        movq            (edi+40, mm1                            );/* write Higher -3 */
            paddusb      (mm4,    mm5                 );/* abs(S_5-S_4)+abs(S_4-S_3) */

            movq        (mm5,    mm2                 );/* mm5 = S_2 */
            psubusb     (mm5,    mm6                 );/* S_2 - S_3 */

            psubusb     (mm6,    mm2                 );/* S_3 - S_2 */
            por        ( mm5,    mm6                 );/* abs(S_3 - S_2) */

            movq       ( mm6,    mm2                 );/* mm6 = S_2 */

                        punpcklbw       (mm2,   mm7                                     );/* lower four -2 */
                        eax=    eax + ecx *4;/* eax = Src */

                        punpckhbw       (mm3,   mm7                                     );/* higher four -2 */

                        movq            (mm0,   eax + edx                       );/* mm2 = Src-Pitch */
                        movq            (edi+48, mm2                            );/* lower -2   */

            paddusb   (  mm4,    mm5                 );/* abs(S_5-S_4)+abs(S_4-S_3)+abs(S_3-S_2) */
            movq      (  mm5,    mm0                 );/* mm5 = S_1 */

                        movq    (       edi+56, mm3                             );/* higher -2 */
            movq        (       mm1,    mm0                                     );/* mm1 = S_1 */

            psubusb     (mm5,    mm6                 );/* S_1 - S_2 */
            psubusb     (mm6,    mm1                 );/* S_2 - S_1 */

            punpcklbw   (mm0,   mm7                                     );/* lower -1 */
            por         (mm5,    mm6                 );/* abs(S_2 - S_1) */

            movq        (       edi+64, mm0                             );/* Lower -1 */
            movq        (mm6,    mm1                 );/* mm6 = S_1 */

            punpckhbw   (mm1,   mm7                                     );/* Higher -1 */
                        movq    (       edi+72, mm1                             );/* Higher -1 */

                        movq            (mm0,   eax                             );/* mm0 = Src0 */
            paddusb    (   mm4,    mm5               );/* abs(S_5-S_4)+abs(S_4-S_3)+abs(S_3-S_2)+abs(S_2 - S_1) */

            movq       ( Variance11, mm4          );/* save the variance */

            movq       ( mm5,    FLimitMmx           );/* mm5 = FFFF FFFF */
            psubb      ( mm4,    Eight128c           );/* abs(..) - 128 */

            pcmpgtb  (   mm5,    mm4                 );/* abs(S_5-S_4)+abs(S_4-S_3)+abs(S_3-S_2)+abs(S_2 - S_1) < FLimit ? */

            movq                (mm1,   mm0                                     );/* mm1 = S0 */
                        punpcklbw(      mm0,    mm7                                     );/* lower 0 */

            movq    (    mm4,    mm1                 );/* mm4 = S0 */
                        movq(           edi+80, mm0                             );/* write lower 0 */

            psubusb     (mm4,    mm6                 );/* S0 - S_1 */
            psubusb    ( mm6,    mm1                 );/* S_1 - S0 */

                        movq(           mm0,    eax + ecx                       );/* mm0 = SrcPitch */
            movq       ( mm3,    QStepMmx            );/* mm3 = QQQQQQQQQ */

            por       (  mm4,    mm6                 );/* abs(S0 - S_1) */
            movq     (   mm6,    mm1                 );/* mm6 = S0 */

            pcmpgtb (    mm3,    mm4                 );/* abs(S0-S_1) < QStep */
                        punpckhbw(      mm1,    mm7                                     );/* higher 0 */

            movq        (mm4,    mm0                 );/* mm4 = S1 */
            pand        (mm5,    mm3                 );/* abs(S_5-S_4)+abs(S_4-S_3)+abs(S_3-S_2)+abs(S_2 - S_1) < FLimit &&
                                                       abs(S0-S_1) < QStep */

            movq                (edi+88, mm1                             );/* write higher 0 */

                        movq    (        mm1,    mm0                                     );/* mm1 = S1 */
            psubusb   (  mm4,    mm6                 );/* S1 - S0 */

                        punpcklbw  (     mm0,    mm7                                     );/* lower 1 */
            psubusb    ( mm6,    mm1                 );/* S0 - S1 */

            movq       (         edi+96, mm0                             );/* write lower 1 */
            por        ( mm4,    mm6                 );/* mm4 = abs(S1-S0) */

                        movq   (         mm2,    eax + ecx *2      );/* mm2 = Src2*Pitch */
            movq       ( mm6,    mm1                 );/* mm6 = S1 */

            eax=    eax + ecx *4;/* eax = Src + 4 * Pitch  */
            punpckhbw  ( mm1,    mm7                                     );/* higher 1 */


                        movq  (          mm0,    mm2                                     );/* mm0 = S2 */
                        movq  (          edi+104, mm1                            );/* wirte higher 1 */


            movq       ( mm3,    mm0                 );/* mm3 = S2 */
                       movq   (         mm1,    eax + edx               );/* mm4 = Src3*pitch */

            punpcklbw  ( mm2,    mm7                                     );/* lower 2 */
            psubusb    ( mm3,    mm6                 );/* S2 - S1 */

            psubusb    ( mm6,    mm0                 );/* S1 - S2 */
            por (        mm3,    mm6                 );/* abs(S1-S2) */

            movq(               edi+112, mm2                            );/* write lower 2 */
            movq        (mm6,    mm0                 );/* mm6 = S2 */

                        punpckhbw(      mm0,    mm7                                     );/* higher 2 */
            paddusb    (   mm4,    mm3                 );/* abs(S0-S1)+abs(S1-S2) */

            movq       ( mm2,    mm1                 );/* mm2 = S3 */
            movq       ( mm3,    mm1                 );/* mm3 = S3 */

                        movq    (       edi+120, mm0                            );/* write higher 2 */
                        punpcklbw(      mm1,    mm7                                     );/* Low 3      */

                        movq    (       mm0,    eax                             );/* mm0 = Src4*pitch */
            psubusb  (   mm3,    mm6                 );/* S3 - S2 */

            psubusb  (   mm6,    mm2                 );/* S2 - S3 */
            por      (   mm3,    mm6                 );/* abs(S2-S3) */

            movq(               edi+128, mm1                            );/* low 3 */
            movq      (  mm6,    mm2                 );/* mm6 = S3 */

                        punpckhbw(      mm2,    mm7                                     );/* high 3 */
                        paddusb     (  mm4,    mm3                 );/* abs(S0-S1)+abs(S1-S2)+abs(S2-S3) */


                        movq(           mm1,    mm0                                     );/* mm1 = S4 */
            movq (       mm3,    mm0                 );/* mm3 = S4 */

            movq(               edi+136, mm2                            );/* high 3 */
            punpcklbw(  mm0,    mm7                                     );/* low 4 */

            psubusb (    mm3,    mm6                 );/* S4 - S3 */
                        movq(           edi+144, mm0                            );/* low 4 */

            psubusb    ( mm6,    mm1                 );/* S3 - S4 */
            por        ( mm3,    mm6                 );/* abs(S3-S4) */

            punpckhbw(  mm1,    mm7                                     );/* high 4 */
                        paddusb  (   mm4,    mm3                 );/* abs((S0-S1)+abs(S1-S2)+abs(S2-S3)+abs(S3-S4) */

            movq     (   Variance21, mm4          );/* save the variance */

            movq     (   mm6,    FLimitMmx           );/* mm6 = FFFFFFFFF */
                        psubb   (     mm4,    Eight128c           );/* abs(..) - 128 */

            movq        (       edi+152, mm1                            );/* high 4 */

                pcmpgtb  (   mm6,    mm4                 );/* abs((S0-S1)+abs(S1-S2)+abs(S2-S3)+abs(S3-S4)<FLimit? */
            pand        (mm6,    mm5                 );/* Flag */

                        /* done with copying everything to intermediate buffer */
            /* mm7 = 0, mm6 = Flag */
            movq       ( mm0,    mm6);
            movq       ( mm7,    mm6 );

            punpckhbw  ( mm0,    mm6);
            punpcklbw  ( mm7,    mm6);

                        /* mm0 and mm7 now are in use  */
            /* Let's do the filtering now */
            movq              (  mm3,    edi                         );/* mm3 = -5 */
            movq              (  mm2,    edi+144                 );/* mm2 = 4 */

            movq              (  mm1,    mm3                                     );/* x0 = -4 */
                        paddw (          mm3,    mm3                                     );/* mm3 = x0 + x0 */

                        movq  (          mm4,    edi+16                  );/* mm4 = x1 */
                        paddw (          mm3,    mm1                                     );/* mm3 = x0+x0+x0*/

                        paddw (          mm3,    edi+32                  );/* mm3 = x0+x0+x0+x2 */
                        paddw (          mm4,    edi+48                  );/* mm4 = x1+x3 */

                        paddw (          mm3,    edi+64                  );/* mm3 = x4 */
                        paddw (          mm4,    FourFours                       );/* mm4 = x1+x3+4 */

                        paddw  (         mm3,    mm4                                     );/* mm3 = x0+x0+x0+x1+x2+x3+x4+4 */

            /* Des-4*Pitch = ((sum + x1) >> 3; */

                        movq   (         mm4,    mm3                                     );/* mm4 = mm3 */
                        movq   (         mm5,    edi+16                  );/* mm5 = x1 */

            paddw              ( mm4,    mm5                                     );/* mm4 = sum+x1 */
                        psraw  (         mm4,    3                                       );/* mm4 >>=4 */

            psubw              ( mm4,    mm5                                     );/* New Value - old Value */
                        pand   (         mm4,    mm7                                     );/* And the flag */

                        paddw   (        mm4,    mm5                                     );/* add the old value back */
                        movq    (        esi,    mm4                                     );/* Write new x1 */

                        /* sum += x5 -x0 */
                        /* Des-3*Pitch=(sum+x2)>>3 */

                        movq    (        mm5,    edi+32                  );/* mm5= x2 */
                        psubw   (        mm3,    mm1                                     );/* sum=sum-x0 */

                        paddw   (        mm3,    edi+80                  );/* sum=sum+x5 */
                        movq    (        mm4,    mm5                                     );/* copy sum */

                        paddw   (        mm4,    mm3                                     );/* mm4=sum+x2 */
                        psraw   (        mm4,    3                                       );/* mm4=(sum+x2)>>3 */

                        psubw   (        mm4,    mm5                                     );/* new value - old value      */
                        pand    (        mm4,    mm7                                     );/* And the flag */

                        paddw   (        mm4,    mm5                                     );/* add the old value back */
                        movq    (        esi+16, mm4                             );/* write new x2 */

                        /* sum += x6 - x0 */
                        /* Des-2*Pitch=(sum+x3)>>3 */

                        movq  (          mm5,    edi+48                  );/* mm5= x3 */
                        psubw (          mm3,    mm1                                     );/* sum=sum-x0 */

                        paddw (          mm3,    edi+96                  );/* sum=sum+x6 */
                        movq  (          mm4,    mm5                                     );/* copy x3 */

                        paddw (          mm4,    mm3                                     );/* mm4=sum+x3 */
                        psraw (          mm4,    3                                       );/* mm4=(sum+x3)>>3 */

            psubw             (  mm4,    mm5                                     );/* new value - old value      */
                        pand  (          mm4,    mm7                                     );/* And the flag */

            paddw             (  mm4,    mm5                                     );/* add the old value back */
                        movq  (          esi+32, mm4                             );/* write new x3 */

                        /* sum += x7 - x0 */
                        /* Des-Pitch=(sum+x4)>>3 */

                        movq     (       mm5,    edi+64                  );/* mm5 = x4 */
                        psubw    (       mm3,    mm1                                     );/* sum = sum-x0 */

                        paddw    (       mm3,    edi+112                 );/* sum = sum+x7 */
                        movq     (       mm4,    mm5                                     );/* mm4 = x4 */

                        paddw    (       mm4,    mm3                                     );/* mm4 = sum + x4 */
            psraw       (        mm4,    3                                       );/* >>=4 */

            psubw       (        mm4,    mm5                                     );/* -=x4 */
            pand        (        mm4,    mm7                                     );/* and flag */

            paddw       (        mm4,    mm5                                     );/* += x4 */
            movq        (        esi+48, mm4                             );/* write new x4 */

                        /* sum+= x8-x1 */
                        /* Des0=((sum+x5)>>3 */

                        movq  (          mm5,    edi+80                  );/* mm5 = x5 */
                        psubw (          mm3,    edi+16                  );/* sum -= x1 */

                        paddw (          mm3,    edi+128                 );/* sub += x8 */
                        movq  (          mm4,    mm5                                     );/* mm4 = x5 */

                        paddw (          mm4,    mm3                                     );/* mm4= sum+x5 */
            psraw             (  mm4,    3                                       );/* >>=4 */

            psubw       (        mm4,    mm5                                     );/* -=x5 */
            pand        (        mm4,    mm7                                     );/* and flag */

            paddw       (        mm4,    mm5                                     );/* += x5 */
            movq        (        esi+64, mm4                             );/* write new x5 */

                        /* sum += x9 - x2 */
                        /* DesPitch = ((sum+x6)>>3 */

                        movq (           mm5,    edi+96                  );/* mm5 = x6 */
                        psubw(           mm3,    edi+32                  );/* -= x2 */

                        paddw(           mm3,    mm2                                     );/* += x9 */
                        movq (           mm4,    mm5                                     );/* mm4 = x6 */

                        paddw(           mm4,    mm3                                     );/* mm4 = sum+x6 */
                        psraw(           mm4,    3                                       );/* >>=3 */

                        psubw(           mm4,    mm5                                     );/* -=x6 */
                        pand (           mm4,    mm7                                     );/* and flag */

            paddw            (   mm4,    mm5                                     );/* += x6 */
                        movq (           esi+80, mm4                             );/* write new x6 */

                        /* sum += x9 - x3 */
                        /* Des2*Pitch = (sum+x7)>>3 */

                        movq (           mm5,    edi+112                 );/* mm5 = x7 */
                        psubw(           mm3,    edi+48                  );/* -= x3 */

                        paddw(           mm3,    mm2                                     );/* += x9 */
                        movq (           mm4,    mm5                                     );/* mm4 = x7 */

                        paddw(           mm4,    mm3                                     );/* mm4 = sum+x7 */
                        psraw(           mm4,    3                                       );/* >>=3 */

                        psubw(           mm4,    mm5                                     );/* -=x7 */
                        pand (           mm4,    mm7                                     );/* and flag */

            paddw            (   mm4,    mm5                                     );/* += x7 */
                        movq (           esi+96, mm4                             );/* write new x7 */

                        /* sum += x9 - x4 */
                        /* Des3*Pitch = (sum+x8)>>3 */

                        movq  (          mm5,    edi+128                 );/* mm5 = x8 */
                        psubw (          mm3,    edi+64                  );/* -= x4 */

                        paddw (          mm3,    mm2                                     );/* += x9 */
                        movq  (          mm4,    mm5                                     );/* mm4 = x8 */

                        paddw (          mm4,    mm3                                     );/* mm4 = sum+x8 */
                        psraw (          mm4,    3                                       );/* >>=3 */

            psubw            (   mm4,    mm5                                     );/* -=x8 */
                        pand (           mm4,    mm7                                     );/* and flag */

            paddw            (   mm4,    mm5                                     );/* += x8 */
                        movq (           esi+112, mm4                            );/* write new x8 */

                        /* done with left four columns */
                        /* now do the righ four columns */

                        edi+=    8;                                       /* shift to right four column */
                        esi+=    8;                                       /* shift to right four column */

                        /* mm0 now are in use  */
            /* Let's do the filtering now */
            /* sum = x0 + x0 + x0 + x1 + x2 + x3 + x4 + 4; */

            movq           (     mm3,    edi                         );/* mm3 = -5 */
            movq           (     mm2,    edi+144                 );/* mm2 = 4 */

            movq              (  mm1,    mm3                                     );/* x0 = -4 */
                        paddw (          mm3,    mm3                                     );/* mm3 = x0 + x0 */

                        movq (           mm4,    edi+16                  );/* mm4 = x1 */
                        paddw(           mm3,    mm1                                     );/* mm3 = x0 + x0 + x0 */

                        paddw(           mm3,    edi+32                  );/* mm3 = x0+x0+x0+ x2 */
                        paddw(           mm4,    edi+48                  );/* mm4 = x1+x3 */

                        paddw(           mm3,    edi+64                  );/* mm3 += x4 */
                        paddw(           mm4,    FourFours                       );/* mm4 = x1 + x3 + 4 */

                        paddw(           mm3,    mm4                                     );/* mm3 = 3*x0+x1+x2+x3+x4+4 */

            /* Des-4*Pitch = (((sum + x1) >> 3; */
                        movq  (          mm4,    mm3                                     );/* mm4 = mm3 */
                        movq  (          mm5,    edi+16                  );/* mm5 = x1 */

            paddw             (  mm4,    mm5                                     );/* mm4 = sum+x1 */
                        psraw (          mm4,    3                                       );/* mm4 >>=4 */

            psubw             (  mm4,    mm5                                     );/* New Value - old Value */
                        pand  (          mm4,    mm0                                     );/* And the flag */

                        paddw (          mm4,    mm5                                     );/* add the old value back */
                        movq  (          esi,    mm4                                     );/* Write new x1 */

                        /* sum += x5 -x0 */
                        /* Des-3*Pitch=((sum+x2)>>3 */

                        movq     (       mm5,    edi+32                  );/* mm5= x2 */
                        psubw    (       mm3,    mm1                                     );/* sum=sum-x0 */

                        paddw    (       mm3,    edi+80                  );/* sum=sum+x5 */
                        movq     (       mm4,    mm5                                     );/* copy sum */

                        paddw    (       mm4,    mm3                                     );/* mm4=sum+x2 */
                        psraw    (       mm4,    3                                       );/* mm4=(sum+x2)>>3 */

                        psubw    (       mm4,    mm5                                     );/* new value - old value      */
                        pand     (       mm4,    mm0                                     );/* And the flag */

            paddw              ( mm4,    mm5                                     );/* add the old value back */
                        movq   (         esi+16, mm4                             );/* write new x2 */

                        /* sum += x6 - x0 */
                        /* Des-2*Pitch=((sum+x3)>>3 */

                        movq   (         mm5,    edi+48                  );/* mm5= x3 */
                        psubw  (         mm3,    mm1                                     );/* sum=sum-x0 */

                        paddw  (         mm3,    edi+96                  );/* sum=sum+x6 */
                        movq   (         mm4,    mm5                                     );/* copy x3 */

                        paddw  (         mm4,    mm3                                     );/* mm4=sum+x3 */
                        psraw  (         mm4,    3                                       );/* mm4=(sum+x3)>>3 */

            psubw              ( mm4,    mm5                                     );/* new value - old value      */
                        pand   (         mm4,    mm0                                     );/* And the flag */

            paddw              ( mm4,    mm5                                     );/* add the old value back */
                        movq   (         esi+32, mm4                             );/* write new x3 */

                        /* sum += x7 - x0 */
                        /* Des-Pitch=(sum+x4)>>3 */

                        movq    (        mm5,    edi+64                  );/* mm5 = x4 */
                        psubw   (        mm3,    mm1                                     );/* sum = sum-x0 */

                        paddw   (        mm3,    edi+112                 );/* sum = sum+x7 */
                        movq    (        mm4,    mm5                                     );/* mm4 = x4 */

                        paddw  (         mm4,    mm3                                     );/* mm4 = sum + x4 */
            psraw              ( mm4,    3                                       );/* >>=4 */

            psubw              ( mm4,    mm5                                     );/* -=x4 */
            pand               ( mm4,    mm0                                     );/* and flag */

            paddw              ( mm4,    mm5                                     );/* += x4 */
            movq               ( esi+48, mm4                             );/* write new x4 */

                        /* sum+= x8-x1 */
                        /* Des0=((sum+x5)>>3 */

                        movq     (       mm5,    edi+80                  );/* mm5 = x5 */
                        psubw    (       mm3,    edi+16                  );/* sum -= x1 */

                        paddw    (       mm3,    edi+128                 );/* sub += x8 */
                        movq     (       mm4,    mm5                                     );/* mm4 = x5 */

                        paddw  (         mm4,    mm3                                     );/* mm4= sum+x5 */
            psraw              ( mm4,    3                                       );/* >>=4 */

            psubw              ( mm4,    mm5                                     );/* -=x5 */
            pand               ( mm4,    mm0                                     );/* and flag */

            paddw              ( mm4,    mm5                                     );/* += x5 */
            movq               ( esi+64, mm4                             );/* write new x5 */

                        /* sum += x9 - x2 */
                        /* DesPitch = ((sum+x6)>>3 */

                        movq   (         mm5,    edi+96                  );/* mm5 = x6 */
                        psubw  (         mm3,    edi+32                  );/* -= x2 */

                        paddw  (         mm3,    mm2                                     );/* += x9 */
                        movq   (         mm4,    mm5                                     );/* mm4 = x6 */

                        paddw  (         mm4,    mm3                                     );/* mm4 = sum+x6 */
                        psraw  (         mm4,    3                                       );/* >>=3 */

                        psubw  (         mm4,    mm5                                     );/* -=x6 */
                        pand   (         mm4,    mm0                                     );/* and flag */

            paddw              ( mm4,    mm5                                     );/* += x6 */
                        movq   (         esi+80, mm4                             );/* write new x6 */

                        /* sum += x9 - x3 */
                        /* Des2*Pitch = (sum+x7)>>3 */

                        movq     (       mm5,    edi+112                 );/* mm5 = x7 */
                        psubw    (       mm3,    edi+48                  );/* -= x3 */

                        paddw    (       mm3,    mm2                                     );/* += x9 */
                        movq     (       mm4,    mm5                                     );/* mm4 = x7 */

                        paddw    (       mm4,    mm3                                     );/* mm4 = sum+x7 */
                        psraw    (       mm4,    3                                       );/* >>=3 */

                        psubw    (       mm4,    mm5                                     );/* -=x7 */
                        pand     (       mm4,    mm0                                     );/* and flag */

            paddw             (  mm4,    mm5                                     );/* += x7 */
                        movq  (          esi+96, mm4                             );/* write new x7 */

                        /* sum += x9 - x4 */
                        /* Des3*Pitch = ((sum+x8)>>3 */

                        movq     (       mm5,    edi+128                 );/* mm5 = x8 */
                        psubw    (       mm3,    edi+64                  );/* -= x4 */

                        paddw    (       mm3,    mm2                                     );/* += x9 */
                        movq     (       mm4,    mm5                                     );/* mm4 = x8 */

                        paddw    (       mm4,    mm3                                     );/* mm4 = sum+x8 */
                        psraw    (       mm4,    3                                       );/* >>=3 */

            psubw            (   mm4,    mm5                                     );/* -=x8 */
                        pand (           mm4,    mm0                                     );/* and flag */

            paddw            (   mm4,    mm5                                     );/* += x8 */
                        movq (           esi+112, mm4                            );/* write new x8 */


                        /* done with right four column */
                        edi+=    8;/* shift edi to point x1 */
                        esi-=    8;/* shift esi back to x1 */

                        uint8_t*ebp=Des;/* the destination */
                        ebp= ebp + edx *4 ;/* point to des-4*Pitch */

                        movq      (      mm0, esi);
                        packuswb  (      mm0, esi + 8);

                        movq      (      ebp, mm0                                        );/* write des-4*Pitch */

                        movq      (      mm1, esi + 16);
                        packuswb  (      mm1, esi + 24);

                        movq      (      ebp+ecx , mm1                           );/* write des-3*Pitch */

                        movq      (      mm2, esi + 32);
                        packuswb  (      mm2, esi + 40);

                        movq      (      ebp+ecx*2 , mm2                 );/* write des-2*Pitch */

                        movq      (      mm3, esi + 48 );
                        packuswb  (      mm3, esi + 56 );

                        ebp= ebp+ecx*4;/* point to des0 */
                        movq   (         ebp+edx, mm3                            );/* write des-Pitch */

                        movq      (      mm0, esi + 64   );
                        packuswb  (      mm0, esi + 72   );

                        movq      (      ebp , mm0                                       );/* write des0 */

                        movq      (      mm1, esi + 80     );
                        packuswb  (      mm1, esi + 88     );

                        movq      (      ebp+ecx, mm1                            );/* write desPitch */

                        movq      (      mm2, esi + 96      );
                        packuswb  (      mm2, esi + 104     );

                        movq      (      ebp+ecx*2, mm2                  );/* write des2*Pitch */

                        movq      (      mm3, esi + 112     );
                        packuswb  (      mm3, esi + 120     );

                        ebp= ebp+ecx*2;/* point to des4*Pitch */
                        movq      (      ebp+ecx, mm3                            );/* write des3*Pitch */


                Var1 = Variance11[0]+ Variance11[1]+Variance11[2]+Variance11[3];
                Var1 += Variance11[4]+ Variance11[5]+Variance11[6]+Variance11[7];
                pbi->FragmentVariances[CurrentFrag] += Var1;

                Var2 = Variance21[0]+ Variance21[1]+Variance21[2]+Variance21[3];
                Var2 += Variance21[4]+ Variance21[5]+Variance21[6]+Variance21[7];
                pbi->FragmentVariances[CurrentFrag + FragAcross] += Var2;

        if(CurrentFrag==StartFrag)
                        CurrentFrag++;
                else
                {

                        Des=DesPtr-8*PlaneLineStep+8*(CurrentFrag-StartFrag);
                        Src=Des;

                        QStep = QuantScale[pbi->FragQIndex[CurrentFrag]];


                        for( j=0; j<8;j++)
                        {
                                Rows[j] = (short) (Src[-5+j*PlaneLineStep]);
                                Rows[72+j] = (short)(Src[4+j*PlaneLineStep]);
                        }

                                /* Calculate the FLimit and store FLimit and QStep */
                                eax1=    QStep;                           /* get QStep */
                                movd         (   mm0,    eax1                                    );/* mm0 = 0, 0, 0, Q */

                                punpcklwd    (   mm0,    mm0                                     );/* mm0 = 0, 0, Q, Q */
                                punpckldq    (   mm0,    mm0                                     );/* mm0 = Q, Q, Q, Q */

                movq      (  mm1,    mm0                 );/* mm1 = Q, Q, Q, Q */
                paddw     (  mm1,    mm0);


               paddw      (  mm1,    mm0);
               packuswb   (  mm0,    mm0);

                movq        (    QStepMmx,       mm0                             );/* write the Q step */
                                psraw       (    mm1,    2                                       );/* mm1 = FLimit */

                packuswb  (  mm1,    mm1                 );/* mm1 = FFFF FFFF */
                psubb     (  mm1,    Eight128c           );/* F-128 */

                movq      (      FLimitMmx, mm1                        );/* Save FLimit */

                                /* setup the pointers to data */

                                eax=    Src;/* eax = Src */
                                edx=0;/* clear edx */

                                eax-=    4;/* eax = Src-4 */
                                esi=    (uint8_t*)NewRows;/* esi = NewRows */
                                edi=    (uint8_t*)Rows;/* edi = Rows */

                                ecx=    PlaneLineStep;/* ecx = Pitch */
                                edx-=    ecx;/* edx = -Pitch */

                                /* Get the data to the intermediate buffer */

                                movq    (        mm0,    eax                           );/* mm0 = 07 06 05 04 03 02 01 00 */
                                movq    (        mm1,    eax+ecx                       );/* mm1 = 17 16 15 14 13 12 11 10 */

                                movq   (         mm2,    eax+ecx*2                     );/* mm2 = 27 26 25 24 23 22 21 20 */
                                eax=    eax+ecx*4;/* Go down four Rows */

                                movq        (    mm3,    eax+edx                       );/* mm3 = 37 36 35 34 33 32 31 30 */
                                movq        (    mm4,    mm0                                     );/* mm4 = 07 06 05 04 03 02 01 00 */

                                punpcklbw   (    mm0,    mm1                                     );/* mm0 = 13 03 12 02 11 01 10 00 */
                                punpckhbw   (    mm4,    mm1                                     );/* mm4 = 17 07 16 06 15 05 14 04 */

                                movq        (    mm5,    mm2                                     );/* mm5 = 27 26 25 24 23 22 21 20 */
                                punpcklbw   (    mm2,    mm3                                     );/* mm2 = 33 23 32 22 31 21 30 20 */

                                punpckhbw   (    mm5,    mm3                                     );/* mm5 = 37 27 36 26 35 25 34 24 */
                                movq        (    mm1,    mm0                                     );/* mm1 = 13 03 12 02 11 01 10 00 */

                                punpcklwd   (    mm0,    mm2                                     );/* mm0 = 31 21 11 01 30 20 10 00 */
                                punpckhwd   (    mm1,    mm2                                     );/* mm1 = 33 23 13 03 32 22 12 02 */

                                movq        (    mm2,    mm4                                     );/* mm2 = 17 07 16 06 15 05 14 04 */
                                punpckhwd   (    mm4,    mm5                                     );/* mm4 = 37 27 17 07 36 26 16 06 */

                                punpcklwd   (    mm2,    mm5                                     );/* mm2 = 35 25 15 05 34 24 14 04 */
                                pxor        (    mm7,    mm7                                     );/* clear mm7 */

                                movq        (    mm5,    mm0                                     );/* make a copy */
                                punpcklbw   (    mm0,    mm7                                     );/* mm0 = 30 20 10 00 */

                                movq        (    edi+16, mm0                           );/* write 00 10 20 30 */

                                punpckhbw   (    mm5,    mm7                                     );/* mm5 = 31 21 11 01 */

                                movq        (    mm0,    mm1                                     );/* mm0 =33 23 13 03 32 22 12 02 */
                                movq        (    edi+32, mm5                           );/* write 01 11 21 31 */

                                punpcklbw   (    mm1,    mm7                                     );/* mm1 = 32 22 12 02 */
                                punpckhbw   (    mm0,    mm7                                     );/* mm0 = 33 23 12 03 */

                                movq        (    edi+48, mm1                           );/* write 02 12 22 32 */
                                movq        (    mm3,    mm2                                     );/* mm3 = 35 25 15 05 34 24 14 04 */

                                movq        (    mm5,    mm4                                     );/* mm5 = 37 27 17 07 36 26 16 06 */
                                movq        (    edi+64, mm0                           );/* write 03 13 23 33 */


                                punpcklbw   (    mm2,    mm7                                     );/* mm2 = 34 24 14 04 */
                                punpckhbw   (    mm3,    mm7                                     );/* mm3 = 35 25 15 05 */

                                movq        (    edi+80, mm2                           );/* write 04 14 24 34 */
                                punpcklbw   (    mm4,    mm7                                     );/* mm4 = 36 26 16 06 */

                                punpckhbw   (    mm5,    mm7                                     );/* mm5 = 37 27 17 07 */
                                movq        (    edi+96, mm3                           );/* write 05 15 25 35 */

                                movq        (    mm0,    eax                           );/* mm0 = 47 46 45 44 43 42 41 40 */
                                movq        (    mm1,    eax + ecx             );/* mm1 = 57 56 55 54 53 52 51 50 */

                                movq        (    edi+112, mm4                          );/* write 06 16 26 37 */
                                movq        (    mm2,    eax+ecx*2                     );/* mm2 = 67 66 65 64 63 62 61 60 */

                                eax=    eax+ ecx*4;/* Go down four rows */
                                movq          (  edi+128, mm5                          );/* write 07 17 27 37 */

                                movq          (  mm4,    mm0                                     );/* mm4 = 47 46 45 44 43 42 41 40 */
                                movq          (  mm3,    eax+edx                       );/* mm3 = 77 76 75 74 73 72 71 70 */

                                punpcklbw     (  mm0,    mm1                                     );/* mm0 = 53 43 52 42 51 41 50 40 */
                                punpckhbw     (  mm4,    mm1                                     );/* mm4 = 57 57 56 46 55 45 54 44 */

                                movq          (  mm5,    mm2                                     );/* mm5 = 67 66 65 64 63 62 61 60 */
                                punpcklbw     (  mm2,    mm3                                     );/* mm2 = 73 63 72 62 71 61 70 60 */

                                punpckhbw     (  mm5,    mm3                                     );/* mm5 = 77 67 76 66 75 65 74 64 */
                                movq          (  mm1,    mm0                                     );/* mm1 = 53 43 52 42 51 41 50 40 */

                                punpcklwd     (  mm0,    mm2                                     );/* mm0 = 71 61 51 41 70 60 50 40 */
                                punpckhwd     (  mm1,    mm2                                     );/* mm1 = 73 63 53 43 72 62 52 42 */

                                movq          (  mm2,    mm4                                     );/* mm2 = 57 57 56 46 55 45 54 44 */
                                punpckhwd     (  mm4,    mm5                                     );/* mm4 = 77 67 57 47 76 66 56 46 */

                                punpcklwd     (  mm2,    mm5                                     );/* mm2 = 75 65 55 45 74 64 54 44 */

                                movq          (  mm5,    mm0                                     );/* make a copy */
                                punpcklbw     (  mm0,    mm7                                     );/* mm0 = 70 60 50 40 */

                                movq          (  edi+24, mm0                           );/* write 40 50 60 70 */
                                punpckhbw     (  mm5,    mm7                                     );/* mm5 = 71 61 51 41 */

                                movq          (  mm0,    mm1                                     );/* mm0 = 73 63 53 43 72 62 52 42 */
                                movq          (  edi+40, mm5                           );/* write 41 51 61 71 */

                                punpcklbw     (  mm1,    mm7                                     );/* mm1 = 72 62 52 42 */
                                punpckhbw     (  mm0,    mm7                                     );/* mm0 = 73 63 53 43 */

                                movq          (  edi+56, mm1                           );/* write 42 52 62 72 */
                                movq          (  mm3,    mm2                                     );/* mm3 = 75 65 55 45 74 64 54 44 */

                                movq          (  mm5,    mm4                                     );/* mm5 = 77 67 57 47 76 66 56 46 */
                                movq          (  edi+72, mm0                           );/* write 43 53 63 73 */

                                punpcklbw     (  mm2,    mm7                                     );/* mm2 = 74 64 54 44 */
                                punpckhbw     (  mm3,    mm7                                     );/* mm3 = 75 65 55 45 */

                                movq          (  edi+88, mm2                           );/* write 44 54 64 74 */
                                punpcklbw     (  mm4,    mm7                                     );/* mm4 = 76 66 56 46 */

                                punpckhbw     (  mm5,    mm7                                     );/* mm5 = 77 67 57 47 */
                                movq          (  edi+104, mm3                          );/* write 45 55 65 75 */

                                movq          (  edi+120, mm4                          );/* write 46 56 66 76 */
                                movq          (  edi+136, mm5                          );/* write 47 57 67 77 */


                            /* Now, compute the variances for Pixel  1-4 and 5-8 */


                movq        (mm0,    edi               );/* S_5 */
                movq        (mm1,    edi+16            );/* S_4 */

                movq        (mm2,    edi+32            );/* S_3 */
                packuswb    (mm0,    edi+8     );

                packuswb    (mm1,    edi+24);
                packuswb    (mm2,    edi+40);

                movq        (mm3,    edi+48            );/* S_2 */
                movq        (mm4,    edi+64            );/* S_1 */

                packuswb   ( mm3,    edi+56);
                packuswb   ( mm4,    edi+72);

                movq       ( mm5,    mm1                 );/* S_4 */
                movq       ( mm6,    mm2                 );/* S_3 */

                psubusb    ( mm5,    mm0                 );/* S_4 - S_5 */
                psubusb    ( mm0,    mm1                 );/* S_5 - S_4 */

                por        ( mm0,    mm5                 );/* abs(S_5-S_4) */
                psubusb    ( mm6,    mm1                 );/* S_3 - S_4 */

                psubusb    ( mm1,    mm2                 );/* S_4 - S_3 */
                movq       ( mm5,    mm3                 );/* S_2 */

                por        ( mm1,    mm6                 );/* abs(S_4-S_3) */
                psubusb    ( mm5,    mm2                 );/* S_2 - S_3 */

                psubusb    ( mm2,    mm3                 );/* S_3 - S_2 */
                movq       ( mm6,    mm4                 );/* S_1 */

                por        ( mm2,    mm5                 );/* abs(S_3-S_2) */
                psubusb    ( mm6,    mm3                 );/* S_1 - S_2 */

                psubusb    ( mm3,    mm4                 );/* S_2 - S_1 */
                por        ( mm3,    mm6                 );/* abs(S_2-S_1) */

                paddusb    (  mm0,    mm1                 );/* abs(S_5-S_4)+abs(S_4-S_3) */
                paddusb    (  mm2,    mm3                 );/* abs(S_3-S_2)+abs(S_2-S_1) */

                movq       ( mm7,    FLimitMmx              );/* FFFFF FFFF */
                paddusb    (  mm0,    mm2                 );/* abs(S_5-S_4)+abs(S_4-S_3)+abs(S_3-S_2)+abs(S_2-S_1) */

                movq       ( Variance11, mm0           );/* Save the variance */

                movq       ( mm6,    mm4                 );/* S_1 */
                psubb      ( mm0,    Eight128c           );/* abs(..) - 128 */
                pcmpgtb    ( mm7,    mm0                 );/* abs(S_5-S_4)+abs(S_4-S_3)+abs(S_3-S_2)+abs(S_2-S_1)<? */

                                movq (       mm5,    edi+80            );/* S0 */
                movq       ( mm1,    edi+96            );/* S1 */

                movq       ( mm2,    edi+112           );/* S2 */
                packuswb   ( mm5,    edi+88     );

                packuswb   ( mm1,    edi+104);
                packuswb   ( mm2,    edi+120);

                movq       ( mm3,    edi+128           );/* S3 */
                movq       ( mm4,    edi+144           );/* S4 */

                packuswb   ( mm3,    edi+136);
                packuswb   ( mm4,    edi+152);

                movq       ( mm0,    mm5                 );/* S0 */
                psubusb    ( mm5,    mm6                 );/* S0-S_1 */

                psubusb    ( mm6,    mm0                 );/* S_1-S0 */
                por        ( mm5,    mm6                 );/* abs(S_1-S0) */

                movq       ( mm6,    QStepMmx            );/* QQQQ QQQQ */
                pcmpgtb    ( mm6,    mm5                 );/* abs(S_1-S0)<QStep? */

                movq       ( mm5,    mm1                 );/* S1 */
                pand       ( mm7,    mm6                 );/* abs(S_1-S0)<QStep &&
                                                            abs(S_5-S_4)+abs(S_4-S_3)+abs(S_3-S_2)+abs(S_2-S_1)<FLimit? */
                movq       ( mm6,    mm2                 );/* S2 */
                psubusb    ( mm5,    mm0                 );/* S1 - S0 */

                psubusb    ( mm0,    mm1                 );/* S0 - S1*/

                por        ( mm0,    mm5                 );/* abs(S0-S1) */
                psubusb    ( mm6,    mm1                 );/* S2 - S1 */

                psubusb    ( mm1,    mm2                 );/* S1 - S2*/
                movq       ( mm5,    mm3                 );/* S3 */

                por        ( mm1,    mm6                 );/* abs(S1-S2) */
                psubusb    ( mm5,    mm2                 );/* S3 - S2 */

                psubusb    ( mm2,    mm3                 );/* S2 - S3 */
                movq       ( mm6,    mm4                 );/* S4 */

                por        ( mm2,    mm5                 );/* abs(S2-S3) */
                psubusb    ( mm6,    mm3                 );/* S4 - S3 */

                psubusb    ( mm3,    mm4                 );/* S3 - S4 */
                por        ( mm3,    mm6                 );/* abs(S3-S4) */

                paddusb    (  mm0,    mm1                 );/* abs(S0-S1)+abs(S1-S2) */
                paddusb    (  mm2,    mm3                 );/* abs(S2-S3)+abs(S3-S4) */

                movq       ( mm6,    FLimitMmx           );/* FFFFF FFFF */
                paddusb    (  mm0,    mm2                 );/* abs(S0-S1)+abs(S1-S2)+abs(S2-S3)+abs(S3-S4) */

                movq       ( Variance21, mm0           );/* Save the variance */

                psubb      (  mm0,    Eight128c            );/* abs(..) - 128 */
                pcmpgtb    ( mm6,    mm0                 );/* abs(S0-S1)+abs(S1-S2)+abs(S2-S3)+abs(S3-S4)<FLimit */
                pand       ( mm6,    mm7                 );/* Flag */

                movq       ( mm0,    mm6);
                movq       ( mm7,    mm6 );

                punpckhbw  ( mm0,    mm6);
                punpcklbw  ( mm7,    mm6);

                                /* mm0 and mm7 now are in use  */

                /* Let's do the filtering now */
                /* sum = x0 + x0 + x0 + x1 + x2 + x3 + x4 + 4; */

                movq     (       mm3,    edi                       );/* mm3 = -5 */
                movq     (       mm2,    edi+144                       );/* mm2 = 4 */

                movq     (       mm1,    mm3                                     );/* x0 = -4 */
                paddw    (       mm3,    mm3                                     );/* mm3 = x0 + x0 */

                movq     (       mm4,    edi+16                        );/* mm4 = x1 */
                paddw    (       mm3,    mm1                                     );/* mm3 = x0 + x0 + x0 */

                paddw    (       mm3,    edi+32                        );/* mm3 = x0+x0+x0+ x2 */
                paddw    (       mm4,    edi+48                        );/* mm4 = x1+x3 */

                paddw    (       mm3,    edi+64                        );/* mm3 += x4 */
                paddw    (       mm4,    FourFours                       );/* mm4 = x1 + x3 + 4 */

                paddw    (       mm3,    mm4                                     );/* mm3 = 3*x0+x1+x2+x3+x4+4 */

                /* Des-4*Pitch = (((sum + x1) >> 3; */

                movq   (         mm4,    mm3                                     );/* mm4 = mm3 */
                movq   (         mm5,    edi+16                        );/* mm5 = x1 */

                paddw  (         mm4,    mm5                                     );/* mm4 = sum+x1 */
                psraw  (         mm4,    3                                       );/* mm4 >>=3 */

                psubw  (         mm4,    mm5                                     );/* New Value - old Value */
                pand   (         mm4,    mm7                                     );/* And the flag */

                paddw  (         mm4,    mm5                                     );/* add the old value back */
                movq   (         esi,  mm4                                     );/* Write new x1 */

                /* sum += x5 -x0 */
                /* Des-3*Pitch=((sum+x2)>>3 */

                movq    (        mm5,    edi+32                        );/* mm5= x2 */
                psubw   (        mm3,    mm1                                     );/* sum=sum-x0 */

                paddw   (        mm3,    edi+80                        );/* sum=sum+x5 */
                movq    (        mm4,    mm5                                     );/* copy sum */

                paddw   (        mm4,    mm3                                     );/* mm4=sum+x2 */
                psraw   (        mm4,    3                                       );/* mm4=(sum+x2)>>3 */
                psubw   (        mm4,    mm5                                     );/* new value - old value        */

                pand    (        mm4,    mm7                                     );/* And the flag */
                paddw   (        mm4,    mm5                                     );/* add the old value back */

                movq    (        esi+16, mm4                           );/* write new x2 */

                /* sum += x6 - x0 */
                /* Des-2*Pitch=((sum+x3)>>3 */

                movq  (          mm5,    edi+48                        );/* mm5= x3 */
                psubw (          mm3,    mm1                                     );/* sum=sum-x0 */

                paddw (          mm3,    edi+96                        );/* sum=sum+x6 */
                movq  (          mm4,    mm5                                     );/* copy x3 */

                paddw (          mm4,    mm3                                     );/* mm4=sum+x3 */
                psraw (          mm4,    3                                       );/* mm4=(sum+x3)>>3 */

                psubw (          mm4,    mm5                                     );/* new value - old value        */
                pand  (          mm4,    mm7                                     );/* And the flag */

                paddw (          mm4,    mm5                                     );/* add the old value back */
                movq  (          esi+32, mm4                           );/* write new x3 */

                /* sum += x7 - x0 */
                /* Des-Pitch=(sum+x4)>>3 */

                movq      (      mm5,    edi+64                        );/* mm5 = x4 */
                psubw     (      mm3,    mm1                                     );/* sum = sum-x0 */

                paddw     (      mm3,    edi+112                       );/* sum = sum+x7 */
                movq      (      mm4,    mm5                                     );/* mm4 = x4 */

                paddw     (      mm4,    mm3                                     );/* mm4 = sum + x4 */
                psraw     (      mm4,    3                                       );/* >>=4 */

                psubw     (      mm4,    mm5                                     );/* -=x4 */
                pand      (      mm4,    mm7                                     );/* and flag */

                paddw     (      mm4,    mm5                                     );/* += x4 */
                movq      (      esi+48, mm4                           );/* write new x4 */

                /* sum+= x8-x1 */
                /* Des0=((sum+x5)>>3 */

                movq       (     mm5,    edi+80                        );/* mm5 = x5 */
                psubw      (     mm3,    edi+16                        );/* sum -= x1 */

                paddw      (     mm3,    edi+128                       );/* sub += x8 */
                movq       (     mm4,    mm5                                     );/* mm4 = x5 */

                paddw      (     mm4,    mm3                                     );/* mm4= sum+x5 */
                psraw      (     mm4,    3                                       );/* >>=4 */

                psubw      (     mm4,    mm5                                     );/* -=x5 */
                pand       (     mm4,    mm7                                     );/* and flag */

                paddw      (     mm4,    mm5                                     );/* += x5 */
                movq       (     esi+64, mm4                           );/* write new x5 */

                /* sum += x9 - x2 */
                /* DesPitch = ((sum+x6)>>3 */

                movq    (        mm5,    edi+96                        );/* mm5 = x6 */
                psubw   (        mm3,    edi+32                        );/* -= x2 */

                paddw   (        mm3,    mm2                                     );/* += x9 */
                movq    (        mm4,    mm5                                     );/* mm4 = x6 */

                paddw   (        mm4,    mm3                                     );/* mm4 = sum+x6 */
                psraw   (        mm4,    3                                       );/* >>=3 */

                psubw   (        mm4,    mm5                                     );/* -=x6 */
                pand    (        mm4,    mm7                                     );/* and flag */

                paddw   (        mm4,    mm5                                     );/* += x6 */
                movq    (        esi+80, mm4                           );/* write new x6 */

                /* sum += x9 - x3 */
                /* Des2*Pitch = (sum+x7)>>3 */

                movq     (       mm5,    edi+112                       );/* mm5 = x7 */
                psubw    (       mm3,    edi+48                        );/* -= x3 */

                paddw    (       mm3,    mm2                                     );/* += x9 */
                movq     (       mm4,    mm5                                     );/* mm4 = x7 */

                paddw    (       mm4,    mm3                                     );/* mm4 = sum+x7 */
                psraw    (       mm4,    3                                       );/* >>=3 */

                psubw    (       mm4,    mm5                                     );/* -=x7 */
                pand     (       mm4,    mm7                                     );/* and flag */

                paddw    (       mm4,    mm5                                     );/* += x7 */
                movq     (       esi+96, mm4                           );/* write new x7 */

                /* sum += x9 - x4 */
                /* Des3*Pitch = ((sum+x8)>>3 */

                movq       (     mm5,    edi+128                       );/* mm5 = x8 */
                psubw      (     mm3,    edi+64                        );/* -= x4 */

                paddw      (     mm3,    mm2                                     );/* += x9 */
                movq       (     mm4,    mm5                                     );/* mm4 = x8 */

                paddw      (     mm4,    mm3                                     );/* mm4 = sum+x8 */
                psraw      (     mm4,    3                                       );/* >>=3 */

                psubw      (     mm4,    mm5                                     );/* -=x8 */
                pand       (     mm4,    mm7                                     );/* and flag */

                paddw      (     mm4,    mm5                                     );/* += x8 */
                movq       (     esi+112, mm4                          );/* write new x8 */

                /* done with left four columns */
                /* now do the righ four columns */
                                edi+=    8;/* shift to right four column */
                                esi+=    8;/* shift to right four column */

                                /* mm0 now are in use  */
                /* Let's do the filtering now */
                /* sum = x0 + x0 + x0 + x1 + x2 + x3 + x4 + 4; */

                movq   (         mm3,    edi                       );/* mm3 = -5 */
                movq   (         mm2,    edi+144                       );/* mm2 = 4 */

                movq   (         mm1,    mm3                                     );/* x0 = -4 */
                paddw  (         mm3,    mm3                                     );/* mm3 = x0 + x0 */

                movq   (         mm4,    edi+16                        );/* mm4 = x1 */
                paddw  (         mm3,    mm1                                     );/* mm3 = x0 + x0 + x0 */

                paddw  (         mm3,    edi+32                        );/* mm3 = x0+x0+x0+ x2 */
                paddw  (         mm4,    edi+48                        );/* mm4 = x1+x3 */

                paddw  (         mm3,    edi+64                        );/* mm3 += x4 */
                paddw  (         mm4,    FourFours                       );/* mm4 = x1 + x3 + 4 */

                paddw  (         mm3,    mm4                                     );/* mm3 = 3*x0+x1+x2+x3+x4+4 */

                /* Des-4*Pitch = (((sum + x1) >> 3; */

                movq  (          mm4,    mm3                                     );/* mm4 = mm3 */
                movq  (          mm5,    edi+16                        );/* mm5 = x1 */

                paddw (          mm4,    mm5                                     );/* mm4 = sum+x1 */
                psraw (          mm4,    3                                       );/* mm4 >>=4 */

                psubw (          mm4,    mm5                                     );/* New Value - old Value */
                pand  (          mm4,    mm0                                     );/* And the flag */

                paddw (          mm4,    mm5                                     );/* add the old value back */
                movq  (          esi,  mm4                                     );/* Write new x1 */

                /* sum += x5 -x0 */
                /* Des-3*Pitch=((sum+x2)>>3 */

                movq     (       mm5,    edi+32                        );/* mm5= x2 */
                psubw    (       mm3,    mm1                                     );/* sum=sum-x0 */

                paddw    (       mm3,    edi+80                        );/* sum=sum+x5 */
                movq     (       mm4,    mm5                                     );/* copy sum */

                paddw    (       mm4,    mm3                                     );/* mm4=sum+x2 */
                psraw    (       mm4,    3                                       );/* mm4=(sum+x2)>>3 */
                psubw    (       mm4,    mm5                                     );/* new value - old value        */

                pand     (       mm4,    mm0                                     );/* And the flag */
                paddw    (       mm4,    mm5                                     );/* add the old value back */

                movq     (       esi+16, mm4                           );/* write new x2 */

                /* sum += x6 - x0 */
                /* Des-2*Pitch=((sum+x3)>>3 */

                movq         (   mm5,    edi+48                        );/* mm5= x3 */
                psubw        (   mm3,    mm1                                     );/* sum=sum-x0 */

                paddw        (   mm3,    edi+96                        );/* sum=sum+x6 */
                movq         (   mm4,    mm5                                     );/* copy x3 */

                paddw        (   mm4,    mm3                                     );/* mm4=sum+x3 */
                psraw        (   mm4,    3                                       );/* mm4=(sum+x3)>>3 */

                psubw        (   mm4,    mm5                                     );/* new value - old value        */
                pand         (   mm4,    mm0                                     );/* And the flag */

                paddw        (   mm4,    mm5                                     );/* add the old value back */
                movq         (   esi+32, mm4                           );/* write new x3 */

                /* sum += x7 - x0 */
                /* Des-Pitch=(sum+x4)>>3 */

                movq   (         mm5,    edi+64                        );/* mm5 = x4 */
                psubw  (         mm3,    mm1                                     );/* sum = sum-x0 */

                paddw  (         mm3,    edi+112                       );/* sum = sum+x7 */
                movq   (         mm4,    mm5                                     );/* mm4 = x4 */

                paddw  (         mm4,    mm3                                     );/* mm4 = sum + x4 */
                psraw  (         mm4,    3                                       );/* >>=4 */

                psubw  (         mm4,    mm5                                     );/* -=x4 */
                pand   (         mm4,    mm0                                     );/* and flag */

                paddw  (         mm4,    mm5                                     );/* += x4 */
                movq   (         esi+48, mm4                           );/* write new x4 */

                /* sum+= x8-x1 */
                /* Des0=((sum+x5)>>3 */

                movq     (       mm5,    edi+80                        );/* mm5 = x5 */
                psubw    (       mm3,    edi+16                        );/* sum -= x1 */

                paddw    (       mm3,    edi+128                       );/* sub += x8 */
                movq     (       mm4,    mm5                                     );/* mm4 = x5 */

                paddw    (       mm4,    mm3                                     );/* mm4= sum+x5 */
                psraw    (       mm4,    3                                       );/* >>=4 */

                psubw    (       mm4,    mm5                                     );/* -=x5 */
                pand     (       mm4,    mm0                                     );/* and flag */

                paddw    (       mm4,    mm5                                     );/* += x5 */
                movq     (       esi+64, mm4                           );/* write new x5 */

                /* sum += x9 - x2 */
                /* DesPitch = ((sum+x6)>>3 */

                movq      (      mm5,    edi+96                        );/* mm5 = x6 */
                psubw     (      mm3,    edi+32                        );/* -= x2 */

                paddw     (      mm3,    mm2                                     );/* += x9 */
                movq      (      mm4,    mm5                                     );/* mm4 = x6 */

                paddw     (      mm4,    mm3                                     );/* mm4 = sum+x6 */
                psraw     (      mm4,    3                                       );/* >>=3 */

                psubw     (      mm4,    mm5                                     );/* -=x6 */
                pand      (      mm4,    mm0                                     );/* and flag */

                paddw     (      mm4,    mm5                                     );/* += x6 */
                movq      (      esi+80, mm4                           );/* write new x6 */

                /* sum += x9 - x3 */
                /* Des2*Pitch = (sum+x7)>>3 */

                movq       (     mm5,    edi+112                       );/* mm5 = x7 */
                psubw      (     mm3,    edi+48                        );/* -= x3 */

                paddw      (     mm3,    mm2                                     );/* += x9 */
                movq       (     mm4,    mm5                                     );/* mm4 = x7 */

                paddw      (     mm4,    mm3                                     );/* mm4 = sum+x7 */
                psraw      (     mm4,    3                                       );/* >>=3 */

                psubw      (     mm4,    mm5                                     );/* -=x7 */
                pand       (     mm4,    mm0                                     );/* and flag */

                paddw      (     mm4,    mm5                                     );/* += x7 */
                movq       (     esi+96, mm4                           );/* write new x7 */

                /* sum += x9 - x4 */
                /* Des3*Pitch = ((sum+x8)>>3 */

                movq     (       mm5,    edi+128                       );/* mm5 = x8 */
                psubw    (       mm3,    edi+64                        );/* -= x4 */

                paddw    (       mm3,    mm2                                     );/* += x9 */
                movq     (       mm4,    mm5                                     );/* mm4 = x8 */

                paddw    (       mm4,    mm3                                     );/* mm4 = sum+x8 */
                psraw    (       mm4,    3                                       );/* >>=3 */

                psubw    (       mm4,    mm5                                     );/* -=x8 */
                pand     (       mm4,    mm0                                     );/* and flag */

                paddw    (       mm4,    mm5                                     );/* += x8 */
                movq     (       esi+112, mm4                          );/* write new x8 */

                                /* done with right four column */
                                /* transpose */
                                eax=    Des ;/* the destination */
                                edi+=    8;/* shift edi to point x1 */

                                esi-=    8;/* shift esi back to left x1 */
                                eax-=    4;

                                movq         (   mm0,    esi                           );/* mm0 = 30 20 10 00 */
                                movq         (   mm1,    esi+16                        );/* mm1 = 31 21 11 01 */

                                movq         (   mm4,    mm0                                     );/* mm4 = 30 20 10 00 */
                                punpcklwd    (   mm0,    mm1                                     );/* mm0 = 11 10 01 00 */

                                punpckhwd    (   mm4,    mm1                                     );/* mm4 = 31 30 21 20 */
                                movq         (   mm2,    esi+32                        );/* mm2 = 32 22 12 02 */

                                movq         (   mm3,    esi+48                        );/* mm3 = 33 23 13 03 */
                                movq         (   mm5,    mm2                                     );/* mm5 = 32 22 12 02 */

                                punpcklwd    (   mm2,    mm3                                     );/* mm2 = 13 12 03 02 */
                                punpckhwd    (   mm5,    mm3                                     );/* mm5 = 33 32 23 22 */

                                movq         (   mm1,    mm0                                     );/* mm1 = 11 10 01 00 */
                                punpckldq    (   mm0,    mm2                                     );/* mm0 = 03 02 01 00 */

                                movq         (   edi,  mm0                                     );/* write 00 01 02 03 */
                                punpckhdq    (   mm1,    mm2                                     );/* mm1 = 13 12 11 10 */

                                movq         (   mm0,    mm4                                     );/* mm0 = 31 30 21 20 */
                                movq         (   edi+16, mm1                           );/* write 10 11 12 13 */

                                punpckldq    (   mm0,    mm5                                     );/* mm0 = 23 22 21 20 */
                                punpckhdq    (   mm4,    mm5                                     );/* mm4 = 33 32 31 30 */

                                movq         (   mm1,    esi+64                        );/* mm1 = 34 24 14 04 */
                                movq         (   mm2,    esi+80                        );/* mm2 = 35 25 15 05 */

                                movq         (   mm5,    esi+96                        );/* mm5 = 36 26 16 06 */
                                movq         (   mm6,    esi+112                       );/* mm6 = 37 27 17 07 */

                                movq         (   mm3,    mm1                                     );/* mm3 = 34 24 14 04 */
                                movq         (   mm7,    mm5                                     );/* mm7 = 36 26 16 06 */

                                punpcklwd    (   mm1,    mm2                                     );/* mm1 = 15 14 05 04 */
                                punpckhwd    (   mm3,    mm2                                     );/* mm3 = 35 34 25 24 */

                                punpcklwd    (   mm5,    mm6                                     );/* mm5 = 17 16 07 06 */
                                punpckhwd    (   mm7,    mm6                                     );/* mm7 = 37 36 27 26 */

                                movq         (   mm2,    mm1                                     );/* mm2 = 15 14 05 04 */
                                movq         (   mm6,    mm3                                     );/* mm6 = 35 34 25 24 */

                                punpckldq    (   mm1,    mm5                                     );/* mm1 = 07 06 05 04 */
                                punpckhdq    (   mm2,    mm5                                     );/* mm2 = 17 16 15 14 */

                                punpckldq    (   mm3,    mm7                                     );/* mm3 = 27 26 25 24 */
                                punpckhdq    (   mm6,    mm7                                     );/* mm6 = 37 36 35 34 */

                                movq         (   mm5,    edi                           );/* mm5 = 03 02 01 00 */
                                packuswb     (   mm5,    mm1                                     );/* mm5 = 07 06 05 04 03 02 01 00 */

                                movq         (   eax,  mm5                                     );/* write 00 01 02 03 04 05 06 07 */
                                movq         (   mm7,    edi+16                        );/* mm7 = 13 12 11 10 */

                                packuswb     (   mm7,    mm2                                     );/* mm7 = 17 16 15 14 13 12 11 10 */
                                movq         (   eax+ecx, mm7                          );/* write 10 11 12 13 14 15 16 17 */

                                packuswb     (   mm0,    mm3                                     );/* mm0 = 27 26 25 24 23 22 21 20 */
                                packuswb     (   mm4,    mm6                                     );/* mm4 = 37 36 35 34 33 32 31 30 */

                                movq         (   eax+ecx*2, mm0                        );/* write 20 21 22 23 24 25 26 27 */
                                eax=    eax+ecx*4;/* mov forward the desPtr */

                                movq          (  eax+edx,      mm4                             );/* write 30 31 32 33 34 35 36 37 */
                                edi+= 8;/* move to right four column */
                                esi+= 8;/* move to right x1 */

                                movq      (      mm0,    esi                           );/* mm0 = 70 60 50 40 */
                                movq      (      mm1,    esi+16                        );/* mm1 = 71 61 51 41 */

                                movq      (      mm4,    mm0                                     );/* mm4 = 70 60 50 40 */
                                punpcklwd (      mm0,    mm1                                     );/* mm0 = 51 50 41 40 */

                                punpckhwd (      mm4,    mm1                                     );/* mm4 = 71 70 61 60 */
                                movq      (      mm2,    esi+32                        );/* mm2 = 72 62 52 42 */

                                movq      (      mm3,    esi+48                        );/* mm3 = 73 63 53 43 */
                                movq      (      mm5,    mm2                                     );/* mm5 = 72 62 52 42 */

                                punpcklwd (      mm2,    mm3                                     );/* mm2 = 53 52 43 42 */
                                punpckhwd (      mm5,    mm3                                     );/* mm5 = 73 72 63 62 */

                                movq      (      mm1,    mm0                                     );/* mm1 = 51 50 41 40 */
                                punpckldq (      mm0,    mm2                                     );/* mm0 = 43 42 41 40 */

                                movq      (      edi,  mm0                                     );/* write 40 41 42 43 */
                                punpckhdq (      mm1,    mm2                                     );/* mm1 = 53 52 51 50 */

                                movq      (      mm0,    mm4                                     );/* mm0 = 71 70 61 60 */
                                movq      (      edi+16, mm1                           );/* write 50 51 52 53 */

                                punpckldq (      mm0,    mm5                                     );/* mm0 = 63 62 61 60 */
                                punpckhdq (      mm4,    mm5                                     );/* mm4 = 73 72 71 70 */

                                movq      (      mm1,    esi+64                        );/* mm1 = 74 64 54 44 */
                                movq      (      mm2,    esi+80                        );/* mm2 = 75 65 55 45 */

                                movq      (      mm5,    esi+96                        );/* mm5 = 76 66 56 46 */
                                movq      (      mm6,    esi+112                       );/* mm6 = 77 67 57 47 */

                                movq      (      mm3,    mm1                                     );/* mm3 = 74 64 54 44 */
                                movq      (      mm7,    mm5                                     );/* mm7 = 76 66 56 46 */

                                punpcklwd (      mm1,    mm2                                     );/* mm1 = 55 54 45 44 */
                                punpckhwd (      mm3,    mm2                                     );/* mm3 = 75 74 65 64 */

                                punpcklwd (      mm5,    mm6                                     );/* mm5 = 57 56 47 46 */
                                punpckhwd (      mm7,    mm6                                     );/* mm7 = 77 76 67 66 */

                                movq      (      mm2,    mm1                                     );/* mm2 = 55 54 45 44 */
                                movq      (      mm6,    mm3                                     );/* mm6 = 75 74 65 64 */

                                punpckldq (      mm1,    mm5                                     );/* mm1 = 47 46 45 44 */
                                punpckhdq (      mm2,    mm5                                     );/* mm2 = 57 56 55 54 */

                                punpckldq (      mm3,    mm7                                     );/* mm3 = 67 66 65 64 */
                                punpckhdq (      mm6,    mm7                                     );/* mm6 = 77 76 75 74 */

                                movq      (      mm5,    edi                           );/* mm5 = 43 42 41 40 */
                                packuswb  (      mm5,    mm1                                     );/* mm5 = 47 46 45 44 43 42 41 40 */

                                movq      (      eax,  mm5                                     );/* write 40 41 42 43 44 45 46 47 */
                                movq      (      mm7,    edi+16                        );/* mm7 = 53 52 51 50 */

                                packuswb  (      mm7,    mm2                                     );/* mm7 = 57 56 55 54 53 52 51 50 */
                                movq      (      eax+ecx, mm7                          );/* write 50 51 52 53 54 55 56 57 */

                                packuswb  (      mm0,    mm3                                     );/* mm0 = 67 66 65 64 63 62 61 60 */
                                packuswb  (      mm4,    mm6                                     );/* mm4 = 77 76 75 74 73 72 71 70 */

                                movq      (      eax+ecx*2, mm0                        );/* write 60 61 62 63 64 65 66 67 */
                                eax=    eax+ecx*4;/* mov forward the desPtr */

                                movq    (        eax+edx,      mm4                             );/* write 70 71 72 73 74 75 76 77 */

                Var1 = Variance11[0]+ Variance11[1]+Variance11[2]+Variance11[3];
                Var1 += Variance11[4]+ Variance11[5]+Variance11[6]+Variance11[7];
                pbi->FragmentVariances[CurrentFrag-1] += Var1;

                Var2 = Variance21[0]+ Variance21[1]+Variance21[2]+Variance21[3];
                Var2 += Variance21[4]+ Variance21[5]+Variance21[6]+Variance21[7];
                pbi->FragmentVariances[CurrentFrag] += Var2;


        CurrentFrag ++;
                }//else

        }//while

}
