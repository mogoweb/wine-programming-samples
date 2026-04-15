# SetForegroundWindow Demo

演示 `SetForegroundWindow` API 的用法，实现两个窗口之间的焦点切换和前台控制。

## 功能说明

本示例创建两个独立的窗口：
- **主控制台**: 主窗口，包含"把【工具面板】叫到前台来！"按钮
- **工具面板**: 工具窗口，包含"回到【主控制台】"按钮

两个窗口在同一线程中运行，通过 `SetForegroundWindow` 实现互相激活：

1. 主窗口点击按钮 → 调用 ShowWindow(SW_RESTORE) + SetForegroundWindow → 工具窗口获得前台焦点
2. 工具窗口点击按钮 → 同样的操作 → 主窗口获得前台焦点

## 技术要点

| 步骤 | API | 作用 |
|------|-----|------|
| 1 | `ShowWindow(hwnd, SW_RESTORE)` | 如果窗口被最小化，先恢复并显示 |
| 2 | `SetForegroundWindow(hwnd)`` | 将窗口带到前台并赋予键盘/鼠标焦点 |

## Wine/Wine Wayland 测试

本示例用于测试 Wine/Wine Wayland 下的前台窗口切换行为。

## 编译

```bash
cd window/setforgroundwindow
make
```

生成的可执行文件：`setforegroundwindow.exe`

默认使用 `i686-w64-mingw32-gcc` 编译 32-bit 程序。

如需原生 Windows 编译（MSYS2/MinGW）：
```bash
make CC=gcc
```

## 运行

```bash
# 普通 Wine
wine setforegroundwindow.exe

# Wine Wayland
DISPLAY= WAYLAND_DISPLAY=wayland-1 WINEFSYNC=1 wine setforegroundwindow.exe
```

## 使用方法

1. 运行程序后会出现两个窗口
2. 点击主窗口的按钮，观察工具窗口是否获得焦点
3. 点击工具窗口的按钮，观察主窗口是否获得焦点
4. 可以最小化其中一个窗口，再看焦点切换是否能正确恢复

## 文件说明

- `main.c` - 主程序源码
- `Makefile` - MinGW 编译配置
