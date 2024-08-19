xtensa-esp32-elf-g++ -c -fPIC -Os -ffunction-sections -fdata-sections lib.cpp -o lib.o || exit /b
xtensa-esp32-elf-g++ -o lib.elf lib.o -fPIE -pie -flto -nostartfiles -e lib -Wl,--gc-sections || exit /b
xtensa-esp32-elf-strip -R .interp -R .hash -R .dynsym -R .dynstr -R .got.loc -R .eh_frame -R .dynamic -R .got -R .comment -R .xtensa.info -R .xt.lit -R .xt.prop lib.elf
xtensa-esp32-elf-objdump -xSrsh lib.elf >lib.lst
xtensa-esp32-elf-readelf -a lib.elf >lib1.lst
mhmt.exe -hst lib.elf lib.hst
python dfl.py lib.elf lib.dfz
python bin2h16.py lib.dfz ../src/lib.h lib
