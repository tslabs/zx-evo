#include "gcWin.h"
#include "items.h"

void cb_doKeyMap();

extern u8 padModeRaw;
extern u8 joy1Type;
extern u8 joy2Type;
extern u8 joyMappingType;

extern u8 joyNames[2][16][16];
extern u8 joyMap[2][16];
extern u8 joyAutofire[2][16];
extern u8 JoyAutofireRaw[2][2];

enum {
    buttonLabelsX = 3,
    buttonKeyX    = 11,
    buttonLabelsY = 2+6,
    autofireX     = 22,


    joy1X         = 3,
    joy2X         = 33
};

#define arrayof(a) (sizeof(a) / sizeof(a[0]))

const GC_WINDOW_t wndPress = 
{
/*id*/          1,
/*type*/        GC_WND_INFO,
/*xy*/          (SCREEN_WIDTH-50)/2,(SCREEN_HIGHT-5)/2,
/*wh*/          50,5,
/*attr*/        (WIN_COL_BRIGHT_WHITE<<4) | WIN_COL_BLACK,
/*frame_type*/  GC_FRM_SINGLE | GC_FRM_NOLOGO | GC_FRM_NOHEADER,
/*frame_attr*/  (WIN_COL_BRIGHT_WHITE<<4) | WIN_COL_BLACK,
/*header_txt*/  INK_BLACK
                "",
/*window_txt*/  "\n"MARK_CENTER"Press keys on keyboard to map...",
/*menu_ptr*/    0
};

const GC_WINDOW_t wndEepromClear = 
{
/*id*/          1,
/*type*/        GC_WND_INFO,
/*xy*/          (SCREEN_WIDTH-50)/2,(SCREEN_HIGHT-3)/2,
/*wh*/          50,3,
/*attr*/        (WIN_COL_BRIGHT_WHITE<<4) | WIN_COL_BLACK,
/*frame_type*/  GC_FRM_SINGLE | GC_FRM_NOLOGO | GC_FRM_NOHEADER,
/*frame_attr*/  (WIN_COL_BRIGHT_WHITE<<4) | WIN_COL_BLACK,
/*header_txt*/  INK_BLACK
                "",
/*window_txt*/  MARK_CENTER"Clearing EEPROM, please wait...",
/*menu_ptr*/    0
};

const GC_WINDOW_t wndJoyConfigLoad = 
{
/*id*/          1,
/*type*/        GC_WND_INFO,
/*xy*/          (SCREEN_WIDTH-50)/2,(SCREEN_HIGHT-3)/2,
/*wh*/          50,3,
/*attr*/        (WIN_COL_BRIGHT_WHITE<<4) | WIN_COL_BLACK,
/*frame_type*/  GC_FRM_SINGLE | GC_FRM_NOLOGO | GC_FRM_NOHEADER,
/*frame_attr*/  (WIN_COL_BRIGHT_WHITE<<4) | WIN_COL_BLACK,
/*header_txt*/  INK_BLACK
                "",
/*window_txt*/  MARK_CENTER"Loading Joystick Configuration, please wait...",
/*menu_ptr*/    0
};

const GC_WINDOW_t wndJoyConfigSave = 
{
/*id*/          1,
/*type*/        GC_WND_INFO,
/*xy*/          (SCREEN_WIDTH-50)/2,(SCREEN_HIGHT-3)/2,
/*wh*/          50,3,
/*attr*/        (WIN_COL_BRIGHT_WHITE<<4) | WIN_COL_BLACK,
/*frame_type*/  GC_FRM_SINGLE | GC_FRM_NOLOGO | GC_FRM_NOHEADER,
/*frame_attr*/  (WIN_COL_BRIGHT_WHITE<<4) | WIN_COL_BLACK,
/*header_txt*/  INK_BLACK
                "",
/*window_txt*/  MARK_CENTER"Saving Joystick Configuration, please wait...",
/*menu_ptr*/    0
};


const GC_DITEM_t itmJoyMapTypeBox = {
/*type*/        DI_GROUPBOX,
/*id*/          0,
/*xy*/          1,1+21,
/*wh*/          65,3,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        "Joystick 1/2 mapping type",
/*handler*/     0
};

const GC_DITEM_t itmJoyMapTypeKempKeyb = {
/*type*/        DI_RADIOBUTTON,
/*id*/          0,
/*xy*/          3,1+22,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,1},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      'e',
/*select*/      0,
/*var*/         &joyMappingType,
/*name*/        "K"INK_BRIGHT_BLUE"e"INK_BLACK"mpston/Keyboard",
/*handler*/     0
};

const GC_DITEM_t itmJoyMapTypeKeybKemp = {
/*type*/        DI_RADIOBUTTON,
/*id*/          0,
/*xy*/          3+20,1+22,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      'y',
/*select*/      1,
/*var*/         &joyMappingType,
/*name*/        "Ke"INK_BRIGHT_BLUE"y"INK_BLACK"board/Kempston",
/*handler*/     0
};

const GC_DITEM_t itmJoyMapTypeKeybKeyb = {
/*type*/        DI_RADIOBUTTON,
/*id*/          0,
/*xy*/          3+20+20,1+22,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      'b',
/*select*/      2,
/*var*/         &joyMappingType,
/*name*/        "Key"INK_BRIGHT_BLUE"b"INK_BLACK"oard/Keyboard",
/*handler*/     0
};

const GC_DITEM_t itmJoy1Box = {
/*type*/        DI_GROUPBOX,
/*id*/          0,
/*xy*/          1,1,
/*wh*/          32,21,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      '1',
/*select*/      0,
/*var*/         0,
/*name*/        "Joystick "INK_BRIGHT_BLUE"1",
/*handler*/     0
};

const GC_DITEM_t itmJoy1TypeKemp = {
/*type*/        DI_RADIOBUTTON,
/*id*/          0,
/*xy*/          3,3,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,1},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      'k',
/*select*/      0,
/*var*/         &joy1Type,
/*name*/        INK_BRIGHT_BLUE"K"INK_BLACK"empston 5-bit",
/*handler*/     0
};


const GC_DITEM_t itmJoy1TypeSega = {
/*type*/        DI_RADIOBUTTON,
/*id*/          0,
/*xy*/          3,4,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      's',
/*select*/      2,
/*var*/         &joy1Type,
/*name*/        INK_BRIGHT_BLUE"S"INK_BLACK"ega Mega Drive 3-button",
/*handler*/     0
};

const GC_DITEM_t itmJoy2Box = {
/*type*/        DI_GROUPBOX,
/*id*/          0,
/*xy*/          1+33,1,
/*wh*/          32,21,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      '2',
/*select*/      0,
/*var*/         0,
/*name*/        "Joystick "INK_BRIGHT_BLUE"2",
/*handler*/     0
};

const GC_DITEM_t itmJoy2TypeKemp = {
/*type*/        DI_RADIOBUTTON,
/*id*/          0,
/*xy*/          3+33,3,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,1},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      'n',
/*select*/      0,
/*var*/         &joy2Type,
/*name*/        INK_BRIGHT_BLUE"N"INK_BLACK"one",
/*handler*/     0
};

const GC_DITEM_t itmJoy2TypeSega = {
/*type*/        DI_RADIOBUTTON,
/*id*/          0,
/*xy*/          3+33,4,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      'm',
/*select*/      2,
/*var*/         &joy2Type,
/*name*/        "Sega "INK_BRIGHT_BLUE"M"INK_BLACK"ega Drive 3-button",
/*handler*/     0
};


const GC_DITEM_t itmJoy1MappingLabel = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          3,6,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        "Button      Key     Autofire",
/*handler*/     0
};

const GC_DITEM_t itmJoy2MappingLabel = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          3+33,6,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/         "Button      Key     Autofire",
/*handler*/     0
};

const GC_DITEM_t itmBtnOK = {
/*type*/        DI_BUTTON,
/*id*/          BUTTON_OK,
/*xy*/          15,25,
/*wh*/          40,0,
/*flags*/       {0,0,0,0,0,0,0,1},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      'o',
/*select*/      0,
/*var*/         0,
/*name*/        PAPER_BRIGHT_WHITE INK_BRIGHT_RED"O"INK_BLACK"K",
/*handler*/     0,
};

GC_DIALOG_t wndMainDlg =
{
/*flag*/                {0,0,0,0,0,0,0,0},
/*current*/             0,
/*all_count*/           arrayof(dlgInfoItemsList) + arrayof(dlgInfoItemsListInactive),
/*act_count*/           arrayof(dlgInfoItemsList),
/*cur_attr*/            (WIN_COL_BLUE<<4) | WIN_COL_BRIGHT_YELLOW,
/*box_attr*/            WIN_COL_BLACK,
/*btn_focus_attr*/      (WIN_COL_BRIGHT_YELLOW<<4) | WIN_COL_BLUE,
/*btn_unfocus_attr*/    (WIN_COL_BRIGHT_WHITE<<4) | WIN_COL_BLACK,
/*lbox_focus_attr*/     (WIN_COL_BLUE<<4) | WIN_COL_BRIGHT_WHITE,
/*lbox_unfocus_attr*/   (WIN_COL_BLUE<<4) | WIN_COL_WHITE,
/*items*/               dlgInfoItemsList
};

const GC_WINDOW_t wndBackdrop = 
{
/*id*/          1,
/*type*/        GC_WND_NOMENU,
/*xy*/          0,0,
/*wh*/          80,30,
/*attr*/        (WIN_COL_BLUE<<4) | WIN_COL_BLUE,
/*frame_type*/  GC_FRM_NONE | GC_FRM_NOLOGO,
/*frame_attr*/  (WIN_COL_BLUE<<4) | WIN_COL_BLUE,
/*header_txt*/  INK_BLACK
                "",
/*window_txt*/  0,
/*menu_ptr*/    0
};

const GC_WINDOW_t wndMain =
{
/*id*/          0,
/*type*/        GC_WND_DIALOG,
/*xy*/          (SCREEN_WIDTH-70)/2,(SCREEN_HIGHT-29)/2,
/*wh*/          70,29,
/*attr*/        (WIN_COL_WHITE<<4) | WIN_COL_BLACK,
/*frame_type*/  GC_FRM_SINGLE,
/*frame_attr*/  (WIN_COL_WHITE<<4) | WIN_COL_BRIGHT_WHITE,
/*header_txt*/  INK_BLACK
                "ZX Evolution Joystick Setup",
/*window_txt*/  0,
/*menu_ptr*/    (u16)&wndMainDlg,
};


const GC_DITEM_t *dlgInfoItemsList[] = {
    &itmJoy1TypeKemp,
    &itmJoy1TypeSega,

    &itmJoy1KeyRight,
    &itmJoy1KeyLeft,
    &itmJoy1KeyDown,
    &itmJoy1KeyUp,
    &itmJoy1KeyB,
    &itmJoy1KeyC,
    &itmJoy1KeyA,
    &itmJoy1KeyStart,

    &itmJoy1AutofireRight,
    &itmJoy1AutofireLeft,
    &itmJoy1AutofireDown,
    &itmJoy1AutofireUp,
    &itmJoy1AutofireB,
    &itmJoy1AutofireC,
    &itmJoy1AutofireA,
    &itmJoy1AutofireStart,

    &itmJoy2TypeKemp,
    &itmJoy2TypeSega,

    &itmJoy2KeyRight,
    &itmJoy2KeyLeft,
    &itmJoy2KeyDown,
    &itmJoy2KeyUp,
    &itmJoy2KeyB,
    &itmJoy2KeyC,
    &itmJoy2KeyA,
    &itmJoy2KeyStart,

    &itmJoy2AutofireRight,
    &itmJoy2AutofireLeft,
    &itmJoy2AutofireDown,
    &itmJoy2AutofireUp,
    &itmJoy2AutofireB,
    &itmJoy2AutofireC,
    &itmJoy2AutofireA,
    &itmJoy2AutofireStart,

    &itmJoyMapTypeKempKeyb,
    &itmJoyMapTypeKeybKemp,
    &itmJoyMapTypeKeybKeyb,

    &itmBtnOK,
};

const GC_DITEM_t *dlgInfoItemsListInactive[] = {
    &itmJoy1Box,
    &itmJoy2Box,
    &itmJoyMapTypeBox,

    &itmJoy1MappingLabel,
    &itmJoy2MappingLabel,


    &itmJoy1LabelRight,
    &itmJoy1LabelLeft,
    &itmJoy1LabelDown,
    &itmJoy1LabelUp,
    &itmJoy1LabelB,
    &itmJoy1LabelC,
    &itmJoy1LabelA,
    &itmJoy1LabelStart,

    &itmJoy2LabelRight,
    &itmJoy2LabelLeft,
    &itmJoy2LabelDown,
    &itmJoy2LabelUp,
    &itmJoy2LabelB,
    &itmJoy2LabelC,
    &itmJoy2LabelA,
    &itmJoy2LabelStart,
    &itmJoy2LabelStart
};

const GC_DITEM_t itmJoy1LabelRight = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          buttonLabelsX,buttonLabelsY+0,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,1},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        MARK_LINK"\x0",
/*handler*/     0
};
const GC_DITEM_t itmJoy1LabelLeft = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          buttonLabelsX,buttonLabelsY+1,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        MARK_LINK"\x1",
/*handler*/     0
};
const GC_DITEM_t itmJoy1LabelDown = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          buttonLabelsX,buttonLabelsY+2,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        MARK_LINK"\x2",
/*handler*/     0
};
const GC_DITEM_t itmJoy1LabelUp = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          buttonLabelsX,buttonLabelsY+3,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        MARK_LINK"\x3",
/*handler*/     0
};
const GC_DITEM_t itmJoy1LabelB = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          buttonLabelsX,buttonLabelsY+4,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        MARK_LINK"\x4",
/*handler*/     0
};
const GC_DITEM_t itmJoy1LabelC = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          buttonLabelsX,buttonLabelsY+5,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        MARK_LINK"\x5",
/*handler*/     0
};
const GC_DITEM_t itmJoy1LabelA = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          buttonLabelsX,buttonLabelsY+6,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        MARK_LINK"\x6",
/*handler*/     0
};
const GC_DITEM_t itmJoy1LabelStart = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          buttonLabelsX,buttonLabelsY+7,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        MARK_LINK"\x7",
/*handler*/     0
};

// --------------------------
// KEY LABELS

const GC_DITEM_t itmJoy1KeyRight = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          buttonKeyX,buttonLabelsY+0,
/*wh*/          autofireX-buttonKeyX,0,
/*flags*/       {0,0,0,0,0,0,0,1},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        &joyNames[0][0][0],
/*handler*/     cb_doKeyMap,
};
const GC_DITEM_t itmJoy1KeyLeft = {
/*type*/        DI_TEXT,
/*id*/          1,
/*xy*/          buttonKeyX,buttonLabelsY+1,
/*wh*/          autofireX-buttonKeyX,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        &joyNames[0][1][0],
/*handler*/     cb_doKeyMap
};
const GC_DITEM_t itmJoy1KeyDown = {
/*type*/        DI_TEXT,
/*id*/          2,
/*xy*/          buttonKeyX,buttonLabelsY+2,
/*wh*/          autofireX-buttonKeyX,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        &joyNames[0][2][0],
/*handler*/     cb_doKeyMap
};
const GC_DITEM_t itmJoy1KeyUp = {
/*type*/        DI_TEXT,
/*id*/          3,
/*xy*/          buttonKeyX,buttonLabelsY+3,
/*wh*/          autofireX-buttonKeyX,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        &joyNames[0][3][0],
/*handler*/     cb_doKeyMap
};
const GC_DITEM_t itmJoy1KeyB = {
/*type*/        DI_TEXT,
/*id*/          4,
/*xy*/          buttonKeyX,buttonLabelsY+4,
/*wh*/          autofireX-buttonKeyX,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        &joyNames[0][4][0],
/*handler*/     cb_doKeyMap
};
const GC_DITEM_t itmJoy1KeyC = {
/*type*/        DI_TEXT,
/*id*/          5,
/*xy*/          buttonKeyX,buttonLabelsY+5,
/*wh*/          autofireX-buttonKeyX,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        &joyNames[0][5][0],
/*handler*/     cb_doKeyMap
};
const GC_DITEM_t itmJoy1KeyA = {
/*type*/        DI_TEXT,
/*id*/          6,
/*xy*/          buttonKeyX,buttonLabelsY+6,
/*wh*/          autofireX-buttonKeyX,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        &joyNames[0][6][0],
/*handler*/     cb_doKeyMap
};
const GC_DITEM_t itmJoy1KeyStart = {
/*type*/        DI_TEXT,
/*id*/          7,
/*xy*/          buttonKeyX,buttonLabelsY+7,
/*wh*/          autofireX-buttonKeyX,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        &joyNames[0][7][0],
/*handler*/     cb_doKeyMap
};

// --------------------------
// AUTOFIRE CHECKBOXES

const GC_DITEM_t itmJoy1AutofireRight = {
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          4+autofireX,buttonLabelsY+0,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,1},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         &joyAutofire[0][0],
/*name*/        0,
/*handler*/     0
};
const GC_DITEM_t itmJoy1AutofireLeft = {
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          4+autofireX,buttonLabelsY+1,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         &joyAutofire[0][1],
/*name*/        0,
/*handler*/     0
};
const GC_DITEM_t itmJoy1AutofireDown = {
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          4+autofireX,buttonLabelsY+2,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         &joyAutofire[0][2],
/*name*/        0,
/*handler*/     0
};
const GC_DITEM_t itmJoy1AutofireUp = {
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          4+autofireX,buttonLabelsY+3,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         &joyAutofire[0][3],
/*name*/        0,
/*handler*/     0
};
const GC_DITEM_t itmJoy1AutofireB = {
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          4+autofireX,buttonLabelsY+4,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         &joyAutofire[0][4],
/*name*/        0,
/*handler*/     0
};
const GC_DITEM_t itmJoy1AutofireC = {
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          4+autofireX,buttonLabelsY+5,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         &joyAutofire[0][5],
/*name*/        0,
/*handler*/     0
};
const GC_DITEM_t itmJoy1AutofireA = {
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          4+autofireX,buttonLabelsY+6,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         &joyAutofire[0][6],
/*name*/        0,
/*handler*/     0
};
const GC_DITEM_t itmJoy1AutofireStart = {
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          4+autofireX,buttonLabelsY+7,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         &joyAutofire[0][7],
/*name*/        0,
/*handler*/     0
};








// --------------------------
// JOYSTICK 2


const GC_DITEM_t itmJoy2LabelRight = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          buttonLabelsX+33,buttonLabelsY+0,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        MARK_LINK"\x0",
/*handler*/     0
};
const GC_DITEM_t itmJoy2LabelLeft = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          buttonLabelsX+33,buttonLabelsY+1,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        MARK_LINK"\x1",
/*handler*/     0
};
const GC_DITEM_t itmJoy2LabelDown = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          buttonLabelsX+33,buttonLabelsY+2,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        MARK_LINK"\x2",
/*handler*/     0
};
const GC_DITEM_t itmJoy2LabelUp = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          buttonLabelsX+33,buttonLabelsY+3,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        MARK_LINK"\x3",
/*handler*/     0
};
const GC_DITEM_t itmJoy2LabelB = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          buttonLabelsX+33,buttonLabelsY+4,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        MARK_LINK"\x4",
/*handler*/     0
};
const GC_DITEM_t itmJoy2LabelC = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          buttonLabelsX+33,buttonLabelsY+5,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        MARK_LINK"\x5",
/*handler*/     0
};
const GC_DITEM_t itmJoy2LabelA = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          buttonLabelsX+33,buttonLabelsY+6,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        MARK_LINK"\x6",
/*handler*/     0
};
const GC_DITEM_t itmJoy2LabelStart = {
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          buttonLabelsX+33,buttonLabelsY+7,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        MARK_LINK"\x7",
/*handler*/     0
};


// --------------------------
// KEY LABELS

const GC_DITEM_t itmJoy2KeyRight = {
/*type*/        DI_TEXT,
/*id*/          0+16,
/*xy*/          buttonKeyX+33,buttonLabelsY+0,
/*wh*/          autofireX-buttonKeyX,0,
/*flags*/       {0,0,0,0,0,0,0,1},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        &joyNames[1][0][0],
/*handler*/     cb_doKeyMap
};
const GC_DITEM_t itmJoy2KeyLeft = {
/*type*/        DI_TEXT,
/*id*/          1+16,
/*xy*/          buttonKeyX+33,buttonLabelsY+1,
/*wh*/          autofireX-buttonKeyX,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        &joyNames[1][1][0],
/*handler*/     cb_doKeyMap
};
const GC_DITEM_t itmJoy2KeyDown = {
/*type*/        DI_TEXT,
/*id*/          2+16,
/*xy*/          buttonKeyX+33,buttonLabelsY+2,
/*wh*/          autofireX-buttonKeyX,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        &joyNames[1][2][0],
/*handler*/     cb_doKeyMap
};
const GC_DITEM_t itmJoy2KeyUp = {
/*type*/        DI_TEXT,
/*id*/          3+16,
/*xy*/          buttonKeyX+33,buttonLabelsY+3,
/*wh*/          autofireX-buttonKeyX,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        &joyNames[1][3][0],
/*handler*/     cb_doKeyMap
};
const GC_DITEM_t itmJoy2KeyB = {
/*type*/        DI_TEXT,
/*id*/          4+16,
/*xy*/          buttonKeyX+33,buttonLabelsY+4,
/*wh*/          autofireX-buttonKeyX,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        &joyNames[1][4][0],
/*handler*/     cb_doKeyMap
};
const GC_DITEM_t itmJoy2KeyC = {
/*type*/        DI_TEXT,
/*id*/          5+16,
/*xy*/          buttonKeyX+33,buttonLabelsY+5,
/*wh*/          autofireX-buttonKeyX,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        &joyNames[1][5][0],
/*handler*/     cb_doKeyMap
};
const GC_DITEM_t itmJoy2KeyA = {
/*type*/        DI_TEXT,
/*id*/          6+16,
/*xy*/          buttonKeyX+33,buttonLabelsY+6,
/*wh*/          autofireX-buttonKeyX,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        &joyNames[1][6][0],
/*handler*/     cb_doKeyMap
};
const GC_DITEM_t itmJoy2KeyStart = {
/*type*/        DI_TEXT,
/*id*/          7+16,
/*xy*/          buttonKeyX+33,buttonLabelsY+7,
/*wh*/          autofireX-buttonKeyX,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        &joyNames[1][7][0],
/*handler*/     cb_doKeyMap
};

// --------------------------
// AUTOFIRE CHECKBOXES

const GC_DITEM_t itmJoy2AutofireRight = {
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          4+33+autofireX,buttonLabelsY+0,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,1},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         &joyAutofire[1][0],
/*name*/        0,
/*handler*/     0
};
const GC_DITEM_t itmJoy2AutofireLeft = {
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          4+33+autofireX,buttonLabelsY+1,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         &joyAutofire[1][1],
/*name*/        0,
/*handler*/     0
};
const GC_DITEM_t itmJoy2AutofireDown = {
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          4+33+autofireX,buttonLabelsY+2,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         &joyAutofire[1][2],
/*name*/        0,
/*handler*/     0
};
const GC_DITEM_t itmJoy2AutofireUp = {
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          4+33+autofireX,buttonLabelsY+3,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         &joyAutofire[1][3],
/*name*/        0,
/*handler*/     0
};
const GC_DITEM_t itmJoy2AutofireB = {
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          4+33+autofireX,buttonLabelsY+4,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         &joyAutofire[1][4],
/*name*/        0,
/*handler*/     0
};
const GC_DITEM_t itmJoy2AutofireC = {
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          4+33+autofireX,buttonLabelsY+5,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         &joyAutofire[1][5],
/*name*/        0,
/*handler*/     0
};
const GC_DITEM_t itmJoy2AutofireA = {
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          4+33+autofireX,buttonLabelsY+6,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         &joyAutofire[1][6],
/*name*/        0,
/*handler*/     0
};
const GC_DITEM_t itmJoy2AutofireStart = {
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          4+33+autofireX,buttonLabelsY+7,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         &joyAutofire[1][7],
/*name*/        0,
/*handler*/     0
};


// ---------------------------------------------------------
// ----------------- SYSTEM CONFIG DIALOG ------------------ 




// ---------------------------------------------------------
// --------------------- MAIN MENU -------------------------

const GC_WINDOW_t wndSVMnu =
{
/*id*/          0,
/*type*/        GC_WND_SVMENU,
/*xy*/          (SCREEN_WIDTH-50)/2,(SCREEN_HIGHT-10)/2,
/*wh*/          50,10,
/*attr*/        (WIN_COL_WHITE<<4) | WIN_COL_BRIGHT_WHITE,
/*frame_type*/  GC_FRM_SINGLE,
/*frame_attr*/  (WIN_COL_WHITE<<4) | WIN_COL_BRIGHT_WHITE,
/*header_txt*/  INK_BLACK"ZX-Evolution Configuration Utility",
/*window_txt*/  0,
/*menu_ptr*/    (u16)&svmTest
};
GC_SVMENU_t svmTest =
{
/*flags*/       {0,0,0,0,0,0,0,0},
/*attr*/        (WIN_COL_BRIGHT_BLUE<<4) | WIN_COL_BRIGHT_YELLOW,
/*margin*/      1,
/*cur_pos*/     0,
/*win_pos*/     0,
/*win_cnt*/     arrayof(svm_lines),
/*all_cnt*/     arrayof(svm_lines),
/*cb_cursor*/   0,
/*cb_keys*/     0,
/*cb_cross*/    0,
/*lines*/       svm_lines,
/*opt_list*/    0
};


GC_SVM_LINE_t svm_line[] = {
    {GC_SVMT_TEXT, (const char*)MARK_CENTER"System Configuration"},
    {GC_SVMT_TEXT, (const char*)MARK_CENTER"Joystick Configuration"},
    {GC_SVMT_TEXT, (const char*)MARK_CENTER""},
    {GC_SVMT_TEXT, (const char*)MARK_CENTER"Save and Soft Reset"},
    {GC_SVMT_TEXT, (const char*)MARK_CENTER""},
    {GC_SVMT_TEXT, (const char*)MARK_CENTER"Service Menu"},
    {GC_SVMT_TEXT, (const char*)MARK_CENTER"About..."},
};

// not the best design, honestly
const GC_SVM_LINE_t* svm_lines[] =
{
    &svm_line[0],
    &svm_line[1],
    &svm_line[2],
    &svm_line[3],
    &svm_line[4],
    &svm_line[5],
    &svm_line[6],
};

// ---------------------------------------------------------
// --------------------- SERVICE MENU ----------------------

const GC_WINDOW_t wndSVMService =
{
/*id*/          0,
/*type*/        GC_WND_SVMENU,
/*xy*/          (SCREEN_WIDTH-50)/2,(SCREEN_HIGHT-10)/2,
/*wh*/          50,10,
/*attr*/        (WIN_COL_WHITE<<4) | WIN_COL_BRIGHT_WHITE,
/*frame_type*/  GC_FRM_SINGLE,
/*frame_attr*/  (WIN_COL_WHITE<<4) | WIN_COL_BRIGHT_WHITE,
/*header_txt*/  INK_BLACK"Service",
/*window_txt*/  0,
/*menu_ptr*/    (u16)&svmService
};
GC_SVMENU_t svmService =
{
/*flags*/       {0,0,0,0,0,0,0,0},
/*attr*/        (WIN_COL_BRIGHT_BLUE<<4) | WIN_COL_BRIGHT_YELLOW,
/*margin*/      1,
/*cur_pos*/     svmService_MainMenu,
/*win_pos*/     0,
/*win_cnt*/     arrayof(svmServiceLines),
/*all_cnt*/     arrayof(svmServiceLines),
/*cb_cursor*/   0,
/*cb_keys*/     0,
/*cb_cross*/    0,
/*lines*/       svmServiceLines,
/*opt_list*/    0
};

GC_SVM_LINE_t svmServiceLine[] = {
    {GC_SVMT_TEXT, (const char*)MARK_CENTER"Reset CMOS"},
    {GC_SVMT_TEXT, (const char*)MARK_CENTER"Reset EEPROM"},
    {GC_SVMT_TEXT, (const char*)MARK_CENTER""},
    {GC_SVMT_TEXT, (const char*)MARK_CENTER"Hard Reset"},
    {GC_SVMT_TEXT, (const char*)MARK_CENTER"Reset and Flash from SD Sard"},
    {GC_SVMT_TEXT, (const char*)MARK_CENTER""},
    {GC_SVMT_TEXT, (const char*)MARK_CENTER"Back to Main Menu"},
};

// not the best design, honestly
const GC_SVM_LINE_t* svmServiceLines[] =
{
    &svmServiceLine[0],
    &svmServiceLine[1],
    &svmServiceLine[2],
    &svmServiceLine[3],
    &svmServiceLine[4],
    &svmServiceLine[5],
    &svmServiceLine[6],
};

