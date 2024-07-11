# ESP32 SPI WiFi

## Description

The module is based on ESP32-S2 and provides high-speed WiFi access via SPI bus.

The key-features are:

- 4-wire 1-bit SPI interface, Slave Mode.
- Up to 14Mbit/s data transfer speed.
- DMA capable transfers.

## Programming

### Communication protocol

The SPI Slave communication protocol provides a few types of transactions: ***READ_REGS***, ***WRITE_REGS***, ***READ_DATA***, ***WRITE_DATA***.

#### READ_REGS

This type of transaction allows reading a number of bytes from the register memory. It can be used to read a register value or a status.

##### Scenario

| Step | Action                                   |
| ---- | ---------------------------------------- |
| 1    | Assert SPI_CS.                           |
| 2    | Write SPI_DATA with **0x02**.            |
| 3    | Write SPI_DATA with register number.     |
| 4    | Write SPI_DATA with **two** dummy bytes. |
| 5    | Read one or more bytes from SPI_DATA.    |
| 6    | De-assert SPI_CS.                        |

##### Read 1-byte register in C

```c
__sfr __at 0x57 SPI_DATA;
__sfr __at 0x77 SPI_CTRL;
#define SPI_ESP_CS_ON  0x0B
#define SPI_ESP_CS_OFF 0x03

u8 esp_rd_reg(u8 reg_num)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = 0x02;
  SPI_DATA = reg_num;
  SPI_DATA; // dummy read means write with 0xFF
  SPI_DATA;
  u8 rc = SPI_DATA;
  SPI_CTRL = SPI_ESP_CS_OFF;

  return rc;
}
```

#### WRITE_REGS

This type of transaction allows writing a number of bytes in the register memory. Generally, the only reasonable way to write registers is to execute command.

**ATTENTION!**
Please notice, that each WRITE_REGS transaction triggers the command execution. Thus, all arguments **must** be transmitted in the same transaction along with the command.

##### Scenario

| Step | Action                                  |
| ---- | --------------------------------------- |
| 1    | Assert SPI_CS.                          |
| 2    | Write SPI_DATA with **0x01**.           |
| 3    | Write SPI_DATA with register number.    |
| 4    | Write SPI_DATA with **one** dummy byte. |
| 5    | Write SPI_DATA with one or more bytes.  |
| 6    | De-assert SPI_CS.                       |

##### Write 1-byte register in C

The following code snippet is just an example not to use. Please refer to 'Write command' snippet for more practical case.

```c
__sfr __at 0x57 SPI_DATA;
__sfr __at 0x77 SPI_CTRL;
#define SPI_ESP_CS_ON  0x0B
#define SPI_ESP_CS_OFF 0x03

void esp_rd_reg(u8 reg_num, u8 reg_value)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = 0x01;
  SPI_DATA = reg_num;
  SPI_DATA; // dummy read means write with 0xFF
  SPI_DATA = reg_value;
  SPI_CTRL = SPI_ESP_CS_OFF;
}
```
#### READ_DATA

The transaction must only be launched when STATUS register contains DATA_S2M status.

The number of bytes to transfer must be read from DATA_SIZE register before the transaction.

##### Scenario

| Step | Action                                                       |
| ---- | ------------------------------------------------------------ |
| 1    | Assert SPI_CS.                                               |
| 2    | Write SPI_DATA with **0x04**.                                |
| 4    | Write SPI_DATA with **three** dummy bytes.                   |
| 5    | Read one or more bytes from SPI_DATA.                        |
| 6    | De-assert SPI_CS.                                            |
| 7    | Assert SPI_CS.                                               |
| 8    | Write SPI_DATA with **0x08**. (!!! Doesn't work for S3. *To do: add DMA_END command.*) |
| 9    | De-assert SPI_CS.                                            |

##### Read a data array in C

```c
__sfr __at 0x57 SPI_DATA;
__sfr __at 0x77 SPI_CTRL;
#define SPI_ESP_CS_ON  0x0B
#define SPI_ESP_CS_OFF 0x03

void esp_recv(u8 *addr, u16 num)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = 0x04;
  SPI_DATA; // dummy read means write with 0xFF
  SPI_DATA;
  SPI_DATA;
  while (num--) *addr++ = SPI_DATA;
  SPI_CTRL = SPI_ESP_CS_OFF;
  
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = 0x08;	// !!! doesn't work for S3. To do: add a DMA_END example.
  SPI_CTRL = SPI_ESP_CS_OFF;
}
```

#### WRITE_DATA

The transaction must only be launched when STATUS register contains DATA_M2S status.

The number of bytes to transfer must be read from DATA_SIZE register before the transaction.

**ATTENTION!**
When writing data, all transactions **must** be truncated to 4 bytes. If we need to write 1 byte only, we must write 4 bytes and so on.

*To do: add code example.*

### Registers

All registers are read/writeable from both SPI Master via SPI and Slave firmware. At a low level, they're a shared memory cells inside ESP32 Slave.

#### Register summary

| Address, HEX | Size  | Name      | Description                                                  |
| ------------ | ----- | --------- | ------------------------------------------------------------ |
| 00           | 1     | EXT_STAT  | This register contains a code of the extended status or an error. |
| 04           | 4     | DATA_SIZE | When DMA transfer is prepared, this indicates the number of bytes to transfer, both for . |
| 08           | 1     | COMMAND   | Command. Set by Master.                                      |
| 09           | 1     | STATUS    | Status of device. Set by Slave while the command is being executed or after its execution. Must be zeroed by Master in the same transaction as the command issued, to ensure the correct synchronization. |
| 0A           | 0..62 | PARAMS    | If required, must be set in the same transaction as the command issued. |

#### STATUS Register

| Code, HEX | Name     | Description                                                  |
| --------- | -------- | ------------------------------------------------------------ |
| 00        | IDLE     | No status. Must be set along every command issued.           |
| 01        | ERROR    | Error happened. Read EXT_STAT register to get the error code. |
| 02        | READY    | Device is ready for a new command.                           |
| 03        | BUSY     | Device is busy executing a command.                          |
| 04        | DATA_M2S | Device is ready to receive data from host.                   |
| 05        | DATA_S2M | Device is ready to send data to host.                        |

#### EXT_STAT Register

| Code, HEX | Name  | Description                 |
| --------- | ----- | --------------------------- |
| FE        | RESET | Is set when reset happened. |

### Commands

#### Command summary

**Data** - indicated if and extra data send/receive is expected: **R** - data is being read from SPI Slave, **W** - data is being written into the Slave.

| Code, HEX | Params size | Data | Name     | Description                                                  |
| --------- | ----------- | ---- | -------- | ------------------------------------------------------------ |
| 00        | 0           | -    | NOP      | No operation. Issuing this command has no effect.            |
| 01        | 0           | R    | GET_INFO | Get device info string.                                      |
| 02        | 0           | -    | BREAK    | Break an operation. <...>                                    |
| 03        | 0           | R    | GET_CAPS | Get device capabilities.                                     |
| 10        | 0           | W    | AP_NAME  | Set WiFi AP name. 0-terminated string, 32 bytes max.         |
| 11        | 0           | W    | AP_PWD   | Set WiFi AP password. 0-terminated string, 32 bytes max.     |
| 12        | 0           | -    | AP_CONN  | Connect to AP. If another AP is connected, it will be dropped. |
| 13        | 0           | -    | AP_DISCN | Disconnect to AP.                                            |
| 14        | 0           | -    | AP_SCAN  | Scan WiFi APs.                                               |
| 15        | 0           | R    | GET_AP   | Get scanned APs.                                             |
| D0        | 0           | -    | DMA_END  | End DMA transaction. (A workaround for S3 CMD8 bug).         |
| EE        | 0           | -    | RESET    | Reset device. To be issued in the case of an unpredicted behavior. The reset takes <TBD> milliseconds to complete. The STATUS register after the reset must contain READY value. The EXT_STAT register must contain RESET value. |
| F0        | 2           | R    | GET_RND  | Generate random numbers array.<br />Params: (u16) Length of array. |
| F1        | 0           | R    | TEST2    | Generate tonnel DL for FT812.                                |
| F2        | 0           | R    | TEST3    | Generate 3D model DL for FT812.                              |





