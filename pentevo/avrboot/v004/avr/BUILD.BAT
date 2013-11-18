@echo on
if exist zxevo_bl.hex del /f /q zxevo_bl.hex
if exist fpga.mlz del /f /q fpga.mlz
if exist fpga.inc del /f /q fpga.inc
if exist evotitle.inc del /f /q evotitle.inc
..\..\..\tools\bin2avr\bin2avr.exe evotitle.ans
..\..\..\tools\mhmt\mhmt.exe -mlz -maxwin4096 ..\fpga\quartus\main.rbf fpga.mlz
..\..\..\tools\bin2avr\bin2avr.exe fpga.mlz
..\..\..\tools\avrasm\avrasm32.exe -fI boot_evo.asm -l boot_evo.lst
if exist boot_evo.hex ..\..\..\tools\crcbldr\trunk\release\crcbldr.exe boot_evo.hex
if exist boot_evo.hex del /f /q boot_evo.hex
