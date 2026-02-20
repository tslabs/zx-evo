
import time
import spidev


# -------------------- registers --------------------
ESP_REG_COMMAND     = 0x00  # 1 byte
ESP_REG_STATUS      = 0x01  # 1 byte

# String commands
ESP_REG_STRING_TYPE = 0x02  # 1 byte
ESP_REG_STRING_SIZE = 0x02  # 1 byte
ESP_REG_STRING_DATA = 0x03  # byte array

# Object/data commands
ESP_REG_OBJ_TYPE    = 0x02  # 1 byte
ESP_REG_DATA_SIZE   = 0x03  # 4 bytes
ESP_REG_DATA_OFFSET = 0x07  # 4 bytes
ESP_REG_OBJ_HANDLE  = 0x0B  # 1 byte

# Lib commands
ESP_REG_FUNC        = 0x02
ESP_REG_OPT         = 0x03
ESP_REG_ARG         = 0x03  # 4 bytes
ESP_REG_RETVAL      = 0x07  # 4 bytes
ESP_REG_ARR1_HANDLE = 0x0C
ESP_REG_ARR2_HANDLE = 0x0D
ESP_REG_ARR3_HANDLE = 0x0E
ESP_REG_LIB_HANDLE  = 0x0F

# Network
ESP_REG_NETSTATE    = 0x02
ESP_REG_IP          = 0x2C
ESP_REG_OWN_IP      = 0x30
ESP_REG_MASK        = 0x34
ESP_REG_GATE        = 0x38

# Stats
ESP_EXEC_TIME       = 0x3C

# -------------------- commands --------------------
ESP_CMD_NOP          = 0x00
ESP_CMD_GET_INFO_STR = 0x01
ESP_CMD_GET_NETSTATE = 0x11
ESP_CMD_WSCAN        = 0x12
ESP_CMD_SET_AP_NAME  = 0x13
ESP_CMD_SET_AP_PWD   = 0x14
ESP_CMD_AP_CONNECT   = 0x16
ESP_CMD_GET_IP       = 0x19
ESP_CMD_HTTP_GET     = 0x20

ESP_CMD_XM_INIT      = 0xA1
ESP_CMD_XM_PLAY      = 0xA3
ESP_CMD_XM_STOP      = 0xA4

ESP_CMD_LOAD_ELF     = 0xD0
ESP_CMD_RUN_FUNC0    = 0xD2
ESP_CMD_RUN_FUNC1    = 0xD3
ESP_CMD_RUN_FUNC2    = 0xD4
ESP_CMD_RUN_FUNC3    = 0xD5

ESP_CMD_MAKE_OBJECT   = 0xE0
ESP_CMD_WRITE_OBJECT  = 0xE1
ESP_CMD_READ_OBJECT   = 0xE2
ESP_CMD_DELETE_OBJECT = 0xE3
ESP_CMD_KILL_OBJECTS  = 0xE4
ESP_CMD_REBOOT        = 0xED
ESP_CMD_RESET         = 0xEE

ESP_CMD_GET_RND       = 0xF0
ESP_CMD_DEHST         = 0xF1
ESP_CMD_UNZIP         = 0xF2
ESP_CMD_STREAM_UNZIP  = 0xF3

# -------------------- options --------------------
ESP_OPT_DATA_SRAM   = 0x01
ESP_OPT_RODATA_SRAM = 0x02
ESP_OPT_BSS_SRAM    = 0x04

# -------------------- status --------------------
ESP_ST_IDLE     = 0x00
ESP_ST_READY    = 0x01
ESP_ST_BUSY     = 0x02
ESP_ST_DATA_M2S = 0x03
ESP_ST_DATA_S2M = 0x04
ESP_ST_RESET    = 0x7E

# -------------------- object types --------------------
OBJ_TYPE_NONE  = 0
OBJ_TYPE_DATA  = 1
OBJ_TYPE_DATAF = 2
OBJ_TYPE_ELF   = 3
OBJ_TYPE_LIB   = 4
OBJ_TYPE_XM    = 5
OBJ_TYPE_WAV   = 6
OBJ_TYPE_HST   = 7
OBJ_TYPE_ZIP   = 8
OBJ_TYPE_XMC   = 9

# -------------------- spi slave commands --------------------
ESP_SPI_CMD_WR_REGS = 0x01
ESP_SPI_CMD_RD_REGS = 0x02
ESP_SPI_CMD_WR_DATA = 0x03
ESP_SPI_CMD_RD_DATA = 0x04
ESP_SPI_CMD_W_END   = 0x07
ESP_SPI_CMD_R_END   = 0x08

# -------------------- transfer limits --------------------
DMA_BUF_SIZE = 4000

# -------------------- helpers --------------------
def le32(v: int) -> bytes:
  return bytes([(v >> 0) & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF, (v >> 24) & 0xFF])

def u32_from_le(b: bytes) -> int:
  return b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24)


class EspSpi:
  def __init__(self, bus=0, dev=0, speed_hz=14_000_000, mode=0):
    self.spi = spidev.SpiDev()
    self.spi.open(int(bus), int(dev))  # spidev0.0 => 0,0
    self.spi.max_speed_hz = int(speed_hz)
    self.spi.mode = int(mode)
    self.spi.bits_per_word = 8

  def rd_regs(self, reg: int, n: int) -> bytes:
    tx = [ESP_SPI_CMD_RD_REGS, reg & 0xFF, 0x00] + [0x00] * n
    rx = self.spi.xfer2(tx)
    return bytes(rx[3:3 + n])

  def wr_regs(self, reg: int, data: bytes) -> None:
    tx = [ESP_SPI_CMD_WR_REGS, reg & 0xFF, 0x00] + list(data)
    self.spi.xfer2(tx)

  def rd_reg8(self, reg: int) -> int:
    return self.rd_regs(reg, 1)[0]

  def rd_reg32(self, reg: int) -> int:
    b = self.rd_regs(reg, 4)
    return u32_from_le(b)

  def wr_reg8(self, reg: int, v: int) -> None:
    self.wr_regs(reg, bytes([v & 0xFF]))

  def wr_reg32(self, reg: int, v: int) -> None:
    self.wr_regs(reg, le32(v))

  def cmd(self, cmd: int) -> None:
    self.spi.xfer2([ESP_SPI_CMD_WR_REGS, ESP_REG_COMMAND, 0x00, cmd & 0xFF, ESP_ST_IDLE])

  def wait_busy(self, timeout_s: float) -> bool:
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
      if self.rd_reg8(ESP_REG_STATUS) != ESP_ST_BUSY:
        return True
    return False

  def wait_status(self, want: int, timeout_s: float) -> bool:
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
      if self.rd_reg8(ESP_REG_STATUS) == want:
        return True
    return False

  def wait_not_status(self, not_want: int, timeout_s: float) -> bool:
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
      if self.rd_reg8(ESP_REG_STATUS) != not_want:
        return True
    return False

  # WR_DATA for one chunk (<= DMA_BUF_SIZE) + W_END + wait_busy
  def send_chunk(self, data: bytes, frag: int = 4096) -> None:
    i = 0
    first = min(len(data), frag)
    self.spi.xfer2([ESP_SPI_CMD_WR_DATA, 0x00, 0x00] + list(data[:first]))
    i = first

    while i < len(data):
      part = data[i:i + frag]
      self.spi.xfer2(list(part))
      i += len(part)

    self.spi.xfer2([ESP_SPI_CMD_W_END, 0x00])
    self.wait_busy(10.0)

  def make_object(self, size: int, obj_type: int, timeout_s: float = 5.0) -> int:
    self.wr_reg8(ESP_REG_OBJ_TYPE, obj_type)
    self.wr_reg32(ESP_REG_DATA_SIZE, size)
    self.cmd(ESP_CMD_MAKE_OBJECT)

    if not self.wait_busy(timeout_s):
      st = self.rd_reg8(ESP_REG_STATUS)
      raise RuntimeError(f"MAKE_OBJECT timeout, status=0x{st:02X}")

    st = self.rd_reg8(ESP_REG_STATUS)
    if st != ESP_ST_READY:
      raise RuntimeError(f"MAKE_OBJECT non-READY, status=0x{st:02X}")

    return self.rd_reg8(ESP_REG_OBJ_HANDLE)

  # CHUNKED WRITE_OBJECT
  def write_object_stream(self, handle: int, data: bytes, offset0: int = 0, chunk_sz: int = DMA_BUF_SIZE) -> None:
    self.wr_reg8(ESP_REG_OBJ_HANDLE, handle)
    self.wr_reg32(ESP_REG_DATA_OFFSET, offset0)

    pos = 0
    total = len(data)

    while pos < total:
      chunk = data[pos:pos + chunk_sz]

      self.wr_reg32(ESP_REG_DATA_SIZE, len(chunk))
      self.cmd(ESP_CMD_WRITE_OBJECT)

      if not self.wait_status(ESP_ST_DATA_M2S, 2.0):
        st = self.rd_reg8(ESP_REG_STATUS)
        raise RuntimeError(f"WRITE_OBJECT wait DATA_M2S timeout, status=0x{st:02X}")

      self.send_chunk(chunk)

      if not self.wait_not_status(ESP_ST_DATA_M2S, 2.0):
        st = self.rd_reg8(ESP_REG_STATUS)
        raise RuntimeError(f"WRITE_OBJECT wait end DATA_M2S timeout, status=0x{st:02X}")

      st = self.rd_reg8(ESP_REG_STATUS)
      if st != ESP_ST_READY:
        raise RuntimeError(f"WRITE_OBJECT non-READY, status=0x{st:02X}")

      pos += len(chunk)

  def delete_object(self, handle: int, timeout_s: float = 5.0) -> None:
    self.wr_reg8(ESP_REG_OBJ_HANDLE, handle)
    self.cmd(ESP_CMD_DELETE_OBJECT)

    if not self.wait_busy(timeout_s):
      st = self.rd_reg8(ESP_REG_STATUS)
      raise RuntimeError(f"DELETE_OBJECT timeout, status=0x{st:02X}")

    st = self.rd_reg8(ESP_REG_STATUS)
    if st != ESP_ST_READY:
      raise RuntimeError(f"DELETE_OBJECT non-READY, status=0x{st:02X}")
