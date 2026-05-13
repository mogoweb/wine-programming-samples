/*
 * 托盘演示 - 透明通道图标
 *
 * 演示带透明通道（Alpha Channel）的托盘图标显示
 */

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>

#define WM_TRAYICON (WM_USER + 1)
#define WM_TIMER_UPDATE_ICON (WM_USER + 2)
#define ID_TRAY_SHOW_WINDOW 1001
#define ID_TRAY_EXIT 1002
#define TIMER_ID_UPDATE_ICON 1

static const char *CLASS_NAME = "TrayAlphaDemoWindow";
static const char *WINDOW_TITLE = "Tray Alpha Icon Demo";
static NOTIFYICONDATA nid = {0};
static HWND g_hwnd = NULL;
static HFONT g_hFont = NULL;
static HICON g_hTrayIcon = NULL;
static HICON g_hTransparentIcon = NULL;
static HICON g_hContentIcon = NULL;

/* 创建一个指定大小的全透明图标 */
static HICON CreateTransparentIcon(int size)
{
    /* 创建 32位 RGBA 位图 */
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

    /* 填充全透明像素 (alpha = 0) */
    for (int i = 0; i < size * size; i++) {
        bits[i * 4 + 0] = 0;     /* Blue */
        bits[i * 4 + 1] = 0;     /* Green */
        bits[i * 4 + 2] = 0;     /* Red */
        bits[i * 4 + 3] = 0;     /* Alpha = 0 (全透明) */
    }

    /* 创建掩码位图 */
    HBITMAP hMask = CreateBitmap(size, size, 1, 1, NULL);

    /* 创建图标 */
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

/* 创建一个带透明度内容的图标 */
static HICON CreateAlphaIcon(int size)
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

    /* 绘制一个半透明的圆形 */
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
                /* 圆内：蓝色半透明 */
                bits[idx + 0] = 219;   /* Blue */
                bits[idx + 1] = 152;   /* Green */
                bits[idx + 2] = 52;    /* Red */
                bits[idx + 3] = 180;   /* Alpha = 70% 透明度 */
            } else {
                /* 圆外：全透明 */
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

static HICON LoadTrayIconFromFile(const char *iconPath)
{
    HICON hIcon = (HICON)LoadImageA(NULL, iconPath, IMAGE_ICON,
                               GetSystemMetrics(SM_CXSMICON),
                               GetSystemMetrics(SM_CYSMICON),
                               LR_LOADFROMFILE);
    if (hIcon) {
        fprintf(stderr, "[Tray] Loaded icon from ICO: %s\n", iconPath);
        return hIcon;
    }
    fprintf(stderr, "[Tray] LoadImage failed, error: %lu\n", GetLastError());
    return NULL;
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

    /* 创建两个图标 */
    g_hTransparentIcon = CreateTransparentIcon(iconSize);
    g_hContentIcon = CreateAlphaIcon(iconSize);

    if (!g_hTransparentIcon || !g_hContentIcon) {
        fprintf(stderr, "[Tray] Failed to create icons\n");
        g_hTrayIcon = LoadIcon(NULL, IDI_APPLICATION);
    } else {
        g_hTrayIcon = /*g_hTransparentIcon*/g_hContentIcon;
        fprintf(stderr, "[Tray] Created transparent and content icons\n");
    }

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = g_hTrayIcon;
    strcpy(nid.szTip, "Tray Alpha Icon Demo");

    Shell_NotifyIcon(NIM_ADD, &nid);

    /* 设置定时器，500ms 后更新图标 */
    SetTimer(hwnd, TIMER_ID_UPDATE_ICON, 5000, NULL);
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
        if (wParam == TIMER_ID_UPDATE_ICON) {
            KillTimer(hwnd, TIMER_ID_UPDATE_ICON);
            fprintf(stderr, "[Tray] Timer fired, updating to content icon\n");
            if (g_hContentIcon) {
                UpdateTrayIcon(g_hContentIcon);
                g_hTrayIcon = g_hContentIcon;
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
        if (g_hTransparentIcon) DestroyIcon(g_hTransparentIcon);
        if (g_hContentIcon) DestroyIcon(g_hContentIcon);
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
        "Demo: Icon starts fully transparent,\n"
        "then changes to semi-transparent after 500ms",
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
