
from pyftdi.spi import SpiController, SpiIOError
from pyftdi.ftdi import Ftdi
from  binascii import hexlify

Ftdi.show_devices()

spi = SpiController()
spi.configure('ftdi://ftdi:232h:0:1/1')

slave = spi.get_port(cs=0, freq=14000000, mode=0)

buf = b'\x01\x08\x00\x01\x00'   # write regs - reg cmd - dummy - get info - idle status
a = slave.exchange(buf, duplex=True)

buf = b'\x02\x09\x00\x00\x00'   # read regs - reg status - dummy - dummy - read reg
while (True):
  a = slave.exchange(buf, duplex=True)
  print(' '.join(['{:02x}'.format(b) for b in a]))
  if (a[3] == 0x05):  # data s2m
    break

buf = b'\x02\x04\x00\x00\x00\x00\x00\x00'   # read regs - reg status - dummy - dummy - read reg
a = slave.exchange(buf, duplex=True)
print(' '.join(['{:02x}'.format(b) for b in a]))
len = a[3]
buf = b'\x04'   # read data - reg status - dummy - dummy - read reg
a = slave.exchange(buf, len + 3, duplex=True)
print(a[3:])

buf = b'\x01\x08\x00\xD0\x00'   # write regs - reg cmd - dummy - dma end - idle status
a = slave.exchange(buf, duplex=True)
