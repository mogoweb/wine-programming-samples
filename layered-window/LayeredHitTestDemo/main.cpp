// main.cpp
// 验证 Layered Window（分层窗口）透明区域鼠标事件的传递机制
//
// 核心问题
// --------
// layered 窗口部分区域透明，这些透明区域的鼠标事件，是直接透传到
// 下方的窗口，还是通过消息转发传给下层窗口？
//
// 结论（取决于“透明”的实现方式）
// --------
// 1) UpdateLayeredWindow + ULW_ALPHA（逐像素 Alpha）
//    alpha = 0 的像素，系统在命中测试（hit-test）阶段就跳过本窗口，
//    鼠标事件“直接透传”给下方窗口。本窗口的窗口过程收不到任何
//    鼠标消息——连 WM_NCHITTEST 都收不到。
//
// 2) SetLayeredWindowAttributes + LWA_COLORKEY（色键透明）
//    色键区域只是“视觉上透明”，鼠标仍然命中本窗口，窗口过程会
//    收到 WM_NCHITTEST；需要自行判断并返回 HTTRANSPARENT，系统才会
//    继续沿 Z 序向下查找下一个窗口——这属于“链式命中测试/消息转发”。
//
// 验证方法
// --------
// 程序创建两个窗口，**两个窗口都监听同一组鼠标事件**：
//   - Bottom：底层普通窗口
//   - Overlay：顶层分层窗口，覆盖在 Bottom 的客户区之上，
//     左半不透明、右半透明
// 屏幕右半分两栏实时显示两个窗口各自收到的事件流（含 NCHITTEST 的
// 返回值）。按空格在两种透明方式间切换：
//   - 逐像素 Alpha 模式：鼠标在右半（透明区）移动——Overlay 事件流
//     静止（收不到任何消息），Bottom 事件流滚动（直接收到）=> 直接透传
//   - 色键模式：鼠标在右半（色键透明区）移动——Overlay 事件流出现
//     NCHITTEST->HTTRANSPARENT，随后 Bottom 事件流才滚动 => 消息转发
//
// 编译 (MinGW-w64 交叉编译):
//   make          # 默认 32 位
//   make ARCH=x86_64

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <windowsx.h>
#include <stdarg.h>
#include <stdio.h>

// ---- 常量 ----
constexpr int  CLIENT_W = 800;
constexpr int  CLIENT_H = 500;
constexpr COLORREF KEY_COLOR = RGB(255, 0, 255);   // 色键：品红

#define BOTTOM_CLASS  L"LayeredHitTest_Bottom"
#define OVERLAY_CLASS L"LayeredHitTest_Overlay"

// ---- 透明方式 ----
enum TransparencyMode {
    MODE_PER_PIXEL_ALPHA,   // UpdateLayeredWindow + ULW_ALPHA
    MODE_COLORKEY           // SetLayeredWindowAttributes + LWA_COLORKEY
};
static TransparencyMode g_mode = MODE_PER_PIXEL_ALPHA;

// ---- 全局窗口句柄 ----
static HWND g_hBottom  = NULL;
static HWND g_hOverlay = NULL;

// ---- 事件流（环形缓冲）----
constexpr int HISTORY_MAX = 8;

struct EventItem {
    wchar_t name[40];   // 事件名（NCHITTEST 带 ->返回值）
    POINT   pt;         // 客户区坐标
    DWORD   tick;       // GetTickCount
};

struct EventLog {
    EventItem items[HISTORY_MAX];
    int head;           // 下一个写入位置
    int count;          // 已记录条数
};

static EventLog g_overlayLog = {};
static EventLog g_bottomLog  = {};

static void PushEvent(EventLog* log, const wchar_t* name, POINT pt) {
    EventItem* it = &log->items[log->head];
    wcscpy_s(it->name, 40, name);
    it->pt   = pt;
    it->tick = GetTickCount();
    log->head = (log->head + 1) % HISTORY_MAX;
    if (log->count < HISTORY_MAX) log->count++;
}

// ---- 刷新节流，避免鼠标移动刷屏导致重绘风暴 ----
static DWORD g_lastRefreshTick = 0;
static void RequestRefresh() {
    DWORD now = GetTickCount();
    if (now - g_lastRefreshTick > 16) {   // ~60fps
        g_lastRefreshTick = now;
        if (g_hBottom) InvalidateRect(g_hBottom, NULL, FALSE);
    }
}

// ---- 工具 ----
static void DebugLog(const wchar_t* fmt, ...) {
    wchar_t buf[256];
    va_list args; va_start(args, fmt);
    _vsnwprintf(buf, 255, fmt, args);
    va_end(args);
    int len = (int)wcslen(buf);
    if (len < 255 - 1) { buf[len] = L'\n'; buf[len + 1] = 0; }
    OutputDebugStringW(buf);
}

static HFONT MakePropFont(int size, int weight) {
    return CreateFontW(size, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");
}

static HFONT MakeMonoFont(int size) {
    return CreateFontW(size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
}

static const wchar_t* ModeName() {
    return (g_mode == MODE_PER_PIXEL_ALPHA)
        ? L"逐像素 Alpha (UpdateLayeredWindow + ULW_ALPHA)"
        : L"色键 (SetLayeredWindowAttributes + LWA_COLORKEY)";
}

static void HitTestName(int r, wchar_t* out) {
    switch (r) {
    case HTCLIENT:      wcscpy_s(out, 24, L"HTCLIENT");      break;
    case HTTRANSPARENT: wcscpy_s(out, 24, L"HTTRANSPARENT"); break;
    case HTCAPTION:     wcscpy_s(out, 24, L"HTCAPTION");     break;
    case HTNOWHERE:     wcscpy_s(out, 24, L"HTNOWHERE");     break;
    default:            wsprintfW(out, L"%d", r);            break;
    }
}

// 记录一条命中测试事件（带返回值）
static void LogHitTest(EventLog* log, const wchar_t* who, POINT pt, int r) {
    wchar_t rn[24]; HitTestName(r, rn);
    wchar_t name[40];
    wsprintfW(name, L"NCHITTEST->%s", rn);
    PushEvent(log, name, pt);
    DebugLog(L"[%s] NCHITTEST pt=(%d,%d) -> %s", who, pt.x, pt.y, rn);
}

// ---- 绘制 Overlay（逐像素 Alpha 模式）----
static void UpdateOverlayBitmap(HWND hwnd, POINT dst) {
    HDC screen = GetDC(NULL);
    HDC mem = CreateCompatibleDC(screen);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = CLIENT_W;
    bmi.bmiHeader.biHeight = -CLIENT_H;   // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = NULL;
    HBITMAP bmp = CreateDIBSection(screen, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

    // 左半：不透明深色
    RECT left = { 0, 0, CLIENT_W / 2, CLIENT_H };
    HBRUSH b1 = CreateSolidBrush(RGB(42, 42, 42));
    FillRect(mem, &left, b1);
    DeleteObject(b1);

    // 左半说明文字
    SetBkMode(mem, TRANSPARENT);
    SetTextColor(mem, RGB(255, 255, 255));
    HFONT f = MakePropFont(22, FW_BOLD);
    HFONT oldf = (HFONT)SelectObject(mem, f);
    RECT txtRc = { 20, 24, CLIENT_W / 2 - 20, 200 };
    DrawTextW(mem,
        L"Overlay 不透明区域（左半）\n"
        L"鼠标命中 Overlay\n"
        L"(逐像素 Alpha: alpha=255)",
        -1, &txtRc, DT_LEFT | DT_WORDBREAK);
    SelectObject(mem, oldf);
    DeleteObject(f);

    // 修正 alpha 通道：左半 alpha=255，右半 alpha=0（完全透明）
    // 系统对 alpha=0 的像素直接跳过命中测试 -> 鼠标事件直接透传
    DWORD* px = (DWORD*)bits;
    for (int y = 0; y < CLIENT_H; ++y) {
        for (int x = 0; x < CLIENT_W; ++x) {
            if (x < CLIENT_W / 2)
                px[y * CLIENT_W + x] |= 0xFF000000;
            else
                px[y * CLIENT_W + x] = 0x00000000;
        }
    }

    SIZE sz = { CLIENT_W, CLIENT_H };
    POINT src = { 0, 0 };
    BLENDFUNCTION bf = {};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA;
    UpdateLayeredWindow(hwnd, screen, &dst, &sz, mem, &src, 0, &bf, ULW_ALPHA);

    SelectObject(mem, old);
    DeleteObject(bmp);
    DeleteDC(mem);
    ReleaseDC(NULL, screen);
}

// ---- 同步 Overlay 位置/内容 ----
static void SyncOverlay() {
    if (!g_hOverlay) return;
    POINT origin = { 0, 0 };
    ClientToScreen(g_hBottom, &origin);
    if (g_mode == MODE_PER_PIXEL_ALPHA) {
        UpdateOverlayBitmap(g_hOverlay, origin);
    } else {
        MoveWindow(g_hOverlay, origin.x, origin.y, CLIENT_W, CLIENT_H, TRUE);
        InvalidateRect(g_hOverlay, NULL, TRUE);
    }
    InvalidateRect(g_hBottom, NULL, FALSE);
}

// ---- 切换透明方式 ----
static void SwitchMode(TransparencyMode m) {
    g_mode = m;
    if (m == MODE_PER_PIXEL_ALPHA) {
        DebugLog(L"[Mode] -> 逐像素 Alpha: 透明区域(alpha=0)由系统直接透传");
    } else {
        DebugLog(L"[Mode] -> 色键: 透明区域需 WM_NCHITTEST 返回 HTTRANSPARENT 转发");
        SetLayeredWindowAttributes(g_hOverlay, KEY_COLOR, 0, LWA_COLORKEY);
    }
    SyncOverlay();
}

// 绘制一个事件流块（标题 + 逐行事件），返回结束 y
static int DrawEventLog(HDC hdc, int x, int y, const wchar_t* title,
                        COLORREF titleColor, const EventLog* log) {
    HFONT prop = MakePropFont(15, FW_BOLD);
    HFONT oldp = (HFONT)SelectObject(hdc, prop);
    SetTextColor(hdc, titleColor);
    TextOutW(hdc, x, y, title, (int)wcslen(title));
    y += 22;

    HFONT mono = MakeMonoFont(15);
    HFONT oldm = (HFONT)SelectObject(hdc, mono);
    SetTextColor(hdc, RGB(30, 30, 30));
    int n = log->count;
    if (n == 0) {
        TextOutW(hdc, x, y, L"  (尚无事件)", 9);
        y += 18;
    } else {
        for (int i = 0; i < n; ++i) {
            int idx = (log->head - 1 - i + HISTORY_MAX) % HISTORY_MAX;
            const EventItem* it = &log->items[idx];
            wchar_t line[80];
            wsprintfW(line, L" %s  (%d,%d)", it->name, it->pt.x, it->pt.y);
            // 最新一条高亮
            SetTextColor(hdc, i == 0 ? RGB(200, 30, 30) : RGB(30, 30, 30));
            TextOutW(hdc, x, y, line, (int)wcslen(line));
            y += 18;
        }
    }
    SelectObject(hdc, oldm);
    DeleteObject(mono);
    SelectObject(hdc, oldp);
    DeleteObject(prop);
    return y;
}

// ================= Overlay 窗口过程 =================
static LRESULT CALLBACK OverlayProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_NCHITTEST: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        ScreenToClient(hwnd, &pt);

        if (g_mode == MODE_COLORKEY && pt.x >= CLIENT_W / 2) {
            // 色键模式 + 右半（色键透明区）：返回 HTTRANSPARENT，
            // 让系统继续向下查找窗口——消息转发/链式命中测试
            LogHitTest(&g_overlayLog, L"Overlay", pt, HTTRANSPARENT);
            RequestRefresh();
            return HTTRANSPARENT;
        }
        // 逐像素 Alpha 模式：透明区域根本不会进入这里（系统直接透传）
        // 不透明区域命中 Overlay -> 默认 HTCLIENT
        LRESULT r = DefWindowProcW(hwnd, msg, wp, lp);
        LogHitTest(&g_overlayLog, L"Overlay", pt, (int)r);
        RequestRefresh();
        return r;
    }

    case WM_MOUSEMOVE: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        PushEvent(&g_overlayLog, L"MOUSEMOVE", pt);
        DebugLog(L"[Overlay] WM_MOUSEMOVE pt=(%d,%d)", pt.x, pt.y);
        RequestRefresh();
        return 0;
    }

    case WM_LBUTTONDOWN: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        PushEvent(&g_overlayLog, L"LBUTTONDOWN", pt);
        DebugLog(L"[Overlay] WM_LBUTTONDOWN pt=(%d,%d)", pt.x, pt.y);
        RequestRefresh();
        return 0;
    }

    case WM_RBUTTONDOWN: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        PushEvent(&g_overlayLog, L"RBUTTONDOWN", pt);
        DebugLog(L"[Overlay] WM_RBUTTONDOWN pt=(%d,%d)", pt.x, pt.y);
        RequestRefresh();
        return 0;
    }

    case WM_NCMOUSEMOVE: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        ScreenToClient(hwnd, &pt);
        PushEvent(&g_overlayLog, L"NCMOUSEMOVE", pt);
        RequestRefresh();
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    case WM_ERASEBKGND:
        return 1;   // 自绘，避免闪烁

    case WM_PAINT: {
        // 仅色键模式绘制（逐像素 Alpha 模式由 UpdateLayeredWindow 管理表面，
        // 系统不会发送 WM_PAINT）
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT left  = { 0, 0, CLIENT_W / 2, CLIENT_H };
        RECT right = { CLIENT_W / 2, 0, CLIENT_W, CLIENT_H };
        HBRUSH b1 = CreateSolidBrush(RGB(42, 42, 42));
        FillRect(hdc, &left, b1); DeleteObject(b1);
        HBRUSH b2 = CreateSolidBrush(KEY_COLOR);
        FillRect(hdc, &right, b2); DeleteObject(b2);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        HFONT f = MakePropFont(22, FW_BOLD);
        HFONT oldf = (HFONT)SelectObject(hdc, f);
        RECT txtRc = { 20, 24, CLIENT_W / 2 - 20, 200 };
        DrawTextW(hdc,
            L"Overlay 不透明区域（左半）\n"
            L"鼠标命中 Overlay\n"
            L"(色键: 此处非色键色)",
            -1, &txtRc, DT_LEFT | DT_WORDBREAK);
        SelectObject(hdc, oldf);
        DeleteObject(f);
        EndPaint(hwnd, &ps);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ================= Bottom 窗口过程 =================
static LRESULT CALLBACK BottomProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;   // 棋盘格由 WM_PAINT 整体绘制，避免闪烁

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);

        // 棋盘格背景（便于辨识透明区域）
        const int cell = 40;
        HBRUSH bA = CreateSolidBrush(RGB(235, 235, 235));
        HBRUSH bB = CreateSolidBrush(RGB(215, 215, 215));
        for (int y = 0; y < CLIENT_H; y += cell) {
            for (int x = 0; x < CLIENT_W; x += cell) {
                RECT c = { x, y, x + cell, y + cell };
                FillRect(hdc, &c, (((x / cell) + (y / cell)) & 1) ? bB : bA);
            }
        }
        DeleteObject(bA); DeleteObject(bB);
#if 0
        // 右半信息面板
        RECT bgRc   = { CLIENT_W / 2 + 6, 12, CLIENT_W - 12, CLIENT_H - 12 };
        RECT border = { bgRc.left - 1, bgRc.top - 1, bgRc.right + 1, bgRc.bottom + 1 };
        HBRUSH bd = CreateSolidBrush(RGB(60, 60, 60));
        FrameRect(hdc, &border, bd); DeleteObject(bd);
        HBRUSH bg = CreateSolidBrush(RGB(250, 250, 250));
        FillRect(hdc, &bgRc, bg); DeleteObject(bg);

        int x = bgRc.left + 14;
        int y = bgRc.top + 10;

        // 标题：模式
        SetBkMode(hdc, TRANSPARENT);
        HFONT prop16 = MakePropFont(16, FW_BOLD);
        HFONT oldp16 = (HFONT)SelectObject(hdc, prop16);
        SetTextColor(hdc, RGB(20, 20, 20));
        wchar_t title[96];
        wsprintfW(title, L"当前模式：%s", ModeName());
        TextOutW(hdc, x, y, title, (int)wcslen(title));
        y += 24;

        HFONT prop14 = MakePropFont(13, FW_NORMAL);
        HFONT oldp14 = (HFONT)SelectObject(hdc, prop14);
        SetTextColor(hdc, RGB(90, 90, 90));
        TextOutW(hdc, x, y,
            L"透明区(右半)鼠标直达底层窗口；点击右半 -> Bottom 收到事件",
            56);
        y += 22;
        SelectObject(hdc, oldp16);
        DeleteObject(prop16);
        SelectObject(hdc, oldp14);
        DeleteObject(prop14);

        // Overlay 事件流
        y += 6;
        y = DrawEventLog(hdc, x, y, L"── Overlay 事件（最新在上）──",
                         RGB(40, 80, 160), &g_overlayLog);

        // Bottom 事件流
        y += 10;
        y = DrawEventLog(hdc, x, y, L"── Bottom 事件（最新在上）──",
                         RGB(160, 40, 40), &g_bottomLog);

        // 操作提示
        HFONT prop13 = MakePropFont(13, FW_NORMAL);
        HFONT oldp13 = (HFONT)SelectObject(hdc, prop13);
        SetTextColor(hdc, RGB(110, 110, 110));
        TextOutW(hdc, x, bgRc.bottom - 26,
            L"操作: [空格] 切换透明方式   [ESC] 退出", 26);
        SelectObject(hdc, oldp13);
        DeleteObject(prop13);
#endif
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_NCHITTEST: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        ScreenToClient(hwnd, &pt);
        LRESULT r = DefWindowProcW(hwnd, msg, wp, lp);
        LogHitTest(&g_bottomLog, L"Bottom", pt, (int)r);
        RequestRefresh();
        return r;
    }

    case WM_MOUSEMOVE: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        PushEvent(&g_bottomLog, L"MOUSEMOVE", pt);
        DebugLog(L"[Bottom] WM_MOUSEMOVE pt=(%d,%d)", pt.x, pt.y);
        RequestRefresh();
        return 0;
    }

    case WM_LBUTTONDOWN: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        PushEvent(&g_bottomLog, L"LBUTTONDOWN", pt);
        DebugLog(L"[Bottom] WM_LBUTTONDOWN pt=(%d,%d)", pt.x, pt.y);
        RequestRefresh();
        return 0;
    }

    case WM_RBUTTONDOWN: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        PushEvent(&g_bottomLog, L"RBUTTONDOWN", pt);
        DebugLog(L"[Bottom] WM_RBUTTONDOWN pt=(%d,%d)", pt.x, pt.y);
        RequestRefresh();
        return 0;
    }

    case WM_NCMOUSEMOVE: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        ScreenToClient(hwnd, &pt);
        PushEvent(&g_bottomLog, L"NCMOUSEMOVE", pt);
        RequestRefresh();
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    case WM_KEYDOWN:
        if (wp == VK_SPACE) {
            SwitchMode(g_mode == MODE_PER_PIXEL_ALPHA ? MODE_COLORKEY
                                                       : MODE_PER_PIXEL_ALPHA);
        } else if (wp == VK_ESCAPE) {
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_MOVE:
        SyncOverlay();
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ================= 入口 =================
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nShow) {
    // 注册 Bottom 窗口类
    WNDCLASSEXW wcB = {};
    wcB.cbSize = sizeof(wcB);
    wcB.style = CS_HREDRAW | CS_VREDRAW;
    wcB.lpfnWndProc = BottomProc;
    wcB.hInstance = hInst;
    wcB.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcB.hbrBackground = NULL;
    wcB.lpszClassName = BOTTOM_CLASS;
    RegisterClassExW(&wcB);

    // 注册 Overlay 窗口类
    WNDCLASSEXW wcO = {};
    wcO.cbSize = sizeof(wcO);
    wcO.lpfnWndProc = OverlayProc;
    wcO.hInstance = hInst;
    wcO.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcO.hbrBackground = NULL;
    wcO.lpszClassName = OVERLAY_CLASS;
    RegisterClassExW(&wcO);

    // 按客户区 800x500 反算窗口尺寸
    RECT rc = { 0, 0, CLIENT_W, CLIENT_H };
    AdjustWindowRectEx(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE, 0);
    int ww = rc.right - rc.left;
    int wh = rc.bottom - rc.top;

    g_hBottom = CreateWindowExW(0, BOTTOM_CLASS,
        L"Layered 透明区域鼠标事件验证",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, ww, wh,
        NULL, NULL, hInst, NULL);
    if (!g_hBottom) return 1;

    POINT origin = { 0, 0 };
    ClientToScreen(g_hBottom, &origin);
    // Overlay 的 owner 设为 Bottom，保证 Overlay 永远在 Bottom 上方
    g_hOverlay = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_NOACTIVATE,   // 不抢焦点，键盘留给 Bottom
        OVERLAY_CLASS, L"Overlay",
        WS_POPUP,
        origin.x, origin.y, CLIENT_W, CLIENT_H,
        g_hBottom, NULL, hInst, NULL);
    if (!g_hOverlay) return 1;

    ShowWindow(g_hBottom, nShow);
    UpdateWindow(g_hBottom);

    SwitchMode(MODE_PER_PIXEL_ALPHA);             // 初始：逐像素 Alpha
    ShowWindow(g_hOverlay, SW_SHOWNOACTIVATE);
    SetFocus(g_hBottom);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
