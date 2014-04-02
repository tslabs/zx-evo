#include "_global.h"
#include "_screen.h"
#include "_ps2k.h"

#define WIN_ATTR 0x9f
#define CURSOR_ATTR 0xf0
#define WIN_SHADOW_ATTR 0x01

//-----------------------------------------------------------------------------
// Установка текущего атрибута
void scr_set_attr(u8 attr)
{
 fpga_reg(SCR_ATTR,attr);
}

//-----------------------------------------------------------------------------
// Установка позиции печати на экране (x - 0..52; y - 0..24)
void scr_set_cursor(u8 x, u8 y)
{
 u16 addr;
 addr=y*53+x-1;
 fpga_reg(SCR_LOADDR,addr&0xff);
 fpga_reg(SCR_HIADDR,(addr>>8)&0x07);
}

//-----------------------------------------------------------------------------

void scr_print_msg(const u8 *msg)
{
 SetSPICS();
 fpga_sel_reg(SCR_CHAR);
 u8 ch;
 do
 {
  ch=pgm_read_byte(msg); msg++;
  if (ch==0x15)
  {
   u8 attr;
   attr=pgm_read_byte(msg); msg++;
   scr_set_attr(attr);
   SetSPICS();
   fpga_sel_reg(SCR_CHAR);
  }
  else if (ch==0x16)
  {
   u8 x, y;
   x=pgm_read_byte(msg); msg++;
   y=pgm_read_byte(msg); msg++;
   scr_set_cursor(x,y);
   SetSPICS();
   fpga_sel_reg(SCR_CHAR);
  }
  else if (ch) fpga_same_reg(ch);
 }while (ch);
}

//-----------------------------------------------------------------------------

void scr_print_mlmsg(const u8 * const *mlmsg)
{
 scr_print_msg((const u8 *)pgm_read_word(mlmsg+lang));
}

//-----------------------------------------------------------------------------

void scr_print_msg_n(const u8 *msg, u8 size)
{
 SetSPICS();
 fpga_sel_reg(SCR_CHAR);
 u8 ch;
 do
 {
  ch=pgm_read_byte(msg); msg++;
  fpga_same_reg(ch);
 }while (--size);
}

//-----------------------------------------------------------------------------

void scr_print_rammsg_n(u8 *msg, u8 size)
{
 SetSPICS();
 fpga_sel_reg(SCR_CHAR);
 do
 {
  fpga_same_reg(*msg++);
 }while (--size);
}

//-----------------------------------------------------------------------------

void scr_putchar(u8 ch)
{
 fpga_reg(SCR_CHAR,ch);
}

//-----------------------------------------------------------------------------

void scr_fill_char(u8 ch, u16 count)
{
 fpga_reg(SCR_CHAR,ch);
 while (--count)
 {
  ClrSPICS();
  SetSPICS();
 }
}

//-----------------------------------------------------------------------------

void scr_fill_char_attr(u8 ch, u8 attr, u16 count)
{
 fpga_reg(SCR_ATTR,attr);
 fpga_reg(SCR_CHAR,ch);
 while (--count)
 {
  ClrSPICS();
  SetSPICS();
 }
}

//-----------------------------------------------------------------------------

void scr_fill_attr(u8 attr, u16 count)
{
 fpga_reg(SCR_FILL,attr);
 while (--count)
 {
  ClrSPICS();
  SetSPICS();
 }
}

//-----------------------------------------------------------------------------

void scr_backgnd(void)
{
 scr_set_cursor(0,0);
 scr_fill_char_attr(0x20,0xf0,53);              // ' '
 scr_fill_char_attr(0xb0,0x77,53*23);           // '░'
 scr_fill_char_attr(0x20,0xf0,53);              // ' '
 flags1&=~(ENABLE_DIRECTUART|ENABLE_UART);
 flags1|=ENABLE_SCR;
 scr_set_cursor(0,0);
 scr_print_msg(msg_title1);
 print_short_vers();
 scr_set_cursor(15,24);
 scr_print_msg(msg_title2);
}

//-----------------------------------------------------------------------------

void scr_fade(void)
{
 scr_set_cursor(0,1);
 scr_fill_attr(0x77,53*23);
}

//-----------------------------------------------------------------------------

void scr_window(const P_WIND_DESC pwindesc)
{
 u8 x, y, width, height, wind_attr, wind_flag;
 x        =pgm_read_byte(&pwindesc->x);
 y        =pgm_read_byte(&pwindesc->y);
 width    =pgm_read_byte(&pwindesc->width)-2;
 height   =pgm_read_byte(&pwindesc->height)-2;
 wind_attr=pgm_read_byte(&pwindesc->attr);
 wind_flag=pgm_read_byte(&pwindesc->flag);

 scr_set_cursor(x,y);
 scr_set_attr(wind_attr);
 scr_putchar(0xda);                            // '┌'
 scr_fill_char(0xc4,width);                    // '─'
 scr_putchar(0xbf);                            // '┐'
 u8 i;
 for (i=0;i<height;i++)
 {
  scr_set_cursor(x,y+i+1);
  scr_set_attr(wind_attr);
  scr_putchar(0xb3);                           // '│'
  scr_fill_char(0x20,width);                   // '─'
  scr_putchar(0xb3);                           // '│'
  if (wind_flag&0x01)
   scr_fill_attr(WIN_SHADOW_ATTR,1);
 }
 scr_set_cursor(x,y+height+1);
 scr_set_attr(wind_attr);
 scr_putchar(0xc0);                            // '└'
 scr_fill_char(0xc4,width);                    // '─'
 scr_putchar(0xd9);                            // '┘'
 if (wind_flag&0x01)
 {
  scr_fill_attr(WIN_SHADOW_ATTR,1);
  scr_set_cursor(x+1,y+height+2);
  scr_fill_attr(WIN_SHADOW_ATTR,width+2);
  scr_set_attr(wind_attr);
 }
}

//-----------------------------------------------------------------------------

const WIND_DESC wind_menu_help PROGMEM = { 3,13,37,9,0xcf,0x01 };
#define p_wind_menu_help ((const P_WIND_DESC)&wind_menu_help)

void menu_help(void)
{
 scr_fade();
 scr_window(p_wind_menu_help);
 scr_print_mlmsg(mlmsg_menu_help);
 waitkey();
}

//-----------------------------------------------------------------------------

const WIND_DESC wind_swlng PROGMEM = { 13,11,27,3,0x9f,0x01 };
#define p_wind_swlng ((const P_WIND_DESC)&wind_swlng)

void menu_swlng(void)
{
 u8 go2;
 do
 {
  lang++;
  if (lang>=TOTAL_LANG) lang=0;
  save_lang();
  scr_fade();
  scr_window(p_wind_swlng);
  scr_set_attr(0x9e);
  scr_print_mlmsg(mlmsg_swlng);
  u16 to;
  set_timeout_ms(&to,2000);
  go2=GO_READKEY;
  do
  {
   u16 key;
   if (inkey(&key))
   {
    if (!((u8)key&(1<<PS2K_BIT_EXTKEY)))
    {
     if ((u8)(key>>8)==KEY_CAPSLOCK)  go2=GO_REPEAT;
     if ((u8)(key>>8)==KEY_ESC)  go2=GO_EXIT;
    }
   }
   else
   {
    if (check_timeout_ms(&to))  go2=GO_EXIT;
   }
  }while (go2==GO_READKEY);

 }while (go2!=GO_EXIT);
}

//-----------------------------------------------------------------------------

#define menu_draw_cursor(x,y,attr,width)\
{                                       \
  scr_set_cursor(x,y);                  \
  scr_fill_attr(attr,width);            \
}

//-----------------------------------------------------------------------------

void scr_menu(const P_MENU_DESC pmenudesc)
{
 u8 menu_select=0, go2;
 do
 {
  u8 x, y, width, items;
  PBKHNDL pBkHndl;
  const u8 * strptr;
  u16 to, BkTO, key;

  scr_backgnd();
  x      =pgm_read_byte(&pmenudesc->x);
  y      =pgm_read_byte(&pmenudesc->y);
  width  =pgm_read_byte(&pmenudesc->width);
  items  =pgm_read_byte(&pmenudesc->items);
  pBkHndl=(PBKHNDL)pgm_read_word(&pmenudesc->bkgnd_task);
  BkTO   =pgm_read_word(&pmenudesc->bgtask_period);
  strptr=(const u8 *)( pgm_read_word(&pmenudesc->strings) + (u16)(lang*items*width) );
  scr_set_cursor(x,y);
  scr_set_attr(WIN_ATTR);
  scr_putchar(0xda);                            // '┌'
  scr_fill_char(0xc4,width+2);                  // '─'
  scr_putchar(0xbf);                            // '┐'
  u8 i;
  for (i=0;i<items;i++)
  {
   scr_set_cursor(x,y+i+1);
   scr_set_attr(WIN_ATTR);
   scr_putchar(0xb3);                           // '│'
   scr_putchar(0x20);                           // ' '
   scr_print_msg_n(strptr,width);
   strptr+=width;
   scr_putchar(0x20);                           // ' '
   scr_putchar(0xb3);                           // '│'
   scr_fill_attr(WIN_SHADOW_ATTR,1);
  }
  scr_set_cursor(x,y+items+1);
  scr_set_attr(WIN_ATTR);
  scr_putchar(0xc0);                            // '└'
  scr_fill_char(0xc4,width+2);                  // '─'
  scr_putchar(0xd9);                            // '┘'
  scr_fill_attr(WIN_SHADOW_ATTR,1);

  scr_set_cursor(x+1,y+items+2);
  scr_fill_attr(WIN_SHADOW_ATTR,width+4);

  if (pBkHndl) { pBkHndl(0); set_timeout_ms(&to,BkTO); }

  menu_draw_cursor(x+1,y+1+menu_select,CURSOR_ATTR,width+2);
  go2=GO_READKEY;

  do
  {
   if (inkey(&key))
   {
    switch ((u8)(key>>8))
    {
     case KEY_ENTER:
       scr_fade();
       {
        const u16 *ptr=(const u16 *)pgm_read_word(&pmenudesc->handlers);
        PITEMHNDL pItemHndl=(PITEMHNDL)pgm_read_word(&ptr[menu_select]);
        if (pItemHndl) pItemHndl();
       }
       go2=GO_REDRAW;
       break;
     case KEY_UP:
       if (menu_select)
       {
        menu_draw_cursor(x+1,y+1+menu_select,WIN_ATTR,width+2);
        menu_select--;
        menu_draw_cursor(x+1,y+1+menu_select,CURSOR_ATTR,width+2);
       }
       break;
     case KEY_DOWN:
       if (menu_select<(items-1))
       {
        menu_draw_cursor(x+1,y+1+menu_select,WIN_ATTR,width+2);
        menu_select++;
        menu_draw_cursor(x+1,y+1+menu_select,CURSOR_ATTR,width+2);
       }
       break;
     case KEY_PAGEUP:
     case KEY_HOME:
       menu_draw_cursor(x+1,y+1+menu_select,WIN_ATTR,width+2);
       menu_select=0;
       menu_draw_cursor(x+1,y+1+menu_select,CURSOR_ATTR,width+2);
       break;
     case KEY_PAGEDOWN:
     case KEY_END:
       menu_draw_cursor(x+1,y+1+menu_select,WIN_ATTR,width+2);
       menu_select=items-1;
       menu_draw_cursor(x+1,y+1+menu_select,CURSOR_ATTR,width+2);
       break;
     case KEY_ESC:
       go2=GO_EXIT;
       break;
     case KEY_CAPSLOCK:
       menu_swlng();
       go2=GO_REDRAW;
       break;
     case KEY_SCROLLLOCK:
       toggle_vga();
       ps2k_setsysled();
       break;
     case KEY_F1:
       menu_help();
       go2=GO_REDRAW;
    }
   }
   else
   {
    if ( (pBkHndl) && (check_timeout_ms(&to)) )
    {
     pBkHndl(1);
     set_timeout_ms(&to,BkTO);
    }
   }
  }while (go2==GO_READKEY);

 }while (go2==GO_REDRAW);

}

//-----------------------------------------------------------------------------
