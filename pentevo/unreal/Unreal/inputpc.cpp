#include "std.h"
#include "emul.h"
#include "vars.h"
#include "inputpc.h"

const u16 dik_scan[256] =
{
    0x0000,
    0x0076, // DIK_ESCAPE
    0x0016, // DIK_1               
    0x001E, // DIK_2               
    0x0026, // DIK_3               
    0x0025, // DIK_4               
    0x002E, // DIK_5               
    0x0036, // DIK_6               
    0x003D, // DIK_7               
    0x003E, // DIK_8               
    0x0046, // DIK_9               
    0x0045, // DIK_0
    0x004E, // DIK_MINUS
    0x0055, // DIK_EQUALS          
    0x0066, // DIK_BACK
    0x000D, // DIK_TAB             
    0x0015, // DIK_Q               
    0x001D, // DIK_W               
    0x0024, // DIK_E               
    0x002D, // DIK_R               
    0x002C, // DIK_T               
    0x0035, // DIK_Y               
    0x003C, // DIK_U               
    0x0043, // DIK_I               
    0x0044, // DIK_O               
    0x004D, // DIK_P
    0x0054, // DIK_LBRACKET        
    0x005B, // DIK_RBRACKET        
    0x005A, // DIK_RETURN
    0x0014, // DIK_LCONTROL        
    0x001C, // DIK_A               
    0x001B, // DIK_S               
    0x0023, // DIK_D               
    0x002B, // DIK_F               
    0x0034, // DIK_G               
    0x0033, // DIK_H               
    0x003B, // DIK_J               
    0x0042, // DIK_K               
    0x004B, // DIK_L
    0x004C, // DIK_SEMICOLON       
    0x0052, // DIK_APOSTROPHE      
    0x000E, // DIK_GRAVE
    0x0012, // DIK_LSHIFT          
    0x005D, // DIK_BACKSLASH       
    0x001A, // DIK_Z               
    0x0022, // DIK_X               
    0x0021, // DIK_C               
    0x002A, // DIK_V               
    0x0032, // DIK_B               
    0x0031, // DIK_N               
    0x003A, // DIK_M
    0x0041, // DIK_COMMA           
    0x0049, // DIK_PERIOD              /* . on main keyboard */
    0x004A, // DIK_SLASH               /* / on main keyboard */
    0x0059, // DIK_RSHIFT          
    0x007C, // DIK_MULTIPLY            /* * on numeric keypad */
    0x0011, // DIK_LMENU               /* left Alt */
    0x0029, // DIK_SPACE           
    0x0058, // DIK_CAPITAL         
    0x0005, // DIK_F1              
    0x0006, // DIK_F2              
    0x0004, // DIK_F3              
    0x000C, // DIK_F4              
    0x0003, // DIK_F5              
    0x000B, // DIK_F6              
    0x0083, // DIK_F7              
    0x000A, // DIK_F8              
    0x0001, // DIK_F9              
    0x0009, // DIK_F10             
    0x0077, // DIK_NUMLOCK         
    0x007E, // DIK_SCROLL              /* Scroll Lock */
    0x006C, // DIK_NUMPAD7         
    0x0075, // DIK_NUMPAD8         
    0x007D, // DIK_NUMPAD9         
    0x007B, // DIK_SUBTRACT            /* - on numeric keypad */
    0x006B, // DIK_NUMPAD4         
    0x0073, // DIK_NUMPAD5         
    0x0074, // DIK_NUMPAD6         
    0x0079, // DIK_ADD                 /* + on numeric keypad */
    0x0069, // DIK_NUMPAD1         
    0x0072, // DIK_NUMPAD2         
    0x007A, // DIK_NUMPAD3         
    0x0070, // DIK_NUMPAD0         
    0x0071, // DIK_DECIMAL             /* . on numeric keypad */
    0x0000,
    0x0000,
    0x0000, // DIK_OEM_102             /* <> or \| on RT 102-key keyboard (Non-U.S.) */
    0x0078, // DIK_F11             
    0x0007, // DIK_F12
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000, // DIK_F13                 /*                     (NEC PC98) */
    0x0000, // DIK_F14                 /*                     (NEC PC98) */
    0x0000, // DIK_F15                 /*                     (NEC PC98) */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000, // DIK_KANA                /* (Japanese keyboard)            */
    0x0000,
    0x0000,
    0x0000, // DIK_ABNT_C1             /* /? on Brazilian keyboard */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000, // DIK_CONVERT             /* (Japanese keyboard)            */
    0x0000,
    0x0000, // DIK_NOCONVERT           /* (Japanese keyboard)            */
    0x0000,
    0x0000, // DIK_YEN                 /* (Japanese keyboard)            */
    0x0000, // DIK_ABNT_C2             /* Numpad . on Brazilian keyboard */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000, // DIK_NUMPADEQUALS        /* = on numeric keypad (NEC PC98) */
    0x0000,
    0x0000,
    0x0000, // DIK_PREVTRACK           /* Previous Track (DIK_CIRCUMFLEX on Japanese keyboard) */
    0x0000, // DIK_AT                  /*                     (NEC PC98) */
    0x0000, // DIK_COLON               /*                     (NEC PC98) */
    0x0000, // DIK_UNDERLINE           /*                     (NEC PC98) */
    0x0000, // DIK_KANJI               /* (Japanese keyboard)            */
    0x0000, // DIK_STOP                /*                     (NEC PC98) */
    0x0000, // DIK_AX                  /*                     (Japan AX) */
    0x0000, // DIK_UNLABELED           /*                        (J3100) */
    0x0000,
    0x0000, // DIK_NEXTTRACK           /* Next Track */
    0x0000,
    0x0000,
    0x015A, // DIK_NUMPADENTER         /* Enter on numeric keypad */
    0x0114, // DIK_RCONTROL        
    0x0000,
    0x0000,
    0x0000, // DIK_MUTE                /* Mute */
    0x0000, // DIK_CALCULATOR          /* Calculator */
    0x0000, // DIK_PLAYPAUSE           /* Play / Pause */
    0x0000,
    0x0000, // DIK_MEDIASTOP           /* Media Stop */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000, // DIK_VOLUMEDOWN          /* Volume - */
    0x0000,
    0x0000, // DIK_VOLUMEUP            /* Volume + */
    0x0000,
    0x0000, // DIK_WEBHOME             /* Web home */
    0x0000, // DIK_NUMPADCOMMA         /* , on numeric keypad (NEC PC98) */
    0x0000,
    0x014A, // DIK_DIVIDE              /* / on numeric keypad */
    0x0000,
    0x0112, // DIK_SYSRQ           
    0x0111, // DIK_RMENU               /* right Alt */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000, // DIK_PAUSE               /* Pause */
    0x0000,
    0x016C, // DIK_HOME                /* Home on arrow keypad */
    0x0175, // DIK_UP                  /* UpArrow on arrow keypad */
    0x017D, // DIK_PRIOR               /* PgUp on arrow keypad */
    0x0000,
    0x016B, // DIK_LEFT                /* LeftArrow on arrow keypad */
    0x0000,
    0x0174, // DIK_RIGHT               /* RightArrow on arrow keypad */
    0x0000,
    0x0169, // DIK_END                 /* End on arrow keypad */
    0x0172, // DIK_DOWN                /* DownArrow on arrow keypad */
    0x017A, // DIK_NEXT                /* PgDn on arrow keypad */
    0x0170, // DIK_INSERT              /* Insert on arrow keypad */
    0x0171, // DIK_DELETE              /* Delete on arrow keypad */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x011F, // DIK_LWIN                /* Left Windows key */
    0x0127, // DIK_RWIN                /* Right Windows key */
    0x012F, // DIK_APPS                /* AppMenu key */
    0x0137, // DIK_POWER               /* System Power */
    0x013F, // DIK_SLEEP               /* System Sleep */
    0x0000,
    0x0000,
    0x0000,
    0x015E, // DIK_WAKE                /* System Wake */
    0x0000,
    0x0000, // DIK_WEBSEARCH           /* Web Search */
    0x0000, // DIK_WEBFAVORITES        /* Web Favorites */
    0x0000, // DIK_WEBREFRESH          /* Web Refresh */
    0x0000, // DIK_WEBSTOP             /* Web Stop */
    0x0000, // DIK_WEBFORWARD          /* Web Forward */
    0x0000, // DIK_WEBBACK             /* Web Back */
    0x0000, // DIK_MYCOMPUTER          /* My Computer */
    0x0000, // DIK_MAIL                /* Mail */
    0x0000, // DIK_MEDIASELECT         /* Media Select */
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

const PC_KEY pc_layout[] =
{
   { DIK_1, 0x31, 0xB1 },
   { DIK_2, 0x32, 0xB2 },
   { DIK_3, 0x33, 0xB3 },
   { DIK_4, 0x34, 0xB4 },
   { DIK_5, 0x35, 0xB5 },
   { DIK_6, 0x45, 0xE5 },
   { DIK_7, 0x44, 0xC5 },
   { DIK_8, 0x43, 0xF5 },
   { DIK_9, 0x42, 0xC3 },
   { DIK_0, 0x41, 0xC2 },
   { DIK_MINUS, 0xE4, 0xC1 }, // -_
   { DIK_EQUALS, 0xE2, 0xE3 }, // =+

   { DIK_Q, 0x21, 0x29 },
   { DIK_W, 0x22, 0x2A },
   { DIK_E, 0x23, 0x2B },
   { DIK_R, 0x24, 0x2C },
   { DIK_T, 0x25, 0x2D },
   { DIK_Y, 0x55, 0x5D },
   { DIK_U, 0x54, 0x5C },
   { DIK_I, 0x53, 0x5B },
   { DIK_O, 0x52, 0x5A },
   { DIK_P, 0x51, 0x59 },
//   { DIK_LBRACKET, 0xD5, 0x94 }, // [{
//   { DIK_RBRACKET, 0xD4, 0x95 }, // ]}

   { DIK_A, 0x11, 0x19 },
   { DIK_S, 0x12, 0x1A },
   { DIK_D, 0x13, 0x1B },
   { DIK_F, 0x14, 0x1C },
   { DIK_G, 0x15, 0x1D },
   { DIK_H, 0x65, 0x6D },
   { DIK_J, 0x64, 0x6C },
   { DIK_K, 0x63, 0x6B },
   { DIK_L, 0x62, 0x6A },
   { DIK_SEMICOLON, 0xD2, 0x82 }, // ;:
   { DIK_APOSTROPHE, 0xC4, 0xD1 }, // '"

   { DIK_Z, 0x02, 0x0A },
   { DIK_X, 0x03, 0x0B },
   { DIK_C, 0x04, 0x0C },
   { DIK_V, 0x05, 0x0D },
   { DIK_B, 0x75, 0x7D },
   { DIK_N, 0x74, 0x7C },
   { DIK_M, 0x73, 0x7B },
   { DIK_COMMA, 0xF4, 0xA4 }, // ,<
   { DIK_PERIOD, 0xF3, 0xA5 }, // .>
   { DIK_SLASH, 0x85, 0x84 }, // /?
   { DIK_BACKSLASH, 0x93, 0x92 }, // \|
};

const size_t pc_layout_count = _countof(pc_layout);
