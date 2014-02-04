echo off

set IAR=c:\Tools\PROG\IAR ewz80\z80\bin

mhmt.exe -hst 866_code.fnt
bin2defb.exe 866_code.fnt.hst 866_code.asm
mhmt.exe -hst  rslsys.bin
bin2defb.exe rslsys.bin.hst rslsys.asm
mhmt.exe -hst sysvars.bin
bin2defb.exe sysvars.bin.hst sysvars.asm
bin2defb.exe tsfat.bin tsfat.asm

"%IAR%\az80.exe" ts-bios.asm -l ts-bios.lst
"%IAR%\xlink.exe" -Hff -f ts-bios.xcl ts-bios.r01
if ERRORLEVEL 1 pause
del ts-bios.r01

"%IAR%\az80.exe" starter.asm
"%IAR%\xlink.exe" -f starter.xcl starter.r01
del starter.r01

copy /b header.bin + starter.bin "../bin/TS-BIOS.$C"
rem del starter.bin

copy /b ts-bios.bin + trdos504T.rom + 128.rom "../bin/ts-bios.rom"
copy /b ts-bios.bin + trdos504T.rom + 128.rom + zxevo_upper.rom "../bin/zxevo.rom"
copy /b ts-bios.bin + trdos504T.rom + glukpen.rom + 48.rom "../bin/ts-bios-gluk.rom"
copy /b ts-bios.bin + trdos504T.rom + qc3_11.rom + 48.rom "../bin/ts-bios-qc311.rom"
copy /b ts-bios.bin + trdos504T.rom + rc1_96.rom + 48.rom "../bin/ts-bios-rc196.rom"

copy ts-bios.bin "../bin/ts-bios.bin"
del ts-bios.bin

