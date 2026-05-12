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

## Wine/Wine Wayland 测试意义

- 托盘图标显示和隐藏
- 右键菜单弹出位置
- 自绘菜单字体渲染
- 窗口隐藏/显示状态切换
- 自定义弹出窗口 vs TrackPopupMenu 行为对比
