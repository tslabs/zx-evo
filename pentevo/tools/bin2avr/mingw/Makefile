TARGET = ../bin2avr
CC = gcc
CFLAGS = -Wall -Os

all: $(TARGET).exe

$(TARGET).exe: ../source/bin2avr.c
	$(CC) $(CFLAGS) -s -o $@ $<

.PHONY: clean
clean:
	rm -rf $(TARGET).exe
