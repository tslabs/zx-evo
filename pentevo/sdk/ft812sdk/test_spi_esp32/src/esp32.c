
#define SPI_ESP_CS_ON  0x13
#define SPI_ESP_CS_OFF 0x03

void esp_cmd(u8 cmd)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_WR_REGS;          // HD Slave: write cmd packet
  SPI_DATA = ESP_REG_COMMAND;
  SPI_DATA;                 // dummy
  SPI_DATA = cmd;           // Command
  SPI_DATA = ESP_ST_IDLE;   // Reset status
  SPI_CTRL = SPI_ESP_CS_OFF;
}

void esp_cmdd(u8 cmd, void *parm, u8 len)
{
  u8 *d = (u8*)parm;
  
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_WR_REGS;          // HD Slave: write cmd packet
  SPI_DATA = ESP_REG_COMMAND;
  SPI_DATA;                 // dummy
  SPI_DATA = cmd;           // Command
  SPI_DATA = ESP_ST_IDLE;   // Reset status
  SPI_DATA = len;           // Length
  while (len--) SPI_DATA = *d++;
  SPI_CTRL = SPI_ESP_CS_OFF;
}

void esp_cmdp(u8 cmd, void *parm, u8 len)
{
  u8 *d = (u8*)parm;
  
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_WR_REGS;          // HD Slave: write cmd packet
  SPI_DATA = ESP_REG_COMMAND;
  SPI_DATA;                 // dummy
  SPI_DATA = cmd;           // Command
  SPI_DATA = ESP_ST_IDLE;   // Reset status
  while (len--) SPI_DATA = *d++;
  SPI_CTRL = SPI_ESP_CS_OFF;
}

u8 esp_rd_reg(u8 reg)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_RD_REGS;          // HD Slave: read Shared Regs
  SPI_DATA = reg;
  SPI_DATA;                 // dummy
  SPI_DATA;                 // dummy
  u8 rc = SPI_DATA;
  SPI_CTRL = SPI_ESP_CS_OFF;

  return rc;
}

void esp_rd_regs(u8 reg, void *data, u8 size)
{
  u8 *d = (u8*)data;
  
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_RD_REGS;          // HD Slave: read Shared Regs
  SPI_DATA = reg;
  SPI_DATA;                 // dummy
  SPI_DATA;                 // dummy
  while (size--) *d++ = SPI_DATA;
  SPI_CTRL = SPI_ESP_CS_OFF;
}

u32 esp_wait_status(u8 status, u32 timeout)
{
  TS_SYSCONFIG = TS_SYS_ZCLK3_5 | TS_SYS_CACHEEN;

  // --- do not edit !!! ---
  while (--timeout)
    if (esp_rd_reg(ESP_REG_STATUS) == status) break;
  // --- T-states ---
  
  TS_SYSCONFIG = TS_SYS_ZCLK14 | TS_SYS_CACHEEN;
  
  return timeout;
}

u32 esp_wait_busy(u32 timeout)
{
  TS_SYSCONFIG = TS_SYS_ZCLK3_5 | TS_SYS_CACHEEN;

  // --- do not edit !!! ---
  while (--timeout)
    if (esp_rd_reg(ESP_REG_STATUS) != ESP_ST_BUSY) break;
  // --- T-states ---
  
  TS_SYSCONFIG = TS_SYS_ZCLK14 | TS_SYS_CACHEEN;
  
  return timeout;
}

void ts_set_dma_daddr_p(u16 a, u8 p)
{
  TS_DMADADDRL = (u8)a;
  TS_DMADADDRH = (u8)(a >> 8);
  TS_DMADADDRX = p;
}

void ts_set_dma_size(u16 l, u16 n)
{
  TS_DMALEN = (u8)((l >> 1) - 1);
  TS_DMANUM = (u8)(n - 1);
}

void esp_recv_dma(u16 a, u8 p, u16 l, u16 n)
{
  ts_set_dma_daddr_p(a, p);
  ts_set_dma_size(l, n);
  
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_RD_DATA;              // HD Slave: read via DMA
  SPI_DATA;                     // dummy
  SPI_DATA;                     // dummy
  SPI_DATA;                     // dummy
  TS_DMACTR = TS_DMA_SPI_RAM;
  while (TS_DMASTATUS & 0x80);
  SPI_CTRL = SPI_ESP_CS_OFF;
  
  esp_cmd(ESP_CMD_DATA_END);
  
  // SPI_CTRL = SPI_ESP_CS_ON;
  // SPI_DATA = 0x08;              // HD Slave: end DMA receive
  // SPI_CTRL = SPI_ESP_CS_OFF;
}

void esp_recv(u8 *addr, u16 num)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_RD_DATA;              // HD Slave: read via DMA
  SPI_DATA;                     // dummy
  SPI_DATA;                     // dummy
  SPI_DATA;                     // dummy
  while (num--) *addr++ = SPI_DATA;
  SPI_CTRL = SPI_ESP_CS_OFF;
  
  esp_cmd(ESP_CMD_DATA_END);
  
  // SPI_CTRL = SPI_ESP_CS_ON;
  // SPI_DATA = 0x08;              // HD Slave: end DMA receive
  // SPI_CTRL = SPI_ESP_CS_OFF;
}
