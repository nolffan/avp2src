// mr_window.cpp -- mrplatform.h on Windows: display modes and the window
// through user32 (what d3d.ren's DirectDraw enumeration and cooperative
// level did).

#include <string.h>

#include "mr.h"

static HWND  s_hWnd;
static bool  s_bChangedDisplay;         // ChangeDisplaySettings to undo
static LONG  s_nOldStyle, s_nOldExStyle;
static RECT  s_rcOldWindow;

void plat_EnumModes(PlatModeFn fn, void* pUser)
{
    DEVMODEA dm;
    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    for (DWORD i = 0; EnumDisplaySettingsA(NULL, i, &dm); i++)
        fn(dm.dmPelsWidth, dm.dmPelsHeight, dm.dmBitsPerPel, pUser);
}

void plat_OpenWindow(void* hWnd, uint32_t nWidth, uint32_t nHeight, bool* pbWindowed)
{
    s_hWnd = (HWND)hWnd;
    s_nOldStyle = GetWindowLongA(s_hWnd, GWL_STYLE);
    s_nOldExStyle = GetWindowLongA(s_hWnd, GWL_EXSTYLE);
    GetWindowRect(s_hWnd, &s_rcOldWindow);

    if (!*pbWindowed)
    {
        DEVMODEA dm;
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        dm.dmPelsWidth = nWidth;
        dm.dmPelsHeight = nHeight;
        dm.dmBitsPerPel = 32;
        dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
        if (ChangeDisplaySettingsA(&dm, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL)
        {
            s_bChangedDisplay = true;
            SetWindowLongA(s_hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            SetWindowLongA(s_hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);
            SetWindowPos(s_hWnd, HWND_TOPMOST, 0, 0, nWidth, nHeight, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            return;
        }
        mr_Print("%s: could not switch the display to %ux%u; running in a window", g_RDDll, nWidth, nHeight);
        *pbWindowed = true;
    }

    // A window whose client area is the mode, centred on the desktop
    // (d3d.ren's AdjustWindowRect + SetWindowPos).
    LONG style = s_nOldStyle;
    if (style & WS_POPUP)
        style = (style & ~WS_POPUP) | WS_OVERLAPPEDWINDOW;
    SetWindowLongA(s_hWnd, GWL_STYLE, style);
    RECT rc = { 0, 0, (LONG)nWidth, (LONG)nHeight };
    AdjustWindowRectEx(&rc, style, FALSE, s_nOldExStyle & ~WS_EX_TOPMOST);
    int cx = rc.right - rc.left, cy = rc.bottom - rc.top;
    int x = (GetSystemMetrics(SM_CXSCREEN) - cx) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - cy) / 2;
    SetWindowPos(s_hWnd, HWND_NOTOPMOST, x < 0 ? 0 : x, y < 0 ? 0 : y, cx, cy, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

void plat_CloseWindow(void)
{
    if (!s_bChangedDisplay)
        return;
    ChangeDisplaySettingsA(NULL, 0);
    s_bChangedDisplay = false;
    SetWindowLongA(s_hWnd, GWL_STYLE, s_nOldStyle);
    SetWindowLongA(s_hWnd, GWL_EXSTYLE, s_nOldExStyle);
    SetWindowPos(s_hWnd, HWND_NOTOPMOST, s_rcOldWindow.left, s_rcOldWindow.top,
                 s_rcOldWindow.right - s_rcOldWindow.left, s_rcOldWindow.bottom - s_rcOldWindow.top,
                 SWP_FRAMECHANGED);
}
