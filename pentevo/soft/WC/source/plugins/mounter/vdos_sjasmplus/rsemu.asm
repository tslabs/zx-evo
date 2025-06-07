;PACKET OUT: RSASKWORD, OPER 6/5,TRACK, SECTOR,(DATA),CRC
;PACKET IN: RSANSWORD, (DATA), CRC

;INPUT HL ADRESS  D TRACK E SECTOR C OPER 5/6

RSASKWORD EQU   #AA55
RSANSWORD EQU   #CCEE
RSATTEMPTS EQU  10

RSLSR   EQU     #FD
RSMSR   EQU     #FE
RSBASE  EQU     #EF
RSDATA  EQU     #F8
RSFCR   EQU     #FA
RSSPEED EQU     #0001

;       CALL    RSINIT
RSEMU
        LD      A,C
        SUB     5
        JR      Z,RSE_RD
        DEC     A
        RET     NZ
RSE_RD
        EX      AF,AF'
        PUSH    AF
        PUSH    IX
        LD      (RSADR),HL
        LD      (RSTRKS),DE
        LD      (RSOPDR),BC

        LD      C,RSBASE
        LD      LX,RSATTEMPTS ;ATTEMPTS TO LOAD

        LD      A,RSLSR
        IN      A,(RSBASE)
        RRA

RSCLRBUF
        CALL    C,RSGETBYTE2
        JR      C,RSCLRBUF

        CALL    RSWFIFO
        LD      B,RSDATA
        LD      HL,RSASKWORD
        OUT     (C),H
        OUT     (C),L
        LD      A,H
        XOR     L

RSOPDR  EQU     $+1
        LD      DE,#2121
        OUT     (C),D           ;DRIVE
        OUT     (C),E           ;OPER
RSTRKS  EQU     $+1
        LD      HL,#2121
        OUT     (C),H
        OUT     (C),L
        XOR     E
        XOR     D
        XOR     H
        XOR     L
        OUT     (C),A

        ld      A,(RSOPDR)
        cp      5
        jr      NZ,RSSAVE

        LD      HL,RSANSWORD

        LD      HX,200
        CALL    RSGETBYTE
        JR      NC,RSERRLOAD
        CP      H
        JR      NZ,RSERRLOAD
        CALL    RSGETBYTE2
        JR      NC,RSERRLOAD
        CP      L
        JR      NZ,RSERRLOAD

        LD      A,H
        XOR     L
        EX      AF,AF'
        LD      DE,#0100
RSADR   EQU     $+1
        LD      HL,#2121

RSLDDATA
        CALL    RSGETBYTE2
        JR      NC,RSERRLOAD
        LD      (HL),A
        EX      AF,AF'
        XOR     (HL)
        EX      AF,AF'
        INC     HL
        DEC     DE
        LD      A,D
        OR      E
        JR      NZ,RSLDDATA

        CALL    RSGETBYTE2      ;CRC

;       CALL    JSTS

        JR      NC,RSERRLOAD
        LD      L,A
        EX      AF,AF'
        XOR     L
        JR      NZ,RSERRLOAD
        JR      EXIT

RSSAVE
        XOR     A
        EX      AF,AF'
        LD      DE,#0100
        LD      HL,(RSADR)

RSSVDATA
        CALL    RSWFIFO
        LD      A,(HL)
        OUT     (C),A
        EX      AF,AF'
        XOR     (HL)
        EX      AF,AF'
        INC     HL
        DEC     DE
        LD      A,D
        OR      E
        JR      NZ,RSSVDATA

        EX      AF,AF'
        OUT     (C),A

EXIT 
        POP     IX
        POP     AF
        EX      AF,AF'
        SCF
        RET

RSERRLOAD
        DEC     LX
        JP      NZ,RSCLRBUF
        POP     IX
        POP     AF
        EX      AF,AF'
        OR      A
        RET                     ;NOT LOADED

RSWFIFO
        LD      A,RSLSR
        IN      A,(RSBASE)
        AND     #20
        JR      Z,RSWFIFO
        RET

RSGETBYTE2
        LD      HX,2*7*4+2 ;2 X INT  7*10000*4 (14MHZ)
RSGETBYTE
        LD      B,0
RSGETBYTEC
        LD      A,RSLSR
        IN      A,(RSBASE)
        RRA
        JR      C,RSLYBYTE
        DJNZ    RSGETBYTEC      ;7 11 4 7 13 = 42 * 256 = 10752
        DEC     HX
        RET     Z               ;C=0
        JR      RSGETBYTEC
RSLYBYTE
        LD      B,RSDATA
        IN      A,(C)
        RET                     ;C=1

RSINIT
        LD      BC,#FBEF
        LD      A,#83
        OUT     (C),A

        LD      B,#F8
        LD      HL,RSSPEED
        OUT     (C),L
        INC     B
        OUT     (C),H

        LD      B,#FB
        LD      A,#03
        OUT     (C),A
        LD      B,RSFCR
        LD      A,#07
        OUT     (C),A

;       LD      BC,#FCEF
;       LD      A,1
;       OUT     (C),A   ;DTR ON

        RET