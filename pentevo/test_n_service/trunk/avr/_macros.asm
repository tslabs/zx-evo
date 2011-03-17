.MACRO  PUSHW
        PUSH    WL
        PUSH    WH
.ENDMACRO

.MACRO  POPW
        POP     WH
        POP     WL
.ENDMACRO

.MACRO  PUSHX
        PUSH    XL
        PUSH    XH
.ENDMACRO

.MACRO  POPX
        POP     XH
        POP     XL
.ENDMACRO

.MACRO  PUSHY
        PUSH    YL
        PUSH    YH
.ENDMACRO

.MACRO  POPY
        POP     YH
        POP     YL
.ENDMACRO

.MACRO  PUSHZ
        PUSH    ZL
        PUSH    ZH
.ENDMACRO

.MACRO  POPZ
        POP     ZH
        POP     ZL
.ENDMACRO

.MACRO  LDIW
        LDI     WL,LOW(@0)
        LDI     WH,HIGH(@0)
.ENDMACRO

.MACRO  LDIX
        LDI     XL,LOW(@0)
        LDI     XH,HIGH(@0)
.ENDMACRO

.MACRO  LDIY
        LDI     YL,LOW(@0)
        LDI     YH,HIGH(@0)
.ENDMACRO

.MACRO  LDIZ
        LDI     ZL,LOW(@0)
        LDI     ZH,HIGH(@0)
.ENDMACRO

.MACRO  LDSW
        LDS     WL,@0
        LDS     WH,@0+1
.ENDMACRO

.MACRO  LDSX
        LDS     XL,@0
        LDS     XH,@0+1
.ENDMACRO

.MACRO  LDSY
        LDS     YL,@0
        LDS     YH,@0+1
.ENDMACRO

.MACRO  LDSZ
        LDS     ZL,@0
        LDS     ZH,@0+1
.ENDMACRO

.MACRO  STSW
        STS     @0,WL
        STS     @0+1,WH
.ENDMACRO

.MACRO  STSX
        STS     @0,XL
        STS     @0+1,XH
.ENDMACRO

.MACRO  STSY
        STS     @0,YL
        STS     @0+1,YH
.ENDMACRO

.MACRO  STSZ
        STS     @0,ZL
        STS     @0+1,ZH
.ENDMACRO

; @0 - port, @1 - regs
.MACRO  OUTPORT
        STS     @0+$20*(@0<$40),@1
.ENDMACRO

; @0 - regs, @1 - port
.MACRO  INPORT
        LDS     @0,@1+$20*(@1<$40)
.ENDMACRO

; ADDI reg,const   Осторожно с флагами!
.MACRO  ADDI
        SUBI    @0,(-(@1)&$FF)
.ENDMACRO
;X+const16
.MACRO  ADIWX
        SUBI    XL,LOW(-@0&$FFFF)
        SBCI    XH,HIGH(-@0&$FFFF)
.ENDMACRO

.MACRO  ADIWY
        SUBI    YL,LOW(-@0&$FFFF)
        SBCI    YH,HIGH(-@0&$FFFF)
.ENDMACRO

.MACRO  ADIWZ
        SUBI    ZL,LOW(-@0&$FFFF)
        SBCI    ZH,HIGH(-@0&$FFFF)
.ENDMACRO

.MACRO  GETMEM
        SBIW    YL,@0
.ENDMACRO

.MACRO  FREEMEM
        ADIW    YL,@0
.ENDMACRO

.MACRO  LDH
        LDD     @0,Y+@1
.ENDMACRO

.MACRO  STH
        STD     Y+@0,@1
.ENDMACRO

;max. 23703 us
;CPU @ 11059200Hz
.MACRO  DELAY_US
.IF (@0<70)
        LDI     WL,LOW(@0*36864/10000)
DLY_US1:DEC     WL      ;1
        BRNE    DLY_US1 ;2
.ELSE
        LDI     WL,LOW(@0*27648/10000)
        LDI     WH,HIGH(@0*27648/10000)
DLY_US2:SBIW    WL,1    ;2
        BRNE    DLY_US2 ;2
.ENDIF
.ENDMACRO
