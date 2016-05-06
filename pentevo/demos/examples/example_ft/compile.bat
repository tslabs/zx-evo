@echo off

copy example_ft_c.trd example_ft.trd

set PRJ=main
set SJ=..\..\..\tools\sjasmplus\sjasmplus.exe
set INC=..\includes

%SJ% --inc=%INC% --lst=%PRJ%.lst %PRJ%.asm

mount.bat

pause
