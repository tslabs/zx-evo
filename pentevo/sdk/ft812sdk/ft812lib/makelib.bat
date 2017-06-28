@echo off

set path=%path%;c:\Tools\PROG\SDCC\bin\

if not exist obj md obj
del /q /s obj >nul
del *.lib
copy *.c obj
copy *.h obj
cd obj

sdcc-lib-split.exe ft812dl.c   >>lib.lst
sdcc-lib-split.exe ft812func.c >>lib.lst
sdcc-lib-split.exe ft812math.c >>lib.lst

for /f %%i in (lib.lst) do (
  echo %%i
  sdcc -mz80 --std-sdcc11 --opt-code-speed -c %%i -o %%i.rel
  sdcclib ../ft812.lib %%i.rel
)

pause
