# 系统托盘示例集

本目录包含多个 Windows 系统托盘（System Tray / Notify Icon）编程示例，用于研究 Wine/Wine Wayland 的兼容性。

## 示例列表

| 示例 | 描述 | 编译产物 |
|------|------|----------|
| [tray-demo](./tray-demo/) | 基础托盘功能：图标、菜单、窗口隐藏 | `tray-demo.exe`, `tray-demo-menu-window.exe` |
| [tray-alpha-demo](./tray-alpha-demo/) | 透明通道图标 + 动态切换 | `tray-alpha-demo.exe` |
| [tray-switch-demo](./tray-switch-demo/) | 托盘图标动态切换演示 | `tray-switch-demo.exe` |

## 快速开始

```bash
# 编译所有示例
cd tray-demo && make
cd ../tray-alpha-demo && make
cd ../tray-switch-demo && make

# 运行示例
wine tray-demo/tray-demo.exe
```

## 核心技术要点

### Shell_NotifyIcon API

系统托盘的核心 API，用于添加、删除、修改托盘图标：

```c
Shell_NotifyIcon(NIM_ADD, &nid);      // 添加图标
Shell_NotifyIcon(NIM_DELETE, &nid);    // 删除图标
Shell_NotifyIcon(NIM_MODIFY, &nid);   // 修改图标
```

### NOTIFYICONDATA 结构

```c
NOTIFYICONDATA nid = {0};
nid.cbSize = sizeof(NOTIFYICONDATA);
nid.hWnd = hwnd;                              // 关联窗口句柄
nid.uID = 1;                                  // 图标唯一 ID
nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
nid.uCallbackMessage = WM_TRAYICON;            // 自定义消息
nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);  // 图标句柄
wcscpy(nid.szTip, L"托盘提示文本");            // 悬停提示
```

### 托盘事件处理

```c
case WM_TRAYICON:
    switch (LOWORD(lParam)) {
        case WM_RBUTTONUP:     // 右键点击
            ShowTrayMenu();
            break;
        case WM_LBUTTONUP:    // 左键点击
            ShowWindow(hwnd, SW_SHOW);
            break;
        case WM_LBUTTONDBLCLK: // 左键双击
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
            break;
    }
    return 0;
```

### 创建带 Alpha 通道的图标

使用 `CreateIconIndirect` 从 32 位 RGBA 位图创建图标：

```c
ICONINFO ii = {0};
ii.fIcon = TRUE;
ii.hbmColor = hColorBitmap;  // 32位 RGBA 位图
ii.hbmMask = hMaskBitmap;    // 单色掩码

HICON hIcon = CreateIconIndirect(&ii);
```

### 动态修改托盘图标

```c
// 更新图标
nid.hIcon = hNewIcon;
Shell_NotifyIcon(NIM_MODIFY, &nid);

// 更新提示文本
wcscpy(nid.szTip, L"新提示");
Shell_NotifyIcon(NIM_MODIFY, &nid);
```

## Wine/Wine Wayland 测试要点

| 功能 | 测试目标 |
|------|----------|
| 托盘图标显示 | 图标是否正确出现在系统托盘区域 |
| 透明通道渲染 | Alpha 通道图标是否正常显示 |
| 右键菜单弹出 | 菜单位置是否正确，内容是否显示 |
| 自绘菜单 | Owner-Drawn 菜单在 Wine 下的兼容性 |
| 图标动态切换 | NIM_MODIFY 是否正常工作 |
| 窗口隐藏/显示 | 托盘图标与窗口状态同步 |

## Wine Wayland 运行命令

```bash
DISPLAY= WAYLAND_DISPLAY=wayland-1 WINEFSYNC=1 wine <demo.exe>
```

## 目录结构

```
systray/
├── tray-demo/              # 基础托盘功能
│   ├── main.c             # TrackPopupMenu 版本
│   ├── menu-window.c      # 自定义窗口菜单版本
│   ├── Makefile
│   ├── tray-icon.ico       # 可选自定义图标
│   └── README.md
├── tray-alpha-demo/        # 透明图标演示
│   ├── main.c             # 带 Alpha 通道的图标创建
│   ├── Makefile
│   └── README.md
└── tray-switch-demo/       # 图标切换演示
    ├── main.c
    ├── Makefile
    └── README.md
```
