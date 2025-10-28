CC = i686-w64-mingw32-g++
CFLAGS = -Wall -O2 -static-libgcc -static-libstdc++ -std=c++20 -mconsole
LDFLAGS =

TARGET = hello-time.exe
SRC := $(shell find . -type f \( -name "*.cpp" -o -name "*.c" \))
OBJ := $(SRC:.cpp=.o)
OBJ := $(OBJ:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@

# 编译 .cpp 或 .c -> .o
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 自动生成依赖文件
DEPS := $(SRC:.cpp=.d)
DEPS := $(DEPS:.c=.d)
%.d: %.cpp
	$(CC) -MM $< -MF $@
%.d: %.c
	$(CC) -MM $< -MF $@

# 包含依赖
-include $(DEPS)

clean:
	rm -f $(TARGET) $(OBJ) $(DEPS)