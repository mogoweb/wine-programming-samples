# 使用 MinGW 交叉编译器
CC = i686-w64-mingw32-g++
CFLAGS = -Wall -O2 -static-libgcc -static-libstdc++
LDFLAGS = -mwindows

TARGET = Hello1.exe
SRC = Hello1.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o