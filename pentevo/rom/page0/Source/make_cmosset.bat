@ECHO OFF

..\..\..\tools\asw\bin\asw -cpu z80undoc -U -L make_cmosset.a80
..\..\..\tools\asw\bin\p2bin make_cmosset.p cmosset.rom -r $-$ -k

REM ..\..\..\tools\sjasmplus\sjasmplus --sym=sym.log --lst=dump.log -isrc make_cmosset.a80

..\..\..\tools\mhmt\mhmt -mlz cmosset.rom cmosset_pack.rom

pause