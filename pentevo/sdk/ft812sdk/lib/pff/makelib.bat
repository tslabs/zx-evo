@echo off

set path=%path%;c:\Tools\PROG\SDCC\bin\

if not exist obj md obj
del /q /s obj >nul
del *.lib
copy *.c obj
copy *.h obj
cd obj

sdcc-lib-split.exe diskio.c   >>lib.lst
if errorlevel 1 pause & exit
sdcc-lib-split.exe pff.c >>lib.lst
if errorlevel 1 pause & exit

for /f %%i in (lib.lst) do (
  echo %%i
  sdcc -I../../tslib -mz80 --std-sdcc11 --opt-code-speed -c %%i -o %%i.rel
  if errorlevel 1 pause & exit
  sdcclib ../pff.lib %%i.rel
  if errorlevel 1 pause & exit
)
