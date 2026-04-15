#include <windows.h>

// 控件ID定义
#define IDC_BTN_FOCUS_TOOL 1001
#define IDC_BTN_FOCUS_MAIN 1002
#define IDC_EDIT_MAIN      1003

// 全局保存两个窗口的句柄，方便互相调用
HWND g_hwndMain = NULL;
HWND g_hwndTool = NULL;
HWND g_hwndEdit = NULL; 

// 窗口过程函数声明
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ToolWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wcMain = {0};
    WNDCLASS wcTool = {0};

    // ==========================================
    // 1. 注册主窗口类
    // ==========================================
    wcMain.lpfnWndProc   = MainWindowProc;
    wcMain.hInstance     = hInstance;
    wcMain.lpszClassName = TEXT("MainWindowClass");
    wcMain.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1); // 使用系统按钮颜色作为背景
    wcMain.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wcMain);

    // ==========================================
    // 2. 注册工具窗口类
    // ==========================================
    wcTool.lpfnWndProc   = ToolWindowProc;
    wcTool.hInstance     = hInstance;
    wcTool.lpszClassName = TEXT("ToolWindowClass");
    wcTool.hbrBackground = (HBRUSH)(COLOR_INFOBK + 1); // 使用淡黄色背景区分
    wcTool.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wcTool);

    // ==========================================
    // 3. 创建两个独立窗口
    // ==========================================
    g_hwndMain = CreateWindow(
        TEXT("MainWindowClass"), TEXT("Main Control Window"),
        WS_OVERLAPPEDWINDOW,
        100, 100, 400, 300,  // 位置和大小
        NULL, NULL, hInstance, NULL
    );

    g_hwndTool = CreateWindow(
        TEXT("ToolWindowClass"), TEXT("Tool Panel"),
        WS_OVERLAPPEDWINDOW,
        550, 150, 350, 200,  // 位置错开，方便观察
        NULL, NULL, hInstance, NULL
    );

    // 显示两个窗口
    ShowWindow(g_hwndMain, nCmdShow);
    UpdateWindow(g_hwndMain);
    ShowWindow(g_hwndTool, SW_SHOWNA); // 初始显示工具窗口但不抢主窗口焦点
    UpdateWindow(g_hwndTool);

    // ==========================================
    // 4. 消息循环 (同一线程管理两个窗口)
    // ==========================================
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// ==========================================
// 主窗口的消息处理
// ==========================================
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            g_hwndEdit = CreateWindow(
                "EDIT", "", 
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                50, 30, 250, 25,   // 放在按钮上方
                hwnd, (HMENU)IDC_EDIT_MAIN,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
            );
            // 在主窗口中创建一个按钮
            CreateWindow(
                TEXT("BUTTON"), TEXT("Bring Tool Panel to Front!"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                50, 80, 250, 50,
                hwnd, (HMENU)IDC_BTN_FOCUS_TOOL,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
            );
            return 0;
        case WM_SETFOCUS:
            // 不管是从工具窗口通过 SetForegroundWindow 唤醒的，
            // 还是用户直接用鼠标点击了主窗口的空白处，或者是用 Alt+Tab 切回来的，
            // 只要主窗口拿到了焦点，就立刻把焦点接力传递给内部的 Edit 控件！
            if (g_hwndEdit != NULL) {
                SetFocus(g_hwndEdit);
            }
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BTN_FOCUS_TOOL) {
                // 【核心逻辑】：主窗口呼叫工具窗口
                if (g_hwndTool != NULL && IsWindow(g_hwndTool)) {
                    // 第一步：如果工具窗口被最小化了，必须先恢复它
                    ShowWindow(g_hwndTool, SW_RESTORE);
                    // 第二步：将其带到前台，并给予键盘/鼠标焦点
                    SetForegroundWindow(g_hwndTool);
                }
            }
            return 0;

        case WM_DESTROY:
            // 关闭主窗口时退出程序
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ==========================================
// 工具窗口的消息处理
// ==========================================
LRESULT CALLBACK ToolWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            // 在工具窗口中创建一个按钮
            CreateWindow(
                TEXT("BUTTON"), TEXT("Back to Main Control"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                50, 50, 200, 40,
                hwnd, (HMENU)IDC_BTN_FOCUS_MAIN,
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
            );
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BTN_FOCUS_MAIN) {
                // 【核心逻辑】：工具窗口呼叫主窗口
                if (g_hwndMain != NULL && IsWindow(g_hwndMain)) {
                    // 第一步：恢复状态（防最小化）
                    ShowWindow(g_hwndMain, SW_RESTORE);
                    // 第二步：赋予前台焦点
                    SetForegroundWindow(g_hwndMain);
                }
            }
            return 0;

        case WM_CLOSE:
            // 为了演示，点击工具窗口的 X 只是隐藏它，而不是销毁它
            ShowWindow(hwnd, SW_HIDE);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
