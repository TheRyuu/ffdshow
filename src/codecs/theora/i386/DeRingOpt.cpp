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
 *   Module Title :     DeRingingOpt.c
 *
 *   Description  :     Optimized functions for PostProcessor
 *
 *****************************************************************************
 */


#pragma warning(disable:4799)
#pragma warning(disable:4731)


/****************************************************************************
 *  Header Files
 *****************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include "inttypes.h"
#include "codec_internal.h"
#include "simd.h"

/****************************************************************************
 *  Module constants.
 *****************************************************************************
 */    
#pragma warning(disable:4305) 

/****************************************************************************
 *  Explicit Imports
 *****************************************************************************
 */              

extern "C" uint32_t SharpenModifier[];

/*******************************************************************************/


/****************************************************************************
 * 
 *  ROUTINE       :     DeRingBlockStrong_MMX()
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Filtering a block for deringing purpose
 *
 *  SPECIAL NOTES :     
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

static __align16(const unsigned short, Four128s[]) = {128, 128, 128, 128};
static __align16(const unsigned short, Four64s[] ) = { 64,  64,  64,  64};
                
static __align16(const char, eight64s [] )= { 64,64,64,64,64,64,64,64};
static __align16(const char, eight32s [] )= { 32,32,32,32,32,32,32,32};
static __align16(const char, eight127s [])= { 127, 127, 127, 127, 127, 127, 127, 127};
static __align16(const char, eight128s [])= { 128, 128, 128, 128, 128, 128, 128, 128};
static __align16(const unsigned char ,eight223s[]) = { 223,223,223,223,223,223,223,223};
static __align16(const unsigned char ,eight231s[]) = { 231,231,231,231,231,231,231,231};

extern "C" void DeringBlockStrong_MMX( 
                       const uint8_t *SrcPtr,
                       uint8_t *DstPtr,
                       const int32_t Pitch,
                       uint32_t FragQIndex,
                       const uint32_t *QuantScale)


{

        __align16(short, UDMod[72]);
        __align16(short, LRMod[128]);

        unsigned int PlaneLineStep = Pitch;
        const unsigned char * Src = SrcPtr;
        unsigned char * Des = DstPtr;
    
        short * UDPointer = UDMod;
        short * LRPointer = LRMod;
    
    uint32_t QStep = QuantScale[FragQIndex];
        int32_t Sharpen = SharpenModifier[FragQIndex];

                const uint8_t *                     esi=    Src;                                             /* Source Pointer */
                uint8_t *                     edi=    (uint8_t*)UDPointer                               ;/* UD modifier pointer */

                unsigned int                      ecx=    PlaneLineStep                   ;/* Pitch Step */
                int edx=0;


                int eax_=    QStep;                                   /* QValue */
                int ebx_=    Sharpen;                                 /* Sharpen */
                __m64 mm0,mm1,mm2,mm3,mm4,mm5,mm6,mm7;
                movd           ( mm0,    eax_                                             );/* QValue */
                movd           ( mm2,    ebx_                                             );/* sharpen */

                punpcklbw   (    mm0,    mm0                                             );/* 00 00 00 QQ */
        edx-=    ecx;/* Negative Pitch */

                punpcklbw   (    mm2,    mm2                                             );/* 00 00 00 SS */
        pxor      (  mm7,    mm7                     );/* clear mm7 for unpacks */
                  
                punpcklbw    (   mm0,    mm0                                             );/* 00 00 qq qq */
                uint8_t *eax=    (uint8_t*)LRPointer;/* Left and Right Modifier */                

                punpcklbw   (    mm2,    mm2                                             );/* 00 00 ss ss */
                const uint8_t *ebx=    esi+ecx*8;/* Source Pointer of last row */        

                punpcklbw  (     mm0,    mm0                                             );/* qq qq qq qq */
                movq       ( mm1,    mm0                    );/* make a copy */
                
                punpcklbw  (     mm2,    mm2                                             );/* ss ss ss ss */
                paddb      (     mm1,    mm0                                             );/* QValue * 2 */

        paddb     (  mm1,    mm0                     );/* High = 3 * Qvalue */
        paddusb   (      mm1,    eight223s                               );/* clamping high to 32 */       

                paddb    (   mm0,    eight32s                );/* 32+QValues */
                psubusb  (       mm1,    eight223s                               );/* Get the real value back */

        movq   (         mm3,    eight127s                               );/* 7f 7f 7f 7f 7f 7f 7f 7f */
        pandn  (     mm1,    mm3                     );/* ClampHigh */

        /* mm0,mm1,mm2,mm7 are in use  */
        /* mm0---> QValue+32           */
        /* mm1---> ClampHigh               */
                /* mm2---> Sharpen             */
                /* mm7---> Cleared for unpack  */

FillModLoop1:
        movq   (     mm3,    esi         );/* read 8 pixels p  */
        movq   (     mm4,    esi+edx     );/* Pixels on top pu */

        movq   (     mm5,    mm3                     );/* make a copy of p */
        psubusb(     mm3,    mm4                     );/* p-pu */
        
        psubusb(     mm4,    mm5                     );/* pu-p */
        por    (     mm3,    mm4                     );/* abs(p-pu) */

        movq   (     mm6,    mm0                     );/* 32+QValues */
               
        movq            (mm4,    mm0                                             );/* 32+QValues */
                psubusb (        mm6,    mm3                     );/* zero clampled TmpMod */

                movq    (        mm5,    eight128s                               );/* 80 80 80 80 80 80 80 80 */
                paddb   (        mm4,    eight64s                                );/* 32+QValues + 64 */

                pxor    (        mm4,    mm5                                             );/* convert to a sign number */
                pxor    (        mm3,    mm5                                             );/* convert to a sign number */

                pcmpgtb (        mm3,    mm4                                             );/* 32+QValue- 2*abs(p-pu) <-64 ? */
                pand    (        mm3,    mm2                                             );/* use sharpen */

        paddsb         ( mm6,    mm1                                             );/* clamping to high */
                psubsb (         mm6,    mm1                                             );/* offset back */

                por    (                 mm6,    mm3                                             );/* Mod value to be stored */
                pxor   (         mm5,    mm5                                             );/* clear mm5 */

                pxor       (     mm4,    mm4                                             );/* clear mm4 */
                punpcklbw  (     mm5,    mm6                                             );/* 03 xx 02 xx 01 xx 00 xx */

                psraw      (     mm5,    8                                               );/* sign extended */
                movq       (  edi, mm5            );/* writeout UDmod, low four */
                
                punpckhbw    (   mm4,    mm6);
                psraw        (   mm4,    8);

        movq       ( edi+8, mm4          );/* writeout UDmod, high four */
                   
        
        /* left Mod */
        movq       ( mm3,    esi         );/* read 8 pixels p  */
        movq       ( mm4,    esi-1     );/* Pixels on top pu */

        movq       ( mm5,    mm3                     );/* make a copy of p */
        psubusb    ( mm3,    mm4                     );/* p-pu */
        
        psubusb    ( mm4,    mm5                     );/* pu-p */
        por        ( mm3,    mm4                     );/* abs(p-pu) */

        movq       ( mm6,    mm0                     );/* 32+QValues */
                   
        movq           ( mm4,    mm0                                             );/* 32+QValues */
                psubusb(         mm6,    mm3                     );/* zero clampled TmpMod */

                movq   (         mm5,    eight128s                               );/* 80 80 80 80 80 80 80 80 */
                paddb  (         mm4,    eight64s                                );/* 32+QValues + 64 */

                pxor   (         mm4,    mm5                                             );/* convert to a sign number */
                pxor   (         mm3,    mm5                                             );/* convert to a sign number */

                pcmpgtb(         mm3,    mm4                                             );/* 32+QValue- 2*abs(p-pu) <-64 ? */
                pand   (         mm3,    mm2                                             );/* use sharpen */

        paddsb         ( mm6,    mm1                                             );/* clamping to high */
                psubsb (         mm6,    mm1                                             );/* offset back */

                por    (                 mm6,    mm3                                             );/* Mod value to be stored */
                pxor   (         mm5,    mm5                                             );/* clear mm5 */

                pxor     (       mm4,    mm4                                             );/* clear mm4 */
                punpcklbw(       mm5,    mm6                                             );/* 03 xx 02 xx 01 xx 00 xx */

                psraw    (       mm5,    8                                               );/* sign extended */
                movq     (   eax, mm5            );/* writeout UDmod, low four */
                
                punpckhbw(       mm4,    mm6);
                psraw    (       mm4,    8);

        movq      (  eax+8, mm4          );/* writeout UDmod, high four */
                  


        /* Right Mod */
        movq      (  mm3,    esi         );/* read 8 pixels p  */
        movq      (  mm4,    esi+1       );/* Pixels on top pu */

        movq      (  mm5,    mm3                     );/* make a copy of p */
        psubusb   (  mm3,    mm4                     );/* p-pu */
        
        psubusb   (  mm4,    mm5                     );/* pu-p */
        por       (  mm3,    mm4                     );/* abs(p-pu) */

        movq      (  mm6,    mm0                     );/* 32+QValues */
                  
        movq           ( mm4,    mm0                                             );/* 32+QValues */
                psubusb(         mm6,    mm3                     );/* zero clampled TmpMod */

                movq   (         mm5,    eight128s                               );/* 80 80 80 80 80 80 80 80 */
                paddb  (         mm4,    eight64s                                );/* 32+QValues + 64 */

                pxor   (         mm4,    mm5                                             );/* convert to a sign number */
                pxor   (         mm3,    mm5                                             );/* convert to a sign number */

                pcmpgtb(         mm3,    mm4                                             );/* 32+QValue- 2*abs(p-pu) <-64 ? */
                pand   (         mm3,    mm2                                             );/* use sharpen */

        paddsb         ( mm6,    mm1                                             );/* clamping to high */
                psubsb (         mm6,    mm1                                             );/* offset back */

                por    (                 mm6,    mm3                                             );/* Mod value to be stored */
                pxor   (         mm5,    mm5                                             );/* clear mm5 */

                pxor        (    mm4,    mm4                                             );/* clear mm4 */
                punpcklbw   (    mm5,    mm6                                             );/* 03 xx 02 xx 01 xx 00 xx */

                psraw      (     mm5,    8                                               );/* sign extended */
                movq       ( eax+128, mm5            );/* writeout UDmod, low four */
                
                punpckhbw  (     mm4,    mm6);
                psraw      (     mm4,    8);

        movq      (  eax+136, mm4          );/* writeout UDmod, high four */
        esi+=    ecx;
        
        
        edi+=    16                  ;
        eax+=    16      ;

        if (esi!=ebx)//cmp         esi,    ebx
         goto /*jne         */FillModLoop1;
        
        /* last UDMod */

        movq      (  mm3,    esi         );/* read 8 pixels p  */
        movq      (  mm4,    esi+edx     );/* Pixels on top pu */

        movq      (  mm5,    mm3                     );/* make a copy of p */
        psubusb   (  mm3,    mm4                     );/* p-pu */
        
        psubusb   (  mm4,    mm5                     );/* pu-p */
        por       (  mm3,    mm4                     );/* abs(p-pu) */

        movq      (  mm6,    mm0                     );/* 32+QValues */
                  
        movq           ( mm4,    mm0                                             );/* 32+QValues */
                psubusb(         mm6,    mm3                     );/* zero clampled TmpMod */

                movq   (         mm5,    eight128s                               );/* 80 80 80 80 80 80 80 80 */
                paddb  (         mm4,    eight64s                                );/* 32+QValues + 64 */

                pxor   (         mm4,    mm5                                             );/* convert to a sign number */
                pxor   (         mm3,    mm5                                             );/* convert to a sign number */

                pcmpgtb(         mm3,    mm4                                             );/* 32+QValue- 2*abs(p-pu) <-64 ? */
                pand   (         mm3,    mm2                                             );/* use sharpen */

        paddsb         ( mm6,    mm1                                             );/* clamping to high */
                psubsb (         mm6,    mm1                                             );/* offset back */

                por    (                 mm6,    mm3                                             );/* Mod value to be stored */
                pxor   (         mm5,    mm5                                             );/* clear mm5 */

                pxor      (      mm4,    mm4                                             );/* clear mm4 */
                punpcklbw (      mm5,    mm6                                             );/* 03 xx 02 xx 01 xx 00 xx */

                psraw     (      mm5,    8                                               );/* sign extended */
                movq      (  edi, mm5            );/* writeout UDmod, low four */
                
                punpckhbw (      mm4,    mm6);
                psraw     (      mm4,    8);

        movq      (  edi+8, mm4          );/* writeout UDmod, high four */
                  
                esi=    Src;
                edi=    Des;
                
                eax=    (uint8_t*)UDPointer;
                ebx=    (const uint8_t*)LRPointer;

                /* First Row */
                movq      (      mm0,    esi+edx               );/* mm0 = Pixels above */
                pxor      (      mm7,    mm7                             );/* clear mm7 */

                movq      (      mm1,    mm0                             );/* make a copy of mm0 */                        
                punpcklbw (      mm0,    mm7                             );/* lower four pixels */
                
                movq      (      mm4,    eax                   );/* au */
                punpckhbw (      mm1,    mm7                             );/* high four pixels */
                
                movq      (      mm5,    eax+8                 );/* au */
                          
                pmullw    (      mm0,    mm4                             );/* pu*au */
                movq      (      mm2,    esi+ecx               );/* mm2 = pixels below */
                
                pmullw    (      mm1,    mm5                             );/* pu*au */
                movq      (      mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw (      mm2,    mm7                             );/* lower four */
                movq      (      mm6,    eax+16                );/* ad */

                punpckhbw (      mm3,    mm7                             );/* higher four */                       
                paddw     (      mm4,    mm6                             );/* au+ad */
                
                pmullw    (      mm2,    mm6                             );/* au*pu+ad*pd */
                movq      (      mm6,    eax+24                );/* ad */

                paddw     (      mm0,    mm2                     );
                paddw     (      mm5,    mm6                             );/* au+ad */
                
                pmullw    (      mm3,    mm6                             );/* ad*pd */
                movq      (      mm2,    esi-1                 );/* pixel to the left */

                paddw     (      mm1,    mm3                             );/* au*pu+ad*pd */
                movq      (      mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw (      mm2,    mm7                             );/* four left pixels */
                movq      (      mm6,    ebx                   );/* al */

                punpckhbw (      mm3,    mm7                             );/* four right pixels */
                paddw     (      mm4,    mm6                             );/* au + ad + al */
                
                pmullw    (      mm2,    mm6                             );/* pl * al */
                movq      (      mm6,    ebx+8                 );/* al */

                paddw     (      mm0,    mm2                             );/* au*pu+ad*pd+al*pl */
                paddw     (      mm5,    mm6                             );/* au+ad+al */
                
                pmullw    (      mm3,    mm6                             );/* al*pl */
                movq      (      mm2,    esi+1                 );/* pixel to the right */

                paddw     (      mm1,    mm3                             );/* au*pu+ad*pd+al*pl */                 
                movq      (      mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw (      mm2,    mm7                             );/* four left pixels */
                movq      (      mm6,    ebx+128                       );/* ar */

                punpckhbw (      mm3,    mm7                             );/* four right pixels */                 
                paddw     (      mm4,    mm6                             );/* au + ad + al + ar */
                
                pmullw    (      mm2,    mm6                             );/* pr * ar */
                movq      (      mm6,    ebx+136               );/* ar */

                paddw     (      mm0,    mm2                             );/* au*pu+ad*pd+al*pl+pr*ar */
                paddw     (      mm5,    mm6                             );/* au+ad+al+ar */
                
                pmullw    (      mm3,    mm6                             );/* ar*pr */
                movq      (      mm2,    esi                   );/* p */

                paddw     (      mm1,    mm3                             );/* au*pu+ad*pd+al*pl+ar*pr */
                movq      (      mm3,    mm2                             );/* make a copy of the pixel */
                
                /* mm0, mm1 ---  au*pu+ad*pd+al*pl+ar*pr */
                /* mm4, mm5     ---      au + ad + al + ar */
                
                punpcklbw  (     mm2,    mm7                             );/* left four pixels */
                movq       (     mm6,    Four128s                );/* 0080  0080 0080 0080 */

                punpckhbw  (     mm3,    mm7                             );/* right four pixels */
                psubw      (     mm6,    mm4                             );/* 128-(au+ad+al+ar) */
                
                pmullw     (     mm2,    mm6                             );/* p*(128-(au+ad+al+ar)) */
                movq       (     mm6,    Four128s                );/* 0080  0080 0080 0080 */

                paddw      (     mm0,    mm2                             );/* sum */
                psubw      (     mm6,    mm5                             );/* 128-(au+ad+al+ar) */
                
                pmullw     (     mm3,    mm6                             );/* p*(128-(au+ad+al+ar)) */ 
                movq       (     mm6,    Four64s                 );/* {64, 64, 64, 64 } */

                movq       (     mm7,    mm6                             );/* {64, 64, 64, 64} */
                paddw      (     mm0,    mm6                             );/* sum+B */

                paddw      (     mm1,    mm3                             );/* sum */
                psllw      (     mm7,    8                               );/* {16384, .. } */

                paddw      (     mm0,    mm7                             );/* clamping */
                paddw      (     mm1,    mm6                             );/* sum+B */

                paddw      (     mm1,    mm7                             );/* clamping */
                psubusw    (     mm0,    mm7                             );/* clamping */

                psubusw    (     mm1,    mm7                             );/* clamping */
                psrlw      (     mm0,    7                               );/* (sum+B)>>7 */

                psrlw      (     mm1,    7                               );/* (sum+B)>>7 */
                packuswb   (     mm0,    mm1                             );/* pack to 8 bytes */
                
                movq       (     edi,  mm0                             );/* write to destination */
                           
                esi+=    ecx;/* Src += Pitch */
                edi+=    ecx;/* Des += Pitch */

                eax+=    16;/* UDPointer += 8 */
                ebx+=    16;/* LPointer +=8 */
                

                /* Second Row */
                movq           ( mm0,    esi+edx               );/* mm0 = Pixels above */
                pxor           ( mm7,    mm7                             );/* clear mm7 */

                movq           ( mm1,    mm0                             );/* make a copy of mm0 */                        
                punpcklbw      ( mm0,    mm7                             );/* lower four pixels */
                
                movq           ( mm4,    eax                   );/* au */
                punpckhbw      ( mm1,    mm7                             );/* high four pixels */
                
                movq           ( mm5,    eax+8                 );/* au */
                               
                pmullw         ( mm0,    mm4                             );/* pu*au */
                movq           ( mm2,    esi+ecx               );/* mm2 = pixels below */
                
                pmullw         ( mm1,    mm5                             );/* pu*au */
                movq           ( mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw      ( mm2,    mm7                             );/* lower four */
                movq           ( mm6,    eax+16                );/* ad */

                punpckhbw      ( mm3,    mm7                             );/* higher four */                       
                paddw          ( mm4,    mm6                             );/* au+ad */
                
                pmullw         ( mm2,    mm6                             );/* au*pu+ad*pd */
                movq           ( mm6,    eax+24                );/* ad */

                paddw          ( mm0,    mm2                     );
                paddw          ( mm5,    mm6                             );/* au+ad */
                
                pmullw         ( mm3,    mm6                             );/* ad*pd */
                movq           ( mm2,    esi-1                 );/* pixel to the left */

                paddw          ( mm1,    mm3                             );/* au*pu+ad*pd */
                movq           ( mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw      ( mm2,    mm7                             );/* four left pixels */
                movq           ( mm6,    ebx                   );/* al */

                punpckhbw      ( mm3,    mm7                             );/* four right pixels */
                paddw          ( mm4,    mm6                             );/* au + ad + al */
                
                pmullw         ( mm2,    mm6                             );/* pl * al */
                movq           ( mm6,    ebx+8                 );/* al */

                paddw          ( mm0,    mm2                             );/* au*pu+ad*pd+al*pl */
                paddw          ( mm5,    mm6                             );/* au+ad+al */
                
                pmullw         ( mm3,    mm6                             );/* al*pl */
                movq           ( mm2,    esi+1                 );/* pixel to the right */

                paddw          ( mm1,    mm3                             );/* au*pu+ad*pd+al*pl */                 
                movq           ( mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw      ( mm2,    mm7                             );/* four left pixels */
                movq           ( mm6,    ebx+128                       );/* ar */

                punpckhbw      ( mm3,    mm7                             );/* four right pixels */                 
                paddw          ( mm4,    mm6                             );/* au + ad + al + ar */
                
                pmullw         ( mm2,    mm6                             );/* pr * ar */
                movq           ( mm6,    ebx+136               );/* ar */

                paddw          ( mm0,    mm2                             );/* au*pu+ad*pd+al*pl+pr*ar */
                paddw          ( mm5,    mm6                             );/* au+ad+al+ar */
                
                pmullw         ( mm3,    mm6                             );/* ar*pr */
                movq           ( mm2,    esi                   );/* p */

                paddw          ( mm1,    mm3                             );/* au*pu+ad*pd+al*pl+ar*pr */
                movq           ( mm3,    mm2                             );/* make a copy of the pixel */
                
                /* mm0, mm1 ---  au*pu+ad*pd+al*pl+ar*pr */
                /* mm4, mm5     ---      au + ad + al + ar */
                
                punpcklbw    (   mm2,    mm7                             );/* left four pixels */
                movq         (   mm6,    Four128s                );/* 0080  0080 0080 0080 */

                punpckhbw    (   mm3,    mm7                             );/* right four pixels */
                psubw        (   mm6,    mm4                             );/* 128-(au+ad+al+ar) */
                
                pmullw       (   mm2,    mm6                             );/* p*(128-(au+ad+al+ar)) */
                movq         (   mm6,    Four128s                );/* 0080  0080 0080 0080 */

                paddw        (   mm0,    mm2                             );/* sum */
                psubw        (   mm6,    mm5                             );/* 128-(au+ad+al+ar) */
                
                pmullw       (   mm3,    mm6                             );/* p*(128-(au+ad+al+ar)) */ 
                movq         (   mm6,    Four64s                 );/* {64, 64, 64, 64 } */

                movq         (   mm7,    mm6                             );/* {64, 64, 64, 64} */
                paddw        (   mm0,    mm6                             );/* sum+B */

                paddw        (   mm1,    mm3                             );/* sum */
                psllw        (   mm7,    8                               );/* {16384, .. } */

                paddw        (   mm0,    mm7                             );/* clamping */
                paddw        (   mm1,    mm6                             );/* sum+B */

                paddw        (   mm1,    mm7                             );/* clamping */
                psubusw      (   mm0,    mm7                             );/* clamping */

                psubusw      (   mm1,    mm7                             );/* clamping */
                psrlw        (   mm0,    7                               );/* (sum+B)>>7 */

                psrlw        (   mm1,    7                               );/* (sum+B)>>7 */
                packuswb     (   mm0,    mm1                             );/* pack to 8 bytes */
                
                movq         (   edi,  mm0                             );/* write to destination */
                             
                esi+=    ecx;/* Src += Pitch */
                edi+=    ecx;/* Des += Pitch */

                eax+=    16;/* UDPointer += 8 */
                ebx+=    16;/* LPointer +=8 */
                

        /* Third Row */
                movq           ( mm0,    esi+edx               );/* mm0 = Pixels above */
                pxor           ( mm7,    mm7                             );/* clear mm7 */

                movq           ( mm1,    mm0                             );/* make a copy of mm0 */                        
                punpcklbw      ( mm0,    mm7                             );/* lower four pixels */
                
                movq           ( mm4,    eax                   );/* au */
                punpckhbw      ( mm1,    mm7                             );/* high four pixels */
                
                movq           ( mm5,    eax+8                 );/* au */
                               
                pmullw         ( mm0,    mm4                             );/* pu*au */
                movq           ( mm2,    esi+ecx               );/* mm2 = pixels below */
                
                pmullw         ( mm1,    mm5                             );/* pu*au */
                movq           ( mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw      ( mm2,    mm7                             );/* lower four */
                movq           ( mm6,    eax+16                );/* ad */

                punpckhbw      ( mm3,    mm7                             );/* higher four */                       
                paddw          ( mm4,    mm6                             );/* au+ad */
                
                pmullw         ( mm2,    mm6                             );/* au*pu+ad*pd */
                movq           ( mm6,    eax+24                );/* ad */

                paddw          ( mm0,    mm2                     );
                paddw          ( mm5,    mm6                             );/* au+ad */
                
                pmullw         ( mm3,    mm6                             );/* ad*pd */
                movq           ( mm2,    esi-1                 );/* pixel to the left */

                paddw          ( mm1,    mm3                             );/* au*pu+ad*pd */
                movq           ( mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw      ( mm2,    mm7                             );/* four left pixels */
                movq           ( mm6,    ebx                   );/* al */

                punpckhbw      ( mm3,    mm7                             );/* four right pixels */
                paddw          ( mm4,    mm6                             );/* au + ad + al */
                
                pmullw         ( mm2,    mm6                             );/* pl * al */
                movq           ( mm6,    ebx+8                 );/* al */

                paddw          ( mm0,    mm2                             );/* au*pu+ad*pd+al*pl */
                paddw          ( mm5,    mm6                             );/* au+ad+al */
                
                pmullw         ( mm3,    mm6                             );/* al*pl */
                movq           ( mm2,    esi+1                 );/* pixel to the right */

                paddw          ( mm1,    mm3                             );/* au*pu+ad*pd+al*pl */                 
                movq           ( mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw      ( mm2,    mm7                             );/* four left pixels */
                movq           ( mm6,    ebx+128                       );/* ar */

                punpckhbw      ( mm3,    mm7                             );/* four right pixels */                 
                paddw          ( mm4,    mm6                             );/* au + ad + al + ar */
                
                pmullw         ( mm2,    mm6                             );/* pr * ar */
                movq           ( mm6,    ebx+136               );/* ar */

                paddw          ( mm0,    mm2                             );/* au*pu+ad*pd+al*pl+pr*ar */
                paddw          ( mm5,    mm6                             );/* au+ad+al+ar */
                
                pmullw         ( mm3,    mm6                             );/* ar*pr */
                movq           ( mm2,    esi                   );/* p */

                paddw          ( mm1,    mm3                             );/* au*pu+ad*pd+al*pl+ar*pr */
                movq           ( mm3,    mm2                             );/* make a copy of the pixel */
                
                /* mm0, mm1 ---  au*pu+ad*pd+al*pl+ar*pr */
                /* mm4, mm5     ---      au + ad + al + ar */
                
                punpcklbw      ( mm2,    mm7                             );/* left four pixels */
                movq           ( mm6,    Four128s                );/* 0080  0080 0080 0080 */

                punpckhbw      ( mm3,    mm7                             );/* right four pixels */
                psubw          ( mm6,    mm4                             );/* 128-(au+ad+al+ar) */
                
                pmullw         ( mm2,    mm6                             );/* p*(128-(au+ad+al+ar)) */
                movq           ( mm6,    Four128s                );/* 0080  0080 0080 0080 */

                paddw          ( mm0,    mm2                             );/* sum */
                psubw          ( mm6,    mm5                             );/* 128-(au+ad+al+ar) */
                
                pmullw         ( mm3,    mm6                             );/* p*(128-(au+ad+al+ar)) */ 
                movq           ( mm6,    Four64s                 );/* {64, 64, 64, 64 } */

                movq           ( mm7,    mm6                             );/* {64, 64, 64, 64} */
                paddw          ( mm0,    mm6                             );/* sum+B */

                paddw          ( mm1,    mm3                             );/* sum */
                psllw          ( mm7,    8                               );/* {16384, .. } */

                paddw          ( mm0,    mm7                             );/* clamping */
                paddw          ( mm1,    mm6                             );/* sum+B */

                paddw          ( mm1,    mm7                             );/* clamping */
                psubusw        ( mm0,    mm7                             );/* clamping */

                psubusw        ( mm1,    mm7                             );/* clamping */
                psrlw          ( mm0,    7                               );/* (sum+B)>>7 */

                psrlw          ( mm1,    7                               );/* (sum+B)>>7 */
                packuswb       ( mm0,    mm1                             );/* pack to 8 bytes */
                
                movq           ( edi,  mm0                             );/* write to destination */
                               
                esi+=    ecx;/* Src += Pitch */
                edi+=    ecx;/* Des += Pitch */

                eax+=    16;/* UDPointer += 8 */
                ebx+=    16;/* LPointer +=8 */
                



        /* Fourth Row */
                movq         (   mm0,    esi+edx               );/* mm0 = Pixels above */
                pxor         (   mm7,    mm7                             );/* clear mm7 */

                movq         (   mm1,    mm0                             );/* make a copy of mm0 */                        
                punpcklbw    (   mm0,    mm7                             );/* lower four pixels */
                
                movq         (   mm4,    eax                   );/* au */
                punpckhbw    (   mm1,    mm7                             );/* high four pixels */
                
                movq         (   mm5,    eax+8                 );/* au */
                             
                pmullw        (  mm0,    mm4                             );/* pu*au */
                movq          (  mm2,    esi+ecx               );/* mm2 = pixels below */
                
                pmullw        (  mm1,    mm5                             );/* pu*au */
                movq          (  mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );/* lower four */
                movq          (  mm6,    eax+16                );/* ad */

                punpckhbw     (  mm3,    mm7                             );/* higher four */                       
                paddw         (  mm4,    mm6                             );/* au+ad */
                
                pmullw        (  mm2,    mm6                             );/* au*pu+ad*pd */
                movq          (  mm6,    eax+24                );/* ad */

                paddw         (  mm0,    mm2                     );
                paddw         (  mm5,    mm6                             );/* au+ad */
                
                pmullw        (  mm3,    mm6                             );/* ad*pd */
                movq          (  mm2,    esi-1                 );/* pixel to the left */

                paddw         (  mm1,    mm3                             );/* au*pu+ad*pd */
                movq          (  mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );/* four left pixels */
                movq          (  mm6,    ebx                   );/* al */

                punpckhbw     (  mm3,    mm7                             );/* four right pixels */
                paddw         (  mm4,    mm6                             );/* au + ad + al */
                
                pmullw        (  mm2,    mm6                             );/* pl * al */
                movq          (  mm6,    ebx+8                 );/* al */

                paddw         (  mm0,    mm2                             );/* au*pu+ad*pd+al*pl */
                paddw         (  mm5,    mm6                             );/* au+ad+al */
                
                pmullw        (  mm3,    mm6                             );/* al*pl */
                movq          (  mm2,    esi+1                 );/* pixel to the right */

                paddw         (  mm1,    mm3                             );/* au*pu+ad*pd+al*pl */                 
                movq          (  mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );/* four left pixels */
                movq          (  mm6,    ebx+128                       );/* ar */

                punpckhbw     (  mm3,    mm7                             );/* four right pixels */                 
                paddw         (  mm4,    mm6                             );/* au + ad + al + ar */
                
                pmullw        (  mm2,    mm6                             );/* pr * ar */
                movq          (  mm6,    ebx+136               );/* ar */

                paddw         (  mm0,    mm2                             );/* au*pu+ad*pd+al*pl+pr*ar */
                paddw         (  mm5,    mm6                             );/* au+ad+al+ar */
                
                pmullw        (  mm3,    mm6                             );/* ar*pr */
                movq          (  mm2,    esi                   );/* p */

                paddw         (  mm1,    mm3                             );/* au*pu+ad*pd+al*pl+ar*pr */
                movq          (  mm3,    mm2                             );/* make a copy of the pixel */
                
                /* mm0, mm1 ---  au*pu+ad*pd+al*pl+ar*pr */
                /* mm4, mm5     ---      au + ad + al + ar */
                
                punpcklbw    (   mm2,    mm7                             );/* left four pixels */
                movq         (   mm6,    Four128s                );/* 0080  0080 0080 0080 */

                punpckhbw    (   mm3,    mm7                             );/* right four pixels */
                psubw        (   mm6,    mm4                             );/* 128-(au+ad+al+ar) */
                
                pmullw       (   mm2,    mm6                             );/* p*(128-(au+ad+al+ar)) */
                movq         (   mm6,    Four128s                );/* 0080  0080 0080 0080 */

                paddw        (   mm0,    mm2                             );/* sum */
                psubw        (   mm6,    mm5                             );/* 128-(au+ad+al+ar) */
                
                pmullw       (   mm3,    mm6                             );/* p*(128-(au+ad+al+ar)) */ 
                movq         (   mm6,    Four64s                 );/* {64, 64, 64, 64 } */

                movq         (   mm7,    mm6                             );/* {64, 64, 64, 64} */
                paddw        (   mm0,    mm6                             );/* sum+B */

                paddw        (   mm1,    mm3                             );/* sum */
                psllw        (   mm7,    8                               );/* {16384, .. } */

                paddw        (   mm0,    mm7                             );/* clamping */
                paddw        (   mm1,    mm6                             );/* sum+B */

                paddw        (   mm1,    mm7                             );/* clamping */
                psubusw      (   mm0,    mm7                             );/* clamping */

                psubusw      (   mm1,    mm7                             );/* clamping */
                psrlw        (   mm0,    7                               );/* (sum+B)>>7 */

                psrlw        (   mm1,    7                               );/* (sum+B)>>7 */
                packuswb     (   mm0,    mm1                             );/* pack to 8 bytes */
                
                movq         (   edi,  mm0                             );/* write to destination */
                             
                esi+=    ecx                             ;/* Src += Pitch */
                edi+=    ecx                             ;/* Des += Pitch */

                eax+=    16                              ;/* UDPointer += 8 */
                ebx+=    16              ;/* LPointer +=8 */
                

        /* Fifth Row */

                movq          (  mm0,    esi+edx               );/* mm0 = Pixels above */
                pxor          (  mm7,    mm7                             );/* clear mm7 */

                movq          (  mm1,    mm0                             );/* make a copy of mm0 */                        
                punpcklbw     (  mm0,    mm7                             );/* lower four pixels */
                
                movq          (  mm4,    eax                   );/* au */
                punpckhbw     (  mm1,    mm7                             );/* high four pixels */
                
                movq          (  mm5,    eax+8                 );/* au */
                              
                pmullw        (  mm0,    mm4                             );/* pu*au */
                movq          (  mm2,    esi+ecx               );/* mm2 = pixels below */
                
                pmullw        (  mm1,    mm5                             );/* pu*au */
                movq          (  mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );/* lower four */
                movq          (  mm6,    eax+16                );/* ad */

                punpckhbw     (  mm3,    mm7                             );/* higher four */                       
                paddw         (  mm4,    mm6                             );/* au+ad */
                
                pmullw        (  mm2,    mm6                             );/* au*pu+ad*pd */
                movq          (  mm6,    eax+24                );/* ad */

                paddw         (  mm0,    mm2                     );
                paddw         (  mm5,    mm6                             );/* au+ad */
                
                pmullw        (  mm3,    mm6                             );/* ad*pd */
                movq          (  mm2,    esi-1                 );/* pixel to the left */

                paddw         (  mm1,    mm3                             );/* au*pu+ad*pd */
                movq          (  mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );/* four left pixels */
                movq          (  mm6,    ebx                   );/* al */

                punpckhbw     (  mm3,    mm7                             );/* four right pixels */
                paddw         (  mm4,    mm6                             );/* au + ad + al */
                
                pmullw        (  mm2,    mm6                             );/* pl * al */
                movq          (  mm6,    ebx+8                 );/* al */

                paddw         (  mm0,    mm2                             );/* au*pu+ad*pd+al*pl */
                paddw         (  mm5,    mm6                             );/* au+ad+al */
                
                pmullw        (  mm3,    mm6                             );/* al*pl */
                movq          (  mm2,    esi+1                 );/* pixel to the right */

                paddw         (  mm1,    mm3                             );/* au*pu+ad*pd+al*pl */                 
                movq          (  mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );/* four left pixels */
                movq          (  mm6,    ebx+128                       );/* ar */

                punpckhbw     (  mm3,    mm7                             );/* four right pixels */                 
                paddw         (  mm4,    mm6                             );/* au + ad + al + ar */
                
                pmullw        (  mm2,    mm6                             );/* pr * ar */
                movq          (  mm6,    ebx+136               );/* ar */

                paddw         (  mm0,    mm2                             );/* au*pu+ad*pd+al*pl+pr*ar */
                paddw         (  mm5,    mm6                             );/* au+ad+al+ar */
                
                pmullw        (  mm3,    mm6                             );/* ar*pr */
                movq          (  mm2,    esi                   );/* p */

                paddw         (  mm1,    mm3                             );/* au*pu+ad*pd+al*pl+ar*pr */
                movq          (  mm3,    mm2                             );/* make a copy of the pixel */
                
                /* mm0, mm1 ---  au*pu+ad*pd+al*pl+ar*pr */
                /* mm4, mm5     ---      au + ad + al + ar */
                
                punpcklbw  (     mm2,    mm7                             );/* left four pixels */
                movq       (     mm6,    Four128s                );/* 0080  0080 0080 0080 */

                punpckhbw  (     mm3,    mm7                             );/* right four pixels */
                psubw      (     mm6,    mm4                             );/* 128-(au+ad+al+ar) */
                
                pmullw     (     mm2,    mm6                             );/* p*(128-(au+ad+al+ar)) */
                movq       (     mm6,    Four128s                );/* 0080  0080 0080 0080 */

                paddw      (     mm0,    mm2                             );/* sum */
                psubw      (     mm6,    mm5                             );/* 128-(au+ad+al+ar) */
                
                pmullw     (     mm3,    mm6                             );/* p*(128-(au+ad+al+ar)) */ 
                movq       (     mm6,    Four64s                 );/* {64, 64, 64, 64 } */

                movq       (     mm7,    mm6                             );/* {64, 64, 64, 64} */
                paddw      (     mm0,    mm6                             );/* sum+B */

                paddw      (     mm1,    mm3                             );/* sum */
                psllw      (     mm7,    8                               );/* {16384, .. } */

                paddw      (     mm0,    mm7                             );/* clamping */
                paddw      (     mm1,    mm6                             );/* sum+B */

                paddw      (     mm1,    mm7                             );/* clamping */
                psubusw    (     mm0,    mm7                             );/* clamping */

                psubusw    (     mm1,    mm7                             );/* clamping */
                psrlw      (     mm0,    7                               );/* (sum+B)>>7 */

                psrlw      (     mm1,    7                               );/* (sum+B)>>7 */
                packuswb   (     mm0,    mm1                             );/* pack to 8 bytes */
                
                movq       (     edi,  mm0                             );/* write to destination */
                           
                esi+=    ecx                             ;/* Src += Pitch */
                edi+=    ecx                             ;/* Des += Pitch */

                eax+=    16                              ;/* UDPointer += 8 */
                ebx+=    16              ;/* LPointer +=8 */
                

        /* Sixth Row */

                movq          (  mm0,    esi+edx               );/* mm0 = Pixels above */
                pxor          (  mm7,    mm7                             );/* clear mm7 */

                movq          (  mm1,    mm0                             );/* make a copy of mm0 */                        
                punpcklbw     (  mm0,    mm7                             );/* lower four pixels */
                
                movq          (  mm4,    eax                   );/* au */
                punpckhbw     (  mm1,    mm7                             );/* high four pixels */
                
                movq          (  mm5,    eax+8                 );/* au */
                              
                pmullw        (  mm0,    mm4                             );/* pu*au */
                movq          (  mm2,    esi+ecx               );/* mm2 = pixels below */
                
                pmullw        (  mm1,    mm5                             );/* pu*au */
                movq          (  mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );/* lower four */
                movq          (  mm6,    eax+16                );/* ad */

                punpckhbw     (  mm3,    mm7                             );/* higher four */                       
                paddw         (  mm4,    mm6                             );/* au+ad */
                
                pmullw        (  mm2,    mm6                             );/* au*pu+ad*pd */
                movq          (  mm6,    eax+24                );/* ad */

                paddw         (  mm0,    mm2                     );
                paddw         (  mm5,    mm6                             );/* au+ad */
                
                pmullw        (  mm3,    mm6                             );/* ad*pd */
                movq          (  mm2,    esi-1                 );/* pixel to the left */

                paddw         (  mm1,    mm3                             );/* au*pu+ad*pd */
                movq          (  mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );/* four left pixels */
                movq          (  mm6,    ebx                   );/* al */

                punpckhbw     (  mm3,    mm7                             );/* four right pixels */
                paddw         (  mm4,    mm6                             );/* au + ad + al */
                
                pmullw        (  mm2,    mm6                             );/* pl * al */
                movq          (  mm6,    ebx+8                 );/* al */

                paddw         (  mm0,    mm2                             );/* au*pu+ad*pd+al*pl */
                paddw         (  mm5,    mm6                             );/* au+ad+al */
                
                pmullw        (  mm3,    mm6                             );/* al*pl */
                movq          (  mm2,    esi+1                 );/* pixel to the right */

                paddw         (  mm1,    mm3                             );/* au*pu+ad*pd+al*pl */                 
                movq          (  mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );/* four left pixels */
                movq          (  mm6,    ebx+128                       );/* ar */

                punpckhbw     (  mm3,    mm7                             );/* four right pixels */                 
                paddw         (  mm4,    mm6                             );/* au + ad + al + ar */
                
                pmullw        (  mm2,    mm6                             );/* pr * ar */
                movq          (  mm6,    ebx+136               );/* ar */

                paddw         (  mm0,    mm2                             );/* au*pu+ad*pd+al*pl+pr*ar */
                paddw         (  mm5,    mm6                             );/* au+ad+al+ar */
                
                pmullw        (  mm3,    mm6                             );/* ar*pr */
                movq          (  mm2,    esi                   );/* p */

                paddw         (  mm1,    mm3                             );/* au*pu+ad*pd+al*pl+ar*pr */
                movq          (  mm3,    mm2                             );/* make a copy of the pixel */
                
                /* mm0, mm1 ---  au*pu+ad*pd+al*pl+ar*pr */
                /* mm4, mm5     ---      au + ad + al + ar */
                
                punpcklbw   (    mm2,    mm7                             );/* left four pixels */
                movq        (    mm6,    Four128s                );/* 0080  0080 0080 0080 */

                punpckhbw   (    mm3,    mm7                             );/* right four pixels */
                psubw       (    mm6,    mm4                             );/* 128-(au+ad+al+ar) */
                
                pmullw      (    mm2,    mm6                             );/* p*(128-(au+ad+al+ar)) */
                movq        (    mm6,    Four128s                );/* 0080  0080 0080 0080 */

                paddw       (    mm0,    mm2                             );/* sum */
                psubw       (    mm6,    mm5                             );/* 128-(au+ad+al+ar) */
                
                pmullw      (    mm3,    mm6                             );/* p*(128-(au+ad+al+ar)) */ 
                movq        (    mm6,    Four64s                 );/* {64, 64, 64, 64 } */

                movq        (    mm7,    mm6                             );/* {64, 64, 64, 64} */
                paddw       (    mm0,    mm6                             );/* sum+B */

                paddw       (    mm1,    mm3                             );/* sum */
                psllw       (    mm7,    8                               );/* {16384, .. } */

                paddw       (    mm0,    mm7                             );/* clamping */
                paddw       (    mm1,    mm6                             );/* sum+B */

                paddw       (    mm1,    mm7                             );/* clamping */
                psubusw     (    mm0,    mm7                             );/* clamping */

                psubusw     (    mm1,    mm7                             );/* clamping */
                psrlw       (    mm0,    7                               );/* (sum+B)>>7 */

                psrlw       (    mm1,    7                               );/* (sum+B)>>7 */
                packuswb    (    mm0,    mm1                             );/* pack to 8 bytes */
                
                movq        (    edi,  mm0                             );/* write to destination */
                            
                esi+=    ecx                             ;/* Src += Pitch */
                edi+=    ecx                             ;/* Des += Pitch */

                eax+=    16                              ;/* UDPointer += 8 */
                ebx+=    16              ;/* LPointer +=8 */
                

        /* Seventh Row */

                movq         (   mm0,    esi+edx               );/* mm0 = Pixels above */
                pxor         (   mm7,    mm7                             );/* clear mm7 */

                movq         (   mm1,    mm0                             );/* make a copy of mm0 */                        
                punpcklbw    (   mm0,    mm7                             );/* lower four pixels */
                
                movq         (   mm4,    eax                   );/* au */
                punpckhbw    (   mm1,    mm7                             );/* high four pixels */
                
                movq         (   mm5,    eax+8                 );/* au */
                             
                pmullw       (   mm0,    mm4                             );/* pu*au */
                movq         (   mm2,    esi+ecx               );/* mm2 = pixels below */
                
                pmullw       (   mm1,    mm5                             );/* pu*au */
                movq         (   mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw    (   mm2,    mm7                             );/* lower four */
                movq         (   mm6,    eax+16                );/* ad */

                punpckhbw    (   mm3,    mm7                             );/* higher four */                       
                paddw        (   mm4,    mm6                             );/* au+ad */
                
                pmullw       (   mm2,    mm6                             );/* au*pu+ad*pd */
                movq         (   mm6,    eax+24                );/* ad */

                paddw        (   mm0,    mm2                     );
                paddw        (   mm5,    mm6                             );/* au+ad */
                
                pmullw       (   mm3,    mm6                             );/* ad*pd */
                movq         (   mm2,    esi-1                 );/* pixel to the left */

                paddw        (   mm1,    mm3                             );/* au*pu+ad*pd */
                movq         (   mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw    (   mm2,    mm7                             );/* four left pixels */
                movq         (   mm6,    ebx                   );/* al */

                punpckhbw    (   mm3,    mm7                             );/* four right pixels */
                paddw        (   mm4,    mm6                             );/* au + ad + al */
                
                pmullw       (   mm2,    mm6                             );/* pl * al */
                movq         (   mm6,    ebx+8                 );/* al */

                paddw        (   mm0,    mm2                             );/* au*pu+ad*pd+al*pl */
                paddw        (   mm5,    mm6                             );/* au+ad+al */
                
                pmullw       (   mm3,    mm6                             );/* al*pl */
                movq         (   mm2,    esi+1                 );/* pixel to the right */

                paddw        (   mm1,    mm3                             );/* au*pu+ad*pd+al*pl */                 
                movq         (   mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw    (   mm2,    mm7                             );/* four left pixels */
                movq         (   mm6,    ebx+128                       );/* ar */

                punpckhbw    (   mm3,    mm7                             );/* four right pixels */                 
                paddw        (   mm4,    mm6                             );/* au + ad + al + ar */
                
                pmullw       (   mm2,    mm6                             );/* pr * ar */
                movq         (   mm6,    ebx+136               );/* ar */

                paddw        (   mm0,    mm2                             );/* au*pu+ad*pd+al*pl+pr*ar */
                paddw        (   mm5,    mm6                             );/* au+ad+al+ar */
                
                pmullw       (   mm3,    mm6                             );/* ar*pr */
                movq         (   mm2,    esi                   );/* p */

                paddw        (   mm1,    mm3                             );/* au*pu+ad*pd+al*pl+ar*pr */
                movq         (   mm3,    mm2                             );/* make a copy of the pixel */
                
                /* mm0, mm1 ---  au*pu+ad*pd+al*pl+ar*pr */
                /* mm4, mm5     ---      au + ad + al + ar */
                
                punpcklbw    (   mm2,    mm7                             );/* left four pixels */
                movq         (   mm6,    Four128s                );/* 0080  0080 0080 0080 */

                punpckhbw    (   mm3,    mm7                             );/* right four pixels */
                psubw        (   mm6,    mm4                             );/* 128-(au+ad+al+ar) */
                
                pmullw       (   mm2,    mm6                             );/* p*(128-(au+ad+al+ar)) */
                movq         (   mm6,    Four128s                );/* 0080  0080 0080 0080 */

                paddw        (   mm0,    mm2                             );/* sum */
                psubw        (   mm6,    mm5                             );/* 128-(au+ad+al+ar) */
                
                pmullw       (   mm3,    mm6                             );/* p*(128-(au+ad+al+ar)) */ 
                movq         (   mm6,    Four64s                 );/* {64, 64, 64, 64 } */

                movq         (   mm7,    mm6                             );/* {64, 64, 64, 64} */
                paddw        (   mm0,    mm6                             );/* sum+B */

                paddw        (   mm1,    mm3                             );/* sum */
                psllw        (   mm7,    8                               );/* {16384, .. } */

                paddw        (   mm0,    mm7                             );/* clamping */
                paddw        (   mm1,    mm6                             );/* sum+B */

                paddw        (   mm1,    mm7                             );/* clamping */
                psubusw      (   mm0,    mm7                             );/* clamping */

                psubusw      (   mm1,    mm7                             );/* clamping */
                psrlw        (   mm0,    7                               );/* (sum+B)>>7 */

                psrlw        (   mm1,    7                               );/* (sum+B)>>7 */
                packuswb     (   mm0,    mm1                             );/* pack to 8 bytes */
                
                movq         (   edi,  mm0                             );/* write to destination */
                             
                esi+=    ecx                             ;/* Src += Pitch */
                edi+=    ecx                             ;/* Des += Pitch */

                eax+=    16                              ;/* UDPointer += 8 */
                ebx+=    16              ;/* LPointer +=8 */
                
        /* Eighth Row */

                movq          (  mm0,    esi+edx               );/* mm0 = Pixels above */
                pxor          (  mm7,    mm7                             );/* clear mm7 */

                movq          (  mm1,    mm0                             );/* make a copy of mm0 */                        
                punpcklbw     (  mm0,    mm7                             );/* lower four pixels */
                
                movq          (  mm4,    eax                   );/* au */
                punpckhbw     (  mm1,    mm7                             );/* high four pixels */
                
                movq          (  mm5,    eax+8                 );/* au */
                              
                pmullw        (  mm0,    mm4                             );/* pu*au */
                movq          (  mm2,    esi+ecx               );/* mm2 = pixels below */
                
                pmullw        (  mm1,    mm5                             );/* pu*au */
                movq          (  mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );/* lower four */
                movq          (  mm6,    eax+16                );/* ad */

                punpckhbw     (  mm3,    mm7                             );/* higher four */                       
                paddw         (  mm4,    mm6                             );/* au+ad */
                
                pmullw        (  mm2,    mm6                             );/* au*pu+ad*pd */
                movq          (  mm6,    eax+24                );/* ad */

                paddw         (  mm0,    mm2                     );
                paddw         (  mm5,    mm6                             );/* au+ad */
                
                pmullw        (  mm3,    mm6                             );/* ad*pd */
                movq          (  mm2,    esi-1                 );/* pixel to the left */

                paddw         (  mm1,    mm3                             );/* au*pu+ad*pd */
                movq          (  mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );/* four left pixels */
                movq          (  mm6,    ebx                   );/* al */

                punpckhbw     (  mm3,    mm7                             );/* four right pixels */
                paddw         (  mm4,    mm6                             );/* au + ad + al */
                
                pmullw        (  mm2,    mm6                             );/* pl * al */
                movq          (  mm6,    ebx+8                 );/* al */

                paddw         (  mm0,    mm2                             );/* au*pu+ad*pd+al*pl */
                paddw         (  mm5,    mm6                             );/* au+ad+al */
                
                pmullw        (  mm3,    mm6                             );/* al*pl */
                movq          (  mm2,    esi+1                 );/* pixel to the right */

                paddw         (  mm1,    mm3                             );/* au*pu+ad*pd+al*pl */                 
                movq          (  mm3,    mm2                             );/* make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );/* four left pixels */
                movq          (  mm6,    ebx+128                       );/* ar */

                punpckhbw     (  mm3,    mm7                             );/* four right pixels */                 
                paddw         (  mm4,    mm6                             );/* au + ad + al + ar */
                
                pmullw        (  mm2,    mm6                             );/* pr * ar */
                movq          (  mm6,    ebx+136               );/* ar */

                paddw         (  mm0,    mm2                             );/* au*pu+ad*pd+al*pl+pr*ar */
                paddw         (  mm5,    mm6                             );/* au+ad+al+ar */
                
                pmullw        (  mm3,    mm6                             );/* ar*pr */
                movq          (  mm2,    esi                   );/* p */

                paddw         (  mm1,    mm3                             );/* au*pu+ad*pd+al*pl+ar*pr */
                movq          (  mm3,    mm2                             );/* make a copy of the pixel */
                
                /* mm0, mm1 ---  au*pu+ad*pd+al*pl+ar*pr */
                /* mm4, mm5     ---      au + ad + al + ar */
                
                punpcklbw     (  mm2,    mm7                             );/* left four pixels */
                movq          (  mm6,    Four128s                );/* 0080  0080 0080 0080 */

                punpckhbw     (  mm3,    mm7                             );/* right four pixels */
                psubw         (  mm6,    mm4                             );/* 128-(au+ad+al+ar) */
                
                pmullw        (  mm2,    mm6                             );/* p*(128-(au+ad+al+ar)) */
                movq          (  mm6,    Four128s                );/* 0080  0080 0080 0080 */

                paddw         (  mm0,    mm2                             );/* sum */
                psubw         (  mm6,    mm5                             );/* 128-(au+ad+al+ar) */
                
                pmullw        (  mm3,    mm6                             );/* p*(128-(au+ad+al+ar)) */ 
                movq          (  mm6,    Four64s                 );/* {64, 64, 64, 64 } */

                movq          (  mm7,    mm6                             );/* {64, 64, 64, 64} */
                paddw         (  mm0,    mm6                             );/* sum+B */

                paddw         (  mm1,    mm3                             );/* sum */
                psllw         (  mm7,    8                               );/* {16384, .. } */

                paddw         (  mm0,    mm7                             );/* clamping */
                paddw         (  mm1,    mm6                             );/* sum+B */

                paddw         (  mm1,    mm7                             );/* clamping */
                psubusw       (  mm0,    mm7                             );/* clamping */

                psubusw       (  mm1,    mm7                             );/* clamping */
                psrlw         (  mm0,    7                               );/* (sum+B)>>7 */

                psrlw         (  mm1,    7                               );/* (sum+B)>>7 */
                packuswb      (  mm0,    mm1                             );/* pack to 8 bytes */
                
                movq          (  edi,  mm0                             );/* write to destination */
                              
}




/****************************************************************************
 * 
 *  ROUTINE       :     DeRingBlockWeak()
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Filtering a block for deringing purpose
 *
 *  SPECIAL NOTES :     
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

extern "C" void DeringBlockWeak_MMX( 
                       const uint8_t *SrcPtr,
                       uint8_t *DstPtr,
                       const int32_t Pitch,
                       uint32_t FragQIndex,
                       const uint32_t *QuantScale)
{

        __align16( short, UDMod[72]);
        __align16( short,     LRMod[128]);
    
        unsigned int PlaneLineStep = Pitch;
        const unsigned char * Src = SrcPtr;
        unsigned char * Des = DstPtr;
    
        short * UDPointer = UDMod;
        short * LRPointer = LRMod;
    
        uint32_t QStep = QuantScale[FragQIndex];
        int32_t Sharpen = SharpenModifier[FragQIndex];

                const uint8_t *esi=    Src;// Source Pointer */
                uint8_t *edi=(uint8_t*)UDPointer                               ;// UD modifier pointer */

                unsigned int ecx=    PlaneLineStep                   ;// Pitch Step */
                int edx=0;

                int eax_= QStep                                   ;// QValue */
                int ebx_= Sharpen                                 ;// Sharpen */
                __m64 mm0,mm1,mm2,mm3,mm4,mm5,mm6,mm7;
                movd        (    mm0,    eax_                                             );// QValue */
                movd        (    mm2,    ebx_                                             );// sharpen */

                punpcklbw     (  mm0,    mm0                                             );// 00 00 00 QQ */
                edx-=    ecx;// Negative Pitch */

                punpcklbw   (    mm2,    mm2                                             );// 00 00 00 SS */
                pxor        (mm7,    mm7                     );// clear mm7 for unpacks */

                punpcklbw    (   mm0,    mm0                                             );// 00 00 qq qq */
                uint8_t *eax=(uint8_t*)    LRPointer                               ;// Left and Right Modifier */                

                punpcklbw  (     mm2,    mm2                                             );// 00 00 ss ss */
                const unsigned char *ebx=    esi+ecx*8;// Source Pointer of last row */        

                punpcklbw (      mm0,    mm0                                             );// qq qq qq qq */
                movq      (  mm1,    mm0                    );// make a copy */
                
                punpcklbw   (    mm2,    mm2                                             );// ss ss ss ss */
                paddb       (    mm1,    mm0                                             );// QValue * 2 */

        paddb   (    mm1,    mm0                     );// High = 3 * Qvalue */
        paddusb (        mm1,    eight231s                               );// clamping high to 24 */       

                paddb    (   mm0,    eight32s                );// 32+QValues */
                psubusb  (       mm1,    eight231s                               );// Get the real value back */

        movq      (      mm3,    eight127s                               );// 7f 7f 7f 7f 7f 7f 7f 7f */
        pandn     (  mm1,    mm3                     );// ClampHigh */

        // mm0,mm1,mm2,mm7 are in use  */
        // mm0---> QValue+32           */
        // mm1---> ClampHigh               */
                // mm2---> Sharpen             */
                // mm7---> Cleared for unpack  */

FillModLoop1:
        movq      (  mm3,    esi         );// read 8 pixels p  */
        movq      (  mm4,    esi+edx     );// Pixels on top pu */

        movq      (  mm5,    mm3                     );// make a copy of p */
        psubusb   (  mm3,    mm4                     );// p-pu */
        
        psubusb   (  mm4,    mm5                     );// pu-p */
        por       (  mm3,    mm4                     );// abs(p-pu) */

                  movq     (   mm6,    mm0                     );// 32+QValues */
                paddusb    (     mm3,    mm3                                             );// 2*abs(p-pu) */

        movq           ( mm4,    mm0                                             );// 32+QValues */
                psubusb(         mm6,    mm3                     );// zero clampled TmpMod */

                movq   (         mm5,    eight128s                               );// 80 80 80 80 80 80 80 80 */
                paddb  (         mm4,    eight64s                                );// 32+QValues + 64 */

                pxor   (         mm4,    mm5                                             );// convert to a sign number */
                pxor   (         mm3,    mm5                                             );// convert to a sign number */

                pcmpgtb(         mm3,    mm4                                             );// 32+QValue- 2*abs(p-pu) <-64 ? */
                pand   (         mm3,    mm2                                             );// use sharpen */

        paddsb         ( mm6,    mm1                                             );// clamping to high */
                psubsb (         mm6,    mm1                                             );// offset back */

                por    (                 mm6,    mm3                                             );// Mod value to be stored */
                pxor   (         mm5,    mm5                                             );// clear mm5 */

                pxor         (   mm4,    mm4                                             );// clear mm4 */
                punpcklbw    (   mm5,    mm6                                             );// 03 xx 02 xx 01 xx 00 xx */

                psraw   (        mm5,    8                                               );// sign extended */
                movq    (    edi, mm5            );// writeout UDmod, low four */
                
                punpckhbw (      mm4,    mm6);
                psraw     (      mm4,    8);

        movq     (   edi+8, mm4          );// writeout UDmod, high four */
                 
        
        // left Mod */
        movq       ( mm3,    esi         );// read 8 pixels p  */
        movq       ( mm4,    esi-1     );// Pixels on top pu */

        movq       ( mm5,    mm3                     );// make a copy of p */
        psubusb    ( mm3,    mm4                     );// p-pu */
        
        psubusb    ( mm4,    mm5                     );// pu-p */
        por        ( mm3,    mm4                     );// abs(p-pu) */

        movq            ( mm6,    mm0                     );// 32+QValues */
                paddusb (        mm3,    mm3                                             );// 2*abs(p-pu) */

        movq            (mm4,    mm0                                             );// 32+QValues */
                psubusb (        mm6,    mm3                     );// zero clampled TmpMod */

                movq    (        mm5,    eight128s                               );// 80 80 80 80 80 80 80 80 */
                paddb   (        mm4,    eight64s                                );// 32+QValues + 64 */

                pxor    (        mm4,    mm5                                             );// convert to a sign number */
                pxor    (        mm3,    mm5                                             );// convert to a sign number */

                pcmpgtb (        mm3,    mm4                                             );// 32+QValue- 2*abs(p-pu) <-64 ? */
                pand    (        mm3,    mm2                                             );// use sharpen */

        paddsb          (mm6,    mm1                                             );// clamping to high */
                psubsb  (        mm6,    mm1                                             );// offset back */

                por     (                mm6,    mm3                                             );// Mod value to be stored */
                pxor    (        mm5,    mm5                                             );// clear mm5 */

                pxor     (       mm4,    mm4                                             );// clear mm4 */
                punpcklbw(       mm5,    mm6                                             );// 03 xx 02 xx 01 xx 00 xx */

                psraw    (       mm5,    8                                               );// sign extended */
                movq     (   eax, mm5            );// writeout UDmod, low four */
                
                punpckhbw(       mm4,    mm6);
                psraw    (       mm4,    8);

        movq      (  eax+8, mm4          );// writeout UDmod, high four */
                  


        // Right Mod */
        movq       ( mm3,    esi         );// read 8 pixels p  */
        movq       ( mm4,    esi+1       );// Pixels on top pu */

        movq       ( mm5,    mm3                     );// make a copy of p */
        psubusb    ( mm3,    mm4                     );// p-pu */
        
        psubusb    ( mm4,    mm5                     );// pu-p */
        por        ( mm3,    mm4                     );// abs(p-pu) */

        movq             ( mm6,    mm0                     );// 32+QValues */
                paddusb  (       mm3,    mm3                                             );// 2*abs(p-pu) */

        movq           ( mm4,    mm0                                             );// 32+QValues */
                psubusb(         mm6,    mm3                     );// zero clampled TmpMod */

                movq   (         mm5,    eight128s                               );// 80 80 80 80 80 80 80 80 */
                paddb  (         mm4,    eight64s                                );// 32+QValues + 64 */

                pxor   (         mm4,    mm5                                             );// convert to a sign number */
                pxor   (         mm3,    mm5                                             );// convert to a sign number */

                pcmpgtb(         mm3,    mm4                                             );// 32+QValue- 2*abs(p-pu) <-64 ? */
                pand   (         mm3,    mm2                                             );// use sharpen */

        paddsb         ( mm6,    mm1                                             );// clamping to high */
                psubsb (         mm6,    mm1                                             );// offset back */

                por    (                 mm6,    mm3                                             );// Mod value to be stored */
                pxor   (         mm5,    mm5                                             );// clear mm5 */

                pxor       (     mm4,    mm4                                             );// clear mm4 */
                punpcklbw  (     mm5,    mm6                                             );// 03 xx 02 xx 01 xx 00 xx */

                psraw      (     mm5,    8                                               );// sign extended */
                movq       ( eax+128, mm5            );// writeout UDmod, low four */
                
                punpckhbw  (     mm4,    mm6);
                psraw      (     mm4,    8);

        movq      (  eax+136, mm4          );// writeout UDmod, high four */
        esi+=    ecx;
        
        
        edi+=    16                  ;
        eax+=    16      ;

        if (esi!=ebx) //cmp         esi,    ebx
          goto /*jne         */FillModLoop1;
        
        // last UDMod */

        movq       ( mm3,    esi         );// read 8 pixels p  */
        movq       ( mm4,    esi+edx     );// Pixels on top pu */

        movq      (  mm5,    mm3                     );// make a copy of p */
        psubusb   (  mm3,    mm4                     );// p-pu */
        
        psubusb   (  mm4,    mm5                     );// pu-p */
        por       (  mm3,    mm4                     );// abs(p-pu) */

        movq               (mm6,    mm0                     );// 32+QValues */
                paddusb    (     mm3,    mm3                                             );// 2*abs(p-pu) */

        movq           ( mm4,    mm0                                             );// 32+QValues */
                psubusb(         mm6,    mm3                     );// zero clampled TmpMod */

                movq      (      mm5,    eight128s                               );// 80 80 80 80 80 80 80 80 */
                paddb     (      mm4,    eight64s                                );// 32+QValues + 64 */

                pxor       (     mm4,    mm5                                             );// convert to a sign number */
                pxor       (     mm3,    mm5                                             );// convert to a sign number */

                pcmpgtb  (       mm3,    mm4                                             );// 32+QValue- 2*abs(p-pu) <-64 ? */
                pand     (       mm3,    mm2                                             );// use sharpen */

        paddsb         ( mm6,    mm1                                             );// clamping to high */
                psubsb (         mm6,    mm1                                             );// offset back */

                por    (                 mm6,    mm3                                             );// Mod value to be stored */
                pxor   (         mm5,    mm5                                             );// clear mm5 */

                pxor      (      mm4,    mm4                                             );// clear mm4 */
                punpcklbw (      mm5,    mm6                                             );// 03 xx 02 xx 01 xx 00 xx */

                psraw     (      mm5,    8                                               );// sign extended */
                movq      (  edi, mm5            );// writeout UDmod, low four */
                
                punpckhbw (      mm4,    mm6);
                psraw     (      mm4,    8);

        movq      (  edi+8, mm4          );// writeout UDmod, high four */
                  
                esi=    Src;
                edi=    Des;
                
                eax=  (uint8_t*)  UDPointer;
                ebx=  (uint8_t*)  LRPointer;

                // First Row */
                movq          (  mm0,    esi+edx               );// mm0 = Pixels above */
                pxor          (  mm7,    mm7                             );// clear mm7 */

                movq          (  mm1,    mm0                             );// make a copy of mm0 */                        
                punpcklbw     (  mm0,    mm7                             );// lower four pixels */
                
                movq          (  mm4,    eax                   );// au */
                punpckhbw     (  mm1,    mm7                             );// high four pixels */
                
                movq          (  mm5,    eax+8                 );// au */
                              
                pmullw        (  mm0,    mm4                             );// pu*au */
                movq          (  mm2,    esi+ecx               );// mm2 = pixels below */
                
                pmullw        (  mm1,    mm5                             );// pu*au */
                movq          (  mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );// lower four */
                movq          (  mm6,    eax+16                );// ad */

                punpckhbw     (  mm3,    mm7                             );// higher four */                       
                paddw         (  mm4,    mm6                             );// au+ad */
                
                pmullw        (  mm2,    mm6                             );// au*pu+ad*pd */
                movq          (  mm6,    eax+24                );// ad */

                paddw         (  mm0,    mm2                     );
                paddw         (  mm5,    mm6                             );// au+ad */
                
                pmullw        (  mm3,    mm6                             );// ad*pd */
                movq          (  mm2,    esi-1                 );// pixel to the left */

                paddw         (  mm1,    mm3                             );// au*pu+ad*pd */
                movq          (  mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );// four left pixels */
                movq          (  mm6,    ebx                   );// al */

                punpckhbw     (  mm3,    mm7                             );// four right pixels */
                paddw         (  mm4,    mm6                             );// au + ad + al */
                
                pmullw        (  mm2,    mm6                             );// pl * al */
                movq          (  mm6,    ebx+8                 );// al */

                paddw         (  mm0,    mm2                             );// au*pu+ad*pd+al*pl */
                paddw         (  mm5,    mm6                             );// au+ad+al */
                
                pmullw        (  mm3,    mm6                             );// al*pl */
                movq          (  mm2,    esi+1                 );// pixel to the right */

                paddw         (  mm1,    mm3                             );// au*pu+ad*pd+al*pl */                 
                movq          (  mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );// four left pixels */
                movq          (  mm6,    ebx+128                       );// ar */

                punpckhbw     (  mm3,    mm7                             );// four right pixels */                 
                paddw         (  mm4,    mm6                             );// au + ad + al + ar */
                
                pmullw        (  mm2,    mm6                             );// pr * ar */
                movq          (  mm6,    ebx+136               );// ar */

                paddw         (  mm0,    mm2                             );// au*pu+ad*pd+al*pl+pr*ar */
                paddw         (  mm5,    mm6                             );// au+ad+al+ar */
                
                pmullw        (  mm3,    mm6                             );// ar*pr */
                movq          (  mm2,    esi                   );// p */

                paddw         (  mm1,    mm3                             );// au*pu+ad*pd+al*pl+ar*pr */
                movq          (  mm3,    mm2                             );// make a copy of the pixel */
                
                // mm0, mm1 ---  au*pu+ad*pd+al*pl+ar*pr */
                // mm4, mm5     ---      au + ad + al + ar */
                
                punpcklbw    (   mm2,    mm7                             );// left four pixels */
                movq         (   mm6,    Four128s                );// 0080  0080 0080 0080 */

                punpckhbw    (   mm3,    mm7                             );// right four pixels */
                psubw        (   mm6,    mm4                             );// 128-(au+ad+al+ar) */
                
                pmullw       (   mm2,    mm6                             );// p*(128-(au+ad+al+ar)) */
                movq         (   mm6,    Four128s                );// 0080  0080 0080 0080 */

                paddw        (   mm0,    mm2                             );// sum */
                psubw        (   mm6,    mm5                             );// 128-(au+ad+al+ar) */
                
                pmullw       (   mm3,    mm6                             );// p*(128-(au+ad+al+ar)) */ 
                movq         (   mm6,    Four64s                 );// {64, 64, 64, 64 } */

                movq         (   mm7,    mm6                             );// {64, 64, 64, 64} */
                paddw        (   mm0,    mm6                             );// sum+B */

                paddw        (   mm1,    mm3                             );// sum */
                psllw        (   mm7,    8                               );// {16384, .. } */

                paddw        (   mm0,    mm7                             );// clamping */
                paddw        (   mm1,    mm6                             );// sum+B */

                paddw        (   mm1,    mm7                             );// clamping */
                psubusw      (   mm0,    mm7                             );// clamping */

                psubusw      (   mm1,    mm7                             );// clamping */
                psrlw        (   mm0,    7                               );// (sum+B)>>7 */

                psrlw        (   mm1,    7                               );// (sum+B)>>7 */
                packuswb     (   mm0,    mm1                             );// pack to 8 bytes */
                
                movq         (   edi,  mm0                             );// write to destination */
                             
                esi+=    ecx                             ;// Src += Pitch */
                edi+=    ecx                             ;// Des += Pitch */

                eax+=    16                              ;// UDPointer += 8 */
        ebx+=    16              ;// LPointer +=8 */
                

                // Second Row */
                movq           ( mm0,    esi+edx               );// mm0 = Pixels above */
                pxor           ( mm7,    mm7                             );// clear mm7 */

                movq           ( mm1,    mm0                             );// make a copy of mm0 */                        
                punpcklbw      ( mm0,    mm7                             );// lower four pixels */
                
                movq           ( mm4,    eax                   );// au */
                punpckhbw      ( mm1,    mm7                             );// high four pixels */
                
                movq           ( mm5,    eax+8                 );// au */
                               
                pmullw         ( mm0,    mm4                             );// pu*au */
                movq           ( mm2,    esi+ecx               );// mm2 = pixels below */
                
                pmullw         ( mm1,    mm5                             );// pu*au */
                movq           ( mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw      ( mm2,    mm7                             );// lower four */
                movq           ( mm6,    eax+16                );// ad */

                punpckhbw      ( mm3,    mm7                             );// higher four */                       
                paddw          ( mm4,    mm6                             );// au+ad */
                
                pmullw         ( mm2,    mm6                             );// au*pu+ad*pd */
                movq           ( mm6,    eax+24                );// ad */

                paddw          ( mm0,    mm2                     );
                paddw          ( mm5,    mm6                             );// au+ad */
                
                pmullw          (mm3,    mm6                             );// ad*pd */
                movq            (mm2,    esi-1                 );// pixel to the left */

                paddw           (mm1,    mm3                             );// au*pu+ad*pd */
                movq            (mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw       (mm2,    mm7                             );// four left pixels */
                movq            (mm6,    ebx                   );// al */

                punpckhbw       (mm3,    mm7                             );// four right pixels */
                paddw           (mm4,    mm6                             );// au + ad + al */
                
                pmullw          (mm2,    mm6                             );// pl * al */
                movq            (mm6,    ebx+8                 );// al */

                paddw           (mm0,    mm2                             );// au*pu+ad*pd+al*pl */
                paddw           (mm5,    mm6                             );// au+ad+al */
                
                pmullw          (mm3,    mm6                             );// al*pl */
                movq            (mm2,    esi+1                 );// pixel to the right */

                paddw           (mm1,    mm3                             );// au*pu+ad*pd+al*pl */                 
                movq            (mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw       (mm2,    mm7                             );// four left pixels */
                movq            (mm6,    ebx+128                       );// ar */

                punpckhbw       (mm3,    mm7                             );// four right pixels */                 
                paddw           (mm4,    mm6                             );// au + ad + al + ar */
                
                pmullw          (mm2,    mm6                             );// pr * ar */
                movq            (mm6,    ebx+136               );// ar */

                paddw           (mm0,    mm2                             );// au*pu+ad*pd+al*pl+pr*ar */
                paddw           (mm5,    mm6                             );// au+ad+al+ar */
                
                pmullw          (mm3,    mm6                             );// ar*pr */
                movq            (mm2,    esi                   );// p */

                paddw           (mm1,    mm3                             );// au*pu+ad*pd+al*pl+ar*pr */
                movq            (mm3,    mm2                             );// make a copy of the pixel */
                
                // mm0, mm1 ---  au*pu+ad*pd+al*pl+ar*pr */
                // mm4, mm5     ---      au + ad + al + ar */
                
                punpcklbw      ( mm2,    mm7                             );// left four pixels */
                movq           ( mm6,    Four128s                );// 0080  0080 0080 0080 */

                punpckhbw      ( mm3,    mm7                             );// right four pixels */
                psubw          ( mm6,    mm4                             );// 128-(au+ad+al+ar) */
                
                pmullw         ( mm2,    mm6                             );// p*(128-(au+ad+al+ar)) */
                movq           ( mm6,    Four128s                );// 0080  0080 0080 0080 */

                paddw          ( mm0,    mm2                             );// sum */
                psubw          ( mm6,    mm5                             );// 128-(au+ad+al+ar) */
                
                pmullw         ( mm3,    mm6                             );// p*(128-(au+ad+al+ar)) */ 
                movq           ( mm6,    Four64s                 );// {64, 64, 64, 64 } */

                movq           ( mm7,    mm6                             );// {64, 64, 64, 64} */
                paddw          ( mm0,    mm6                             );// sum+B */

                paddw          ( mm1,    mm3                             );// sum */
                psllw          ( mm7,    8                               );// {16384, .. } */

                paddw          ( mm0,    mm7                             );// clamping */
                paddw          ( mm1,    mm6                             );// sum+B */

                paddw          ( mm1,    mm7                             );// clamping */
                psubusw        ( mm0,    mm7                             );// clamping */

                psubusw        ( mm1,    mm7                             );// clamping */
                psrlw          ( mm0,    7                               );// (sum+B)>>7 */

                psrlw          ( mm1,    7                               );// (sum+B)>>7 */
                packuswb       ( mm0,    mm1                             );// pack to 8 bytes */
                
                movq           ( edi,  mm0                             );// write to destination */
                               
                esi+=    ecx                             ;// Src += Pitch */
                edi+=    ecx                             ;// Des += Pitch */

                eax+=    16                              ;// UDPointer += 8 */
        ebx+=    16              ;// LPointer +=8 */
                

        // Third Row */
                movq         (   mm0,    esi+edx               );// mm0 = Pixels above */
                pxor         (   mm7,    mm7                             );// clear mm7 */

                movq         (   mm1,    mm0                             );// make a copy of mm0 */                        
                punpcklbw    (   mm0,    mm7                             );// lower four pixels */
                
                movq         (   mm4,    eax                   );// au */
                punpckhbw    (   mm1,    mm7                             );// high four pixels */
                
                movq         (   mm5,    eax+8                 );// au */
                             
                pmullw       (   mm0,    mm4                             );// pu*au */
                movq         (   mm2,    esi+ecx               );// mm2 = pixels below */
                
                pmullw       (   mm1,    mm5                             );// pu*au */
                movq         (   mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw    (   mm2,    mm7                             );// lower four */
                movq         (   mm6,    eax+16                );// ad */

                punpckhbw    (   mm3,    mm7                             );// higher four */                       
                paddw        (   mm4,    mm6                             );// au+ad */
                
                pmullw       (   mm2,    mm6                             );// au*pu+ad*pd */
                movq         (   mm6,    eax+24                );// ad */

                paddw        (   mm0,    mm2                     );
                paddw        (   mm5,    mm6                             );// au+ad */
                
                pmullw       (   mm3,    mm6                             );// ad*pd */
                movq         (   mm2,    esi-1                 );// pixel to the left */

                paddw        (   mm1,    mm3                             );// au*pu+ad*pd */
                movq         (   mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw    (   mm2,    mm7                             );// four left pixels */
                movq         (   mm6,    ebx                   );// al */

                punpckhbw    (   mm3,    mm7                             );// four right pixels */
                paddw        (   mm4,    mm6                             );// au + ad + al */
                
                pmullw       (   mm2,    mm6                             );// pl * al */
                movq         (   mm6,    ebx+8                 );// al */

                paddw        (   mm0,    mm2                             );// au*pu+ad*pd+al*pl */
                paddw        (   mm5,    mm6                             );// au+ad+al */
                
                pmullw       (   mm3,    mm6                             );// al*pl */
                movq         (   mm2,    esi+1                 );// pixel to the right */

                paddw        (   mm1,    mm3                             );// au*pu+ad*pd+al*pl */                 
                movq         (   mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw    (   mm2,    mm7                             );// four left pixels */
                movq         (   mm6,    ebx+128                       );// ar */

                punpckhbw    (   mm3,    mm7                             );// four right pixels */                 
                paddw        (   mm4,    mm6                             );// au + ad + al + ar */
                
                pmullw       (   mm2,    mm6                             );// pr * ar */
                movq         (   mm6,    ebx+136               );// ar */

                paddw        (   mm0,    mm2                             );// au*pu+ad*pd+al*pl+pr*ar */
                paddw        (   mm5,    mm6                             );// au+ad+al+ar */
                
                pmullw       (   mm3,    mm6                             );// ar*pr */
                movq         (   mm2,    esi                   );// p */

                paddw        (   mm1,    mm3                             );// au*pu+ad*pd+al*pl+ar*pr */
                movq         (   mm3,    mm2                             );// make a copy of the pixel */
                
                // mm0, mm1 ---  au*pu+ad*pd+al*pl+ar*pr */
                // mm4, mm5     ---      au + ad + al + ar */
                
                punpcklbw   (    mm2,    mm7                             );// left four pixels */
                movq        (    mm6,    Four128s                );// 0080  0080 0080 0080 */

                punpckhbw   (    mm3,    mm7                             );// right four pixels */
                psubw       (    mm6,    mm4                             );// 128-(au+ad+al+ar) */
                
                pmullw      (    mm2,    mm6                             );// p*(128-(au+ad+al+ar)) */
                movq        (    mm6,    Four128s                );// 0080  0080 0080 0080 */

                paddw       (    mm0,    mm2                             );// sum */
                psubw       (    mm6,    mm5                             );// 128-(au+ad+al+ar) */
                
                pmullw      (    mm3,    mm6                             );// p*(128-(au+ad+al+ar)) */ 
                movq        (    mm6,    Four64s                 );// {64, 64, 64, 64 } */

                movq        (    mm7,    mm6                             );// {64, 64, 64, 64} */
                paddw       (    mm0,    mm6                             );// sum+B */

                paddw       (    mm1,    mm3                             );// sum */
                psllw       (    mm7,    8                               );// {16384, .. } */

                paddw       (    mm0,    mm7                             );// clamping */
                paddw       (    mm1,    mm6                             );// sum+B */

                paddw       (    mm1,    mm7                             );// clamping */
                psubusw     (    mm0,    mm7                             );// clamping */

                psubusw     (    mm1,    mm7                             );// clamping */
                psrlw       (    mm0,    7                               );// (sum+B)>>7 */

                psrlw       (    mm1,    7                               );// (sum+B)>>7 */
                packuswb    (    mm0,    mm1                             );// pack to 8 bytes */
                
                movq        (    edi,  mm0                             );// write to destination */
                            
                esi+=    ecx                             ;// Src += Pitch */
                edi+=    ecx                             ;// Des += Pitch */

                eax+=    16                              ;// UDPointer += 8 */
        ebx+=    16              ;// LPointer +=8 */
                



        // Fourth Row */
                movq           ( mm0,    esi+edx               );// mm0 = Pixels above */
                pxor           ( mm7,    mm7                             );// clear mm7 */

                movq           ( mm1,    mm0                             );// make a copy of mm0 */                        
                punpcklbw      ( mm0,    mm7                             );// lower four pixels */
                
                movq           ( mm4,    eax                   );// au */
                punpckhbw      ( mm1,    mm7                             );// high four pixels */
                
                movq           ( mm5,    eax+8                 );// au */
                               
                pmullw         ( mm0,    mm4                             );// pu*au */
                movq           ( mm2,    esi+ecx               );// mm2 = pixels below */
                
                pmullw         ( mm1,    mm5                             );// pu*au */
                movq           ( mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw      ( mm2,    mm7                             );// lower four */
                movq           ( mm6,    eax+16                );// ad */

                punpckhbw      ( mm3,    mm7                             );// higher four */                       
                paddw          ( mm4,    mm6                             );// au+ad */
                
                pmullw         ( mm2,    mm6                             );// au*pu+ad*pd */
                movq           ( mm6,    eax+24                );// ad */

                paddw          ( mm0,    mm2                     );
                paddw          ( mm5,    mm6                             );// au+ad */
                
                pmullw         ( mm3,    mm6                             );// ad*pd */
                movq           ( mm2,    esi-1                 );// pixel to the left */

                paddw          ( mm1,    mm3                             );// au*pu+ad*pd */
                movq           ( mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw      ( mm2,    mm7                             );// four left pixels */
                movq           ( mm6,    ebx                   );// al */

                punpckhbw      ( mm3,    mm7                             );// four right pixels */
                paddw          ( mm4,    mm6                             );// au + ad + al */
                
                pmullw         ( mm2,    mm6                             );// pl * al */
                movq           ( mm6,    ebx+8                 );// al */

                paddw          ( mm0,    mm2                             );// au*pu+ad*pd+al*pl */
                paddw          ( mm5,    mm6                             );// au+ad+al */
                
                pmullw         ( mm3,    mm6                             );// al*pl */
                movq           ( mm2,    esi+1                 );// pixel to the right */

                paddw          ( mm1,    mm3                             );// au*pu+ad*pd+al*pl */                 
                movq           ( mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw      ( mm2,    mm7                             );// four left pixels */
                movq           ( mm6,    ebx+128                       );// ar */

                punpckhbw      ( mm3,    mm7                             );// four right pixels */                 
                paddw          ( mm4,    mm6                             );// au + ad + al + ar */
                
                pmullw         ( mm2,    mm6                             );// pr * ar */
                movq           ( mm6,    ebx+136               );// ar */

                paddw          ( mm0,    mm2                             );// au*pu+ad*pd+al*pl+pr*ar */
                paddw          ( mm5,    mm6                             );// au+ad+al+ar */
                
                pmullw         ( mm3,    mm6                             );// ar*pr */
                movq           ( mm2,    esi                   );// p */

                paddw          ( mm1,    mm3                             );// au*pu+ad*pd+al*pl+ar*pr */
                movq           ( mm3,    mm2                             );// make a copy of the pixel */
                
                // mm0, mm1 ---  au*pu+ad*pd+al*pl+ar*pr */
                // mm4, mm5     ---      au + ad + al + ar */
                
                punpcklbw  (     mm2,    mm7                             );// left four pixels */
                movq       (     mm6,    Four128s                );// 0080  0080 0080 0080 */

                punpckhbw  (     mm3,    mm7                             );// right four pixels */
                psubw      (     mm6,    mm4                             );// 128-(au+ad+al+ar) */
                
                pmullw     (     mm2,    mm6                             );// p*(128-(au+ad+al+ar)) */
                movq       (     mm6,    Four128s                );// 0080  0080 0080 0080 */

                paddw      (     mm0,    mm2                             );// sum */
                psubw      (     mm6,    mm5                             );// 128-(au+ad+al+ar) */
                
                pmullw     (     mm3,    mm6                             );// p*(128-(au+ad+al+ar)) */ 
                movq       (     mm6,    Four64s                 );// {64, 64, 64, 64 } */

                movq       (     mm7,    mm6                             );// {64, 64, 64, 64} */
                paddw      (     mm0,    mm6                             );// sum+B */

                paddw      (     mm1,    mm3                             );// sum */
                psllw      (     mm7,    8                               );// {16384, .. } */

                paddw      (     mm0,    mm7                             );// clamping */
                paddw      (     mm1,    mm6                             );// sum+B */

                paddw      (     mm1,    mm7                             );// clamping */
                psubusw    (     mm0,    mm7                             );// clamping */

                psubusw    (     mm1,    mm7                             );// clamping */
                psrlw      (     mm0,    7                               );// (sum+B)>>7 */

                psrlw      (     mm1,    7                               );// (sum+B)>>7 */
                packuswb   (     mm0,    mm1                             );// pack to 8 bytes */
                
                movq       (     edi,  mm0                             );// write to destination */
                           
                esi+=    ecx                             ;// Src += Pitch */
                edi+=    ecx                             ;// Des += Pitch */

                eax+=    16                              ;// UDPointer += 8 */
        ebx+=    16              ;// LPointer +=8 */
                

        // Fifth Row */

                movq           ( mm0,    esi+edx               );// mm0 = Pixels above */
                pxor           ( mm7,    mm7                             );// clear mm7 */

                movq           ( mm1,    mm0                             );// make a copy of mm0 */                        
                punpcklbw      ( mm0,    mm7                             );// lower four pixels */
                
                movq           ( mm4,    eax                   );// au */
                punpckhbw      ( mm1,    mm7                             );// high four pixels */
                
                movq           ( mm5,    eax+8                 );// au */
                               
                pmullw         ( mm0,    mm4                             );// pu*au */
                movq           ( mm2,    esi+ecx               );// mm2 = pixels below */
                
                pmullw         ( mm1,    mm5                             );// pu*au */
                movq           ( mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw      ( mm2,    mm7                             );// lower four */
                movq           ( mm6,    eax+16                );// ad */

                punpckhbw      ( mm3,    mm7                             );// higher four */                       
                paddw          ( mm4,    mm6                             );// au+ad */
                
                pmullw         ( mm2,    mm6                             );// au*pu+ad*pd */
                movq           ( mm6,    eax+24                );// ad */

                paddw          ( mm0,    mm2                     );
                paddw          ( mm5,    mm6                             );// au+ad */
                
                pmullw         ( mm3,    mm6                             );// ad*pd */
                movq           ( mm2,    esi-1                 );// pixel to the left */

                paddw          ( mm1,    mm3                             );// au*pu+ad*pd */
                movq           ( mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw      ( mm2,    mm7                             );// four left pixels */
                movq           ( mm6,    ebx                   );// al */

                punpckhbw      ( mm3,    mm7                             );// four right pixels */
                paddw          ( mm4,    mm6                             );// au + ad + al */
                
                pmullw         ( mm2,    mm6                             );// pl * al */
                movq           ( mm6,    ebx+8                 );// al */

                paddw          ( mm0,    mm2                             );// au*pu+ad*pd+al*pl */
                paddw          ( mm5,    mm6                             );// au+ad+al */
                
                pmullw         ( mm3,    mm6                             );// al*pl */
                movq           ( mm2,    esi+1                 );// pixel to the right */

                paddw          ( mm1,    mm3                             );// au*pu+ad*pd+al*pl */                 
                movq           ( mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw      ( mm2,    mm7                             );// four left pixels */
                movq           ( mm6,    ebx+128                       );// ar */

                punpckhbw      ( mm3,    mm7                             );// four right pixels */                 
                paddw          ( mm4,    mm6                             );// au + ad + al + ar */
                
                pmullw         ( mm2,    mm6                             );// pr * ar */
                movq           ( mm6,    ebx+136               );// ar */

                paddw          ( mm0,    mm2                             );// au*pu+ad*pd+al*pl+pr*ar */
                paddw          ( mm5,    mm6                             );// au+ad+al+ar */
                
                pmullw         ( mm3,    mm6                             );// ar*pr */
                movq           ( mm2,    esi                   );// p */

                paddw          ( mm1,    mm3                             );// au*pu+ad*pd+al*pl+ar*pr */
                movq           ( mm3,    mm2                             );// make a copy of the pixel */
                
                // mm0, mm1 ---  au*pu+ad*pd+al*pl+ar*pr */
                // mm4, mm5     ---      au + ad + al + ar */
                
                punpcklbw     (  mm2,    mm7                             );// left four pixels */
                movq          (  mm6,    Four128s                );// 0080  0080 0080 0080 */

                punpckhbw     (  mm3,    mm7                             );// right four pixels */
                psubw         (  mm6,    mm4                             );// 128-(au+ad+al+ar) */
                
                pmullw        (  mm2,    mm6                             );// p*(128-(au+ad+al+ar)) */
                movq          (  mm6,    Four128s                );// 0080  0080 0080 0080 */

                paddw         (  mm0,    mm2                             );// sum */
                psubw         (  mm6,    mm5                             );// 128-(au+ad+al+ar) */
                
                pmullw        (  mm3,    mm6                             );// p*(128-(au+ad+al+ar)) */ 
                movq          (  mm6,    Four64s                 );// {64, 64, 64, 64 } */

                movq          (  mm7,    mm6                             );// {64, 64, 64, 64} */
                paddw         (  mm0,    mm6                             );// sum+B */

                paddw         (  mm1,    mm3                             );// sum */
                psllw         (  mm7,    8                               );// {16384, .. } */

                paddw         (  mm0,    mm7                             );// clamping */
                paddw         (  mm1,    mm6                             );// sum+B */

                paddw         (  mm1,    mm7                             );// clamping */
                psubusw       (  mm0,    mm7                             );// clamping */

                psubusw       (  mm1,    mm7                             );// clamping */
                psrlw         (  mm0,    7                               );// (sum+B)>>7 */

                psrlw         (  mm1,    7                               );// (sum+B)>>7 */
                packuswb      (  mm0,    mm1                             );// pack to 8 bytes */
                
                movq          (  edi,  mm0                             );// write to destination */
                              
                esi+=    ecx                             ;// Src += Pitch */
                edi+=    ecx                             ;// Des += Pitch */

                eax+=    16                              ;// UDPointer += 8 */
        ebx+=    16              ;// LPointer +=8 */
                

        // Sixth Row */

                movq         (   mm0,    esi+edx               );// mm0 = Pixels above */
                pxor         (   mm7,    mm7                             );// clear mm7 */

                movq         (   mm1,    mm0                             );// make a copy of mm0 */                        
                punpcklbw    (   mm0,    mm7                             );// lower four pixels */
                
                movq         (   mm4,    eax                   );// au */
                punpckhbw    (   mm1,    mm7                             );// high four pixels */
                
                movq         (   mm5,    eax+8                 );// au */
                             
                pmullw       (   mm0,    mm4                             );// pu*au */
                movq         (   mm2,    esi+ecx               );// mm2 = pixels below */
                
                pmullw       (   mm1,    mm5                             );// pu*au */
                movq         (   mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw    (   mm2,    mm7                             );// lower four */
                movq         (   mm6,    eax+16                );// ad */

                punpckhbw    (   mm3,    mm7                             );// higher four */                       
                paddw        (   mm4,    mm6                             );// au+ad */
                
                pmullw       (   mm2,    mm6                             );// au*pu+ad*pd */
                movq         (   mm6,    eax+24                );// ad */

                paddw        (   mm0,    mm2                     );
                paddw        (   mm5,    mm6                             );// au+ad */
                
                pmullw        (  mm3,    mm6                             );// ad*pd */
                movq          (  mm2,    esi-1                 );// pixel to the left */

                paddw         (  mm1,    mm3                             );// au*pu+ad*pd */
                movq          (  mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );// four left pixels */
                movq          (  mm6,    ebx                   );// al */

                punpckhbw     (  mm3,    mm7                             );// four right pixels */
                paddw         (  mm4,    mm6                             );// au + ad + al */
                
                pmullw        (  mm2,    mm6                             );// pl * al */
                movq          (  mm6,    ebx+8                 );// al */

                paddw         (  mm0,    mm2                             );// au*pu+ad*pd+al*pl */
                paddw         (  mm5,    mm6                             );// au+ad+al */
                
                pmullw        (  mm3,    mm6                             );// al*pl */
                movq          (  mm2,    esi+1                 );// pixel to the right */

                paddw         (  mm1,    mm3                             );// au*pu+ad*pd+al*pl */                 
                movq          (  mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );// four left pixels */
                movq          (  mm6,    ebx+128                       );// ar */

                punpckhbw     (  mm3,    mm7                             );// four right pixels */                 
                paddw         (  mm4,    mm6                             );// au + ad + al + ar */
                
                pmullw        (  mm2,    mm6                             );// pr * ar */
                movq          (  mm6,    ebx+136               );// ar */

                paddw         (  mm0,    mm2                             );// au*pu+ad*pd+al*pl+pr*ar */
                paddw         (  mm5,    mm6                             );// au+ad+al+ar */
                
                pmullw        (  mm3,    mm6                             );// ar*pr */
                movq          (  mm2,    esi                   );// p */

                paddw         (  mm1,    mm3                             );// au*pu+ad*pd+al*pl+ar*pr */
                movq          (  mm3,    mm2                             );// make a copy of the pixel */
                
                // mm0, mm1 ---  au*pu+ad*pd+al*pl+ar*pr */
                // mm4, mm5     ---      au + ad + al + ar */
                
                punpcklbw    (   mm2,    mm7                             );// left four pixels */
                movq         (   mm6,    Four128s                );// 0080  0080 0080 0080 */

                punpckhbw    (   mm3,    mm7                             );// right four pixels */
                psubw        (   mm6,    mm4                             );// 128-(au+ad+al+ar) */
                
                pmullw       (   mm2,    mm6                             );// p*(128-(au+ad+al+ar)) */
                movq         (   mm6,    Four128s                );// 0080  0080 0080 0080 */

                paddw        (   mm0,    mm2                             );// sum */
                psubw        (   mm6,    mm5                             );// 128-(au+ad+al+ar) */
                
                pmullw       (   mm3,    mm6                             );// p*(128-(au+ad+al+ar)) */ 
                movq         (   mm6,    Four64s                 );// {64, 64, 64, 64 } */

                movq         (   mm7,    mm6                             );// {64, 64, 64, 64} */
                paddw        (   mm0,    mm6                             );// sum+B */

                paddw        (   mm1,    mm3                             );// sum */
                psllw        (   mm7,    8                               );// {16384, .. } */

                paddw        (   mm0,    mm7                             );// clamping */
                paddw        (   mm1,    mm6                             );// sum+B */

                paddw        (   mm1,    mm7                             );// clamping */
                psubusw      (   mm0,    mm7                             );// clamping */

                psubusw      (   mm1,    mm7                             );// clamping */
                psrlw        (   mm0,    7                               );// (sum+B)>>7 */

                psrlw        (   mm1,    7                               );// (sum+B)>>7 */
                packuswb     (   mm0,    mm1                             );// pack to 8 bytes */
                
                movq         (   edi,  mm0                             );// write to destination */
                             
                esi+=    ecx                             ;// Src += Pitch */
                edi+=    ecx                             ;// Des += Pitch */

                eax+=    16                              ;// UDPointer += 8 */
        ebx+=    16              ;// LPointer +=8 */
                

        // Seventh Row */

                movq           ( mm0,    esi+edx               );// mm0 = Pixels above */
                pxor           ( mm7,    mm7                             );// clear mm7 */

                movq           ( mm1,    mm0                             );// make a copy of mm0 */                        
                punpcklbw      ( mm0,    mm7                             );// lower four pixels */
                
                movq           ( mm4,    eax                   );// au */
                punpckhbw      ( mm1,    mm7                             );// high four pixels */
                
                movq           ( mm5,    eax+8                 );// au */
                               
                pmullw         ( mm0,    mm4                             );// pu*au */
                movq           ( mm2,    esi+ecx               );// mm2 = pixels below */
                
                pmullw         ( mm1,    mm5                             );// pu*au */
                movq           ( mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw      ( mm2,    mm7                             );// lower four */
                movq           ( mm6,    eax+16                );// ad */

                punpckhbw      ( mm3,    mm7                             );// higher four */                       
                paddw          ( mm4,    mm6                             );// au+ad */
                
                pmullw         ( mm2,    mm6                             );// au*pu+ad*pd */
                movq           ( mm6,    eax+24                );// ad */

                paddw          ( mm0,    mm2                     );
                paddw          ( mm5,    mm6                             );// au+ad */
                
                pmullw         ( mm3,    mm6                             );// ad*pd */
                movq           ( mm2,    esi-1                 );// pixel to the left */

                paddw          ( mm1,    mm3                             );// au*pu+ad*pd */
                movq           ( mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw      ( mm2,    mm7                             );// four left pixels */
                movq           ( mm6,    ebx                   );// al */

                punpckhbw      ( mm3,    mm7                             );// four right pixels */
                paddw          ( mm4,    mm6                             );// au + ad + al */
                
                pmullw         ( mm2,    mm6                             );// pl * al */
                movq           ( mm6,    ebx+8                 );// al */

                paddw          ( mm0,    mm2                             );// au*pu+ad*pd+al*pl */
                paddw          ( mm5,    mm6                             );// au+ad+al */
                
                pmullw         ( mm3,    mm6                             );// al*pl */
                movq           ( mm2,    esi+1                 );// pixel to the right */

                paddw          ( mm1,    mm3                             );// au*pu+ad*pd+al*pl */                 
                movq           ( mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw      ( mm2,    mm7                             );// four left pixels */
                movq           ( mm6,    ebx+128                       );// ar */

                punpckhbw      ( mm3,    mm7                             );// four right pixels */                 
                paddw          ( mm4,    mm6                             );// au + ad + al + ar */
                
                pmullw         ( mm2,    mm6                             );// pr * ar */
                movq           ( mm6,    ebx+136               );// ar */

                paddw          ( mm0,    mm2                             );// au*pu+ad*pd+al*pl+pr*ar */
                paddw          ( mm5,    mm6                             );// au+ad+al+ar */
                
                pmullw         ( mm3,    mm6                             );// ar*pr */
                movq           ( mm2,    esi                   );// p */

                paddw          ( mm1,    mm3                             );// au*pu+ad*pd+al*pl+ar*pr */
                movq           ( mm3,    mm2                             );// make a copy of the pixel */
                
                // mm0, mm1 ---  au*pu+ad*pd+al*pl+ar*pr */
                // mm4, mm5     ---      au + ad + al + ar */
                
                punpcklbw       (mm2,    mm7                             );// left four pixels */
                movq            (mm6,    Four128s                );// 0080  0080 0080 0080 */

                punpckhbw       (mm3,    mm7                             );// right four pixels */
                psubw           (mm6,    mm4                             );// 128-(au+ad+al+ar) */
                
                pmullw          (mm2,    mm6                             );// p*(128-(au+ad+al+ar)) */
                movq            (mm6,    Four128s                );// 0080  0080 0080 0080 */

                paddw           (mm0,    mm2                             );// sum */
                psubw           (mm6,    mm5                             );// 128-(au+ad+al+ar) */
                
                pmullw          (mm3,    mm6                             );// p*(128-(au+ad+al+ar)) */ 
                movq            (mm6,    Four64s                 );// {64, 64, 64, 64 } */

                movq            (mm7,    mm6                             );// {64, 64, 64, 64} */
                paddw           (mm0,    mm6                             );// sum+B */

                paddw           (mm1,    mm3                             );// sum */
                psllw           (mm7,    8                               );// {16384, .. } */

                paddw           (mm0,    mm7                             );// clamping */
                paddw           (mm1,    mm6                             );// sum+B */

                paddw           (mm1,    mm7                             );// clamping */
                psubusw         (mm0,    mm7                             );// clamping */

                psubusw         (mm1,    mm7                             );// clamping */
                psrlw           (mm0,    7                               );// (sum+B)>>7 */

                psrlw           (mm1,    7                               );// (sum+B)>>7 */
                packuswb        (mm0,    mm1                             );// pack to 8 bytes */
                
                movq            (edi,  mm0                             );// write to destination */
                                
                esi+=    ecx                             ;// Src += Pitch */
                edi+=    ecx                             ;// Des += Pitch */

                eax+=    16                              ;// UDPointer += 8 */
        ebx+=    16              ;// LPointer +=8 */
                
        // Eighth Row */

                movq          (  mm0,    esi+edx               );// mm0 = Pixels above */
                pxor          (  mm7,    mm7                             );// clear mm7 */

                movq          (  mm1,    mm0                             );// make a copy of mm0 */                        
                punpcklbw     (  mm0,    mm7                             );// lower four pixels */
                
                movq          (  mm4,    eax                   );// au */
                punpckhbw     (  mm1,    mm7                             );// high four pixels */
                
                movq          (  mm5,    eax+8                 );// au */
                              
                pmullw        (  mm0,    mm4                             );// pu*au */
                movq          (  mm2,    esi+ecx               );// mm2 = pixels below */
                
                pmullw        (  mm1,    mm5                             );// pu*au */
                movq          (  mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );// lower four */
                movq          (  mm6,    eax+16                );// ad */

                punpckhbw     (  mm3,    mm7                             );// higher four */                       
                paddw         (  mm4,    mm6                             );// au+ad */
                
                pmullw        (  mm2,    mm6                             );// au*pu+ad*pd */
                movq          (  mm6,    eax+24                );// ad */

                paddw         (  mm0,    mm2                     );
                paddw         (  mm5,    mm6                             );// au+ad */
                
                pmullw        (  mm3,    mm6                             );// ad*pd */
                movq          (  mm2,    esi-1                 );// pixel to the left */

                paddw         (  mm1,    mm3                             );// au*pu+ad*pd */
                movq          (  mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );// four left pixels */
                movq          (  mm6,    ebx                   );// al */

                punpckhbw     (  mm3,    mm7                             );// four right pixels */
                paddw         (  mm4,    mm6                             );// au + ad + al */
                
                pmullw        (  mm2,    mm6                             );// pl * al */
                movq          (  mm6,    ebx+8                 );// al */

                paddw         (  mm0,    mm2                             );// au*pu+ad*pd+al*pl */
                paddw         (  mm5,    mm6                             );// au+ad+al */
                
                pmullw        (  mm3,    mm6                             );// al*pl */
                movq          (  mm2,    esi+1                 );// pixel to the right */

                paddw         (  mm1,    mm3                             );// au*pu+ad*pd+al*pl */                 
                movq          (  mm3,    mm2                             );// make a copy of mm2 */
                
                punpcklbw     (  mm2,    mm7                             );// four left pixels */
                movq          (  mm6,    ebx+128                       );// ar */

                punpckhbw     (  mm3,    mm7                             );// four right pixels */                 
                paddw         (  mm4,    mm6                             );// au + ad + al + ar */
                
                pmullw        (  mm2,    mm6                             );// pr * ar */
                movq          (  mm6,    ebx+136               );// ar */

                paddw         (  mm0,    mm2                             );// au*pu+ad*pd+al*pl+pr*ar */
                paddw         (  mm5,    mm6                             );// au+ad+al+ar */
                
                pmullw        (  mm3,    mm6                             );// ar*pr */
                movq          (  mm2,    esi                   );// p */

                paddw         (  mm1,    mm3                             );// au*pu+ad*pd+al*pl+ar*pr */
                movq          (  mm3,    mm2                             );// make a copy of the pixel */
                
                // mm0, mm1 ---  au*pu+ad*pd+al*pl+ar*pr */
                // mm4, mm5     ---      au + ad + al + ar */
                
                punpcklbw      ( mm2,    mm7                             );// left four pixels */
                movq           ( mm6,    Four128s                );// 0080  0080 0080 0080 */

                punpckhbw      ( mm3,    mm7                             );// right four pixels */
                psubw          ( mm6,    mm4                             );// 128-(au+ad+al+ar) */
                
                pmullw         ( mm2,    mm6                             );// p*(128-(au+ad+al+ar)) */
                movq           ( mm6,    Four128s                );// 0080  0080 0080 0080 */

                paddw          ( mm0,    mm2                             );// sum */
                psubw          ( mm6,    mm5                             );// 128-(au+ad+al+ar) */
                
                pmullw         ( mm3,    mm6                             );// p*(128-(au+ad+al+ar)) */ 
                movq           ( mm6,    Four64s                 );// {64, 64, 64, 64 } */

                movq           ( mm7,    mm6                             );// {64, 64, 64, 64} */
                paddw          ( mm0,    mm6                             );// sum+B */

                paddw          ( mm1,    mm3                             );// sum */
                psllw          ( mm7,    8                               );// {16384, .. } */

                paddw          ( mm0,    mm7                             );// clamping */
                paddw          ( mm1,    mm6                             );// sum+B */

                paddw          ( mm1,    mm7                             );// clamping */
                psubusw        ( mm0,    mm7                             );// clamping */

                psubusw        ( mm1,    mm7                             );// clamping */
                psrlw          ( mm0,    7                               );// (sum+B)>>7 */

                psrlw          ( mm1,    7                               );// (sum+B)>>7 */
                packuswb       ( mm0,    mm1                             );// pack to 8 bytes */
                
                movq           ( edi,  mm0                             );// write to destination */

}



