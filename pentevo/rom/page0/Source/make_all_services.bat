@ECHO OFF

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_micro_boot_fat.a80
..\..\..\tools\asw\bin\p2bin make_micro_boot_fat.p micro_boot_fat.rom -r $-$ -k

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_main.a80
..\..\..\tools\asw\bin\p2bin make_main.p main.rom -r $-$ -k

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_cmosset.a80
..\..\..\tools\asw\bin\p2bin make_cmosset.p cmosset.rom -r $-$ -k

..\..\..\tools\mhmt\mhmt -mlz main.rom main_pack.rom
..\..\..\tools\mhmt\mhmt -mlz cmosset.rom cmosset_pack.rom
..\..\..\tools\mhmt\mhmt -mlz chars_eng.bin chars_pack.bin

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_all_services.a80
..\..\..\tools\asw\bin\p2bin make_all_services.p ../services.rom -r $-$ -k

del micro_boot_fat.rom
del main.rom
del cmosset.rom

del main_pack.rom
del cmosset_pack.rom

copy /B /Y ..\services.rom d:\unrealspeccy\

pause
