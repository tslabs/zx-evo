
// WildCommander API Header
// Refer to plugins/READ_ME.TXT

#pragma once

//---------------------------------------
// Addresses
#define _WCINT  0x5BFF // адрес обработки int wild commander

#define _PAGE0  0x6000 // номер страницы подключенной с адреса 0x0000-0x3fff
#define _PAGE1  0x6001 // номер страницы подключенной с адреса 0x4000-0x7fff
#define _PAGE2  0x6002 // номер страницы подключенной с адреса 0x8000-0xdfff
#define _PAGE3  0x6003 // номер страницы подключенной с адреса 0xc000-0xffff
#define _ABT    0x6004 // флаг выставляется, если был нажат ESC
#define _ENT    0x6005 // флаг выставляется, если был нажат ENTER
#define _WCAPI  0x6006 // точка доступа к функциям
#define _TMN    0x6009 // синхра. переменная-таймер, инкрементится по инту

//---------------------------------------
// API functions
#define _MNGC_PL 0x00  // включение страницы на 0xC000 (из выделенного блока)
                      // нумерация совпадает с использующейся в +36
                      // i:A' - номер страницы (от 0)
                      // 0xFF - страница с фонтом (1го текстового экрана)
                      // 0xFE - первый текстовый экран (в нём панели)
#define _MNG0_PL 0x78
#define _MNG8_PL 0x79

#define _PRWOW 0x01  // вывод окна на экран
                    // i:IX - адрес по которому лежит структура окна (SOW)

#define _RRESB 0x02  // cтирание окна (восстановление информации)
                    // i:IX - SOW

#define _PRSRW 0x03  // печать строки в окне
                    // i:IX - SOW
                    // HL - Text addres (должен быть в 0x8000-0xBFFF!)
                    // D - Y
                    // E - X
                    // BC - Length

#define _PRIAT 0x04  // выставление цвета (вызывается сразу после PRSRW)
                    // i:PRSRW - выставленные координаты и длина
                    // A' - цвет

#define _GADRW 0x05  // получение адреса в окне
                    // i:IX - SOW
                    // D - Y
                    // E - X
                    // o:HL - Address

#define _CURSOR 0x06 // печать курсора
                    // i:IX - SOW

#define _CURSER 0x07 // стирание курсора (восстановление цвета)
                    // i:IX - SOW

#define _YN 0x08     // меню ok/cancel
                    // i:A'
                    // 0x01 - инициализация (вычисляет координаты)
                    // 0x00 - обработка нажатий (вызывать раз в фрейм)
                    // 0xFF - выход
                    // o:NZ - выбран CANCEL
                    // Z - выбран OK

#define _ISTR 0x09   // редактирование строки
                    // i:A'
                    // 0xFF - инициализация (рисует курсор)
                    // i:HL - адрес строки
                    // DE - CURMAX+CURNOW (длина строки + начальная позиция курсора в ней)
                    // 0x00 - опрос клавиатуры
                    // >опрашивает LF,RG,BackSpace
                    // >собственно редактируется строка
                    // >нужно вызывать каждый фрейм
                    // 0x01 - выход (стирает курсор)

#define _NORK 0x0A   // перевод байта в HEX (текстовый формат)
                    // i:HL - Text Address
                    // A - Value

#define _DMAPL 0x0D  // работа с DMA
                    // i: A' - тип операции
                    // 0x00 - инит S и D (BHL - Source, CDE - Destination)
                    // 0x01 - инит S (BHL)
                    // 0x02 - инит D (CDE)
                    // 0x03 - инит S с пагой из окна (HL, B - 0-3 [номер окна])
                    // 0x04 - инит D с пагой из окна (HL, B - 0-3 [номер окна])
                    // 0x05 - выставление DMA_T (B - кол-во бёрстов)
                    // 0x06 - выставление DMA_N (B - размер бёрста)
                    //
                    // 0xFD - запуск без ожидания завершения (o:NZ - DMA занята)
                    // 0xFE - запуск с ожиданием завершения (o:NZ - DMA занята)
                    // 0xFF - ожидание готовности дма
                    //
                    // в функциях 0x00-0x02 формат B/C следующий:
                    // [7]:%1 - выбор страницы из блока выделенного плагину (0-5)
                    // %0 - выбор страницы из видео буферов (0-31)
                    // [6-0]:номер страницы

#define _TURBOPL 0x0E  // i:B - выбор Z80/AY
                      // 0x00 - меняется частота Z80
                      // i:C - %00 - 3.5 MHz
                      // %01 - 7 MHz
                      // %10 - 14 MHz
                      // %11 - 28 MHz (в данный момент 14MHz)
                      // 0x01 - меняется частота AY
                      // i:C - %00 - 1.75 MHz
                      // %01 - 1.7733 MHz
                      // %10 - 3.5 MHz
                      // %11 - 3.546 MHz

#define _GEDPL 0x0F  // восстановление палитры, всех офсетов и txt режима
                    // ! обязательно вызывать при запуске плагина!
                    // (включает основной txt экран)
                    // i:none

// клавиатура: NZ - клавиша нажата
//--------------------------------------------------
#define _SPKE   0x10 // (SPACE)
#define _UPPP   0x11 // (UP Arrow)
#define _DWWW   0x12 // (Down Arrow)
#define _LFFF   0x13 // (Left Arrow)
#define _RGGG   0x14 // (Right Arrow)
#define _TABK   0x15 // (Tab)
#define _ENKE   0x16 // (Enter)
#define _ESC    0x17 // (Escape)
#define _BSPC   0x18 // (Backspace)
#define _PGU    0x19 // (pgUP)
#define _PGD    0x1A // (pgDN)
#define _HOME   0x1B
#define _END    0x1C
#define _F1     0x1D
#define _F2     0x1E
#define _F3     0x1F
#define _F4     0x20
#define _F5     0x21
#define _F6     0x22
#define _F7     0x23
#define _F8     0x24
#define _F9     0x25
#define _F10    0x26
#define _ALT    0x27
#define _SHIFT  0x28
#define _CTRL   0x29
#define _KBSCN  0x2A // опрос клавиш
                    // i:A' - обработчик
                    // 0x00 - учитывает SHIFT (TAI1/TAI2)
                    // 0x01 - всегда выдает код из TAI1
                    // o: NZ: A - TAI1/TAI2 (see PS2P.ASM)
                    // Z: A=0x00 - unknown key
                    // A=0xFF - buffer end

#define _CAPS   0x2C // (Caps Lock)
#define _ANYK   0x2D // any key
#define _USPO   0x2E // pause for keyboard ready (recomended for NUSP)
#define _NUSP   0x2F // waiting for any key

// работа с файлами:
//---------------------------------------
#define _LOAD512 0x30  // потоковая загрузка файла
                      // i:HL - Address
                      // B - Blocks (512b)
                      // o:HL - New Value

#define _SAVE512 0x31  // потоковая запись файла
                      // i:HL - Address
                      // B - Blocks (512b)
                      // o:HL - New Value

#define _GIPAGPL 0x32  // позиционировать на начало файла
                      // (сразу после запуска плагина — уже вызвана)

#define _TENTRY 0x33   // получить ENTRY(32) из коммандера
                      // (структура как в каталоге FAT32)
                      // i:DE - Address
                      // o:DE(32) - ENTRY

#define _CHTOSEP 0x34  // разложение цепочки активного файла в сектора
                      // i:DE - BUFFER (куда кидать номера секторов)
                      // BC - BUFFER END (=BUFFER+BUFFERlenght)

#define _TENTRYN 0x35  // reserved ???

#define _TMRKDFL 0x36  // получить заголовок маркированного файла
                      // i:HL - File number (1-2047)
                      // DE - Address (32byte buffer) [0x8000-0xBFFF!]
                      // (if HL=0// o:BC - count of marked files)
                      // o:NZ - File not found or other error
                      // Z - Buffer updated
                      // >так же делается позиционированиена на начало этого файла!!!
                      // >соотв. функции LOAD512/SAVE512 будут читать/писать этот файл от начала.

#define _TMRKNXT 0x37  // reserved ???

#define _STREAM 0x39   // работа с потоками
                      // i:D - номер потока (0/1)
                      // B - устройство: 0-SD(ZC)
                      // 1-Nemo IDE Master
                      // 2-Nemo IDE Slave
                      // C - раздел (не учитывается)
                      // BC=0xFFFF: включает поток из D (не возвращает флагов)
                      // иначе создает/пересоздает поток.
                      // o:NZ - устройство или раздел не найдены
                      // Z - можно начинать работать с потоком

#define _MKFILE 0x3A   // создание файла в активном каталоге
                      // i:DE(12) - name(8)+ext(3)+flag(1)
                      // HL(4) - File Size
                      // o:NZ - Operation failed
                      // A - type of error (in next versions!)
                      // Z - File created
                      // ENTRY(32) [use TENTRY]

#define _FENTRY 0x3B   // поиск файла/каталога в активной директории
                      // i:HL - flag(1),name(1-12),0x00
                      // flag:0x00 - file
                      // 0x10 - dir
                      // name:"NAME.TXT","DIR"...
                      // o: Z - entry not found
                      // NZ - CALL GFILE/GDIR for activating file/dir
                      // [DE,HL] - file length

#define _LOAD256 0x3C  // reserved ???
#define _LOADNONE 0x3D // reserved ???

#define _GFILE 0x3E    // выставить указатель на начало найденного файла
                      // (вызывается после FENTRY!)

#define _GDIR 0x3F     // сделать найденный каталог активным
                      // (вызывается после FENTRY!)

// работа с режимами графики
//---------------------------------------
#define _MNGV_PL 0x40  // включение видео страницы
                      // i:A' - номер видео страницы
                      // 0x00 - основной экран (тхт)
                      // >палитра выставляется автоматом
                      // >как и все режимы и смещения
                      // 0x01 - 1й видео буфер (16 страниц)
                      // 0x02 - 2й видео буфер (16 страниц)

#define _MNGCVPL 0x41  // включение страницы из видео буферов
                      // i:A' - номер страницы
                      // 0x00-0x0F - страницы из 1го видео буфера
                      // 0x10-0x1F - страницы из 2го видео буфера

#define _GVmod 0x42    // включение видео режима (разрешение+тип)
                      // i:A' - видео режим
                      // [7-6]: %00 - 256x192
                      // %01 - 320x200
                      // %10 - 320x240
                      // %11 - 360x288
                      // [5-2]: %0000
                      // [1-0]: %00 - ZX
                      // %01 - 16c
                      // %10 - 256c
                      // %11 - txt

#define _GYoff 0x43    // выставление смещения экрана по Y
                      // i:HL - Y (0-511)

#define _GXoff 0x44    // выставление смещения экрана по X
                      // i:HL - X (0-511)

#define _GVtm  0x69    // выставление страницы для TileMap
#define _GVtl  0x70    // выставление страницы для TileGraphics
#define _GVsgp 0x71    // выставление страницы для SpriteGraphics

typedef enum
{
  WC_WIND_HDR_TXT = 1,
} WC_WIND_TYPE;

typedef struct
{
  WC_WIND_TYPE type;
  u8 curs_col_mask;
  u8 x_pos;
  u8 y_pos;
  u8 x_size;
  u8 y_size;
  u8 attr;
  u8 res0;
  u16 restore_addr;
  u8 div0;
  u8 div1;
  u16 *hdr_txt;
  u16 *ftr_txt;
  u16 *wnd_txt;
} WC_TX_WINDOW;

typedef enum
{
  WC_CALL_EXT    = 0x00,
  WC_CALL_ONLOAD = 0x01,
  WC_CALL_TIMER  = 0x02,
  WC_CALL_MENU   = 0x03,
  WC_CALL_EDIT   = 0x14,
} WC_CALL_TYPE;

typedef enum
{
  WC_EXIT       = 0,
  WC_TYPE_UNK   = 1,
  WC_NEXT_FILE  = 2,
  WC_REREAD_DIR = 3,
  WC_PREV_FILE  = 4,
} WC_EXIT_SIGNAL;
