;CHARDEMO.ASM
;Demo program for the char directive
	DB "AAA\n\r"
#char 'A','a'
	DB "AAA\n\r"
#char 'A','A'
	DB "AAA\n\r"
#char 65,'A'
	DB "AAA\n\r"
#char 65,65
	DB "AAA\n\r"
#char 'A','a'
;End of the file CHARDEMO.ASM
