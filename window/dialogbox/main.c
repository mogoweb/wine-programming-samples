// main.c
#include <windows.h>
#include "resource.h"

// 宏定义窗口类名，完美绕过截断Bug
#define MY_WINDOW_CLASS "MyMainWindowClass"

// ==========================================
// 1. 模态对话框的消息处理回调函数
// ==========================================
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BTN_OK || LOWORD(wParam) == IDCANCEL) {
                // 关闭对话框并解除对主窗口的锁定
                EndDialog(hwndDlg, 0);
                return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwndDlg, 0);
            return TRUE;
    }
    return FALSE;
}

// ==========================================
// 2. 主窗口的消息处理函数
// ==========================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            // 主窗口创建时，在上面动态放一个按钮
            CreateWindow(
                "BUTTON", "Click to Show Modal Dialog",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                100, 80, 180, 40,       // 按钮的 x, y 坐标及宽高
                hwnd,                   // 父窗口是当前主窗口
                (HMENU)ID_BTN_SHOW_DLG, // 按钮 ID
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL);
            break;

        case WM_COMMAND:
            // 捕获按钮的点击事件
            if (LOWORD(wParam) == ID_BTN_SHOW_DLG) {
                // 【核心】：调用 DialogBox 弹出模态对话框
                // 参数 3: 传入 hwnd (主窗口句柄)。指定了父窗口后，
                // 弹出的对话框才会变为模态，强制锁定主窗口无法点击。
                DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MAINDIALOG), hwnd, DialogProc);
            }
            break;

        case WM_DESTROY:
            // 点击右上角X关闭主窗口，退出整个程序
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ==========================================
// 3. 程序的入口函数 WinMain
// ==========================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    // 注册窗口类
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = MY_WINDOW_CLASS; 
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // 创建主窗口
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        MY_WINDOW_CLASS,
        "Windows Main Window Test",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL, NULL, hInstance, NULL);

    if(hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // 显示并更新主窗口
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 消息循环 (Message Loop)
    while(GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
