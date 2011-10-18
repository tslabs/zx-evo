
@ECHO OFF

REM ..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L basic48.a80
REM ..\..\..\tools\asw\bin\p2bin basic48.p ..\basic48.rom -r $-$ -k

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L spec128_0.a80
..\..\..\tools\asw\bin\p2bin spec128_0.p ..\basic128.rom -r $-$ -k

pause