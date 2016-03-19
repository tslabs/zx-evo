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
        breq    .L_u0rx9

        push    r27
        ldi     r26,lo8(zf_rxbuff)
        ldi     r27,hi8(zf_rxbuff)
        add     r26,r19
        brcc    .L_u0rx1
        inc     r27
.L_u0rx1:
        st      X,r18
        inc     r19
        sts     zf_rx_hd,r19
        pop     r27

.L_u0rx9:
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
        breq    .L_u0tx1

        push    r26
        push    r27
        ldi     r26,lo8(zf_txbuff)
        ldi     r27,hi8(zf_txbuff)
        add     r26,r19
        brcc    .L_u0tx2
        inc     r27
.L_u0tx2:
        ld      r18,X
        out     _SFR_IO_ADDR(UDR0),r18
        inc     r19
        sts     zf_tx_tl,r19
        pop     r27
        pop     r26
        rjmp    .L_u0tx9

.L_u0tx1:
        cbi     _SFR_IO_ADDR(UCSR0B),UDRIE0

.L_u0tx9:
        pop     r19
        pop     r18
        out     _SFR_IO_ADDR(SREG),r18
        pop     r18
        reti
;
;------------------------------------------------------------------------------
;
.global USART1_RX_vect
USART1_RX_vect:
        push    r18
        in      r18,_SFR_IO_ADDR(SREG)
        push    r18
        push    r19
        push    r26

        lds     r18,_SFR_IO_ADDR(UDR1)
        lds     r19,rs_rx_hd
        lds     r26,rs_rx_tl
        sec
        sbc     r26,r19
        breq    .L_u1rx2

        push    r27
        ldi     r26,lo8(rs_rxbuff)
        ldi     r27,hi8(rs_rxbuff)
        add     r26,r19
        brcc    .L_u1rx1
        inc     r27
.L_u1rx1:
        st      X,r18
        inc     r19
        sts     rs_rx_hd,r19
        pop     r27

.L_u1rx9:
        lds     r19,rs232_LSR
        ori     r19,0x01  ;set data received flag
.L_u1rx3:
        sts     rs232_LSR,r19
        pop     r26
        pop     r19
        pop     r18
        out     _SFR_IO_ADDR(SREG),r18
        pop     r18
        reti
        
.L_u1rx2:
        lds     r19,rs232_LSR
        ori     r19,0x03  ;set data received flag + overrun flag
        rjmp    .L_u1rx3
;
;------------------------------------------------------------------------------
;
.global USART1_UDRE_vect
USART1_UDRE_vect:
        push    r18
        in      r18,_SFR_IO_ADDR(SREG)
        push    r18
        push    r19

        ;if FIFO is empty, disable TX interrupt and exit
        lds     r19,rs_tx_tl
        lds     r18,rs_tx_hd
        cp      r18,r19
        breq    .L_u1tx1

        ;send data byte
        push    r26
        push    r27
        ldi     r26,lo8(rs_txbuff)
        ldi     r27,hi8(rs_txbuff)
        add     r26,r19
        brcc    .L_u1tx2
        inc     r27
.L_u1tx2:
        ld      r18,X
        sts     _SFR_IO_ADDR(UDR1),r18
        inc     r19
        sts     rs_tx_tl,r19
        pop     r27
        pop     r26
        
        ;if FIFO is empty, set flag
        lds     r19,rs_tx_tl
        lds     r18,rs_tx_hd
        cp      r18,r19
        brne    .L_u1tx9
        
        lds     r19,rs232_LSR
        ori     r19,0x60  ;set fifo is empty flag
        sts     rs232_LSR,r19
        rjmp    .L_u1tx9

.L_u1tx1:
        lds     r19,_SFR_IO_ADDR(UCSR1B)
        andi    r19,~_BV(UDRIE1)
        sts     _SFR_IO_ADDR(UCSR1B),r19

.L_u1tx9:
        pop     r19
        pop     r18
        sts     _SFR_IO_ADDR(SREG),r18
        pop     r18
        reti
;
;------------------------------------------------------------------------------
