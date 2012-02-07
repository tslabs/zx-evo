az80.exe bios.a80 -l bios.lst
xlink.exe -f bios.xcl bios.r01
del bios.r01

az80.exe starter.a80 -l starter.lst
xlink.exe -f starter.xcl starter.r01
del starter.r01

copy /b header.bin + starter.bin bios.c$
rem del starter.bin

pause