# 托盘透明图标 Demo

演示带透明通道（Alpha Channel）的系统托盘图标显示。

## 功能

- 加载带透明通道的 ICO 图标
- 图标在系统托盘中正确显示透明效果
- 支持多种图标尺寸（16/24/32/48/64）

## 编译

```bash
cd tray-alpha-demo
make
```

生成文件：`tray-alpha-demo.exe`

## 运行

```bash
# 普通 Wine
wine tray-alpha-demo.exe

# Wine Wayland
DISPLAY= WAYLAND_DISPLAY=wayland-1 WINEFSYNC=1 wine tray-alpha-demo.exe
```

## 技术要点

### ICO 文件透明通道

Windows ICO 文件可以包含：
- 多个尺寸的图标图像
- 每个像素的 Alpha 通道值（0-255）

使用 ImageMagick 创建透明 ICO：

```bash
convert -size 64x64 xc:none \
    -fill "rgba(52, 152, 219, 0.8)" -draw "circle 32,32 32,8" \
    -define icon:auto-resize=16,24,32,48,64 tray-alpha.ico
```

### LoadImage 加载 ICO

```c
HICON hIcon = (HICON)LoadImageA(NULL, iconPath, IMAGE_ICON,
                                 GetSystemMetrics(SM_CXSMICON),
                                 GetSystemMetrics(SM_CYSMICON),
                                 LR_LOADFROMFILE);
```

`LoadImage` 会自动处理 ICO 文件中的透明通道。

### 图标尺寸

系统托盘图标推荐尺寸：
- 16x16 - 标准显示器
- 24x24 - 高 DPI 显示器
- 32x32 - 更高 DPI

`GetSystemMetrics(SM_CXSMICON)` 获取当前系统推荐的小图标尺寸。

## Wine/Wine Wayland 测试意义

- 透明通道图标的正确渲染
- Alpha 混合效果
- 不同 DPI 下的图标缩放
- ICO 文件格式解析
