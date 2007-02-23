/*****************************************************************************
 * cpu.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <inttypes.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "x264.h"
#include "cpu.h"
#include <mmintrin.h>

void     x264_cpu_restore( uint32_t cpu )
{
 #if (defined(HAVE_MMXEXT) || defined(HAVE_SSE2)) && (defined(ARCH_X86) || defined(ARCH_X86_64))
    if( cpu&(X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_3DNOW|X264_CPU_3DNOWEXT) )
    {
     #ifdef WIN64
        x264_emms();
     #else
        _mm_empty();
     #endif
    }
 #endif
}
