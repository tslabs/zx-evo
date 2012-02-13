
                org vars

                
; -------- BIOS

kbd_del         defs 1
kbd_last        defs 1
fld_curr        defs 1

        
; -------- FAT driver

nsdc            defs 1      ; номер сектора в clus
eoc             defs 1      ; флаг конца цепочки
abt             defs 1
nr0             defs 2      ; 4;кол-во грузимых секторов
lstse           defs 4
rezde           defs 2
clhl            defs 2
clde            defs 2
llhl            defs 4
lthl            defs 2;last
ltde            defs 2
entry           defs 11     ;------ шаблон элемента каталога
clsde           defs 2
clshl           defs 2
brezs           defs 2      ; fat parameters
fsinf           defs 4      ;
bfats           defs 1
bftsz           defs 4
bsecpc          defs 2      ; use 1
brootc          defs 4
addtop          defs 4
sfat            defs 2
sdfat           defs 4
cuhl            defs 2
cude            defs 2
ldhl            defs 2      ; адрес с которого/в который идет запись/чтение
count           defs 1
duhl            defs 2
dude            defs 2
lstcat          defs 4      ; активный каталог
blknum          defs 4
dahl            defs 2      ; see hdd proc.
dade            defs 2
zes             defs 1

                org fat_bufs
secbu           defs 512
secbe
lobu            defs 512
lobe            defs 32

