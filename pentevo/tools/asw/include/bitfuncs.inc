                save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.39 - Datei BITFUNCS.INC                                           *
;*   								            *
;*   Sinn : enthÑlt Funktionen zur Bitmanipulation                          *
;* 									    *
;*   letzte énderungen : 30. 5.1993                                         *
;*                       11. 9.1993 Meldung nur im 1.Pass                   *
;*									    *
;****************************************************************************

		ifndef   bitfuncsinc    ; verhindert Mehrfacheinbindung

bitfuncsinc     equ      1

		if	 mompass=1
		 message "Standardbitfunktionen (C) 1993 Alfred Arnold"
		endif

;----------------------------------------------------------------------------
; erstmal einige Unterfunktionen:

; liefert eine Maske in der ab 'start' 'bits' Bit gesetzt sind, z.B. zum
; Ausmaskieren einzelner Bits:

mask            function start,bits,((1<<bits)-1)<<start

; das gleiche invers zum Lîschen von Bitgruppen:

invmask         function start,bits,~mask(start,bits)

; liefert die Bits 'start' bis 'start'+'bits'-1 aus 'x':

cutout          function x,start,bits,x&mask(start,bits)

;----------------------------------------------------------------------------
; einige hÑufig gebrauchte Sondermasken:

; oberes Byte eines 16-Bit-Wortes:

hi              function x,(x>>8)&255

; dito unteres Byte:

lo              function x,x&255

; obere HÑlfte aus einem 32-Bit-Wort:

hiword          function x,(x>>16)&65535

; dito untere HÑlfte:

loword          function x,x&65535

; Boolean-Funktionen, ob eine Zahl gerade oder ungerade ist:

odd             function x,(x&1)=1
even            function x,(x&1)=0

; liefert Bit 'n' aus 'x':

getbit          function x,n,(x>>n)&1

;----------------------------------------------------------------------------
; Schiebefunktionen:

; Schieben eines 'size'-Bit-Wortes um 'n' Stellen nach links oder rechts:

shln            function x,size,n,(x<<n)&mask(0,size)
shrn            function x,size,n,(x>>n)&mask(0,size-n)

; Rotation eines 'size'-Bit-Wortes um 'n' Stellen nach links oder rechts;
; der erste Term lÑ·t die restlichen Bits unverÑndert und kann bei Bedarf
; gelîscht werden:

rotln           function x,size,n,cutout(x,size,32-size)|shln(x,size,n)|shrn(x,size,size-n)
rotrn           function x,size,n,cutout(x,size,32-size)|shrn(x,size,n)|shln(x,size,size-n)

                endif

                restore                 ; wieder erlauben

