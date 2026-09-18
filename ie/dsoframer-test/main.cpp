/*
 * main.cpp - C++ / pure Win32 API port of the DsoFramer Vb6Test sample
 * (ie/DsoFramer/Samples/Vb6Test/Src/FTestApp.frm).
 *
 * Hosts the DSOFramer.FramerControl OCX in a plain Win32 window and
 * reproduces the VB6 UI: title bar, "Current File" status line, and the
 * File/Show menus (minimal core subset: no Web Open, no Save-to-Web,
 * no custom printer dialog).
 *
 * Build:  make          (i686-w64-mingw32 cross compiler)
 * Run:    make run      (uses ~/.deepinwine/org.deepin-wine.browser.deepin)
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ole2.h>
#include <commdlg.h>
#include <stdio.h>
#include <wchar.h>
#include <string>

#include "resource.h"
#include "framerhost.h"

/* ===== VB6 form layout constants (FTestApp.frm, pixel scale mode) ===== */
#define FRAMER_X        10
#define FRAMER_Y        52
#define STATUS_Y        24
#define STATUS_LINE_Y   48

static const WCHAR APP_CLASS[]  = L"FramerCppTestApp";
static const WCHAR APP_TITLE[]  = L"C++ Test Application for DsoFramer Control";
static const WCHAR HOST_NAME[]  = L"CppTestApp";
static const WCHAR STATUS_NONE[]      = L"Current File: [None]";
static const WCHAR STATUS_UNSAVED[]   = L"Current File: Unsaved Document";

/* ===== Globals ===== */
static HWND   g_hwndMain   = NULL;
static HWND   g_hwndStatus = NULL;      /* static text: "Current File: ..." */
static HFONT  g_hFont      = NULL;
static CFramerControl *g_framer = NULL;

/* Document open state mirrors the VB EnableItems() logic */
static BOOL g_fDocOpen = FALSE;
static BOOL g_fInPreview = FALSE;

/* File command enable bits, mirrors mnuDisableItem checked state */
static BOOL g_fDisableItem[9] = {0};

/* Automation from command line: "new:Word.Document" / "open:C:\\doc.doc" */
static WCHAR g_szAutoCmd[512] = L"";

/* =====================================================================
 * Helpers
 * ===================================================================== */
static void UpdateLayout(void)
{
    RECT rc;
    GetClientRect(g_hwndMain, &rc);

    MoveWindow(g_hwndStatus, FRAMER_X + 80, STATUS_Y - 8,
               (rc.right - rc.left) - 190, 24, TRUE);

    if (g_framer && g_framer->Window()) {
        /* oFramer.Move 10, 52, ScaleWidth-20, ScaleHeight-60.
           Container resize protocol (same as VB forms engine): SetExtent
           + SetObjectRects. Moving the OCX window directly would leave
           the control's internal size state stale. */
        RECT rcCtl = { FRAMER_X, FRAMER_Y,
                       (rc.right - rc.left) - 2 * FRAMER_X,
                       (rc.bottom - rc.top) - 60 };
        g_framer->SetRects(&rcCtl);
    }
}

static void SetStatusText(LPCWSTR text)
{
    SetWindowTextW(g_hwndStatus, text);
}

/* VB EnableItems(fEnable) - enable/disable doc-dependent menu items */
static void EnableItems(HMENU hmenu, BOOL fEnable)
{
    static const int ids[] = {
        IDM_FILE_CLOSE, IDM_FILE_SAVE, IDM_FILE_SAVEAS,
        IDM_FILE_PAGESETUP, IDM_FILE_PRINTPREVIEW, IDM_FILE_PRINT,
        IDM_FILE_PROPERTIES,
    };
    for (size_t i = 0; i < sizeof(ids)/sizeof(ids[0]); i++)
        EnableMenuItem(hmenu, ids[i], fEnable ? MF_ENABLED : MF_GRAYED);
}

/* =====================================================================
 * Framer event handlers (VB oFramer_* event subs)
 * ===================================================================== */

/* oFramer_OnFileCommand: we intercept Open/SaveAs and run our own
 * dialogs (ShowDialog dsoDialogOpen / dsoDialogSave), cancelling the
 * control's default action. */
static BOOL WINAPI_OnFileCommand(dsoFileCommandType item, BOOL /*cancelDefault*/)
{
    printf("[event] OnFileCommand item=%d\n", item); fflush(stdout);
    if (item == dsoFileOpen || item == dsoFileSaveAs) {
        dsoShowDialogType dlg = (item == dsoFileOpen) ? dsoDialogOpen : dsoDialogSave;
        HRESULT hr = g_framer->ShowDialog(dlg);
        if (FAILED(hr) && hr != E_ABORT) {
            WCHAR msg[128];
            swprintf(msg, 128, L"Unable to %s document. (0x%08lX)",
                     (item == dsoFileOpen) ? L"open" : L"save", hr);
            MessageBoxW(g_hwndMain, msg, L"Error", MB_ICONERROR);
        }
        return TRUE;  /* Cancel = True: we handled it */
    }
    return FALSE;     /* let the control run the default action */
}

/* oFramer_OnDocumentOpened */
static void WINAPI_OnDocumentOpened(LPCWSTR file, IDispatch * /*document*/)
{
    printf("[event] OnDocumentOpened: %ls\n", file ? file : L""); fflush(stdout);
    g_fDocOpen = TRUE;
    EnableItems(GetMenu(g_hwndMain), TRUE);
    if (file && *file)
        SetStatusText((std::wstring(L"Current File: ") + file).c_str());
    else
        SetStatusText(STATUS_UNSAVED);
}

/* oFramer_OnDocumentClosed */
static void WINAPI_OnDocumentClosed(void)
{
    printf("[event] OnDocumentClosed\n"); fflush(stdout);
    g_fDocOpen = FALSE;
    EnableItems(GetMenu(g_hwndMain), FALSE);
    SetStatusText(STATUS_NONE);
}

/* oFramer_BeforeDocumentClosed: ask to save when dirty */
static BOOL WINAPI_BeforeDocumentClosed(IDispatch * /*document*/)
{
    printf("[event] BeforeDocumentClosed dirty=%d readonly=%d\n",
           g_framer->IsDirty(), g_framer->IsReadOnly()); fflush(stdout);

    if (!g_framer->IsDirty())
        return FALSE;  /* proceed with close */

    int answer = MessageBoxW(g_hwndMain,
        L"Would you like to save the file before closing it?",
        L"Save Changes?", MB_ICONQUESTION | MB_YESNOCANCEL);

    if (answer == IDCANCEL)
        return TRUE;   /* Cancel = True: abort close */

    if (answer == IDYES) {
        if (g_framer->IsReadOnly() || !g_framer->GetDocumentFullName(NULL, 0)) {
            /* note: GetDocumentFullName(buf,0) is invalid; use a probe */
        }
        WCHAR fullName[MAX_PATH*2];
        BOOL haveName = g_framer->GetDocumentFullName(fullName, MAX_PATH*2);
        if (!haveName || !fullName[0] || g_framer->IsReadOnly()) {
            g_framer->ShowDialog(dsoDialogSave);   /* SaveAs dialog */
        } else {
            HRESULT hr = g_framer->Save();
            if (FAILED(hr) && hr != E_ABORT) {
                MessageBoxW(g_hwndMain, L"Unable to save document.",
                            L"Error", MB_ICONERROR);
            }
        }
    }
    return FALSE;      /* proceed with close */
}

/* oFramer_OnPrintPreviewExit */
static void WINAPI_OnPrintPreviewExit(void)
{
    printf("[event] OnPrintPreviewExit\n"); fflush(stdout);
    g_fInPreview = FALSE;
    EnableItems(GetMenu(g_hwndMain), TRUE);
}

/* =====================================================================
 * Menu commands (VB mnu*_Click subs)
 * ===================================================================== */

/* Simple InputBox replacement for Custom Caption */
static BOOL PromptForText(HWND hwnd, LPCWSTR title, LPCWSTR prompt,
                          WCHAR *buf, DWORD cch)
{
    /* Minimal one-edit dialog built at runtime */
    DLGTEMPLATE tmpl = {WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_SETFONT |
                        DS_MODALFRAME, 0, 3, 0, 0, 0, 0};
    /* Use a fixed-size buffer layout built with WriteDialogData approach:
       easier to use CreateWindow directly with a modal loop. */
    (void)tmpl;

    HWND hDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"#32770", title,
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        0, 0, 420, 130, hwnd, NULL, GetModuleHandleW(NULL), NULL);
    if (!hDlg) return FALSE;

    CreateWindowExW(0, L"STATIC", prompt, WS_CHILD | WS_VISIBLE,
        12, 10, 380, 20, hDlg, NULL, NULL, NULL);
    HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", buf,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
        12, 35, 380, 22, hDlg, (HMENU)100, NULL, NULL);
    CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE |
        WS_TABSTOP | BS_DEFPUSHBUTTON, 180, 65, 100, 26,
        hDlg, (HMENU)IDOK, NULL, NULL);
    CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE |
        WS_TABSTOP | BS_PUSHBUTTON, 292, 65, 100, 26,
        hDlg, (HMENU)IDCANCEL, NULL, NULL);

    SetWindowTextW(hDlg, title);
    SendMessageW(hDlg, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessageW(hEdit, EM_SETSEL, 0, -1);
    SetFocus(hEdit);

    /* Center on owner */
    RECT rcOwn, rcDlg;
    GetWindowRect(hwnd, &rcOwn);
    GetWindowRect(hDlg, &rcDlg);
    SetWindowPos(hDlg, NULL,
        rcOwn.left + ((rcOwn.right-rcOwn.left) - (rcDlg.right-rcDlg.left))/2,
        rcOwn.top + ((rcOwn.bottom-rcOwn.top) - (rcDlg.bottom-rcDlg.top))/2,
        0, 0, SWP_NOSIZE | SWP_NOZORDER);

    BOOL fOK = FALSE;
    ShowWindow(hDlg, SW_SHOW);
    EnableWindow(hwnd, FALSE);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (msg.message == WM_QUIT) { PostQuitMessage((int)msg.wParam); break; }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) break;
        if (msg.message == WM_COMMAND) {
            int id = LOWORD(msg.wParam);
            if (id == IDOK) {
                GetWindowTextW(hEdit, buf, cch);
                fOK = TRUE;
                break;
            }
            if (id == IDCANCEL) break;
        }
        /* simple IsDialogMessage for tab handling */
        if (!IsDialogMessageW(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    EnableWindow(hwnd, TRUE);
    DestroyWindow(hDlg);
    SetFocus(hwnd);
    return fOK;
}

static void ShowOleError(HWND hwnd, LPCWSTR action, HRESULT hr)
{
    if (hr == E_ABORT || SUCCEEDED(hr)) return;
    WCHAR msg[192];
    swprintf(msg, 192, L"Unable to %s.\n(%ld): 0x%08lX", action, (long)hr, (unsigned long)hr);
    MessageBoxW(hwnd, msg, L"Error", MB_ICONERROR);
}

static void SyncShowChecks(HMENU hmenu)
{
    BOOL fCap = FALSE, fBar = FALSE, fTb = FALSE;
    dsoBorderStyle style = dsoBorderFlat;
    if (g_framer) {
        g_framer->get_Titlebar(&fCap);
        g_framer->get_Menubar(&fBar);
        g_framer->get_Toolbars(&fTb);
        g_framer->get_BorderStyle(&style);
        if (style < dsoBorderNone || style > dsoBorder3DThin)
            style = dsoBorderFlat;
    }
    CheckMenuItem(hmenu, IDM_SHOW_CAPTION,  MF_BYCOMMAND | (fCap ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hmenu, IDM_SHOW_MENUBAR,  MF_BYCOMMAND | (fBar ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hmenu, IDM_SHOW_TOOLBAR,  MF_BYCOMMAND | (fTb ? MF_CHECKED : MF_UNCHECKED));
    for (int i = 0; i < 4; i++)
        CheckMenuItem(hmenu, IDM_SHOW_BORDER_FIRST + i,
                      MF_BYCOMMAND | (i == (int)style ? MF_CHECKED : MF_UNCHECKED));
    for (int i = 0; i < 9; i++)
        CheckMenuItem(hmenu, IDM_SHOW_DISABLE_FIRST + i,
                      MF_BYCOMMAND | (g_fDisableItem[i] ? MF_CHECKED : MF_UNCHECKED));
}

static void HandleCommand(HWND hwnd, int id)
{
    HMENU hmenu = GetMenu(hwnd);
    HRESULT hr = S_OK;

    switch (id) {
    /* ---- File ---- */
    case IDM_FILE_NEW:      /* mnuFileNew_Click: ShowDialog dsoDialogNew */
        hr = g_framer->ShowDialog(dsoDialogNew);
        ShowOleError(hwnd, L"create new item", hr);
        break;

    case IDM_FILE_OPEN:     /* mnuFileOpen_Click: ShowDialog dsoDialogOpen */
        hr = g_framer->ShowDialog(dsoDialogOpen);
        ShowOleError(hwnd, L"open document", hr);
        break;

    case IDM_FILE_CLOSE:    /* mnuFileClose_Click */
        g_framer->Close();
        break;

    case IDM_FILE_SAVE:     /* mnuFileSave_Click */
        hr = g_framer->Save();
        ShowOleError(hwnd, L"save document", hr);
        break;

    case IDM_FILE_SAVEAS:   /* mnuFileSaveAs_Click: ShowDialog dsoDialogSave */
        hr = g_framer->ShowDialog(dsoDialogSave);
        ShowOleError(hwnd, L"save document", hr);
        break;

    case IDM_FILE_PAGESETUP:
        hr = g_framer->ShowDialog(dsoDialogPageSetup);
        ShowOleError(hwnd, L"show page setup", hr);
        break;

    case IDM_FILE_PRINTPREVIEW:  /* mnuFilePrintPreview_Click */
        hr = g_framer->PrintPreview();
        if (SUCCEEDED(hr)) {
            g_fInPreview = TRUE;
            EnableItems(hmenu, FALSE);
        } else {
            ShowOleError(hwnd, L"go into print preview", hr);
        }
        break;

    case IDM_FILE_PRINT:    /* mnuFilePrint_Click(0): PrintOut fPrompt=False */
        hr = g_framer->ShowDialog(dsoDialogPrint);
        ShowOleError(hwnd, L"print document", hr);
        break;

    case IDM_FILE_PROPERTIES:
        hr = g_framer->ShowDialog(dsoDialogProperties);
        ShowOleError(hwnd, L"show properties page", hr);
        break;

    case IDM_FILE_QUIT:     /* mnuFileQuit_Click: close doc, then unload */
        if (g_fDocOpen)
            g_framer->Close();   /* fires BeforeDocumentClosed */
        DestroyWindow(hwnd);
        break;

    /* ---- Show ---- */
    case IDM_SHOW_CAPTION: { /* oFramer.Titlebar = Not Checked */
        BOOL b = FALSE; g_framer->get_Titlebar(&b);
        g_framer->put_Titlebar(!b);
        SyncShowChecks(hmenu);
        break;
    }
    case IDM_SHOW_MENUBAR: {
        BOOL b = FALSE; g_framer->get_Menubar(&b);
        g_framer->put_Menubar(!b);
        SyncShowChecks(hmenu);
        break;
    }
    case IDM_SHOW_TOOLBAR: {
        BOOL b = FALSE; g_framer->get_Toolbars(&b);
        g_framer->put_Toolbars(!b);
        SyncShowChecks(hmenu);
        break;
    }
    case IDM_SHOW_BORDER_NONE:
    case IDM_SHOW_BORDER_OUTLINE:
    case IDM_SHOW_BORDER_3D:
    case IDM_SHOW_BORDER_3DTHIN:  /* mnuBorderStyle_Click(Index) */
        g_framer->put_BorderStyle((dsoBorderStyle)(id - IDM_SHOW_BORDER_FIRST));
        SyncShowChecks(hmenu);
        break;

    case IDM_SHOW_DISABLE_FIRST + 0:
    case IDM_SHOW_DISABLE_FIRST + 1:
    case IDM_SHOW_DISABLE_FIRST + 2:
    case IDM_SHOW_DISABLE_FIRST + 3:
    case IDM_SHOW_DISABLE_FIRST + 4:
    case IDM_SHOW_DISABLE_FIRST + 5:
    case IDM_SHOW_DISABLE_FIRST + 6:
    case IDM_SHOW_DISABLE_FIRST + 7:
    case IDM_SHOW_DISABLE_FIRST + 8: { /* mnuDisableItem_Click(Index) */
        int idx = id - IDM_SHOW_DISABLE_FIRST;
        dsoFileCommandType item = (dsoFileCommandType)idx;
        /* VB: oFramer.EnableFileCommand(Index) = Checked (invert) */
        g_fDisableItem[idx] = !g_fDisableItem[idx];
        g_framer->put_EnableFileCommand(item, !g_fDisableItem[idx]);
        SyncShowChecks(hmenu);
        break;
    }
    case IDM_SHOW_CUSTOMCAPTION: { /* mnuCustomCaption_Click */
        WCHAR cap[128] = L"";
        if (PromptForText(hwnd, L"Caption",
                L"Provide a custom caption for the framer titlebar:",
                cap, 128) && cap[0])
            g_framer->put_Caption(cap);
        break;
    }
    }
}

/* =====================================================================
 * Window procedure
 * ===================================================================== */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        g_hwndMain = hwnd;

        /* lbCurrentFile status label */
        g_hwndStatus = CreateWindowExW(0, L"STATIC", STATUS_NONE,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 400, 24, hwnd, (HMENU)IDC_STATUS_FILE,
            GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hwndStatus, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        /* Separator line below the header (VB lnTitle) */
        CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE |
            SS_ETCHEDHORZ, 8, STATUS_LINE_Y - 4, 800, 4,
            hwnd, NULL, NULL, NULL);

        /* Create the control (Form_Load: oFramer setup) */
        FramerEventHandlers handlers = {
            &WINAPI_OnFileCommand,
            &WINAPI_OnDocumentOpened,
            &WINAPI_OnDocumentClosed,
            &WINAPI_BeforeDocumentClosed,
            &WINAPI_OnPrintPreviewExit,
        };
        g_framer = new CFramerControl();
        HRESULT hr = g_framer->Create(hwnd, &handlers);
        if (FAILED(hr)) {
            MessageBoxW(hwnd, L"Failed to create DsoFramer control.\n"
                        "Check that dsoframer.ocx is registered in the WINEPREFIX.",
                        L"Error", MB_ICONERROR);
            return -1;
        }

        /* Form_Load: HostName, initial check states, EnableItems(False) */
        g_framer->put_HostName(HOST_NAME);
        SyncShowChecks(GetMenu(hwnd));
        EnableItems(GetMenu(hwnd), FALSE);
        UpdateLayout();

        /* Command-line automation (test aid): new:<progid> / open:<file> */
        if (wcsncmp(g_szAutoCmd, L"new:", 4) == 0) {
            printf("[auto] CreateNew(%ls)\n", g_szAutoCmd + 4); fflush(stdout);
            HRESULT hr2 = g_framer->CreateNew(g_szAutoCmd + 4);
            printf("[auto] CreateNew hr=0x%08lX\n", (unsigned long)hr2); fflush(stdout);
        } else if (wcsncmp(g_szAutoCmd, L"open:", 5) == 0) {
            printf("[auto] Open(%ls)\n", g_szAutoCmd + 5); fflush(stdout);
            HRESULT hr2 = g_framer->Open(g_szAutoCmd + 5, FALSE);
            printf("[auto] Open hr=0x%08lX\n", (unsigned long)hr2); fflush(stdout);
        }
        return 0;
    }

    case WM_SIZE:
        UpdateLayout();
        return 0;

    case WM_COMMAND:
        if (HIWORD(wParam) == 0)   /* menu / accelerator */
            HandleCommand(hwnd, LOWORD(wParam));
        return 0;

    case WM_SETFOCUS:
        /* forward focus into the embedded control (VB Activate) */
        if (g_framer && g_framer->Window())
            SetFocus(g_framer->Window());
        return 0;

    case WM_CLOSE:
        /* Form_QueryUnload: close doc first (BeforeDocumentClosed runs) */
        if (g_fDocOpen && g_framer)
            g_framer->Close();
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        delete g_framer; g_framer = NULL;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* =====================================================================
 * WinMain
 * ===================================================================== */
int PASCAL wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR pszCmd, int nCmdShow)
{
    OleInitialize(NULL);

    /* Parse automation arg: framerapp.exe new:Word.Document */
    if (pszCmd && *pszCmd) {
        WCHAR *p = pszCmd;
        while (*p == L' ') p++;
        wcsncpy(g_szAutoCmd, p, 511);
        g_szAutoCmd[511] = L'\0';
        printf("[auto] cmdline: %ls\n", g_szAutoCmd); fflush(stdout);
    }

    /* Arial 8pt-ish status font (VB lbCurrentFile uses Arial 8.25) */
    g_hFont = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Arial");

    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = APP_CLASS;
    RegisterClassW(&wc);

    /* VB form 8430x7290 twips ≈ 562x486 pixels client area */
    HWND hwnd = CreateWindowExW(0, APP_CLASS, APP_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 700, 560,
        NULL, LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_MAINMENU)), hInst, NULL);
    if (!hwnd) {
        OleUninitialize();
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG m;
    while (GetMessageW(&m, NULL, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }

    DeleteObject(g_hFont);
    OleUninitialize();
    return (int)m.wParam;
}
