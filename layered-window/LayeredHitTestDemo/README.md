# Layered Hit-Test Demo

验证 Layered Window（分层窗口）**透明区域鼠标事件的传递机制**：
是“直接透传”给下方窗口，还是“消息转发”传给下层窗口。

## 结论

答案取决于“透明”的实现方式：

| 透明方式 | API | 透明区域鼠标事件 |
| --- | --- | --- |
| 逐像素 Alpha | `UpdateLayeredWindow` + `ULW_ALPHA` | **直接透传**：系统在命中测试阶段就跳过 `alpha=0` 像素，本窗口收不到任何鼠标消息（含 `WM_NCHITTEST`） |
| 色键 | `SetLayeredWindowAttributes` + `LWA_COLORKEY` | **消息转发**：色键区仅视觉透明，鼠标仍命中本窗口；需在 `WM_NCHITTEST` 返回 `HTTRANSPARENT`，系统才继续向下查找 |

> 说明：`LWA_ALPHA`（整体常量透明度）即使很透明，窗口仍“实体存在”，鼠标仍命中本窗口、不会透传，因此不在本示例对比范围。

## 功能

- 创建底层普通窗口（Bottom）与覆盖其上的顶层分层窗口（Overlay）
- Overlay 左半不透明、右半透明，覆盖 Bottom 客户区
- **按空格切换**两种透明方式：
  - 逐像素 Alpha（`UpdateLayeredWindow`）：右半 `alpha=0`
  - 色键（`LWA_COLORKEY`）：右半填充色键色（品红）
- 屏幕实时显示三个事件记录：
  - Overlay 最近 `WM_NCHITTEST`
  - Overlay 最近鼠标事件（`WM_MOUSEMOVE`/`WM_LBUTTONDOWN`）
  - Bottom 最近鼠标事件
- 同时用 `OutputDebugStringW` 输出调试日志

## 编译

```bash
cd LayeredHitTestDemo
make                # 默认 32 位（i686-w64-mingw32-g++）
make ARCH=x86_64    # 64 位
make clean
```

生成文件：`LayeredHitTestDemo.exe`

## 运行

```bash
# 普通 Wine（同时抓取 OutputDebugString 输出）
WINEDEBUG=+debugstr wine LayeredHitTestDemo.exe 2>debug.log

# Wine Wayland
DISPLAY= WAYLAND_DISPLAY=wayland-1 WINEFSYNC=1 wine LayeredHitTestDemo.exe
```

## 验证步骤

1. **默认进入逐像素 Alpha 模式**
   - 鼠标移到 Overlay **左半（不透明）**：
     - “Overlay 最近 hit-test”持续更新为 `WM_NCHITTEST -> HTCLIENT`
     - “Overlay 最近鼠标”更新为 `WM_MOUSEMOVE`
   - 鼠标移到 **右半（透明）**：
     - “Overlay 最近 hit-test”**停止更新**（收不到 `WM_NCHITTEST`）
     - “Bottom 最近鼠标”直接更新为 `WM_MOUSEMOVE`
   - 结论：透明区域事件**直接透传**，Overlay 完全无感知。

2. **按空格切换到色键模式**
   - 鼠标移到 Overlay **左半（非色键色）**：行为同上，命中 Overlay
   - 鼠标移到 **右半（色键透明）**：
     - “Overlay 最近 hit-test”**会更新**为 `WM_NCHITTEST -> HTTRANSPARENT`
     - 随后“Bottom 最近鼠标”才更新
   - 结论：色键透明区域事件**消息转发**——Overlay 先被询问命中测试，
     返回 `HTTRANSPARENT` 后系统才把事件交给下层窗口。

## 技术要点

### 1. 逐像素 Alpha：系统级命中测试透传

```c
BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
UpdateLayeredWindow(hwnd, hdcScreen, &dst, &size,
    hdcMem, &src, 0, &bf, ULW_ALPHA);
```

提供 32 位 ARGB 位图，`alpha=0` 的像素既视觉透明，也**被系统命中测试
跳过**。本窗口的窗口过程不会被询问，鼠标消息直接发往 Z 序中下一个
窗口。这是一种“硬件级/系统级”的穿透，无需应用代码参与。

### 2. 色键：链式命中测试

```c
SetLayeredWindowAttributes(hwnd, KEY_COLOR, 0, LWA_COLORKEY);
```

色键色像素视觉透明，但**命中测试仍命中本窗口**。要让鼠标事件穿过，
必须在 `WM_NCHITTEST` 中对透明区域返回 `HTTRANSPARENT`：

```c
case WM_NCHITTEST:
    if (在透明区域)
        return HTTRANSPARENT;   // 告诉系统：我不处理，请继续向下找
    return DefWindowProc(...);  // 否则正常命中
```

`HTTRANSPARENT` 让系统沿 Z 序继续向下查找下一个窗口，相当于“消息转发”。
两者机制完全不同：

| 特征 | 逐像素 Alpha | 色键 + `HTTRANSPARENT` |
| --- | --- | --- |
| Overlay 是否收到 `WM_NCHITTEST` | 否（直接跳过） | 是（先询问） |
| 谁决定穿透 | 系统（按 alpha 像素） | 应用代码（返回值） |
| 粒度 | 像素级，任意形状 | 应用自行判断，任意形状 |
| 性能 | 系统原生，高效 | 每次鼠标移动都进入窗口过程 |

## Wine/Wine Wayland 测试意义

- `UpdateLayeredWindow` + `ULW_ALPHA` 的逐像素 Alpha 命中测试在 Wine 下
  是否与 Windows 一致（透明区是否真正透传鼠标）
- `SetLayeredWindowAttributes` + `LWA_COLORKEY` 的色键合成是否正确
- `WM_NCHITTEST` 返回 `HTTRANSPARENT` 后，Wine 是否正确沿 Z 序转发
- 同一分层窗口在两种透明方式间动态切换的表现

## 相关示例

本目录（`layered-window/`）下的互补示例：

| 示例 | API | 说明 |
| --- | --- | --- |
| `SimpleLayeredWindow` | `LWA_COLORKEY` | 色键透明 + `HTTRANSPARENT` 点击穿透 |
| `LayeredAlphaDemo` | `LWA_ALPHA` | 整体常量 Alpha 半透明 |
| `UpdateLayeredWindowDemo` | `ULW_ALPHA` | 逐像素 Alpha 任意形状 |
| `ShadowBorderDemo` | `ULW_ALPHA` | 用逐像素 Alpha 实现窗口外阴影 |
| `LayeredChildWindow` | 子窗口 + `LWA_COLORKEY` | 子窗口作为分层窗口的注意事项 |
| **`LayeredHitTestDemo`**（本示例） | 两种对比 | 验证透明区域鼠标事件传递机制 |
