//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                       ::
//::               by dr_max^gc (c)2018-2019                 ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

#pragma once

#include "ps2.h"
#include "mouse.h"
#include "numbers.h"

#define SCREEN_WIDTH    80
#define SCREEN_HIGHT    30

#define EIHALT __asm__("ei\n halt\n");
#define DIHALT __asm__("di\n halt\n");

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
// STRING MARKUP DEFINITIONS
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#define MARK_INVERT             "\f"
#define MARK_TAB                "\t"
#define MARK_CENTER             "\x0E"
#define MARK_RIGHT              "\x0F"
#define MARK_LINK               "\xFE"

#define INK_BLACK               "\a\x80"
#define INK_BLUE                "\a\x1"
#define INK_RED                 "\a\x2"
#define INK_MAGENTA             "\a\x3"
#define INK_GREEN               "\a\x4"
#define INK_CYAN                "\a\x5"
#define INK_YELLOW              "\a\x6"
#define INK_WHITE               "\a\x7"

#define INK_BRIGHT_BLUE         "\a\x9"
#define INK_BRIGHT_RED          "\a\xA"
#define INK_BRIGHT_MAGENTA      "\a\xB"
#define INK_BRIGHT_GREEN        "\a\xC"
#define INK_BRIGHT_CYAN         "\a\xD"
#define INK_BRIGHT_YELLOW       "\a\xE"
#define INK_BRIGHT_WHITE        "\a\xF"

#define PAPER_BLACK             "\b\x80"
#define PAPER_BLUE              "\b\x1"
#define PAPER_RED               "\b\x2"
#define PAPER_MAGENTA           "\b\x3"
#define PAPER_GREEN             "\b\x4"
#define PAPER_CYAN              "\b\x5"
#define PAPER_YELLOW            "\b\x6"
#define PAPER_WHITE             "\b\x7"

#define PAPER_BRIGHT_BLUE       "\b\x9"
#define PAPER_BRIGHT_RED        "\b\xA"
#define PAPER_BRIGHT_MAGENTA    "\b\xB"
#define PAPER_BRIGHT_GREEN      "\b\xC"
#define PAPER_BRIGHT_CYAN       "\b\xD"
#define PAPER_BRIGHT_YELLOW     "\b\xE"
#define PAPER_BRIGHT_WHITE      "\b\xF"

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: WINDOW COLORS
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef enum WIN_COLORS_t
{
    WIN_COL_BLACK           = ((u8)0x00),
    WIN_COL_BLUE            = ((u8)0x01),
    WIN_COL_RED             = ((u8)0x02),
    WIN_COL_MAGENTA         = ((u8)0x03),
    WIN_COL_GREEN           = ((u8)0x04),
    WIN_COL_CYAN            = ((u8)0x05),
    WIN_COL_YELLOW          = ((u8)0x06),
    WIN_COL_WHITE           = ((u8)0x07),
    WIN_COL_BRIGHT_BLUE     = ((u8)0x09),
    WIN_COL_BRIGHT_RED      = ((u8)0x0A),
    WIN_COL_BRIGHT_MAGENTA  = ((u8)0x0B),
    WIN_COL_BRIGHT_GREEN    = ((u8)0x0C),
    WIN_COL_BRIGHT_CYAN     = ((u8)0x0D),
    WIN_COL_BRIGHT_YELLOW   = ((u8)0x0E),
    WIN_COL_BRIGHT_WHITE    = ((u8)0x0F)
} WIN_COLORS_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: MESSAGEBOX TYPES
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef enum MB_TYPE_t
{
    MB_OK               = ((u8)0),
    MB_OKCANCEL         = ((u8)1),
    MB_YESNO            = ((u8)2),
    MB_YESNOCANCEL      = ((u8)3),
    MB_RETRYABORTIGNORE = ((u8)4)
} MB_TYPE_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: DIALOG BUTTONS
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef enum BTN_TYPE_t
{
    BUTTON_OK           = ((u8)0xFF),
    BUTTON_CANCEL       = ((u8)0xFE),
    BUTTON_YES          = ((u8)0xFD),
    BUTTON_NO           = ((u8)0xFC),
    BUTTON_RETRY        = ((u8)0xFB),
    BUTTON_ABORT        = ((u8)0xFA),
    BUTTON_IGNORE       = ((u8)0xF9)
} BTN_TYPE_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: WINDOW TYPES
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef enum GC_WND_TYPE_t
{
    GC_WND_NOMENU   = ((u8)0x00),
    GC_WND_SVMENU   = ((u8)0x01),
    GC_WND_DIALOG   = ((u8)0x02),
    GC_WND_INFO     = ((u8)0x03),
} GC_WND_TYPE_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: FRAME TYPES
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef enum GC_FRM_TYPE_t
{
    GC_FRM_NONE     = ((u8)0x00),
    GC_FRM_SINGLE   = ((u8)0x01),
    GC_FRM_DOUBLE   = ((u8)0x02),     //cp866 symbols yet
    GC_FRM_NOGREETZ = ((u8)0x10),
    GC_FRM_NOLOGO   = ((u8)0x20),
    GC_FRM_NOHEADER = ((u8)0x40),
    GC_FRM_NOSHADOW = ((u8)0x80)
} GC_FRM_TYPE_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: WINDOW DESCRIPTOR
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef struct GC_WINDOW_t
{
    u8              id;
    GC_WND_TYPE_t   type;
    u8              x;
    u8              y;
    u8              width;
    u8              hight;
    WIN_COLORS_t    attr;
    GC_FRM_TYPE_t   frame_type;
    WIN_COLORS_t    frame_attr;
    char            *header_txt;
    char            *window_txt;
    void            *menu_ptr;
    void            *int_proc;
} GC_WINDOW_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: SIMPLE VERTICAL MENU RETURN CODE
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
enum
{
    SVM_RC_KEY      = (u8)0xFD,
    SVM_RC_TAB      = (u8)0xFE,
    SVM_RC_EXIT     = (u8)0xFF
};

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: SIMPLE VERTICAL MENU KEYS CALLBACK RETURN CODE
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef enum SVM_CBKEY_RC_t
{
    SVM_CBKEY_RC_NONE   = (u8)0x00,
    SVM_CBKEY_RC_REDRAW = (u8)0xFE,
    SVM_CBKEY_RC_EXIT   = (u8)0xFF
} SVM_CB_KEY_RC_t;

typedef enum GC_SVM_TAG_t
{
    GC_SVMT_TEXT    = (u8)0x00,
    GC_SVMT_OPTION  = (u8)0x01,
    GC_SVMT_CALLBACK= (u8)0x02
} GC_SVM_TAG_t;

typedef struct GC_SVM_OPTION_t
{
    u8      *option;
    char    *text;
} GC_SVM_OPTION_t;

typedef struct GC_SVM_LINE_t
{
    GC_SVM_TAG_t    tag;
    void            *ptr;
} GC_SVM_LINE_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: SIMPLE VERTICAL MENU FLAGS
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef struct GC_SVM_FLAG_t
{
    unsigned    SVMF_SCROLLBAR  :1;
    unsigned    bit1            :1;
    unsigned    bit2            :1;
    unsigned    bit3            :1;
    unsigned    bit4            :1;
    unsigned    bit5            :1;
    unsigned    SVMF_NOWRAP     :1;
    unsigned    SVMF_EXIT       :1;
} GC_SVM_FLAG_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: SIMPLE VERTICAL MENU
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef struct GC_SVMENU_t
{
    GC_SVM_FLAG_t   flags;      // флаги
    WIN_COLORS_t    attr;       // атрибут цвета
    u8              margin;     // отступ от верха
    u8              cur_pos;    // текущая позиция (в окне)
    u8              win_pos;    // позиция(смещение) окна
    u8              win_num;    // кол-во пунктов в окне
    u8              all_num;    // общее кол-во пунктов
    void            *cb_cursor; // *svmcb_cursor_t
    void            *cb_keys;   // *svmcb_keys_t
    void            *cb_cross;  // top&bottom border crossing callbacks
    GC_SVM_LINE_t   **lines;    //
    char            **txt_list; //
} GC_SVMENU_t;
// cursor moving callback function
typedef void (*svmcb_cursor_t)(GC_SVMENU_t(*svm));
// key pressed callback function
typedef u8 (*svmcb_keys_t)(GC_SVMENU_t(*svm), u8 key);
// top&bottom across the border callbacks
//typedef void (*svm_cb_top_bot)(GC_SVMENU_t(*svm), u8 cnt);

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: HORIZONTAL MENU
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
/*
typedef struct
{
    u8  attr;
    u8  indent;
    u8  current;
    u8  count;
} GC_HMENU;
*/
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
/*
typedef struct
{
    u8  x;
    u8  y;
    u8  current;
    u8  count;
    u16 *items;
} GC_VMENU;
*/

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: DIALOG ITEMS
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef enum GC_DITEM_TYPE_t
{
    DI_TEXT         = ((u8)0),
    DI_HDIV         = ((u8)1),
    DI_GROUPBOX     = ((u8)2),
    DI_EDIT         = ((u8)4),
    DI_BUTTON       = ((u8)7),
    DI_CHECKBOX     = ((u8)8),
    DI_RADIOBUTTON  = ((u8)9),
    DI_LISTBOX      = ((u8)10),
    DI_LISTVIEW     = ((u8)11),
    DI_NUMBER       = ((u8)12),
    DI_PROGRESSBAR  = ((u8)13)
} GC_DITEM_TYPE_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
// ITEM VAR SIZES
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef enum GCIV_SIZE_t
{
    GCIVS_BYTE    = 0,
    GCIVS_WORD    = 1,
    GCIVS_DWORD   = 2,
    GCIVS_QWORD   = 3     // not yet
}  GCIV_SIZE_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
// ITEM VAR TYPES
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef enum GCIV_TYPE_t
{
    GCIVT_DEC     = 0,
    GCIVT_HEX     = 1,
    GCIVT_BIN     = 2     // not yet
} GCIV_TYPE_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: DIALOG ITEM VARIABLE TYPE STRUCTURE
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef struct GC_DITEM_VAR_t
{
    GCIV_SIZE_t     DIV_SIZE    :2;
    GCIV_TYPE_t     DIV_TYPE    :2;
    unsigned        bit4        :1;
    unsigned        bit5        :1;
    unsigned        bit6        :1;
    unsigned        DIV_TEXT    :1; // not yet
} GC_DITEM_VAR_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: DIALOG ITEM FLAGS
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef struct GC_DITEM_FLAG_t
{
    unsigned    DIF_GREY    :1;
    unsigned    bit1        :1;
    unsigned    bit2        :1;
    unsigned    bit3        :1;
    unsigned    bit4        :1;
    unsigned    bit5        :1;
    unsigned    DIF_RIGHT   :1;     // not yet
    unsigned    DIF_TABSTOP :1;
} GC_DITEM_FLAG_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

/*
typedef struct
{
    u8  flags;
    u8  row_count;
    u8  column_count;
    u8  *column_types;
    u8  *column_widths;
    u16 **column_titles;
    u16 **column_list;
} GC_LISTVIEW_t;
*/

// GC_COLUMN_t

/*
row_count - количество рядов
column_count - количество колонок
column_types - указатель на массив типов колонок
column_widths - указатель на массив размеров колонок
column_titles - указатель на массив указателей названий колонок
column_list - указатель на массив указателей на данные колонок

типы колонок:
1 - текст
2 - число
3 - filesize
*/

/*
 * DI_LISTVIEW
 * width - ширина
 * hight - высота
 * var - указатель на переменную (u8) на текущуюю позицию
 * name - указатель на описатель GC_LISTVIEW
*/

/* ЭЛЕМЕНТЫ ДИАЛОГА*/

/*
 * DI_NUMBER
 * width - ширина поля для числа (происходит выравнивание по правому краю)
 *         (!WARNING!проверка ширины не производится)
 * var - указатель на переменную,
 *       тип которой задан в GC_DITEM_VAR_t vartype.DIV_SIZE
 * name - название элемента
 */

/*
 * DI_CHECKBOX
 * width - рассчитывается автоматически
 * var - указатель на переменную (u8) 0x00 - OFF  0xFF - ON
 * name - название элемента
 */

/*
 * DI_RADIOBUTTON
 * width - рассчитывается автоматически
 * select - значение для занесения в переменную при выборе
 * var - указатель на переменную (u8)
 * name - название элемента
 */

/*
 * DI_EDIT
 * width - ширина поля ввода
 * name - указатель на строку
 */

/*
 * DI_LISTBOX
 * width - ширина поля выбора подпунктов
 * select - количество подпунктов
 * var - указатель на переменную (u8) на текущуюю позицию
 * name - указатель на массив строк
 */

/*
 * DI_BUTTON
 * id - код элемента, возвращаемый диалогом
 *      предопределенные коды
 *          BUTTON_OK
 *          BUTTON_CANCEL
 *          BUTTON_YES
 *          BUTTON_NO
 *          BUTTON_RETRY
 *          BUTTON_ABORT
 *          BUTTON_IGNORE
 * width - ширина элемента
 * select - должен быть 0x00 (используется для анимации нажатия)
 * name - название элемента
 */

/* DI_PROGRESSBAR
 * width - ширина
 * var - указатель на переменную (u8)
 * select - атрибут цвета
 */

/*
Описатель элемента диалога
    id - идентификатор элемента;
            используется для возвращения
            нажатой кнопки в диалоге;
    x,y -  координаты элемента;
    width - ширина элемента;
            для элемента checkbox ширина просчитывается автоматически;
    hight - высота элемента;
            используется для элемента DI_GROUPBOX;
            в других элементах значение игнорируется;
    flags - флаги элемента;
            DIF_GREY - неактивный элемент;
            DIF_TABSTOP - стоп-флаг для навигации по клавише TAB (EXT)
                            и PgUp PgDn;
                  (!WARNING! обязательно наличие одного флага)
            DIF_RIGHT - выравнивание вправо по width;
    vartype - тип переменной;
    hotkey - горячая клавиша;
    select - используется в зависимости от типа элемента;
    var - указатель на переменную;
    name - название элемента;
    exec - указатель на функцию, вызываемую при выборе элемента;
            может использоваться для изменения активности/неактивности
            элемента выставлением бита DIF_GREY;
*/

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: DIALOG ITEM STRUCTURE
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef struct GC_DITEM_t
{
    GC_DITEM_TYPE_t type;
    u8              id;
    u8              x;
    u8              y;
    u8              width;
    u8              hight;
    GC_DITEM_FLAG_t flags;
    GC_DITEM_VAR_t  vartype;
    KEY_t           hotkey;
    u8              select;
    u8              *var;
    u8  const       *name;
    func_t          exec;
} GC_DITEM_t;

/*
Описатель диалога
    current - текущий пункт
    all_count - общее количество пунктов
    act_count - количество активных пунктов
    curr_attr - цвет курсора
    box_attr - цвет элемента DI_GROUPBOX
    btn_focus_attr - цвет элемента DI_BUTTON в фокусе
    btn_unfocus_attr - цвет элемента DI_BUTTON не в фокусе
    lbox_focus_attr (*) - цвет элемента DI_LISTBOX в фокусе
    lbox_unfocus_attr(*) - цвет элемента DI_LISTBOX не в фокусе
    **items - указатель на массив указателей на элементы диалога
        (ВАЖНО! активные элементы перечисляются первыми, в конце списка
         пассивные элементы, такие как DI_TEXT, DI_GROUPBOX, DI_HDIV)

    * - также используется для DI_EDIT
*/

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: DIALOG FLAGS
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef struct GC_DLG_FLAG_t
{
    unsigned    bit0        :1;
    unsigned    bit1        :1;
    unsigned    bit2        :1;
    unsigned    bit3        :1;
    unsigned    bit4        :1;
    unsigned    bit5        :1;
    unsigned    bit6        :1;
    unsigned    DLGF_CURSOR :1;
} GC_DLG_FLAG_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: DIALOG STRUCTURE
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef struct GC_DIALOG_t
{
    GC_DLG_FLAG_t flag;             // dialog flags
    u8  current;                    // current item
    u8  all_count;                  // count of all items
    u8  act_count;                  // count of acive items
    WIN_COLORS_t cur_attr;          // cursor attribute
    WIN_COLORS_t box_attr;          // DI_GROUPBOX attribute
    WIN_COLORS_t btn_focus_attr;    // DI_BUTTON focus attribute
    WIN_COLORS_t btn_unfocus_attr;  // DI_BUTTON unfocus attribute
    WIN_COLORS_t lbox_focus_attr;   // DI_LISTBOX (and other) focus attribute
    WIN_COLORS_t lbox_unfocus_attr; // DI_LISTBOX (and other) unfocus attribute
    GC_DITEM_t **items;             // pointer to array of items pointers
} GC_DIALOG_t;
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

/*
все строки (и названия элементов) поддерживают разметку, которая
назначена в #defines
цвет чернил можно задать через:
    INK_[COLOR]
цвет бумаги:
    PAPER_[COLOR]
    при переводе строки разметка сбрасывается на атрибуты окна
инвертирование:
    MARK_INVERT
выравнивание строки:
    MARK_CENTER
    MARK_RIGHT
табуляция на N-символов:
    MARK_TAB "\xN"
подлинковка строки:
    MARK_LINK "\xN" где N номер строки из массива, который необходимо
    инициализировать через gcSetLinkedMessage(u16 **ptr)
*/

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

/*
Описатель узла списка окон
    id - номер окна (0 - узел свободен)
    flags - флаги окна
    GC_WINDOW_t *window - указатель на стуктуру окна
*/

typedef struct GC_WIN_NODE_t
{
    u8  id;
    u8  flags;
    GC_WINDOW_t *window;
} GC_WIN_NODE_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

void gcWindowsInit(u8 vpage, u8 spage) __naked;

void gcSetLinkedMessage(u16 **ptr) __naked __z88dk_fastcall;

BTN_TYPE_t gcMessageBox(MB_TYPE_t type, GC_FRM_TYPE_t frame, char *header, char *message);

BTN_TYPE_t gcExecuteWindow(GC_WINDOW_t *wnd);

void gcCloseWindow(void) __naked;
void gcPrintChainWindows(void) __naked;

void gcDrawWindow(u8 id, u8 x, u8 y, u8 width, u8 hight, WIN_COLORS_t attr, GC_FRM_TYPE_t frame_type, WIN_COLORS_t frame_attr) __naked;
u8 gcGetMessageLines(u8 *msg) __naked __z88dk_fastcall;
u8 gcGetMessageMaxLength(u8 *msg) __naked __z88dk_fastcall;
void gcUpdateWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall;
void gcPrintWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall;
void gcPrintWindowHeader(GC_WINDOW_t *wnd) __naked __z88dk_fastcall;
void gcPrintWindowText(GC_WINDOW_t *wnd) __naked __z88dk_fastcall;
void gcSelectWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall;
void gcClearWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall;

void gcScrollUpWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall;
void gcScrollDownWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall;
void gcScrollUpRect(u8 x, u8 y, u8 width, u8 hight) __naked;
void gcScrollDownRect(u8 x, u8 y, u8 width, u8 hight) __naked;

void gcProgressBar(u8 x, u8 y, WIN_COLORS_t attr, u8 width, u8 percent) __naked;

/* DIALOG */
u8 gcDialog(GC_DIALOG_t *dlg) __naked __z88dk_fastcall;
void gcPrintDialog(GC_DIALOG_t *dlg) __naked __z88dk_fastcall;
void gcPrintActiveDialog(GC_DIALOG_t *dlg) __naked __z88dk_fastcall;
void gcPrintDialogShownItems(GC_DIALOG_t *dlg, GC_DITEM_TYPE_t type) __naked;
void gcPrintDialogItem(GC_DITEM_t *ditm) __naked __z88dk_fastcall;
void gcPrintDialogCursor(GC_DITEM_t *ditm) __naked __z88dk_fastcall;
void gcRestoreDialogCursor(GC_DITEM_t *ditm) __naked __z88dk_fastcall;
u8 gcFindNextTabItem(GC_DIALOG_t *dlg) __naked __z88dk_fastcall;
u8 gcFindPrevTabItem(GC_DIALOG_t *dlg) __naked __z88dk_fastcall;
u8 gcFindNextItem(GC_DIALOG_t *dlg) __naked __z88dk_fastcall;
u8 gcFindPrevItem(GC_DIALOG_t *dlg) __naked __z88dk_fastcall;
u8 gcFindHotkey(GC_DIALOG_t *dlg) __naked __z88dk_fastcall;

u8 gcFindClickItem(GC_DIALOG_t *dlg) __naked __z88dk_fastcall;

/* Simple Vertical Menu */
void gcPrintSimpleVMenu(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall;
void gcInitSimpleVMenu(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall;
u8 gcSimpleVMenu(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall;
void gcPrintSVMCursor(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall;
void gcRestoreSVMCursor(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall;

void gcGotoXY(u8 x, u8 y) __naked;
void gcPrintSymbol(u8 x, u8 y, u8 sym, u8 attr) __naked;
void gcPrintString(char *str) __naked __z88dk_fastcall;
void gcEditString(char *str, u8 len, u8 x, u8 y) __naked;

void gcSetFontSym(u8 sym, u8 *udg) __naked;
void gcSetPalette(u16 *palette, u8 palsel) __naked;

void putsym(char c) __naked __z88dk_fastcall;
