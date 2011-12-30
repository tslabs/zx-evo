@ECHO OFF

cd page0\source

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L main.a80
..\..\..\tools\asw\bin\p2bin main.p main.rom -r $-$ -k

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_cmosset.a80
..\..\..\tools\asw\bin\p2bin make_cmosset.p cmosset.rom -r $-$ -k

..\..\..\tools\mhmt\mhmt -mlz main.rom main_pack.rom
..\..\..\tools\mhmt\mhmt -mlz cmosset.rom cmosset_pack.rom
..\..\..\tools\mhmt\mhmt -mlz chars_eng.bin chars_pack.bin

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L services.a80
..\..\..\tools\asw\bin\p2bin services.p ..\services.rom -r $-$ -k

del main.rom
del cmosset.rom
del main_pack.rom
del cmosset_pack.rom
del ..\..\fat\source\micro_boot_fat.rom


copy /B /Y ..\services.rom d:\unrealspeccy\

pause
