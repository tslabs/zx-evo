	
PROJECT = zxevo_bl.hex
TARGET_AD = m128
PROGRAMMER = usbasp
LOCK = 0x3F
HFUSE = 0xC0
LFUSE = 0xDF

AP = ../../../tools/avrdude/
AD = "$(AP)avrdude.exe" -q -q -u -C "$(AP)avrdude.conf" -c $(PROGRAMMER) -p $(TARGET_AD)

flash: $(PROJECT)
	@$(AD) -B .5 \
	-U flash:w:"$(PROJECT)":a

fuses:
	@$(AD) -B 20 \
	-U hfuse:w:$(HFUSE):m \
	-U lfuse:w:$(LFUSE):m \
	-U lock:w:$(LOCK):m

read:
	@$(AD) -B .5 -U flash:r:"flash.bin":r
	@$(AD) -B .5 -U eeprom:r:"eeprom.bin":r
