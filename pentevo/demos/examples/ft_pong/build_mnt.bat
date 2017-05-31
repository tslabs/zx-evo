@echo off

set PRJ=main
set INC=..\includes

sjasmplus.exe --inc=%INC% --lst=%PRJ%.lst %PRJ%.asm
spgbld.exe -b spg.ini %PRJ%.spg
trdtool # %PRJ%.trd
trdtool + %PRJ%.trd ../includes/boot.$b
copy /b %PRJ%.bin code.C
trdtool + %PRJ%.trd code.C

rs232mnt.exe -a %PRJ%.trd -com com6 -baud 115200 -slowpoke
