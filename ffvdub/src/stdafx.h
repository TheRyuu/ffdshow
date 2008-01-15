#ifndef _STDAFX_H_
#define _STDAFX_H_

// Windows Header Files:
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <unknwn.h>
#include <mmsystem.h>
// C RunTime Header Files
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <malloc.h>
// VirtualDub
#include "ScriptInterpreter.h"
#include "ScriptError.h"
#include "ScriptValue.h"
#include "Filter.h"
#include <limits.h>
#include <ctype.h>
#include <wchar.h>
// STLcd 
#include "../../src/PODtypes.h"
#if defined(UCLIBCPP) && (defined(__INTEL_COMPILER) || defined(__GNUC__) || (_MSC_VER>=1300))
 #include "../../src/uClibc++/vector"
 #include "../../src/uClibc++/algorithm"
 #include "../../src/uClibc++/map"
 #include "../../src/uClibc++/list"
 #include "../../src/uClibc++/hash_map"
 #include "../../src/uClibc++/utility"
 #include "../../src/uClibc++/limits"
#endif
#if defined(__INTEL_COMPILER) || defined(__GNUC__) || (_MSC_VER>=1300)
 #include "../../src/tuple.h"
#endif
#include "../../src/dwstring.h"
#endif
