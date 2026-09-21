/*
 * axformctl.c - ActiveX 控件 (OCX) 示例：Label + 文本框 + 提交按钮
 *
 * 纯 Win32/COM API 实现的窗口化 (windowed) ActiveX 控件，无 MFC/ATL/类型库。
 * 通过网页 <object classid="clsid:..."> 标签嵌入，由 MSHTML 宿主加载
 * （Wine 下即 wine iexplore / WebBrowser 宿主），用于分析 ActiveX 在 Wine
 * 中的加载与渲染流程。
 *
 * 实现的接口（控件侧，与 ie/call_external 的宿主侧互为镜像）：
 *   IClassFactory        COM 对象工厂
 *   IDispatch            供 JavaScript 调用 (GetText / ShowText)
 *   IOleObject           容器握手：SetClientSite / DoVerb / SetExtent ...
 *   IOleInPlaceObject    就地激活窗口管理：GetWindow / SetObjectRects
 *   IViewObject2         无窗口绘制入口（本控件为窗口化，Draw 仅记录）
 *   IPersistPropertyBag  读取 <object> 里的 <param> 参数
 *   IObjectSafety        向 MSHTML 声明脚本安全，允许 JS 调用
 *
 * 渲染流程观察点：
 *   所有关键调用以 "[AXFORM]" 前缀输出到 stderr，可与
 *   WINEDEBUG=+loaddll,+ole,+mshtml,+win 的 Wine 日志对照，完整还原：
 *   CoCreateInstance -> SetClientSite -> DoVerb -> IOleInPlaceSite::GetWindow
 *   -> CreateWindowExW(WS_CHILD) -> 子控件创建 -> WM_PAINT -> 消息循环。
 *
 * Build: make  (i686-w64-mingw32-gcc -shared -o axformctl.dll axformctl.c axformctl.def ...)
 */

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#define INITGUID

#include <windows.h>
#include <ole2.h>
#include <oleidl.h>
#include <ocidl.h>
#include <olectl.h>
#include <oaidl.h>
#include <oleauto.h>
#include <stdio.h>
#include <stdlib.h>

/* ===== MinGW-w64 头文件缺失 IObjectSafety，手动声明 ===== */
#ifndef INTERFACESAFE_FOR_UNTRUSTED_CALLER
#define INTERFACESAFE_FOR_UNTRUSTED_CALLER 0x00000001
#define INTERFACESAFE_FOR_UNTRUSTED_DATA   0x00000002
#endif

DEFINE_GUID(IID_IObjectSafety, 0xcb5bdc81,0x93c1,0x11cf,0x8f,0x20,0x00,0x80,0x5f,0x2c,0xd0,0x64);

typedef struct IObjectSafety IObjectSafety;
typedef struct IObjectSafetyVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IObjectSafety *, REFIID, void **);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IObjectSafety *);
    ULONG   (STDMETHODCALLTYPE *Release)(IObjectSafety *);
    HRESULT (STDMETHODCALLTYPE *GetInterfaceSafetyOptions)(IObjectSafety *, REFIID, DWORD *, DWORD *);
    HRESULT (STDMETHODCALLTYPE *SetInterfaceSafetyOptions)(IObjectSafety *, REFIID, DWORD, DWORD);
} IObjectSafetyVtbl;
struct IObjectSafety { const IObjectSafetyVtbl *lpVtbl; };

/* ===== 控件标识与常量 ===== */
DEFINE_GUID(CLSID_AxFormCtl, 0x5e8f4a2c,0x1d3b,0x4c6e,0x9f,0x70,0xa1,0xb2,0xc3,0xd4,0xe5,0xf6);
/* {5E8F4A2C-1D3B-4C6E-9F70-A1B2C3D4E5F6} */

#define PROGID          L"AxForm.FormCtl.1"
#define PROGID_VERIND   L"AxForm.FormCtl"
#define CTL_CLASS_W     L"AxFormCtlClass"
#define CTL_W           300
#define CTL_H           150
#define PX2HM(px)       ((LONG)((px) * 2540 / 96))   /* 像素 -> HIMETRIC */

#define IDC_LABEL  1001
#define IDC_EDIT   1002
#define IDC_BUTTON 1003

#define DISPID_GETTEXT    1
#define DISPID_SHOWTEXT   2
#define DISPID_SETBGCOLOR 3

static HINSTANCE g_hinst;
static LONG g_obj_count;   /* 存活控件对象数 */
static LONG g_locks;       /* IClassFactory::LockServer 计数 */

/* ===== 日志：渲染流程观察点，全部打到 stderr ===== */
static void ax_log(const char *fmt, ...)
{
    va_list ap;
    fprintf(stderr, "[AXFORM] ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    fflush(stderr);
}

/* GUID 调试字符串（非线程安全，仅日志用） */
static const char *dbg_guid(REFIID riid)
{
    static WCHAR wbuf[64];
    static char abuf[80];
    if (!StringFromGUID2(riid, wbuf, 64)) return "{?}";
    WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, abuf, sizeof(abuf), NULL, NULL);
    return abuf;
}

/* ===== 控件对象 ===== */
typedef struct {
    IOleObject           IOleObject_iface;
    IOleInPlaceObject    IOleInPlaceObject_iface;
    IViewObject2         IViewObject2_iface;
    IPersistPropertyBag  IPersistPropertyBag_iface;
    IDispatch            IDispatch_iface;
    IObjectSafety        IObjectSafety_iface;
    LONG ref;                       /* 所有接口共享一个引用计数 */
    IOleClientSite   *client_site;  /* MSHTML 提供的客户端站点 */
    IOleInPlaceSite  *inplace_site;
    IOleInPlaceFrame *inplace_frame;
    IOleInPlaceUIWindow *inplace_doc;
    HWND hwnd;                      /* 控件顶层子窗口（父窗口为 MSHTML 宿主窗口） */
    HWND hwnd_label, hwnd_edit, hwnd_button;
    BOOL activated;
    SIZEL extent;                   /* HIMETRIC 逻辑尺寸 */
    IAdviseSink *advise_sink;
    WCHAR label_text[64];           /* <param name="label"> */
    WCHAR caption_text[64];         /* <param name="caption"> */
    COLORREF bgcolor;               /* <param name="bgcolor"> "#RRGGBB"，默认 COLOR_BTNFACE */
} AxFormCtl;

typedef struct {
    IClassFactory IClassFactory_iface;
    LONG ref;
} AxClassFactory;

static IOleObjectVtbl          OleObjectVtbl;
static IOleInPlaceObjectVtbl   OleInPlaceObjectVtbl;
static IViewObject2Vtbl        ViewObject2Vtbl;
static IPersistPropertyBagVtbl PersistPropertyBagVtbl;
static IDispatchVtbl           DispatchVtbl;
static IObjectSafetyVtbl       ObjectSafetyVtbl;
static IClassFactoryVtbl       ClassFactoryVtbl;

static inline AxFormCtl *impl_from_IOleObject(IOleObject *f)          { return CONTAINING_RECORD(f, AxFormCtl, IOleObject_iface); }
static inline AxFormCtl *impl_from_IOleInPlaceObject(IOleInPlaceObject *f) { return CONTAINING_RECORD(f, AxFormCtl, IOleInPlaceObject_iface); }
static inline AxFormCtl *impl_from_IViewObject2(IViewObject2 *f)      { return CONTAINING_RECORD(f, AxFormCtl, IViewObject2_iface); }
static inline AxFormCtl *impl_from_IPersistPropertyBag(IPersistPropertyBag *f) { return CONTAINING_RECORD(f, AxFormCtl, IPersistPropertyBag_iface); }
static inline AxFormCtl *impl_from_IDispatch(IDispatch *f)            { return CONTAINING_RECORD(f, AxFormCtl, IDispatch_iface); }
static inline AxFormCtl *impl_from_IObjectSafety(IObjectSafety *f)    { return CONTAINING_RECORD(f, AxFormCtl, IObjectSafety_iface); }
static inline AxClassFactory *impl_from_IClassFactory(IClassFactory *f) { return CONTAINING_RECORD(f, AxClassFactory, IClassFactory_iface); }

/* ---------- 引用计数 / QueryInterface ---------- */

static ULONG ctl_addref(AxFormCtl *This) { return InterlockedIncrement(&This->ref); }

static ULONG ctl_release(AxFormCtl *This)
{
    ULONG r = InterlockedDecrement(&This->ref);
    if (!r) {
        ax_log("object destroyed, all references released");
        if (This->hwnd) DestroyWindow(This->hwnd);
        if (This->client_site) IOleClientSite_Release(This->client_site);
        if (This->inplace_site) IOleInPlaceSite_Release(This->inplace_site);
        if (This->inplace_frame) IOleInPlaceFrame_Release(This->inplace_frame);
        if (This->inplace_doc) IOleInPlaceUIWindow_Release(This->inplace_doc);
        if (This->advise_sink) IAdviseSink_Release(This->advise_sink);
        free(This);
        InterlockedDecrement(&g_obj_count);
    }
    return r;
}

static HRESULT ctl_queryinterface(AxFormCtl *This, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDispatch))
        *ppv = &This->IDispatch_iface;
    else if (IsEqualIID(riid, &IID_IOleObject))
        *ppv = &This->IOleObject_iface;
    else if (IsEqualIID(riid, &IID_IOleInPlaceObject))
        *ppv = &This->IOleInPlaceObject_iface;
    else if (IsEqualIID(riid, &IID_IViewObject) || IsEqualIID(riid, &IID_IViewObject2))
        *ppv = &This->IViewObject2_iface;
    else if (IsEqualIID(riid, &IID_IPersist) || IsEqualIID(riid, &IID_IPersistPropertyBag))
        *ppv = &This->IPersistPropertyBag_iface;
    else if (IsEqualIID(riid, &IID_IObjectSafety))
        *ppv = &This->IObjectSafety_iface;

    if (*ppv) {
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }
    ax_log("QueryInterface(%s) -> E_NOINTERFACE", dbg_guid(riid));
    return E_NOINTERFACE;
}

/* ---------- 提交按钮：弹出消息框显示文本框内容 ---------- */

static void ctl_show_message(AxFormCtl *This)
{
    WCHAR text[256] = L"";
    if (This->hwnd_edit)
        GetWindowTextW(This->hwnd_edit, text, 256);
    ax_log("submit: text='%ls', showing MessageBox (parent=%p)", text, This->hwnd);
    MessageBoxW(This->hwnd, text[0] ? text : L"(文本框为空)",
                L"AxForm 提交的内容", MB_OK | MB_ICONINFORMATION);
}

/* ---------- 颜色解析与应用 ---------- */

/* "#RRGGBB" / "RRGGBB" -> COLORREF（0x00BBGGRR），失败返回 FALSE */
static BOOL parse_color(const WCHAR *s, COLORREF *out)
{
    DWORD v = 0;
    int n = 0;

    if (*s == L'#') s++;
    while (*s) {
        WCHAR c = *s;
        DWORD d;
        if (c >= L'0' && c <= L'9')      d = c - L'0';
        else if (c >= L'a' && c <= L'f') d = c - L'a' + 10;
        else if (c >= L'A' && c <= L'F') d = c - L'A' + 10;
        else break;
        v = v * 16 + d;
        n++;
        s++;
    }
    if (n != 6) return FALSE;
    *out = RGB((v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff);
    return TRUE;
}

static void ctl_set_bgcolor(AxFormCtl *This, COLORREF color)
{
    This->bgcolor = color;
    ax_log("bgcolor -> #%06lx", (unsigned long)color);
    if (This->hwnd) InvalidateRect(This->hwnd, NULL, TRUE);
}

/* ---------- 控件窗口：一个 label + 一个 EDIT + 一个 BUTTON ---------- */

static LRESULT CALLBACK ctl_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    AxFormCtl *This = (AxFormCtl *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_NCCREATE:
        This = (AxFormCtl *)((CREATESTRUCTW *)lp)->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)This);
        break;

    case WM_CREATE:
        /* 渲染流程关键点：控件在宿主窗口内创建子控件 */
        This->hwnd_label = CreateWindowExW(0, L"STATIC", This->label_text,
                WS_CHILD | WS_VISIBLE, 12, 14, 72, 22,
                hwnd, (HMENU)(DWORD_PTR)IDC_LABEL, g_hinst, NULL);
        This->hwnd_edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 88, 12, 196, 26,
                hwnd, (HMENU)(DWORD_PTR)IDC_EDIT, g_hinst, NULL);
        This->hwnd_button = CreateWindowExW(0, L"BUTTON", L"提 交",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 96, 50, 108, 30,
                hwnd, (HMENU)(DWORD_PTR)IDC_BUTTON, g_hinst, NULL);
        ax_log("WM_CREATE: hwnd=%p children label=%p edit=%p button=%p",
               hwnd, This->hwnd_label, This->hwnd_edit, This->hwnd_button);
        SetFocus(This->hwnd_edit);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HBRUSH br;
        HDC hdc = BeginPaint(hwnd, &ps);
        /* 背景色：默认 COLOR_BTNFACE，<param name="bgcolor"> 或 SetBgColor 可改 */
        br = CreateSolidBrush(This->bgcolor);
        FillRect(hdc, &ps.rcPaint, br);
        DeleteObject(br);
        EndPaint(hwnd, &ps);
        ax_log("WM_PAINT: hwnd=%p rect=(%ld,%ld)-(%ld,%ld) bg=#%06lx", hwnd,
               ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom,
               (unsigned long)This->bgcolor);
        return 0;
    }

    case WM_SIZE:
        ax_log("WM_SIZE: hwnd=%p %dx%d", hwnd, (int)LOWORD(lp), (int)HIWORD(lp));
        break;

    case WM_COMMAND:
        if (LOWORD(wp) == IDC_BUTTON && HIWORD(wp) == BN_CLICKED)
            ctl_show_message(This);
        break;

    case WM_DESTROY:
        ax_log("WM_DESTROY: hwnd=%p", hwnd);
        This->hwnd = This->hwnd_label = This->hwnd_edit = This->hwnd_button = NULL;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static BOOL ctl_register_class(void)
{
    static BOOL done;
    WNDCLASSW wc;

    if (done) return TRUE;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc   = ctl_wndproc;
    wc.hInstance     = g_hinst;
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = CTL_CLASS_W;
    done = RegisterClassW(&wc);
    return done;
}

/* ---------- 就地激活：DoVerb -> GetWindow -> 建窗口 -> SetObjectRects ---------- */

static HRESULT ctl_inplace_deactivate(AxFormCtl *This);

static HRESULT ctl_inplace_activate(AxFormCtl *This, IOleClientSite *active_site)
{
    RECT pos, clip;
    OLEINPLACEFRAMEINFO fi = { sizeof(fi) };
    HWND parent = NULL;
    HRESULT hr;

    if (This->activated) return S_OK;
    if (!active_site) active_site = This->client_site;
    if (!active_site) return E_POINTER;

    /* 1. 从容器取 IOleInPlaceSite（宿主侧实现在 MSHTML 内部） */
    hr = IOleClientSite_QueryInterface(active_site, &IID_IOleInPlaceSite,
                                       (void **)&This->inplace_site);
    if (FAILED(hr)) {
        ax_log("QI(IOleInPlaceSite) failed: 0x%08x", hr);
        return hr;
    }

    /* 2. 询问容器能否就地激活 */
    hr = IOleInPlaceSite_CanInPlaceActivate(This->inplace_site);
    ax_log("IOleInPlaceSite::CanInPlaceActivate -> 0x%08x", hr);
    if (hr != S_OK) return E_FAIL;

    /* 3. 通知容器即将就地激活 */
    IOleInPlaceSite_OnInPlaceActivate(This->inplace_site);

    /* 4. 拿到宿主窗口 HWND，作为控件窗口的父窗口 —— 渲染树挂载点 */
    hr = IOleInPlaceSite_GetWindow(This->inplace_site, &parent);
    if (FAILED(hr)) {
        ax_log("IOleInPlaceSite::GetWindow failed: 0x%08x", hr);
        return hr;
    }
    ax_log("IOleInPlaceSite::GetWindow -> parent(MSHTML host)=%p", parent);

    /* 5. 创建控件窗口（WS_CHILD 挂到 MSHTML 窗口下），WM_CREATE 中再建三个子控件 */
    if (!ctl_register_class()) return E_FAIL;
    This->hwnd = CreateWindowExW(0, CTL_CLASS_W, This->caption_text,
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            0, 0, CTL_W, CTL_H, parent, NULL, g_hinst, This);
    if (!This->hwnd) return E_FAIL;
    ax_log("CreateWindowExW(WS_CHILD) -> hwnd=%p", This->hwnd);

    /* 6. 取容器框架窗口与目标矩形，把窗口摆到 <object> 元素的位置 */
    IOleInPlaceSite_GetWindowContext(This->inplace_site, &This->inplace_frame,
                                     &This->inplace_doc, &pos, &clip, &fi);
    ax_log("IOleInPlaceSite::GetWindowContext pos=(%ld,%ld)-(%ld,%ld) clip=(%ld,%ld)-(%ld,%ld)",
           pos.left, pos.top, pos.right, pos.bottom,
           clip.left, clip.top, clip.right, clip.bottom);

    This->activated = TRUE;
    MoveWindow(This->hwnd, pos.left, pos.top,
               pos.right - pos.left, pos.bottom - pos.top, TRUE);
    IOleInPlaceSite_OnUIActivate(This->inplace_site);
    ax_log("in-place activation done, control window is now live");
    return S_OK;
}

static HRESULT ctl_inplace_deactivate(AxFormCtl *This)
{
    if (!This->activated) return S_OK;
    This->activated = FALSE;
    ax_log("in-place deactivate");
    IOleInPlaceSite_OnUIDeactivate(This->inplace_site, FALSE);
    if (This->hwnd) DestroyWindow(This->hwnd);
    IOleInPlaceSite_OnInPlaceDeactivate(This->inplace_site);
    if (This->inplace_frame) { IOleInPlaceFrame_Release(This->inplace_frame); This->inplace_frame = NULL; }
    if (This->inplace_doc)   { IOleInPlaceUIWindow_Release(This->inplace_doc); This->inplace_doc = NULL; }
    if (This->inplace_site)  { IOleInPlaceSite_Release(This->inplace_site); This->inplace_site = NULL; }
    return S_OK;
}

/* ---------- IOleObject ---------- */

static HRESULT STDMETHODCALLTYPE OleObject_QueryInterface(IOleObject *f, REFIID riid, void **ppv)
{
    return ctl_queryinterface(impl_from_IOleObject(f), riid, ppv);
}
static ULONG STDMETHODCALLTYPE OleObject_AddRef(IOleObject *f)
{
    return ctl_addref(impl_from_IOleObject(f));
}
static ULONG STDMETHODCALLTYPE OleObject_Release(IOleObject *f)
{
    return ctl_release(impl_from_IOleObject(f));
}

static HRESULT STDMETHODCALLTYPE OleObject_SetClientSite(IOleObject *f, IOleClientSite *site)
{
    AxFormCtl *This = impl_from_IOleObject(f);
    ax_log("IOleObject::SetClientSite site=%p", site);
    if (This->client_site) IOleClientSite_Release(This->client_site);
    This->client_site = site;
    if (site) IOleClientSite_AddRef(site);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE OleObject_GetClientSite(IOleObject *f, IOleClientSite **site)
{
    AxFormCtl *This = impl_from_IOleObject(f);
    *site = This->client_site;
    if (*site) IOleClientSite_AddRef(*site);
    return *site ? S_OK : E_FAIL;
}

static HRESULT STDMETHODCALLTYPE OleObject_SetHostNames(IOleObject *f, LPCOLESTR app, LPCOLESTR obj)
{
    ax_log("IOleObject::SetHostNames app='%ls' obj='%ls'", app, obj);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE OleObject_Close(IOleObject *f, DWORD saveopt)
{
    ax_log("IOleObject::Close saveopt=%lu", (unsigned long)saveopt);
    ctl_inplace_deactivate(impl_from_IOleObject(f));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE OleObject_SetMoniker(IOleObject *f, DWORD which, IMoniker *mk)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE OleObject_GetMoniker(IOleObject *f, DWORD assign, DWORD which, IMoniker **mk)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE OleObject_InitFromData(IOleObject *f, IDataObject *dobj, BOOL create, DWORD reserved)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE OleObject_GetClipboardData(IOleObject *f, DWORD reserved, IDataObject **dobj)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE OleObject_DoVerb(IOleObject *f, LONG verb, LPMSG msg,
        IOleClientSite *active_site, LONG index, HWND hwnd_parent, LPCRECT rect)
{
    AxFormCtl *This = impl_from_IOleObject(f);
    ax_log("IOleObject::DoVerb verb=%ld active_site=%p", verb, active_site);
    switch (verb) {
    case OLEIVERB_INPLACEACTIVATE:
    case OLEIVERB_UIACTIVATE:
    case OLEIVERB_SHOW:     /* MSHTML 用 SHOW 激活嵌入控件 */
    case OLEIVERB_OPEN:
        return ctl_inplace_activate(This, active_site);
    case OLEIVERB_HIDE:
        return ctl_inplace_deactivate(This);
    default:
        return E_NOTIMPL;
    }
}

static HRESULT STDMETHODCALLTYPE OleObject_EnumVerbs(IOleObject *f, IEnumOLEVERB **penum)
{
    return OleRegEnumVerbs(&CLSID_AxFormCtl, penum);
}

static HRESULT STDMETHODCALLTYPE OleObject_Update(IOleObject *f) { return S_OK; }
static HRESULT STDMETHODCALLTYPE OleObject_IsUpToDate(IOleObject *f) { return S_OK; }

static HRESULT STDMETHODCALLTYPE OleObject_GetUserClassID(IOleObject *f, CLSID *clsid)
{
    *clsid = CLSID_AxFormCtl;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE OleObject_GetUserType(IOleObject *f, DWORD form, LPOLESTR *type)
{
    return OleRegGetUserType(&CLSID_AxFormCtl, form, type);
}

static HRESULT STDMETHODCALLTYPE OleObject_SetExtent(IOleObject *f, DWORD aspect, SIZEL *sz)
{
    AxFormCtl *This = impl_from_IOleObject(f);
    ax_log("IOleObject::SetExtent aspect=0x%lx himetric=%ldx%ld",
           (unsigned long)aspect, sz->cx, sz->cy);
    if (aspect != DVASPECT_CONTENT) return DV_E_DVASPECT;
    This->extent = *sz;
    if (This->hwnd)
        MoveWindow(This->hwnd, 0, 0, sz->cx * 96 / 2540, sz->cy * 96 / 2540, TRUE);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE OleObject_GetExtent(IOleObject *f, DWORD aspect, SIZEL *sz)
{
    AxFormCtl *This = impl_from_IOleObject(f);
    if (aspect != DVASPECT_CONTENT) return DV_E_DVASPECT;
    *sz = This->extent;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE OleObject_Advise(IOleObject *f, IAdviseSink *sink, DWORD *conn)
{
    AxFormCtl *This = impl_from_IOleObject(f);
    if (This->advise_sink) IAdviseSink_Release(This->advise_sink);
    This->advise_sink = sink;
    IAdviseSink_AddRef(sink);
    *conn = 1;
    ax_log("IOleObject::Advise sink=%p", sink);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE OleObject_Unadvise(IOleObject *f, DWORD conn)
{
    AxFormCtl *This = impl_from_IOleObject(f);
    if (conn != 1 || !This->advise_sink) return OLE_E_NOCONNECTION;
    IAdviseSink_Release(This->advise_sink);
    This->advise_sink = NULL;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE OleObject_EnumAdvise(IOleObject *f, IEnumSTATDATA **penum)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE OleObject_GetMiscStatus(IOleObject *f, DWORD aspect, DWORD *status)
{
    ax_log("IOleObject::GetMiscStatus aspect=0x%lx", (unsigned long)aspect);
    /* RECOMPOSEONRESIZE|CANTLINKINSIDE|INSIDEOUT|ACTIVATEWHENVISIBLE|SETCLIENTSITEFIRST */
    *status = 0x20181;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE OleObject_SetColorScheme(IOleObject *f, LOGPALETTE *pal)
{
    return S_OK;
}

static IOleObjectVtbl OleObjectVtbl = {
    OleObject_QueryInterface,
    OleObject_AddRef,
    OleObject_Release,
    OleObject_SetClientSite,
    OleObject_GetClientSite,
    OleObject_SetHostNames,
    OleObject_Close,
    OleObject_SetMoniker,
    OleObject_GetMoniker,
    OleObject_InitFromData,
    OleObject_GetClipboardData,
    OleObject_DoVerb,
    OleObject_EnumVerbs,
    OleObject_Update,
    OleObject_IsUpToDate,
    OleObject_GetUserClassID,
    OleObject_GetUserType,
    OleObject_SetExtent,
    OleObject_GetExtent,
    OleObject_Advise,
    OleObject_Unadvise,
    OleObject_EnumAdvise,
    OleObject_GetMiscStatus,
    OleObject_SetColorScheme,
};

/* ---------- IOleInPlaceObject ---------- */

static HRESULT STDMETHODCALLTYPE InPlaceObject_QueryInterface(IOleInPlaceObject *f, REFIID riid, void **ppv)
{
    return ctl_queryinterface(impl_from_IOleInPlaceObject(f), riid, ppv);
}
static ULONG STDMETHODCALLTYPE InPlaceObject_AddRef(IOleInPlaceObject *f)
{
    return ctl_addref(impl_from_IOleInPlaceObject(f));
}
static ULONG STDMETHODCALLTYPE InPlaceObject_Release(IOleInPlaceObject *f)
{
    return ctl_release(impl_from_IOleInPlaceObject(f));
}

static HRESULT STDMETHODCALLTYPE InPlaceObject_GetWindow(IOleInPlaceObject *f, HWND *hwnd)
{
    AxFormCtl *This = impl_from_IOleInPlaceObject(f);
    ax_log("IOleInPlaceObject::GetWindow -> %p", This->hwnd);
    if (!This->hwnd) return E_UNEXPECTED;
    *hwnd = This->hwnd;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE InPlaceObject_ContextSensitiveHelp(IOleInPlaceObject *f, BOOL enter)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE InPlaceObject_InPlaceDeactivate(IOleInPlaceObject *f)
{
    ax_log("IOleInPlaceObject::InPlaceDeactivate");
    return ctl_inplace_deactivate(impl_from_IOleInPlaceObject(f));
}

static HRESULT STDMETHODCALLTYPE InPlaceObject_UIDeactivate(IOleInPlaceObject *f)
{
    ax_log("IOleInPlaceObject::UIDeactivate");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE InPlaceObject_SetObjectRects(IOleInPlaceObject *f, LPCRECT pos, LPCRECT clip)
{
    AxFormCtl *This = impl_from_IOleInPlaceObject(f);
    ax_log("IOleInPlaceObject::SetObjectRects pos=(%ld,%ld)-(%ld,%ld) clip=(%ld,%ld)-(%ld,%ld)",
           pos->left, pos->top, pos->right, pos->bottom,
           clip->left, clip->top, clip->right, clip->bottom);
    if (This->hwnd)
        MoveWindow(This->hwnd, pos->left, pos->top,
                   pos->right - pos->left, pos->bottom - pos->top, TRUE);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE InPlaceObject_ReactivateAndUndo(IOleInPlaceObject *f)
{
    return E_NOTIMPL;
}

static IOleInPlaceObjectVtbl OleInPlaceObjectVtbl = {
    InPlaceObject_QueryInterface,
    InPlaceObject_AddRef,
    InPlaceObject_Release,
    InPlaceObject_GetWindow,
    InPlaceObject_ContextSensitiveHelp,
    InPlaceObject_InPlaceDeactivate,
    InPlaceObject_UIDeactivate,
    InPlaceObject_SetObjectRects,
    InPlaceObject_ReactivateAndUndo,
};

/* ---------- IViewObject2（窗口化控件：绘制由自己的 WndProc 完成） ---------- */

static HRESULT STDMETHODCALLTYPE ViewObject2_QueryInterface(IViewObject2 *f, REFIID riid, void **ppv)
{
    return ctl_queryinterface(impl_from_IViewObject2(f), riid, ppv);
}
static ULONG STDMETHODCALLTYPE ViewObject2_AddRef(IViewObject2 *f)
{
    return ctl_addref(impl_from_IViewObject2(f));
}
static ULONG STDMETHODCALLTYPE ViewObject2_Release(IViewObject2 *f)
{
    return ctl_release(impl_from_IViewObject2(f));
}

static HRESULT STDMETHODCALLTYPE ViewObject2_Draw(IViewObject2 *f, DWORD aspect, LONG index,
        void *aspect_info, DVTARGETDEVICE *ptd, HDC hdc_target, HDC hdc_draw,
        LPCRECTL bounds, LPCRECTL wbounds,
        BOOL (STDMETHODCALLTYPE *cont)(ULONG_PTR), ULONG_PTR cont_arg)
{
    ax_log("IViewObject2::Draw aspect=0x%lx (windowed control: self-painted via WM_PAINT)",
           (unsigned long)aspect);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ViewObject2_GetColorSet(IViewObject2 *f, DWORD aspect, LONG index,
        void *aspect_info, DVTARGETDEVICE *ptd, HDC hdc, LOGPALETTE **pal)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE ViewObject2_Freeze(IViewObject2 *f, DWORD aspect, LONG index,
        void *aspect_info, DWORD *freeze)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE ViewObject2_Unfreeze(IViewObject2 *f, DWORD freeze)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE ViewObject2_SetAdvise(IViewObject2 *f, DWORD aspects, DWORD advf, IAdviseSink *sink)
{
    AxFormCtl *This = impl_from_IViewObject2(f);
    if (This->advise_sink) IAdviseSink_Release(This->advise_sink);
    This->advise_sink = sink;
    if (sink) IAdviseSink_AddRef(sink);
    This->advise_sink = sink;
    ax_log("IViewObject2::SetAdvise aspects=0x%lx advf=0x%lx sink=%p",
           (unsigned long)aspects, (unsigned long)advf, sink);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ViewObject2_GetAdvise(IViewObject2 *f, DWORD *aspects, DWORD *advf, IAdviseSink **sink)
{
    AxFormCtl *This = impl_from_IViewObject2(f);
    if (aspects) *aspects = DVASPECT_CONTENT;
    if (advf) *advf = 0;
    if (sink) {
        *sink = This->advise_sink;
        if (*sink) IAdviseSink_AddRef(*sink);
    }
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ViewObject2_GetExtent(IViewObject2 *f, DWORD aspect,
        LONG index, DVTARGETDEVICE *ptd, SIZE *sz)
{
    AxFormCtl *This = impl_from_IViewObject2(f);
    if (aspect != DVASPECT_CONTENT) return DV_E_DVASPECT;
    sz->cx = This->extent.cx;
    sz->cy = This->extent.cy;
    return S_OK;
}

static IViewObject2Vtbl ViewObject2Vtbl = {
    ViewObject2_QueryInterface,
    ViewObject2_AddRef,
    ViewObject2_Release,
    ViewObject2_Draw,
    ViewObject2_GetColorSet,
    ViewObject2_Freeze,
    ViewObject2_Unfreeze,
    ViewObject2_SetAdvise,
    ViewObject2_GetAdvise,
    ViewObject2_GetExtent,
};

/* ---------- IPersistPropertyBag：读取 <object> 的 <param> 参数 ---------- */

static void read_param(IPropertyBag *bag, const WCHAR *name, WCHAR *out, DWORD len)
{
    VARIANT v;
    HRESULT hr;

    VariantInit(&v);
    hr = IPropertyBag_Read(bag, name, &v, NULL);
    if (SUCCEEDED(hr) && V_VT(&v) == VT_BSTR && V_BSTR(&v)) {
        lstrcpynW(out, V_BSTR(&v), len);
        ax_log("  param '%ls' = '%ls'", name, out);
    }
    VariantClear(&v);
}

static HRESULT STDMETHODCALLTYPE PersistPropertyBag_QueryInterface(IPersistPropertyBag *f, REFIID riid, void **ppv)
{
    return ctl_queryinterface(impl_from_IPersistPropertyBag(f), riid, ppv);
}
static ULONG STDMETHODCALLTYPE PersistPropertyBag_AddRef(IPersistPropertyBag *f)
{
    return ctl_addref(impl_from_IPersistPropertyBag(f));
}
static ULONG STDMETHODCALLTYPE PersistPropertyBag_Release(IPersistPropertyBag *f)
{
    return ctl_release(impl_from_IPersistPropertyBag(f));
}

static HRESULT STDMETHODCALLTYPE PersistPropertyBag_GetClassID(IPersistPropertyBag *f, CLSID *clsid)
{
    *clsid = CLSID_AxFormCtl;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE PersistPropertyBag_InitNew(IPersistPropertyBag *f)
{
    ax_log("IPersistPropertyBag::InitNew");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE PersistPropertyBag_Load(IPersistPropertyBag *f,
        IPropertyBag *bag, IErrorLog *errlog)
{
    AxFormCtl *This = impl_from_IPersistPropertyBag(f);
    WCHAR color_str[16] = L"";
    ax_log("IPersistPropertyBag::Load (reading <param> from <object>)");
    read_param(bag, L"label",   This->label_text,   64);
    read_param(bag, L"caption", This->caption_text, 64);
    read_param(bag, L"bgcolor", color_str, 16);
    if (color_str[0] && parse_color(color_str, &This->bgcolor))
        ax_log("  bgcolor parsed = #%06lx", (unsigned long)This->bgcolor);
    if (This->hwnd_label)
        SetWindowTextW(This->hwnd_label, This->label_text);
    if (This->hwnd)
        InvalidateRect(This->hwnd, NULL, TRUE);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE PersistPropertyBag_Save(IPersistPropertyBag *f,
        IPropertyBag *bag, BOOL clear, BOOL saveall)
{
    return S_OK;
}

static IPersistPropertyBagVtbl PersistPropertyBagVtbl = {
    PersistPropertyBag_QueryInterface,
    PersistPropertyBag_AddRef,
    PersistPropertyBag_Release,
    PersistPropertyBag_GetClassID,
    PersistPropertyBag_InitNew,
    PersistPropertyBag_Load,
    PersistPropertyBag_Save,
};

/* ---------- IDispatch：供 JavaScript 调用 GetText / ShowText ---------- */

static const struct { const WCHAR *name; DISPID id; } disp_map[] = {
    { L"GetText",    DISPID_GETTEXT },
    { L"ShowText",   DISPID_SHOWTEXT },
    { L"SetBgColor", DISPID_SETBGCOLOR },
};

static HRESULT STDMETHODCALLTYPE Disp_QueryInterface(IDispatch *f, REFIID riid, void **ppv)
{
    return ctl_queryinterface(impl_from_IDispatch(f), riid, ppv);
}
static ULONG STDMETHODCALLTYPE Disp_AddRef(IDispatch *f)
{
    return ctl_addref(impl_from_IDispatch(f));
}
static ULONG STDMETHODCALLTYPE Disp_Release(IDispatch *f)
{
    return ctl_release(impl_from_IDispatch(f));
}

static HRESULT STDMETHODCALLTYPE Disp_GetTypeInfoCount(IDispatch *f, UINT *count)
{
    *count = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Disp_GetTypeInfo(IDispatch *f, UINT itinfo, LCID lcid, ITypeInfo **info)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Disp_GetIDsOfNames(IDispatch *f, REFIID riid,
        LPOLESTR *names, UINT cnames, LCID lcid, DISPID *dispids)
{
    UINT i, j;
    HRESULT hr = S_OK;

    for (i = 0; i < cnames; i++) {
        dispids[i] = DISPID_UNKNOWN;
        for (j = 0; j < sizeof(disp_map)/sizeof(disp_map[0]); j++) {
            if (lstrcmpiW(names[i], disp_map[j].name) == 0) {
                dispids[i] = disp_map[j].id;
                break;
            }
        }
        if (dispids[i] == DISPID_UNKNOWN) hr = DISP_E_UNKNOWNNAME;
        else ax_log("IDispatch::GetIDsOfNames '%ls' -> dispid %ld", names[i], (long)dispids[i]);
    }
    return hr;
}

static HRESULT STDMETHODCALLTYPE Disp_Invoke(IDispatch *f, DISPID dispid, REFIID riid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excep, UINT *argerr)
{
    AxFormCtl *This = impl_from_IDispatch(f);
    ax_log("IDispatch::Invoke dispid=%ld flags=0x%04x", (long)dispid, flags);

    switch (dispid) {
    case DISPID_GETTEXT: {
        WCHAR text[256];
        if (!(flags & (DISPATCH_METHOD | DISPATCH_PROPERTYGET)))
            return DISP_E_MEMBERNOTFOUND;
        if (This->hwnd_edit)
            GetWindowTextW(This->hwnd_edit, text, 256);
        else
            text[0] = 0;
        if (result) {
            VariantClear(result);
            V_VT(result) = VT_BSTR;
            V_BSTR(result) = SysAllocString(text);
        }
        return S_OK;
    }
    case DISPID_SHOWTEXT:
        if (!(flags & (DISPATCH_METHOD | DISPATCH_PROPERTYGET)))
            return DISP_E_MEMBERNOTFOUND;
        ctl_show_message(This);
        return S_OK;
    case DISPID_SETBGCOLOR: {
        VARIANT *arg, tmp;
        if (!(flags & (DISPATCH_METHOD | DISPATCH_PROPERTYPUT)))
            return DISP_E_MEMBERNOTFOUND;
        if (!params || params->cArgs < 1)
            return DISP_E_BADPARAMCOUNT;
        arg = &params->rgvarg[params->cArgs - 1];   /* rgvarg 倒序，第一个实参在尾部 */
        VariantInit(&tmp);
        if (SUCCEEDED(VariantChangeType(&tmp, arg, 0, VT_I4)))
            ctl_set_bgcolor(This, (COLORREF)V_I4(&tmp));    /* COLORREF 0x00BBGGRR */
        else
            ax_log("SetBgColor: bad arg vt=%d", (int)V_VT(arg));
        VariantClear(&tmp);
        return S_OK;
    }
    }
    return DISP_E_MEMBERNOTFOUND;
}

static IDispatchVtbl DispatchVtbl = {
    Disp_QueryInterface,
    Disp_AddRef,
    Disp_Release,
    Disp_GetTypeInfoCount,
    Disp_GetTypeInfo,
    Disp_GetIDsOfNames,
    Disp_Invoke,
};

/* ---------- IObjectSafety：向 MSHTML 声明脚本安全 ---------- */

static HRESULT STDMETHODCALLTYPE Safety_QueryInterface(IObjectSafety *f, REFIID riid, void **ppv)
{
    return ctl_queryinterface(impl_from_IObjectSafety(f), riid, ppv);
}
static ULONG STDMETHODCALLTYPE Safety_AddRef(IObjectSafety *f)
{
    return ctl_addref(impl_from_IObjectSafety(f));
}
static ULONG STDMETHODCALLTYPE Safety_Release(IObjectSafety *f)
{
    return ctl_release(impl_from_IObjectSafety(f));
}

static HRESULT STDMETHODCALLTYPE Safety_GetInterfaceSafetyOptions(IObjectSafety *f,
        REFIID riid, DWORD *supported, DWORD *enabled)
{
    ax_log("IObjectSafety::GetInterfaceSafetyOptions %s", dbg_guid(riid));
    *supported = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;
    *enabled   = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Safety_SetInterfaceSafetyOptions(IObjectSafety *f,
        REFIID riid, DWORD mask, DWORD options)
{
    ax_log("IObjectSafety::SetInterfaceSafetyOptions %s mask=0x%08lx options=0x%08lx",
           dbg_guid(riid), (unsigned long)mask, (unsigned long)options);
    return S_OK;
}

static IObjectSafetyVtbl ObjectSafetyVtbl = {
    Safety_QueryInterface,
    Safety_AddRef,
    Safety_Release,
    Safety_GetInterfaceSafetyOptions,
    Safety_SetInterfaceSafetyOptions,
};

/* ---------- IClassFactory ---------- */

static HRESULT STDMETHODCALLTYPE ClassFactory_QueryInterface(IClassFactory *f, REFIID riid, void **ppv)
{
    AxClassFactory *This = impl_from_IClassFactory(f);
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory)) {
        *ppv = &This->IClassFactory_iface;
        IClassFactory_AddRef(f);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE ClassFactory_AddRef(IClassFactory *f)
{
    return InterlockedIncrement(&impl_from_IClassFactory(f)->ref);
}

static ULONG STDMETHODCALLTYPE ClassFactory_Release(IClassFactory *f)
{
    AxClassFactory *This = impl_from_IClassFactory(f);
    ULONG r = InterlockedDecrement(&This->ref);
    if (!r) free(This);
    return r;
}

static HRESULT STDMETHODCALLTYPE ClassFactory_CreateInstance(IClassFactory *f,
        IUnknown *outer, REFIID riid, void **ppv)
{
    AxFormCtl *This;

    ax_log("IClassFactory::CreateInstance riid=%s", dbg_guid(riid));
    if (outer) return CLASS_E_NOAGGREGATION;

    This = calloc(1, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IOleObject_iface.lpVtbl          = &OleObjectVtbl;
    This->IOleInPlaceObject_iface.lpVtbl   = &OleInPlaceObjectVtbl;
    This->IViewObject2_iface.lpVtbl        = &ViewObject2Vtbl;
    This->IPersistPropertyBag_iface.lpVtbl = &PersistPropertyBagVtbl;
    This->IDispatch_iface.lpVtbl           = &DispatchVtbl;
    This->IObjectSafety_iface.lpVtbl       = &ObjectSafetyVtbl;
    This->ref = 1;
    InterlockedIncrement(&g_obj_count);

    lstrcpynW(This->label_text,   L"姓名：",            64);
    lstrcpynW(This->caption_text, L"AxForm 表单控件",   64);
    This->bgcolor = GetSysColor(COLOR_BTNFACE);
    This->extent.cx = PX2HM(CTL_W);
    This->extent.cy = PX2HM(CTL_H);

    return ctl_queryinterface(This, riid, ppv);
}

static HRESULT STDMETHODCALLTYPE ClassFactory_LockServer(IClassFactory *f, BOOL lock)
{
    if (lock) InterlockedIncrement(&g_locks);
    else InterlockedDecrement(&g_locks);
    return S_OK;
}

static IClassFactoryVtbl ClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer,
};

/* ---------- COM DLL 导出函数 ---------- */

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    AxClassFactory *factory;

    ax_log("DllGetClassObject clsid=%s riid=%s", dbg_guid(rclsid), dbg_guid(riid));
    if (!IsEqualCLSID(rclsid, &CLSID_AxFormCtl))
        return CLASS_E_CLASSNOTAVAILABLE;

    factory = calloc(1, sizeof(*factory));
    if (!factory) return E_OUTOFMEMORY;
    factory->IClassFactory_iface.lpVtbl = &ClassFactoryVtbl;
    factory->ref = 1;

    *ppv = &factory->IClassFactory_iface;
    return S_OK;
}

STDAPI DllCanUnloadNow(void)
{
    return (g_obj_count == 0 && g_locks == 0) ? S_OK : S_FALSE;
}

/* ---------- 注册表注册（regsvr32 调用） ---------- */

static LSTATUS reg_set_str(HKEY root, const WCHAR *subkey, const WCHAR *name, const WCHAR *value)
{
    HKEY hkey;
    LSTATUS r;

    r = RegCreateKeyExW(root, subkey, 0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_WRITE, NULL, &hkey, NULL);
    if (r) return r;
    r = RegSetValueExW(hkey, name, 0, REG_SZ, (const BYTE *)value,
                       (lstrlenW(value) + 1) * sizeof(WCHAR));
    RegCloseKey(hkey);
    return r;
}

static LSTATUS reg_del_tree(HKEY root, const WCHAR *subkey)
{
    HKEY hkey;
    WCHAR name[256];
    DWORD len;
    LSTATUS r;

    r = RegOpenKeyExW(root, subkey, 0, KEY_READ | KEY_WRITE, &hkey);
    if (r) return r;
    for (;;) {
        len = 256;
        r = RegEnumKeyExW(hkey, 0, name, &len, NULL, NULL, NULL, NULL);
        if (r != ERROR_SUCCESS) break;
        {
            WCHAR child[512];
            wsprintfW(child, L"%s\\%s", subkey, name);
            reg_del_tree(root, child);
        }
    }
    RegCloseKey(hkey);
    return RegDeleteKeyW(root, subkey);
}

static WCHAR *reg_clsid_path(const WCHAR *suffix, WCHAR *buf, DWORD len)
{
    WCHAR clsid[64];
    StringFromGUID2(&CLSID_AxFormCtl, clsid, 64);
    wsprintfW(buf, L"CLSID\\%s%s", clsid, suffix ? suffix : L"");
    return buf;
}

STDAPI DllRegisterServer(void)
{
    WCHAR path[MAX_PATH], subkey[512], clsid[64];
    LSTATUS r = ERROR_SUCCESS;

    GetModuleFileNameW(g_hinst, path, MAX_PATH);
    StringFromGUID2(&CLSID_AxFormCtl, clsid, 64);
    ax_log("DllRegisterServer: dll=%ls clsid=%ls", path, clsid);

    /* HKCR\CLSID\{...} */
    reg_set_str(HKEY_CLASSES_ROOT, reg_clsid_path(NULL, subkey, 512), NULL,
                L"AxForm Form Control (AxFormCtl Class)");
    /* InprocServer32: DLL 路径 + 线程模型 */
    reg_set_str(HKEY_CLASSES_ROOT, reg_clsid_path(L"\\InprocServer32", subkey, 512), NULL, path);
    reg_set_str(HKEY_CLASSES_ROOT, reg_clsid_path(L"\\InprocServer32", subkey, 512),
                L"ThreadingModel", L"Apartment");
    /* Control 标记：声明这是一个 ActiveX 控件 */
    reg_set_str(HKEY_CLASSES_ROOT, reg_clsid_path(L"\\Control", subkey, 512), NULL, L"");
    /* MiscStatus: SETCLIENTSITEFIRST 等，容器加载顺序依据 */
    reg_set_str(HKEY_CLASSES_ROOT, reg_clsid_path(L"\\MiscStatus", subkey, 512), NULL, L"131457");
    /* ProgID 双向链接 */
    reg_set_str(HKEY_CLASSES_ROOT, reg_clsid_path(L"\\ProgID", subkey, 512), NULL, PROGID);
    reg_set_str(HKEY_CLASSES_ROOT, reg_clsid_path(L"\\VersionIndependentProgID", subkey, 512),
                NULL, PROGID_VERIND);
    reg_set_str(HKEY_CLASSES_ROOT, PROGID, NULL, L"AxForm Form Control (AxFormCtl Class)");
    wsprintfW(subkey, L"%s\\CLSID", PROGID);
    reg_set_str(HKEY_CLASSES_ROOT, subkey, NULL, clsid);
    reg_set_str(HKEY_CLASSES_ROOT, PROGID_VERIND, NULL, L"AxForm Form Control (AxFormCtl Class)");
    wsprintfW(subkey, L"%s\\CLSID", PROGID_VERIND);
    reg_set_str(HKEY_CLASSES_ROOT, subkey, NULL, clsid);

    if (r) {
        ax_log("DllRegisterServer failed: %ld", (long)r);
        return SELFREG_E_CLASS;
    }
    ax_log("DllRegisterServer: S_OK");
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    WCHAR subkey[512];

    reg_del_tree(HKEY_CLASSES_ROOT, reg_clsid_path(NULL, subkey, 512));
    reg_del_tree(HKEY_CLASSES_ROOT, PROGID);
    reg_del_tree(HKEY_CLASSES_ROOT, PROGID_VERIND);
    ax_log("DllUnregisterServer: S_OK");
    return S_OK;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        g_hinst = hinst;
        DisableThreadLibraryCalls(hinst);
        ax_log("DllMain(DLL_PROCESS_ATTACH) hinst=%p (DLL loaded into MSHTML process)", hinst);
        break;
    case DLL_PROCESS_DETACH:
        ax_log("DllMain(DLL_PROCESS_DETACH)");
        break;
    }
    return TRUE;
}
