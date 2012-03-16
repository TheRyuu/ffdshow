@echo off
REM You must have yasm.exe in a directory pointed by PATH.

if "%1" == "Win32" (
  yasm.exe -f win32 -m x86 -DWIN32=1 -DARCH_X86_32=1 -DARCH_X86_64=0 -DPREFIX -Pconfig.asm -I .. -I ../libavutil/x86 -I %2 -o %3 %4
) else (
  yasm.exe -f win64 -m amd64 -DWIN64=1 -DARCH_X86_32=0 -DARCH_X86_64=1 -Pconfig.asm -I .. -I ../libavutil/x86 -I %2 -o %3 %4
)
