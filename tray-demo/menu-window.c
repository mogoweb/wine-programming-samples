/*
 * 托盘演示 - 自定义弹出菜单实现
 *
 * 不使用 TrackPopupMenu API，而是创建自定义窗口模拟弹出菜单
 */

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_SHOW_WINDOW 1001
#define ID_TRAY_EXIT 1002

#define MENU_WINDOW_CLASS L"PopupMenuWindow"

static const wchar_t *CLASS_NAME = L"TrayDemoWindow";
static const wchar_t *WINDOW_TITLE = L"托盘演示程序 - 自定义菜单";
static NOTIFYICONDATA nid = {0};
static HWND g_hwnd = NULL;
static HWND g_menuHwnd = NULL;
static HFONT g_hFont = NULL;
static HFONT g_hMenuFont = NULL;

typedef struct {
    wchar_t *text;
    UINT id;
    int height;
    int width;
} MenuItemInfo;

static MenuItemInfo menuItems[] = {
    { L"显示主窗口", ID_TRAY_SHOW_WINDOW, 30, 120 },
    { NULL, 0, 5, 120 },           /* separator */
    { L"退出", ID_TRAY_EXIT, 30, 120 }
};
#define MENU_ITEM_COUNT (sizeof(menuItems) / sizeof(menuItems[0]))

/* 菜单窗口状态 */
static int g_hoverIndex = -1;
static int g_menuWidth = 140;
static int g_menuHeight = 65;

static void CreateFonts(void)
{
    LOGFONT lf = {0};
    lf.lfHeight = -16;
    lf.lfWeight = FW_NORMAL;
    wcscpy(lf.lfFaceName, L"WenQuanYi Micro Hei");

    g_hFont = CreateFontIndirect(&lf);

    lf.lfHeight = -14;
    g_hMenuFont = CreateFontIndirect(&lf);
}

static void CreateTrayIcon(HWND hwnd)
{
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy(nid.szTip, L"托盘演示程序 - 自定义菜单");

    Shell_NotifyIcon(NIM_ADD, &nid);
}

static void RemoveTrayIcon(void)
{
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

/* 计算菜单项总高度 */
static void CalculateMenuSize(void)
{
    if (!g_hMenuFont) return;

    HDC hdc = GetDC(NULL);
    HFONT hOldFont = SelectObject(hdc, g_hMenuFont);

    g_menuHeight = 0;
    g_menuWidth = 140;

    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        if (menuItems[i].text) {
            SIZE size;
            GetTextExtentPoint32(hdc, menuItems[i].text, wcslen(menuItems[i].text), &size);
            menuItems[i].width = size.cx + 30;
            menuItems[i].height = size.cy + 12;
            if (menuItems[i].width > g_menuWidth) {
                g_menuWidth = menuItems[i].width;
            }
        }
        g_menuHeight += menuItems[i].height;
    }

    SelectObject(hdc, hOldFont);
    ReleaseDC(NULL, hdc);

    /* 添加边框 */
    g_menuWidth += 4;
    g_menuHeight += 4;
}

/* 获取菜单项在窗口中的 Y 坐标范围 */
static void CalcMenuItemRect(int index, RECT *rect)
{
    int y = 2;
    for (int i = 0; i < index; i++) {
        y += menuItems[i].height;
    }
    rect->left = 2;
    rect->right = g_menuWidth - 2;
    rect->top = y;
    rect->bottom = y + menuItems[index].height;
}

/* 根据 Y 坐标获取菜单项索引 */
static int GetMenuItemFromY(int y)
{
    int itemY = 2;
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        if (y >= itemY && y < itemY + menuItems[i].height) {
            return i;
        }
        itemY += menuItems[i].height;
    }
    return -1;
}

/* 绘制菜单窗口 */
static void DrawMenuWindow(HWND hwnd, HDC hdc)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    /* 背景 */
    HBRUSH hBgBrush = CreateSolidBrush(GetSysColor(COLOR_MENU));
    FillRect(hdc, &clientRect, hBgBrush);
    DeleteObject(hBgBrush);

    /* 边框 */
    HPEN hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME));
    HPEN hOldPen = SelectObject(hdc, hPen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, 0, 0, clientRect.right, clientRect.bottom);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    /* 绘制菜单项 */
    HFONT hOldFont = SelectObject(hdc, g_hMenuFont);

    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        RECT itemRect;
        CalcMenuItemRect(i, &itemRect);

        if (menuItems[i].text == NULL) {
            /* 分隔线 */
            HPEN hSepPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_GRAYTEXT));
            HPEN hOldSepPen = SelectObject(hdc, hSepPen);
            MoveToEx(hdc, itemRect.left + 5, itemRect.top + itemRect.bottom / 2, NULL);
            LineTo(hdc, itemRect.right - 5, itemRect.top + itemRect.bottom / 2);
            SelectObject(hdc, hOldSepPen);
            DeleteObject(hSepPen);
        } else {
            /* 悬停高亮 */
            if (i == g_hoverIndex) {
                HBRUSH hHighlightBrush = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
                FillRect(hdc, &itemRect, hHighlightBrush);
                DeleteObject(hHighlightBrush);

                SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
            } else {
                SetTextColor(hdc, GetSysColor(COLOR_MENUTEXT));
            }

            SetBkMode(hdc, TRANSPARENT);

            RECT textRect = itemRect;
            textRect.left += 10;
            DrawText(hdc, menuItems[i].text, wcslen(menuItems[i].text), &textRect,
                     DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
    }

    SelectObject(hdc, hOldFont);
}

/* 菜单窗口过程 */
static LRESULT CALLBACK MenuWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawMenuWindow(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        int y = HIWORD(lParam);
        int newHover = GetMenuItemFromY(y);

        /* 跳过分隔线 */
        if (newHover >= 0 && menuItems[newHover].text == NULL) {
            newHover = -1;
        }

        if (newHover != g_hoverIndex) {
            g_hoverIndex = newHover;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSELEAVE:
        g_hoverIndex = -1;
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    {
        int y = HIWORD(lParam);
        int index = GetMenuItemFromY(y);

        if (index >= 0 && menuItems[index].text != NULL) {
            UINT cmdId = menuItems[index].id;
            ShowWindow(hwnd, SW_HIDE);
            g_menuHwnd = NULL;
            /* 发送命令到主窗口 */
            PostMessage(g_hwnd, WM_COMMAND, cmdId, 0);
        }
        return 0;
    }

    case WM_KILLFOCUS:
        ShowWindow(hwnd, SW_HIDE);
        g_menuHwnd = NULL;
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            ShowWindow(hwnd, SW_HIDE);
            g_menuHwnd = NULL;
        }
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/* 注册菜单窗口类 */
static void RegisterMenuWindowClass(HINSTANCE hInstance)
{
    WNDCLASS wc = {0};
    wc.lpfnWndProc = MenuWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = MENU_WINDOW_CLASS;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;

    RegisterClass(&wc);
}

/* 创建并显示菜单窗口 */
static void ShowCustomMenu(HWND parent, POINT *pt)
{
    if (g_menuHwnd && IsWindowVisible(g_menuHwnd)) {
        ShowWindow(g_menuHwnd, SW_HIDE);
        return;
    }

    CalculateMenuSize();

    if (!g_menuHwnd) {
        g_menuHwnd = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            MENU_WINDOW_CLASS,
            NULL,
            WS_POPUP,
            pt->x, pt->y,
            g_menuWidth, g_menuHeight,
            parent,
            NULL,
            (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE),
            NULL
        );
        fprintf(stderr, "hwnd: %p, menu hwnd: %p\n", parent, g_menuHwnd);
    } else {
        SetWindowPos(g_menuHwnd, NULL, pt->x, pt->y, g_menuWidth, g_menuHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }

    g_hoverIndex = -1;

    /* 确保菜单在屏幕可见范围内 */
    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

    int x = pt->x;
    int y = pt->y;

    if (x + g_menuWidth > workArea.right) {
        x = workArea.right - g_menuWidth;
    }
    if (y + g_menuHeight > workArea.bottom) {
        y = workArea.bottom - g_menuHeight;
    }

    SetWindowPos(g_menuHwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    ShowWindow(g_menuHwnd, SW_SHOWNOACTIVATE);
    SetForegroundWindow(g_menuHwnd);
    SetFocus(g_menuHwnd);

    /* 设置鼠标捕获，点击菜单外关闭 */
    SetCapture(g_menuHwnd);
}

static void ShowContextMenu(HWND hwnd)
{
    POINT pt;
    GetCursorPos(&pt);

    SetForegroundWindow(hwnd);
    fprintf(stderr, "[Custom Menu]hwnd=%p, pt.x = %ld, pt.y = %ld\n", hwnd, pt.x, pt.y);

    ShowCustomMenu(hwnd, &pt);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        return (INT_PTR)GetStockObject(WHITE_BRUSH);
    }

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_TRAYICON:
        if (LOWORD(lParam) == WM_RBUTTONUP) {
            ShowContextMenu(hwnd);
        } else if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_SHOW_WINDOW:
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
            break;
        case ID_TRAY_EXIT:
            RemoveTrayIcon();
            PostQuitMessage(0);
            break;
        }
        return 0;

    case WM_DESTROY:
        if (g_menuHwnd) {
            DestroyWindow(g_menuHwnd);
            g_menuHwnd = NULL;
        }
        RemoveTrayIcon();
        if (g_hFont) DeleteObject(g_hFont);
        if (g_hMenuFont) DeleteObject(g_hMenuFont);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    CreateFonts();
    RegisterMenuWindowClass(hInstance);

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    g_hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 300,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (g_hwnd == NULL) {
        return 0;
    }

    CreateTrayIcon(g_hwnd);

    ShowWindow(g_hwnd, nCmdShow);

    HWND hStatic = CreateWindowEx(
        0,
        L"STATIC",
        L"点击窗口关闭按钮将最小化到系统托盘\n\n"
        L"右键点击托盘图标可显示菜单\n"
        L"双击托盘图标可显示窗口\n\n"
        L"此版本使用自定义窗口实现弹出菜单，\n"
        L"不调用 TrackPopupMenu API",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        30, 60, 340, 120,
        g_hwnd,
        NULL,
        hInstance,
        NULL
    );

    if (g_hFont && hStatic) {
        SendMessage(hStatic, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
