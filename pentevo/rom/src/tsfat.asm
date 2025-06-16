
;-------
DEFREQ  EQU %00000010

SYC     EQU #20AF
PW2     EQU #12AF
;-------
PG0     EQU #F7
;-------
BUFZZ   EQU #8000
PWA     EQU BUFZZ+#4000
;-------
GENBU   EQU BUFZZ+#0000
GENBE   EQU BUFZZ+#1000

SECBU   EQU GENBE
SECBE   EQU SECBU+512
LOBU    EQU SECBE
LOBE    EQU LOBU+512

LoCALL  EQU LOBE+512
INTC    EQU BUFZZ+#1F00-1
VALS    EQU BUFZZ+#2000
;---------------------------------------
STST    EQU VALS; Stream Status (0-notR)

NSDC    EQU STST+1;\ Sec num in Cluster
EOC     EQU NSDC+1;/ Flag (EndOfChain)

ABT     EQU EOC+1

BZN     EQU ABT+1;Blocks
NR0     EQU BZN+1

CAHL    EQU NR0+2
CADE    EQU CAHL+2

LSTSE   EQU CADE+2

REZDE   EQU LSTSE+4

PR      EQU REZDE+2

CLHL    EQU PR+4
CLDE    EQU CLHL+2

LLHL    EQU CLDE+2

LTHL    EQU LLHL+4;LAST
LTDE    EQU LTHL+2
;------------Entry Pattern:-------------
ENTRY   EQU LTDE+2;DS 11
EFLG    EQU ENTRY+11
CLSDE   EQU ENTRY+20
CLSHL   EQU ENTRY+26
SIZIK   EQU ENTRY+28
;---------------------------------------
;------------FAT PARAMETERS:------------
BREZS   EQU ENTRY+33;           [+14(2)]
FSINF   EQU BREZS+2;   [+48(2)]+[ADDTOP]

BFATS   EQU FSINF+4
BFTSZ   EQU BFATS+1
BSECPC  EQU BFTSZ+4
BROOTC  EQU BSECPC+2

FSTFRC  EQU BROOTC+4

ADDTOP  EQU FSTFRC+4

SFAT    EQU ADDTOP+4
SDFAT   EQU SFAT+2

CUHL    EQU SDFAT+4
CUDE    EQU CUHL+2

NXDE    EQU CUDE+2
NXHL    EQU NXDE+2

LDHL    EQU NXHL+2;ADDRESS COPY

COUNT   EQU LDHL+2
DUHL    EQU COUNT+1
DUDE    EQU DUHL+2

DUBA    EQU DUDE+2
UUHL    EQU DUBA+1
BUHL    EQU UUHL+2
CLCNT   EQU BUHL+2; COUNTER USED in MKSG

FCTS    EQU CLCNT+4; FirstClusTS
BUTS    EQU FCTS+4;  BUferTS

LSTCAT  EQU BUTS+4;  Active DIR
RECCAT  EQU LSTCAT+4;RECYCLED BIN DIR
;-------
BLKNUM  EQU RECCAT+4
DRVRE   EQU BLKNUM+4
STRMED  EQU DRVRE+1
;---------------------------------------

;       INCL "STREAM"
;---------------------------------------
E_ENF   EQU 1; entry not found
E_NES   EQU 2; not enough space

E_DNF   EQU 8; device not found
E_DNR   EQU 9; device not ready
E_PNF   EQU 10;partition not found

E_WRP   EQU 16;write protection

E_WGS   EQU 24;wrong stream
E_SNR   EQU 25;stream not ready
;---------------------------------------
        DI

        EXX
        LD D,A
        LD BC,SYC:LD A,DEFREQ:OUT (C),A
        LD B,HIGH PW2:IN A,(C)
        LD E,PG0:OUT (C),E
        LD (PGR),A

        LD (SPBU),SP
        LD SP,PWA

        LD HL,FUX
        LD B,0,C,D
        ADD HL,BC
        ADD HL,BC
        LD A,(HL):INC HL
        LD H,(HL),L,A
        LD BC,RORO:PUSH BC
        PUSH HL
        EXX:EXA
        RET

RORO    LD SP,(SPBU)
        LD BC,PW2,A,(PGR):OUT (C),A
FEX     RET
;-------
DIHALT  PUSH HL
        LD A,I
        PUSH AF
        LD HL,I_N_T
        LD (INTC),HL
        LD A,HIGH INTC
        LD I,A
        IM 2

        EI:HALT:DI:DJNZ $-3

        POP AF
        LD I,A:CP 64:JR NC,$+4:IM 1
        POP HL
        RET

I_N_T   EI:RET
;---------------------------------------
FUX     DW LOAD512;0;   READ512
        DW SAVE512;1;   WRITE512
        DW GFILE;  2;   SEEK0
        DW FEX;    3;   SEEK512
        DW LOADNON;4;   SKIP512
      DUP 11
        DW FEX
      EDUP
;-------
        DW FENTRY; 16;  FENTRY
        DW GDIR;   17;  SETDIR
        DW MKfile; 18;  MKFILE
        DW MKdir;  19;  MKDIR
        DW FNDELFL;20;  DELFILE
        DW GROOT;  21;  SETROOT
      DUP 10
        DW FEX
      EDUP
;-------
        DW STREAM; 32;  CRTSTREAM
        DW STREAMS;33;  SELSTREAM
        DW STREAMD;34;  DELSTREAM
;---------------------------------------
STREAM  ;i: B - Device
;               (0 - SD, 1/2 - HDD M/S)
;           C - Partition

;        o: C - Stream Number

        PUSH BC:CALL STRMCLR
        POP BC:RET NZ

        LD A,B
TSBC    CP 3:JR NC,EWGS
        OR A:JR Z,BDSD
        DEC A:JR Z,BDSD+2
        DEC A:RET NZ
        CALL DOS_SWP
        LD A,1
        JR BDIS
BDSD    LD A,1:CALL DOS_SWP
        CALL IDE_INI
        XOR A
BDIS    CALL SEL_DEV:JR NZ,EDNF
        CALL HDD:JR NZ,EPNF

        LD HL,0,(CRRR),HL,(CRRR+2),HL

        LD A,1,(STST),A;Stream Ready
        XOR A:LD C,A
        RET
EWGS    LD A,E_WGS:OR A:RET
EDNF    LD A,E_DNF:OR A:RET
EPNF    LD A,E_PNF:OR A:RET

STREAMS RET
STREAMD RET

GROOT   LD HL,0,(CRRR),HL,(CRRR+2),HL
        JR GDIR
;-------
STRMCLR LD HL,VALS,DE,HL:INC DE
        LD (HL),0,BC,STRMED-VALS-1:LDIR

        XOR A
        RET
;---------------------------------------
FENTRY  CALL SRHDRN:JR Z,EENF
        LD (CRRR),HL
        LD (CRRR+2),DE
        EXX
        CALL GFILE
        LD HL,(ENTRY+28)
        LD DE,(ENTRY+30)

        XOR A
        RET
EENF    LD A,E_ENF:OR A:RET
;-------
GFILE   LD HL,CRRR:JP GIPAG
GDIR    LD HL,CRRR:JP GLSTCAT

MKfile  CALL CRTM
        CALL MKFILE:JR NZ,ENES

        LD (CRRR),HL,(CRRR+2),DE

        PUSH HL,DE:CALL GFILE
        POP DE,HL

        XOR A
        RET
ENES    LD A,E_NES:OR A:RET
;-------
MKdir   CALL CRTM
        CALL MKDIR:JR NZ,ENES
        RET
;-------
CRTM    PUSH HL,DE,BC:CALL TMTOEN
        POP BC,DE,HL
        RET
;-------
FNDELFL ;i:HL - Flag(1),Name(1-12),0

        CALL SRHDRN:JR Z,EENF
        LD (CRRR),HL
        LD (CRRR+2),DE
        LD A,#E5,(BC),A
        CALL STAMP

        LD HL,CRRR:CALL DLSG
        XOR A
        RET
;---------------------------------------
TMTOEN  LD D,0
        LD A,2:CALL SEDT
        LD E,A
        SLA E:RL D
        SLA E:RL D
        SLA E:RL D
        SLA E:RL D
        SLA E:RL D
        XOR A:CALL SEDT:SRL A
        OR E:LD E,A
        LD A,4:CALL SEDT
        SLA A
        SLA A
        SLA A
        OR D:LD D,A
        LD (ENTRY+14),DE
        LD (ENTRY+22),DE

        LD A,9:CALL SEDT:ADD A,20
        LD D,A
        LD A,8:CALL SEDT
        SLA A
        SLA A
        SLA A
        SLA A
        SLA A
        RL D:LD E,A
        LD A,7:CALL SEDT
        OR E:LD E,A
        LD (ENTRY+16),DE
        LD (ENTRY+18),DE
        LD (ENTRY+24),DE
        RET

SEDT    LD BC,#DFF7:OUT (C),A
        LD B,#BF:IN A,(C)

        LD C,A:AND %00001111:LD B,A
        SRL C
        SRL C
        SRL C
        SRL C
        LD A,C
      DUP 9
        ADD A,C
      EDUP
        ADD A,B
        RET
;---------------------------------------
DOS_SWP ;0 - IDE, 1 - SD

        LD HL,HDRE
        OR A:JR Z,$+5:LD HL,SDRE

        LD DE,IDE_INI,BC,4*3:LDIR
        RET
;-------
SDRE    JP INIsd
        JP SELsd
        JP RDDSEsd
        JP SDDSEsd
;-------
HDRE    RET:RET:RET
        JP SELide
        JP RDDSEhd
        JP SDDSEhd
;---------------------------------------
;       !INCL "STREAM"

;---------------------------------------
GENTRYX ;i:HL - Text String
;        o:ENTRY(11)

        LD DE,ENTRY
        LD B,8:CALL DOTZ,RYX,Z,NOPIDE
        LD A,(HL):CP ".":JR NZ,$+3:INC HL
        LD B,3:CALL RYX,Z,NOPIDE
        RET

RYX     LD A,(HL):OR A:RET Z
        CP ".":INC HL:RET Z
        LD (DE),A:INC DE:DJNZ RYX
        LD A,1:OR A
        RET

DOTZ    LD A,(HL):CP ".":RET NZ
        LD (DE),A:INC HL,DE:DEC B
        LD A,(HL):CP ".":RET NZ
        LD (DE),A:INC HL,DE:DEC B
        RET

;---------------------------------------
;GENTRY  ;i:HL(32) -> ENTRY
;LD DE,ENTRY,BC,32:LDIR
;RET

;TENTRY  ;i:DE(32) <- ENTRY
;LD HL,ENTRY,BC,32:LDIR
;RET

;---------------------------------------
;GIVE/TAKE Active DIR:
GLSTCAT ;i:HL(4) - CLUS num
        LD DE,LSTCAT,BC,4:LDIR
        RET

TLSTCAT ;i:DE(4) - Addres of value
        LD HL,LSTCAT,BC,4:LDIR
        RET

;---------------------------------------
;READ SECTORS WITH POSITIONING:
;RDD     ;i:[DE,HL] - SEC number
;               BC - Address
;                A - number
;PUSH BC
;CALL XPOZI
;POP HL
;JP RDDSE

;---------------------------------------
;;SAVE SECTORS WITH POSITIONING:
;SDD     ;i:[DE,HL] - SEC number
;               BC - Address
;                A - number
;PUSH BC
;CALL XPOZI
;POP HL
;JP SDDSE

;---------------------------------------
;SKIP DATA from FAT32:
LOADNON LD A,3,(LDMD),A:JP LOAD

;---------------------------------------
;LOAD DATA to FAT32 (STREAM):
LOAD512 LD (PGO),BC:EX DE,HL
        LD A,H:AND #3F:LD H,A
        LD A,1,(LDMD),A:CALL LOAD
        LD BC,(PGO):EX DE,HL
        RET
;-------
Load512 LD A,1,(LDMD),A:JP LOAD

;---------------------------------------
;SAVE DATA to FAT32 (STREAM):
SAVE512 LD (PGO),BC:EX DE,HL
        LD A,H:AND #3F:LD H,A
        LD A,2,(LDMD),A:CALL LOAD
        LD BC,(PGO):EX DE,HL
        RET

;---------------------------------------
;READ DATA from FAT32 (STREAM):
LOAD    ;i:HL - Address + (PGO)
;           B - lenght (512b blocks)
;     CUHL(4) - CLUSnum (if EOC+NSDC=0)

;        o:HL - New value + (PGO)
;           A - EndOfChain

        XOR A:LD (ABT),A
        CALL LPREX:JR NZ,RH

        LD A,B:CALL NEWCLA

RH      LD HL,(LDHL)
        LD A,(EOC)
        RET

;Positioning to Cluster, if needed:
LPREX   LD (LDHL),HL
        LD A,(EOC):OR A:RET NZ
        LD A,(NSDC):OR A:JR NZ,RX

        PUSH BC:LD HL,CUHL:CALL GIPAG
        POP BC
        RET
RX      XOR A
        RET
;-------
NEWCLA  LD (BZN),A

NXTC    LD HL,(LTHL),DE,(LTDE):CALL PROZ

        LD HL,BZN
        LD A,(BSECPC),BC,(NSDC)
        SUB C:LD B,A
        LD A,(HL):OR A:RET Z
        SUB B:JR NC,KN
        ADD A,B:LD B,A:XOR A
KN      LD (HL),A

        LD A,B,(NR0),A
        LD HL,(LDHL)
        CALL RDDXX;       READ Sector(s)
        LD (LDHL),HL;   updating Address

        LD HL,LTHL,DE,LLHL,BC,4:LDIR

        LD HL,(LTHL),DE,(LTDE)
        LD BC,(NR0):ADD HL,BC:JR NC,$+3:INC DE
        LD (LTHL),HL,(LTDE),DE

        LD HL,NSDC,A,C:ADD A,(HL)
        LD (HL),A

        LD BC,(BSECPC)
        CP C:JP C,NXTC;  End OF Cluster?
;                        YES!

        LD HL,(CUHL),DE,(CUDE)
        CALL CURIT,GIPAG:JP Z,NXTC
        RET
;-------
RDDXX   LD BC,(LDMD)
        DEC C:JP Z,RDDSE
        DEC C:JP Z,SDDSE
        DEC C:RET Z
        RET

;------Searching for free Cluster:------
SRHFCL  LD HL,FSTFRC,DE,CAHL,BC,4:LDIR
SRHFC   ;i:[CADE,CAHL]-Cluster number
;        o:[DE,HL]- Cluster number
;          EXX HL - position in BUFFER+4

        LD HL,(CAHL),DE,(CADE)
        CALL CURIT:RET C

FC      LD A,(HL):INC HL
        OR (HL):INC HL
        OR (HL):INC HL
        OR (HL):INC HL:JR Z,GETZE

        EX DE,HL
        LD HL,CAHL
        INC (HL):JP NZ,AGA:INC HL
        INC (HL):JR NZ,AGA:INC HL
        INC (HL):JR NZ,AGA:INC HL
        INC (HL)
AGA     EX DE,HL
        LD A,H:CP HIGH SECBE:JP C,FC

        LD HL,(LTHL),DE,(LTDE):INC HL
        LD A,H:OR L:JR NZ,$+3:INC DE
        CALL XPOZI

        LD HL,SECBU,A,1:CALL RDDSE
        LD HL,SECBU
        JP FC
;-------
GETZE   EXX
        LD HL,(CAHL),DE,(CADE)
        PUSH HL:LD HL,CAHL:CALL INC4b
        POP HL
        XOR A
        RET

;------Read Sector from FAT:------------
CURIT   ;i:[DE,HL]-Cluster number
;        o:SECBU(512)
;          HL-poz in SECBU where Cluster

        CALL DEL128
        SLA C:RL B
        SLA C:RL B

        PUSH BC
        LD (LSTSE+2),DE,(LSTSE),HL

        LD BC,(BFTSZ+2)
        LD A,D:CP B:JR C,JK
        LD A,E:CP C:JR C,JK
        LD BC,(BFTSZ)
        LD A,H:CP B:JR C,JK
        LD A,L:CP C:JR NC,FATEND

JK      LD BC,(SFAT)
        CALL ADD4B
        CALL XSPOZ
        CALL XPOZI
        LD HL,SECBU,A,1:CALL RDDSE
        POP BC
        LD HL,SECBU:ADD HL,BC
        XOR A
        RET
;-------
FATEND  POP BC
        SCF
        RET

;----Pos. to First Sector of Cluster:---
GIPAG   ;i:HL(4) - Cluster number
        CALL TOS

        LD E,(HL):INC HL
        LD D,(HL):INC HL
        LD A,(HL):INC HL
        LD H,(HL),L,A:OR H,E,D:JR Z,RDIR

        LD A,H:AND #0F:CP #0F:JR Z,MDC
        EX DE,HL
POM     LD (CUHL),HL,(CUDE),DE

        LD BC,2:OR A:SBC HL,BC:JR NC,$+3:DEC DE

        LD A,(BSECPC):CALL UMNOX2
        LD BC,(SDFAT):CALL ADD4B
        EX DE,HL
        LD BC,(SDFAT+2):ADD HL,BC
        EX DE,HL
        CALL XSPOZ
        CALL XPOZI
        XOR A
        RET

RDIR    LD HL,(BROOTC),DE,(BROOTC+2)
        JR POM

MDC     LD (EOC),A
        OR A
        RET

;---Getting Absolute Position of SEC:---
XSPOZ   LD BC,(ADDTOP),(CLHL),BC
        LD BC,(ADDTOP+2),(CLDE),BC
        JP ADD4BF

;-----ADD NEW ENTRY TO ACTIVE DIR:------
SVHDFL  ;i:ENTRY-name,flag,size
;          LSTCAT(4)-Active DIR
;          FCTS(4)-Cluster numbr of DATA
;        o:[DE,HL]=FCTS(4)

        LD HL,ENTRY:CALL ENCEN
;-------
        LD HL,(LSTCAT),DE,(LSTCAT+2)
SHDFL   LD (CUHL),HL,(CUDE),DE
        CALL TOS
NXDCL   LD HL,LOBU
        LD B,1:CALL Load512
        LD HL,LOBU,DE,32,B,16
        XOR A
FIEL    CP (HL):JP Z,FELD
        ADD HL,DE
        DJNZ FIEL
        LD A,(EOC):CP #0F:JR NZ,NXDCL
        CALL SRHFCL:JR C,IEL
        LD (BUTS),HL,(BUTS+2),DE
        EXX
        LD A,#FF
        DEC HL:LD (HL),#0F
        DEC HL:LD (HL),A
        DEC HL:LD (HL),A
        DEC HL:LD (HL),A
        LD HL,SECBU,A,1:CALL SDDSE

        LD HL,(CUHL),DE,(CUDE)
        CALL CURIT:JR C,IEL
        EX DE,HL
        LD HL,BUTS,BC,4:LDIR
        LD HL,SECBU,A,1:CALL SDDSE

        LD HL,BUTS
        CALL GIPAG:RET NZ

        LD HL,SECBU,DE,HL:INC DE
        LD (HL),0,BC,512-1:LDIR

        LD HL,(LTHL),DE,(LTDE)
        PUSH HL,DE
        LD HL,SECBU
        LD A,1:CALL SDDSE

        LD A,(BSECPC):CALL NOPCLA
        POP DE,HL
        CALL XPOZI
        JP NXDCL
;-------
IEL     LD A,1:OR A:RET
;-------
FELD    EX DE,HL
        LD HL,(FCTS),(CLSHL),HL
        LD HL,(FCTS+2),(CLSDE),HL

        LD HL,ENTRY,BC,33:LDIR
        LD A,D,(DUBA),A

        LD HL,(LLHL),DE,(LLHL+2)
        CALL PROZ
        LD HL,LOBU
        LD A,1:CALL SDDSE

        LD HL,(FCTS),DE,(FCTS+2)
        XOR A
        RET

;------------Name Filter:---------------
ENCEN   ;i:HL - ENTRY(11) Address

        PUSH HL
        LD B,11
REGUP   LD A,(HL)
        CP #21:JR Z,MUR
        CP #2D:JR Z,MUR
        CP #2A:JR NC,TER
        CP #23:JR NC,MUR

TER     CP #30:JR C,MU
        CP #3A:JR C,NU
        CP #40:JR NC,NU
MU      LD (HL),#5F
        JR MUR
NU      CP #5B:JR Z,MU
        CP #5C:JR Z,MU
        CP #5D:JR Z,MU
        CP #61:JR C,MUR
        CP #7B:JR C,MuR
        CP #7C:JR Z,MU
        CP #F2:JR NC,MU
        JR MUR
MuR     SUB #20:LD (HL),A
MUR     INC HL
        DJNZ REGUP
        POP HL

        PUSH HL
        LD BC,7:ADD HL,BC:LD B,7:CALL RU
        POP HL
        LD BC,10:ADD HL,BC:LD B,3:CALL RU
        RET

RU      LD A,(HL):CP #5F:RET NZ
        LD (HL),#20:DEC HL
        DJNZ RU
        RET

;-----Generate Chain in FAT:------------
ERG2    LD HL,FCTS:CALL DLSG
        LD HL,FCTS,DE,FSTFRC,BC,4:LDIR
ERG1    LD A,1:OR A
        RET
;-------
MKSG    ;i:[DE,HL] - Number of Bytes
;       FSTFRC - First free Cluster
;o:FCTS(4)-First Cluster in New Chain
;FSTFRC(4)-New value

        CALL DEL512
        LD A,(BSECPC):CALL DELITX2

        LD (CLCNT+2),DE
        LD (CLCNT+0),HL

        CALL SGENBU:JR C,ERG1
ROSTIK  CALL PREPFC:JR C,ERG2
        CALL BUtoFAT:RET Z
        CALL SGENB2:JR C,ERG2
        JR ROSTIK
;---------------------------------------
SGENBU  LD HL,GENBU,(GARY),HL

        CALL SRHFCL:RET C
        LD (FCTS),HL,(FCTS+2),DE
GENB    LD (FSTFRC),HL,(FSTFRC+2),DE

        LD A,H,C,L
        LD HL,(GARY)
        LD (HL),C:INC HL
        LD (HL),A:INC HL
        LD (HL),E:INC HL
        LD (HL),D:INC HL
        LD (GARY),HL
        XOR A
        RET
;-------
SGENB2  CALL SRHFC:RET C
        JR GENB
;-------
PREPFC  LD HL,(CLCNT):DEC HL
        LD (CLCNT),HL
        LD A,H:OR L:JR Z,EOFG

AMM     EXX
        LD A,H:CP HIGH SECBE
        JR C,ARM
        CALL SRHFC:RET C
        JR AR2
ARM     CALL FC
AR2     LD (FSTFRC),HL,(FSTFRC+2),DE
;-------
        LD A,H,C,L
        LD HL,(GARY)
        LD (HL),C:INC HL
        LD (HL),A:INC HL
        LD (HL),E:INC HL
        LD (HL),D:INC HL
        LD A,H:CP HIGH GENBE:RET NC
        LD (GARY),HL
        JP PREPFC
;-------
EOFG    LD HL,(CLCNT+2)
        LD A,H:OR L:JR NZ,AmM

        LD DE,#0FFF
        LD HL,(GARY)
        LD (HL),E:INC HL
        LD (HL),E:INC HL
        LD (HL),E:INC HL
        LD (HL),D:INC HL
        LD (GARY),HL
        RET
;-------
AmM     DEC HL:LD (CLCNT+2),HL:JP AMM
;---------------------------------------
BUtoFAT LD HL,GENBU
GENFC   LD C,(HL):INC HL
        LD B,(HL):INC HL
        LD E,(HL):INC HL
        LD D,(HL):INC HL
;-------
        PUSH HL
        LD HL,BC:CALL CURIT:EX DE,HL
        POP HL

GNFC    LD (UUHL),HL
        LD BC,4:LDIR

        DEC HL:LD A,(HL):CP #0F:JR Z,LSTSR
        INC HL:LD A,H:CP HIGH GENBE:JR NC,LSTsr

        LD HL,(UUHL)
        LD C,(HL):INC HL
        LD B,(HL):INC HL
        LD E,(HL):INC HL
        LD D,(HL):INC HL
        LD (BUHL),HL

        LD HL,BC:CALL DEL128
        LD BC,(LSTSE)
        OR A:SBC HL,BC:JR NZ,GGC
        EX DE,HL:LD BC,(LSTSE+2)
        OR A:SBC HL,BC:JR Z,GFC

GGC     LD HL,SECBU,A,1:CALL SDDSE
        LD HL,(UUHL)
        JP GENFC

GFC     LD H,0,L,A
        SLA L:RL H
        SLA L:RL H
        LD BC,SECBU:ADD HL,BC
        EX DE,HL
        LD HL,(BUHL)
        JR GNFC
;-------
LSTSR   LD HL,SECBU:LD A,1:CALL SDDSE
;CALL RFRH
        XOR A
        RET
;-------
LSTsr   CALL LSTSR
        LD HL,(UUHL),DE,GENBU,BC,4:LDIR
        LD (GARY),DE

        LD A,1:OR A
        RET

;-----Delete Chain from FAT:------------
DLSG    ;i:HL(4) - Cluster number

        LD DE,LOBU,BC,4:LDIR
        LD HL,0
        LD (FSTFRC),HL
        LD (FSTFRC+2),HL
LWT     LD HL,(LOBU),DE,(LOBU+2)
        LD A,D:CP #0F:RET Z
        OR E,H,L:RET Z

        CALL CURIT:RET C
GOCE    LD DE,LOBU,C,0
        LD A,(HL),(HL),C,(DE),A:INC L,E
        LD A,(HL),(HL),C,(DE),A:INC L,E
        LD A,(HL),(HL),C,(DE),A:INC L,E
        LD A,(HL),(HL),C,(DE),A

        LD HL,(LOBU),DE,(LOBU+2)
        LD A,D:CP #0F:JR Z,Ne
        CALL DEL128
        LD BC,(LSTSE)
        OR A:SBC HL,BC:JR NZ,NE:EX DE,HL
        LD BC,(LSTSE+2)
        OR A:SBC HL,BC:JR NZ,NE

        LD H,0,L,A
        SLA L:RL H
        SLA L:RL H
        LD BC,SECBU:ADD HL,BC
        JP GOCE

NE      LD HL,SECBU,A,1:CALL SDDSE
        JP LWT
Ne      LD HL,SECBU,A,1:JP SDDSE

;-----Searching ENTRY in Active DIR:----
;BY NAME:
SRHDRN  ;i:LSTCAT(4) - Active DIR
;          HL - Buffer with Name
;               [flag(1)+name(1-12)+#00]
;        o: Z: NOT FOUNT
;          NZ: ENTRY(32)
;              [DE,HL] - Cluster Number
;                   BC - Address in LOBU

        LD A,(HL):INC HL
        LD (ENTRY+11),A:CALL GENTRYX

        LD HL,LSTCAT,DE,CUHL,BC,4:LDIR
        CALL TOS

HDR     LD A,(EOC):CP #0F:RET Z
        LD HL,LOBU,B,1:CALL Load512
        LD (HL),0

        LD HL,LOBU-32:CALL VEGA:RET NZ
        JR HDR
;-------
VEGA    LD BC,32:ADD HL,BC
        LD A,(HL):OR A:RET Z
        CALL CHEB:JR NZ,VEGA
        LD A,1:OR A
        RET
;-------
CHEB    PUSH HL
        LD DE,ENTRY,B,11:CALL CHEE,Z,CHF
        POP HL:RET NZ

        PUSH HL:LD DE,ENTRY,BC,32:LDIR
        POP BC
        LD HL,(CLSHL)
        LD DE,(CLSDE)
        XOR A
        RET
;-------
CHF     LD A,(DE),C,A
        LD A,(HL):AND #10:CP C
        RET
;-------
CHEE    LD A,(DE):CP (HL):RET NZ
        INC HL,DE
        DJNZ CHEE
        RET

;---------------------------------------
;SAVE Sector with ENTRY, if modified:

STAMP   LD HL,(LLHL),DE,(LLHL+2)
        CALL PROZ

        LD HL,LOBU,A,1:JP SDDSE

;---------------------------------------
;MAKE NEW FILE:
MKFILE  ;i:     HL - Flag(1),Name,#00
;          [BC,DE] - Lenght

        LD (SIZIK),DE,(SIZIK+2),BC
        LD A,(HL),(EFLG),A:INC HL

        CALL GENTRYX

        LD HL,(SIZIK),DE,(SIZIK+2)
        CALL MKSG:RET NZ
        CALL SVHDFL:RET NZ
        XOR A
        RET

;---------------------------------------
;MAKE NEW DIRECTORY:
MKDIR   ;i:HL - DIR NAME(1-12), #00

        CALL GENTRYX

        LD HL,0,(SIZIK),HL,(SIZIK+2),HL
        LD A,#10,(EFLG),A
        LD DE,0,HL,512
        CALL MKSG:RET NZ
        CALL SVHDFL:RET NZ
        LD HL,FCTS
        CALL GIPAG

        LD HL,ENTRY
        LD (HL),".":INC HL
        LD (HL),#20:INC HL
        LD A,32,B,9:CALL NOPING+1
        LD HL,(CUHL),(CLSHL),HL
        LD HL,(CUDE),(CLSDE),HL
        LD HL,ENTRY,DE,LOBU,BC,32:LDIR

        LD HL,ENTRY+1
        LD (HL),"."
        LD HL,(LSTCAT),(CLSHL),HL
        LD HL,(LSTCAT+2),(CLSDE),HL
        LD HL,ENTRY,BC,32:LDIR

        LD HL,DE:INC DE
        LD BC,512-32*2-1
        LD (HL),0
        LDIR
        LD HL,LOBU,A,1:CALL SDDSE

        LD A,(BSECPC):CALL NOPCLA
        XOR A
        RET

RFRH    LD HL,(FSINF),DE,(FSINF+2)
        CALL XPOZI
        LD HL,LOBU
        LD A,1:CALL RDDSE
        LD HL,FSTFRC,DE,LOBU+492,BC,4
        LDIR
        LD HL,LOBU
        LD A,1:CALL SDDSE
        RET
;---------------------------------------
;CHAIN->SECTORS:
CHTOSE  ;i:HL(4) - First Cluster number
;             DE - BUFFER Address
;             BC - End Of BUFFER

        LD (DABC),BC
        LD (DADE),DE
CKAGO   LD (DAHL),HL

        CALL GIPP:RET Z
        CALL CLUSSEC
        LD DE,(DABC)
        OR A:SBC HL,DE:RET NC

        LD HL,(DAHL)
        LD E,(HL):INC HL
        LD D,(HL):INC HL
        LD A,(HL):INC HL
        LD H,(HL),L,A:EX DE,HL

        CALL CURIT
        JR CKAGO

;-------
CLUSSEC ;i:  (DADE) - Buffer Address
;          (BSECPC) - Sectors per CLUS
;           [DE,HL] - Sector number
;
;        o:  (DADE) - New Value

        LD BC,HL
        LD A,(BSECPC)
        LD HL,(DADE)
USS     LD (HL),C:INC HL
        LD (HL),B:INC HL
        LD (HL),E:INC HL
        LD (HL),D:INC HL
        LD (DADE),HL:DEC A:RET Z

        INC BC
        EXA:LD A,B:OR C:JR NZ,$+3:INC DE
        EXA
        JR USS

;-------
GIPP    ;i:  HL(4) - Cluster number
;        o:[DE,HL] - Sector number
;                Z - EndOfChain

        LD E,(HL):INC HL
        LD D,(HL):INC HL
        LD A,(HL):INC HL
        LD H,(HL),L,A:OR H,E,D:RET Z
        LD A,H:CP #0F:RET Z
        EX DE,HL

        LD BC,2:OR A:SBC HL,BC:JR NC,$+3:DEC DE

        LD A,(BSECPC):CALL UMNOX2
        LD BC,(SDFAT):CALL ADD4B
        EX DE,HL
        LD BC,(SDFAT+2):ADD HL,BC
        EX DE,HL
        CALL XSPOZ

        LD A,1:OR A
        RET

;---------------------------------------
;SEARCH PARTITION:
HDD     ;i:none
;        o:NZ - FAT32 not found
;           Z - all FAT32 vars are
;               initialized

        LD HL,0,DE,HL
        LD (CUHL),HL,(CUDE),HL
        LD (DAHL),HL,(DADE),HL
        LD (DUHL),HL,(DUDE),HL
        CALL XPOZI
        LD HL,LOBU,A,1:CALL RDDSE

        LD A,3,(COUNT),A,(ZES),A

        LD HL,LOBU+446+4,DE,16,B,4
KKO     LD A,(HL)
        CP #05:JR Z,OKK
        CP #0B:JR Z,OKK
        CP #0C:JR Z,OKK
        CP #0F:JR Z,OKK
        ADD HL,DE
        DJNZ KKO

FHDD    LD A,(ZES):OR A:JP Z,Nhdd

        LD DE,(DADE),HL,(DAHL)
        CALL XPOZI

        LD HL,LOBU:LD A,1:CALL RDDSE

        LD HL,COUNT:DEC (HL):JP Z,NHDD

        LD HL,LOBU+446+16,B,16
        XOR A:OR (HL):INC HL:DJNZ $-2
        JP NZ,NHDD

        LD HL,(LOBU+446+16+8)
        LD DE,(LOBU+446+16+8+2)
        LD (CLHL),HL,(CLDE),DE
        LD HL,(DAHL),DE,(DADE)
        CALL ADD4BF
        LD (DADE),DE,(DAHL),HL
        CALL XPOZI

        LD HL,LOBU,A,1:CALL RDDSE

        LD HL,(LOBU+446+8)
        LD DE,(LOBU+446+8+2)
        CALL ADD4BF
        JR LDBPB

OKK     INC HL,HL,HL,HL
        LD E,(HL):INC HL
        LD D,(HL):INC HL
        LD A,(HL):INC HL
        LD H,(HL),L,A
        EX DE,HL

LDBPB   LD (ADDTOP),HL,(ADDTOP+2),DE
        CALL XPOZI

        LD HL,LOBU;LOAD BPB SECTOR
        LD A,1:CALL RDDSE

;LD HL,LOBU+3,B,6,A,#1D
;FIVE CP (HL):INC HL:JP NC,FHDD
;DJNZ FIVE

        LD HL,(LOBU+11)
        LD A,H:DEC A,A:OR L:JP NZ,FHDD
        LD A,(LOBU+13):OR A:JP Z,FHDD
        LD A,(LOBU+14):OR A:JP Z,FHDD
        LD A,(LOBU+16):OR A:JP Z,FHDD

        LD HL,(LOBU+17),A,H:OR L
;LD HL,(LOBU+19):OR H,L
        LD HL,(LOBU+22):OR H,L
        JP NZ,FHDD
        LD HL,(LOBU+36):OR H,L
        LD HL,(LOBU+36+2):OR H,L
        JP Z,FHDD

;LD HL,ADDTOP,DE,DUHL,BC,4:LDIR
;LD HL,DNU,A,(HL):DEC (HL)
;OR A:JP NZ,FHDD
;LD (HL),0

        LD A,(LOBU+13),(BSECPC),A
        LD B,8:SRL A:JR C,NER:DJNZ $-4:LD A,1
NER     OR A:JP NZ,FHDD
        LD HL,(LOBU+14),(BREZS),HL
        LD HL,(LOBU+48),DE,0
        CALL XSPOZ
        LD (FSINF),HL,(FSINF+2),DE

        LD A,(LOBU+16),(BFATS),A
        LD HL,(LOBU+36),(BFTSZ),HL
        LD HL,(LOBU+36+2),(BFTSZ+2),HL
        LD HL,(LOBU+44),(BROOTC),HL
        LD HL,(LOBU+44+2),(BROOTC+2),HL

        LD HL,(BFTSZ),DE,(BFTSZ+2)
        LD BC,(BFATS),B,0
        CALL UMN4B
        PUSH HL,DE
        LD HL,(BREZS)
        LD (SFAT),HL
        POP DE,BC
        CALL ADD4B
        LD (SDFAT),HL,(SDFAT+2),DE

        LD HL,0
        LD (CUHL),HL
        LD (CUDE),HL
        LD (LSTCAT),HL
        LD (LSTCAT+2),HL

;LD HL,(FSINF),DE,(FSINF+2)
;CALL XPOZI
;LD HL,LOBU
;LD A,1:CALL RDDSE
;LD HL,LOBU+492,DE,FSTFRC
;LD BC,4:LDIR
        LD HL,FSTFRC,B,4:CALL NOPING

        CALL TOS

        XOR A
        RET

NHDD    LD HL,(DUHL),DE,(DUDE)
        XOR A:LD (ZES),A
        JP LDBPB

Nhdd    LD A,1:OR A
        RET

;---------------------------------------
;ARITHMETICS BLOCK:
DEL128  ;i:[DE,HL]/128
;        o:[DE,HL]
;          BC - Remainder

        LD A,L:EXA
        LD A,L,L,H,H,E,E,D,D,0
        RLA
        RL L,H,E,D
        EXA
        AND 127
        LD B,0,C,A
        RET
;-------
DEL512  ;i:[DE,HL]/512
        LD A,L,L,H,H,E,E,D,D,0
        LD BC,1:OR A:CALL NZ,ADD4B
        LD A,2
;-------
DELITX2 ;i:[DE,HL]/A
;          A - Power of Two
;        o:[DE,HL]

        CP 2:RET C
        LD C,0
        SRL A
L33T    SRL D:RR E,H,L,C
        SRL A:JR NC,L33T

        LD A,C:OR A:RET Z
        LD BC,1:CALL ADD4B
        RET
;-------
UMNOX2  ;i:[DE,HL]*A
;          A - Power of Two
;        o:[DE,HL]

        CP 2:RET C
        SRL A
L33t    SLA L:RL H,E,D
        SRL A:JR NC,L33t
        RET
;-------
;UMNOG   ;HL*BC=HL DEdest
;LD DE,HL
;LD A,B,B,C,C,A:INC C
;XOR A:DEC B:JR Z,ODN
;BSR     ADD HL,DE
;DJNZ BSR
;ODN     LD B,A
;DEC C
;JR NZ,BSR
;RET

INC4b   LD B,4
EkE     INC (HL):RET NZ:INC HL:DJNZ EkE
        RET

ADD4B   ADD HL,BC:RET NC:INC DE
        RET

ADD4BF  ;i:[DE,HL]+[CLDE,CLHL]
;        o:[DE,HL]
        EX DE,HL
        LD BC,(CLDE)
        ADD HL,BC
        EX DE,HL
        LD BC,(CLHL)
        ADD HL,BC:JR NC,KNH
        INC DE
KNH     LD (CLHL),HL
        LD (CLDE),DE
        RET

UMN4B   ;i:[DE,HL]*BC o:[DE,HL]
        LD A,B,B,C,C,A:INC C
        OR A:JR NZ,TEKNO
        DEC B:JR Z,UMN1
        INC B
TEKNO   XOR A
        CP B:JR NZ,TYS
        DEC C
TYS     DEC B
        PUSH HL,BC
        LD HL,DE
        CP B:JR Z,NEGRY
EFRO    ADD HL,DE
        DJNZ EFRO
        LD B,A
NEGRY   DEC C:JR NZ,EFRO
        LD (REZDE),HL
        POP BC,HL
        LD DE,HL
        CP B:JR Z,NEGRA
OFER    ADD HL,DE
        JR C,INCDE
ENJO    DJNZ OFER
        LD B,A
NEGRA   DEC C:JR NZ,OFER
        LD DE,(REZDE)
UMN1    RET
INCDE   EXX
        LD HL,(REZDE)
        INC HL
        LD (REZDE),HL
        EXX
        JR ENJO
;---------------------------------------
TOS     XOR A:LD (NSDC),A,(EOC),A:RET

XPOZI   LD (LTHL),HL,(LTDE),DE
PROZ    LD (BLKNUM),HL,(BLKNUM+2),DE:RET

NOPCLA  LD HL,SECBU,DE,HL:INC DE
        LD (HL),0,BC,512-1:LDIR
NOPC    DEC A:RET Z
        PUSH AF
        LD HL,(LTHL),DE,(LTDE)
        LD BC,1:ADD HL,BC
        JR NC,$+3:INC DE
        CALL XPOZI

        LD HL,SECBU,A,1:CALL SDDSE
        POP AF
        JR NOPC

NOPING  XOR A:LD (HL),A:INC HL:DJNZ $-2
        RET
NOPIDE  LD A,32,(DE),A:INC DE:DJNZ $-2
        RET
;---------------------------------------

;       INCL "DMA"
;---------------------------------------
PREDMA  LD A,H,C,PG0
        CP #80:JR NC,$+6:LD BC,(PGO)
        AND %00111111
        LD D,A,A,C

        LD BC,STS:INF:JP M,$-2
        RET
;-------
POSDMA  LD DE,512:ADD HL,DE
        LD A,H
        CP #40:JR C,TSDL
        CP #80:JR NC,TSDL
        AND %00111111:LD H,A
        LD A,(PGO):INC A
        LD (PGO),A
TSDL    INF:JP M,$-2
        RET
;---------------------------------------
STS     EQU #27AF

DMASL   EQU #1AAF
DMASH   EQU #1BAF
DMASX   EQU #1CAF
DMADL   EQU #1DAF
DMADH   EQU #1EAF
DMADX   EQU #1FAF

DMA_T   EQU #28AF;0=1, 255=256
DMA_N   EQU #26AF;255=512
DMA_C   EQU #27AF;RW 1 - - - 0 0 1
;---------------------------------------
;       !INCL "DMA"

;       INCL "DSDTS"
;---------------------------------------
P_CONF  EQU #77
P_DATA  EQU #57

CMD_12  EQU #4C
CMD_18  EQU #52
CMD_25  EQU #59
CMD_55  EQU #77
CMD_58  EQU #7A
CMD_59  EQU #7B
ACMD_41 EQU #69
;---------------------------------------

INIsd   JP SD__OFF
;=======================================
RDDSEsd LD DE,(BLKNUM)
        LD BC,(BLKNUM+2)
        EXA
        LD A,CMD_18:CALL SECM200
        EXA
RD1     EXA
        CALL IN_OUT:CP #FE:JR NZ,$-5
        CALL READSsd
        EXA
        DEC A:JR NZ,RD1

        LD A,CMD_12
        CALL OUT_COM
        CALL IN_OUT:INC A:JR NZ,$-4
        JP CS_HIGH

SDDSEsd LD DE,(BLKNUM)
        LD BC,(BLKNUM+2)
        EXA
        XOR A:IN A,(P_CONF)
        AND 2:RET NZ
        LD A,B:OR C,D,E
        LD A,3:RET Z

        LD A,CMD_25:CALL SECM200
        CALL IN_OUT:INC A:JR NZ,$-4
        EXA
SD1     EXA
        LD A,#FC:CALL SAVDSsd
        CALL IN_OUT:INC A:JR NZ,$-4
        EXA
        DEC A:JR NZ,SD1

        LD C,P_DATA,A,#FD:OUT (C),A
        CALL IN_OUT:INC A:JR NZ,$-4
        JP CS_HIGH
;---------------------------------------
READSsd PUSH BC,DE

        CALL PREDMA

        LD B,HIGH DMADX:OUT (C),A
        DEC B:OUT (C),D
        DEC B:OUT (C),E

        LD B,HIGH DMA_T:XOR A:OUT (C),A
        LD B,HIGH DMA_N:DEC A:OUT (C),A
        LD B,HIGH DMA_C:LD A,%00000010:OUT (C),A

        CALL POSDMA

        LD BC,P_DATA
        IN A,(C)
        IN A,(C)
        POP DE,BC
        RET

;---------------------------------------
SAVDSsd PUSH BC,DE
        LD BC,P_DATA:OUT (C),A

        CALL PREDMA

        LD B,HIGH DMASX:OUT (C),A
        DEC B:OUT (C),D
        DEC B:OUT (C),E

        LD B,HIGH DMA_T:XOR A:OUT (C),A
        LD B,HIGH DMA_N:DEC A:OUT (C),A
        LD B,HIGH DMA_C:LD A,%10000010:OUT (C),A

        CALL POSDMA

        LD BC,P_DATA,A,#FF
        OUT (C),A:NOP
        OUT (C),A
        POP DE,BC
        RET

;---------------------------------------
CMD00   DB #40:DS 4:DB #95
CMD08   DB #48,0,0,1,#AA,#87
CMD16   DB #50,0,0,2,0,#FF
;---------------------------------------
SELsd   ;i:A - N of Dev
        OR A:RET NZ
DRDET   CALL SD_INIT
        LD DE,2:OR A:RET NZ
        LD DE,0
        RET
;---------------------------------------
SD_INIT CALL CS_HIGH
        LD BC,P_DATA,DE,#10FF
        OUT (C),E:DEC D:JR NZ,$-3
        XOR A:EXA
ZAW001  LD HL,CMD00:CALL OUTCOM,IN_OUT
        EXA:DEC A:JR Z,ZAW003
        EXA:DEC A:JR NZ,ZAW001
        LD HL,CMD08:CALL OUTCOM,IN_OUT
        IN H,(C):NOP
        IN H,(C):NOP
        IN H,(C):NOP
        IN H,(C)
        LD HL,0:BIT 2,A
        JR NZ,ZAW006:LD H,#40
ZAW006  LD A,CMD_55:CALL OUT_COM,IN_OUT
        LD A,ACMD_41:OUT (C),A:NOP
        OUT (C),H:NOP
        OUT (C),E:NOP
        OUT (C),E:NOP
        OUT (C),E
        LD A,#FF:OUT (C),A
        CALL IN_OUT:AND A:JR NZ,ZAW006
ZAW004  LD A,CMD_59:CALL OUT_COM,IN_OUT
        AND A:JR NZ,ZAW004
ZAW005  LD HL,CMD16:CALL OUTCOM,IN_OUT
        AND A:JR NZ,ZAW005

CS_HIGH PUSH DE,BC
        LD E,3,BC,P_CONF
        OUT (C),E:LD E,0,C,P_DATA
        OUT (C),E
        POP BC,DE
        RET

ZAW003  CALL SD__OFF
        LD A,1
        RET

SD__OFF XOR A
        OUT (P_CONF),A
        OUT (P_DATA),A
        RET

CS__LOW PUSH DE,BC
        LD BC,P_CONF,E,1:OUT (C),E
        POP BC,DE
        RET

OUTCOM  CALL CS__LOW
        PUSH BC
        LD BC,P_DATA
        OUTI:NOP
        OUTI:NOP
        OUTI:NOP
        OUTI:NOP
        OUTI:NOP
        OUTI:NOP
        POP BC
        RET

OUT_COM PUSH BC
        CALL CS__LOW
        LD BC,P_DATA
        OUT (C),A:XOR A
        OUT (C),A:NOP
        OUT (C),A:NOP
        OUT (C),A:NOP
        OUT (C),A:DEC A
        OUT (C),A
        POP BC
        RET

SECM200 PUSH HL,DE,BC,AF,BC
        LD BC,P_DATA,A,CMD_58
        CALL OUT_COM,IN_OUT
        IN A,(C):NOP
        IN H,(C):NOP
        IN H,(C):NOP
        IN H,(C):BIT 6,A:POP HL
        JR NZ,SECN200
        EX DE,HL:ADD HL,HL:EX DE,HL
        ADC HL,HL
        LD H,L,L,D,D,E,E,0
SECN200 POP AF
        LD BC,P_DATA:OUT (C),A
        NOP:OUT (C),H
        NOP:OUT (C),E
        NOP:OUT (C),D
        NOP:OUT (C),E
        LD A,#FF:OUT (C),A
        POP BC,DE,HL
        RET

IN_OUT  PUSH BC,DE
        LD DE,#10FF
        LD BC,P_DATA
IN_WAIT IN A,(C)
        CP E:JR NZ,IN_EXIT
IN_NEXT DEC D:JR NZ,IN_WAIT
IN_EXIT POP DE,BC
        RET
;---------------------------------------
;       !INCL "DSDTS"

;       INCL "DIDETS"
;---------------------------------------
COMAH   LD BC,#F0:OUT (C),A:RET
COMM    LD BC,#F0:OUT (C),A:RET
LOLL    LD BC,#F0:IN A,(C):RET
;---------------------------------------
REREG   PUSH HL,DE
        LD HL,(BLKNUM)
        LD DE,(BLKNUM+2)
        LD A,H,H,D,D,E,E,A

        LD A,H:AND %00001111:LD H,A
        LD A,(DRVRE)
        OR H
        LD BC,#FFD0:OUT (C),A
        LD C,#70:OUT (C),E
        LD C,#B0:OUT (C),D
        LD C,#90:OUT (C),E
        POP DE,HL
        RET

;---------------------------------------
RPOZ    LD BC,#FFD0:IN A,(C):AND #0F
        LD H,A,C,#70:IN L,(C)
        LD C,#B0:IN D,(C)
        LD C,#90:IN E,(C)
        RET

;---------------------------------------
;i:HL - Address
;   A - Sectors

RDDSEhd EXA:CALL DRDY
        EXA
        LD BC,#50:OUT (C),A
        EXA
        CALL REREG
        LD A,#20:CALL COMAH,READY
        EXA
RDH1    EXA
        CALL WAITDRQ
        CALL READShd
        CALL READY
        EXA:DEC A:JR NZ,RDH1
        RET

;---------------------------------------
SDDSEhd EXA:CALL DRDY
        EXA
        LD BC,#50:OUT (C),A
        EXA
        CALL REREG
        LD A,#30:CALL COMAH,READY
        EXA
SDH1    EXA
        CALL WAITDRQ
        CALL SAVDShd
        CALL READY
        EXA:DEC A:JR NZ,SDH1
        RET

;---------------------------------------
READY   CALL LOLL:RLCA:RET NC:JR READY
WAITDRQ CALL LOLL:AND 8:RET NZ
        JR WAITDRQ
DRDY    CALL LOLL
        AND %11000000
        CP %01000000:RET Z
        JR DRDY

ERROR_7 CALL LOLL:RRCA:RET

SEL_SLA LD A,#B0
        LD (DRVRE),A
        LD BC,#D0:OUT (C),A:CALL LOLL
        RLCA
        RET
SEL_MAS LD A,#E0
        JR SEL_SLA+2

;---------------------------------------
DV2     CALL SEL_SLA:JR DRDEThd
;-------
SELide  ;i:A - N of dev
        CP 2:RET NC
        DEC A:JR Z,DV2
        CALL SEL_MAS
DRDEThd LD A,#08:CALL COMM
        LD L,32
YDET    CALL LOLL:RLCA:JR NC,RRR
        CALL ERROR_7:JR C,RRR
        LD B,1:CALL DIHALT
        DEC L:JR NZ,YDET

        LD DE,500
        JR RUhd

RRR     LD DE,0
        LD H,D,L,E
        CALL XPOZI,REREG
        LD BC,#F0,A,#EC:OUT (C),A;ECCO
        LD B,4:CALL DIHALT
        CALL RPOZ:LD A,D:OR E:JR Z,KRU
;LD HL,#EB14
;OR A:SBC HL,DE:RET Z;ATAPI
RUhd    LD A,1:OR A
        RET
;-------
KRU     LD HL,LOBU:CALL READShd
        LD DE,0
        XOR A
        RET

;---------------------------------------
;READ 512b from IDE Device BUFFER
READShd CALL PREDMA

        LD B,HIGH DMADX:OUT (C),A
        DEC B:OUT (C),D
        DEC B:OUT (C),E

        LD B,HIGH DMA_T:XOR A:OUT (C),A
        LD B,HIGH DMA_N:DEC A:OUT (C),A

        LD B,HIGH DMA_C:LD A,%00000011:OUT (C),A
        JP POSDMA
;---------------------------------------
;SAVE 512b to IDE Device BUFFER
SAVDShd CALL PREDMA

        LD B,HIGH DMASX:OUT (C),A
        DEC B:OUT (C),D
        DEC B:OUT (C),E

        LD B,HIGH DMA_T:XOR A:OUT (C),A
        LD B,HIGH DMA_N:DEC A:OUT (C),A

        LD B,HIGH DMA_C:LD A,%10000011:OUT (C),A
        JP POSDMA
;---------------------------------------
;       !INCL "DIDETS"

;---------------------------------------
LDMD    EQU LoCALL;2

GARY    EQU LDMD+2;2;see MKSG
DABC    EQU GARY+2;2;see CHTOSE
DAHL    EQU DABC+2;2;see HDD proc.
DADE    EQU DAHL+2;2; /
ZES     EQU DADE+2;1;/

PGO     EQU ZES+1;2;Temp Page
PGR     EQU PGO+2;2;Restore Page

CRRR    EQU PGR+2;4

SPBU    EQU CRRR+4;2
;-------
IDE_INI EQU SPBU+2
SEL_DEV EQU IDE_INI+3
RDDSE   EQU SEL_DEV+3
SDDSE   EQU RDDSE+3
;---------------------------------------
END
