@echo off

set path=%path%;%SDCC%\bin\

if not exist obj md obj
del /q /s obj >nul

sdasz80 -o obj/crt0.rel ../sdklib/crt0.s
if errorlevel 1 pause & exit

xcopy /b /y obj\crt0.rel %SDCC%\lib\z80\ >nul
sdcc -I ../sdklib -I ../ft812lib -I ../tslib -mz80 --std-sdcc11 --opt-code-speed -Wl-b_CODE=0x0040 -Wl-b_DATA=0xB000 src/main.c ../ft812lib/ft812.lib ../tslib/ts.lib ../sdklib/sdk.lib -o obj/out.hex
if errorlevel 1 pause & exit
if exist obj/out.hex hex2bin obj/out.hex >nul

ren obj\out.bin code.C
copy ..\sdklib\warning.scr obj\warning.C >nul

trdtool # %1.trd >nul
REM trdtool + %PRJ%.trd res/boot.$b obj/warning.C obj/code.C >nul
trdtool + %1.trd res/boot.$b obj/code.C >nul
if errorlevel 1 pause & exit

spgbld.exe -b res/spg.ini %1.spg
if errorlevel 1 pause & exit
