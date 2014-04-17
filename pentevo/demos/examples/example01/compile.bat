
set PRJ=example01
set SJ=..\..\..\tools\sjasmplus\sjasmplus.exe
set INC=..\includes

%SJ% --inc=%INC% --lst=%PRJ%.lst %PRJ%.asm
pause
