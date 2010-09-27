/*
 * Copyright (c) 2002-2006 Milan Cutka
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "stdafx.h"
#include "Tlibmplayer.h"
#include "TpostprocSettings.h"
#include "Tdll.h"
#include "ffImgfmt.h"

const char_t* Tlibmplayer::dllname=_l("libmplayer.dll");

Tlibmplayer::Tlibmplayer(const Tconfig *config):refcount(0)
{
 dll=new Tdll(dllname,config);
 dll->loadFunction(init_mplayer,"init_mplayer");
 dll->loadFunction(MP3_Init,"MP3_Init");
 dll->loadFunction(MP3_DecodeFrame,"MP3_DecodeFrame");
 dll->loadFunction(MP3_Done,"MP3_Done");

 if (dll->ok)
  init_mplayer(Tconfig::cpu_flags&FF_CPU_MMX,
               Tconfig::cpu_flags&FF_CPU_MMXEXT,
               Tconfig::cpu_flags&FF_CPU_3DNOW,
               Tconfig::cpu_flags&FF_CPU_3DNOWEXT,
               Tconfig::cpu_flags&FF_CPU_SSE,
               Tconfig::cpu_flags&FF_CPU_SSE2,
               Tconfig::cpu_flags&FF_CPU_SSSE3);
}
Tlibmplayer::~Tlibmplayer()
{
 delete dll;
}
