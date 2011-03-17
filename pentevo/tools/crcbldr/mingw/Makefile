TARGET = ../crcbldr
CC = gcc
CFLAGS = -Wall -Os

all: $(TARGET).exe

$(TARGET).exe: ../source/crcbldr.c
	$(CC) $(CFLAGS) -s -o $@ $<

.PHONY: clean
clean:
	rm -rf $(TARGET).exe
