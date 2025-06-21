
		DEVICE	ZXSPECTRUM48

A = 10
B = 20
C = 40

	MACRO TEST A, B, C
		
		nop

	IF A == 10 || B == 20 || C == 40
		DISPLAY "yes"
	ELSEIF C == 15
		DISPLAY "strange"
	ELSE
		DISPLAY "no"
	ENDIF

		nop

	ENDM

		ORG 32768

		TEST 10,20,40

		nop