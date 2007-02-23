/*
 * bitstream.h
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of a52dec, a free ATSC A-52 stream decoder.
 * See http://liba52.sourceforge.net/ for updates.
 *
 * a52dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * a52dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* code from ffmpeg/libavcodec */
#if defined(__GNUC__)
#    define __forceinline __attribute__((__always_inline__)) inline
#endif

#ifdef ALT_BITSTREAM_READER

/* used to avoid missaligned exceptions on some archs (alpha, ...) */
#if defined (ARCH_X86) || defined(ARCH_ARMV4L)
#    define unaligned32(a) (*(uint32_t*)(a))
#else
#    ifdef __GNUC__
static __forceinline uint32_t unaligned32(const void *v) {
    struct Unaligned {
	uint32_t i;
    } __attribute__((packed));

    return ((const struct Unaligned *) v)->i;
}
#    elif defined(__DECC)
static inline uint32_t unaligned32(const void *v) {
    return *(const __unaligned uint32_t *) v;
}
#    else
static inline uint32_t unaligned32(const void *v) {
    return *(const uint32_t *) v;
}
#    endif
#endif //!ARCH_X86

#endif

/* (stolen from the kernel) */
#ifdef WORDS_BIGENDIAN

#	define swab32(x) (x)

#else

#	ifdef __GNUC__

        #ifdef ARCH_X86_64
        #  define LEGACY_REGS "=Q"
        #else
        #  define LEGACY_REGS "=q"
        #endif

        static __forceinline uint32_t swab32(uint32_t x)
        {
         __asm("bswap	%0":
              "=r" (x)     :
              "0" (x));
          return x;
        }

#	else

	static __forceinline const uint32_t swab32(uint32_t x)
	{
	#ifdef WIN64
	return _byteswap_ulong(x);
	#else
	__asm mov eax,x __asm bswap eax// __asm mov x, eax;
	#endif
	}
#	endif
#endif

void a52_bitstream_set_ptr (a52_state_t * state, uint8_t * buf);
uint32_t a52_bitstream_get_bh (a52_state_t * state, uint32_t num_bits);
int32_t a52_bitstream_get_bh_2 (a52_state_t * state, uint32_t num_bits);

static inline uint32_t bitstream_get (a52_state_t * state, uint32_t num_bits) // note num_bits is practically a constant due to inlineing
{
#ifdef ALT_BITSTREAM_READER
    uint32_t result= swab32( unaligned32(((uint8_t *)state->buffer_start)+(state->indx>>3)) );

    result<<= (state->indx&0x07);
    result>>= 32 - num_bits;
    state->indx+= num_bits;

    return result;
#else
    uint32_t result;

    if (num_bits < state->bits_left) {
	result = (state->current_word << (32 - state->bits_left)) >> (32 - num_bits);
	state->bits_left -= num_bits;
	return result;
    }

    return a52_bitstream_get_bh (state, num_bits);
#endif
}


static inline int32_t bitstream_get_2 (a52_state_t * state, uint32_t num_bits)
{
#ifdef ALT_BITSTREAM_READER
    int32_t result= swab32( unaligned32(((uint8_t *)state->buffer_start)+(state->indx>>3)) );

    result<<= (state->indx&0x07);
    result>>= 32 - num_bits;
    state->indx+= num_bits;

    return result;
#else
    int32_t result;

    if (num_bits < state->bits_left) {
	result = (((int32_t)state->current_word) << (32 - state->bits_left)) >> (32 - num_bits);
	state->bits_left -= num_bits;
	return result;
    }

    return a52_bitstream_get_bh_2 (state, num_bits);
#endif
}
