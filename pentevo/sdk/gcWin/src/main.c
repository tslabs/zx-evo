//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                       ::
//::                  by dr_max^gc (c)2018                   ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "tsio.h"
#include "keyboard.h"
#include "gcWin.h"
#include "main.h"

#define EIHALT __asm__("ei\n halt\n");
#define DIHALT __asm__("di\n halt\n");

#define VPAGE   0x80

// checkbox 1.1 var
u8  itmVarCB11 = 0;
// checkbox 1.2 var
u8  itmVarCB12 = 0;
// checkbox 2.1 var
u8  itmVarCB21 = 0;
// checkbox 2.2 var
u8  itmVarCB22 = 0;
// checkbox 3 var
u8  itmVarCB3 = 1;
// radiobutton var
u8 itmVarRB1 = 0;
// listbox var
u8 itmVarLBX11 = 0;

// numbers
u32 itmNUM1 = (u32)-1;
u16 itmNUM2 = (u16)-1;
u8 itmNUM3 = 255;

char c;
u8 rb, lb, cb11, cb12;
u8 pcx, pcx0, pcy, pca, pcst;   //for putchar

BTN_TYPE_t select;

void exec()
{
    while (1)
    {

    }
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void main(void)
{
    //gcSetFontSym(0xF0, sym1);

    TS_PAGE3 = VPAGE;
    TS_VPAGE = VPAGE;

// set videomode
    TS_VCONFIG = TS_VID_320X240 | TS_VID_TEXT;

// setup linked messages
    gcSetLinkedMessage(msg_arr);

//    while(gcGetKey() != KEY_ENTER);

    gcPrintWindow(&wndMain);
    gcPrintWindow(&wndTest1);
    gcPrintWindow(&wndTest2);

    BORDER = 4;

    rb = cb11 = cb12 = lb = 0;

    itmVarRB1 = rb;
    itmVarCB11 = cb11;
    itmVarCB12 = cb12;
    itmVarLBX11 = lb;

    select = gcWindowHandler(&wndDialog);
    // if press OK button
    if (select == BUTTON_OK)
    {
            rb = itmVarRB1;
            cb11 = itmVarCB11;
            cb12 = itmVarCB12;
            lb = itmVarLBX11;
    }

    gcWindowHandler(&wndInfo);

    select = gcMessageBox(MB_RETRYABORTIGNORE, GC_FRM_SINGLE, "MessageBox",
                INK_BRIGHT_WHITE
                "Lorem ipsum dolor sit amet, consectetur\r"
                INK_GREEN
                "adipiscing elit, sed do eiusmod tempor\r"
                INK_BLUE
                "incididunt ut labore et dolore magna aliqua."
                 );

    pcx = 0; pcy = 0; pcst = 0;
    pca = (u8)(WIN_COL_BRIGHT_WHITE<<4) | WIN_COL_RED;

    while(1)
    {
        EIHALT
        c = gcGetKey();
        if(c > 0x00) putchar(c);
        if(pcx == 80) pcx = 0;
    }

}
