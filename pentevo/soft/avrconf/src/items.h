#pragma once
#include "gcWin.h"

//
#define KEY_SP	0
#define KEY_EN	1
#define KEY_P	2
#define KEY_0	3
#define KEY_1	4
#define KEY_Q	5
#define KEY_A	6
#define KEY_CS	7
//
#define KEY_SS	8
#define KEY_L	9
#define KEY_O  10
#define KEY_9  11
#define KEY_2  12
#define KEY_W  13
#define KEY_S  14
#define KEY_Z  15
//
#define KEY_M  16
#define KEY_K  17
#define KEY_I  18
#define KEY_8  19
#define KEY_3  20
#define KEY_E  21
#define KEY_D  22
#define KEY_X  23
//
#define KEY_N  24
#define KEY_J  25
#define KEY_U  26
#define KEY_7  27
#define KEY_4  28
#define KEY_R  29
#define KEY_F  30
#define KEY_C  31
//
#define KEY_B  32
#define KEY_H  33
#define KEY_Y  34
#define KEY_6  35
#define KEY_5  36
#define KEY_T  37
#define KEY_G  38
#define KEY_V  39
//


extern const GC_WINDOW_t wndPress;
extern const GC_WINDOW_t wndJoyConfigLoad;
extern const GC_WINDOW_t wndJoyConfigSave;
extern const GC_WINDOW_t wndEepromClear;

extern const GC_DITEM_t itmJoyMapTypeBox;
extern const GC_DITEM_t itmJoyMapTypeKempKeyb;
extern const GC_DITEM_t itmJoyMapTypeKeybKemp;
extern const GC_DITEM_t itmJoyMapTypeKeybKeyb;

extern const GC_DITEM_t itmJoy1Box;
extern const GC_DITEM_t itmJoy1TypeKemp;
extern const GC_DITEM_t itmJoy1TypeSega;

extern const GC_DITEM_t itmJoy2Box;
extern const GC_DITEM_t itmJoy2TypeKemp;
extern const GC_DITEM_t itmJoy2TypeSega;

extern const GC_DITEM_t itmJoy1MappingLabel;
extern const GC_DITEM_t itmJoy2MappingLabel;

extern const GC_DITEM_t itmBtnOK;
extern  GC_DIALOG_t wndMainDlg;
extern const GC_WINDOW_t wndBackdrop;
extern const GC_WINDOW_t wndMain;
extern const GC_DITEM_t *dlgInfoItemsList[];
extern const GC_DITEM_t *dlgInfoItemsListInactive[];


extern const GC_DITEM_t itmJoy1LabelRight;
extern const GC_DITEM_t itmJoy1LabelLeft;
extern const GC_DITEM_t itmJoy1LabelDown;
extern const GC_DITEM_t itmJoy1LabelUp;
extern const GC_DITEM_t itmJoy1LabelB;
extern const GC_DITEM_t itmJoy1LabelC;
extern const GC_DITEM_t itmJoy1LabelA;
extern const GC_DITEM_t itmJoy1LabelStart;

extern const GC_DITEM_t itmJoy1KeyRight;
extern const GC_DITEM_t itmJoy1KeyLeft;
extern const GC_DITEM_t itmJoy1KeyDown;
extern const GC_DITEM_t itmJoy1KeyUp;
extern const GC_DITEM_t itmJoy1KeyB;
extern const GC_DITEM_t itmJoy1KeyC;
extern const GC_DITEM_t itmJoy1KeyA;
extern const GC_DITEM_t itmJoy1KeyStart;

extern const GC_DITEM_t itmJoy1AutofireRight;
extern const GC_DITEM_t itmJoy1AutofireLeft;
extern const GC_DITEM_t itmJoy1AutofireDown;
extern const GC_DITEM_t itmJoy1AutofireUp;
extern const GC_DITEM_t itmJoy1AutofireB;
extern const GC_DITEM_t itmJoy1AutofireC;
extern const GC_DITEM_t itmJoy1AutofireA;
extern const GC_DITEM_t itmJoy1AutofireStart;


extern const GC_DITEM_t itmJoy2LabelRight;
extern const GC_DITEM_t itmJoy2LabelLeft;
extern const GC_DITEM_t itmJoy2LabelDown;
extern const GC_DITEM_t itmJoy2LabelUp;
extern const GC_DITEM_t itmJoy2LabelB;
extern const GC_DITEM_t itmJoy2LabelC;
extern const GC_DITEM_t itmJoy2LabelA;
extern const GC_DITEM_t itmJoy2LabelStart;

extern const GC_DITEM_t itmJoy2KeyRight;
extern const GC_DITEM_t itmJoy2KeyLeft;
extern const GC_DITEM_t itmJoy2KeyDown;
extern const GC_DITEM_t itmJoy2KeyUp;
extern const GC_DITEM_t itmJoy2KeyB;
extern const GC_DITEM_t itmJoy2KeyC;
extern const GC_DITEM_t itmJoy2KeyA;
extern const GC_DITEM_t itmJoy2KeyStart;

extern const GC_DITEM_t itmJoy2AutofireRight;
extern const GC_DITEM_t itmJoy2AutofireLeft;
extern const GC_DITEM_t itmJoy2AutofireDown;
extern const GC_DITEM_t itmJoy2AutofireUp;
extern const GC_DITEM_t itmJoy2AutofireB;
extern const GC_DITEM_t itmJoy2AutofireC;
extern const GC_DITEM_t itmJoy2AutofireA;
extern const GC_DITEM_t itmJoy2AutofireStart;

enum {
    svmMain_SystemConfig,
    svmMain_JoystickConfig,
    svmMain_Dummy,
    svmMain_SoftReset,
    svmMain_Dummy2,
    svmMain_Service,
    svmMain_About,
};


// service menu items
enum {
    svmService_ResetCMOS   = 0,
    svmService_ResetEEPROM,
    svmService_Dummy,
    svmService_HardReset,
    svmService_ResetFlash,
    svmService_Dummy2,
    svmService_MainMenu,
};

extern const GC_WINDOW_t wndSVMnu;
extern const GC_WINDOW_t wndSVMService;

