@echo off

set path=%path%;%SDCC%\bin\

if not exist obj md obj
del /q /s obj >nul

sdasz80 -o obj/crt0.rel src/crt0.s 
if errorlevel 1 pause & exit
sdcc -I -mz80 --std-sdcc11 --opt-code-speed -c src/main.c -o obj/main.rel
if errorlevel 1 pause & exit
sdcc -I -mz80 --std-sdcc11 --opt-code-speed -c src/wc_api.c -o obj/wc_api.rel
if errorlevel 1 pause & exit
sdcc -I -mz80 --std-sdcc11 --opt-code-speed -c src/numbers.c -o obj/numbers.rel
if errorlevel 1 pause & exit
sdcc -I -mz80 --std-sdcc11 --opt-code-speed -c src/trdos.c -o obj/trdos.rel
if errorlevel 1 pause & exit
sdcc -I -mz80 --std-sdcc11 --opt-code-speed -c src/hobeta.c -o obj/hobeta.rel
if errorlevel 1 pause & exit

sdcc.exe -o obj/out.ihx --no-std-crt0 -mz80 --opt-code-speed --std-c11 -Wl-b_CODE=0x8003 -Wl-b_DATA=0xB000 obj/crt0.rel obj/main.rel obj/wc_api.rel obj/trdos.rel obj/hobeta.rel obj/numbers.rel

if exist obj/out.ihx hex2bin -e bin obj/out.ihx

for %%i in (obj/out.bin) do set sz=%%~zi
set /a sz=(%sz% - 1) / 512 + 1

sdcc -DNBLK0=%sz% --no-std-crt0 -mz80 --std-sdcc11 --opt-code-speed src/wc_h.c -o obj/hdr.ihx
if errorlevel 1 pause & exit
if exist obj/hdr.ihx hex2bin obj/hdr.ihx >nul

copy /b obj\hdr.bin + obj\out.bin TRDisp.wmf >nul

