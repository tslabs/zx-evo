# Оконная библиотека gcWindow

## Функции

### Инициалицация

```
void gcWindowsInit(u8 vpage, u8 spage) __naked;
```

- **vpage** - видеостраница
- **spage** - страница теневого экрана



### Вывод окна

```
void gcPrintWindow(GC_WINDOW_t *wnd) __naked __z88dk_fastcall;
```

- ***wnd** - указатель на описатель окна



```
void gcDrawWindow(u8 id, u8 x, u8 y, u8 width, u8 hight, WIN_COLORS_t attr, GC_FRM_TYPE_t frame_type, WIN_COLORS_t frame_attr) __naked;
```

(внутренняя функция)

- **x** - X координата окна
- **y** - Y координата окна
- **width** - ширина окна
- **hight** - высота окна
- **attr** - атрибут цвета окна
- **frame_type** - тип рамки окна
- **frame_attr** - атрибут цвета рамки окна



## Описатели

### Описатель окна

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
- **attr** - атрибут цвета окна
- **frame_type** - тип рамки окна
- **frame_attr** - атрибут цвета рамки окна
- ***header_txt** - указатель на текст заголовка окна
- ***window_txt** - указатель на текст окна
- ***menu_ptr** - указатель на описатель меню
- ***int_proc** - не реализовано



#### Типы окон

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



#### Типы рамок окна

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



#### Атрибуты цвета окна

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
- **attr** - атрибут цвета курсора
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
- **cur_attr** - атрибуты цвета курсора
- **box_attr** - атрибуты цвета элемента DI_GROUPBOX
- **btn_focus_attr** - атрибуты цвета элемента DI_BUTTON в фокусе
- **btn_unfocus_attr** - атрибуты цвета элемента DI_BUTTON не в фокусе
- **lbox_focus_attr** - атрибуты цвета элемента DI_LISTBOX(*) в фокусе
- **lbox_unfocus_attr** - атрибуты цвета элемента DI_LISTBOX(*) не в фокусе
- ** **items** - указатель на массив указателей элементов диалога

#### Элементы диалога









