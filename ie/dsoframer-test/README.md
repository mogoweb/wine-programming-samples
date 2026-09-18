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

1. **Toggle Toolbar 后 WPS 区域不刷新**：`put_Toolbars` →
   `OLECMDID_HIDETOOLBARS` 后 WPS 自身窗口尺寸变化不触发同步重绘，
   鼠标移过区域才逐步刷新（直接暴露 DC 无 WM_PAINT 链）。宿主
   `RedrawWindow(RDW_ALLCHILDREN|RDW_UPDATENOW)` 可强刷，但随后
   resize 布局链路会异常（控件认为组件失活，画 "inactive document"）。
2. **窗口 resize/maximize 后 WPS 区域不跟随**：`DSOFramerDocWnd`
   （文档 site 窗口）保持旧尺寸。控件 `OnResize` →
   `CDsoDocObject::OnNotifySizeChange` → `SetWindowPos(m_hwnd)` 链路
   中 `m_Size`/`m_rcViewRect` 未按预期更新，或 `SetWindowPos` 后
   WPS 嵌套 QWidget 子窗口未级联 resize。

调试辅助：`wininfo.c` / `framerprobe.c`（`make wininfo` 需手工编译），
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
