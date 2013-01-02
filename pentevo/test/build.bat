@echo off

"%IAR%\az80.exe" func.asm
"%IAR%\iccz80.exe" -ml -uua -z9 -xDFT -T -i -A -q -K -I"%IAR%/../inc/" main.c -L
"%IAR%\xlink.exe" -f main.xcl -l main.html -o main.hex main.r01 func.r01 "%IAR%\..\lib\clz80.r01"
del *.r01
objcopy -I ihex -O binary -j.sec1 main.hex main1.bin
objcopy -I ihex -O binary -j.sec2 main.hex main2.bin

copy /b main1.bin + main2.bin mem.C

pause
