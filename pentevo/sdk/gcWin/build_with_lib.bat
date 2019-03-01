
set path=%path%;%SDCC%\bin\

sdasz80 -g -o obj\crt0.rel src\crt0.s
if errorlevel 1 pause & exit

sdcc.exe --std-c11 -mz80 --no-std-crt0 --opt-code-speed -c src\tsio.c -o obj\tsio.rel
if errorlevel 1 pause & exit

sdcc.exe -o bin\out.hex --std-c11 -mz80 --no-std-crt0 --opt-code-speed -Wl-b_CODE=0x0200 -Wl-b_DATA=0xB000 obj\crt0.rel obj\tsio.rel src\main.c lib\gcwin.lib
if errorlevel 1 pause & exit

if exist bin\out.hex hex2bin -e bin bin\out.hex

cp bin\out.bin out.bin

spgbld -b res\spg.ini gcWin.spg
