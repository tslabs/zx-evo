# ZX-Evolution GluClock Version Extension



​	ZX-Evolution provides extensions for GluClock (GC) such as EEPROM write, version readback, access to the external SPI Flash Interface.

​	The GC interface in ZX-Evolution is implemented as so-called 'wait-ports', meaning that Z80 is being held waiting, while data is transferred to/from external MCU.



## Ports

| Z80 port, HEX | Function |
| -------- | ----------------- |
| EFF7 | Bit 7: Access to GluClock (0 - disabled, 1 - enabled). |
| DFF7 | GluClock Register |
| BFF7 | GluClock Data |



## Access

​	To enable access to GC, bit 7 in #EFF7 port must be set to 1.

​	To enable access to the Extension register, GC register 0x0C must be written with value 0 to disable EEPROM access and enable other extensions. The extended GC functionality is selected via GC register 0xF0. The SFI has extension number 0x10 which must be first written for any following operations. The SFI then uses GC registers 0xF1..0xFF for its functionality.

| Step | Activity                   | Assembly pseudocode                                          |
| ---- | -------------------------- | ------------------------------------------------------------ |
| 1    | Open GluClock ports        | ```OUT #EFF7, 0x80 ```                                       |
| 2    | Disable EEPROM operations  | ```OUT #DFF7, 0x0C : OUT #BFF7, 0 ```                        |
| 3    | Select an extension        | ```OUT #DFF7, 0xF0 : OUT #BFF7, <extension_code> ```         |
| ..   | (Any extension operations) | (See the 'Access scenario' sub-sections for any particular extension.) |
| n    | Close GluClock ports       | ```OUT #EFF7, 0 ```                                          |



# Extensions description

| Name | Code, HEX | Description |
| ---- | ---- | ---- |
| CONF_VERSION | 00 | FPGA Configuration version |
| BOOTLOADER_VERSION | 01 | Bootloader version |
| PS2KEYBOARDS_LOG | 02 | PS2 keyboard log |
| RDCFG | 03 | Config bytes |
| CONFIG | 0E | [Configuration Interface](#config_link) |
| SPIFL | 10 | SPI Flash Interface |



## Configuration version

​	This allows to read the version of an FPGA configuration.



### Registers

| Name  | Address, HEX | Mode | Description                                         |
| ----- | ------------ | ---- | --------------------------------------------------- |
| EXTSW | F0           | W    | GluClock extension (0x00 for Configuration version) |
| DATA  | F0..FF       | R    | Version data                                        |

​	See the 'zxevo_base_configuration.pdf' document for details.



## Bootloader version

​	This allows to read the version of the AVR Bootloader.



### Registers

| Name  | Address, HEX | Mode | Description                                      |
| ----- | ------------ | ---- | ------------------------------------------------ |
| EXTSW | F0           | W    | GluClock extension (0x01 for Bootloader version) |
| DATA  | F0..FF       | R    | Version data                                     |

​	See the 'zxevo_base_configuration.pdf' document for details.



## PS/2 keyboard log

​	This extension is used to read the raw PS/2 keyboard log for keyboard polling.



### Registers

| Name  | Address, HEX | Mode | Description                                    |
| ----- | ------------ | ---- | ---------------------------------------------- |
| EXTSW | F0           | W    | GluClock extension (0x02 for PS2 keyboard log) |
| DATA  | F0..FF       | R    | PS2 keyboard log data                          |

​	See the 'zxevo_base_configuration.pdf' document for details.



## Config bytes

### Registers

| Name  | Address, HEX | Mode | Description                                |
| ----- | ------------ | ---- | ------------------------------------------ |
| EXTSW | F0           | W    | GluClock extension (0x03 for Config bytes) |
| CMD   | F0           | R    | Modes Register                             |

​	The only cell 0 is available for reading.



### Modes Register

​	The values for this register differ for TS-Config and BaseConf.



#### TS-Config

| Bit  | Name      | Description                |
| ---- | --------- | -------------------------- |
| 7    | Beeper_Mux[1] | (see below) |
| 6    | Floppy Swap   | 0 - Not swapped<br/>1 - Drives A/C swapped with B/D |
| 5    | (reserved) |                            |
| 4    | 50/60Hz       | 0 - 50Hz<br/>1 - 60Hz<br/>(Only used in a specialized build.) |
| 3    | Beeper_Mux[0] | 00 - Beep (D4)<br/>01 - Tape Out (D3)<br/>1x - Tape In |
| 2    | CAPS_LOCK | Caps Lock status on PS/2 keyboard |
| 1    | (reserved) |                            |
| 0    | VGA/TV | 0 - TV<br/>1 - VGA |



#### BaseConfig

| Bit  | Name      | Description                |
| ---- | --------- | -------------------------- |
| 7..6 | (reserved) |  |
| 5..4 | ULA Mode | 00 - Pentagon  (71680 clocks)<br/>01 - 60Hz<br/>10 - 48k (69888 clocks)<br/>11 - 128k (70908 clocks) |
| 3    | Beeper_Mux | 0 - Beep (D4)<br/>1 - Tape Out (D3) |
| 2    | CAPS_LOCK | Caps Lock status on PS/2 keyboard |
| 1    | (reserved) |                            |
| 0    | VGA/TV | 0 - TV<br/>1 - VGA |



## SPI Flash Interface

​	The SPI Flash Interface (SFI) allows access to an external SPI flash chip in 1-bit mode for reading, writing and booting the FPGA configurations.

### Hardware

​	The flash chip is supposed to be attached to AVR's JTAG interface pins using the following scheme.

(draw wiring here)



### Registers

| Name   | Address, HEX | Mode | Description                       |
| ------ | ------- | ---- | --------------------------------- |
| EXTSW  | F0    | W    | GluClock extension (0x10 for SFI) |
| CMD    | F1    | W    | Execute command                   |
| STAT   | F1    | R    | Status                            |
| ADDR   | F2    | W    | Address                           |
| PARAM  | F3    | W    | Parameters data                   |
| REPORT | F3    | R    | Report data                       |
| DATA   | F8    | R/W  | Flash read or write data |
| VER    | FF    | R    | SFI version                  |



#### Address

​	Address register (ADDR) must be written before any command requiring addressing (READ, WRITE, ERSSEC). Four bytes required, MSB goes first. Example: sequential writes 00 10 80 A0 will set address to 0x1080A0. The internal state of address is preserved between commands and incremented after each transferred byte.



#### Parameters data

​	Parameters register (PARAM) must be written before command execution, then command requires such.



#### Report data

​	Report must be read from REPORT register after the REPORT command.



#### Flash read or write data

​	DATA register is used to send or receive data byte in data transfers.



#### SFI version

​	VER register returns API version (currently 1).



#### Status

| Name | Code, HEX | Description |
| ---- | --------- | ----------- |
| IDLE | 00        | Idle        |
| BUSY | 01        | Busy        |
| ERR  | 02        | Error       |

​	Current SFI status is read from the STAT register.



### Commands

| Name   | Code, HEX | Description                          |
| ------ | --------- | ------------------------------------ |
| NOP    | 00      | No command                           |
| ENA    | 01      | Enable SFI, disable JTAG pins        |
| DIS    | 02      | Disable SFI, enable JTAG pins        |
| FINISH | 03      | Finish current Flash command (nCS is de-asserted) |
| ID     | 04      | Read Flash chip ID                   |
| READ   | 05      | Read Flash                           |
| WRITE  | 06      | Write Flash                          |
| ERSCHP | 07      | Erase chip                           |
| ERSSEC | 08      | Erase sector                         |
| BREAK  | 09      | Break                                |
| F_CHIP | 0A      | Format. Chip-wide erase, no check    |
| F_BLK  | 0B      | Format. Block-wide erase, no check   |
| F_CHK  | 0C      | Format. Block-wide erase, with check |
| BSLOAD | 0D      | Load bitstream from Flash         |
| REPORT | 0E      | Prepare report                      |
| DETECT | 0F | Detect SPI flash |

​	Commands are written to CMD register. It is not allowed to send any command when STAT register is **not** indicating IDLE.

​	Before any operations, ENA command must be issued to enable physical interface for the SPI Flash. It is not necessary to issue DIS command afterwards, unless JTAG pins are used.

​	Commands READ and WRITE start data transfer and must be terminated with FINISH command.

​	After READ command has been issued, data can be transferred for unlimited number of bytes. The read address is incremented automatically after each byte read.

​	WRITE command can only write data within pages of 256 bytes. Start address can be any, but the data for a single command cannot cross the 256 bytes boundary. After 256 (or less) bytes have been written the FINISHcommand must be executed.

​	After the execution of WRITE, ERSBLK and ERSSEC commands (in a case of WRITE - after the subsequent FINISH), STAT register must be polled while BUSY status asserted.



#### Prepare report

​	This command is used to get the detailed progress while long Flash operations are in progress.



##### Report tags

The [Binary Tagged Format (BTF)](#btf_link) is used for the report.

| Name        | Code, HEX | Data type | Description                   |
| ----------- | --------- | --------- | ----------------------------- |
| COMMAND     | 80        | uint8     | Current command               |
| PROGRESS    | 81        | uint8     | Command progress              |
| ADDRESS     | 82        | uint32    | Current processing address    |
| BLOCKS      | 83        | uint16    | Current processed blocks      |
| BLOCKS_GOOD | 84        | uint16    | Current processed good blocks |



#### Load bitstream from Flash

​	This command is used for 'hot' FPGA configuration loading from SPI Flash.

​	Before the command issued, the host must load bitstream parameters into the PARAM register. The following fields must be set.



##### Bitstream parameter tags

The [Binary Tagged Format (BTF)](#btf_link) is used for the parameter tags.

| Name    | Code, HEX | Data type | Description                                                  |
| ------- | --------- | --------- | ------------------------------------------------------------ |
| BSTREAM | 01        | string    | Bitstream filename                                           |
| ROM     | 02        | string    | ROM filename                                                 |
| ISBASE  | 03        | bool      | Is Baseconf<br/>This attribute is used to handle the BaseConf specific features correctly, e.g. Modes Register. |



### Access scenario

​	To use SPI Flash Interface, perform the following steps.

| Step | Activity                  | Assembly pseudocode                       |
| ---- | ------------------------- | ----------------------------------------- |
| 1    | Open GluClock ports       | ```OUT #EFF7, 0x80 ```                    |
| 2    | Disable EEPROM operations | ```OUT #DFF7, 0x0C : OUT #BFF7, 0 ```     |
| 3    | Select SFI                | ```OUT #DFF7, 0xF0 : OUT #BFF7, 0x10 ```  |
| 4    | Enable SFI                | ```OUT #DFF7, 0xF1 : OUT #BFF7, 0x01 ```  |
| 5    | Write/read SFI regs       | ```OUT #DFF7, #F1..#FF : IN/OUT #BFF7 ``` |
| 6    | Deselect SFI              | ```OUT #DFF7, 0xF0 : OUT #BFF7, 0 ```     |
| 7    | Close GluClock ports      | ```OUT #EFF7, 0 ```                       |



<a name="config_link"></a>

## Configuration Interface

### Registers

| Name        | Address, HEX | Mode | Description                                                  |
| ----------- | ------------ | ---- | ------------------------------------------------------------ |
| EXTSW       | F0           | W    | GluClock extension (0x0E for Configuration Interface)        |
| MODES_VIDEO | F1           | R/W  | Modes video                                                  |
| MODES_MISC  | F2           | R/W  | Modes miscellaneous                                          |
| HOTKEYS     | F3           | R/W  | Hotkey mapping                                               |
| PAD_MODE    | F4           | R/W  | Control pad modes                                            |
| PAD_KMAP0   | F5           | R/W  | Control pad key mapping for Joystick 1                       |
| PAD_KMAP1   | F6           | R/W  | Control pad key mapping for Joystick 2                       |
| PROTECT     | FE           | R/W  | Bit 7: Protection. Written to 1, disables certain functions until Reset. |
| COMMAND     | FF           | W    | Command                                                      |
| STATUS      | FF           | R    | Status                                                       |



#### Modes Video

| Bits | Name      | Description    | Values                                                       |
| ---- | --------- | -------------- | ------------------------------------------------------------ |
| 7    | VSYNC_POL | VSync polarity | 0 - negative<br/>1 - positive                                |
| 6    | HSYNC_POL | HSync polarity | 0 - negative<br/>1 - positive                                |
| 5..4 | ULA       | ULA type       | 00 - Pentagon  (71680 clocks)<br/>01 - 60Hz<br/>10 - 48k (69888 clocks)<br/>11 - 128k (70908 clocks) |
| 0    | TV_VGA    | TV/VGA         | 0 - negative<br/>1 - positive                                |



#### Modes miscellaneous

| Bits | Name        | Description | Values                                                 |
| ---- | ----------- | ----------- | ------------------------------------------------------ |
| 2..1 | BEEPER_MUX  | Beeper mux  | 00 - Beep (D4)<br/>01 - Tape Out (D3)<br/>1x - Tape In |
| 0    | FLOPPY_SWAP | Floppy swap | 0 - no swap<br/>1 - A/C <-> B/D                        |



#### Hotkey mapping

​	Must be loaded in a batch.

| Byte offset | Key                   |
| ----------- | --------------------- |
| 0           | TV/VGA toggle         |
| 1           | HSync polarity toggle |
| 2           | VSync polarity toggle |
| 3           | Floppy swap toggle    |
| 4           | Beeper mux toggle     |



##### Hotkey values

(TBD)



#### Control pad modes

##### Interface selection

| PAD_MODE[2:0] | ZX Keyboard | Joystick 1 | Joystick 2 |
| ------------- | ----------- | ---------- | ---------- |
| 00x           | on          | Kempston   | -          |
| 010           | on          | NES        | -          |
| 011           | on          | NES        | NES        |
| 100           | off         | SEGA       | -          |
| 101           | off         | SEGA       | SEGA       |
| 11x           | (reserved)  |            |            |



##### Alternative proposed mapping:

| Bits | Name        | Description          | Values    |
| ---- | ----------- | -------------------- | --------- |
| 7..6 | PAD_MAPPING | Joystick key mapping | see below |
| 5..3 | PAD1_MODE   | Joystick 2 mode      | see below |
| 2..0 | PAD0_MODE   | Joystick 1 mode      | see below |

| PADx_MODE | ZX Keyboard | Joystick 1                                            |
| --------- | ----------- | ----------------------------------------------------- |
| 000       | on          | Kempston for PAD0, none for PAD1                      |
| 001       | on          | NES                                                   |
| 010       | off         | SEGA 3-button                                         |
| 011       | off         | SEGA 6-button (unsupported, aliased to SEGA 3-button) |
| 1xx       | (reserved)  |                                                       |



##### Mapping

| PAD_MAPPING | Joystick 1          | Joystick 2          |
| ----------- | ------------------- | ------------------- |
| 00          | Kempston (port #1F) | Keys (port #FE)     |
| 01          | Keys (port #FE)     | Kempston (port #1F) |
| 10          | Keys (port #FE)     | Keys (port #FE)     |
| 11          | (reserved)          |                     |



#### Control pad key mappings

​	Must be loaded in a batch.

| Byte offset | SEGA  | NES    |
| ----------- | ----- | ------ |
| 0           | Right | Right  |
| 1           | Left  | Left   |
| 2           | Down  | Down   |
| 3           | Up    | Up     |
| 4           | B     | B      |
| 5           | C     | A      |
| 6           | A     | Select |
| 7           | Start | Start  |
| 8           | Mode  | -      |
| 9           | X     | -      |
| 10          | Y     | -      |
| 11          | Z     | -      |



##### Mapping values

| Value, HEX | Key   | Value, HEX | Key    | Value, HEX | Key  | Value, HEX | Key  | Value, HEX | Key  |
| ---------- | ----- | ---------- | ------ | ---------- | ---- | ---------- | ---- | ---------- | ---- |
| 00         | SPACE | 08         | SYMBOL | 10         | M    | 18         | N    | 20         | B    |
| 01         | ENTER | 09         | L      | 11         | K    | 19         | J    | 21         | H    |
| 02         | P     | 0A         | O      | 12         | I    | 1A         | U    | 22         | Y    |
| 03         | 0     | 0B         | 9      | 13         | 8    | 1B         | 7    | 23         | 6    |
| 04         | 1     | 0C         | 2      | 14         | 3    | 1C         | 4    | 24         | 5    |
| 05         | Q     | 0D         | W      | 15         | E    | 1D         | R    | 25         | T    |
| 06         | A     | 0E         | S      | 16         | D    | 1E         | F    | 26         | G    |
| 07         | CAPS  | 0F         | Z      | 17         | X    | 1F         | C    | 27         | V    |

​	Value 0xFF means no mapping for current key.

​	Bit 6 = 1 in a value means CAPS is pressed together with the key.

​	Bit 7 = 1 in a value means SYMBOL is pressed together with the key.



### Commands

| Command, HEX | Name         | Description                           |
| ------------ | ------------ | ------------------------------------- |
| F7           | REBOOT       | Reboot the AVR                        |
| FE           | REBOOT_FLASH | Reboot the AVR and flash from SD card |



<a name="btf_link"></a>

# Binary Tagged Format

​	The Binary Tagged Format (BTF) allows creating and parsing messages, containing a set of pre-defined fields of custom length and type.

## Tag format

​	Each tag must contain at least 1-byte 'Code' field, and optionally 'Length' and 'Data' fields.

| Offset | Bytes | Description |
| ------ | ----- | ----------- |
| 0      | 1     | Code        |
| 1      | 1     | Length      |
| 2      | n     | Data        |

​	The 'END' tag marks the end of a BTF message. This tag does **not** contain Length and Data fields. The 'END' tag absolutely **must** be added to the end of the message, otherwise the message parser may behavior unpredictedly.

​	The 'Data' field can contain an arbitrary data, e.g. an integer value of any size (1, 2, 4 bytes), or a string. The string is not zero-terminated, for place saving purposes.

​	When creating a message not all fields can be added to it. So, it is allowable to miss certain fields when parsing the message. The parser returns the value of a field, if found, or indicate its absence.



## General tags

| Name  | Code, HEX | Description                                                  |
| ----- | --------- | ------------------------------------------------------------ |
| END   | 00        | This tag marks the end of a BTF message. Does **not** contain Length and Data fields. |
| SIG   | 78        | Magic signature. Should be checked for a message validity.   |
| VER   | 79        | Version of the user message. (E.g. version of the setup.)    |
| START | FF        | This token is used when creating a new message as a function argument only. Doesn't appear in messages. |
