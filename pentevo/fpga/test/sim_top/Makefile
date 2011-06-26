 
CC=gcc

MODELLIB=./sndpix.so
APPL=./viewer

PLI_INCLUDE=/home/lvd/modelsim/6.6a/modeltech/include

SRCLIB=./sndpix.c
SRCAPP=./viewer.c

all : $(APPL) $(MODELLIB) Makefile
#all : $(MODELLIB) Makefile

clean:
	rm $(APPLICATION)

$(MODELLIB) : $(SRCLIB) sndpix.h Makefile
	gcc -m32 -O2 -g -shared -Bsymbolic -fPIC -I$(PLI_INCLUDE) $(SRCLIB) -o $(MODELLIB)

$(APPL) : $(SRCAPP) Makefile
	gcc -O2 -g -L/usr/lib -lSDL $(SRCAPP) -o $(APPL)

