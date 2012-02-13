
p_conf  equ h'77
p_data  equ h'57
cmd_12  equ h'4c
cmd_18  equ h'52
cmd_25  equ h'59
cmd_55  equ h'77
cmd_58  equ h'7a
cmd_59  equ h'7b
acmd_41 equ h'69


;---------------------------------------
start
        ld hl, 0
        ld (lstcat), hl
        ld (lstcat + 2), hl
        call ide_ini
        xor a
        call sel_dev
        call hdd
        ld hl, file1
        ld de, entry
        ld bc, 11
        ldir
        call srhdrn
        ld a, 1
        jr nz, $ + 4
        ld a, 7
        out (254), a
        ld (lobu), hl
        ld (lobu + 2), de
        ld hl, lobu
        call gipag
        ld hl, lobu
        ld b, 1
        call load512
        ld hl, lobu + 17
        ld de, (lobu + 9)
        push de
        ld bc, 512-17
        ldir
        ex de, hl
thg     ld b, 1
        call load512
        ld a, (eoc)
        cp h'0f
        jr nz, thg
        ret

;---------------------------------------
;read data from fat32 (flow)
        
load512 ;i:        hl - address
;           b - lenght (512b blocks)
;     cuhl(4) - clusnum (if eoc + nsdc=0!)
;        o:        hl - new value
;           a - endofchain
        xor a
        ld (abt), a
        call lprex
        jr nz, rh
kukry   push bc
        ld hl, (ldhl)
        ld a, 1
        call newcla
        pop bc
        djnz kukry
rh      ld hl, (ldhl)
        ld a, (eoc)
        ret

;positioning to cluster,  if needed
        
lprex   ld (ldhl), hl
        ld a, (nsdc)
        or a
        jr nz, rx
        ld a, (eoc)
        or a
        ret nz
        push bc
        ld hl, cuhl
        call gipag
        pop bc
        ret

rx      xor a
        ret

;-------
newcla  ld (nr0), a
        push hl
        ld hl, (lthl)
        ld de, (ltde)
        call proz
        pop hl
        ld a, (nr0)
nw0     call rddse;       read sector(s)
        ld (ldhl), hl;   updating address
        ld hl, lthl
        ld de, llhl
        ld bc, 4
        ldir
        ld hl, (lthl)
        ld de, (ltde)
        ld bc, (nr0)
        add hl, bc
        jr nc, $ + 3
        inc de
        ld (lthl), hl
        ld (ltde), de
        ld hl, nsdc
        ld a, c
        add a, (hl)
        ld (hl), a
        ld bc, (bsecpc)
        cp c
        ret c;      end of cluster?
;                        yes!
        ld hl, (cuhl)
        ld de, (cude)
        call curit
        call gipag
        ret z
eofc    pop bc
        pop bc
        jp rh
        
;------read sector from fat
curit   ;i:        [de, hl]-cluster number
;        o:        secbu(512)
;          hl-poz in secbu where cluster
        call del128
        sla c
        rl b
        sla c
        rl b
        push bc
        ld (lstse + 2), de
        ld (lstse), hl
        ld bc, (sfat)
        call add4b
        call xspoz
        call xpozi
        ld hl, secbu
        ld a, 1
        call rddse
        pop bc
        ld hl, secbu
        add hl, bc
        ret

;----pos. to first sector of cluster
gipag   ;i:        hl(4) - cluster number
        call tos
        ld e, (hl)
        inc hl
        ld d, (hl)
        inc hl
        ld a, (hl)
        inc hl
        ld h, (hl)
        ld l, a
        or h
        or e
        or d
        jr z, rdir
        
        ld a, h
        cp h'0f
        jr z, mdc
        ex de, hl
pom     ld (cuhl), hl
        ld (cude), de
        ld bc, 2
        or a
        sbc hl, bc
        jr nc, $ + 3
        dec de
        ld a, (bsecpc)
        call umnox2
        ld bc, (sdfat)
        call add4b
        ex de, hl
        ld bc, (sdfat + 2)
        add hl, bc
        ex de, hl
        call xspoz
        call xpozi
        xor a
        ret

rdir    ld hl, (brootc)
        ld de, (brootc + 2)
        jr pom
        
mdc     ld (eoc), a
        or a
        ret

;---getting absolute position of sec
xspoz   ld bc, (addtop)
        ld (clhl), bc
        ld bc, (addtop + 2)
        ld (clde), bc
        jp add4bf
        
;-----searching entry in active dir by name
srhdrn  ;i:        lstcat(4) - active dir
;          entry - name (11 chars)
;        o:        entry(32)
;          [de, hl] - cluster number

        ld hl, lstcat
        ld de, cuhl
        ld bc, 4
        ldir
        call tos
hdr     ld a, (eoc)
        cp h'0f
        ret z
        ld hl, lobu
        ld b, 1
        call load512
        ld (hl), 0
        ld hl, lobu-32
        call vega
        ret nz
        ld a, h
        cp high(lobu) + 2
        jr hdr
        
;-------
vega    ld bc, 32
        add hl, bc
        ld a, (hl)
        or a
        ret z
        call cheb
        jr nz, vega
        ld a, 1
        or a
        ret

;-------
cheb    push hl
        ld de, entry
        ld b, 11
        call chee
        pop hl
        ret nz
        ld de, entry
        ld bc, 32
        ldir
        ld hl, (clshl)
        ld de, (clsde)
        xor a
        ret

chee    ld a, (de)
        cp (hl)
        ret nz
        inc hl
        inc de
        djnz chee
        ret

;---------------------------------------
;search partition
        
hdd     ;i:        none
;        o:        nz - fat32 not found
;           z - all fat32 vars are
;               initialized
        ld de, 0
        ld hl, 0
        ld (cuhl), hl
        ld (cude), hl
        ld (dahl), hl
        ld (dade), hl
        ld (duhl), hl
        ld (dude), hl
        call xpozi
        ld hl, lobu
        ld a, 1
        call rddse
        ld a, 3
        ld (count), a
        ld (zes), a
        ld hl, lobu + 446 + 4
        ld de, 16
        ld b, 4
kko     ld a, (hl)
        cp h'05
        jr z, okk
        cp h'0b
        jr z, okk
        cp h'0c
        jr z, okk
        cp h'0f
        jr z, okk
        add hl, de
        djnz kko
fhdd    ld a, (zes)
        or a
        jp z, nhdd1
        ld de, (dade)
        ld hl, (dahl)
        call xpozi
        ld hl, lobu
        ld a, 1
        call rddse
        ld hl, count
        dec (hl)
        jp z, nhdd
        ld hl, lobu + 446 + 16
        ld b, 16
        xor a
        or (hl)
        inc hl
        djnz $-2
        jp nz, nhdd
        ld hl, (lobu + 446 + 16 + 8)
        ld de, (lobu + 446 + 16 + 8 + 2)
        ld (clhl), hl
        ld (clde), de
        ld hl, (dahl)
        ld de, (dade)
        call add4bf
        ld (dade), de
        ld (dahl), hl
        call xpozi
        ld hl, lobu
        ld a, 1
        call rddse
        ld hl, (lobu + 446 + 8)
        ld de, (lobu + 446 + 8 + 2)
        call add4bf
        jr ldbpb
        
okk     inc hl
        inc hl
        inc hl
        inc hl
        ld e, (hl)
        inc hl
        ld d, (hl)
        inc hl
        ld a, (hl)
        inc hl
        ld h, (hl)
        ld l, a
        ex de, hl
ldbpb   ld (addtop), hl
        ld (addtop + 2), de
        call xpozi
        ld hl, lobu ;load bpb sector
        ld a, 1
        call rddse
        ld hl, lobu + 3
        ld b, 6
        ld a, h'1d
five    cp (hl)
        inc hl
        jp nc, fhdd
        djnz five
        
        ld hl, (lobu + 11)
        ld a, h
        dec a
        dec a
        or l
        jp nz, fhdd
        ld a, (lobu + 13)
        or a
        jp z, fhdd
        ld a, (lobu + 14)
        or a
        jp z, fhdd
        ld a, (lobu + 16)
        or a
        jp z, fhdd
        ld hl, (lobu + 17)
        ld a, h
        or l
        ld hl, (lobu + 19)
        or h
        or l
        ld hl, (lobu + 22)
        or h
        or l
        jp nz, fhdd
        ld hl, (lobu + 36)
        or h
        or l
        ld hl, (lobu + 36 + 2)
        or h
        or l
        jp z, fhdd

        ld a, (lobu + 13)
        ld (bsecpc), a
        ld b, 8
        srl a
        jr c, ner
        djnz $-4
        ld a, 1
ner     or a
        jp nz, fhdd
        ld hl, (lobu + 14)
        ld (brezs), hl
        
        ld a, (lobu + 16)
        ld (bfats), a
        ld hl, (lobu + 36)
        ld (bftsz), hl
        ld hl, (lobu + 36 + 2)
        ld (bftsz + 2), hl
        ld hl, (lobu + 44)
        ld (brootc), hl
        ld hl, (lobu + 44 + 2)
        ld (brootc + 2), hl
        ld hl, (bftsz)
        ld de, (bftsz + 2)
        ld bc, (bfats)
        ld b, 0
        call umn4b
        push hl
        push de
        ld hl, (brezs)
        ld (sfat), hl
        pop de
        pop bc
        call add4b
        ld (sdfat), hl
        ld (sdfat + 2), de
        ld hl, 0
        ld (cuhl), hl
        ld (cude), hl
        ld (lstcat), hl
        ld (lstcat + 2), hl
        xor a
        ret

nhdd    ld hl, (duhl)
        ld de, (dude)
        xor a
        ld (zes), a
        jp ldbpb
        
nhdd1   ld a, 1
        or a
        ret

;---------------------------------------
;arithmetics block
        
del128  ;i:        [de, hl]/128
;        o:        [de, hl]
;          bc - remainder
        ld a, l
        ex af, af'
        ld a, l
        ld l, h
        ld h, e
        ld e, d
        ld d, 0
        rla
        rl l
        rl h
        rl e
        rl d
        ex af, af'
        and 127
        ld b, 0
        ld c, a
        ret

;-------
umnox2  ;i:        [de, hl]*a
;                a - power of two
;        o:        [de, hl]
        cp 2
        ret c
        srl a
l33t1   sla l
        rl h
        rl e
        rl d
        srl a
        jr nc, l33t1
        ret

;-------
add4b   add hl, bc
        ret nc
        inc de
        ret

add4bf  ;i:        [de, hl] + [clde, clhl]
;        o:        [de, hl]
        ex de, hl
        ld bc, (clde)
        add hl, bc
        ex de, hl
        ld bc, (clhl)
        add hl, bc
        jr nc, knh
        inc de
knh     ld (clhl), hl
        ld (clde), de
        ret

umn4b   ;i:        [de, hl]*bc
        ; o:        [de, hl]
        ld a, b
        ld b, c
        ld c, a
        inc c
        or a
        jr nz, tekno
        dec b
        jr z, umn1
        inc b
tekno   xor a
        cp b
        jr nz, tys
        dec c
tys     dec b
        push hl
        push bc
        ld h, d
        ld l, e
        cp b
        jr z, negry
efro    add hl, de
        djnz efro
        
        ld b, a
negry   dec c
        jr nz, efro
        ld (rezde), hl
        pop bc
        pop hl
        ld d, h
        ld e, l
        cp b
        jr z, negra
ofer    add hl, de
        jr c, incde
enjo    djnz ofer
        ld b, a
negra   dec c
        jr nz, ofer
        ld de, (rezde)
umn1    ret

incde   exx
        ld hl, (rezde)
        inc hl
        ld (rezde), hl
        exx
        jr enjo
        
;---------------------------------------
tos     xor a
        ld (nsdc), a
        ld (eoc), a
        ret

        
;---------------------------------------
;   SD DRIVER
;---------------------------------------
;инициализация sd карты

ide_ini ;cp h'f1
        ; ret z
        call sd__off
        ret

;=======================================
xpozi   ld (lthl), hl
        ld (ltde), de
        
proz    ld (blknum), hl
        ld (blknum+2), de
        ret

;hl, in da kudy a, secs
rddse   ld de, (blknum)
        ld bc, (blknum+2)
        ex af, af'
        ld a, cmd_18
        call secm200
        ex af, af'
rd1     ex af, af'
        call in_out
        cp h'fe
        jr nz, $-5
        call reads
        ex af, af'
        dec a
        jr nz, rd1
        ld a, cmd_12
        call out_com
        call in_out
        inc a
        jr nz, $-4
        jp cs_high
        
;---------------------------------------
reads   push bc
        push de
        ld bc, p_data
        inir
        inir
        in a, (c)
        in a, (c)
        pop de
        pop bc
        ret

;---------------------------------------
;определение наличия карты
        
sel_dev ;i:        a - n of dev
        or a
        ret nz
drdet   call sd_init
        ld de, 2
        or a
        ret nz
        ld de, 0
        ret

;---------------------------------------
sd_init call cs_high
        ld bc, p_data
        ld de, h'10ff
        out (c), e
        dec d
        jr nz, $-3
        xor a
        ex af, af'
zaw001  ld hl, cmd00
        call outcom
        call in_out
        ex af, af'
        dec a
        jr z, zaw003
        ex af, af'
        dec a
        jr nz, zaw001
        ld hl, cmd08
        call outcom
        call in_out
        rept 3
        in h, (c)
        nop
        endr
        in h, (c)
        ld hl, 0
        bit 2, a
        jr nz, zaw006
        
        ld h, h'40
zaw006  ld a, cmd_55
        call out_com
        call in_out
        ld a, acmd_41
        out (c), a
        nop
        out (c), h
        nop
        out (c), l
        nop
        out (c), l
        nop
        out (c), l
        ld a, h'ff
        out (c), a
        call in_out
        and a
        jr nz, zaw006
        
zaw004  ld a, cmd_59
        call out_com
        call in_out
        and a
        jr nz, zaw004
        
zaw005  ld hl, cmd16
        call outcom
        call in_out
        and a
        jr nz, zaw005
        
cs_high push de
        push bc
        ld e, 3
        ld bc, p_conf
        out (c), e
        ld e, 0
        ld c, p_data
        out (c), e
        pop bc
        pop de
        ret

zaw003  call sd__off
        ld a, 1
        ret

sd__off xor a
        out (p_conf), a
        out (p_data), a
        ret

cs__low push de
        push bc
        ld bc, p_conf
        ld e, 1
        out (c), e
        pop bc
        pop de
        ret

outcom  call cs__low
        push bc
        ld bc, p_data
        rept 6
        outi
        nop
        endr
        pop bc
        ret

out_com push bc
        call cs__low
        ld bc, p_data
        out (c), a
        xor a
        rept 3
        out (c), a
        nop
        endr
        out (c), a
        dec a
        out (c), a
        pop bc
        ret

secm200 push hl
        push de
        push bc
        push af
        push bc
        ld bc, p_data
        ld a, cmd_58
        call out_com
        call in_out
        in a, (c)
        nop
        in h, (c)
        nop
        in h, (c)
        nop
        in h, (c)
        bit 6, a
        pop hl
        jr nz, secn200
        ex de, hl
        add hl, hl
        ex de, hl
        adc hl, hl
        ld h, l
        ld l, d
        ld d, e
        ld e, 0
secn200 pop af
        ld bc, p_data
        out (c), a
        nop
        out (c), h
        nop
        out (c), l
        nop
        out (c), d
        nop
        out (c), e
        ld a, h'ff
        out (c), a
        pop bc
        pop de
        pop hl
        ret

in_out  push bc
        push de
        ld de, h'10ff
        ld bc, p_data
in_wait in a, (c)
        cp e
        jr nz, in_exit
in_next dec d
        jr nz, in_wait
in_exit pop de
        pop bc
        ret

        
;-------
file1   defb "boot    $c "

;---------------------------------------
cmd00   defb 0x40, 0, 0, 0, 0, 0x95
cmd08   defb 0x48, 0, 0, 1, 0xaa, 0x87
cmd16   defb 0x50, 0, 0, 2, 0, 0xff

