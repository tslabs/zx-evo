TARGET = ../make_fw
CC = gcc

CFLAGS = -Wall -Os

OBJECT = make_fw.o

all: $(TARGET)

## Compile
$(OBJECT): ../source/make_fw.c
	$(CC) $(CFLAGS) -o $(OBJECT) -c $<

##Link
$(TARGET): $(OBJECT)
	 $(CC) $(OBJECT) -o $(TARGET)


## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECT) $(TARGET)

