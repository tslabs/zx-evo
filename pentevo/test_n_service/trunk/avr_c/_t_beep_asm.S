#include <avr/io.h>
#include "_global.h"
;
;------------------------------------------------------------------------------
;
.global t_beep_int
.func t_beep_int
t_beep_int:
        push    r18
        push    r27
        push    r24
        push    r25
        push    r30
        push    r31
        ldi     r31,hi8(tabl_sin)
        lds     r27,t_beep_ptr
        lds     r30,(t_beep_ptr)+1
        lds     r24,t_beep_delta
        lds     r25,(t_beep_delta)+1

.L_tbeep1:
        add     r27,r24
        adc     r30,r25
        lpm     r18,Z
        cbi     _SFR_IO_ADDR(PORTB),0           ; clear nSPICS
        out     _SFR_IO_ADDR(SPDR),r18
.L_tbeep2:
        sbis    _SFR_IO_ADDR(SPSR),SPIF
        rjmp    .L_tbeep2
        in      r18,_SFR_IO_ADDR(SPDR)
        sbi     _SFR_IO_ADDR(PORTB),0           ; set nSPICS
        cpi     r18,12
        brcs    .L_tbeep1

        sts     t_beep_ptr,r27
        sts     (t_beep_ptr)+1,r30
        pop     r31
        pop     r30
        pop     r25
        pop     r24
        pop     r27
        ldi     r18,(1<<INTF6)
        out     _SFR_IO_ADDR(EIFR),r18
        pop     r18
        ret
.endfunc
;
;------------------------------------------------------------------------------
