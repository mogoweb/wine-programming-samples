# DsoFramer C++ Test Client - port of DsoFramer Vb6Test sample to pure Win32 API
CC      = i686-w64-mingw32-g++
WINDRES = i686-w64-mingw32-windres
CXXFLAGS = -O2 -Wall -std=c++11
LDFLAGS  = -municode -static-libgcc -static-libstdc++ -lole32 -loleaut32 -luuid -lcomdlg32 -luser32 -lgdi32

TARGET  = framerapp.exe
OBJS    = main.o framerhost.o framerapp_res.o

WINE    = ~/work/projects/deepin-wine/source/deepin-wine10-stable/build/wine
WINEPREFIX = ~/.deepinwine/org.deepin-wine.browser.deepin

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

main.o: main.cpp framerhost.h dsoframer.h resource.h
	$(CC) $(CXXFLAGS) -c main.cpp

framerhost.o: framerhost.cpp framerhost.h dsoframer.h
	$(CC) $(CXXFLAGS) -c framerhost.cpp

framerapp_res.o: framerapp.rc resource.h
	$(WINDRES) -O coff -o framerapp_res.o framerapp.rc

install: $(TARGET)
	cp $(TARGET) $(WINEPREFIX)/drive_c/DsoFramer/

run: install
	WINEPREFIX=$(WINEPREFIX) $(WINE) "c:/DsoFramer/framerapp.exe"

tools: wininfo.exe framerprobe.exe

wininfo.exe: wininfo.c
	$(CC) -O2 -o wininfo.exe wininfo.c

framerprobe.exe: framerprobe.c
	$(CC) -O2 -o framerprobe.exe framerprobe.c

clean: 
	rm -f $(TARGET) $(OBJS)

.PHONY: all install run clean tools
