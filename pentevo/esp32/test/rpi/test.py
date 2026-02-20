#!/usr/bin/env python3
import time
import spidev
from esp_spi import *


def hexdump(b: bytes, width: int = 16) -> str:
  lines = []
  for i in range(0, len(b), width):
    chunk = b[i:i+width]
    lines.append(f"{i:02X} " + " ".join(f"{x:02X}" for x in chunk))
  return "\n".join(lines)


def test_make_object_data(size_bytes: int = 1 * 1024 * 1024):
  esp = EspSpi()

  print(f"MAKE_OBJECT: {size_bytes} bytes, type DATA")

  print("Sending ESP_CMD_MAKE_OBJECT")
  esp.wr_reg8(ESP_REG_OBJ_TYPE, OBJ_TYPE_DATA)
  esp.wr_reg32(ESP_REG_DATA_SIZE, size_bytes)
  esp.cmd(ESP_CMD_MAKE_OBJECT)

  if not esp.wait_busy(2.0):
    st = esp.rd_reg8(ESP_REG_STATUS)
    raise RuntimeError(f"MAKE_OBJECT timeout, status=0x{st:02X}")

  st = esp.rd_reg8(ESP_REG_STATUS)
  if st != ESP_ST_READY:
    raise RuntimeError(f"MAKE_OBJECT returned non-READY status=0x{st:02X}")

  h = esp.rd_reg8(ESP_REG_OBJ_HANDLE)
  print(f"Handle: {h}")
  print("Object created")

  print("Sending ESP_CMD_DELETE_OBJECT")
  esp.wr_reg8(ESP_REG_OBJ_HANDLE, h)
  esp.cmd(ESP_CMD_DELETE_OBJECT)

  if not esp.wait_busy(2.0):
    st = esp.rd_reg8(ESP_REG_STATUS)
    raise RuntimeError(f"DELETE_OBJECT timeout, status=0x{st:02X}")

  st = esp.rd_reg8(ESP_REG_STATUS)
  if st != ESP_ST_READY:
    raise RuntimeError(f"DELETE_OBJECT returned non-READY status=0x{st:02X}")

  print("Object deleted")


def xm_load_and_play(path: str):
  esp = EspSpi(bus=0, dev=0, speed_hz=14_000_000, mode=0)

  with open(path, "rb") as f:
    xm = f.read()

  print(f"XM: {path}  size={len(xm)}")

  esp.cmd(ESP_CMD_XM_STOP)
  esp.wait_busy(1.0)

  # create XM object and upload
  h = esp.make_object(len(xm), OBJ_TYPE_XM)
  print(f"XM handle: {h}")

  esp.write_object_stream(h, xm, offset0=0, chunk_sz=DMA_BUF_SIZE)
  print("XM uploaded")

  # init + play
  esp.wr_reg8(ESP_REG_OBJ_HANDLE, h)
  esp.cmd(ESP_CMD_XM_INIT)
  esp.wait_busy(2.0)

  esp.wr_reg8(ESP_REG_OBJ_HANDLE, h)
  esp.cmd(ESP_CMD_XM_PLAY)
  esp.wait_busy(2.0)

  input("Press Enter to stop...")

  esp.cmd(ESP_CMD_XM_STOP)
  esp.wait_busy(2.0)

  esp.wr_reg8(ESP_REG_OBJ_HANDLE, h)
  esp.cmd(ESP_CMD_DELETE_OBJECT)
  esp.wait_busy(2.0)

  print("Done")


def main():
  dev = EspSpi(speed_hz=40_000_000, mode=0)

  b = dev.rd_regs(0x00, 64)
  print("Reg dump (0x00..0x3F):")
  print("    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F")
  for row in range(4):
    chunk = b[row*16:(row+1)*16]
    print(f"{row*16:02X}  " + " ".join(f"{x:02X}" for x in chunk))

  test_make_object_data()
  # xm_load_and_play("xm/astar.xm")
  # xm_load_and_play("xm/AT4RE - MagicTweak 4.01kg.xm")
  # xm_load_and_play("xm/future dreams.xm")
  xm_load_and_play("xm/av_out.xm")
  # xm_load_and_play("xm/alien_threat.xm")


if __name__ == "__main__":
  main()
