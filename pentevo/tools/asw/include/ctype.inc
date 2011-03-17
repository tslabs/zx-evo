		save
		listing off   ; kein Listing ber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.40 - Datei CTYPE.INC                                              *
;*   								            *
;*   Sinn : enth„lt Funktionen zur Untersuchung von Zeichen                 *
;* 									    *
;*   letzte Žnderungen : 20. 3.1993                                         *
;*                       11. 9.1993 Meldung nur im 1.Pass                   *
;*                       19. 2.1994 Funktionen vervollst„ndigt,             *
;*                                  isascii dem C-Standard angepaát         *
;*									    *
;* ACHTUNG!  isalpha bercksichtigt keine landesspezifischen Sonderzeichen! *
;*									    *
;****************************************************************************

		ifndef   ctypeinc       ; verhindert Mehrfacheinbindung

ctypeinc        equ      1

		if	 mompass=1
		 message "Standardzeichenfunktionen (C) 1993 Alfred Arnold"
		endif

;----------------------------------------------------------------------------
; liefert TRUE, falls das Argument eine Ziffer ist:

isdigit         function ch,(ch>='0')&&(ch<='9')

;----------------------------------------------------------------------------
; liefert TRUE, falls das Argument eine hexadezimale Ziffer ist:

isxdigit	function ch,(isdigit(ch))||((toupper(ch)>='A')&&(toupper(ch)<='F'))

;----------------------------------------------------------------------------
; liefert TRUE, falls das Argument im Bereich normaler ASCII-Zeichen liegt:

isascii         function ch,(ch>=0)&&(ch<128)

;----------------------------------------------------------------------------
; liefert TRUE, falls das Argument ein Groábuchstabe ist:

isupper		function ch,(ch>='A')&&(ch<='Z')

;----------------------------------------------------------------------------
; liefert TRUE, falls das Argument ein Kleinbuchstabe ist:

islower		function ch,(ch>='a')&&(ch<='z')

;----------------------------------------------------------------------------
; liefert TRUE, falls das Argument ein Buchstabe ist:

isalpha         function ch,(toupper(ch)>='A')&&(toupper(ch)<='Z')

;----------------------------------------------------------------------------
; liefert TRUE, falls das Argument Ziffer oder Buchstabe ist:

isalnum         function ch,isdigit(ch)||isalpha(ch)

;----------------------------------------------------------------------------
; liefert TRUE, falls das Argument ein Leerzeichen ist:
; Hinweis: 11=vertikaler Tabulator

isspace		function ch,(ch=' ')||(ch=12)||(ch='\n')||(ch='\r')||(ch='\t')||(ch=11)

;----------------------------------------------------------------------------
; liefert TRUE, falls das Argument ein druckbares Zeichen ist:
; strenggenommen máte man DEL (127) ausnehmen, aber auf dem PC ist das ein
; druckbares Zeichen

isprint		function ch,(ch>31)&&(ch<255)

;----------------------------------------------------------------------------
; liefert TRUE, falls das Argument ein Steuerzeichen ist:

iscntrl		function ch,~~isprint(ch)

;----------------------------------------------------------------------------
; liefert TRUE, falls das Argument ein druckbares, sichtbares Zeichen ist:

isgraph		function ch,isprint(ch)&&(~~isspace(ch))

;----------------------------------------------------------------------------
; liefert TRUE, falls das Argument ein Sonderzeichen ist:

ispunct		function ch,isprint(ch)&&(~~isspace(ch))&&(~~isalnum(ch))

		endif

		restore                 ; wieder erlauben


