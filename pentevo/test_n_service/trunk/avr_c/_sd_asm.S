#include "_global.h"
#include "_sd.h"
#include "_output.h"
#include "_messages.h"
;
.macro  set_spics
        sbi     _SFR_IO_ADDR(PORTB),0
.endm
;
.macro  clr_spics
        cbi     _SFR_IO_ADDR(PORTB),0
.endm
;
;--------------------------------------
;
.comm sd_cardtype, 1
;
;--------------------------------------
;
cmd08:  .byte   0x48,0x00,0x00,0x01,0xaa,0x87
;
;--------------------------------------
.global sd_receive
sd_receive:                                     ; u8 sd_receive(void);
        ser     r24
; - - - - - - - - - - - - - - - - - - -
.global sd_exchange
sd_exchange:                                    ; u8 sd_exchange(u8 data);
        lds     r19,flags1
        sbrc    r19,BIT_ENABLE_SD_LOG
        rjmp    .L_sdexch
;то же, что и fpga_same_reg
        clr_spics
        out     _SFR_IO_ADDR(SPDR),r24
.L_sdrdy1:
        sbis    _SFR_IO_ADDR(SPSR),SPIF
        rjmp    .L_sdrdy1
        in      r24,_SFR_IO_ADDR(SPDR)
        set_spics
        ret
;то же, что и fpga_same_reg, но с логом в rs-232
.L_sdexch:
        push    r18
        push    r19
        cbr     r19,(ENABLE_SCR|ENABLE_DIRECTUART)
        sbr     r19,ENABLE_UART
        sts     flags1,r19
        push    r20
        push    r21
        push    r22
        push    r23
        push    xl
        push    xh
        push    zl
        push    zh
        push    r24
        ldi     r24,lo8(msg_tsd_out)
        ldi     r25,hi8(msg_tsd_out)
        call    print_msg
        pop     r24
        push    r24
        call    print_hexbyte
        pop     r24
        clr_spics
        out     _SFR_IO_ADDR(SPDR),r24
.L_sdrdy2:
        sbis    _SFR_IO_ADDR(SPSR),SPIF
        rjmp    .L_sdrdy2
        in      r24,_SFR_IO_ADDR(SPDR)
        set_spics
        push    r24
        ldi     r24,lo8(msg_tsd_in)
        ldi     r25,hi8(msg_tsd_in)
        call    print_msg
        pop     r24
        push    r24
        call    print_hexbyte
        pop     r24
        pop     zh
        pop     zl
        pop     xh
        pop     xl
        pop     r23
        pop     r22
        pop     r21
        pop     r20
        pop     r19
        sts     flags1,r19
        pop     r18
        ret
;
;--------------------------------------
.global sd_rd_dummy
sd_rd_dummy:                                    ; void sd_rd_dummy(u8 count);
        mov     r18,r24
.L_sdrddummy:
        ser     r24
        rcall   sd_exchange
        dec     r18
        brne    .L_sdrddummy
        ret
;
;--------------------------------------
.global sd_cmd_without_arg
sd_cmd_without_arg:                             ; u8 sd_cmd_without_arg(u8 cmd);
        clr     r23
        clr     r22
; - - - - - - - - - - - - - - - - - - -
.global sd_cmd_with_1arg
sd_cmd_with_1arg:                               ; u8 sd_cmd_with_1arg(u8 cmd, u16 h_arg);
        clr     r21
        clr     r20
; - - - - - - - - - - - - - - - - - - -
.global sd_cmd_with_arg
sd_cmd_with_arg:                                ; u8 sd_cmd_with_arg(u8 cmd, u32 arg);
        push    r24
        ldi     r24,2
        rcall   sd_rd_dummy
        pop     r24
        rcall   sd_exchange
        mov     r24,r23
        rcall   sd_exchange
        mov     r24,r22
        rcall   sd_exchange
        mov     r24,r21
        rcall   sd_exchange
        mov     r24,r20
        rcall   sd_exchange
        ldi     r24,0x95
        rcall   sd_exchange
; - - - - - - - - - - - - - - - - - - -
.global sd_wait_notff
sd_wait_notff:                                  ; u8 sd_wait_notff(void);
        ldi     r18,32
sdwnff2:
        rcall   sd_receive
        cpi     r24,0xff
        brne    sdwnff1
        dec     r18
        brne    sdwnff2
sdwnff1:
        ret
;
;--------------------------------------
;
.global sd_read_sector
sd_read_sector:                                 ; u8 sd_read_sector(u8 *buff, u32 sectnumb);   out: !0=ok
        lds     r18,flags1
        sbrs    r18,BIT_ENABLE_SD_LOG
        rjmp    .L_sdrdse0
        push    r18
        cbr     r18,(ENABLE_SCR|ENABLE_DIRECTUART)
        sbr     r18,ENABLE_UART
        sts     flags1,r18
        push    r24
        push    r25
;        push    xl
;        push    xh
        push    zl
        push    zh
        push    r20
        push    r21
        push    r22
        push    r23
        ldi     r24,lo8(msg_tsd_readsector)
        ldi     r25,hi8(msg_tsd_readsector)
        call    print_msg
        pop     r25
        pop     r24
        pop     r23
        pop     r22
        push    r22
        push    r23
        push    r24
        push    r25
        call    print_hexlong
        pop     r23
        pop     r22
        pop     r21
        pop     r20
        pop     zh
        pop     zl
;        pop     xh
;        pop     xl
        pop     r25
        pop     r24
        pop     r18
        sts     flags1,r18
.L_sdrdse0:
        lds     r18,sd_cardtype
        sbrc    r18,BIT_SDHCFLAG
        rjmp    .L_sdrdse1
        lsl     r20
        rol     r21
        rol     r22
        mov     r23,r22
        mov     r22,r21
        mov     r21,r20
        clr     r20
.L_sdrdse1:
        movw    zl,r24
        ldi     r24,0x40|17                     ;CMD17 (read_single_block)
        rcall   sd_cmd_with_arg
        cpi     r24,0xfe
        brne    .L_sdrdse8

        lds     r18,flags1
        push    r18
        sbrs    r18,BIT_ENABLE_SD_LOG
        rjmp    .L_sdrdse5
        cbr     r18,(ENABLE_SCR|ENABLE_DIRECTUART)
        sbr     r18,ENABLE_UART
        sts     flags1,r18
        push    zl
        push    zh
        ldi     r24,lo8(msg_tsd_skip)
        ldi     r25,hi8(msg_tsd_skip)
        call    print_msg
        pop     zh
        pop     zl
.L_sdrdse5:
        ldi     xl,0
        ldi     xh,2
.L_sdrdse3:
        rcall   sd_receive
        st      Z+,r24
        sbiw    xl,1
        brne    .L_sdrdse3
        pop     r18
        sts     flags1,r18

        ldi     r24,2+1
        rcall   sd_rd_dummy

        ldi     r24,1
        ret
;ошибка при чтении сектора
.L_sdrdse8:
        clr     r24
        ret
;
;--------------------------------------
;
.L_sd_crc_off:
        ldi     r24,0x40|59                     ;CMD59 (crc_on_off)
        rcall   sd_cmd_without_arg
        tst     r24
        brne    .L_sd_crc_off
        ret
;
;--------------------------------------
;
.L_sd_setblklen:
        ldi     r24,0x40|16                     ;CMD16 (set_blocklen)
        clr     r23
        clr     r22
        ldi     r21,0x02
        clr     r20
        rcall   sd_cmd_with_arg
        tst     r24
        brne    .L_sd_setblklen
        ret
;
;--------------------------------------
;
.global sd_init
sd_init:                                        ; u8 sd_init(void);   out: !0 - ok
        push    r17

        ldi     r24,SD_CS1
        call    fpga_sel_reg
        ldi     r24,32
        rcall   sd_rd_dummy
        ldi     r24,SD_CS0
        call    fpga_sel_reg

        ser     r17
.L_sdinit1:
        ldi     r24,0x40                        ;CMD0 (go_idle_state)
        rcall   sd_cmd_without_arg
        cpi     r24,0x01
        breq    .L_sdinit2
        dec     r17
        brne    .L_sdinit1

        pop     r17
        ldi     r24,0                           ;нет карты
        ret

.L_sdinit2:
        ldi     r24,2
        rcall   sd_rd_dummy
        ldi     zl,lo8(cmd08)                   ;CMD8 (send_if_cond)
        ldi     zh,hi8(cmd08)
.L_sdinit4:
        lpm     r24,Z+
        cpi     r24,0x87
        breq    .L_sdinit3
        rcall   sd_exchange
        rjmp    .L_sdinit4
.L_sdinit3:
        rcall   sd_exchange
        rcall   sd_wait_notff
        ldi     r17,0x00
        sbrs    r24,2
        ldi     r17,0x40
        ldi     r24,4
        rcall   sd_rd_dummy

.L_sdinit5:
        ldi     r24,0x40|55                     ;CMD55
        rcall   sd_cmd_without_arg
        ldi     r24,0x40|41                     ;ACMD41 (sd_send_op_cond)
        mov     r23,r17
        clr     r22
        rcall   sd_cmd_with_1arg
        tst     r24
        breq    .L_sdinit6
        sbrs    r24,2
        rjmp    .L_sdinit5

.L_sdinit7:
        ldi     r24,0x40|1                      ;CMD1 (send_op_cond)
        rcall   sd_cmd_without_arg
        tst     r24
        brne    .L_sdinit7
        rcall   .L_sd_crc_off
        rcall   .L_sd_setblklen
        ldi     r24,MMCFLAG
        rjmp    .L_sdinit9

.L_sdinit6:
        rcall   .L_sd_crc_off
        rcall   .L_sd_setblklen

        ldi     r24,SDV1FLAG
        tst     r17
        breq    .L_sdinit9
        ldi     r24,0x40|58                     ;CMD58 (read_ocr)
        rcall   sd_cmd_without_arg
        rcall   sd_receive
        mov     r17,r24
        ldi     r24,3+2
        rcall   sd_rd_dummy
        ldi     r24,SDV2FLAG
        sbrc    r17,6
        ldi     r24,SDHCFLAG
.L_sdinit9:
        sts     sd_cardtype,r24

        pop     r17
        ldi     r24,1                           ;ok
        ret
;
;--------------------------------------
