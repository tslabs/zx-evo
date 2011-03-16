; for AS31 assembler


;equs
	;size & origin of queue
	.equ	Queue_size,	8	;must be a power of 2
	.equ	Queue_org,	0x58	;must have LSB's 0

	;timer1 const for waiting response (wait 47660*3 cycles)
	.equ	R_const_h,	0x45	; -47660 = $45D4
	.equ	R_const_l,	0xD4


;regs

;	set 0 regs - for main prog

;	set 1 regs - for receive/send prog
;		r0 - ptr: save/read PSWs with C bit

;	set 2 regs - for tim1/int1 progs (ammy)

;hardware pins
	.equ	PCdata,		0xB5	;P3.5
	.equ	PCklk,		0xB2	;P3.2	PCklk must be /INT0

	.equ	AMYdata,	0xB3	;P3.3	AMYdata must be /INT1
	.equ	AMYklk,		0xB0	;P3.0
	.equ	AMYres,		0xB1	;P3.1

;bits
	;tranceiver mode (0 - receive, 1 - send)
	.equ	TC_mode,	0x00

	;error during sending or receiving (1 - error)
	.equ	TC_error,	0x01


	;change led status at 0xFA (means 0xFA is response to 0xED)
	.equ	PS_setLED,	0x02

	;actual capslock status	(1 - turned on, isn't it? :)
	.equ	PS_capslock,	0x03

	;if we're skipping pckbd bytes (useless keys like pause)
	.equ	PS_doskip,	0x04

	;if we've received 0xE0 modifier
	.equ	PS_E0,		0x05

	;if we've received 0xF0 - release
	.equ	PS_release,	0x06

	;last press scancode received E0 flag
	.equ	PS_last_E0,	0x07

	;ramiga tracking
	.equ	KYb_ramiga,	0x08

	;ctrl-ramiga-lamiga pressed simultaneously
	.equ	AMY_reset,	0x09

	;if we froze pckbd
	.equ	TC_freeze,	0x0A

	;indicating we've got response on AMYdata line
	.equ	AMY_gotresp,	0x0B

	;indicating we've had to resync
	.equ	AMY_resync,	0x0C



	;for bits doublebuffering during sending
	.equ	TC_dbuff_i,	0x7F	;7th bit in TC_dbuff_y       bIt
;bytes
	.equ	TC_dbuff_y,	0x2F	;in bit addressable area ^^^ bYte



	.equ	TC_buf_0,	0x30	;store here PSWs with carry bit
	.equ	TC_buf_1,	0x31
	.equ	TC_buf_2,	0x32
	.equ	TC_buf_3,	0x33
	.equ	TC_buf_4,	0x34
	.equ	TC_buf_5,	0x35
	.equ	TC_buf_6,	0x36
	.equ	TC_buf_7,	0x37
	.equ	TC_buf_8,	0x38
	.equ	TC_buf_9,	0x39
	.equ	TC_buf_A,	0x3A

	.equ	TC_byte,	0x3B	;byte received
	.equ	TC_lastsent,	0x3C	;byte sent last time

	.equ	PS_last,	0x3D	;last press scancode received
	.equ	PS_skipcount,	0x3E	;skip counter

	.equ	Q_in,		0x3F	;a little (8 byte) queue for
	.equ	Q_out,		0x40	;amiga scancodes
	.equ	Q_num,		0x41


;for keys filtering & ctrl-lamiga-ramiga tracking
	.equ	KY_ctrl,	0x42
	.equ	KY_lamiga,	0x43
	.equ	KY_rshift,	0x44
	.equ	KY_m,		0x45
	.equ	KY_curup,	0x46
	.equ	KY_curdn,	0x47
	.equ	KY_curlf,	0x48
	.equ	KY_currt,	0x49
	.equ	KY_bslsh,	0x4A

	;counting 3 timer ints before out-of-sync
	.equ	AMY_intcount,	0x4B

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

		.org	0
		ajmp	ProgBeg

		.org	0x03
		push	PSW		;2 save psw... don't know why :)
		jb	TC_mode,I0_send	;2 if sending
		sjmp	I0_receive	;2 then go to further receiving

		.org	0x0b
		ajmp	irq_TMR0

		.org	0x13
		setb	AMY_gotresp	;we're here - GOT RESPONSE !!!!!
		clr	TR1	;stop tmr1 & clr its int flag
		clr	TF1
		reti			;main prog should wait for AMYdata=1

		.org	0x1b
		ajmp	irq_TMR1

		.org	0x23
;		reti	;serial

I0_receive:	;6us already spent
		mov	C,PCdata	;1	save databit (at 7th us)

		setb	TR0		;1	start timer0 for timeouting

		clr	RS1		;1	turn to reg1 set
		setb	RS0		;1

		mov	@r0,PSW		;2	save bit
		inc	r0		;1

		cjne	r0,#(TC_buf_A+1),I0_end	;2	if not end

	;if all received - start thinking :)

		clr	PCklk	;handshake/freeze further activity

		push	ACC	;then save accumulator

		clr	TR0	;turn off & reinit timer0
		clr	TF0	;clr overflow bit
		clr	A
		mov	TL0,A
		mov	TH0,A

	;first of all check errs - startbit must be 0, stopbit must be 1
	;parity must be a complement of P bit in PSW

		clr	TC_error

		mov	r0,#TC_buf_0
		mov	A,@r0
		jb	ACC.7,I0_rec_error	;if startbit not 0
		inc	r0

I0_rec_read8bits:		;read in acc all bits
		mov	PSW,@r0
		rrc	a
		inc	r0
		cjne	r0,#TC_buf_9,I0_rec_read8bits

		mov	TC_byte,A	;save received byte

		mov	a,PSW
		rr	a		;ACC.7 = calculated parity

		xrl	a,@r0		;ACC.7 xored with received parity
		inc	r0

		jnb	ACC.7,I0_rec_error	;if parities equal - err
						;mcs51 - even parity
						;ps/2 - odd parity

		mov	a,@r0
		jb	ACC.7,I0_rec_noerror	;if stopbit =1
I0_rec_error:
		setb	TC_error
I0_rec_noerror:

		acall	Receive_Action	;mustnot release PCklk!

I0_preend:
		mov	r0,#TC_buf_0		;restore r0

		pop	ACC

		jb	TC_freeze,I0_norelease	;if we're frozen
		setb	PCklk	;release PCklk
I0_norelease:

		clr	IE0	;clear ext int flag
I0_end:
		pop	PSW		;2
		reti			;2   summary 19us/receive, 19us/send



I0_send:	;4us already spent
		mov	C,TC_dbuff_i	;1	for speedup -
		mov	PCdata,C	;2	doublebuffering for bits :)

		setb	TR0		;1

		clr	RS1		;1	turn to reg1 set
		setb	RS0		;1

		inc	r0		;1

		mov	TC_dbuff_y,@r0	;2	store bit with whole byte

		cjne	r0,#(TC_buf_A+1),I0_end	;2

		clr	PCklk	;handshake/freeze further activity

		;jb	PCdata,TC_snd_error	;because too slow

		push	ACC	;then save accumulator

		clr	TR0	;turn off & reinit timer0
		clr	TF0	;clr overflow bit
		clr	A
		mov	TL0,A
		mov	TH0,A

		clr	TC_error	;assume no error detection here yet
I0_snd_noerror:
		acall	Send_Action

		sjmp	I0_preend






irq_TMR0:	;we are here - means PCklk timeout (not synced)
		;break transfer & call (Send|Receive)_Action with error flag
		push	PSW
		push	ACC

		mov	PSW,0b00001000	;switch to bank 1 regs

		clr	PCklk	;pulldown PCklk

		clr	TR0	;turn off & reinit timer0
		clr	TF0	;clr overflow bit
		clr	A
		mov	TL0,A
		mov	TH0,A

		setb	TC_error	;set error flag

		jnb	TC_mode,I0_rec_noerror	;then continue
		sjmp	I0_snd_noerror








irq_TMR1:
		djnz	AMY_intcount,T1_end
		mov	AMY_intcount,#3
;if we're here - means out of sync

		push	PSW

		clr	RS0	;reg 2 set
		setb	RS1

		setb	AMY_resync	;we've really out of sync

		clr	AMYdata		;clock out 1's (low level)

		mov	r2,#9
T1_w0:		djnz	r2,T1_w0

		clr	AMYklk		;after 20us send clk falling edge

		mov	2,#9
T1_w1:		djnz	r2,T1_w1

		setb	AMYklk		;clk rising edge

		mov	r2,#9
T1_w2:		djnz	r2,T1_w2

		setb	AMYdata		;release AMYdata for possible resp

		pop	PSW

		clr	IE1		;AMYdata was 0 - but no int

T1_end:
		clr	TR1		;stop timer
		mov	TH1,#R_const_h	;reload timer
		mov	TL1,#R_const_l
		setb	TR1		;start timer again

		reti















ProgBeg:

		clr	A		;clear all mem
		mov	r0,#0x80
Clr_mem:
		mov	@r0,A
		djnz	r0,Clr_mem

		mov	SP,#0x5F	;32 bytes for stack


		mov	PSW,#0b00001000		;set bank 1 regs
		mov	r0,#TC_buf_0		;set r0 for sending/receiving
		clr	RS0			;set bank 0 regs


		mov	P3,#0b11111011	;PCklk = 0
		mov	P1,#0xFF

		;hardware configuration
		mov	IP,#0b00000011	;receiving has higher priority
					;on int0,timer0

		mov	TMOD,#0b00010001	;all timers in mode 1 (16bit)

		mov	TCON,#0b00000101	;ints by front, timers stopd

		mov	IE,#0b10000010		;ints

		mov	Q_in,#Queue_org	;8 bytes queue
		mov	Q_out,#Queue_org
;		mov	Q_num,#0x00		;assume mem is cleared

;		clr	PS_setLED	;some transcoder inits
;		clr	PS_capslock
;		clr	PS_doskip
;		clr	PS_E0
;		clr	PS_release
;		clr	KYb_ramiga
;		clr	AMY_reset
;		clr	TC_freeze

		setb	PS_last_E0	;E0 00 - impossible scancode
;		mov	PS_last,A

;		mov	r0,#KY_ctrl
;		mov	r1,#9
;klrkeys:
;		mov	@r0,a
;		inc	r0
;		djnz	r1,klrkeys

		mov	TL0,A		;assume A is still cleared
		mov	TH0,A

		mov	a,#0xFF		;reset code
		acall	RA_normsend	;it will prepare our byte for sending
					;also will clear PCdata & set TC_mode
		clr	IE0
		setb	EX0	;enable ext int

		setb	PCklk	;release kbd
		;here pckbd should start operating interrupts-driven only...


		;now sync up with amykbd

Amykbd_poweron:
		mov	TH1,#R_const_h	;preload timer
		mov	TL1,#R_const_l
		mov	AMY_intcount,#3	;count 3 ints

		clr	IE1

		setb	ET1
		setb	TR1	;start timer!

		setb	EX1	;enable waiting for response
Wait_pwon_sync:
		jnb	AMY_gotresp,Wait_pwon_sync	;waitin' sync

		mov	r7,#0xFD		;initiate powerup keystream
		acall	SendByte		;send r7 to Ammy

		mov	r7,#0xFE		;terminate powerup keystream
		acall	SendByte



Main:
		jnb	AMY_reset,M_norst	;ctrl-lamiga-ramiga pressed?

		clr	EX1

		setb	AMYdata
		clr	AMYklk		;pulldown klk & reset
		clr	AMYres


		mov	dptr,#0		;approx .5 sec
M_reswait:
		nop
		nop
		inc	dptr
		mov	a,dpl
		orl	a,dph
		jnz	M_reswait


M_relwait:	jb	AMY_reset,M_relwait	;wait till release

		setb	AMYres		;release reset
		setb	AMYklk

		sjmp	Amykbd_poweron	;go for resyncing & so on...



M_norst:
		mov	A,Q_num			;is there byte in queue?
		jz	Main

		mov	r0,Q_out	;take off one byte from queue
		mov	a,@r0
		mov	r7,a
		mov	a,r0
		inc	a
		anl	a,#(Queue_size-1)
		orl	a,#Queue_org
		mov	Q_out,a
		dec	Q_num

		jnb	TC_freeze,M_nofr	;if frozen, warm up it!

		clr	TC_freeze	;no more cold
		setb	PCklk		;release pckbd

M_nofr:

		acall	SendByte
		sjmp	Main









SendByte:	;sends r7 to Ammy

		mov	A,r7
		acall	ClockByte	;just clocks ACC out

SB_waitresp:	jnb	AMY_gotresp,SB_waitresp
		jb	AMY_resync,SB_resend		;is all ok?
		ret
SB_resend:
		mov	A,#0xF9		;send 'last scancode bad'
		acall	ClockByte

SB_wrbad:	jnb	AMY_gotresp,SB_wrbad
		jb	AMY_resync,SB_resend	;if failed - try again...

		sjmp	SendByte








ClockByte:	;clocks ACC out

		mov	TH1,#R_const_h	;preload timer (already stopped)
		mov	TL1,#R_const_l
		mov	AMY_intcount,#3	;for counting 3 ints

		clr	AMY_gotresp
		clr	AMY_resync


		rl	A	;6-5-4-3-2-1-0-7 bit order
		cpl	A	;1 - low level, 0 - high level

		clr	EX1	;disable int1

CB_waitdata:	jnb	AMYdata,CB_waitdata	;wait for end of response

		mov	r0,#8	;send 8 bits
CB_bit:
		rlc	A
		mov	AMYdata,C

		mov	r1,#9
CB_w0:		djnz	r1,CB_w0

		clr	AMYklk

		mov	r1,#9
CB_w1:		djnz	r1,CB_w1

		setb	AMYklk

		mov	r1,#7
CB_w2:		djnz	r1,CB_w2

		djnz	r0,CB_bit
		setb	AMYdata

		clr	IE1	;start waiting response...
		setb	TR1
		setb	EX1

		ret
















Receive_Action:	;after byte receiving

		jb	TC_error,RA_plzresend	;if error - cmd resend

		mov	a,TC_byte

		cjne	a,#0xFA,RA_noack

;RA_ack:	;if acknowledge byte received

		jb	PS_setLED,RA_caps
		ret
RA_caps:
		clr	PS_setLED

		clr	a
		mov	C,PS_capslock
		mov	ACC.2,C

RA_normsend:
		mov	TC_lastsent,a
RA_specsend:
		setb	TC_mode		;1 means send mode

		acall	Prep2Send	;prepare byte for sending

		clr	PCdata		;tell kbd we want to send smth
		ret

RA_noack:
		cjne	a,#0xFE,RA_noresend

;RA_resend:	;resend last byte sent, if kbd asks

		mov	a,TC_lastsent
		sjmp	RA_specsend



RA_plzresend:	;if bad byte received, tell kbd to resend byte

		mov	A,#0xFE
		sjmp	RA_normsend

RA_noresend:
		cjne	a,#0xAA,RA_transcode
		ret




RA_transcode:	;transcoder starts here

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

		jnb	PS_doskip,TR_noskip	;skipping (pause key)

		djnz	PS_skipcount,TR_ret
		clr	PS_doskip
TR_ret:
		ret

TR_noskip:
		cjne	a,#0xE0,TR_noE0
;TR_E0:					;if we've got 0xE0 modifier
		setb	PS_E0
		ret
TR_noE0:
		cjne	a,#0xF0,TR_norelease
;TR_release:				;if we've got 0xF0 release flag
		setb	PS_release
		ret
TR_norelease:


		cjne	A,#0xE1,TR_nopause	;if we've got 0xE1 - first
						;byte of pause sequence
;TR_pause:	;pause pressed
		setb	PS_doskip	;skip next 7 bytes
		mov	PS_skipcount,#7
		ret
TR_nopause:

		;checking for repeat - only last press scancode can repeat
		cjne	A,PS_last,TR_wrk	;if scancodes differ

		jb	PS_E0,TR_rep1
		jb	PS_last_E0,TR_wrk
		sjmp	TR_rep2
TR_rep1:
		jnb	PS_last_E0,TR_wrk
TR_rep2:
		jnb	PS_release,TR_end	;if press - just ignore

		setb	PS_last_E0	;else set last scancode to E0 00
		mov	PS_last,#0

TR_wrk:
		jb	PS_release,TR_wrk1

		mov	PS_last,A
		mov	C,PS_E0
		mov	PS_last_E0,C
TR_wrk1:

		push	DPL
		push	DPH

		mov	DPTR,#Trans_table
		jnb	PS_E0,TR_noinc
		inc	DPH		;other part of table if was 0xE0
TR_noinc:

		movc	a,@a+dptr	;probably the main part of all prog

		jb	ACC.7,TR_special	;bit7 means special, i.e.
						;several scancodes, etc.

		mov	C,PS_release		;set press/release bit
		mov	ACC.7,C		

TR_sendend:
		acall	Send2Ammy	;ACC goes as scancode to Amiga

TR_popend:
		pop	DPH
		pop	DPL
TR_end:
		clr	PS_release
		clr	PS_E0
		ret
TR_special:
;$FF means no scancode generating, $8x - special

;	$81 - CAPS LOCK
;	$83 - LAMIGA-M
;	$84 - RSHIFT-CURSORLEFT
;	$85 - RSHIFT-CURSORRIGHT
;	$86 - RSHIFT-CURSORUP
;	$87 - RSHIFT-CURSORDOWN

		cjne	A,#0xFF,TR_spec2
		sjmp	TR_popend
TR_spec2:

		cjne	A,#0x81,TR_nocaps
;TR_caps:	;capslock pressed
		jb	PS_release,TR_popend	;capslock released - nothing

		mov	C,PS_capslock	;get capslock bit

		mov	A,#0x62		;prepare amiga capslock scancode
		mov	ACC.7,C		;if capslock was off - press
					;if capslock was on - release

		cpl	C		;invert capslock state
		mov	PS_capslock,C

		acall	Send2Ammy	;send it to amiga

		mov	A,#0xED		;set LED on pckbd
		setb	PS_setLED
		acall	RA_normsend	;prepare it for sending
		sjmp	TR_popend
TR_nocaps:

		cjne	A,#0x83,TR_nolamm
;TR_lamm:	;leftamiga-m
		mov	dptr,#0x6637	;0x66, 0x37 - lamiga-m
		sjmp	TR_doublesend
TR_nolamm:

		cjne	A,#0x84,TR_norsl
;TR_rsl:	;rshift-cursorleft
		mov	dptr,#0x614F
		sjmp	TR_doublesend
TR_norsl:

		cjne	A,#0x85,TR_norsr
;TR_rsr:	;rshift-cursorright
		mov	dptr,#0x614E
		sjmp	TR_doublesend
TR_norsr:

		cjne	A,#0x86,TR_norsu
;TR_rsu:	;rshift-cursorup
		mov	dptr,#0x614C
		sjmp	TR_doublesend
TR_norsu:

		cjne	A,#0x87,TR_popend
;TR_rsd:	;rshift-cursordown
		mov	dptr,#0x614D

TR_doublesend:
		jb	PS_release,TR_ds_rel
		mov	a,DPH
		acall	Send2Ammy
		mov	a,DPL
		sjmp	TR_sendend
TR_ds_rel:
		mov	a,DPL
		setb	ACC.7
		acall	Send2Ammy
		mov	a,DPH
		setb	ACC.7
		sjmp	TR_sendend










Send2Ammy:	;1st of all, track ramiga & filter some multiple keys

		mov	r1,A

		cjne	A,#0x67,SA_noramp
		setb	KYb_ramiga
SA_noramp:
		cjne	A,#0xE7,SA_noramr
		clr	KYb_ramiga
SA_noramr:

		clr	ACC.7

		cjne	A,#0x63,SA_noctrl
		mov	r0,#KY_ctrl
		sjmp	SA_filter
SA_noctrl:

		cjne	A,#0x66,SA_nolamiga
		mov	r0,#KY_lamiga
		sjmp	SA_filter
SA_nolamiga:

		cjne	A,#0x61,SA_norshift
		mov	r0,#KY_rshift
		sjmp	SA_filter
SA_norshift:

		cjne	A,#0x37,SA_nom
		mov	r0,#KY_m
		sjmp	SA_filter
SA_nom:

		cjne	A,#0x4C,SA_nocurup
		mov	r0,#KY_curup
		sjmp	SA_filter
SA_nocurup:

		cjne	A,#0x4D,SA_nocurdn
		mov	r0,#KY_curdn
		sjmp	SA_filter
SA_nocurdn:

		cjne	A,#0x4F,SA_nocurlf
		mov	r0,#KY_curlf
		sjmp	SA_filter
SA_nocurlf:

		cjne	A,#0x4E,SA_nocurrt
		mov	r0,#KY_currt
		sjmp	SA_filter
SA_nocurrt:

		cjne	A,#0x0D,SA_cont
		mov	r0,#KY_bslsh

SA_filter:
		mov	A,r1
		jb	ACC.7,SA_frel
		mov	A,@r0
		inc	@r0
		jz	SA_cont
		ret
SA_frel:
		dec	@r0
		mov	A,@r0
		jz	SA_cont
		ret

SA_cont:
		;reset check

		clr	AMY_reset
		jnb	KYb_ramiga,SA_noreset
		mov	A,KY_lamiga
		jz	SA_noreset
		mov	A,KY_ctrl
		jz	SA_noreset
		setb	AMY_reset
SA_noreset:


		;here store amiga scancode in queue & stall if queue full

		mov	a,Q_num		;check for queue overflow

							;if we write last byte
		cjne	A,#(Queue_size-1),SA_qustore	;we should freeze kbd

		setb	TC_freeze		

SA_qustore:
		inc	Q_num
		mov	r0,Q_in
		mov	a,r1
		mov	@r0,a
		mov	a,r0
		inc	a
		anl	a,#(Queue_size-1)
		orl	a,#Queue_org
		mov	Q_in,a




		ret





Send_Action:	;after byte sending

		clr	TC_mode		;1st of all, set mode to RECEIVING!

		ret	;nothing to do here yet :(








Prep2Send:	;input in ACC !
		mov	r0,#TC_buf_0

p2s_loop0:
		rr	a
		mov	@r0,a
		inc	r0
		cjne	r0,#TC_buf_8,p2s_loop0

		mov	TC_dbuff_y,TC_buf_0

		;parity
		mov	C,PSW.0	;carry=parity
		cpl	C	;ps/2 has inverted parity
		mov	@r0,PSW		;save bit
		inc	r0

		mov	@r0,#0x80	;stopbit is 1
		inc	r0
		mov	@r0,#0x80	;extra stop bit (kbd must pull data)

		ret



Trans_table:	;general table for transcoding


;$FF means no scancode generating, $8x - special

;	$81 - CAPS LOCK
;	$83 - LAMIGA-M
;	$84 - RSHIFT-CURSORLEFT   (CTRL-CURSORUP)
;	$85 - RSHIFT-CURSORRIGHT  (CTRL-CURSORDOWN)
;	$86 - RSHIFT-CURSORUP
;	$87 - RSHIFT-CURSORDOWN

;non-E0 scantable starts

;               AMMY    ;        PC
	.db	0xFF	;	$00
	.db	0x58	;	$01	F9
	.db	0xFF	;	$02
	.db	0x54	;	$03	F5
	.db	0x52	;	$04	F3
	.db	0x50	;	$05	F1
	.db	0x51	;	$06	F2
	.db	0x2B	;	$07	F12 -> RIGHT BLANK
	.db	0xFF	;	$08
	.db	0x59	;	$09	F10
	.db	0x57	;	$0A	F8
	.db	0x55	;	$0B	F6
	.db	0x53	;	$0C	F4
	.db	0x42	;	$0D	TAB
	.db	0x00	;	$0E	~ aka '
	.db	0xFF	;	$0F

	.db	0xFF	;	$10
	.db	0x64	;	$11	LALT
	.db	0x60	;	$12	LSHIFT
	.db	0xFF	;	$13
	.db	0x63	;	$14	LCTRL -> CTRL
	.db	0x10	;	$15	Q
	.db	0x01	;	$16	1
	.db	0xFF	;	$17
	.db	0xFF	;	$18
	.db	0xFF	;	$19
	.db	0x31	;	$1A	Z
	.db	0x21	;	$1B	S
	.db	0x20	;	$1C	A
	.db	0x11	;	$1D	W
	.db	0x02	;	$1E	2
	.db	0xFF	;	$1F

	.db	0xFF	;	$20
	.db	0x33	;	$21	C
	.db	0x32	;	$22	X
	.db	0x22	;	$23	D
	.db	0x12	;	$24	E
	.db	0x04	;	$25	4
	.db	0x03	;	$26	3
	.db	0xFF	;	$27
	.db	0xFF	;	$28
	.db	0x40	;	$29	SPACE
	.db	0x34	;	$2A	V
	.db	0x23	;	$2B	F
	.db	0x14	;	$2C	T
	.db	0x13	;	$2D	R
	.db	0x05	;	$2E	5
	.db	0xFF	;	$2F

	.db	0xFF	;	$30
	.db	0x36	;	$31	N
	.db	0x35	;	$32	B
	.db	0x25	;	$33	H
	.db	0x24	;	$34	G
	.db	0x15	;	$35	Y
	.db	0x06	;	$36	6
	.db	0xFF	;	$37
	.db	0xFF	;	$38
	.db	0xFF	;	$39
	.db	0x37	;	$3A	M
	.db	0x26	;	$3B	J
	.db	0x16	;	$3C	U
	.db	0x07	;	$3D	7
	.db	0x08	;	$3E	8
	.db	0xFF	;	$3F

	.db	0xFF	;	$40
	.db	0x38	;	$41	< aka ,
	.db	0x27	;	$42	K
	.db	0x17	;	$43	I
	.db	0x18	;	$44	O
	.db	0x0A	;	$45	0
	.db	0x09	;	$46	9
	.db	0xFF	;	$47
	.db	0xFF	;	$48
	.db	0x39	;	$49	> aka .
	.db	0x3A	;	$4A	/ aka ?
	.db	0x28	;	$4B	L
	.db	0x29	;	$4C	; aka :
	.db	0x19	;	$4D	P
	.db	0x0B	;	$4E	- aka _
	.db	0xFF	;	$4F

	.db	0xFF	;	$50
	.db	0xFF	;	$51
	.db	0x2A	;	$52	" aka '
	.db	0xFF	;	$53
	.db	0x1A	;	$54	[
	.db	0x0C	;	$55	+ aka =
	.db	0xFF	;	$56
	.db	0xFF	;	$57
	.db	0x81	;	$58	CAPS LOCK
	.db	0x61	;	$59	RSHIFT
	.db	0x44	;	$5A	ENTER
	.db	0x1B	;	$5B	]
	.db	0xFF	;	$5C
	.db	0x0D	;	$5D	\ aka |
	.db	0xFF	;	$5E
	.db	0xFF	;	$5F

	.db	0xFF	;	$60
	.db	0x0D	;	$61	\ aka |   -   same as $5D
	.db	0xFF	;	$62
	.db	0xFF	;	$63
	.db	0xFF	;	$64
	.db	0xFF	;	$65
	.db	0x41	;	$66	BACKSPACE
	.db	0xFF	;	$67
	.db	0xFF	;	$68
	.db	0x1D	;	$69	KEYPAD 1
	.db	0xFF	;	$6A
	.db	0x2D	;	$6B	KEYPAD 4
	.db	0x3D	;	$6C	KEYPAD 7
	.db	0xFF	;	$6D
	.db	0xFF	;	$6E
	.db	0xFF	;	$6F

	.db	0x0F	;	$70	KEYPAD 0
	.db	0x3C	;	$71	KEYPAD .
	.db	0x1E	;	$72	KEYPAD 2
	.db	0x2E	;	$73	KEYPAD 5
	.db	0x2F	;	$74	KEYPAD 6
	.db	0x3E	;	$75	KEYPAD 8
	.db	0x45	;	$76	ESC
	.db	0xFF	;	$77	NUM LOCK -> NOTHING
	.db	0x30	;	$78	F11 -> LEFT BLANK
	.db	0x5E	;	$79	KEYPAD +
	.db	0x1F	;	$7A	KEYPAD 3
	.db	0x4A	;	$7B	KEYPAD -
	.db	0x5D	;	$7C	KEYPAD *
	.db	0x3F	;	$7D	KEYPAD 9
	.db	0x5B	;	$7E	SCROLL LOCK -> KEYPAD )
	.db	0xFF	;	$7F

	.db	0xFF	;	$80
	.db	0xFF	;	$81
	.db	0xFF	;	$82
	.db	0x56	;	$83	F7
	.db	0xFF	;	$84
	.db	0xFF	;	$85
	.db	0xFF	;	$86
	.db	0xFF	;	$87
	.db	0xFF	;	$88
	.db	0xFF	;	$89
	.db	0xFF	;	$8A
	.db	0xFF	;	$8B
	.db	0xFF	;	$8C
	.db	0xFF	;	$8D
	.db	0xFF	;	$8E
	.db	0xFF	;	$8F

	.db	0xFF	;	$90
	.db	0xFF	;	$91
	.db	0xFF	;	$92
	.db	0xFF	;	$93
	.db	0xFF	;	$94
	.db	0xFF	;	$95
	.db	0xFF	;	$96
	.db	0xFF	;	$97
	.db	0xFF	;	$98
	.db	0xFF	;	$99
	.db	0xFF	;	$9A
	.db	0xFF	;	$9B
	.db	0xFF	;	$9C
	.db	0xFF	;	$9D
	.db	0xFF	;	$9E
	.db	0xFF	;	$9F

	.db	0xFF	;	$A0
	.db	0xFF	;	$A1
	.db	0xFF	;	$A2
	.db	0xFF	;	$A3
	.db	0xFF	;	$A4
	.db	0xFF	;	$A5
	.db	0xFF	;	$A6
	.db	0xFF	;	$A7
	.db	0xFF	;	$A8
	.db	0xFF	;	$A9
	.db	0xFF	;	$AA
	.db	0xFF	;	$AB
	.db	0xFF	;	$AC
	.db	0xFF	;	$AD
	.db	0xFF	;	$AE
	.db	0xFF	;	$AF

	.db	0xFF	;	$B0
	.db	0xFF	;	$B1
	.db	0xFF	;	$B2
	.db	0xFF	;	$B3
	.db	0xFF	;	$B4
	.db	0xFF	;	$B5
	.db	0xFF	;	$B6
	.db	0xFF	;	$B7
	.db	0xFF	;	$B8
	.db	0xFF	;	$B9
	.db	0xFF	;	$BA
	.db	0xFF	;	$BB
	.db	0xFF	;	$BC
	.db	0xFF	;	$BD
	.db	0xFF	;	$BE
	.db	0xFF	;	$BF

	.db	0xFF	;	$C0
	.db	0xFF	;	$C1
	.db	0xFF	;	$C2
	.db	0xFF	;	$C3
	.db	0xFF	;	$C4
	.db	0xFF	;	$C5
	.db	0xFF	;	$C6
	.db	0xFF	;	$C7
	.db	0xFF	;	$C8
	.db	0xFF	;	$C9
	.db	0xFF	;	$CA
	.db	0xFF	;	$CB
	.db	0xFF	;	$CC
	.db	0xFF	;	$CD
	.db	0xFF	;	$CE
	.db	0xFF	;	$CF

	.db	0xFF	;	$D0
	.db	0xFF	;	$D1
	.db	0xFF	;	$D2
	.db	0xFF	;	$D3
	.db	0xFF	;	$D4
	.db	0xFF	;	$D5
	.db	0xFF	;	$D6
	.db	0xFF	;	$D7
	.db	0xFF	;	$D8
	.db	0xFF	;	$D9
	.db	0xFF	;	$DA
	.db	0xFF	;	$DB
	.db	0xFF	;	$DC
	.db	0xFF	;	$DD
	.db	0xFF	;	$DE
	.db	0xFF	;	$DF

	.db	0xFF	;	$E0
	.db	0xFF	;	$E1
	.db	0xFF	;	$E2
	.db	0xFF	;	$E3
	.db	0xFF	;	$E4
	.db	0xFF	;	$E5
	.db	0xFF	;	$E6
	.db	0xFF	;	$E7
	.db	0xFF	;	$E8
	.db	0xFF	;	$E9
	.db	0xFF	;	$EA
	.db	0xFF	;	$EB
	.db	0xFF	;	$EC
	.db	0xFF	;	$ED
	.db	0xFF	;	$EE
	.db	0xFF	;	$EF

	.db	0xFF	;	$F0
	.db	0xFF	;	$F1
	.db	0xFF	;	$F2
	.db	0xFF	;	$F3
	.db	0xFF	;	$F4
	.db	0xFF	;	$F5
	.db	0xFF	;	$F6
	.db	0xFF	;	$F7
	.db	0xFF	;	$F8
	.db	0xFF	;	$F9
	.db	0xFF	;	$FA
	.db	0xFF	;	$FB
	.db	0xFF	;	$FC
	.db	0xFF	;	$FD
	.db	0xFF	;	$FE
	.db	0xFF	;	$FF

;non-E0 scantable ends

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;E0 scantable starts

;               AMMY    ;        PC
	.db	0xFF	;	$00
	.db	0xFF	;	$01
	.db	0xFF	;	$02
	.db	0xFF	;	$03
	.db	0xFF	;	$04
	.db	0xFF	;	$05
	.db	0xFF	;	$06
	.db	0xFF	;	$07
	.db	0xFF	;	$08
	.db	0xFF	;	$09
	.db	0xFF	;	$0A
	.db	0xFF	;	$0B
	.db	0xFF	;	$0C
	.db	0xFF	;	$0D
	.db	0xFF	;	$0E
	.db	0xFF	;	$0F

	.db	0xFF	;	$10
	.db	0x65	;	$11	RALT
	.db	0xFF	;	$12	1ST SCANCODE OF PRTSCR -> NOTHING
	.db	0xFF	;	$13
	.db	0x63	;	$14	RCTRL -> CTRL
	.db	0xFF	;	$15
	.db	0xFF	;	$16
	.db	0xFF	;	$17
	.db	0xFF	;	$18
	.db	0xFF	;	$19
	.db	0xFF	;	$1A
	.db	0xFF	;	$1B
	.db	0xFF	;	$1C
	.db	0xFF	;	$1D
	.db	0xFF	;	$1E
	.db	0x66	;	$1F	LWIN -> LAMIGA

	.db	0xFF	;	$20
	.db	0xFF	;	$21
	.db	0xFF	;	$22
	.db	0xFF	;	$23
	.db	0xFF	;	$24
	.db	0xFF	;	$25
	.db	0xFF	;	$26
	.db	0x67	;	$27	RWIN -> RAMIGA
	.db	0xFF	;	$28
	.db	0xFF	;	$29
	.db	0xFF	;	$2A
	.db	0xFF	;	$2B
	.db	0xFF	;	$2C
	.db	0xFF	;	$2D
	.db	0xFF	;	$2E
	.db	0x83	;	$2F	MENU -> LAMIGA-M (special)

	.db	0xFF	;	$30
	.db	0xFF	;	$31
	.db	0xFF	;	$32
	.db	0xFF	;	$33
	.db	0xFF	;	$34
	.db	0xFF	;	$35
	.db	0xFF	;	$36
	.db	0xFF	;	$37	POWER -> NOTHING
	.db	0xFF	;	$38
	.db	0xFF	;	$39
	.db	0xFF	;	$3A
	.db	0xFF	;	$3B
	.db	0xFF	;	$3C
	.db	0xFF	;	$3D
	.db	0xFF	;	$3E
	.db	0xFF	;	$3F	SLEEP -> NOTHING

	.db	0xFF	;	$40
	.db	0xFF	;	$41
	.db	0xFF	;	$42
	.db	0xFF	;	$43
	.db	0xFF	;	$44
	.db	0xFF	;	$45
	.db	0xFF	;	$46
	.db	0xFF	;	$47
	.db	0xFF	;	$48
	.db	0xFF	;	$49
	.db	0x5C	;	$4A	KEYPAD /
	.db	0xFF	;	$4B
	.db	0xFF	;	$4C
	.db	0xFF	;	$4D
	.db	0xFF	;	$4E
	.db	0xFF	;	$4F

	.db	0xFF	;	$50
	.db	0xFF	;	$51
	.db	0xFF	;	$52
	.db	0xFF	;	$53
	.db	0xFF	;	$54
	.db	0xFF	;	$55
	.db	0xFF	;	$56
	.db	0xFF	;	$57
	.db	0xFF	;	$58
	.db	0xFF	;	$59
	.db	0x43	;	$5A	KEYPAD ENTER
	.db	0xFF	;	$5B
	.db	0xFF	;	$5C
	.db	0xFF	;	$5D
	.db	0xFF	;	$5E	WAKE UP -> NOTHING
	.db	0xFF	;	$5F

	.db	0xFF	;	$60
	.db	0xFF	;	$61
	.db	0xFF	;	$62
	.db	0xFF	;	$63
	.db	0xFF	;	$64
	.db	0xFF	;	$65
	.db	0xFF	;	$66
	.db	0xFF	;	$67
	.db	0xFF	;	$68
	.db	0x85	;	$69	END -> RSHIFT-CURSORRIGHT
	.db	0xFF	;	$6A
	.db	0x4F	;	$6B	CURSORLEFT
	.db	0x84	;	$6C	HOME -> RSHIFT-CURSORLEFT
	.db	0xFF	;	$6D
	.db	0xFF	;	$6E
	.db	0xFF	;	$6F

	.db	0x5F	;	$70	INSERT -> HELP
	.db	0x46	;	$71	DELETE
	.db	0x4D	;	$72	CURSORDOWN
	.db	0xFF	;	$73
	.db	0x4E	;	$74	CURSORRIGHT
	.db	0x4C	;	$75	CURSORUP
	.db	0xFF	;	$76
	.db	0xFF	;	$77
	.db	0xFF	;	$78
	.db	0xFF	;	$79
	.db	0x87	;	$7A	PGDN -> RSHIFT-CURSORDOWN
	.db	0xFF	;	$7B
	.db	0x5A	;	$7C	2ND SCANCODE OF PTRSCR -> KEYPAD (
	.db	0x86	;	$7D	PGUP -> RSHIFT-CURSORUP
	.db	0xFF	;	$7E
	.db	0xFF	;	$7F

	.db	0xFF	;	$80
	.db	0xFF	;	$81
	.db	0xFF	;	$82
	.db	0xFF	;	$83
	.db	0xFF	;	$84
	.db	0xFF	;	$85
	.db	0xFF	;	$86
	.db	0xFF	;	$87
	.db	0xFF	;	$88
	.db	0xFF	;	$89
	.db	0xFF	;	$8A
	.db	0xFF	;	$8B
	.db	0xFF	;	$8C
	.db	0xFF	;	$8D
	.db	0xFF	;	$8E
	.db	0xFF	;	$8F

	.db	0xFF	;	$90
	.db	0xFF	;	$91
	.db	0xFF	;	$92
	.db	0xFF	;	$93
	.db	0xFF	;	$94
	.db	0xFF	;	$95
	.db	0xFF	;	$96
	.db	0xFF	;	$97
	.db	0xFF	;	$98
	.db	0xFF	;	$99
	.db	0xFF	;	$9A
	.db	0xFF	;	$9B
	.db	0xFF	;	$9C
	.db	0xFF	;	$9D
	.db	0xFF	;	$9E
	.db	0xFF	;	$9F

	.db	0xFF	;	$A0
	.db	0xFF	;	$A1
	.db	0xFF	;	$A2
	.db	0xFF	;	$A3
	.db	0xFF	;	$A4
	.db	0xFF	;	$A5
	.db	0xFF	;	$A6
	.db	0xFF	;	$A7
	.db	0xFF	;	$A8
	.db	0xFF	;	$A9
	.db	0xFF	;	$AA
	.db	0xFF	;	$AB
	.db	0xFF	;	$AC
	.db	0xFF	;	$AD
	.db	0xFF	;	$AE
	.db	0xFF	;	$AF

	.db	0xFF	;	$B0
	.db	0xFF	;	$B1
	.db	0xFF	;	$B2
	.db	0xFF	;	$B3
	.db	0xFF	;	$B4
	.db	0xFF	;	$B5
	.db	0xFF	;	$B6
	.db	0xFF	;	$B7
	.db	0xFF	;	$B8
	.db	0xFF	;	$B9
	.db	0xFF	;	$BA
	.db	0xFF	;	$BB
	.db	0xFF	;	$BC
	.db	0xFF	;	$BD
	.db	0xFF	;	$BE
	.db	0xFF	;	$BF

	.db	0xFF	;	$C0
	.db	0xFF	;	$C1
	.db	0xFF	;	$C2
	.db	0xFF	;	$C3
	.db	0xFF	;	$C4
	.db	0xFF	;	$C5
	.db	0xFF	;	$C6
	.db	0xFF	;	$C7
	.db	0xFF	;	$C8
	.db	0xFF	;	$C9
	.db	0xFF	;	$CA
	.db	0xFF	;	$CB
	.db	0xFF	;	$CC
	.db	0xFF	;	$CD
	.db	0xFF	;	$CE
	.db	0xFF	;	$CF

	.db	0xFF	;	$D0
	.db	0xFF	;	$D1
	.db	0xFF	;	$D2
	.db	0xFF	;	$D3
	.db	0xFF	;	$D4
	.db	0xFF	;	$D5
	.db	0xFF	;	$D6
	.db	0xFF	;	$D7
	.db	0xFF	;	$D8
	.db	0xFF	;	$D9
	.db	0xFF	;	$DA
	.db	0xFF	;	$DB
	.db	0xFF	;	$DC
	.db	0xFF	;	$DD
	.db	0xFF	;	$DE
	.db	0xFF	;	$DF

	.db	0xFF	;	$E0
	.db	0xFF	;	$E1
	.db	0xFF	;	$E2
	.db	0xFF	;	$E3
	.db	0xFF	;	$E4
	.db	0xFF	;	$E5
	.db	0xFF	;	$E6
	.db	0xFF	;	$E7
	.db	0xFF	;	$E8
	.db	0xFF	;	$E9
	.db	0xFF	;	$EA
	.db	0xFF	;	$EB
	.db	0xFF	;	$EC
	.db	0xFF	;	$ED
	.db	0xFF	;	$EE
	.db	0xFF	;	$EF

	.db	0xFF	;	$F0
	.db	0xFF	;	$F1
	.db	0xFF	;	$F2
	.db	0xFF	;	$F3
	.db	0xFF	;	$F4
	.db	0xFF	;	$F5
	.db	0xFF	;	$F6
	.db	0xFF	;	$F7
	.db	0xFF	;	$F8
	.db	0xFF	;	$F9
	.db	0xFF	;	$FA
	.db	0xFF	;	$FB
	.db	0xFF	;	$FC
	.db	0xFF	;	$FD
	.db	0xFF	;	$FE
	.db	0xFF	;	$FF

;E0 scantable ends
