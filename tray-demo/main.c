#include <windows.h>
#include <shellapi.h>
#include <stdio.h>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_SHOW_WINDOW 1001
#define ID_TRAY_EXIT 1002

static const wchar_t *CLASS_NAME = L"TrayDemoWindow";
static const wchar_t *WINDOW_TITLE = L"托盘演示程序";
static NOTIFYICONDATA nid = {0};
static HWND g_hwnd = NULL;
static HFONT g_hFont = NULL;
static HFONT g_hMenuFont = NULL;

typedef struct {
    wchar_t *text;
    UINT id;
} MenuItemInfo;

static MenuItemInfo menuItems[] = {
    { L"显示主窗口", ID_TRAY_SHOW_WINDOW },
    { L"退出", ID_TRAY_EXIT }
};

static void CreateFonts(void)
{
    LOGFONT lf = {0};
    lf.lfHeight = -16;
    lf.lfWeight = FW_NORMAL;
    wcscpy(lf.lfFaceName, L"WenQuanYi Micro Hei");

    g_hFont = CreateFontIndirect(&lf);

    lf.lfHeight = -14;
    g_hMenuFont = CreateFontIndirect(&lf);
}

static void CreateTrayIcon(HWND hwnd)
{
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy(nid.szTip, L"托盘演示程序");

    Shell_NotifyIcon(NIM_ADD, &nid);
}

static void RemoveTrayIcon(void)
{
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

static void ShowContextMenu(HWND hwnd)
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_OWNERDRAW, ID_TRAY_SHOW_WINDOW, (LPCSTR)&menuItems[0]);
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_OWNERDRAW, ID_TRAY_EXIT, (LPCSTR)&menuItems[1]);

    SetForegroundWindow(hwnd);
    fprintf(stderr, "pt.x = %ld, pt.y = %ld\n", pt.x, pt.y);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

static void OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT *mis)
{
    if (!g_hMenuFont) return;

    HDC hdc = GetDC(hwnd);
    HFONT hOldFont = SelectObject(hdc, g_hMenuFont);

    MenuItemInfo *item = (MenuItemInfo *)mis->itemData;
    if (item) {
        SIZE size;
        GetTextExtentPoint32(hdc, item->text, wcslen(item->text), &size);
        mis->itemWidth = size.cx + 20;
        mis->itemHeight = size.cy + 8;
    }

    SelectObject(hdc, hOldFont);
    ReleaseDC(hwnd, hdc);
}

static void OnDrawItem(HWND hwnd, DRAWITEMSTRUCT *dis)
{
    if (!g_hMenuFont || !dis->itemData) return;

    MenuItemInfo *item = (MenuItemInfo *)dis->itemData;

    HDC hdc = dis->hDC;
    RECT rect = dis->rcItem;

    COLORREF bgColor, textColor;
    if (dis->itemState & ODS_SELECTED) {
        bgColor = GetSysColor(COLOR_HIGHLIGHT);
        textColor = GetSysColor(COLOR_HIGHLIGHTTEXT);
    } else {
        bgColor = GetSysColor(COLOR_MENU);
        textColor = GetSysColor(COLOR_MENUTEXT);
    }

    HBRUSH hBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &rect, hBrush);
    DeleteObject(hBrush);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);

    HFONT hOldFont = SelectObject(hdc, g_hMenuFont);

    rect.left += 10;
    DrawText(hdc, item->text, wcslen(item->text), &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hOldFont);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        return (INT_PTR)GetStockObject(WHITE_BRUSH);
    }

    case WM_MEASUREITEM:
        OnMeasureItem(hwnd, (MEASUREITEMSTRUCT *)lParam);
        return TRUE;

    case WM_DRAWITEM:
        OnDrawItem(hwnd, (DRAWITEMSTRUCT *)lParam);
        return TRUE;

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_TRAYICON:
        if (LOWORD(lParam) == WM_RBUTTONUP) {
            ShowContextMenu(hwnd);
        } else if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_SHOW_WINDOW:
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
            break;
        case ID_TRAY_EXIT:
            RemoveTrayIcon();
            PostQuitMessage(0);
            break;
        }
        return 0;

    case WM_DESTROY:
        RemoveTrayIcon();
        if (g_hFont) DeleteObject(g_hFont);
        if (g_hMenuFont) DeleteObject(g_hMenuFont);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    CreateFonts();

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    g_hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 300,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (g_hwnd == NULL) {
        return 0;
    }

    CreateTrayIcon(g_hwnd);

    ShowWindow(g_hwnd, nCmdShow);

    HWND hStatic = CreateWindowEx(
        0,
        L"STATIC",
        L"点击窗口关闭按钮将最小化到系统托盘\n\n"
        L"右键点击托盘图标可显示菜单\n"
        L"双击托盘图标可显示窗口",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        50, 80, 300, 80,
        g_hwnd,
        NULL,
        hInstance,
        NULL
    );

    if (g_hFont && hStatic) {
        SendMessage(hStatic, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
