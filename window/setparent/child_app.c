#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_ERASEBKGND:
    {
        printf("WM_ERASEBKGND\n");
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);

        HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 0));
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);

        return 1; // 表示已处理
    }

    case WM_DESTROY:
    {
        printf("WM_DESTROY\n");
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: ./child_app.exe <Parent_HWND>\n");
        return -1;
    }

    // 从命令行参数获取进程 A 的 HWND
    HWND hwndParent = (HWND)(UINT_PTR)strtoull(argv[1], NULL, 16);

    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ChildAppClass";
    wc.hbrBackground = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    // 创建子窗口
    HWND hwndChild = CreateWindow("ChildAppClass", "App B - Win32 Child",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  10, 20, 300, 200,
                                  NULL, NULL, hInstance, NULL);

    // 在 Win32 中建立跨进程的层级关系
    SetParent(hwndChild, hwndParent);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
