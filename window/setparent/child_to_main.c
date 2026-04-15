#include <windows.h>

// 全局保存子窗口句柄
HWND g_hChild = NULL;

// 子窗口的过程函数
LRESULT CALLBACK ChildWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            // 给子窗口画一个浅蓝色的背景，方便区分
            HBRUSH hBrush = CreateSolidBrush(RGB(200, 230, 255));
            FillRect(hdc, &ps.rcPaint, hBrush);
            DeleteObject(hBrush);

            SetBkMode(hdc, TRANSPARENT);
            // 判断当前是否还有父窗口
            if (GetParent(hwnd) != NULL) {
                DrawTextW(hdc, L"child window\n(parent window exists)", -1, &rc, DT_CENTER | DT_VCENTER);
            } else {
                DrawTextW(hdc, L"now I am a top-level window！", -1, &rc, DT_CENTER | DT_VCENTER);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            // 注意：子窗口销毁时不要退出整个程序，仅释放自身
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 主（父）窗口的过程函数
LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // 1. 在主窗口创建时，立即创建一个子窗口
            g_hChild = CreateWindowExW(
                0, L"ChildWindowClass", L"Child Window",
                WS_CHILD | WS_VISIBLE | WS_BORDER, // 注意：初始带有 WS_CHILD 样式
                50, 50, 300, 200,                  // 相对于主窗口的坐标和大小
                hwnd, (HMENU)1, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );

            // 2. 设置一个 10 秒的定时器 (10000毫秒)
            SetTimer(hwnd, 1, 10000, NULL);
            return 0;
        }
        case WM_TIMER: {
            if (wParam == 1) { // 检查定时器 ID
                KillTimer(hwnd, 1); // 触发后立刻销毁定时器，只执行一次

                if (g_hChild) {
                    // 【核心步骤 1】：将父窗口设置为 NULL (NULL 即代表桌面窗口)
                    SetParent(g_hChild, NULL);

                    // 【核心步骤 2】：修改窗口样式
                    // 必须移除 WS_CHILD，并添加顶级窗口的样式（标题栏、边框等）
                    LONG_PTR style = GetWindowLongPtrW(g_hChild, GWL_STYLE);
                    style &= ~WS_CHILD;                 // 移除子窗口样式
                    style |= WS_OVERLAPPEDWINDOW;       // 添加标准的独立窗口样式
                    SetWindowLongPtrW(g_hChild, GWL_STYLE, style);

                    // 【核心步骤 3】：刷新窗口状态与位置
                    // SWP_FRAMECHANGED 会通知系统重新计算非客户区（即渲染出标题栏和边框）
                    SetWindowPos(g_hChild, HWND_TOP, 
                                 100, 100, 400, 300,    // 此时的坐标变为相对于屏幕(桌面)的坐标
                                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);

                    // 强制子窗口重新绘制（更新内部文本提示）
                    InvalidateRect(g_hChild, NULL, TRUE);
                }
            }
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            DrawTextW(hdc, L"parent window\nwaiting for 10 seconds...", -1, &rc, DT_CENTER | DT_TOP);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0); // 主窗口销毁时退出应用程序
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 程序入口
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 注册主窗口类
    WNDCLASSW wcParent = {0};
    wcParent.lpfnWndProc = ParentWndProc;
    wcParent.hInstance = hInstance;
    wcParent.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcParent.lpszClassName = L"ParentWindowClass";
    RegisterClassW(&wcParent);

    // 注册子窗口类
    WNDCLASSW wcChild = {0};
    wcChild.lpfnWndProc = ChildWndProc;
    wcChild.hInstance = hInstance;
    wcChild.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcChild.lpszClassName = L"ChildWindowClass";
    RegisterClassW(&wcChild);

    // 创建主窗口
    HWND hwndParent = CreateWindowExW(
        0, L"ParentWindowClass", L"Win32 SetParent Demo",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
        NULL, NULL, hInstance, NULL
    );

    if (hwndParent == NULL) return 0;

    ShowWindow(hwndParent, nCmdShow);
    UpdateWindow(hwndParent);

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
