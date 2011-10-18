@ECHO OFF

rem тестовая сборка основного куска сервиса

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_micro_boot_fat.a80
..\..\..\tools\asw\bin\p2bin make_micro_boot_fat.p micro_boot_fat.rom -r $-$ -k

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_main.a80
..\..\..\tools\asw\bin\p2bin make_main.p main.rom -r $-$ -k

..\..\..\tools\mhmt\mhmt -mlz main.rom main_pack.rom

pause