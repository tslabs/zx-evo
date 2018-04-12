###############################################################################
# Makefile for the project pentevo
###############################################################################


## General Flags
ifeq ($(OS),Windows_NT)
EXE = ".exe"
DEVNULL = "nul"
else
EXE = ""
DEVNULL = "/dev/null"
endif

PROJECT = core
TARGET = $(PROJECT).elf
MCU = atmega128
CC = avr-c++

MHMT = "../../../tools/mhmt/mhmt$(EXE)"
MKFW = "../../../tools/make_fw/make_fw$(EXE)"

## Programmator Settings
TARGET_AD = m128
# PROGRAMMER = usbtiny
PROGRAMMER = usbasp
HFUSE = 0xC1
LFUSE = 0xDF
LOCK = 0x3F

AP = ../../../tools/avrdude/
AD = "$(AP)avrdude$(EXE)" -q -q -u -C "$(AP)avrdude.conf" -c $(PROGRAMMER) -p $(TARGET_AD)

## Compile options common for all C compilation units.
CFLAGS = -mmcu=$(MCU)
CFLAGS += -Os -flto -Wall -Wextra -gdwarf-2 -std=c++11
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -fdata-sections -ffunction-sections
CFLAGS += -Wimplicit-fallthrough=0
CFLAGS += -DF_CPU=11059200UL
CFLAGS += -Wl,--gc-sections,--relax,-Map=$(PROJECT).map

## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings

## Objects that must be built in order to link
SOURCES = ../main.c ../zx.c ../interrupts.c ../ps2.c ../depacker_dirty.c ../rs232.c ../rtc.c ../atx.c ../joystick.c ../version.c ../tape.c ../kbmap.c ../spiflash.c ../rs232_asm.S
HEADERS = ../zx.h ../ps2.h ../spi.h ../depacker_dirty.h ../rs232.h ../rtc.h ../atx.h ../joystick.h ../version.h ../tape.h ../kbmap.h ../spiflash.h

## Objects explicitly added by the user
FPGACORE = tsconf.o baseconf.o egg.o

## Build
all:
	make norm
	make vdac
	make vdac2

norm: clean quartus $(FPGACORE) compile core size
	@$(MKFW) $(PROJECT).hex $(PROJECT).eep zxevo_fw.bin version.txt >$(DEVNULL)

vdac: clean quartus_vdac $(FPGACORE) compile core size
	@$(MKFW) $(PROJECT).hex $(PROJECT).eep zxevo_fw_vdac.bin version_vdac.txt >$(DEVNULL)

vdac2: clean quartus_vdac2 $(FPGACORE) compile core size
	@$(MKFW) $(PROJECT).hex $(PROJECT).eep zxevo_fw_vdac2.bin version_vdac2.txt >$(DEVNULL)

## Compile and link
compile: $(SOURCES) $(HEADERS)
	@$(CC) $(SOURCES) $(FPGACORE) $(CFLAGS) -o $(TARGET)

## Copy FPGA bitstreams
quart%:
	@cp ../../../fpga/current/$@/top.rbf tsconf.rbf
	@cp ../../../fpga/base/$@/top.rbf baseconf.rbf
	@cp ../../../fpga/tennis/$@/zxevo_tennis_compatible.rbf egg.rbf

## Prepare FPGA bitstreams
$(FPGACORE): tsconf.rbf baseconf.rbf egg.rbf
	@$(MHMT) -maxwin2048 tsconf.rbf tsconf.mlz >$(DEVNULL)
	@$(MHMT) -maxwin2048 baseconf.rbf baseconf.mlz >$(DEVNULL)
	@$(MHMT) -maxwin2048 egg.rbf egg.mlz >$(DEVNULL)
	@avr-objcopy -I binary -O elf32-avr -B avr:51 --rename-section .data=.progmem.data,contents,alloc,load,readonly,data --redefine-sym _binary_tsconf_mlz_start=fpga0 tsconf.mlz tsconf.o
	@avr-objcopy -I binary -O elf32-avr -B avr:51 --rename-section .data=.progmem.data,contents,alloc,load,readonly,data --redefine-sym _binary_baseconf_mlz_start=fpga1 baseconf.mlz baseconf.o
	@avr-objcopy -I binary -O elf32-avr -B avr:51 --rename-section .data=.progmem.data,contents,alloc,load,readonly,data --redefine-sym _binary_egg_mlz_start=fpga2 egg.mlz egg.o

core: $(TARGET)
	@avr-objcopy -O ihex $(HEX_FLASH_FLAGS) $< $@.hex
	@avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@.eep || exit 0

size: ${TARGET}
	@avr-size ${TARGET}

## Clean target
.PHONY: clean
clean:
	 @echo >$(PROJECT).dummy
	 @echo >tsconf.dummy
	 @echo >baseconf.dummy
	 @echo >egg.dummy
	 @rm -rf $(FPGACORE) core.* tsconf.* baseconf.* egg.*

## program chip
flash: $(PROJECT).hex
	@$(AD) -B 0.5 \
	-U flash:w:"$(PROJECT).hex":a

fuses:
	@$(AD) -B 20 \
	-U hfuse:w:$(HFUSE):m \
	-U lfuse:w:$(LFUSE):m \
	-U lock:w:$(LOCK):m

read:
	@$(AD) -B 1 -U flash:r:"flash.bin":r
	@$(AD) -B 1 -U eeprom:r:"eeprom.bin":r
