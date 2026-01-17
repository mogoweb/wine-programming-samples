#include <windows.h>
#include <windowsx.h>

#pragma comment(lib, "Msimg32.lib")

constexpr int SHADOW_SIZE = 20;

HWND g_mainWnd = nullptr;
HWND g_shadowWnd = nullptr;
HWND g_button = nullptr;

/*-----------------------------------------
  绘制阴影 Layered Window
-----------------------------------------*/
void UpdateShadow(HWND shadow, RECT rcMain)
{
    int width = (rcMain.right - rcMain.left) + SHADOW_SIZE * 2;
    int height = (rcMain.bottom - rcMain.top) + SHADOW_SIZE * 2;

    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hBmp = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    SelectObject(hdcMem, hBmp);

    memset(bits, 0, width * height * 4);

    // 简单阴影：边缘 alpha 递减
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int dx = min(x, width - x - 1);
            int dy = min(y, height - y - 1);
            int d = min(dx, dy);

            if (d < SHADOW_SIZE)
            {
                BYTE alpha = BYTE(150 * (SHADOW_SIZE - d) / SHADOW_SIZE);
                BYTE* p = (BYTE*)bits + (y * width + x) * 4;
                p[0] = 0;      // B
                p[1] = 0;      // G
                p[2] = 0;      // R
                p[3] = alpha; // A
            }
        }
    }

    POINT ptPos = { rcMain.left - SHADOW_SIZE, rcMain.top - SHADOW_SIZE };
    SIZE sizeWnd = { width, height };
    POINT ptSrc = { 0,0 };

    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

    UpdateLayeredWindow(
        shadow,
        hdcScreen,
        &ptPos,
        &sizeWnd,
        hdcMem,
        &ptSrc,
        0,
        &bf,
        ULW_ALPHA
    );

    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
}

/*-----------------------------------------
  主窗口过程
-----------------------------------------*/
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        g_button = CreateWindowW(
            L"BUTTON",
            L"Click Me",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            50, 50, 120, 40,
            hwnd,
            (HMENU)1,
            GetModuleHandle(nullptr),
            nullptr
        );
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == 1)
        {
            MessageBoxW(hwnd, L"Button clicked!", L"Info", MB_OK);
        }
        return 0;

    case WM_MOVE:
    case WM_SIZE:
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        UpdateShadow(g_shadowWnd, rc);
        return 0;
    }

    case WM_DESTROY:
        DestroyWindow(g_shadowWnd);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/*-----------------------------------------
  WinMain
-----------------------------------------*/
int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow)
{
    WNDCLASSW wc = {};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"MainWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    g_mainWnd = CreateWindowW(
        wc.lpszClassName,
        L"Layered Shadow Demo",
        WS_OVERLAPPEDWINDOW,
        300, 200, 400, 300,
        nullptr, nullptr, hInst, nullptr
    );

    // Shadow window
    g_shadowWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
        L"STATIC",
        nullptr,
        WS_POPUP,
        0, 0, 0, 0,
        nullptr,
        nullptr,
        hInst,
        nullptr
    );
    SetWindowLongPtr(
        g_shadowWnd,
        GWLP_HWNDPARENT,
        (LONG_PTR)g_mainWnd
    );

    ShowWindow(g_shadowWnd, SW_SHOWNA);
    ShowWindow(g_mainWnd, nCmdShow);

    RECT rc;
    GetWindowRect(g_mainWnd, &rc);
    UpdateShadow(g_shadowWnd, rc);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
