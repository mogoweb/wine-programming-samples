CC = i686-w64-mingw32-gcc
CFLAGS = -Wall -municode -finput-charset=UTF-8
LIBS = -lshell32 -lgdi32
LDFLAGS = -mwindows -mconsole

TARGET = tray-demo.exe
X11_TARGET = x11-mouse

all: $(TARGET) $(X11_TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS) $(LDFLAGS)

$(X11_TARGET): x11-mouse.c
	gcc -Wall -o $@ $< -lX11

clean:
	rm -f $(TARGET) $(X11_TARGET)

.PHONY: all clean
