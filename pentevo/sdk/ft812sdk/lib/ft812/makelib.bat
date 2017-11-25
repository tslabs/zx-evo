@echo off

set path=%path%;c:\Tools\PROG\SDCC\bin\

if not exist obj md obj
del /q /s obj >nul
del *.lib
copy *.c obj
copy *.h obj
cd obj

sdcc-lib-split.exe ft812dl.c   >>lib.lst
if errorlevel 1 pause & exit
sdcc-lib-split.exe ft812func.c >>lib.lst
if errorlevel 1 pause & exit
sdcc-lib-split.exe ft812math.c >>lib.lst
if errorlevel 1 pause & exit
sdcc-lib-split.exe ft812cp.c   >>lib.lst
if errorlevel 1 pause & exit

for /f %%i in (lib.lst) do (
  echo %%i
  sdcc -I../../tslib -mz80 --std-sdcc11 --opt-code-speed -c %%i -o %%i.rel
  if errorlevel 1 pause & exit
  sdcclib ../ft812.lib %%i.rel
  if errorlevel 1 pause & exit
)
