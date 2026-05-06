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

生成文件：`tray-demo.exe`

## 运行

```bash
# 普通 Wine
wine tray-demo.exe

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

## Wine/Wine Wayland 测试意义

- 托盘图标显示和隐藏
- 右键菜单弹出位置
- 自绘菜单字体渲染
- 窗口隐藏/显示状态切换
