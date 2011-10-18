
@ECHO OFF

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L aypr4bas.a80
..\..\..\tools\asw\bin\p2bin aypr4bas.p aypr4bas.rom -r $-$ -k

pause