# dsoframer-test

DsoFramer `Vb6Test` 样例（`../DsoFramer/Samples/Vb6Test/Src/FTestApp.frm`）的
C++ / 纯 Win32 API 等价实现。不使用 MFC/ATL/VB，仅 Win32 API + 原生 COM。

## 功能对照

| VB6 原版 | C++ 版 |
|---|---|
| `FTestApp.frm` 主窗体 + `oFramer` 控件 | `main.cpp` 主窗口 + `framerhost.cpp` 就地激活 OCX |
| `HostName = "VB6TestApp"` | `HostName = "CppTestApp"`（main.cpp） |
| `mnuFileNew/Open/...` 菜单 | `framerapp.rc` 菜单模板（最小核心版：无 Web Open/Save to Web/Print to Target） |
| `Form_Resize` → `oFramer.Move` | `WM_SIZE` → `MoveWindow` + `IOleInPlaceObject::SetObjectRects` |
| `EnableItems()` | `EnableItems()`（完全同逻辑） |
| `oFramer_OnFileCommand`（拦截 Open/SaveAs） | `WINAPI_OnFileCommand`：调 `ShowDialog` 并返回 `Cancel=TRUE` |
| `oFramer_BeforeDocumentClosed`（脏保存三态提示） | `WINAPI_BeforeDocumentClosed`：`IsDirty`→YesNoCancel→`Save`/`ShowDialog(dsoDialogSave)` |
| `oFramer_OnDocumentOpened/Closed` | 状态栏 `Current File: ...` + 菜单启用切换 |
| `oFramer_OnPrintPreviewExit` | 恢复菜单启用 |
| Show 菜单（Caption/Menubar/Toolbar/BorderStyle/DisableFileItem/CustomCaption） | 同名菜单项，勾选态经 `CheckMenuItem` 同步 |

## 关键实现（Wine 兼容性研究点）

1. **免 MIDL 接口声明**（`dsoframer.h`）：`_FramerControl` dual 接口按
   `dsoframer.idl` 声明序手工展开 vtable（propget/propput 各占一槽），
   GUID 用 `DEFINE_GUID` + `initguid.h` 实例化。
2. **宿主 site**（`framerhost.cpp`）：单对象多面
   `IOleClientSite/IOleControlSite/IOleInPlaceSiteEx/IOleInPlaceFrame/IServiceProvider`。
   `TranslateAccelerator` 返回 `S_FALSE` 让控件自行处理按键。
3. **事件接收**：静态 `IDispatch` 汇，`QueryInterface` 同时接受
   `DIID__DFramerCtlEvents` 与 `IID_IDispatch`（对齐控件连接点对 .NET RCW 的
   兼容行为，见 dsofcontrol.cpp:3684）。事件参数按 `rgvarg` 逆序解析，
   `Cancel` 为 `VT_BOOL|VT_BYREF` 回写。
4. **激活链**：`CoCreateInstance → SetClientSite → DoVerb(INPLACEACTIVATE)`，
   与 Wine 中 IE 类宿主一致。

## 构建

```bash
make            # i686-w64-mingw32 交叉编译（控件为 32 位 OCX）
make clean
```

## 运行（deepin-wine 容器）

控件已在容器 `~/.deepinwine/org.deepin-wine.browser.deepin` 注册
（`C:\DsoFramer\dsoframer.ocx`，ProgID `DSOFramer.FramerControl`）：

```bash
make run        # 拷贝 exe 到 drive_c/DsoFramer/ 并用容器 wine 启动
```

## Wine 下的已知差异（deepin-wine 兼容性研究点）

Windows 下行为正常，Wine/deepin-wine 下观察到以下差异（待深挖）：

1. **窗口 resize / toggle Toolbar 后 WPS 区域不跟随**（VB6Test 同样复现）。

   根因（+ole/+rpc trace 定位）：
   - 容器→控件→`DSOFramerDocWnd` 链路全部正常（`SetExtent`/`SetObjectRects`/
     `OnPosRectChange`/DocWnd `SetWindowPos` 均执行）；
   - 断点在 `CDsoDocObject::OnNotifySizeChange` 的两个分支：
     Wine 下 WPS（out-of-proc docobj 服务器）**从不调用
     `IOleInPlaceSite::OnUIActivate`**，`m_fObjectUIActive` 恒为 FALSE，
     `IOleInPlaceActiveObject::ResizeBorder` 全程 0 次调用（+rpc 抓包）；
     `IOleDocumentView::SetRect` 分支同样不生效；
   - 于是 WPS 的 Qt 窗口树（`QWidget→OpusApp→_WwG`）收不到任何尺寸
     通知，保持旧尺寸直到鼠标滑过触发局部重绘。Windows 上 WPS 会走
     UI-active 流程，故无此问题。

   绕过（`main.cpp` 的 `SyncEmbeddedServerWindow`，由
   `DISABLE_WINE_SERVER_SYNC` 编译开关控制）：宿主在布局后枚举
   `DSOFramerDocWnd` 的 QWidget 子窗口，用 `SetWindowPos` 推到 DocWnd
   客户区尺寸。注意跨进程 `SetWindowPos` 不能在 `WM_SIZE` 内联执行
   （Wine 下会阻塞 UI 线程导致菜单卡死），必须 `PostMessage` 延迟 +
   `SWP_ASYNCWINDOWPOS`，并按尺寸去重。Windows 上该调用是 no-op。

   **Wine 层根因与修复（2026-09）**：真正的断点在
   `dlls/ole32/usrmarshal.c` ——
   `IOleInPlaceActiveObject_ResizeBorder_Proxy/_Stub` 是 `E_NOTIMPL`
   空壳（上游 Wine 至今未实现），控件跨进程调用 `ResizeBorder` 被
   proxy 静默吞掉（+ole/+rpc trace 证实调用发生但 RPC 从未发出）。
   deepin-wine10-stable 补丁
   `e94ae9efd9e "ole32: Implement IOleInPlaceActiveObject::ResizeBorder
   proxy/stub"` 实现了 proxy（按 fFrameWindow 推导 riid 转发
   RemoteResizeBorder_Proxy）和 stub（参数透传）后，resize/最大化
   在禁用应用层 workaround 的情况下原生工作。
   Toggle Toolbar 不刷新是另一独立问题：`Exec(OLECMDID_HIDETOOLBARS)`
   RPC 全链路正常送达，WPS 内部布局也已更新，只是 Qt 子窗口未触发
   重绘（鼠标滑过才画）——属于 wine 窗口管理/重绘层，与 OLE 无关。

调试辅助：`wininfo.c` / `framerprobe.c`（`make tools`），
容器内运行可枚举主窗口下 Win32 窗口树（含 WPS 的 QWidget/OpusApp 链），
用于观察哪一层窗口未 resize。

## 验证要点

- 主窗口应出现控件标题栏/菜单栏/工具栏（默认 Outline 边框）
- File → Open 选择 Office 文档（容器内装有 WPS Office）→ 嵌入显示，
  状态栏更新 `Current File: <path>`，stdout 打印 `OnDocumentOpened`
- Show → Caption/Menubar/Toolbar 切换后控件立即重排
- Show → Border Style 四档切换
- Show → Disable File Menu Item 后控件内置文件菜单对应项被禁用
- Show → Custom Caption 设置控件标题栏文字
- 修改文档后 File → Close → 弹出保存提示（`BeforeDocumentClosed`）；
  选 Cancel 时文档保持打开（WM_CLOSE 链被取消）
