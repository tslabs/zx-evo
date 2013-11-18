#pragma disable_warning 85

//определения типов переменных

typedef unsigned char u8;
typedef   signed char i8;
typedef unsigned  int u16;
typedef   signed  int i16;
typedef unsigned long u32;
typedef   signed long i32;

#define TRUE	1
#define FALSE	0

//флаги кнопок джойстика

#define JOY_RIGHT	0x01
#define JOY_LEFT	0x02
#define JOY_DOWN	0x04
#define JOY_UP		0x08
#define JOY_FIRE	0x10

//смещения в массиве клавиш для опроса клавиатуры

#define KEY_SPACE	0x23
#define KEY_ENTER	0x1e
#define KEY_SYMBOL	0x24
#define KEY_CAPS	0x00

#define KEY_0		0x14
#define KEY_1		0x0f
#define KEY_2		0x10
#define KEY_3		0x11
#define KEY_4		0x12
#define KEY_5		0x13
#define KEY_6		0x18
#define KEY_7		0x17
#define KEY_8		0x16
#define KEY_9		0x15

#define KEY_A		0x05
#define KEY_B		0x27
#define KEY_C		0x03
#define KEY_D		0x07
#define KEY_E		0x0c
#define KEY_F		0x08
#define KEY_G		0x09
#define KEY_H		0x22
#define KEY_I		0x1b
#define KEY_J		0x21
#define KEY_K		0x20
#define KEY_L		0x1f
#define KEY_M		0x25
#define KEY_N		0x26
#define KEY_O		0x1a
#define KEY_P		0x19
#define KEY_Q		0x0a
#define KEY_R		0x0d
#define KEY_S		0x06
#define KEY_T		0x0e
#define KEY_U		0x1c
#define KEY_V		0x04
#define KEY_W		0x0b
#define KEY_X		0x02
#define KEY_Y		0x1d
#define KEY_Z		0x01

//флаги состояния клавиш

#define KEY_DOWN	0x01	//кнопка удерживается
#define KEY_PRESS	0x02	//кнопка нажата, флаг сбрасывается после вызова keyboard

//флаги кнопок мыши

#define MOUSE_LBTN	0x01
#define MOUSE_RBTN	0x02
#define MOUSE_MBTN	0x04

//получение кода цвета из RGB с двухбитными значениями каналов

#define RGB222(r,g,b)	(((b)&3)|(((g)&3)<<2)|(((r)&3)<<4))

//крайние и средний уровни яркости

#define BRIGHT_MIN	0
#define BRIGHT_MID	3
#define BRIGHT_MAX	6

//код конца списка спрайтов

#define SPRITE_END	0xff00



//заполнение памяти заданным значением

void memset(void* m,u8 b,u16 len) _naked;

//копирование памяти, области не должны пересекаться

void memcpy(void* d,void* s,u16 len) _naked;

//генерация 16-битного псевдослучайного числа

u16 rand16(void) _naked;

//установка цвета бордюра, 0..15

void border(u8 n) _naked;

//ожидание следующего ТВ кадра

void vsync(void) _naked;

//опрос kempston джойстика и курсорных клавиш с пробелом
//для опроса кнопок есть константы JOY_

u8 joystick(void) _naked;

//опрос клавиатуры, возвращает состояние клавиш в 40-байтный массив
//для опроса клавиш есть константы KEY_

void keyboard(u8* keys) _naked;

//получение текущей позиции указателя мыши, разрешение по x понижено вдвое, как для спрайтов
//мышь перемещается в заданной зоне клиппинга

u8 mouse_pos(u8* x,u8* y) _naked;

//установка текущей позиции указателя мыши

void mouse_set(u8 x,u8 y) _naked;

//установка зоны клиппинга для указателя мыши, по умолчанию 0..160, 0..200

void mouse_clip(u8 xmin,u8 ymin,u8 xmax,u8 ymax) _naked;

//получение дельты перемещения мыши, без понижения разрешения по x

u8 mouse_delta(i8* x,i8* y) _naked;

//проигрывание звукового эффекта с указанным номером и относительной громкостью -8..8

void sfx_play(u8 sfx,i8 vol) _naked;

//останов всех проигрываемых звуковых эффектов

void sfx_stop(void) _naked;

//проигрывание музыки с указанным номером

void music_play(u8 mus) _naked;

//прекращение проигрывания музыки

void music_stop(void) _naked;

//проигрывание сэмпла с указанным номером через Covox
//во время проигрывания программа ждёт и прерывания запрещены

void sample_play(u8 sample) _naked;

//установка всех значений в палитре в 0 (чёрный цвет)

void pal_clear(void) _naked;

//установка яркости экрана BRIGHT_MIN..BRIGHT_MID..BRIGHT_MAX (0..3..6)
//от полностью чёрного экрана до нормальной яркости до полностью белого экрана

void pal_bright(u8 bright) _naked;

//выбор предопределённой палитры по номеру

void pal_select(u8 id) _naked;

//копирование предопределённой палитры в массив (16 байт)

void pal_copy(u8 id,u8* pal) _naked;

//установка цвета в палитре, id 0..15, col в формате R2G2B2

void pal_col(u8 id,u8 col) _naked;

//установка всех 16 цветов в палитре значениями R2G2B2 из массива

void pal_custom(u8* pal) _naked;

//выбор изображения для функций draw_tile

void select_image(u8 id) _naked;

//выбор прозрачного цвета 0..15 для функции draw_tile_key

void color_key(u8 col) _naked;

//отрисовка тайла из текущего выбранного изображения
//в одном изображении может быть до 65536 тайлов

void draw_tile(u8 x,u8 y,u16 tile) _naked;

//отрисовка тайла с маской, заданной номером прозрачного цвета

void draw_tile_key(u8 x,u8 y,u16 tile) _naked;

//отрисовка изображения целиком

void draw_image(u8 x,u8 y,u8 id) _naked;

//очистка теневого экрана нужным цветом 0..15

void clear_screen(u8 color) _naked;

//переключение экранов, теневой становится видимым
//ожидание кадра выполняется автоматически, vsync перед вызовом этой функции не нужен
//функция также обновляет спрайты, если они включены

void swap_screen(void) _naked;

//запуск системы вывода спрайтов
//на видимом экране должно быть изображение, поверх которого будут выведены спрайты
//эта функция выполняется медленно, происходит копирование большого объёма данных
//после того как спрайты разрешены, они будут автоматически выводиться при swap_screen

void sprites_start(void) _naked;

//останов системы вывода спрайтов

void sprites_stop(void) _naked;

//установка положения спрайта
//id номер в списке 0..63
//x координата 0..152 (точность вывода по горизонтали два экранных пикселя)
//y координата 0..184
//spr номер изображения спрайта, если SPRITE_END, то вывод списка прекращается
//внимание: клиппинг при выводе отсутствует, спрайт не может частично заходить
//за границу экрана

void set_sprite(u8 id,u8 x,u8 y,u16 spr) _naked;

//время с момента запуска программы в кадрах

u32 time(void) _naked;

//задержка, значение в кадрах (1/50 секунды)

void delay(u16 time) _naked;