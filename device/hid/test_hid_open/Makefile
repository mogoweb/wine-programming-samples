# HID Device Enumeration Demo
# MinGW cross-compile Makefile

CC = i686-w64-mingw32-g++
CFLAGS = -Wall -O2 -DUNICODE -D_UNICODE -static-libgcc -static-libstdc++
LIBS = -lsetupapi -lhid
LDFLAGS = -mconsole

TARGET = test_hid_open.exe
SRC = test_hid_open.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)
