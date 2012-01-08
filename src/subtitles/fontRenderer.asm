;/****************************************************************************
; *
; *  subtitle renderer
; *
; * Copyright (c) 2007-2011 h.yamagata
; *
; * This program is free software; you can redistribute it and/or modify
; * it under the terms of the GNU General Public License as published by
; * the Free Software Foundation; either version 2 of the License, or
; * (at your option) any later version.
; *
; * This program is distributed in the hope that it will be useful,
; * but WITHOUT ANY WARRANTY; without even the implied warranty of
; * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; * GNU General Public License for more details.
; *
; * You should have received a copy of the GNU General Public License
; * along with this program; if not, write to the Free Software
; * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
; *
; ***************************************************************************/

%ifidn __OUTPUT_FORMAT__,win64
  %define ptrsize 8
  %define reg_ax rax
  %define reg_bx rbx
  %define reg_cx rcx
  %define reg_dx rdx
  %define reg_si rsi
  %define reg_di rdi
  %define reg_sp rsp
%else
  %define ptrsize 4
  %define reg_ax eax
  %define reg_bx ebx
  %define reg_cx ecx
  %define reg_dx edx
  %define reg_si esi
  %define reg_di edi
  %define reg_sp esp
%endif

%macro cglobal 1
    %ifdef PREFIX
        global _%1
        %define %1 _%1
    %else
        global %1
    %endif
%endmacro

;=============================================================================
; Data
;=============================================================================
SECTION .data
ALIGN 16
cglobal fontMaskConstants
fontMaskConstants:
msk00ff  dd 0x00ff00ff,0x00ff00ff,0x00ff00ff,0x00ff00ff
mskffff  dd 0xffffffff,0xffffffff,0xffffffff,0xffffffff
msk8080  dd 0x80808080,0x80808080,0x80808080,0x80808080

%ifidn __OUTPUT_FORMAT__,win64
  %define mask00ff msk00ff wrt rip
  %define maskffff mskffff wrt rip
  %define mask8080 msk8080 wrt rip
%else
  %define mask00ff msk00ff
  %define maskffff mskffff
  %define mask8080 msk8080
%endif

;=============================================================================
; Code
;=============================================================================
SECTION .TEXT

;
; void YV12_lum2chr_min_mmx(const unsigned char* lum0,const unsigned char* lum1,unsigned char* chr);
;
cglobal YV12_lum2chr_min_mmx
YV12_lum2chr_min_mmx:
  movq      mm7, [mask00ff]
  movq      mm6, [maskffff]
  movq      mm5, [mask8080]
  mov       reg_cx, [reg_sp+(1+0)*ptrsize]  ; lum0
  mov       reg_dx, [reg_sp+(1+1)*ptrsize]  ; lum1
; horizontal compare line0
  movq      mm0, [reg_cx+8]
  movq      mm1, [reg_cx]
  movq      mm2, mm0
  movq      mm3, mm1
  psrlw     mm0, 8
  psrlw     mm1, 8
  packuswb  mm1, mm0
  psubb     mm1, mm5                        ; because pcmpgtb is signed.

  pand      mm2, mm7
  pand      mm3, mm7
  packuswb  mm3, mm2
  psubb     mm3, mm5

; compare mm1:mm3
  movq      mm0, mm1
  pcmpgtb   mm0, mm3
  pand      mm3, mm0
  pxor      mm0, mm6
  pand      mm1, mm0
  por       mm3, mm1

; horizontal compare line1
  movq      mm0, [reg_dx+8]
  movq      mm1, [reg_dx]
  movq      mm2, mm0
  movq      mm4, mm1
  psrlw     mm0, 8
  psrlw     mm1, 8
  packuswb  mm1, mm0
  psubb     mm1, mm5

  pand      mm2, mm7
  pand      mm4, mm7
  packuswb  mm4, mm2
  psubb     mm4, mm5

; compare mm1:mm4
  movq      mm0, mm1
  pcmpgtb   mm0, mm4
  pand      mm4, mm0
  pxor      mm0, mm6
  pand      mm1, mm0
  por       mm4, mm1

; vertical compare
; compare mm3:mm4
  movq      mm0, mm3
  pcmpgtb   mm0, mm4
  pand      mm3, mm0
  pxor      mm0, mm6
  pand      mm3, mm0
  por       mm4, mm3
  paddb     mm4, mm5

%ifidn __OUTPUT_FORMAT__,win64
  movq      [r8], mm4
%else
  mov       reg_dx, [reg_sp+(1+2)*ptrsize]  ; chr
  movq      [reg_dx], mm4
%endif

  ret

;
; void YV12_lum2chr_max_mmx( /* 0 rcx */ const unsigned char* lum0,
;                            /* 1 rdx */ const unsigned char* lum1,
;                            /* 2 r8  */ unsigned char* chr);
;
cglobal YV12_lum2chr_max_mmx
YV12_lum2chr_max_mmx:
  movq      mm7, [mask00ff]
  movq      mm6, [maskffff]
  movq      mm5, [mask8080]
  mov       reg_cx, [reg_sp+(1+0)*ptrsize]  ; lum0
  mov       reg_dx, [reg_sp+(1+1)*ptrsize]  ; lum1
; horizontal compare line0
  movq      mm0, [reg_cx+8]
  movq      mm1, [reg_cx]
  movq      mm2, mm0
  movq      mm3, mm1
  psrlw     mm0, 8
  psrlw     mm1, 8
  packuswb  mm1, mm0
  psubb     mm1, mm5                        ; because pcmpgtb is signed.

  pand      mm2, mm7
  pand      mm3, mm7
  packuswb  mm3, mm2
  psubb     mm3, mm5

; compare mm1:mm3
  movq      mm0, mm1
  pcmpgtb   mm0, mm3
  pand      mm1, mm0
  pxor      mm0, mm6
  pand      mm3, mm0
  por       mm3, mm1

; horizontal compare line1
  movq      mm0, [reg_dx+8]
  movq      mm1, [reg_dx]
  movq      mm2, mm0
  movq      mm4, mm1
  psrlw     mm0, 8
  psrlw     mm1, 8
  packuswb  mm1, mm0
  psubb     mm1, mm5

  pand      mm2, mm7
  pand      mm4, mm7
  packuswb  mm4, mm2
  psubb     mm4, mm5

; compare mm1:mm4
  movq      mm0, mm1
  pcmpgtb   mm0, mm4
  pand      mm1, mm0
  pxor      mm0, mm6
  pand      mm4, mm0
  por       mm4, mm1

; vertical compare
; compare mm3:mm4
  movq      mm0, mm3
  pcmpgtb   mm0, mm4
  pand      mm3, mm0
  pxor      mm0, mm6
  pand      mm4, mm0
  por       mm4, mm3
  paddb     mm4, mm5

%ifidn __OUTPUT_FORMAT__,win64
  movq      [r8], mm4
%else
  mov       reg_dx, [reg_sp+(1+2)*ptrsize]  ; chr
  movq      [reg_dx], mm4
%endif

  ret

;
; void YV12_lum2chr_max_mmx2( /* 0 rcx */ const unsigned char* lum0,
;                             /* 1 rdx */ const unsigned char* lum1,
;                             /* 2 r8  */ unsigned char* chr);
;
cglobal YV12_lum2chr_max_mmx2
YV12_lum2chr_max_mmx2:
  movq      mm7, [mask00ff]
%ifidn __OUTPUT_FORMAT__,win64
%else
  mov       reg_cx, [reg_sp+(1+0)*ptrsize]  ; lum0
  mov       reg_dx, [reg_sp+(1+1)*ptrsize]  ; lum1
%endif
; horizontal compare line0
  movq      mm0, [reg_cx+8]
  movq      mm1, [reg_cx]
  movq      mm2, mm0
  movq      mm3, mm1
  psrlw     mm0, 8
  psrlw     mm1, 8
  packuswb  mm1, mm0

  pand      mm2, mm7
  pand      mm3, mm7
  packuswb  mm3, mm2

; compare mm1:mm3
  pmaxub    mm3, mm1

; horizontal compare line1
  movq      mm0, [reg_dx+8]
  movq      mm1, [reg_dx]
  movq      mm2, mm0
  movq      mm4, mm1
  psrlw     mm0, 8
  psrlw     mm1, 8
  packuswb  mm1, mm0

  pand      mm2, mm7
  pand      mm4, mm7
  packuswb  mm4, mm2

; compare mm1:mm4
  pmaxub    mm4, mm1

; vertical compare
; compare mm3:mm4
  pmaxub    mm4, mm3

%ifidn __OUTPUT_FORMAT__,win64
  movq      [r8], mm4
%else
  mov       reg_dx, [reg_sp+(1+2)*ptrsize]  ; chr
  movq      [reg_dx], mm4
%endif

  ret

;
; void YV12_lum2chr_min_mmx2( /* 0 rcx */ const unsigned char* lum0,
;                             /* 1 rdx */ const unsigned char* lum1,
;                             /* 2 r8  */ unsigned char* chr);
;
cglobal YV12_lum2chr_min_mmx2
YV12_lum2chr_min_mmx2:
  movq      mm7, [mask00ff]
%ifidn __OUTPUT_FORMAT__,win64
%else
  mov       reg_cx, [reg_sp+(1+0)*ptrsize]  ; lum0
  mov       reg_dx, [reg_sp+(1+1)*ptrsize]  ; lum1
%endif
; horizontal compare line0
  movq      mm0, [reg_cx+8]
  movq      mm1, [reg_cx]
  movq      mm2, mm0
  movq      mm3, mm1
  psrlw     mm0, 8
  psrlw     mm1, 8
  packuswb  mm1, mm0

  pand      mm2, mm7
  pand      mm3, mm7
  packuswb  mm3, mm2

; compare mm1:mm3
  pminub    mm3, mm1

; horizontal compare line1
  movq      mm0, [reg_dx+8]
  movq      mm1, [reg_dx]
  movq      mm2, mm0
  movq      mm4, mm1
  psrlw     mm0, 8
  psrlw     mm1, 8
  packuswb  mm1, mm0

  pand      mm2, mm7
  pand      mm4, mm7
  packuswb  mm4, mm2

; compare mm1:mm4
  pminub    mm4, mm1

; vertical compare
; compare mm3:mm4
  pminub    mm4, mm3

%ifidn __OUTPUT_FORMAT__,win64
  movq      [r8], mm4
%else
  mov       reg_dx, [reg_sp+(1+2)*ptrsize]  ; chr
  movq      [reg_dx], mm4
%endif

  ret

%ifidn __OUTPUT_FORMAT__,win64
;
; void  __cdecl storeXmmRegs(unsigned char* buf);
;
cglobal storeXmmRegs
storeXmmRegs:
  movdqu  [rcx+ 6*16], xmm6
  movdqu  [rcx+ 7*16], xmm7
  movdqu  [rcx+ 8*16], xmm8
  movdqu  [rcx+ 9*16], xmm9
  movdqu  [rcx+10*16], xmm10
  movdqu  [rcx+11*16], xmm11
  movdqu  [rcx+12*16], xmm12
  movdqu  [rcx+13*16], xmm13
  movdqu  [rcx+14*16], xmm14
  movdqu  [rcx+15*16], xmm15
  ret

;
; void  __cdecl restoreXmmRegs(unsigned char* buf);
;
cglobal restoreXmmRegs
restoreXmmRegs:
  movdqu  xmm6, [rcx+ 6*16]
  movdqu  xmm7, [rcx+ 7*16]
  movdqu  xmm8, [rcx+ 8*16]
  movdqu  xmm9, [rcx+ 9*16]
  movdqu  xmm10,[rcx+10*16]
  movdqu  xmm11,[rcx+11*16]
  movdqu  xmm12,[rcx+12*16]
  movdqu  xmm13,[rcx+13*16]
  movdqu  xmm14,[rcx+14*16]
  movdqu  xmm15,[rcx+15*16]
  ret

%endif ; win64

;
; void __cdecl fontRGB32toBW_mmx( /* 0 rcx */ size_t count,
;                                 /* 1 rdx */ unsigned char *ptr);
;
cglobal fontRGB32toBW_mmx
fontRGB32toBW_mmx:
%ifidn __OUTPUT_FORMAT__,win64
%else
  mov       reg_cx, [reg_sp+(1+0)*ptrsize]  ; count
  mov       reg_dx, [reg_sp+(1+1)*ptrsize]  ; ptr
%endif
  mov       reg_ax, reg_dx
.loop:
  movq      mm0, [reg_dx]
  movq      mm1, [reg_dx+8]
  movq      mm2, [reg_dx+16]
  movq      mm3, [reg_dx+24]
  psrld     mm0, 16
  psrld     mm1, 16
  psrld     mm2, 16
  psrld     mm3, 16
  packssdw  mm1, mm0
  packssdw  mm3, mm2
  packuswb  mm1, mm3
  movq      [reg_ax],mm1
  add       reg_ax,8
  add       reg_dx,32
  dec       reg_cx
  jnz       .loop
  ret

;
; unsigned int __cdecl fontPrepareOutline_sse2( /* 0 rcx */ const unsigned char *src,
;                                               /* 1 rdx */ size_t srcStrideGap,
;                                               /* 2 r8  */ const short *matrix,
;                                               /* 3 r9  */ size_t matrixSizeH,
;                                               /* 4     */ size_t matrixSizeV,
;                                               );
;

%ifidn __OUTPUT_FORMAT__,win64
 %define matrixSizeH r9
%else
 %define matrixSizeH [reg_sp+(3+3)*ptrsize]
%endif

cglobal fontPrepareOutline_sse2
fontPrepareOutline_sse2:
  push      reg_si
  push      reg_bx
%ifidn __OUTPUT_FORMAT__,win64
  mov       reg_si, r8
%else
  mov       reg_cx, [reg_sp+(3+0)*ptrsize]  ; src
  mov       reg_dx, [reg_sp+(3+1)*ptrsize]  ; srcStrideGap
  mov       reg_si, [reg_sp+(3+2)*ptrsize]  ; matrix
%endif
  pxor       xmm0, xmm0
  pxor       xmm3, xmm3
  mov       reg_bx, [reg_sp+(3+4)*ptrsize]  ; matrixSizeV
.yloop:
  mov       reg_ax, matrixSizeH
.xloop:
  movq      xmm1, [reg_cx]
  punpcklbw xmm1, xmm3
  movdqa    xmm2, [reg_si]
  pmaddwd   xmm2, xmm1
  paddd     xmm0, xmm2
  add       reg_cx,8
  add       reg_si,16
  dec       reg_ax
  jnz       .xloop

  add       reg_cx, reg_dx
  dec       reg_bx
  jnz       .yloop

  movdqa    xmm1, xmm0
  movdqa    xmm2, xmm0
  movdqa    xmm3, xmm0
  psrldq    xmm1, 4
  psrldq    xmm2, 8
  psrldq    xmm3, 12
  paddd     xmm0, xmm1
  paddd     xmm0, xmm2
  paddd     xmm0, xmm3
  movd      eax, xmm0

  pop       reg_bx
  pop       reg_si
  ret
%undef matrixSizeH

;
; unsigned int __cdecl fontPrepareOutline_mmx ( /* 0 rcx */ const unsigned char *src,
;                                               /* 1 rdx */ size_t srcStrideGap,
;                                               /* 2 r8  */ const short *matrix,
;                                               /* 3 r9  */ size_t matrixSizeH,
;                                               /* 4     */ size_t matrixSizeV,
;                                               /* 5     */ size_t matrixGap);
;

cglobal fontPrepareOutline_mmx
fontPrepareOutline_mmx:
  push      reg_si
  push      reg_bx
  mov       reg_cx, [reg_sp+(3+0)*ptrsize]  ; src
  mov       reg_dx, [reg_sp+(3+1)*ptrsize]  ; srcStrideGap
  mov       reg_si, [reg_sp+(3+2)*ptrsize]  ; matrix
  pxor       mm0, mm0
  pxor       mm3, mm3
  mov       reg_bx, [reg_sp+(3+4)*ptrsize]  ; matrixSizeV
.yloop:
  mov       reg_ax, [reg_sp+(3+3)*ptrsize]  ; matrixSizeH
.xloop:
  movd      mm1, [reg_cx]
  punpcklbw mm1, mm3
  movq      mm2, [reg_si]
  pmaddwd   mm2, mm1
  paddd     mm0, mm2
  add       reg_cx,4
  add       reg_si,8
  dec       reg_ax
  jnz       .xloop

  add       reg_si, [reg_sp+(3+5)*ptrsize]  ; matrixGap
  add       reg_cx, reg_dx
  dec       reg_bx
  jnz       .yloop

  movq      mm1, mm0
  psrlq     mm1, 32
  paddd     mm0, mm1
  movd      eax, mm0

  pop       reg_bx
  pop       reg_si
  ret
