@echo off

set path=%path%;%SDCC%\bin\

if not exist obj md obj
del /q /s obj >nul

sdasz80 -o obj/crt0.rel src/crt0.s
if errorlevel 1 pause & exit
xcopy /b /y obj\crt0.rel %SDCC%\lib\z80\ >nul

sdcc -Isrc -I../sdklib -I../ft812lib -I../tslib -mz80 --std-sdcc11 --opt-code-speed -Wl-b_CODE=0x8003 -Wl-b_DATA=0xA000 src/main.c ../ft812lib/ft812.lib ../tslib/ts.lib ../sdklib/sdk.lib -o obj/out.hex
if errorlevel 1 pause & exit
if exist obj/out.hex hex2bin obj/out.hex >nul

for %%i in (obj/out.bin) do set sz=%%~zi
set /a sz=(%sz% - 1) / 512 + 1

sdcc -DN_BLK0=%sz% -Isrc -I../sdklib -I../ft812lib -I../tslib --no-std-crt0 -mz80 --std-sdcc11 --opt-code-speed src/wc_h.c -o obj/hdr.hex
if errorlevel 1 pause & exit
if exist obj/hdr.hex hex2bin obj/hdr.hex >nul

copy /b obj\hdr.bin + obj\out.bin ftview.wmf >nul

REM imgcpy.exe ftview.wmf unreal\wc.img=c:\wc\ftview.wmf
REM if errorlevel 1 pause & exit

REM unreal\unreal
