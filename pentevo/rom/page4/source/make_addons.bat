@ECHO OFF

cd ..\..\fat\source

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_micro_boot_fat.a80
..\..\..\tools\asw\bin\p2bin make_micro_boot_fat.p micro_boot_fat.rom -r $-$ -k

..\..\..\tools\mhmt\mhmt -mlz micro_boot_fat.rom micro_boot_fat_pack.rom

cd ..\..\page4\source

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L addons.a80
..\..\..\tools\asw\bin\p2bin addons.p ..\addons.rom -r $-$ -k

pause