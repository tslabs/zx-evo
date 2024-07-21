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
  SPI_DATA;             // dummy
  SPI_DATA;
  u8 rc = SPI_DATA;
  SPI_CTRL = SPI_ESP_CS_OFF;

  return rc;
}
```



#### WRITE_REGS

This type of transaction allows writing a number of bytes in the register memory. Not used except for command issuing.
Each WRITE_REGS transaction triggers the command execution. Thus, all arguments **must** be transmitted in the same transaction along with the command.

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
  SPI_DATA;              // dummy
  SPI_DATA = reg_value;
  SPI_CTRL = SPI_ESP_CS_OFF;
}
```



#### WRITE COMMAND

This is a most used WRITE_REGS case, which triggers a command execution.

##### Write command in C

The following code snippet is just an example not to use. Please refer to 'Write command' snippet for more practical case.

```c
__sfr __at 0x57 SPI_DATA;
__sfr __at 0x77 SPI_CTRL;
#define SPI_ESP_CS_ON  0x0B
#define SPI_ESP_CS_OFF 0x03

void esp_cmd(u8 cmd)
{
  SPI_CTRL = SPI_ESP_CS_ON;
  SPI_DATA = 0x01;
  SPI_DATA = 0x00;          // ESP_REG_COMMAND
  SPI_DATA;                 // dummy
  SPI_DATA = cmd;           // Command
  SPI_DATA = 0x00;          // Reset status
  SPI_CTRL = SPI_ESP_CS_OFF;
}
```



#### READ_DATA

The transaction must only be launched when STATUS register contains DATA_S2M status.

The number of bytes to transfer must be read from xx_SIZE register before the transaction.



##### Scenario

| Step | Action                                     |
| ---- | ------------------------------------------ |
| 1    | Assert SPI_CS.                             |
| 2    | Write SPI_DATA with **0x04**.              |
| 4    | Write SPI_DATA with **three** dummy bytes. |
| 5    | Read one or more bytes from SPI_DATA.      |
| 6    | De-assert SPI_CS.                          |
| 7    | Execute `esp_cmd(DATA_END);`               |



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
  esp_cmd(ESP_CMD_DATA_END);
}
```



#### WRITE_DATA

The transaction must only be launched when STATUS register contains DATA_M2S status.

The number of bytes to transfer must be read or written from/to xx_SIZE register before the transaction.

**ATTENTION!**
When writing data, all transactions **must** be truncated to 4 bytes. If we need to write 1 byte only, we must write 4 bytes and so on.

*To do: add code example.*



### Registers

All registers are read/writeable from both SPI Master via SPI and Slave firmware. At a low level, they're a shared memory cells inside ESP32 Slave. The total number of registers is 64.

Common registers are COMMAND and STATUS. Registers with addresses 2-63 are used by the ESP32 firmware for different functions depending on a command.



#### Register summary

| Name       | Register address, HEX | Size  | Description                                                  |
| ---------- | ------------ | ----- | ------------------------------------------------------------ |
| COMMAND    | 00           | 1     | Command. Set by Master.                                      |
| STATUS     | 01           | 1     | Status of device. Set by Slave while the command is being executed or after its execution. Must be zeroed by Master in the same transaction as the command issued, to ensure the correct synchronization. |
| PARAM/RESP | 02           | 0..61 | If written, **must** be sent in the same transaction as the command issued.<br />See parameter/response list for individual commands. |



#### STATUS register

| Name     | Code, HEX | Description                                                  |
| -------- | --------- | ------------------------------------------------------------ |
| IDLE     | 00        | No status. Must be set by Master along every command issued. |
| BUSY     | 01        | Device is busy executing a command.                          |
| READY    | 02        | Device has completed command execution and is ready for a new command. |
| ERROR    | 03        | Error happened. Use GET_ERROR command to get the error code. |
| DATA_M2S | 04        | Device is ready to receive data from host.                   |
| DATA_S2M | 05        | Device is ready to send data to host.                        |



#### ERROR register

If a command has been completed with an error, the register 02 automatically indicates error code.

| Name  | Register address, HEX | Size | Description |
| ----- | ------------ | ---- | ----------- |
| ERROR | 02           | 1    | Error.      |



The following table contains error codes.

| Name            | Code, HEX | Description                                  |
| --------------- | --------- | -------------------------------------------- |
| NONE            | 00        | No error.                                    |
| WRONG_PARAMETER | 01        | Wrong parameter.                             |
| INVALID_COMMAND | 02        | Invalid command.                             |
| WRONG_STATE     | 03        | Command cannot be executed in current state. |
| AP_NOT_CONN     | 10        | Cannot connect AP.                           |
| RESET           | FE        | Is set when reset happened.                  |



### Commands

#### Command summary

| Name             | Code, HEX | Description                                                  |
| ---------------- | --------- | ------------------------------------------------------------ |
| NOP              | 00        | No operation. Issuing this command has no effect.            |
| GET_ERROR        |           | Get extended error info.                                     |
| GET_INFO         | 02        | Get device info string.                                      |
| GET_CHIP_INFO    | 03        | Get chip info.                                               |
| GET_CAPS         |           | Get device capabilities.                                     |
| GET_HEAP         |           | Get heap size.                                               |
| GET_NET_CAPS     |           | Get network capabilities.                                    |
| GET_NETSTATE     | 11        | Get network connection state.                                |
| WSCAN            | 12        | Scan Wi-Fi APs.                                              |
| SET_AP_NAME      | 13        | Set Wi-Fi AP name.                                           |
| SET_AP_PWD       | 14        | Set Wi-Fi AP password.                                       |
| GET_AP_NAME      | 15        | Get Wi-Fi AP name.                                           |
| AP_CONNECT       | 16        | Connect to selected AP.                                      |
| AP_DISCONNECT    | 17        | Disconnect from AP.                                          |
| SET_AUTOCONNECT  | 18        | Set AP auto-connect.                                         |
| GET_IP           | 19        | Get own Wi-Fi IP, net mask, net gateway.                     |
| RESOLVE_DNS      |           | Resolve DNS name.                                            |
| OPEN_UDP         |           |                                                              |
| CLOSE_UDP        |           |                                                              |
| STATUS_UDP       |           |                                                              |
| SEND_DATAGR      |           |                                                              |
| RECV_DATAGR      |           |                                                              |
| OPEN_TCP         |           |                                                              |
| CLOSE_TCP        |           |                                                              |
| STATUS_TCP       |           |                                                              |
| SEND_TCP         |           |                                                              |
| RECV_TCP         |           |                                                              |
|                  |           |                                                              |
| SET_WSCAN_PARAMS |           | Set parameters for Wi-Fi AP scan.                            |
|                  |           |                                                              |
| EXT_STATUS_TCP   |           | ?                                                            |
| CONF_AUTO_IP     |           | ?                                                            |
| CONF_IP          |           | ?                                                            |
|                  |           |                                                              |
| DATA_END         | D0        | End DATA transaction. (A workaround for S3 CMD8 bug).        |
| BREAK            | E0        | Break an operation.                                          |
| RESET            | EE        | Reset device. To be issued in the case of an unpredicted behavior. The reset takes <TBD> milliseconds to complete. The STATUS register after the reset must contain READY value. The EXT_STAT register must contain RESET value. |
| GET_RND          | F0        | Generate random numbers array.                               |
| TEST2            | F1        | Generate tunnel DL for FT812.                                |
| TEST3            | F2        | Generate 3D model DL for FT812.                              |



#### Miscellaneous commands



##### GET_INFO

Get device information string. The string is **not** 0-terminated.

**In:** none.

**Out:**

| Name      | Register address, HEX | Size | Description             |
| --------- | ------------ | ---- | -------------------------------- |
| INFO_SIZE | 02           | 1    | Info string size.                       |

[DATA] - Info string.



##### GET_CHIP_INFO

Get ESP module hardware parameters.

**In:** none.

**Out:** See table below.

| Name      | Register address, HEX | Size, bytes | Description                      |
| --------- | ------------ | ---- | -------------------------------- |
| CPU_TYPE   | 02 | 1 | ESP chip type, hex:<br />02 - ESP32-S3 |
| CPU_CORES  | 03 | 1 | Number of CPU cores.                   |
| CPU_FREQ   | 04 | 2 | CPU frequency, MHz.                    |
| CPU_SI_REV | 06 | 1 | Silicon revision.                      |
| WIFI       | 07 | 1 |                                        |
| BLE        | 08 | 1 |                                        |
| FLASH_SIZE | 09 | 1 | Flash size, MB.                        |
| FLASH_TYPE | 0A | 1 |                                        |
| FLASH_FREQ | 0B | 1 |                                        |
| PSRAM_SIZE | 0C | 1 | PSRAM size, MB.                        |
| PSRAM_TYPE | 0D | 1 |                                        |
| PSRAM_FREQ | 0E | 1 |                                        |



##### GET_CAPS

Read device capabilities.

**In:** none.

**Out:**

| Name | Bit  | Mask, Hex | Description |
| ---- | ---- | --------- | ----------- |
|      |      |           |             |



##### BREAK

Break execution of current command. If no command is being executed, this has no effect.

**In:** none.

**Out:** none.



#### Wi-Fi commands

Most Wi-Fi commands require time to complete. While in progress, the STATUS register indicates BUSY state. It changes to READY upon command completion if successful, or to ERROR, if not.



##### GET_NETSTATE

Get network state.

**In:** none.

**Out:** Network state. See the table below.

| Name     | Register address, HEX | Size | Description    |
| -------- | ------------ | ---- | -------------- |
| NETSTATE | 02           | 1    | Network state. |



**NETSTATE**

| Name            | Code, HEX | Description                        |
| --------------- | --------- | ---------------------------------- |
| NETWORK_CLOSED  | 00        | No network connection established. |
| NETWORK_OPENING | 01        | Network connection in progress.    |
| NETWORK_OPEN    | 02        | Network connection established.    |
| NETWORK_CLOSING | 03        | Network disconnection in progress. |
| NETWORK_UNKNOWN | FF        | Network state undefined.           |



##### WSCAN

Scan for Wi-Fi APs. Scanned APs are sorted by RSSI, highest first.
Requires STATUS register polling to check completion.

**In:**

| Name      | Register address, HEX | Size | Description             |
| --------- | ------------ | ---- | -------------------------------- |
| AP_MAX | 02           | 1  | Max number of AP to scan. |

**Out:**

| Name      | Register address, HEX | Size | Description             |
| --------- | ------------ | ---- | -------------------------------- |
| AP_TABLE_SIZE | 02           | 2    | AP table size.               |

[DATA] - scanned AP table. See the format below.

| Name                           | Size, bytes   | Description            |
| ------------------------------ | ------------- | ---------------------- |
| NUM_APS                        | 1             | Number of APs scanned. |
| AP_AUTH                        | 1             | AP authorization type. |
| AP_RSSI                        | 1             | AP RSSI, dB.           |
| AP_CHAN                        | 1             | AP primary channel     |
| AP_SSID_LEN                    | 1             | AP SSID length.        |
| AP_SSID                        | <AP_SSID_LEN> | AP SSID.               |
| ... repeat AP_xx for <NUM_APS> |               |                        |



**APn_AUTH**

| Name               | Value, HEX | Description |
| ------------------ | ---------- | ----------- |
| AP_AUTH_OPEN       | 00         | Open        |
| AUTH_WEP           | 01         |             |
| AUTH_WPA_PSK       | 02         |             |
| AUTH_WPA2_PSK      | 03         |             |
| AUTH_WPA_WPA2_PSK  | 04         |             |
| AUTH_EAP           | 05         |             |
| AUTH_WPA3_PSK      | 06         |             |
| AUTH_WPA2_WPA3_PSK | 07         |             |
| AUTH_WAPI_PSK      | 08         |             |
| AUTH_OWE           | 09         |             |
| AUTH_WPA3_ENT_192  | 0A         |             |
| AUTH_MAX           | 0B         |             |



##### SET_AP_NAME

Set AP name. 32 bytes max.

**In:**

| Name      | Register address, HEX | Size | Description             |
| --------- | ------------ | ---- | -------------------------------- |
| AP_NAME_LEN | 02           | 1    | AP name length. |
| AP_NAME | 03 | <AP_NAME_LEN> | AP name. |

**Out:** none.



##### SET_AP_PWD

Set AP password. 61 bytes max.

**In:**

| Name      | Register address, HEX | Size | Description             |
| --------- | ------------ | ---- | -------------------------------- |
| AP_PWD_LEN | 02           | 1    | AP password length. |
| AP_PWD | 03 | <AP_PWD_LEN> | AP password. |

**Out:** none.



##### GET_AP_NAME

Set AP name. 32 bytes max.

**In:** none.

**Out:**

| Name      | Register address, HEX | Size | Description             |
| --------- | ------------ | ---- | -------------------------------- |
| AP_NAME_LEN | 02           | 1    | AP name length. |
| AP_NAME | 03 | <AP_NAME_LEN> | AP name. |



##### **AP_**CONNECT

Connect to the selected AP.
Requires STATUS register polling to check completion.

**In:** none.

**Out:** none.



##### AP_DISCONNECT

Disconnect AP.
Requires STATUS register polling to check completion.

**In:** none.

**Out:** none.



##### SET_AUTOCONNECT

Enable or disable auto-connect.
SET_AP_NAME and SET_AP_PWD must be set before this command.

**In:**

| Name      | Register address, HEX | Size | Description             |
| --------- | ------------ | ---- | -------------------------------- |
| AUTOCONNECT | 02           | 1 | Mode:<br />0 - disabled.<br />1 - enabled. |

**Out:** none.



##### GET_IP

Get network IP addresses.

**In:** none.

**Out:**

| Name      | Register address, HEX | Size | Description             |
| --------- | ------------ | ---- | -------------------------------- |
| IP | 02           | 4    | Last DNS resolved IP address. |
| OWN_IP | 06          | 4    | Own IP address. |
| MASK   | 0A           | 4    | Net mask. |
| GATE   | 0E           | 4    | Net gateway. |



##### RESOLVE_DNS

Resolve IP address from domain name, which length must not be longer than 255 symbols.
Requires STATUS register polling to check completion.

**In:**

| Name      | Register address, HEX | Size | Description             |
| --------- | ------------ | ---- | -------------------------------- |
| DNAME_LEN | 02           | 1    | Domain name length. |
| DNAME | 03..3F      | 1..61 | Domain name. |

If DNAME_LEN<= 61, then it is sent using registers 03-3F. If it is longer than 61 symbols, send the symbols starting from 62 using DATA transaction (currently not supported).

**Out:**

Same as GET_IP.
