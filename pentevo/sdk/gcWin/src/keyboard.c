//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                      ::
//::                  by dr_max^gc (c)2018                  ::
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

char gcGetKey(void) __naked
{
  __asm
    call    inkey
    ld      a,(key)
    ld      l,a
    ret

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    KEY_TRU     .equ    0x04    ;(CS+3)
    KEY_INV     .equ    0x05    ;(CS+4)
    KEY_CAPS    .equ    0x06    ;(CS+2)
    KEY_EDIT    .equ    0x07    ;(CS+1)
    KEY_LEFT    .equ    0x08    ;(CS+5)
    KEY_RIGHT   .equ    0x09    ;(CS+8)
    KEY_DOWN    .equ    0x0A    ;(CS+6)
    KEY_UP      .equ    0x0B    ;(CS+7)
    KEY_DELET   .equ    0x0C    ;(CS+0)
    KEY_ENTER   .equ    0x0D
    KEY_EXT     .equ    0x0E    ;(SS+CS)
    KEY_GRAPH   .equ    0x0F    ;(CS+9)
    KEY_PGDN    .equ    0x10    ;(SS+A)
    KEY_SSENT   .equ    0x11    ;(SS+ENTER)
    KEY_SSSP    .equ    0x12    ;(SS+SPACE)
    KEY_DEL     .equ    0x13    ;(SS+W)
    KEY_INS     .equ    0x14    ;(SS+E)
    KEY_PGUP    .equ    0x15    ;(SS+Q)
    KEY_BREAK   .equ    0x16    ;(CS+SP)
    KEY_CSENT   .equ    0x17    ;(CS+ENTER)
    KEY_SPACE   .equ    0x20
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
inkey:
    call    keyin
    or      a
    jr      nz,inkey1
    ld      (key),a
    ld      a,(delay)
    ld      (cntr),a
    ret
inkey1:
    ld      c,a
    ld      a,(delay)
    ld      b,a
    ld      a,(cntr)
    cp      b
    jr      nz,inkey2
    ld      a,b
    dec     a
    ld      (cntr),a
    ld      a,c
    jr      inkey3
inkey2:
    dec     a
    ld      (cntr),a
    ld      a,#0
    jr      nz,inkey3
    ld      a,(adel)
    ld      (cntr),a
    ld      a,c
inkey3:
    ld      (key),a
    ld      a,(keymod)
    ld      c,a
    ld      a,(key)
    cp      #KEY_CAPS       ; caps lock
    jr      nz,inkey4
    bit     4,c             ; switch enable?
    jr      z,inkey4
    push    af
    ld      a,#8
    xor     c
    ld      c,a
    ld      (keymod),a
    pop     af
    ret

inkey4:
    cp      #KEY_SSSP       ; rus/lat
    ret     nz
    bit     1,c             ; switch enable?
    ret     z
    push    af
    ld      a,c
    xor     #1
    ld      (keymod),a
    pop     af
    ret

keyin:
    ld      hl,#keytab
    ld      bc,#0x7FFE
keyin_nextrow:
    ld      d,#0x10
keyin_nextbit:
    in      e,(c)
    ld      a,e
    and     d
    jr      z,setkey
keyin_nextkey1:
    inc     hl
    rrc     d
    jr      nc,keyin_nextbit
    rrc     b
    jr      c,keyin_nextrow

    ld bc,#0xFEFE
    in a,(c)
    rra
    ld a,#0
    ret c
    ld b,#0x7F
    in a,(c)
    cpl
    and #2
    ret z
    ld a,#KEY_EXT
    ret

setkey:
    ld      a,(hl)
    or      a
    jr      z,keyin_nextkey1
setkeylp2:
    rr      d
    jr      c,setkeylp1
    ld      a,e
    and     d
    jr      nz,setkeylp2
    ld      a,d
    cp      #2
    jr      nz,setkeynx2
    ld      a,b
    cp      #0x7F
    jr      z,setkeylp1
setkeynx2:
    xor     a
    dec     d
    ret     nz
    ld      a,b
    cp      #0xFE
    ld      a,#0
    ret     nz
setkeylp1:
    scf
    rr      b
    jr      nc,setkeynx1
    in      a,(c)
    cpl
    and     #0x1F
    jr      z,setkeylp1
    cp      #1
    ld      a,#0
    ret     nz
    ld      a,b
    cp      #0xFE
    ld      a,#0
    ret     nz

setkeynx1:
    ld      bc,#0xFEFE
    ld      de,#40
    in      a,(c)
    rra
    jr      c,setkeynx4     ; CS press?
    add     hl,de

    ld      de,#40*3
    ld      a,(keymod)
    rra
    jr      nc,.+2+1        ; RUS/LAT?
    add     hl,de
    and     #4
    ld      a,(hl)
    call    nz,lowercase
    ret

setkeynx4:
    ld      b,#0x7F
    in      a,(c)
    and     #2
    jr      nz,setkeynx3    ; SS press?
    add     hl,de
    add     hl,de

setkeynx3:
    ld      de,#40*3
    ld      a,(keymod)
    rra
    jr      nc,.+2+1        ; RUS/LAT?
    add     hl,de
    and     #4
    ld      a,(hl)
    call    nz,uppercase
    ret
;;
lowercase:
    cp      #0x41
    ret     c
    cp      #0xA0
    ret     nc
    cp      #0x5B
    jr      c,.+2+2+1+2+2+2
    cp      #0x80
    ret     c
    cp      #0x90
    jr      c,.+2+2
    add     a,#0x30
    add     a,#0x20
    ret
;;
uppercase:
    cp      #0x61
    ret     c
    cp      #0x7B
    jr      c,.+2+2+1+2+2+2+2+1
    cp      #0xA0
    ret     c
    cp      #0xB0
    jr      c,.+2+2+2+1
    sub     #0x30
    cp      #0xC0
    ret     nc
    sub     #0x20
    ret

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;;       KD4 KD3 KD2 KD1 KD0
;;KA8     V   C   X   Z   CS  65278   #FEFE
;;KA9     G   F   D   S   A   65022   #FDFE
;;KA10    T   R   E   W   Q   64510   #FBFE
;;KA11    5   4   3   2   1   63486   #F7FE
;;KA12    6   7   8   9   0   61438   #EFFE
;;KA13    Y   U   I   O   P   57342   #DFFE
;;KA14    H   J   K   L   EN  49150   #BFFE
;;KA15    B   N   M   SS  SP  32766   #7FFE

keytab:
    .ascii  "bnm"
    .db     0,KEY_SPACE
    .ascii  "hjkl"
    .db     KEY_ENTER
    .ascii  "yuiop67890"
    .ascii  "54321trewq"
    .ascii  "gfdsavcxz"
    .db     0
; with CS
    .ascii  "BNM"
    .db     0,KEY_BREAK
    .ascii  "HJKL"
    .db     KEY_CSENT
    .ascii  "YUIOP"
    .db     KEY_DOWN,KEY_UP,KEY_RIGHT,KEY_GRAPH,KEY_DEL
    .db     KEY_LEFT,KEY_INV,KEY_TRU,KEY_CAPS,KEY_EDIT
    .ascii  "TREWQGFDSAVCXZ"
    .db     0
; with SS
    .ascii   "*,."
    .db     0,KEY_SSSP
    .ascii  "^-+="
    .db     KEY_SSENT
    .ascii  "[]"
    .db     0x7F
    .ascii  ';"'
    .ascii  "&'()_%$#@!><"
    .db     0x03,0x02,0x01
    .ascii  "}{\|~/?`:"
    .db     0
;; cyrillic cp866 table
    .db     0xA8,0XE2,0xEC,0,KEY_SPACE              ; טעü
    .db     0xE0,0xAE,0xAB,0xA4,KEY_ENTER           ; נמכה
    .db     0xAD,0xA3,0xE8,0xE9,0xA7                ; םדרשח
    .ascii  "6789054321"
    .db     0xA5,0xAA,0xE3,0xE6,0xA9                ; וךףצי
    .db     0xAF,0xA0,0xA2,0xEB,0xE4                ; ןאגûפ
    .db     0xAC,0xE1,0xE7,0xEF,0x00                ; לסקÿ
    .db     0x88,0x92,0x9C,0,KEY_BREAK              ; ÈÒÜ
    .db     0x90,0x8E,0x8B,0x84,KEY_CSENT           ; ÐÎËÄ
    .db     0x8D,0x83,0x98,0x99,0x87                ; ÍÃØÙÇ
    .db     KEY_DOWN,KEY_UP,KEY_RIGHT,KEY_GRAPH,KEY_DEL
    .db     KEY_LEFT,KEY_INV,KEY_TRU,KEY_CAPS,KEY_EDIT
    .db     0x85,0x8A,0x93,0x96,0x89                ; ÅÊÓÖÉ
    .db     0x8F,0x80,0x82,0x9B,0x94                ; ÏÀÂÛÔ
    .db     0x8C,0x91,0x97,0x9F,0x00                ; ÌÑ×‗
    .db     0x2A,0xA1,0xEE,0x00,KEY_SSSP            ; (*,.)
    .ascii  "^-+="
    .db     KEY_SSENT
    .db     0xE5,0xEA                               ; []
    .db     0x7F
    .db     0xA6,0xED
    .ascii  "&'()_%$#@!><"
    .db     0x03,0x02,0x01
    .ascii  "}{\|~/?`"
    .db     0xA6
    .db     0

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
key:    .db 0           ; key code
adel:   .db 1           ; autorepeat delay
cntr:   .db 30          ; frames counter
delay:  .db 30          ; delay after first pressing
keymod: .db 0b00010010  ; keyboard mode
                        ; bit 0 - LAT/RUS
                        ; bit 1 - ENABLE LAT/RUS SWITCH
                        ; bit 2 -
                        ; bit 3 - CAPS LOCK
                        ; bit 4 - ENABLE CAPS LOCK SWITCH
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    __endasm;
}
