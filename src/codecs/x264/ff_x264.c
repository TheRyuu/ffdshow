/*
 * Copyright (c) 2004-2006 Milan Cutka
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <windows.h>
#include <string.h>
#include <stdarg.h>
#ifdef HAVE_STDINT
#  include <stdint.h>
#else
#  include <inttypes.h>
#endif
#include "x264.h"
#include "common/common.h"
#include "encoder/ratecontrol.h"
#include "../../compiler.h"

CRITICAL_SECTION g_csStaticDataLock;

BOOL pthread_win32_process_attach_np(void);
BOOL pthread_win32_process_detach_np(void);
BOOL pthread_win32_thread_attach_np(void);
BOOL pthread_win32_thread_detach_np(void);

// --- standard WIN32 entrypoints --------------------------------------
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    static int count=0;
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        count++;
#ifdef __GNUC__
        pthread_win32_process_attach_np();
        pthread_win32_thread_attach_np();
#endif
        DisableThreadLibraryCalls(hInstance);
        InitializeCriticalSection( &g_csStaticDataLock );
        break;
    case DLL_PROCESS_DETACH:
        count--;
#ifdef __GNUC__
        pthread_win32_thread_detach_np();
        pthread_win32_process_detach_np();
#endif
        DeleteCriticalSection( &g_csStaticDataLock );
        break;
    }
    return TRUE;
}

void __stdcall getVersion(char *ver,const char* *license)
{
 strcpy(ver, X264_BUILD", build date "__DATE__" "__TIME__" ("COMPILER COMPILER_X64 COMPILER_INFO")");
 *license="";
}
