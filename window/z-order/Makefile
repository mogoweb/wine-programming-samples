# ==========================================
# Z-Order Demo (Win32 + Wine Wayland)
# ==========================================

CC = i686-w64-mingw32-gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -mwindows -mconsole

TARGET = z-order.exe

.PHONY: all
all: $(TARGET)

$(TARGET): main.c
	@echo "  $(CC)      $@"
	@$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	@echo "  CLEAN   $(TARGET)"
	@rm -f $(TARGET)

.PHONY: help
help:
	@echo "可用命令:"
	@echo "  make          - 编译 z-order.exe"
	@echo "  make clean    - 清理生成的文件"
	@echo "  make CC=gcc   - 在 Windows/MSYS2 下使用原生 gcc 编译"
	@echo ""
	@echo "运行指南 (测试 Z-Order 行为):"
	@echo "  wine z-order.exe"
	@echo ""
	@echo "Wine Wayland 测试:"
	@echo "  DISPLAY= WAYLAND_DISPLAY=wayland-1 WINEFSYNC=1 wine z-order.exe"
