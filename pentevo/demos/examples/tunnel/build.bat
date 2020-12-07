@echo off

set PRJ=main
set INC=..\includes

sjasmplus.exe --inc=%INC% --lst=%PRJ%.lst %PRJ%.asm
spgbld.exe -b spg.ini %PRJ%.spg -c 0 >nul

del *.c
ren %PRJ%.bin code.C
REM ..\..\..\tools\finesplit\finesplit.exe dl.bin
REM ren dl.bin.00 dl0.C
REM ren dl.bin.01 dl1.C
REM ren dl.bin.02 dl2.C

trdtool # %PRJ%.trd
trdtool + %PRJ%.trd boot.$b code.C
REM trdtool + %PRJ%.trd boot.$b code.C dl0.C dl1.C dl2.C

pause
