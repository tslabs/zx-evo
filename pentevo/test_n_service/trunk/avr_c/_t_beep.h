#ifndef _T_BEEP_H
#define _T_BEEP_H 1

#ifdef __ASSEMBLER__
/* ------------------------------------------------------------------------- */
.extern tabl_sin
.extern t_beep_ptr
.extern t_beep_delta
.extern t_beep_int
/* ------------------------------------------------------------------------- */
#else // #ifdef __ASSEMBLER__

void Test_Beep(void);

#endif // #ifdef __ASSEMBLER__

#endif // #ifndef _T_BEEP_H
