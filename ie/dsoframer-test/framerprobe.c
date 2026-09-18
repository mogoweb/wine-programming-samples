/* framerprobe.c - runtime probes for the toolbar-toggle repaint issue
 *
 * Usage (inside wine, against running framerapp):
 *   framerprobe.exe                      dump window tree
 *   framerprobe.exe invalidate           force repaint all children of framer window
 *   framerprobe.exe resizenow            shrink+restore framer window to force relayout
 *   framerprobe.exe reactiv              set active object again? (just dump)
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

static void Dump(HWND h, int depth)
{
    WCHAR cls[64] = L"", txt[128] = L"";
    RECT r;
    GetClassNameW(h, cls, 64);
    GetWindowTextW(h, txt, 128);
    GetWindowRect(h, &r);
    printf("%*s 0x%p %-24ls \"%ls\" rect=(%ld,%ld)-(%ld,%ld) vis=%d\n",
        depth*2, "", (void*)h, cls, txt,
        (long)r.left, (long)r.top, (long)r.right, (long)r.bottom,
        IsWindowVisible(h) ? 1 : 0);
    fflush(stdout);
    { HWND ch = GetWindow(h, GW_CHILD);
      while (ch) { Dump(ch, depth + 1); ch = GetWindow(ch, GW_HWNDNEXT); } }
}

static HWND FindFramerCtl(HWND top)
{
    HWND h = GetWindow(top, GW_CHILD);
    while (h) {
        WCHAR cls[64]; GetClassNameW(h, cls, 64);
        if (lstrcmpW(cls, L"DSOFramerOCXWnd") == 0) return h;
        h = GetWindow(h, GW_HWNDNEXT);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    HWND top = FindWindowW(NULL, L"C++ Test Application for DsoFramer Control");
    if (!top) { printf("main window not found\n"); return 1; }

    if (argc >= 2 && lstrcmpA(argv[1], "invalidate") == 0)
    {
        HWND ctl = FindFramerCtl(top);
        if (!ctl) { printf("framer ctl not found\n"); return 1; }
        RedrawWindow(ctl, NULL, NULL,
            RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW | RDW_FRAME);
        printf("RedrawWindow(ALLCHILDREN|UPDATENOW) done on ctl 0x%p\n", (void*)ctl);
        return 0;
    }
    if (argc >= 2 && lstrcmpA(argv[1], "resizenow") == 0)
    {
        RECT rc; GetWindowRect(top, &rc);
        /* shrink 2px then restore: forces WM_SIZE cascade */
        SetWindowPos(top, NULL, 0, 0, rc.right-rc.left-2, rc.bottom-rc.top, SWP_NOMOVE|SWP_NOZORDER);
        Sleep(200);
        SetWindowPos(top, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top, SWP_NOMOVE|SWP_NOZORDER);
        printf("resize cycle done\n");
        return 0;
    }
    Dump(top, 0);
    return 0;
}
