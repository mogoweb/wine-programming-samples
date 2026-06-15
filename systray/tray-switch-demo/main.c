/*
 * 托盘演示 - 图标切换
 *
 * 启动时显示带内容的半透明图标，500ms后自动更新为另一个图标
 */

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_SHOW_WINDOW 1001
#define ID_TRAY_EXIT 1002
#define TIMER_ID_SWITCH_ICON 1

static const char *CLASS_NAME = "TraySwitchDemoWindow";
static const char *WINDOW_TITLE = "Tray Switch Icon Demo";
static NOTIFYICONDATA nid = {0};
static HWND g_hwnd = NULL;
static HFONT g_hFont = NULL;
static HICON g_hIcon1 = NULL;
static HICON g_hIcon2 = NULL;

/* 创建一个带透明度内容的图标 - 蓝色圆形 */
static HICON CreateIcon1(int size)
{
    BITMAPV5HEADER bi = {0};
    bi.bV5Size = sizeof(BITMAPV5HEADER);
    bi.bV5Width = size;
    bi.bV5Height = size;
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask = 0x00FF0000;
    bi.bV5GreenMask = 0x0000FF00;
    bi.bV5BlueMask = 0x000000FF;
    bi.bV5AlphaMask = 0xFF000000;

    HDC hdc = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdc);

    unsigned char *bits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS, (void **)&bits, NULL, 0);
    if (!hBitmap) {
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdc);
        return NULL;
    }

    int cx = size / 2;
    int cy = size / 2;
    int radius = size / 2 - 2;

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int dx = x - cx;
            int dy = y - cy;
            int dist = dx * dx + dy * dy;
            int r2 = radius * radius;

            int idx = (y * size + x) * 4;

            if (dist <= r2) {
                /* 蓝色半透明圆形 */
                bits[idx + 0] = 219;   /* Blue */
                bits[idx + 1] = 152;   /* Green */
                bits[idx + 2] = 52;    /* Red */
                bits[idx + 3] = 200;   /* Alpha */
            } else {
                bits[idx + 0] = 0;
                bits[idx + 1] = 0;
                bits[idx + 2] = 0;
                bits[idx + 3] = 0;
            }
        }
    }

    HBITMAP hMask = CreateBitmap(size, size, 1, 1, NULL);

    ICONINFO ii = {0};
    ii.fIcon = TRUE;
    ii.hbmColor = hBitmap;
    ii.hbmMask = hMask;

    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(hBitmap);
    DeleteObject(hMask);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdc);

    return hIcon;
}

/* 创建另一个带透明度内容的图标 - 绿色方形 */
static HICON CreateIcon2(int size)
{
    BITMAPV5HEADER bi = {0};
    bi.bV5Size = sizeof(BITMAPV5HEADER);
    bi.bV5Width = size;
    bi.bV5Height = size;
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask = 0x00FF0000;
    bi.bV5GreenMask = 0x0000FF00;
    bi.bV5BlueMask = 0x000000FF;
    bi.bV5AlphaMask = 0xFF000000;

    HDC hdc = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdc);

    unsigned char *bits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS, (void **)&bits, NULL, 0);
    if (!hBitmap) {
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdc);
        return NULL;
    }

    int margin = size / 8;

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int idx = (y * size + x) * 4;

            if (x >= margin && x < size - margin && y >= margin && y < size - margin) {
                /* 绿色半透明方形 */
                bits[idx + 0] = 113;   /* Blue */
                bits[idx + 1] = 204;   /* Green */
                bits[idx + 2] = 46;    /* Red */
                bits[idx + 3] = 200;   /* Alpha */
            } else {
                bits[idx + 0] = 0;
                bits[idx + 1] = 0;
                bits[idx + 2] = 0;
                bits[idx + 3] = 0;
            }
        }
    }

    HBITMAP hMask = CreateBitmap(size, size, 1, 1, NULL);

    ICONINFO ii = {0};
    ii.fIcon = TRUE;
    ii.hbmColor = hBitmap;
    ii.hbmMask = hMask;

    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(hBitmap);
    DeleteObject(hMask);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdc);

    return hIcon;
}

static void UpdateTrayIcon(HICON hIcon)
{
    nid.hIcon = hIcon;
    Shell_NotifyIcon(NIM_MODIFY, &nid);
    fprintf(stderr, "[Tray] Icon updated\n");
}

static void CreateTrayIcon(HWND hwnd)
{
    int iconSize = GetSystemMetrics(SM_CXSMICON);
    fprintf(stderr, "[Tray] System icon size: %d\n", iconSize);

    g_hIcon1 = CreateIcon1(iconSize);
    g_hIcon2 = CreateIcon2(iconSize);

    if (!g_hIcon1 || !g_hIcon2) {
        fprintf(stderr, "[Tray] Failed to create icons\n");
        g_hIcon1 = LoadIcon(NULL, IDI_APPLICATION);
    } else {
        fprintf(stderr, "[Tray] Created two icons\n");
    }

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = g_hIcon1;
    strcpy(nid.szTip, "Tray Switch Icon Demo");

    Shell_NotifyIcon(NIM_ADD, &nid);

    /* 设置定时器，500ms 后切换图标 */
    SetTimer(hwnd, TIMER_ID_SWITCH_ICON, 500, NULL);
    fprintf(stderr, "[Tray] Timer set for 500ms\n");
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
    AppendMenu(hMenu, MF_STRING, ID_TRAY_SHOW_WINDOW, "Show Window");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, "Exit");

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
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

    case WM_TIMER:
        if (wParam == TIMER_ID_SWITCH_ICON) {
            KillTimer(hwnd, TIMER_ID_SWITCH_ICON);
            fprintf(stderr, "[Tray] Timer fired, switching to icon 2\n");
            if (g_hIcon2) {
                UpdateTrayIcon(g_hIcon2);
            }
        }
        return 0;

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
        if (g_hIcon1) DestroyIcon(g_hIcon1);
        if (g_hIcon2) DestroyIcon(g_hIcon2);
        if (g_hFont) DeleteObject(g_hFont);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassA(&wc);

    g_hwnd = CreateWindowExA(
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

    CreateWindowExA(
        0,
        "STATIC",
        "Click close button to minimize to system tray\n\n"
        "Right-click tray icon to show menu\n"
        "Double-click tray icon to show window\n\n"
        "Demo: Icon starts as blue circle,\n"
        "switches to green square after 500ms",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        30, 50, 340, 110,
        g_hwnd,
        NULL,
        hInstance,
        NULL
    );

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
