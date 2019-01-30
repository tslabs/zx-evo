//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                       ::
//::               by dr_max^gc (c)2018-2019                 ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#include "defs.h"
#include "tsio.h"
#include "keyboard.h"
#include "gcWin.h"
#include "numbers.h"

//#define PRINT_DELAY

void gcVars (void)
{
    __asm
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; !!! must math with structures in gcWin.h !!!
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; struct GC_SVMENU
;; simple vertical menu offsets
    svm_attr                .equ    #00
    svm_margin              .equ    #01
    svm_current             .equ    #02
    svm_count               .equ    #03
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; struct GC_DLG_FLAG_t
    dlgf_cursor_mask        .equ    #0x80

;; struct GC_DIALOG
    dlg_flag                .equ    #00 ; byte  (flags)
    dlg_current             .equ    #01 ; byte  (current item)
    dlg_all_count           .equ    #02 ; byte  (count of all items)
    dlg_act_count           .equ    #03 ; byte  (count of active items)
    dlg_cur_attr            .equ    #04 ; byte  (cursor attribute)
    dlg_box_attr            .equ    #05 ; byte  (DI_GROUPBOX attribute)
    dlg_btn_focus_attr      .equ    #06 ; byte  (DI_BUTTON focus attribute)
    dlg_btn_unfocus_attr    .equ    #07 ; byte  (DI_BUTTON unfocus attribute)
    dlg_lbox_focus_attr     .equ    #08 ; byte  (DI_LISTBOX (and other) attribute)
    dlg_lbox_unfocus_attr   .equ    #09 ; byte  (DI_LISTBOX focus attribute)
    dlg_items               .equ    #10 ; word  (pointer to items)
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; struct GC_WINDOW
;; window descriptor offsets
    id                      .equ    #00 ; byte
    type                    .equ    #01 ; byte
    x                       .equ    #02 ; byte
    y                       .equ    #03 ; byte
    width                   .equ    #04 ; byte
    hight                   .equ    #05 ; byte
    window_attr             .equ    #06 ; byte
    frame_type              .equ    #07 ; byte
    frame_attr              .equ    #08 ; byte
    header_txt              .equ    #09 ; word
    window_txt              .equ    #11 ; word
    menu_ptr                .equ    #13 ; word
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; struct GC_DITEM_FLAG_t;
    dif_grey_mask           .equ    #0x01
    dif_tabstop_mask        .equ    #0x80

;; struct GC_DITEM_t
;; dialog item descriptor offsets
    di_type                 .equ    #00 ; byte
    di_id                   .equ    #01 ; byte
    di_x                    .equ    #02 ; byte
    di_y                    .equ    #03 ; byte
    di_width                .equ    #04 ; byte
    di_hight                .equ    #05 ; byte
    di_flags                .equ    #06 ; byte
    di_vartype              .equ    #07 ; byte
    di_hotkey               .equ    #08 ; byte
    di_select               .equ    #09 ; byte
    di_var                  .equ    #10 ; word
    di_name                 .equ    #12 ; word
    di_exec                 .equ    #14 ; word
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; struct GC_DITEM_TYPE_t
;; item types offset
    DI_TEXT                 .equ    #00
    DI_HDIV                 .equ    #01
    DI_GROUPBOX             .equ    #02
    DI_EDIT                 .equ    #04
    DI_BUTTON               .equ    #07
    DI_CHECKBOX             .equ    #08
    DI_RADIOBUTTON          .equ    #09
    DI_LISTBOX              .equ    #10
    DI_LISTVIEW             .equ    #11 ; not yet
    DI_NUMBER               .equ    #12
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    SYM_TRI                 .equ    #0xD8
    SYM_RADIO               .equ    #0xD0
    SYM_CHECK               .equ    #0xD4
    SYM_BTNUP               .equ    #0xF2
    SYM_BTNDN               .equ    #0xF4
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; macros
    .macro  LD_IXHL
    push hl
    pop ix
    .endm

    .macro LDF_HLHL
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    .endm
;;::common:::::::::::::::::::::::::::::::::::::::::::::::::::::
save_window_parms:
    ld a,(win_x)
    ld (tmp_win_x),a
    ld a,(win_y)
    ld (tmp_win_y),a
    ld a,(win_w)
    ld (tmp_win_w),a
    ld a,(win_h)
    ld (tmp_win_h),a
    ld a,(win_attr)
    ld (tmp_win_attr),a
    ld a,(frm_attr)
    ld (tmp_frm_attr),a
    ret

restore_window_parms:
    ld a,(tmp_win_x)
    ld (win_x),a
    ld a,(tmp_win_y)
    ld (win_y),a
    ld a,(tmp_win_w)
    ld (win_w),a
    ld a,(tmp_win_h)
    ld (win_h),a
    ld a,(tmp_win_attr)
    ld (win_attr),a
    ld (sym_attr),a
    ld (bg_attr),a
    ld a,(tmp_frm_attr)
    ld (frm_attr),a
    ret
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; variables
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; temp vars for dialog items
tmp_win_x:
    .db 0
tmp_win_y:
    .db 0
tmp_win_w:
    .db 0
tmp_win_h:
    .db 0
tmp_win_attr:
    .db 0
tmp_frm_attr:
    .db 0
;; current window vars
win_x:
    .db 0
win_y:
    .db 0
win_w:
    .db 0
win_h:
    .db 0
win_attr:
    .db 0
frm_attr:
    .db 0
;;
cur_x:
    .db 0
cur_y:
    .db 0
;;
sym_attr:
    .db 0
bg_attr:
    .db 0
inv_attr:
    .db 0
;;
linked_ptr:
    .dw 0
_current_menu_ptr::
    .dw 0
_current_window_ptr::
    .dw 0
_current_dialog_ptr::
    .dw 0
frame_set_addr:
    .dw 0
;;
_window_count::
    .db #0x00
_current_window_id::
    .db #0x00
_vpage::
    .db #0x80
_spage::
    .db #0x82
;;
ascbuff:
    .ds 15
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; configuration
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
cfg_btn_focus_attr:
    .db 0b11100000
cfg_btn_unfocus_attr:
    .db 0b11110000
cfg_cur_attr:
    .db 0b00001110
cfg_sbox_attr:
    .db 0b00000111
cfg_listbox_focus_attr:
    .db 0b10011111
cfg_listbox_unfocus_attr:
    .db 0b00010111
cfg_grey_attr:
    .db 0x08
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; window frame symbols array
;  space            +00
;  left_upper       +01
;  upper            +02
;  right_upper      +03
;  left             +04
;  right            +05
;  left_bottom      +06
;  bottom           +07
;  right_bottom     +08
;  left_divider     +09
;  right_divider    +10
;  horizontal_bar   +11
;;
frame_set0:
    .db 0x20,0xDB,0xDB,0xDB,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0xC4
frame_set1:
    .db 0x20,0xDB,0xDB,0xDB,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0xC4
frame_set2:
    .db 0x20,0xC9,0xCD,0xBB,0xBA,0xBA,0xC8,0xCD,0xBC,0xC7,0xB6,0xC4
frame_set0_noheader:
    .db 0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0xC4
frame_set1_noheader:
    .db 0x20,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0xC4
frame_set2_noheader:
    .db 0x20,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0xC4
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
_windows_list::
    .ds 4*10
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    __endasm;
}

void gcWindowsInit(u8 vpage, u8 spage) __naked
{
    vpage;              // to avoid SDCC warning
    spage;              // to avoid SDCC warning

    __asm
    ld hl,#2
    add hl,sp
    ld a,(hl)
    inc hl
    ld (_vpage),a
    ld a,(hl)
    ld (_spage),a

;; clear shadow screen page
    ld a,(_spage)
    ld bc,#0x13AF
    out (c),a
    call clear_page3

;; clear video page
    ld a,(_vpage)
    ld bc,#0x13AF
    out (c),a
    call clear_page3

    xor a
    ld (_window_count),a
    ld (_current_window_id),a

;; set im1 ISR
    di
    ld hl,#0x0038
    ld de,#isr38
    ld (hl),#0xC3
    inc hl
    ld (hl),e
    inc hl
    ld (hl),d
    ei
    ret

clear_page3:
    ld hl,#0xC000
    ld de,#0xC001
    ld bc,#0x3FFF
    ld (hl),l
    ldir
    ret

isr38:
    push af
    push bc

    ld	bc,#0x27AF      ;; DMASTATUS
0$: in	a,(c)
	and	#0x80
	jr	nz,2$

    ld bc,#0x1AAF       ;; DMASADDRL
    xor a
    out (c),a
    inc b
    out (c),a
    inc b
    ld a,(_spage)
    out (c),a
    inc b
    xor a
    out (c),a
    inc b
    out (c),a
    inc b
    ld a,(_vpage)
    out (c),a
    ld b,#0x28          ;; DMANUM
    ld a,#0xFF
    out (c),a
    dec b
    dec b
    ld a,#20            ;; DMALEN
    out (c),a
    inc b
    ld a,#0x01
    out (c),a

    ld	bc,#0x27AF      ;; DMASTATUS
1$: in	a,(c)
	and	#0x80
	jr	nz,1$

2$: pop bc
    pop af
    ei
    ret
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcSetLinkedMessage(u16 **ptr) __naked __z88dk_fastcall
{
    ptr;        // to avoid SDCC warning

    __asm
    ld (linked_ptr),hl
    ret
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
BTN_TYPE_t gcExecuteWindow(GC_WINDOW_t *wnd)
{
    BTN_TYPE_t rc;
    u16 *ptr;

    rc = 0;
    ptr = wnd->menu_ptr;

    gcPrintWindow(wnd);

    switch (wnd->type)
    {
    case GC_WND_SVMENU:
        rc = gcSimpleVMenu((GC_SVMENU_t*)ptr);
        break;
    case GC_WND_DIALOG:
        rc = gcDialog((GC_DIALOG_t*)ptr);
        break;
    default:
        break;
    }
    return rc;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
BTN_TYPE_t gcMessageBox(MB_TYPE_t type, GC_FRM_TYPE_t frame, char *header, char *message)
{
u8 i, j, len;
u8 x, y;

GC_WINDOW_t wnd;
GC_DIALOG_t dlg;
GC_DITEM_t txt;
GC_DITEM_t btnOk;
GC_DITEM_t btnCancel;
GC_DITEM_t btnRetry;
GC_DITEM_t btnAbort;
GC_DITEM_t btnIgnore;
GC_DITEM_t *dlgItemList[4];

    i = gcGetMessageLines(message);
    len = gcGetMessageMaxLength(message) + 10;
    if (len<40) len = 40;
    x = (SCREEN_WIDTH - len - 2)>>1;
    y = (SCREEN_HIGHT - i - 6)>>1;

    wnd.type = GC_WND_NOMENU;
    wnd.x = x;
    wnd.y = y;
    wnd.width = len + 2;
    wnd.hight = i + 3 + 3;
    wnd.attr = (WIN_COL_WHITE<<4) | WIN_COL_BLACK;
    wnd.frame_type = frame;
    wnd.frame_attr = (WIN_COL_WHITE<<4) | WIN_COL_BRIGHT_WHITE;
    wnd.header_txt = header;
    wnd.window_txt = 0;

    gcPrintWindow(&wnd);

    dlg.current = 0;
    dlg.flag.DLGF_CURSOR = 1;
    dlg.box_attr = (WIN_COL_RED<<4) | WIN_COL_BRIGHT_WHITE;
    dlg.btn_focus_attr = (WIN_COL_BRIGHT_YELLOW<<4) | WIN_COL_BLACK;
    dlg.btn_unfocus_attr = (WIN_COL_BRIGHT_WHITE<<4) | WIN_COL_BLACK;
    dlg.items = dlgItemList;

    txt.type = DI_TEXT;
    txt.id = 0;
    txt.x = 8;
    txt.y = 1;
    txt.width = 0;
    txt.name = message;

    switch (type)
    {
    case MB_OK:
        dlg.act_count = 1;
        dlg.all_count = 2;
        dlgItemList[0] = &btnOk;
        dlgItemList[1] = &txt;

        btnOk.type = DI_BUTTON;
        btnOk.id = BUTTON_OK;
        btnOk.x = (len - 10)>>1;
        btnOk.y = i + 2;
        btnOk.width = 10;
        btnOk.hight = 0;
        btnOk.select = 0;
        btnOk.name = "Ok";
        break;

    case MB_OKCANCEL:
        dlg.act_count = 2;
        dlg.all_count = 3;
        dlgItemList[0] = &btnOk;
        dlgItemList[1] = &btnCancel;
        dlgItemList[2] = &txt;

        btnOk.type = DI_BUTTON;
        btnOk.id = BUTTON_OK;
        btnOk.x = (len>>1) - 12;
        btnOk.y = i + 2;
        btnOk.width = 10;
        btnOk.hight = 0;
        btnOk.select = 0;
        btnOk.name = "Ok";

        btnCancel.type = DI_BUTTON;
        btnCancel.id = BUTTON_CANCEL;
        btnCancel.x = (len>>1) + 2;
        btnCancel.y = i + 2;
        btnCancel.width = 10;
        btnCancel.hight = 0;
        btnCancel.select = 0;
        btnCancel.name = "Cancel";
        break;

    case MB_YESNO:
        dlg.act_count = 2;
        dlg.all_count = 3;
        dlgItemList[0] = &btnOk;
        dlgItemList[1] = &btnCancel;
        dlgItemList[2] = &txt;

        btnOk.type = DI_BUTTON;
        btnOk.id = BUTTON_YES;
        btnOk.x = (len>>1) - 12;
        btnOk.y = i + 2;
        btnOk.width = 11;
        btnOk.hight = 0;
        btnOk.select = 0;
        btnOk.name = "Yes";

        btnCancel.type = DI_BUTTON;
        btnCancel.id = BUTTON_NO;
        btnCancel.x = (len>>1) + 2;
        btnCancel.y = i + 2;
        btnCancel.width = 10;
        btnCancel.hight = 0;
        btnCancel.select = 0;
        btnCancel.name = "No";
        break;

    case MB_RETRYABORTIGNORE:
        dlg.act_count = 3;
        dlg.all_count = 4;
        dlgItemList[0] = &btnRetry;
        dlgItemList[1] = &btnAbort;
        dlgItemList[2] = &btnIgnore;
        dlgItemList[3] = &txt;

        btnRetry.type = DI_BUTTON;
        btnRetry.id = BUTTON_RETRY;
        btnRetry.x = (len>>1) - 7 - 11;
        btnRetry.y = i + 2;
        btnRetry.width = 11;
        btnRetry.hight = 0;
        btnRetry.select = 0;
        btnRetry.name = "Retry";

        btnAbort.type = DI_BUTTON;
        btnAbort.id = BUTTON_ABORT;
        btnAbort.x = (len>>1) - 5;
        btnAbort.y = i + 2;
        btnAbort.width = 11;
        btnAbort.hight = 0;
        btnAbort.select = 0;
        btnAbort.name = "Abort";

        btnIgnore.type = DI_BUTTON;
        btnIgnore.id = BUTTON_IGNORE;
        btnIgnore.x = (len>>1) + 8;
        btnIgnore.y = i + 2;
        btnIgnore.width = 10;
        btnIgnore.hight = 0;
        btnIgnore.select = 0;
        btnIgnore.name = "Ignore";
        break;

    default:
        break;
    }

    for(j=0; j<3; j++)
    {
        for(i=0; i<6; i++)
        {
            gcPrintSymbol(x+i+1, y+j+2, i+(j*6)+0x0E, 0x7E);
        }
    }

    return gcDialog(&dlg);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
u8 gcSimpleVMenu(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall
{
    svmnu;      // to avoid SDCC warning

    __asm

    LD_IXHL
;; IX - simple vertical menu descriptor

    call print_svm_cursor

svmnu_lp:
    ei
    halt
    call _gcGetKey
    ld a,l
    cp #KEY_DOWN
    jr z,svmnu_up
    cp #KEY_UP
    jr z,svmnu_dn
    cp #KEY_ENTER
    jr z,svmnu_ent
    jr svmnu_lp

svmnu_ent:
    call restore_svm_cursor
    ld l,<#svm_current (ix)
    ret

svmnu_up:
    call restore_svm_cursor
    ld a,<#svm_current (ix)
    inc a
    cp <#svm_count (ix)
    jr nz,0$
    xor a
0$: ld <#svm_current (ix),a
    call print_svm_cursor
    jr svmnu_lp

svmnu_dn:
    call restore_svm_cursor
    ld a,<#svm_current (ix)
    dec a
    jp p,0$
    ld a,<#svm_count (ix)
    dec a
0$: ld <#svm_current (ix),a
    call print_svm_cursor
    jr svmnu_lp
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcRestoreSVMCursor(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall
{
    svmnu;              // to avoid SDCC warning

    __asm
    LD_IXHL
;; IX - simple vertical menu descriptor

restore_svm_cursor:
    ld a,(win_y)
    add a,<#svm_margin (ix)
    add a,<#svm_current (ix)
    ld d,a              ; Y coord
    ld a,(win_x)
    dec a
    ld e,a              ; X coord

;; set left sym attr
    ld a,(frm_attr)
    call set_attr

    ld a,(win_w)
    ld b,a
    ld a,(win_attr)
    call set_attr_line

;; set right sym attr
    ld a,(frm_attr)
    jp set_attr
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintSVMCursor(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall
{
    svmnu;              // to avoid SDCC warning

    __asm
    LD_IXHL
;; IX - simple vertical menu descriptor

print_svm_cursor:
    ld a,(win_y)
    add a,<#svm_margin (ix)
    add a,<#svm_current (ix)
    ld d,a              ; Y coord
    ld a,(win_x)
    dec a
    ld e,a              ; X coord

;; set left sym attr
    ld a,<#svm_attr (ix)
    and #0xF0
    ld c,a
    ld a,(frm_attr)
    and #0x0F
    or c
    call set_attr

    ld a,(win_w)
    ld b,a
    ld a,<#svm_attr (ix)
    and #0xF0
    call set_attr_line

;; set right sym attr
    ld a,<#svm_attr (ix)
    and #0xF0
    ld c,a
    ld a,(frm_attr)
    and #0x0F
    or c
    jp set_attr
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
u8 gcDialog(GC_DIALOG_t *dlg) __naked __z88dk_fastcall
{
    dlg;                    // to avoid SDCC warning
    __asm

;; save IX
    push ix

    ld (_current_dialog_ptr),hl

    LD_IXHL
;; IX - dialog descriptor

;; set var
    ld a,<#dlg_cur_attr (ix)
    ld (cfg_cur_attr),a
    ld a,<#dlg_box_attr (ix)
    ld (cfg_sbox_attr),a

    ld a,<#dlg_btn_focus_attr (ix)
    ld (cfg_btn_focus_attr),a
    ld a,<#dlg_btn_unfocus_attr (ix)
    ld (cfg_btn_unfocus_attr),a

    ld a,<#dlg_lbox_focus_attr (ix)
    ld (cfg_listbox_focus_attr),a
    ld a,<#dlg_lbox_unfocus_attr (ix)
    ld (cfg_listbox_unfocus_attr),a

    call _gcPrintDialog

dialog_lp:
    ld a,<#dlg_current (ix)
    call get_dialog_item_addr
    call _gcPrintDialogCursor

dialog_lp1:
    ei
    halt

    ld a,<#dlg_current (ix)
    call get_dialog_item_addr
    push hl

    call _gcGetKey

    ld a,<#dlg_flag (ix)
    and #dlgf_cursor_mask
    jr nz,dialog_leftright

    ld a,l
    cp #KEY_PGDN
    jp z,dialog_pgdn
    cp #KEY_PGUP
    jp z,dialog_pgup
    cp #KEY_DOWN
    jp z,dialog_dn
    cp #KEY_RIGHT
    jp z,dialog_pgdn
    cp #KEY_LEFT
    jp z,dialog_pgup
    cp #KEY_UP
    jp z,dialog_up
    cp #KEY_ENTER
    jp z,dialog_ent
    cp #KEY_SPACE
    jp z,dialog_sp
    pop hl
    jr dialog_lp1

dialog_leftright:
    ld a,l
    cp #KEY_LEFT
    jp z,dialog_up
    cp #KEY_RIGHT
    jp z,dialog_dn
    cp #KEY_ENTER
    jp z,dialog_ent
    cp #KEY_SPACE
    jp z,dialog_sp
    pop hl
    jr dialog_lp1

;;:::::::::::::::::::::::::::::
;; i: IX - dialog descriptor
;;     A - item number
;; o:
;;    HL - dialog item address
;;:::::::::::::::::::::::::::::
get_dialog_item_addr:
    push de
    ld l,a
    ld e,<#dlg_items (ix)
    ld d,<#dlg_items+1 (ix)
    ld h,#0
    add hl,hl
    add hl,de
    LDF_HLHL
    pop de
    ret

;;:::::::::::::::::::::::::::::
;; select item by enter
;; i: IX - dialog descriptor
;;    on stack: dialog item address
;;:::::::::::::::::::::::::::::
dialog_ent:
    pop hl
;; HL - dialog item address
;; IX - dialog descriptor
    push ix

    LD_IXHL             ; IX - dialog item addr

    ld a,<#di_type (ix)
    cp #DI_EDIT
    jp z,dialog_ent_edit
    cp #DI_LISTBOX
    jp z,dialog_ent_list
    cp #DI_BUTTON
    jr z,dialog_exit
    pop ix
;; IX - dialog descriptor
    jp dialog_lp

dialog_exit:
// press button animation
    ld a,#-1
    ld <#di_select (ix),a
    call _gcPrintDialogItem
    ld b,#5
    ei
    halt
    djnz .-1
    xor a
    ld <#di_select (ix),a
    call _gcPrintDialogItem

    ld l,<#di_id (ix)

    pop ix
;; IX - dialog descriptor

;; restore IX
    pop ix
    ret
;;:::::::::::::::::::::::::::::
dialog_dn:
    pop hl
;; HL - dialog item address
    call _gcRestoreDialogCursor
    push ix
    pop hl
    call _gcFindNextItem
    ld a,l
    ld <#dlg_current (ix),a
    jp dialog_lp
;;:::::::::::::::::::::::::::::
dialog_up:
    pop hl
;; HL - dialog item address
    call _gcRestoreDialogCursor
    push ix
    pop hl
    call _gcFindPrevItem
    ld a,l
    ld <#dlg_current (ix),a
    jp dialog_lp
;;:::::::::::::::::::::::::::::
dialog_pgdn:
    pop hl
;; HL - dialog item address
    call _gcRestoreDialogCursor

    push ix
    pop hl
    call _gcFindNextTabItem

    ld a,l
    ld <#dlg_current (ix),a
    jp dialog_lp
;;:::::::::::::::::::::::::::::
dialog_pgup:
    pop hl
;; HL - dialog item address
    call _gcRestoreDialogCursor

    push ix
    pop hl
    call _gcFindPrevTabItem

    ld a,l
    ld <#dlg_current (ix),a
    jp dialog_lp

;;:::::::::::::::::::::::::::::
;; IX - dialog item addr
;; on stack: dialog descriptor
dialog_ent_edit:
;; save current window parameters
    call save_window_parms

;; set vars
    ld a,<#di_width (ix)
    ld (win_w),a
    ld a,(win_x)
    add a,<#di_x (ix)
    ld e,a
    ld (win_x),a
    ld a,(win_y)
    add a,<#di_y (ix)
    ld (win_y),a
    ld d,a

    ld a,(cfg_listbox_unfocus_attr)
    ld (win_attr),a
    ld (sym_attr),a
    ld a,(cfg_listbox_unfocus_attr)
    ld (frm_attr),a

;;gcEditString(u8 *str, u8 len, u8 x, u8 y) {
;; push XY coords
    push de

    ld a,<#di_width (ix)
    push af
    inc sp

    ld l,<#di_name (ix)
    ld h,<#di_name+1 (ix)
    push hl

    call _gcEditString

;; restore stack
    ld hl,#5
    add hl,sp
    ld sp,hl
;;}

;; restore window parameters
    call restore_window_parms

    pop ix
;; IX - dialog descriptor

    jp dialog_lp

;;:::::::::::::::::::::::::::::
;; IX - dialog item addr
;; on stack: dialog descriptor
dialog_ent_list:

;; save current window parameters
    call save_window_parms

;; set vars
    ld a,<#di_width (ix)
    sub #2
    ld (win_w),a
    ld a,(win_x)
    add a,<#di_x (ix)
    ld e,a
    inc a
    ld (win_x),a
    ld a,(win_y)
    add a,<#di_y (ix)
    inc a
    ld (win_y),a
    ld d,a

    ld a,(cfg_listbox_unfocus_attr)
    ld (win_attr),a
    ld (sym_attr),a
    ld a,(cfg_listbox_unfocus_attr)
    ld (frm_attr),a

;; store YX
    push de

;; build temp window onto stack
;;gcDrawWindow(u8 id, u8 x, u8 y, u8 width, u8 hight, u8 attr, u8 frame_type, u8 frame_attr) {

;;set frame attr
    ld a,(cfg_listbox_unfocus_attr)
    push af
    inc sp

;;set frame type
    ld a,#0xC1
    push af
    inc sp

;;set attr
    ld a,(cfg_listbox_unfocus_attr)
    push af
    inc sp

;;set width and hight of window
    ld l,<#di_width (ix)
;; count of listbox items
    ld h,<#di_select (ix)
    inc h
    push hl

;;set YX coords
    push de

    ld a,(_window_count)
    inc a
    push af
    inc sp

    call _gcDrawWindow

;; restore stack
    ld hl,#8
    add hl,sp
    ld sp,hl
;;}

;; restore YX
    pop de
    inc e

;; print list items
;; count of listbox items
    ld b,<#di_select (ix)
    xor a
0$: push af
    push bc
    push de
    add a,a
    ld c,a
    ld b,#0
    ld l,<#di_name (ix)
    ld h,<#di_name+1 (ix)  ; HL - name address
    add hl,bc
    LDF_HLHL
    call strprnz
    pop de
    inc d
    pop bc
    pop af
    inc a
    djnz 0$

;; build temp vertical menu onto stack
    ld e,<#di_var (ix)
    ld d,<#di_var+1 (ix)

    ld a,<#di_select (ix)
;; svm_count
    push af
    inc sp
    ld a,(de)
;; svm_current
    push af
    inc sp
    xor a
;; svm_margin
    push af
    inc sp
    ld a,(cfg_listbox_focus_attr)
    rlca
    rlca
    rlca
    rlca
;; svm_attr
    push af
    inc sp

    ld hl,#0
    add hl,sp   ; HL - svmenu address
    push de
    call _gcSimpleVMenu
    pop de

;; save selected
    ld a,l
    ld (de),a

;; restore stack
    pop af
    pop af

;; restore window parameters
    call restore_window_parms

;; stop screen update
    di
    ld a,(_window_count)
    push af
    ld hl,(_current_window_ptr)
    call _gcPrintWindow
    pop af
    ld (_window_count),a

    pop ix
;; IX - dialog descriptor

    push ix
    pop hl
    call _gcPrintDialog
    jp dialog_lp

;;:::::::::::::::::::::::::::::
;; select item by space
;; i: IX - dialog descriptor
;;    on stack: dialog item address
;;:::::::::::::::::::::::::::::
dialog_sp:
    pop hl
;; HL - dialog item address

    push ix

    LD_IXHL             ; IX - dialog item addr

;; calling function
    ld e,<#di_exec (ix)
    ld d,<#di_exec+1 (ix)
    ld a,e
    or d
    jr z,1$

    push ix
    push hl

    ld hl,#0$
    push hl
    ex de,hl
    jp (hl)

0$:
    pop hl
    pop ix

1$:
    ld a,<#di_type (ix)
    cp #DI_RADIOBUTTON
    jr z,dialog_sel_radio
    cp #DI_LISTBOX
    jr z,dialog_sel_list
    cp #DI_CHECKBOX
    jr z,dialog_sel_check

dialog_sp_exit:
    pop ix
    jp dialog_lp
;;:::::::::::::::::::::::::::::
;; select CHECKBOX item
dialog_sel_check:
    ld e,<#di_var (ix)
    ld d,<#di_var+1 (ix)
    ld a,(de)
    or a
    jr nz,.+2+1+2
    cpl
    jr .+2+1
    xor a
    ld (de),a
    call _gcPrintDialogItem
    jr dialog_sp_exit

;;:::::::::::::::::::::::::::::
;; select LISTBOX item
dialog_sel_list:
    ld e,<#di_var (ix)
    ld d,<#di_var+1 (ix)
    ld a,(de)
    inc a
    cp <#di_select (ix)
    jr c,.+2+1
    xor a
    ld (de),a
    call _gcPrintDialogItem
    jr dialog_sp_exit

;;:::::::::::::::::::::::::::::
;; select RADIOBUTTON item
dialog_sel_radio:
    ld e,<#di_var (ix)
    ld d,<#di_var+1 (ix)
    ld a,<#di_select (ix)
    ld (de),a
    ld a,#DI_RADIOBUTTON
    push af
    inc sp
    ld hl,(_current_dialog_ptr)
    push hl
    call _gcPrintDialogShownItems
    ld hl,#3
    add hl,sp
    ld sp,hl
    jr dialog_sp_exit
;;:::::::::::::::::::::::::::::
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
u8 gcFindPrevTabItem(GC_DIALOG_t *dlg) __naked __z88dk_fastcall
{
    dlg;                    // to avoid SDCC warning

    __asm
    push ix

    LD_IXHL                 ; IX - dialog descriptor

    ld a,<#dlg_current (ix)
0$: or a
    jr z,4$
    dec a
1$: push af
    push ix
    call get_dialog_item_addr
    LD_IXHL             ; IX - dialog item descriptor

;; check TABSTOP bit
    ld a,<#di_flags (ix)
    and #dif_tabstop_mask
    jr nz,3$
    pop ix
    pop af
    jr 0$

;; check GREY bit
3$: ld a,<#di_flags (ix)
    and #dif_grey_mask
    pop ix
    jr z,2$
    pop af
    jr 0$

2$: pop af
    pop ix
    ld l,a
    ret

4$: ld a,<#dlg_act_count (ix)
    dec a
    pop ix
    ld l,a
    ret
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
u8 gcFindNextTabItem(GC_DIALOG_t *dlg) __naked __z88dk_fastcall
{
    dlg;                    // to avoid SDCC warning

    __asm
    push ix

    LD_IXHL                 ; IX - dialog descriptor

    ld a,<#dlg_current (ix)
0$: inc a
    cp <#dlg_act_count (ix)
    jr nz,1$
    xor a
    pop ix
    ld l,a
    ret

1$: push af
    push ix
    call get_dialog_item_addr
    LD_IXHL             ; IX - dialog item descriptor

;; check TABSTOP bit
    ld a,<#di_flags (ix)
    and #dif_tabstop_mask
    jr nz,3$
    pop ix
    pop af
    jr 0$

;; check GREY bit
3$: ld a,<#di_flags (ix)
    and #dif_grey_mask
    pop ix
    jr z,2$
    pop af
    jr 0$

2$: pop af
    pop ix
    ld l,a
    ret
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
u8 gcFindNextItem(GC_DIALOG_t *dlg) __naked __z88dk_fastcall
{
    dlg;                    // to avoid SDCC warning

    __asm
    push ix

    LD_IXHL                 ; IX - dialog descriptor

    ld a,<#dlg_current (ix)
0$: inc a
    cp <#dlg_act_count (ix)
    jr nz,1$
    xor a
    pop ix
    ld l,a
    ret

1$: push af
    push ix
    call get_dialog_item_addr
    LD_IXHL             ; IX - dialog item descriptor

;; check GREY bit
    ld a,<#di_flags (ix)
    and #dif_grey_mask
    pop ix
    jr z,2$
    pop af
    jr 0$

2$: pop af
    pop ix
    ld l,a
    ret
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
u8 gcFindPrevItem(GC_DIALOG_t *dlg) __naked __z88dk_fastcall
{
    dlg;                    // to avoid SDCC warning

    __asm
    push ix

    LD_IXHL                 ; IX - dialog descriptor

    ld a,<#dlg_current (ix)
0$: or a
    jr z,3$
    dec a
1$: push af
    push ix
    call get_dialog_item_addr
    LD_IXHL             ; IX - dialog item descriptor

;; check GREY bit
    ld a,<#di_flags (ix)
    and #dif_grey_mask
    pop ix
    jr z,2$
    pop af
    jr 0$

2$: pop af
    pop ix
    ld l,a
    ret

3$: ld a,<#dlg_act_count (ix)
    dec a
    pop ix
    ld l,a
    ret
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintDialogShownItems(GC_DIALOG_t *dlg, GC_DITEM_TYPE_t type) __naked
{
    dlg;                    // to avoid SDCC warning
    type;                   // to avoid SDCC warning

    __asm
;;::::::::::::::::::::::::::::::
;; var offsets
    _dlg    .equ #0
    _type   .equ #2
;;::::::::::::::::::::::::::::::
    push ix
    ld ix,#4
    add ix,sp

    ld l,<#_dlg (ix)
    ld h,<#_dlg+1 (ix)
    ld e,<#_type (ix)

    LD_IXHL                 ; IX - dialog descriptor

    ld b,<#dlg_all_count (ix)

0$: push bc
    push ix
    push de

    ld a,b
    dec a
    call get_dialog_item_addr
;; HL - dialog item address

    LD_IXHL                 ; IX - dialog item descriptor

    pop de
    push de
    ld a,<#di_type (ix)
    cp e
    call z,_gcPrintDialogItem
    pop de
    pop ix
    pop bc
    djnz 0$

    pop ix
    ret
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintActiveDialog(GC_DIALOG_t *dlg) __naked __z88dk_fastcall
{
    dlg;                    // to avoid SDCC warning

    __asm
    push ix

    LD_IXHL                 ; IX - dialog descriptor

    ld b,<#dlg_act_count (ix)
    jp print_dialog
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintDialog(GC_DIALOG_t *dlg) __naked __z88dk_fastcall
{
    dlg;                    // to avoid SDCC warning

    __asm
    push ix

    LD_IXHL                 ; IX - dialog descriptor

    ld b,<#dlg_all_count (ix)
print_dialog:
0$: push bc
    ld a,b
    dec a
    call get_dialog_item_addr
;; HL - dialog item address

    call _gcPrintDialogItem

    pop bc
    djnz 0$

    pop ix
    ret
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintDialogCursor(GC_DITEM_t *ditm) __naked __z88dk_fastcall
{
    ditm;               // to avoid SDCC warning

    __asm
    push ix

    LD_IXHL             ; IX - dialog item descriptor

    ld a,(win_x)
    add a,<#di_x (ix)
    ld e,a              ; X coord
    ld a,(win_y)
    add a,<#di_y (ix)
    ld d,a              ; Y coord

    ld b,<#di_width (ix)

1$: call get_attr
    and #0xF0
    ld c,a
    ld a,<#di_type (ix)
    cp #DI_BUTTON
    jr nz,2$
    ld a,(cfg_btn_focus_attr)
    jr 3$
2$: cp #DI_LISTBOX
    jr nz,4$
    ld a,(cfg_listbox_focus_attr)
    jr 3$
4$: cp #DI_EDIT
    jr z,6$

5$:
    ld a,(cfg_cur_attr)
3$: call set_attr
    djnz 1$

    pop ix
    ret

6$: ld a,(cfg_listbox_unfocus_attr)
    rrca
    rrca
    rrca
    rrca
    call set_attr
    ld a,(cfg_listbox_focus_attr)
    dec b
    call nz,set_attr_line
    pop ix
    ret

    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcRestoreDialogCursor(GC_DITEM_t *ditm) __naked __z88dk_fastcall
{
    ditm;               // to avoid SDCC warning

    __asm
    push ix

    LD_IXHL             ; IX - dialog item descriptor

    ld a,(win_x)
    add a,<#di_x (ix)
    ld e,a              ; X coord
    ld a,(win_y)
    add a,<#di_y (ix)
    ld d,a              ; Y coord

    ld b,<#di_width (ix)

1$: ld a,<#di_type (ix)
    cp #DI_BUTTON
    jr nz,2$
    ld a,(cfg_btn_unfocus_attr)
    jr 3$
2$: ld a,(win_attr)
3$: call set_attr
    djnz 1$

    push ix
    pop hl
;; HL - dialog item descriptor
    call _gcPrintDialogItem

    pop ix
    ret
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintDialogItem(GC_DITEM_t *ditm) __naked __z88dk_fastcall
{
    ditm;               // to avoid SDCC warning

    __asm

    push ix
    push hl

    LD_IXHL             ; IX - dalog item descriptor

;; save current window parameters
    call save_window_parms

;; calc item coords
;; set local window
    ld a,(win_x)
    add a,<#di_x (ix)
    ld e,a              ; X coord
    ld (cur_x),a
    ld (win_x),a
    ld a,(win_y)
    add a,<#di_y (ix)
    ld d,a              ; Y coord
    ld (cur_y),a
    ld (win_y),a
    ld a,<#di_width (ix)
    ld (win_w),a

    ld l,<#di_name (ix)
    ld h,<#di_name+1 (ix)  ; HL - name address

; store ret addr for item routine
    ld bc,#gc_print_dialog_item_ret$
    push bc

    ld a,<#di_type (ix)
    cp #DI_TEXT
    jp z,print_item_text
    cp #DI_HDIV
    jp z,print_item_hdiv
    cp #DI_GROUPBOX
    jp z,print_item_groupbox
    cp #DI_RADIOBUTTON
    jp z,print_item_radio
    cp #DI_CHECKBOX
    jp z,print_item_check
    cp #DI_BUTTON
    jp z,print_item_button
    cp #DI_NUMBER
    jp z,print_item_number
    cp #DI_LISTBOX
    jp z,print_item_listbox
    cp #DI_EDIT
    jp z,print_item_edit

;restore ret addr
    pop bc

gc_print_dialog_item_ret$:
    ld a,<#di_type (ix)
    cp #DI_CHECKBOX
    jr z,gc_print_dialog_item_ret1$
    cp #DI_RADIOBUTTON
    jr z,gc_print_dialog_item_ret1$
    cp #DI_EDIT
    jr z,gc_print_dialog_item_ret1$

; don`t calc width
    jr gc_print_dialog_item_ret0$

;; calc item width
gc_print_dialog_item_ret1$:
    ld a,(tmp_win_x)
    add a,<#di_x (ix)
    ld c,a
    ld a,e
    sub c
    ld <#di_width (ix),a

gc_print_dialog_item_ret0$:

; restore window parameters
    call restore_window_parms

    pop hl
    pop ix
    ret

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; i:
;;   HL - string address
;;   DE - YX coords
print_item_edit:
    push de
    call print_item_text
    pop de
    ld b,<#di_width (ix)
    ld a,<#di_flags (ix)
    and #dif_grey_mask
    ld a,(cfg_listbox_unfocus_attr)
    jp z,set_attr_line
    and #0xF0
    ld c,a
    ld a,(cfg_grey_attr)
    and #0x0F
    or c
    jp set_attr_line

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; i:
;;   HL - string address
;;   DE - YX coords
print_item_listbox:
    push hl

    ld a,<#di_flags (ix)
    and #dif_grey_mask
    ld a,(cfg_listbox_unfocus_attr)
    jr z,0$
    and #0xF0
    ld c,a
    ld a,(cfg_grey_attr)
    and #0x0F
    or c
0$: ld c,a

;; print arrow down
    push de
    ld a,<#di_width (ix)
    dec a
    dec a
    add a,e
    ld e,a
    ld a,#SYM_BTNDN
    call sym_prn
    ld a,#SYM_BTNDN+1
    call sym_prn
    pop de

;; draw attr stripe
    push de
    ld a,c
    ld (sym_attr),a
    ld b,<#di_width (ix)
    call set_attr_line
    pop de

    ld a,#sym_left
    call sym_prn

    pop hl
;; HL - address of list

    ld c,<#di_var (ix)
    ld b,<#di_var+1 (ix)
    ld a,(bc)
    add a,a
    ld c,a
    ld b,#0
    add hl,bc
    LDF_HLHL
    jp print_item_text

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; i:
;;   HL - string address
;;   DE - YX coords
print_item_hdiv:
    push hl
    push de

    dec e
    ld a,(frm_attr)
    ld c,a
    ld a,#sym_left_divider
    call winfrm_prn
    ld a,(tmp_win_w)
    ld b,a
    ld a,#sym_h_bar
    call winfrm_prn
    djnz .-3
    ld a,#sym_right_divider
    call winfrm_prn

    pop de
    pop hl
    jp print_item_text

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; i:
;;   HL - string address
;;   DE - YX coords
print_item_check:
    ld a,(sym_attr)
    push af
    ld c,a
    ld a,<#di_flags (ix)
    and #dif_grey_mask
    jr z,0$
    ld a,(cfg_grey_attr)
    and #0x0F
    ld c,a
    ld a,(win_attr)
    and #0xF0
    or c
    ld c,a
0$: ld a,c
    ld (sym_attr),a

    ld c,<#di_var (ix)
    ld b,<#di_var+1 (ix)
    ld a,(bc)
    or a
    ld a,#SYM_CHECK
    jr z,.+2+2
    add a,#2
    ld bc,(sym_attr)
    push hl
    call sym_prn
    inc a
    call sym_prn
    pop hl
    call print_item_text
    pop af
    ld (sym_attr),a
    ret

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; i:
;;   HL - string address
;;   DE - YX coords
print_item_radio:
    ld a,(sym_attr)
    push af
    ld c,a
    ld a,<#di_flags (ix)
    and #dif_grey_mask
    jr z,0$
    ld a,(cfg_grey_attr)
    and #0x0F
    ld c,a
    ld a,(win_attr)
    and #0xF0
    or c
    ld c,a
0$: ld a,c
    ld (sym_attr),a

    ld c,<#di_var (ix)
    ld b,<#di_var+1 (ix)
    ld a,(bc)
    cp a,<#di_select (ix)
    ld a,#SYM_RADIO
    jr nz,.+2+2
    add a,#2
    ld bc,(sym_attr)
    push hl
    call sym_prn
    inc a
    call sym_prn
    pop hl
    call print_item_text

    pop af
    ld (sym_attr),a
    ret

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
print_item_groupbox:
    push hl
    push de

;; set vars
    ld a,(tmp_win_x)
    dec a
    add a,<#di_x (ix)
    ld (win_x),a
    ld a,<#di_width (ix)
    ld (win_w),a

    ld a,(cfg_sbox_attr)
    and #0x0F
    ld c,a
    ld a,(sym_attr)
    and #0xF0
    or c
    ld c,a

    push de

;; print left-upper corner
    ld a,#0xDA
    call sym_prn

;; print upper bar
    ld b,<#di_width (ix)
    dec b
    dec b
    ld a,#0xC4
    call sym_prns

;; print right-upper corner
    ld a,#0xBF
    call sym_prn

    pop de
; y++
    inc d

;; print left&right bars
    ld b,<#di_hight (ix)
    dec b
    dec b
2$: push de
    ld a,#0xB3
    call sym_prn
    ld a,e
    add a,<#di_width (ix)
    dec a
    dec a
    ld e,a
    ld a,#0xB3
    call sym_prn
    pop de
    inc d
    djnz 2$

;; print left-bottom corner
    ld a,#0xC0
    call sym_prn
    ld b,<#di_width (ix)
    dec b
    dec b
    ld a,#0xC4
    call sym_prns
;; print right-bottom corner
    ld a,#0xD9
    call sym_prn

;; restore YX coords
    pop de
    inc e
    pop hl
;    jp print_item_text

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; i:
;;   HL - string address
;;   DE - YX coords
print_item_text:
    ld a,l
    or h
    ret z
    jp strprnz

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; i:
;;   HL - string address
;;   DE - YX coords
print_item_button:
;; set vars
    ld a,(tmp_win_x)
    add a,<#di_x (ix)
    ld (win_x),a
    ld a,<#di_width (ix)
    ld (win_w),a

    ld a,<#di_select (ix)
    or a
    jr nz,10$
    push hl
    push de
    ld b,<#di_width (ix)
    ld a,(cfg_btn_unfocus_attr)
    ld c,a
    ld a,#0x20
    call sym_prns
    ld a,(win_attr)
    ld c,a
    ld a,#0xDC
    call sym_prn
    pop de
    pop hl
    push de
    ld a,(sym_attr)
    push af
    ld a,(cfg_btn_unfocus_attr)
    ld (sym_attr),a
    call strprnz_center
    pop af
    ld (sym_attr),a
    pop de
    inc d
    inc e
    ld b,<#di_width (ix)
    ld a,(win_attr)
    ld c,a
    ld a,#0xDF
    jp sym_prns

;; pressed button
10$:
    push hl
    push de
    ld a,(win_attr)
    ld c,a
    ld a,#0x20
    call sym_prn
    ld b,<#di_width (ix)
    ld a,(cfg_btn_unfocus_attr)
    ld c,a
    ld a,#0x20
    call sym_prns
    ld a,(win_attr)
    ld c,a
    ld a,#0x20
    call sym_prn
    pop de
    pop hl
    inc e
    ld a,e
    ld (win_x),a
    push de
    ld a,(sym_attr)
    push af
    ld a,(cfg_btn_unfocus_attr)
    ld (sym_attr),a
    call strprnz_center
    pop af
    ld (sym_attr),a
    pop de
    inc d
    ld b,<#di_width (ix)
    ld a,(win_attr)
    ld c,a
    ld a,#0x20
    jp sym_prns

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
print_item_number:
    call print_item_text

;; set local window coords
    ld a,e
    ld (win_x),a
    ld (cur_x),a

    ld l,<#di_var (ix)
    ld h,<#di_var+1 (ix)  ; HL - var address
    ld a,<#di_vartype (ix)
    ld c,a
    and #4
    jr z,print_itm_dec
    ld a,c
    and #3
    jr z,print_itm_h8
    cp #2
    jr z,print_itm_h32
;;::::::::::::::::::::::::::::::
print_itm_h16:
    LDF_HLHL
    push ix
    ld ix,#ascbuff+1
    call hexasc16
    ld (ix),#0
    pop ix
    ld hl,#ascbuff
    ld (hl),#0x0F
    jp strprnz
;;::::::::::::::::::::::::::::::
print_itm_h8:
    ld a,(hl)
    push ix
    ld ix,#ascbuff+1
    call hexasc8
    ld (ix),#0
    pop ix
    ld hl,#ascbuff
    ld (hl),#0x0F
    jp strprnz
;;::::::::::::::::::::::::::::::
print_itm_h32:
    push de
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    LDF_HLHL
    ex de,hl
    push ix
    ld ix,#ascbuff+1
    call hexasc32
    ld (ix),#0
    pop ix
    pop de
    ld hl,#ascbuff
    ld (hl),#0x0F
    jp strprnz
;;::::::::::::::::::::::::::::::
print_itm_dec:
    ld a,c
    and #3
    jr z,print_itm_d8
    cp #1
    jr z,print_itm_d16
    jr print_itm_d32
;;::::::::::::::::::::::::::::::
print_itm_d8:
    push de
    ld l,(hl)
    ld h,#0
    ld d,h
    ld e,h
    ld c,h
    push ix
    ld ix,#ascbuff+1
    call decasc8
    pop ix
    pop de
    ld hl,#ascbuff
    ld (hl),#0x0F
    jp strprnz
;;::::::::::::::::::::::::::::::
print_itm_d16:
    push de
    LDF_HLHL
    push hl
    exx
    pop hl
    exx
    ld hl,#0
    ld c,h
    push ix
    ld ix,#ascbuff+1
    call decasc16
    pop ix
    pop de
    ld hl,#ascbuff
    ld (hl),#0x0F
    jp strprnz
;;::::::::::::::::::::::::::::::
print_itm_d32:
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    LDF_HLHL
    jp _gcPrintDec32
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcEditString(u8 *str, u8 len, u8 x, u8 y) __naked
{
    str,len;                // to avoid SDCC warning
    x,y;                    // to avoid SDCC warning

    __asm
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; var offsets
    _str    .equ #0
    _len    .equ #2
    _x      .equ #3
    _y      .equ #4
    _offset .equ #-3
    _buflen .equ #80
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    push ix
    ld ix,#4
    add ix,sp

;; allocate local vars _offset and _buff
    ld hl,#-_buflen-1
    add hl,sp
    ld (_buff),hl
    ld sp,hl

;; fill buff
    ld e,l
    ld d,h
    inc de
    ld bc,#_buflen-1
    ld (hl),#0x20
    ldir

;;copy to buff
    ld l,<#_str (ix)
    ld h,<#_str+1 (ix)
    ld de,(_buff)
    ld b,#0
    ld c,<#_len (ix)
    ldir
;;
edit_string_home:
    xor a
edit_string_lp1:
    ld <#_offset (ix),a
;;
edit_string_lp:
    call print_edit_string
;;
edit_string_lp0:
    ei
    halt
    call _gcGetKey
    ld a,l
    cp #KEY_LEFT
    jr z,edit_string_left
    cp #KEY_RIGHT
    jr z,edit_string_right
    cp #KEY_BACK
    jr z,edit_string_delete
    cp #KEY_INS
    jr z,edit_string_ins
    cp #KEY_DEL
    jr z,edit_string_del
    cp #KEY_ENTER
    jp z,edit_string_ent
    cp #KEY_HOME
    jr z,edit_string_home
    cp #KEY_END
    jr z,edit_string_end
    cp #0x20
    jr c,edit_string_lp0

    ex af,af
    call edit_string_insert_sym
    ld hl,(_buff)
    ld b,#0
    ld c,<#_offset (ix)
    add hl,bc
    ex af,af
    ld (hl),a
;;::::::::::::::::::::::::::::::
edit_string_right:
    ld a,<#_offset (ix)
    inc a
    cp <#_len (ix)
    jr nc,edit_string_lp
    jr edit_string_lp1
;;::::::::::::::::::::::::::::::
edit_string_left:
    ld a,<#_offset (ix)
    dec a
    jp m,edit_string_lp
    jr edit_string_lp1
;;::::::::::::::::::::::::::::::
edit_string_end:
    ld a,<#_len (ix)
    dec a
    jr edit_string_lp1
;;::::::::::::::::::::::::::::::
edit_string_delete:
    ld a,<#_offset (ix)
    dec a
    jp m,0$
    ld <#_offset (ix),a
0$:
;;::::::::::::::::::::::::::::::
edit_string_del:
    call edit_string_delete_sym
    dec hl
    ld (hl),#0x20
    jr edit_string_lp
;;::::::::::::::::::::::::::::::
edit_string_ins:
    call edit_string_insert_sym
    inc hl
    inc hl
    ld (hl),#0x20
    jr edit_string_lp
;;::::::::::::::::::::::::::::::
edit_string_delete_sym:
    ld hl,(_buff)
    ld c,<#_offset (ix)
    ld b,#0
    add hl,bc
    ld e,l
    ld d,h
    inc hl
    ld a,#_buflen-1
    sub <#_offset (ix)
    ret z
    ld c,a
    ldir
    ret
;;::::::::::::::::::::::::::::::
edit_string_insert_sym:
    ld hl,(_buff)
    ld c,#_buflen-1
    ld b,#0
    add hl,bc
    ld e,l
    ld d,h
    dec hl
    ld a,#_buflen
    sub <#_offset (ix)
    ld c,a
    lddr
    ret
;;::::::::::::::::::::::::::::::
print_edit_string:
    ld hl,(_buff)
    ld e,<#_x (ix)
    ld d,<#_y (ix)
    push de
    ld b,<#_len (ix)
    call strprnlen
    pop de
    ld a,<#_offset (ix)
    add a,e
    ld e,a
    ld a,(cfg_cur_attr)
    and #0x0F
    rrca
    rrca
    rrca
    rrca
    jp set_attr
;;::::::::::::::::::::::::::::::
edit_string_ent:
    ld hl,(_buff)
    ld e,<#_str (ix)
    ld d,<#_str+1 (ix)
    ld c,<#_len (ix)
    ld b,#0
    ldir

;; free
    ld hl,#_buflen+1
    add hl,sp
    ld sp,hl

    pop ix
    ret
_buff:
    .dw 0
    __endasm;
}

void gcCloseWindow(void) __naked
{
    __asm
    ld a,(_window_count)
    or a
    ret z
    dec a
    ld (_window_count),a
    call _gcPrintChainWindows
    ret
    __endasm;
}

void gcPrintChainWindows(void) __naked
{
    __asm
    di
    ld a,(_window_count)
    push af
    ld b,a
    xor a
0$: ld (_window_count),a
    inc a
    push af
    push bc
    ld l,a
    ld h,#0
    add hl,hl
    add hl,hl
    ld de,#_windows_list
    add hl,de
    inc hl  ;skip id
    inc hl  ;skip flags
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    call _gcPrintWindow
    pop bc
    pop af
    djnz 0$
    pop af
    ld (_window_count),a
    ei
    ret
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcDrawWindow(u8 id, u8 x, u8 y, u8 width, u8 hight, u8 attr, u8 frame_type, u8 frame_attr) __naked
{
    id,x,y,width,hight,attr;// to avoid SDCC warning
    frame_type, frame_attr; // to avoid SDCC warning

    __asm
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; var offsets
    _id     .equ #0
    _x      .equ #1
    _y      .equ #2
    _w      .equ #3
    _h      .equ #4
    _a      .equ #5
    _ft     .equ #6
    _fa     .equ #7
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; frameset symbols offsets
    sym_space           .equ    #00
    sym_left_upper      .equ    #01
    sym_upper           .equ    #02
    sym_right_upper     .equ    #03
    sym_left            .equ    #04
    sym_right           .equ    #05
    sym_left_bottom     .equ    #06
    sym_bottom          .equ    #07
    sym_right_bottom    .equ    #08
    sym_left_divider    .equ    #09
    sym_right_divider   .equ    #10
    sym_h_bar           .equ    #11
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    push ix
    ld ix,#4
    add ix,sp

;; set window id
    ld a,<#_id (ix)
    ld (_current_window_id),a

;; select frameset
    ld de,#frame_set0
    ld a,<#_ft (ix)
    and #0x1F
    or a
    jr z,9$
    ld de,#frame_set1
    cp #1
    jr z,9$
    ld de,#frame_set2
    cp #2
    jr z,9$
    ld de,#frame_set0_noheader
    cp #0x40
    jr z,9$
    ld de,#frame_set1_noheader
    cp #0x41
    jr z,9$
    ld de,#frame_set2_noheader
9$: ld (frame_set_addr),de

    ld e,<#_x (ix)
    ld d,<#_y (ix)
    ld c,<#_fa (ix)

;; check GC_FRM_NOHEADER flag
    bit 6,<#_ft (ix)
    push af
    call nz,print_window_row
    pop af
    jr nz,4$

    ld b,<#_w (ix)
    dec b
    dec b

;; print left-upper corner
    ld a,#sym_left_upper
    call winfrm_prn

;; print upper bar
    ld a,#sym_upper
    call winfrm_prn
    djnz .-3

;; print right-upper corner
    ld a,#sym_right_upper
    call winfrm_prn

;; y++
    inc d

4$: ld b,<#_h (ix)
    dec b
    dec b

;; y loop
    call print_window_row
    djnz .-3

    ld e,<#_x (ix)

;; print left-bottom corner
    ld a,#sym_left_bottom
    ld c,<#_fa (ix)
    call winfrm_prn

    ld b,<#_w (ix)
    dec b
    dec b

;; print bottom bar
    ld a,#sym_bottom
    call winfrm_prn
    djnz .-3

;; print right-bottom corner
    ld a,#sym_right_bottom
    call winfrm_prn

;; check GC_FRM_NOSHADOW flag
    bit 7,<#_ft (ix)
    jr nz,2$

;; print shadow
    ld a,#0x08
    call set_attr
    call set_attr
2$:

;; y++
    inc d

    ld e,<#_x (ix)
;; x++
    inc e
    inc e

;; check GC_FRM_NOSHADOW flag
    bit 7,<#_ft (ix)
    jr nz,3$

;; print bottom shadow
    ld b,<#_w (ix)
    ld a,#0x08
    call set_attr_line

3$:
    pop ix
    ret

;; print window row
print_window_row:
    push bc
    ld e,<#_x (ix)
    ld b,<#_w (ix)
    dec b
    dec b

;; print left bar
    ld c,<#_fa (ix)
    ld a,#sym_left
    call winfrm_prn

;; print space
    ld a,#sym_space
    ld c,<#_a (ix)
    call winfrm_prn
    djnz .-3

;; print right bar
    ld a,#sym_right
    ld c,<#_fa (ix)
    call winfrm_prn

;; check GC_FRM_NOSHADOW flag
    bit 7,<#_ft (ix)
    jr nz,1$

;; print shadow
    ld a,#0x08
    call set_attr
    call set_attr
1$:
;; y++
    inc d

    pop bc
    ret

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; print frame symbol
;; i:
;;   A - symbol
;;   C - attribute
;;   DE - YX coords
;; o:
;;   E++
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
winfrm_prn:
    ld l,a
    push af
    push bc
    ld h,#0
    ld bc,(frame_set_addr)
    add hl,bc
    ld a,(hl)
    pop bc
    call sym_prn
    pop af
    ret
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall
{
    wnd;                // to avoid SDCC warning

    __asm
    push ix

    LD_IXHL             ; IX - window descriptor

;; set variables
    ld d,<#y (ix)
    ld a,d
    inc a
    ld (win_y),a
    ld (cur_y),a
    ld e,<#x (ix)
    ld a,e
    inc a
    ld (win_x),a
    ld (cur_x),a
    ld a,<#width (ix)
    sub #2
    ld (win_w),a
    ld a,<#hight (ix)
    sub #2
    ld (win_h),a
    ld c,<#menu_ptr (ix)
    ld b,<#menu_ptr + 1 (ix)
    ld (_current_menu_ptr),bc
    ld (_current_window_ptr),hl

    ld a,<#frame_attr (ix)
    ld (frm_attr),a

;;gcDrawWindow {
    ld l,<#frame_type (ix)
    ld h,<#frame_attr (ix)
    push hl

    ld a,<#window_attr (ix)
    push af
    inc sp

    ld l,<#width (ix)
    ld h,<#hight (ix)
    push hl

    ld l,<#x (ix)
    ld h,<#y (ix)
    push hl

    ld a,(_window_count)
    inc a
    ld (_window_count),a
    ld <#id (ix),a
    push af
    inc sp

    call _gcDrawWindow

    ld hl,#8
    add hl,sp
    ld sp,hl
;;}

;; check GC_FRM_NOHEADER flag
    bit 6,<#frame_type (ix)
    jr nz,1$

;; set colors
    ld a,<#frame_attr (ix)
    rlca
    rlca
    rlca
    rlca
    and #0xF0
    ld (sym_attr),a
    ld (win_attr),a
    ld (bg_attr),a

;; print window header
    ld d,<#y (ix)
    ld e,<#x (ix)
    inc e
    ld l,<#header_txt (ix)
    ld h,<#header_txt + 1 (ix)
    ld a,l
    or h
    call nz,strprnz_center

;; check GC_FRM_NOLOGO flag
    bit 5,<#frame_type (ix)
    jr nz,1$

;; print spectrum stripes
    ld hl,#header_str
    ld e,<#x (ix)
    inc e

    call strprnz
    ld a,<#frame_attr (ix)
    and #0x0F
    ld c,a
    ld a,#((5|8)<<4)
    or c
    ld c,a
    ld a,#SYM_TRI
    call sym_prn

1$:
;; set colors
    ld a,<#window_attr (ix)
    ld (win_attr),a
    ld (sym_attr),a
    ld (bg_attr),a

;; print window text
    ld d,<#y (ix)
    inc d
    ld e,<#x (ix)
    inc e
    ld l,<#window_txt (ix)
    ld h,<#window_txt + 1 (ix)
    ld a,l
    or h
    call nz,strprnz

;; store to windows list
    ld a,<#id (ix)
    add a,a
    add a,a
    ld l,a
    ld h,#0
    ld de,#_windows_list
    add hl,de
    ex de,hl
    ld a,<#id (ix)
    ld (de),a
    inc de
    xor a   ;flags
    ld (de),a
    inc de
    push ix
    pop hl
    ld a,l
    ld (de),a
    inc de
    ld a,h
    ld (de),a

    pop ix
    ret

header_str:
    .db 0x07, 0x0A, SYM_TRI
    .db 0x08, 0x0A, 0x07, 0x0E, SYM_TRI
    .db 0x08, 0x0E, 0x07, 0x0C, SYM_TRI
    .db 0x08, 0x0C, 0x07, 0x0D, SYM_TRI
    .db 0x00

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; set attribute line
;; i:
;;   A - attribute
;;   B - length
;;   DE - YX coords
;; o:
;;   E++
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set_attr_line:
    call set_attr
    djnz .-3
    ret

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; set attribute
;; i:
;;   A - attribute
;;   DE - YX coords
;; o:
;;   E++
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set_attr:
    ld h,d
    ld l,e
    inc e
    set 7,h
    set 6,h

;; set z-buff address
;    set 5,h
;    ex af,af
;    ld a,(_current_window_id)
;; compare with z-buffer
;    cp (hl)
;    jr c,0$

;; put new z and set attr
    ;ld (hl),a
;    res 5,h
    set 7,l
;    ex af,af
    ld (hl),a
    ret

0$: ex af,af
    ret

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; get attribute
;; i:
;;   DE - YX coords
;; o:
;;   A - attribute
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
get_attr:
    ld h,d
    ld l,e
    set 7,h
    set 6,h
    set 7,l
    ld a,(hl)
    ret

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; print multiple symbols
;; i:
;;   A - symbol
;;   C - attribute
;;   B - count
;;   DE - YX coords
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
sym_prns:
    call sym_prn
    djnz .-3
    ret

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; print symbol
;; i:
;;   A - symbol
;;   C - attribute
;;   DE - YX coords
;; o:
;;   E++
;; corrupt:
;;   BC`, HL`, AF`
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
sym_prn:
    #ifdef PRINT_DELAY
    push bc
    djnz .-0
    djnz .-0
    djnz .-0
    djnz .-0
    pop bc
    #endif // delay
    ld h,d
    ld l,e
    inc e
    set 7,h
    set 6,h

;; set z-buff address
;    set 5,h
;    ex af,af
;    ld a,(_current_window_id)

;; compare with z-buffer
;    cp (hl)
;    jr c,0$

;; put new z and print symbol
;    ld (hl),a
;    res 5,h
;    ex af,af
    ld (hl),a
    set 7,l
    ld (hl),c
    ret

;0$: ex af,af
;    ret

strprnlen:
0$: ld a,(hl)
    inc hl
    or a
    ret z
    push bc
    ld bc,(sym_attr)
    push hl
    call sym_prn
    ld a,e
    ld (cur_x),a
    pop hl
    pop bc
    djnz 0$
    ret

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; print zero-terminated string
;; i:
;;   HL - string address
;;   DE - YX coords
;; string format:
;;      0x07 (BYTE) - ink color (\a)
;;      0x08 (BYTE) - paper color (\b)
;;      0x09 (BYTE) - shift right on (BYTE) symbols (\t)
;;      0x0C - invert attribute (\f)
;;      0x0D - next line (\r)
;;      0x0E - center align string
;;      0x0F - right align string
;;      0xFE (BYTE) - string link
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
strprnz:
    ld a,(hl)
    inc hl
    or a
    ret z
    cp #0x07                ; \a
    jp z,strprnz_ink
    cp #0x08                ; \b
    jp z,strprnz_paper
    cp #0x09                ; \t
    jp z,strprnz_tab
    cp #0x0C                ; \f
    jp z,strprnz_invert
    cp #0x0D                ; \r
    jp z,strprnz_r
    cp #0x0E                ; center align
    jp z,strprnz_center
    cp #0x0F                ; right align
    jp z,strprnz_right
    cp #0xFE                ; string link
    jp z,strprnz_link

    ld bc,(sym_attr)
    push hl
    call sym_prn
    ld a,e
    ld (cur_x),a
    pop hl
    jr strprnz

strprnz_ink:
    ld a,(bg_attr)
    and #0xF0
    ld c,a
    ld a,(hl)
    inc hl
    and #0x0F
    or c
    ld (sym_attr),a
    jr strprnz

strprnz_paper:
    ld a,(sym_attr)
    and #0x0F
    ld c,a
    ld a,(hl)
    inc hl
    and #0x0F
    rlca
    rlca
    rlca
    rlca
    or c
    ld (sym_attr),a
    ld (bg_attr),a
    jr strprnz

strprnz_invert:
    ld a,(sym_attr)
    rlca
    rlca
    rlca
    rlca
    ld (sym_attr),a
    ld (bg_attr),a
    jr strprnz

strprnz_r:
    ld a,(win_attr)
    ld (bg_attr),a
    ld (sym_attr),a
    ld a,(win_x)
    ld (cur_x),a
    ld e,a
    ld a,(cur_y)
    inc a
    ld (cur_y),a
    ld d,a
    jp strprnz

strprnz_tab:
    ld a,(hl)
    inc hl
    add a,e
    ld e,a
    ld (cur_x),a
    jp strprnz

strprnz_center:
    push hl
    call strlen
    pop hl
    ld a,(win_w)
    sub b
    srl a
    ld b,a
    ld a,(win_x)
    add a,b
    ld e,a
    ld (cur_x),a
    jp strprnz

strprnz_right:
    push hl
    call strlen
    pop hl
    ld a,(win_w)
    ld c,a
    ld a,(win_x)
    add a,c
    sub b
    ld e,a
    ld (cur_x),a
    jp strprnz

strprnz_link:
    ld a,(hl)
    inc hl
    push hl
    ld l,a
    ld h,#0
    add hl,hl
    ld bc,(linked_ptr)
    add hl,bc
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    or h
    call nz,strprnz
    pop hl
    jp strprnz

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; i:
;;  HL - string address
;; o:
;;  B - string lenght
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
strlen:
    ld b,#0xFF
1$: ld a,(hl)
    inc hl

;; skip codes
    cp #0x07
    jr z,2$
    cp #0x08
    jr z,2$
    cp #0x09
    jr z,6$
    cp #0x0C
    jr z,2$
    cp #0x0E
    jr z,4$
    cp #0x0F
    jr z,4$
    cp #0xFE
    jr z,3$

    inc b
4$: cp #0x0D
    ret z
    or a
    jr nz,1$
    ret

2$: inc hl
    jr 1$
5$: inc hl
    inc hl
    jr 1$
6$: ld a,(hl)
    inc hl
    add a,b
    ld b,a
    jr 1$

;; linked message
3$: dec b
    ld a,(hl)
    inc hl
    push hl
    ld l,a
    ld h,#0
    add hl,hl
    push bc
    ld bc,(linked_ptr)
    add hl,bc
    pop bc
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    call 1$
    pop hl
    jr 1$
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
u8 gcGetMessageMaxLength(u8 *msg) __naked __z88dk_fastcall
{
    msg;            // to avoid SDCC warning

    __asm
    push hl
    call _gcGetMessageLines
    ld d,l
    pop hl
    ld c,#0
1$: ld a,d
    or a
    jr z,0$
    call strlen
    ld a,c
    cp b
    jr nc,.+2+1
    ld c,b
    dec d
    jr 1$

0$: ld l,c
    ret
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
u8 gcGetMessageLines(u8 *msg) __naked __z88dk_fastcall
{
    msg;            // to avoid SDCC warning

    __asm
    ld b,#0
1$: ld a,(hl)
    or a
    jr z,0$
    inc hl
    cp #0x0D
    jr nz,1$
    inc b
    jr 1$
0$: inc b
    ld l,b
    ret
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintSymbol(u8 x, u8 y, u8 sym, u8 attr) __naked
{
    x, y;           // to avoid SDCC warning
    sym, attr;      // to avoid SDCC warning

    __asm
    ld hl,#2
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld a,(hl)
    inc hl
    ld c,(hl)
    jp sym_prn
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintMessage(u8 *msg) __naked __z88dk_fastcall
{
    msg;            // to avoid SDCC warning

    __asm
    ld a,(cur_x)
    ld e,a
    ld a,(cur_y)
    ld d,a
    jp strprnz
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcGotoXY(u8 x, u8 y) __naked
{
    x, y;           // to avoid SDCC warning
    __asm
    ld hl,#2
    add hl,sp
    ld a,(hl)   ; X
    inc hl
    ld (cur_x),a
    ld a,(hl)   ; Y
    inc hl
    ld (cur_y),a
    ret
    __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcSetFontSym(u8 sym, u8 *udg) __naked
{
    sym, udg;       // to avoid SDCC warning

    __asm
;; set font page
    ld bc,#0x13AF
    ld a,(_vpage)
    xor #0x01
    out (c),a

    ld hl,#2
    add hl,sp
    ld a,(hl)
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    push de

    ld  l,a
    ld  h,#0
    add hl,hl
    add hl,hl
    add hl,hl
    ld de,#0xC000
    add hl,de
    ex de,hl
    pop hl

;; exchange symbol between udg&font
    ld b,#8
1$: ld c,(hl)
    ld a,(de)
    ld (hl),a
    ld a,c
    ld (de),a
    inc hl
    inc de
    djnz 1$
    ret
  __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
// setting dark-grey palette to color 8(for shadow) in palette 0x0F
void gcSetPalette()
{
    __asm
    ld a,#0x10
    ld bc,#0x15AF
    out (c),a
    ld hl,#0x2108
    ld ((0x0F*32)+(8*2)),hl
    xor a
    out (c),a
    ret
    __endasm;
}
