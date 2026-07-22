// main.cpp  
// 编译: i686-w64-mingw32-g++ -Wall -O2 -municode -static-libgcc -static-libstdc++ main.cpp -mwindows -o LayeredChildWindow.exe  
  
#define UNICODE  
#define _UNICODE  
#include <windows.h>  
  
#define MAIN_CLASS   L"MainWindowClass_v2"  
#define CHILD_CLASS  L"LayeredChildClass_v2"  
  
#define CHILD_X      60  
#define CHILD_Y      60  
#define CHILD_W     200  
#define CHILD_H     200  
#define SQUARE_SIZE  80  
  
// 洋红色作为透明色键（不会与红色方块冲突）  
#define COLORKEY     RGB(255, 0, 255)  
  
static HWND g_hChild = NULL;  
  
// ---------------------------------------------------------------  
// 子窗口过程：用 WM_PAINT 绘制内容，背景用 color key 颜色  
// ---------------------------------------------------------------  
static LRESULT CALLBACK ChildWndProc(HWND hwnd, UINT msg,  
                                     WPARAM wParam, LPARAM lParam)  
{  
    switch (msg)  
    {  
    case WM_ERASEBKGND:  
    {  
        HDC hdc = (HDC)wParam;  
        RECT rc;  
        GetClientRect(hwnd, &rc);  
        HBRUSH hBr = CreateSolidBrush(COLORKEY);  
        FillRect(hdc, &rc, hBr);  
        DeleteObject(hBr);  
        return 1;  // 告诉系统背景已处理  
    }  
    case WM_PAINT:  
    {  
        PAINTSTRUCT ps;  
        HDC hdc = BeginPaint(hwnd, &ps);  
        RECT rc;  
        GetClientRect(hwnd, &rc);  
  
        // 背景填充 color key（将被透明化）  
        HBRUSH hBrBg = CreateSolidBrush(COLORKEY);  
        FillRect(hdc, &rc, hBrBg);  
        DeleteObject(hBrBg);  
  
        // 中心红色方块  
        int x0 = (rc.right  - SQUARE_SIZE) / 2;  
        int y0 = (rc.bottom - SQUARE_SIZE) / 2;  
        RECT sq = { x0, y0, x0 + SQUARE_SIZE, y0 + SQUARE_SIZE };  
        HBRUSH hBrRed = CreateSolidBrush(RGB(255, 0, 0));  
        FillRect(hdc, &sq, hBrRed);  
        DeleteObject(hBrRed);  
  
        EndPaint(hwnd, &ps);  
        return 0;  
    }  
    }  
    return DefWindowProcW(hwnd, msg, wParam, lParam);  
}  
  
// ---------------------------------------------------------------  
// 主窗口过程  
// ---------------------------------------------------------------  
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg,  
                                    WPARAM wParam, LPARAM lParam)  
{  
    switch (msg)  
    {  
    case WM_CREATE:  
    {  
        // 1. 先不带 WS_EX_LAYERED 创建子窗口  
        g_hChild = CreateWindowExW(  
            0,                          // ← 不在这里加 WS_EX_LAYERED  
            CHILD_CLASS, NULL,  
            WS_CHILD | WS_VISIBLE,  
            CHILD_X, CHILD_Y, CHILD_W, CHILD_H,  
            hwnd, NULL, GetModuleHandleW(NULL), NULL);  
    
        if (!g_hChild)  
        {  
            wchar_t buf[128];  
            wsprintfW(buf, L"CreateWindowExW failed: %lu", GetLastError());  
            MessageBoxW(hwnd, buf, L"Error", MB_OK | MB_ICONERROR);  
            return -1;  
        }  
    
        // 2. 创建成功后，追加 WS_EX_LAYERED  
        LONG_PTR exStyle = GetWindowLongPtrW(g_hChild, GWL_EXSTYLE);  
        SetWindowLongPtrW(g_hChild, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);  
    
        // 3. 设置 color key  
        SetLayeredWindowAttributes(g_hChild, COLORKEY, 0, LWA_COLORKEY);  
        return 0;   
    }  
  
    case WM_PAINT:  
    {  
        // 条纹背景，用于观察子窗口透明效果  
        PAINTSTRUCT ps;  
        HDC hdc = BeginPaint(hwnd, &ps);  
        RECT rc;  
        GetClientRect(hwnd, &rc);  
        for (int y = 0; y < rc.bottom; y += 20)  
        {  
            COLORREF c = ((y / 20) % 2) ? RGB(180, 210, 255)  
                                        : RGB(220, 235, 255);  
            HBRUSH hBr = CreateSolidBrush(c);  
            RECT band = { 0, y, rc.right, y + 20 };  
            FillRect(hdc, &band, hBr);  
            DeleteObject(hBr);  
        }  
        EndPaint(hwnd, &ps);  
        return 0;  
    }  
  
    case WM_DESTROY:  
        PostQuitMessage(0);  
        return 0;  
    }  
    return DefWindowProcW(hwnd, msg, wParam, lParam);  
}  
  
// ---------------------------------------------------------------  
// 入口（-municode 需要 wWinMain）  
// ---------------------------------------------------------------  
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev,  
                    PWSTR lpCmd, int nShow)  
{  
    // 注册主窗口类  
    WNDCLASSEXW wcMain = {};  
    wcMain.cbSize        = sizeof(wcMain);  
    wcMain.style         = CS_HREDRAW | CS_VREDRAW;  
    wcMain.lpfnWndProc   = MainWndProc;  
    wcMain.hInstance     = hInst;  
    wcMain.hCursor       = LoadCursorW(NULL, IDC_ARROW);  
    wcMain.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);  
    wcMain.lpszClassName = MAIN_CLASS;  
    if (!RegisterClassExW(&wcMain))  
    {  
        MessageBoxW(NULL, L"RegisterClassExW (main) failed", L"Error", MB_OK);  
        return 1;  
    }  
  
    // 注册子窗口类（独立结构体，避免字段污染）  
    WNDCLASSEXW wcChild = {};  
    wcChild.cbSize        = sizeof(wcChild);  
    wcChild.style         = 0;  
    wcChild.lpfnWndProc   = ChildWndProc;  
    wcChild.hInstance     = hInst;  
    wcChild.hCursor       = LoadCursorW(NULL, IDC_ARROW);  
    wcChild.hbrBackground = NULL;   // 不设背景画刷  
    wcChild.lpszClassName = CHILD_CLASS;  
    if (!RegisterClassExW(&wcChild))  
    {  
        MessageBoxW(NULL, L"RegisterClassExW (child) failed", L"Error", MB_OK);  
        return 1;  
    }  
  
    // 创建主窗口，必须加 WS_CLIPCHILDREN，否则父窗口 WM_PAINT 会覆盖子窗口  
    HWND hMain = CreateWindowExW(  
        0, MAIN_CLASS, L"Layered Child Window Demo",  
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,   // ← 关键  
        CW_USEDEFAULT, CW_USEDEFAULT, 420, 360,  
        NULL, NULL, hInst, NULL);  
  
    if (!hMain)  
    {  
        MessageBoxW(NULL, L"CreateWindowExW (main) failed", L"Error", MB_OK);  
        return 1;  
    }  
  
    ShowWindow(hMain, nShow);  
    UpdateWindow(hMain);  
  
    MSG msg;  
    while (GetMessageW(&msg, NULL, 0, 0))  
    {  
        TranslateMessage(&msg);  
        DispatchMessageW(&msg);  
    }  
    return (int)msg.wParam;  
}