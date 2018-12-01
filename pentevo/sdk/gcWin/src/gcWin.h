//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                       ::
//::                  by dr_max^gc (c)2018                   ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

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

#define INK_BLACK               "\a\x0"
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

#define PAPER_BLACK             "\b\x0"
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
typedef enum
{
    WIN_COL_BLACK           = ((u8)0x00),
    WIN_COL_BLUE            = ((u8)0x01),
    WIN_COL_RED		        = ((u8)0x02),
    WIN_COL_MAGENTA         = ((u8)0x03),
    WIN_COL_GREEN	        = ((u8)0x04),
    WIN_COL_CYAN	        = ((u8)0x05),
    WIN_COL_YELLOW	        = ((u8)0x06),
    WIN_COL_WHITE	        = ((u8)0x07),
    WIN_COL_BRIGHT_BLUE	    = ((u8)0x09),
    WIN_COL_BRIGHT_RED      = ((u8)0x0A),
    WIN_COL_BRIGHT_MAGENTA	= ((u8)0x0B),
    WIN_COL_BRIGHT_GREEN    = ((u8)0x0C),
    WIN_COL_BRIGHT_CYAN     = ((u8)0x0D),
    WIN_COL_BRIGHT_YELLOW   = ((u8)0x0E),
    WIN_COL_BRIGHT_WHITE    = ((u8)0x0F)
} WIN_COLORS_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: MESSAGEBOX TYPES
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef enum
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
typedef enum
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
typedef enum
{
    GC_WND_NOMENU = ((u8)0x00),
    GC_WND_SVMENU = ((u8)0x01),
    GC_WND_DIALOG = ((u8)0x02)
} GC_WND_TYPE_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: FRAME TYPES
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef enum
{
    GC_FRM_NONE   = ((u8)0x00),
    GC_FRM_SINGLE = ((u8)0x01),
    GC_FRM_DOUBLE = ((u8)0x02),     //cp866 symbols yet
    GC_FRM_NOLOGO = ((u8)0x20),
    GC_FRM_NOHEADER = ((u8)0x40),
    GC_FRM_NOSHADOW = ((u8)0x80)
} GC_FRM_TYPE_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: WINDOW DESCRIPTOR
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef struct
{
    GC_WND_TYPE_t   type;       // +0
    u8  x;                      // +1
    u8  y;                      // +2
    u8  width;                  // +3
    u8  hight;                  // +4
    WIN_COLORS_t  attr;         // +5
    GC_FRM_TYPE_t frame_type;   // +6
    WIN_COLORS_t frame_attr;    // +7
    u8  *header_txt;            // +8
    u8  *window_txt;            // +10
    u16 *menu_ptr;              // +12
} GC_WINDOW_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: SIMPLE VERTICAL MENU
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef struct
{
    u8  attr;               // атрибут цвета
    u8  margin;             // отступ от верха
    u8  current;            // текущая позиция
    u8  count;              // количество пунктов
} GC_SVMENU_t;

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
typedef enum
{
    DI_TEXT         = ((u8)0),
    DI_HDIV         = ((u8)1),
    DI_SINGLEBOX    = ((u8)2),
    DI_EDIT         = ((u8)4),
    DI_BUTTON       = ((u8)7),
    DI_CHECKBOX     = ((u8)8),
    DI_RADIOBUTTON  = ((u8)9),
    DI_LISTBOX      = ((u8)10),
    DI_LISTVIEW     = ((u8)11),
    DI_NUMBER       = ((u8)12),
    DI_INPUT_NUMBER = ((u8)13)
} GC_DITEM_TYPE_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
// ITEM VAR SIZES
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef enum
{
    BYTE    = 0,
    WORD    = 1,
    DWORD   = 2,
    QWORD   = 3     // not yet
}  GCIV_SIZE_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
// ITEM VAR TYPES
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef enum
{
    DEC     = 0,
    HEX     = 1,
    BIN     = 2     // not yet
} GCIV_TYPE_t;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: DIALOG ITEM VARIABLE TYPE STRUCTURE
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef struct
{
    GCIV_SIZE_t     DIV_SIZE    :2;
    GCIV_TYPE_t     DIV_TYPE    :2;
    unsigned        bit4        :1;
    unsigned        bit5        :1;
    unsigned        bit6        :1;
    unsigned        DIV_TEXT    :1; // not yet
} GC_DITEM_VAR_t;

/*
описатель флагов
    DIF_GRAY - неактивный элемент
    DIF_TABSTOP - стоп-флаг для навигации по клавише TAB (EXT)
                  (!WARNING! обязательно наличие одного флага)
    DIF_RIGHT - выравнивание вправо по width
*/

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: DIALOG ITEM FLAGS
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef struct
{
    unsigned    DIF_GRAY    :1;     // not yet
    unsigned    bit1        :1;
    unsigned    bit2        :1;
    unsigned    bit3        :1;
    unsigned    bit4        :1;
    unsigned    bit5        :1;
    unsigned    DIF_RIGHT   :1;     // not yet
    unsigned    DIF_TABSTOP :1;
} GC_DITEM_FLAG_t;

typedef struct
{
    u8  flags;
    u8  row_count;
    u8  column_count;
    u8  *column_types;
    u8  *column_widths;
    u16 **column_titles;
    u16 **column_list;
} GC_LISTVIEW;

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
 * var -
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

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: DIALOG ITEM STRUCTURE
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef struct
{
    GC_DITEM_TYPE_t type;
    u8              id;
    u8              x;
    u8              y;
    u8              width;
    u8              hight;
    GC_DITEM_FLAG_t flags;
    GC_DITEM_VAR_t  vartype;
    u8              hotkey;
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
    box_attr - цвет элемента SINGLEBOX
    btn_focus_attr - цвет элемента BUTTON в фокусе
    btn_unfocus_attr - цвет элемента BUTTON не в фокусе
    lbox_focus_attr (*) - цвет элемента LISTBOX в фокусе
    lbox_unfocus_attr(*) - цвет элемента LISTBOX не в фокусе
    **items - указатель на массив указателей на элементы диалога
        (ВАЖНО! активные элементы перечисляются первыми, в конце списка
         пассивные элементы, такие как DI_TEXT, DI_SINGLEBOX, DI_HDIV)

    * - также используется для DI_EDIT
*/

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:: DIALOG STRUCTURE
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
typedef struct
{
    u8  current;            // current item
    u8  all_count;          // count of all items
    u8  act_count;          // count of acive items
    u8  cur_attr;           // cursor attribute
    u8  box_attr;           // DI_SINGLEBOX attribute
    u8  btn_focus_attr;     // DI_BUTTON focus attribute
    u8  btn_unfocus_attr;   // DI_BUTTON unfocus attribute
    u8  lbox_focus_attr;    // DI_LISTBOX (and other) focus attribute
    u8  lbox_unfocus_attr;  // DI_LISTBOX (and other) unfocus attribute
    GC_DITEM_t **items;     // pointer to array of items pointers
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

void gcSetLinkedMessage(u16 **ptr) __naked __z88dk_fastcall;

BTN_TYPE_t gcMessageBox(MB_TYPE_t type, GC_FRM_TYPE_t frame, char *header, char *message);

BTN_TYPE_t gcWindowHandler(GC_WINDOW_t *wnd);

void gcDrawWindow(u8 x, u8 y, u8 width, u8 hight, u8 attr, u8 frame_type, u8 frame_attr) __naked;
u8 gcGetMessageLines(u8 *msg) __naked __z88dk_fastcall;
u8 gcGetMessageMaxLength(u8 *msg) __naked __z88dk_fastcall;
void gcPrintMessage(u8 *msg) __naked __z88dk_fastcall;
void gcPrintWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall;
void gcScrollUpWindow(u8 x, u8 y, u8 width, u8 hight);
void gcScrollDownWindow(u8 x, u8 y, u8 width, u8 hight);

/* DIALOG */
u8 gcDialog(GC_DIALOG_t *dlg) __naked __z88dk_fastcall;
void gcPrintDialog(GC_DIALOG_t *dlg) __naked __z88dk_fastcall;
void gcPrintActiveDialog(GC_DIALOG_t *dlg) __naked __z88dk_fastcall;
void gcPrintDialogItem(GC_DITEM_t *ditm) __naked __z88dk_fastcall;
void gcPrintDialogCursor(GC_DITEM_t *ditm) __naked __z88dk_fastcall;
void gcRestoreDialogCursor(GC_DITEM_t *ditm) __naked __z88dk_fastcall;
u8 gcFindNextTabItem(GC_DIALOG_t *dlg) __naked __z88dk_fastcall;
u8 gcFindPrevTabItem(GC_DIALOG_t *dlg) __naked __z88dk_fastcall;

/* Simple Vertical Menu */
u8 gcSimpleVMenu(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall;
void gcPrintSVMCursor(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall;
void gcRestoreSVMCursor(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall;

void gcGotoXY(u8 x, u8 y) __naked;
void gcPrintSymbol(u8 x, u8 y, u8 sym, u8 attr) __naked;

/* Print numbers*/
void gcPrintHex8(u8 num) __naked __z88dk_fastcall;
void gcPrintHex16(u16 num) __naked __z88dk_fastcall;
void gcPrintHex32(u32 num) __naked __z88dk_fastcall;

void gcPrintDec8(u8 num) __naked __z88dk_fastcall;
void gcPrintDec16(u16 num) __naked __z88dk_fastcall;
void gcPrintDec32(u32 num) __naked __z88dk_fastcall;

void gcSetFontSym(u8 sym, u8 *udg) __naked;
void gcSetPalette();
