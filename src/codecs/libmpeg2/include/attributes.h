/*
 * attributes.h
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

#ifndef LIBMPEG2_ATTRIBUTES_H
#define LIBMPEG2_ATTRIBUTES_H

/* use MINGW GCC and MSVC attribs to align critical data structures */
#ifdef __GNUC__
  #define __align8(t,v) t v __attribute__ ((aligned (8)))
  #define __align16(t,v) t v __attribute__ ((aligned (16)))
#else
  #define __align8(t,v) __declspec(align(8)) t v
  #define __align16(t,v) __declspec(align(16)) t v
#endif

#ifdef HAVE_BUILTIN_EXPECT
#define likely(x) __builtin_expect ((x) != 0, 1)
#define unlikely(x) __builtin_expect ((x) != 0, 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#endif /* LIBMPEG2_ATTRIBUTES_H */
