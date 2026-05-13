CC = i686-w64-mingw32-gcc
CFLAGS = -Wall
LIBS = -lshell32 -lgdi32
LDFLAGS = -mwindows -mconsole

TARGET = tray-alpha-demo.exe

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
