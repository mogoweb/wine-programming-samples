# HID Serial Number Reader Demo
# MinGW cross-compile Makefile

CC = i686-w64-mingw32-g++
CFLAGS = -Wall -O2 -DUNICODE -D_UNICODE -static-libgcc -static-libstdc++
LIBS = -lsetupapi -lhid
LDFLAGS = -mconsole

TARGET = test-hid-serial.exe
SRC = test-hid-serial.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)
