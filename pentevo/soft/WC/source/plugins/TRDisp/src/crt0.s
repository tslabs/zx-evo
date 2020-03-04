;==============================
    .module crt0
;==============================
	.globl  _wc_ret_sp
    .globl  _wc_call_type
    .globl  _wc_filesize
    .globl  _wc_file_ext
    .globl  _wc_filename
    .globl  _wc_actpanel
;==============================
    .globl  _main
    .globl  l__INITIALIZER
    .globl  s__INITIALIZED
    .globl  s__INITIALIZER
;==============================
    .area   _HEADER(ABS)
    .org    #0x8000
    jp      start
    .area   _CODE
start:
    ld      (_wc_ret_sp),sp
    ld      (_wc_call_type),a
    ex      af,af
    ld      (_wc_file_ext),a
    ld      (_wc_filename),bc
    ld      (_wc_filesize),hl
    ld      (_wc_filesize + 2),de
    ld      (_wc_actpanel),ix
    call    gsinit
    jp      _main
;==============================
_wc_ret_sp:
    .dw     0
_wc_call_type:
    .db	    0
_wc_file_ext:
    .db	    0
_wc_filename:
    .dw      0
_wc_filesize:
    .ds	    4
_wc_actpanel:
    .dw     0
;==============================
gsinit::
    ld      bc,#l__INITIALIZER
    ld      a,b
    or      a,c
    ret     z
    ld      de,#s__INITIALIZED
    ld      hl,#s__INITIALIZER
    ldir
    ret
;==============================
	.area	_HOME
	.area	_CODE
	.area	_INITIALIZER
	.area	_INITIALIZED
	.area   _DATA
	.area	_BSEG
	.area   _BSS
	.area   _HEAP
