# Layered Alpha Demo

演示 Layered Window（分层窗口）最常见的一种用法：用
`SetLayeredWindowAttributes` + `LWA_ALPHA` 给整个窗口（含标题栏与边框）
设置一个常量 Alpha，从而让窗口整体半透明。通过底部滑块可实时调整
透明度，复选框则演示动态增删 `WS_EX_LAYERED` 扩展风格。

## 功能

- 使用 `WS_EX_LAYERED` 扩展风格创建分层窗口
- 用 `SetLayeredWindowAttributes` / `LWA_ALPHA` 设置整体常量透明度
- 滑块（Trackbar）实时调节 Alpha（30–255），窗口立即重绘
- 复选框动态添加/移除 `WS_EX_LAYERED`，对比分层与普通窗口
- 客户区绘制多彩横条，便于直观观察透明度变化

## 编译

```bash
cd LayeredAlphaDemo
make            # 默认 32 位（i686-w64-mingw32-g++）
make ARCH=x86_64   # 64 位
make clean
```

生成文件：`LayeredAlphaDemo.exe`

## 运行

```bash
# 普通 Wine
wine LayeredAlphaDemo.exe

# Wine Wayland
DISPLAY= WAYLAND_DISPLAY=wayland-1 WINEFSYNC=1 wine LayeredAlphaDemo.exe
```

## 技术要点

### 1. 创建分层窗口

在 `CreateWindowEx` 时带上 `WS_EX_LAYERED` 扩展风格即可创建分层窗口：

```c
HWND hwnd = CreateWindowExW(
    WS_EX_LAYERED,        // 关键：分层窗口扩展风格
    CLASS_NAME, L"Title",
    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
    ...);
```

### 2. 设置整体常量 Alpha（LWA_ALPHA）

```c
SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
```

- 第二个参数 `crKey` 在 `LWA_ALPHA` 模式下不使用，传 0
- 第三个参数 `alpha` 取值 0–255：0 完全透明，255 完全不透明
- 该 Alpha 作用于整个窗口表面，包括标题栏、边框与子控件
- 与 `WS_EX_TRANSPARENT`（点击穿透）不同：`LWA_ALPHA` 仅影响显示，
  窗口仍然可以正常接收鼠标/键盘输入，因此即便很透明也能拖动滑块

### 3. 动态增删 WS_EX_LAYERED

```c
LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex | WS_EX_LAYERED);   // 启用
SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);     // 必须重新设置

// 关闭：移除扩展风格后用 SWP_FRAMECHANGED 通知系统重绘
ex &= ~WS_EX_LAYERED;
SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex);
SetWindowPos(hwnd, NULL, 0,0,0,0,
    SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
```

## Layered Window 的几种用法对照

本目录（`layered-window/`）下包含一组互补的示例，覆盖分层窗口的全部
常见 API：

| 示例 | API | 说明 |
| --- | --- | --- |
| `SimpleLayeredWindow` | `SetLayeredWindowAttributes` + `LWA_COLORKEY` | 色键透明：指定颜色区域变为透明且可点击穿透 |
| `LayeredAlphaDemo`（本示例） | `SetLayeredWindowAttributes` + `LWA_ALPHA` | 整体常量 Alpha：窗口整体半透明 |
| `UpdateLayeredWindowDemo` | `UpdateLayeredWindow` + `ULW_ALPHA` | 逐像素 Alpha：任意形状、平滑半透明 |
| `ShadowBorderDemo` | `UpdateLayeredWindow` + `ULW_ALPHA` | 用逐像素 Alpha 实现窗口外阴影 |
| `LayeredChildWindow` | 子窗口 + `LWA_COLORKEY` | 子窗口作为分层窗口的注意事项（`WS_CLIPCHILDREN`、创建后再追加 `WS_EX_LAYERED`） |

`SetLayeredWindowAttributes` 与 `UpdateLayeredWindow` 的区别：

- `SetLayeredWindowAttributes` 简单高效，但只能“整窗常量 Alpha”或
  “单一色键透明”，适合做半透明或简单的挖孔形状
- `UpdateLayeredWindow` 由程序提供 32 位 ARGB 位图，可实现逐像素 Alpha，
  适合任意形状、抗锯齿边缘、阴影等复杂效果；调用后 `SetLayeredWindowAttributes`
  设置的属性将不再生效

## Wine/Wine Wayland 测试意义

- `WS_EX_LAYERED` 扩展风格是否被正确识别
- `SetLayeredWindowAttributes` / `LWA_ALPHA` 的常量 Alpha 混合
- 滑块实时改变 Alpha 时的重绘与合成性能
- 动态增删 `WS_EX_LAYERED` 后窗口能否恢复为普通不透明窗口
- 半透明窗口在 XWayland 与纯 Wayland 合成器下的呈现差异
