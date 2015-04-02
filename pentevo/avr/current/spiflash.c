
#include <avr/io.h>

#include "pins.h"
#include "mytypes.h"

#include "main.h"
#include "zx.h"
#include "spiflash.h"

ULONG sf_addr = 0;
UBYTE sf_state = SF_ST_INACT;
UBYTE sfi_wr_en = FALSE;

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
    UBYTE m = MCUCSR | _BV(JTD);
    MCUCSR = m; MCUCSR = m;
    DDRF = (DDRF & ~_BV(SFI_BIT_DATA0)) | _BV(SFI_BIT_NCSO) | _BV(SFI_BIT_ASDO) | _BV(SFI_BIT_DCLK);
    PORTF |= _BV(SFI_BIT_DATA0) | _BV(SFI_BIT_NCSO) | _BV(SFI_BIT_DCLK);
    sf_state = SF_ST_IDLE;
}

// Disable SFI, enable JTAG
void sfi_disable(void)
{
    UBYTE m = MCUCSR & ~_BV(JTD);
    MCUCSR = m; MCUCSR = m;
    sf_state = SF_ST_INACT;
}

void sfi_send(UBYTE d)
{
    #define sfi_send_bit(a)  if (d & (a)) { PORTF = c01; PORTF = c11; } else { PORTF = c00; PORTF = c10; }
    
    UBYTE c00 = PORTF & ~(_BV(SFI_BIT_DCLK) | _BV(SFI_BIT_ASDO));
    UBYTE c01 = c00 | _BV(SFI_BIT_ASDO);
    UBYTE c10 = c00 | _BV(SFI_BIT_DCLK);
    UBYTE c11 = c01 | _BV(SFI_BIT_DCLK);
    
    sfi_send_bit(128);
    sfi_send_bit(64); 
    sfi_send_bit(32); 
    sfi_send_bit(16); 
    sfi_send_bit(8);
    sfi_send_bit(4);
    sfi_send_bit(2);
    sfi_send_bit(1);
}

UBYTE sfi_recv(void)
{
    #define sfi_recv_bit(a)  *c2 = c0; *c2 = c1; if (PINF & _BV(SFI_BIT_DATA0)) d |= (a)
    
    UBYTE d = 0;
    UBYTE c0 = PORTF & ~_BV(SFI_BIT_DCLK);
    UBYTE c1 = c0 | _BV(SFI_BIT_DCLK);
    volatile UBYTE* c2 = PORTF;
    
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

void sfi_cmd_h(UBYTE c)
{
    sfi_cs_on();
    sfi_send(c);
}

void sfi_cmd_ha(UBYTE c)
{
    sfi_cmd_h(c);
    sfi_send((UBYTE)(sf_addr >> 16));
    sfi_send((UBYTE)(sf_addr >> 8));
    sfi_send((UBYTE)(sf_addr));
}

void sfi_cmd(UBYTE c)
{
    sfi_cmd_h(c);
    sfi_cs_off();
}

UBYTE sfi_cmd_r(UBYTE c)
{
    sfi_cmd_h(c);
    c = sfi_recv();
    sfi_cs_off();
    return c;
}

void sfi_cmd_w(UBYTE c, UBYTE d)
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

UBYTE sf_status(void)
{
    switch (sf_state)
    {
        case SF_ST_IDLE:
            return SPIFL_STAT_ACT;

        case SF_ST_BUSY:
            return SPIFL_STAT_ACT | SPIFL_STAT_BSY;

        case SF_ST_FBUSY:
            if (sfi_cmd_r(SF_CMD_RDSTAT) & SF_STAT_MASK_WIP)
                return SPIFL_STAT_ACT | SPIFL_STAT_BSY;

            else
            {
                sf_state = SF_ST_IDLE;
                return SPIFL_STAT_ACT;
            }

        case SF_ST_ERR:
            sf_state = SF_ST_IDLE;
            sfi_cs_off();
            return SPIFL_STAT_ACT | SPIFL_STAT_ERR;

        default:
            return SPIFL_STAT_NULL;
    }
}

// Execute SF command
void sf_command(UBYTE cmd)
{
    cmd &= SPIFL_MASK_CMD;

    if (cmd == SPIFL_CMD_DONE)
    {
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

// Write SF control
void sf_ctrl(UBYTE data)
{
    if (data & SPIFL_MASK_EN)
    // SF enabled
    {
        if (sf_state == SF_ST_INACT)
        // enable SFI
            sfi_enable();

        else
        // execute command
            sf_command(data);
    }

    else
        // disable SFI
        sfi_disable();   // enable JTAG
}

UBYTE spi_flash_read(UBYTE index)
{
    switch (index)
    {
        case SPIFL_REG_DATA:
            // SF data
            sf_addr++;
            return sfi_recv();

        case SPIFL_REG_STAT:
            // SF status
            return sf_status();

        case SPIFL_REG_A0:
            // SF low addr
            return (UBYTE)(sf_addr);
        break;

        case SPIFL_REG_A1:
            // SF high addr
            return (UBYTE)(sf_addr >> 8);

        case SPIFL_REG_A2:
            // SF ext addr
            return (UBYTE)(sf_addr >> 16);

        case SPIFL_REG_VER:
            // SF version
            return SPIFL_VER;
    }

    return 0xFF;
}

void spi_flash_write(UBYTE index, UBYTE data)
{
    switch (index)
    {
        case SPIFL_REG_DATA:
            // SF data
            sf_addr++;
            sfi_send(data);
        break;

        case SPIFL_REG_CTRL:
            // SF control
            sf_ctrl(data);
        break;

        case SPIFL_REG_EXTSW:
            // select another Glu Ext register
            ext_type_gluk = data;
        break;

        case SPIFL_REG_A0:
            // SF low addr
            sf_addr &= 0xFFFFFF00;
            sf_addr |= data;
        break;

        case SPIFL_REG_A1:
            // SF high addr
            sf_addr &= 0xFFFF00FF;
            sf_addr |= (ULONG)data << 8;
        break;

        case SPIFL_REG_A2:
            // SF ext addr
            sf_addr &= 0xFF00FFFF;
            sf_addr |= (ULONG)data << 16;
        break;
    }
}
