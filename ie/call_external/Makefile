CC = i686-w64-mingw32-gcc
CFLAGS = -Wno-discarded-qualifiers -Wno-incompatible-pointer-types -O2
LDFLAGS = -lole32 -loleaut32 -luuid -lurlmon -luser32 -lgdi32
TARGET = qahost.exe
SRC = qahost.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

install: $(TARGET)
	cp $(TARGET) ~/.deepinwine/org.deepin-wine.browser.deepin/drive_c/

run: install
	WINEPREFIX=~/.deepinwine/org.deepin-wine.browser.deepin ~/work/projects/deepin-wine/source/deepin-wine10-stable/build/wine "c:/$(TARGET)" "http://10.20.33.129:8089/adjustwh/loginpage.html"

clean:
	rm -f $(TARGET)

.PHONY: all install run clean
