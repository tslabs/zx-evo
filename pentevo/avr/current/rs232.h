#ifndef RS232_H
#define RS232_H 1

#ifdef __ASSEMBLER__
.extern dbuf

.extern rs232_LSR
/* .extern rs_rxbuff */
.extern rs_rx_hd
.extern rs_rx_tl
/* .extern rs_txbuff */
.extern rs_tx_hd
.extern rs_tx_tl

/* .extern zf_rxbuff */
.extern zf_rx_hd
.extern zf_rx_tl
/* .extern zf_txbuff */
.extern zf_tx_hd
.extern zf_tx_tl
#else //__ASSEMBLER__

#include "main.h"

extern volatile u8 rs_tmo_cnt;
extern volatile u8 zf_tmo_cnt;

void rs232_init(void);
//void rs232_transmit(u8 data);

//#define LOGENABLE
#ifdef LOGENABLE
void to_log(char* ptr);
#endif

/**
 * ZX write to Kondratiev's rs232 registers.
 * @param index [in] - index of Kondratiev's rs232 register
 * @param byte [in] - data
 */
void rs232_zx_write(u8 index, u8 data);

/**
 * ZX read from Kondratiev's rs232 registers.
 * @return registers data
 * @param index [in] - index of Kondratiev's rs232 register
 */
u8 rs232_zx_read(u8 index);

/** RS232 task. */
void rs232_task(void);


/** DAT/DLL register. */
#define UART_DAT_DLL_REG  0xF8

/** IER/DLM register. */
#define UART_IER_DLM_REG  0xF9

/** FCR/ISR register. */
#define UART_FCR_ISR_REG  0xFA

/** LCR register. */
#define UART_LCR_REG      0xFB

/** MCR register. */
#define UART_MCR_REG      0xFC

/** LSR register. */
#define UART_LSR_REG      0xFD

/** MSR register. */
#define UART_MSR_REG      0xFE

/** SPR register. */
#define UART_SPR_REG      0xFF


/** ZF_DR registers limit. */
#define ZF_DR_REG_LIM     0xBF

/** ZF_IFR register. */
#define ZF_IFR_REG        0xC0

/** ZF_OFR register. */
#define ZF_OFR_REG        0xC1

/** RS_IFR register. */
#define RS_IFR_REG        0xC2

/** RS_OFR register. */
#define RS_OFR_REG        0xC3

/** RS/ZF_IMR register. */
#define ZF_IMR_REG        0xC4

/** RS/ZF_ISR register. */
#define ZF_ISR_REG        0xC4

/** ZF_IBTR register. */
#define ZF_IBTR_REG       0xC5

/** ZF_ITOR register. */
#define ZF_ITOR_REG       0xC6

/** RS/ZF_CR register. */
#define ZF_CR_REG         0xC7

/** RS/ZF_ER register. */
#define ZF_ER_REG         0xC7

/** RS_IBTR register. */
#define RS_IBTR_REG       0xC8

/** RS_ITOR register. */
#define RS_ITOR_REG       0xC9

/** ZF_registers limit. */
#define ZF_REG_LIM        0xCF


/** Interrupt masks. */
#define ZF_IMR_IBT        0x01
#define ZF_IMR_ITO        0x02
#define RS_IMR_IBT        0x04
#define RS_IMR_ITO        0x08


/** CLRFIFO command. */
#define ZF_CLRFIFO        0b00000000
#define ZF_CLRFIFO_MASK   0b11111100
#define ZF_CLRFIFO_IN     0b00000001
#define ZF_CLRFIFO_OUT    0b00000010
#define RS_CLRFIFO        0b00000100
#define RS_CLRFIFO_MASK   0b11111100
#define RS_CLRFIFO_IN     0b00000001
#define RS_CLRFIFO_OUT    0b00000010

/** SETAPI command. */
#define ZF_SETAPI         0b11110000
#define ZF_SETAPI_MASK    0b11111000

/** GETVER command. */
#define ZF_GETVER         0b11111111
#define ZF_GETVER_MASK    0b11111111

/** OK result. */
#define ZF_OK_RES         0x00


/** ZiFi version. */
#define ZF_VER            0x01

#endif //__ASSEMBLER__

#define rs_rxbuff (dbuf+512)
#define rs_txbuff (dbuf+0)
#define zf_rxbuff (dbuf+1024)
#define zf_txbuff (dbuf+256)

#endif //RS232_H
