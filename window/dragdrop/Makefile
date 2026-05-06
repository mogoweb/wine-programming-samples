# ==========================================
# Drag & Drop Demo (Win32 OLE + Wine Wayland)
# ==========================================

CC = i686-w64-mingw32-gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -mwindows -lole32 -luuid

TARGET = dragdrop.exe

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
	@echo "Available commands:"
	@echo "  make          - Build dragdrop.exe"
	@echo "  make clean    - Remove built files"
	@echo "  make CC=gcc   - Build with native gcc on Windows/MSYS2"
	@echo ""
	@echo "Usage:"
	@echo "  wine dragdrop.exe"
	@echo ""
	@echo "Wine Wayland:"
	@echo "  DISPLAY= WAYLAND_DISPLAY=wayland-1 WINEFSYNC=1 wine dragdrop.exe"
	@echo ""
	@echo "Features:"
	@echo "  - Left window: Drag source, drag text out"
	@echo "  - Right window: Drop target, receive dropped text"
	@echo "  - Supports multiple drops with history display"
