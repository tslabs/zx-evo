//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                       ::
//::               by dr_max^gc (c)2018-2019                 ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#include <string.h>
#include "defs.h"
#include "gcWin.h"

void gcVars (void)
{
  __asm
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    .globl strlen, strprnz, sym_prn, sym_prns
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; !!! must math with structures in gcWin.h !!!
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; struct GC_SVM_FLAG_t
    svmf_nowrap_mask        .equ    #0x40
    svmf_exit_mask          .equ    #0x80

;; typedef GC_SVM_TAG_t
    svmt_text               .equ    #00
    svmt_option             .equ    #01
    svmt_callback           .equ    #02

;; typedef GC_SVM_OPTION_t
    svmopt_var              .equ    #00
    svmopt_text             .equ    #02

    svmrc_key               .equ    #0xFD
    svmrc_tab               .equ    #0xFE
    svmrc_exit              .equ    #0xFF

    svm_cbkey_rc_none       .equ    #0x00
    svm_cbkey_rc_redraw     .equ    #0xFE
    svm_cbkey_rc_exit       .equ    #0xFF

;; struct GC_SVMENU
;; simple vertical menu offsets
    svm_flags               .equ    #00 ; byte
    svm_attr                .equ    #01 ; byte
    svm_margin              .equ    #02 ; byte
    svm_cur_pos             .equ    #03 ; byte
    svm_win_pos             .equ    #04 ; byte
    svm_win_cnt             .equ    #05 ; byte
    svm_all_cnt             .equ    #06 ; byte
    svm_cb_cursor           .equ    #07 ; word
    svm_cb_keys             .equ    #09 ; word
    svm_cb_cross            .equ    #11 ; word
    svm_lines               .equ    #13 ; word
    svm_txt_list            .equ    #15 ; word
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
    DI_PROGRESSBAR          .equ    #13
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    SYM_TRI                 .equ    #0xD8
    SYM_RADIO               .equ    #0xD0
    SYM_CHECK               .equ    #0xD4
    SYM_BTNUP               .equ    #0xF2
    SYM_BTNDN               .equ    #0xF4
    SYM_PRGS                .equ    #0xB0
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    KB_TRU                  .equ    #0x04    // (CS+3)
    KB_PGUP                 .equ    #0x04
    KB_INV                  .equ    #0x05    // (CS+4)
    KB_PGDN                 .equ    #0x05
    KB_CAPS                 .equ    #0x06    // (CS+2)
    KB_EDIT                 .equ    #0x07    // (CS+1)
    KB_ESC                  .equ    #0x07
    KB_LEFT                 .equ    #0x08    // (CS+5)
    KB_RIGHT                .equ    #0x09    // (CS+8)
    KB_DOWN                 .equ    #0x0A    // (CS+6)
    KB_UP                   .equ    #0x0B    // (CS+7)
    KB_BACK                 .equ    #0x0C    // (CS+0)
    KB_ENTER                .equ    #0x0D
    KB_EXT                  .equ    #0x0E    // (SS+CS)
    KB_TAB                  .equ    #0x0E
    KB_GRAPH                .equ    #0x0F    // (CS+9)
    KB_DEL                  .equ    #0x0F
    KB_INS                  .equ    #0x10    // (SS+W)
    KB_HOME                 .equ    #0x11    // (SS+Q)
    KB_END                  .equ    #0x12    // (SS+E)
    KB_SSENT                .equ    #0x13    // (SS+ENTER)
    KB_RALT                 .equ    #0x13
    KB_SSSP                 .equ    #0x14    // (SS+SPACE)
    KB_LALT                 .equ    #0x14
    KB_CSENT                .equ    #0x15    // (CS+ENTER)
    KB_BREAK                .equ    #0x16    // (CS+SPACE)
    KB_SPACE                .equ    #0x20
    KB_F1                   .equ    #0xC0
    KB_F2                   .equ    #0xC1
    KB_F3                   .equ    #0xC2
    KB_F4                   .equ    #0xC3
    KB_F5                   .equ    #0xC4
    KB_F6                   .equ    #0xC5
    KB_F7                   .equ    #0xC6
    KB_F8                   .equ    #0xC7
    KB_F9                   .equ    #0xC8
    KB_F10                  .equ    #0xC9
    KB_F11                  .equ    #0xCA
    KB_POWER                .equ    #0xCB
    KB_SLEEP                .equ    #0xCD
    KB_WAKE                 .equ    #0xCE
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; macros
    .macro  MAC_LD_IXHL
    push hl
    pop ix
    .endm

    .macro  MAC_LDF_HLHL
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    .endm

    .macro  MAC_ITEMCOORD_DE
    ld a,(win_x)
    add a,<#di_x (ix)
    ld e,a              ; X coord
    ld a,(win_y)
    add a,<#di_y (ix)
    ld d,a              ; Y coord
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
win_x::
    .db 0
win_y::
    .db 0
win_w::
    .db 0
win_h::
    .db 0
win_attr::
    .db 0
frm_attr::
    .db 0
;;
cur_x::
    .db 0
cur_y::
    .db 0
;;
sym_attr::
    .db 0
bg_attr::
    .db 0
inv_attr::
    .db 0
;;
_mouse_type::
    .db 0
linked_ptr::
    .dw 0
_current_menu_ptr::
    .dw 0
_current_window_ptr::
    .dw 0
_current_dialog_ptr::
    .dw 0
frame_set_addr::
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
ascbuff::
    .ds 32
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

    call _ps2_init

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
    push de
    push hl

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

2$:
    ld a,(_mouse_type)
    ld l,a
    call _gcMouseUpdate
    pop hl
    pop de
    pop bc
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
        gcPrintSimpleVMenu((GC_SVMENU_t*)ptr);
        rc = gcSimpleVMenu((GC_SVMENU_t*)ptr);
    break;

    case GC_WND_DIALOG:
        rc = gcDialog((GC_DIALOG_t*)ptr);
    break;

    case GC_WND_INFO:
        gcPrintDialog((GC_DIALOG_t*)ptr);
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
        *((u8*)(btnOk.flags)) = 0;
        btnOk.hotkey = 'o';
        btnOk.select = 0;
        btnOk.name = PAPER_BRIGHT_WHITE INK_BRIGHT_RED"O"INK_BLACK"k";
        break;

    case MB_OKCANCEL:
        dlg.all_count = 3;
        dlg.act_count = 2;
        dlgItemList[0] = &btnOk;
        dlgItemList[1] = &btnCancel;
        dlgItemList[2] = &txt;

        btnOk.type = DI_BUTTON;
        btnOk.id = BUTTON_OK;
        btnOk.x = (len>>1) - 12;
        btnOk.y = i + 2;
        btnOk.width = 10;
        btnOk.hight = 0;
        *((u8*)(btnOk.flags)) = 0;
        btnOk.hotkey = 'o';
        btnOk.select = 0;
        btnOk.name = PAPER_BRIGHT_WHITE INK_BRIGHT_RED"O"INK_BLACK"k";

        btnCancel.type = DI_BUTTON;
        btnCancel.id = BUTTON_CANCEL;
        btnCancel.x = (len>>1) + 2;
        btnCancel.y = i + 2;
        btnCancel.width = 10;
        btnCancel.hight = 0;
        *((u8*)(btnCancel.flags)) = 0;
        btnCancel.hotkey = 'c';
        btnCancel.select = 0;
        btnCancel.name = PAPER_BRIGHT_WHITE INK_BRIGHT_RED"C"INK_BLACK"ancel";
        break;

    case MB_YESNO:
        dlg.all_count = 3;
        dlg.act_count = 2;
        dlgItemList[0] = &btnOk;
        dlgItemList[1] = &btnCancel;
        dlgItemList[2] = &txt;

        btnOk.type = DI_BUTTON;
        btnOk.id = BUTTON_YES;
        btnOk.x = (len>>1) - 12;
        btnOk.y = i + 2;
        btnOk.width = 11;
        btnOk.hight = 0;
        *((u8*)(btnOk.flags)) = 0;
        btnOk.hotkey = 'y';
        btnOk.select = 0;
        btnOk.name = PAPER_BRIGHT_WHITE INK_BRIGHT_RED"Y"INK_BLACK"es";

        btnCancel.type = DI_BUTTON;
        btnCancel.id = BUTTON_NO;
        btnCancel.x = (len>>1) + 2;
        btnCancel.y = i + 2;
        btnCancel.width = 10;
        btnCancel.hight = 0;
        *((u8*)(btnCancel.flags)) = 0;
        btnCancel.hotkey = 'n';
        btnCancel.select = 0;
        btnCancel.name = PAPER_BRIGHT_WHITE INK_BRIGHT_RED"N"INK_BLACK"o";
        break;

    case MB_RETRYABORTIGNORE:
        dlg.all_count = 4;
        dlg.act_count = 3;
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
        *((u8*)(btnRetry.flags)) = 0;
        btnRetry.hotkey = 'r';
        btnRetry.select = 0;
        btnRetry.name = PAPER_BRIGHT_WHITE INK_BRIGHT_RED"R"INK_BLACK"etry";

        btnAbort.type = DI_BUTTON;
        btnAbort.id = BUTTON_ABORT;
        btnAbort.x = (len>>1) - 5;
        btnAbort.y = i + 2;
        btnAbort.width = 11;
        btnAbort.hight = 0;
        *((u8*)(btnAbort.flags)) = 0;
        btnAbort.hotkey = 'a';
        btnAbort.select = 0;
        btnAbort.name = PAPER_BRIGHT_WHITE INK_BRIGHT_RED"A"INK_BLACK"bort";

        btnIgnore.type = DI_BUTTON;
        btnIgnore.id = BUTTON_IGNORE;
        btnIgnore.x = (len>>1) + 8;
        btnIgnore.y = i + 2;
        btnIgnore.width = 10;
        btnIgnore.hight = 0;
        *((u8*)(btnIgnore.flags)) = 0;
        btnIgnore.hotkey = 'i';
        btnIgnore.select = 0;
        btnIgnore.name = PAPER_BRIGHT_WHITE INK_BRIGHT_RED"I"INK_BLACK"gnore";
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
void gcPrintSimpleVMenu(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall
{
    svmnu;      // to avoid SDCC warning

  __asm
    push ix
    MAC_LD_IXHL         ; IX - simple vertical menu descriptor

    push hl
    call svmnu_cb_initial_list
    pop hl
    call print_svm_cursor
;;
    pop ix
    ret
  __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcInitSimpleVMenu(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall
{
    svmnu;      // to avoid SDCC warning

  __asm
    push ix
    MAC_LD_IXHL         ; IX - simple vertical menu descriptor

    call svmnu_cb_initial_list
    pop ix
    ret
  __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
u8 gcSimpleVMenu(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall
{
    svmnu;      // to avoid SDCC warning

  __asm
    MAC_LD_IXHL         ; IX - simple vertical menu descriptor

    call svmnu_scanlist
    call print_svm_cursor
    call svmnu_cb_cursor

svmnu_lp:
    ei
    halt

    call svmnu_get_mouse

    call _gcGetKey
    ld a,l
    or a
    jr z,svmnu_lp
    cp #KB_DOWN
    jr z,svmnu_dn
    cp #KB_UP
    jr z,svmnu_up
    cp #KB_LEFT
    jr z,svmnu_begin
    cp #KB_RIGHT
    jr z,svmnu_end
    cp #KB_ENTER
    jr z,svmnu_ent
    cp #KB_EXT
    jr z,svmnu_tab

    call svmnu_cb_keys
    ld a,l
    cp #svm_cbkey_rc_exit
    jr nz,svmnu_lp
    ld l,#svmrc_key
    ret
;;::::::::::::::::::::::::::::::
svmnu_tab:
    ld a,<#svm_flags (ix)
    and #svmf_exit_mask
    jr z,svmnu_lp
    call restore_svm_cursor
    ld l,#svmrc_tab
    ret
;;::::::::::::::::::::::::::::::
svmnu_ent:
    call restore_svm_cursor
    ld l,<#svm_cur_pos (ix)
    ret
;;::::::::::::::::::::::::::::::
svmnu_begin:
    call restore_svm_cursor
    xor a
svmnu_lp1:
    ld <#svm_cur_pos (ix),a
svmnu_lp2:
    call print_svm_cursor
    call svmnu_cb_cursor
    jr svmnu_lp
;;::::::::::::::::::::::::::::::
svmnu_end:
    call restore_svm_cursor
    ld a,<#svm_all_cnt (ix)
    cp <#svm_win_cnt (ix)
    jr c,0$
    ld a,<#svm_win_cnt (ix)
0$: dec a
    jr svmnu_lp1
;;::::::::::::::::::::::::::::::
svmnu_dn:
    call restore_svm_cursor
    ld a,<#svm_cur_pos (ix)
    inc a
    cp <#svm_all_cnt (ix)
    jr nc,svmnu_lp2
    cp <#svm_win_cnt (ix)
    jr nz,svmnu_lp1
    call svmnu_cb_bottom
    ld a,<#svm_flags (ix)
    and #svmf_nowrap_mask
    jr z,svmnu_lp1
    ld a,<#svm_cur_pos (ix)
    jr svmnu_lp1
;;::::::::::::::::::::::::::::::
svmnu_up:
    call restore_svm_cursor
    ld a,<#svm_cur_pos (ix)
    dec a
    jp p,svmnu_lp1
    call svmnu_cb_top
    ld a,<#svm_flags (ix)
    and #svmf_nowrap_mask
    ld a,<#svm_cur_pos (ix)
    jr nz,svmnu_lp1
    ld a,<#svm_win_cnt (ix)
    dec a
    jr svmnu_lp1
;;::::::::::::::::::::::::::::::
svmnu_cb_cross_get_ptr:
    ld e,<#svm_cb_cross (ix)
    ld d,<#svm_cb_cross+1 (ix)
    ld a,e
    or d
    ret
;;
;; in:  C - offset
svmnu_cb_cross_get_num_list:
    ld a,<#svm_cur_pos (ix)
    add a,<#svm_win_pos (ix)
    add a,c
    ret
;;
;; in:  C - offset
svmnu_cb_cross_set_coords:
    ld a,(win_y)
    add a,<#svm_margin (ix)
    add a,<#svm_cur_pos (ix)
    add a,c
    ld (cur_y),a
    ld a,(win_x)
    ld (cur_x),a
    ret
;;::::::::::::::::::::::::::::::
svmnu_cb_initial_list:
    call svmnu_cb_cross_get_ptr
    ret z
    ld c,#0x00
0$: call svmnu_cb_cross_set_coords
    call svmnu_cb_cross_get_num_list
    call svm_callback
    ld a,<#svm_cur_pos (ix)
    add a,<#svm_win_pos (ix)
    inc c
    add a,c
    cp <#svm_all_cnt (ix)
    ret z
    sub a,<#svm_win_pos (ix)
    cp <#svm_win_cnt (ix)
    jr nz,0$
    ret
;;::::::::::::::::::::::::::::::
;; bottom border callback
svmnu_cb_bottom:
    call svmnu_cb_cross_get_ptr
    ret z
;;
    push ix
    push iy
;;
;; store current window pointer
    ld hl,(_current_window_ptr)
    push hl

    ld a,<#svm_cur_pos (ix)
    inc a
    add a,<#svm_win_pos (ix)
    cp <#svm_all_cnt (ix)
    jr nc,0$
    inc <#svm_win_pos (ix)
    push af
    push de
    call _gcScrollUpWindow
;;
    ld c,#0x00
    call svmnu_cb_cross_set_coords
;;
    pop de
    pop af
    call svm_callback
0$:
;; restore current window pointer
    pop hl
    ld (_current_window_ptr),hl
    call _gcSelectWindow
;;
    pop iy
    pop ix
    ret
;;::::::::::::::::::::::::::::::
;; top border callback
svmnu_cb_top:
    call svmnu_cb_cross_get_ptr
    ret z
;;
    push ix
    push iy
;;
;; store current window pointer
    ld hl,(_current_window_ptr)
    push hl

    ld a,<#svm_cur_pos (ix)
    add a,<#svm_win_pos (ix)
    jr z,0$
    dec a
    dec <#svm_win_pos (ix)
    push af
    push de
;;
    ld c,#0x00
    call svmnu_cb_cross_set_coords
;;
    call _gcScrollDownWindow
    pop de
    pop af
    call svm_callback
0$:
;; restore current window pointer
    pop hl
    ld (_current_window_ptr),hl
    call _gcSelectWindow
;;
    pop iy
    pop ix
    ret

;;::::::::::::::::::::::::::::::
;; in:  DE - callback addr
;;      IX - SVM pointer
;;      A - argument
;;
svm_callback:
    push ix
    push hl
    push de
    push bc
    push af
;; store arg
    push af
    inc sp
;; store SVM pointer
    push ix
;;
    ld hl,#0$
    push hl
    ex de,hl
    jp (hl)
0$:
;; restore stack
    pop af
    pop af
    dec sp
;;
    pop af
    pop bc
    pop de
    pop hl
    pop ix
    ret
;;::::::::::::::::::::::::::::::
;; cursor moving callback
svmnu_cb_cursor:
    ld e,<#svm_cb_cursor (ix)
    ld d,<#svm_cb_cursor+1 (ix)
    ld a,e
    or d
    ret z
;;
    push ix
    push iy
;; store current window pointer
    ld hl,(_current_window_ptr)
    push hl

    call svm_callback

;; restore current window pointer
    pop hl
    ld (_current_window_ptr),hl
    call _gcSelectWindow
;;
    pop iy
    pop ix
    ret
;;::::::::::::::::::::::::::::::
;; key pressed callback
svmnu_cb_keys:
    push ix
    push iy
;;
    ld e,<#svm_cb_keys (ix)
    ld d,<#svm_cb_keys+1 (ix)
    ld a,e
    or d
    jr z,0$
;;
;; store current window pointer
    ld bc,(_current_window_ptr)
    push bc

    ld a,l
    call svm_callback

;; restore current window pointer
    pop de
    ld (_current_window_ptr),de
    push hl
    ex de,hl
    call _gcSelectWindow
    pop hl
;;
    ld a,l
    cp #svm_cbkey_rc_redraw
    jr nz,1$
    call svmnu_scanlist
    call print_svm_cursor
;;
0$: ld l,#0x00
1$: pop iy
    pop ix
    ret

;;::::::::::::::::::::::::::::::
svmnu_scanlist:
    ld l,<#svm_lines (ix)
    ld h,<#svm_lines+1 (ix)
    ld a,l
    or h
    ret z
    xor a
0$: push af
    call svmnu_list_item
    pop af
    inc a
    cp <#svm_win_cnt (ix)
    jr nz,0$
    ret

;;i:    A - svm item number
svmnu_list_item:
    push af
    ld c,a

;; set coordinates
    ld a,(win_y)
    add a,<#svm_margin (ix)
    add a,c
    ld h,a              ; Y coord
    ld a,(win_x)
    ld l,a              ; X coord
    push hl
    call _gcGotoXY
    pop hl

    pop af
    add a,a
    ld c,a
    ld b,#0x00
    ld l,<#svm_lines (ix)
    ld h,<#svm_lines+1 (ix)
    add hl,bc
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    ld a,(hl)
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    cp #svmt_text
    jr z,svmnu_text
    cp #svmt_option
    jr z,svmnu_option
    cp #svmt_callback
    jr z,svmnu_callback
    ret

svmnu_text:
    ex de,hl
    jp _gcPrintString

svmnu_option:
    ex de,hl
    push ix
    push hl
    pop ix
    ld l,<#svmopt_var (ix)
    ld h,<#svmopt_var+1 (ix)
    ld a,(hl)
    push af
    ld l,<#svmopt_text (ix)
    ld h,<#svmopt_text+1 (ix)
    call _gcPrintString
    pop af
    pop ix
    ld l,a
    ld h,#0x00
    add hl,hl
    ld e,<#svm_txt_list (ix)
    ld d,<#svm_txt_list+1 (ix)
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    jr svmnu_text

svmnu_callback:
    ex de,hl
    ld de,#0$
    push de
    push ix
    jp (hl)
0$: pop ix
    ret
;;::::::::::::::::::::::::::::::
svmnu_get_mouse:
;; check bounding box
    call _gcGetMouseYS
    ld c,l
    ld h,<#svm_win_cnt (ix)
    ld a,(win_y)
    add a,<#svm_margin (ix)
    add a,h
    dec a
    sub l
    jr c,0$
    cp h
    jr nc,0$

    call _gcGetMouseXS
    ld a,(win_w)
    ld h,a
    ld a,(win_x)
    dec a
    add a,h
    sub l
    jr c,0$
    cp h
    jr nc,0$

;; click inside the window
    ld a,(win_y)
    add a,<#svm_margin (ix)
    ld l,a
    ld a,c
    sub l
    cp <#svm_win_cnt (ix)
    jr nc,0$

    ld c,a
    ld a,#1
    ld (_mouse_type),a
    ld a,(_mouse_lmb)
    or a
    ret nz
    ld a,c

    push af
    call restore_svm_cursor
    pop af
    ld <#svm_cur_pos (ix),a

    call print_svm_cursor

    halt

;; pop ret addr
    pop hl
    ld l,<#svm_cur_pos (ix)
    ret

0$: xor a
    ld (_mouse_type),a
    ret
  __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcRestoreSVMCursor(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall
{
    svmnu;              // to avoid SDCC warning

    __asm
    MAC_LD_IXHL
;; IX - simple vertical menu descriptor

restore_svm_cursor:
    ld a,(win_y)
    add a,<#svm_margin (ix)
    add a,<#svm_cur_pos (ix)
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
    MAC_LD_IXHL
;; IX - simple vertical menu descriptor

print_svm_cursor:
    ld a,(win_y)
    add a,<#svm_margin (ix)
    add a,<#svm_cur_pos (ix)
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
u8 gcFindClickItem(GC_DIALOG_t *dlg) __naked __z88dk_fastcall
{
    dlg;                    // to avoid SDCC warning

  __asm
    push ix
    call _gcFindItem
    ld a,(_mouse_lmb)
    or a
    jr z,.+2+2
    ld l,#0xFF
    pop ix
    ret
  __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
u8 gcFindItem(GC_DIALOG_t *dlg) __naked __z88dk_fastcall
{
    dlg;                    // to avoid SDCC warning

  __asm
    push ix

    MAC_LD_IXHL             ; IX - dialog descriptor

    xor a
0$: push af
    push ix
    call get_dialog_item_addr
    MAC_LD_IXHL             ; IX - dialog item descriptor

    call _gcGetMouseXS
    ld c,l
    call _gcGetMouseYS
    ld b,l

    MAC_ITEMCOORD_DE

    ld a,b
    cp d
    jr nz,9$
    ld a,c
    sub e
    jr c,9$
    cp <#di_width (ix)
    jr c,1$

9$: pop ix
    pop af
    inc a
    cp <#dlg_act_count (ix)
    jr nz,0$
2$: ld l,#0xFF
    xor a
    ld (_mouse_type),a
    pop ix
    ret

3$: pop ix
    pop af
    jr 2$

;; check GREY bit
1$: ld a,<#di_flags (ix)
    and #dif_grey_mask
    jr nz,3$

    ld a,<#di_type (ix)
    ld l,#1
    cp #DI_EDIT
    jr nz,8$
    inc l
8$: ld a,l
    ld (_mouse_type),a
    pop ix
    pop af
    ld l,a
    pop ix
    ret
  __endasm;
}

u8 gcFindHotkey(GC_DIALOG_t *dlg) __naked __z88dk_fastcall
{
    dlg;                    // to avoid SDCC warning

  __asm
    push ix

    MAC_LD_IXHL             ; IX - dialog descriptor

    xor a
0$: push af
    push ix
    call get_dialog_item_addr
    MAC_LD_IXHL             ; IX - dialog item descriptor

;; check GREY bit
    ld a,<#di_flags (ix)
    and #dif_grey_mask
    jr nz,1$

    ld a,(_lastkey)
    or a
    jr z,1$
    ld c,a
    ld a,<#di_hotkey (ix)
    cp c
    jr nz,1$
    pop ix
    pop af
    ld l,a
    pop ix
    ret

1$: pop ix
    pop af
    inc a
    cp <#dlg_act_count (ix)
    jr nz,0$
    ld l,#0xFF
    pop ix
    ret
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

    MAC_LD_IXHL             ; IX - dialog descriptor

    call _gcPrintDialog

dialog_lp:
    ld a,<#dlg_current (ix)
    call get_dialog_item_addr
    call _gcPrintDialogCursor

dialog_lp1:
    xor a
    dec a
    ld (_mouse_lmb),a
    ei
    halt

    ld a,<#dlg_current (ix)
    call get_dialog_item_addr
    push hl

    ld hl,(_current_dialog_ptr)
    call _gcFindClickItem
    ld a,l
    cp #0xFF
    jp nz,dialog_select_mouse

    call _gcGetKey
    push hl

    ld hl,(_current_dialog_ptr)
    call _gcFindHotkey
    ld a,l
    cp #0xFF
    pop hl
    jp nz,dialog_select_hotkey

    ld a,<#dlg_flag (ix)
    and #dlgf_cursor_mask
    jr nz,dialog_leftright

    ld a,l
    cp #KB_PGDN
    jp z,dialog_pgdn
    cp #KB_PGUP
    jp z,dialog_pgup
    cp #KB_DOWN
    jp z,dialog_dn
    cp #KB_RIGHT
    jp z,dialog_pgdn
    cp #KB_LEFT
    jp z,dialog_pgup
    cp #KB_UP
    jp z,dialog_up
    cp #KB_ENTER
    jp z,dialog_ent
    cp #KB_SPACE
    jp z,dialog_sp
    pop hl
    jr dialog_lp1

dialog_leftright:
    ld a,l
    cp #KB_LEFT
    jp z,dialog_up
    cp #KB_RIGHT
    jp z,dialog_dn
    cp #KB_ENTER
    jp z,dialog_ent
    cp #KB_SPACE
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
    MAC_LDF_HLHL
    pop de
    ret
;;:::::::::::::::::::::::::::::
dialog_select_hotkey:
    pop hl
;; HL - dialog item address
    push af
    ld a,<#dlg_current (ix)
    call get_dialog_item_addr
    call _gcRestoreDialogCursor
    pop af
    ld <#dlg_current (ix),a
    call get_dialog_item_addr
;; store item address
    push hl
;;
    call _gcPrintDialogCursor

    ex (sp),ix
;; now IX - item address
    ld a,<#di_type (ix)
;; exchange back
    ex (sp),ix

    cp #DI_EDIT
    jp z,dialog_ent
    cp #DI_BUTTON
    jr z,dialog_ent
    cp #DI_LISTBOX
    jr z,dialog_ent
    jp dialog_sp

;;:::::::::::::::::::::::::::::
dialog_select_mouse:
    pop hl
;; HL - dialog item address

    push af
    ld a,<#dlg_current (ix)
    call get_dialog_item_addr
    call _gcRestoreDialogCursor
    pop af
    ld <#dlg_current (ix),a
    call get_dialog_item_addr

;; store item address
    push hl
;;
    call _gcPrintDialogCursor

0$: halt
    ld a,(_mouse_lmb)
    or a
    jr z,0$

    ex (sp),ix
;; now IX - item address
    ld a,<#di_type (ix)
;; exchange back
    ex (sp),ix

    cp #DI_EDIT
    jp z,dialog_ent
    cp #DI_BUTTON
    jr z,dialog_ent
    cp #DI_LISTBOX
    jr z,dialog_ent
    jp dialog_sp

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

    MAC_LD_IXHL             ; IX - dialog item addr

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
    ld a,(win_x)
    add a,<#di_x (ix)
    ld e,a
    ld a,(win_y)
    add a,<#di_y (ix)
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
;;void gcDrawWindow(u8 id, u8 x, u8 y, u8 width, u8 hight, WIN_COLORS_t attr, GC_FRM_TYPE_t frame_type, WIN_COLORS_t frame_attr) {

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
    MAC_LDF_HLHL
    call strprnz
    pop de
    inc d
    pop bc
    pop af
    inc a
    djnz 0$

;; build temp simple vertical menu onto stack
    ld e,<#di_var (ix)
    ld d,<#di_var+1 (ix)
;; list
    ld hl,#0
    push hl     ;txt_list
    push hl     ;lines
;; callbacks
    push hl     ;cb_cross
    push hl     ;cb_keys
    push hl     ;cb_cursor
    ld c,<#di_select (ix)
    ld b,c
    push bc     ; svm_win_cnt&svm_all_cnt
    xor a
;; svm_win_pos
    push af
    inc sp
    ld a,(de)
;; svm_cur_pos
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
;; svm_flags
    xor a
    push af
    inc sp

    add hl,sp   ; HL - svmenu address
    push de
    call _gcSimpleVMenu
    pop de

;; save selected
    ld a,l
    cp #0xFF
    jr z,.+2+1
    ld (de),a

;; restore stack
    ld hl,#17
    add hl,sp
    ld sp,hl

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

    MAC_LD_IXHL             ; IX - dialog item addr

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

    MAC_LD_IXHL             ; IX - dialog descriptor

    ld a,<#dlg_current (ix)
0$: or a
    jr z,4$
    dec a
1$: push af
    push ix
    call get_dialog_item_addr
    MAC_LD_IXHL             ; IX - dialog item descriptor

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

    MAC_LD_IXHL             ; IX - dialog descriptor

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
    MAC_LD_IXHL             ; IX - dialog item descriptor

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

    MAC_LD_IXHL             ; IX - dialog descriptor

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
    MAC_LD_IXHL             ; IX - dialog item descriptor

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

    MAC_LD_IXHL             ; IX - dialog descriptor

    ld a,<#dlg_current (ix)
0$: or a
    jr z,3$
    dec a
1$: push af
    push ix
    call get_dialog_item_addr
    MAC_LD_IXHL             ; IX - dialog item descriptor

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

    MAC_LD_IXHL             ; IX - dialog descriptor

    ld b,<#dlg_all_count (ix)

0$: push bc
    push ix
    push de

    ld a,b
    dec a
    call get_dialog_item_addr
;; HL - dialog item address

    MAC_LD_IXHL             ; IX - dialog item descriptor

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

    MAC_LD_IXHL             ; IX - dialog descriptor

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

    MAC_LD_IXHL             ; IX - dialog descriptor

;; set vars
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

    MAC_LD_IXHL         ; IX - dialog item descriptor

    MAC_ITEMCOORD_DE

    ld b,<#di_width (ix)
1$: call get_attr
    and #0x0F
    ld c,a
    ld a,<#di_type (ix)
    cp #DI_BUTTON
    jr nz,2$
    ld a,(cfg_btn_focus_attr)
    and #0xF0
    or c
    jr 3$
2$: cp #DI_LISTBOX
    jr nz,4$
    ld a,(cfg_listbox_focus_attr)
    jr 3$
4$: cp #DI_EDIT
    jr z,6$
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

    MAC_LD_IXHL         ; IX - dialog item descriptor

    MAC_ITEMCOORD_DE

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

    MAC_LD_IXHL         ; IX - dalog item descriptor

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
    cp #DI_PROGRESSBAR
    jp z,print_item_progress

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
    cp #DI_PROGRESSBAR
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
    MAC_LDF_HLHL
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
    ld a,(win_attr)
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
print_item_progress:
    ld l,<#di_var (ix)
    ld h,<#di_var+1 (ix)  ; HL - var address
    ld a,(hl)
    push af
    inc sp
    ld l,<#di_select (ix)
    ld h,<#di_width (ix)
    push hl
    push de
    call _gcProgressBar
    ld hl,#5
    add hl,sp
    ld sp,hl
    ret

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
    MAC_LDF_HLHL
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
    MAC_LDF_HLHL
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
    MAC_LDF_HLHL
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
    MAC_LDF_HLHL
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
    _offset .equ #-5
    _buflen .equ #32
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
    ld a,#0x20
    ld b,#_buflen-1
    ld (hl),a
    inc hl
    djnz .-1-1

;;copy to buff
    ld l,<#_str (ix)
    ld h,<#_str+1 (ix)
    ld de,(_buff)
    ld b,#0
    ld c,<#_len (ix)
    ldir

    ;call edit_string_mouse1
    ;jr edit_string_lp
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
    call edit_string_mouse
    call _gcGetKey
    ld a,l
    cp #KB_LEFT
    jr z,edit_string_left
    cp #KB_RIGHT
    jr z,edit_string_right
    cp #KB_BACK
    jr z,edit_string_delete
    cp #KB_INS
    jr z,edit_string_ins
    cp #KB_DEL
    jr z,edit_string_del
    cp #KB_ENTER
    jp z,edit_string_ent
    cp #KB_HOME
    jr z,edit_string_home
    cp #KB_END
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
edit_string_mouse:
    ld hl,(_current_dialog_ptr)
    call _gcFindItem
    ld a,(_mouse_lmb)
    or a
    ret nz
edit_string_mouse1:
    call _gcGetMouseYS
    ld a,<#_y (ix)
    cp l
    jr nz,0$

    call _gcGetMouseXS
    ld a,l
    sub <#_x (ix)
    jr nc,1$
    cp <#_len (ix)
    jr c,1$

;; pop ret addr
0$:  pop hl
    jp edit_string_ent

1$: ld a,l
    sub <#_x (ix)
    cp <#_len (ix)
    jr nc,0$
    ld <#_offset (ix),a

;; pop ret addr
    pop hl
    jp edit_string_lp

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

void gcScrollUpWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall
{
    wnd;                // to avoid SDCC warning

  __asm
    push ix
    MAC_LD_IXHL
    ld l,<#width (ix)
    ld h,<#hight (ix)
    dec l
    dec l
    dec h
    dec h
    push hl
    ld l,<#x (ix)
    ld h,<#y (ix)
    inc l
    inc h
    push hl
    call _gcScrollUpRect
    pop af
    pop af
    pop ix
    ret
  __endasm;
}

void gcScrollDownWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall
{
    wnd;                // to avoid SDCC warning

  __asm
    push ix
    MAC_LD_IXHL
    ld l,<#width (ix)
    ld h,<#hight (ix)
    dec l
    dec l
    dec h
    dec h
    push hl
    ld l,<#x (ix)
    ld h,<#y (ix)
    inc l
    inc h
    push hl
    call _gcScrollDownRect
    pop af
    pop af
    pop ix
    ret
  __endasm;
}

void gcScrollUpRect(u8 x, u8 y, u8 width, u8 hight) __naked
{
    x,y,width,hight;        // to avoid SDCC warning

  __asm
    push ix
    ld ix,#4
    add ix,sp
    ld e,0(ix)
    ld d,1(ix)
    ld c,2(ix)
    ld a,3(ix)
    or a
    jr z,1$
    dec a
    jr z,1$
    ld b,a
    set 7,d
    set 6,d
    ld l,e
    ld h,d
    inc h
0$: push bc
    push hl
    push de
    ld a,c
    ex af,af
    ld b,#0
    ldir
    set 7,l
    set 7,e
    dec l
    dec e
    ex af,af
    ld c,a
    lddr
    pop de
    pop hl
    inc h
    inc d
    pop bc
    djnz 0$
    dec h
    ld b,a
    ld a,#0x20
    ld (hl),a
    inc l
    djnz .-1-1

1$: pop ix
    ret
  __endasm;
}

void gcScrollDownRect(u8 x, u8 y, u8 width, u8 hight) __naked
{
    x,y,width,hight;        // to avoid SDCC warning

  __asm
    push ix
    ld ix,#4
    add ix,sp
    ld e,0(ix)
    ld d,1(ix)
    ld c,2(ix)
    ld a,3(ix)
    or a
    jr z,1$
    dec a
    jr z,1$
    ld b,a
;
    ld a,d
    add a,b
    dec a
    ld d,a
;
    set 7,d
    set 6,d
    ld l,e
    ld h,d
    inc d
0$: push bc
    push hl
    push de
    ld a,c
    ex af,af
    ld b,#0
    ldir
    set 7,l
    set 7,e
    dec l
    dec e
    ex af,af
    ld c,a
    lddr
    pop de
    pop hl
    dec h
    dec d
    pop bc
    djnz 0$
    inc h
    ld b,a
    ld a,#0x20
    ld (hl),a
    inc l
    djnz .-1-1
1$: pop ix
    ret
  __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcDrawWindow(u8 id, u8 x, u8 y, u8 width, u8 hight, WIN_COLORS_t attr, GC_FRM_TYPE_t frame_type, WIN_COLORS_t frame_attr) __naked
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

    xor a
    ld (_mouse_type),a

;; select frameset
    ld de,#frame_set0
    ld a,<#_ft (ix)
    and #0x4F
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

3$: pop ix
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
void gcUpdateWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall
{
    wnd;                // to avoid SDCC warning

  __asm
    push hl
    push hl
    push hl
    call _gcSelectWindow
    pop hl
    call _gcClearWindow
    pop hl
    call _gcPrintWindowHeader
    pop hl
    jp _gcPrintWindowText
  __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcSelectWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall
{
    wnd;                // to avoid SDCC warning

  __asm
    push ix

    MAC_LD_IXHL         ; IX - window descriptor

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
    ld a,<#window_attr (ix)
    ld (win_attr),a
    ld (sym_attr),a
    ld (bg_attr),a

    pop ix
    ret
  __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcClearWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall
{
    wnd;                // to avoid SDCC warning

  __asm
    push ix

    call _gcSelectWindow

    MAC_LD_IXHL         ; IX - window descriptor

;;gcDrawWindow {
    ld l,<#frame_type (ix)
    ld h,<#frame_attr (ix)
    set 7,l
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

    ld a,<#id (ix)
    push af
    inc sp

    call _gcDrawWindow

    ld hl,#8
    add hl,sp
    ld sp,hl
;;}

    push ix
    pop hl
    call _gcPrintWindowHeader

    push ix
    pop hl
    call _gcSelectWindow

    pop ix
    ret
  __endasm;
}

void gcPrintWindowHeader(GC_WINDOW_t *wnd) __naked __z88dk_fastcall
{
    wnd;                // to avoid SDCC warning

  __asm
    push ix

    MAC_LD_IXHL         ; IX - window descriptor

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

    pop ix
    ret

header_str:
    .db 0x07, 0x0A, SYM_TRI
    .db 0x08, 0x0A, 0x07, 0x0E, SYM_TRI
    .db 0x08, 0x0E, 0x07, 0x0C, SYM_TRI
    .db 0x08, 0x0C, 0x07, 0x0D, SYM_TRI
    .db 0x00
  __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintWindowText(GC_WINDOW_t *wnd) __naked __z88dk_fastcall
{
    wnd;                // to avoid SDCC warning

  __asm
    push ix

    MAC_LD_IXHL         ; IX - window descriptor

    ld d,<#y (ix)
    inc d
    ld e,<#x (ix)
    inc e
    ld l,<#window_txt (ix)
    ld h,<#window_txt + 1 (ix)
    ld a,l
    or h
    call nz,strprnz
    pop ix
    ret
  __endasm;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall
{
    wnd;                // to avoid SDCC warning

  __asm
    call _gcSelectWindow

    push ix

    MAC_LD_IXHL         ; IX - window descriptor

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

    push ix
    pop hl
    call _gcPrintWindowHeader

    push ix
    pop hl
    call _gcPrintWindowText

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
sym_prns::
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
sym_prn::
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
  __endasm;
}

void putsym(char c) __naked __z88dk_fastcall
{
    c;                  // to avoid SDCC warning

  __asm
    ld a,(pspfx)
    or a
    jr nz,putsym_pfx
    ld a,l
    cp #0x09                ; \t
    jp z,putsym_tab
    cp #0x0A                ; \n
    jr z,putsym_n
    cp #0x0D                ; \r
    jr z,putsym_r
    ld (pspfx),a
    cp #0x20
    ret c
    xor a
    ld (pspfx),a
;;
putsym_sym:
    ld a,(win_x)
    ld e,a
    ld a,(win_w)
    add a,e
    ld e,a
    ld a,(cur_x)
    cp e
    ret nc
    ld e,a
    ld (cur_x),a
    ld a,(cur_y)
    ld d,a
    ld a,(sym_attr)
    ld c,a
    ld a,l
    call sym_prn
    ld a,e
    ld (cur_x),a
    ret
;;
putsym_n:
    ld a,(win_h)
    ld b,a
    ld a,(win_y)
    add a,b
    ld b,a
    ld a,(cur_y)
    inc a
    cp b
    call nc,putsym_scrollup
    ld (cur_y),a
    ld d,a
putsym_r:
    ld a,(win_x)
    ld (cur_x),a
    ld e,a
    ret
;;
putsym_tab:
    ld a,(win_x)
    ld h,a
    ld a,(cur_x)
    sub h
    add a,#8
    and #0xF8
    add a,h
    ld (cur_x),a
    ret
;;
putsym_pfx:
    ex af,af
    xor a
    ld (pspfx),a
    ex af,af
    cp #0x07                ; \a
    jr z,putsym_ink
    cp #0x08                ; \b
    jr z,putsym_paper
    ret
;;
putsym_ink:
    ld a,l
    and #0x0F
    ld l,a
    ld a,(bg_attr)
    and #0xF0
    or l
    ld (sym_attr),a
    ret
;;
putsym_paper:
    ld a,(sym_attr)
    and #0x0F
    ld h,a
    ld a,l
    add a,a
    add a,a
    add a,a
    add a,a
    ld (bg_attr),a
    or h
    ld (sym_attr),a
    ret
;;
putsym_scrollup:
    push hl
    push de
    ld hl,(_current_window_ptr)
    call _gcScrollUpWindow
    pop de
    pop hl
    ld a,(cur_y)
    ret
;;
pspfx:
    .db 0
  __endasm;
}

void gcPrintString(char *str) __naked __z88dk_fastcall
{
    str;                // to avoid SDCC warning

  __asm
    ld a,(cur_x)
    ld e,a
    ld a,(cur_y)
    ld d,a
;    call strprnz
;    ret

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
;;      0x0D - carriage return (\r)
;;      0x0A - line feed (\n) (with CR)
;;      0x0E - center align string
;;      0x0F - right align string
;;      0xFE (BYTE) - string link
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
strprnz::
    ld a,(hl)
    or a
    ret z
    inc hl
    cp #0x07                ; \a
    jp z,strprnz_ink
    cp #0x08                ; \b
    jp z,strprnz_paper
    cp #0x09                ; \t
    jp z,strprnz_tab
    cp #0x0A                ; \n
    jp z,strprnz_n
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

    push hl
    ld l,a
    ld a,(sym_attr)
    ld c,a
    ld a,l
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

strprnz_n:
    ld a,(win_h)
    ld b,a
    ld a,(win_y)
    add a,b
    ld b,a
    ld a,(cur_y)
    inc a
    cp b
    call nc,strprnz_scrollup
    ld (cur_y),a
    ld d,a
strprnz_r:
    ld a,(win_x)
    ld (cur_x),a
    ld e,a
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

strprnz_scrollup:
    push hl
    push de
    ld hl,(_current_window_ptr)
    call _gcScrollUpWindow
    pop de
    pop hl
    ld a,(cur_y)
    ret

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;; i:
;;  HL - string address
;; o:
;;  B - string lenght
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
strlen::
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
4$: cp #0x0A
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
    cp #0x0A
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

void gcProgressBar(u8 x, u8 y, WIN_COLORS_t attr, u8 width, u8 percent) __naked
{
    x, y;           // to avoid SDCC warning
    attr;           // to avoid SDCC warning
    width, percent; // to avoid SDCC warning

  __asm
    ld hl,#2
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a

;; E - x
;; D - y
;; C - attr
;; L - width
;; H - percent

    push hl
    push de
    push bc

    ld e,l
    ld b,h
    ld hl,#0x0000
    ld d,h
    ld a,b
    or a
    jr z,0$
    add hl,de
    djnz .-1

;; HL/=100
    ld bc,#0x1064
    xor a
    add hl,hl
    rla
    cp c
    jr c,.+2+1+1
    inc l
    sub c
    djnz .-1-1-2-1-1-1

0$: pop bc
;; width
    ld b,l
    pop de
    pop hl

    ld a,b
    or a
    jr z,1$

    push bc
    push hl
    ld a,#0xDB
    call sym_prns
    pop hl
    pop bc

1$: ld a,l
    sub b
    jr z,2$
    ld b,a
    ld a,#0x20
    call sym_prns
2$:
    ret
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
void gcSetPalette(u16 *palette, u8 palsel) __naked
{
    palette, palsel;    // to avoid SDCC warning

  __asm
    ld hl,#2
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld a,(hl)
    and #0x0F
    push af
    add a,a
    add a,a
    add a,a
    add a,a
    ld l,a
    ld h,#0
    add hl,hl
;
    ld a,#0x0C          ;W0RAM | W0MAP_N
    ld bc,#0x21AF       ;MEMCONFIG
    out (c),a
;
    ld a,#0x10
    ld b,#0x15          ;FMADDR
    out (c),a
;
    ex de,hl
    ld a,l
    or h
    jr nz,.+2+3
    ld hl,#default_palette
    ld bc,#32
    ldir
;
    xor a
    ld bc,#0x15AF       ;FMADDR
    out (c),a
;
    pop af
    ld b,#0x07          ;PALSEL
    out (c),a
    ld b,#0x21          ;MEMCONFIG
    ld a,#0x0E          ;W0RAM | W0MAP_N | W0WE
    out (c),a
    ret

default_palette:
    .dw 0x0000,0x0010,0x4000,0x4010,0x0200,0x0210,0x4200,0x4210
	.dw 0x2128,0x0018,0x6000,0x6018,0x0300,0x0318,0x6300,0x6318
  __endasm;
}
