#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>
#include <assert.h>

void CenterText(HDC hDC, int x, int y, LPCTSTR szFace, LPCTSTR szMessage, int point)
{
    HFONT hFont = CreateFont(-point * GetDeviceCaps(hDC, LOGPIXELSY) / 72,
            0, 0, 0, FW_BOLD, TRUE, FALSE, FALSE, ANSI_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, VARIABLE_PITCH, szFace);
    assert(hFont);

    HGDIOBJ hOld = SelectObject(hDC, hFont);
    SetTextAlign(hDC, TA_CENTER | TA_BASELINE);
    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, RGB(0, 0, 0xFF));
    TextOut(hDC, x, y, szMessage, _tcslen(szMessage));

    SelectObject(hDC, hOld);
    DeleteObject(hFont);
}

const TCHAR szMessage[] = _T("Hello, World!");
const TCHAR szFace[] = _T(/*"Times New Roman"*/"Arial");

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    HDC hDC = GetDC(NULL);
    assert(hDC);

    CenterText(hDC, GetSystemMetrics(SM_CXSCREEN) / 2, GetSystemMetrics(SM_CYSCREEN) / 2,
            szFace, szMessage, 72);
    
    ReleaseDC(NULL, hDC);
    return 0;
}