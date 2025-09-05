# 使用 MinGW 交叉编译器
CC = i686-w64-mingw32-g++
CFLAGS = -Wall -O2 -static-libgcc -static-libstdc++
LDFLAGS = -mwindows

TARGET = Hello2.exe
SRC = Hello2.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o