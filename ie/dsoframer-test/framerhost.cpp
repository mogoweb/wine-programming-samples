/*
 * framerhost.cpp - ActiveX container plumbing for DsoFramer FramerControl.
 *
 * Implements the minimal OLE container set the control expects:
 *   IOleClientSite / IOleControlSite / IOleInPlaceSiteEx /
 *   IOleInPlaceFrame / IServiceProvider
 * plus a static dispinterface sink for _DFramerCtlEvents (connected
 * through IConnectionPointContainer::FindConnectionPoint).
 *
 * Modeled after ie/call_external/qahost.c and the CDsoFramerControl
 * sources (dsofcontrol.cpp) of the original sample.
 */
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <ole2.h>
#include <ocidl.h>
#include <olectl.h>
#include <stdio.h>
#include <string.h>

#include "framerhost.h"

/* DISPID_AMBIENT_USERMODE lives in olectl.h (-709); guard for safety */
#ifndef DISPID_AMBIENT_USERMODE
#define DISPID_AMBIENT_USERMODE (-709)
#endif

class CFramerControl;

/* =====================================================================
 * Combined site object: one allocation, several COM faces.
 * ===================================================================== */
class CFramerSite :
    public IOleClientSite,
    public IOleControlSite,
    public IOleInPlaceSiteEx,
    public IOleInPlaceFrame,
    public IServiceProvider
{
public:
    CFramerSite(HWND hwnd, CFramerControl *owner)
        : m_ref(1), m_hwnd(hwnd), m_owner(owner) {}
    virtual ~CFramerSite() {}

    /* IUnknown (shared across all faces) */
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override
    {
        if (!ppv) return E_POINTER;
        *ppv = NULL;
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IOleClientSite))
            *ppv = static_cast<IOleClientSite*>(this);
        else if (IsEqualIID(riid, IID_IOleControlSite))
            *ppv = static_cast<IOleControlSite*>(this);
        else if (IsEqualIID(riid, IID_IOleInPlaceSite) || IsEqualIID(riid, IID_IOleInPlaceSiteEx))
            *ppv = static_cast<IOleInPlaceSite*>(this);
        else if (IsEqualIID(riid, IID_IOleInPlaceFrame) || IsEqualIID(riid, IID_IOleInPlaceUIWindow))
            *ppv = static_cast<IOleInPlaceFrame*>(this);
        else if (IsEqualIID(riid, IID_IServiceProvider))
            *ppv = static_cast<IServiceProvider*>(this);
        else
            return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef() override  { return InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() override
    {
        ULONG r = InterlockedDecrement(&m_ref);
        if (r == 0) delete this;
        return r;
    }

    /* IOleWindow / IOleInPlaceSite / IOleInPlaceSiteEx */
    STDMETHODIMP GetWindow(HWND *phwnd) override                 { *phwnd = m_hwnd; return S_OK; }
    STDMETHODIMP ContextSensitiveHelp(BOOL) override             { return S_OK; }
    STDMETHODIMP CanInPlaceActivate() override                   { return S_OK; }
    STDMETHODIMP OnInPlaceActivate() override                    { return S_OK; }
    STDMETHODIMP OnUIActivate() override                         { return S_OK; }
    STDMETHODIMP GetWindowContext(IOleInPlaceFrame **ppFrame,
        IOleInPlaceUIWindow **ppDoc, LPRECT prcPos, LPRECT prcClip,
        LPOLEINPLACEFRAMEINFO pfi) override
    {
        *ppFrame = static_cast<IOleInPlaceFrame*>(this);
        (*ppFrame)->AddRef();
        *ppDoc = NULL;
        GetClientRect(m_hwnd, prcPos);
        *prcClip = *prcPos;
        if (pfi) {
            pfi->cb = sizeof(*pfi);
            pfi->fMDIApp = FALSE;
            pfi->hwndFrame = m_hwnd;
            pfi->haccel = NULL;
            pfi->cAccelEntries = 0;
        }
        return S_OK;
    }
    STDMETHODIMP Scroll(SIZE) override                           { return S_OK; }
    STDMETHODIMP OnUIDeactivate(BOOL) override                   { return S_OK; }
    STDMETHODIMP OnInPlaceDeactivate() override                  { return S_OK; }
    STDMETHODIMP DiscardUndoState() override                     { return S_OK; }
    STDMETHODIMP DeactivateAndUndo() override                    { return S_OK; }
    STDMETHODIMP OnPosRectChange(LPCRECT) override               { return S_OK; }
    STDMETHODIMP OnInPlaceActivateEx(BOOL *pfNoRedraw, DWORD) override
    {
        if (pfNoRedraw) *pfNoRedraw = FALSE;
        return S_OK;
    }
    STDMETHODIMP OnInPlaceDeactivateEx(BOOL) override            { return S_OK; }
    STDMETHODIMP RequestUIActivate() override                    { return S_FALSE; }

    /* IOleClientSite */
    STDMETHODIMP SaveObject() override                           { return S_OK; }
    STDMETHODIMP GetMoniker(DWORD, DWORD, IMoniker **pmk) override { *pmk = NULL; return E_NOTIMPL; }
    STDMETHODIMP GetContainer(IOleContainer **pc) override       { *pc = NULL; return E_NOTIMPL; }
    STDMETHODIMP ShowObject() override                           { return S_OK; }
    STDMETHODIMP OnShowWindow(BOOL) override                     { return S_OK; }
    STDMETHODIMP RequestNewObjectLayout() override               { return E_NOTIMPL; }

    /* IOleControlSite */
    STDMETHODIMP OnControlInfoChanged() override                 { return S_OK; }
    STDMETHODIMP LockInPlaceActive(BOOL) override                { return S_OK; }
    STDMETHODIMP GetExtendedControl(IDispatch **pd) override     { *pd = NULL; return E_NOTIMPL; }
    STDMETHODIMP TransformCoords(POINTL *, POINTF *, DWORD) override { return E_NOTIMPL; }
    STDMETHODIMP TranslateAccelerator(LPMSG, DWORD) override     { return S_FALSE; } /* control handles keys */
    STDMETHODIMP OnFocus(BOOL) override                          { return S_OK; }
    STDMETHODIMP ShowPropertyFrame() override                    { return E_NOTIMPL; }

    /* IOleInPlaceUIWindow / IOleInPlaceFrame */
    STDMETHODIMP GetBorder(LPRECT) override                      { return S_OK; }
    STDMETHODIMP RequestBorderSpace(LPCRECT) override            { return S_OK; }
    STDMETHODIMP SetBorderSpace(LPCRECT) override                { return S_OK; }
    STDMETHODIMP SetActiveObject(IOleInPlaceActiveObject *, LPCOLESTR) override { return S_OK; }
    STDMETHODIMP InsertMenus(HMENU, LPOLEMENUGROUPWIDTHS) override { return S_OK; } /* control draws own menus */
    STDMETHODIMP SetMenu(HMENU, HOLEMENU, HWND) override         { return S_OK; }
    STDMETHODIMP RemoveMenus(HMENU) override                     { return S_OK; }
    STDMETHODIMP SetStatusText(LPCOLESTR) override               { return S_OK; }
    STDMETHODIMP EnableModeless(BOOL) override                   { return S_OK; }
    STDMETHODIMP TranslateAccelerator(LPMSG, WORD) override      { return S_FALSE; }

    /* IServiceProvider */
    STDMETHODIMP QueryService(REFGUID, REFIID, void **ppv) override
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

private:
    LONG m_ref;
    HWND m_hwnd;
    CFramerControl *m_owner;
};

/* =====================================================================
 * Event sink: static dispinterface for _DFramerCtlEvents.
 * ===================================================================== */
class CFramerEventSink : public IDispatch
{
public:
    CFramerEventSink(CFramerControl *owner) : m_ref(1), m_owner(owner) {}

    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override
    {
        /* The control QIs for DIID__DFramerCtlEvents first; keep
           IDispatch as fallback (matches .NET interop behavior in
           CDsoFramerControl's connection point). */
        if (IsEqualIID(riid, DIID__DFramerCtlEvents) ||
            IsEqualIID(riid, IID_IDispatch) || IsEqualIID(riid, IID_IUnknown))
        {
            *ppv = static_cast<IDispatch*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override  { return InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() override { return InterlockedDecrement(&m_ref); } /* no delete: owner lifetime */

    STDMETHODIMP GetTypeInfoCount(UINT *pct) override { *pct = 0; return S_OK; }
    STDMETHODIMP GetTypeInfo(UINT, LCID, ITypeInfo **) override { return E_NOTIMPL; }
    STDMETHODIMP GetIDsOfNames(REFIID, LPOLESTR *, UINT, LCID, DISPID *) override { return DISP_E_UNKNOWNNAME; }

    STDMETHODIMP Invoke(DISPID dispid, REFIID, LCID, WORD,
                        DISPPARAMS *params, VARIANT *, EXCEPINFO *, UINT *) override
    {
        FramerEventHandlers *h = &m_owner->Handlers();

        /* dispinterface args arrive in reverse order (rgvarg[0] = last param) */
        switch (dispid) {
        case DSOF_DISPID_FILECMD: {
            if (params->cArgs != 2) return DISP_E_BADPARAMCOUNT;
            /* rgvarg[1] = Item (VT_I4), rgvarg[0] = Cancel (VT_BOOL|VT_BYREF) */
            dsoFileCommandType item = (dsoFileCommandType)params->rgvarg[1].lVal;
            BOOL cancel = (params->rgvarg[0].pboolVal && *params->rgvarg[0].pboolVal != 0);
            if (h->OnFileCommand && h->OnFileCommand(item, cancel))
                *params->rgvarg[0].pboolVal = -1; /* VARIANT_TRUE: cancel default */
            return S_OK;
        }
        case DSOF_DISPID_DOCOPEN: {
            if (params->cArgs != 2) return DISP_E_BADPARAMCOUNT;
            /* rgvarg[1] = File (BSTR), rgvarg[0] = Document (IDispatch*) */
            LPCWSTR file = (params->rgvarg[1].vt == VT_BSTR && params->rgvarg[1].bstrVal)
                            ? params->rgvarg[1].bstrVal : L"";
            IDispatch *doc = (params->rgvarg[0].vt == VT_DISPATCH) ? params->rgvarg[0].pdispVal : NULL;
            if (h->OnDocumentOpened) h->OnDocumentOpened(file, doc);
            return S_OK;
        }
        case DSOF_DISPID_DOCCLOSE:
            if (h->OnDocumentClosed) h->OnDocumentClosed();
            return S_OK;
        case DSOF_DISPID_ACTIVATE:
            return S_OK;
        case DSOF_DISPID_BDOCCLOSE: {
            if (params->cArgs != 2) return DISP_E_BADPARAMCOUNT;
            /* rgvarg[1] = Document, rgvarg[0] = Cancel (BYREF) */
            IDispatch *doc = (params->rgvarg[1].vt == VT_DISPATCH) ? params->rgvarg[1].pdispVal : NULL;
            if (h->BeforeDocumentClosed && h->BeforeDocumentClosed(doc))
                *params->rgvarg[0].pboolVal = -1;
            return S_OK;
        }
        case DSOF_DISPID_BDOCSAVE:
            return S_OK;
        case DSOF_DISPID_ENDPREVIEW:
            if (h->OnPrintPreviewExit) h->OnPrintPreviewExit();
            return S_OK;
        case DSOF_DISPID_SAVECOMPLETE:
            return S_OK;
        }
        return DISP_E_MEMBERNOTFOUND;
    }

private:
    LONG m_ref;
    CFramerControl *m_owner;
};

/* =====================================================================
 * In-memory IPropertyBag: feeds the control the same defaults the VB6
 * container loads from FTestApp.frm (Titlebar/Toolbars/Menubar=-1,
 * BorderStyle=1, ...). Without this Load call the control keeps its
 * internal defaults (all flags 0) and draws no chrome.
 * ===================================================================== */
struct BagEntry { const OLECHAR *name; VARIANT var; };

class CPropertyBag : public IPropertyBag
{
public:
    CPropertyBag(BagEntry *entries, ULONG count)
        : m_ref(1), m_entries(entries), m_count(count) {}
    virtual ~CPropertyBag() {}

    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override
    {
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IPropertyBag))
        {
            *ppv = static_cast<IPropertyBag*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override  { return InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() override { ULONG r = InterlockedDecrement(&m_ref); if (!r) delete this; return r; }

    STDMETHODIMP Read(LPCOLESTR name, VARIANT *pvar, IErrorLog *log) override
    {
        printf("[bag] Read(%ls)\n", name); fflush(stdout);
        for (ULONG i = 0; i < m_count; i++)
        {
            if (lstrcmpiW(m_entries[i].name, name) == 0)
            {
                VariantClear(pvar);
                HRESULT hr = VariantCopy(pvar, &m_entries[i].var);
                printf("[bag]   -> hit vt=%d val=%ld hr=0x%08lX\n",
                       pvar->vt, (long)pvar->lVal, (unsigned long)hr);
                return hr;
            }
        }
        printf("[bag]   -> miss\n");
        return E_INVALIDARG; /* property not in bag */
    }
    STDMETHODIMP Write(LPCOLESTR, VARIANT *) override { return S_OK; }

private:
    LONG m_ref;
    BagEntry *m_entries;
    ULONG m_count;
};

/* =====================================================================
 * CFramerControl
 * ===================================================================== */
CFramerControl::CFramerControl()
    : m_ctl(NULL), m_unkCtl(NULL), m_oleObj(NULL), m_hwndCtl(NULL),
      m_hwndParent(NULL), m_dwEventCookie(0)
{
    memset(&m_handlers, 0, sizeof(m_handlers));
}

CFramerControl::~CFramerControl()
{
    UnadviseEvents();
    if (m_oleObj) {
        IOleObject *obj = m_oleObj;
        m_oleObj = NULL;
        obj->Close(OLECLOSE_NOSAVE);
        obj->Release();
    }
    if (m_ctl) { m_ctl->Release(); m_ctl = NULL; }
    if (m_unkCtl) { m_unkCtl->Release(); m_unkCtl = NULL; }
}

HRESULT CFramerControl::LoadDefaultProperties(void)
{
    /* Mirrors FTestApp.frm control properties (VB6 container parity) */
    static const WCHAR names[][12] = {
        L"_ExtentX", L"_ExtentY", L"Titlebar", L"Toolbars",
        L"Menubar", L"BorderStyle"
    };
    static VARIANT vars[6];
    static BOOL fInit = FALSE;

    if (!fInit)
    {
        ULONG i;
        for (i = 0; i < 6; i++) { VariantInit(&vars[i]); vars[i].vt = VT_I4; }
        vars[0].lVal = 14526;   /* _ExtentX  (himetric, from .frm) */
        vars[1].lVal = 11245;   /* _ExtentY */
        vars[2].lVal = -1;      /* Titlebar  = TRUE */
        vars[3].lVal = -1;      /* Toolbars  = TRUE */
        vars[4].lVal = -1;      /* Menubar   = TRUE */
        vars[5].lVal = 1;       /* BorderStyle = Outline */
        fInit = TRUE;
    }

    BagEntry entries[6] = {
        { names[0], vars[0] }, { names[1], vars[1] }, { names[2], vars[2] },
        { names[3], vars[3] }, { names[4], vars[4] }, { names[5], vars[5] },
    };

    HRESULT hr = S_OK;
    IPersistPropertyBag *ppb = NULL;
    if (SUCCEEDED(m_unkCtl->QueryInterface(IID_IPersistPropertyBag, (void**)&ppb)))
    {
        CPropertyBag *bag = new CPropertyBag(entries, 6);
        hr = ppb->Load(bag, NULL);
        bag->Release();
        ppb->Release();
    }
    return hr;
}

HRESULT CFramerControl::Create(HWND hwndParent, const FramerEventHandlers *handlers)
{
    HRESULT hr;
    RECT rc;

    m_hwndParent = hwndParent;
    if (handlers) m_handlers = *handlers;

    hr = CoCreateInstance(CLSID_FramerControl, NULL, CLSCTX_INPROC_SERVER,
                          IID_IUnknown, (void**)&m_unkCtl);
    if (FAILED(hr)) { printf("[framer] CoCreateInstance failed: 0x%08lX\n", hr); return hr; }

    hr = m_unkCtl->QueryInterface(IID_IOleObject, (void**)&m_oleObj);
    if (FAILED(hr)) { printf("[framer] QI IOleObject failed: 0x%08lX\n", hr); return hr; }

    hr = m_unkCtl->QueryInterface(IID__FramerControl, (void**)&m_ctl);
    if (FAILED(hr)) { printf("[framer] QI _FramerControl failed: 0x%08lX\n", hr); return hr; }

    /* Standard container init: load design-time properties before
       activation (VB6 does this from the .frm; without it the control
       draws no titlebar/menubar/toolbars). */
    hr = LoadDefaultProperties();
    printf("[framer] LoadDefaultProperties hr=0x%08lX\n", (unsigned long)hr);

    CFramerSite *site = new CFramerSite(hwndParent, this);
    m_oleObj->SetClientSite(site);
    site->Release(); /* control holds its own ref */
    OleSetContainedObject((IUnknown*)m_oleObj, TRUE);

    GetClientRect(hwndParent, &rc);
    hr = m_oleObj->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, NULL, 0, hwndParent, &rc);
    if (FAILED(hr)) { printf("[framer] DoVerb(INPLACEACTIVATE) failed: 0x%08lX\n", hr); return hr; }

    /* Cache the in-place window for positioning */
    IOleInPlaceObject *ipo = NULL;
    if (SUCCEEDED(m_unkCtl->QueryInterface(IID_IOleInPlaceObject, (void**)&ipo))) {
        ipo->GetWindow(&m_hwndCtl);
        ipo->SetObjectRects(&rc, &rc);
        ipo->Release();
    }

    hr = AdviseEvents();
    if (FAILED(hr)) { printf("[framer] Advise events failed: 0x%08lX\n", hr); return hr; }

    return S_OK;
}

void CFramerControl::SetRects(const RECT *rc)
{
    if (!m_oleObj || !rc) return;

    /* Pixels -> himetric (2540 units/cm at 96dpi), matching
       DsoPixelsToHimetric in the control source. */
    SIZEL sl;
    sl.cx = MulDiv(rc->right - rc->left, 2540, 96);
    sl.cy = MulDiv(rc->bottom - rc->top, 2540, 96);
    HRESULT hr = m_oleObj->SetExtent(DVASPECT_CONTENT, &sl);
    if (FAILED(hr))
        printf("[framer] SetExtent failed: 0x%08lX\n", (unsigned long)hr);

    /* Reposition the in-place window */
    IOleInPlaceObject *ipo = NULL;
    if (SUCCEEDED(m_unkCtl->QueryInterface(IID_IOleInPlaceObject, (void**)&ipo))) {
        ipo->SetObjectRects(rc, rc);
        ipo->Release();
    }
}

HRESULT CFramerControl::AdviseEvents(void)
{
    HRESULT hr;
    IConnectionPointContainer *cpc = NULL;
    IConnectionPoint *cp = NULL;

    hr = m_unkCtl->QueryInterface(IID_IConnectionPointContainer, (void**)&cpc);
    if (FAILED(hr)) return hr;

    hr = cpc->FindConnectionPoint(DIID__DFramerCtlEvents, &cp);
    cpc->Release();
    if (FAILED(hr)) return hr;

    CFramerEventSink *sink = new CFramerEventSink(this);
    hr = cp->Advise((IUnknown*)sink, &m_dwEventCookie);
    cp->Release();
    sink->Release(); /* connection point holds its own ref */
    return hr;
}

void CFramerControl::UnadviseEvents(void)
{
    if (m_dwEventCookie && m_unkCtl) {
        IConnectionPointContainer *cpc = NULL;
        if (SUCCEEDED(m_unkCtl->QueryInterface(IID_IConnectionPointContainer, (void**)&cpc))) {
            IConnectionPoint *cp = NULL;
            if (SUCCEEDED(cpc->FindConnectionPoint(DIID__DFramerCtlEvents, &cp))) {
                cp->Unadvise(m_dwEventCookie);
                cp->Release();
            }
            cpc->Release();
        }
        m_dwEventCookie = 0;
    }
}

HRESULT CFramerControl::Open(LPCWSTR file, BOOL readOnly)
{
    VARIANT vDoc, vRO, vOpt;

    VariantInit(&vDoc); VariantInit(&vRO); VariantInit(&vOpt);
    vDoc.vt = VT_BSTR;
    vDoc.bstrVal = SysAllocString(file);
    if (!vDoc.bstrVal) return E_OUTOFMEMORY;
    vRO.vt = VT_BOOL;
    vRO.boolVal = readOnly ? -1 : 0;
    vOpt.vt = VT_ERROR;
    vOpt.scode = DISP_E_PARAMNOTFOUND;
    HRESULT hr = m_ctl->Open(vDoc, vRO, vOpt, vOpt, vOpt);
    VariantClear(&vDoc);
    return hr;
}

HRESULT CFramerControl::CreateNew(LPCWSTR progIdOrTemplate)
{
    BSTR b = SysAllocString(progIdOrTemplate);
    if (!b) return E_OUTOFMEMORY;
    HRESULT hr = m_ctl->CreateNew(b);
    SysFreeString(b);
    return hr;
}

HRESULT CFramerControl::SaveAs(LPCWSTR file, BOOL overwrite)
{
    VARIANT vDoc, vOver, vOpt;
    VariantInit(&vDoc); VariantInit(&vOver); VariantInit(&vOpt);
    vDoc.vt = VT_BSTR;
    vDoc.bstrVal = SysAllocString(file);
    if (!vDoc.bstrVal) return E_OUTOFMEMORY;
    vOver.vt = VT_BOOL;
    vOver.boolVal = overwrite ? -1 : 0;
    vOpt.vt = VT_ERROR;
    vOpt.scode = DISP_E_PARAMNOTFOUND;
    HRESULT hr = m_ctl->Save(vDoc, vOver, vOpt, vOpt);
    VariantClear(&vDoc);
    return hr;
}

HRESULT CFramerControl::put_HostName(LPCWSTR name)
{
    BSTR b = SysAllocString(name);
    if (!b) return E_OUTOFMEMORY;
    HRESULT hr = m_ctl->put_HostName(b);
    SysFreeString(b);
    return hr;
}

BOOL CFramerControl::IsDirty(void)
{
    VARIANT_BOOL v = 0;
    if (FAILED(m_ctl->get_IsDirty(&v))) return FALSE;
    return v != 0;
}

BOOL CFramerControl::IsReadOnly(void)
{
    VARIANT_BOOL v = 0;
    if (FAILED(m_ctl->get_IsReadOnly(&v))) return FALSE;
    return v != 0;
}

BOOL CFramerControl::GetDocumentFullName(WCHAR *buf, DWORD cch)
{
    BSTR b = NULL;
    if (!buf || cch == 0) return FALSE;
    buf[0] = L'\0';
    if (FAILED(m_ctl->get_DocumentFullName(&b)))
        return FALSE;
    if (b) {
        wcsncpy(buf, b, cch - 1);
        buf[cch - 1] = L'\0';
        SysFreeString(b);
    }
    return TRUE;
}
