;*****************************************************************************
;* cpu.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003 x264 project
;* $Id: cpu.asm,v 1.1 2004/06/03 19:27:07 fenrir Exp $
;*
;* Authors: Laurent Aimar <fenrir@via.ecp.fr>
;*
;* This program is free software; you can redistribute it and/or modify
;* it under the terms of the GNU General Public License as published by
;* the Free Software Foundation; either version 2 of the License, or
;* (at your option) any later version.
;*
;* This program is distributed in the hope that it will be useful,
;* but WITHOUT ANY WARRANTY; without even the implied warranty of
;* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;* GNU General Public License for more details.
;*
;* You should have received a copy of the GNU General Public License
;* along with this program; if not, write to the Free Software
;* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
;*****************************************************************************

BITS 32

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%include "common/i386/i386inc.asm"

;=============================================================================
; Code
;=============================================================================

SECTION .text

;-----------------------------------------------------------------------------
;   void __cdecl x264_emms( void )
;-----------------------------------------------------------------------------
cglobal x264_emms
    emms
    ret

;-----------------------------------------------------------------------------
; void x264_stack_align( void (*func)(void*), void *arg )
;-----------------------------------------------------------------------------
cglobal x264_stack_align
    push ebp
    mov  ebp, esp
    sub  esp, 4
    and  esp, ~15
    mov  ecx, [ebp+8]
    mov  edx, [ebp+12]
    mov  [esp], edx
    call ecx
    mov  esp, ebp
    pop  ebp
    ret

