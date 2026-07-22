// main.cpp
// 演示 Layered Window 的 LWA_ALPHA 用法：
//   用 SetLayeredWindowAttributes + LWA_ALPHA 给整个窗口（含标题栏/边框）
//   设置一个常量 Alpha，从而让窗口整体半透明。这是分层窗口最常见、
//   也最简单的用法。通过底部的滑块可实时调整 Alpha，勾选框则演示
//   动态增删 WS_EX_LAYERED 扩展风格（开启/关闭分层效果）。
//
// 编译 (MinGW-w64 交叉编译):
//   i686-w64-mingw32-g++ -Wall -O2 -municode -static-libgcc -static-libstdc++
//       main.cpp manifest.res -mwindows -municode -lcomctl32 -o LayeredAlphaDemo.exe

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#pragma comment(lib, "Comctl32.lib")

#define MAIN_CLASS   L"LayeredAlphaDemoClass"

// 子控件 ID
#define IDC_TRACKBAR   1001
#define IDC_ENABLE     1002
#define IDC_ALPHALBL   1003

// Alpha 范围（不让窗口完全透明，避免“丢失”无法点回）
#define ALPHA_MIN   30
#define ALPHA_MAX   255
#define ALPHA_INIT  220

static HWND g_hTrackbar = NULL;
static HWND g_hEnable   = NULL;
static HWND g_hAlphaLbl = NULL;

// 给窗口设置整体常量 Alpha（LWA_ALPHA）
static void ApplyAlpha(HWND hwnd, BYTE alpha)
{
    SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
}

// 刷新 Alpha 数值标签
static void RefreshAlphaLabel(int pos)
{
    wchar_t buf[32];
    wsprintfW(buf, L"Alpha: %d", pos);
    SetWindowTextW(g_hAlphaLbl, buf);
}

// 窗口过程
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg,
                                WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        // 滑块（Trackbar）
        g_hTrackbar = CreateWindowExW(0, TRACKBAR_CLASSW, NULL,
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
            0, 0, 200, 30,
            hwnd, (HMENU)(INT_PTR)IDC_TRACKBAR,
            GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hTrackbar, TBM_SETRANGE, TRUE,
            MAKELONG(ALPHA_MIN, ALPHA_MAX));
        SendMessageW(g_hTrackbar, TBM_SETPOS, TRUE, ALPHA_INIT);

        // Alpha 数值标签
        g_hAlphaLbl = CreateWindowExW(0, WC_STATICW, L"",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            0, 0, 90, 22,
            hwnd, (HMENU)(INT_PTR)IDC_ALPHALBL,
            GetModuleHandleW(NULL), NULL);
        RefreshAlphaLabel(ALPHA_INIT);

        // “启用分层窗口”复选框
        g_hEnable = CreateWindowExW(0, WC_BUTTONW,
            L"启用分层窗口 (WS_EX_LAYERED)",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            0, 0, 220, 24,
            hwnd, (HMENU)(INT_PTR)IDC_ENABLE,
            GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hEnable, BM_SETCHECK, BST_CHECKED, 0); // 默认启用

        // 窗口已在 WinMain 中以 WS_EX_LAYERED 创建，这里应用初始 Alpha
        ApplyAlpha(hwnd, ALPHA_INIT);
        return 0;
    }

    case WM_SIZE:
    {
        // 控件统一布置在底部
        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        int row1 = h - 64;          // 滑块行
        int row2 = h - 30;          // 复选框行

        MoveWindow(g_hTrackbar, 20, row1, w - 140, 30, TRUE);
        MoveWindow(g_hAlphaLbl, w - 110, row1 + 5, 90, 22, TRUE);
        MoveWindow(g_hEnable,  20, row2, 240, 24, TRUE);
        return 0;
    }

    case WM_HSCROLL:
    {
        // 拖动滑块 -> 实时改变窗口整体透明度
        if ((HWND)lParam == g_hTrackbar)
        {
            int pos = (int)SendMessageW(g_hTrackbar, TBM_GETPOS, 0, 0);
            ApplyAlpha(hwnd, (BYTE)pos);
            RefreshAlphaLabel(pos);
        }
        return 0;
    }

    case WM_COMMAND:
    {
        // 勾选/取消复选框 -> 动态增删 WS_EX_LAYERED 扩展风格
        if (LOWORD(wParam) == IDC_ENABLE &&
            HIWORD(wParam) == BN_CLICKED)
        {
            BOOL checked =
                (SendMessageW(g_hEnable, BM_GETCHECK, 0, 0) == BST_CHECKED);
            LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
            if (checked)
            {
                ex |= WS_EX_LAYERED;
                SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex);
                int pos = (int)SendMessageW(g_hTrackbar, TBM_GETPOS, 0, 0);
                ApplyAlpha(hwnd, (BYTE)pos);
            }
            else
            {
                ex &= ~WS_EX_LAYERED;
                SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex);
                // 通知系统框架改变，重绘为普通不透明窗口
                SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                    SWP_NOACTIVATE | SWP_FRAMECHANGED);
            }
        }
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        // 多彩横条：透过半透明窗口时能直观看到透明度变化
        static const COLORREF colors[] = {
            RGB( 33,  50, 122),
            RGB( 60, 130, 210),
            RGB(210,  80,  80),
            RGB(240, 190,  60),
            RGB( 80, 170,  90),
            RGB(120,  80, 180),
        };
        int n = sizeof(colors) / sizeof(colors[0]);
        int bandH = h / n;
        for (int i = 0; i < n; ++i)
        {
            RECT b = { 0, i * bandH, w, (i + 1) * bandH };
            HBRUSH br = CreateSolidBrush(colors[i]);
            FillRect(hdc, &b, br);
            DeleteObject(br);
        }
        if (n * bandH < h)
        {
            RECT b = { 0, n * bandH, w, h };
            HBRUSH br = CreateSolidBrush(colors[n - 1]);
            FillRect(hdc, &b, br);
            DeleteObject(br);
        }

        // 标题文字
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        HFONT hFont = CreateFontW(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");
        HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
        const wchar_t* title = L"LWA_ALPHA Demo";
        SIZE sz;
        GetTextExtentPoint32W(hdc, title, (int)wcslen(title), &sz);
        TextOutW(hdc, (w - sz.cx) / 2, 30, title, (int)wcslen(title));

        // 提示文字
        HFONT hSmall = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");
        HFONT oldSmall = (HFONT)SelectObject(hdc, hSmall);
        const wchar_t* hint =
            L"拖动滑块实时调整窗口整体透明度；取消勾选则关闭分层效果";
        GetTextExtentPoint32W(hdc, hint, (int)wcslen(hint), &sz);
        TextOutW(hdc, (w - sz.cx) / 2, 78, hint, (int)wcslen(hint));

        SelectObject(hdc, oldSmall);
        DeleteObject(hSmall);
        SelectObject(hdc, oldFont);
        DeleteObject(hFont);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------
// 入口（-municode 需要 wWinMain）
// ---------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev,
                    PWSTR lpCmd, int nShow)
{
    (void)hPrev; (void)lpCmd;

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;                 // 背景由 WM_PAINT 自绘
    wc.lpszClassName = MAIN_CLASS;
    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(NULL, L"RegisterClassExW failed", L"Error", MB_OK);
        return 1;
    }

    // 按目标客户区 500x360 反算窗口尺寸
    RECT rc = { 0, 0, 500, 360 };
    AdjustWindowRectEx(&rc, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_LAYERED);

    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED,                      // 关键：分层窗口扩展风格
        MAIN_CLASS,
        L"Layered Alpha Demo",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInst, NULL);
    if (!hwnd)
    {
        MessageBoxW(NULL, L"CreateWindowExW failed", L"Error", MB_OK);
        return 1;
    }

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
