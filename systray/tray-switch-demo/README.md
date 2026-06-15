# 托盘图标切换 Demo

演示托盘图标的动态切换效果。

## 功能

- 启动时显示蓝色半透明圆形图标
- 500ms 后自动切换为绿色半透明方形图标
- 演示使用 `CreateIconIndirect` 创建带 Alpha 通道的图标

## 编译

```bash
cd tray-switch-demo
make
```

生成文件：`tray-switch-demo.exe`

## 运行

```bash
# 普通 Wine
wine tray-switch-demo.exe

# Wine Wayland
DISPLAY= WAYLAND_DISPLAY=wayland-1 WINEFSYNC=1 wine tray-switch-demo.exe
```

## 技术要点

### CreateIconIndirect

使用 `CreateIconIndirect` 从位图创建图标：

```c
ICONINFO ii = {0};
ii.fIcon = TRUE;
ii.hbmColor = hBitmap;  // 32位 RGBA 位图
ii.hbmMask = hMask;     // 单色掩码

HICON hIcon = CreateIconIndirect(&ii);
```

### BITMAPV5HEADER

使用 `BITMAPV5HEADER` 创建 32位带 Alpha 通道的 DIB Section：

```c
BITMAPV5HEADER bi = {0};
bi.bV5Size = sizeof(BITMAPV5HEADER);
bi.bV5BitCount = 32;
bi.bV5Compression = BI_BITFIELDS;
bi.bV5AlphaMask = 0xFF000000;
```

### Shell_NotifyIcon NIM_MODIFY

动态更新托盘图标：

```c
nid.hIcon = hNewIcon;
Shell_NotifyIcon(NIM_MODIFY, &nid);
```

## Wine/Wine Wayland 测试意义

- 动态图标更新
- Alpha 通道渲染
- 图标切换时的刷新行为
