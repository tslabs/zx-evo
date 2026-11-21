# ESP32 SPI WiFi (ZiFi32)

## Description

The module is an ESP32-S3 (8 MB PSRAM, 16 MB flash) on the SPI bus of the host. It is an SPI slave that
provides Wi-Fi, network clients (HTTP(S), Gopher), memory objects, audio players (XM / MOD / S3M, SFX)
and the loading of ELF libraries.

- 4-wire 1-bit SPI, slave mode (ESP-IDF `spi_slave_hd` driver: shared registers + DMA buffers).
- Data transfers by DMA, up to 16 KB (`DMA_BUF_SIZE`) per transfer.

The command, register, status and object type codes are defined in `src/main/esp_spi_defs.h`; this
document follows it. The reference host code is `pentevo/sdk/ft812sdk/lib/esp32/esp32.c` (Z80, SDCC)
and `test/rpi/esp_spi.py` (Python).

## SPI transactions

The host talks to the slave with six SPI commands (the first byte of a transaction):

| Name     | Code | Description                                       |
| -------- | ---- | ------------------------------------------------- |
| WR_REGS  | 01   | Write the shared registers. Triggers a command.   |
| RD_REGS  | 02   | Read the shared registers.                        |
| WR_DATA  | 03   | Write the DMA buffer (DATA_M2S).                  |
| RD_DATA  | 04   | Read the DMA buffer (DATA_S2M).                   |
| W_END    | 07   | End of a WR_DATA transfer.                        |
| R_END    | 08   | End of a RD_DATA transfer.                        |

The examples are for TS-Conf: `SPI_DATA` is port `#57`, `SPI_CTRL` port `#77`; `#13` selects the ESP
(bit 4), `#03` deselects it. A read of `SPI_DATA` returns the byte of the previous exchange and starts
the next one, hence the extra dummy reads.

### RD_REGS

| Step | Action                                     |
| ---- | ------------------------------------------ |
| 1    | Assert CS.                                 |
| 2    | Write **0x02**.                            |
| 3    | Write the register number.                 |
| 4    | Two dummy reads.                           |
| 5    | Read one or more bytes (registers in a row). |
| 6    | De-assert CS.                              |

```c
u8 esp_rd_reg8(u8 reg)
{
  SPI_CTRL = SPI_ESP_CS_ON;      // 0x13
  SPI_DATA = ESP_SPI_CMD_RD_REGS;
  SPI_DATA = reg;
  SPI_DATA;                      // dummy
  SPI_DATA;                      // dummy
  u8 rc = SPI_DATA;
  SPI_CTRL = SPI_ESP_CS_OFF;     // 0x03
  return rc;
}
```

Multi-byte registers are little-endian.

### WR_REGS

| Step | Action                                     |
| ---- | ------------------------------------------ |
| 1    | Assert CS.                                 |
| 2    | Write **0x01**.                            |
| 3    | Write the register number.                 |
| 4    | One dummy read.                            |
| 5    | Write one or more bytes.                   |
| 6    | De-assert CS.                              |

**Every WR_REGS transaction makes the slave look at the COMMAND register.** If COMMAND is not 0, the
command is executed and COMMAND is cleared. Parameters can be written before by separate WR_REGS
transactions that leave COMMAND at 0 (the registers 02..), or in the same transaction after the
command.

A command is issued by writing COMMAND and STATUS = IDLE (0) in one transaction. The host then polls
STATUS: the slave sets BUSY at once and the final status when done.

```c
void esp_cmd(u8 cmd)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_WR_REGS;
  SPI_DATA = ESP_REG_COMMAND;
  SPI_DATA;                      // dummy
  SPI_DATA = cmd;                // COMMAND
  SPI_DATA = ESP_ST_IDLE;        // STATUS
  SPI_CTRL = SPI_ESP_CS_OFF;
}
```

**Polling.** Right after the command STATUS may still read IDLE (the slave has not run the command
yet). Wait for "not IDLE and not BUSY" (or for a specific status), not only for "not BUSY".

**While a command is running (BUSY), a new command is ignored.** Wait for the end of the previous
command first.

### WR_DATA (host to slave)

Only when STATUS is DATA_M2S (03). The number of bytes is the one the command expects
(DATA_SIZE).

| Step | Action                                     |
| ---- | ------------------------------------------ |
| 1    | Assert CS.                                 |
| 2    | Write **0x03**, two dummy reads.           |
| 3    | Write the data.                            |
| 4    | De-assert CS.                              |
| 5    | W_END: assert CS, write **0x07**, one dummy read, de-assert CS. |
| 6    | Wait for "not DATA_M2S, not BUSY", then check READY.  |

```c
void esp_send(u8 *addr, u16 num)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_WR_DATA;
  SPI_DATA;                      // dummy
  SPI_DATA;                      // dummy
  while (num--) SPI_DATA = *addr++;
  SPI_CTRL = SPI_ESP_CS_OFF;
  esp_wr_end();                  // 0x07 + dummy
  esp_wait_busy(10000);
}
```

DMA transfers of the ESP32-S3 work in 4-byte units: keep the sizes a multiple of 4 where the command
allows it (the stream load commands require it).

### RD_DATA (slave to host)

Only when STATUS is DATA_S2M (04). Read DATA_SIZE first: it is the number of bytes to read.

| Step | Action                                     |
| ---- | ------------------------------------------ |
| 1    | Assert CS.                                 |
| 2    | Write **0x04**, three dummy reads.         |
| 3    | Read DATA_SIZE bytes.                      |
| 4    | De-assert CS.                              |
| 5    | R_END: assert CS, write **0x08**, one dummy read, de-assert CS. |
| 6    | Wait for "not DATA_S2M" (READY).           |

```c
void esp_recv(u8 *addr, u16 num)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = ESP_SPI_CMD_RD_DATA;
  SPI_DATA;                      // dummy
  SPI_DATA;                      // dummy
  SPI_DATA;                      // dummy
  while (num--) *addr++ = SPI_DATA;
  SPI_CTRL = SPI_ESP_CS_OFF;
  esp_rd_end();                  // 0x08 + dummy
  esp_wait_busy(10000);
}
```

## Registers

64 shared registers, read and written by both sides. 00 and 01 are common, the others depend on the
command.

| Name           | Address | Size | Description                                          |
| -------------- | ------- | ---- | ---------------------------------------------------- |
| COMMAND        | 00      | 1    | Command. Set by the host, cleared by the slave.      |
| STATUS         | 01      | 1    | Status or error code. Set to IDLE by the host along with each command. |
| STRING_TYPE    | 02      | 1    | GET_INFO_STR: string type.                           |
| STRING_SIZE    | 02      | 1    | String commands: length.                             |
| STRING_DATA    | 03      | ..61 | String commands: the string (not 0-terminated).      |
| API            | 02      | 1    | GET_VER: API version.                                |
| FEAT           | 03      | 2    | GET_VER: feature version.                            |
| VER0..VER2     | 05..07  | 1    | GET_VER: product version.                            |
| OBJ_TYPE       | 02      | 1    | Object type.                                         |
| DATA_SIZE      | 03      | 4    | Data size.                                           |
| DATA_OFFSET    | 07      | 4    | Data offset.                                         |
| OBJ_HANDLE     | 0B      | 1    | Object handle.                                       |
| FUNC           | 02      | 1    | RUN_FUNCx: function number.                          |
| OPT            | 03      | 1    | LOAD_ELF_OPT: options.                               |
| ARG            | 03      | 4    | RUN_FUNCx: argument.                                 |
| RETVAL         | 07      | 4    | RUN_FUNCx: return value.                             |
| ARR1..3_HANDLE | 0C..0E  | 1    | RUN_FUNCx: array object handles.                     |
| LIB_HANDLE     | 0F      | 1    | Library handle.                                      |
| NETSTATE       | 02      | 1    | GET_NETSTATE: network state.                         |
| IP             | 2C      | 4    | GET_IP: last resolved IP.                            |
| OWN_IP         | 30      | 4    | GET_IP: own IP.                                      |
| MASK           | 34      | 4    | GET_IP: net mask.                                    |
| GATE           | 38      | 4    | GET_IP: gateway.                                     |
| EXEC_TIME      | 3C      | 4    | Execution time of the last command, us.              |
| SFX_*          | 02..27  |      | SFX commands, see below.                             |

## Status and error codes

| Name            | Code  | Description                                         |
| --------------- | ----- | --------------------------------------------------- |
| IDLE            | 00    | Set by the host along with a command.               |
| READY           | 01    | Command completed.                                  |
| BUSY            | 02    | Command running (or initializing).                  |
| DATA_M2S        | 03    | Ready to receive data from the host (WR_DATA).      |
| DATA_S2M        | 04    | Ready to send data to the host (RD_DATA).           |
| RESET           | 7E    | After a reset, ready.                               |

Error codes (the command completed with an error):

| Name             | Code | Name              | Code |
| ---------------- | ---- | ----------------- | ---- |
| INV_COMMAND      | 80   | INV_ARG_HANDLE    | 8D   |
| INV_PARAM        | 81   | INV_SRC_HANDLE    | 8E   |
| INV_STATE        | 82   | INV_DST_HANDLE    | 8F   |
| INV_STR_LEN      | 83   | INV_XM            | 90   |
| INV_SIZE         | 84   | INV_LIB           | 91   |
| INV_OBJ_TYPE     | 85   | INV_ELF           | 92   |
| INV_ARR_NUM      | 86   | INV_HST           | 93   |
| INV_HANDLE       | 87   | INV_ZIP           | 94   |
| INV_XM_HANDLE    | 88   | INV_MOD           | 95   |
| INV_LIB_HANDLE   | 89   | INV_MOD_HANDLE    | 96   |
| INV_ELF_HANDLE   | 8A   | INV_S3M           | 97   |
| INV_HST_HANDLE   | 8B   | INV_S3M_HANDLE    | 98   |
| INV_BSS_HANDLE   | 8C   | OUT_OF_MEMORY     | 99   |
|                  |      | OUT_OF_HANDLES    | 9A   |
|                  |      | OBJ_NOT_DELETED   | 9B   |

Network codes:

| Name / use           | Code | Description                                            |
| -------------------- | ---- | ------------------------------------------------------ |
| AP_NOT_CONNECTED     | B0   | AP_CONNECT failed. Stream start: no reader task.       |
| NET_BUSY             | B1   | WSCAN while busy. GOPHER: DNS lookup failed.           |
| -                    | B2   | Network clients: socket / connect / HTTP open failed.  |
| -                    | B3   | HTTP status not 2xx.                                   |
| -                    | B4   | Empty response.                                        |
| -                    | A0   | Network clients: no memory for the receive buffer.     |

## Commands

| Name                | Code | In                                           | Out                                       |
| ------------------- | ---- | -------------------------------------------- | ----------------------------------------- |
| NOP                 | 00   |                                              |                                           |
| GET_INFO_STR        | 01   | STRING_TYPE                                  | STRING_SIZE, STRING_DATA                  |
| GET_VER             | 02   |                                              | API, FEAT, VER0..2                        |
| GET_NETSTATE        | 11   |                                              | NETSTATE                                  |
| WSCAN               | 12   |                                              | AP table by RD_DATA                       |
| SET_AP_NAME         | 13   | STRING_SIZE (1..32), STRING_DATA             |                                           |
| SET_AP_PWD          | 14   | STRING_SIZE (0..61), STRING_DATA             |                                           |
| AP_CONNECT          | 16   |                                              |                                           |
| AP_DISCONNECT       | 17   |                                              |                                           |
| SET_URL             | 18   | DATA_SIZE (1..1023); the URL by WR_DATA      |                                           |
| GET_IP              | 19   |                                              | IP, OWN_IP, MASK, GATE                    |
| HTTP_GET            | 20   | URL (SET_URL)                                | OBJ_HANDLE, DATA_SIZE                     |
| HTTPS_GET           | 21   | URL                                          | OBJ_HANDLE, DATA_SIZE                     |
| GOPHER_GET          | 22   | URL                                          | OBJ_HANDLE, DATA_SIZE                     |
| HTTP_STREAM_START   | 23   | URL                                          | DATA_SIZE (total, FFFFFFFF if unknown)    |
| HTTPS_STREAM_START  | 24   | URL                                          | DATA_SIZE (total)                         |
| GOPHER_STREAM_START | 25   | URL                                          | DATA_SIZE (FFFFFFFF)                      |
| STREAM_READ         | 26   |                                              | DATA_SIZE (chunk, up to 512) by RD_DATA; 0 at the end |
| STREAM_CLOSE        | 27   |                                              |                                           |
| TRACK_INIT          | A0   | OBJ_HANDLE (XM / MOD / S3M object)           | the object becomes XMC / MDC / S3C        |
| TRACK_PLAY          | A1   | OBJ_HANDLE (XMC / MDC / S3C)                 |                                           |
| TRACK_STOP          | A2   |                                              |                                           |
| TRACK_RESET         | A3   | OBJ_HANDLE (XMC / MDC / S3C)                 | stop and rewind                           |
| XM_STREAM_LOAD      | AB   | DATA_SIZE (chunk, aligned), DATA_OFFSET (total on the first chunk, 0 later) | DATA_OFFSET, DATA_SIZE, OBJ_HANDLE |
| MOD_STREAM_LOAD     | AC   | as XM_STREAM_LOAD                            | as XM_STREAM_LOAD                         |
| S3M_STREAM_LOAD     | AD   | as XM_STREAM_LOAD                            | as XM_STREAM_LOAD                         |
| SFX_PLAY            | B0   | OBJ_HANDLE (WAV), SFX_GROUP                  | SFX_CHANNEL                               |
| SFX_PLAY_EX         | B1   | OBJ_HANDLE, SFX_GROUP, SFX_VOLUME, SFX_PAN, SFX_PITCH | SFX_CHANNEL                      |
| SFX_STOP            | B2   | SFX_CHANNEL                                  |                                           |
| SFX_SET_PARAMS      | B3   | SFX_CHANNEL, SFX_VOLUME, SFX_PAN, SFX_PITCH  |                                           |
| SFX_SET_VOLUME      | B4   | SFX_VOLUME (master)                          |                                           |
| SFX_GET_STATE       | B5   | SFX_CHANNEL                                  | SFX_STATE_*                               |
| SFX_STOP_GROUP      | B6   | SFX_GROUP                                    |                                           |
| LOAD_ELF            | D0   | OBJ_HANDLE (ELF object)                      | LIB_HANDLE                                |
| LOAD_ELF_OPT        | D1   | OBJ_HANDLE, OPT                              | LIB_HANDLE                                |
| RUN_FUNC0..3        | D2..D5 | LIB_HANDLE, FUNC, ARG, ARR1..3_HANDLE (by the number) | RETVAL                      |
| MAKE_OBJECT         | E0   | DATA_SIZE, OBJ_TYPE                          | OBJ_HANDLE, DATA_OFFSET (0), DATA_SIZE (first chunk) |
| WRITE_OBJECT        | E1   | OBJ_HANDLE, DATA_OFFSET, DATA_SIZE (1..16384); data by WR_DATA | DATA_OFFSET (next)  |
| READ_OBJECT         | E2   | OBJ_HANDLE, DATA_OFFSET, DATA_SIZE (1..16384); data by RD_DATA | DATA_OFFSET (next)  |
| DELETE_OBJECT       | E3   | OBJ_HANDLE                                   |                                           |
| KILL_OBJECTS        | E4   |                                              | all objects deleted                       |
| REBOOT              | ED   |                                              | restart (STATUS RESET when up)            |
| RESET               | EE   |                                              | restart (STATUS RESET when up)            |
| BREAK               | EF   |                                              | abort a tracker stream load               |
| GET_RND             | F0   | DATA_SIZE (up to 16384)                      | random bytes by RD_DATA                   |
| DEHST               | F1   | OBJ_HANDLE (HST object), DATA_SIZE (packed, up to 64 KB), OBJ_TYPE (new) | DATA_OFFSET, DATA_SIZE |
| UNZIP               | F2   | OBJ_HANDLE (ZIP object), DATA_SIZE (packed), OBJ_TYPE (new) | DATA_OFFSET, DATA_SIZE  |
| STREAM_UNZIP        | F3   | OBJ_TYPE, DATA_SIZE (chunk; 0 = reset); data by WR_DATA | OBJ_HANDLE                  |

Codes not listed (03 GET_CHIP_INFO, A4..AA, C0, E5..E7, ...) are reserved and answer INV_COMMAND.

### Notes on commands

- **GET_INFO_STR**: STRING_TYPE 0 copyright, 1 API string, 2 build, 3 ESP-IDF version.
- **WSCAN**: when done, the slave sets DATA_S2M; read DATA_SIZE and the table by RD_DATA:
  `NUM_APS (1)`, then per AP `AUTH (1)`, `RSSI (1, signed)`, `CHAN (1)`, `SSID_LEN (1)`, `SSID`.
  AUTH is the `wifi_auth_mode_t` of ESP-IDF (0 open, 1 WEP, 2 WPA, 3 WPA2, 4 WPA/WPA2, ...).
  At most 64 APs.
- **AP_CONNECT** uses SET_AP_NAME / SET_AP_PWD and can take seconds (association and DHCP); poll
  STATUS. GET_NETSTATE: 00 closed, 01 opening, 02 open, 03 closing, FF unknown.
- **SET_URL**: the slave answers DATA_M2S; send the URL (DATA_SIZE bytes) by WR_DATA + W_END.
- **xx_GET**: the whole response (up to 2 MB) goes into a new DATA object; read it with READ_OBJECT.
- **Streams**: after xx_STREAM_START, repeat STREAM_READ: wait for DATA_S2M, read DATA_SIZE bytes by
  RD_DATA, until DATA_SIZE is 0 (the end) with status READY. Then STREAM_CLOSE. One stream at a time.
- **Objects**: MAKE_OBJECT returns the handle; WRITE_OBJECT / READ_OBJECT move up to 16 KB per call
  (DATA_OFFSET + DATA_SIZE must stay inside the object). Objects live until DELETE_OBJECT or
  KILL_OBJECTS (or a reset): a program should delete what it made.
- **Tracker**: make an object of type XM / MOD / S3M with the module, TRACK_INIT turns it into a
  context (XMC / MDC / S3C, same handle), TRACK_PLAY plays it. The stream load commands build the
  context chunk by chunk without the whole module in memory.

### Object types

| Name  | Code | Memory | Description                       |
| ----- | ---- | ------ | --------------------------------- |
| NONE  | 00   |        | Free handle.                      |
| DATA  | 01   | PSRAM  | Data.                             |
| DATAF | 02   | SRAM   | Data in the internal RAM (fast).  |
| ELF   | 03   | PSRAM  | ELF file.                         |
| LIB   | 04   | -      | Loaded library.                   |
| XM    | 05   | PSRAM  | XM module.                        |
| WAV   | 06   | PSRAM  | WAV sound (SFX).                  |
| HST   | 07   | PSRAM  | Hrust packed data.                |
| ZIP   | 08   | PSRAM  | Zlib packed data (u32 unpacked size + zlib stream). |
| XMC   | 09   | PSRAM  | XM player context.                |
| MOD   | 0A   | PSRAM  | MOD module.                       |
| MDC   | 0B   | PSRAM  | MOD player context.               |
| S3M   | 0C   | PSRAM  | S3M module.                       |
| S3C   | 0D   | PSRAM  | S3M player context.               |
| MBC   | 0E   | PSRAM  | Context.                          |

### LOAD_ELF_OPT options

| Name        | Value | Description                       |
| ----------- | ----- | --------------------------------- |
| DATA_SRAM   | 01    | `.data` in the internal RAM.      |
| RODATA_SRAM | 02    | `.rodata` in the internal RAM.    |
| BSS_SRAM    | 04    | `.bss` in the internal RAM.       |

## Versioning

GET_VER returns API (incompatible protocol changes), FEAT (a counter of feature additions) and the
product version. A host should check API and the FEAT it needs.
