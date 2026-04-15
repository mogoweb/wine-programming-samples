# wine-programming-samples

研究 Wine/Wine Wayland 兼容性过程中编写的 Windows 编程示例程序。

## 目录结构

```
├── layered-window/          # Windows 层级窗口演示
│   ├── ShadowBorderDemo/       # 使用层级窗口实现窗口阴影边框
│   ├── SimpleLayeredWindow/    # 最简单的层级窗口演示
│   └── UpdateLayeredWindowDemo/# 使用 UpdateLayeredWindow 方法刷新层级窗口内容
│
├── window/                  # Windows 普通窗口演示
│   ├── modal-dialog/           # 四种模态对话框实现方式演示
│   ├── setforgroundwindow/     # SetForegroundWindow API 焦点切换演示
│   ├── setparent/              # 跨进程 SetParent 调用演示
│   └── viewporter/             # Wayland wp_viewporter 扩展演示
│
├── cross-process/           # 跨进程窗口操作演示
│
├── hello-win/               # 基础 Win32 窗口示例，输出窗口几何信息
├── hello-edit/              # IME (Input Method Editor) 消息处理演示
├── hello-time/              # Windows 时间 API 演示 (GetLocalTime/GetSystemTime)
├── hello-vulkan/            # Vulkan 实例演示
├── hello-cef/               # CEF (Chromium Embedded Framework) 最小集成示例
├── hello-wine/              # Wine 基础示例
├── wine-wayland/            # Wine Wayland 相关示例
└── windows-graphics-programming/ # Windows 图形编程示例
```

## 编译

### 交叉编译 (Linux)

大多数示例使用 MinGW-w64 进行交叉编译：

```bash
cd <sample-directory>
make
```

常用编译目标：
- `make` - 构建所有目标 (通常生成 .exe 文件)
- `make clean` - 清理构建产物

默认编译器：
- `i686-w64-mingw32-gcc` / `i686-w64-mingw32-g++` (32-bit Windows)
- `x86_64-w64-mingw32-g++` (64-bit Windows，如 hello-cef)

### 原生 Windows 编译 (MSYS2/MinGW)

```bash
make CC=gcc
```

### 特殊说明

- **hello-vulkan**: 使用原生 `gcc` 配合 `-lvulkan` (Linux 原生，非 Windows)
- **hello-edit**: 链接 `-limm32 -lgdi32 -static` 以支持 IME
- **hello-cef**: 需要 CEF SDK 位于 `sdk/` 目录

## 运行

示例程序设计用于在 Wine/Wine Wayland 下运行：

```bash
# 普通 Wine
wine sample.exe

# Wine Wayland
DISPLAY= WAYLAND_DISPLAY=wayland-1 WINEFSYNC=1 wine sample.exe
```

## 示例说明

### layered-window/ - 层级窗口

演示 Windows 层级窗口 API 的使用，实现 per-pixel alpha 透明效果。

| 示例 | 说明 |
|------|------|
| SimpleLayeredWindow | 最基础的层级窗口创建 |
| ShadowBorderDemo | 使用层级窗口实现窗口阴影边框效果 |
| UpdateLayeredWindowDemo | 使用 `UpdateLayeredWindow` 高效更新层级窗口内容 |

### window/ - 窗口管理

| 示例 | 说明 |
|------|------|
| modal-dialog | 演示四种模态对话框实现：MessageBox、DialogBox、DialogBoxParam、手动 EnableWindow |
| setforgroundwindow | 演示 `SetForegroundWindow` 实现窗口焦点切换 |
| setparent | 演示跨进程 `SetParent` 调用，将子窗口附加到另一个进程的父窗口 |
| viewporter | 演示 Wayland `wp_viewporter` 协议扩展，实现窗口内容缩放 |

### cross-process/ - 跨进程窗口操作

| 示例 | 说明 |
|------|------|
| target | 目标窗口程序，创建窗口并打印 HWND |
| controller | 跨进程使用 `SetWindowPos`/`MoveWindow` 控制目标窗口位置和大小 |

### hello-* 系列

| 示例 | 说明 |
|------|------|
| hello-win | 基础 Win32 窗口创建，输出窗口位置和尺寸信息 |
| hello-edit | IME 消息处理演示，监控键盘输入和输入法事件 |
| hello-time | Windows 时间 API 演示 (`GetLocalTime`, `GetSystemTime`, `FILETIME`) |
| hello-vulkan | Vulkan 实例创建和销毁演示 |
| hello-cef | CEF 最小集成示例，嵌入 Chromium 浏览器 |
| hello-wine | Wine 基础测试示例 |

## 许可证

MIT License - 详见 [LICENSE](LICENSE)
