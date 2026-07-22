# 使用 MinGW 交叉编译器
# 可通过环境变量 ARCH 切换位数：make ARCH=x86_64 （64位），默认 32 位
ARCH ?= i686
CXX = $(ARCH)-w64-mingw32-g++
WINDRES = $(ARCH)-w64-mingw32-windres
CXXFLAGS = -Wall -O2 -municode
LDFLAGS = -mwindows -municode -static-libgcc -static-libstdc++
LIBS = -lcomctl32

TARGET = LayeredAlphaDemo.exe
SRC := $(shell find . -type f -name "*.cpp")
OBJ := $(SRC:.cpp=.o)
RES := manifest.res

all: $(TARGET)

$(TARGET): $(OBJ) $(RES)
	$(CXX) $(OBJ) $(RES) $(LDFLAGS) $(LIBS) -o $@

# 编译 .cpp -> .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 编译资源（含 manifest）
$(RES): manifest.rc app.manifest
	$(WINDRES) manifest.rc -O coff -o $@

# 自动生成依赖文件
DEPS := $(SRC:.cpp=.d)
%.d: %.cpp
	$(CXX) -MM $< -MF $@
-include $(DEPS)

clean:
	rm -f $(TARGET) $(OBJ) $(DEPS) $(RES)
