
@ECHO OFF

REM ..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_bas48_only.a80
REM ..\..\..\tools\asw\bin\p2bin make_bas48_only.p ..\basic48_only.rom -r $-$ -k

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_bas48_128.a80
..\..\..\tools\asw\bin\p2bin make_bas48_128.p ..\basic48.rom -r $-$ -k

pause