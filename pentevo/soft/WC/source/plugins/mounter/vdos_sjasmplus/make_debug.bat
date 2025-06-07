rem @echo off

rem Build vdos, and run it in unreal
rem 1. Building vdos (emu.bin)
rem 2. Make a copy of original unpatched mounter.wmf
rem 3. Split original mounter.wmf by plugin, vdos and boot
rem 4. Merge plugin, new vdos and boot (3 + 4 = replacing a part of original plugin with compiled emu.bin)
rem 5. Copy new plugin to WC binary folder then make new wc.img
rem 6. Copy wc.img to unreal folder then start unreal

set WC=..\..\..\..
set BINARY=%WC%\exe
set TOOLS=..\..\..\..\..\..\tools
set UNREAL=..\..\..\..\..\..\unreal\Unreal\bin\Debug

%TOOLS%\sjasmplus.exe emu.asm --lst=emu.lst
copy MOUNTER_ORIGINAL.WMF MOUNTER.WMF
%TOOLS%\fsplit\fsplit.exe MOUNTER.WMF 19968 16384
del MOUNTER.WMF
copy /b MOUNTER.WMF.0+emu.bin+MOUNTER.WMF.2 MOUNTER.WMF
del MOUNTER.WMF.0 & del MOUNTER.WMF.1 & del MOUNTER.WMF.2
del %BINARY%\WC\MOUNTER.WMF
move MOUNTER.WMF %BINARY%\WC\MOUNTER.WMF
%TOOLS%\robimg\robimg.exe -p="%WC%\wc.img" -s=102400 -C="%BINARY%"
del %UNREAL%\wc.img
copy %WC%\wc.img %UNREAL%\wc.img
%UNREAL%\unreal.exe