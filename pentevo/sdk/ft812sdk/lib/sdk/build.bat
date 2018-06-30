@echo off

set path=%path%;%SDCC%\bin\

if not exist obj md obj
del /q /s obj >nul

sdasz80 -o obj/crt0.rel ../lib/sdk/crt0.s
if errorlevel 1 pause & exit

xcopy /b /y obj\crt0.rel %SDCC%\lib\z80\ >nul
sdcc -Isrc -I../lib/sdk -I../lib/ft812 -I../lib/tsconf -I../lib/pff -mz80 --std-sdcc11 --opt-code-speed -Wl-b_CODE=0x0040 -Wl-b_DATA=0xB000 src/main.c ../lib/ft812/ft812.lib ../lib/tsconf/ts.lib ../lib/pff/pff.lib ../lib/sdk/sdk.lib -o obj/out.hex
if errorlevel 1 pause & exit
if exist obj/out.hex hex2bin.exe obj/out.hex >nul

ren obj\out.bin code.C
copy ..\lib\sdk\warning.scr obj\warning.C >nul

trdtool # %1.trd >nul
trdtool + %1.trd ../lib/sdk/boot.$b obj/code.C >nul
if errorlevel 1 pause & exit

spgbld.exe -b res/spg.ini %1.spg
if errorlevel 1 pause & exit
