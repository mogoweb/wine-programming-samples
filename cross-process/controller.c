#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: ./controller.exe <Target_HWND>\n");
        printf("\nExample:\n");
        printf("  1. First run target.exe in one terminal\n");
        printf("  2. Copy the HWND printed by target.exe\n");
        printf("  3. Run: ./controller.exe <HWND>\n");
        return -1;
    }

    // 从命令行参数获取目标窗口的 HWND
    HWND hwndTarget = (HWND)(UINT_PTR)strtoull(argv[1], NULL, 16);

    printf("=== Window Position Controller ===\n");
    printf("Target HWND: %p\n", hwndTarget);

    // 检查窗口是否有效
    if (!IsWindow(hwndTarget)) {
        printf("Error: Invalid window handle!\n");
        return -1;
    }

    // 获取窗口当前信息
    RECT rc;
    GetWindowRect(hwndTarget, &rc);
    printf("Current position: (%ld, %ld)\n", rc.left, rc.top);
    printf("Current size: %ldx%ld\n", rc.right - rc.left, rc.bottom - rc.top);

    // 获取窗口标题
    char title[256];
    GetWindowTextA(hwndTarget, title, sizeof(title));
    printf("Window title: %s\n", title);

    // 获取窗口所属进程ID
    DWORD pid;
    GetWindowThreadProcessId(hwndTarget, &pid);
    printf("Owner PID: %lu\n", pid);
    printf("Controller PID: %lu\n", GetCurrentProcessId());

    printf("\n--- SetWindowPos Demos ---\n");

    // Demo 1: 移动窗口到左上角
    printf("\n[1] Moving window to (0, 0)...\n");
    SetWindowPos(hwndTarget, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    Sleep(1000);

    // Demo 2: 移动窗口到屏幕中央
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;
    int centerX = (screenW - winW) / 2;
    int centerY = (screenH - winH) / 2;

    printf("[2] Moving window to center (%d, %d)...\n", centerX, centerY);
    SetWindowPos(hwndTarget, NULL, centerX, centerY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    Sleep(1000);

    // Demo 3: 改变窗口大小
    printf("[3] Resizing window to 600x400...\n");
    SetWindowPos(hwndTarget, NULL, 0, 0, 600, 400, SWP_NOMOVE | SWP_NOZORDER);
    Sleep(1000);

    // Demo 4: 移动并改变大小
    printf("[4] Moving to (200, 200) and resizing to 500x350...\n");
    SetWindowPos(hwndTarget, NULL, 200, 200, 500, 350, SWP_NOZORDER);
    Sleep(1000);

    // Demo 5: 使用 MoveWindow (另一种方法)
    printf("[5] Using MoveWindow to (300, 150, 450, 300)...\n");
    MoveWindow(hwndTarget, 300, 150, 450, 300, TRUE);
    Sleep(1000);

    // Demo 6: 置顶窗口
    printf("[6] Setting window to topmost...\n");
    SetWindowPos(hwndTarget, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    Sleep(1000);

    // Demo 7: 取消置顶
    printf("[7] Removing topmost flag...\n");
    SetWindowPos(hwndTarget, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    Sleep(1000);

    // Demo 8: 恢复原始位置和大小
    printf("[8] Restoring original position and size...\n");
    SetWindowPos(hwndTarget, NULL, rc.left, rc.top,
                 rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);

    printf("\nDone! Check the target window for position changes.\n");
    return 0;
}
