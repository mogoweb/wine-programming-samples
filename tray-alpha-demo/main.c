/*
 * 托盘演示 - 透明通道图标
 *
 * 演示带透明通道（Alpha Channel）的托盘图标显示
 */

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_SHOW_WINDOW 1001
#define ID_TRAY_EXIT 1002

static const char *CLASS_NAME = "TrayAlphaDemoWindow";
static const char *WINDOW_TITLE = "Tray Alpha Icon Demo";
static NOTIFYICONDATA nid = {0};
static HWND g_hwnd = NULL;
static HFONT g_hFont = NULL;
static HICON g_hTrayIcon = NULL;

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

static void CreateTrayIcon(HWND hwnd)
{
    char exePath[MAX_PATH];
    char iconPath[MAX_PATH];

    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    char *lastSlash = strrchr(exePath, '\\');
    if (lastSlash) {
        *(lastSlash + 1) = '\0';
    }

    /* 尝试加载 ICO 文件 */
    snprintf(iconPath, MAX_PATH, "%stray-alpha.ico", exePath);
    fprintf(stderr, "[Tray] Trying ICO: %s\n", iconPath);
    g_hTrayIcon = LoadTrayIconFromFile(iconPath);

    if (!g_hTrayIcon) {
        /* 回退到系统默认图标 */
        g_hTrayIcon = LoadIcon(NULL, IDI_APPLICATION);
        fprintf(stderr, "[Tray] Using default system icon\n");
    }

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = g_hTrayIcon;
    strcpy(nid.szTip, "Tray Alpha Icon Demo");

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
        if (g_hTrayIcon) DestroyIcon(g_hTrayIcon);
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
        "This demo uses ICO icon with alpha channel",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        30, 60, 340, 100,
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
