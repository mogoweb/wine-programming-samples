/*
 * axhost.c - 原生 Win32 宿主窗口内嵌 AxFormCtl ActiveX 控件
 *
 * 目的：验证窗口化 ActiveX 控件在原生容器中是否随宿主窗口 resize 跟随变化，
 *       与 MSHTML（iexplore/testpage.html）容器行为对照。
 *
 * 宿主侧接口（与 axformctl.c 控件侧互为镜像，风格对齐 ie/call_external/qahost.c）：
 *   IOleClientSite      基本站点
 *   IOleInPlaceSite     就地激活支持：GetWindow / GetWindowContext / OnPosRectChange
 *   IOleInPlaceFrame    最小框架桩
 *
 * resize 验证路径：
 *   菜单「模式 A: SetObjectRects」—— WM_SIZE 中 QI(IOleInPlaceObject) 后
 *     SetObjectRects(新客户区)，标准容器同步路径；
 *   菜单「模式 B: SetExtent」—— WM_SIZE 中 IOleObject::SetExtent(HIMETRIC)，
 *     依赖控件的 SetExtent->MoveWindow 实现（Wine 几何调试常用对照路径）；
 *   标题栏实时显示 [模式 | 客户区 WxH]，配合 [AXFORM]/[AXHOST] 日志确认跟随。
 *
 * Build: make axhost.exe  (i686-w64-mingw32-gcc ...)
 */

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#define INITGUID

#include <windows.h>
#include <ole2.h>
#include <oleidl.h>
#include <ocidl.h>
#include <oaidl.h>
#include <cguid.h>      /* GUID_NULL */
#include <stdio.h>

/* 与 axformctl.c 中 DEFINE_GUID(CLSID_AxFormCtl) 一致 */
DEFINE_GUID(CLSID_AxFormCtl, 0x5e8f4a2c,0x1d3b,0x4c6e,0x9f,0x70,0xa1,0xb2,0xc3,0xd4,0xe5,0xf6);
/* {5E8F4A2C-1D3B-4C6E-9F70-A1B2C3D4E5F6} */

#define APP_CLASS_W  L"AxHostWndClass"
#define APP_TITLE_W  L"AxHost - ActiveX Resize Test"
#define CTL_MIN_W    120
#define CTL_MIN_H    60

#define IDM_MODE_RECTS  2001
#define IDM_MODE_EXTENT 2002

static HINSTANCE g_hinst;

/* 容器几何同步模式：
 * A: WM_SIZE -> IOleInPlaceObject::SetObjectRects (标准路径)
 * B: WM_SIZE -> IOleObject::SetExtent(HIMETRIC)   (对照路径) */
static int g_mode = 'A';

static void ax_log(const char *fmt, ...)
{
    va_list ap;
    fprintf(stderr, "[AXHOST] ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    fflush(stderr);
}

/* ===== HostSite: IOleClientSite + IOleInPlaceSite + IOleInPlaceFrame ===== */
typedef struct {
    IOleClientSite      IOleClientSite_iface;
    IOleInPlaceSite     IOleInPlaceSite_iface;
    IOleInPlaceFrame    IOleInPlaceFrame_iface;
    LONG ref;
    HWND hwnd;
} HostSite;

static inline HostSite *site_from_cs(IOleClientSite *f)  { return CONTAINING_RECORD(f, HostSite, IOleClientSite_iface); }
static inline HostSite *site_from_ips(IOleInPlaceSite *f){ return CONTAINING_RECORD(f, HostSite, IOleInPlaceSite_iface); }
static inline HostSite *site_from_ipf(IOleInPlaceFrame *f){ return CONTAINING_RECORD(f, HostSite, IOleInPlaceFrame_iface); }

static HRESULT site_qi(HostSite *t, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IOleClientSite))
        *ppv = &t->IOleClientSite_iface;
    else if (IsEqualIID(riid, &IID_IOleInPlaceSite) || IsEqualIID(riid, &IID_IOleInPlaceSiteEx))
        *ppv = &t->IOleInPlaceSite_iface;
    else if (IsEqualIID(riid, &IID_IOleInPlaceFrame) || IsEqualIID(riid, &IID_IOleInPlaceUIWindow))
        *ppv = &t->IOleInPlaceFrame_iface;
    else { *ppv = NULL; return E_NOINTERFACE; }
    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

/* ---------- IOleClientSite ---------- */
static HRESULT STDMETHODCALLTYPE CS_QI(IOleClientSite *f, REFIID riid, void **ppv) { return site_qi(site_from_cs(f), riid, ppv); }
static ULONG STDMETHODCALLTYPE CS_AddRef(IOleClientSite *f) { return InterlockedIncrement(&site_from_cs(f)->ref); }
static ULONG STDMETHODCALLTYPE CS_Release(IOleClientSite *f)
{
    HostSite *t = site_from_cs(f);
    ULONG r = InterlockedDecrement(&t->ref);
    if (!r) free(t);
    return r;
}
static HRESULT STDMETHODCALLTYPE CS_SaveObject(IOleClientSite *f) { return S_OK; }
static HRESULT STDMETHODCALLTYPE CS_GetMoniker(IOleClientSite *f, DWORD a, DWORD b, IMoniker **mk) { *mk = NULL; return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE CS_GetContainer(IOleClientSite *f, IOleContainer **c) { *c = NULL; return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE CS_ShowObject(IOleClientSite *f) { return S_OK; }
static HRESULT STDMETHODCALLTYPE CS_OnShowWindow(IOleClientSite *f, BOOL show) { return S_OK; }
static HRESULT STDMETHODCALLTYPE CS_RequestNewObjectLayout(IOleClientSite *f) { return E_NOTIMPL; }

static IOleClientSiteVtbl CSVtbl = {
    CS_QI, CS_AddRef, CS_Release, CS_SaveObject, CS_GetMoniker,
    CS_GetContainer, CS_ShowObject, CS_OnShowWindow, CS_RequestNewObjectLayout,
};

/* ---------- IOleInPlaceSite ---------- */
static HRESULT STDMETHODCALLTYPE IPS_QI(IOleInPlaceSite *f, REFIID riid, void **ppv) { return site_qi(site_from_ips(f), riid, ppv); }
static ULONG STDMETHODCALLTYPE IPS_AddRef(IOleInPlaceSite *f) { return IOleClientSite_AddRef(&site_from_ips(f)->IOleClientSite_iface); }
static ULONG STDMETHODCALLTYPE IPS_Release(IOleInPlaceSite *f) { return IOleClientSite_Release(&site_from_ips(f)->IOleClientSite_iface); }
static HRESULT STDMETHODCALLTYPE IPS_GetWindow(IOleInPlaceSite *f, HWND *phwnd)
{
    *phwnd = site_from_ips(f)->hwnd;
    ax_log("IOleInPlaceSite::GetWindow -> host hwnd=%p", *phwnd);
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE IPS_ContextSensitiveHelp(IOleInPlaceSite *f, BOOL enter) { return S_OK; }
static HRESULT STDMETHODCALLTYPE IPS_CanInPlaceActivate(IOleInPlaceSite *f)
{
    ax_log("IOleInPlaceSite::CanInPlaceActivate -> S_OK");
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE IPS_OnInPlaceActivate(IOleInPlaceSite *f) { return S_OK; }
static HRESULT STDMETHODCALLTYPE IPS_OnUIActivate(IOleInPlaceSite *f) { return S_OK; }
static HRESULT STDMETHODCALLTYPE IPS_GetWindowContext(IOleInPlaceSite *f,
        IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc,
        LPRECT prcPos, LPRECT prcClip, LPOLEINPLACEFRAMEINFO fi)
{
    HostSite *t = site_from_ips(f);
    *ppFrame = &t->IOleInPlaceFrame_iface;
    IOleInPlaceFrame_AddRef(*ppFrame);
    *ppDoc = NULL;
    GetClientRect(t->hwnd, prcPos);
    *prcClip = *prcPos;
    if (fi) {
        fi->cb = sizeof(*fi);
        fi->fMDIApp = FALSE;
        fi->hwndFrame = t->hwnd;
        fi->haccel = NULL;
        fi->cAccelEntries = 0;
    }
    ax_log("IOleInPlaceSite::GetWindowContext pos=(%ld,%ld)-(%ld,%ld)",
           prcPos->left, prcPos->top, prcPos->right, prcPos->bottom);
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE IPS_Scroll(IOleInPlaceSite *f, SIZE scroll) { return S_OK; }
static HRESULT STDMETHODCALLTYPE IPS_OnUIDeactivate(IOleInPlaceSite *f, BOOL undo) { return S_OK; }
static HRESULT STDMETHODCALLTYPE IPS_OnInPlaceDeactivate(IOleInPlaceSite *f) { return S_OK; }
static HRESULT STDMETHODCALLTYPE IPS_DiscardUndoState(IOleInPlaceSite *f) { return S_OK; }
static HRESULT STDMETHODCALLTYPE IPS_DeactivateAndUndo(IOleInPlaceSite *f) { return S_OK; }
static HRESULT STDMETHODCALLTYPE IPS_OnPosRectChange(IOleInPlaceSite *f, LPCRECT rc)
{
    ax_log("IOleInPlaceSite::OnPosRectChange (%ld,%ld)-(%ld,%ld) (ignored)",
           rc->left, rc->top, rc->right, rc->bottom);
    return S_OK;
}

static IOleInPlaceSiteVtbl IPSVtbl = {
    IPS_QI, IPS_AddRef, IPS_Release, IPS_GetWindow, IPS_ContextSensitiveHelp,
    IPS_CanInPlaceActivate, IPS_OnInPlaceActivate, IPS_OnUIActivate,
    IPS_GetWindowContext, IPS_Scroll, IPS_OnUIDeactivate, IPS_OnInPlaceDeactivate,
    IPS_DiscardUndoState, IPS_DeactivateAndUndo, IPS_OnPosRectChange,
};

/* ---------- IOleInPlaceFrame（最小桩） ---------- */
static HRESULT STDMETHODCALLTYPE IPF_QI(IOleInPlaceFrame *f, REFIID riid, void **ppv) { return site_qi(site_from_ipf(f), riid, ppv); }
static ULONG STDMETHODCALLTYPE IPF_AddRef(IOleInPlaceFrame *f) { return IOleClientSite_AddRef(&site_from_ipf(f)->IOleClientSite_iface); }
static ULONG STDMETHODCALLTYPE IPF_Release(IOleInPlaceFrame *f) { return IOleClientSite_Release(&site_from_ipf(f)->IOleClientSite_iface); }
static HRESULT STDMETHODCALLTYPE IPF_GetWindow(IOleInPlaceFrame *f, HWND *phwnd) { *phwnd = site_from_ipf(f)->hwnd; return S_OK; }
static HRESULT STDMETHODCALLTYPE IPF_ContextSensitiveHelp(IOleInPlaceFrame *f, BOOL enter) { return S_OK; }
static HRESULT STDMETHODCALLTYPE IPF_GetBorder(IOleInPlaceFrame *f, LPRECT rc) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE IPF_RequestBorderSpace(IOleInPlaceFrame *f, LPCRECT rc) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE IPF_SetBorderSpace(IOleInPlaceFrame *f, LPCRECT rc) { return S_OK; }
static HRESULT STDMETHODCALLTYPE IPF_SetActiveObject(IOleInPlaceFrame *f, IOleInPlaceActiveObject *ao, LPCOLESTR name) { return S_OK; }
static HRESULT STDMETHODCALLTYPE IPF_InsertMenus(IOleInPlaceFrame *f, HMENU m, OLEMENUGROUPWIDTHS *w) { return S_OK; }
static HRESULT STDMETHODCALLTYPE IPF_SetMenu(IOleInPlaceFrame *f, HMENU shared, HOLEMENU inplace, HWND active) { return S_OK; }
static HRESULT STDMETHODCALLTYPE IPF_RemoveMenus(IOleInPlaceFrame *f, HMENU m) { return S_OK; }
static HRESULT STDMETHODCALLTYPE IPF_SetStatusText(IOleInPlaceFrame *f, LPCOLESTR text) { return S_OK; }
static HRESULT STDMETHODCALLTYPE IPF_EnableModeless(IOleInPlaceFrame *f, BOOL enable) { return S_OK; }
static HRESULT STDMETHODCALLTYPE IPF_TranslateAccelerator(IOleInPlaceFrame *f, LPMSG msg, WORD id) { return S_FALSE; }

static IOleInPlaceFrameVtbl IPFVtbl = {
    IPF_QI, IPF_AddRef, IPF_Release, IPF_GetWindow, IPF_ContextSensitiveHelp,
    IPF_GetBorder, IPF_RequestBorderSpace, IPF_SetBorderSpace, IPF_SetActiveObject,
    IPF_InsertMenus, IPF_SetMenu, IPF_RemoveMenus, IPF_SetStatusText,
    IPF_EnableModeless, IPF_TranslateAccelerator,
};

static HostSite *create_site(HWND hwnd)
{
    HostSite *s = calloc(1, sizeof(*s));
    s->IOleClientSite_iface.lpVtbl   = &CSVtbl;
    s->IOleInPlaceSite_iface.lpVtbl  = &IPSVtbl;
    s->IOleInPlaceFrame_iface.lpVtbl = &IPFVtbl;
    s->ref = 1;
    s->hwnd = hwnd;
    return s;
}

/* ===== 主窗口 ===== */

/* 经 IDispatch 调用控件的 SetBgColor(COLORREF)（dispid=3），验证脚本属性路径 */
static void host_set_bgcolor(IDispatch *disp, COLORREF color)
{
    DISPPARAMS dp = { NULL, NULL, 0, 0 };
    VARIANT arg;
    DISPID did = DISPID_UNKNOWN;
    OLECHAR *name = (OLECHAR *)L"SetBgColor";

    if (!disp) return;
    if (FAILED(IDispatch_GetIDsOfNames(disp, &IID_NULL, &name, 1, 0, &did)) ||
        did == DISPID_UNKNOWN) {
        ax_log("SetBgColor: GetIDsOfNames failed");
        return;
    }
    VariantInit(&arg);
    V_VT(&arg) = VT_I4;
    V_I4(&arg) = (LONG)color;   /* COLORREF 0x00BBGGRR */
    dp.rgvarg = &arg;
    dp.cArgs = 1;
    if (SUCCEEDED(IDispatch_Invoke(disp, did, &IID_NULL, 0, DISPATCH_METHOD, &dp, NULL, NULL, NULL)))
        ax_log("SetBgColor(#%06lx) via IDispatch OK", (unsigned long)color);
    else
        ax_log("SetBgColor via IDispatch FAILED");
}

static void update_title(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    SetWindowTextW(hwnd, (g_mode == 'A')
            ? (LPCWSTR)L"AxHost [模式A: SetObjectRects]"
            : (LPCWSTR)L"AxHost [模式B: SetExtent]");
}

/* WM_SIZE 中按当前模式同步控件几何 */
static void sync_control(HWND hwnd, IOleObject *oo)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    if (rc.right - rc.left < CTL_MIN_W || rc.bottom - rc.top < CTL_MIN_H) {
        ax_log("WM_SIZE: client %ldx%ld too small, skip sync",
               rc.right - rc.left, rc.bottom - rc.top);
        return;
    }

    if (g_mode == 'A') {
        IOleInPlaceObject *ipo = NULL;
        if (SUCCEEDED(IOleObject_QueryInterface(oo, &IID_IOleInPlaceObject, (void **)&ipo)) && ipo) {
            ax_log("WM_SIZE %ldx%ld -> SetObjectRects", rc.right - rc.left, rc.bottom - rc.top);
            IOleInPlaceObject_SetObjectRects(ipo, &rc, &rc);
            IOleInPlaceObject_Release(ipo);
        }
    } else {
        SIZEL sz;
        sz.cx = (LONG)((rc.right - rc.left) * 2540 / 96);
        sz.cy = (LONG)((rc.bottom - rc.top) * 2540 / 96);
        ax_log("WM_SIZE %ldx%ld -> SetExtent himetric=%ldx%ld",
               rc.right - rc.left, rc.bottom - rc.top, sz.cx, sz.cy);
        IOleObject_SetExtent(oo, DVASPECT_CONTENT, &sz);
    }

    /* 标题栏显示当前客户区尺寸，肉眼确认控件窗口是否一致 */
    {
        WCHAR title[128];
        wsprintfW(title, L"AxHost [模式%c] 客户区 %ldx%ld — 控件应同尺寸跟随",
                  (WCHAR)g_mode, rc.right - rc.left, rc.bottom - rc.top);
        SetWindowTextW(hwnd, title);
    }
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    static IOleObject *oo = NULL;
    static HostSite *site = NULL;

    switch (msg) {
    case WM_CREATE: {
        HRESULT hr;
        RECT rc;
        HMENU menu;
        GetClientRect(hwnd, &rc);

        site = create_site(hwnd);
        if (!site) return -1;

        ax_log("CoCreateInstance(CLSID_AxFormCtl) ...");
        hr = CoCreateInstance(&CLSID_AxFormCtl, NULL, CLSCTX_INPROC_SERVER,
                              &IID_IOleObject, (void **)&oo);
        if (FAILED(hr) || !oo) {
            ax_log("CoCreateInstance failed: 0x%08x (控件未注册？先 make install)", hr);
            MessageBoxW(hwnd, L"CoCreateInstance(CLSID_AxFormCtl) 失败：请先 make install 注册控件",
                        L"AxHost", MB_ICONERROR);
            return -1;
        }

        IOleObject_SetClientSite(oo, &site->IOleClientSite_iface);
        OleSetContainedObject((IUnknown *)oo, TRUE);

        ax_log("DoVerb(OLEIVERB_INPLACEACTIVATE) ...");
        hr = IOleObject_DoVerb(oo, OLEIVERB_INPLACEACTIVATE, NULL,
                               &site->IOleClientSite_iface, 0, hwnd, &rc);
        if (FAILED(hr))
            ax_log("DoVerb failed: 0x%08x", hr);
        else
            ax_log("DoVerb OK, control window should be a WS_CHILD of hwnd=%p", hwnd);

        /* 设置控件背景为蓝色：QI(IDispatch) 后调用 SetBgColor(RGB(0,0,255)) */
        {
            IDispatch *disp = NULL;
            if (SUCCEEDED(IOleObject_QueryInterface(oo, &IID_IDispatch, (void **)&disp))) {
                host_set_bgcolor(disp, RGB(0, 0, 255));
                IDispatch_Release(disp);
            }
        }

        /* 顶栏菜单：切换 A/B 两种几何同步模式 */
        menu = CreateMenu();
        {
            HMENU popup = CreatePopupMenu();
            AppendMenuW(popup, MF_STRING, IDM_MODE_RECTS,  L"模式 A: SetObjectRects");
            AppendMenuW(popup, MF_STRING, IDM_MODE_EXTENT, L"模式 B: SetExtent");
            AppendMenuW(menu, MF_POPUP, (UINT_PTR)popup, L"同步模式");
        }
        SetMenu(hwnd, menu);
        CheckMenuRadioItem(menu, IDM_MODE_RECTS, IDM_MODE_EXTENT, IDM_MODE_RECTS, MF_BYCOMMAND);

        update_title(hwnd);
        return 0;
    }

    case WM_SIZE:
        if (oo)
            sync_control(hwnd, oo);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wp) == IDM_MODE_RECTS || LOWORD(wp) == IDM_MODE_EXTENT) {
            g_mode = (LOWORD(wp) == IDM_MODE_RECTS) ? 'A' : 'B';
            CheckMenuRadioItem(GetMenu(hwnd), IDM_MODE_RECTS, IDM_MODE_EXTENT,
                               LOWORD(wp), MF_BYCOMMAND);
            ax_log("switched to mode %c", (char)g_mode);
            if (oo)
                sync_control(hwnd, oo);
        }
        break;

    case WM_DESTROY:
        ax_log("WM_DESTROY: releasing control and site");
        if (oo) {
            IOleObject_Close(oo, OLECLOSE_NOSAVE);
            IOleObject_Release(oo);
            oo = NULL;
        }
        if (site) {
            IOleClientSite_Release(&site->IOleClientSite_iface);
            site = NULL;
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show)
{
    WNDCLASSW wc;
    HWND hwnd;
    MSG m;

    g_hinst = hInst;
    OleInitialize(NULL);

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc   = wndproc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = APP_CLASS_W;
    RegisterClassW(&wc);

    hwnd = CreateWindowExW(0, APP_CLASS_W, APP_TITLE_W,
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
                           NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    ax_log("host window created, resizing it should keep the control in sync");

    while (GetMessageW(&m, NULL, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    OleUninitialize();
    return 0;
}
