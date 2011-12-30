@ECHO OFF

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L main.a80
..\..\..\tools\asw\bin\p2bin main.p main.rom -r $-$ -k

..\..\..\tools\mhmt\mhmt -mlz main.rom main_pack.rom

pause