
#include <avr/io.h>

#include "pins.h"
#include "mytypes.h"

#include "main.h"
#include "zx.h"
#include "spiflash.h"

u32 sf_addr = 0;
u8 sf_state = SF_ST_NONE;
u8 sfi_wr_en = FALSE;

void sfi_cs_off(void)
{
    PORTF |= _BV(SFI_BIT_NCSO);
}

void sfi_cs_on(void)
{
    PORTF |= _BV(SFI_BIT_DCLK);
    PORTF &= ~_BV(SFI_BIT_NCSO);
}

// Enable SFI, disable JTAG
void sfi_enable(void)
{
    u8 m = MCUCSR | _BV(JTD);
    MCUCSR = m; MCUCSR = m;
    DDRF = (DDRF & ~_BV(SFI_BIT_DATA0)) | _BV(SFI_BIT_NCSO) | _BV(SFI_BIT_ASDO) | _BV(SFI_BIT_DCLK);
    PORTF |= _BV(SFI_BIT_DATA0) | _BV(SFI_BIT_NCSO) | _BV(SFI_BIT_DCLK);
}

// Disable SFI, enable JTAG
void sfi_disable(void)
{
    u8 m = MCUCSR & ~_BV(JTD);
    MCUCSR = m; MCUCSR = m;
}

void sfi_send(u8 d)
{
    #define sfi_send_bit(a)  if (d & (a)) { PORTF = c01; PORTF = c11; } else { PORTF = c00; PORTF = c10; }

    u8 c00 = PORTF & ~(_BV(SFI_BIT_DCLK) | _BV(SFI_BIT_ASDO));
    u8 c01 = c00 | _BV(SFI_BIT_ASDO);
    u8 c10 = c00 | _BV(SFI_BIT_DCLK);
    u8 c11 = c01 | _BV(SFI_BIT_DCLK);

    sfi_send_bit(128);
    sfi_send_bit(64);
    sfi_send_bit(32);
    sfi_send_bit(16);
    sfi_send_bit(8);
    sfi_send_bit(4);
    sfi_send_bit(2);
    sfi_send_bit(1);
}

u8 sfi_recv(void)
{
    #define sfi_recv_bit(a) PORTF = c0; PORTF = c1; if (PINF & _BV(SFI_BIT_DATA0)) d |= (a)

    u8 d = 0;
    u8 c0 = PORTF & ~_BV(SFI_BIT_DCLK);
    u8 c1 = c0 | _BV(SFI_BIT_DCLK);

    sfi_recv_bit(128);
    sfi_recv_bit(64);
    sfi_recv_bit(32);
    sfi_recv_bit(16);
    sfi_recv_bit(8);
    sfi_recv_bit(4);
    sfi_recv_bit(2);
    sfi_recv_bit(1);

    return d;
}

void sfi_cmd_h(u8 c)
{
    sfi_cs_on();
    sfi_send(c);
}

void sfi_cmd_ha(u8 c)
{
    sfi_cmd_h(c);
    sfi_send((u8)(sf_addr >> 16));
    sfi_send((u8)(sf_addr >> 8));
    sfi_send((u8)(sf_addr));
}

void sfi_cmd(u8 c)
{
    sfi_cmd_h(c);
    sfi_cs_off();
}

u8 sfi_cmd_r(u8 c)
{
    sfi_cmd_h(c);
    c = sfi_recv();
    sfi_cs_off();
    return c;
}

void sfi_cmd_w(u8 c, u8 d)
{
    sfi_cmd_h(c);
    sfi_send(d);
    sfi_cs_off();
}

void sfi_wren(void)
{
    if (!sfi_wr_en)
    {
        if (sfi_cmd_r(SF_CMD_RDSTAT) & SF_STAT_MASK_BPRT)
        {
            sfi_cmd(SF_CMD_WREN);
            sfi_cmd_w(SF_CMD_WRSTAT, 0);
        }

        sfi_wr_en = TRUE;
    }
}

u8 sf_status(void)
{
    switch (sf_state)
    {
        case SF_ST_IDLE:
            return SPIFL_STAT_NULL;

        case SF_ST_BUSY:
            return SPIFL_STAT_BSY;

        case SF_ST_FBUSY:
            if (sfi_cmd_r(SF_CMD_RDSTAT) & SF_STAT_MASK_WIP)
                return SPIFL_STAT_BSY;
            else
            {
                sf_state = SF_ST_IDLE;
                return SPIFL_STAT_NULL;
            }

        case SF_ST_ERR:
            sf_state = SF_ST_IDLE;
            sfi_cs_off();
            return SPIFL_STAT_ERR;

        default:
            return SPIFL_STAT_NULL;
    }
}

// Execute SF command
void sf_command(u8 cmd)
{
    switch (cmd)
    {
        case SPIFL_CMD_ENA:
            sfi_enable();
        break;
        
        case SPIFL_CMD_DIS:
            sfi_disable();
        break;
        
        case SPIFL_CMD_END:
            sfi_cs_off();
            sf_state = SF_ST_IDLE;
        return;
    }

    if (sf_state == SF_ST_FBUSY)
    {
        if (!(sfi_cmd_r(SF_CMD_RDSTAT) & SF_STAT_MASK_WIP))
            sf_state = SF_ST_IDLE;
    }

    if (sf_state != SF_ST_IDLE)
        return;

    switch (cmd)
    {
        case SPIFL_CMD_ID:
            sfi_cmd_ha(SF_CMD_RDID);
            sf_state = SF_ST_BUSY;
        break;

        case SPIFL_CMD_READ:
            sfi_cmd_ha(SF_CMD_RD);
            sf_state = SF_ST_BUSY;
        break;

        case SPIFL_CMD_WRITE:
            sfi_wren();
            sfi_cmd_ha(SF_CMD_WR);
            sf_state = SF_ST_FBUSY;
        break;

        case SPIFL_CMD_ERSBLK:
            sfi_wren();
            sfi_cmd(SF_CMD_ERBULK);
            sf_state = SF_ST_FBUSY;
        break;

        case SPIFL_CMD_ERSSEC:
            sfi_wren();
            sfi_cmd_ha(SF_CMD_ERSECT);
            sf_state = SF_ST_FBUSY;
        break;

        default:
            sf_state = SF_ST_IDLE;
    }
}

u8 spi_flash_read(u8 index)
{
    switch (index)
    {
        // SF data
        case SPIFL_REG_DATA:
            sf_addr++;
            return sfi_recv();

        // SF status
        case SPIFL_REG_STAT:
            return sf_status();

        // SF low addr
        case SPIFL_REG_A0:
            return (u8)(sf_addr);
        break;

        // SF high addr
        case SPIFL_REG_A1:
            return (u8)(sf_addr >> 8);

        // SF ext addr
        case SPIFL_REG_A2:
            return (u8)(sf_addr >> 16);

        // SF version
        case SPIFL_REG_VER:
            return SPIFL_VER;
    }

    return 0xFF;
}

void spi_flash_write(u8 index, u8 data)
{
    switch (index)
    {
        // SF data
        case SPIFL_REG_DATA:
            sf_addr++;
            sfi_send(data);
        break;

        // SF command
        case SPIFL_REG_CMD:
            sf_command(data);
        break;

        // select another Glu Ext register
        case SPIFL_REG_EXTSW:
            ext_type_gluk = data;
        break;

        // SF low addr
        case SPIFL_REG_A0:
            sf_addr &= 0xFFFFFF00;
            sf_addr |= data;
        break;

        // SF high addr
        case SPIFL_REG_A1:
            sf_addr &= 0xFFFF00FF;
            sf_addr |= (u32)data << 8;
        break;

        // SF ext addr
        case SPIFL_REG_A2:
            sf_addr &= 0xFF00FFFF;
            sf_addr |= (u32)data << 16;
        break;
    }
}
