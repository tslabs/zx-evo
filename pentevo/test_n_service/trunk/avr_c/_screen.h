#ifndef _SCREEN_H
#define _SCREEN_H

#include "_types.h"

typedef struct {
 u8 x;                                  // коорд.лев.верхн угола окна
 u8 y;                                  //
 u8 width;                              // ширина (без учёта тени)
 u8 height;                             // высота (без учёта тени)
 u8 attr;                               // атрибут окна
 u8 flag;                               // флаги: .0 - "с тенью/без тени"
} WIND_DESC, * const P_WIND_DESC;

typedef struct {
 u8 x;                                  // коорд.лев.верхн угола окна
 u8 y;                                  //
 u8 width;                              // длина_строки + 2 = ширина без учёта рамки и тени
 u8 items;                              // количество пунктов меню
 PBKHNDL bkgnd_task;                    // ссылка на фоновую задачу
 u16 bgtask_period;                     // период вызова фоновой задачи, мс (1..16383)
 const PITEMHNDL * const handlers;      // указатель на структуру указателей на обработчики
 const u8 *strings;                     // указатель на текст меню
} MENU_DESC, * const P_MENU_DESC;


void scr_set_attr(u8 attr);
void scr_set_cursor(u8 x, u8 y);
void scr_print_msg(const u8 *msg);
void scr_print_mlmsg(const u8 * const *mlmsg);
void scr_print_msg_n(const u8 *msg, u8 size);
void scr_print_rammsg_n(u8 *msg, u8 size);
void scr_putchar(u8 ch);
void scr_fill_char(u8 ch, u16 count);
void scr_fill_char_attr(u8 ch, u8 attr, u16 count);
void scr_fill_attr(u8 attr, u16 count);
void scr_backgnd(void);
void scr_fade(void);
void scr_window(const P_WIND_DESC pwindesc);
void scr_menu(const P_MENU_DESC pmenudesc);

#endif
