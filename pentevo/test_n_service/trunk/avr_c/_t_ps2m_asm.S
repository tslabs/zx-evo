#include <avr/io.h>
#include "_t_ps2m.h"
;
;--------------------------------------
;
.macro  ps2m_dataline_up
        cbi     _SFR_IO_ADDR(DDRD),7
        sbi     _SFR_IO_ADDR(PORTD),7
.endm
;
.macro  ps2m_dataline_down
        cbi     _SFR_IO_ADDR(PORTD),7
        sbi     _SFR_IO_ADDR(DDRD),7
.endm
;
.macro  ps2m_clockline_up
        cbi     _SFR_IO_ADDR(DDRE),5
        sbi     _SFR_IO_ADDR(PORTE),5
.endm
;
.macro  ps2m_clockline_down
        cbi     _SFR_IO_ADDR(PORTE),5
        sbi     _SFR_IO_ADDR(DDRE),5
.endm
;
;--------------------------------------
;
.global INT5_vect
.func INT5_vect
INT5_vect:
        push    r18
        in      r18,_SFR_IO_ADDR(SREG)
        push    r18
        push    r19
        push    r20
        push    r21
        ldi     r21,1
        lds     r19,ps2m_data
        lds     r20,ps2m_bit_count
        lds     r18,ps2m_flags
        sbrc    r18,PS2M_BIT_TX
        rjmp    .L_int5tx_0

        cpi     r20,9
        breq    .L_int5rx9
        cpi     r20,10
        brcc    .L_int5rxs
        tst     r20
        brne    .L_int5rx1
;start bit
        sbic    _SFR_IO_ADDR(PIND),7   ; data line
        rjmp    .L_int5rx_clr_d
        cbr     r18,(1<<PS2M_BIT_PARITY)
        inc     r20
        rjmp    .L_int5_exit
;data bits
.L_int5rx1:
        lsr     r19
        sbis    _SFR_IO_ADDR(PIND),7   ; data line
        rjmp    .L_int5rx2
        ori     r19,0b10000000
        eor     r18,r21
.L_int5rx2:
        inc     r20
        rjmp    .L_int5_exit
;parity bit
.L_int5rx9:
        sbic    _SFR_IO_ADDR(PIND),7   ; data line
        eor     r18,r21
        sbrs    r18,PS2M_BIT_PARITY
        rjmp    .L_int5rx_clr_d
        inc     r20
        rjmp    .L_int5_exit
;stop bit
.L_int5rxs:
        sbis    _SFR_IO_ADDR(PIND),7   ; data line
        rjmp    .L_int5rx_clr_d

        sts     ps2m_raw_ready,r20
        sts     ps2m_raw_code,r19

.L_int5rx_clr_f:
        clr     r18
.L_int5rx_clr_d:
        clr     r19
        clr     r20
        rjmp    .L_int5_exit
;-------
.L_int5tx_0:
        cpi     r20,9
        breq    .L_int5tx9
        cpi     r20,10
        breq    .L_int5txs
        cpi     r20,11
        brcc    .L_int5txa
        tst     r20
        brne    .L_int5tx1
;start bit
        sbr     r18,(1<<PS2M_BIT_PARITY)
        inc     r20
        rjmp    .L_int5_exit
;data bits
.L_int5tx1:
        ror     r19
        brcc    .L_int5tx2
        ps2m_dataline_up
        eor     r18,r21
        inc     r20
        rjmp    .L_int5_exit
.L_int5tx2:
        ps2m_dataline_down
        inc     r20
        rjmp    .L_int5_exit
;parity bit
.L_int5tx9:
        sbrc    r18,PS2M_BIT_PARITY
        rjmp    .L_int5txp
        ps2m_dataline_down
        inc     r20
        rjmp    .L_int5_exit
.L_int5txp:
        ps2m_dataline_up
        inc     r20
        rjmp    .L_int5_exit
;stop bit
.L_int5txs:ps2m_dataline_up
        inc     r20
        rjmp    .L_int5_exit
;ack-bit
.L_int5txa:
        clr     r18
        sbis    _SFR_IO_ADDR(PIND),7   ; data line
        ldi     r18,(1<<PS2M_BIT_ACKBIT)
        clr     r19
        clr     r20

.L_int5_exit:
        sts     ps2m_bit_count,r20
        sts     ps2m_data,r19
        sts     ps2m_flags,r18
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
;
