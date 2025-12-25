#include <windows.h>
#include <windowsx.h>

#pragma comment(lib, "Msimg32.lib")

constexpr int WIN_WIDTH = 600;
constexpr int WIN_HEIGHT = 400;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 使用 UpdateLayeredWindow 更新窗口内容
void UpdateWindowBitmap(HWND hwnd)
{
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = WIN_WIDTH;
    bmi.bmiHeader.biHeight = -WIN_HEIGHT; // top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hBmp = CreateDIBSection(
        hdcScreen, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);

    HBITMAP oldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);

    // =========================
    // 填充背景（不透明）
    // =========================
    RECT rc = { 0, 0, WIN_WIDTH, WIN_HEIGHT };
    HBRUSH bg = CreateSolidBrush(RGB(60, 60, 60));
    FillRect(hdcMem, &rc, bg);
    DeleteObject(bg);

    // =========================
    // 中心圆：alpha = 0（真正透明）
    // =========================
    int cx = WIN_WIDTH / 2;
    int cy = WIN_HEIGHT / 2;
    int r = min(WIN_WIDTH, WIN_HEIGHT) / 4;

    DWORD* pixel = (DWORD*)bits;

    for (int y = 0; y < WIN_HEIGHT; ++y)
    {
        for (int x = 0; x < WIN_WIDTH; ++x)
        {
            int dx = x - cx;
            int dy = y - cy;
            if (dx * dx + dy * dy <= r * r)
            {
                // ARGB = 0x00RRGGBB → alpha = 0
                pixel[y * WIN_WIDTH + x] &= 0x00FFFFFF;
            }
            else
            {
                // alpha = 255
                pixel[y * WIN_WIDTH + x] |= 0xFF000000;
            }
        }
    }

    POINT ptDst = { 100, 100 };
    SIZE  size = { WIN_WIDTH, WIN_HEIGHT };
    POINT ptSrc = { 0, 0 };

    BLENDFUNCTION bf = {};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA;

    UpdateLayeredWindow(
        hwnd,
        hdcScreen,
        &ptDst,
        &size,
        hdcMem,
        &ptSrc,
        0,
        &bf,
        ULW_ALPHA
    );

    // 清理资源
    SelectObject(hdcMem, oldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
}

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int)
{
    const wchar_t CLASS_NAME[] = L"ULW_Demo_Window";

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED,
        CLASS_NAME,
        L"UpdateLayeredWindow Demo",
        WS_POPUP,           // 无边框，最清晰
        0, 0, 0, 0,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindowBitmap(hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
