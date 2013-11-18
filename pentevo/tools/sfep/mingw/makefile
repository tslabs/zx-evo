TARGET = ../sfep
CC = gcc
CFLAGS = -Wall -Os

all: $(TARGET).exe

$(TARGET).exe: ../source/sfep.c
	$(CC) $(CFLAGS) -s -o $@ $<

.PHONY: clean
clean:
	rm -rf $(TARGET).exe
