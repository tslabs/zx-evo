@echo off

REM --- Prepare folders ---
set OBJ=obj
set ROM=rom
if exist %OBJ% rd /s /q %OBJ%
md %OBJ%

REM --- Prepare binaries ---
mhmt.exe -hst 866_code.fnt "%OBJ%\866_code.fnt.hst" >nul
bin2defb.exe "%OBJ%\866_code.fnt.hst" "%OBJ%\866_code.inc" >nul
mhmt.exe -hst rslsys.bin "%OBJ%\rslsys.bin.hst" >nul
bin2defb.exe "%OBJ%\rslsys.bin.hst" "%OBJ%\rslsys.inc" >nul
mhmt.exe -hst sysvars.bin "%OBJ%\sysvars.bin.hst" >nul
bin2defb.exe "%OBJ%\sysvars.bin.hst" "%OBJ%\sysvars.inc" >nul
bin2defb.exe tsfat.bin "%OBJ%\tsfat.inc" >nul

REM --- Compile sources ---
set CC="%IAR%\iccz80.exe" -ml -s8 -uu -P -v0 -e -xDFT -T -i -q -K -I"%IAR%/../inc/" -I"pff/" -L"%OBJ%/" -O"%OBJ%/"
set AC="%IAR%\az80.exe" -I"%OBJ%/" -uu -L"%OBJ%/" -O"%OBJ%/"

%AC% start.asm
%AC% ts-bios.asm
%AC% biosapi.asm
%CC% main.c
%CC% pff\pff.c
%CC% pff\diskio.c

REM --- Link sources ---
set LC="%IAR%\xlink.exe" -cz80 -Fraw-binary -xehmisn -Hff
REM set LC="%IAR%\xlink.exe" -cz80 -Fintel-extended -xehmisn -Hff

set LC=%LC% -Z(CODE)START=0000-0007
set LC=%LC% -Z(CODE)RST08=0008-000F
set LC=%LC% -Z(CODE)RST10=0010-0017
set LC=%LC% -Z(CODE)RST18=0018-001F
set LC=%LC% -Z(CODE)RST20=0020-0027
set LC=%LC% -Z(CODE)RST28=0028-002F
set LC=%LC% -Z(CODE)RST30=0030-0037
set LC=%LC% -Z(CODE)RST38=0038-0065
set LC=%LC% -Z(CODE)NMISR=0066-007F
set LC=%LC% -Z(CODE)CODE,RCODE,CDATA0,CONST,CSTR,CCSTR=0080-1FFF
set LC=%LC% -Z(CODE)TSFAT=2000-3FFF

set LC=%LC% -Z(DATA)FATBUF=4200-47FF
set LC=%LC% -Z(DATA)IDATA0,UDATA0,ECSTR=5D00-5FFF

set LC=%LC% "%OBJ%\start.r01"
set LC=%LC% "%OBJ%\ts-bios.r01"
set LC=%LC% "%OBJ%\biosapi.r01"
REM set LC=%LC% "%OBJ%\main.r01"
REM set LC=%LC% "%OBJ%\pff.r01"
REM set LC=%LC% "%OBJ%\diskio.r01"
set LC=%LC% "clz80.r01"

%LC% -o "%OBJ%\ts-bios.bin" -l "%OBJ%\ts-bios.html"

REM --- Make binaries ---
..\..\tools\fsplit\fsplit.exe %ROM%\zxevo.rom 65536

copy /b "%OBJ%\ts-bios.bin" + "%ROM%\trdos504T.rom" + "%ROM%\128.rom" "..\bin\ts-bios.rom" >nul
copy /b "%OBJ%\ts-bios.bin" + "%ROM%\trdos504T.rom" + "%ROM%\128.rom" + "%ROM%\zxevo.rom.1" "..\bin\zxevo.rom" >nul
copy /b "%OBJ%\ts-bios.bin" + "%ROM%\trdos504T.rom" + "%ROM%\glukpen.rom" + "%ROM%\48.rom" "..\bin\ts-bios-gluk.rom" >nul
copy /b "%OBJ%\ts-bios.bin" + "%ROM%\trdos504T.rom" + "%ROM%\qc3_11.rom" + "%ROM%\48.rom" "..\bin\ts-bios-qc311.rom" >nul
copy /b "%OBJ%\ts-bios.bin" + "%ROM%\trdos504T.rom" + "%ROM%\rc1_96.rom" + "%ROM%\48.rom" "..\bin\ts-bios-rc196.rom" >nul

REM --- Clean up ---
del "%ROM%\*.0"
del "%ROM%\*.1"

pause
