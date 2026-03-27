// main.c
#define UNICODE
#define _UNICODE

#include <windows.h>
#include "resource.h"

#define MY_WINDOW_CLASS L"MyMainWindowClass"
#define MY_MODAL_CLASS  L"CustomModalClass"

// ==========================================
// 1. 资源对话框的消息处理回调函数 (用于方法2 和 方法3)
// ==========================================
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
            // 【方法3 专属逻辑】：如果 lParam 不为 0，说明有数据传过来
            if (lParam != 0) {
                const wchar_t* receivedMsg = (const wchar_t*)lParam;
                // 将传进来的字符串，设置到对话框的文本控件上
                SetDlgItemText(hwndDlg, IDC_TXT_MSG, receivedMsg);
            }
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
// 2. 纯手工模态窗口的处理函数 (用于方法4)
// ==========================================
LRESULT CALLBACK CustomModalWndProc(HWND hModal, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hModal, &ps);
            TextOut(hdc, 20, 20, L"Close me to unlock parent!", 26);
            EndPaint(hModal, &ps);
            break;
        }
        case WM_CLOSE:
            // 【核心】：关闭自己之前，必须先解除父窗口的锁定！
            EnableWindow(GetParent(hModal), TRUE);
            // 把焦点还给父窗口
            SetFocus(GetParent(hModal));
            // 销毁当前的自定义模态窗口
            DestroyWindow(hModal);
            return 0;
            
        default:
            return DefWindowProc(hModal, msg, wParam, lParam);
    }
    return 0;
}

// ==========================================
// 3. 主窗口的消息处理函数
// ==========================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

            // 1. 创建文本输入框
            HWND hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"Type something here...",
                WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
                50, 20, 280, 25, hwnd, (HMENU)IDC_EDIT_INPUT, NULL, NULL);

            // 2. 创建四个按钮
            HWND hBtn1 = CreateWindow(L"BUTTON", L"1. MessageBox Modal", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                50, 60, 280, 35, hwnd, (HMENU)ID_BTN_MSG_BOX, NULL, NULL);

            HWND hBtn2 = CreateWindow(L"BUTTON", L"2. DialogBox Modal", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                50, 105, 280, 35, hwnd, (HMENU)ID_BTN_DLG_BOX, NULL, NULL);

            HWND hBtn3 = CreateWindow(L"BUTTON", L"3. DialogBoxParam (Pass Text)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                50, 150, 280, 35, hwnd, (HMENU)ID_BTN_DLG_PARAM, NULL, NULL);

            HWND hBtn4 = CreateWindow(L"BUTTON", L"4. EnableWindow Manual Modal", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                50, 195, 280, 35, hwnd, (HMENU)ID_BTN_MANUAL, NULL, NULL);

            // 设置字体
            SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
            SendMessage(hBtn1, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
            SendMessage(hBtn2, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
            SendMessage(hBtn3, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
            SendMessage(hBtn4, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
            break;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            
            // 获取输入框里的文本，存入 buf 中
            wchar_t buf[256] = {0};
            GetWindowText(GetDlgItem(hwnd, IDC_EDIT_INPUT), buf, 256);

            switch (wmId) {
                // ==============================
                // 方法 1：系统 MessageBox 模态
                // ==============================
                case ID_BTN_MSG_BOX:
                    MessageBox(hwnd, buf, L"System MessageBox", MB_OK | MB_ICONINFORMATION);
                    break;

                // ==============================
                // 方法 2：普通的 DialogBox 模态
                // ==============================
                case ID_BTN_DLG_BOX:
                    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MAINDIALOG), hwnd, DialogProc);
                    break;

                // ==============================
                // 方法 3：带传参的 DialogBoxParam 模态
                // ==============================
                case ID_BTN_DLG_PARAM:
                    // 将 buf 作为最后一个参数 (lParam) 传给对话框
                    // 注意：由于 DialogBoxParam 会阻塞当前线程，所以分配在栈上的 buf 是安全的
                    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MAINDIALOG), hwnd, DialogProc, (LPARAM)buf);
                    break;

                // ==============================
                // 方法 4：纯手工禁用父窗口的模态
                // ==============================
                case ID_BTN_MANUAL: {
                    // 1. 创建一个普通窗口，由于指定了父窗口，它会显示在父窗口上方
                    HWND hModal = CreateWindowEx(
                        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, 
                        MY_MODAL_CLASS, L"Manual Modal",
                        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                        200, 200, 300, 150,
                        hwnd, NULL, GetModuleHandle(NULL), NULL
                    );
                    // 2. 强行禁用主窗口，产生模态阻塞效果！
                    EnableWindow(hwnd, FALSE);
                    break;
                }
            }
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ==========================================
// 4. 程序入口函数
// ==========================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc = {0};
    MSG Msg;

    // 1. 注册主窗口类
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = MY_WINDOW_CLASS; 
    RegisterClassEx(&wc);

    // 2. 注册自定义的手工模态窗口类 (给方法 4 使用)
    WNDCLASSEX wcModal = wc; 
    wcModal.lpfnWndProc = CustomModalWndProc;
    wcModal.lpszClassName = MY_MODAL_CLASS;
    RegisterClassEx(&wcModal);

    // 3. 创建主窗口
    HWND hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        MY_WINDOW_CLASS,
        L"4 Ways to Create Modal Dialogs",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 320, // 增加了主窗口的高度，容纳所有按钮
        NULL, NULL, hInstance, NULL);

    if(hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 4. 消息循环
    while(GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
