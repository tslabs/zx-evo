
                org vars

                
; -------- BIOS

kbd_del         defs 1      ; current value of autorepeat counter
last_key        defs 1      ; last pressed key
map_key         defs 1      ; key mapped to layout
key             defs 1      ; actually pressed key mapped to layout (affected by autorepead)
evt             defs 1      ; event code
fld_curr        defs 1      ; current field of setup
fld_top         defs 2      ; coordinates of 1st option in the field
fld_tab         defs 2      ; addr of tab options
fld_max         defs 1      ; number of options in field
fld0_pos        defs 1      ; cursor position in field0


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
lthl            defs 2      ;last
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

