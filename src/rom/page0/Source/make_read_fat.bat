@ECHO OFF

rem тестовая сборка фата и драйверов

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_read_fat.a80
..\..\..\tools\asw\bin\p2bin make_read_fat.p read_fat.rom -r $-$ -k

REM ..\..\..\tools\sjasmplus\sjasmplus --sym=sym.log --lst=dump.log -isrc make_read_fat.a80

..\..\..\tools\mhmt\mhmt -mlz read_fat.rom read_fat_pack.rom

pause