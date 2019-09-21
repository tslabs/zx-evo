//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                       ::
//::               by dr_max^gc (c)2018-2019                 ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

#include <stdio.h>
#include "defs.h"
#include "tsio.h"
#include "gcWin.h"
#include "dialogs.h"
#include "main.h"

#define DI __asm__("di\n");
#define EI __asm__("ei\n");
#define EIHALT __asm__("ei\n halt\n");
#define DIHALT __asm__("di\n halt\n");

#define VPAGE   0x80    // video page
#define SPAGE   0x82    // shadow screen page
#define GPAGE   0x88    // mouse sprite page

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
// svm_current_item var
u8 svm_current_item = 0;
u8 svm_progress_item = 0;

// numbers
u32 itmNUM1 = (u32)-1;
u16 itmNUM2 = (u16)-1;
u8 itmNUM3 = 255;

char c;

u8 rb, lb, cb11, cb12, cb21, cb22, cb3, cb4;
u8 pcx, pcx0, pcy, pca, pcst;   //for putchar

BTN_TYPE_t select;

void testMouse()
{
    u8 winx = 20;
    u8 winy = 10;

    wndMouseTest.x = winx;
    wndMouseTest.y = winy;

    gcPrintWindow(&wndMouseTest);
    gcGotoXY(winx, winy+16);
    gcPrintString(
                   INK_BRIGHT_BLUE
                   MARK_CENTER
                   "Press SPACE to exit"
                   );

    while(gcGetKey() != KEY_SPACE)
    {
        EIHALT
        gcGotoXY(winx+1, winy+2);

        gcPrintf(INK_BLUE"\tMouse X:"INK_MAGENTA"%04d\n"
                 INK_BLUE"\tMouse Y:"INK_MAGENTA"%04d\n\n", gcGetMouseX()-(320/2), gcGetMouseY()-(240/2));
        gcPrintf(INK_BLUE"\tMouse X:"INK_MAGENTA"%04u\n"
                 INK_BLUE"\tMouse Y:"INK_MAGENTA"%04u\n\n", gcGetMouseX(), gcGetMouseY());
        gcPrintf(INK_BLUE"\tMouse X:"INK_MAGENTA"%4x\n"
                 INK_BLUE"\tMouse Y:"INK_MAGENTA"%4x\n\n\n", gcGetMouseX(), gcGetMouseY());
        gcPrintf(INK_BLUE"32 bit number:"INK_MAGENTA"%12ld\n", a2d32s("-1234567890"));
        gcPrintf(INK_BLUE"32 bit number:"INK_MAGENTA"%012ld\n", a2d32s("-1234567890"));
    }

    gcCloseWindow();
}

// checkbox1 callback
void func_cb3()
{
    itmItemRB1.flags.DIF_GREY = (itmVarCB3&1);
    itmItemRB2.flags.DIF_GREY = (itmVarCB3&1);
    itmItemRB3.flags.DIF_GREY = (itmVarCB3&1);
    gcPrintDialogShownItems(&dlgTest, DI_RADIOBUTTON);
}

// checkbox1 callback
void func_cb4()
{
    itmItemED1.flags.DIF_GREY = (itmVarCB4&1);
    itmItemLBX1.flags.DIF_GREY = (itmVarCB4&1);
    gcPrintDialogShownItems(&dlgTest, DI_EDIT);
    gcPrintDialogShownItems(&dlgTest, DI_LISTBOX);
}

// SVM cursor callback
void cb_svmcur(GC_SVMENU_t *svm)
{
    gcSelectWindow(&wndSVMInfo);
    svm_current_item = svm->current;
    svm_progress_item = svm->current * 20;
    gcPrintDialog(&dlgSVMInfo);
    gcSelectWindow(&wndSVMHelp);
    gcPrintf("\nItem help "INK_BRIGHT_BLUE"%hd", svm_current_item);
    gcSelectWindow(&wndSVMnu);
}

// SVM keys callback
u8 cb_svmkeys(GC_SVMENU_t *svm, KEY_t key)
{
    gcSelectWindow(&wndSVMInfo);
    gcPrintf(INK_BRIGHT_WHITE"Key pressed:"INK_BLUE"0x%hx", key);
    gcSelectWindow(&wndSVMnu);
    if(svm->current == 5)
    {
        if(key == KEY_SPACE)
        {
            svm_opt5_var++;
            if(svm_opt5_var>3) svm_opt5_var=0;
            return SVM_CBKEY_RC_REDRAW;
        }
    }
    return SVM_CBKEY_RC_NONE;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void main(void)
{
    gcWindowsInit(VPAGE, SPAGE);
    gcMouseInit(GPAGE, 0);

    TS_VPAGE = VPAGE;
    TS_PAGE3 = SPAGE;

// set videomode
    TS_VCONFIG = TS_VID_320X240 | TS_VID_TEXT;
    TS_TSCONFIG = TS_TSU_SEN;

// set default palette
    gcSetPalette(NULL, 0);

// set border
    BORDER = 1;

// setup linked messages
    gcSetLinkedMessage(msg_arr);

    gcPrintWindow(&wndMain);

    gcPrintWindow(&wndTest1);

    gcPrintWindow(&wndTest2);

    gcDrawWindow(0, 9, 19, 52, 3, WIN_COL_WHITE<<4 | WIN_COL_BRIGHT_WHITE, GC_FRM_SINGLE | GC_FRM_NOHEADER, WIN_COL_WHITE<<4 | WIN_COL_BRIGHT_WHITE);

    for(u8 i=0; i<=100; i++)
    {
        for(u8 j=0; j<1; j++) EIHALT;
        gcProgressBar(10, 20, (WIN_COL_GREEN<<4) | WIN_COL_BRIGHT_WHITE, 50, i);
    }

// simple vertical menu test
    gcExecuteWindow(&wndSVMInfo);
    gcExecuteWindow(&wndSVMHelp);
    select = gcExecuteWindow(&wndSVMnu);
    gcCloseWindow();    // close wndSVMnu
    gcCloseWindow();    // close wndSVMHelp
    gcCloseWindow();    // close wndSVMInfo

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

    select = gcExecuteWindow(&wndDialog);
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
    // close dialog window
    gcCloseWindow();


    gcExecuteWindow(&wndInfo);
    // close info window
    gcCloseWindow();


    select = gcMessageBox(MB_RETRYABORTIGNORE, GC_FRM_SINGLE, "MessageBox",
                INK_BRIGHT_WHITE
                "Lorem ipsum dolor sit amet, consectetur\n"
                INK_GREEN
                "adipiscing elit, sed do eiusmod tempor\n"
                INK_BLUE
                "incididunt ut labore et dolore magna aliqua."
                 );
    // close messagebox
    gcCloseWindow();

    testMouse();

    gcWaitKey(KEY_ENTER);

    // close test window 2
    gcCloseWindow();
    gcWaitKey(KEY_ENTER);

    // close test window 1
    gcCloseWindow();

    pcx = 0; pcy = 0; pcst = 0;
    pca = (u8)(WIN_COL_BRIGHT_WHITE<<4) | WIN_COL_RED;

    while(1)
    {
        EIHALT
        c = gcGetKey();
        switch(c)
        {
        case KEY_UP:
            //gcScrollUpRect(1,1,78,28);
            gcScrollUpWindow(&wndMain);

            break;
        case KEY_DOWN:
            //gcScrollDownRect(1,1,78,28);
            gcScrollDownWindow(&wndMain);
            break;
        default:
            putsym(c);
            if(c == KEY_ENTER) putsym('\x0A');
        }
        if(pcx == 80) pcx = 0;
    }
}
