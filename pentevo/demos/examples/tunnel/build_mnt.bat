@echo off

set PRJ=main
set INC=..\includes

sjasmplus.exe --inc=%INC% --lst=%PRJ%.lst %PRJ%.asm
spgbld.exe -b spg.ini %PRJ%.spg >nul

del *.c
..\..\..\tools\finesplit\finesplit.exe dl.bin
ren %PRJ%.bin code.C
ren dl.bin.00 dl0.C
ren dl.bin.01 dl1.C
ren dl.bin.02 dl2.C

trdtool # %PRJ%.trd
trdtool + %PRJ%.trd boot.$b code.C dl0.C dl1.C dl2.C
REM trdtool + %PRJ%.trd boot.$b code.C

rs232mnt.exe -a %PRJ%.trd -com com6 -baud 115200 -slowpoke
