;==============================
    .module crt0
;==============================
	.globl  _ret_sp
    .globl  _call_type
    .globl  _filesize
    .globl  _file_ext
    .globl  _actpanel
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
    ld      (_ret_sp),sp
    ld      (_call_type),a
    ex      af,af
    ld      (_file_ext),a
    ld      (_filesize),hl
    ld      (_filesize + 2),de
    ld      (_actpanel),ix
    call    gsinit
    jp      _main
;==============================
_ret_sp:
    .dw     0
_call_type:
    .db	    0
_file_ext:
    .db	    0
_filesize:
    .ds	    4
_actpanel:
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
