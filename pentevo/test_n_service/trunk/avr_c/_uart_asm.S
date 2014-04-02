#include <avr/io.h>
#include "_global.h"
#include "_uart.h"
;
;------------------------------------------------------------------------------
;
.global USART1_RX_vect
.func USART1_RX_vect
USART1_RX_vect:
        push    r18
        in      r18,_SFR_IO_ADDR(SREG)
        push    r18
        push    r19
        push    r26

        lds     r18,_SFR_MEM_ADDR(UDR1)
        lds     r19,uart_rx_hd
        lds     r26,uart_rx_tl
        sec
        sbc     r26,r19
        andi    r26,U_RX_MASK
        breq    .L_urx9

        push    r27
        ldi     r26,lo8(uart_rxbuff)
        ldi     r27,hi8(uart_rxbuff)
        add     r26,r19
        brcc    .L_urx1
        inc     r27
.L_urx1:st      X,r18
        inc     r19
        andi    r19,U_RX_MASK
        sts     uart_rx_hd,r19

        lds     r26,flags1
        andi    r26,RTSCTS_FLOWCTRL
        breq    .L_urx2
        lds     r26,uart_rx_tl
        sec
        sbc     r26,r19
        andi    r26,U_RX_MASK
        cpi     r26,21
        brcc    .L_urx2
        sbi     _SFR_IO_ADDR(PORTD),5           ; set RTS
.L_urx2:
        pop     r27
.L_urx9:
        pop     r26
        pop     r19
        pop     r18
        out     _SFR_IO_ADDR(SREG),r18
        pop     r18
        reti
.endfunc
;
;------------------------------------------------------------------------------
;
.global USART1_UDRE_vect
.func USART1_UDRE_vect
USART1_UDRE_vect:
        push    r18
        in      r18,_SFR_IO_ADDR(SREG)
        push    r18
        push    r19

        lds     r19,uart_tx_tl
        lds     r18,uart_tx_hd
        cp      r18,r19
        breq    .L_utx1

        lds     r18,flags1
        andi    r18,RTSCTS_FLOWCTRL
        breq    .L_utx2
        sbic    _SFR_IO_ADDR(PINB),6
        rjmp    .L_utx1

.L_utx2:push    r26
        push    r27
        ldi     r26,lo8(uart_txbuff)
        ldi     r27,hi8(uart_txbuff)
        add     r26,r19
        brcc    .L_utx3
        inc     r27
.L_utx3:ld      r18,X
        sts     _SFR_MEM_ADDR(UDR1),r18
        inc     r19
        andi    r19,U_TX_MASK
        sts     uart_tx_tl,r19
        pop     r27
        pop     r26
        rjmp    .L_utx9

.L_utx1:lds     r18,_SFR_MEM_ADDR(UCSR1B)
        cbr     r18,(1<<UDRIE1)
        sts     _SFR_MEM_ADDR(UCSR1B),r18

.L_utx9:pop     r19
        pop     r18
        out     _SFR_IO_ADDR(SREG),r18
        pop     r18
        reti
.endfunc
;
;------------------------------------------------------------------------------
