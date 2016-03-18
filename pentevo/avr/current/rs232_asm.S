#include <avr/io.h>
#include "rs232.h"
;
;------------------------------------------------------------------------------
;
.global USART0_RX_vect
USART0_RX_vect:
        push    r18
        in      r18,_SFR_IO_ADDR(SREG)
        push    r18
        push    r19
        push    r26

        in      r18,_SFR_IO_ADDR(UDR0)
        lds     r19,zf_rx_hd
        lds     r26,zf_rx_tl
        sec
        sbc     r26,r19
        breq    .L_urx9

        push    r27
        ldi     r26,lo8(zf_rxbuff)
        ldi     r27,hi8(zf_rxbuff)
        add     r26,r19
        brcc    .L_urx1
        inc     r27
.L_urx1:st      X,r18
        inc     r19
        sts     zf_rx_hd,r19
        pop     r27
.L_urx9:
        pop     r26
        pop     r19
        pop     r18
        out     _SFR_IO_ADDR(SREG),r18
        pop     r18
        reti
;
;------------------------------------------------------------------------------
;
.global USART0_UDRE_vect
USART0_UDRE_vect:
        push    r18
        in      r18,_SFR_IO_ADDR(SREG)
        push    r18
        push    r19

        lds     r19,zf_tx_tl
        lds     r18,zf_tx_hd
        cp      r18,r19
        breq    .L_utx1

        push    r26
        push    r27
        ldi     r26,lo8(zf_txbuff)
        ldi     r27,hi8(zf_txbuff)
        add     r26,r19
        brcc    .L_utx2
        inc     r27
.L_utx2:ld      r18,X
        out     _SFR_IO_ADDR(UDR0),r18
        inc     r19
        sts     zf_tx_tl,r19
        pop     r27
        pop     r26
        rjmp    .L_utx9

.L_utx1:cbi     _SFR_IO_ADDR(UCSR0B),UDRIE0

.L_utx9:pop     r19
        pop     r18
        out     _SFR_IO_ADDR(SREG),r18
        pop     r18
        reti
;
;------------------------------------------------------------------------------
