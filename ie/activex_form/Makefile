CC      = i686-w64-mingw32-gcc
CFLAGS  = -O2 -Wall
LDFLAGS = -lole32 -loleaut32 -luuid -luser32 -lgdi32 -ladvapi32
TARGET  = axformctl.dll
SRC     = axformctl.c
DEF     = axformctl.def

# Wine 及容器路径，可用变量覆盖：
#   make run WINEPREFIX=~/.deepinwine/xxx WINE=/path/to/wine
WINE       ?= $(HOME)/work/projects/deepin-wine/source/deepin-wine10-stable/build/wine
WINEPREFIX ?= $(HOME)/.deepinwine/org.deepin-wine.browser.deepin

all: $(TARGET)

$(TARGET): $(SRC) $(DEF)
	$(CC) $(CFLAGS) -shared -o $(TARGET) $(SRC) $(DEF) $(LDFLAGS)

# 拷贝 DLL 与测试页到容器 C:\axform 并用 wine regsvr32 注册控件
install: $(TARGET)
	mkdir -p $(WINEPREFIX)/drive_c/axform
	cp $(TARGET) testpage.html $(WINEPREFIX)/drive_c/axform/
	WINEPREFIX=$(WINEPREFIX) $(WINE) regsvr32 /s "c:\\axform\\$(TARGET)"

# 注册控件后用容器内 iexplore 打开测试页
run: install
	WINEPREFIX=$(WINEPREFIX) $(WINE) iexplore "file:///c:/axform/testpage.html"

uninstall:
	WINEPREFIX=$(WINEPREFIX) $(WINE) regsvr32 /s /u "c:\\axform\\$(TARGET)"

clean:
	rm -f $(TARGET)

.PHONY: all run uninstall clean
