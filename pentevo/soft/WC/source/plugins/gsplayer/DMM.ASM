;Digital Music Maker;PLAYER & LOADER
;(C) LAVE software 1995;modified by KOSHI/2025;---------------------------------------;--------                    ;onto tune data
SETPATS  LD A,(PATSIZE)      ;Creates PATTERN
         LD B,A              ;POINTERS TABLE
         LD HL,0,DE,9
SPAT02   ADD HL,DE:DJNZ SPAT02
         LD (PSB),HL
         LD B,24
         LD DE,MDATA
         LD HL,PATTAB
SPAT1    LD (HL),E:INC HL
         LD (HL),D:INC HL
         PUSH HL:LD HL,(PSB):ADD HL,DE:EX DE,HL
         POP HL:DJNZ SPAT1
         RET

NEXTTC   PUSH HL             ;Make nessesary settings
         PUSH DE             ;to play NEXT NOTE
         LD HL,(CURPL)
         LD (OLDP),HL
         LD DE,9:ADD HL,DE
         LD (CURPL),HL
         EX DE,HL
pEND     LD HL,#0000
         OR A:SBC HL,DE:JR NC,NTC1
         LD A,(PLPOS):INC A:LD E,A
         LD A,(ENDT):CP E:LD A,E:JR NC,NTC2
         LD A,(LOOPT)
NTC2     LD (PLPOS),A
         CALL S_PLPOS
NTC1     POP DE
         POP HL
         RET

PA2HL    LD L,A              ;Converts PATTERN NUMBER
         LD H,0              ;(reg A) to PATTERN ADDR
         ADD HL,HL           ;(regs HL) with
         EX DE,HL            ;PATTERN POINTERS TABLE
         LD HL,PATTAB:ADD HL,DE
         LD E,(HL):INC HL
         LD D,(HL)
         EX DE,HL
         RET

S_PLPOS  PUSH DE             ;Make settings to play
         PUSH AF             ;POSITION
         LD E,A,D,0
         LD HL,PATS:ADD HL,DE
         LD A,(HL),(PLPAT),A
         CALL PA2HL
         LD (CURPL),HL
         INC A:CALL PA2HL
         LD DE,#FFF7:ADD HL,DE;65527
         LD (pEND+1),HL
         POP AF
         POP DE
         RET
;--------IM2HANDS PUSH AF,BC:LD BC,PW4:IN A:LD (PAGEC),A         POP BC,AF
IM2HAND  CALL 0:DI         LD (SAVEHL),HL         POP HL         PUSH HL         PUSH AF         PUSH BC         LD BC,HL         LD HL,INTA:OR A:SBC HL,BC:JP Z,INTXX         LD HL,INTB:OR A:SBC HL,BC:JP Z,INTXX         LD HL,INTC:OR A:SBC HL,BC:JR Z,INTXC         LD HL,INTc:OR A:SBC HL,BC:JR Z,INTxx         LD HL,INTCC:OR A:SBC HL,BC:JR Z,INTXx         LD HL,INTcc:OR A:SBC HL,BC:JR Z,INTXx         LD HL,INTAA:OR A:SBC HL,BC:JR Z,INTxX         LD HL,INTaa:OR A:SBC HL,BC:JR Z,INTxX         LD HL,INTa:OR A:SBC HL,BC:JR Z,INTxC;!         LD HL,INTBB:OR A:SBC HL,BC:JR Z,INTxX         LD HL,INTb:OR A:SBC HL,BC:JR Z,INTxC;!         LD HL,INTbb:OR A:SBC HL,BC:JR Z,INTxX
         POP BCJINXA    POP AF         JR JINXINTxX    POP BC:LD BC,(BCGO):JR JINXAINTXx    POP BC:LD BC,AYregSEL:JR JINXAINTxx    POP BC:LD BC,AYregDATINTJ     OUT A:LD BC,AYregSEL:JR INTjjINTXC    POP BC:LD BC,PW4:JR INTJINTxC    POP BC:LD BC,(BCGO):JR INTjjINTXX    POP BC:LD BC,PW4:OUT AINTjj    POP AF         POP HL.2       INC HL         PUSH HLJINX     LD HL,(SAVEHL);--------
pEFA     JR pEFB:NOP
pEFB     JR pEFC:NOPpEFC     JR pEFC+3:NOP
         EXA:JR C,RETii
         DEC A:JR Z,NOEXi
         EXA
WHGO     EI
         RETRETii    SCF:EXA:JR WHGO
NOEXi    AND A:LD A,(TEMPOdmm)
         EXA
         PUSH AF
         LD (HLSTO),HL
         LD HL,#C9FB,(WHGO),HL
         LD HL,AYregSEL:OR A:SBC HL,BC
         LD HL,(HLSTO):JR Z,$+3:EXX         CALL SET_A
         CALL SET_B
         CALL SET_C
         CALL NEXTTC
         POP AF
.2       INC SP
         EI
         JR PLAY_LP

NOEXX2   LD (HLSTO),HL
         LD HL,#C9FB,(WHGO),HL
         LD HL,AYregSEL:OR A:SBC HL,BC
         LD HL,(HLSTO):JR Z,$+3:EXX
.2       INC SP
         EI
         JR PLAY_LP

SETRS    LD DE,ZEROES               ;Sets player registers
         LD BC,ZEROES
         LD (pEND_A+1),DE
         LD (pEND_B+1),DE
         LD (pEND_C+1),DE
         LD L,0
         EXX:LD DE,ZEROES,BC,ZEROES,HL,0
         EXX
         RET
;--------PLAY     CALL SETPATS               ;This procedure PLAYS
         XOR A                      ;whole tune from begin
         LD (PLPOS),A
         CALL S_PLPOS
         EXA:XOR A:LD A,1
         EXA         DI         LD HL,(#5BFF),(IM2HAND+1),HL         LD HL,IM2HANDS,(#5BFF),HL
PLAY_SI  CALL SETRS    ;Begin playing with
                       ;silence
         LD BC,AYregSEL,A,7:OUT A
         LD BC,AYregDAT,A,#FF:OUT A
         EI:HALT
;--------
PLAY_LP  LD A,0:INC A:DEC A:JR Z,PP:DI:RET;7,4,4,12PP       LD H,8,BC,AYregSEL:OUT H;7,10,12
         EXX;4
pRAM_A   LD A,VPL;7         LD (BCGO),BC,BC,PW4;20,10INTA     OUT A;12INTAA    LD BC,(BCGO);20
         LD A,(DE);7
pVOL_A   SUB 1:JP C,pEND_A;7,10         LD BC,AYregDAT;10INTa     OUT A;12INTaa    LD BC,(BCGO);20
         LD A,H;4
pLOB_A   ADD A,#FF:LD H,A,A,E;7,4,4
pHIB_A   ADC A,0:LD E,A:ADC A,D:SUB E:LD D,A;7,4,4,4,4
ACNT     EXX:INC H:OUT H;4,4,12
         EXX;4 =257->128,5 (131)
pRAM_B   LD A,VPL;7         ;LD (BCGO),BC;20         LD BC,PW4;10INTB     OUT A;12BCGO     EQU $+1
INTBB    LD BC,0,A,(BC);10,7
pVOL_B   SUB 1:JP C,pEND_B;7,10         LD BC,AYregDAT;10INTb     OUT A;12INTbb    LD BC,(BCGO);20         LD A,L;4
pLOB_B   ADD A,#FF:LD L,A,A,C;7,4,4
pHIB_B   ADC A,1:LD C,A:ADC A,B:SUB C:LD B,A;7,4,4,4,4         LD (BCGO),BC;20
BCNT     EXX:INC H:OUT H;4,4,12 =187->94 (111)         LD A,3:DEC A:JR NZ,$-1;WAIT 22t
pRAM_C   LD A,VPL;7         LD BC,PW4;10INTC     OUT A;12INTCC    LD BC,AYregSEL;10
         LD A,(DE);7
pVOL_C   SUB 1:JP C,pEND_C;7,10         LD BC,AYregDAT;10INTc     OUT A;12INTcc    LD BC,AYregSEL;10
         LD A,L;4
pLOB_C   ADD A,#FF:LD L,A,A,E;7,4,4
pHIB_C   ADC A,2:LD E,A:ADC A,D:SUB E:LD D,A;7,4,4,4,4
CCNT     LD A,4:DEC A:JR NZ,$-1;WAIT 28t         JP PLAY_LP;10 =147->73,5 (102)

pEND_A   LD DE,#0000   ;This parts calls when sample ends
         LD A,4:DEC A:JR NZ,$-1
         JP ACNT

pEND_B   LD BC,#0000;10,4
         LD A,4:DEC A:JR NZ,$-1
         JP BCNT;10 (48)

pEND_C   LD DE,#0000
         LD A,4:DEC A:JR NZ,$-1
         JP CCNT
;--------SET_A    LD IY,(CURPL)              ;Subrotinue SET_
         LD A,(IY)                  ;using to set ALL
         CP 70:RET Z                ;params of channel
         OR A:JP Z,SETI_A
         CP 62:JP C,SETN_A
         OR A:JP Z,SETI_A
         SUB 62:JR NZ,SFLT_A        ;Setting effex data
         LD A,(IY+1):OR A:RET Z
         LD (TEMPOdmm),A
         RET
SFLT_A   DEC A:JR NZ,SVIB_A
         LD A,(IY+1)
         LD (pFDA_A+1),A
         LD (pFUA_A+1),A
         RET
SVIB_A   DEC A:JR NZ,SARP_A
         LD A,(IY+1)
         LD (pVBA1_A+1),A
         LD (pVBA2_A+1),A
         LD A,(IY+2)
         LD (pVBT_A+1),A
         RET
SARP_A   DEC A:JR NZ,SSTP_A
         LD A,(IY+1)
         LD (pAPA1_A+1),A
         LD (pAPA2_A+1),A
         LD A,(IY+2)
         LD (pAPT_A+1),A
         RET
SSTP_A   DEC A:JR NZ,SDBL_A
         LD A,(IY+1)
         LD (pSUA_A+1),A
         LD (pSDA_A+1),A
         LD A,(IY+2)
         LD (pSUT_A+1),A
         LD (pSDT_A+1),A
         RET
SDBL_A   DEC A:JR NZ,SATT_A
         LD A,(IY+1)
         LD (pDUT_A+1),A
         RET
SATT_A   LD IX,VTAB
         DEC A:JR NZ,SDCY_A
         LD A,(IY+1):AND #F:LD (SA1_A+2),A
SA1_A    LD A,(IX+0),(pATL_A+1),A
         LD A,(IY+2),(pATT_A+1),A
         RET
SDCY_A   DEC A:RET NZ
         LD A,(IY+1):AND #F:LD (SD1_A+2),A
SD1_A    LD A,(IX+0),(pDYL_A+1),A
         LD A,(IY+2),(pDYT_A+1),A
         RET

SETN_A   PUSH DE                    ;Setting note
         LD (NOTN_A),A              ;frequency
         SLA A
         LD E,A,D,0
         LD IX,NOTESdmm:ADD IX,DE
         POP DE
         LD A,(IX),(pLOB_A+1),A
         LD A,(IX+1),(pHIB_A+1),A
         XOR A
         LD (COUN_A),A
         LD (pVBF_A+1),A
         LD (pAPF_A+1),A
SETI_A   LD A,(IY+1)                ;Setting sample data
         AND #F0:JR Z,SETV_A
         PUSH DE
         LD E,A,D,0
         LD IX,INDAT-16:ADD IX,DE
         POP DE
         LD A,(IX)
         CP ".":JR Z,SETV_A
         EXX:LD E,(IX+9),D,(IX+10)
         EXX
         PUSH DE
         LD E,(IX+14)
         LD D,(IX+15),(pEND_A+1),DE
         LD A,(IX+11):AND %111:ADD A,VPL:LD (pRAM_A+1),A
         POP DE
SETV_A   LD A,(IY+1)                ;Setting volume
         AND #F:JR Z,SETF_A
         LD IX,VTAB
         LD (SV1_A+2),A
SV1_A    LD A,(IX+0),(pVOL_A+1),A
SETF_A   LD A,(IY+2)                ;Setting effex
         OR A
EFCOM_A  RET Z
         CP #A:JR NC,MIXI_A
         SLA A
         PUSH DE
         LD E,A,D,0
         LD IX,EFHT_A-2:ADD IX,DE
         LD E,(IX)
         LD D,(IX+1),(pEFA+1),DE
         LD E,(IY)
         LD D,(IY+1),(OLDT_A),DE
         POP DE
         LD A,#C3;JP
         LD (pEFA),A
         RET
MIXI_A   CP #F:JP Z,BLCK_A          ;Setting MIXS
         PUSH DE
         LD E,(IY)
         LD D,(IY+1),(OLDT_A),DE
         SUB #A
         SLA A,A
         LD E,A
         LD D,0
         LD IY,DDAT:ADD IY,DE
         EXX:LD (ADR_A),DE
         EXX         LD DE,(pEND_A+1),(END_A),DE
         LD A,(pLOB_A+1),E,A
         LD A,(pHIB_A+1),D,A
         LD (FQU_A),DE
         LD A,(pRAM_A+1),(RAM_A),A
         LD A,(pVOL_A+1),(VOL_A),A
         LD DE,MIX_A
         LD (pEFA+1),DE
         LD A,#C3;JP
         LD (pEFA),A
         POP DE
         LD A,(IY+3),(pMXT_A+1),A
         LD A,#C9;RET
         LD (EFCOM_A),A
         CALL SET_A+4
         LD A,#C8;RET Z
         LD (EFCOM_A),A
         RET
BLCK_A   PUSH DE                      ;Kills effex
         LD DE,#0118;JR +3
         LD (pEFA),DE
         POP DE
         RET

EFHT_A   DW FLU_A,FLD_A,VIB_A,ARP_A,STU_A,STD_A
         DW DOU_A,ATT_A,DCY_A,DCY_A ;Using to get
                                    ;effex subrotinue
                                    ;address
FLU_A    PUSH AF                    ;Float up
         LD A,(pLOB_A+1)
pFUA_A   ADD A,1
         LD (pLOB_A+1),A:JR NC,EXE_A
         LD A,(pHIB_A+1):INC A
         CP 12:LD (pHIB_A+1),A:JR Z,CLE_A
EXE_A    POP AF
         JP pEFA+3
CLE_A    PUSH DE
         LD DE,#0118;JR +3
         LD (pEFA),DE
         POP DE
         JR EXE_A

FLD_A    PUSH AF                      ;Float down
         LD A,(pLOB_A+1)
pFDA_A   SUB 1:LD (pLOB_A+1),A
         JR NC,EXE_A
         LD A,(pHIB_A+1)
         DEC A:CP 255:JR Z,CFD_A
         LD (pHIB_A+1),A
         JR EXE_A
CFD_A    XOR A
         LD (pLOB_A+1),A
         LD (pHIB_A+1),A
         JR CLE_A

VIB_A    PUSH AF                      ;Vibrato
         LD A,(COUN_A):INC A:LD (COUN_A),A
pVBT_A   CP 4:JR NZ,pVBF_A
         LD A,(pVBF_A+1):CPL:LD (pVBF_A+1),A
         XOR A
         LD (COUN_A),A
pVBF_A   LD A,0:OR A:JR Z,VB1_A
         LD A,(pLOB_A+1)
pVBA1_A  SUB 3:LD (pLOB_A+1),A:JR NC,EXE_A
         LD A,(pHIB_A+1):DEC A
         CP 255:JR Z,CFD_A
         LD (pHIB_A+1),A
         JR EXE_A
VB1_A    LD A,(pLOB_A+1)
pVBA2_A  ADD A,3
         LD (pLOB_A+1),A:JR NC,EXE_A
         LD A,(pHIB_A+1):INC A
         CP 12:LD (pHIB_A+1),A:JR Z,CLE_A
         JR EXE_A

ARP_A    PUSH AF                      ;ARPG
         LD A,(COUN_A):INC A:LD (COUN_A),A
pAPT_A   CP 1:JP NZ,EXE_A
         LD A,(pAPF_A+1):CPL:LD (pAPF_A+1),A
         XOR A
         LD (COUN_A),A
pAPF_A   LD A,0:OR A:LD A,(NOTN_A):JR Z,pAPA2_A
pAPA1_A  SUB #12
AP2_A    PUSH DE
         LD (NOTN_A),A
         SLA A
         LD E,A,D,0
         LD IX,NOTESdmm:ADD IX,DE
         POP DE
         LD A,(IX),(pLOB_A+1),A
         LD A,(IX+1),(pHIB_A+1),A
         JP EXE_A
pAPA2_A  ADD A,#12
         JR AP2_A

STU_A    PUSH AF                      ;Step up
         LD A,(COUN_A):INC A:JP Z,EXE_A
         LD (COUN_A),A
pSUT_A   CP 2:JP NZ,EXE_A
         LD A,(NOTN_A)
pSUA_A   ADD A,12
         JP AP2_A

STD_A    PUSH AF                      ;Step down
         LD A,(COUN_A):INC A:JP Z,EXE_A
         LD (COUN_A),A
pSDT_A   CP 2:JP NZ,EXE_A
         LD A,(NOTN_A)
pSDA_A   SUB 12
         JP AP2_A

DOU_A    PUSH AF                      ;Double
         LD A,(COUN_A):INC A:LD (COUN_A),A
pDUT_A   CP 3:JP NZ,EXE_A
         LD (HLSTO),HL
         LD HL,#FFFD:OR A:SBC HL,BC
         LD HL,(HLSTO):JR Z,$+3:EXX         CALL SET_A+4
         LD A,#18;JR
         LD (WHGO),A,A,NOEXX2-2-WHGO,(WHGO+1),A
         JP CLE_A

ATT_A    PUSH AF                      ;Attack
         LD A,(COUN_A):INC A:LD (COUN_A),A
pATT_A   CP 1:JP NZ,EXE_A
         XOR A
         LD (COUN_A),A
         LD A,(pVOL_A+1)
         DEC A
pATL_A   CP 1:JP C,CLE_A
         LD (pVOL_A+1),A
         JP EXE_A

DCY_A    PUSH AF                      ;Decay
         LD A,(COUN_A):INC A:LD (COUN_A),A
pDYT_A   CP 1:JP NZ,EXE_A
         XOR A
         LD (COUN_A),A
         LD A,(pVOL_A+1)
         INC A
pDYL_A   CP #F:JR Z,DY1_A
         JP NC,CLE_A
DY1_A    LD (pVOL_A+1),A
         JP EXE_A

MIX_A    PUSH AF                      ;MIX
         LD A,(COUN_A):INC A:LD (COUN_A),A
pMXT_A   CP 3:JP NZ,EXE_A
         LD (HLSTO),HL
         LD HL,#FFFD:OR A:SBC HL,BC
         LD HL,(HLSTO):JR Z,$+3:EXX

         LD IY,OLDT_A
         LD A,#C9
         LD (EFCOM_A),A
         CALL SET_A+4

         EXX
         LD (HLSTO),HL
         LD HL,(END_A),(pEND_A+1),HL
         LD HL,(FQU_A)
         LD A,L,(pLOB_A+1),A
         LD A,H,(pHIB_A+1),A
         LD A,(RAM_A),(pRAM_A+1),A
         LD A,(VOL_A),(pVOL_A+1),A

         EX DE,HL
         LD HL,(ADR_A)
         LD A,(pMXT_A+1)

MX0_A    ADD HL,DE:JR C,MX1_A
         DEC A:JR NZ,MX0_A
         EX DE,HL
         LD HL,(END_A)
         OR A:SBC HL,DE:JR NC,MX1_A+4
MX1_A    LD DE,(ADR_A)
         LD HL,(HLSTO)
         EXX

MX2_A    LD A,#C8,(EFCOM_A),A
         LD A,#18;JR
         LD (WHGO),A,A,NOEXX2-2-WHGO,(WHGO+1),A
         JP CLE_A
;--------;Handlers of B
SET_B    LD IY,(CURPL)
         LD A,(IY+3)
         CP 70:RET Z
         OR A:JP Z,SETI_B
         CP 62:JP C,SETN_B
         OR A:JP Z,SETI_B
         SUB 62:JR NZ,SFLT_B
         LD A,(IY+4):OR A:RET Z
         LD (TEMPOdmm),A
         RET
SFLT_B   DEC A:JR NZ,SVIB_B
         LD A,(IY+4)
         LD (pFDA_B+1),A
         LD (pFUA_B+1),A
         RET
SVIB_B   DEC A:JR NZ,SARP_B
         LD A,(IY+4)
         LD (pVBA1_B+1),A
         LD (pVBA2_B+1),A
         LD A,(IY+5)
         LD (pVBT_B+1),A
         RET
SARP_B   DEC A:JR NZ,SSTP_B
         LD A,(IY+4)
         LD (pAPA1_B+1),A
         LD (pAPA2_B+1),A
         LD A,(IY+5)
         LD (pAPT_B+1),A
         RET
SSTP_B   DEC A:JR NZ,SDBL_B
         LD A,(IY+4)
         LD (pSUA_B+1),A
         LD (pSDA_B+1),A
         LD A,(IY+5)
         LD (pSUT_B+1),A
         LD (pSDT_B+1),A
         RET
SDBL_B   DEC A:JR NZ,SATT_B
         LD A,(IY+4)
         LD (pDUT_B+1),A
         RET
SATT_B   LD IX,VTAB
         DEC A:JR NZ,SDCY_B
         LD A,(IY+4):AND #F:LD (SA1_B+2),A
SA1_B    LD A,(IX+0),(pATL_B+1),A
         LD A,(IY+5),(pATT_B+1),A
         RET
SDCY_B   DEC A:RET NZ
         LD A,(IY+4):AND #F:LD (SD1_B+2),A
SD1_B    LD A,(IX+0),(pDYL_B+1),A
         LD A,(IY+5),(pDYT_B+1),A
         RET
SETN_B   PUSH DE
         LD (NOTN_B),A
         SLA A
         LD E,A
         LD D,0
         LD IX,NOTESdmm:ADD IX,DE
         POP DE
         LD A,(IX),(pLOB_B+1),A
         LD A,(IX+1),(pHIB_B+1),A
         XOR A
         LD (COUN_B),A
         LD (pVBF_B+1),A
         LD (pAPF_B+1),A
SETI_B   LD A,(IY+4)
         AND #F0:JR Z,SETV_B
         PUSH DE
         LD E,A
         LD D,0
         LD IX,INDAT-16:ADD IX,DE
         POP DE
         LD A,(IX)
         CP ".":JR Z,SETV_B
         EXX
         LD C,(IX+9)
         LD B,(IX+10)
         EXX
         PUSH DE
         LD E,(IX+14)
         LD D,(IX+15)
         LD (pEND_B+1),DE
         LD A,(IX+11):AND %111:ADD A,VPL:LD (pRAM_B+1),A
         POP DE
SETV_B   LD A,(IY+4)
         AND #F:JR Z,SETF_B
         LD IX,VTAB
         LD (SV1_B+2),A
SV1_B    LD A,(IX+0),(pVOL_B+1),A
SETF_B   LD A,(IY+5)
         OR A
EFCOM_B  RET Z
         CP #A:JR NC,MIXI_B
         SLA A
         PUSH DE
         LD E,A
         LD D,0
         LD IX,EFHT_B-2:ADD IX,DE
         LD E,(IX)
         LD D,(IX+1),(pEFB+1),DE
         LD E,(IY+3)
         LD D,(IY+4),(OLDT_B),DE
         POP DE
         LD A,#C3
         LD (pEFB),A
         RET
MIXI_B   CP #F:JP Z,BLCK_B
         PUSH DE
         LD E,(IY+3)
         LD D,(IY+4),(OLDT_B),DE
         SUB #A
         SLA A,A
         LD E,A
         LD D,0
         LD IY,DDAT-3:ADD IY,DE
         EXX:LD (ADR_B),BC
         EXX
         LD DE,(pEND_B+1)
         LD (END_B),DE
         LD A,(pLOB_B+1),E,A
         LD A,(pHIB_B+1),D,A
         LD (FQU_B),DE
         LD A,(pRAM_B+1),(RAM_B),A
         LD A,(pVOL_B+1),(VOL_B),A
         LD DE,MIX_B
         LD (pEFB+1),DE
         LD A,#C3,(pEFB),A
         POP DE
         LD A,(IY+6)
         LD (pMXT_B+1),A
         LD A,#C9,(EFCOM_B),A
         CALL SET_B+4
         LD A,#C8,(EFCOM_B),A
         RET
BLCK_B   PUSH DE
         LD DE,#0118
         LD (pEFB),DE
         POP DE
         RET

EFHT_B   DW FLU_B,FLD_B,VIB_B,ARP_B,STU_B,STD_B
         DW DOU_B,ATT_B,DCY_B,DCY_B

FLU_B    PUSH AF
         LD A,(pLOB_B+1)
pFUA_B   ADD A,1
         LD (pLOB_B+1),A
         JR NC,EXE_B
         LD A,(pHIB_B+1)
         INC A:CP 12:LD (pHIB_B+1),A:JR Z,CLE_B
EXE_B    POP AF
         JP pEFB+3
CLE_B    PUSH DE:LD DE,#0118:LD (pEFB),DE
         POP DE
         JR EXE_B

FLD_B    PUSH AF
         LD A,(pLOB_B+1)
pFDA_B   SUB 1
         LD (pLOB_B+1),A:JR NC,EXE_B
         LD A,(pHIB_B+1)
         DEC A:CP 255:JR Z,CFD_B
         LD (pHIB_B+1),A
         JR EXE_B
CFD_B    XOR A
         LD (pLOB_B+1),A
         LD (pHIB_B+1),A
         JR CLE_B

VIB_B    PUSH AF
         LD A,(COUN_B):INC A:LD (COUN_B),A
pVBT_B   CP 4:JR NZ,pVBF_B
         LD A,(pVBF_B+1):CPL:LD (pVBF_B+1),A
         XOR A
         LD (COUN_B),A
pVBF_B   LD A,0:OR A:JR Z,VB1_B
         LD A,(pLOB_B+1)
pVBA1_B  SUB 3
         LD (pLOB_B+1),A
         JR NC,EXE_B
         LD A,(pHIB_B+1)
         DEC A:CP 255:JR Z,CFD_B
         LD (pHIB_B+1),A
         JR EXE_B
VB1_B    LD A,(pLOB_B+1)
pVBA2_B  ADD A,3
         LD (pLOB_B+1),A
         JR NC,EXE_B
         LD A,(pHIB_B+1)
         INC A
         CP 12
         LD (pHIB_B+1),A
         JR Z,CLE_B
         JR EXE_B

ARP_B    PUSH AF
         LD A,(COUN_B):INC A:LD (COUN_B),A
pAPT_B   CP 1:JR NZ,pAPF_B
         LD A,(pAPF_B+1)
         CPL         LD (pAPF_B+1),A
         XOR A
         LD (COUN_B),A
pAPF_B   LD A,0:OR A:LD A,(NOTN_B):JR Z,pAPA2_B
pAPA1_B  SUB #12
AP2_B    PUSH DE
         LD (NOTN_B),A
         SLA A
         LD E,A
         LD D,0
         LD IX,NOTESdmm:ADD IX,DE
         POP DE
         LD A,(IX),(pLOB_B+1),A
         LD A,(IX+1),(pHIB_B+1),A
         JP EXE_B
pAPA2_B  ADD A,#12
         JR AP2_B

STU_B    PUSH AF
         LD A,(COUN_B):INC A
         JP Z,EXE_B
         LD (COUN_B),A
pSUT_B   CP 2:JP NZ,EXE_B
         LD A,(NOTN_B)
pSUA_B   ADD A,12
         JP AP2_B

STD_B    PUSH AF
         LD A,(COUN_B):INC A:JP Z,EXE_B
         LD (COUN_B),A
pSDT_B   CP 2:JP NZ,EXE_B
         LD A,(NOTN_B)
pSDA_B   SUB 12
         JP AP2_B

DOU_B    PUSH AF         LD A,(COUN_B):INC A:LD (COUN_B),A
pDUT_B   CP 3:JP NZ,EXE_B
         LD (HLSTO),HL
         LD HL,#FFFD:OR A:SBC HL,BC
         LD HL,(HLSTO):JR Z,$+3:EXX
         CALL SET_B+4
         LD A,#18;JR
         LD (WHGO),A,A,NOEXX2-2-WHGO,(WHGO+1),A
         JP CLE_B

ATT_B    PUSH AF
         LD A,(COUN_B):INC A:LD (COUN_B),A
pATT_B   CP 1:JP NZ,EXE_B
         XOR A
         LD (COUN_B),A
         LD A,(pVOL_B+1)
         DEC A
pATL_B   CP 1:JP C,CLE_B
         LD (pVOL_B+1),A
         JP EXE_B

DCY_B    PUSH AF
         LD A,(COUN_B):INC A:LD (COUN_B),A
pDYT_B   CP 1:JP NZ,EXE_B
         XOR A
         LD (COUN_B),A         LD A,(pVOL_B+1):INC A
pDYL_B   CP #F:JR Z,DY1_B
         JP NC,CLE_B
DY1_B    LD (pVOL_B+1),A
         JP EXE_B

MIX_B    PUSH AF
         LD A,(COUN_B):INC A:LD (COUN_B),A
pMXT_B   CP 3:JP NZ,EXE_B
         LD (HLSTO),HL
         LD HL,#FFFD:OR A:SBC HL,BC
         LD HL,(HLSTO):JR Z,$+3:EXX

         LD IY,OLDT_B
         LD A,#C9
         LD (EFCOM_B),A
         CALL SET_B+4

         EXX
         LD (HLSTO),HL
         LD HL,(END_B)
         LD (pEND_B+1),HL
         LD HL,(FQU_B)
         LD A,L,(pLOB_B+1),A
         LD A,H,(pHIB_B+1),A
         LD A,(RAM_B),(pRAM_B+1),A
         LD A,(VOL_B),(pVOL_B+1),A

         LD C,L
         LD B,H
         LD HL,(ADR_B)
         LD A,(pMXT_B+1)

MX0_B    ADD HL,BC:JR C,MX1_B
         DEC A:JR NZ,MX0_B
         LD C,L
         LD B,H
         LD HL,(END_B)
         OR A:SBC HL,BC:JR NC,MX1_B+4
MX1_B    LD BC,(ADR_B)
         LD HL,(HLSTO)
         EXX

MX2_B    LD A,#C8
         LD (EFCOM_B),A
         LD A,#18;JR
         LD (WHGO),A,A,NOEXX2-2-WHGO,(WHGO+1),A
         JP CLE_B
;-------
;Handlers of C
SET_C    LD IY,(CURPL)
         LD A,(IY+6)
         CP 70:RET Z
         OR A:JP Z,SETI_C
         CP 62:JP C,SETN_C
         OR A:JP Z,SETI_C
         SUB 62:JR NZ,SFLT_C
         LD A,(IY+7)
         OR A:RET Z
         LD (TEMPOdmm),A
         RET
SFLT_C   DEC A:JR NZ,SVIB_C
         LD A,(IY+7)
         LD (pFDA_C+1),A
         LD (pFUA_C+1),A
         RET
SVIB_C   DEC A:JR NZ,SARP_C
         LD A,(IY+7)
         LD (pVBA1_C+1),A
         LD (pVBA2_C+1),A
         LD A,(IY+8)
         LD (pVBT_C+1),A
         RET
SARP_C   DEC A:JR NZ,SSTP_C
         LD A,(IY+7)
         LD (pAPA1_C+1),A
         LD (pAPA2_C+1),A
         LD A,(IY+8)
         LD (pAPT_C+1),A
         RET
SSTP_C   DEC A:JR NZ,SDBL_C
         LD A,(IY+7)
         LD (pSUA_C+1),A
         LD (pSDA_C+1),A
         LD A,(IY+8)
         LD (pSUT_C+1),A
         LD (pSDT_C+1),A
         RET
SDBL_C   DEC A:JR NZ,SATT_C
         LD A,(IY+7)
         LD (pDUT_C+1),A
         RET
SATT_C   LD IX,VTAB
         DEC A:JR NZ,SDCY_C
         LD A,(IY+8)
         AND #F
         LD (SA1_C+2),A
SA1_C    LD A,(IX+0),(pATL_C+1),A
         LD A,(IY+8),(pATT_C+1),A
         RET
SDCY_C   DEC A:RET NZ
         LD A,(IY+7)
         AND #F
         LD (SD1_C+2),A
SD1_C    LD A,(IX+0),(pDYL_C+1),A
         LD A,(IY+8),(pDYT_C+1),A
         RET
SETN_C   PUSH DE
         LD (NOTN_C),A
         SLA A
         LD E,A
         LD D,0
         LD IX,NOTESdmm:ADD IX,DE
         POP DE
         LD A,(IX),(pLOB_C+1),A
         LD A,(IX+1),(pHIB_C+1),A
         XOR A
         LD (COUN_C),A
         LD (pVBF_C+1),A
         LD (pAPF_C+1),A
SETI_C   LD A,(IY+7)
         AND #F0:JR Z,SETV_C
         PUSH DE
         LD E,A
         LD D,0
         LD IX,INDAT-16:ADD IX,DE
         POP DE
         LD A,(IX)
         CP ".":JR Z,SETV_C
         LD E,(IX+9)
         LD D,(IX+10)
         PUSH DE
         LD E,(IX+14)
         LD D,(IX+15)
         LD (pEND_C+1),DE
         LD A,(IX+11):AND %111:ADD A,VPL:LD (pRAM_C+1),A
         POP DE
SETV_C   LD A,(IY+7)
         AND #F:JR Z,SETF_C
         LD IX,VTAB
         LD (SV1_C+2),A
SV1_C    LD A,(IX+0),(pVOL_C+1),A
SETF_C   LD A,(IY+8)
         OR A
EFCOM_C  RET Z
         CP #A:JR NC,MIXI_C
         SLA A
         PUSH DE
         LD E,A
         LD D,0
         LD IX,EFHT_C-2:ADD IX,DE
         LD E,(IX)
         LD D,(IX+1)
         LD (pEFC+1),DE
         LD E,(IY+6)
         LD D,(IY+7)
         LD (OLDT_C),DE
         POP DE
         LD A,#C3
         LD (pEFC),A
         RET
MIXI_C   CP #F:JP Z,BLCK_C
         PUSH DE
         LD E,(IY+6)
         LD D,(IY+7)
         LD (OLDT_C),DE
         SUB #A
         SLA A,A
         LD E,A
         LD D,0
         LD IY,DDAT-6:ADD IY,DE
         POP DE
         LD (ADR_C),DE
         PUSH DE
         LD DE,(pEND_C+1),(END_C),DE
         LD A,(pLOB_C+1),E,A
         LD A,(pHIB_C+1),D,A
         LD (FQU_C),DE
         LD A,(pRAM_C+1),(RAM_C),A
         LD A,(pVOL_C+1),(VOL_C),A
         LD DE,MIX_C
         LD (pEFC+1),DE
         LD A,#C3
         LD (pEFC),A
         POP DE
         LD A,(IY+9)
         LD (pMXT_C+1),A
         LD A,#C9
         LD (EFCOM_C),A
         CALL SET_C+4
         LD A,#C8
         LD (EFCOM_C),A
         RET
BLCK_C   PUSH DE
         LD DE,#0118
         LD (pEFC),DE
         POP DE
         RET

EFHT_C   DW FLU_C,FLD_C,VIB_C,ARP_C,STU_C,STD_C
         DW DOU_C,ATT_C,DCY_C,DCY_C

FLU_C    PUSH AF
         LD A,(pLOB_C+1)
pFUA_C   ADD A,1:LD (pLOB_C+1),A
         JR NC,EXE_C
         LD A,(pHIB_C+1)
         INC A
         CP 12
         LD (pHIB_C+1),A
         JR Z,CLE_C
EXE_C    POP AF
         JP pEFC+3
CLE_C    PUSH DE
         LD DE,#0118
         LD (pEFC),DE
         POP DE
         JR EXE_C

FLD_C    PUSH AF
         LD A,(pLOB_C+1)
pFDA_C   SUB 1:LD (pLOB_C+1),A:JR NC,EXE_C
         LD A,(pHIB_C+1)
         DEC A
         CP 255:JR Z,CFD_C
         LD (pHIB_C+1),A
         JR EXE_C
CFD_C    XOR A
         LD (pLOB_C+1),A
         LD (pHIB_C+1),A
         JR CLE_C

VIB_C    PUSH AF
         LD A,(COUN_C):INC A:LD (COUN_C),A
pVBT_C   CP 4:JR NZ,pVBF_C
         LD A,(pVBF_C+1)
         CPL
         LD (pVBF_C+1),A
         XOR A
         LD (COUN_C),A
pVBF_C   LD A,0:OR A:JR Z,VB1_C
         LD A,(pLOB_C+1)
pVBA1_C  SUB 3:LD (pLOB_C+1),A:JR NC,EXE_C
         LD A,(pHIB_C+1):DEC A:CP 255:JR Z,CFD_C
         LD (pHIB_C+1),A
         JR EXE_C
VB1_C    LD A,(pLOB_C+1)
pVBA2_C  ADD A,3:LD (pLOB_C+1),A:JR NC,EXE_C
         LD A,(pHIB_C+1):INC A:CP 12
         LD (pHIB_C+1),A:JR Z,CLE_C
         JR EXE_C

ARP_C    PUSH AF
         LD A,(COUN_C):INC A:LD (COUN_C),A
pAPT_C   CP 1:JP NZ,EXE_C
         LD A,(pAPF_C+1):CPL:LD (pAPF_C+1),A
         XOR A
         LD (COUN_C),A
pAPF_C   LD A,0:OR A:LD A,(NOTN_C):JR Z,pAPA2_C
pAPA1_C  SUB #12
AP2_C    PUSH DE
         LD (NOTN_C),A
         SLA A
         LD E,A
         LD D,0
         LD IX,NOTESdmm:ADD IX,DE
         POP DE
         LD A,(IX),(pLOB_C+1),A
         LD A,(IX+1),(pHIB_C+1),A
         JP EXE_C
pAPA2_C  ADD A,#12
         JR AP2_C

STU_C    PUSH AF
         LD A,(COUN_C):INC A:JP Z,EXE_C
         LD (COUN_C),A
pSUT_C   CP 2:JP NZ,EXE_C
         LD A,(NOTN_C)
pSUA_C   ADD A,12
         JP AP2_C

STD_C    PUSH AF
         LD A,(COUN_C):INC A:JP Z,EXE_C
         LD (COUN_C),A
pSDT_C   CP 2:JP NZ,EXE_C
         LD A,(NOTN_C)
pSDA_C   SUB 12
         JP AP2_C

DOU_C    PUSH AF
         LD A,(COUN_C):INC A:LD (COUN_C),A
pDUT_C   CP 3:JP NZ,EXE_C
         LD (HLSTO),HL
         LD HL,#FFFD:OR A:SBC HL,BC
         LD HL,(HLSTO):JR Z,$+3:EXX
         CALL SET_C+4
         LD A,#18;JR
         LD (WHGO),A,A,NOEXX2-2-WHGO,(WHGO+1),A
         JP CLE_C

ATT_C    PUSH AF
         LD A,(COUN_C):INC A:LD (COUN_C),A
pATT_C   CP 1:JP NZ,EXE_C
         XOR A
         LD (COUN_C),A
         LD A,(pVOL_C+1)
         DEC A
pATL_C   CP 1:JP C,CLE_C
         LD (pVOL_C+1),A
         JP EXE_C

DCY_C    PUSH AF
         LD A,(COUN_C)
         INC A
         LD (COUN_C),A
pDYT_C   CP 1:JP NZ,EXE_C
         XOR A
         LD (COUN_C),A
         LD A,(pVOL_C+1)
         INC A
pDYL_C   CP #F:JR Z,DY1_C
         JP NC,CLE_C
DY1_C    LD (pVOL_C+1),A
         JP EXE_C

MIX_C    PUSH AF
         LD A,(COUN_C):INC A:LD (COUN_C),A
pMXT_C   CP 3:JP NZ,EXE_C
         LD (HLSTO),HL
         LD HL,#FFFD:OR A:SBC HL,BC
         LD HL,(HLSTO):JR Z,$+3:EXX

         LD IY,OLDT_C
         LD A,#C9
         LD (EFCOM_C),A
         CALL SET_C+4

         LD (HLSTO),HL
         LD HL,(END_C)
         LD (pEND_C+1),HL
         LD HL,(FQU_C)
         LD A,L,(pLOB_C+1),A
         LD A,H,(pHIB_C+1),A
         LD A,(RAM_C),(pRAM_C+1),A         LD A,(VOL_C),(pVOL_C+1),A

         EX DE,HL
         LD HL,(ADR_C)
         LD A,(pMXT_C+1)

MX0_C    ADD HL,DE:JR C,MX1_C
         DEC A:JR NZ,MX0_C
         EX DE,HL
         LD HL,(END_C)
         AND A:SBC HL,DE:JR NC,MX1_C+4
MX1_C    LD DE,(ADR_C)
         LD HL,(HLSTO)

MX2_C    LD A,#C8
         LD (EFCOM_C),A
         LD A,#18;JR
         LD (WHGO),A,A,NOEXX2-2-WHGO,(WHGO+1),A
         JP CLE_C
;--------
COUN_A   DB 0
NOTN_A   DW 0
OLDT_A   DB 20,#F1,0
COUN_B   DB 0
NOTN_B   DW 0
OLDT_B   DB 20,#F1,0
COUN_C   DB 0
NOTN_C   DW 0
OLDT_C   DB 20,#F1,0

;DATA
ZEROES   DW #A1A1,#A1A1,#A1A1,#A1A1,#A1A1,#A1A1,#A1A1,#A1A1
         DW 0,0,0,0,0,0,0  ;This descriptor used
                           ;when plays silence

VDTAB    DB 0,0,1,3,7,15,31,63,127
ST2TB    DB 1,1,2,4,8,16,32,64,128

NOTESdmm DW 0
         DW 44,47,50,53,56,59,63,66,70,74,79,83
         DW 88,94,99,105,111,118,125,133,140,149,158,167
         DW 177,187,199,210,223,236,250,265,281,297,315,334
         DW 354,375,397,421,446,472,500,530,561,595,630,668
         DW 707,749,794,841,891,944,1001,1060,1123,1189,1216,1335
         DW 0 ;Notes coeffs

VTAB     DB #F,#F,#E,#D,#C,#B,#A,#9,#8,#7,#6,#5,#4,#3,#2         DB #1                        ;Used to get volume
;--------CURPL    DW 0

HLSTO    DW 0
;FLAG     DB 0
;HLaSTO   DW 0
;DESTO    DW 0
OLDP     DW 0
;TCOU     DB 0

;EXFLAG   DB 0
;A_ST     DB 0
;END_P1   DB 0PATTAB   DS 50
PSB      DW 0
PLPOS    DB 0
PLPAT    DB 0
ADR_A    DW 0
END_A    DW 0
FQU_A    DW 0
VOL_A    DB 0
RAM_A    DB 0
ADR_B    DW 0
END_B    DW 0
FQU_B    DW 0
VOL_B    DB 0
RAM_B    DB 0
ADR_C    DW 0
END_C    DW 0
FQU_C    DW 0
VOL_C    DB 0
RAM_C    DB 0
;Pages
BKTAB    DB 0,1,3,4,6,7;#50,#51,#53,#54,#56,#57SAVEHL   DS 2;---------------------------------------