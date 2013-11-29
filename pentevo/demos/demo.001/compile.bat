"%IAR%\az80.exe" demo001.asm -l demo001.lst
"%IAR%\xlink.exe" -f demo001.xcl demo001.r01
del demo001.r01
pause