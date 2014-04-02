#ifndef _T_PS2M_H
#define _T_PS2M_H 1

#define PS2M_BIT_PARITY  0
/* принят ACK-бит */
#define PS2M_BIT_ACKBIT  1
/* передача */
#define PS2M_BIT_TX      7

#define PS2M_BIT_READY   7

#ifdef __ASSEMBLER__
/* ------------------------------------------------------------------------- */
.extern ps2m_bit_count
.extern ps2m_data
.extern ps2m_raw_ready
.extern ps2m_raw_code
.extern ps2m_flags
/* ------------------------------------------------------------------------- */
#else // #ifdef __ASSEMBLER__

void Test_PS2Mouse(void);

#endif // #ifdef __ASSEMBLER__

#endif // #ifndef _PS2M_H
