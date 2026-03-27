# Makefile
PREFIX = i686-w64-mingw32-

# 使用带有前缀的编译器和资源编译器
CC = $(PREFIX)gcc
WINDRES = $(PREFIX)windres

# 编译选项：-Wall 显示所有警告，-O2 优化
CFLAGS = -Wall -O2
# 链接选项：-mwindows 指定这是一个GUI程序，运行时不会弹出黑色命令行控制台
LDFLAGS = -mwindows

TARGET = ModalDialog.exe
OBJS = main.o resource.o

# 默认目标
all: $(TARGET)

# 链接生成最终的 exe
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# 编译 C 文件
main.o: main.c resource.h
	$(CC) $(CFLAGS) -c $< -o $@

# 编译资源文件
resource.o: resource.rc resource.h
	$(WINDRES) $< -O coff -o $@

# 清理生成的文件
clean:
	rm -f $(OBJS) $(TARGET)
