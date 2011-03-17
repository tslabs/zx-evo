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
        SUBI    @0,(-@1&$FF)
.ENDMACRO
