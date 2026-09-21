# ActiveX 表单控件示例 (axformctl)

一个纯 Win32/COM API 实现的**窗口化 (windowed) ActiveX 控件 (OCX)**，无 MFC/ATL/类型库。
通过网页 `<object classid="clsid:...">` 标签嵌入 IE (Wine 下为 Wine 自带 iexplore/mshtml)，
控件内包含 **Label + 文本框 + 提交按钮**，点击提交按钮弹出一个消息框，显示文本框中输入的内容。

**目的**：以最小实现观察 ActiveX 控件在 Wine 中的完整加载与渲染流程。

## 功能

- `<object>` 标签按 CLSID 加载控件，`<param>` 参数经 `IPersistPropertyBag` 传入
- 控件窗口内含 STATIC label + EDIT 文本框 + BUTTON 提交按钮（全部为真实 Win32 子窗口）
- **背景色可配置**：`<param name="bgcolor" value="#RRGGBB">`（IPersistPropertyBag 路径），
  或 JS/宿主调用 `SetBgColor(COLORREF)`（IDispatch 路径）；WM_PAINT 用该颜色填充
- 点击"提 交"按钮 → `MessageBoxW` 显示文本框内容
- JavaScript 可经 `IDispatch` 调用 `GetText()` / `ShowText()` / `SetBgColor()`
- 所有关键调用以 `[AXFORM]` 前缀输出到 stderr，作为 Wine 渲染流程分析观察点

## 文件说明

| 文件 | 说明 |
|------|------|
| `axformctl.c` | 控件完整实现（COM 类工厂 + 6 个接口 + 控件窗口 + 注册代码） |
| `axformctl.def` | DLL 导出（DllGetClassObject / DllCanUnloadNow / DllRegisterServer / DllUnregisterServer） |
| `axhost.c` | 原生 Win32 宿主窗口：内嵌 AxFormCtl，验证 resize 时控件是否跟随 |
| `testpage.html` | 测试页面（`<object>` 嵌入 + JS 调用按钮） |
| `Makefile` | 交叉编译 + `make run`（MSHTML 容器）/ `make run-host`（原生宿主） |

## 编译与运行

```bash
# 编译 (Linux MinGW-w64 交叉编译)
make

# 拷贝 DLL/测试页到 Wine 容器 C:\axform 并注册
make install

# 注册 + 用容器内 iexplore 打开测试页
make run

# 注册 + 运行原生宿主 axhost.exe（resize 跟随验证）
make run-host

# 卸载注册
make uninstall
```

Makefile 默认参数（可用变量覆盖）：

```bash
make run WINE=~/work/projects/deepin-wine/source/deepin-wine10-stable/build/wine \
         WINEPREFIX=~/.deepinwine/org.deepin-wine.browser.deepin
```

## 标识信息

| 项目 | 值 |
|------|-----|
| CLSID | `5E8F4A2C-1D3B-4C6E-9F70-A1B2C3D4E5F6` |
| ProgID | `AxForm.FormCtl.1` |
| 窗口类 | `AxFormCtlClass` |
| 默认尺寸 | 300x150 (HIMETRIC 7938x3969) |
| ThreadingModel | Apartment |
| 注册键 | `HKCR\CLSID\{...}\{InprocServer32,Control,MiscStatus,ProgID,...}` |
| 测试页 | 容器内 `c:\axform\testpage.html` |

## 实现的接口

| 接口 | 作用 |
|------|------|
| `IClassFactory` | COM 对象工厂（DllGetClassObject 返回） |
| `IDispatch` | 供 JavaScript 调用 GetText / ShowText |
| `IOleObject` | 容器握手：SetClientSite / DoVerb / GetMiscStatus / SetExtent |
| `IOleInPlaceObject` | 就地激活窗口管理：GetWindow / SetObjectRects |
| `IViewObject2` | 无窗口绘制入口（窗口化控件 Draw 仅记录，绘制走自身 WM_PAINT） |
| `IPersistPropertyBag` | 读取 `<object>` 里的 `<param>`（label / caption） |
| `IObjectSafety` | 向 MSHTML 声明脚本安全（MinGW 头文件缺失，手动声明） |

## 原生宿主内嵌与 resize 跟随验证 (axhost.exe)

`axhost.c` 是一个纯 Win32 原生容器（镜像 `ie/call_external/qahost.c` 的宿主侧实现）：
`CoCreateInstance(CLSID_AxFormCtl)` → `SetClientSite` → `OleSetContainedObject` →
`DoVerb(OLEIVERB_INPLACEACTIVATE)` → 控件以 `WS_CHILD` 挂到宿主 HWND 下。

**resize 同步提供两种容器模式**（顶栏菜单切换，对照实验用）：

| 模式 | WM_SIZE 时的同步调用 | 说明 |
|------|---------------------|------|
| A（默认） | `IOleInPlaceObject::SetObjectRects(客户区)` | 标准容器路径（MSHTML 同款） |
| B | `IOleObject::SetExtent(HIMETRIC)` | 依赖控件 SetExtent→MoveWindow 实现的对照路径 |

标题栏实时显示 `[模式] 客户区 WxH`，与 `[AXFORM] WM_SIZE`/`SetObjectRects` 日志对照，
即可确认控件窗口尺寸是否跟随宿主变化。

### 实测结论（wine-10.14 / deepin-wine10-stable）

- **模式 A（SetObjectRects）**：拖拽缩放宿主窗口时，每次 WM_SIZE 触发
  `SetObjectRects` → 控件 `MoveWindow` → 控件 `WM_SIZE` + `WM_PAINT`（日志序列完整），
  控件窗口与宿主客户区同尺寸跟随，无残留/错位。
- **模式 B（SetExtent）**：同样跟随（控件 `SetExtent` 内部走 MoveWindow），
  HIMETRIC↔像素换算经 96dpi 假设，在非 96dpi 环境（Wine 虚拟桌面 dpi 缩放）会有圆整偏差。
- 结论：**窗口化 ActiveX 控件在 Wine 中 resize 跟随由容器驱动**——容器在 WM_SIZE 里
  调 SetObjectRects（或 SetExtent），控件负责 MoveWindow 自身窗口；与 MSHTML 容器行为一致。


## Wine 中的 ActiveX 渲染流程（实测，wine-10.14 / deepin-wine10-stable）

`make run` 后 stderr 得到的完整时序（`[AXFORM]` 日志 + `WINEDEBUG=+loaddll,+ole` 交叉印证）：

```
页面解析 <object classid>
  └─ MSHTML: CoCreateInstance(CLSID, CLSCTX_INPROC_SERVER)
       ├─ DllMain(DLL_PROCESS_ATTACH)                     # 控件 DLL 装载进 iexplore 进程
       ├─ DllGetClassObject(clsid, IID_IClassFactory)     # 取类工厂
       └─ IClassFactory::CreateInstance(IID_IDispatch)    # 创建控件实例
            ├─ QI(IClassFactory 0x13D) -> E_NOINTERFACE   # MSHTML 探测（不应响应）
            ├─ QI(IObjectWithSite)    -> E_NOINTERFACE
            ├─ IOleObject::GetMiscStatus               # 读 SETCLIENTSITEFIRST 等标志决定加载顺序
            ├─ IOleObject::SetClientSite(site)          # 容器站点注入（ MiscStatus|SETCLIENTSITEFIRST 时先于 Load ）
            ├─ QI(IOleControl) / QI(IActiveScript) 等 -> E_NOINTERFACE
            ├─ IPersistPropertyBag::Load(<param>)       # label/caption 参数传入
            ├─ QI(IPersistPropertyBag2) -> E_NOINTERFACE
            ├─ IDispatch::Invoke(DISPID_AMBIENT_*)      # 容器通知环境属性变化 (-525/-514/-5511)
            └─ IOleObject::DoVerb(OLEIVERB_SHOW/-5)
                 ├─ IOleInPlaceSite::CanInPlaceActivate -> S_OK
                 ├─ IOleInPlaceSite::OnInPlaceActivate
                 ├─ IOleInPlaceSite::GetWindow -> 父 HWND  # ★ 渲染树挂载点：MSHTML 宿主窗口
                 ├─ CreateWindowExW(WS_CHILD|WS_VISIBLE)  # ★ 控件窗口挂到 MSHTML 窗口下
                 │    └─ WM_CREATE: 创建 STATIC/EDIT/BUTTON 三个子控件
                 ├─ IOleInPlaceSite::GetWindowContext(pos/clip)
                 ├─ MoveWindow(到 <object> 元素位置)
                 ├─ IOleInPlaceSite::OnUIActivate
                 └─ IOleInPlaceObject::SetObjectRects(pos, clip)  # 容器同步几何

页面渲染/滚动/resize
  └─ MSHTML 反复调用 SetObjectRects 同步 <object> 元素几何
     控件收到 WM_PAINT 自行绘制（窗口化控件由 Wine 窗口管理器合成）

页面卸载 / iexplore 退出
  └─ IOleObject::Close / IOleInPlaceObject::InPlaceDeactivate
     └─ DestroyWindow + DllMain(DLL_PROCESS_DETACH)
```

### 关键结论（Wine 渲染观察）

1. **窗口化控件在 Wine 中是真实 X11/Wayland 子窗口**：`IOleInPlaceSite::GetWindow`
   返回的父窗口即 Wine mshtml 的宿主 HWND，控件 `CreateWindowExW(WS_CHILD)` 后
   由 Wine 窗口管理器合成，与 Linux 侧 XWayland/Wayland surface 一一对应。
2. **几何同步链**：`<object>` 元素位置 → `SetObjectRects` → `MoveWindow`，
   页面滚动时会连续触发（日志可见 SetObjectRects/WM_SIZE 重复出现）。
3. **加载顺序由 MiscStatus 决定**：注册表 `MiscStatus=131457` 含
   `SETCLIENTSITEFIRST(0x20000)`，因此 SetClientSite 先于 IPersistPropertyBag::Load。
4. **MSHTML 大量 QI 探测可选接口**（IOleControl/IActiveScript/IOleCommandTarget/
   IPersistPropertyBag2 等），缺失时全部走 E_NOINTERFACE 回退，不影响加载。
5. **JS → 控件路径**：`GetIDsOfNames('GetText')` → `IObjectSafety` 检查 →
   `IDispatch::Invoke`，Wine 的 jscript 引擎完整实现该链路。

### 推荐联调命令

```bash
# 控件日志 + Wine 加载/OLE 跟踪
WINEDEBUG=+loaddll,+ole,+msgfile DISPLAY=:0 wine "C:\\windows\\syswow64\\iexplore.exe" \
    "file:///c:/axform/testpage.html" 2>&1 | grep -E "AXFORM|loaddll:.*axform"

# 验证 32/64 位注册（32 位 DLL 须注册到 syswow64 视图）
WINEPREFIX=... wine reg query "HKCR\\WOW6432Node\\CLSID\\{5E8F4A2C-...}" /s
```

> 注意：本容器 wine 为 wow64 模式（64 位 iexplore 引导 32 位 syswow64/iexplore.exe），
> 32 位控件的注册信息在 `WOW6432Node\CLSID` 下；运行时须显式执行
> `C:\windows\syswow64\iexplore.exe`（32 位）才能正确加载 32 位 InprocServer32。

## 已验证功能（wine-10.14, deepin-wine10-stable, org.deepin-wine.browser.deepin 容器）

- ✅ regsvr32 注册（HKCR\WOW6432Node\CLSID 完整键树）
- ✅ iexplore 解析 `<object>` → CoCreateInstance → DLL 加载
- ✅ IPersistPropertyBag 读取 `<param>`（label="姓名："）
- ✅ 就地激活 → WS_CHILD 窗口挂载 → STATIC/EDIT/BUTTON 创建
- ✅ SetObjectRects 几何同步
- ✅ 控件内"提 交"按钮 → MessageBox 显示文本框输入内容（实测输入 'ttt'）
- ✅ JS `GetText()` / `ShowText()` 经 IDispatch 调用成功
