del zxevo_fw.bin
del zxevo_fw_r0bat.bin

del *.o
rd /q /s dep
copy /y ..\kbmap_r0bat.c ..\kbmap.c
make

ren zxevo_fw.bin zxevo_fw_r0bat.bin

del *.o
rd /q /s dep
copy /y ..\kbmap_nedopc.c ..\kbmap.c
make

