@ECHO OFF

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_micro_boot_fat.a80
..\..\..\tools\asw\bin\p2bin make_micro_boot_fat.p micro_boot_fat.rom -r $-$ -k

rem ..\..\..\tools\sjasmplus\sjasmplus --sym=sym.log --lst=dump.log -isrc make_micro_boot_fat.a80

..\..\..\tools\mhmt\mhmt -mlz micro_boot_fat.rom micro_boot_fat_pack.rom

pause