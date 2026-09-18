/*
 * framerhost.h - ActiveX container plumbing for hosting DsoFramer's
 * FramerControl with plain Win32 API (no MFC/ATL).
 */
#ifndef FRAMERHOST_H
#define FRAMERHOST_H

#include "dsoframer.h"

/* Host-side event callbacks (invoked from the control's event sink) */
struct FramerEventHandlers
{
    /* OnFileCommand(Item, Cancel*): return TRUE to cancel default action */
    BOOL (*OnFileCommand)(dsoFileCommandType item, BOOL cancelDefault);
    void (*OnDocumentOpened)(LPCWSTR file, IDispatch *document);
    void (*OnDocumentClosed)(void);
    /* BeforeDocumentClosed(Document, Cancel*): return TRUE to cancel close */
    BOOL (*BeforeDocumentClosed)(IDispatch *document);
    void (*OnPrintPreviewExit)(void);
};

/* Wrapper around the FramerControl OCX */
class CFramerControl
{
public:
    CFramerControl();
    ~CFramerControl();

    /* Create the OCX and activate it in-place inside hwndParent.
       Returns S_OK on success. */
    HRESULT Create(HWND hwndParent, const FramerEventHandlers *handlers);

    HWND Window() const { return m_hwndCtl; }
    /* Event sink access (used by CFramerEventSink) */
    FramerEventHandlers &Handlers() { return m_handlers; }

    /* Document operations */
    HRESULT Open(LPCWSTR file, BOOL readOnly);
    HRESULT CreateNew(LPCWSTR progIdOrTemplate);
    HRESULT Save(void) { return m_ctl->Save(optV(), optV(), optV(), optV()); }
    HRESULT SaveAs(LPCWSTR file, BOOL overwrite);
    HRESULT Close(void) { return m_ctl->Close(); }
    HRESULT ShowDialog(dsoShowDialogType dlg) { return m_ctl->ShowDialog(dlg); }
    HRESULT PrintPreview(void) { return m_ctl->PrintPreview(); }
    HRESULT Activate(void) { return m_ctl->Activate(); }

    /* Properties */
    HRESULT put_HostName(LPCWSTR name);
    HRESULT put_Titlebar(BOOL b)   { return m_ctl->put_Titlebar(b ? -1 : 0); }
    HRESULT put_Toolbars(BOOL b)   { return m_ctl->put_Toolbars(b ? -1 : 0); }
    HRESULT put_Menubar(BOOL b)    { return m_ctl->put_Menubar(b ? -1 : 0); }
    HRESULT put_BorderStyle(dsoBorderStyle s) { return m_ctl->put_BorderStyle(s); }
    HRESULT put_Caption(LPCWSTR cap) { BSTR b = SysAllocString(cap); HRESULT hr = m_ctl->put_Caption(b); SysFreeString(b); return hr; }
    HRESULT put_EnableFileCommand(dsoFileCommandType item, BOOL enable) { return m_ctl->put_EnableFileCommand(item, enable ? -1 : 0); }

    HRESULT get_Titlebar(BOOL *b)  { VARIANT_BOOL v; HRESULT hr = m_ctl->get_Titlebar(&v); if (SUCCEEDED(hr)) *b = (v != 0); return hr; }
    HRESULT get_Toolbars(BOOL *b)  { VARIANT_BOOL v; HRESULT hr = m_ctl->get_Toolbars(&v); if (SUCCEEDED(hr)) *b = (v != 0); return hr; }
    HRESULT get_Menubar(BOOL *b)   { VARIANT_BOOL v; HRESULT hr = m_ctl->get_Menubar(&v); if (SUCCEEDED(hr)) *b = (v != 0); return hr; }
    HRESULT get_BorderStyle(dsoBorderStyle *s) { return m_ctl->get_BorderStyle(s); }
    /* IsDirty/IsReadOnly fail with DSO_E_DOCUMENTNOTOPEN when no doc; treat as FALSE */
    BOOL IsDirty(void);
    BOOL IsReadOnly(void);
    /* DocumentFullName; returns empty string when no doc open */
    BOOL GetDocumentFullName(WCHAR *buf, DWORD cch);

    /* Standard OCX container resize protocol (this is what VB6/VB forms
       do on oFramer.Move): IOleObject::SetExtent syncs the control's
       internal size (m_Size, used by its WM_SIZE layout math), and
       IOleInPlaceObject::SetObjectRects repositions the in-place window.
       Resizing the OCX child window directly leaves m_Size stale and the
       embedded document never follows. */
    void SetRects(const RECT *rc);

private:
    static VARIANT optV()
    {
        VARIANT v;
        v.vt = VT_ERROR;
        v.scode = DISP_E_PARAMNOTFOUND;
        return v;
    }
    HRESULT LoadDefaultProperties(void);
    HRESULT AdviseEvents(void);
    void UnadviseEvents(void);

    static HRESULT WINAPI SiteQI(void *siteUser, REFIID riid, void **ppv);
    friend class CFramerSite;

    __FramerControl *m_ctl;
    IUnknown        *m_unkCtl;
    IOleObject      *m_oleObj;
    HWND             m_hwndCtl;
    HWND             m_hwndParent;
    DWORD            m_dwEventCookie;
    LONG             m_refSite;
    FramerEventHandlers m_handlers;
};

#endif /* FRAMERHOST_H */
