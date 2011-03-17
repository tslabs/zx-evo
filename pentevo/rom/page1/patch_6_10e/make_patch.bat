@ECHO OFF

REM ..\..\..\tools\sjasmplus\sjasmplus --sym=sym.log --lst=dump.log -isrc make_patch.a80

..\..\..\tools\sjasmplus\sjasmplus -isrc make_patch.a80

rem ..\mhmt -mlz filename.rom filename_pack.rom

pause