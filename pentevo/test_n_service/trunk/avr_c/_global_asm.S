#include "_global.h"
; include "_t_beep.h"
;------------------------------------------------------------------------------
;uint8_t random8()
;LFSR; taps:31,28; 8 shift-cycles per call
.global random8
.func random8
random8:
        lds     r18,(_rnd)+2
        lds     r19,(_rnd)+3
        sts     (_rnd)+3,r18
        rol     r18
        rol     r19
        mov     r24,r19
        rol     r18
        rol     r19
        rol     r18
        rol     r19
        rol     r18
        rol     r19
        eor     r24,r19
        lds     r18,(_rnd)+1
        sts     (_rnd)+2,r18
        lds     r18,_rnd
        sts     (_rnd)+1,r18
        sts     _rnd,r24
        ret
.endfunc
;
;------------------------------------------------------------------------------
;DBMASK=0x07ff
#define DBMASK_HI 7
;------------------------------------------------------------------------------
;void load_fpga(uint_farptr_t confdata)
;1) запрет прерываний
;2) инициализация SPI
;3) распаковка данных и загрузка FPGA
.global load_fpga
.func load_fpga
load_fpga:
        cli
;инициализация SPI
        ldi     r18,(1<<SPI2X)
        out     _SFR_IO_ADDR(SPSR),r18
        ldi     r18,(1<<SPE)|(1<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)
        out     _SFR_IO_ADDR(SPCR),r18
;загрузка FPGA
        lds     r18,_SFR_MEM_ADDR(DDRF)
        sbr     r18,(1<<PF0)                    ;устанавливаем nCONFIG
        sts     _SFR_MEM_ADDR(DDRF),r18

        ldi     r19,184                         ;задержка 50 мкс
.L_1:   dec     r19     ;1
        brne    .L_1    ;2

        cbr     r18,(1<<PF0)                    ;сбрасываем nCONFIG
        sts     _SFR_MEM_ADDR(DDRF),r18

.L_2:   sbis    _SFR_IO_ADDR(PINF),PF1          ;ждём 1 на nSTATUS
        rjmp    .L_2

        mov     r30,r22                         ;значение входного параметра в RAMPZ,Z
        mov     r31,r23
        out     _SFR_IO_ADDR(RAMPZ),r24
        push    r28
        push    r29
        ldi     r28,lo8(megabuffer)             ;указатель на буфер в Y
        ldi     r29,hi8(megabuffer)
        ldi     r18,0x80
;
.L_ms:  elpm    r0,Z+
        st      Y+,r0
;
        out     _SFR_IO_ADDR(SPDR),r0
.L_3:   sbis    _SFR_IO_ADDR(SPSR),SPIF
        rjmp    .L_3
;
        subi    r28,lo8(megabuffer)             ;
        sbci    r29,hi8(megabuffer)             ;
        andi    r29,DBMASK_HI                   ;Y warp
        subi    r28,lo8(-(megabuffer))          ;
        sbci    r29,hi8(-(megabuffer))          ;
.L_m0:  ldi     r21,0x02
        ldi     r20,0xff
.L_m1:
.L_m1x: add     r18,r18
        brne    .L_m2
        elpm    r18,Z+
        rol     r18
.L_m2:  rol     r20
        brcc    .L_m1x
        dec     r21
        brne    .L_x2
        ldi     r19,2
        asr     r20
        brcs    .L_n1
        inc     r19
        inc     r20
        breq    .L_n2
        ldi     r21,0x03
        ldi     r20,0x3f
        rjmp    .L_m1
.L_x2:  dec     r21
        brne    .L_x3
        lsr     r20
        brcs    .L_ms
        inc     r21
        rjmp    .L_m1
.L_x6:  add     r19,r20
.L_n2:  ldi     r21,0x04
        ldi     r20,0xff
        rjmp    .L_m1
.L_n1:  inc     r20
        brne    .L_m4
        inc     r21
.L_n5:  ror     r20
        brcs    .L_demlzend
        rol     r21
        add     r18,r18
        brne    .L_n6
        elpm    r18,Z+
        rol     r18
.L_n6:  brcc    .L_n5
        add     r19,r21
        ldi     r21,6
        rjmp    .L_m1
.L_x3:  dec     r21
        brne    .L_x4
        ldi     r19,1
        rjmp    .L_m3
.L_x4:  dec     r21
        brne    .L_x5
        inc     r20
        brne    .L_m4
        ldi     r21,0x05
        ldi     r20,0x1f
        rjmp    .L_m1
.L_x5:  dec     r21
        brne    .L_x6
        mov     r21,r20
.L_m4:  elpm    r20,Z+
.L_m3:  dec     r21
        mov     r26,r20
        mov     r27,r21
        add     r26,r28
        adc     r27,r29
.L_ldirloop:
        subi    r26,lo8(megabuffer)             ;
        sbci    r27,hi8(megabuffer)             ;
        andi    r27,DBMASK_HI                   ;X warp
        subi    r26,lo8(-(megabuffer))          ;
        sbci    r27,hi8(-(megabuffer))          ;
        ld      r0,X+
        st      Y+,r0
;
        out     _SFR_IO_ADDR(SPDR),r0
.L_4:   sbis    _SFR_IO_ADDR(SPSR),SPIF
        rjmp    .L_4
;
        subi    r28,lo8(megabuffer)             ;
        sbci    r29,hi8(megabuffer)             ;
        andi    r29,DBMASK_HI                   ;Y warp
        subi    r28,lo8(-(megabuffer))          ;
        sbci    r29,hi8(-(megabuffer))          ;
        dec     r19
        brne    .L_ldirloop
        rjmp    .L_m0

.L_demlzend:
        sbis    _SFR_IO_ADDR(PINF),PF2          ;ждём 1 на CONFDONE
        rjmp    .L_demlzend
        pop     r29
        pop     r28
        ret
.endfunc
;
;------------------------------------------------------------------------------
;
.global TIMER3_COMPA_vect
.func TIMER3_COMPA_vect
TIMER3_COMPA_vect:
        push    r18
        in      r18,_SFR_IO_ADDR(SREG)
        push    r18

        lds     r18,mscounter
        sbrc    r18,4
        cbi     _SFR_IO_ADDR(PORTB),7           ; led on
        inc     r18
        sts     mscounter,r18
        brne    .L_t3_cmpa
        sbi     _SFR_IO_ADDR(PORTB),7           ; led off
        lds     r18,(mscounter)+1
        inc     r18
        sts     (mscounter)+1,r18
.L_t3_cmpa:

        pop     r18
        out     _SFR_IO_ADDR(SREG),r18
        pop     r18
        reti
.endfunc
;
;------------------------------------------------------------------------------
;
.global INT6_vect
.func INT6_vect
INT6_vect:
        push    r18
        in      r18,_SFR_IO_ADDR(SREG)
        push    r18

        lds     r18,int6vect
        sbrc    r18,0
        call    t_beep_int
        sbrc    r18,1
        sts     newframe,r18

        pop     r18
        out     _SFR_IO_ADDR(SREG),r18
        pop     r18
        reti
.endfunc
;
;------------------------------------------------------------------------------
