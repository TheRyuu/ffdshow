#ifndef MPLAYER_CONFIG_H
#define MPLAYER_CONFIG_H

#include "../ffmpeg/config.h"

#define NAMED_ASM_ARGS 1
#define USE_FASTMEMCPY 1

#ifdef __GNUC__
	#define memalign(a,b) __mingw_aligned_malloc(b,a)
#else
	#define inline __inline
	
	#ifndef __attribute__
		#define __attribute__(x) /**/
	#endif
	#pragma warning (disable:4002)
	
	#include <malloc.h>
	#define memalign(a,b) _aligned_malloc(b,a)
#endif

#endif /* MPLAYER_CONFIG_H */