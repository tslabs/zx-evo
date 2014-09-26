@echo off

REM --- Prepare folders ---
set OBJ=obj
set ROM=rom
if exist %OBJ% rd /s /q %OBJ%
md %OBJ%

REM --- Prepare binaries ---
mhmt.exe -hst 866_code.fnt "%OBJ%\866_code.fnt.hst" >nul
bin2defb.exe "%OBJ%\866_code.fnt.hst" "%OBJ%\866_code.inc" >nul
mhmt.exe -hst rslsys.bin "%OBJ%\rslsys.bin.hst" >nul
bin2defb.exe "%OBJ%\rslsys.bin.hst" "%OBJ%\rslsys.inc" >nul
mhmt.exe -hst sysvars.bin "%OBJ%\sysvars.bin.hst" >nul
bin2defb.exe "%OBJ%\sysvars.bin.hst" "%OBJ%\sysvars.inc" >nul
bin2defb.exe tsfat.bin "%OBJ%\tsfat.inc" >nul

REM --- Compile sources ---
set CC="%IAR%\iccz80.exe" -ml -s8 -uu -P -v0 -e -xDFT -T -i -q -K	-I"%IAR%\..\inc" -I"..\pff" -L"%OBJ%/"
set AC="%IAR%\az80.exe" -I"%OBJ%" -uu -L"%OBJ%/" -O"%OBJ%/"
set LC="%IAR%\xlink.exe" -cz80 -Fraw-binary -xmisn -Hff

%AC% ts-bios.asm
%LC% -Z(CODE)CODE=0000-3FFF "%OBJ%\ts-bios.r01" -o "%OBJ%\ts-bios.bin" -l "%OBJ%\ts-bios.map"

REM --- Make binaries ---
..\..\tools\fsplit\fsplit.exe %ROM%\zxevo.rom 65536

copy /b "%OBJ%\ts-bios.bin" + "%ROM%\trdos504T.rom" + "%ROM%\128.rom" "..\bin\ts-bios.rom"
copy /b "%OBJ%\ts-bios.bin" + "%ROM%\trdos504T.rom" + "%ROM%\128.rom" + "%ROM%\zxevo.rom.1" "..\bin\zxevo.rom"
copy /b "%OBJ%\ts-bios.bin" + "%ROM%\trdos504T.rom" + "%ROM%\glukpen.rom" + "%ROM%\48.rom" "..\bin\ts-bios-gluk.rom"
copy /b "%OBJ%\ts-bios.bin" + "%ROM%\trdos504T.rom" + "%ROM%\qc3_11.rom" + "%ROM%\48.rom" "..\bin\ts-bios-qc311.rom"
copy /b "%OBJ%\ts-bios.bin" + "%ROM%\trdos504T.rom" + "%ROM%\rc1_96.rom" + "%ROM%\48.rom" "..\bin\ts-bios-rc196.rom"

REM --- Clean up ---
del "%ROM%\*.0"
del "%ROM%\*.1"

pause
