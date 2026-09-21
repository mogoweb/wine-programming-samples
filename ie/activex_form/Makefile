CC      = i686-w64-mingw32-gcc
CFLAGS  = -O2 -Wall
LDFLAGS = -lole32 -loleaut32 -luuid -luser32 -lgdi32 -ladvapi32
TARGET  = axformctl.dll
HOST    = axhost.exe
SRC     = axformctl.c
DEFS    = axformctl.def

# Wine 及容器路径，可用变量覆盖：
#   make run WINEPREFIX=~/.deepinwine/xxx WINE=/path/to/wine
WINE       ?= $(HOME)/work/projects/deepin-wine/source/deepin-wine10-stable/build/wine
WINEPREFIX ?= $(HOME)/.deepinwine/org.deepin-wine.browser.deepin

all: $(TARGET) $(HOST)

$(TARGET): $(SRC) $(DEFS)
	$(CC) $(CFLAGS) -shared -o $(TARGET) $(SRC) $(DEFS) $(LDFLAGS)

$(HOST): axhost.c
	$(CC) $(CFLAGS) -o $(HOST) axhost.c $(LDFLAGS)

# 拷贝 DLL/测试页/宿主到容器 C:\axform 并用 wine regsvr32 注册控件
install: $(TARGET) $(HOST)
	mkdir -p $(WINEPREFIX)/drive_c/axform
	cp $(TARGET) $(HOST) testpage.html $(WINEPREFIX)/drive_c/axform/
	WINEPREFIX=$(WINEPREFIX) $(WINE) regsvr32 /s "c:\\axform\\$(TARGET)"

# 注册控件后用容器内 iexplore 打开测试页（MSHTML 容器路径）
run: install
	WINEPREFIX=$(WINEPREFIX) $(WINE) iexplore "file:///c:/axform/testpage.html"

# 注册控件后运行原生宿主 axhost.exe（resize 跟随验证）
run-host: install
	WINEPREFIX=$(WINEPREFIX) $(WINE) "c:\\axform\\$(HOST)"

uninstall:
	WINEPREFIX=$(WINEPREFIX) $(WINE) regsvr32 /s /u "c:\\axform\\$(TARGET)"

clean:
	rm -f $(TARGET) $(HOST)

.PHONY: all run run-host uninstall clean
