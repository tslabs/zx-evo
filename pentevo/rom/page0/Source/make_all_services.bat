@ECHO OFF

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_micro_boot_fat.a80
..\..\..\tools\asw\bin\p2bin make_micro_boot_fat.p micro_boot_fat.rom -r $-$ -k

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_main.a80
..\..\..\tools\asw\bin\p2bin make_main.p main.rom -r $-$ -k

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_cmosset.a80
..\..\..\tools\asw\bin\p2bin make_cmosset.p cmosset.rom -r $-$ -k

rem ..\..\..\tools\sjasmplus\sjasmplus --sym=sym.log --lst=dump.log -isrc make_micro_boot_fat.a80
rem ..\..\..\tools\sjasmplus\sjasmplus --sym=sym.log --lst=dump.log -isrc make_main.a80
rem ..\..\..\tools\sjasmplus\sjasmplus --sym=sym.log --lst=dump.log -isrc make_cmosset.a80

..\..\..\tools\mhmt\mhmt -mlz main.rom main_pack.rom
..\..\..\tools\mhmt\mhmt -mlz cmosset.rom cmosset_pack.rom
..\..\..\tools\mhmt\mhmt -mlz altstd.bin chars_pack.bin

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_all_services.a80
..\..\..\tools\asw\bin\p2bin make_all_services.p ../services.rom -r $-$ -k

rem ..\..\..\tools\sjasmplus\sjasmplus --sym=sym.log --lst=dump.log -isrc make_all_services.a80

del micro_boot_fat.rom
REM del micro_boot_fat_pack.rom
del main.rom
del cmosset.rom

del main_pack.rom
del cmosset_pack.rom

copy /B /Y ..\services.rom d:\unrealspeccy\

pause
