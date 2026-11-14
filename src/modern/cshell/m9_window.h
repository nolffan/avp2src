// ----------------------------------------------------------------------- //
//
// MODULE  : m9_window.h
//
// PURPOSE : The modern build's window modes (docs/MODERN.md, M-9): full
//           screen, a window, or a borderless window covering the
//           desktop, chosen in Options, Display ("Window mode").
//
//           Retail has a window only from the command line (++windowed
//           1), and leaves it to the renderer, which places it wrongly
//           (RETAIL_BUGS.md, "A window is placed and sized for a frame
//           it does not have"): the top is centred from the widths, the
//           size is the mode's client area under WS_OVERLAPPEDWINDOW's
//           frame while the engine's window has a one-pixel border and
//           no title bar, so its client area is larger than the mode, the
//           picture stretched onto it and the menus' clicks off by the
//           difference, and the window cannot be moved.  The client
//           shell's HookWindow sizes it only once, at start-up, and only
//           for full screen.  After every renderer start (a mode set,
//           focus regained) this puts the window right:
//
//             full screen          what HookWindow does: no frame,
//                                  topmost, the mode's size (only needed
//                                  after having been a window)
//             window               a title bar and the mode's size as
//                                  the client area, centred in the work
//                                  area (and left where the player moved
//                                  it, while it is right)
//             borderless window    WS_POPUP, covering the desktop at the
//                                  desktop's own resolution
//
//           The mode is the engine's Windowed (0 full screen, 1 a window:
//           what both renderers read when they start) and our WindowMode
//           (2: that window borderless).  A borderless window is always
//           at the desktop's resolution: the start-up mode is changed to
//           it (M9Window_StartMode) and, when the mode is another later
//           (the console), it is set on the next frame (M9Window_Update).
//
//           A config with no WindowMode (a first run) starts borderless,
//           unless it asks for a window (Windowed 1): the renderers' full
//           screen changes the display's mode, which under Wine fails
//           unless the display can take that mode (Xvfb, or X servers
//           whose RandR offers no such mode; docs/MODERN.md M-9), and a
//           full screen that fails falls back to the borderless window
//           (M9Window_StartFailed, and the Display folder's own).
//
//           The focus (Alt+Tab) is the engine's, in every mode as in
//           retail: it shuts the renderer down when the focus goes and
//           starts it again when it comes back, and the window is put
//           right again then (LTEVENT_GAINEDFOCUS, LTEVENT_RENDERINIT; a
//           full screen DirectDraw minimised is restored).  A window
//           larger than the screen is let be (WM_GETMINMAXINFO).
//
//           Retail files include this inside #ifdef AVP2SRC_MODERN; one
//           of them (GameClientShell.cpp) defines M9_WINDOW_IMPLEMENTATION
//           first and so holds the code.
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M9_WINDOW_H__
#define __M9_WINDOW_H__

#define M9_VAR_WINDOWED			"Windowed"		// the engine's: 1 a window
#define M9_VAR_WINDOWMODE		"WindowMode"	// ours: 2 that window borderless

enum
{
	M9_WINDOW_FULLSCREEN = 0,
	M9_WINDOW_WINDOWED,
	M9_WINDOW_BORDERLESS,
	M9_NUM_WINDOW_MODES
};

int		M9Window_GetMode();						// M9_WINDOW_*, from Windowed and WindowMode
void	M9Window_SetMode(int nMode);			// writes both (saved); the mode must then be set again
void	M9Window_DesktopSize(int* pWidth, int* pHeight);
void	M9Window_StartMode(RMode* pMode);		// OnEngineInitialized, before the first SetRenderMode
LTBOOL	M9Window_StartFailed(RMode* pMode);		// ... when it failed: LTTRUE if a borderless window came up
void	M9Window_Apply();						// LTEVENT_RENDERINIT, LTEVENT_GAINEDFOCUS
void	M9Window_Update();						// CGameClientShell::Update, every frame
void	M9Window_OnWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);	// HookedWindowProc

#ifdef M9_WINDOW_IMPLEMENTATION

extern HWND		g_hMainWnd;		// GameClientShell.cpp
extern RECT*	g_prcClip;		// GameClientShell.cpp: the retail full-screen clip

// The virtual screen (every monitor), which VC6's headers (WINVER 0x400)
// predate
#ifndef SM_XVIRTUALSCREEN
#define SM_XVIRTUALSCREEN	76
#define SM_YVIRTUALSCREEN	77
#define SM_CXVIRTUALSCREEN	78
#define SM_CYVIRTUALSCREEN	79
#endif

namespace
{
	const DWORD kM9FramedStyle		= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;
	const DWORD kM9BorderlessStyle	= WS_POPUP | WS_VISIBLE;
	const DWORD kM9FullScreenStyle	= WS_VISIBLE;		// as HookWindow sets it

	// What the window was last made, so that a full-screen window that
	// was never a window is left to the retail code.
	enum M9Placement
	{
		M9_PLACED_NONE,
		M9_PLACED_FULLSCREEN,
		M9_PLACED_FRAMED,
		M9_PLACED_BORDERLESS
	};

	M9Placement	s_eM9Placed		= M9_PLACED_NONE;
	uint32		s_nM9Width		= 0;		// the mode it was placed for
	uint32		s_nM9Height		= 0;
	bool		s_bM9WantDesktop = false;	// borderless at another mode: set the desktop's
	uint32		s_nM9TriedWidth	= 0;		// ... once for each mode it is at
	uint32		s_nM9TriedHeight = 0;

	int M9GetInt(const char* pName, int nDefault)
	{
		HCONSOLEVAR hVar = g_pLTClient->GetConsoleVar((char*)pName);
		return hVar ? (int)g_pLTClient->GetVarValueFloat(hVar) : nDefault;
	}

	void M9SetInt(const char* pName, int nValue)
	{
		char szCmd[64];
		sprintf(szCmd, "+%s %d", pName, nValue);
		g_pLTClient->RunConsoleString(szCmd);
	}

	// Set the engine's mode as the Display folder does, and say so in the
	// variables it keeps
	LTRESULT M9SetRenderMode(RMode* pMode)
	{
		g_pInterfaceMgr->SetSwitchingRenderModes(LTTRUE);
		LTRESULT result = g_pLTClient->SetRenderMode(pMode);
		g_pInterfaceMgr->SetSwitchingRenderModes(LTFALSE);
		if (result != LT_OK)
			return result;

		g_pInterfaceMgr->ScreenDimsChanged();

		M9SetInt("GameScreenWidth", (int)pMode->m_Width);
		M9SetInt("GameScreenHeight", (int)pMode->m_Height);
		return LT_OK;
	}
}

int M9Window_GetMode()
{
	if (M9GetInt(M9_VAR_WINDOWED, 0) != 1)
		return M9_WINDOW_FULLSCREEN;

	return M9GetInt(M9_VAR_WINDOWMODE, M9_WINDOW_WINDOWED) == M9_WINDOW_BORDERLESS ?
		M9_WINDOW_BORDERLESS : M9_WINDOW_WINDOWED;
}

void M9Window_SetMode(int nMode)
{
	M9SetInt(M9_VAR_WINDOWED, nMode != M9_WINDOW_FULLSCREEN ? 1 : 0);
	M9SetInt(M9_VAR_WINDOWMODE, nMode);
}

// The desktop's own resolution -- not the screen's, which in full screen
// is the game's mode.
void M9Window_DesktopSize(int* pWidth, int* pHeight)
{
	DEVMODE dm;
	memset(&dm, 0, sizeof(dm));
	dm.dmSize = sizeof(dm);

	if (EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &dm) && dm.dmPelsWidth && dm.dmPelsHeight)
	{
		*pWidth  = (int)dm.dmPelsWidth;
		*pHeight = (int)dm.dmPelsHeight;
		return;
	}

	*pWidth  = GetSystemMetrics(SM_CXSCREEN);
	*pHeight = GetSystemMetrics(SM_CYSCREEN);
}

// The first mode: a first run's window mode, and a borderless window's
// size.
void M9Window_StartMode(RMode* pMode)
{
	if (!g_pLTClient->GetConsoleVar(M9_VAR_WINDOWMODE))
		M9Window_SetMode(M9GetInt(M9_VAR_WINDOWED, 0) == 1 ? M9_WINDOW_WINDOWED : M9_WINDOW_BORDERLESS);

	if (M9Window_GetMode() == M9_WINDOW_BORDERLESS)
	{
		int nDeskW, nDeskH;
		M9Window_DesktopSize(&nDeskW, &nDeskH);
		pMode->m_Width  = (uint32)nDeskW;
		pMode->m_Height = (uint32)nDeskH;
	}
}

// The first mode could not be set.  Full screen: try the borderless
// window, which needs no change of the display's mode.
LTBOOL M9Window_StartFailed(RMode* pMode)
{
	if (M9Window_GetMode() != M9_WINDOW_FULLSCREEN)
		return LTFALSE;

	g_pLTClient->DebugOut("Full screen failed: a borderless window instead.\n");
	M9Window_SetMode(M9_WINDOW_BORDERLESS);

	RMode mode = *pMode;
	M9Window_StartMode(&mode);
	if (g_pLTClient->SetRenderMode(&mode) != LT_OK)
		return LTFALSE;

	*pMode = mode;
	return LTTRUE;
}

// CGameClientShell::Update, every frame: a borderless window's mode.
void M9Window_Update()
{
	if (!s_bM9WantDesktop)
		return;

	s_bM9WantDesktop = false;

	RMode mode;
	if (g_pLTClient->GetRenderMode(&mode) != LT_OK)
		return;

	// once per mode, so that a desktop mode the renderer refuses is not
	// asked for every frame
	if (mode.m_Width == s_nM9TriedWidth && mode.m_Height == s_nM9TriedHeight)
		return;
	s_nM9TriedWidth  = mode.m_Width;
	s_nM9TriedHeight = mode.m_Height;

	int nDeskW, nDeskH;
	M9Window_DesktopSize(&nDeskW, &nDeskH);
	mode.m_Width  = (uint32)nDeskW;
	mode.m_Height = (uint32)nDeskH;
	M9SetRenderMode(&mode);
}

void M9Window_Apply()
{
	if (!g_hMainWnd)
		return;

	RMode mode;
	if (g_pLTClient->GetRenderMode(&mode) != LT_OK || !mode.m_Width || !mode.m_Height)
		return;

	bool bModeChanged = (mode.m_Width != s_nM9Width || mode.m_Height != s_nM9Height);
	s_nM9Width  = mode.m_Width;
	s_nM9Height = mode.m_Height;

	int nMode = M9Window_GetMode();

	if (nMode == M9_WINDOW_FULLSCREEN)
	{
		if (s_eM9Placed == M9_PLACED_FRAMED || s_eM9Placed == M9_PLACED_BORDERLESS || IsIconic(g_hMainWnd))
		{
			// back from a window (or minimised): what HookWindow does at
			// start-up
			if (IsIconic(g_hMainWnd))
				ShowWindow(g_hMainWnd, SW_RESTORE);
			SetWindowLong(g_hMainWnd, GWL_STYLE, kM9FullScreenStyle);
			SetWindowPos(g_hMainWnd, HWND_TOPMOST, 0, 0, mode.m_Width, mode.m_Height, SWP_FRAMECHANGED);
			if (!g_prcClip)
				g_prcClip = new RECT;
			GetWindowRect(g_hMainWnd, g_prcClip);
		}
		s_eM9Placed = M9_PLACED_FULLSCREEN;
		return;
	}

	// A window: the display is at the desktop's own mode.
	int nDeskW, nDeskH;
	M9Window_DesktopSize(&nDeskW, &nDeskH);
	bool bBorderless = (nMode == M9_WINDOW_BORDERLESS);
	DWORD dwStyle = bBorderless ? kM9BorderlessStyle : kM9FramedStyle;

	// A borderless window covers the desktop; its mode is the desktop's.
	if (bBorderless && ((int)mode.m_Width != nDeskW || (int)mode.m_Height != nDeskH))
		s_bM9WantDesktop = true;
	M9Placement ePlace = bBorderless ? M9_PLACED_BORDERLESS : M9_PLACED_FRAMED;

	RECT rcWant;
	if (bBorderless)
		SetRect(&rcWant, 0, 0, nDeskW, nDeskH);
	else
		SetRect(&rcWant, 0, 0, mode.m_Width, mode.m_Height);
	AdjustWindowRect(&rcWant, dwStyle, FALSE);
	int cx = rcWant.right - rcWant.left;
	int cy = rcWant.bottom - rcWant.top;

	RECT rcNow;
	GetWindowRect(g_hMainWnd, &rcNow);
	DWORD dwNow = (DWORD)GetWindowLong(g_hMainWnd, GWL_STYLE);
	bool bRight = !bModeChanged && s_eM9Placed == ePlace &&
		(dwNow & (WS_POPUP | WS_CAPTION)) == (dwStyle & (WS_POPUP | WS_CAPTION)) &&
		rcNow.right - rcNow.left == cx && rcNow.bottom - rcNow.top == cy &&
		(!bBorderless || (rcNow.left == 0 && rcNow.top == 0));
	if (!bRight)
	{
		int x = 0, y = 0;
		if (!bBorderless)
		{
			RECT rcWork;
			if (!SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0))
				SetRect(&rcWork, 0, 0, nDeskW, nDeskH);

			int nWorkW = rcWork.right - rcWork.left;
			int nWorkH = rcWork.bottom - rcWork.top;
			if (cx <= nWorkW && cy <= nWorkH)
			{
				x = rcWork.left + (nWorkW - cx) / 2;
				y = rcWork.top + (nWorkH - cy) / 2;
			}
			else
			{
				// Too large for the work area with its frame (a window at
				// the desktop's resolution): the client area centred in it,
				// the frame off the edges, and never above or left of it --
				// under Wine a window whose client area runs off the
				// screen is drawn black (its renderer's blit to the
				// screen fails)
				int nClientW = (int)mode.m_Width, nClientH = (int)mode.m_Height;
				x = rcWork.left + (nWorkW - nClientW) / 2;
				y = rcWork.top + (nWorkH - nClientH) / 2;
				if (x < rcWork.left) x = rcWork.left;
				if (y < rcWork.top) y = rcWork.top;
				x += rcWant.left;		// AdjustWindowRect's frame: negative
				y += rcWant.top;
			}
		}

		SetWindowLong(g_hMainWnd, GWL_STYLE, dwStyle);
		SetWindowPos(g_hMainWnd, HWND_NOTOPMOST, x, y, cx, cy, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
		s_eM9Placed = ePlace;
	}

	// The retail full-screen clip rectangle, which some interface code
	// still applies, must not hold the cursor in a window: the whole
	// virtual screen.
	if (g_prcClip)
	{
		SetRect(g_prcClip, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
			GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN),
			GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN));
	}
	// and a clip of the old place let go (M-6 confines the cursor to the
	// new one in play, after this: M6Mouse_OnRenderInit)
	if (!bRight)
		ClipCursor(NULL);
}

// A window with a title bar as large as the screen (or larger) would be
// cut down to the screen by Windows' largest tracking size, its client
// area then smaller than the mode and the picture squeezed into it: let
// it be the mode's size, running off the screen's edge.
void M9Window_OnWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_GETMINMAXINFO && lParam && M9Window_GetMode() == M9_WINDOW_WINDOWED)
	{
		MINMAXINFO* pInfo = (MINMAXINFO*)lParam;
		pInfo->ptMaxTrackSize.x = 32767;
		pInfo->ptMaxTrackSize.y = 32767;
		pInfo->ptMaxSize.x = 32767;
		pInfo->ptMaxSize.y = 32767;
	}
}

#endif // M9_WINDOW_IMPLEMENTATION

#endif // __M9_WINDOW_H__
