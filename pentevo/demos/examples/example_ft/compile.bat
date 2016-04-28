@echo off

set PRJ=main
set SJ=..\..\..\tools\sjasmplus\sjasmplus.exe
set INC=..\includes

%SJ% --inc=%INC% --lst=%PRJ%.lst %PRJ%.asm
pause
