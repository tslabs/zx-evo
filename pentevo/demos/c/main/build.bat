@echo off

set CC="%IAR%\iccz80.exe" -ml -s8 -uu -P -v0 -e -xDFT -T -i -q -K -L -I"%IAR%/../inc/" -I"../pff/" -I"../tjpg/" -I"../tsconf/" -I"../common/"
set LC="%IAR%\xlink.exe" -f

%CC% main.c
%CC% ..\pff\pff.c
%CC% ..\pff\diskio.c
%CC% ..\tjpg\tjpgd.c
%CC% ..\tsconf\tsconf.c
%LC% main.xcl -l main.html -o main.hex main.r01 pff.r01 diskio.r01 tjpgd.r01 tsconf.r01 "%IAR%\..\lib\clz80.r01"
del *.r01

objcopy -I ihex -O binary -j.sec2 main.hex maincode.C

pause
