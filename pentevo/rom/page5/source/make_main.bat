@ECHO OFF

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L rst8service.a80
..\..\..\tools\asw\bin\p2bin rst8service.p ..\rst8service.rom -r $-$ -k

pause