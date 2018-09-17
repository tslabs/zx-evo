# ZiFi

## Registers

| Address | Access Mode | Name | Description |
| ------- | ----------- | ---- | ----------- |
|0x00EF..0xBFEF| RW     |DR    |Data Register.|
|0xC0EF   | R           |ZIFR  |Input ZiFi FIFO Used Register. 0 - input FIFO is empty, 255 - input FIFO is full.|
|0xC1EF   | R           |ZOFR  |Output ZiFi FIFO Free Register. 0 - output FIFO is full, 255 - output FIFO is empty.|
|0xC2EF   | R           |RIFR  |Input RS-232 FIFO Used Register. 0 - input FIFO is empty, 255 - input FIFO is full.|
|0xC3EF   | R           |ROFR  |Output RS-232 FIFO Free Register. 0 - output FIFO is full, 255 - output FIFO is empty.|
|0xC4EF   | W           |IMR   |Interrupt Mask Register.|
|0xC4EF   | R           |ISR   |Interrupt Source Register.|
|0xC5EF   | RW          |ZIBTR |Input Buffer Threshold Register. Number of bytes in input buffer which triggers the interrupt if enabled. (Default 0x80)|
|0xC6EF   | RW          |ZITOR |Input Buffer Timeout Register. Number of milliseconds after last byte arrived in input buffer when interrupt is triggered if enabled. (Default 0x01)|
|0xC7EF   | W           |CR    |Command Register. Command set depends on API mode selected.|
|0xC7EF   | R           |ER    |Error Register - command execution result code. Depends on command issued.|
|0xC8EF   | RW          |RIBTR |Input Buffer Threshold Register. Number of bytes in input buffer which triggers the interrupt if enabled. (Default 0x80)|
|0xC9EF   | RW          |RITOR |Input Buffer Timeout Register. Number of milliseconds after last byte arrived in input buffer when interrupt is triggered if enabled. (Default 0x01)|

### Data Register.

Receives byte from input FIFO. Input FIFO must not be empty (xIFR > 0).
Sends byte into output FIFO. Output FIFO must not be full (xOFR > 0).
**Serves both ZiFi and RS-232. To select required device read correspondent FIFO register.**
E.g if ZIFR was read, ZiFi data will be read from DR.

### Command register.

#### Commands applicable in all modes

| Code | Command | Description |
| ---- | ---- | ---- |
| 000000oi | CLRFIFO | Clear FIFOs  i: 1 - clear input FIFO, o: 1 - clear output FIFO. |
| 11110mmm | API     | Set API mode or disable API. |
| 11111111 | VER     | Get Version. ER returns highest supported API version. 0xFF - no API available. |

#### Set API command

| Value | Description |
| ----- | ----------- |
| 0     | API disabled |
| 1     | Transparent mode: all data is sent/received to/from external UART directly |
| 2..7  | Reserved |

### Error register.

#### Responses applicable in all modes

| Code | Response | Description |
| ---- | ---- | ---- |
| 0x00 | OK   | No error         |
| 0xFF | REJ  | Command rejected |

### Interrupt Mask Register.

Enables one-time trigger for particular interrupt source when correspondent bit written to 1.
Each interrupt event disables correspondent source, so must be written again to re-enable after interrupt routine has completed.
Works as '1' mask on individual bits. E.g. two writes with 0b01 and 0b10 are equal to one write with 0b11.

| Bit  | Trigger |
| ---- | ---- |
|  0   | Buffer threshold reached (ZIBTR).   |
|  1   | Buffer timeout reached (ZITOR).     |
|  2   | Buffer threshold reached (RIBTR).   |
|  3   | Buffer timeout reached (RITOR).     |

### Interrupt Source Register.

Returns interrupt source after the interrupt has occured. Each interrupt source is represented by a bit.
Automatically cleared after read.

| Bit  | Event |
| ---- | ---- |
|  0   | Buffer threshold reached (ZIBTR).   |
|  1   | Buffer timeout reached (ZITOR).     |
|  2   | Buffer threshold reached (RIBTR).   |
|  3   | Buffer timeout reached (RITOR).     |

## Initialization sequence

Until initialization sequence performed API is disabled and the only command recognized is 'Set API mode'.

- Send 'Set API mode = 1' command,
- Send 'Get Version' command,
- Read highest supported API mode from ER,
- Select desired API mode.

```assembly
zifi_init:
  ; select API mode 1
  ld bc, 0xC7EF
  ld a, 0xF1
  out (c), a
  ; get highest supported API mode
  ld a, 0xFF
  out (c), a
  in a, (c)
  ; select desired API mode, if need
  ld a, mode  ; possible values are 0xF2..0xF7
  out (c), a

; read 128 bytes from ZiFi
zifi_read:
  ; set B to number of bytes (0x80)
  ld bc, 0x80EF
  ld hl, buf
  inir
```
