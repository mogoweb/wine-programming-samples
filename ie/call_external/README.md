# WebBrowser Host + window.external 示例

一个纯 Win32 API 实现的 WebBrowser 宿主程序，演示如何嵌入 IE 控件并实现 JavaScript 与 C++ 的双向通信。

## 功能特性

- 使用纯 Win32 API（无 MFC、无 ATL）
- 实现完整的 OLE 客户端站点层次结构
- 支持 `window.external` 调用 C++ 函数
- 支持窗口尺寸调整、居中、拖动等操作
- 示例 HTML 页面演示 JavaScript 交互

## 编译

```bash
# 交叉编译 (Linux)
make

# 原生编译 (MSYS2/MinGW)
make CC=gcc
```

## 运行

```bash
# 直接运行
wine qahost.exe

# 指定 URL
wine qahost.exe http://example.com

# 运行示例页面
wine qahost.exe http://10.20.33.129:8089/adjustwh/loginpage.html
```

## 架构说明

### 组件结构

```
┌─────────────────────────────────────────┐
│           Main Window (HWND)            │
│  ┌─────────────────────────────────┐    │
│  │      WebBrowser Control         │    │
│  │    (IWebBrowser2 / MSHTML)      │    │
│  └─────────────────────────────────┘    │
│                                         │
│  ┌─────────────────────────────────┐    │
│  │      DocHostSite                │    │
│  │  ├─ IOleClientSite              │    │
│  │  ├─ IOleInPlaceSite             │    │
│  │  ├─ IDocHostUIHandler           │    │
│  │  └─ IOleInPlaceFrame            │    │
│  └─────────────────────────────────┘    │
│                                         │
│  ┌─────────────────────────────────┐    │
│  │      ExternalDispatch           │    │
│  │      (window.external)          │    │
│  └─────────────────────────────────┘    │
└─────────────────────────────────────────┘
```

### 支持的 JavaScript 命令

| 命令 | 参数 | 说明 |
|------|------|------|
| `adjustwh` | `{width, height}` | 调整窗口尺寸 |
| `set_center` | `{}` | 窗口居中 |
| `drag_window` | `{}` | 拖动窗口 |
| `set_modalresult` | `{result}` | 设置对话框结果 (2=退出) |
| `close_window` | `{}` | 关闭窗口 |
| `min_wndow` | `{}` | 最小化窗口 |
| `hide_window` | `{}` | 隐藏窗口 |

### JavaScript 调用示例

```javascript
// 调整窗口尺寸
window.external.doCommand('adjustwh', {width: 494, height: 353});

// 窗口居中
window.external.doCommand('set_center', {});

// 关闭窗口
window.external.doCommand('close_window', {});

// 拖动窗口
window.external.doCommand('drag_window', {});
```

## 文件说明

| 文件 | 说明 |
|------|------|
| `qahost.c` | WebBrowser Host 完整实现 (308 行) |
| `loginpage.html` | 示例 HTML 登录页面 |
| `js/qasui.js` | JavaScript 桥接库 |
| `js/jquery-1.7.2.js` | jQuery 库 |
| `Makefile` | 编译配置 |

## Wine 兼容性

### 已验证功能

- ✅ WebBrowser 控件创建
- ✅ 页面导航 (Navigate2)
- ✅ JavaScript 执行
- ✅ window.external 调用
- ✅ 窗口尺寸调整
- ✅ COM 引用计数管理

### 已知问题

- 某些复杂 CSS 布局可能渲染异常
- 部分 ActiveX 控件可能无法加载
- JavaScript 弹窗 (alert) 行为可能与原生 IE 不同

## 扩展开发

### 添加新的 JavaScript 命令

1. 在 `cmd_map` 数组中添加命令映射：
```c
static const struct { const char *name; int id; } cmd_map[] = {
    {"adjustwh", 1}, {"set_center", 2}, {"drag_window", 3},
    {"set_modalresult", 4}, {"close_window", 5}, {"min_wndow", 6}, {"hide_window", 7},
    {"new_command", 8},  // 新命令
};
```

2. 在 `do_cmd` 函数中添加处理逻辑：
```c
case 8: /* new_command */
    // 处理新命令
    break;
```

### 自定义 IDocHostUIHandler 行为

修改 `IDocHostUIHandlerVtbl` 中的函数实现：
```c
static HRESULT STDMETHODCALLTYPE UI_ShowContextMenu(IDocHostUIHandler *f, ...) {
    // 自定义右键菜单
    return S_OK;  // 返回 S_OK 表示已处理
}
```