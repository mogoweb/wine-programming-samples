#include <windows.h>
#include <stdio.h>

// 按钮控件ID — 按钮分为两组：对蓝色窗口操作、对红色窗口操作
// 蓝色窗口组 (1001-1006)
#define IDC_BLUE_TOP          1001
#define IDC_BLUE_BOTTOM       1002
#define IDC_BLUE_TOPMOST      1003
#define IDC_BLUE_NOTOPMOST    1004
#define IDC_BLUE_BRING        1005
#define IDC_BLUE_AFTER_RED    1006
// 红色窗口组 (1011-1016)
#define IDC_RED_TOP           1011
#define IDC_RED_BOTTOM        1012
#define IDC_RED_TOPMOST       1013
#define IDC_RED_NOTOPMOST     1014
#define IDC_RED_BRING         1015
#define IDC_RED_BEFORE_BLUE   1016

// 全局窗口句柄
HWND g_hwndCtrl  = NULL;  // 控制面板
HWND g_hwndBlue  = NULL;  // 蓝色目标窗口
HWND g_hwndRed   = NULL;  // 红色目标窗口

// 获取窗口在同层级中的z-order位置（1=最顶）
static int GetZOrder(HWND hwnd) {
    int z = 0;
    HWND h = hwnd;
    while (h != NULL) {
        h = GetNextWindow(h, GW_HWNDPREV);
        z++;
    }
    return z;
}

// 刷新所有目标窗口的z-order状态显示
static void RefreshAllZOrderInfo(void) {
    if (g_hwndBlue) InvalidateRect(g_hwndBlue, NULL, TRUE);
    if (g_hwndRed)  InvalidateRect(g_hwndRed, NULL, TRUE);
}

// 在目标窗口中绘制z-order信息
static void PaintZOrderInfo(HWND hwnd, HDC hdc, const char *title, COLORREF bgColor) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    HBRUSH bgBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &rc, bgBrush);
    DeleteObject(bgBrush);

    int z = GetZOrder(hwnd);
    BOOL topmost = GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST;

    char line1[128], line2[128], line3[128];
    snprintf(line1, sizeof(line1), "%s", title);
    snprintf(line2, sizeof(line2), "Z-Order: %d  (%s)", z, z == 1 ? "top" : "not top");
    snprintf(line3, sizeof(line3), "TopMost: %s", topmost ? "YES" : "NO");

    SetBkMode(hdc, TRANSPARENT);

    HFONT hFontBig = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, NULL);
    HFONT hFontSmall = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, NULL);

    int cx = rc.right / 2;

    SelectObject(hdc, hFontBig);
    SetTextColor(hdc, RGB(30, 30, 80));
    TextOut(hdc, cx - 70, 30, line1, (int)strlen(line1));

    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, RGB(50, 50, 50));
    TextOut(hdc, cx - 90, 80, line2, (int)strlen(line2));
    TextOut(hdc, cx - 60, 110, line3, (int)strlen(line3));

    DeleteObject(hFontBig);
    DeleteObject(hFontSmall);
}

// 目标窗口共享的窗口过程（只显示状态，不含按钮）
LRESULT CALLBACK TargetWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    const char *title = NULL;
    COLORREF bg = 0;

    if (hwnd == g_hwndBlue) {
        title = "Blue Window";
        bg = RGB(180, 200, 240);
    } else {
        title = "Red Window";
        bg = RGB(240, 190, 180);
    }

    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            PaintZOrderInfo(hwnd, hdc, title, bg);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_WINDOWPOSCHANGED:
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 控制面板窗口过程
LRESULT CALLBACK CtrlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND: {
            UINT id = LOWORD(wParam);
            // 所有 SetWindowPos 调用都加 SWP_NOACTIVATE，
            // 避免激活目标窗口而干扰z-order演示效果
            switch (id) {
                // 蓝色窗口操作
                case IDC_BLUE_TOP:
                    SetWindowPos(g_hwndBlue, HWND_TOP, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    break;
                case IDC_BLUE_BOTTOM:
                    SetWindowPos(g_hwndBlue, HWND_BOTTOM, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    break;
                case IDC_BLUE_TOPMOST:
                    SetWindowPos(g_hwndBlue, HWND_TOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    break;
                case IDC_BLUE_NOTOPMOST:
                    SetWindowPos(g_hwndBlue, HWND_NOTOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    break;
                case IDC_BLUE_BRING:
                    SetWindowPos(g_hwndBlue, HWND_TOP, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    SetForegroundWindow(g_hwndBlue);
                    break;
                case IDC_BLUE_AFTER_RED:
                    // 将蓝色窗口放在红色窗口之后（z-order低于红色）
                    SetWindowPos(g_hwndBlue, g_hwndRed, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    break;
                // 红色窗口操作
                case IDC_RED_TOP:
                    SetWindowPos(g_hwndRed, HWND_TOP, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    break;
                case IDC_RED_BOTTOM:
                    SetWindowPos(g_hwndRed, HWND_BOTTOM, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    break;
                case IDC_RED_TOPMOST:
                    SetWindowPos(g_hwndRed, HWND_TOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    break;
                case IDC_RED_NOTOPMOST:
                    SetWindowPos(g_hwndRed, HWND_NOTOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    break;
                case IDC_RED_BRING:
                    SetWindowPos(g_hwndRed, HWND_TOP, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    SetForegroundWindow(g_hwndRed);
                    break;
                case IDC_RED_BEFORE_BLUE:
                    // 将红色窗口放在蓝色窗口之前（z-order高于蓝色）
                    SetWindowPos(g_hwndRed, g_hwndBlue, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    break;
            }
            RefreshAllZOrderInfo();
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 在控制面板中创建按钮
static void CreateControlButtons(HWND hwnd) {
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
    int btnW = 170, btnH = 26, gap = 4;
    int xLeft = 10, xRight = 190;
    int y = 30;

    // 左列标题：蓝色窗口
    CreateWindow("STATIC", "Blue Window:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xLeft, y - 20, btnW, 18,
        hwnd, NULL, hInst, NULL);

    struct { int id; const char *text; } blueBtns[] = {
        { IDC_BLUE_TOP,        "HWND_TOP" },
        { IDC_BLUE_BOTTOM,     "HWND_BOTTOM" },
        { IDC_BLUE_TOPMOST,    "HWND_TOPMOST" },
        { IDC_BLUE_NOTOPMOST,  "HWND_NOTOPMOST" },
        { IDC_BLUE_BRING,      "BringWindowToTop" },
        { IDC_BLUE_AFTER_RED,  "After Red (hWndAfter)" },
    };

    for (int i = 0; i < 6; i++) {
        CreateWindow("BUTTON", blueBtns[i].text,
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            xLeft, y + i * (btnH + gap), btnW, btnH,
            hwnd, (HMENU)(INT_PTR)blueBtns[i].id, hInst, NULL);
    }

    // 右列标题：红色窗口
    CreateWindow("STATIC", "Red Window:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        xRight, y - 20, btnW, 18,
        hwnd, NULL, hInst, NULL);

    struct { int id; const char *text; } redBtns[] = {
        { IDC_RED_TOP,          "HWND_TOP" },
        { IDC_RED_BOTTOM,       "HWND_BOTTOM" },
        { IDC_RED_TOPMOST,      "HWND_TOPMOST" },
        { IDC_RED_NOTOPMOST,    "HWND_NOTOPMOST" },
        { IDC_RED_BRING,        "BringWindowToTop" },
        { IDC_RED_BEFORE_BLUE,  "Before Blue (hWndAfter)" },
    };

    for (int i = 0; i < 6; i++) {
        CreateWindow("BUTTON", redBtns[i].text,
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            xRight, y + i * (btnH + gap), btnW, btnH,
            hwnd, (HMENU)(INT_PTR)redBtns[i].id, hInst, NULL);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance; (void)lpCmdLine;

    WNDCLASS wcTarget = {0};
    WNDCLASS wcCtrl   = {0};

    // 注册目标窗口类（蓝/红窗口共用）
    wcTarget.lpfnWndProc   = TargetWndProc;
    wcTarget.hInstance     = hInstance;
    wcTarget.lpszClassName = TEXT("TargetWindowClass");
    wcTarget.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wcTarget);

    // 注册控制面板窗口类
    wcCtrl.lpfnWndProc   = CtrlWndProc;
    wcCtrl.hInstance     = hInstance;
    wcCtrl.lpszClassName = TEXT("CtrlWindowClass");
    wcCtrl.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcCtrl.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClass(&wcCtrl);

    // 创建蓝色窗口
    g_hwndBlue = CreateWindow(
        TEXT("TargetWindowClass"), TEXT("Z-Order: Blue"),
        WS_OVERLAPPEDWINDOW,
        80, 80, 400, 250,
        NULL, NULL, hInstance, NULL
    );

    // 创建红色窗口（部分重叠，方便观察z-order）
    g_hwndRed = CreateWindow(
        TEXT("TargetWindowClass"), TEXT("Z-Order: Red"),
        WS_OVERLAPPEDWINDOW,
        250, 160, 400, 250,
        NULL, NULL, hInstance, NULL
    );

    // 创建控制面板（放在右下区域，不与目标窗口重叠）
    g_hwndCtrl = CreateWindow(
        TEXT("CtrlWindowClass"), TEXT("Z-Order Control Panel"),
        WS_OVERLAPPEDWINDOW,
        500, 80, 380, 260,
        NULL, NULL, hInstance, NULL
    );

    CreateControlButtons(g_hwndCtrl);

    ShowWindow(g_hwndBlue, nCmdShow);
    UpdateWindow(g_hwndBlue);
    ShowWindow(g_hwndRed, SW_SHOWNA);
    UpdateWindow(g_hwndRed);
    ShowWindow(g_hwndCtrl, SW_SHOWNA);
    UpdateWindow(g_hwndCtrl);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
