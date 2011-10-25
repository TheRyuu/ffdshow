/*
 * mmx.h
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with mpeg2dec; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LIBMPEG2_MMX_H
#define LIBMPEG2_MMX_H

/*
 * The type of an value that fits in an MMX register (note that long
 * long constant values MUST be suffixed by LL and unsigned long long
 * values by ULL, lest they be truncated by the compiler)
 */
 
#include "../../csimd.h"

#define	movd_m2r(var,reg)	mmx_m2r (movd, var, reg)
#define	movd_r2m(reg,var)	mmx_r2m (movd, reg, var)

#define	movd_v2r(var,reg)       movd(var,reg)

#define	movq_m2r(var,reg)	movq(&(var),reg)
#define	movq_r2m(reg,var)	movq(reg,&(var))
#define	movq_r2r(regs,regd)	movq(regs,regd)

#define	packssdw_m2r(var,reg)	packssdw( &(var), reg)
#define	packssdw_r2r(regs,regd) packssdw( regs, regd)
#define	packsswb_m2r(var,reg)	packsswb( &(var), reg)
#define	packsswb_r2r(regs,regd) packsswb( regs, regd)

#define	packuswb_m2r(var,reg)	packuswb( &(var), reg)
#define	packuswb_r2r(regs,regd) packuswb( regs, regd)

#define	paddb_m2r(var,reg)	paddb( &(var), reg)
#define	paddb_r2r(regs,regd)	paddb( regs, regd)
#define	paddd_m2r(var,reg)	paddd( &(var), reg)
#define	paddd_r2r(regs,regd)	paddd( regs, regd)
#define	paddw_m2r(var,reg)	paddw( &(var), reg)
#define	paddw_r2r(regs,regd)	paddw( regs, regd)

#define	paddsb_m2r(var,reg)	paddsb( &(var), reg)
#define	paddsb_r2r(regs,regd)	paddsb( regs, regd)
#define	paddsw_m2r(var,reg)	paddsw( &(var), reg)
#define	paddsw_r2r(regs,regd)	paddsw( regs, regd)

#define	paddusb_m2r(var,reg)	paddusb( &(var), reg)
#define	paddusb_r2r(regs,regd)	paddusb( regs, regd)
#define	paddusw_m2r(var,reg)	paddusw( &(var), reg)
#define	paddusw_r2r(regs,regd)	paddusw( regs, regd)

#define	pand_m2r(var,reg)	pand( &(var), reg)
#define	pand_r2r(regs,regd)	pand( regs, regd)

#define	pandn_m2r(var,reg)	mmx_m2r (pandn, var, reg)
#define	pandn_r2r(regs,regd)	mmx_r2r (pandn, regs, regd)

#define	pcmpeqb_m2r(var,reg)	mmx_m2r (pcmpeqb, var, reg)
#define	pcmpeqb_r2r(regs,regd)	mmx_r2r (pcmpeqb, regs, regd)
#define	pcmpeqd_m2r(var,reg)	mmx_m2r (pcmpeqd, var, reg)
#define	pcmpeqd_r2r(regs,regd)	mmx_r2r (pcmpeqd, regs, regd)
#define	pcmpeqw_m2r(var,reg)	mmx_m2r (pcmpeqw, var, reg)
#define	pcmpeqw_r2r(regs,regd)	mmx_r2r (pcmpeqw, regs, regd)

#define	pcmpgtb_m2r(var,reg)	mmx_m2r (pcmpgtb, var, reg)
#define	pcmpgtb_r2r(regs,regd)	mmx_r2r (pcmpgtb, regs, regd)
#define	pcmpgtd_m2r(var,reg)	mmx_m2r (pcmpgtd, var, reg)
#define	pcmpgtd_r2r(regs,regd)	mmx_r2r (pcmpgtd, regs, regd)
#define	pcmpgtw_m2r(var,reg)	mmx_m2r (pcmpgtw, var, reg)
#define	pcmpgtw_r2r(regs,regd)	mmx_r2r (pcmpgtw, regs, regd)

#define	pmaddwd_m2r(var,reg)	pmaddwd (&(var), reg)
#define	pmaddwd_r2r(regs,regd)	pmaddwd (regs, regd)

#define	pmulhw_m2r(var,reg)	pmulhw( &(var), reg)
#define	pmulhw_r2r(regs,regd)	pmulhw( regs, regd)

#define	pmullw_m2r(var,reg)	pmullw( &(var), reg)
#define	pmullw_r2r(regs,regd)	pmullw( regs, regd)

#define	por_m2r(var,reg)	por( &(var), reg)
#define	por_r2r(regs,regd)	por( regs, regd)

#define	pslld_i2r(imm,reg)	pslld( imm, reg)
#define	pslld_m2r(var,reg)	pslld( &(var), reg)
#define	pslld_r2r(regs,regd)	pslld( regs, regd)
#define	psllq_i2r(imm,reg)	psllq( imm, reg)
#define	psllq_m2r(var,reg)	psllq( &(var), reg)
#define	psllq_r2r(regs,regd)	psllq( regs, regd)
#define	psllw_i2r(imm,reg)	psllw( imm, reg)
#define	psllw_m2r(var,reg)	psllw( &(var), reg)
#define	psllw_r2r(regs,regd)	psllw( regs, regd)

#define	psrad_i2r(imm,reg)	psrad( imm, reg)
#define	psrad_m2r(var,reg)	psrad( var, reg)
#define	psrad_r2r(regs,regd)	psrad( regs, regd)
#define	psraw_i2r(imm,reg)	psraw( imm, reg)
#define	psraw_m2r(var,reg)	psraw( var, reg)
#define	psraw_r2r(regs,regd)	psraw( regs, regd)

#define	psrld_i2r(imm,reg)	psrld( imm, reg)
#define	psrld_m2r(var,reg)	psrld( &(var), reg)
#define	psrld_r2r(regs,regd)	psrld( regs, regd)
#define	psrlq_i2r(imm,reg)	psrlq( imm, reg)
#define	psrlq_m2r(var,reg)	psrlq( &(var), reg)
#define	psrlq_r2r(regs,regd)	psrlq( regs, regd)
#define	psrlw_i2r(imm,reg)	psrlw( imm, reg)
#define	psrlw_m2r(var,reg)	psrlw( &(var), reg)
#define	psrlw_r2r(regs,regd)	psrlw( regs, regd)

#define	psubb_m2r(var,reg)	psubb( &(var), reg)
#define	psubb_r2r(regs,regd)	psubb( regs, regd)
#define	psubd_m2r(var,reg)	psubd( &(var), reg)
#define	psubd_r2r(regs,regd)	psubd( regs, regd)
#define	psubw_m2r(var,reg)	psubw( &(var), reg)
#define	psubw_r2r(regs,regd)	psubw( regs, regd)

#define	psubsb_m2r(var,reg)	psubsb( &(var), reg)
#define	psubsb_r2r(regs,regd)	psubsb( regs, regd)
#define	psubsw_m2r(var,reg)	psubsw( &(var), reg)
#define	psubsw_r2r(regs,regd)	psubsw( regs, regd)

#define	psubusb_m2r(var,reg)	psubusb( &(var), reg)
#define	psubusb_r2r(regs,regd)	psubusb( regs, regd)
#define	psubusw_m2r(var,reg)	psubusw( &(var), reg)
#define	psubusw_r2r(regs,regd)	psubusw( regs, regd)

#define	punpckhbw_m2r(var,reg)		punpckhbw( &(var), reg)
#define	punpckhbw_r2r(regs,regd)	punpckhbw( regs, regd)
#define	punpckhdq_m2r(var,reg)		punpckhdq( &(var), reg)
#define	punpckhdq_r2r(regs,regd)	punpckhdq( regs, regd)
#define	punpckhwd_m2r(var,reg)		punpckhwd( &(var), reg)
#define	punpckhwd_r2r(regs,regd)	punpckhwd( regs, regd)

#define	punpcklbw_m2r(var,reg) 		punpcklbw( &(var), reg)
#define	punpcklbw_r2r(regs,regd)	punpcklbw( regs, regd)
#define	punpckldq_m2r(var,reg)		punpckldq( &(var), reg)
#define	punpckldq_r2r(regs,regd)	punpckldq( regs, regd)
#define	punpcklwd_m2r(var,reg)		punpcklwd( &(var), reg)
#define	punpcklwd_r2r(regs,regd)	punpcklwd( regs, regd)

#define	pxor_m2r(var,reg)	pxor( &(var), reg)
#define	pxor_r2r(regs,regd)	pxor( regs, regd)

/* 3DNOW extensions */

#define pavgusb_m2r(var,reg)	pavgusb( &(var), reg)
#define pavgusb_r2r(regs,regd)	pavgusb( regs, regd)

/* AMD MMX extensions - also available in intel SSE */

#define	maskmovq(regs,maskreg)		mmx_r2ri (maskmovq, regs, maskreg)

#define	movntq_r2m(mmreg,var)		mmx_r2m (movntq, mmreg, var)

#define	pavgb_m2r(var,reg)		pavgb( &(var), reg)
#define	pavgb_r2r(regs,regd)		pavgb( regs, regd)
#define	pavgw_m2r(var,reg)		pavgw( &(var), reg)
#define	pavgw_r2r(regs,regd)		pavgw( regs, regd)

#define	pextrw_r2r(mmreg,reg,imm)	mmx_r2ri (pextrw, mmreg, reg, imm)

#define	pinsrw_r2r(reg,mmreg,imm)	mmx_r2ri (pinsrw, reg, mmreg, imm)

#define	pmaxsw_m2r(var,reg)		mmx_m2r (pmaxsw, var, reg)
#define	pmaxsw_r2r(regs,regd)		mmx_r2r (pmaxsw, regs, regd)

#define	pmaxub_m2r(var,reg)		mmx_m2r (pmaxub, var, reg)
#define	pmaxub_r2r(regs,regd)		mmx_r2r (pmaxub, regs, regd)

#define	pminsw_m2r(var,reg)		mmx_m2r (pminsw, var, reg)
#define	pminsw_r2r(regs,regd)		mmx_r2r (pminsw, regs, regd)

#define	pminub_m2r(var,reg)		mmx_m2r (pminub, var, reg)
#define	pminub_r2r(regs,regd)		mmx_r2r (pminub, regs, regd)

#define	pmulhuw_m2r(var,reg)		mmx_m2r (pmulhuw, var, reg)
#define	pmulhuw_r2r(regs,regd)		mmx_r2r (pmulhuw, regs, regd)

#define	prefetcht0(mem)			mmx_fetch (mem, t0)
#define	prefetcht1(mem)			mmx_fetch (mem, t1)
#define	prefetcht2(mem)			mmx_fetch (mem, t2)
#define	prefetchnta(mem)		mmx_fetch (mem, nta)

#define	psadbw_m2r(var,reg)		mmx_m2r (psadbw, var, reg)
#define	psadbw_r2r(regs,regd)		mmx_r2r (psadbw, regs, regd)

#define	pshufw_m2r(var,reg,imm)		reg=_mm_shuffle_pi16(&(var),imm)
#define	pshufw_r2r(regs,regd,imm)	regd=_mm_shuffle_pi16(regs,imm)

#endif /* LIBMPEG2_MMX_H */
