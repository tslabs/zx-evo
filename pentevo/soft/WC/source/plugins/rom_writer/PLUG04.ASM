;ROM WRITER v0.3
;(C) KOSHI/MGN/2025
;(C) ZBOSZOR/2015
;    VIRTUAL ROM @ RAM #F8 PAGE
;---------------------------------------
        SAVEBIN "rom_prog.wmf";---------------------------------------        ORG #6000
;-------WNDW    EQU 32WNDH    EQU 6ROM64   EQU 4; PAGESROM512  EQU 32;PAGESENTRYL  EQU 256;-------
WLD     EQU #6006
;---------------------------------------
;HEADER:
        DS 16
        DB "WildCommanderMDL";    Header
        DB #10;                  Version
        DB 0;                       Type
        DB 1; Pages
        DB 0; Page to #8000
;-------
;       page number + block size
;       (4*512 bytes)
        DB 0,4
.5      DB 0,0
;-------
        DS 16
;-------
        DB "ROM"
        DS 31*3
;-------
        DB 0
;-------
        DW #0000,#0008; MAX SIZE 512KB
;-------
        DB "ROM WRITER v0.3 "
        DB "                "
;-------
        DB 0;0-BY EXTENSION ONLY
;            1-IF LOADED
;            2-BY TIMER
;            3-FROM MENU
;---------------------------------------
        ORG #8000,#6200
;-------
LOBU    EQU #A000
;---------------------------------------
PLUGIN  LD (DAHL),HL,(DADE),DE        LD A,H:OR L:JR NZ,HUTA        LD A,E:OR A:JR Z,HUTA        CP 8:JR Z,ROMGO        CP 1:JR Z,ROMGOHUTA    LD A,1        RETROMGO   ADD A,A,A,A:LD (ROMmode),A        PUSH BC:CALL NAMEcls        POP HL:LD DE,ENTRY,B,ENTRYL:CALL NAMEcopy
        CALL ROMLOAD
        LD IX,PLWND:CALL PRWOW
        CALL STATUS
        CALL GEDPL
        LD HL,0,(BLKN),HL
;-------
NAIN    LD IX,PLWND
        LD A,1:CALL YN
MAIN    EI:HALT:XOR A:CALL YN
        CALL BLK40
        CALL ESC:JR NZ,EXIT
        CALL ENKE:JR Z,MAIN

        LD A,#FF:CALL YN:JR NZ,EXIT
;-------        LD IX,FIWND:CALL PRWOW
        LD A,1:CALL YN
MAIN2   EI:HALT:XOR A:CALL YN
        CALL ESC:JR NZ,NAINR
        CALL ENKE:JR Z,MAIN2
        CALL RRESB
        LD A,#FF:CALL YN:JR NZ,NAIN        LD A,(ROMmode):CP ROM64:JR Z,ROM64go        XOR A:LD (BLKN),A:JR ROMM
ROM64go LD A,(BLKN):CP 8:JR C,ROMM
        CALL F8COPY
        JR EXITNAINR   CALL RRESB:JR NAIN
ROMM    LD A,(BLKN):ADD A,A,A,A:LD (BLKN),A        LD C,0:CALL TURBO;3,5 MHz        CALL ROMWRITE
EXIT    LD IX,PLWND:CALL RRESB
        LD A,(ESTAT)
        RET
;---------------------------------------
BLK40   LD A,(ROMmode):CP ROM64:RET NZ        CALL UPPP,NZ,IBLK
        CALL DWWW,NZ,DBLK
        LD A,(BLKN):CP 8:JR C,BLK40R
        LD HL,SR01,DE,#0401,BC,17:CALL PRSRW
        RET
BLK40R  LD HL,SR00+12,A,(BLKN):CALL NORK
        LD HL,SR00,DE,#0401,BC,17:CALL PRSRW
        RET
;-------
IBLK    LD A,(BLKN):INC A:JR BLK
DBLK    LD A,(BLKN):DEC A
BLK     CP 9:RET NC
        LD (BLKN),A
        RET
;---------------------------------------
MEPRNT  ;i:HL - Text Address
;          BC - Lenght
        PUSH HL,BC
        LD IX,MEWND:CALL PRWOW
        POP BC,HL
        LD A,(IX+4):SUB C:SRL A:LD E,A
        LD D,#01:CALL PRSRW
        RET
;-------
MERRES  LD IX,MEWND:JP RRESB
;---------------------------------------
ERPRNT  ;i:HL - Text Address
;          BC - Lenght
        PUSH HL,BC
        LD IX,ERWND:CALL PRWOW
        POP BC,HL
        LD A,(IX+4):SUB C:SRL A:LD E,A
        LD D,#01:CALL PRSRW
        RET
;-------
ERRRES  LD IX,ERWND:JP RRESB
;---------------------------------------STATUS  LD IX,PLWND
        LD HL,TXT01,DE,#0101,BC,5:CALL PRSRW
        LD A,%01110001:CALL PRIAT
        LD HL,ENTRY,DE,#0106,BC,WNDW-5:CALL PRSRW
        LD A,%01110000:CALL PRIAT
        CALL SIZEK
        LD HL,TXT03,DE,#0201,BC,5:CALL PRSRW
        LD A,%01110010:CALL PRIAT
        LD HL,TXT04,DE,#0206,BC,10:CALL PRSRW
        LD A,%01110000:CALL PRIAT
        RET
;--------------------------------------- NAMEcls  LD HL,ENTRY,DE,ENTRY+1,(HL)," ",BC,ENTRYL-1:LDIR:RETNAMEcopy LD A,(HL):OR A:RET Z         LD (DE),A:INC HL,DE         DJNZ NAMEcopy         RET

;-------
SIZEK   LD HL,TXT04+1        LD DE,(DADE)        LD A,D:CALL NORK        LD A,E:CALL NORK        LD HL,TXT05+1        LD DE,(DAHL)        LD A,D:CALL NORK        LD A,E:CALL NORK        RET
;---------------------------------------
ROMLOAD LD A,0-1,(PAG),APIN     LD A,(PAG),HL,(ROMmode):INC A:CP L:RET NC        LD (PAG),A:CALL MNGCVPL        LD HL,#C000,B,32:CALL LOAD512        CP #0F:JR NZ,PIN        RET
;-------
ROMCHE  LD A,0-1,(PAG),ABLOCKb  LD A,(PAG),HL,(ROMmode):INC A:CP L:RET NC        LD (PAG),A:CALL MNGCVPL        LD BC,PW0:OUT E
        LD HL,#C000;Data        LD BC,#0000;ROM
B16b    LD A,(BC):CP (HL):RET NZ:INC HL,BC        LD A,(BC):CP (HL):RET NZ:INC HL,BC        LD A,H:OR A:JR NZ,B16b        INC E        JR BLOCKb
;---------------------------------------PAGEC   EQU #6003;---------------------------------------
F8COPY  CALL GIPAGPL ; rewind file        LD A,#F8-1:LD (PAG),AF8C     LD A,(PAG):INC A        CP #F8+4:RET NC        LD (PAG),A:LD (PAGEC),A        LD HL,#C000,B,32:CALL LOAD512        JR F8C
;---------------------------------------
PW0     EQU #10AFPE0     EQU #21AF
WRE     EQU %00000110WRD     EQU %00001110;---------------------------------------ROMWRITE DI
;ROM WRITE ENABLE        LD BC,PE0,A,WRE:OUT A        XOR A:LD (ERR),A
;ERASE        LD HL,ME00,BC,10:CALL MEPRNT        LD DE,(BLKN)        LD A,(ROMmode):CP ROM64:JR Z,ROM64GO        LD B,7ROM512A PUSH BC:CALL ERASblk        POP BC:LD A,4:ADD A,E:LD E,A        DJNZ ROM512A        ROM64GO CALL ERASblk
        CALL MERRES;-------
;PROGRAM        LD HL,ME02,BC,10:CALL MEPRNT        LD DE,(BLKN):CALL SVBLOCK:JR NC,DOT:LD A,1,(ERR),A
DOT     CALL MERRES;-------
;VERIFY        LD HL,ME03,BC,12:CALL MEPRNT        LD DE,(BLKN):CALL ROMCHE        PUSH AF:CALL MERRES:POP AF:JR Z,WHU:LD A,(ERR):OR 2:LD (ERR),A
;DISABLE ROM WRITEWHU     LD BC,PE0,A,WRD:OUT A        LD BC,PW0,A,(#6000):OUT A        EI
        LD A,(ERR):OR A:JR NZ,ERD        LD HL,ME04,BC,7:CALL MEPRNTWTT     CALL NUSP        JP RRESB
;-------ERD     CP 2:JR NZ,ER2        LD HL,ER00,BC,16:CALL ERPRNT        JR WTT
ER2     LD HL,ER01,BC,15:CALL ERPRNT        JR WTT
;---------------------------------------
;WRITE BLOCK
SVBLOCK LD A,0-1,(PAG),ABLOCK   LD A,(PAG),HL,(ROMmode):INC A:CP L:RET NC        LD (PAG),A:CALL MNGCVPL
        LD HL,#C000;Data
B16     LD A,(HL):CP #FF:CALL NZ,PROG:RET C
        INC HL:LD A,H:OR A:JR NZ,B16        INC E        JR BLOCK
;---------------------------------------
PROG    PUSH HL        LD BC,PW0:LD L,0:OUT L        LD HL,#0555,(HL),#AA        LD HL,#02AA,(HL),#55        LD HL,#0555,(HL),#A0        LD BC,PW0:OUT E        POP HL        LD C,L,B,H:RES 7,B,6,B:LD (BC),A        JR WAITROM
;---------------------------------------
;erases 64kB block of ROMERASblk LD BC,PW0:XOR A:OUT A        LD HL,#0555,(HL),#AA        LD HL,#02AA,(HL),#55        LD HL,#0555,(HL),#80,(HL),#AA        LD HL,#02AA,(HL),#55        LD BC,PW0:OUT E:LD H,A,L,A,(HL),#30        LD BC,HL
;>100uS
        LD A,87:DEC A:JR NZ,$-1WAITROM CALL WAIT:RET NC
;send read/reset command to ROM        LD A,#F0:CALL WRBYTE
;>10uS        LD A,9:DEC A:JR NZ,$-1        SCF        RET
;---------------------------------------;wait for ROMWAIT    CALL RDBYTE:LD D,A        CALL RDBYTE:XOR D
;no toggle - end        BIT 6,A:RET Z;if toggle and error bit toggles - repeat reading        BIT 5,A:JR NZ,WAIT
;toggle, error bit is set - error        BIT 5,D:JR Z,WAIT
;otherwise - just toggle, wait more        SCF        RET
;---------------------------------------
RDBYTE  PUSH BC:LD BC,PW0:OUT E        POP BC:LD A,(BC)        RETWRBYTE  PUSH BC:LD BC,PW0:OUT E        POP BC:LD (BC),A        RET;---------------------------------------
PRWOW   LD A,1:JP WLDRRESB   LD A,2:JP WLDPRSRW   LD A,3:JP WLDPRIAT   EXA:LD A,4:JP WLDYN      EXA:LD A,8:JP WLDNORK    EXA:LD A,10:JP WLDTURBO   LD B,0,A,14:JP WLD;TURBOD  LD B,#FF,A,14:JP WLDGEDPL   LD A,15:JP WLD;-------UPPP    LD A,17:JP WLDDWWW    LD A,18:JP WLDENKE    LD A,22:JP WLDESC     LD A,23:JP WLD
USPO    LD A,46:JP WLDNUSP    LD A,47:JP WLD;-------LOAD512 LD A,48:JP WLDTENTRY2 LD A,51:JP WLD;-------MNGCVPL EXA:LD A,65:JP WLD;-------GIPAGPL LD A,50:JP WLD;---------------------------------------PLWND   DB %1001 0111,0        DB 255,255;     X,Y        DB WNDW+2,WNDH+2; W,H        DB %01111111;PAPER+INK        DB 0        DW #0000;    BUFFER        DB 2,0;      LINES        DW ROMHEAD        DW 0        DW 0ROMHEAD DB #E,9,"ROM WRITER v0.3",0;-------FIWND   DB %1001 0111,0        DB 255,255;  X,Y        DB 22+2,4+2; W,H        DB %01101110;PAPER+INK        DB 0        DW #0000;    BUFFER        DB 2,0;      LINES        DW 0        DW 0        DW STARTQSTARTQ  DB "START PROGRAMMING ROM?",0;-------MEWND   DB %1000 0110,0        DB 255,255;  X,Y        DB 16+2,1+2; W,H        DB %01011111;PAPER+INK        DB 0        DW #0000;    BUFFER        DB 2,0;      LINES;-------ERWND   DB %1000 0110,0        DB 255,255;    X,Y        DB 30+2,1+2; W,H        DB %00101111;PAPER+INK        DB 0        DW #0000;    BUFFER        DB 0,0;      LINES;-------ME00    DB "ERASING..."ME02    DB "Writing..."ME03    DB "Verifying..."ME04    DB "DONE!!!"ER00    DB "Verify Failed!!!"ER01    DB "Writing Failed!";-------TXT01   DB "File:"TXT03   DB "Size:"TXT04   DB "#0000"TXT05   DB " 0000";-------SR00    DB "Block: ROM #00 ",#18,#19SR01    DB "Block: RAM #F8 ",#18,#19;---------------------------------------ERR     NOPROMmode NOP
PAG     NOPBLKN    DS 2;-------DAHL    DS 2DADE    DS 2
ESTAT   NOP;-------ENTRY   DS ENTRYL;---------------------------------------