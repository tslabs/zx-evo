SRCDIR = .
OBJDIR = obj
BINDIR = bin
MCU = atmega128
TARGET = $(BINDIR)/proga

SRC = _dummy.c _messages.c _uart.c _pintest.c _ps2k.c _output.c _screen.c _global.c
SRC += _t_video.c _t_dram.c _t_beep.c _t_zxkbd.c _t_ps2k.c _t_ps2m.c _t_rs232.c
SRC += _t_sd.c main.c
ASRC = _global_asm.S _ps2k_asm.S _t_ps2m_asm.S _uart_asm.S _t_beep_asm.S _sd_asm.S

TABLEDATA = tabl_sin.bin
FPGADATA = fpgadat1.mlz

#---------------- Compiler Options C ----------------
#  -g*:          generate debugging information
#  -O*:          optimization level
#  -f...:        tuning, see GCC manual and avr-libc documentation
#  -Wall...:     warning level
#  -Wa,...:      tell GCC to pass this to the assembler.
#    -adhlns...: create assembler listing
CFLAGS = -mmcu=$(MCU)
CFLAGS += -I$(SRCDIR)
# CFLAGS += -gdwarf-2
CFLAGS += -O2
CFLAGS += -funsigned-char
CFLAGS += -funsigned-bitfields
CFLAGS += -fpack-struct
CFLAGS += -fshort-enums
CFLAGS += -Wall
# CFLAGS += -Wstrict-prototypes
# CFLAGS += -mshort-calls
# CFLAGS += -fno-unit-at-a-time
# CFLAGS += -Wundef
CFLAGS += -Wunreachable-code
# CFLAGS += -Wsign-compare
CFLAGS += -Wa,-adhlns=$(<:%.c=$(OBJDIR)/%.lst)
# CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))
CFLAGS += -std=gnu99
# Compiler flags to generate dependency files.
# CFLAGS += -MMD -MP -MF .dep/$(@F).d


#---------------- Assembler Options ----------------
#  -Wa,...:   tell GCC to pass this to the assembler.
#  -adhlns:   create listing
#  -gstabs:   have the assembler create line number information; note that
#             for use in COFF files, additional information about filenames
#             and function names needs to be present in the assembler source
#             files -- see avr-libc docs [FIXME: not yet described there]
#  -listing-cont-lines: Sets the maximum number of continuation lines of hex 
#       dump that will be displayed for a given single line of source input.
ASFLAGS = -mmcu=$(MCU) -I$(SRCDIR) -x assembler-with-cpp
ASFLAGS += -Wa,-adhlns=$(<:%.S=$(OBJDIR)/%.lst),-gstabs,--listing-cont-lines=100

#---------------- Linker Options ----------------
#  -Wl,...:     tell GCC to pass this to linker.
#    -Map:      create map file
#    --cref:    add cross reference to  map file
LDFLAGS = -Wl,--section-start=.tabledata=0x0ff00,--section-start=.bigdata=0x10000,-Map=$(TARGET).map,--cref
# LDFLAGS += $(EXTMEMOPTS)
# LDFLAGS += $(patsubst %,-L%,$(EXTRALIBDIRS))
# LDFLAGS += $(PRINTF_LIB) $(SCANF_LIB) $(MATH_LIB)
# LDFLAGS += -T linker_script.x

#============================================================================

# Define programs and commands.
SHELL = sh
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
AR = avr-ar rcs
NM = avr-nm
REMOVE = -rm -f
REMOVEDIR = -rm -rf
COPY = cp
WINSHELL = cmd

# Define all object files.
OBJ = $(FPGADATA:%.mlz=$(OBJDIR)/%.o)
OBJ += $(TABLEDATA:%.bin=$(OBJDIR)/%.o)
OBJ += $(SRC:%.c=$(OBJDIR)/%.o)
OBJ += $(ASRC:%.S=$(OBJDIR)/%.o)

# Define all listing files.
LST = $(SRC:%.c=$(OBJDIR)/%.lst) $(ASRC:%.S=$(OBJDIR)/%.lst)



all: dbg_echo elf hex eep lss sym

dbg_echo:
	@echo OBJ=$(OBJ)

elf: $(TARGET).elf
hex: $(TARGET).hex
eep: $(TARGET).eep
lss: $(TARGET).lss
sym: $(TARGET).sym
lib: $(LIBNAME)

# Create final output files (.hex, .eep) from ELF output file.
%.hex: %.elf
	@echo --- MAKE HEX ---
	$(OBJCOPY) -O ihex -R .eeprom -R .fuse -R .lock $< $@
#	$(OBJCOPY) -O binary -R .eeprom -R .fuse -R .lock $(TARGET).elf $(TARGET).bin

%.eep: %.elf
	@echo --- MAKE EEP ---
	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 --no-change-warnings -O ihex $< $@ || exit 0

# Create extended listing file from ELF output file.
%.lss: %.elf
	@echo --- MAKE LST ---
	$(OBJDUMP) -h -S -z $< > $@

# Create a symbol table from ELF output file.
%.sym: %.elf
	@echo --- MAKE SYM ---
	$(NM) -n $< > $@


# Link: create ELF output file from object files.
.SECONDARY : $(TARGET).elf
.PRECIOUS : $(OBJ)
%.elf: $(OBJ)
	@echo --- LINK ---
	$(CC) $(CFLAGS) $^ --output $@ $(LDFLAGS)


# Compile: create object files from C source files.
$(OBJDIR)/%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@ 


## Compile: create assembler files from C source files.
#%.s : %.c
#	@echo --- Compile to assembler ---
#	$(CC) -S $(CFLAGS) $< -o $@


# Assemble: create object files from assembler source files.
$(OBJDIR)/%.o : %.S
	@echo --- Assemble to objects ---
	$(CC) -c $(ASFLAGS) $< -o $@

$(OBJDIR)/%.o : %.mlz
	$(OBJCOPY) -I binary -O elf32-avr -B avr:51 --rename-section .data=.bigdata,contents,alloc,load,readonly,data --redefine-sym _binary_$(*F)_mlz_start=$(*F) $(*F).mlz $@

$(OBJDIR)/%.o : %.bin
	$(OBJCOPY) -I binary -O elf32-avr -B avr:51 --rename-section .data=.tabledata,contents,alloc,load,readonly,data --redefine-sym _binary_$(*F)_bin_start=$(*F) $(*F).bin $@

# Target: clean project.
clean:
	@$(REMOVE) $(TARGET).bin
	@$(REMOVE) $(TARGET).hex
	@$(REMOVE) $(TARGET).eep
	@$(REMOVE) $(TARGET).cof
	@$(REMOVE) $(TARGET).elf
	@$(REMOVE) $(TARGET).map
	@$(REMOVE) $(TARGET).sym
	@$(REMOVE) $(TARGET).lss
	@$(REMOVE) $(OBJDIR)/*.*
#	@$(REMOVE) $(SRC:%.c=$(OBJDIR)/%.o)
#	@$(REMOVE) $(SRC:%.c=$(OBJDIR)/%.lst)
#	@$(REMOVE) $(SRC:.c=.s)
	@$(REMOVE) $(SRC:.c=.d)
	@$(REMOVE) $(SRC:.c=.i)
	@$(REMOVEDIR) .dep
	@echo --- CLEAN OK ---

# Create object files directory
$(shell mkdir $(OBJDIR) 2>/dev/null)
$(shell mkdir $(BINDIR) 2>/dev/null)
# Include the dependency files.
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)


# Listing of phony targets.
.PHONY : all elf hex eep lss sym clean
