
;Распаковщик файлов, упакованных методом
;Hrust1:  (программы Hrust1.0, Hrust1.1)
;
;Не использует регистр SP,  поэтому рас-
;паковку  можно проводить и при включен-
;ных прерываниях.
;         !!!Неперемещаемый!!!
;
;Можно использовать "стандартный", пере-
;мещаемый распаковщик длиной 256 байт.
;Однако он использует регистр SP.
;
;Для распаковки файла надо задать в:
;HL - откуда распаковывать
;DE - куда распаковывать
;и вызвать депакер (CALL DEHRUST).
;Области откуда/куда распаковывать могут
;пересекаться, депакер это учитывает.
;Если эти области не пересекаются, то
;распаковать файл можно несколько раз.
;Больше депакер не использует никаких
;областей (исключая 6 байт в стеке).
;
;
;**************************************
;*    E-mail: dp@fmf.gasu.gorny.ru    *
;*    tel:(38822) 244-21. Дмитрий.    *
;*     Россия. г.Горно - Алтайск.     *
;*         01.03.97 - 28.12.98        *
;**************************************

DEHRUST
        LD B,0
        EXX
        LD D,0xBF
        LD C,0x10
        CALL R_DATA
LL4036  LD A,(IX+0x00)
        INC IX
        EXX
LL403C  LD (DE),A
        INC DE
LL403E  EXX
LL403F  ADD HL,HL
        DJNZ LL4045
        CALL R_DATA
LL4045  JR C,LL4036
        LD E,0x01
LL4049  LD A,0x80
LL404B  ADD HL,HL
        DJNZ LL4051
        CALL R_DATA
LL4051  RLA
        JR C,LL404B
        CP 0x03
        JR C,LL405D
        ADD A,E
        LD E,A
        XOR C
        JR NZ,LL4049
LL405D  ADD A,E
        CP 0x04
        JR Z,LL40C4
        ADC A,0xFF
        CP 0x02
        EXX
LL4067  LD C,A
LL4068  EXX
        LD A,0xBF
        JR C,LL4082
LL406D  ADD HL,HL
        DJNZ LL4073
        CALL R_DATA
LL4073  RLA
        JR C,LL406D
        JR Z,LL407D
        INC A
        ADD A,D
        JR NC,LL4084
        SUB D
LL407D  INC A
        JR NZ,LL408D
        LD A,0xEF
LL4082  RRCA
        CP A
LL4084  ADD HL,HL
        DJNZ LL408A
        CALL R_DATA
LL408A  RLA
        JR C,LL4084
LL408D  EXX
        LD H,0xFF
        JR Z,LL409B
        LD H,A
        INC A
        LD A,(IX+0x00)
        INC IX
        JR Z,LL40A6
LL409B  LD L,A
        ADD HL,DE
        LDIR
LL409F  JR LL403E

LL40A1  EXX
        RRC D
        JR LL403F

LL40A6  CP 0xE0
        JR C,LL409B
        RLCA
        XOR C
        INC A
        JR Z,LL40A1
        SUB 0x10
LL40B1  LD L,A
        LD C,A
        LD H,0xFF
        ADD HL,DE
        LDI
        LD A,(IX+0x00)
        INC IX
        LD (DE),A
        INC HL
        INC DE
        LD A,(HL)
        JP LL403C

LL40C4  LD A,0x80
LL40C6  ADD HL,HL
        DJNZ LL40CC
        CALL R_DATA
LL40CC  ADC A,A
        JR NZ,LL40F3
        JR C,LL40C6
        LD A,0xFC
        JR LL40F6

LL40D5  LD B,A
        LD C,(IX+0x00)
        INC IX
        CCF
        JR LL4068

EXIT    CP 0x0F
        JR C,LL40D5
        JR NZ,LL4067
        RET

LL40F3  SBC A,A
        LD A,0xEF
LL40F6  ADD HL,HL
        DJNZ LL40FC
        CALL R_DATA
LL40FC  RLA
        JR C,LL40F6
        EXX
        JR NZ,LL40B1
        BIT 7,A
        JR Z,EXIT
        SUB 0xEA
        ADD A,A
        LD B,A
LL410A  LD A,(IX+0x00)
        INC IX
        LD (DE),A
        INC DE
        DJNZ LL410A
        JR LL409F

R_DATA  LD B,C
        LD L,(IX+0x00)
        LD H,(IX+0x01)
        INC IX
        INC IX
        RET
