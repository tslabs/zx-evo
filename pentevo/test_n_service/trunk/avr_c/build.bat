@echo off
if exist fpgadat1.mlz del fpgadat1.mlz
..\..\..\tools\mhmt\mhmt.exe -mlz -maxwin2048 ..\fpga\main.rbf fpgadat1.mlz
rem set path=C:\WinAVR\utils\bin;C:\WinAVR\bin
set path=C:\AVR_Toolchain\bin
make.exe clean all
cd bin
if exist proga.hex ..\..\..\..\tools\make_fw\make_fw.exe proga.hex proga.eep zxevo_fw.bin ..\version.txt
cd ..
