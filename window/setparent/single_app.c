#include <windows.h>

HWND g_child = NULL;

const char* PARENT_CLASS = "ParentClass";
const char* CHILD_CLASS  = "ChildClass";

// 简单画背景 + 文本
void PaintWindow(HWND hwnd, HDC hdc, const char* text, COLORREF color) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, &rc, brush);
    DeleteObject(brush);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    DrawText(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

LRESULT CALLBACK ChildWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        PaintWindow(hwnd, hdc, "CHILD WINDOW", RGB(0, 180, 0)); // 绿色

        EndPaint(hwnd, &ps);
        break;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        // 创建第二个窗口（初始是 top-level）
        g_child = CreateWindowEx(
            0,
            CHILD_CLASS,
            "Child Window (before SetParent)",
            WS_OVERLAPPEDWINDOW,
            300, 200, 300, 200,
            NULL,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );

        ShowWindow(g_child, SW_SHOW);
        SetWindowPos(g_child, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

        // 延迟一点再 SetParent（方便观察变化）
        SetTimer(hwnd, 1, 10000, NULL); // 10秒后触发
        break;

    case WM_TIMER:
        if (wParam == 1 && g_child) {
            KillTimer(hwnd, 1);

            // ⭐ 核心：把第二个窗口设置为当前窗口的子窗口
            SetParent(g_child, hwnd);

            // 修改 style，确保它是 child window
            LONG style = GetWindowLong(g_child, GWL_STYLE);
            style &= ~WS_OVERLAPPEDWINDOW;
            style |= WS_CHILD | WS_VISIBLE;
            SetWindowLong(g_child, GWL_STYLE, style);

            // 重新设置位置（现在是相对父窗口）
            SetWindowPos(g_child, NULL,
                50, 50, 300, 200,
                SWP_NOZORDER | SWP_FRAMECHANGED);

            SetWindowText(g_child, "Child Window (after SetParent)");
        }
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        PaintWindow(hwnd, hdc, "PARENT WINDOW", RGB(200, 0, 0)); // 红色

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int main() {
    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = PARENT_CLASS;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    // 注册子窗口类
    WNDCLASS wc2 = {0};
    wc2.lpfnWndProc = ChildWndProc;
    wc2.hInstance = hInstance;
    wc2.lpszClassName = CHILD_CLASS;
    wc2.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc2);

    // 主窗口
    HWND hwnd = CreateWindowEx(
        0,
        PARENT_CLASS,
        "Parent Window",
        WS_OVERLAPPEDWINDOW,
        100, 100, 600, 400,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    ShowWindow(hwnd, SW_SHOW);

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
