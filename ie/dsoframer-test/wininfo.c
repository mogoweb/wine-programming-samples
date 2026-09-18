/* wininfo.c - enumerate Win32 window tree of a top-level window (wine debug aid) */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

static void Dump(HWND h, int depth)
{
    WCHAR cls[64] = L"", txt[128] = L"";
    RECT r; LONG st, ex; HWND ch;
    GetClassNameW(h, cls, 64);
    GetWindowTextW(h, txt, 128);
    GetWindowRect(h, &r);
    st = GetWindowLongW(h, GWL_STYLE);
    ex = GetWindowLongW(h, GWL_EXSTYLE);
    printf("%*s hwnd=0x%p cls=%ls text=\"%ls\" rect=(%ld,%ld)-(%ld,%ld) style=0x%08lX ex=0x%08lX vis=%d\n",
        depth*2, "", (void*)h, cls, txt,
        (long)r.left, (long)r.top, (long)r.right, (long)r.bottom,
        st, ex, IsWindowVisible(h) ? 1 : 0);
    fflush(stdout);
    ch = GetWindow(h, GW_CHILD);
    while (ch) { Dump(ch, depth + 1); ch = GetWindow(ch, GW_HWNDNEXT); }
}

int main(int argc, char **argv)
{
    HWND h;
    if (argc < 2) { printf("usage: wininfo <window-title>\n"); return 1; }
    {
        WCHAR title[256];
        MultiByteToWideChar(CP_ACP, 0, argv[1], -1, title, 256);
        h = FindWindowW(NULL, title);
    }
    if (!h) { printf("window not found\n"); return 1; }
    Dump(h, 0);
    return 0;
}
