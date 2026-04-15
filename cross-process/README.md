# 跨进程窗口位置控制 Demo

演示跨进程使用 `SetWindowPos` 和 `MoveWindow` API 控制另一个进程窗口的位置和大小。

## 程序说明

| 文件 | 说明 |
|------|------|
| target.c | 目标窗口程序，创建一个蓝色窗口并打印其 HWND |
| controller.c | 控制器程序，通过 HWND 跨进程设置目标窗口的位置和大小 |

## 编译

```bash
cd cross-process
make
```

生成文件：
- `target.exe` - 目标窗口程序
- `controller.exe` - 控制器程序

## 运行

### 步骤 1：启动目标窗口

```bash
# 普通 Wine
wine target.exe

# Wine Wayland
DISPLAY= WAYLAND_DISPLAY=wayland-1 WINEFSYNC=1 wine target.exe
```

终端会输出类似：

```
=== Target Window ===
HWND: 0x0002004e
PID: 1234

Run controller to move this window:
./controller.exe 0x0002004e
```

### 步骤 2：运行控制器

新开一个终端，运行：

```bash
# 普通 Wine
wine controller.exe 0x0002004e

# Wine Wayland
DISPLAY= WAYLAND_DISPLAY=wayland-1 WINEFSYNC=1 wine controller.exe 0x0002004e
```

## 演示内容

控制器会依次执行以下操作：

| 步骤 | API | 操作 |
|------|-----|------|
| 1 | SetWindowPos | 移动窗口到 (0, 0) |
| 2 | SetWindowPos | 移动窗口到屏幕中央 |
| 3 | SetWindowPos | 调整窗口大小为 600x400 |
| 4 | SetWindowPos | 同时移动到 (200, 200) 并调整为 500x350 |
| 5 | MoveWindow | 使用 MoveWindow 设置位置和大小 |
| 6 | SetWindowPos | 设置窗口为置顶 (HWND_TOPMOST) |
| 7 | SetWindowPos | 取消窗口置顶 (HWND_NOTOPMOST) |
| 8 | SetWindowPos | 恢复原始位置和大小 |

## 技术要点

### SetWindowPos 参数

```c
BOOL SetWindowPos(
    HWND hWnd,              // 目标窗口句柄
    HWND hWndInsertAfter,   // Z序位置 (HWND_TOP, HWND_BOTTOM, HWND_TOPMOST, HWND_NOTOPMOST)
    int X, int Y,           // 新位置
    int cx, int cy,         // 新大小
    UINT uFlags             // 标志位
);
```

常用标志位：
- `SWP_NOSIZE` - 保持当前大小
- `SWP_NOMOVE` - 保持当前位置
- `SWP_NOZORDER` - 保持当前 Z 序

### MoveWindow

```c
BOOL MoveWindow(
    HWND hWnd,      // 目标窗口句柄
    int X, int Y,   // 新位置
    int nWidth, int nHeight,  // 新大小
    BOOL bRepaint   // 是否重绘
);
```

`MoveWindow` 是 `SetWindowPos` 的简化版本，内部调用 `SetWindowPos`。

## Wine/Wine Wayland 测试意义

测试跨进程窗口操作在 Wine/Wine Wayland 下的行为：
- 窗口位置是否正确更新
- 窗口大小调整是否生效
- 置顶功能是否正常工作
- Wayland 协议限制下的行为差异
