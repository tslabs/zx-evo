#include <avr/io.h>
#include "_ps2k.h"
;
;--------------------------------------
;
.macro  ps2k_dataline_up
        cbi     _SFR_IO_ADDR(DDRD),6
        sbi     _SFR_IO_ADDR(PORTD),6
.endm
;
.macro  ps2k_dataline_down
        cbi     _SFR_IO_ADDR(PORTD),6
        sbi     _SFR_IO_ADDR(DDRD),6
.endm
;
.macro  ps2k_clockline_up
        cbi     _SFR_IO_ADDR(DDRE),4
        sbi     _SFR_IO_ADDR(PORTE),4
.endm
;
.macro  ps2k_clockline_down
        cbi     _SFR_IO_ADDR(PORTE),4
        sbi     _SFR_IO_ADDR(DDRE),4
.endm
;
;--------------------------------------
;
.global TIMER0_OVF_vect
.func TIMER0_OVF_vect
TIMER0_OVF_vect:
        ps2k_dataline_up
        push    r18
        in      r18,_SFR_IO_ADDR(SREG)
        push    r18
        in      r18,_SFR_IO_ADDR(TIMSK)
        cbr     r18,(1<<TOIE0)
        out     _SFR_IO_ADDR(TIMSK),r18
        pop     r18
        out     _SFR_IO_ADDR(SREG),r18
        pop     r18
        ps2k_clockline_up
        reti
.endfunc
;
;--------------------------------------
;
.global INT4_vect
.func INT4_vect
INT4_vect:
        push    r18
        in      r18,_SFR_IO_ADDR(SREG)
        push    r18
        push    r19
        push    r20
        push    r21
        ldi     r21,1
        lds     r19,ps2k_data
        lds     r20,ps2k_bit_count
        lds     r18,ps2k_flags
        sbrc    r18,PS2K_BIT_TX
        rjmp    .L_int4tx_0

        cpi     r20,9
        breq    .L_int4rx9
        cpi     r20,10
        brcc    .L_int4rxs
        tst     r20
        brne    .L_int4rx1
;start bit
        sbic    _SFR_IO_ADDR(PIND),6   ; data line
        rjmp    .L_int4rx_clr_d
        cbr     r18,(1<<PS2K_BIT_PARITY)
        inc     r20
        rjmp    .L_int4_exit
;data bits
.L_int4rx1:
        lsr     r19
        sbis    _SFR_IO_ADDR(PIND),6   ; data line
        rjmp    .L_int4rx2
        ori     r19,0b10000000
        eor     r18,r21
.L_int4rx2:
        inc     r20
        rjmp    .L_int4_exit
;parity bit
.L_int4rx9:
        sbic    _SFR_IO_ADDR(PIND),6   ; data line
        eor     r18,r21
        sbrs    r18,PS2K_BIT_PARITY
        rjmp    .L_int4rx_clr_d
        inc     r20
        rjmp    .L_int4_exit
;stop bit
.L_int4rxs:
        sbis    _SFR_IO_ADDR(PIND),6   ; data line
        rjmp    .L_int4rx_clr_d

        sts     ps2k_raw_ready,r20
        sts     ps2k_raw_code,r19
        ps2k_clockline_down
        ldi     r20,0x80
        out     _SFR_IO_ADDR(TCNT0),r20     ;Tclk*8*128=~92us
        ldi     r20,(1<<TOV0)
        out     _SFR_IO_ADDR(TIFR),r20
        in      r20,_SFR_IO_ADDR(TIMSK)
        ori     r20,(1<<TOIE0)
        out     _SFR_IO_ADDR(TIMSK),r20

        lds     r20,ps2k_skip
        tst     r20
        brne    .L_int4rx_skip

        cpi     r19,0xe1
        breq    .L_int4rx_e1
        cpi     r19,0xaa
        breq    .L_int4rx_clr_f
        cpi     r19,0xab
        breq    .L_int4rx_ab
        cpi     r19,0xee
        breq    .L_int4rx_clr_f
        cpi     r19,0xfa
        breq    .L_int4rx_clr_f
        cpi     r19,0xfe
        breq    .L_int4rx_clr_f
        cpi     r19,0xe0
        breq    .L_int4rx_e0
        cpi     r19,0xf0
        breq    .L_int4rx_f0

        sbr     r18,(1<<PS2K_BIT_READY)
        sts     ps2k_key_flags_and_code,r18
        sts     (ps2k_key_flags_and_code)+1,r19
        rjmp    .L_int4rx_clr_f

.L_int4rx_e0:
        sbr     r18,(1<<PS2K_BIT_EXTKEY)
        rjmp    .L_int4rx_clr_d

.L_int4rx_f0:
        sbr     r18,(1<<PS2K_BIT_RELEASE)
        rjmp    .L_int4rx_clr_d

.L_int4rx_skip:
        dec     r20
        sts     ps2k_skip,r20
        brne    .L_int4rx_clr_d
        lds     r19,(ps2k_key_flags_and_code)+1
        cpi     r19,0x7e
        brne    .L_int4rx_clr_d
        ldi     r18,(1<<PS2K_BIT_READY)|(1<<PS2K_BIT_RELEASE)|(1<<PS2K_BIT_EXTKEY)
        sts     ps2k_key_flags_and_code,r18
        rjmp    .L_int4rx_clr_f

.L_int4rx_ab:
        clr     r19
        sts     (ps2k_key_flags_and_code)+1,r19
        sts     ps2k_skip,r21
        rjmp    .L_int4rx_clr_f

.L_int4rx_e1:
        ldi     r18,(1<<PS2K_BIT_READY)|(1<<PS2K_BIT_EXTKEY)
        sts     ps2k_key_flags_and_code,r18
        ldi     r19,0x7e
        sts     (ps2k_key_flags_and_code)+1,r19
        ldi     r20,7
        sts     ps2k_skip,r20

.L_int4rx_clr_f:
        clr     r18
.L_int4rx_clr_d:
        clr     r19
        clr     r20
        rjmp    .L_int4_exit
;-------
.L_int4tx_0:
        cpi     r20,9
        breq    .L_int4tx9
        cpi     r20,10
        breq    .L_int4txs
        cpi     r20,11
        brcc    .L_int4txa
        tst     r20
        brne    .L_int4tx1
;start bit
        sbr     r18,(1<<PS2K_BIT_PARITY)
        inc     r20
        rjmp    .L_int4_exit
;data bits
.L_int4tx1:
        ror     r19
        brcc    .L_int4tx2
        ps2k_dataline_up
        eor     r18,r21
        inc     r20
        rjmp    .L_int4_exit
.L_int4tx2:
        ps2k_dataline_down
        inc     r20
        rjmp    .L_int4_exit
;parity bit
.L_int4tx9:
        sbrc    r18,PS2K_BIT_PARITY
        rjmp    .L_int4txp
        ps2k_dataline_down
        inc     r20
        rjmp    .L_int4_exit
.L_int4txp:
        ps2k_dataline_up
        inc     r20
        rjmp    .L_int4_exit
;stop bit
.L_int4txs:
        ps2k_dataline_up
        inc     r20
        rjmp    .L_int4_exit
;здесь раньше проверялся ack-bit, но CHRV знает ;)
; где есть такие клавиатуры, которые не выдают этот бит.
.L_int4txa:
        ;clr     r18
        ;sbis    _SFR_IO_ADDR(PIND),6   ; data line
        ldi     r18,(1<<PS2K_BIT_ACKBIT)
        clr     r19
        clr     r20

.L_int4_exit:
        sts     ps2k_bit_count,r20
        sts     ps2k_data,r19
        sts     ps2k_flags,r18
        pop     r21
        pop     r20
        pop     r19
        pop     r18
        out     _SFR_IO_ADDR(SREG),r18
        pop     r18
        reti
.endfunc
;
;--------------------------------------
