wcPage0 EQU     #6000
wcPage1 EQU     #6001
wcPage2 EQU     #6002
wcPage3 EQU     #6003
wcABT   EQU     #6004
wcENT   EQU     #6005
wcTNM   EQU     #6009
wcIN    EQU     #6006
		org		0x7e00
        DS      16,0
        DB      "WildCommanderMDL"
        DB      #03
        DB      0

        DB      2
        DB      0

        DB      0,(codend-0x8000)/512
		DB		1,32
        DUP     4
        DB      0,0
        EDUP 

        DS      16,0

        DB      "LBM"
        DB      "ILB"
		DB		"IFF"
        DUP     29
        DB      0,0,0
        EDUP 
        DB      0
        DW      0x0000,0x0008

        DB      "IFF/ILBM viewer v.0.3 "
        DS      10,0

        DB      0

        DS      314,0
MNGC_PL EQU     0
PRWOW   EQU     1
DMA_PL  EQU     13
TURBOPL EQU     14
GEDPL   EQU     15
LOAD512 EQU     48
MNGV_PL EQU     64
GVmod   EQU     66
