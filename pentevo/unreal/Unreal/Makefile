# MinGW Makefile by thims
# Copy ddraw.h dinput.h dsound.h from WSDK\include and sal.h from MSVC\include
# to MinGW\include before running this with mingw32-make [DEBUG=1]

CXXFLAGS = -msse2 -D_M_IX86=700 -D_M_IX86_FP=2 -DUSE_SND_EXTERNAL_BUFFER

ifdef DEBUG
	CXXFLAGS += -g -D_DEBUG
	O = bin/Debug
else
	CXXFLAGS += -O3 -DNDEBUG
	O = bin/Release
endif

$(O)/Unreal.exe: $(O)/Unreal.o $(patsubst %.cpp,$(O)/%.o,$(wildcard *.cpp))
	g++ -mwindows -mconsole -o $@ $^ -static-libgcc -static-libstdc++ \
		-luuid -lcomctl32 -lsetupapi -lwinmm -ldxguid -ldxerr8 -ldinput -lgdiplus

$(O)/%.o: %.cpp
	g++ $(CXXFLAGS) -c -o $@ $<

_O = $(subst /,\,$(O))

$(O)/%.o: %.rc
	@if not exist "$(_O)" md "$(_O)" >nul
	windres -o $@ -i $<
	
clean:
	del /f $(_O)\*.o
