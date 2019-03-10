//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                       ::
//::               by dr_max^gc (c)2018-2019                 ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

// SIMPLE VERTICAL MENU
const GC_SVMENU_t mnuSVM =
{
/*attr*/        (WIN_COL_BRIGHT_CYAN<<4) | WIN_COL_BLACK,
/*margin*/      1,
/*current*/     0,
/*count*/       8
};

const u8 txt_lorem[] =
"Lorem ipsum dolor sit amet, facer vocent an pri. Volumus iudicabit cum ei.\n"
"Ipsum quando feugiat te cum, et sea mundi principes assueverit. Eam no\n"
"lobortis quaerendum definitionem. Velit semper maluisset vix an, te eius\n"
"omittam cum. Tamquam lobortis tincidunt id pro, quo vocent aliquid ne.\n"
"Altera mucius qui te. Cum iriure oblique sadipscing ad. Placerat inimicus \n"
"similique pro an, duo at movet elaboraret accommodare, quo no adhuc denique.\n"
"Dicunt habemus in pri, reque viderer per no, nec et pertinax delicata. Natum \n"
"vivendo neglegentur sea ex, mel ex justo principes sadipscing. Vix no quando\n"
"consectetuer, tation dignissim quo at. An habeo omnesque vis, et laudem\n"
"lucilius est. Perfecto vulputate ex est. Ad aliquip ponderum insolens sit,\n"
"quis agam case ad nam.\n"
"Sit utinam feugait eleifend at, eam eu errem verear, ex eam officiis\n"
"perfecto ullamcorper.\n"
"Meis evertitur vulputate per ex, nibh possit sit at. Ex habemus rationibus\n"
"mediocritatem vis, mel essent eleifend no.\n"
"At omnis labitur duo. Et sale complectitur pri. Cu sea quidam reprimique.\n"
"Te sed convenire consequat. Est et equidem mandamus. Aperiam praesent \n"
"gloriatur an nam, vix veri epicuri inimicus ut.\n"
"Nulla appareat et sea, quodsi commune ocurreret mei ex. Quo et semper aeterno\n"
"veritus. His in suas unum meis. Ut vix velit hendrerit.\n";

// MAIN WINDOW
const GC_WINDOW_t wndMain =
{
/*id*/          1,
/*type*/        GC_WND_NOMENU,
/*xy*/          0,0,
/*wh*/          80,30,
/*attr*/        (WIN_COL_WHITE<<4) | WIN_COL_BRIGHT_WHITE,
/*frame_type*/  GC_FRM_SINGLE | GC_FRM_NOSHADOW | GC_FRM_NOLOGO,
/*frame_attr*/  (WIN_COL_WHITE<<4) | WIN_COL_BRIGHT_WHITE,
/*header_txt*/  INK_BLACK
                "Main Window",
/*window_txt*/  txt_lorem,
/*menu_ptr*/    0
};

GC_WINDOW_t wndMouseTest =
{
    /*id*/          0,
    /*type*/        GC_WND_NOMENU,
    /*xy*/          0,0,
    /*wh*/          30,12,
    /*attr*/        (WIN_COL_WHITE<<4) | WIN_COL_BRIGHT_WHITE,
    /*frame_type*/  GC_FRM_SINGLE,
    /*frame_attr*/  (WIN_COL_WHITE<<4) | WIN_COL_BRIGHT_WHITE,
    /*header_txt*/  INK_BLACK
                    "Mouse Test",
    /*window_txt*/  0,
    /*menu_ptr*/    0
};

const GC_WINDOW_t wndTest1 =
{
/*id*/          2,
/*type*/        GC_WND_NOMENU,
/*xy*/          8,5,
/*wh*/          40,10,
/*attr*/        (WIN_COL_BLUE<<4) | WIN_COL_BRIGHT_WHITE,
/*frame_type*/  GC_FRM_SINGLE,
/*frame_attr*/  (WIN_COL_BLUE<<4) | WIN_COL_BRIGHT_WHITE,
/*header_txt*/  " Window 1 ",
/*window_txt*/  "Test "
                INK_WHITE
                "string "
                INK_YELLOW
                "number "
                INK_RED
                "one\n"
                INK_BRIGHT_WHITE
                "\xD0\xD1 Radiobutton\n"
                "\xD2\xD3 Radiobutton\n"
                "\xD4\xD5 Checkbox\n"
                "\xD6\xD7 Checkbox\n"
                MARK_LINK"\x0\n"
                MARK_LINK"\x1\n"
                MARK_LINK"\x2",
/*menu_ptr*/    0
};

const GC_WINDOW_t wndTest2 =
{
/*id*/          3,
/*type*/        GC_WND_NOMENU,
/*xy*/          35,2,
/*wh*/          40,11,
/*attr*/        (WIN_COL_RED<<4) | WIN_COL_BRIGHT_WHITE,
/*frame_type*/  GC_FRM_SINGLE,
/*frame_attr*/  (WIN_COL_RED<<4) | WIN_COL_BRIGHT_YELLOW,
/*header_txt*/  " Window 2 ",
/*window_txt*/  "Test "
                INK_GREEN
                "string "
                INK_BRIGHT_GREEN
                "number "
                INK_BRIGHT_WHITE
                "two\n"
                PAPER_BLACK "  "
                PAPER_BLUE "  "
                PAPER_RED "  "
                PAPER_MAGENTA "  "
                PAPER_GREEN "  "
                PAPER_CYAN "  "
                PAPER_YELLOW "  "
                PAPER_WHITE "  "
                "\n"
                PAPER_BLACK "  "
                PAPER_BRIGHT_BLUE "  "
                PAPER_BRIGHT_RED "  "
                PAPER_BRIGHT_MAGENTA "  "
                PAPER_BRIGHT_GREEN "  "
                PAPER_BRIGHT_CYAN "  "
                PAPER_BRIGHT_YELLOW "  "
                PAPER_BRIGHT_WHITE "  "
                "\n"
                INK_BRIGHT_RED
                MARK_LINK"\x3 1\n"
                INK_BRIGHT_YELLOW
                MARK_LINK"\x4\n"
                INK_BRIGHT_MAGENTA
                MARK_LINK"\x5\n"
                "\xB9\xBA""Folder\n"
                "\xBB\xBC""Folder\n"
                "\xBD\xBE""File\n",
/*menu_ptr*/    0
};

const GC_WINDOW_t wndInfo =
{
/*id*/          0,
/*type*/        GC_WND_DIALOG,
/*xy*/          18,6,
/*wh*/          40,18,
/*attr*/        (WIN_COL_WHITE<<4) | WIN_COL_BLACK,
/*frame_type*/  GC_FRM_SINGLE,
/*frame_attr*/  (WIN_COL_WHITE<<4) | WIN_COL_BRIGHT_WHITE,
/*header_txt*/  INK_BLACK
                "Info",
/*window_txt*/  0,
/*menu_ptr*/    (u16)&dlgInfo
};

const GC_WINDOW_t wndSVMnu =
{
/*id*/          0,
/*type*/        GC_WND_SVMENU,
/*xy*/          20,8,
/*wh*/          28,18,
/*attr*/        (WIN_COL_WHITE<<4) | WIN_COL_BRIGHT_WHITE,
/*frame_type*/  GC_FRM_SINGLE,
/*frame_attr*/  (WIN_COL_WHITE<<4) | WIN_COL_BRIGHT_WHITE,
/*header_txt*/  INK_BLACK
                "SVMenu test",
/*window_txt*/  "\n"INK_BRIGHT_BLUE MARK_CENTER"Please select item...\n"
                "\n\n\n"
                MARK_CENTER"Item 1\n"
                MARK_CENTER"Item 2\n"
                MARK_CENTER"Item 3\n"
                MARK_CENTER"Item 4\n"
                MARK_CENTER"Item 5\n"
                MARK_CENTER"Item 6\n",
/*menu_ptr*/    (u16)&svmTest
};

GC_SVMENU_t svmTest =
{
/*attr*/        (WIN_COL_BRIGHT_BLUE<<4) | WIN_COL_BRIGHT_YELLOW,
/*margin*/      5,
/*current*/     0,
/*count*/       6
};

const GC_WINDOW_t wndDialog =
{
/*id*/          0,
/*type*/        GC_WND_DIALOG,
/*xy*/          10,4,
/*wh*/          55,24,
/*attr*/        (WIN_COL_CYAN<<4) | WIN_COL_BLACK,
/*frame_type*/  GC_FRM_SINGLE,
/*frame_attr*/  (WIN_COL_CYAN<<4) | WIN_COL_BRIGHT_WHITE,
/*header_txt*/  INK_BLACK
                "Configuration",
/*window_txt*/  0,
/*menu_ptr*/    (u16)&dlgTest
};

const GC_DITEM_t itmItemNMH1 =
{
/*type*/        DI_NUMBER,
/*id*/          0,
/*xy*/          1,12,
/*wh*/          10,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {DWORD,HEX,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         (u32)&itmNUM1,
/*name*/        "32b HEX Number:"INK_BRIGHT_CYAN,
/*handler*/     0
};

const GC_DITEM_t itmItemNMH2 =
{
/*type*/        DI_NUMBER,
/*id*/          0,
/*xy*/          1,13,
/*wh*/          10,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {WORD,HEX,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         (u16)&itmNUM2,
/*name*/        "16b HEX Number:"INK_BRIGHT_GREEN,
/*handler*/     0
};

const GC_DITEM_t itmItemNMH3 =
{
/*type*/        DI_NUMBER,
/*id*/          0,
/*xy*/          1,14,
/*wh*/          10,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {BYTE,HEX,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         (u8)&itmNUM3,
/*name*/        " 8b HEX Number:"INK_BLUE,
/*handler*/     0
};

const GC_DITEM_t itmItemNM1 =
{
/*type*/        DI_NUMBER,
/*id*/          0,
/*xy*/          28,12,
/*wh*/          10,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {DWORD,DEC,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         (u32)&itmNUM1,
/*name*/        "32b Number:"INK_BRIGHT_CYAN,
/*handler*/     0
};

const GC_DITEM_t itmItemNM2 =
{
/*type*/        DI_NUMBER,
/*id*/          0,
/*xy*/          28,13,
/*wh*/          10,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {WORD,DEC,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         (u16)&itmNUM2,
/*name*/        "16b Number:"INK_BRIGHT_GREEN,
/*handler*/     0
};

const GC_DITEM_t itmItemNM3 =
{
/*type*/        DI_NUMBER,
/*id*/          0,
/*xy*/          28,14,
/*wh*/          10,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {BYTE,DEC,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         (u8)&itmNUM3,
/*name*/        " 8b Number:"INK_BLUE,
/*handler*/     0
};

const GC_DITEM_t itmItemG1 =
{
/*type*/        DI_GROUPBOX,
/*id*/          0,
/*xy*/          0,1,
/*wh*/          26,4,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        INK_BRIGHT_BLUE"Group1",
/*handler*/    0
};

const GC_DITEM_t itmItemG2 =
{
/*type*/        DI_GROUPBOX,
/*id*/          0,
/*xy*/          27,1,
/*wh*/          26,4,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        INK_BRIGHT_BLUE MARK_RIGHT"Group2",
/*handler*/     0
};

const GC_DITEM_t itmItemG3 =
{
/*type*/        DI_GROUPBOX,
/*id*/          0,
/*xy*/          0,7,
/*wh*/          26,5,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        INK_BRIGHT_BLUE MARK_CENTER"Group3",
/*handler*/     0
};

const GC_DITEM_t itmItemCB11 =
{
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          1,2,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,1},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         (u8)&itmVarCB11,
/*name*/        MARK_LINK"\x0""1.1",
/*handler*/     0
};

const GC_DITEM_t itmItemCB12 =
{
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          1,3,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         (u8)&itmVarCB12,
/*name*/        MARK_LINK"\x0""1.2",
/*handler*/     0
};

const GC_DITEM_t itmItemCB3 =
{
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          1,5,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         (u8)&itmVarCB3,
/*name*/        "Enable radiobuttons",
/*handler*/     func_cb3
};

const GC_DITEM_t itmItemCB21 =
{
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          28,2,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,1},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         (u8)&itmVarCB21,
/*name*/        MARK_LINK"\x0""2.1",
/*handler*/     0
};

const GC_DITEM_t itmItemCB22 =
{
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          28,3,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         (u8)&itmVarCB22,
/*name*/        MARK_LINK"\x0""2.2",
/*handler*/     0
};

GC_DITEM_t itmItemCB4 =
{
/*type*/        DI_CHECKBOX,
/*id*/          0,
/*xy*/          28,5,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         (u8)&itmVarCB4,
/*name*/        "Enable edit",
/*handler*/     func_cb4
};

GC_DITEM_t itmItemRB1 =
{
/*type*/        DI_RADIOBUTTON,
/*id*/          0,
/*xy*/          1,8,
/*wh*/          0,0,
/*flags*/       {1,0,0,0,0,0,0,1},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         (u8)&itmVarRB1,
/*name*/        MARK_LINK"\x1""1.1",
/*handler*/     0
};

GC_DITEM_t itmItemRB2 =
{
/*type*/        DI_RADIOBUTTON,
/*id*/          0,
/*xy*/          1,9,
/*wh*/          0,0,
/*flags*/       {1,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      1,
/*var*/         (u8)&itmVarRB1,
/*name*/        MARK_LINK"\x1""1.2",
/*handler*/     0
};

GC_DITEM_t itmItemRB3 =
{
/*type*/        DI_RADIOBUTTON,
/*id*/          0,
/*xy*/          1,10,
/*wh*/          0,0,
/*flags*/       {1,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      2,
/*var*/         (u8)&itmVarRB1,
/*name*/        MARK_LINK"\x1""1.3",
/*handler*/     0
};

GC_DITEM_t itmItemLBX1 =
{
/*type*/        DI_LISTBOX,
/*id*/          0,
/*xy*/          28,8,
/*wh*/          18,0,
/*flags*/       {1,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      4, //items count
/*var*/         (u8)&itmVarLBX11,
/*name*/        (u16)&listbox,
/*handler*/     0
};

const GC_DITEM_t itmItemTX1 =
{
/*type*/        DI_TEXT,
/*id*/          0,
/*xy*/          28,5,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        0,
/*handler*/     0
};

GC_DITEM_t itmItemED1 =
{
/*type*/        DI_EDIT,
/*id*/          0,
/*xy*/          38,6,
/*wh*/          10,0,
/*flags*/       {1,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        "Edit field",
/*handler*/     0
};

const GC_DITEM_t itmItemHD1 =
{
/*type*/        DI_HDIV,
/*id*/          0,
/*xy*/          0,18,
/*wh*/          0,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        INK_BRIGHT_GREEN
                "Horizontal divider",
/*handler*/     0
};

const GC_DITEM_t itmItemBtnOK =
{
/*type*/        DI_BUTTON,
/*id*/          BUTTON_OK,
/*xy*/          3,20,
/*wh*/          20,0,
/*flags*/       {0,0,0,0,0,0,0,1},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        "Ok",
/*handler*/     0
};

const GC_DITEM_t itmItemBtnCN =
{
/*type*/        DI_BUTTON,
/*id*/          BUTTON_CANCEL,
/*xy*/          30,20,
/*wh*/          20,0,
/*flags*/       {0,0,0,0,0,0,0,1},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        "Cancel",
/*handler*/     0
};

const GC_DITEM_t itmItemInfoCB11 =
{
/*type*/        DI_NUMBER,
/*id*/          0,
/*xy*/          0,1,
/*wh*/          3,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         (u8)&cb11,
/*name*/        MARK_LINK"\x0""1.1:"INK_BLUE,
/*handler*/     0
};

const GC_DITEM_t itmItemInfoCB12 =
{
/*type*/        DI_NUMBER,
/*id*/          0,
/*xy*/          0,2,
/*wh*/          3,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         (u8)&cb12,
/*name*/        MARK_LINK"\x0""1.2:"INK_BLUE,
/*handler*/     0
};

const GC_DITEM_t itmItemInfoRB =
{
/*type*/        DI_NUMBER,
/*id*/          0,
/*xy*/          0,4,
/*wh*/          3,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         (u8)&rb,
/*name*/        MARK_LINK"\x1 :"INK_BLUE,
/*handler*/     0
};

const GC_DITEM_t itmItemInfoBtnOK =
{
/*type*/        DI_BUTTON,
/*id*/          BUTTON_OK,
/*xy*/          9,14,
/*wh*/          20,0,
/*flags*/       {0,0,0,0,0,0,0,0},
/*vartype*/     {0,0,0,0,0,0},
/*hotkey*/      0,
/*select*/      0,
/*var*/         0,
/*name*/        "Ok",
/*handler*/     0
};

const GC_DIALOG_t dlgInfo =
{
/*flag*/                {0,0,0,0,0,0,0,0},
/*current*/             0,
/*all_count*/           4,
/*act_count*/           1,
/*cur_attr*/            (WIN_COL_CYAN<<4) | WIN_COL_BRIGHT_YELLOW,
/*box_attr*/            (WIN_COL_CYAN<<4) | WIN_COL_BLUE,
/*btn_focus_attr*/      (WIN_COL_BRIGHT_YELLOW<<4) | WIN_COL_BLACK,
/*btn_unfocus_attr*/    (WIN_COL_BRIGHT_WHITE<<4) | WIN_COL_BLACK,
/*lbox_focus_attr*/     (WIN_COL_BLUE<<4) | WIN_COL_BRIGHT_WHITE,
/*lbox_unfocus_attr*/   (WIN_COL_BLUE<<4) | WIN_COL_WHITE,
/*items*/               dlgInfoItemsList
};

const GC_DITEM_t *dlgInfoItemsList[] =
{
    &itmItemInfoBtnOK,
    &itmItemInfoCB11,
    &itmItemInfoCB12,
    &itmItemInfoRB
};


// DIALOG
const GC_DIALOG_t dlgTest =
{
/*flag*/                {0,0,0,0,0,0,0,0},
/*current*/             0,
/*all_count*/           24,
/*act_count*/           13,
/*cur_attr*/            (WIN_COL_CYAN<<4) | WIN_COL_BRIGHT_YELLOW,
/*box_attr*/            (WIN_COL_CYAN<<4) | WIN_COL_BLUE,
/*btn_focus_attr*/      (WIN_COL_BRIGHT_YELLOW<<4) | WIN_COL_BLACK,
/*btn_unfocus_attr*/    (WIN_COL_BRIGHT_WHITE<<4) | WIN_COL_BLACK,
/*lbox_focus_attr*/     (WIN_COL_BLUE<<4) | WIN_COL_BRIGHT_WHITE,
/*lbox_unfocus_attr*/   (WIN_COL_BLUE<<4) | WIN_COL_WHITE,
/*items*/               dlgTestItemsList
};

// DIALOG ITEMS (active items first)
const GC_DITEM_t *dlgTestItemsList[] =
{
    &itmItemCB11, &itmItemCB12, &itmItemCB3,
    &itmItemCB21, &itmItemCB22, &itmItemCB4, &itmItemED1,
    &itmItemLBX1,
    &itmItemRB1, &itmItemRB2, &itmItemRB3,
    &itmItemBtnOK, &itmItemBtnCN,
    &itmItemHD1, &itmItemTX1,
    &itmItemNMH1, &itmItemNMH2, &itmItemNMH3,
    &itmItemNM1, &itmItemNM2, &itmItemNM3,
    &itmItemG1, &itmItemG2, &itmItemG3
};

// LISTBOX items with linked messages
const u8 *listbox[] =
{
    MARK_LINK"\x2 "MARK_LINK"\x7""1",
    MARK_LINK"\x2 "MARK_LINK"\x7""2",
    MARK_LINK"\x2 "MARK_LINK"\x7""3",
    MARK_LINK"\x2 "MARK_LINK"\x7""4",
    MARK_LINK"\x2 "MARK_LINK"\x7""5",
};

// linked messages (with linked too for example)
// !warning! avoid linking himself!
const u8 *msg_arr[] =
{
/*0*/    "Checkbox ",
/*1*/   "Radio" MARK_LINK "\x6",
/*2*/    "Listbox ",
/*3*/    "linked message",
/*4*/    MARK_CENTER "centered " MARK_LINK "\x3 2 " MARK_LINK "\x8",
/*5*/    MARK_RIGHT "right align " MARK_LINK "\x3 3",
/*6*/    "button",
/*7*/    "item ",
/*8*/    "test"
};
