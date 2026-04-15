#include <windows.h>
#include <stdio.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);

        HBRUSH hBrush = CreateSolidBrush(RGB(50, 150, 250));
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);

        return 1;
    }
    case WM_MOVE:
    {
        int x = (int)(short)LOWORD(lParam);
        int y = (int)(short)HIWORD(lParam);
        printf("Window moved to: (%d, %d)\n", x, y);
        return 0;
    }
    case WM_SIZE:
    {
        int w = (int)(short)LOWORD(lParam);
        int h = (int)(short)HIWORD(lParam);
        printf("Window resized to: %dx%d\n", w, h);
        return 0;
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
    wc.lpszClassName = "TargetWindowClass";
    wc.hbrBackground = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    HWND hwnd = CreateWindow("TargetWindowClass", "Target Window (被控制窗口)",
                             WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             100, 100, 400, 300,
                             NULL, NULL, hInstance, NULL);

    printf("=== Target Window ===\n");
    printf("HWND: %p\n", hwnd);
    printf("PID: %lu\n", GetCurrentProcessId());
    printf("\nRun controller to move this window:\n");
    printf("./controller.exe %p\n", hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
