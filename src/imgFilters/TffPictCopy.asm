;*****************************
;* Assembler bitblit by Steady
;*****************************

BITS 32

%macro cglobal 1 
 %ifdef PREFIX
  global _%1 
  %define %1 _%1
 %else
  global %1
 %endif
%endmacro

section .text

align 16

cglobal asm_BitBlt_ISSE
 %define dstp [esp+16+4]
 %define dst_pitch [esp+16+8]
 %define srcp [esp+16+12]
 %define src_pitch [esp+16+16]
 %define row_size [esp+16+20]
 %define height [esp+16+24]
asm_BitBlt_ISSE
 push esi
 push edi
 push ebx
 push ecx
 
 mov eax,height
 mov edx,row_size
 cmp edx,0
 je near asm_BitBlt_SSE_end
 cmp eax,0
 je near asm_BitBlt_SSE_end 
 
 dec eax
 
 mov esi,eax
 imul esi,src_pitch
 add esi,srcp
 
 mov edi,eax
 imul edi,dst_pitch
 add edi,dstp
 
 cmp edx,64
 jge asm_BitBlt_SSE_big
; ==================== small =====================
 dec   edx
 mov   ebx,height
 align 16
memoptS_rowloop:
 mov   ecx,edx
;      rep movsb
memoptS_byteloop:
 mov   AL,[esi+ecx]
 mov   [edi+ecx],AL
 sub   ecx,1
 jnc   memoptS_byteloop
 sub   esi,src_pitch
 sub   edi,dst_pitch
 dec   ebx
 jne   memoptS_rowloop
 jmp		asm_BitBlt_SSE_end
 
asm_BitBlt_SSE_big: 
 mov eax,dstp
 or  eax,row_size
 or  eax,src_pitch
 or  eax,dst_pitch
 test eax,7
 jz	near	asm_BitBlt_SSE_align
 ;==================== unaligned =====================
 mov   AL,[esi]
 mov   edx,row_size
 mov   ebx,height

 ;loop starts here

 align 16
memoptU_rowloop:
 mov   ecx,edx     ;row_size
 dec   ecx         ;offset to last byte in row
 add   ecx,esi     ;ecx= ptr last byte in row
 and   ecx,~63     ;align to first byte in cache line
memoptU_prefetchloop:
 mov   AX,[ecx]    ;tried AL,AX,EAX, AX a tiny bit faster
 sub   ecx,64
 cmp   ecx,esi
 jae   memoptU_prefetchloop

;write 

 movq    mm6,[esi]     ;move the first unaligned bytes
 movntq  [edi],mm6

 mov   eax,edi
 neg   eax
 mov   ecx,eax
 and   eax,63      ;eax=bytes from [edi] to start of next 64 byte cache line
 and   ecx,7       ;ecx=bytes from [edi] to next QW
 align 16
memoptU_prewrite8loop:        ;write out odd QW's so 64 bit write is cache line aligned
 cmp   ecx,eax           ;start of cache line ?
 jz    memoptU_pre8done  ;if not, write single QW
 movq    mm7,[esi+ecx]
 movntq  [edi+ecx],mm7
 add   ecx,8
 jmp   memoptU_prewrite8loop

 align 16
memoptU_write64loop:
 movntq  [edi+ecx-64],mm0
 movntq  [edi+ecx-56],mm1
 movntq  [edi+ecx-48],mm2
 movntq  [edi+ecx-40],mm3
 movntq  [edi+ecx-32],mm4
 movntq  [edi+ecx-24],mm5
 movntq  [edi+ecx-16],mm6
 movntq  [edi+ecx- 8],mm7
memoptU_pre8done:
 add   ecx,64
 cmp   ecx,edx         ;while(offset <= row_size) do {...
 ja    memoptU_done64
 movq    mm0,[esi+ecx-64]
 movq    mm1,[esi+ecx-56]
 movq    mm2,[esi+ecx-48]
 movq    mm3,[esi+ecx-40]
 movq    mm4,[esi+ecx-32]
 movq    mm5,[esi+ecx-24]
 movq    mm6,[esi+ecx-16]
 movq    mm7,[esi+ecx- 8]
 jmp   memoptU_write64loop
memoptU_done64:

 sub     ecx,64    ;went to far
 align 16
memoptU_write8loop:
 add     ecx,8           ;next QW
 cmp     ecx,edx         ;any QW's left in row ?
 ja      memoptU_done8
 movq    mm0,[esi+ecx-8]
 movntq  [edi+ecx-8],mm0
 jmp   memoptU_write8loop
memoptU_done8:

 movq    mm1,[esi+edx-8] ;write the last unaligned bytes
 movntq  [edi+edx-8],mm1
 sub   esi,src_pitch
 sub   edi,dst_pitch
 dec   ebx               ;row counter (=height at start)
 jne   memoptU_rowloop
 sfence
 emms
 jmp asm_BitBlt_SSE_end

asm_BitBlt_SSE_align: 
; ==================== aligned =====================
      mov   ebx,height
      align 16
memoptA_rowloop:
      mov   ecx,edx ;row_size
      dec   ecx     ;offset to last byte in row

;********forward routine
      add   ecx,esi
      and   ecx,~63   ;align prefetch to first byte in cache line(~3-4% faster)
      align 16
memoptA_prefetchloop:
      mov   AX,[ecx]
      sub   ecx,64
      cmp   ecx,esi
      jae   memoptA_prefetchloop

      mov   eax,edi
      xor   ecx,ecx
      neg   eax
      and   eax,63            ;eax=bytes from edi to start of cache line
      align 16
memoptA_prewrite8loop:        ;write out odd QW's so 64bit write is cache line aligned
      cmp   ecx,eax           ;start of cache line ?
      jz    memoptA_pre8done  ;if not, write single QW
      movq    mm7,[esi+ecx]
      movntq  [edi+ecx],mm7
      add   ecx,8
      jmp   memoptA_prewrite8loop

      align 16
memoptA_write64loop:
      movntq  [edi+ecx-64],mm0
      movntq  [edi+ecx-56],mm1
      movntq  [edi+ecx-48],mm2
      movntq  [edi+ecx-40],mm3
      movntq  [edi+ecx-32],mm4
      movntq  [edi+ecx-24],mm5
      movntq  [edi+ecx-16],mm6
      movntq  [edi+ecx- 8],mm7
memoptA_pre8done:
      add   ecx,64
      cmp   ecx,edx
      ja    memoptA_done64    ;less than 64 bytes left
      movq    mm0,[esi+ecx-64]
      movq    mm1,[esi+ecx-56]
      movq    mm2,[esi+ecx-48]
      movq    mm3,[esi+ecx-40]
      movq    mm4,[esi+ecx-32]
      movq    mm5,[esi+ecx-24]
      movq    mm6,[esi+ecx-16]
      movq    mm7,[esi+ecx- 8]
      jmp   memoptA_write64loop

memoptA_done64:
      sub   ecx,64

      align 16
memoptA_write8loop:           ;less than 8 QW's left
      add   ecx,8
      cmp   ecx,edx
      ja    memoptA_done8     ;no QW's left
      movq    mm7,[esi+ecx-8]
      movntq  [edi+ecx-8],mm7
      jmp   memoptA_write8loop

memoptA_done8:
      sub   esi,src_pitch
      sub   edi,dst_pitch
      dec   ebx               ;row counter (height)
      jne   memoptA_rowloop

      sfence
      emms

asm_BitBlt_SSE_end:
 pop ecx
 pop ebx
 pop edi
 pop esi
 ret
