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
#include "ffImgfmt.h"
#include "Tlibavcodec.h"
#include "avisynth/Tavisynth.h"
#include "TvideoCodecLibmpeg2.h"
#include "TaudioCodecLibMAD.h"
#include "TaudioCodecLibFAAD.h"
#include "TvideoCodecXviD4.h"
#include "TvideoCodecWmv9.h"
#include "TvideoCodecQuickSync.h"
#include "TaudioCodecLiba52.h"
#include "TaudioCodecLibDTS.h"
#include "Tdll.h"
#include <tlhelp32.h>
#include <shlwapi.h>
#include "ThtmlColors.h"
#include "TdialogSettings.h"

int Tconfig::cpu_flags = 0;
int Tconfig::cache_line = 32;

extern "C"
{
    // cpu_flag detection helper functions
    extern int check_cpu_features(void);

#if defined(WIN64)
    extern void prime_xmm(void *);
    extern void get_xmm(void *);
#endif
}

Tconfig::Tconfig(TintStrColl *Icoll): Toptions(Icoll), htmlcolors(NULL)
{
    kernel = NULL;
    static const TintOptionT<Tconfig> iopts[] = {
        IDFF_cpuFlags           , &Tconfig::cpu_flags0          , -1, -1,       _l(""), 0, NULL, 0,
        0
    };
    addOptions(iopts);
    static const TstrOption sopts[] = {
        IDFF_installPath        , (TstrVal)&Tconfig::pth        , MAX_PATH, 0,  _l(""), 0, NULL, NULL,
        0
    };
    addOptions(sopts);
    is_WMEncEng = done_WMEncEng = false;
}
Tconfig::Tconfig(HINSTANCE hInst): Toptions(NULL), htmlcolors(NULL)
{
    kernel = NULL;
    init1(hInst);
    initCPU();
    init2();
}
Tconfig::~Tconfig()
{
    if (htmlcolors) {
        delete htmlcolors;
    }
    if (kernel) {
        delete kernel;
    }
}

bool Tconfig::check(const char_t *dllname) const
{
    return Tdll::check(dllname, this);
}

void Tconfig::init1(HINSTANCE hi)
{
    char_t eflnm[MAX_PATH];
    GetModuleFileName(hi, eflnm, MAX_PATH);
    extractfilepath(eflnm, epth);
    exeflnm[0] = '\0';

    load();

    shellversion = (unsigned int) - 1;

    memset(isDecoder, 0, sizeof(isDecoder));
    isDecoder[IDFF_MOVIE_LAVC] = Tlibavcodec::check(this);
    isDecoder[IDFF_MOVIE_FFMPEG_DXVA] = Tlibavcodec::check(this);
    isDecoder[IDFF_MOVIE_LIBMPEG2] = check(TvideoCodecLibmpeg2::dllname);
    isDecoder[IDFF_MOVIE_RAW] = 1;
    isDecoder[IDFF_MOVIE_LIBMAD] = check(TaudioCodecLibMAD::dllname);
    isDecoder[IDFF_MOVIE_LIBFAAD] = check(TaudioCodecLibFAAD::dllname);
    isDecoder[IDFF_MOVIE_XVID4] = check(TvideoCodecXviD4::dllname);
    isDecoder[IDFF_MOVIE_AVIS] = check(Tavisynth_c::dllname);
    isDecoder[IDFF_MOVIE_WMV9] = check(TvideoCodecWmv9::dllname);
    isDecoder[IDFF_MOVIE_LIBA52] = check(TaudioCodecLiba52::dllname);
    isDecoder[IDFF_MOVIE_LIBDTS] = check(TaudioCodecLibDTS::dllname);
    isDecoder[IDFF_MOVIE_QUICK_SYNC] = TvideoCodecQuickSync::check(this);

    is_WMEncEng = done_WMEncEng = false;
}

void Tconfig::init2(void)
{
    cpu_flags0 = cpu_flags;
}

void Tconfig::load(void)
{
    savePth = getpth(pth);
}
void Tconfig::save(void)
{
    if (savePth) {
        TregOpRegWrite t(HKEY_LOCAL_MACHINE, FFDSHOW_REG_PARENT _l("\\") FFDSHOW);
        t._REG_OP_S(0, _l("pth"), pth, MAX_PATH, _l(""));
    }
}

void Tconfig::initCPU(void)
{
    if (cpu_flags == 0) {
        cpu_flags = check_cpu_features();
#ifdef __INTEL_COMPILER
        cpu_flags |= FF_CPU_MMX | FF_CPU_MMXEXT;
#endif
        cache_line = 64;
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
    int trayIconFullColorOS = false;
    OSVERSIONINFO vi;
    vi.dwOSVersionInfoSize = sizeof(vi);
    if (GetVersionEx(&vi)) {
        if (vi.dwMajorVersion > 5) {
            trayIconFullColorOS = true;  // Vista or later.
        }
        if (vi.dwMajorVersion == 5 && vi.dwMinorVersion >= 1) {
            trayIconFullColorOS = true;  // Xp, Server 2003
        }
        // Windows 2000 does not support full color icon for task tray. Microsoft's document may be wrong.
    }
    return trayIconFullColorOS;
}

const char_t* Tconfig::getExeflnm(void) const
{
    if (exeflnm[0] == '\0') {
        DWORD pid = GetCurrentProcessId();

        if (!kernel) {
            kernel = new Tdll(_l("kernel32.dll"), this);
        }

        HANDLE(WINAPI * CreateToolhelp32Snapshot)(DWORD dwFlags, DWORD th32ProcessID);
        kernel->loadFunction(CreateToolhelp32Snapshot, "CreateToolhelp32Snapshot");
        if (!CreateToolhelp32Snapshot) { //Windows NT
            return _l("");
        }
#ifdef UNICODE
        BOOL (WINAPI * Process32First)(HANDLE hSnapshot, LPPROCESSENTRY32W lppe);
        BOOL (WINAPI * Process32Next)(HANDLE hSnapshot, LPPROCESSENTRY32W lppe);
        kernel->loadFunction(Process32First, "Process32FirstW");
        kernel->loadFunction(Process32Next, "Process32NextW");
#else
        BOOL (WINAPI * Process32First)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
        BOOL (WINAPI * Process32Next)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
        kernel->loadFunction(Process32First, "Process32First");
        kernel->loadFunction(Process32Next, "Process32Next");
#endif
        if (Process32First && Process32Next) {
            HANDLE hs = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(pe32);
            BOOL ret = Process32First(hs, &pe32);
            while (ret == TRUE) {
                if (pe32.th32ProcessID == pid) {
                    extractfilename(pe32.szExeFile, exeflnm);
                    break;
                }
                ret = Process32Next(hs, &pe32);
            }
            CloseHandle(hs);
        }
    }
    return exeflnm;
}
unsigned int Tconfig::getShellVersion(void) const
{
    if (shellversion == (unsigned int) - 1) {
        shellversion = 4 << 24;
        Tdll shell(_l("shell32.dll"), this);
        DLLGETVERSIONPROC getversion;
        shell.loadFunction(getversion, "DllGetVersion");
        if (shell.ok) {
            DLLVERSIONINFO dvi;
            dvi.cbSize = sizeof(dvi);
            getversion(&dvi);
            shellversion = (dvi.dwMajorVersion << 24) | (dvi.dwMinorVersion << 16) | dvi.dwBuildNumber;
        }
    }
    return shellversion;
}

ThtmlColors* Tconfig::getHtmlColors(void) const
{
    if (!htmlcolors) {
        htmlcolors = new ThtmlColors;
    }
    return htmlcolors;
}

bool Tconfig::is_WMEncEng_loaded(void) const
{
    if (!done_WMEncEng) {
        done_WMEncEng = true;
        DWORD pid = GetCurrentProcessId();
        if (!kernel) {
            kernel = new Tdll(_l("kernel32.dll"), this);
        }
        HANDLE(WINAPI * CreateToolhelp32Snapshot)(DWORD dwFlags, DWORD th32ProcessID);

        kernel->loadFunction(CreateToolhelp32Snapshot, "CreateToolhelp32Snapshot");

        if (!CreateToolhelp32Snapshot) { //Windows NT
            return false;
        }

#ifdef UNICODE
        BOOL (WINAPI * Module32First)(HANDLE hSnapshot, LPMODULEENTRY32W lppe);
        BOOL (WINAPI * Module32Next)(HANDLE hSnapshot, LPMODULEENTRY32W lppe);
        kernel->loadFunction(Module32First, "Module32FirstW");
        kernel->loadFunction(Module32Next, "Module32NextW");
#else
        BOOL (WINAPI * Module32First)(HANDLE hSnapshot, LPMODULEENTRY32 lppe);
        BOOL (WINAPI * Module32Next)(HANDLE hSnapshot, LPMODULEENTRY32 lppe);
        kernel->loadFunction(Module32First, "Module32First");
        kernel->loadFunction(Module32Next, "Module32Next");
#endif
        if (Module32First && Module32Next) {
            HANDLE hs = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
            MODULEENTRY32 me32;
            me32.dwSize = sizeof(me32);
            BOOL ret = Module32First(hs, &me32);
            while (ret == TRUE) {
                if (me32.th32ProcessID == pid) {
                    if (stricmp(me32.szModule, _l("WMEncEng.dll")) == 0) {
                        is_WMEncEng = true;
                        break;
                    }
                }
                ret = Module32Next(hs, &me32);
            }
            CloseHandle(hs);
        }
    }
    return is_WMEncEng;
}
