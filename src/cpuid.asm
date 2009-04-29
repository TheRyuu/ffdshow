;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - CPUID check processors capabilities -
; *
; *  Copyright (C) 2001-2008 Michael Militzer <michael@xvid.org>
; *
; *  This program is free software ; you can redistribute it and/or modify
; *  it under the terms of the GNU General Public License as published by
; *  the Free Software Foundation ; either version 2 of the License, or
; *  (at your option) any later version.
; *
; *  This program is distributed in the hope that it will be useful,
; *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
; *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; *  GNU General Public License for more details.
; *
; *  You should have received a copy of the GNU General Public License
; *  along with this program ; if not, write to the Free Software
; *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
; *
; * $Id: cpuid.asm,v 1.18 2008/12/04 14:41:50 Isibaar Exp $
; *
; ***************************************************************************/

%include "cpuid.inc"

;=============================================================================
; Constants
;=============================================================================

%define CPUID_TSC               0x00000010
%define CPUID_MMX               0x00800000
%define CPUID_SSE               0x02000000
%define CPUID_SSE2              0x04000000
%define CPUID_SSE3              0x00000001
%define CPUID_SSSE3             0x00000200
%define CPUID_SSE41             0x00080000
%define CPUID_SSE42             0x00100000
%define CPUID_SSE4A             0x00000040
%define CPUID_SSE5              0x00000800

%define EXT_CPUID_3DNOW         0x80000000
%define EXT_CPUID_AMD_3DNOWEXT  0x40000000
%define EXT_CPUID_AMD_MMXEXT    0x00400000

%define FF_CPU_MMX              0x00000001
%define FF_CPU_MMXEXT           0x00000002
%define FF_CPU_SSE              0x00000004
%define FF_CPU_SSE2             0x00000008
%define FF_CPU_SSE3             0x00000010
%define FF_CPU_SSSE3            0x00000020
%define FF_CPU_3DNOW            0x00000040
%define FF_CPU_3DNOWEXT         0x00000080
%define FF_CPU_TSC              0x00000100
%define FF_CPU_SSE41            0x00000200
%define FF_CPU_SSE42            0x00000400
%define FF_CPU_SSE4A            0x00000800
%define FF_CPU_SSE5             0x00001000

;=============================================================================
; Read only data
;=============================================================================

ALIGN SECTION_ALIGN

DATA

vendorAMD:
db "AuthenticAMD"

;=============================================================================
; Macros
;=============================================================================

%macro  CHECK_FEATURE         4
  mov eax, %1
  and eax, %4
  neg eax
  sbb eax, eax
  and eax, %2
  or %3, eax
%endmacro

;=============================================================================
; Code
;=============================================================================

%ifdef WIN64
%define FF_PUSHFD pushfq
%define FF_POPFD  popfq
%else
%define FF_PUSHFD pushfd
%define FF_POPFD  popfd
%endif

TEXT

; int check_cpu_feature(void)

cglobal check_cpu_features
check_cpu_features:

  push _EBX
  push _ESI
  push _EDI
  push _EBP

  sub _ESP, 12             ; Stack space for vendor name
  
  xor ebp, ebp

; get vendor string, used later
  xor eax, eax
  cpuid
  mov [_ESP], ebx        ; vendor string
  mov [_ESP+4], edx
  mov [_ESP+8], ecx
  test eax, eax

  jz near .cpu_quit

  mov eax, 1
  cpuid

; RDTSC command ?
  CHECK_FEATURE CPUID_TSC, FF_CPU_TSC, ebp, edx

; MMX support ?
  CHECK_FEATURE CPUID_MMX, FF_CPU_MMX, ebp, edx

; SSE support ?
  CHECK_FEATURE CPUID_SSE, (FF_CPU_MMXEXT|FF_CPU_SSE), ebp, edx

; SSE2 support?
  CHECK_FEATURE CPUID_SSE2, FF_CPU_SSE2, ebp, edx

; SSE3 support?
  CHECK_FEATURE CPUID_SSE3, FF_CPU_SSE3, ebp, ecx

; SSSE3 support?
  CHECK_FEATURE CPUID_SSSE3, FF_CPU_SSSE3, ebp, ecx

; SSE41 support?
  CHECK_FEATURE CPUID_SSE41, FF_CPU_SSE41, ebp, ecx

; SSE42 support?
  CHECK_FEATURE CPUID_SSE42, FF_CPU_SSE42, ebp, ecx

; extended functions?
  mov eax, 0x80000000
  cpuid
  cmp eax, 0x80000000
  jbe near .cpu_quit

  mov eax, 0x80000001
  cpuid

; AMD cpu ?
  lea _ESI, [vendorAMD]
  lea _EDI, [_ESP]
  mov ecx, 12
  cld
  repe cmpsb
  jnz .cpu_quit

; 3DNow! support ?
  CHECK_FEATURE EXT_CPUID_3DNOW, FF_CPU_3DNOW, ebp, edx

; 3DNOW extended ?
  CHECK_FEATURE EXT_CPUID_AMD_3DNOWEXT, FF_CPU_3DNOWEXT, ebp, edx

; extended MMX ?
  CHECK_FEATURE EXT_CPUID_AMD_MMXEXT, FF_CPU_MMXEXT, ebp, edx

; SSE4A support?
  CHECK_FEATURE CPUID_SSE4A, FF_CPU_SSE4A, ebp, ecx

; SSE5 support?
  CHECK_FEATURE CPUID_SSE5, FF_CPU_SSE5, ebp, ecx

.cpu_quit:

  mov eax, ebp

  add _ESP, 12

  pop _EBP
  pop _EDI
  pop _ESI
  pop _EBX

  ret
ENDFUNC

; enter/exit mmx state
ALIGN SECTION_ALIGN
cglobal emms_mmx
emms_mmx:
  emms
  ret
ENDFUNC

; faster enter/exit mmx state
ALIGN SECTION_ALIGN
cglobal emms_3dn
emms_3dn:
  femms
  ret
ENDFUNC

%ifdef WIN64
cglobal prime_xmm
prime_xmm:
  movdqa xmm6, [prm1]
  movdqa xmm7, [prm1+16]
  ret
ENDFUNC

cglobal get_xmm
get_xmm:
  movdqa [prm1], xmm6
  movdqa [prm1+16], xmm7
  ret
ENDFUNC
%endif

%ifidn __OUTPUT_FORMAT__,elf
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif

