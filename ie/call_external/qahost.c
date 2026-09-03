/*
 * WebBrowser Host - Pure Win32 API (No MFC, No ATL)
 *
 * Implements: IOleClientSite, IOleInPlaceSite, IDocHostUIHandler,
 *             IOleInPlaceFrame, window.external (IDispatch)
 *
 * Build: i686-w64-mingw32-gcc -o qahost.exe qahost.c -lole32 -loleaut32 -luuid -lurlmon -luser32 -lgdi32
 */

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <ole2.h>
#include <oleidl.h>
#include <mshtmhst.h>
#include <oaidl.h>
#include <exdisp.h>
#include <stdio.h>

/* ===== ExternalDispatch: window.external ===== */
typedef struct {
    IDispatch IDispatch_iface;
    LONG ref;
    HWND hwnd;
} ExternalDispatch;

#define DISPID_DOCOMMAND 1

static const struct { const char *name; int id; } cmd_map[] = {
    {"adjustwh", 1}, {"set_center", 2}, {"drag_window", 3},
    {"set_modalresult", 4}, {"close_window", 5}, {"min_wndow", 6}, {"hide_window", 7},
};

static int find_cmd(const char *n) {
    for(int i=0;i<sizeof(cmd_map)/sizeof(cmd_map[0]);i++)
        if(_stricmp(n,cmd_map[i].name)==0) return cmd_map[i].id;
    return 0;
}

static int js_int(IDispatch *d, const wchar_t *name) {
    DISPID did; DISPPARAMS dp={NULL,NULL,0,0}; VARIANT r; OLECHAR *n=(OLECHAR*)name;
    if(FAILED(IDispatch_GetIDsOfNames(d,&IID_NULL,&n,1,0,&did))) return 0;
    VariantInit(&r);
    if(FAILED(IDispatch_Invoke(d,did,&IID_NULL,0,DISPATCH_METHOD|DISPATCH_PROPERTYGET,&dp,&r,NULL,NULL))) return 0;
    int v=0;
    if(V_VT(&r)==VT_I4) v=V_I4(&r); else if(V_VT(&r)==VT_I2) v=V_I2(&r); else if(V_VT(&r)==VT_R8) v=(int)V_R8(&r); else if(V_VT(&r)==VT_BOOL) v=V_BOOL(&r)?1:0;
    VariantClear(&r);
    return v;
}

static void do_cmd(HWND hw, int id, IDispatch *args) {
    switch(id) {
    case 1: { /* adjustwh */
        int w=js_int(args,L"width"), h=js_int(args,L"height");
        RECT rc; SetRect(&rc,0,0,w,h);
        AdjustWindowRectEx(&rc,GetWindowLongW(hw,GWL_STYLE),FALSE,GetWindowLongW(hw,GWL_EXSTYLE));
        SetWindowPos(hw,NULL,0,0,rc.right-rc.left,rc.bottom-rc.top,SWP_NOMOVE|SWP_NOZORDER);
        printf("[adjustwh] %dx%d\n",w,h); break;
    }
    case 2: { /* set_center */
        RECT rc; GetWindowRect(hw,&rc);
        SetWindowPos(hw,NULL,(GetSystemMetrics(SM_CXSCREEN)-(rc.right-rc.left))/2,
            (GetSystemMetrics(SM_CYSCREEN)-(rc.bottom-rc.top))/2,0,0,SWP_NOSIZE|SWP_NOZORDER);
        break;
    }
    case 3: /* drag_window */
        ReleaseCapture(); SendMessageW(hw,WM_NCLBUTTONDOWN,HTCAPTION,0); break;
    case 4: /* set_modalresult */
        if(js_int(args,L"result")==2) PostQuitMessage(0); break;
    case 5: PostQuitMessage(0); break;
    case 6: ShowWindow(hw,SW_MINIMIZE); break;
    case 7: ShowWindow(hw,SW_HIDE); break;
    }
}

static inline ExternalDispatch *ed_from_disp(IDispatch *f) { return CONTAINING_RECORD(f,ExternalDispatch,IDispatch_iface); }

static HRESULT STDMETHODCALLTYPE ED_QI(IDispatch *f, REFIID r, void **p) {
    if(IsEqualIID(r,&IID_IUnknown)||IsEqualIID(r,&IID_IDispatch)){*p=f;IDispatch_AddRef(f);return S_OK;}
    *p=NULL; return E_NOINTERFACE;
}
static ULONG STDMETHODCALLTYPE ED_AddRef(IDispatch *f) { return InterlockedIncrement(&ed_from_disp(f)->ref); }
static ULONG STDMETHODCALLTYPE ED_Release(IDispatch *f) { ExternalDispatch *t=ed_from_disp(f); LONG r=InterlockedDecrement(&t->ref); if(!r)free(t); return r; }
static HRESULT STDMETHODCALLTYPE ED_GetTypeInfoCount(IDispatch *f,UINT*p){*p=0;return S_OK;}
static HRESULT STDMETHODCALLTYPE ED_GetTypeInfo(IDispatch *f,UINT i,LCID l,ITypeInfo**p){return E_NOTIMPL;}
static HRESULT STDMETHODCALLTYPE ED_GetIDsOfNames(IDispatch *f,REFIID r,LPOLESTR*n,UINT c,LCID l,DISPID*d){
    for(UINT i=0;i<c;i++) d[i]=(wcscmp(n[i],L"doCommand")==0)?DISPID_DOCOMMAND:DISPID_UNKNOWN;
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE ED_Invoke(IDispatch *f,DISPID id,REFIID r,LCID l,WORD w,DISPPARAMS*p,VARIANT*res,EXCEPINFO*e,UINT*err) {
    ExternalDispatch *t=ed_from_disp(f);
    if(id==DISPID_DOCOMMAND && p->cArgs>=2) {
        VARIANT *a=p->rgvarg; char cn[256]={0};
        if(V_VT(&a[1])==VT_BSTR) WideCharToMultiByte(CP_ACP,0,V_BSTR(&a[1]),-1,cn,256,NULL,NULL);
        int cid=find_cmd(cn);
        if(cid) {
            IDispatch *obj=V_VT(&a[0])==VT_DISPATCH?V_DISPATCH(&a[0]):NULL;
            do_cmd(t->hwnd,cid,obj);
        }
    }
    return S_OK;
}
static const IDispatchVtbl EDVtbl={ED_QI,ED_AddRef,ED_Release,ED_GetTypeInfoCount,ED_GetTypeInfo,ED_GetIDsOfNames,ED_Invoke};

static ExternalDispatch *create_ed(HWND hw) {
    ExternalDispatch *d=calloc(1,sizeof(*d)); d->IDispatch_iface.lpVtbl=&EDVtbl; d->ref=1; d->hwnd=hw;
    return d;
}

/* ===== DocHostSite: IOleClientSite + IOleInPlaceSite + IDocHostUIHandler ===== */
typedef struct {
    IOleClientSite IOleClientSite_iface;
    IOleInPlaceSite IOleInPlaceSite_iface;
    IDocHostUIHandler IDocHostUIHandler_iface;
    IOleInPlaceFrame IOleInPlaceFrame_iface;
    LONG ref;
    HWND hwnd;
    ExternalDispatch *external;
} DocHostSite;

static inline DocHostSite *site_from_cs(IOleClientSite *f){return CONTAINING_RECORD(f,DocHostSite,IOleClientSite_iface);}
static inline DocHostSite *site_from_ips(IOleInPlaceSite *f){return CONTAINING_RECORD(f,DocHostSite,IOleInPlaceSite_iface);}
static inline DocHostSite *site_from_ui(IDocHostUIHandler *f){return CONTAINING_RECORD(f,DocHostSite,IDocHostUIHandler_iface);}
static inline DocHostSite *site_from_ipf(IOleInPlaceFrame *f){return CONTAINING_RECORD(f,DocHostSite,IOleInPlaceFrame_iface);}

static HRESULT site_qi(DocHostSite *t, REFIID r, void **p) {
    if(IsEqualIID(r,&IID_IUnknown)||IsEqualIID(r,&IID_IOleClientSite)){*p=&t->IOleClientSite_iface;}
    else if(IsEqualIID(r,&IID_IOleInPlaceSite)||IsEqualIID(r,&IID_IOleInPlaceSiteEx)){*p=&t->IOleInPlaceSite_iface;}
    else if(IsEqualIID(r,&IID_IDocHostUIHandler)){*p=&t->IDocHostUIHandler_iface;}
    else if(IsEqualIID(r,&IID_IOleInPlaceFrame)||IsEqualIID(r,&IID_IOleInPlaceUIWindow)){*p=&t->IOleInPlaceFrame_iface;}
    else {*p=NULL; return E_NOINTERFACE;}
    IUnknown_AddRef((IUnknown*)*p); return S_OK;
}

/* IOleClientSite */
static HRESULT STDMETHODCALLTYPE CS_QI(IOleClientSite *f,REFIID r,void**p){return site_qi(site_from_cs(f),r,p);}
static ULONG STDMETHODCALLTYPE CS_AddRef(IOleClientSite *f){return InterlockedIncrement(&site_from_cs(f)->ref);}
static ULONG STDMETHODCALLTYPE CS_Release(IOleClientSite *f){DocHostSite *t=site_from_cs(f);LONG r=InterlockedDecrement(&t->ref);if(!r){if(t->external)IDispatch_Release(&t->external->IDispatch_iface);free(t);}return r;}
static HRESULT STDMETHODCALLTYPE CS_SaveObject(IOleClientSite *f){return S_OK;}
static HRESULT STDMETHODCALLTYPE CS_GetMoniker(IOleClientSite *f,DWORD a,DWORD b,IMoniker**p){*p=NULL;return E_NOTIMPL;}
static HRESULT STDMETHODCALLTYPE CS_GetContainer(IOleClientSite *f,IOleContainer**p){*p=NULL;return E_NOTIMPL;}
static HRESULT STDMETHODCALLTYPE CS_ShowObject(IOleClientSite *f){return S_OK;}
static HRESULT STDMETHODCALLTYPE CS_OnShowWindow(IOleClientSite *f,BOOL s){return S_OK;}
static HRESULT STDMETHODCALLTYPE CS_RequestNewObjectLayout(IOleClientSite *f){return E_NOTIMPL;}
static const IOleClientSiteVtbl CSVtbl={CS_QI,CS_AddRef,CS_Release,CS_SaveObject,CS_GetMoniker,CS_GetContainer,CS_ShowObject,CS_OnShowWindow,CS_RequestNewObjectLayout};

/* IOleInPlaceSite */
static HRESULT STDMETHODCALLTYPE IPS_QI(IOleInPlaceSite *f,REFIID r,void**p){return site_qi(site_from_ips(f),r,p);}
static ULONG STDMETHODCALLTYPE IPS_AddRef(IOleInPlaceSite *f){return IOleClientSite_AddRef(&site_from_ips(f)->IOleClientSite_iface);}
static ULONG STDMETHODCALLTYPE IPS_Release(IOleInPlaceSite *f){return IOleClientSite_Release(&site_from_ips(f)->IOleClientSite_iface);}
static HRESULT STDMETHODCALLTYPE IPS_GetWindow(IOleInPlaceSite *f,HWND*p){*p=site_from_ips(f)->hwnd;return S_OK;}
static HRESULT STDMETHODCALLTYPE IPS_ContextSensitiveHelp(IOleInPlaceSite *f,BOOL e){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPS_CanInPlaceActivate(IOleInPlaceSite *f){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPS_OnInPlaceActivate(IOleInPlaceSite *f){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPS_OnUIActivate(IOleInPlaceSite *f){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPS_GetWindowContext(IOleInPlaceSite *f,IOleInPlaceFrame**ppFrame,IOleInPlaceUIWindow**ppDoc,LPRECT prcPos,LPRECT prcClip,LPOLEINPLACEFRAMEINFO pi){
    DocHostSite *t=site_from_ips(f);
    *ppFrame=&t->IOleInPlaceFrame_iface; IOleInPlaceFrame_AddRef(*ppFrame);
    *ppDoc=NULL;
    GetClientRect(t->hwnd,prcPos); *prcClip=*prcPos;
    if(pi){pi->cb=sizeof(*pi);pi->fMDIApp=FALSE;pi->hwndFrame=t->hwnd;pi->haccel=NULL;pi->cAccelEntries=0;}
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE IPS_Scroll(IOleInPlaceSite *f,SIZE s){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPS_OnUIDeactivate(IOleInPlaceSite *f,BOOL e){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPS_OnInPlaceDeactivate(IOleInPlaceSite *f){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPS_DiscardUndoState(IOleInPlaceSite *f){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPS_DeactivateAndUndo(IOleInPlaceSite *f){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPS_OnPosRectChange(IOleInPlaceSite *f,LPCRECT r){return S_OK;}
static const IOleInPlaceSiteVtbl IPSVtbl={IPS_QI,IPS_AddRef,IPS_Release,IPS_GetWindow,IPS_ContextSensitiveHelp,
    IPS_CanInPlaceActivate,IPS_OnInPlaceActivate,IPS_OnUIActivate,IPS_GetWindowContext,IPS_Scroll,
    IPS_OnUIDeactivate,IPS_OnInPlaceDeactivate,IPS_DiscardUndoState,IPS_DeactivateAndUndo,IPS_OnPosRectChange};

/* IDocHostUIHandler */
static HRESULT STDMETHODCALLTYPE UI_QI(IDocHostUIHandler *f,REFIID r,void**p){return site_qi(site_from_ui(f),r,p);}
static ULONG STDMETHODCALLTYPE UI_AddRef(IDocHostUIHandler *f){return IOleClientSite_AddRef(&site_from_ui(f)->IOleClientSite_iface);}
static ULONG STDMETHODCALLTYPE UI_Release(IDocHostUIHandler *f){return IOleClientSite_Release(&site_from_ui(f)->IOleClientSite_iface);}
static HRESULT STDMETHODCALLTYPE UI_ShowContextMenu(IDocHostUIHandler *f,DWORD d,POINT*p,IUnknown*r,IDispatch*d2){return S_OK;}
static HRESULT STDMETHODCALLTYPE UI_GetHostInfo(IDocHostUIHandler *f,DOCHOSTUIINFO*p){p->dwFlags=DOCHOSTUIFLAG_NO3DBORDER|DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE;return S_OK;}
static HRESULT STDMETHODCALLTYPE UI_ShowUI(IDocHostUIHandler *f,DWORD d,IOleInPlaceActiveObject*a,IOleCommandTarget*c,IOleInPlaceFrame*fr,IOleInPlaceUIWindow*w){return S_OK;}
static HRESULT STDMETHODCALLTYPE UI_HideUI(IDocHostUIHandler *f){return S_OK;}
static HRESULT STDMETHODCALLTYPE UI_UpdateUI(IDocHostUIHandler *f){return S_OK;}
static HRESULT STDMETHODCALLTYPE UI_EnableModeless(IDocHostUIHandler *f,BOOL e){return S_OK;}
static HRESULT STDMETHODCALLTYPE UI_OnDocWindowActivate(IDocHostUIHandler *f,BOOL e){return S_OK;}
static HRESULT STDMETHODCALLTYPE UI_OnFrameWindowActivate(IDocHostUIHandler *f,BOOL e){return S_OK;}
static HRESULT STDMETHODCALLTYPE UI_ResizeBorder(IDocHostUIHandler *f,LPCRECT r,IOleInPlaceUIWindow*w,BOOL fw){return S_OK;}
static HRESULT STDMETHODCALLTYPE UI_TranslateAccelerator(IDocHostUIHandler *f,LPMSG m,const GUID*g,DWORD d){return S_FALSE;}
static HRESULT STDMETHODCALLTYPE UI_GetOptionKeyPath(IDocHostUIHandler *f,LPOLESTR*p,DWORD d){return S_FALSE;}
static HRESULT STDMETHODCALLTYPE UI_GetDropTarget(IDocHostUIHandler *f,IDropTarget*d,IDropTarget**r){return E_NOTIMPL;}
static HRESULT STDMETHODCALLTYPE UI_GetExternal(IDocHostUIHandler *f,IDispatch**p){
    DocHostSite *t=site_from_ui(f);
    if(t->external){*p=&t->external->IDispatch_iface;IDispatch_AddRef(*p);return S_OK;}
    *p=NULL; return E_NOTIMPL;
}
static HRESULT STDMETHODCALLTYPE UI_TranslateUrl(IDocHostUIHandler *f,DWORD d,OLECHAR*i,LPOLESTR*o){return S_FALSE;}
static HRESULT STDMETHODCALLTYPE UI_FilterDataObject(IDocHostUIHandler *f,IDataObject*d,IDataObject**r){return S_FALSE;}
static const IDocHostUIHandlerVtbl UIVtbl={UI_QI,UI_AddRef,UI_Release,UI_ShowContextMenu,UI_GetHostInfo,
    UI_ShowUI,UI_HideUI,UI_UpdateUI,UI_EnableModeless,UI_OnDocWindowActivate,UI_OnFrameWindowActivate,
    UI_ResizeBorder,UI_TranslateAccelerator,UI_GetOptionKeyPath,UI_GetDropTarget,
    UI_GetExternal,UI_TranslateUrl,UI_FilterDataObject};

/* IOleInPlaceFrame */
static HRESULT STDMETHODCALLTYPE IPF_QI(IOleInPlaceFrame *f,REFIID r,void**p){return site_qi(site_from_ipf(f),r,p);}
static ULONG STDMETHODCALLTYPE IPF_AddRef(IOleInPlaceFrame *f){return IOleClientSite_AddRef(&site_from_ipf(f)->IOleClientSite_iface);}
static ULONG STDMETHODCALLTYPE IPF_Release(IOleInPlaceFrame *f){return IOleClientSite_Release(&site_from_ipf(f)->IOleClientSite_iface);}
static HRESULT STDMETHODCALLTYPE IPF_GetWindow(IOleInPlaceFrame *f,HWND*p){*p=site_from_ipf(f)->hwnd;return S_OK;}
static HRESULT STDMETHODCALLTYPE IPF_ContextSensitiveHelp(IOleInPlaceFrame *f,BOOL e){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPF_GetBorder(IOleInPlaceFrame *f,LPRECT r){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPF_RequestBorderSpace(IOleInPlaceFrame *f,LPCRECT r){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPF_SetBorderSpace(IOleInPlaceFrame *f,LPCRECT r){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPF_SetActiveObject(IOleInPlaceFrame *f,IOleInPlaceActiveObject*a,LPCOLESTR t){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPF_InsertMenus(IOleInPlaceFrame *f,HMENU h,OLEMENUGROUPWIDTHS*w){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPF_SetMenu(IOleInPlaceFrame *f,HMENU hm,HMENU hw,HWND a){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPF_RemoveMenus(IOleInPlaceFrame *f,HMENU h){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPF_SetStatusText(IOleInPlaceFrame *f,LPCOLESTR t){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPF_EnableModeless(IOleInPlaceFrame *f,BOOL e){return S_OK;}
static HRESULT STDMETHODCALLTYPE IPF_TranslateAccelerator(IOleInPlaceFrame *f,LPMSG m,WORD w){return S_FALSE;}
static const IOleInPlaceFrameVtbl IPFVtbl={IPF_QI,IPF_AddRef,IPF_Release,IPF_GetWindow,IPF_ContextSensitiveHelp,
    IPF_GetBorder,IPF_RequestBorderSpace,IPF_SetBorderSpace,IPF_SetActiveObject,
    IPF_InsertMenus,IPF_SetMenu,IPF_RemoveMenus,IPF_SetStatusText,IPF_EnableModeless,IPF_TranslateAccelerator};

static DocHostSite *create_site(HWND hw) {
    DocHostSite *s=calloc(1,sizeof(*s));
    s->IOleClientSite_iface.lpVtbl=&CSVtbl;
    s->IOleInPlaceSite_iface.lpVtbl=&IPSVtbl;
    s->IDocHostUIHandler_iface.lpVtbl=&UIVtbl;
    s->IOleInPlaceFrame_iface.lpVtbl=&IPFVtbl;
    s->ref=1; s->hwnd=hw; s->external=create_ed(hw);
    return s;
}

/* ===== Main Window ===== */
static wchar_t g_url[1024]=L"about:blank";

static LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam) {
    static IWebBrowser2 *wb=NULL;
    static DocHostSite *site=NULL;

    switch(msg) {
    case WM_CREATE: {
        HRESULT hr; RECT rc; GetClientRect(hwnd,&rc);
        printf("[CREATE] start\n"); fflush(stdout);

        site=create_site(hwnd);
        if(!site) return -1;
        printf("[CREATE] site OK\n"); fflush(stdout);

        IOleObject *oo=NULL;
        hr=CoCreateInstance(&CLSID_WebBrowser,NULL,CLSCTX_INPROC_SERVER,&IID_IOleObject,(void**)&oo);
        if(FAILED(hr)||!oo){printf("[CREATE] CoCreate failed:0x%08lx\n",hr);return -1;}
        printf("[CREATE] WebBrowser OK\n"); fflush(stdout);

        IOleObject_SetClientSite(oo,&site->IOleClientSite_iface);
        OleSetContainedObject((IUnknown*)oo,TRUE);

        hr=IOleObject_DoVerb(oo,OLEIVERB_INPLACEACTIVATE,NULL,&site->IOleClientSite_iface,0,hwnd,&rc);
        if(FAILED(hr)){printf("[CREATE] DoVerb failed:0x%08lx\n",hr);return -1;}
        printf("[CREATE] DoVerb OK\n"); fflush(stdout);

        hr=IOleObject_QueryInterface(oo,&IID_IWebBrowser2,(void**)&wb);
        IOleObject_Release(oo);
        if(FAILED(hr)||!wb){printf("[CREATE] QI IWebBrowser2 failed:0x%08lx\n",hr);return -1;}
        printf("[CREATE] IWebBrowser2 OK\n"); fflush(stdout);

        VARIANT vu,ve; VariantInit(&vu); VariantInit(&ve);
        V_VT(&vu)=VT_BSTR; V_BSTR(&vu)=SysAllocString(g_url);
        IWebBrowser2_Navigate2(wb,&vu,&ve,&ve,&ve,&ve);
        VariantClear(&vu);
        printf("[CREATE] Navigate: %ls\n",g_url); fflush(stdout);
        return 0;
    }
    case WM_SIZE:
        if(wb) {
            IOleInPlaceObject *ipo=NULL;
            if(SUCCEEDED(IWebBrowser2_QueryInterface(wb,&IID_IOleInPlaceObject,(void**)&ipo))&&ipo) {
                RECT rc; GetClientRect(hwnd,&rc);
                IOleInPlaceObject_SetObjectRects(ipo,&rc,&rc);
                IOleInPlaceObject_Release(ipo);
            }
        }
        return 0;
    case WM_DESTROY:
        if(wb){IWebBrowser2_Release(wb);wb=NULL;}
        if(site){IOleClientSite_Release(&site->IOleClientSite_iface);site=NULL;}
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd,msg,wParam,lParam);
}

int WINAPI WinMain(HINSTANCE hInst,HINSTANCE hPrev,LPSTR cmd,int show) {
    if(cmd&&*cmd) MultiByteToWideChar(CP_ACP,0,cmd,-1,g_url,1024);
    OleInitialize(NULL);

    WNDCLASSW wc={0}; wc.lpfnWndProc=WndProc; wc.hInstance=hInst;
    wc.hCursor=LoadCursor(NULL,IDC_ARROW); wc.lpszClassName=L"QAHost";
    RegisterClassW(&wc);

    HWND hw=CreateWindowExW(WS_EX_APPWINDOW,L"QAHost",L"QA Host",
        WS_POPUP|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,1920,1080,NULL,NULL,hInst,NULL);
    ShowWindow(hw,show); UpdateWindow(hw);

    MSG m;
    while(GetMessage(&m,NULL,0,0)){TranslateMessage(&m);DispatchMessage(&m);}
    OleUninitialize();
    return 0;
}
