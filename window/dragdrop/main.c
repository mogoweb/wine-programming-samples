#include <windows.h>
#include <ole2.h>
#include <shobjidl.h>
#include <stdio.h>

// ============================================
// Drag Source Window
// ============================================

// IDataObject implementation
typedef struct {
    IDataObjectVtbl *lpVtbl;
    LONG refCount;
    char *textData;
} MyDataObject;

// IEnumFORMATETC - enumerate supported formats
typedef struct {
    IEnumFORMATETCVtbl *lpVtbl;
    LONG refCount;
    int index;
} MyEnumFORMATETC;

// Forward declaration
static MyDataObject *CreateDataObject(const char *text);

// IEnumFORMATETC implementation
static HRESULT STDMETHODCALLTYPE EnumFORMATETC_QueryInterface(
    IEnumFORMATETC *This, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IEnumFORMATETC) || IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = This;
        This->lpVtbl->AddRef(This);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE EnumFORMATETC_AddRef(IEnumFORMATETC *This)
{
    MyEnumFORMATETC *self = (MyEnumFORMATETC *)This;
    return InterlockedIncrement(&self->refCount);
}

static ULONG STDMETHODCALLTYPE EnumFORMATETC_Release(IEnumFORMATETC *This)
{
    MyEnumFORMATETC *self = (MyEnumFORMATETC *)This;
    LONG count = InterlockedDecrement(&self->refCount);
    if (count == 0) {
        free(self);
    }
    return count;
}

static HRESULT STDMETHODCALLTYPE EnumFORMATETC_Next(
    IEnumFORMATETC *This, ULONG celt, FORMATETC *rgelt, ULONG *pceltFetched)
{
    MyEnumFORMATETC *self = (MyEnumFORMATETC *)This;
    static FORMATETC formats[] = {
        { CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        { CF_TEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
    };
    int formatCount = 2;

    if (pceltFetched) *pceltFetched = 0;

    if (self->index >= formatCount) return S_FALSE;

    int copied = 0;
    for (ULONG i = 0; i < celt && self->index < formatCount; i++) {
        rgelt[i] = formats[self->index++];
        copied++;
    }

    if (pceltFetched) *pceltFetched = copied;
    return (copied == (int)celt) ? S_OK : S_FALSE;
}

static HRESULT STDMETHODCALLTYPE EnumFORMATETC_Skip(IEnumFORMATETC *This, ULONG celt)
{
    MyEnumFORMATETC *self = (MyEnumFORMATETC *)This;
    self->index += celt;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE EnumFORMATETC_Reset(IEnumFORMATETC *This)
{
    MyEnumFORMATETC *self = (MyEnumFORMATETC *)This;
    self->index = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE EnumFORMATETC_Clone(
    IEnumFORMATETC *This, IEnumFORMATETC **ppEnum)
{
    MyEnumFORMATETC *self = (MyEnumFORMATETC *)This;
    MyEnumFORMATETC *clone = (MyEnumFORMATETC *)malloc(sizeof(MyEnumFORMATETC));
    if (!clone) return E_OUTOFMEMORY;

    static IEnumFORMATETCVtbl vtbl = {
        EnumFORMATETC_QueryInterface,
        EnumFORMATETC_AddRef,
        EnumFORMATETC_Release,
        EnumFORMATETC_Next,
        EnumFORMATETC_Skip,
        EnumFORMATETC_Reset,
        EnumFORMATETC_Clone,
    };

    clone->lpVtbl = &vtbl;
    clone->refCount = 1;
    clone->index = self->index;
    *ppEnum = (IEnumFORMATETC *)clone;
    return S_OK;
}

static IEnumFORMATETCVtbl g_EnumFORMATETCVtbl = {
    EnumFORMATETC_QueryInterface,
    EnumFORMATETC_AddRef,
    EnumFORMATETC_Release,
    EnumFORMATETC_Next,
    EnumFORMATETC_Skip,
    EnumFORMATETC_Reset,
    EnumFORMATETC_Clone,
};

// IDataObject implementation
static HRESULT STDMETHODCALLTYPE DataObject_QueryInterface(
    IDataObject *This, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IDataObject) || IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = This;
        This->lpVtbl->AddRef(This);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE DataObject_AddRef(IDataObject *This)
{
    MyDataObject *self = (MyDataObject *)This;
    return InterlockedIncrement(&self->refCount);
}

static ULONG STDMETHODCALLTYPE DataObject_Release(IDataObject *This)
{
    MyDataObject *self = (MyDataObject *)This;
    LONG count = InterlockedDecrement(&self->refCount);
    if (count == 0) {
        if (self->textData) free(self->textData);
        free(self);
    }
    return count;
}

static HRESULT STDMETHODCALLTYPE DataObject_GetData(
    IDataObject *This, FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
{
    MyDataObject *self = (MyDataObject *)This;

    if (pformatetcIn->cfFormat == CF_UNICODETEXT &&
        (pformatetcIn->tymed & TYMED_HGLOBAL)) {
        int len = MultiByteToWideChar(CP_UTF8, 0, self->textData, -1, NULL, 0);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(WCHAR));
        if (!hMem) return E_OUTOFMEMORY;

        WCHAR *ptr = (WCHAR *)GlobalLock(hMem);
        MultiByteToWideChar(CP_UTF8, 0, self->textData, -1, ptr, len);
        GlobalUnlock(hMem);

        pmedium->tymed = TYMED_HGLOBAL;
        pmedium->hGlobal = hMem;
        pmedium->pUnkForRelease = NULL;
        return S_OK;
    }

    if (pformatetcIn->cfFormat == CF_TEXT &&
        (pformatetcIn->tymed & TYMED_HGLOBAL)) {
        size_t len = strlen(self->textData) + 1;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
        if (!hMem) return E_OUTOFMEMORY;

        char *ptr = (char *)GlobalLock(hMem);
        memcpy(ptr, self->textData, len);
        GlobalUnlock(hMem);

        pmedium->tymed = TYMED_HGLOBAL;
        pmedium->hGlobal = hMem;
        pmedium->pUnkForRelease = NULL;
        return S_OK;
    }

    return DV_E_FORMATETC;
}

static HRESULT STDMETHODCALLTYPE DataObject_GetDataHere(
    IDataObject *This, FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    (void)This; (void)pformatetc; (void)pmedium;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE DataObject_QueryGetData(
    IDataObject *This, FORMATETC *pformatetc)
{
    (void)This;
    if (pformatetc->cfFormat == CF_UNICODETEXT || pformatetc->cfFormat == CF_TEXT) {
        if (pformatetc->tymed & TYMED_HGLOBAL) {
            return S_OK;
        }
    }
    return DV_E_FORMATETC;
}

static HRESULT STDMETHODCALLTYPE DataObject_GetCanonicalFormatEtc(
    IDataObject *This, FORMATETC *pformatectIn, FORMATETC *pformatetcOut)
{
    (void)This; (void)pformatectIn;
    pformatetcOut->ptd = NULL;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE DataObject_SetData(
    IDataObject *This, FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    (void)This; (void)pformatetc; (void)pmedium; (void)fRelease;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE DataObject_EnumFormatEtc(
    IDataObject *This, DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
{
    (void)This;
    if (dwDirection != DATADIR_GET) {
        *ppenumFormatEtc = NULL;
        return E_NOTIMPL;
    }

    MyEnumFORMATETC *enumerator = (MyEnumFORMATETC *)malloc(sizeof(MyEnumFORMATETC));
    if (!enumerator) return E_OUTOFMEMORY;

    enumerator->lpVtbl = &g_EnumFORMATETCVtbl;
    enumerator->refCount = 1;
    enumerator->index = 0;
    *ppenumFormatEtc = (IEnumFORMATETC *)enumerator;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DataObject_DAdvise(
    IDataObject *This, FORMATETC *pformatetc, DWORD advf,
    IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    (void)This; (void)pformatetc; (void)advf; (void)pAdvSink; (void)pdwConnection;
    return OLE_E_ADVISENOTSUPPORTED;
}

static HRESULT STDMETHODCALLTYPE DataObject_DUnadvise(
    IDataObject *This, DWORD dwConnection)
{
    (void)This; (void)dwConnection;
    return OLE_E_ADVISENOTSUPPORTED;
}

static HRESULT STDMETHODCALLTYPE DataObject_EnumDAdvise(
    IDataObject *This, IEnumSTATDATA **ppenumAdvise)
{
    (void)This; (void)ppenumAdvise;
    return OLE_E_ADVISENOTSUPPORTED;
}

static IDataObjectVtbl g_DataObjectVtbl = {
    DataObject_QueryInterface,
    DataObject_AddRef,
    DataObject_Release,
    DataObject_GetData,
    DataObject_GetDataHere,
    DataObject_QueryGetData,
    DataObject_GetCanonicalFormatEtc,
    DataObject_SetData,
    DataObject_EnumFormatEtc,
    DataObject_DAdvise,
    DataObject_DUnadvise,
    DataObject_EnumDAdvise,
};

static MyDataObject *CreateDataObject(const char *text)
{
    MyDataObject *obj = (MyDataObject *)malloc(sizeof(MyDataObject));
    if (!obj) return NULL;

    obj->lpVtbl = &g_DataObjectVtbl;
    obj->refCount = 1;
    obj->textData = strdup(text);
    return obj;
}

// IDropSource implementation
typedef struct {
    IDropSourceVtbl *lpVtbl;
    LONG refCount;
} MyDropSource;

static HRESULT STDMETHODCALLTYPE DropSource_QueryInterface(
    IDropSource *This, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IDropSource) || IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = This;
        This->lpVtbl->AddRef(This);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE DropSource_AddRef(IDropSource *This)
{
    MyDropSource *self = (MyDropSource *)This;
    return InterlockedIncrement(&self->refCount);
}

static ULONG STDMETHODCALLTYPE DropSource_Release(IDropSource *This)
{
    MyDropSource *self = (MyDropSource *)This;
    LONG count = InterlockedDecrement(&self->refCount);
    if (count == 0) {
        free(self);
    }
    return count;
}

static HRESULT STDMETHODCALLTYPE DropSource_QueryContinueDrag(
    IDropSource *This, BOOL fEscapePressed, DWORD grfKeyState)
{
    (void)This;
    if (fEscapePressed) return DRAGDROP_S_CANCEL;
    if (!(grfKeyState & MK_LBUTTON)) return DRAGDROP_S_DROP;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DropSource_GiveFeedback(
    IDropSource *This, DWORD dwEffect)
{
    (void)This; (void)dwEffect;
    return DRAGDROP_S_USEDEFAULTCURSORS;
}

static IDropSourceVtbl g_DropSourceVtbl = {
    DropSource_QueryInterface,
    DropSource_AddRef,
    DropSource_Release,
    DropSource_QueryContinueDrag,
    DropSource_GiveFeedback,
};

static IDropSource *CreateDropSource(void)
{
    MyDropSource *src = (MyDropSource *)malloc(sizeof(MyDropSource));
    if (!src) return NULL;

    src->lpVtbl = &g_DropSourceVtbl;
    src->refCount = 1;
    return (IDropSource *)src;
}

// Drag source window procedure
static char g_dragText[] = "Hello from the drag source! This is a drag-and-drop test message.";

// Helper to check if point is in drag source area
static BOOL IsInDragArea(HWND hwnd, int x, int y)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    RECT dragRect = { 20, 90, rc.right - 20, 140 };
    return (x >= dragRect.left && x <= dragRect.right &&
            y >= dragRect.top && y <= dragRect.bottom);
}

LRESULT CALLBACK SourceWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static BOOL s_dragging = FALSE;
    static POINT s_dragStart;
    static BOOL s_inDragArea = FALSE;

    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rc;
            GetClientRect(hwnd, &rc);

            HBRUSH bgBrush = CreateSolidBrush(RGB(200, 220, 255));
            FillRect(hdc, &rc, bgBrush);
            DeleteObject(bgBrush);

            HFONT hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, NULL);
            SelectObject(hdc, hFont);
            SetBkMode(hdc, TRANSPARENT);

            const char *title = "Drag Source Window";
            const char *hint = "Drag from yellow area to Target Window";

            TextOutA(hdc, 20, 20, title, (int)strlen(title));
            TextOutA(hdc, 20, 50, hint, (int)strlen(hint));

            // Draggable area (yellow)
            RECT dragRect = { 20, 90, rc.right - 20, 140 };
            HBRUSH dragBrush = CreateSolidBrush(RGB(255, 255, 200));
            FillRect(hdc, &dragRect, dragBrush);
            DeleteObject(dragBrush);

            // Draw border for drag area
            HPEN hPen = CreatePen(PS_SOLID, 2, RGB(200, 150, 0));
            SelectObject(hdc, hPen);
            Rectangle(hdc, dragRect.left, dragRect.top, dragRect.right, dragRect.bottom);
            DeleteObject(hPen);

            HFONT hFontBig = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, NULL);
            SelectObject(hdc, hFontBig);
            TextOutA(hdc, 30, 105, g_dragText, (int)strlen(g_dragText));
            DeleteObject(hFontBig);

            // Non-draggable area hint
            HFONT hFontSmall = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, NULL);
            SelectObject(hdc, hFontSmall);
            SetTextColor(hdc, RGB(100, 100, 100));
            const char *noDrag = "Outside yellow area: no drag";
            TextOutA(hdc, 20, 160, noDrag, (int)strlen(noDrag));
            DeleteObject(hFontSmall);

            DeleteObject(hFont);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            // Only start drag if click is in the draggable area
            if (IsInDragArea(hwnd, x, y)) {
                s_dragging = TRUE;
                s_inDragArea = TRUE;
                s_dragStart.x = x;
                s_dragStart.y = y;
                SetCapture(hwnd);
            } else {
                s_inDragArea = FALSE;
            }
            return 0;
        }

        case WM_MOUSEMOVE: {
            if (s_dragging && s_inDragArea) {
                int dx = abs((int)(LOWORD(lParam) - s_dragStart.x));
                int dy = abs((int)(HIWORD(lParam) - s_dragStart.y));

                // Start drag when moved beyond threshold
                if (dx > 5 || dy > 5) {
                    ReleaseCapture();
                    s_dragging = FALSE;

                    // Start OLE drag operation
                    IDataObject *pDataObj = (IDataObject *)CreateDataObject(g_dragText);
                    IDropSource *pDropSource = CreateDropSource();

                    if (pDataObj && pDropSource) {
                        DWORD dwEffect;
                        DoDragDrop(pDataObj, pDropSource, DROPEFFECT_COPY, &dwEffect);
                    }

                    if (pDataObj) pDataObj->lpVtbl->Release(pDataObj);
                    if (pDropSource) pDropSource->lpVtbl->Release(pDropSource);
                }
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            if (s_dragging) {
                ReleaseCapture();
                s_dragging = FALSE;
            }
            return 0;
        }

        case WM_SETCURSOR: {
            // Change cursor when over draggable area
            if (LOWORD(lParam) == HTCLIENT) {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hwnd, &pt);
                if (IsInDragArea(hwnd, pt.x, pt.y)) {
                    SetCursor(LoadCursor(NULL, IDC_HAND));
                    return TRUE;
                }
            }
            break;
        }

        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ============================================
// Drop Target Window
// ============================================

#define MAX_DROP_HISTORY 10
#define MAX_TEXT_LENGTH 256

// IDropTarget implementation
typedef struct {
    IDropTargetVtbl *lpVtbl;
    LONG refCount;
    HWND hwndTarget;
    char dropHistory[MAX_DROP_HISTORY][MAX_TEXT_LENGTH];
    int dropCount;
    BOOL isOverDropArea;
} MyDropTarget;

// Helper to check if point is in drop target area
static BOOL IsInDropArea(HWND hwnd, POINTL pt)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    POINT clientPt = { pt.x, pt.y };
    ScreenToClient(hwnd, &clientPt);

    RECT dropRect = { 20, 100, rc.right - 20, rc.bottom - 20 };
    return (clientPt.x >= dropRect.left && clientPt.x <= dropRect.right &&
            clientPt.y >= dropRect.top && clientPt.y <= dropRect.bottom);
}

static void GetDropAreaRect(HWND hwnd, RECT *dropRect)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    dropRect->left = 20;
    dropRect->top = 100;
    dropRect->right = rc.right - 20;
    dropRect->bottom = rc.bottom - 20;
}

static HRESULT STDMETHODCALLTYPE DropTarget_QueryInterface(
    IDropTarget *This, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IDropTarget) || IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = This;
        This->lpVtbl->AddRef(This);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE DropTarget_AddRef(IDropTarget *This)
{
    MyDropTarget *self = (MyDropTarget *)This;
    return InterlockedIncrement(&self->refCount);
}

static ULONG STDMETHODCALLTYPE DropTarget_Release(IDropTarget *This)
{
    MyDropTarget *self = (MyDropTarget *)This;
    LONG count = InterlockedDecrement(&self->refCount);
    if (count == 0) {
        free(self);
    }
    return count;
}

static HRESULT STDMETHODCALLTYPE DropTarget_DragEnter(
    IDropTarget *This, IDataObject *pDataObj, DWORD grfKeyState,
    POINTL pt, DWORD *pdwEffect)
{
    (void)grfKeyState;
    MyDropTarget *self = (MyDropTarget *)This;

    // Check if over drop area
    self->isOverDropArea = IsInDropArea(self->hwndTarget, pt);

    if (self->isOverDropArea) {
        // Check if text format is supported
        FORMATETC fmt = { CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        if (pDataObj->lpVtbl->QueryGetData(pDataObj, &fmt) == S_OK) {
            *pdwEffect = DROPEFFECT_COPY;
        } else {
            fmt.cfFormat = CF_TEXT;
            if (pDataObj->lpVtbl->QueryGetData(pDataObj, &fmt) == S_OK) {
                *pdwEffect = DROPEFFECT_COPY;
            } else {
                *pdwEffect = DROPEFFECT_NONE;
            }
        }
    } else {
        *pdwEffect = DROPEFFECT_NONE;
    }

    InvalidateRect(self->hwndTarget, NULL, TRUE);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DropTarget_DragOver(
    IDropTarget *This, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    (void)grfKeyState;
    MyDropTarget *self = (MyDropTarget *)This;

    // Check if over drop area
    self->isOverDropArea = IsInDropArea(self->hwndTarget, pt);

    if (self->isOverDropArea) {
        *pdwEffect = DROPEFFECT_COPY;
    } else {
        *pdwEffect = DROPEFFECT_NONE;
    }

    InvalidateRect(self->hwndTarget, NULL, TRUE);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DropTarget_DragLeave(IDropTarget *This)
{
    MyDropTarget *self = (MyDropTarget *)This;
    self->isOverDropArea = FALSE;
    InvalidateRect(self->hwndTarget, NULL, TRUE);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DropTarget_Drop(
    IDropTarget *This, IDataObject *pDataObj, DWORD grfKeyState,
    POINTL pt, DWORD *pdwEffect)
{
    (void)grfKeyState;
    MyDropTarget *self = (MyDropTarget *)This;

    // Check if dropped in the drop area
    if (!IsInDropArea(self->hwndTarget, pt)) {
        *pdwEffect = DROPEFFECT_NONE;
        InvalidateRect(self->hwndTarget, NULL, TRUE);
        return S_OK;
    }

    char tempText[MAX_TEXT_LENGTH] = {0};
    STGMEDIUM stg;
    FORMATETC fmt = { CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    BOOL gotData = FALSE;
    if (pDataObj->lpVtbl->GetData(pDataObj, &fmt, &stg) == S_OK) {
        WCHAR *ptr = (WCHAR *)GlobalLock(stg.hGlobal);
        if (ptr) {
            WideCharToMultiByte(CP_UTF8, 0, ptr, -1,
                tempText, sizeof(tempText), NULL, NULL);
            GlobalUnlock(stg.hGlobal);
        }
        ReleaseStgMedium(&stg);
        gotData = TRUE;
    } else {
        fmt.cfFormat = CF_TEXT;
        if (pDataObj->lpVtbl->GetData(pDataObj, &fmt, &stg) == S_OK) {
            char *ptr = (char *)GlobalLock(stg.hGlobal);
            if (ptr) {
                strncpy(tempText, ptr, sizeof(tempText) - 1);
                tempText[sizeof(tempText) - 1] = '\0';
                GlobalUnlock(stg.hGlobal);
            }
            ReleaseStgMedium(&stg);
            gotData = TRUE;
        }
    }

    if (gotData && tempText[0]) {
        // Shift history if full
        if (self->dropCount >= MAX_DROP_HISTORY) {
            for (int i = 0; i < MAX_DROP_HISTORY - 1; i++) {
                strcpy(self->dropHistory[i], self->dropHistory[i + 1]);
            }
            self->dropCount = MAX_DROP_HISTORY - 1;
        }

        // Add new entry
        strcpy(self->dropHistory[self->dropCount], tempText);
        self->dropCount++;

        *pdwEffect = DROPEFFECT_COPY;
    } else {
        *pdwEffect = DROPEFFECT_NONE;
    }

    self->isOverDropArea = FALSE;
    InvalidateRect(self->hwndTarget, NULL, TRUE);
    return S_OK;
}

static IDropTargetVtbl g_DropTargetVtbl = {
    DropTarget_QueryInterface,
    DropTarget_AddRef,
    DropTarget_Release,
    DropTarget_DragEnter,
    DropTarget_DragOver,
    DropTarget_DragLeave,
    DropTarget_Drop,
};

static IDropTarget *CreateDropTarget(HWND hwnd)
{
    MyDropTarget *target = (MyDropTarget *)malloc(sizeof(MyDropTarget));
    if (!target) return NULL;

    target->lpVtbl = &g_DropTargetVtbl;
    target->refCount = 1;
    target->hwndTarget = hwnd;
    target->dropCount = 0;
    target->isOverDropArea = FALSE;
    for (int i = 0; i < MAX_DROP_HISTORY; i++) {
        target->dropHistory[i][0] = '\0';
    }
    return (IDropTarget *)target;
}

#define IDC_CLEAR_BUTTON 1001

// Drop target window procedure
LRESULT CALLBACK TargetWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static IDropTarget *s_dropTarget = NULL;

    switch (msg) {
        case WM_CREATE:
            s_dropTarget = CreateDropTarget(hwnd);
            if (s_dropTarget) {
                RegisterDragDrop(hwnd, s_dropTarget);
            }
            // Create clear button
            CreateWindowA("BUTTON", "Clear History",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                20, 60, 120, 28,
                hwnd, (HMENU)IDC_CLEAR_BUTTON,
                ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_CLEAR_BUTTON) {
                MyDropTarget *target = (MyDropTarget *)s_dropTarget;
                if (target) {
                    target->dropCount = 0;
                    for (int i = 0; i < MAX_DROP_HISTORY; i++) {
                        target->dropHistory[i][0] = '\0';
                    }
                    InvalidateRect(hwnd, NULL, TRUE);
                }
            }
            return 0;

        case WM_DESTROY:
            if (s_dropTarget) {
                RevokeDragDrop(hwnd);
                s_dropTarget->lpVtbl->Release(s_dropTarget);
                s_dropTarget = NULL;
            }
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rc;
            GetClientRect(hwnd, &rc);

            // Background
            HBRUSH bgBrush = CreateSolidBrush(RGB(220, 255, 220));
            FillRect(hdc, &rc, bgBrush);
            DeleteObject(bgBrush);

            HFONT hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, NULL);
            SelectObject(hdc, hFont);
            SetBkMode(hdc, TRANSPARENT);

            const char *title = "Drop Target Window";
            TextOutA(hdc, 20, 20, title, (int)strlen(title));

            HFONT hFontSmall = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, NULL);
            SelectObject(hdc, hFontSmall);
            SetTextColor(hdc, RGB(100, 100, 100));
            const char *hint = "Drop only in white area below";
            TextOutA(hdc, 20, 45, hint, (int)strlen(hint));

            // Drop area (white)
            RECT dropRect;
            GetDropAreaRect(hwnd, &dropRect);

            // Highlight if dragging over drop area
            MyDropTarget *target = (MyDropTarget *)s_dropTarget;
            COLORREF dropAreaColor = target && target->isOverDropArea ?
                RGB(200, 255, 200) : RGB(255, 255, 255);
            HBRUSH dropBrush = CreateSolidBrush(dropAreaColor);
            FillRect(hdc, &dropRect, dropBrush);
            DeleteObject(dropBrush);

            // Draw border for drop area
            HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 150, 0));
            SelectObject(hdc, hPen);
            Rectangle(hdc, dropRect.left, dropRect.top, dropRect.right, dropRect.bottom);
            DeleteObject(hPen);

            // Show drop history
            if (target && target->dropCount > 0) {
                HFONT hFontHist = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, NULL);
                SelectObject(hdc, hFontHist);
                SetTextColor(hdc, RGB(0, 0, 0));

                char line[300];
                int y = dropRect.top + 10;
                for (int i = 0; i < target->dropCount && y < dropRect.bottom - 20; i++) {
                    snprintf(line, sizeof(line), "[%d] %s", i + 1, target->dropHistory[i]);
                    TextOutA(hdc, dropRect.left + 10, y, line, (int)strlen(line));
                    y += 22;
                }

                DeleteObject(hFontHist);
            } else {
                HFONT hFontGray = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, NULL);
                SelectObject(hdc, hFontGray);
                SetTextColor(hdc, RGB(128, 128, 128));

                const char *empty = "< Drop text here >";
                TextOutA(hdc, dropRect.left + 50, dropRect.top + 30, empty, (int)strlen(empty));

                DeleteObject(hFontGray);
            }

            // Non-drop area hint
            SelectObject(hdc, hFontSmall);
            SetTextColor(hdc, RGB(180, 180, 180));
            const char *noDrop = "Outside white area: no drop";
            TextOutA(hdc, rc.right - 180, 20, noDrop, (int)strlen(noDrop));

            DeleteObject(hFontSmall);
            DeleteObject(hFont);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ============================================
// Main Program
// ============================================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance; (void)lpCmdLine;

    // Initialize OLE
    OleInitialize(NULL);

    // Register window classes
    WNDCLASSA wcSource = {0};
    wcSource.lpfnWndProc = SourceWndProc;
    wcSource.hInstance = hInstance;
    wcSource.lpszClassName = "DragSourceClass";
    wcSource.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcSource.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassA(&wcSource);

    WNDCLASSA wcTarget = {0};
    wcTarget.lpfnWndProc = TargetWndProc;
    wcTarget.hInstance = hInstance;
    wcTarget.lpszClassName = "DropTargetClass";
    wcTarget.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcTarget.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassA(&wcTarget);

    // Create source window (left)
    HWND hwndSource = CreateWindowA(
        "DragSourceClass", "Drag Source",
        WS_OVERLAPPEDWINDOW,
        100, 100, 550, 250,
        NULL, NULL, hInstance, NULL
    );

    // Create target window (right)
    HWND hwndTarget = CreateWindowA(
        "DropTargetClass", "Drop Target",
        WS_OVERLAPPEDWINDOW,
        670, 100, 500, 350,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwndSource, nCmdShow);
    UpdateWindow(hwndSource);
    ShowWindow(hwndTarget, nCmdShow);
    UpdateWindow(hwndTarget);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup OLE
    OleUninitialize();

    return (int)msg.wParam;
}
