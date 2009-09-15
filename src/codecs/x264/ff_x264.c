#include <windows.h>
#include <string.h>
#include <stdarg.h>
#ifdef HAVE_STDINT
#  include <stdint.h>
#else
#  include "inttypes.h"
#endif
#ifdef __GNUC__
#  include "../../pthreads/pthread.h"
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
    char pomS[40];
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        count++;
#ifdef __GNUC__
        pthread_win32_process_attach_np();
        pthread_win32_thread_attach_np();
#endif
        //snprintf(pomS,40,"libavcodec: %i %i\n",count,hInstance);OutputDebugString(pomS);
        DisableThreadLibraryCalls(hInstance);
        InitializeCriticalSection( &g_csStaticDataLock );
        break;

    case DLL_PROCESS_DETACH:
        count--;
#ifdef __GNUC__
        pthread_win32_thread_detach_np();
        pthread_win32_process_detach_np();
#endif
        //snprintf(pomS,40,"libavcodec: %i %i\n",count,hInstance);OutputDebugString(pomS);
        //if (count<=0)
        //av_free_static();
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
