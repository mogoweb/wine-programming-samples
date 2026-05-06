CC = i686-w64-mingw32-gcc
CFLAGS = -Wall -municode -finput-charset=UTF-8
LIBS = -lshell32 -lgdi32

TARGET = tray-demo.exe

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
