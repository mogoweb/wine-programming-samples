# IE (Internet Explorer) 相关示例

本目录包含 Internet Explorer / WebBrowser 控件编程示例，用于研究 Wine 的兼容性。

## 示例列表

| 示例 | 描述 | 产物 |
|------|------|------|
| [call_external](./call_external/) | WebBrowser Host + window.external 实现 | `qahost.exe` |
| [activex_form](./activex_form/) | 窗口化 ActiveX 控件 (OCX) 示例：label+文本框+提交按钮，分析 Wine 中 ActiveX 渲染流程 | `axformctl.dll` + `testpage.html` |
| [pageoffice](./pageoffice/) | PageOffice 5.x ActiveX 控件内嵌演示（测试页 + 客户端安装程序，无源码） | `test.html` / `posetup.exe` |
| [embedword](./embedword/) | DsoFramer（dsoframer.ocx）方案内嵌 Word 文档演示页面 | `index.html` |

## 快速开始

```bash
cd call_external
make

# 运行示例
wine qahost.exe http://example.com
```

## 核心技术要点

### WebBrowser 控件 (IWebBrowser2)

Internet Explorer 的核心组件，通过 OLE/COM 嵌入到应用程序中：

```c
// 创建 WebBrowser 实例
IOleObject *oo = NULL;
CoCreateInstance(&CLSID_WebBrowser, NULL, CLSCTX_INPROC_SERVER, 
                 &IID_IOleObject, (void**)&oo);

// 设置客户端站点
IOleObject_SetClientSite(oo, &site->IOleClientSite_iface);
OleSetContainedObject((IUnknown*)oo, TRUE);

// 就地激活
IOleObject_DoVerb(oo, OLEIVERB_INPLACEACTIVATE, ...);

// 获取 IWebBrowser2 接口
IWebBrowser2 *wb = NULL;
IOleObject_QueryInterface(oo, &IID_IWebBrowser2, (void**)&wb);
```

### window.external (IDispatch)

实现 JavaScript 与宿主程序的双向通信：

```c
// C++ 端实现 IDispatch 接口
static HRESULT STDMETHODCALLTYPE ED_Invoke(IDispatch *f, DISPID id, ...) {
    if (id == DISPID_DOCOMMAND && p->cArgs >= 2) {
        // 解析 JavaScript 传来的命令
        VARIANT *a = p->rgvarg;
        if (V_VT(&a[1]) == VT_BSTR) {
            // 执行对应的 C++ 函数
            do_cmd(t->hwnd, cid, obj);
        }
    }
}
```

```javascript
// JavaScript 端调用
window.external.doCommand('adjustwh', {width: 494, height: 353});
window.external.doCommand('close_window', {});
```

### 客户端站点接口

实现完整的 OLE 客户端站点层次结构：

| 接口 | 用途 |
|------|------|
| `IOleClientSite` | 基础客户端站点，管理对象生命周期 |
| `IOleInPlaceSite` | 就地激活支持，处理窗口尺寸变化 |
| `IDocHostUIHandler` | 自定义 UI 行为（上下文菜单、键盘加速键等） |
| `IOleInPlaceFrame` | 框架窗口支持（菜单、工具栏集成） |

## Wine 测试要点

| 功能 | 测试目标 |
|------|----------|
| WebBrowser 创建 | CoCreateInstance 是否成功加载 MSHTML |
| 就地激活 | DoVerb(OLEIVERB_INPLACEACTIVATE) 是否正常 |
| 页面导航 | Navigate2 是否能加载本地/远程 URL |
| JavaScript 执行 | 页面脚本是否正常运行 |
| window.external | JavaScript → C++ 调用是否畅通 |
| 窗口尺寸同步 | WM_SIZE 时 SetObjectRects 是否生效 |
| COM 生命周期 | 引用计数管理是否正确 |

## Wine 运行命令

```bash
wine qahost.exe
```

## 目录结构

```
ie/
├── README.md
├── call_external/
│   ├── qahost.c              # WebBrowser Host 完整实现
│   ├── loginpage.html        # 示例 HTML 页面（演示 JavaScript 交互）
│   ├── Makefile
│   ├── js/
│   │   ├── jquery-1.7.2.js   # jQuery 库
│   │   ├── jquery-1.7.2.min.js
│   │   └── qasui.js          # JavaScript 桥接库
│   └── README.md
├── pageoffice/
│   ├── test.html             # PageOffice ActiveX 内嵌测试页
│   ├── posetup.exe           # PageOffice 5.x 客户端安装程序（32 位，LFS 存储）
│   └── README.md
├── activex_form/
│   ├── axformctl.c           # 窗口化 ActiveX 控件完整实现（纯 Win32/COM）
│   ├── axformctl.def         # COM DLL 导出定义
│   ├── testpage.html         # <object> 嵌入测试页 + JS 调用
│   ├── Makefile              # 交叉编译 + make run 一键注册运行
│   └── README.md             # 含 Wine 中 ActiveX 渲染流程实测分析
└── embedword/
    ├── index.html            # DsoFramer 控件内嵌 Word 演示页
    └── README.md
```

## 编译依赖

- MinGW-w64 (i686-w64-mingw32-gcc)
- Windows SDK headers (mshtmhst.h, exdisp.h)
- 链接库: ole32, oleaut32, uuid, urlmon, user32, gdi32