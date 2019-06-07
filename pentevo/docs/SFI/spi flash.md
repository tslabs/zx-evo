# SPI Flash Interface

## Ports

ZX-Evo design provides extensions for GluClock (GC) such as EEPROM write, version readback etc. This includes access to the external SPI Flash Interface (SFI).

To enable SFI access GC register 0x0C must be written with value 0 to disable EEPROM access and enable other extensions. The extended GC functionality is selected via GC register 0xF0. The SFI has extension number 0x10 which must be first written for any following operations. The SFI then uses GC registers 0xF1..0xFF for its functionality.

GluClock registers:

| Z80 port | Function          |
| -------- | ----------------- |
| #DFF7    | GluClock Register |
| #BFF7    | GluClock Data     |

See GluClock manual for the details of GC ports access.

## Registers

| Name | Address | Mode | Description |
| ---- | ---- | ---- | ---- |
| EXTSW| 0xF0 | W | GluClock extension (0x10 for SFI) |
| CMD  | 0xF1 | W | Execute command |
| STAT | 0xF1 | R | Read status |
| A0   | 0xF2 | R/W | Set address LSB to be accessed in SPI Flash |
| A1   | 0xF3 | R/W | Set address HSB to be accessed in SPI Flash |
| A2   | 0xF4 | R/W | Set address MSB to be accessed in SPI Flash |
| DATA | 0xF8 | R/W | Read or write data from SPI Flash |
| VER  | 0xFF | R | Read SFI version |

STAT register returns current SFI state. See below.

Address must be set to A0..A2 registers before any command requiring addressing (READ, WRITE, ERSSEC). A0 stand for the less significant address byte and A2 for the most significant.

DATA register is used to send or receive data byte in data transfers.

VER register returns API version (currently 1).

## Commands

| Name | Code | Description |
| ---- | ---- | ---- |
| NOP | 0x00 | No command |
| ENA | 0x01 | Enable SFI, disable JTAG pins |
| DIS | 0x02 | Disable SFI, enable JTAG pins |
| END | 0x03 | End current command |
| ID | 0x04 | Read Flash chip ID |
| READ | 0x05 | Read Flash |
| WRITE | 0x06 | Write Flash |
| ERSBLK | 0x07 | Erase bulk |
| ERSSEC | 0x08 | Erase sector |

Commands are written to CMD register. It is not allowed to send any command when STAT register is not indicating IDLE.

Before any operations, ENA command must be issued to enable physical interface for the SPI Flash. It is not necessary to issue DIS command afterwards, unless JTAG pins are used.

Commands READ and WRITE start data transfer and must be terminated with END command.

After READ command has been issued, data can be transferred for unlimited number of bytes. The read address is incremented automatically after each byte read.

WRITE command can only write data within pages of 256 bytes. Start address can be any, but the data for one command cannot cross the 256 bytes boundary.

After the execution of WRITE, ERSBLK and ERSSEC commands (in a case of WRITE - after the subsequent END), STAT register must be polled while BUSY status asserted.

## Status

| Name | Code | Description |
| ---- | ---- | ---- |
| IDLE | 0x00 | Idle |
| BUSY | 0x01 | Busy |
| ERR | 0x02 | Error |

Current SFI status is read from the STAT register.

## Access scenario

To use SPI Flash Interface do the following steps.

| Step | Activity | Assembly pseudocode |
| ---- | ---- | ---- |
| 1 | Open GluClock ports | ```OUT #EFF7, 0x80 ``` |
| 2 | Disable EEPROM operations | ```OUT #DFF7, 0x0C : OUT #BFF7, 0 ``` |
| 3 | Select SFI | ```OUT #DFF7, 0xF0 : OUT #BFF7, 0x10 ``` |
| 4 | Enable SFI | ```OUT #DFF7, 0xF1 : OUT #BFF7, 0x01 ``` |
| 5 | Write/read SFI regs | ```OUT #DFF7, #F1..#FF : IN/OUT #BFF7 ``` |
| 6 | Deselect SFI | ```OUT #DFF7, 0xF0 : OUT #BFF7, 0 ``` |
| 7 | Close GluClock ports | ```OUT #EFF7, 0 ``` |


