
copy /B /Y page0\services.rom+page1\dos6_12e_patch.rom+page2\*.rom+page3\*.rom zxevo_pen.rom
copy /B /Y page3\*.rom+page1\dos6_12e_patch.rom+page2\*.rom+page0\services.rom zxevo_atm.rom

..\tools\addcrc\addcrc zxevo_pen.rom
ren crc.bin crc_pen.bin
..\tools\addcrc\addcrc zxevo_atm.rom
ren crc.bin crc_atm.bin

..\tools\sjasmplus\sjasmplus --sym=sym.log --lst=dump.log -isrc build_update.a80
..\tools\addcrc\addcrc header.rom

copy /B /Y header.rom+crc.bin+zxevo_pen.rom+zxevo_atm.rom zxevo_rom.upd

del crc_pen.bin
del crc_atm.bin
del crc.bin
del header.rom
del zxevo_pen.rom
del zxevo_atm.rom

PAUSE