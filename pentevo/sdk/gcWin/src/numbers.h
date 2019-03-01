//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                       ::
//::               by dr_max^gc (c)2018-2019                 ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

char *dec2asc8(u8 num) __naked __z88dk_fastcall;
char *dec2asc8s(s8 num) __naked __z88dk_fastcall;
char *dec2asc16(u16 num) __naked __z88dk_fastcall;
char *dec2asc16s(s16 num) __naked __z88dk_fastcall;
char *dec2asc32(u32 num) __naked __z88dk_fastcall;
char *dec2asc32s(s32 num) __naked __z88dk_fastcall;

void gcPrintDec8(u8 num) __naked __z88dk_fastcall;
void gcPrintDec8s(s8 num) __naked __z88dk_fastcall;
void gcPrintDec16(u16 num) __naked __z88dk_fastcall;
void gcPrintDec16s(s16 num) __naked __z88dk_fastcall;
void gcPrintDec32(u32 num) __naked __z88dk_fastcall;
void gcPrintDec32s(s32 num) __naked __z88dk_fastcall;

void gcPrintHex8(u8 num) __naked __z88dk_fastcall;
void gcPrintHex16(u16 num) __naked __z88dk_fastcall;
void gcPrintHex32(u32 num) __naked __z88dk_fastcall;

void gcPrintf(char *string, ...) __naked;
