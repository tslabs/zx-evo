;GIF Viewer;(C) GREY 2013-2017, KOSHI/MGN 2019;-----------------------------------------------         SAVEBIN "gifview.wmf";--------         ORG #7E00
         INCLUDE "WCHEADER.ASM"
;-----------------------------------------------         ORG #8000;--------
codstart EXA         XOR A         LD (ESTA),A         LD (ESCENKEF),A         LD (wcABT),A         LD (wcENT),A         EXA         OR A:JP NZ,PLUGIN_EXIT ; вызывать плагин можно только по расширению         PUSH HL,DE
         LD BC,%10 ; 14MHz
         CALL TURBO         LD A,1; time refresh disable         CALL INT_PL         LD BC,CYC,A,CACHE:OUT A; Cache ON         POP DE,HL
        ld (ExSp),sp
        ld (LenL),hl
        ld (LenH),de  ; сохраняем длинну файла
        call CalkLoad ; расчитаем сколько буферов/страниц/секторов грузить
        ld a,#00
        exa
        ld a,65         
        call wcIN     ; включаем - видеобуфер0 и узнаем - какая это страница
        ld a,(wcPage3)
        ld (ScrPg),a
        add A,7:ld (EndScr4),a
        add A,8:ld (EndScr8),a
        ld a,#10
        exa
        ld a,65         
        call wcIN       ; включаем - видеобуфер1 и узнаем - какая это страница
        ld a,(wcPage3)  ; используется как файловый буфер
        ld (BufPg),a
        ld a,#20
        exa
        ld a,65         
        call wcIN       ; включаем - видеобуфер2 и узнаем - какая это страница
        ld a,(wcPage3)  ; используется как массив с таблицами
        ld (TabPg),a
        ld (ctfirst1),a
        ld (ctfirst2),a
        ld (ctfirst3),a
        inc a
        ld (ctlast1),a
        ld (ctlast2),a
        ld (ctlast3),a
        inc a
        ld (ctlink1),a
        ld (ctlink2),a
        ld (ctlink3),a
        ld (ctlink4),a
        ld (ctlink5),a
        inc a
        ld (palPage),a
        call load_file  ; грузим файл в буфер
       
        ld a,#FF
        ld (ClsScr),a   ; первый проход - очищаем экран
        ld hl,0         ; начальное смещение экрана
        xor a
        ld (zagl1),a    ;заглушка для центровки экрана (только первый раз центруем)
        ld (ScrX_Scrl),hl
        ld (ScrY_Scrl),hl        CALL USPO
l20_PL  ; цикл загрузки ролика - начинаем с начала файла. 
        ; (только если весь файл в памяти)
        XOR A        LD (wcABT),A        LD (wcENT),A        
        ld a,(ScrPg)    ; Куда и с какой старнцы распаковывать
        ld e,a
        ld a,(BufPg)    ; A - начиная с какой страницы находится Gif
        call UnpackGif  ; возвращает в A количестов цветов GIF-а
        xor a
        ld (ClsScr),a   ; второй проход - не очищаем экран
        ld hl,(Buf_N_Org)
        ld a,l          ; если фалй длинный, несколько буферов
        or h            ; то его нельзя сразу запускать заного
        jp nz,WaitBreak ; поэтому уходим на выход
        ld a,(wcPage0)
        ld bc,wcPage0
        out (c),a
        ld a,(wcPage3)
        ld bc,wcPage3
        out (c),a
                HALT
        call ESC:JR NZ,EX_ESC
        call ENKE:JR NZ,EX_ENT
        JP l20_PL
EX_ESC  ld hl,0
        xor a
        ld (zagl1),a    ;заглушка для центровки экрана (только первый раз центруем)
        ld (ScrX_Scrl),hl
        ld (ScrY_Scrl),hl
        call ScrlScr
        ld a,0
        exa
        ld a,64 ;│MNGV_PL
        call wcIN
        XOR A
        RET;-----------------------------PLUGIN_EXIT LD A,(ESTA)            RET
EX_ENT  
WEnter  ld hl,0
        xor a
        ld (zagl1),a    ;заглушка для центровки экрана (только первый раз центруем)
        ld (ScrX_Scrl),hl
        ld (ScrY_Scrl),hl
        LD A,2
        RET

EX_ERR  DI        ld sp,(ExSp)        EI
        ld hl,0
        xor a
        ld (zagl1),a    ;заглушка для центровки экрана (только первый раз центруем)
        ld (ScrX_Scrl),hl
        ld (ScrY_Scrl),hl
        ld a,0
        exa
        ld a,64 ;│MNGV_PL
        call wcIN
        LD A,1
        RET

exEnter DI:LD SP,(ExSp):EI        XOR A:LD (wcENT),A        LD A,(ESCENKEF):SET 1,A:LD (ESCENKEF),A
        ld a,(wcPage0)
        ld bc,wcPage0
        out (c),a
        ld a,(wcPage3)
        ld bc,wcPage3
        out (c),a
;HALT
        JP EX_ENT

WaitBreak
        DI        ld sp,0
ExSp    equ $-2        EI
        ld a,(wcPage0)
        ld bc,wcPage0
        out (c),a
        ld a,(wcPage3)
        ld bc,wcPage3
        out (c),a        LD A,(ESCENKEF)        BIT 0,A:JP NZ,EX_ESC        BIT 1,A:JP NZ,EX_ENT
WBloop  HALT
        call ESC:JP NZ,EX_ESC
        call ENKE:JP NZ,EX_ENT
        JR WBloop

LoadNext;Загрузка следующего буфера
        ld a,(Buf_N)
        ld l,a
        ld a,(Pag_N)
        ld h,a
        ld a,(Sec_N)
        or h
        or l
        jp z,errGif

        push de
        exa
        exx
        push AF
        push HL
        push BC
        push DE

        ld a,(wcPage0)
        ld bc,Page0
        out (c),a
        ld a,(wcPage3)
        ld bc,Page3
        out (c),a
        call load_file
        ld bc,Page3
        ld a,(BufPg)
        out (c),a

        pop de
        pop bc
        pop hl
        pop AF
        exa
        exx
        pop de

        pop AF
        pop bc
        ld hl,#C000
        ret

load_file 
        ld a,(Buf_N)
        or a
        jp z,LastBuf ; это последний буфер
        dec a        ; иначе - грузим целый буфер
        ld (Buf_N),a
        ld a,(BufPg)
        add A,15
        ld (Pg_Last),a ; последняя старница бцфера

        ld a,#10 ;первая виртуальная для WC страница буфера
        ld b,16  ;количество страниц в буфере
LoadPg  ; в ь - количество загружаемых целых  страниц
        ; в а - первая виртуальная для WC - куда грузить
        push AF
        push bc
        exa
        ld a,65  ;│MNGCVPL
        call wcIN
        ld a,48  ;│LOAD512
        ld hl,#C000
        ld b,32 ; целая страница 32 сектора по 512 байт
        call wcIN
        pop bc
        pop AF
        inc a
        djnz LoadPg
        ret
LastBuf
        ld a,(Pag_N)
        or a
        ld a,#10 ;первая виртуальная для WC страница буфера
        jp z,LastPage
        ld b,a
        call LoadPg
        xor a
        ld (Pag_N),a
LastPage         ;это последняя страница в буфере
        exa
        ld a,65  ;│MNGCVPL       
        call wcIN        
        ld a,(Sec_N)
        or a
        ret z
        ld b,a   ; количество секторов по 512ь
        ld a,48  ;│LOAD512
        ld hl,#C000
        call wcIN  
        xor a
        ld (Sec_N),a
        ret     

Buf_N     db 0,0  ;количестов буферов по 256к под файл
Buf_N_Org db 0,0  ;первоначальное количестов буферов
Pag_N     db 0    ;количестов страниц по 16к в последенем буфере 
Sec_N     db 0    ;количестов секторов по 512б на последней странице
CalkLoad
        push de
        srl d
        rr e
        srl d
        rr e   ; в де - количесвто целых буферов по 256 к
        ld (Buf_N),de
        ld (Buf_N_Org),de
        pop de
        push hl
        ld a,e
        sla h   ; двигаем влево 
        rla     ; 2 раза, 
        sla h   ; двигаем влево 
        rla     ; 2 раза, 
        and 63  ; в A - количество/остаток целых страниц в послденем буфере
        ld (Pag_N),a
        pop hl
        ld a,h
        and 1
        or l
        ld a,h
        jr z,nl ; количесвто байт кратно сектору 512
        srl a
        inc a   ; в следующем секторе есть остаток 
        ld (Sec_N),a
        ret
nl      srl a ; количесвто байт кратно сектору 512
        ld (Sec_N),a
        ret

ShowGif cp 17             ; проверяеm - в каком режиме 4 бит ил 8 бит показыватьж
        jp c,ShowGif16

        ld a,(wcPage0)
        ld bc,Page0
        out (c),a
        ld bc,Page3
        in a,(c)
        ld (sacep3),a
        ld a,(wcPage3)
        out (c),a

        LD  A,%11000010
        exa
        ld a,66 ;│GVmod
        call wcIN
        ld a,1
        exa
        ld a,64 ;│MNGV_PL
        call wcIN
;HALT
        ld bc,Page3
        ld a,(sacep3)
        out (c),a
        ret

sacep3  db 0
ShowGif16
        ld a,(wcPage0)
        ld bc,Page0
        out (c),a

        ld bc,Page3
        in a,(c)
        ld (sacep3),a
        ld a,(wcPage3)
        out (c),a
        LD  A,%11000001
        exa
        ld a,66 ;│GVmod
        call wcIN
        ld a,1
        exa
        ld a,64 ;│MNGV_PL
        call wcIN
;HALT
        ld bc,Page3
        ld a,(sacep3)
        out (c),a
        ret

ScrlScr    
zagl1   nop
        ld a,(KodZagl)
        ld (zagl1),a    
        push hl
        ld hl,(ScrX_Scrl)
        ld bc,GXOffsL
        out (c),l
        ld bc,GXOffsH
        out (c),h
        ld hl,(ScrY_Scrl)
        ld bc,GYOffsL
        out (c),l
        ld bc,GYOffsH
        out (c),h
        
        ld a,(wcPage0)
        ld bc,Page0
        out (c),a
        ld bc,Page3
        in a,(c)
        ld (sacep3),a
        ld a,(wcPage3)
        out (c),a        
        ld a,67
        ld hl,(ScrY_Scrl)
        call wcIN
        ld a,68
        ld hl,(ScrX_Scrl)
        call wcIN

        ld bc,Page3
        ld a,(sacep3)
        out (c),a

        pop hl
KodZagl ret

;/********************************/
;/* библиотека распаковки GIF а */
;/********************************/

palPage   db 0 ;equ #07;сюда просто мусорим палитру - можно переделать                ;через любой буфер в 512 байт

SysConfig equ   #20AF
DMACtrl   equ   #27AF
DMAStatus equ   #27AF
DMALen    equ   #26AF
DMANum    equ   #28AF
DMASAddrL equ   #1AAF
DMASAddrH equ   #1BAF
DMASAddrX equ   #1CAF
DMADAddrL equ   #1DAF
DMADAddrH equ   #1EAF
DMADAddrX equ   #1FAF
GXOffsL   equ   #02AF
GXOffsH   equ   #03AF
GYOffsL   equ   #04AF
GYOffsH   equ   #05AF

VConfig   equ   #00AF
VPage     equ   #01AF   
MemConfig equ   #21AF   
MemRet    equ   %00000001   
Page0     equ   #10AF        
Page2     equ   #12AF
Page3     equ   #13AF
trdos     equ   #3D13
FMAddr    EQU   #15AF
PalSel    equ   #07AF

clearScr ; очищаем весь экран 512*512
        ex AF,AF'
        ld bc,Page3
        in a,(c)
        ld (pad3st),a
        ex AF,AF'
        out (c),a
        ex AF,AF'
        ld a,0
back_p  equ $-1

        ld (#C000),a
        ld (#C001),a

        ld bc,DMALen
        ld a,255
        out (c),a
        ld bc,DMANum
        out (c),a
        xor a
        ld bc,DMASAddrL
        out (c),a
        ld bc,DMASAddrH
        out (c),a
        ex AF,AF'
        ld bc,DMASAddrX
        out (c),a
        ex AF,AF'
        ld bc,DMADAddrL
        inc a
        inc a
        out (c),a
        ld bc,DMADAddrH
        dec a
        dec a
        out (c),a
        ex AF,AF'
        ld bc,DMADAddrX
        out (c),a
        ex AF,AF'
        ld a,#1
        ld bc,DMACtrl
        out (c),a

        ld bc,DMACtrl
wtdma   in a,(c)
        bit 7,a
        jp nz,wtdma

        ld bc,DMALen
        ld a,255
        out (c),a
        ld bc,DMANum
        ;ld a,255
        out (c),a
        xor a
        ld bc,DMASAddrL
        out (c),a
        ld bc,DMASAddrH
        out (c),a
        ex AF,AF'
        ld bc,DMASAddrX
        out (c),a
        ex AF,AF'
        ld bc,DMADAddrL
        out (c),a
        ld bc,DMADAddrH
        out (c),a
        ex AF,AF'
        add a,#08
        ld bc,DMADAddrX
        out (c),a
        ex AF,AF'
        ld a,#1
        ld bc,DMACtrl
        out (c),a

        ld bc,Page3
        ld a,0
pad3st  equ $-1
        out (c),a
        ret

UnpackGif
        ex AF,AF'
        ld a,e
        ld (pgloc),a
        ld (pgloc4),a        
        ld (pgScr),a
        ex AF,AF'

        ld (saveSP),SP
        ld bc,Page3
        out (c),a

; /* Очишаем jamp-ы на проверку прозрачного цвета*/
; /* по молчанию каждый новый кадр без прозрачного цвета. */
; /* включим, если будет бит в блоке информации*/
        ld de,notr
        ld (j_notr),de
        ld de,notr1
        ld (j_notr1),de
        ld de,notr2
        ld (j_notr2),de

        LD hl,#C000     ; откуда читаем
        LD de,#C000     ; куда распаковываем
        ld (scrofs),de
        ld (scrofs4),de
        ; проверяем заголовок, что это GIF
        ld a,(hl)
        cp "G"
        jp nz,EX_ERR
        inc hl
        ld a,(hl)
        cp "I"
        jp nz,EX_ERR
        inc hl
        ld a,(hl)
        cp "F"
        jp nz,EX_ERR
        inc hl 
        inc hl
        inc hl
        inc hl
        ; читаем разрешение по X и Y
        ld c,(hl)
        inc hl
        ld b,(hl)
        ld (s_x),bc
        push hl
        ld l,c
        ld h,b
        ld bc,360      ; скролируем экран, что бы изображение отцентровалось
        OR A:SBC hl,bc ; это нужно только для просмотрщика, для распаковки                       ; в память - убрать
        sra h
        rr l
        ld (ScrX_Scrl),hl
        pop hl
        inc hl
        ld c,(hl)
        inc hl
        ld b,(hl)
        ld (s_y),bc
        push hl
        ld l,c
        ld h,b
        ld bc,288      ; скролируем экран, что бы изображение отцентровалось
        OR A:SBC hl,bc ; это нужно только для просмотрщика, для распаковки                       ; в память - убрать
        sra h
        rr l
        ld (ScrY_Scrl),hl
        pop hl
        inc hl
        ; читаем байт флагов
        ld a,(hl)
        ld (gl_pal),a
        and 7
        inc a
        ld (bitpix),a
        inc hl
        cp 5
        ld a,(hl)
        jr nc,A8btBck
        rlca
        rla
        rla
        rla        
        or (hl)
        ; цвет фона
A8btBck
        ld (back_p),a
        ld a,0
ClsScr  equ $-1
        or a        
        jp z,nocls
        ld a,(pgScr) 
        call clearScr
nocls   inc hl
        inc hl
        ld a,(bitpix)
        ld de,pixcol
        add a,e ; из таблицы в DE узнаем солько цветов в битах изображения (2^A)
        ld e,a  ; граница параграфа - пока что приходистя прибавлять A к DE
        ld a,0
        adc a,d
        ld d,a
        ld a,(de)
        ld (colpix),a
        push hl
        push AF
        call ShowGif
        pop AF
        pop hl
        cp 17
        jp nc,A4bit
        ld de,DoPixel16
        jp A8bit
A4bit   ld de,DoPixel
A8bit   ld (DoPixelHL),de
        ld a,(gl_pal)
        bit 7,a
        call nz,LoadPal ; если есть - загрузим палитру
l15     ld a,(hl)
        cp "!"     ; блок вспомогательных данных - обарабатываем управляющую                   ; информацию об изображении
        jp z,blok1
        cp ","
        jp nz,neimg
        call Image ; Блок изображения

        ld a,(colpix)
        push hl
        call ShowGif
        pop hl
        ld a,(pgScr)
        ld (pgloc),a
        ld (pgloc4),a
        LD de,#C000     ; куда распаковываем
        ld (scrofs),de
        ld (scrofs4),de    
; /* Очишаем jamp-ы на проверку прозрачного цвета*/
; /* по молчанию каждый новый кадр без прозрачного увета. */
; /* включим, если будет бит в блоке информации*/
        ld de,notr
        ld (j_notr),de
        ld de,notr1
        ld (j_notr1),de
        ld de,notr2
        ld (j_notr2),de
        jp l15

neimg   ld a,(hl)
        cp ";"          ; код коца данных (если не внутри блока)
        jp z,endGif
        inc l
        call z,NxPgGif
        jp l15

errGif  DI        ld SP,0
saveSP  equ $-2        EI
        ld bc,Page0
        ld a,(wcPage0)
        out (c),a        
        ld a,(colpix)
        scf
        ret

endGif  ; восстановим что нибудь в Паге0 - пока получился Басик 48
        ld bc,Page0
        ld a,(wcPage0)
        out (c),a
        ld a,(colpix)
        or a
        ret ;в A - количество фветов gif  С=0 нет ошибок

NxPgGif inc h
        ret nz  ; проверка на конец страницы
        ld h,#C0
        push bc
        push AF        
        ld bc,Page3
        in a,(c)        ; как хорошо, что из Паге3 можно читать
        cp 0
Pg_Last equ $-1
        jp z,LoadNext
        inc a
        out (c),a
        pop AF
        pop bc
        ret

blok1   inc l           ; код блока пока расшифровывать не умеем
        call z,NxPgGif
        ld a,(hl)
        cp #F9
        jp z,gif98a ; Но кое что умее - дополнительную информацию об изображении
        inc l
        call z,NxPgGif
        ld b,(hl)       ; блинна блока
l16     inc l
        call z,NxPgGif
        djnz l16
        inc l
        call z,NxPgGif
        ld b,(hl)
        xor a
        or b
        jp nz,l16  ;errGif  0 - конец либо длинна следующего блока
        inc l
        call z,NxPgGif
        jp l15

oldTr   db 0
gif98a 
        inc l
        call z,NxPgGif
        ld b,(hl)       ; блинна блока
        inc l
        call z,NxPgGif
        ld a,(oldTr)
        and %11100
        cp %01000
        ld a,(hl)
        ld (oldTr),a
        jp z,Notransp
        and 1
        jp z,Notransp
; /* Вставляем jamp-ы на проверку прозрачного цвета*/
        ld de,j_notr+2
        ld (j_notr),de
        ld de,j_notr1+2
        ld (j_notr1),de
        ld de,j_notr2+2
        ld (j_notr2),de
Notransp
        inc l
        call z,NxPgGif
        inc l
        call z,NxPgGif
        inc l
        call z,NxPgGif
        ld a,(hl)       ; и запоминаем цпрозрачный цвет
        ld (trcol),a
        ld (trcol1),a
        ld (trcol2),a        
        inc l
        call z,NxPgGif
        jp l15

LoadPal
        ld bc,Page0
        ld a,(palPage)
        out (c),a
        ld a,(colpix)
        inc a
        ld b,a 
        ld de,#0000+1
lp14
        ld a,(hl)
        inc l
        call z,NxPgGif
        rrca
        and %01111100
        ld c,a
        ld a,(hl)
        rlca
        rlca
        and %00000011
        or c
        ld (de),a
        dec de
        ld a,(hl)
        inc l
        call z,NxPgGif
        rlca
        rlca
        and %11100000
        ld c,a
        ld a,(hl)
        inc l
        call z,NxPgGif
        rrca
        rrca 
        rrca 
        and %00011111
        or c
        ld (de),a
        inc de
        inc de 
        inc de
        djnz lp14

        push hl
        LD  A,%00000000  ; T1PAL[7:6] T0PAL[7:6] GPAL[7:4] Palette selection for 4 bit modes
        LD  BC,PalSel    ; GPAL 0-15 colors
        OUT (C),A
        LD  A,#10;%00010000   ; Включаем маппинг по адресу #0000
        LD  BC,FMAddr
        OUT (C),A

        ld hl,(colpix)
        inc hl
        add hl,hl
        ld c,l
        ld b,h
        XOR  A
        ld h,a,l,a
        ld d,a,e,a
        ldir

        XOR  A            ; Отключаем маппинг
        LD  BC,FMAddr
        OUT (C),A        
        pop hl
        ret

Image   ; Блок изображения. Дискириптор, локальная палитра, и растровые данные
        ld a,(bitpix)   ;  записываем размер пикселя из глобальных параметров
        ld (pixelsize),a
        xor a
        ld (interlaced),a ; обнуляем режим чередования строк

        inc l
        call z,NxPgGif
        ld c,(hl)
        inc l
        call z,NxPgGif
        ld b,(hl)
        ld (imagex),bc   ; читаем расположение Х картинки на витруальном экране
        ld (xloc),bc
        ld (xloc4),bc
        inc l 
        call z,NxPgGif
        ld c,(hl)
        inc l
        call z,NxPgGif
        ld b,(hl)
        ld (yloc),bc  ; читаем расположение Y картинки на витруальном экране
        ld a,(colpix)
        cp 17
        jp c,I4bit
        ld a,c          ; переводим Y в номер страницы и смещение старшего байта
        sla c           ; для 8 бит режима
        rl b
        sla c
        rl b
        sla c 
        rl b   
        and 31
        add A,A
        add A,#C0
C0_1    equ $-1             
        ld (scrofs+1),a
        ld a,(pgScr)
        add a,b
        ld (pgloc),a
        jp I8bit

I4bit   ld a,c          ; переводжм У в номер страницы и смещение старшего байта
        sla c           ; для 4 бит режима
        rl b
        sla c 
        rl b           
        and 63
        add A,#C0
C0_2    equ $-1     
        ld (scrofs4+1),a   
        ld a,(pgScr)             
        add a,b
        ld (pgloc4),a

I8bit   call ScrlScr ; wtynhetv центруем экрна согласно координат
        inc l
        call z,NxPgGif
        ld c,(hl)
        inc l
        call z,NxPgGif
        ld b,(hl)       ; читаем размер картинки
        ld (imagewide),bc    ; ведь каждый кадр может быть разного размера  
        inc l
        call z,NxPgGif
        ld c,(hl)
        inc l
        call z,NxPgGif
        ld b,(hl)
        ld (imagedeep),bc
        inc l
        call z,NxPgGif
        ld a,(hl)
        bit 7,a
        jp z,locpal
        push AF
        and 7
        inc a
        ld (pixelsize),a
        call local_pal  ; Эх - все же есть локальная плитра и её нужно загрузить
        pop AF
locpal
        bit 6,a
        jp z,interliv
        ld (interlaced),a
interliv; здесь нужно обработать вариант, когда изображение черезстрочное ( режим переплитение) на будущее
        inc l
        call z,NxPgGif
        ld b,1  ; bufct
        ld a,(hl)
        inc l
        call z,NxPgGif
        ld (codestart),a
        call UnpackImage ; вот добрадись до самих кодов растровых данных
        ret

local_pal
        inc l
        call z,NxPgGif
        ld de,pixcol
        add a,e ; из таблицы в DE узнаем солько цветов в битах изображения (2^A)
        ld e,a  ; так как таблица не выравнена по границек паррафа - пока что приходистя прибадять A к DE
        ld a,0
        adc a,d
        ld d,a
        ld a,(de)
        ld (colpix),a
        call LoadPal
        ret

LenL    dw 0
LenH    dw 0
ScrPg   db 0    ;первая страница видео буфера0 - для экрана.
BufPg   db 0    ;первая страница видео буфера1 - используется как файловый буфер.
TabPg   db 0    ;первая страница видео буфера2 - используются как таблицы с массивами.


ScrX_Scrl dw 0
ScrY_Scrl dw 0

imagex  dw 0
;bufct   dw 0    ; будем хранить в b'
pixelsize  db 0
interlaced db 0
imagewide  dw 0
imagedeep  dw 0
;clearcode  dw 0        ; перенс хранится внутри кода
codestart  dw 0        
;eoi        dw 0        ; храним в коде
;reqct      dw 0
oldcode    dw 0
;pass       dw 0
rowcnt     dw 0
;xloc       dw 0
yloc       dw 0
yloc4       dw 0

;pgloc      dw 0
pgScr      db 0
;code       dw 0        ; храним в коде
nextcode   dw 0
;nextlim    dw 0
;remct      dw 0
rem        dw 0
;scrofs     dw 0

;/******************************/
;        MACRO callGetCode
;
;        cp 9 ;sub hl,de       ; { If reqct<=8 }
;        jp nc,._g01
;        call GetBCode   ;{   then return GetBCode(reqct) }
;        jp ._gex
;._g01   push AF ;push de         
;        ld a,8 ;ld de,8
;        call GetBCode
;        ld (._pushde),de ;{ temp:=GetBCode(8) }
;        pop AF ;pop hl
;        sub 8  ;ld de,8
;        call GetBCode
;        ex de,hl
;        ld h,l
;        ld l,0
;        ld de,0
;._pushde equ $-2
;        ld a,l  
;        or e
;        ld e,a
;        ld a,d
;        or h
;        ld d,a        
;._gex    
;        ENDM
;/******************************/

UnpackImage
        cp 13       ; не бывает кодов более 12 бит 
        jp nc,errGif; что то не то нам подсовывают - Валим от сюда На фиг
        exx
        ld b,a      ; размер элемента кода (типа бит на пиксель только для архиватора от 2 до  макс 12 
        ld de,1
l17     sla e
        rl d 
        djnz l17    ; Код очистки  clearcode:= 2 в степени  codestart (для 4 - 10000)
        ld (clearcode),de
        inc de
        ld (eoi),de ;  Код конца потока  eoi:=clearcode+1 
        inc a
        ld (reqct),a; Счеитчик побитоногоо сдвига  reqct:=codestart+1 }
        dec de      ; в de - clearcode
        call InitCodeTable
        ld de,#FFFF
        ld (oldcode),de
        inc de ; de=0
       ;ld (pass),de
       ;ld (xloc),de    ; Обнуляем   screen position }
        ld (yloc),de
       ;ld (xloc4),de   ; Обнуляем   screen position }
        ld (yloc4),de
        xor a
;ld (done),a
        ld (remct),a  
        ld de,(imagewide)
        ld (rowcnt),de;-------
ui01    ld a,0 ;(reqct)    
reqct   equ $-1
;-------callGetCode:        cp 9 ;sub hl,de       ; { If reqct<=8 }
        jp nc,Ag01
        call GetBCode   ;{   then return GetBCode(reqct) }
        jp Agex
Ag01    push AF ;push de         
        ld a,8 ;ld de,8
        call GetBCode
        ld (Apushde),de ;{ temp:=GetBCode(8) }
        pop AF ;pop hl
        sub 8  ;ld de,8
        call GetBCode
        ex de,hl
        ld h,l
        ld l,0
        ld de,0
Apushde equ $-2
        ld a,l  
        or e
        ld e,a
        ld a,d
        or h
        ld d,a        
Agex    ;-------macroend
          ld (code),de  
          ld hl,00        ;(clearcode)
clearcode equ $-2        
          OR A:SBC hl,de:jp nz,ui02
        ld de,(clearcode)
        call InitCodeTable
        ld a,(codestart)
        inc a
        ld (reqct),a ;  reqct:=codestart+1 
        ld de,#FFFF
        ld (oldcode),de
;jp ui07        LD A,(wcABT):OR A:JP NZ,exgif        LD A,(wcENT):OR A:JP NZ,exEnter        JP ui01 
ui02    ld hl,0;(eoi)
eoi     equ $-2        
        OR A:SBC hl,de
        jp nz,ui03
        call FlushIn    ; Закончилось все
;ld a,#FF ; фдаг для выхода
;ld (done),a
;jp ui07        ;LD A,(wcABT):OR A:JP NZ,exgif        ;LD A,(wcENT):OR A:JP NZ,exEnter        EXX        RET
ui03    ld hl,(code)
        add hl,hl
        ld a,0;(ctlink)
ctlink1 equ $-1        
        ld bc,Page0
        out (c),a
        ld e,(hl)
        inc hl
        ld d,(hl)
        ld h,d          ;ld hl,#FEFF;#FFFE ; (-2)
        inc h           ;sub hl,de
        inc h
        jp z,ui05
        ; тут оптимизировла проверек - было     ;ld hl,#FFFF
        ld de,(oldcode)                         ;ld de,(oldcode)
        inc d                                   ;sub hl,de
        jp z,ui06
        ld bc,(code)
        ld de,(oldcode)
        call InsertCode
ui04    jp ui06
ui05    ld bc,(oldcode)
        ld d,b ;ld de,(oldcode)
        ld e,c
        call InsertCode
ui06    ld hl,0 ;(code)
code    equ $-2
        ld (oldcode),hl
;/**********************************/
PutX    ld bc,Page3    ; пишем коды в Стэк пикселей
        in a,(c)
        ld (PgGif),a
        ld d,endcode[  ; ld de,0
        ld e,#FF
        ld b,Page0[
px01   
        ld a,0;(ctlast)
ctlast2 equ $-1        
        out (c),a
        ld a,(hl)
        ld (de),a
        inc de
        add hl,hl
        ld a,0;(ctlink)
ctlink3 equ $-1        
        out (c),a
       
        ld a,(hl)
        inc hl
        ld h,(hl)
        ld l,a         ; В общем тут оптимизировал проверку
        ld a,h
        inc a
        jp nz,px01

loop    dec de
        ld a,(de)
        push de
        jp DoPixel ; Выводим пиксель
DoPixelHL equ $-2  ; в режимах 4/8 бит - подставляется разный адрес процедуры
ExDoPix pop de
        ld a,d
        cp endcode[
        jp nz,loop
    ;ld bc,Page3   ; Page3 должно остаться из DoPixel
        ld a,0
PgGif   equ $-1
        out (c),a        LD A,(wcABT):OR A:JP NZ,exgif        LD A,(wcENT):OR A:JP NZ,exEnter        JP ui01
;/********************************/

;ui07    LD A,(wcABT):OR A:JP NZ,exgif;        LD A,(wcENT):OR A:JP NZ,exEnter
;ld a,0
;done    equ $-1
;or a:jp z,ui01;exx
;ret

exgif   XOR A:LD (wcABT),A        LD A,(ESCENKEF):SET 0,A:LD (ESCENKEF),A        exx
        pop hl
        jp WaitBreak

FlushIn exx      ; в b - bufct  блинна блока +1
        dec b
        jp z,fi02 
fi01    inc l 
        call z,NxPgGif
        dec b
        jp nz,fi01
fi02    ld b,(hl) ;  bufct
        inc b
        inc l 
        call z,NxPgGif
        dec b
        jp nz,fi01       
        exx
        ret

InsertCode      ; Вставляем в таблица новый код
        push bc ;        ld de,(oldcode)
        ld hl,(nextcode)
        add hl,hl
     
        ld bc,Page0
        ld a,0;(ctlink)
ctlink2 equ $-1
        out (c),a
        ld (hl),e ; массив сслок  ctlink[nextcode]:=oldcode
        inc hl
        ld (hl),d

        ld a,0;(ctfirst)
ctfirst1 equ $-1
        out (c),a
        pop hl
        ld a,(hl)       ; A = ctfirst[code]
        ld hl,(nextcode)

        ex AF,AF'
        ld a,0;(ctlast)
ctlast1 equ $-1
        out (c),a
        ex AF,AF'
        ld (hl),a       ; ctlast[nextcode]:=A

        ld a,0          ;(ctfirst)
ctfirst2 equ $-1
        out (c),a
        ld a,(de)       ;ld a,(hl)

        ld (hl),a       ; вот и сдвинули старык код в следующий код
        inc hl          ; ctfirst[nextcode]:=ctfirst[oldcode]
        ld de,0         ;(nextlim)
nextlim equ $-2
        ex de,hl        ; если достигли максимального кода то нужно увеличивать бит на код
        OR A:SBC hl,de
        jp nz,icEx
        ld a,(reqct) ;ld hl,(reqct)
        cp 12  ;ld bc,12        ; но помним что код не может првыщуть 12 бит 
        jp nc,icEx
        ld a,(reqct)
        inc a
        ld (reqct),a
        ld hl,(nextlim)
        add hl,hl
        ld (nextlim),hl
icEx    ld (nextcode),de
        ret

DoPixel16       ; 4 бита на пиксель
        ld b,Page3[
        ld e,0  ;(pgloc)
pgloc4  equ $-1
        out (c),e
        ld hl,0 ;(xloc)
xloc4   equ $-2
        ld de,0;(scrofs)
scrofs4 equ $-2
        srl h
        rr l
        jp c,lbt4
        add hl,de
        jp c,Xovr16
        jp notr1      ; перепрыгиваем через проверку прозрачного цвета
j_notr1 equ $-2       ; если не нужно проверять
        cp 0          ; проверям на прозрачный цвето
trcol1  equ $-1       ; код цвета и адрес перепрыгивания подстаивляются заранее
        jp z,Xovr16
notr1   rlca
        rla
        rla
        rla
        ld e,a
        ld a,#0F
        and (hl)
        or e
        ld (hl),a
        jp hbt4

lbt4    add hl,de
        jp c,Xovr16
        jp notr2       ; перепрыгиваем через проверку прозрачного цвета
j_notr2 equ $-2       ; все тоже самое для нечетного полубайта точки
        cp 0
trcol2  equ $-1
        jp z,Xovr16
notr2  
        ld e,a
        ld a,#F0
        and (hl)
        or e
        ld (hl),a
hbt4
Xovr16
        ld hl,(xloc4)
        inc hl
        ld (xloc4),hl

        ld hl,(rowcnt)
        dec hl
        ld (rowcnt),hl
        ld a,l
        or h
        jp nz,ExDoPix ;ret nz ;jr nz,_dpex16
        ld hl,(imagex)
        ld (xloc4),hl
        ld hl,(imagewide)
        ld (rowcnt),hl

dp0216  ld hl,(yloc)
        inc hl
        ld (yloc),hl
        ;для Y
        ld a,(scrofs4+1)
        inc a
        jp nz,dpY16
        ld a,(pgloc4)
        inc a
        cp a,0 ;#1f
EndScr4 equ $-1
        jp nc,ovpg16
        ld (pgloc4),a
ovpg16  ld a,#C0
dpY16   ld (scrofs4+1),a
        jp ExDoPix ;ret

DoPixel          ; 8 бит на пиксель  - хотя 5,6,7 бит тоже бывает 
        ld b,Page3[
        ld e,0  ;(pgloc)
pgloc   equ $-1
        out (c),e
        ld de,0 ;(xloc)
xloc    equ $-2
        ld hl,0 ;(scrofs)
scrofs  equ $-2
        add hl,de
        jp c,Xovr
        jp notr        ; перепрыгиваем через проверку прозрачного цвета
j_notr  equ $-2
        cp 0
trcol   equ $-1
        jp z,Xovr
notr    ld (hl),a
Xovr    inc de
        ld (xloc),de
        ld hl,(rowcnt)
        dec hl
        ld (rowcnt),hl
        ld a,l
        or h
        jp nz,ExDoPix ;ret nz ;jr nz,_dpex минимальный цикл вывода точки
        ld hl,(imagex);          и так 0
        ld (xloc),hl
        ld hl,(imagewide)
        ld (rowcnt),hl

dp02    ld hl,(yloc)
        inc hl
        ld (yloc),hl
        ;для Y
        ld a,(scrofs+1)
        inc a
        jp z,NdpY
        inc a   ; для 256 цветов
        jp nz,dpY
NdpY    ld a,(pgloc)
        inc a
        cp a,0 ;#1f
EndScr8 equ $-1
        jp nc,overpg
        ld (pgloc),a
overpg  ld a,#C0
dpY     ld (scrofs+1),a
        jp ExDoPix ;ret
;/*
;CleraLine8      ; отчистка строки, как по стандарту - но с ней не красиво мерцает
;        ld bc,DMALen
;        ld hl,(imagewide)
;        srl h
;        rr l
;        dec l
;        ret z
;        out (c),l
;        ld bc,DMANum
;        ld a,1
;        out (c),a
;        ld hl,(scrofs)
;        ld bc,DMASAddrL
;        out (c),l
;        ld bc,DMADAddrL
;        inc l
;        out (c),l
;        ld bc,DMASAddrH
;        out (c),h
;        ld bc,DMADAddrH
;        out (c),h
;        ld bc,DMASAddrX
;        ld a,(pgloc)
;        out (c),a
;        ld bc,DMADAddrX
;        out (c),a
;        ld a,#3
;        ld bc,DMACtrl
;        out (c),a
;        ret
;*/
eof     equ #FFFF       ; заполняем таблицу кодов
InitCodeTable
        ld hl,(clearcode)
        inc hl          ;ld bc,2
        inc hl          ;add hl,bc
        ld (nextcode),hl
        ld hl,(clearcode)
        add hl,hl
        ld (nextlim),hl
        ld hl,0         ; Начиаем с  code =0

A01     ld de,(clearcode) ;  крутимся пока  code < HL do ...
        push hl
        OR A:SBC hl,de
        pop hl
        jp nc,A02
        ld a,0;(ctfirst)
ctfirst3 equ $-1
        ld bc,Page0
        out (c),a
        ld (hl),l       ; ctfirst[code]:=code
        ld a,0;(ctlast)
ctlast3 equ $-1
        out (c),a
        ld (hl),l       ;  ctlast[code]:=code 
        add hl,hl
        ld a,0;(ctlink)
ctlink4 equ $-1
        out (c),a
        ld bc,0-1        ;  ctlink[code]:= -1 - таблица указателй на цепочки кодов
        ld (hl),c
        inc hl
        ld (hl),b
        srl h
        rr l
        inc hl
        jp A01
A02     ld de,4096      ; максиально возможный  размер таблицы при 12 битах
        ex de,hl
        OR A:SBC hl,de
        ret c
        ex de,hl
        add hl,hl
        ld bc,Page0
        ld a,0;(ctlink)
ctlink5 equ $-1
        out (c),a
        ld bc,#FEFF ;-2
        inc d           ;(+1 что бы цикл сделать по флагу Z )
RepStw  ld (hl),c
        inc hl
        ld (hl),b
        inc hl
        dec e 
        jp nz,RepStw
        dec d
        jp nz,RepStw

        ld bc,#FFFF ;-1
        ld (hl),c
        inc hl
        ld (hl),b
        inc hl        
        ret

;/********************************/
;        MACRO   CallGetGB
;        exx   ; Cp     bufct = 0
;        dec b
;        jp nz,._n02
;        xor a
;        ld b,(hl)       ; новый bufct
;        or b
;        jp z,._g02
;        inc l
;        call z,NxPgGif
;._n02   ld a,(hl)
;        inc l
;        call z,NxPgGif
;._g02   exx
;        ENDM
;/********************************/

GetBCode 
        ld e,a        ;a  = reqct ; de = reqct
        ld a,0 ;(remct)    ; счетчик сдвига
remct   equ $-1
        or a            ;  If remct=0 
        jp nz,gb01     ;  then rem:=GetGB;------- 
;CallGetGB:        exx   ; Cp bufct = 0
        dec b
        jp nz,A1n02
        xor a
        ld b,(hl)   ; новый bufct
        or b
        jp z,A1g02
        inc l
        call z,NxPgGif
A1n02   ld a,(hl)
        inc l
        call z,NxPgGif
A1g02   exx;-------
        ld (rem),a
        xor a
        ld (rem+1),a
        ld a,8          ;  remct:=8 }

gb01    cp e          ;  If remct<reqct }
        jp nc,gb02
        ex AF,AF';-------
;CallGetGB:        exx   ; Cp bufct = 0
        dec b:jp nz,A2n02
        xor a:ld b,(hl):or b:jp z,A2g02  ; новый bufct
        inc l:call z,NxPgGif
A2n02   ld a,(hl)
        inc l:call z,NxPgGif
A2g02   exx;-------
        ld l,a
        ld h,0
        exa

        ld b,a
        exa
        ld a,12
        sub b
        ld (djn),a
        jr $-1
djn     equ $-1 ; двигаем считаенный байт что бы достать биты
.12     add hl,hl

        ld a,(rem)      ; скалдываем с прошлым сдинтуым полубатом    rem,a
        or l
        ld (rem),a      ; и получаем 12 бит слово кода
        ld a,(rem+1)
        or h
        ld (rem+1),a
        ex AF,AF'
        add a,8

gb02    ex AF,AF'
        ld ix,cmask
        ld a,e
        ld (cm1),a
        ld l,(ix+1)
cm1     equ $-1 

        ld h,0
        ld a,(rem)
        and l
        ld l,a
        ld (ax1),hl
        ex AF,AF'
        sub e
        ld (remct),a

        ld a,12
        sub e
        rlca
        rlca
        ld hl,(rem)
        ld (djn1),a
        jr $-1
djn1    equ $-1 ; двигаем считаенный байт что бы достать биты
.12     srl h:rr l
        ld (rem),hl
        ld de,0
ax1     equ $-2
        ret;----------------------------------------------INT_PL  EXA:LD A,86:JP WLD;MNGC_PL EXA:LD A,0:JP WLD;MNG0_PL EXA:LD A,78:JP WLD;MNG8_PL EXA:LD A,79:JP WLD;PRWOW   LD A,1:JP WLD;RRESB   LD A,2:JP WLD;PRSRW   LD A,3:JP WLD;PRIAT   EXA:LD A,4:JP WLD;GADRW   LD A,5:JP WLD;CURSOR  LD A,6:JP WLD;CURSER  LD A,7:JP WLD;YN      EXA:LD A,8:JP WLD;ISTR    EXA:LD A,9:JP WLD;NORK    EXA:LD A,10:JP WLD;TXTPR   LD A,11:JP WLD;DMAPL   EXA:LD A,13:JP WLDTURBO   LD A,14:JP WLDGEDPL   LD A,15:JP WLD;-------;KBSCN   EXA:LD A,42:JP WLD;SPKE    LD A,16:JP WLD;UPPP    LD A,17:JP WLD;DWWW    LD A,18:JP WLD;LFFF    LD A,19:JP WLD;RGGG    LD A,20:JP WLD;TABK    LD A,21:JP WLDENKE    LD A,22:JP WLD;BSPC    LD A,24:JP WLD;PGUP    LD A,25:JP WLD;PGDN    LD A,26:JP WLD;DEL     LD A,43:JP WLDESC     LD A,23:JP WLD;HOME    LD A,27:JP WLD;END     LD A,28:JP WLD;F1      LD A,29:JP WLD;F2      LD A,30:JP WLD;F5      LD A,33:JP WLD;F10     LD A,38:JP WLD;ALT     LD A,39:JP WLD;SHIFT   LD A,40:JP WLD;CTRL    LD A,41:JP WLDUSPO    LD A,46:JP WLDNUSP    LD A,47:JP WLD;-------;LOAD512 LD A,48:JP WLD;LOAD256 LD A,60:JP WLD;LOADNON LD A,61:JP WLD;SAVE512 LD A,49:JP WLD;FENTRY  LD A,59:JP WLD;GFILE   LD A,62:JP WLD;MKFILE  LD A,72:JP WLD;DELFL   LD A,75:JP WLD;-------MNGV_PL EXA:LD A,64:JP WLDMNGCVPL EXA:LD A,65:JP WLDMNG0VPL EXA:LD A,80:JP WLDGVmod   EXA:LD A,66:JP WLDGYoff   LD A,67:JP WLDGXoff   LD A,68:JP WLD
;----------------------------------------------ESTA      NOP; Exit StateESCENKEF  NOP; ESC+ENTER Flag (bit 0 = ESC, bit 1 = ENTER)
;---------cmask     db 0,1,3,7,#F,#1F,#3F,#7F,#FF
inctable  db 8,8,4,2,0
startable db 0,4,2,1,0

pixcol    db 1,1,3,7,15,31,63,127,255  ; (0=256) количество цветов в зависимости от бит/пиксель
s_x       dw 0    ; sixe of x
s_y       dw 0    ; sixe of y
bitpix    db 0    ; bit на pyxel
colpix    db 0,0    ; цветов на пиксель - 1 
;back_p db 0    ; цветов фона
gl_pal    db 0    ; бит 7 - если есть глобальная таблица цветов
Pg_Sec    dw 0
;_SI dw 0
        nop
        nop
codendendcode;----------------------------------------------