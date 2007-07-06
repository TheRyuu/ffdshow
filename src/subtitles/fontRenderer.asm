;/****************************************************************************
; *
; *  subtitle renderer
; *
; * Copyright (c) 2007 h.yamagata
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
msk256   dd 0x01000100,0x01000100,0x01000100,0x01000100 ; 'DQ and DT do not accept numeric constants or string constants as operands.'
msk00ff  dd 0x00ff00ff,0x00ff00ff,0x00ff00ff,0x00ff00ff
mskffff  dd 0xffffffff,0xffffffff,0xffffffff,0xffffffff
msk8080  dd 0x80808080,0x80808080,0x80808080,0x80808080

struc        mask
  .mask256   resq 2
  .mask00ff  resq 2
  .maskffff  resq 2
endstruc

;=============================================================================
; Code
;=============================================================================
SECTION .TEXT

struc        colortbl
  .body_Y    resq 2
  .body_U    resq 2
  .body_V    resq 2
  .body_A    resq 2

  .outline_Y resq 2
  .outline_U resq 2
  .outline_V resq 2
  .outline_A resq 2

  .shadow_Y  resq 2
  .shadow_U  resq 2
  .shadow_V  resq 2
  .shadow_A  resq 2
endstruc

struc        temp_bos
  .b1:       resq 2           ; xmm8
  .b2:       resq 2           ; xmm9
  .o1:       resq 2           ; xmm10
  .o2:       resq 2           ; xmm11
  .s1:       resq 2           ; xmm12
  .s2:       resq 2           ; xmm13
endstruc

;
; void* (__cdecl TrenderedSubtitleWord_print_mmx)(/* 0 rcx */ const unsigned char* bmp,
;                                                 /* 1 rdx */ const unsigned char* outline,
;                                                 /* 2 r8  */ const unsigned char* shadow,
;                                                 /* 3 r9  */ const unsigned short* colortbl,
;                                                 /* 4     */ const unsigned char* dst,
;                                                 /* 5     */ const DWORD* fontmaskconstants);
;
cglobal TrenderedSubtitleWord_printY_mmx
TrenderedSubtitleWord_printY_mmx:

%define _movdqu movq
%define _movdqa movq
%define _mm0 mm0
%define _mm1 mm1
%define _mm2 mm2
%define _mm3 mm3
%define _mm4 mm4
%define _mm5 mm5
%define _mm6 mm6
%define _mm7 mm7

%macro printY 0
  push      reg_di
  push      reg_si

%ifidn __OUTPUT_FORMAT__,win64
  mov       [reg_sp+(3+1)*ptrsize], reg_dx  ; outline
  mov       reg_si, r9
  mov       reg_ax, r8
%else
  mov       reg_si, [reg_sp+(3+3)*ptrsize]  ; si = colortbl
  mov       reg_ax, [reg_sp+(3+2)*ptrsize]  ; ax = shadow
%endif
  mov       reg_di, [reg_sp+(3+4)*ptrsize]  ; di = dst
  mov       reg_dx, [reg_sp+(3+5)*ptrsize]  ; dx = fontMaskConstants
  _movdqa   _mm0, [reg_si + colortbl.shadow_A]
  _movdqa   _mm1, [reg_ax]
  _movdqa   _mm2, _mm1
  pxor      _mm3, _mm3
  punpckhbw _mm1, _mm3
  punpcklbw _mm2, _mm3
  pmullw    _mm1, _mm0
  psrlw     _mm1, 8
  pmullw    _mm2, _mm0
  psrlw     _mm2, 8           ; _mm1:_mm2 = s = m_shadowYUV.A *shadow [0][srcPos]>>8;

  _movdqa   _mm4, [reg_dx + mask.mask256]
  _movdqa   _mm5, _mm4
  psubw     _mm4, _mm1
  psubw     _mm5, _mm2         ; _mm4:_mm5 = (256-s)

  _movdqu   _mm6, [reg_di]
  _movdqa   _mm7, _mm6
  punpckhbw _mm6, _mm3
  punpcklbw _mm7, _mm3         ; _mm6;_mm7 = dstLn[0][dstPos]
  pmullw    _mm4, _mm6
  psrlw     _mm4, 8
  pmullw    _mm5, _mm7
  psrlw     _mm5, 8           ; _mm4:_mm5 = ((256-s)*dstLn[0][dstPos]>>8)

  _movdqa   _mm0, [reg_si + colortbl.shadow_Y]
  pmullw    _mm1, _mm0
  psrlw     _mm1, 8
  pmullw    _mm2, _mm0
  psrlw     _mm2, 8           ; _mm1:_mm2 = (s*m_shadowYUV.Y>>8)

  paddusb   _mm1,_mm4
  paddusb   _mm2,_mm5          ; _mm1:_mm2 = d =((256-s)*dstLn[0][dstPos]>>8)+(s*m_shadowYUV.Y>>8);

%ifidn __OUTPUT_FORMAT__,win64
  mov       reg_ax, reg_cx                  ; bmp
%else
  mov       reg_ax, [reg_sp+(3+0)*ptrsize]  ; bmp
%endif
  _movdqa   _mm0, [reg_si + colortbl.body_A]
  _movdqa   _mm4, [reg_ax]
  _movdqa   _mm5, _mm4
  punpckhbw _mm4, _mm3
  punpcklbw _mm5, _mm3
  pmullw    _mm4, _mm0
  psrlw     _mm4, 8
  pmullw    _mm5, _mm0
  psrlw     _mm5, 8           ; _mm4:_mm5 = b= m_bodyYUV.A * bmp[0][srcPos]>>8;

  _movdqa   _mm6, [reg_dx+ mask.mask256]
  _movdqa   _mm7, _mm6
  psubw     _mm6, _mm4
  psubw     _mm7, _mm5         ; _mm6:_mm7 = (256-b)

  pmullw    _mm6, _mm1
  psrlw     _mm6, 8
  pmullw    _mm7, _mm2
  psrlw     _mm7, 8           ; _mm6:_mm7 = ((256-b)*d>>8)

  _movdqa   _mm0, [reg_si + colortbl.body_Y]
  pmullw    _mm4, _mm0
  psrlw     _mm4, 8
  pmullw    _mm5, _mm0
  psrlw     _mm5, 8           ; _mm4:_mm5 = (b*m_bodyYUV.Y>>8)

  paddusb   _mm4, _mm6
  paddusb   _mm5, _mm7         ; _mm4:_mm5 = d = ((256-b)*d>>8)+(b*m_bodyYUV.Y>>8);

  mov       reg_ax, [reg_sp+(3+1)*ptrsize]  ; outline
  _movdqa   _mm0, [reg_si + colortbl.outline_A]
  _movdqa   _mm1, [reg_ax]
  _movdqa   _mm2, _mm1
  punpckhbw _mm1, _mm3
  punpcklbw _mm2, _mm3
  pmullw    _mm1, _mm0
  psrlw     _mm1, 8
  pmullw    _mm2, _mm0
  psrlw     _mm2, 8           ; _mm1:_mm2 = o = m_outlineYUV.A * outline[0][srcPos]>>8;

  _movdqa   _mm6, [reg_dx+ mask.mask256]
  _movdqa   _mm7, _mm6
  psubw     _mm6, _mm1
  psubw     _mm7, _mm2         ; _mm6:_mm7 = (256-o)

  pmullw    _mm6, _mm4
  psrlw     _mm6, 8
  pmullw    _mm7, _mm5
  psrlw     _mm7, 8           ; _mm6:_mm7 = ((256-o)*d>>8)

  _movdqa   _mm0, [reg_si + colortbl.outline_Y]
  pmullw    _mm1, _mm0
  psrlw     _mm1, 8
  pmullw    _mm2, _mm0
  psrlw     _mm2, 8           ; _mm1:_mm2 = (o*m_outlineYUV.Y>>8)

  paddusb   _mm1, _mm6
  paddusb   _mm2, _mm7         ; _mm1:_mm2 = ((256-o)*d>>8)+(o*m_outlineYUV.Y>>8);

  packuswb  _mm2, _mm1
  _movdqu   [reg_di],_mm2

  pop       reg_si
  pop       reg_di
  ret
%endmacro

%ifidn __OUTPUT_FORMAT__,win64
%else
  printY
%endif

;
; void* (__cdecl TrenderedSubtitleWord_printUV_mmx)( /* 0 rcx */ const unsigned char* bmp,
;                                                    /* 1 rdx */ const unsigned char* outline,
;                                                    /* 2 r8  */ const unsigned char* shadow,
;                                                    /* 3 r9  */ const unsigned short* colortbl,
;                                                    /* 4     */ const unsigned char* dstU,
;                                                    /* 5     */ const unsigned char* dstV,
;                                                    /* 6     */ const DWORD* fontmaskconstants);
;
; mmx version is not available for WIN64
;
cglobal TrenderedSubtitleWord_printUV_mmx
TrenderedSubtitleWord_printUV_mmx:
%macro printUV 0
  push      reg_di
  push      reg_si
  push      reg_bx
  mov       reg_bx, reg_sp
%ifidn __OUTPUT_FORMAT__,win64
  mov       [reg_bx+(4+2)*ptrsize], reg_dx
  mov       rsi, r9
%else
  sub       esp, temp_bos_size
  and       esp, 0xfffffff0                 ; To align for SSE2.
  mov       esi, [reg_bx+(4+3)*ptrsize]     ; si = colortbl
%endif
  mov       reg_dx, [reg_bx+(4+6)*ptrsize]  ; dx = fontMaskConstants

; U
  mov       reg_di, [reg_bx+(4+4)*ptrsize]  ; di = dst1. (4+4) means: First 4 is pcr,reg_di,reg_si,reg_bx. Second 4 means it's 5th (count from 0) parameter.
%ifidn __OUTPUT_FORMAT__,win64
  _movdqa   _mm1, [r8]
%else
  mov       eax, [reg_bx+(4+2)*ptrsize]  ; ax = shadow
  _movdqa   _mm1, [eax]
%endif
  _movdqa   _mm0, [reg_si + colortbl.shadow_A]  ; _movdqa = movq for MMX.
  _movdqa   _mm2, _mm1
  pxor      _mm3, _mm3
  punpckhbw _mm1, _mm3
  punpcklbw _mm2, _mm3
  pmullw    _mm1, _mm0
  psrlw     _mm1, 8
%ifdef HAVE_LOCAL_SSE2_x64
  movdqa    xmm12, xmm1
%else
  _movdqa   [reg_sp + temp_bos.s1], _mm1
%endif
  pmullw    _mm2, _mm0
  psrlw     _mm2, 8                         ; _mm1:_mm2 = s =  = m_shadowYUV.A *shadow [1][srcPos1]>>8;
%ifdef HAVE_LOCAL_SSE2_x64
  movdqa    xmm13, xmm2
%else
  _movdqa   [reg_sp + temp_bos.s2], _mm2
%endif

  _movdqa   _mm4, [reg_dx+ mask.mask256]
  _movdqa   _mm5, _mm4
  psubw     _mm4, _mm1
  psubw     _mm5, _mm2                      ; _mm4:_mm5 = (256-s)

  _movdqu   _mm6, [reg_di]
  _movdqa   _mm7, _mm6
  punpckhbw _mm6, _mm3
  punpcklbw _mm7, _mm3                      ; _mm6;_mm7 = dstLn[1][dstPos]
  pmullw    _mm4, _mm6
  psrlw     _mm4, 8
  pmullw    _mm5, _mm7
  psrlw     _mm5, 8                         ; _mm4:_mm5 = ((256-s)*dstLn[1][dstPos]>>8)

  _movdqa   _mm0, [reg_si + colortbl.shadow_U]
  pmullw    _mm1, _mm0
  psrlw     _mm1, 8
  pmullw    _mm2, _mm0
  psrlw     _mm2, 8                         ; _mm1:_mm2 = (s*m_shadowYUV.U>>8)

  paddusb    _mm1,_mm4
  paddusb    _mm2,_mm5                      ; _mm1:_mm2 = d =((256-s)*dstLn[1][dstPos]>>8)+(s*m_shadowYUV.U>>8);

%ifidn __OUTPUT_FORMAT__,win64
%else
  mov       ecx, [reg_bx+(4+0)*ptrsize]     ; bmp
%endif
  _movdqa   _mm0, [reg_si + colortbl.body_A]
  _movdqa   _mm4, [reg_cx]
  _movdqa   _mm5, _mm4
  punpckhbw _mm4, _mm3
  punpcklbw _mm5, _mm3
  pmullw    _mm4, _mm0
  psrlw     _mm4, 8
%ifdef HAVE_LOCAL_SSE2_x64
  movdqa    xmm8, xmm4
%else
  _movdqa   [reg_sp + temp_bos.b1], _mm4
%endif
  pmullw    _mm5, _mm0
  psrlw     _mm5, 8                         ; _mm4:_mm5 = b= m_bodyYUV.A * bmp[1][srcPos]>>8;
%ifdef HAVE_LOCAL_SSE2_x64
  movdqa    xmm9, xmm5
%else
  _movdqa   [reg_sp + temp_bos.b2], _mm5
%endif

  _movdqa   _mm6, [reg_dx+ mask.mask256]
  _movdqa   _mm7, _mm6
  psubw     _mm6, _mm4
  psubw     _mm7, _mm5                      ; _mm6:_mm7 = (256-b)

  pmullw    _mm6, _mm1
  psrlw     _mm6, 8
  pmullw    _mm7, _mm2
  psrlw     _mm7, 8                         ; _mm6:_mm7 = ((256-b)*d>>8)

  _movdqa   _mm0, [reg_si + colortbl.body_U]
  pmullw    _mm4, _mm0
  psrlw     _mm4, 8
  pmullw    _mm5, _mm0
  psrlw     _mm5, 8                         ; _mm4:_mm5 = (b*m_bodyYUV.U>>8)

  paddusb    _mm4, _mm6
  paddusb    _mm5, _mm7                     ; _mm4:_mm5 = d = ((256-b)*d>>8)+(b*m_bodyYUV.U>>8);

  mov       reg_ax, [reg_bx+(4+1)*ptrsize]  ; outline
  _movdqa   _mm0, [reg_si + colortbl.outline_A]
  _movdqa   _mm1, [reg_ax]
  _movdqa   _mm2, _mm1
  punpckhbw _mm1, _mm3
  punpcklbw _mm2, _mm3
  pmullw    _mm1, _mm0
  psrlw     _mm1, 8
%ifdef HAVE_LOCAL_SSE2_x64
  movdqa    xmm10, xmm1
%else
  _movdqa   [reg_sp + temp_bos.o1], _mm1
%endif
  pmullw    _mm2, _mm0
  psrlw     _mm2, 8                         ; _mm1:_mm2 = o = m_outlineYUV.A * outline[1][srcPos]>>8;
%ifdef HAVE_LOCAL_SSE2_x64
  movdqa    xmm11, xmm2
%else
  _movdqa   [reg_sp + temp_bos.o2], _mm2
%endif

  _movdqa   _mm6, [reg_dx+ mask.mask256]
  _movdqa   _mm7, _mm6
  psubw     _mm6, _mm1
  psubw     _mm7, _mm2                      ; _mm6:_mm7 = (256-o)

  pmullw    _mm6, _mm4
  psrlw     _mm6, 8
  pmullw    _mm7, _mm5
  psrlw     _mm7, 8                         ; _mm6:_mm7 = ((256-o)*d>>8)

  _movdqa   _mm0, [reg_si + colortbl.outline_U]
  pmullw    _mm1, _mm0
  psrlw     _mm1, 8
  pmullw    _mm2, _mm0
  psrlw     _mm2, 8                         ; _mm1:_mm2 = (o*m_outlineYUV.U>>8)

  paddusb   _mm1, _mm6
  paddusb   _mm2, _mm7                      ; _mm1:_mm2 = ((256-o)*d>>8)+(o*m_outlineYUV.U>>8);

  packuswb  _mm2, _mm1
  _movdqu   [reg_di],_mm2

; V

  mov       reg_di, [reg_bx+(4+5)*ptrsize]  ; di = dst2
%ifdef HAVE_LOCAL_SSE2_x64
  movdqa    xmm1, xmm12
  movdqa    xmm2, xmm13
%else
  _movdqa   _mm1, [reg_sp + temp_bos.s1]
  _movdqa   _mm2, [reg_sp + temp_bos.s2]    ; _mm1:_mm2 = s = m_shadowYUV.A *shadow [1][srcPos1]>>8;
%endif

  _movdqa   _mm4, [reg_dx+ mask.mask256]
  _movdqa   _mm5, _mm4
  psubw     _mm4, _mm1
  psubw     _mm5, _mm2                      ; _mm4:_mm5 = (256-s)

  _movdqu   _mm6, [reg_di]
  _movdqa   _mm7, _mm6
  punpckhbw _mm6, _mm3
  punpcklbw _mm7, _mm3                      ; _mm6;_mm7 = dstLn[2][dstPos]
  pmullw    _mm4, _mm6
  psrlw     _mm4, 8
  pmullw    _mm5, _mm7
  psrlw     _mm5, 8                         ; _mm4:_mm5 = ((256-s)*dstLn[2][dstPos]>>8)

  _movdqa   _mm0, [reg_si + colortbl.shadow_V]
  pmullw    _mm1, _mm0
  psrlw     _mm1, 8
  pmullw    _mm2, _mm0
  psrlw     _mm2, 8                         ; _mm1:_mm2 = (s*m_shadowYUV.V>>8)

  paddusb   _mm1,_mm4
  paddusb   _mm2,_mm5                       ; _mm1:_mm2 = d =((256-s)*dstLn[2][dstPos]>>8)+(s*m_shadowYUV.V>>8);

%ifdef HAVE_LOCAL_SSE2_x64
  movdqa    xmm4, xmm8
  movdqa    xmm5, xmm9
%else
  _movdqa   _mm4, [reg_sp + temp_bos.b1]
  _movdqa   _mm5, [reg_sp + temp_bos.b2]    ; _mm4:_mm5 = b= m_bodyYUV.A * bmp[1][srcPos]>>8;
%endif

  _movdqa   _mm6, [reg_dx+ mask.mask256]
  _movdqa   _mm7, _mm6
  psubw     _mm6, _mm4
  psubw     _mm7, _mm5                      ; _mm6:_mm7 = (256-b)

  pmullw    _mm6, _mm1
  psrlw     _mm6, 8
  pmullw    _mm7, _mm2
  psrlw     _mm7, 8                         ; _mm6:_mm7 = ((256-b)*d>>8)

  _movdqa   _mm0, [reg_si + colortbl.body_V]
  pmullw    _mm4, _mm0
  psrlw     _mm4, 8
  pmullw    _mm5, _mm0
  psrlw     _mm5, 8                         ; _mm4:_mm5 = (b*m_bodyYUV.V>>8)

  paddusb   _mm4, _mm6
  paddusb   _mm5, _mm7                      ; _mm4:_mm5 = d = ((256-b)*d>>8)+(b*m_bodyYUV.V>>8);

%ifdef HAVE_LOCAL_SSE2_x64
  movdqa    xmm1, xmm10
  movdqa    xmm2, xmm11
%else
  _movdqa   _mm1, [reg_sp + temp_bos.o1]
  _movdqa   _mm2, [reg_sp + temp_bos.o2]    ; _mm1:_mm2 = o = m_outlineYUV.A * outline[1][srcPos]>>8;
%endif

  _movdqa   _mm6, [reg_dx+ mask.mask256]
  _movdqa   _mm7, _mm6
  psubw     _mm6, _mm1
  psubw     _mm7, _mm2                      ; _mm6:_mm7 = (256-o)

  pmullw    _mm6, _mm4
  psrlw     _mm6, 8
  pmullw    _mm7, _mm5
  psrlw     _mm7, 8                         ; _mm6:_mm7 = ((256-o)*d>>8)

  _movdqa   _mm0, [reg_si + colortbl.outline_V]
  pmullw    _mm1, _mm0
  psrlw     _mm1, 8
  pmullw    _mm2, _mm0
  psrlw     _mm2, 8                         ; _mm1:_mm2 = (o*m_outlineYUV.V>>8)

  paddusb   _mm1, _mm6
  paddusb   _mm2, _mm7                      ; _mm1:_mm2 = ((256-o)*d>>8)+(o*m_outlineYUV.V>>8);

  packuswb  _mm2, _mm1
  _movdqu   [reg_di],_mm2

%ifidn __OUTPUT_FORMAT__,win64
%else
  mov       reg_sp, reg_bx
%endif
  pop       reg_bx
  pop       reg_si
  pop       reg_di
  ret
%endmacro

%ifidn __OUTPUT_FORMAT__,win64
%else
  printUV
%endif

%define _movdqa movdqa
%define _movdqu movdqu
%define _mm0 xmm0
%define _mm1 xmm1
%define _mm2 xmm2
%define _mm3 xmm3
%define _mm4 xmm4
%define _mm5 xmm5
%define _mm6 xmm6
%define _mm7 xmm7
%ifidn __OUTPUT_FORMAT__,win64
%define HAVE_LOCAL_SSE2_x64
%endif

;
; void* (__cdecl TrenderedSubtitleWord_print_sse2)(const unsigned char* bmp,const unsigned char* outline,const unsigned char* shadow,const unsigned short* colortbl,const unsigned char* dst,const DWORD* fontmaskconstants);
;
cglobal TrenderedSubtitleWord_printY_sse2
TrenderedSubtitleWord_printY_sse2:
  printY

;
; void* (__cdecl TrenderedSubtitleWord_printUV_sse2)(const unsigned char* bmp,const unsigned char* outline,const unsigned char* shadow,const unsigned short* colortbl,const unsigned char* dstU,const unsigned char* dstV,const DWORD* fontmaskconstants);
;
cglobal TrenderedSubtitleWord_printUV_sse2
TrenderedSubtitleWord_printUV_sse2:
  printUV

;
; void YV12_lum2chr_min_mmx(const unsigned char* lum0,const unsigned char* lum1,unsigned char* chr);
;
cglobal YV12_lum2chr_min_mmx
YV12_lum2chr_min_mmx:
%ifidn __OUTPUT_FORMAT__,win64
  mov       rax, 0x00ff00ff00ff00ff
  movq      mm7, rax
  mov       rax, 0xffffffffffffffff
  movq      mm6, rax
  mov       rax, 0x8080808080808080
  movq      mm5, rax
%else
  movq      mm7, [msk00ff]
  movq      mm6, [mskffff]
  movq      mm5, [msk8080]
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
%ifidn __OUTPUT_FORMAT__,win64
  mov       rax, 0x00ff00ff00ff00ff
  movq      mm7, rax
  mov       rax, 0xffffffffffffffff
  movq      mm6, rax
  mov       rax, 0x8080808080808080
  movq      mm5, rax
%else
  movq      mm7, [msk00ff]
  movq      mm6, [mskffff]
  movq      mm5, [msk8080]
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
;                            /* 1 rdx */ const unsigned char* lum1,
;                            /* 2 r8  */ unsigned char* chr);
;
cglobal YV12_lum2chr_max_mmx2
YV12_lum2chr_max_mmx2:
%ifidn __OUTPUT_FORMAT__,win64
  mov       rax, 0x00ff00ff00ff00ff
  movq      mm7, rax
%else
  movq      mm7, [msk00ff]
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
;                            /* 1 rdx */ const unsigned char* lum1,
;                            /* 2 r8  */ unsigned char* chr);
;
cglobal YV12_lum2chr_min_mmx2
YV12_lum2chr_min_mmx2:
%ifidn __OUTPUT_FORMAT__,win64
  mov       rax, 0x00ff00ff00ff00ff
  movq      mm7, rax
%else
  movq      mm7, [msk00ff]
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

 	  	 
