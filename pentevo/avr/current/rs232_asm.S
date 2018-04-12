#include <avr/io.h>
#include "rs232.h"
;
;------------------------------------------------------------------------------
;
.global USART1_RX_vect
USART1_RX_vect:
        push    r24
        in      r24,_SFR_IO_ADDR(SREG)
        push    r24
        push    r25
        push    r26
        push    r27
        push    r20

        lds     r20,_SFR_MEM_ADDR(UDR1)
        lds     r24,rs_rx_hd
        lds     r25,rs_rx_hd+1
        lds     r26,rs_rx_tl
        lds     r27,rs_rx_tl+1
        sbiw    r26,1
        andi    r27,0x01
        sub     r26,r24
        sbc     r27,r25
        breq    .Lu1rx8

        ldi     r26,lo8(rs_rxbuff)
        ldi     r27,hi8(rs_rxbuff)
        add     r26,r24
        adc     r27,r25
        st      X,r20
        adiw    r24,1
        andi    r25,0x01
        sts     rs_rx_hd,r24
        sts     rs_rx_hd+1,r25
        lds     r20,rs232_LSR
        rjmp    .Lu1rx9

.Lu1rx8:lds     r20,rs232_LSR
        sbr     r20,0x02
.Lu1rx9:sbr     r20,0x01
        sts     rs232_LSR,r20

        ldi     r24,0
        sts     rs_tmo_cnt,r24

        pop     r20
        pop     r27
        pop     r26
        pop     r25
        pop     r24
        out     _SFR_IO_ADDR(SREG),r24
        pop     r24
        reti
;
;------------------------------------------------------------------------------
;
.global USART1_UDRE_vect
USART1_UDRE_vect:
        push    r24
        in      r24,_SFR_IO_ADDR(SREG)
        push    r24
        push    r25

        lds     r25,rs_tx_tl
        lds     r24,rs_tx_hd
        cp      r24,r25
        breq    .Lu1tx1

        push    r26
        push    r27
        ldi     r26,lo8(rs_txbuff)
        ldi     r27,hi8(rs_txbuff)
        add     r26,r25
        brcc    .Lu1tx2
        inc     r27
.Lu1tx2:ld      r24,X
        sts     _SFR_MEM_ADDR(UDR1),r24
        inc     r25
        sts     rs_tx_tl,r25
        pop     r27
        pop     r26
        rjmp    .Lu1tx9

.Lu1tx1:lds     r24,_SFR_MEM_ADDR(UCSR1B)
        cbr     r24,(1<<UDRIE1)
        sts     _SFR_MEM_ADDR(UCSR1B),r24

.Lu1tx9:lds     r24,rs232_LSR
        sbr     r24,0x20
        sts     rs232_LSR,r24

        pop     r25
        pop     r24
        out     _SFR_IO_ADDR(SREG),r24
        pop     r24
        reti
;
;------------------------------------------------------------------------------
;
.global USART0_RX_vect
USART0_RX_vect:
        push    r24
        in      r24,_SFR_IO_ADDR(SREG)
        push    r24
        push    r25
        push    r26
        push    r27
        push    r20

        in      r20,_SFR_IO_ADDR(UDR0)
        lds     r24,zf_rx_hd
        lds     r25,zf_rx_hd+1
        lds     r26,zf_rx_tl
        lds     r27,zf_rx_tl+1
        sbiw    r26,1
        andi    r27,0x01
        sub     r26,r24
        sbc     r27,r25
        breq    .Lu0rx9

        ldi     r26,lo8(zf_rxbuff)
        ldi     r27,hi8(zf_rxbuff)
        add     r26,r24
        adc     r27,r25
        st      X,r20
        adiw    r24,1
        andi    r25,0x01
        sts     zf_rx_hd,r24
        sts     zf_rx_hd+1,r25

.Lu0rx9:
        ldi     r24,0
        sts     zf_tmo_cnt,r24

        pop     r20
        pop     r27
        pop     r26
        pop     r25
        pop     r24
        out     _SFR_IO_ADDR(SREG),r24
        pop     r24
        reti
;
;------------------------------------------------------------------------------
;
.global USART0_UDRE_vect
USART0_UDRE_vect:
        push    r24
        in      r24,_SFR_IO_ADDR(SREG)
        push    r24
        push    r25

        lds     r25,zf_tx_tl
        lds     r24,zf_tx_hd
        cp      r24,r25
        breq    .Lu0tx1

        push    r26
        push    r27
        ldi     r26,lo8(zf_txbuff)
        ldi     r27,hi8(zf_txbuff)
        add     r26,r25
        brcc    .Lu0tx2
        inc     r27
.Lu0tx2:ld      r24,X
        out     _SFR_IO_ADDR(UDR0),r24
        inc     r25
        sts     zf_tx_tl,r25
        pop     r27
        pop     r26
        rjmp    .Lu0tx9

.Lu0tx1:cbi     _SFR_IO_ADDR(UCSR0B),UDRIE0

.Lu0tx9:pop     r25
        pop     r24
        out     _SFR_IO_ADDR(SREG),r24
        pop     r24
        reti
;
;------------------------------------------------------------------------------
