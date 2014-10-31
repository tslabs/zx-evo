Build:
------
Build two versions of firmware (zxevo_fw.bin and zxevo_fw_vdac.bin): make
Build and flash normal version of firmware: make norm flash
Build and flash VDAC version of firmware: make vdac flash
Flash ATmega128 fuses: make fuses

USBasp programmer is used by default. You can change it in the Makefile.

Usage:
------
To switch between configurations press:
	L_Ctrl + Alt + Insert
