# 系统托盘 Demo

演示 Windows 系统托盘（Tray Icon）功能，包括托盘图标创建、右键菜单和窗口隐藏/显示。

## 功能

- 启动时显示主窗口
- 点击窗口关闭按钮，窗口隐藏到系统托盘
- 右键点击托盘图标显示菜单：
  - **显示主窗口** - 重新显示窗口
  - **退出** - 完全退出程序
- 双击托盘图标可快速显示主窗口

## 编译

```bash
cd tray-demo
make
```

生成文件：
- `tray-demo.exe` - 使用 TrackPopupMenu API 的版本
- `tray-demo-menu-window.exe` - 使用自定义窗口模拟菜单的版本

## 运行

```bash
# TrackPopupMenu 版本
wine tray-demo.exe

# 自定义菜单窗口版本
wine tray-demo-menu-window.exe

# Wine Wayland
DISPLAY= WAYLAND_DISPLAY=wayland-1 WINEFSYNC=1 wine tray-demo.exe
```

## 技术要点

### Shell_NotifyIcon

```c
// 添加托盘图标
Shell_NotifyIcon(NIM_ADD, &nid);

// 删除托盘图标
Shell_NotifyIcon(NIM_DELETE, &nid);

// 修改托盘图标
Shell_NotifyIcon(NIM_MODIFY, &nid);
```

### NOTIFYICONDATA 结构

```c
NOTIFYICONDATA nid = {0};
nid.cbSize = sizeof(NOTIFYICONDATA);
nid.hWnd = hwnd;                    // 关联窗口
nid.uID = 1;                        // 图标 ID
nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
nid.uCallbackMessage = WM_TRAYICON; // 自定义消息
nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);  // 图标
wcscpy(nid.szTip, L"托盘提示");      // 提示文本
```

### 托盘事件处理

```c
case WM_TRAYICON:
    if (LOWORD(lParam) == WM_RBUTTONUP) {
        // 右键点击 - 显示菜单
    } else if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
        // 左键双击 - 显示窗口
    }
    return 0;
```

### 自绘菜单 (Owner-Drawn Menu)

为了在 Wine 下正确显示中文菜单，使用了自绘菜单：

- `WM_MEASUREITEM` - 计算菜单项尺寸
- `WM_DRAWITEM` - 绘制菜单项内容

通过自绘菜单可以指定字体，确保中文正确显示。

### 自定义菜单窗口 (menu-window.c)

不使用 TrackPopupMenu API，而是创建 WS_POPUP 窗口模拟弹出菜单：

- 创建 `WS_EX_TOPMOST | WS_EX_TOOLWINDOW` 样式的弹出窗口
- 手动处理 `WM_PAINT` 绘制菜单项
- 处理 `WM_MOUSEMOVE` 实现悬停高亮
- 处理 `WM_LBUTTONUP`/`WM_RBUTTONUP` 执行菜单命令
- 使用 `SetCapture` 捕获鼠标，点击菜单外关闭

优点：
- 完全控制菜单外观和行为
- 不依赖系统菜单 API
- 便于调试 Wine/Wayland 菜单相关问题

### 自定义托盘图标

程序启动时会尝试从 exe 同目录加载 `tray-icon.ico` 文件作为托盘图标：

```
程序目录/
├── tray-demo.exe
└── tray-icon.ico    # 可选的自定义图标
```

如果图标文件不存在，则使用系统默认图标。

图标文件要求：
- 格式：ICO 文件
- 推荐尺寸：16x16 或 32x32 像素
- 可包含多种尺寸以适应不同 DPI

## Wine/Wine Wayland 测试意义

- 托盘图标显示和隐藏
- 右键菜单弹出位置
- 自绘菜单字体渲染
- 窗口隐藏/显示状态切换
- 自定义弹出窗口 vs TrackPopupMenu 行为对比

## X11 参考实现 (x11-mouse.c)

`x11-mouse.c` 是纯 X11 实现，不依赖 Windows API，用于对比理解托盘工作原理：

### 编译

```bash
gcc -o x11-mouse x11-mouse.c -lX11
./x11-mouse
```

### 核心概念

#### _NET_SYSTEM_TRAY 协议

X11 系统托盘遵循 [FreeDesktop.org System Tray 规范](https://www.freedesktop.org/wiki/Specifications/system-tray-specification/)：

```c
// 获取托盘管理器
Atom tray_atom = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
Window tray = XGetSelectionOwner(dpy, tray_atom);

// 请求停靠到托盘
XEvent ev;
ev.xclient.type = ClientMessage;
ev.xclient.message_type = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
ev.xclient.data.l[0] = CurrentTime;
ev.xclient.data.l[1] = 0;  // SYSTEM_TRAY_REQUEST_DOCK
ev.xclient.data.l[2] = tray_icon;  // 我们的图标窗口
XSendEvent(dpy, tray, False, NoEventMask, &ev);
```

#### 托盘图标窗口

```c
// 创建小窗口作为托盘图标
tray_icon = XCreateSimpleWindow(
    dpy,
    RootWindow(dpy, screen),
    0, 0,
    TRAY_ICON_SIZE, TRAY_ICON_SIZE,  // 22x22 像素
    0, BlackPixel(dpy, screen),
    WhitePixel(dpy, screen));

XSelectInput(dpy, tray_icon,
             ExposureMask | ButtonPressMask);

// 处理鼠标事件
case ButtonPress:
    if (bev->button == Button1) {
        show_main_window();       // 左键显示窗口
    } else if (bev->button == Button3) {
        show_menu(bev->x_root, bev->y_root);  // 右键显示菜单
    }
```

#### Wine 的托盘实现

Wine 在 Wayland 环境下需要通过类似方式与系统托盘交互：

| Windows API | X11 等价实现 |
|-------------|-------------|
| `Shell_NotifyIcon(NIM_ADD)` | `XGetSelectionOwner` + `XSendEvent` |
| `NOTIFYICONDATA.hWnd` | 事件路由到对应窗口 |
| `NIM_MODIFY` | 更新托盘窗口内容 |
| `NIM_DELETE` | `XDestroyWindow` |

### 功能对比

| 功能 | x11-mouse.c | Windows |
|------|-------------|---------|
| 托盘图标 | ✅ | ✅ |
| 左键点击 | ✅ 显示窗口 | ✅ |
| 右键菜单 | ✅ 简单菜单窗口 | ✅ TrackPopupMenu |
| 窗口隐藏 | ✅ XUnmapWindow | ✅ ShowWindow(SW_HIDE) |
