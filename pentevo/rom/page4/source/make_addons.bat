@ECHO OFF

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_addons.a80
..\..\..\tools\asw\bin\p2bin make_addons.p ..\addons.rom -r $-$ -k

pause