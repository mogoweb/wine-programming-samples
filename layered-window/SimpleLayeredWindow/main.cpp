#include <windows.h>
#include <windowsx.h>

#pragma comment(lib, "Msimg32.lib")

constexpr COLORREF KEY_COLOR = RGB(255, 255, 0); // 黄色作为 ColorKey

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        // 设置 Layered Window + ColorKey
        SetLayeredWindowAttributes(
            hwnd,
            KEY_COLOR,
            0,
            LWA_COLORKEY
        );
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        // 1. 填充背景（深灰色，不透明）
        HBRUSH bg = CreateSolidBrush(RGB(60, 60, 60));
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);

        // 2. 计算中心圆
        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;
        int radius = min(width, height) / 4;

        int cx = width / 2;
        int cy = height / 2;

        // 3. 画一个“纯黄色”的圆（ColorKey 区域）
        HBRUSH circleBrush = CreateSolidBrush(KEY_COLOR);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, circleBrush);

        Ellipse(
            hdc,
            cx - radius,
            cy - radius,
            cx + radius,
            cy + radius
        );

        SelectObject(hdc, oldBrush);
        DeleteObject(circleBrush);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_NCHITTEST:
    {
        // 将屏幕坐标转为客户区坐标
        POINT pt = {
            GET_X_LPARAM(lParam),
            GET_Y_LPARAM(lParam)
        };
        ScreenToClient(hwnd, &pt);

        RECT rc;
        GetClientRect(hwnd, &rc);

        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;

        int cx = width / 2;
        int cy = height / 2;
        int r = min(width, height) / 4;

        int dx = pt.x - cx;
        int dy = pt.y - cy;

        // 如果命中“视觉透明圆形区域”
        if (dx * dx + dy * dy <= r * r)
        {
            OutputDebugStringW(
                L"[LayeredDemo] Enter transparent area\n"
            );
            return HTTRANSPARENT; // 关键点：输入穿透
        }

        // 其余区域交给系统默认处理
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"ColorKeyDemoWindow";

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; // 我们自己绘制

    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED,            // 关键：Layered Window
        CLASS_NAME,
        L"LWA_COLORKEY Demo - Center Transparent Circle",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        600, 400,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
