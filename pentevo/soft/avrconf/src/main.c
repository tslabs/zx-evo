/*
    ZX-Evolution Configuration Utility 
    --wbcbz7 18.11.2021

    based on gcWin windows system by Doctor Max 
*/

#include <stdio.h>
#include <string.h>

// gcWin definitions
#include <defs.h>
#include <tsio.h>
#include <gcWin.h>

#include "main.h"
#include "items.h"

#include "config_interface.h"

#define DI __asm__("di\n");
#define EI __asm__("ei\n");
#define EIHALT __asm__("ei\n halt\n");
#define DIHALT __asm__("di\n halt\n");

#define VPAGE   0x80    // video page
#define SPAGE   0x82    // shadow screen page
#define GPAGE   0x88    // mouse sprite page

#define arrayof(a) (sizeof(a) / sizeof(a[0]))

__sfr __banked __at 0xDFF7 CMOS_ADDR;
__sfr __banked __at 0xBFF7 CMOS_DATA;
__sfr __banked __at 0xEFF7 CMOS_CONF;

// -------------------------------
// stuff functions

inline void rtcUnlock() {
    CMOS_CONF = 0x80;
}
inline void rtcLock() {
    CMOS_CONF = 0x00;
}
inline u8 rtcRead(u8 index) {
    CMOS_ADDR = index;
    return CMOS_DATA;
}

inline void rtcWrite(u8 index, u8 data) {
    CMOS_ADDR = index;
    CMOS_DATA = data;    
}

inline void configUnlock() {
    rtcUnlock();
    rtcWrite(0xF0 + CFGIF_REG_EXTSW, 0x0E);
}

inline void configLock() {
    rtcWrite(0xF0 + CFGIF_REG_EXTSW, 0x02);
    //rtcLock();
}

// zero if not found
u8 configCheck() {
    DI
    configUnlock();

    u8 rtn = 0;

    // test for ZX-Evo returning 0xFF for non-existent gluexts
    u8 reg0 = rtcRead(0xF0 + CFGIF_REG_MODES_VIDEO);
    u8 reg1 = rtcRead(0xF0 + CFGIF_REG_MODES_MISC);

    if ((reg0 == 0xFF) && (reg1 == 0xFF)) goto exit;

    // all checks passed, return 1
    rtn = 1;

exit:
    configUnlock();
    EI
    
    return rtn;
}

// binary data
u8 resetTrampoline[] = {
    0x3E, 0x04,         // ld      a,  MEM_W0MAP_N
    0x01, 0xAF, 0x21,   // ld      bc, MEMCONFIG
    0xED, 0x79,         // out     (c), a
    0xAF,               // xor     a
    0x06, 0x20,         // ld      b,  high SYSCONFIG
    0xED, 0x79,         // out     (c), a
    0x06, 0x10,         // ld      b,  high PAGE0
    0xED, 0x79,         // out     (c), a
    0xC7,               // rst     0
};

void bios_reset() {
    DI
    
    TS_VCONFIG  =   TS_VID_ZX | TS_VID_256X192;
    TS_VPAGE    =   5;
    TS_TSCONFIG =   0;
    memcpy((u8*)0xC000, resetTrampoline, sizeof(resetTrampoline));
    __asm__("jp 0xC000\n");
}


u8 joyNames[2][16][16];
u8 joyMap[2][16];
u8 joyAutofire[2][16];
u8 JoyAutofireRaw[2][2];

u8 padModeRaw;
u8 joy1Type;
u8 joy2Type;
u8 joyMappingType;

const u8 remapTable[] = {
    7, 15, 23, 31, 39,
    6, 14, 22, 30, 38,
    5, 13, 21, 29, 37,
    4, 12, 20, 28, 36,
    3, 11, 19, 27, 35,
    2, 10, 18, 26, 34,
    1,  9, 17, 25, 33,
    0,  8, 16, 24, 32,
};

u8 keyScan() __naked __z88dk_fastcall {
    __asm 
    // --------------------------------------------------
    // joy mapper, second try
    // out: A - scancode (D[5:0] - key code, D[6] - CS pressed, D[7] - SS pressed)
    di

    push        bc
    push        de
    push        hl

    /*
        стейты:
        0: кнопки не нажимались ни разу
        1: нажата хотя бы одна кнопка (в том числе шифты)
        2: все кнопки отжаты, но стейт сохранен
        
        во всех случаях сохранять шифты
    */
    
    // wait for no keypress
1$:
    xor         a
    in          a, (0xFE)
    and         #0x1F
    cp          #0x1F
    jp          nz, 1$
    
    // wait for first keypress
2$:
    xor         a
    in          a, (0xFE)
    and         #0x1F
    cp          #0x1F
    jp          z, 2$ 
    
    // key is pressed, save its state
    // D - CS/SS state, E - last key index (excluding CS/SS)
    
    ld          de, #0
outer:
    
    // fill CS state
    ld          a, #0xFE
    in          a, (0xFE)
    bit         0, a
    jr          nz, 3$
    set         6, d
3$:
    
    // fill SS state
    ld          a, #0x7F
    in          a, (0xFE)
    bit         1, a
    jr          nz, 4$
    set         7, d
4$:
    
    // parse each key
    ld          bc, #0x08FE      // b - xxFE, c - iterations count
    ld          hl, #0           // local CS/SS and last key index

keyscan:
    ld          a, c
    in          a, (0xFE)
    
    rrca
    call    nc, test
    inc     l
    rrca
    call    nc, test
    inc     l
    rrca
    call    nc, test
    inc     l
    rrca
    call    nc, test
    inc     l
    rrca
    call    nc, test
    inc     l
    
    rlc         c
    djnz        keyscan
    
    ld          a, l
    cp          #40
    jr          nz, outer    // keydown
    ld          a, h
    and         a
    jr          z, merge
    jp          outer
test:
    // check if it's actually CS/SS
    ex          af, af      // '
    ld          a, l
    and         a
    jr          z, testcs
    cp          #0x24
    jr          z, testss
    pop         af          // remove return address from stack
    ex          af, af      // '
    
    ld          e, l
    jp          outer
    
testcs:
    set         6, h
    ex          af, af      //'
    ret
testss:
    set         7, h
    ex          af, af      //'
    ret
    
    // end here
merge:

    ld          a, d
    or          e
    jp          z, nokey

    ld          a, e
    and         a
    jp          z, convertShifts

merge2:
    ld          hl, #_remapTable
    ld          b, #0
    ld          c, e
    add         hl, bc
    ld          a, (hl)
    or          d
    jp          done
    
convertShifts:
    bit         7, d
    jr          nz, mergess
    bit         6, d
    jr          nz, mergecs
    jp          merge2
    
mergess:
    res         7, d
    ld          e, #0x24
    jp          merge2
    
mergecs:
    res         6, d
    ld          e, #0
    jp          merge2   
    
nokey:    
    ld          a, #0xFF
done:
    
    pop         hl
    pop         de
    pop         bc

    ld          l, a
    ei
    ret

    __endasm;
}

char buttonLabels[40][8] = {
    "Space", "Enter", "P", "0", "1", "Q", "A", "CS",
    "SS", "L", "O", "9", "2", "W", "S", "Z",
    "M", "K", "I", "8", "3", "E", "D", "X",
    "N", "J", "U", "7", "4", "R", "F", "C",
    "B", "H", "Y", "6", "5", "T", "G", "V",
};
char buttonNone[] = "None";
char buttonUnknown[] = "Unknown";

char *btnMessages[] = {
    "RIGHT",
    "LEFT",
    "DOWN",
    "UP",
    "B",
    "C",
    "A",
    "START",

    // reserved for more keys
    "", "", "", "", "", "", "", "",

    // key
};

u8* strAppend(u8* dst, u8* src) {
    while (*src != 0) *dst++ = *src++; *dst = '\0';
    return dst;
}

void fillButtonString(u8* ptr, u8 button) {

    if (button == CFGIF_PAD_MAPPING_NO_KEY) {
        strAppend(ptr, buttonNone);
        return;
    }
    if ((button & CFGIF_PAD_MAPPING_KEY_MASK) >= 40) {
        strAppend(ptr, buttonUnknown);
        return;
    }

    u8 *p = ptr;
    if (button & CFGIF_PAD_MAPPING_MOD_CS) {
        p = strAppend(p, buttonLabels[KEY_CS]);
        p = strAppend(p, "+");
    }
    if (button & CFGIF_PAD_MAPPING_MOD_SS) {
        p = strAppend(p, buttonLabels[KEY_SS]);
        p = strAppend(p, "+");
    }
    p = strAppend(p, buttonLabels[(button & CFGIF_PAD_MAPPING_KEY_MASK)]);
}


void queryJoyVariables() {
    DI
    configUnlock(); 

    // read pad mode
    padModeRaw = rtcRead(0xF0 + CFGIF_REG_PAD_MODE);
    joy1Type = padModeRaw & 7;
    joy2Type = (padModeRaw >> 3) & 7;
    joyMappingType = (padModeRaw >> 6) & 3;

    joy1Type = min(joy1Type, CFGIF_PAD_MODE_SEGA);
    joy2Type = min(joy2Type, CFGIF_PAD_MODE_SEGA);
    joyMappingType = min(joyMappingType, CFGIF_PAD_MAPPING_KEYS_KEYS);

    // read mappings
    for (u8 joy = 0; joy < 2; joy++) {
        for (u8 i = 0; i < 16; i++) {
            u8 btn = rtcRead(0xF0 + CFGIF_REG_PAD_KEYMAP0 + joy);
            joyMap[joy][i] = btn;
            fillButtonString(joyNames[joy][i], btn);
        }
        for (u8 i = 0; i < 2; i++) {
            JoyAutofireRaw[joy][i] = rtcRead(0xF0 + CFGIF_REG_PAD_AUTOFIRE0 + joy);

            for (u8 p = 0; p < 8; p++) {
                joyAutofire[joy][i*8+p] = ((JoyAutofireRaw[joy][i] >> p) & 1) ? 0xFF : 0;
            }
        }
        rtcRead(0xF0 + CFGIF_REG_STATUS);
    }

    configLock();
    EI
}

void cb_writeJoyChanges() {
    DI
    configUnlock(); 

    padModeRaw = (joy1Type & 7) | ((joy2Type & 7) << 3) | ((joyMappingType & 3) << 6);
    rtcWrite(0xF0 + CFGIF_REG_PAD_MODE, padModeRaw);
    
    for (u8 joy = 0; joy < 2; joy++) {
        for (u8 i = 0; i < 16; i++) {
            rtcWrite(0xF0 + CFGIF_REG_PAD_KEYMAP0 + joy, joyMap[joy][i]);
        }

        for (u8 i = 0; i < 2; i++) {
            u8 rtn = 0;
            for (u8 p = 0; p < 8; p++) {
                rtn |= ((joyAutofire[joy][i*8+p] != 0 ? 1 : 0) << p);
            }
            rtcWrite(0xF0 + CFGIF_REG_PAD_AUTOFIRE0 + joy, rtn);

        }
    }


    configLock();
    EI
}

void cb_doKeyMap(GC_DITEM_t *ditm) {
    gcPrintWindow(&wndPress);
    
    EIHALT
    gcWaitNoKey();
    joyMap[0][ditm->id] = keyScan();
    gcCloseWindow();

    while(gcGetKey());
    DI
    CMOS_CONF = 0x80;
    CMOS_ADDR = 0x0C;
    CMOS_DATA = 1;
    EI

    fillButtonString(joyNames[0][ditm->id], joyMap[0][ditm->id]);
    gcPrintDialog(&wndMainDlg);
}

void cb_resetCmos() {
    if (gcMessageBox(
            MB_OKCANCEL, GC_FRM_SINGLE, "Warning",
            "Clearing CMOS will reset all your TS-BIOS settings! Proceed?"
        ) == BUTTON_OK) {

        DI
        rtcUnlock();

        // write 0s to CMOS (don't touch date/time!)
        for (u8 reg = 0xF; reg < 0xF0; reg++)
            rtcWrite(reg, 0);

        rtcLock();
        EI
    }
}

void cb_resetEeprom() {
    if (gcMessageBox(
            MB_OKCANCEL, GC_FRM_SINGLE, "Warning",
            "Clearing EEPROM will reset all your CMOS settings! Proceed?"
        ) == BUTTON_OK) {

        gcExecuteWindow(&wndEepromClear);
        EIHALT
        EIHALT
        DI
        rtcUnlock();

        // enable EEPROM access
        rtcWrite(0xC, rtcRead(0xC) | 0x80);

        // SDCC is so freaking slow at port access so I will do eeprom write in asm
        __asm
            push        af
            push        bc
            push        de
            push        hl

            ld          bc, #0xDFF7
            ld          de, #0x00FF
            ld          hl, #0xDFBF

        1$:
            ld          a, #0x0A
            ld          b, h
            out         (c), a
            ld          b, l
            out         (c), d
            ld          a, #0xF0
            
        2$:
            // inner loop
            ld          b, h
            out         (c), a
            ld          b, l
            out         (c), e

            inc         a
            jp          nz, 2$

            dec         d
            jp          nz, 1$

            pop         hl
            pop         de
            pop         bc
            pop         af

        __endasm;

        /*
        for (u8 page = 0; page <= 0xFF; page++) {
            rtcWrite(0xA, page);
            for (u8 offset = 0; offset < (page == 0xFF ? 15 : 16); offset++) {
                rtcWrite(0xF0 + offset, 0xFF);
            }
        }
        */

        // disable EEPROM access
        rtcWrite(0xC, rtcRead(0xC) & ~0x80);

        rtcLock();
        EI
    }
}

void cb_resetFlash() {
    if (gcMessageBox(
            MB_OKCANCEL, GC_FRM_SINGLE, "Warning",
            "Are you really sure you want to brick your ZX Evolution?"
        ) == BUTTON_OK) {
        
        DI
        rtcUnlock();
        rtcWrite(0xF0 + CFGIF_REG_EXTSW, 0x0E);
        rtcWrite(0xF0 + CFGIF_REG_COMMAND, CFGIF_CMD_REBOOT_FLASH);

        // reset to bios if failed
        bios_reset();
    }
}

void cb_hardReset() {
    DI
    rtcUnlock();
    rtcWrite(0xF0 + CFGIF_REG_EXTSW, 0x0E);
    rtcWrite(0xF0 + CFGIF_REG_COMMAND, CFGIF_CMD_REBOOT);

    // reset to bios if failed
    bios_reset();
}

void cbAbout() {
    gcMessageBox(
            MB_OK, GC_FRM_SINGLE, "About...",
            INK_BRIGHT_YELLOW"ZX-Evolution"INK_BRIGHT_BLUE" Configuration Utility"INK_BLACK" v.0.000001\n"
            "^(cl)^ by ""Artem Vasilev / wbcbz7"", compiled at "__DATE__" "__TIME__"\n"
            "Based on "INK_BRIGHT_YELLOW"gcWin"INK_BLACK" GUI framework by Doctor Max/Global corp."
    );
}

inline void cbConfigIfNotFound() {
    gcMessageBox(
        MB_OK, GC_FRM_SINGLE, "Error",
        "Configuration Interface not found!\n"
        "Update TS-Conf to the latest version and restart application"
    );
}

// --------------------------

u8 joyConfigProc() {
    if (configCheck() == 0) {
        cbConfigIfNotFound();
        return 1;
    }

    gcPrintWindow(&wndJoyConfigLoad);
    EIHALT
    queryJoyVariables();
    gcSetLinkedMessage(btnMessages);

    u8 select = gcExecuteWindow(&wndMain);
    if (select == BUTTON_OK) {
        gcPrintWindow(&wndJoyConfigSave);
        EIHALT
        cb_writeJoyChanges();
    }

    return 0;
}

u8 svmServiceProc() {
    u8 select;

    while (1) {
        gcPrintWindow(&wndBackdrop);
        select = gcExecuteWindow(&wndSVMService);

        switch (select) {

        case svmService_ResetCMOS:
            cb_resetCmos();
            break;

        case svmService_ResetEEPROM:
            cb_resetEeprom();
            break;

        case svmService_HardReset:
            cb_hardReset();
            break;

        case svmService_ResetFlash:
            cb_resetFlash();
            break;

        case svmService_MainMenu:
            gcCloseWindow();    // close wndSVMService
            return 0;

        default:
            break;
        }

        gcCloseWindow();    // close wndSVMService
    }

    return select;
}

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

    while (1) {
        gcPrintWindow(&wndBackdrop);

        u8 select;

        select = gcExecuteWindow(&wndSVMnu);
        switch(select) {
            case svmMain_JoystickConfig:
                joyConfigProc();
                break;
            
            case svmMain_SoftReset:
                bios_reset();
                break;

            case svmMain_Service:
                svmServiceProc();
                break;

            case svmMain_About:
                cbAbout();
                break;

            default:
                break;
        }
        
        gcCloseWindow();    // close wndSVMnu
    }

}
