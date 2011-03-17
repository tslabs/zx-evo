TARGET = ../make_fw
CC = gcc
CFLAGS = -Wall -Os

all: $(TARGET).exe

$(TARGET).exe: ../source/make_fw.c
	$(CC) $(CFLAGS) -s -o $@ $<

.PHONY: clean
clean:
	rm -rf $(TARGET).exe
