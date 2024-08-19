
#define SPI_ESP_CS_ON  0x13
#define SPI_ESP_CS_OFF 0x03

u32 esp_wait_busy(u32 timeout);

void esp_cmd(u8 cmd)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_WR_REGS;
  SPI_DATA = ESP_REG_COMMAND;
  SPI_DATA;                 // dummy
  SPI_DATA = cmd;           // Command
  SPI_DATA = ESP_ST_IDLE;   // Reset status
  SPI_CTRL = SPI_ESP_CS_OFF;
}

void esp_cmdd(u8 cmd, void *data, u8 len)
{
  u8 *d = (u8*)data;

  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_WR_REGS;
  SPI_DATA = ESP_REG_COMMAND;
  SPI_DATA;                       // dummy
  SPI_DATA = cmd;                 // Command
  SPI_DATA = ESP_ST_IDLE;         // Reset status
  SPI_DATA = len;                 // Length
  while (len--) SPI_DATA = *d++;  // Data
  SPI_CTRL = SPI_ESP_CS_OFF;
}

void esp_cmdp1(u8 cmd, u8 par)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_WR_REGS;
  SPI_DATA = ESP_REG_COMMAND;
  SPI_DATA;                       // dummy
  SPI_DATA = cmd;                 // Command
  SPI_DATA = ESP_ST_IDLE;         // Reset status
  SPI_DATA = par;                 // Parameter
  SPI_CTRL = SPI_ESP_CS_OFF;
}

void esp_cmdp(u8 cmd, void *par, u8 len)
{
  u8 *d = (u8*)par;

  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_WR_REGS;
  SPI_DATA = ESP_REG_COMMAND;
  SPI_DATA;                       // dummy
  SPI_DATA = cmd;                 // Command
  SPI_DATA = ESP_ST_IDLE;         // Reset status
  while (len--) SPI_DATA = *d++;  // Parameters
  SPI_CTRL = SPI_ESP_CS_OFF;
}

u8 esp_rd_reg8(u8 reg)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_RD_REGS;
  SPI_DATA = reg;
  SPI_DATA;                 // dummy
  SPI_DATA;                 // dummy
  u8 rc = SPI_DATA;
  SPI_CTRL = SPI_ESP_CS_OFF;

  return rc;
}

u32 esp_rd_reg32(u8 reg)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_RD_REGS;
  SPI_DATA = reg;
  SPI_DATA;                 // dummy
  SPI_DATA;                 // dummy
  u32 rc = SPI_DATA;
  rc |= (u32)SPI_DATA << 8;
  rc |= (u32)SPI_DATA << 16;
  rc |= (u32)SPI_DATA << 24;
  SPI_CTRL = SPI_ESP_CS_OFF;

  return rc;
}

void esp_rd_regs(u8 reg, void *data, u8 size)
{
  u8 *d = (u8*)data;

  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_RD_REGS;
  SPI_DATA = reg;
  SPI_DATA;                 // dummy
  SPI_DATA;                 // dummy
  while (size--) *d++ = SPI_DATA;
  SPI_CTRL = SPI_ESP_CS_OFF;
}

void esp_wr_reg8(u8 reg, u8 data)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_WR_REGS;
  SPI_DATA = reg;
  SPI_DATA;                 // dummy
  SPI_DATA = data;
  SPI_CTRL = SPI_ESP_CS_OFF;
}

void esp_wr_reg32(u8 reg, u32 data)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_WR_REGS;
  SPI_DATA = reg;
  SPI_DATA;                 // dummy
  SPI_DATA = (u8)data;
  SPI_DATA = (u8)(data >> 8);
  SPI_DATA = (u8)(data >> 16);
  SPI_DATA = (u8)(data >> 24);
  SPI_CTRL = SPI_ESP_CS_OFF;
}

void esp_wr_end()
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_W_END;
  SPI_DATA;                 // dummy
  SPI_CTRL = SPI_ESP_CS_OFF;
}

void esp_send(u8 *addr, u16 num)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_WR_DATA;
  SPI_DATA;                     // dummy
  SPI_DATA;                     // dummy
  while (num--) SPI_DATA = *addr++;
  SPI_CTRL = SPI_ESP_CS_OFF;

  esp_wr_end();
  esp_wait_busy(10000);
}

void esp_send_dma(u16 a, u8 p, u16 l, u16 n)
{
  
  TS_DMASADDRL = (u8)a;
  TS_DMASADDRH = (u8)((u16)a >> 8);
  TS_DMASADDRX = p;
  TS_DMALEN = (u8)((l >> 1) - 1);
  TS_DMANUM = (u8)(n - 1);

  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_WR_DATA;
  SPI_DATA;                     // dummy
  SPI_DATA;                     // dummy
  TS_DMACTR = TS_DMA_RAM_SPI;
  while (TS_DMASTATUS & 0x80);
  SPI_CTRL = SPI_ESP_CS_OFF;

  esp_wr_end();
  esp_wait_busy(10000);
}

void esp_rd_end()
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_R_END;
  SPI_DATA;                 // dummy
  SPI_CTRL = SPI_ESP_CS_OFF;
}

void esp_recv(u8 *addr, u16 num)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_RD_DATA;
  SPI_DATA;                     // dummy
  SPI_DATA;                     // dummy
  SPI_DATA;                     // dummy
  while (num--) *addr++ = SPI_DATA;
  SPI_CTRL = SPI_ESP_CS_OFF;

  esp_rd_end();
  esp_wait_busy(10000);
}

void esp_recv_dma(u16 a, u8 p, u16 l, u16 n)
{
  TS_DMADADDRL = (u8)a;
  TS_DMADADDRH = (u8)((u16)a >> 8);
  TS_DMADADDRX = p;
  TS_DMALEN = (u8)((l >> 1) - 1);
  TS_DMANUM = (u8)(n - 1);

  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_RD_DATA;
  SPI_DATA;                     // dummy
  SPI_DATA;                     // dummy
  SPI_DATA;                     // dummy
  TS_DMACTR = TS_DMA_SPI_RAM;
  while (TS_DMASTATUS & 0x80);
  SPI_CTRL = SPI_ESP_CS_OFF;

  esp_rd_end();
  esp_wait_busy(10000);
}

u32 esp_wait_status(u8 status, u32 timeout)
{
  while (--timeout) if (esp_rd_reg8(ESP_REG_STATUS) == status) break;

  return timeout;
}

u32 esp_wait_busy(u32 timeout)
{
  while (--timeout) if (esp_rd_reg8(ESP_REG_STATUS) != ESP_ST_BUSY) break;

  return timeout;
}

int esp_create_obj(u32 size, u8 type)
{
  esp_wr_reg32(ESP_REG_DATA_SIZE, size);
  esp_wr_reg8(ESP_REG_OBJ_TYPE, type);
  esp_cmd(ESP_CMD_MAKE_OBJECT);
  esp_wait_busy(2000);
  // +++ check error

  return esp_rd_reg8(ESP_REG_OBJ_HANDLE);
}
