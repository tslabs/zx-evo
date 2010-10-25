call clean.bat
@echo on
..\..\tools\mhmt\mhmt.exe -mlz -maxwin4096 ..\fpga\main.rbf fpga.mlz
..\..\tools\bin2avr\bin2avr.exe fpga.mlz
..\..\tools\avra\avra.exe -fI evoflash.asm -l evoflash.lst
if not exist evoflash.hex goto err
..\..\tools\make_fw\make_fw.exe evoflash.hex evoflash.eep.hex zxevo_fw.bin version.txt %1
rem del /f /q evoflash.hex
:err