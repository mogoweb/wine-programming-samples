#include <windows.h>
#include <imm.h>
#include <string>
#include <vector>
#include <map>

#define IDC_EDIT 101
#define IDC_LISTBOX 102

// 全局变量
HINSTANCE hInst;
HWND hEdit, hListBox;
WNDPROC oldEditProc;

// 消息代码到字符串的映射
std::map<UINT, const char*> messageMap;

void InitializeMessageMap() {
    messageMap[WM_KEYDOWN] = "WM_KEYDOWN";
    messageMap[WM_KEYUP] = "WM_KEYUP";
    messageMap[WM_CHAR] = "WM_CHAR";
    messageMap[WM_DEADCHAR] = "WM_DEADCHAR";
    messageMap[WM_SYSKEYDOWN] = "WM_SYSKEYDOWN";
    messageMap[WM_SYSKEYUP] = "WM_SYSKEYUP";
    messageMap[WM_SYSCHAR] = "WM_SYSCHAR";
    messageMap[WM_SYSDEADCHAR] = "WM_SYSDEADCHAR";
    messageMap[WM_UNICHAR] = "WM_UNICHAR";
    messageMap[WM_IME_SETCONTEXT] = "WM_IME_SETCONTEXT";
    messageMap[WM_IME_NOTIFY] = "WM_IME_NOTIFY";
    messageMap[WM_IME_CONTROL] = "WM_IME_CONTROL";
    messageMap[WM_IME_COMPOSITIONFULL] = "WM_IME_COMPOSITIONFULL";
    messageMap[WM_IME_SELECT] = "WM_IME_SELECT";
    messageMap[WM_IME_CHAR] = "WM_IME_CHAR";
    messageMap[WM_IME_REQUEST] = "WM_IME_REQUEST";
    messageMap[WM_IME_KEYDOWN] = "WM_IME_KEYDOWN";
    messageMap[WM_IME_KEYUP] = "WM_IME_KEYUP";
    messageMap[WM_IME_STARTCOMPOSITION] = "WM_IME_STARTCOMPOSITION";
    messageMap[WM_IME_ENDCOMPOSITION] = "WM_IME_ENDCOMPOSITION";
    messageMap[WM_IME_COMPOSITION] = "WM_IME_COMPOSITION";
}

// 将消息记录到列表框
void LogMessage(const char* msg) {
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)msg);
    int count = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    SendMessage(hListBox, LB_SETTOPINDEX, count - 1, 0);
}

// 子类化的编辑框过程
LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    std::string msgString;
    if (messageMap.count(message)) {
        msgString = messageMap[message];
    } else if (message > WM_USER) {
        // 对于未知消息，不进行记录
    } else {
        // 其它消息，不予记录
        // char buffer[256];
        // snprintf(buffer, sizeof(buffer), "Unknown Message: 0x%X", message);
        // msgString = buffer;
    }

    if (!msgString.empty()) {
        LogMessage(msgString.c_str());
    }

    if (message == WM_IME_COMPOSITION) {
        HIMC hImc = ImmGetContext(hWnd);
        if (hImc) {
            CANDIDATEFORM cf;
            if (ImmGetCandidateWindow(hImc, 0, &cf)) {
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "  Candidate Window Pos: (%ld, %ld)", cf.ptCurrentPos.x, cf.ptCurrentPos.y);
                LogMessage(buffer);
            }
            ImmReleaseContext(hWnd, hImc);
        }
    }

    return CallWindowProc(oldEditProc, hWnd, message, wParam, lParam);
}

// 主窗口过程
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            InitializeMessageMap();

            // 创建文本输入框
            hEdit = CreateWindowEx(
                WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
                10, 10, 300, 200,
                hWnd, (HMENU)IDC_EDIT, hInst, NULL);

            // 创建用于显示消息的列表框
            hListBox = CreateWindowEx(
                WS_EX_CLIENTEDGE, "LISTBOX", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
                320, 10, 450, 200,
                hWnd, (HMENU)IDC_LISTBOX, hInst, NULL);

            // 子类化编辑框以截获其消息
            oldEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
            break;
        }
        case WM_DESTROY:
            // 恢复原始的编辑框过程
            if (hEdit && oldEditProc) {
                SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)oldEditProc);
            }
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 程序入口点
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    const char CLASS_NAME[] = "ImeMessageDemoClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, "Windows Input Message Demo",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 300,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
