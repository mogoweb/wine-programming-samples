#include <windows.h>
#include <stdio.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);

        HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 255));
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);

        return 1; // 表示已处理
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int main() {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ParentAppClass";
    wc.hbrBackground = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    HWND hwnd = CreateWindow("ParentAppClass", "App A - Win32 Parent",
                             WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             10, 20, 600, 400,
                             NULL, NULL, hInstance, NULL);

    // 打印 HWND，供进程 B 使用
    printf("please run at another terminal:\n");
    printf("./child_app.exe %p\n", hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
