echo off

"%IAR%\az80.exe" ts-bios.asm -l ts-bios.lst
"%IAR%\xlink.exe" -Hff -f ts-bios.xcl ts-bios.r01
del ts-bios.r01

"%IAR%\az80.exe" starter.asm
"%IAR%\xlink.exe" -f starter.xcl starter.r01
del starter.r01

copy /b header.bin + starter.bin "../bin/TS-BIOS.$C"
rem del starter.bin

copy /b ts-bios.bin + trdos504T.rom + 128.rom "../bin/ts-bios.rom"

copy /b ts-bios.bin + trdos504T.rom + glukpen.rom + 48.rom "../bin/ts-bios-gluk.rom"

copy ts-bios.bin "../bin/ts-bios.bin"
del ts-bios.bin

pause