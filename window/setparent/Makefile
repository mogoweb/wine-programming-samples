# ==========================================
# 跨进程窗口挂载 Demo (Win32 + Wine Wayland)
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
# -mconsole: 强制打开控制台，以便我们在终端能看到 printf 打印的 HWND 地址
LDFLAGS = -mwindows -mconsole

# 最终生成的目标文件
TARGETS = parent_app.exe child_app.exe single_app.exe child_to_main.exe

# 默认目标
.PHONY: all
all: $(TARGETS)

# 编译父窗口程序
parent_app.exe: parent_app.c
	@echo "  $(CC)      $@"
	@$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# 编译子窗口程序
child_app.exe: child_app.c
	@echo "  $(CC)      $@"
	@$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# 编译单窗口程序
single_app.exe: single_app.c
	@echo "  $(CC)      $@"
	@$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# 编译单窗口程序
child_to_main.exe: child_to_main.c
	@echo "  $(CC)      $@"
	@$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# 清理生成的文件
.PHONY: clean
clean:
	@echo "  CLEAN   $(TARGETS)"
	@rm -f $(TARGETS)

# 提供使用帮助说明
.PHONY: help
help:
	@echo "可用命令:"
	@echo "  make          - 编译所有 .exe 文件 (默认使用 x86_64-w64-mingw32-gcc)"
	@echo "  make clean    - 清理生成的 .exe 文件"
	@echo "  make CC=gcc   - (可选) 在 Windows/MSYS2 环境下使用原生 gcc 编译"
	@echo ""
	@echo "运行指南 (在 Linux 终端中测试 Wine Wayland):"
	@echo "  终端 1: DISPLAY= WAYLAND_DISPLAY=wayland-1 WINEFSYNC=1 wine parent_app.exe"
	@echo "  终端 2: DISPLAY= WAYLAND_DISPLAY=wayland-1 WINEFSYNC=1 wine child_app.exe <第一步终端里打印的句柄>"
