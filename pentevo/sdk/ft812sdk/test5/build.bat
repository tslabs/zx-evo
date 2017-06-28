@echo off

set PRJ=main
set path=%path%;c:\Tools\PROG\SDCC\bin\

if not exist obj md obj
del /q /s obj >nul

sdasz80 -o obj/crt0.rel ../gamelib/crt0.s
xcopy /b /y obj\crt0.rel c:\Tools\PROG\SDCC\lib\z80\ >nul
sdcc -I ../gamelib -I ../ft812lib -I ../tslib -mz80 --std-sdcc11 --opt-code-speed -Wl-b_CODE=0x0040 -Wl-b_DATA=0xB000 src/main.c ../ft812lib/ft812.lib ../tslib/ts.lib ../gamelib/game.lib -o obj/out.hex
if exist obj/out.hex hex2bin obj/out.hex >nul
ren obj\out.bin code.C

spgbld.exe -b res/spg.ini %PRJ%.spg

pause
