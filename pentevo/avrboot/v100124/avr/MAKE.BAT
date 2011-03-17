call clean.bat
@echo on
..\..\..\tools\bin2avr\bin2avr.exe evotitle.ans 0
..\..\..\tools\mhmt\mhmt.exe -mlz -maxwin4096 ..\fpga\main.rbf fpga.mlz
..\..\..\tools\bin2avr\bin2avr.exe fpga.mlz
..\..\..\tools\avra\avra.exe -fI boot_evo.asm -l boot_evo.lst
if not exist boot_evo.hex goto err
..\..\..\tools\crcbldr\crcbldr.exe boot_evo.hex version.txt %1
del /f /q boot_evo.hex
:err