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
#include "Tconfig.h"
#include "reg.h"
#include "Tlibavcodec.h"
#include "Tlibmplayer.h"
#include "avisynth/Tavisynth.h"
#include "TvideoCodecLibmpeg2.h"
#include "TvideoCodecTheora.h"
#include "TaudioCodecLibMAD.h"
#include "TaudioCodecLibFAAD.h"
#include "TvideoCodecXviD4.h"
#include "TvideoCodecWmv9.h"
#include "TvideoCodecX264.h"
#include "TvideoCodecSkal.h"
#include "TaudioCodecLiba52.h"
#include "TaudioCodecLibDTS.h"
#include "TaudioCodecTremor.h"
#include "TaudioCodecRealaac.h"
#include "TaudioCodecAudX.h"
#include "Tdll.h"
#include "cpudetect.h"
#include <tlhelp32.h>
#include <shlwapi.h>
#include "ThtmlColors.h"
#include "TdialogSettings.h"

#ifndef WIN64
extern "C"
{
 Tconfig::Tfastmemcpy memcpy_x86;
 Tconfig::Tfastmemcpy memcpy_mmx;
 Tconfig::Tfastmemcpy memcpy_xmm;
 Tconfig::Tfastmemcpy memcpy_sse;
}
#endif

bool Tconfig::winNT;
int Tconfig::cpu_flags=0,Tconfig::available_cpu_flags=0,Tconfig::lavc_cpu_flags=0,Tconfig::sws_cpu_flags=0;
int Tconfig::cache_line=32;
Tconfig::Tfastmemcpy* Tconfig::fastmemcpy=memcpy;

extern "C"
{
 // cpu_flag detection helper functions
 extern int check_cpu_features(void);
 extern void sse_os_trigger(void);
 extern void sse2_os_trigger(void);
 extern void sse3_os_trigger(void);
 extern void ssse3_os_trigger(void);
}

Tconfig::Tconfig(TintStrColl *Icoll):Toptions(Icoll),htmlcolors(NULL)
{
 static const TintOptionT<Tconfig> iopts[]=
  {
   IDFF_availableCpuFlags  ,&Tconfig::available_cpu_flags0,-1,-1,_l(""),0,NULL,0,
   IDFF_cpuFlags           ,&Tconfig::cpu_flags0          ,-1,-1,_l(""),0,NULL,0,
   IDFF_unicodeOS          ,&Tconfig::unicodeOS           ,-1,-1,_l(""),0,NULL,0,
   0
  };
 addOptions(iopts);
 static const TstrOption sopts[]=
  {
   IDFF_installPath,(TstrVal)&Tconfig::pth,MAX_PATH,_l(""),0,NULL,NULL,
   0
  };
 addOptions(sopts);
}
Tconfig::Tconfig(HINSTANCE hInst,int allowedCpuGFlags):Toptions(NULL),htmlcolors(NULL)
{
 init1(hInst);
 initCPU(allowedCpuGFlags);
 init2();
}
Tconfig::~Tconfig()
{
 if (htmlcolors) delete htmlcolors;
 delete gdi32;
}

bool Tconfig::check(const char_t *dllname) const
{
 return Tdll::check(dllname,this);
}

void Tconfig::init1(HINSTANCE hi)
{
 char_t eflnm[MAX_PATH];
 GetModuleFileName(hi,eflnm,MAX_PATH);
 extractfilepath(eflnm,epth);
 exeflnm[0]='\0';

 load();

 shellversion=(unsigned int)-1;

 OSVERSIONINFO vi;
 vi.dwOSVersionInfoSize=sizeof(vi);
 if (GetVersionEx(&vi))
  winNT=vi.dwPlatformId==VER_PLATFORM_WIN32_NT;
 else
  winNT=false;

 memset(isDecoder,0,sizeof(isDecoder));
 isDecoder[IDFF_MOVIE_LAVC]=Tlibavcodec::check(this);
 isDecoder[IDFF_MOVIE_LIBMPEG2]=check(TvideoCodecLibmpeg2::dllname);
 isDecoder[IDFF_MOVIE_THEO]=check(TvideoCodecTheora::dllname);
 isDecoder[IDFF_MOVIE_RAW]=1;
 isDecoder[IDFF_MOVIE_MPLAYER]=check(Tlibmplayer::dllname);
 isDecoder[IDFF_MOVIE_LIBMAD]=check(TaudioCodecLibMAD::dllname);
 isDecoder[IDFF_MOVIE_LIBFAAD]=check(TaudioCodecLibFAAD::dllname);
 isDecoder[IDFF_MOVIE_XVID4]=check(TvideoCodecXviD4::dllname);
 isDecoder[IDFF_MOVIE_AVIS]=check(Tavisynth_c::dllname);
 isDecoder[IDFF_MOVIE_WMV9]=check(TvideoCodecWmv9::dllname);
 isDecoder[IDFF_MOVIE_X264]=check(TvideoCodecX264::dllname);
 isDecoder[IDFF_MOVIE_SKAL]=check(TvideoCodecSkal::dllname);
 isDecoder[IDFF_MOVIE_LIBA52]=check(TaudioCodecLiba52::dllname);
 isDecoder[IDFF_MOVIE_LIBDTS]=check(TaudioCodecLibDTS::dllname);
 isDecoder[IDFF_MOVIE_SPDIF]=1;
 isDecoder[IDFF_MOVIE_TREMOR]=check(TaudioCodecTremor::dllname);
 isDecoder[IDFF_MOVIE_REALAAC]=check(TaudioCodecRealaac::dllname);
 isDecoder[IDFF_MOVIE_AUDX]=check(TaudioCodecAudX::dllname);

 gdiA.getTextExtentExPoint=GetTextExtentExPointA;
 gdiA.getTextExtentPoint32=GetTextExtentPoint32A;
 gdiA.textOut=TextOutA;
 gdi32=new Tdll(_l("gdi32.dll"),this);
 gdi32->loadFunction(gdiW.getTextExtentExPoint,"GetTextExtentExPointW");
 gdi32->loadFunction(gdiW.getTextExtentPoint32,"GetTextExtentPoint32W");
 gdi32->loadFunction(gdiW.textOut,"TextOutW");
 unicodeOS=gdi32->ok;
}

void Tconfig::init2(void)
{
 available_cpu_flags0=available_cpu_flags;cpu_flags0=cpu_flags;
}

void Tconfig::load(void)
{
 savePth= getpth(pth);
}
void Tconfig::save(void)
{
 if(savePth)
  {
   TregOpRegWrite t(HKEY_LOCAL_MACHINE,FFDSHOW_REG_PARENT _l("\\") FFDSHOW);
   t._REG_OP_S(0,_l("pth"),pth,MAX_PATH,_l(""));
  }
}

 #ifdef __GNUC__
  #include <signal.h>
  #include <setjmp.h>
  static jmp_buf mark;
  static void sigill_handler(int signal)
   {
    longjmp(mark, 1);
   }
  static int sigill_check(void (*func)())
   {
    void (*old_handler)(int);
    int jmpret;
    old_handler=signal(SIGILL,sigill_handler);
    if (old_handler==SIG_ERR)
     return -1;

    jmpret=setjmp(mark);
    if (jmpret==0)
     func();
    signal(SIGILL,old_handler);
    return jmpret;
   }
 #else
  static int sigill_check(void (*func)())
   {
    __try
     {
      func();
     }
    __except(EXCEPTION_EXECUTE_HANDLER)
     {
      if (_exception_code()==STATUS_ILLEGAL_INSTRUCTION)
       return 1;
     }
    return 0;
   }
 #endif

void Tconfig::initCPU(int allowed_cpu_flags)
{
 if (available_cpu_flags==0)
  {
   #ifndef __GNUC__
   available_cpu_flags=check_cpu_features();
   if ((available_cpu_flags&FF_CPU_SSE) && sigill_check(sse_os_trigger))
    available_cpu_flags&=~FF_CPU_SSE;
   if ((available_cpu_flags&FF_CPU_SSE2) && sigill_check(sse2_os_trigger))
    available_cpu_flags&=~FF_CPU_SSE2;
   if ((available_cpu_flags&FF_CPU_SSE3) && sigill_check(sse3_os_trigger))
    available_cpu_flags&=~FF_CPU_SSE3;
   if ((available_cpu_flags&FF_CPU_SSSE3) && sigill_check(ssse3_os_trigger))
    available_cpu_flags&=~FF_CPU_SSSE3;
   #else
   available_cpu_flags=(IsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE)?FF_CPU_MMX|FF_CPU_MMXEXT:0)|
                       (IsProcessorFeaturePresent(PF_3DNOW_INSTRUCTIONS_AVAILABLE)?FF_CPU_3DNOW|FF_CPU_3DNOWEXT:0)|
                       (IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE)?FF_CPU_SSE:0)|
                       (IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE)?FF_CPU_SSE2:0);
   /*Vista only        (IsProcessorFeaturePresent(PF_SSE3_INSTRUCTIONS_AVAILABLE)?FF_CPU_SSE3:0);*/
   /*For MinGW GCC 4.0.x compiled version of ffdshow.ax*/
   #ifdef __INTEL_COMPILER
   available_cpu_flags|=FF_CPU_MMX|FF_CPU_MMXEXT;
   #endif
   #endif
   cpu_flags=available_cpu_flags&allowed_cpu_flags;
   sws_cpu_flags=Tlibmplayer::swsCpuCaps();
   lavc_cpu_flags=Tlibavcodec::lavcCpuFlags();
   //GetCpuCaps(&gCpuCaps);
   cache_line=64;//gCpuCaps.cl_size;
   #ifndef WIN64
   //if      (cpu_flags&FF_CPU_SSE2)   fastmemcpy=fast_memcpy_SSE;
   if      (cpu_flags&FF_CPU_MMXEXT) fastmemcpy=memcpy_xmm;
   //else if (cpu_flags&FF_CPU_3DNOW)  fastmemcpy=memcpy_;
   else if (cpu_flags&FF_CPU_MMX)    fastmemcpy=memcpy_mmx;
   else                              fastmemcpy=memcpy_x86;
   #else
   fastmemcpy=memcpy;
   #endif

  }
}

DWORD Tconfig::getCPUcount(void)
{
 SYSTEM_INFO si;
 GetSystemInfo(&si);
 return si.dwNumberOfProcessors;
}

int Tconfig::get_trayIconFullColorOS(void)
{
 int trayIconFullColorOS=false;
 OSVERSIONINFO vi;
 vi.dwOSVersionInfoSize=sizeof(vi);
 if (GetVersionEx(&vi))
  {
   if (vi.dwMajorVersion>5) trayIconFullColorOS=true; // Vista or later.
   if (vi.dwMajorVersion==5 && vi.dwMinorVersion>=1) trayIconFullColorOS=true; // Xp, Server 2003
   // Windows 2000 does not support full color icon for task tray. Microsoft's document may be wrong.
  }
 return trayIconFullColorOS;
}

const char_t* Tconfig::getExeflnm(void) const
{
 if (exeflnm[0]=='\0')
  {
   DWORD pid=GetCurrentProcessId();
   Tdll kernel(_l("kernel32.dll"),this);
   HANDLE (WINAPI *CreateToolhelp32Snapshot)(DWORD dwFlags,DWORD th32ProcessID);kernel.loadFunction(CreateToolhelp32Snapshot,"CreateToolhelp32Snapshot");
   if (!CreateToolhelp32Snapshot)  //Windows NT
    return _l("");
  #ifdef UNICODE
   BOOL (WINAPI *Process32First)(HANDLE hSnapshot,LPPROCESSENTRY32W lppe);kernel.loadFunction(Process32First,"Process32FirstW");
   BOOL (WINAPI *Process32Next)(HANDLE hSnapshot,LPPROCESSENTRY32W lppe);kernel.loadFunction(Process32Next,"Process32NextW");
  #else
   BOOL (WINAPI *Process32First)(HANDLE hSnapshot,LPPROCESSENTRY32 lppe);kernel.loadFunction(Process32First,"Process32First");
   BOOL (WINAPI *Process32Next)(HANDLE hSnapshot,LPPROCESSENTRY32 lppe);kernel.loadFunction(Process32Next,"Process32Next");
  #endif
   if (Process32First && Process32Next)
    {
     HANDLE hs=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
     PROCESSENTRY32 pe32;
     pe32.dwSize=sizeof(pe32);
     BOOL ret=Process32First(hs,&pe32);
     while (ret==TRUE)
      {
       if (pe32.th32ProcessID==pid)
        {
         extractfilename(pe32.szExeFile,exeflnm);
         break;
        }
       ret=Process32Next(hs,&pe32);
      }
     CloseHandle(hs);
    }
  }
 return exeflnm;
}
unsigned int Tconfig::getShellVersion(void) const
{
 if (shellversion==(unsigned int)-1)
  {
   shellversion=4<<24;
   Tdll shell(_l("shell32.dll"),this);
   DLLGETVERSIONPROC getversion;
   shell.loadFunction(getversion,"DllGetVersion");
   if (shell.ok)
    {
     DLLVERSIONINFO dvi;
     dvi.cbSize=sizeof(dvi);
     getversion(&dvi);
     shellversion=(dvi.dwMajorVersion<<24)|(dvi.dwMinorVersion<<16)|dvi.dwBuildNumber;
    }
  }
 return shellversion;
}

ThtmlColors* Tconfig::getHtmlColors(void) const
{
 if (!htmlcolors) htmlcolors=new ThtmlColors;
 return htmlcolors;
}
