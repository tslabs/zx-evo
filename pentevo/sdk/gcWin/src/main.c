//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                       ::
//::               by dr_max^gc (c)2018-2019                 ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

#include <stdio.h>
#include "defs.h"
#include "tsio.h"
#include "keyboard.h"
#include "gcWin.h"
#include "dialogs.h"
#include "main.h"

#define EIHALT __asm__("ei\n halt\n");
#define DIHALT __asm__("di\n halt\n");

#define VPAGE   0x80
#define SPAGE   0x7F

// checkbox 1.1 var
u8  itmVarCB11 = 0;
// checkbox 1.2 var
u8  itmVarCB12 = 0;
// checkbox 2.1 var
u8  itmVarCB21 = 0;
// checkbox 2.2 var
u8  itmVarCB22 = 0;
// checkbox 3 var
u8  itmVarCB3 = 0;
// checkbox 4 var
u8 itmVarCB4 = 0;
// radiobutton var
u8 itmVarRB1 = 0;
// listbox var
u8 itmVarLBX11 = 0;

// numbers
u32 itmNUM1 = (u32)-1;
u16 itmNUM2 = (u16)-1;
u8 itmNUM3 = 255;

char c;
u8 rb, lb, cb11, cb12, cb21, cb22, cb3, cb4;
u8 pcx, pcx0, pcy, pca, pcst;   //for putchar

BTN_TYPE_t select;

void func_cb3()
{
    itmItemRB1.flags.DIF_GRAY = (itmVarCB3&1);
    itmItemRB2.flags.DIF_GRAY = (itmVarCB3&1);
    itmItemRB3.flags.DIF_GRAY = (itmVarCB3&1);
    gcPrintActiveDialog(&dlgTest);
}

void func_cb4()
{
    itmItemED1.flags.DIF_GRAY = (itmVarCB4&1);
    itmItemLBX1.flags.DIF_GRAY = (itmVarCB4&1);
    gcPrintActiveDialog(&dlgTest);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void main(void)
{
    //gcSetFontSym(0xF0, sym1);

    TS_PAGE3 = VPAGE;
    TS_VPAGE = VPAGE;

// set videomode
    TS_VCONFIG = TS_VID_320X240 | TS_VID_TEXT;
    TS_TSCONFIG = TS_TSU_SEN;

    gcSetPalette();

// setup linked messages
    gcSetLinkedMessage(msg_arr);

    gcPrintWindow(&wndMain);

    gcPrintWindow(&wndTest1);
//    gcWaitKey(KEY_ENTER);

    gcPrintWindow(&wndTest2);
//    gcWaitKey(KEY_ENTER);

    BORDER = 4;

    rb = cb11 = cb12 = cb21 = cb22 = cb3 = cb4 = lb = 0;

    itmVarRB1 = rb;
    itmVarCB11 = cb11;
    itmVarCB12 = cb12;
    itmVarCB21 = cb21;
    itmVarCB22 = cb22;
    itmVarCB3 = cb3;
    itmVarCB4 = cb4;
    itmVarLBX11 = lb;

    select = gcWindowHandler(&wndDialog);
    // if press OK button
    if (select == BUTTON_OK)
    {
            rb = itmVarRB1;
            cb11 = itmVarCB11;
            cb12 = itmVarCB12;
            cb21 = itmVarCB21;
            cb22 = itmVarCB22;
            cb3 = itmVarCB3;
            cb4 = itmVarCB4;
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
        if(c >= 0x20) putchar(c);
        if(pcx == 80) pcx = 0;
    }

}
