
from pyftdi.spi import SpiController, SpiIOError
from pyftdi.ftdi import Ftdi as ft
from  binascii import hexlify

# ---------------------------------------------------------------------

def spix(d):
  a = spi.exchange(d, duplex=True)
  print(' '.join(['{:02x}'.format(b) for b in a]))
  return a

def rd_reg8(reg):
# wr ESP_SPI_CMD_RD_REGS
# wr <reg>
# dummy
# rd <data>
  buf = b'\x02'
  buf = buf + reg.to_bytes(1, 'little')
  buf = buf + b'\x00\x00'
  a = spix(buf)
  return a[3]

def rd_reg32(reg):
# wr ESP_SPI_CMD_RD_REGS
# wr <reg>
# dummy
# rd <data>
  buf = b'\x02'
  buf = buf + reg.to_bytes(1, 'little')
  buf = buf + b'\x00\x00\x00\x00\x00'
  a = spix(buf)
  return int.from_bytes(a[3:7], 'little')

def rd_dma(len):
# wr ESP_SPI_CMD_RD_DMA
# dummy
# dummy
# rd <data> ...
  buf = b'\x04'
  a = spi.exchange(buf, len + 3, duplex=True)
  return a[3:]

def wr_cmd(cmd):
# wr ESP_SPI_CMD_WR_REGS
# wr REG_CMD
# dummy
# wr <cmd>
# wr ESP_ST_IDLE
  buf = b'\x01\x08\x00'
  buf = buf + cmd.to_bytes(1, 'little')
  buf = buf + b'\x00'
  a = spix(buf)

def wait_data():
  i = 20
  while (i):
    if (rd_reg8(0x09) == 0x05):  # ESP_ST_DATA_S2M
      break
    i -= 1

# ---------------------------------------------------------------------

ft.show_devices()

spi = SpiController()
spi.configure('ftdi://ftdi:232h:0:1/1')

spi = spi.get_port(cs=0, freq=14000000, mode=0)

wr_cmd(0x01)          # Send GET_INFO command
wait_data()           # Wait for data
len = rd_reg32(0x04)  # Read data length
print(rd_dma(len))    # Read info string
wr_cmd(0xD0)          # Stop DMA transaction
