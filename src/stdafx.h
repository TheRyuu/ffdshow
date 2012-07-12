#ifndef _STDAFX_H_
#define _STDAFX_H_

#define _STLP_NEW_PLATFORM_SDK
#define _STLP_NO_OWN_IOSTREAMS 1

#define _WIN32_DCOM

#ifndef __GNUC__
#define __attribute__(x)
#endif

#ifdef __GNUC__
#define __forceinline __attribute__((__always_inline__)) inline
#endif

#define always_inline __forceinline
#define av_always_inline __forceinline

// Windows Header Files:
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <vfw.h>
// C RunTime Header Files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>
#include <time.h>
#include <assert.h>
#include <io.h>
#include <process.h>
#include <limits.h>
#include <ctype.h>
#include <wchar.h>
// STL
#include "PODtypes.h"
#include <vector>
#include <algorithm>
#include <map>
#include <list>
#include <hash_map>
#include <utility>
#include <limits>
#include <deque>
#include <array>
#include "tuple.h"
// baseclasses
#include <streams.h>
// DirectX/VFW/ACM
#include <mmreg.h>
#define DIRECTINPUT_VERSION 0x0300
#include <dinput.h>
#include <dvdmedia.h>
#include <mpconfig.h>
#include <ks.h>
#include <ksmedia.h>
#include <qnetwork.h>
#include <d3d9.h>
#include <vmr9.h>
#include <ppl.h>
#include "msacmdrv.h"

// BOOST
#include "boost/foreach.hpp"
#include "boost/thread.hpp"

// BOOST_FOREACH is documented in
// http://www.boost.org/doc/libs/1_38_0/doc/html/foreach.html
#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH

// ffdshow
//#define OSDTIMETABALE // OSD debug item "Time table" to reserch multithread time table. if you don't need this item comment out.
//#define OSD_H264POC

#include "stdint.h" // ISO C9x  compliant stdint.h for Microsoft Visual Studio
#include "inttypes.h" // ISO C9x  compliant inttypes.h for Microsoft Visual Studio
#include "dwstring.h"
#include "mem_align.h"
#include "array_allocator.h"
#include "ffglobals.h"
#include "comptr.h"
#include "ffdebug.h"

#if defined(UNICODE) && defined(__GNUC__)
#undef TEXT
#define TEXT(q) L##q
#endif

#endif
