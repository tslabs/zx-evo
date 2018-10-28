# TS-Configuration

## Features

- High compatibility with original Pentagon-128 clone

- Advanced video features:
  - Pixel resolutions 360x288, 320x240, 320x200, 256x192
  - Up to 720x288 Hi-res pixel resolution
  - Hardware scrolled graphic planes
  - 256 and 16 indexed colors per pixel
  - Programmable color RAM with RGB555 color space and 256 cells
  - 512 and 256 bytes per line addressing
  - Text mode with loadable font and hardware vertical scroll
  - Up to 256 graphic screens

- Hardware engine for Tiles and Sprites graphics
  - Up to 85 sprites per line
  - Sprites sized from 8x8 to 64x64 pixels
  - Up to 3 sprite planes
  - Up to 2 tile planes with 8x8 pixels tiles
  - Up to 16 palettes for sprites per line
  - Up to 4 palettes for tiles per line for each tile plane

- Z80 Memory addressing enhancements:
  - Programmable RAM page for any 16kB window

- Z80 acceleration features
  - Selectable CPU clock 14MHz, 7MHz and 3,5MHz
  - 512 bytes of zero-wait RAM for 14MHz
  - On-the-fly programmable maskable interrupt position
  - Separate IM2 vectors for different interrupt sources

- Advanced hardware features
  - DRAM-to-Device, Device-to-DRAM and DRAM-to-DRAM DMA Controller

## Conventions

### Formating and styles

The following formatting is used in this document.

#C000 - hexadecimal numbers.

**VConfig** - registers names.

***NOGFX*** - register bits names.

*window* - special term.

`HALT` - CPU mnemonics.

### Terms

R/W - register or register bit access. R - read, W - write.

**REG**[n] - bit n from register **REG**.

**REG**[n..m] - bits n to m from register **REG**.

## Achitecture

(Here should be a figure, but I still cannot handle it...)

## Programming model

Hardware in TS-Configuration is controlled via dedicated pull of ports with common address #nnAF. They can be written and/or read. Their access modes are specified in descriptions.

Z-Controller, Gluclock and Pentagon-128 ports are also supported. See correspondent sections for their description.

It is also possible to access #nnAF registers for write by writing memory in pre-configured addresses using **FMAddr** register.

Dedicated memory files, e.g. color RAM and sprite descriptors are only accessed using **FMAddr**.

### Register access

Access to TS-Configuration registers is performed via set of CPU ports with address #nnAF, where lower A0..A7 address part is #AF and higher A8..A15 is a register number.

Some hardware internal registers are more than 8 bits wide. They are split to a number of #nnAF registers.

On Reset some registers are initialized with default values.

<u>***Please notice:***</u>

Unused bits in a register must always be written 0, if not specified otherwise. This is to maintain compatibility with future versions of configuration.

When reading from a register unused bits must be ignored.

### FMAPS

### Registers table

Currently available in this [file](TSconf.xls).

## Процессор и память

### Страничная адресация памяти

TS конфигурация позволяет адресовать до 4096кБ ОЗУ и 512кБ ПЗУ.

Since Z80 CPU has 16-bit address bus it can only access 64kB of memory. CPU addressable span is split into four regions - *windows*. Each *window* is 16kB and has its dedicated 8-bit register to select memory *page* to be mapped into the correspondent CPU addresses. 256 *pages* 16kB each give 4096kB of addressable memory.

Since ROM is only 512kB three MSBs of correspondent *page* register are not used when ROM is selected for mapping.

See table below for *windows* addresses and *page* registers.

CPU *window*|CPU address|*Page* register|R/W|Reset value
----------|---------------|------------------------|---|----
0         |#0000..#3FFF    |Page0                   | W |#00
1         |#4000..#7FFF    |Page1                   | W |#05
2         |#8000..#BFFF    |Page2                   |R/W|#02
3         |#C000..#FFFF    |Page3                   |R/W|#00

Only RAM can be mapped into *windows* 1..3.

RAM or ROM can be mapped into *window* 0. See description below.

#### Маппинг страниц в окне 0

В нулевом окне можно отображать страницы RAM/ROM в двух режимах:

- Режим маппинга страниц
- Обычный режим

Режим выбирается с помощью бита *!W0_MAP* регистра **MemConfig**: 0 - режим маппинга страниц, 1 - обычный режим.

В обычном режиме номер проецируемой страницы берется из регистра **Page0**[7:0] для RAM и **Page0**[4:0] для ROM.

Режим маппинга страниц используется для подключения 64кб прошивок ПЗУ, которые могут располагаться как в ROM, так и в RAM компьютера. Выбор страницы этого ПЗУ происходит с помощью сигнала DOS и бита *ROM128* регистра **MemConfig** (или порта #7FFD). Сигнал DOS и бит *ROM128* подставляются в качестве 1 и 0 бита номера страницы соответственно, а все остальные биты берутся из регистра **Page0**, таким образом прошивки ПЗУ должны располагаться в страницах кратных 4.

Формат прошивок ПЗУ и управляющие сигналы для выбора страниц этого ПЗУ представлены в следующей таблице:

| DOS  | ROM128 | Страницы ПЗУ |
| ---- | ------ | ------------ |
| 0    | 0      | Service      |
| 0    | 1      | DOS          |
| 1    | 0      | 128          |
| 1    | 1      | 48           |

#### Paging via port #7FFD

Port #7FFD provides compatibility with Pentagon-128k (with 512k and 1024k extensions).

Paging mode *LCK128*[1:0] регистра **MemConfig**:

LCK128[1:0]|Режим порта #7FFD| Подстановка бит в регистр Page3
-----------|-----------------|--------------------------------------
00         | 512кБ        | Page3[4:0] = #7FFD[7:6], #7FFD[2:0]
01         | 128кБ         | Page3[2:0] = #7FFD[2:0]
10         | Авто            | See description below
11         | 1024кБ        | Page3[5:0] = #7FFD[5], #7FFD[7:6], #7FFD[2:0]

При записи значений в порт #7FFD в режимах 128кБ, 512кБ и 1024кБ, биты, отвечающие за переключение страниц, записываются в младшие разряды регистра **Page3**.

Режим *Авто* позволяет с помощью длинной адресации порта работать с 512кБ RAM (`out (c),a`), а через короткую адресацию с 128кБ RAM (`out (#FD),a`). Режим адресации определяется по опкоду команды..

The function of bits 5..7 of port #7FFD depends on Lock mode set in **MemConfig** register.

| Bits   | 7     | 6     | 5     | 4    | 3    | 2     | 1     | 0     |
| ------ | ----- | ----- | ----- | ---- | ---- | ----- | ----- | ----- |
| Name*1 | -     | -     | LOCK  | ROM  | SCR  | PG[2] | PG[1] | PG[0] |
| Name*2 | PG[4] | PG[3] | LOCK  | ROM  | SCR  | PG[2] | PG[1] | PG[0] |
| Name*3 | PG[4] | PG[3] | PG[5] | ROM  | SCR  | PG[2] | PG[1] | PG[0] |
| R/W    | W     | W     | W     | W    | W    | W     | W     | W     |
| Init   | 0     | 0     | 0     | 0    | 0    | 0     | 0     | 0     |

*1 - for 128k mode
*2 - for 512k mode
*3 - for 1024k mode

* ***PG*** - Selects page mapped at address #C000. Only first 1024kB of RAM available for this paging.
* ***LOCK*** - Disables paging and locks the selected page at #C000. For example, if value #23 is written, the page number 3 will be mapped to #C000 and cannot be changed via port #7FFD until system reset. Paging via **Page0..3** is still available.
* ***ROM*** - Selects Basic-128 ROM page. See description of **MemConfig**.
* ***SCR*** - Selects active screen. 0 writes 5 to ***VPage***, while 1 writes 7. Has no line-latch effect (see [line-latch]).

### CPU modes control

CPU clock frequency can be selected from 14 / 7 / 3.5MHz.

In 14MHz mode each access to RAM is delayed to be synchronized with DRAM Controller which works slower than CPU. To minimize the effect of such throttling Cache Controller is used. A dedicated 512 bytes Cache Memory caches RAM read accesses and provides data for the CPU without delay on second and next accesses to the cached addresses. See [Cache description] for more detailes.

Cache has no effect in 3.5 and 7MHz modes.

#### Частота процессора

Всего поддерживается 3 режима работы процессора: 3.5MHz, 7.0MHz, 14.0MHz. Два младших бита регистра **SysConfig** позволяют выбрать один из этих режимов согласно следующей таблице:

ZCLK[1:0]| MHz
---------|----
00       |3.5
01       |7.0
10       |14.0
11       |зарезервировано

#### Кэш

В TS конфигурации можно включить кэширование всех запросов на чтение/запись между процессором и RAM. Запросы к ROM не кэшируются.

Отображаемая процессору память (64кб) условно делится на 4 окна по 16 килобайт и для каждого такого окна в регистре **CacheConfig** предусмотрен соответствующий бит, который включает или отключает кэшироване в этом окне.

### Registers

#### MemConfig

| Bit         | 7         | 6         | 5    | 4    | 3      | 2       | 1     | 0      |
| ----------- | --------- | --------- | ---- | ---- | ------ | ------- | ----- | ------ |
| Name        | LCK128[1] | LCK128[0] | -    | -    | W0_RAM | !W0_MAP | W0_WE | ROM128 |
| R/W         | W         | W         | -    | -    | W      | W       | W     | W      |
| Reset value | 0         | 0         | x    | x    | 0      | 1       | 0     | 0      |

* Биты 7:6 - выбор режима ограничений порта #7FFD,
* Биты 5:4 - зарезервированы,
* Бит 3 - выбор ROM/RAM,
* Бит 2 - отключение маппинга,
* Бит 1 - разрешение записи в ROM/RAM,
* Бит 0 - выбор страниц маппинга.

С помощью бита *W0_WE* регистра **MemConfig** можно запретить (значение 0) или разрешить (значение 1) запись в окно 0. Для страниц RAM - это просто запрет записи данных. Для ROM запись будет производится на флеш память.

#### SysConfig

Bit        |7|6|5|4|3|2|1|0
-----------|-|-|-|-|-|-|-|-|
Name       |-|-|-|-|-|CACHE|ZCLK[1]|ZCLK[0]
R/W        |-|-|-|-|-|W|W|W
Reset value|x|x|x|x|x|0|0|0

* Биты 7:3 - зарезервированы.
* Бит 2 - разрешение кэширования запросов в RAM,
* Биты 1:0 - управление частотой процессора

#### CacheConfig

	Биты 7  6  5  4    3       2       1       0
	     -  -  -  - EN_C000 EN_8000 EN_4000 EN_0000
	R/W                W       W       W       W
	Init x  x  x  x    0       0       0       0

* Биты 7:4 - зарезервированы.
* Бит 3 - разрешение кэширования окна RAM #C000-#FFFF,
* Бит 2 - разрешение кэширования окна RAM #8000-#BFFF,
* Бит 1 - разрешение кэширования окна RAM #4000-#7FFF,
* Бит 0 - разрешение кэширования окна RAM #0000-#3FFF,

При записи значения в бит *CACHE* регистра **SysConfig** это же значение копируется в биты 0-3 регистра **CacheConfig**. Это позволяет включать и отключать кэширование глобально для всех страниц RAM.

**Внимание!** Запросы от DMA к RAM не кэшируются. По этой причине после DMA транзакций, работающих с окнами RAM для которых включено кэширование, необходимо проводить инвалидацию кэша.

Для инвалидации кэша требуется записать 512 байт в любые последовательные адреса. При этом необходимо, чтоб по указанным адресам находилось ОЗУ.
Пример инвалидации кэша:

```assembly
LD HL, #FE00
LD DE, #FE00
LD BC, #0200
LDIR
```

#### Page0..3

#### FMAddr

## Контроллер прерываний

Для всех режимов прерываний (`IM 0`, `IM 1`, `IM 2`) TS конфигурация поддерживает несколько источников маскируемого прерывания INT:

* Кадровый (*FRAME*)
* Строчный (*LINE*)
* Окончание DMA транзакции (*DMA*)

В случае прихода нескольких событий одновременно, сначала обрабатывается прерывание с меньшим приоритетом. При завершении ISR инструкциями `EI : RET` сразу произойдет обработка следующего по порядку прерывания, которое распознается в последнем машинном цикле инструкции `RET`.

#### Frame interrupt

Источник *FRAME* срабатывает, когда значение счетчиков растра совпадает с регистрами **HSINT**, **VSINT**. Источник *LINE* срабатывает в каждой строке, когда горизонтальный счетчик растра равен 0. Источник *DMA* срабатывает после окончания DMA транзакции.

#### Line interrupt
#### DMA interrupt
#### Wait Ports interrupt

В режиме `IM 2` каждый источник прерывания формирует сигнал INT и выставляет собственный адрес на шину данных.

Источник|Приоритет|Адрес
--------|---------|-----
FRAME   |    0    | #FF
LINE    |    1    | #FD
DMA     |    2    | #FB

Регистр **INTMask** содержит биты индивидуального источника маскируемого прерывания: 0 - запрещен, 1 - разрешен. По RESET туда записывается значение #01 - разрешен *FRAME*, все остальные запрешены. Если из ISR с меньшим приоритетом записать 0 в соответствующий бит маски источника прерывания, ожидающего в данный момент обработки, то произойдет его сброс и прерывание обработано не будет. Запись 1 не влияет на состояние ожидающего прерывания.

### Registers

#### INTMask

Биты | 7 |  6  | 5  | 4 |  3 |  2  |   1  |   0
-----|---|-----|----|---|----|-----|------|-----
-    | - |  -  | -  | - |  - | DMA | LINE | FRAME
R/W  |   |     |    |   |    |  W  |   W  |   W
Init | x |  x  | x  | x |  x |  0  |   0  |   1

* Биты 7:3 - зарезервированы,
* Бит 2 - разрешение источника *DMA*,
* Бит 1 - разрешение источника *LINE*,
* Бит 0 - разрешение источника *FRAME*,

#### HSINT

#### VSINTL

#### VSINTH

## Graphics

### Video signals

The hardware generates video signals for TV (PAL) and VGA. Originally video signal is generated for PAL TV. Each TV frame contains 320 lines and each TV line contains 448 7MHz pixel periods.

320 lines are non-standard for TV signal and support for this was made for historical reasons. The ZX clone 'Penatagon-128' has an error on its board which adds extra 8 lines to 312 lines of TV PAL standard. This increases number of T-states that CPU executes during the video frame and thus became a feature of ex-USSR demoscene. Some displays may be incompatible with such video signal and won't show the picture properly.

The scan-doubler converts TV signal on-the-fly for VGA displays. In the VGA signal each TV line is doubled, so VGA frame contains 640 lines, but VGA line has twice shorter period than TV line. Each TV line is written into a dedicated line buffer at the time when it's displayed on TV. After the whole line is written the scan-doubler reads it twice and displays it on VGA.

<u>***Please notice:***</u>
The scan-doubler delay should be taken into account in raster effects that use CRAM dynamic change. Line buffer contains indexed pixels (not a true color) and uses the same CRAM as TV output. CRAM change effect designed for TV output will affect VGA output also, but since VGA output is delayed for one TV line it will cause color change one line before currently displayed in VGA output.

### Raster timings

For convenience raster timings are described in terms of TV video signal.

Each TV frame includes blanking fields and visible area.

**Horizontal timings**

| Pixel number | Field              |
| -------------- | ----------------------- |
| 0-87           | Horizontal Blank        |
| 88-447         | Horizontal Visible Area |

**Vertical timings**

| Line number | Field             |
| ------------------ | --------------------- |
| 0-31               | Vertical Blank        |
| 32-319             | Vertical Visible Area |

The visible area contains border and pixels. The size of pixel area depends on bits *RRES*[1:0] of **VConfig** register.

**Horizontal timings**

| RRES[1:0] | Left border area, pixels | Horizontal pixel area, pixels | Right border area, pixels |
| --------- | ------------------------ | ----------------------------- | ------------------------- |
| 00        | 52                       | 256                           | 52                        |
| 01        | 20                       | 320                           | 20                        |
| 10        | 20                       | 320                           | 20                        |
| 11        | 0                        | 360                           | 0                         |

**Vertical timings**

| RRES[1:0] | Upper border area, lines | Vertical pixel area, lines | Lower border area, lines |
| --------- | ------------------------ | -------------------------- | ------------------------ |
| 00        | 48                       | 192                        | 48                       |
| 01        | 44                       | 200                        | 44                       |
| 10        | 24                       | 240                        | 24                       |
| 11        | 0                        | 288                        | 0                        |

### Graphic modes

There are four video modes supported: 

- classic ZX, 
- 16 colors per pixel, 
- 256 colors per pixel,
- text.

The graphic mode of the video controller is selected with bits *VM*[1:0] of **VConfig** register.

| VM[1:0] | Mode |
| ------- | ---- |
| 00      | ZX   |
| 01      | 16c  |
| 10      | 256c |
| 11      | Text |



#### ZX

#### 16c
#### 256c
#### text

### Screen page
### CRAM
#### VDAC mode
#### PWM mode
#### DMA
### Scrolls
### TSU

#### Features
#### Bitmap

#### Tiles

##### Overview

##### Tile map
#### Sprites
##### Overview

##### SFILE

### Registers

#### VConfig

#### VPage

#### GXOffsL

#### GXOffsH

#### GYOffsL

#### GYOffsH

#### T0XOffsL

#### T0XOffsH

#### T0YOffsL

#### T0YOffsH

#### T1XOffsL

#### T1XOffsH

#### T1YOffsL

#### T1YOffsH

#### TSConfig

#### PalSel

#### Border

#### TMPage

#### T0GPage

#### T1GPage

#### SGPage

## Контроллер DMA

Контроллер DMA позволяет передавать данные между памятью и устройствами без участия процессора. 

Скорость передачи данных ограничена скоростью работы контроллера памяти, загруженностью памяти обращениями и скоростью периферийных устройств.

<u>***Обратите внимание:***</u>

Одновременно может выполняться лишь одна DMA транзакция.

Для управления DMA предусмотрены следующие регистры:

Регистр  |Назначение
---------|-----------------------------
DMASAddr*| Адрес источника данных в RAM
DMADAddr*| Адрес приемника данных в RAM
DMALen   | Длина блока
DMANum   | Количество блоков
DMACtrl  | Регистр управления DMA
DMAStatus| Состояние контроллера DMA

DMA транзакция производит передачу данных блоками. Единицей данных в блоках является машинный WORD, т.е. 16 бит. В регистрах DMA контроллера можно установить как размер самого блока (регистр **DMALen**), так и количество блоков (регистр **DMANum**) для обработки. Дополнительно в регистре **DMACtrl** можно задать выравнивание для каждого блока транзакции.

Значения в регистрах **DMALen** и **DMANum** должны быть на единицу меньше, т.к. они находятся в диапазоне 1 - 256.

### Registers

#### DMACtrl

	Биты  7   6   5      4     3    2  1  0
	     W/R  - S_ALGN D_ALGN A_SZ DDEV[2:0]
	R/W   W       W      W     W    W  W  W
	Init  x   x   x      x     x    x  x  x

* Бит 7 - направление передачи данных: 0 - в память, 1 - из памяти,
* Бит 6 - зарезервировано,
* Бит 5 - включение выравнивания адреса источника (0 - отключено, 1 - включено),
* Бит 4 - включение выравнивания адреса приемника (0 - отключено, 1 - включено),
* Бит 3 - размер выравнивания: 0 - 256 байт, 1 - 512 байт,
* Биты 2:0 - выбор устройства для обмена данными.

Если включено выравнивания адреса источника или приемника, то после обработки каждого блока к предыдущему значению этого регистра прибавляется 256 или 512 байт в зависимости от бита *A_SZ*. Если выравнивание для адреса отключено, то значение регистра не изменяется и соответствует тому, что было в нем на момент окончания обработки последнего блока.

Адреса источника и приемника данных (*DMASAddr* и *DMADAddr*) являются 22 битными и для записи в них значений используются по 3 регистра: **DMASAddrL**, **DMASAddrH** и **DMASAddrX** - для адреса источника данных, **DMADAddrL**, **DMADAddrH**, **DMADAddrX** - для адреса приемника данных.

Для удобства адресации регистры **DMASAddrL** и **DMASAddrH** задают смещение внутри страницы памяти, а регистр **DMASAddrX** задает номер страницы. Для адреса приемника все аналогично.

Таблица выбора устройств:

Бит W/R|DDEV[2:0]|Источник|Приемник|Описание
:-----:|:-------:|:------:|:------:|----------------
   0   |   000   |   -    |   -    | Зарезервировано
   1   |   000   |   -    |   -    | Зарезервировано
   0   |   001   |  RAM   |  RAM   | Копирование RAM
   1   |   001   |  BLT   |  RAM   | Блиттинг RAM
   0   |   010   |  SPI   |  RAM   | Копирование из SPI в RAM
   1   |   010   |  RAM   |  SPI   | Копирование из RAM в SPI
   0   |   011   |  IDE   |  RAM   | Копирование из IDE в RAM
   1   |   011   |  RAM   |  IDE   | Копирование из RAM в IDE
   0   |   100   |  FILL  |  RAM   | Заполнение RAM
   1   |   100   |  RAM   |  CRAM  | Копирование из RAM в CRAM
   0   |   101   |   -    |   -    | Зарезервировано
   1   |   101   |  RAM   |  SFILE | Копирование из RAM в SFILE
   0   |   110   |   -    |   -    | Зарезервировано
   1   |   110   |   -    |   -    | Зарезервировано
   0   |   111   |   -    |   -    | Зарезервировано
   1   |   111   |   -    |   -    | Зарезервировано

**Блиттинг RAM**

Данные читаются из источника и приемника. Если значение в приемника не равно 0, то данные из источника записываются туда.

**Заполнение RAM**

Читается один WORD из источника и это значение используется для заполнения приемника.

#### DMAStatus

	Биты    7    6  5  4  3  2  1  0
	     DMA_ACT -  -  -  -  -  -  -
	R/W     R
	Init    x    x  x  x  x  x  x  x

* Бит 7 - состояние DMA транзакции (0 - не активна, 1 - активна),
* Биты 6:0 - зарезервированы.

Состояние DMA транзакции можно определить по биту *DMA_ACT* регистра **DMAStatus**. По окончанию транзакции генерируется соответствующее прерывание, если оно было разрешено. Подробнее об этом можно узнать в разделе [Контроллер прерываний][interrupts].

**Внимание!** Данные, передаваемые с помощью DMA не кэшируются, поэтому работать с ними необходимо через окна памяти для которых отключено кэширование, либо после транзакций производить инвалидацию кэша. Подробнее о работе кэша описано в разделе [Кэширование запросов к RAM][cpu#cache].

#### DMASAddrL

#### DMASAddrH

#### DMASAddrX

#### DMADAddrL

#### DMADAddrH

#### DMADAddrX

#### DMAWPDev

#### DMALen

#### DMANum

#### DMANumH

#### DMAWPAddr

## Sound

### Registers

## SPI

### Registers

## IDE

### Registers

## RTC

### Registers

## VDOS

### Registers

#### FDDVirt

## Miscellaneous

### Version detect

### Registers

#### Status

## Configuration extensions

### FDD Ripper

### Registers

#### FRCtrl

#### FRCnt0

#### FRCnt1

#### FRCnt2

## Pentagon-128 compatibility

### Video controller

### INT position

## Credits

Special thanks to: Earl (Dmitry Limonov) for initial version of this datasheet in Russian.

(+++ everybody)