
@echo off

set PRJ=ts-bios

if exist obj rd /s /q obj
md obj

mhmt.exe -hst 866_code.fnt "obj\866_code.fnt.hst" >nul
mhmt.exe -hst rslsys.bin "obj\rslsys.bin.hst" >nul
mhmt.exe -hst sysvars.bin "obj\sysvars.bin.hst" >nul

sjasmplus.exe --inc=obj --lst=obj\%PRJ%.lst %PRJ%.asm

copy /b "obj\ts-bios.bin" + "rom\trdos504T.rom" + "rom\128.rom" "..\bin\ts-bios.rom" >nul
copy /b "obj\ts-bios.bin" + "rom\trdos504T.rom" + "rom\128.rom" + "rom\zxevo.rom.1" "..\bin\zxevo.rom" >nul
copy /b "obj\ts-bios.bin" + "rom\trdos504T.rom" + "rom\glukpen.rom" + "rom\48.rom" "..\bin\ts-bios-gluk.rom" >nul
copy /b "obj\ts-bios.bin" + "rom\trdos504T.rom" + "rom\qc3_11.rom" + "rom\48.rom" "..\bin\ts-bios-qc311.rom" >nul
copy /b "obj\ts-bios.bin" + "rom\trdos504T.rom" + "rom\rc1_96.rom" + "rom\48.rom" "..\bin\ts-bios-rc196.rom" >nul

pause
