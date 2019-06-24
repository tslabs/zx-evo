# Оконная библиотека gcWin

[TOC]

## Функции

### Инициализация

```
void gcWindowsInit(u8 vpage, u8 spage) __naked;
```

- **vpage** - видеостраница
- **spage** - страница теневого экрана



### Вывод окна

```
void gcPrintWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall;
```

- ***wnd** - указатель на [описатель окна](#GC_WINDOW_t)

  

```
void gcDrawWindow(u8 id, u8 x, u8 y, u8 width, u8 hight, WIN_COLORS_t attr, GC_FRM_TYPE_t frame_type, WIN_COLORS_t frame_attr) __naked;
```

(внутренняя функция)

- **x** - X координата окна
- **y** - Y координата окна
- **width** - ширина окна
- **hight** - высота окна
- **attr** - [атрибут цвета](#WIN_COLORS_t) окна
- **frame_type** - [тип рамки](#GC_FRM_TYPE_t) окна
- **frame_attr** - [атрибут цвета](#WIN_COLORS_t) рамки окна



### Обработка окна

(с вызовом меню)

```
BTN_TYPE_t gcExecuteWindow(GC_WINDOW_t *wnd);
```

- ***wnd** - указатель на [описатель окна](#GC_WINDOW_t)



### Закрытие окна

```
void gcCloseWindow(void) __naked;
```



### Выбор окна

```
void gcSelectWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall;
```

- ***wnd** - указатель на [описатель окна](#GC_WINDOW_t)



### Скроллинг окна вверх

```
void gcScrollUpWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall;
```

- ***wnd** - указатель на [описатель окна](#GC_WINDOW_t)



### Скроллинг окна вниз

```
void gcScrollDownWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall;
```

- ***wnd** - указатель на [описатель окна](#GC_WINDOW_t)



### Строки и символы

------



#### Печать символа по координатам

```
void gcPrintSymbol(u8 x, u8 y, u8 sym, u8 attr) __naked;
```

- **x** - X координата
- **y** - Y координата
- **sym** - код символа
- **attr** - атрибут цвета



#### Позиционирование каретки

```
void gcGotoXY(u8 x, u8 y) __naked;
```

- **x** - X координата
- **y** - Y координата



#### Печать символа

```
void putsym(char c) __naked __z88dk_fastcall;
```

- **с** - код символа



#### Печать строки

```
void gcPrintString(char *str) __naked __z88dk_fastcall;
```

- ***str** - указатель на строку текста

  

#### Печать строки форматированная

```
void gcPrintf(char *string, ...) __naked;
```

​	Спецификатор формата:

​	%\[флаги\]\[ширина\]тип

флаги:

- **0** - дополнять нулями

типы:

- **hu** - десятичное беззнаковое число 8бит
- **hd** - десятичное знаковое число 8бит
- **u** - десятичное беззнаковое число 16бит
- **d** - десятичное знаковое число 16бит
- **lu** - десятичное беззнаковое число 32бит
- **ld** - десятичное знаковое число 32бит
- **hx** - шестнадцатеричное беззнаковое число 8бит
- **x** - шестнадцатеричное беззнаковое число 16бит
- **s** - строка
- **с** - символ



### Простое вертикальное меню

------



#### Вызов меню

```
u8 gcSimpleVMenu(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall;
```

- ***svmnu** - указатель на описатель меню



#### Печать курсора

```
void gcPrintSVMCursor(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall;
```

- ***svmnu** - указатель на описатель меню



#### Восстановление курсора

```
void gcRestoreSVMCursor(GC_SVMENU_t *svmnu) __naked __z88dk_fastcall;
```

- ***svmnu** - указатель на описатель меню





### Диалоги

------



#### Вызов диалога

```
u8 gcDialog(GC_DIALOG_t *dlg) __naked __z88dk_fastcall;
```

- ***dlg** - указатель на описатель диалога



#### Печать диалога

```
void gcPrintDialog(GC_DIALOG_t *dlg) __naked __z88dk_fastcall;
```

- ***dlg** - указатель на описатель диалога



#### Печать активных элементов диалога

```
void gcPrintActiveDialog(GC_DIALOG_t *dlg) __naked __z88dk_fastcall;
```

- ***dlg** - указатель на описатель диалога



#### Печать элементов диалога указанного типа

```
void gcPrintDialogShownItems(GC_DIALOG_t *dlg, GC_DITEM_TYPE_t type) __naked;
```

- ***dlg** - указатель на описатель диалога
- **type** - тип элементов диалога



#### Печать элемента диалога

```
void gcPrintDialogItem(GC_DITEM_t *ditm) __naked __z88dk_fastcall;
```

- ***ditm** - указатель на элемент диалога



## Описатели

### <a name="GC_WINDOW_t">Описатель окна</a>

```
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
```

- **x** - X координата окна
- **y** - Y координата окна
- **width** - ширина окна
- **hight** - высота окна
- **attr** - [атрибут цвета](#WIN_COLORS_t) окна
- **frame_type** - [тип рамки](#GC_FRM_TYPE_t) окна
- **frame_attr** - [атрибут цвета](#WIN_COLORS_t) рамки окна
- ***header_txt** - указатель на текст заголовка окна
- ***window_txt** - указатель на текст окна
- ***menu_ptr** - указатель на описатель меню
- ***int_proc** - не реализовано



#### <a name="GC_WND_TYPE_t">Типы окон</a>

```
typedef enum GC_WND_TYPE_t
{
    GC_WND_NOMENU   = ((u8)0x00),
    GC_WND_SVMENU   = ((u8)0x01),
    GC_WND_DIALOG   = ((u8)0x02),
    GC_WND_INFO     = ((u8)0x03),
} GC_WND_TYPE_t;
```

- **GC_WND_NOMENU** - окно без меню
- **GC_WND_SVMENU** - простое вертикальное меню
- **GC_WND_DIALOG** - диалоговое окно
- **GC_WND_INFO** - информационное окно



#### <a name="GC_FRM_TYPE_t">Типы рамок</a>

```
typedef enum GC_FRM_TYPE_t
{
    GC_FRM_NONE     = ((u8)0x00),
    GC_FRM_SINGLE   = ((u8)0x01),
    GC_FRM_DOUBLE   = ((u8)0x02),
    GC_FRM_NOLOGO   = ((u8)0x20),
    GC_FRM_NOHEADER = ((u8)0x40),
    GC_FRM_NOSHADOW = ((u8)0x80)
} GC_FRM_TYPE_t;
```

- **GC_FRM_NONE** - окно без рамок
- **GC_FRM_SINGLE** - окно с одинарной рамкой
- **GC_FRM_DOUBLE** - окно с двойной рамкой
- **GC_FRM_NOLOGO** - не выводить лого в заголовке окна
- **GC_FRM_NOHEADER** - окно без заголовка
- **GC_FRM_NOSHADOW** - окно без тени



#### <a name="WIN_COLORS_t">Атрибуты цвета</a>

```
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
```



### Описатель Simple Vertical Menu

```
typedef struct GC_SVMENU_t
{
    GC_SVM_FLAG_t   flags;
    WIN_COLORS_t    attr;
    u8              margin;
    u8              current;
    u8              count;
    void            *cb_cursor; // *svmcb_cursor_t
    void            *cb_keys;   // *svmcb_keys_t
} GC_SVMENU_t;
```

- **flags** - флаги
- **attr** - [атрибут цвета](#WIN_COLORS_t) курсора
- **margin** - отступ от верха
- **current** - текущая позиция
- **count** - количество пунктов
- ***cb_cursor** - CALLBACK функция при перемещении курсора
- ***cb_keys** - CALLBACK функция при нажатии на клавишу

#### Флаги Simple Vertical Menu

```
typedef struct GC_SVM_FLAG_t
{
    unsigned    bit0        :1;
    unsigned    bit1        :1;
    unsigned    bit2        :1;
    unsigned    bit3        :1;
    unsigned    bit4        :1;
    unsigned    bit5        :1;
    unsigned    SVMF_NOWRAP :1;
    unsigned    SVMF_EXIT   :1;
} GC_SVM_FLAG_t;
```

- **SVMF_NOWRAP** - не зацикливать курсор

- **SVMF_EXIT** - выход из меню по нажатию клавиши **KEY_TAB**

  

### Описатель диалога

```
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
```

- **current** - текущий пункт

- **all_count** - количество всех пунктов

- **act_count** - количество активных пунктов

- **cur_attr** - [атрибут цвета](#WIN_COLORS_t) курсора

- **box_attr** - [атрибут цвета](#WIN_COLORS_t) элемента **DI_GROUPBOX**

- **btn_focus_attr** - [атрибут цвета](#WIN_COLORS_t) элемента **DI_BUTTON** в фокусе

- **btn_unfocus_attr** - [атрибут цвета](#WIN_COLORS_t) элемента **DI_BUTTON** не в фокусе

- **lbox_focus_attr** - [атрибут цвета](#WIN_COLORS_t) элемента **DI_LISTBOX**[^1] в фокусе

- **lbox_unfocus_attr** - [атрибут цвета](#WIN_COLORS_t) элемента **DI_LISTBOX**[^1]не в фокусе

- ** **items** - указатель на массив указателей [элементов диалога](#GC_DITEM_t)

  (**ВАЖНО!** активные элементы должны перечисляться первыми, в конце списка
  пассивные элементы, такие как **DI_TEXT**, **DI_GROUPBOX**, **DI_HDIV**)

  

#### <a name="GC_DITEM_t">Элементы диалога</a>

```
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
```

- **type** - тип элемента диалога

- **id** - идентификатор элемента (используется для возвращения нажатой кнопки в диалоге)

- **x**, **y** -  координаты элемента

- **width** - ширина элемента (для элемента **DI_CHECKBOX** ширина просчитывается автоматически)

- **hight** - высота элемента; используется для элемента **DI_GROUPBOX**

  в других элементах значение игнорируется

- **flags** - флаги элемента
  **DIF_GREY** - неактивный элемент
  **DIF_TABSTOP** - стоп-флаг для навигации по клавише **KEY_TAB** и **KEY_PGUP**, **KEY_PGDN**
  (**Внимание!** обязательно наличие одного флага)
  **DIF_RIGHT** - выравнивание вправо по width

- **vartype** - тип переменной

- **hotkey** - горячая клавиша

- **select** - используется в зависимости от типа элемента

- ***var** - указатель на переменную

- ***name** - указатель на название элемента

- **exec** - указатель на функцию, вызываемую при выборе элемента.
  (может использоваться для изменения активности/неактивности элемента выставлением флага **DIF_GREY**)



##### Типы элементов диалога

```
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
```



###### DI_TEXT

- **x** - X координата
- **y** - Y координата

- **name** - название элемента

  

###### DI_HDIV

- **y** - Y координата

- **name** - название элемента



###### DI_GROUPBOX

- **x** - X координата
- **y** - Y координата
- **name** - название элемента



###### DI_EDIT

- **x** - X координата
- **y** - Y координата

 * **width** - ширина поля ввода
 * **name** - указатель на строку



###### DI_CHECKBOX

- **x** - X координата
- **y** - Y координата

- **width** - рассчитывается автоматически

- **var** - указатель на переменную (u8) 0x00 - OFF  0xFF - ON

- **name** - название элемента

  

###### DI_RADIOBUTTON

- **x** - X координата
- **y** - Y координата

- **width** - рассчитывается автоматически
- **select** - значение для занесения в переменную при выборе
- **var** - указатель на переменную (u8)
- **name** - название элемента



###### DI_LISTBOX

- **x** - X координата
- **y** - Y координата

- **width** - ширина поля выбора подпунктов
- **select** - количество подпунктов
- **var** - указатель на переменную (u8) на текущуюю позицию
- **name** - указатель на массив строк



###### DI_BUTTON

- **id** - код элемента, возвращаемый диалогом

- **x** - X координата
- **y** - Y координата

 * **width** - ширина элемента
 * **select** - должен быть 0x00 (используется для анимации нажатия)
 * **name** - название элемента



###### DI_PROGRESSBAR

- **x** - X координата
- **y** - Y координата

- **width** - ширина элемента
- **var** - указатель на переменную (u8) (0-100)%



##### Коды возврата диалога

```
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
```



##### Флаги элементов диалога

```
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
```

- **DIF_GREY** - активный/неактивный элемент
- **DIF_TABSTOP** - стоп-флаг для навигации по кнопке **KEY_TAB** и **KEY_PGUP**, **KEY_PGDN**
  (**Внимание!** обязательно наличие одного флага)



##### Переменные диалога

```
typedef struct GC_DITEM_VAR_t
{
    GCIV_SIZE_t     DIV_SIZE    :2;
    GCIV_TYPE_t     DIV_TYPE    :2;
    unsigned        bit4        :1;
    unsigned        bit5        :1;
    unsigned        bit6        :1;
    unsigned        DIV_TEXT    :1; // not yet
} GC_DITEM_VAR_t;
```

##### Типы переменных диалога

```
typedef enum GCIV_TYPE_t
{
    GCIVT_DEC     = 0,
    GCIVT_HEX     = 1,
    GCIVT_BIN     = 2     // not yet
} GCIV_TYPE_t;
```

```
typedef enum GCIV_SIZE_t
{
    GCIVS_BYTE    = 0,
    GCIVS_WORD    = 1,
    GCIVS_DWORD   = 2,
    GCIVS_QWORD   = 3     // not yet
}  GCIV_SIZE_t;
```



## Формат текстовой строки

Все строки и названия элементов поддерживают разметку

INK_[COLOR]

PAPER_[COLOR]

где [COLOR]:

- **BLACK** - черный
- **BLUE** - синий
  **RED** - красный
- **MAGENTA** - пурпурный
- **GREEN** - зеленый
- **CYAN** - голубой
- **YELLOW** - желтый
- **WHITE** - белый
- **BRIGHT_BLUE** - ярко-синий
- **BRIGHT_RED** - ярко-красный
- **BRIGHT_MAGENTA** - ярко-пурпурный
- **BRIGHT_GREEN** - ярко-зеленый
- **BRIGHT_CYAN** - ярко-голубой
- **BRIGHT_YELLOW** - ярко-желтый
- **BRIGHT_WHITE** - ярко-белый



**MARK_INVERT** - инвертирование атрибута цвета

**MARK_CENTER** - выравнивание по центру

**MARK_RIGHT** - выравнивание по правому краю

**MARK_LINK** "\xN" - подлинковка строки

где N номер строки из массива, который необходимо
инициализировать через gcSetLinkedMessage(u16 **ptr)





[^1]: также используется для элементов DI_EDIT