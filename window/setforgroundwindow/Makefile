# ==========================================
# SetForegroundWindow Demo (Win32 + Wine Wayland)
# ==========================================

# 编译器设置
# 默认使用 Linux 下的 MinGW-w64 交叉编译器
# 如果你在 MSYS2/MinGW 环境下原生编译，可以执行 `make CC=gcc`
CC = i686-w64-mingw32-gcc

# 编译选项
# -Wall -Wextra: 开启常见警告
# -O2: 基础优化
CFLAGS = -Wall -Wextra -O2

# 链接选项
# -mwindows: 链接基础的 Windows GUI 库 (User32, Gdi32 等)
# -mconsole: 强制打开控制台，便于调试输出
LDFLAGS = -mwindows -mconsole

# 最终生成的目标文件
TARGET = setforegroundwindow.exe

# 默认目标
.PHONY: all
all: $(TARGET)

# 编译主程序
$(TARGET): main.c
	@echo "  $(CC)      $@"
	@$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# 清理生成的文件
.PHONY: clean
clean:
	@echo "  CLEAN   $(TARGET)"
	@rm -f $(TARGET)

# 提供使用帮助说明
.PHONY: help
help:
	@echo "可用命令:"
	@echo "  make          - 编译 setforegroundwindow.exe (默认使用 i686-w64-mingw32-gcc)"
	@echo "  make clean    - 清理生成的 .exe 文件"
	@echo "  make CC=gcc   - (可选) 在 Windows/MSYS2 环境下使用原生 gcc 编译"
	@echo ""
	@echo "运行指南 (测试 SetForegroundWindow 行为):"
	@echo "  wine setforegroundwindow.exe"
	@echo ""
	@echo "Wine Wayland 测试:"
	@echo "  DISPLAY= WAYLAND_DISPLAY=wayland-1 WINEFSYNC=1 wine setforegroundwindow.exe"
