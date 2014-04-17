@echo off

set PRJ1=example02a
set PRJ2=example02b
set SJ=..\..\..\tools\sjasmplus\sjasmplus.exe
set INC=..\includes

%SJ% --inc=%INC% --lst=%PRJ1%.lst %PRJ1%.asm
%SJ% --inc=%INC% --lst=%PRJ2%.lst %PRJ2%.asm
pause
