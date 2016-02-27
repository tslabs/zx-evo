#ifndef RS232_H
#define RS232_H

void rs232_init(void);
void rs232_transmit(u8 data);

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


/** DR registers limit. */
#define ZF_DR_REG_LIM     0xBF

/** IFR register. */
#define ZF_IFR_REG        0xC0

/** OFR register. */
#define ZF_OFR_REG        0xC1

/** CR/ER register. */
#define ZF_CR_ER_REG      0xC7


/** CLRFIFO command. */
#define ZF_CLRFIFO        0b00000000
#define ZF_CLRFIFO_MASK   0b11111100
#define ZF_CLRFIFO_IN     0b00000001
#define ZF_CLRFIFO_OUT    0b00000010

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

#endif //RS232_H
