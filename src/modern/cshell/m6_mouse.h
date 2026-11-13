// ----------------------------------------------------------------------- //
//
// MODULE  : m6_mouse.h
//
// PURPOSE : The modern build's mouse look (docs/MODERN.md, M-6): raw
//           input (WM_INPUT) in place of DirectInput's mouse axes.
//
//           Retail's look is the engine's axis offsets
//           (ILTClient::GetAxisOffsets), which the engine builds from
//           DirectInput's buffered mouse data once a frame
//           (input_ReadActions, clientinput.cpp), with the cursor put
//           back in the middle of the desktop every frame (CursorCenter
//           1).  That read takes one buffered event at a time and adds
//           every axis's latest value for each one, smooths it towards
//           the mean of its last three (InputRate), and keeps a value
//           until a tenth of InputRate's milliseconds (GetTickCount's)
//           have passed with no event: so how far a mouse count turns
//           the view depends on the frame rate and on how the mouse's
//           motion is cut into events (RETAIL_BUGS.md, "The mouse look
//           turns further at a higher frame rate").
//
//           Here the look is the mouse's own motion: WM_INPUT on the
//           game's window (which the client shell already subclasses),
//           summed between frames and taken once a frame (M6Mouse_Update,
//           CGameClientShell::Update), so a count turns the view the same
//           at any frame rate.  A count is the retail scale's offset
//           (SetMouseSensitivityScale: MouseSensitivity, the zoom's and
//           the EMP's scale, pitch 1.1 times yaw) times M6_GAIN (2):
//           about what the retail look turned a count at 60 frames a
//           second or less (docs/MODERN.md M-6).  While the game
//           has the mouse (InterfaceMgr::UseCursor(FALSE)) the cursor is
//           hidden, kept to the window and parked in its middle; the
//           menus get it back, and the retail full-screen clip.
//
//             OldMouseLook 1   the retail look (the engine's offsets,
//                              CursorCenter 1), unchanged
//             NoRawInput 1     no raw input: the cursor's own motion,
//                              read and put back in the middle each
//                              frame (also what is used if raw input
//                              cannot be had)
//
//           In the new look the engine's offsets are not read: they hold
//           only the mouse's axes (no other axis is bound: M-7's sticks
//           are read by the client shell), so nothing else is lost.  The
//           controller's look (M7Pad_AddLook) is added as before.
//
//           VC6's headers predate raw input: its types are declared here
//           and its two functions are looked up in user32.dll.
//
//           Retail files include this inside #ifdef AVP2SRC_MODERN; one
//           of them (GameClientShell.cpp) defines M6_MOUSE_IMPLEMENTATION
//           first and so holds the code.
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M6_MOUSE_H__
#define __M6_MOUSE_H__

// The console variables (saved in autoexec.cfg), default 0
#define M6_VAR_OLDMOUSELOOK		"OldMouseLook"
#define M6_VAR_NORAWINPUT		"NoRawInput"

void	M6Mouse_UseCursor(LTBOOL bUseCursor);		// InterfaceMgr::UseCursor, in place of its CursorCenter
void	M6Mouse_Update();							// CGameClientShell::Update, once a frame
void	M6Mouse_GetAxisOffsets(LTFLOAT* pOffsets);	// in place of ILTClient::GetAxisOffsets, for the look
void	M6Mouse_SetLookScale(LTFLOAT fScale);		// SetMouseSensitivityScale's (the zoom's, the EMP's)
void	M6Mouse_OnWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);	// HookedWindowProc
void	M6Mouse_OnRenderInit();						// LTEVENT_RENDERINIT, after the retail clip
void	M6Mouse_OnWindowUnhooked();					// UnhookWindow

// A 0/1 variable (0 if it is not set).
inline LTBOOL M6Mouse_GetFlag(const char* pName)
{
	HCONSOLEVAR hVar = g_pLTClient->GetConsoleVar((char*)pName);
	return hVar && g_pLTClient->GetVarValueFloat(hVar) != 0.0f;
}

// Set one, to be saved in autoexec.cfg.
inline void M6Mouse_SetFlag(const char* pName, LTBOOL bOn)
{
	char szCmd[64];
	sprintf(szCmd, "+%s %d", pName, bOn ? 1 : 0);
	g_pLTClient->RunConsoleString(szCmd);
}

#ifdef M6_MOUSE_IMPLEMENTATION

extern HWND		g_hMainWnd;		// GameClientShell.cpp
extern RECT*	g_prcClip;		// GameClientShell.cpp: the retail full-screen clip

namespace
{
	// ------------------------------------------------------------------ //
	// Raw input, as winuser.h (Windows XP and later) declares it.

	#define M6_WM_INPUT					0x00FF
	#define M6_RID_INPUT				0x10000003
	#define M6_RIM_TYPEMOUSE			0
	#define M6_RIDEV_REMOVE				0x00000001
	#define M6_MOUSE_MOVE_ABSOLUTE		0x01
	#define M6_MOUSE_VIRTUAL_DESKTOP	0x02
	#define M6_HID_USAGE_PAGE_GENERIC	0x01
	#define M6_HID_USAGE_GENERIC_MOUSE	0x02
	#define M6_SM_CXVIRTUALSCREEN		78		// winuser.h, Windows 98 and later
	#define M6_SM_CYVIRTUALSCREEN		79

	struct M6RawInputDevice				// RAWINPUTDEVICE
	{
		USHORT	usUsagePage;
		USHORT	usUsage;
		DWORD	dwFlags;
		HWND	hwndTarget;
	};

	struct M6RawInputHeader				// RAWINPUTHEADER
	{
		DWORD	dwType;
		DWORD	dwSize;
		HANDLE	hDevice;
		WPARAM	wParam;
	};

	struct M6RawMouse					// RAWMOUSE
	{
		USHORT	usFlags;
		USHORT	usPad;					// the union below is 4-aligned
		ULONG	ulButtons;				// usButtonFlags, usButtonData
		ULONG	ulRawButtons;
		LONG	lLastX;
		LONG	lLastY;
		ULONG	ulExtraInformation;
	};

	struct M6RawInputMouse				// RAWINPUT, its mouse arm
	{
		M6RawInputHeader	header;
		M6RawMouse			mouse;
	};

	typedef BOOL (WINAPI *M6PFNREGISTERRAWINPUTDEVICES)(const M6RawInputDevice* pDevices, UINT nDevices, UINT cbSize);
	typedef UINT (WINAPI *M6PFNGETRAWINPUTDATA)(HANDLE hRawInput, UINT uiCommand, LPVOID pData, UINT* pcbSize, UINT cbSizeHeader);

	// About the retail look's turn per count at 60 frames a second or less,
	// over the nominal scale its axes are given (docs/MODERN.md, M-6)
	const LTFLOAT	M6_GAIN			= 2.0f;

	enum M6Mode
	{
		M6_MODE_OLD,		// OldMouseLook: the engine's offsets
		M6_MODE_RAW,		// WM_INPUT
		M6_MODE_CURSOR		// the cursor's motion, put back in the middle
	};

	M6PFNREGISTERRAWINPUTDEVICES	s_pfnM6Register	= NULL;
	M6PFNGETRAWINPUTDATA			s_pfnM6GetData	= NULL;
	bool		s_bM6LookedUp	= false;
	HWND		s_hM6RawWnd		= NULL;			// registered for

	bool		s_bM6Captured	= false;		// the game has the mouse (no cursor)
	bool		s_bM6Active		= true;			// the application is in front
	M6Mode		s_eM6Mode		= M6_MODE_OLD;	// how it is read while captured
	LONG		s_nM6DX			= 0;			// raw counts since the last frame
	LONG		s_nM6DY			= 0;
	bool		s_bM6HaveAbs	= false;		// absolute devices (tablets, remote
	LONG		s_nM6AbsX		= 0;			// desktops): the last position, to
	LONG		s_nM6AbsY		= 0;			// turn into motion
	LTFLOAT		s_fM6Scale		= 1.0f;			// SetMouseSensitivityScale's
	LTFLOAT		s_afM6Offsets[3];				// this frame's look

	bool M6LookUpRawInput()
	{
		if (!s_bM6LookedUp)
		{
			s_bM6LookedUp = true;

			HMODULE hUser = GetModuleHandle("user32.dll");
			if (hUser)
			{
				s_pfnM6Register = (M6PFNREGISTERRAWINPUTDEVICES)GetProcAddress(hUser, "RegisterRawInputDevices");
				s_pfnM6GetData = (M6PFNGETRAWINPUTDATA)GetProcAddress(hUser, "GetRawInputData");
			}
		}

		return s_pfnM6Register && s_pfnM6GetData;
	}

	// The mouse's WM_INPUT on the game's window (once per window).  The
	// mouse's own messages are kept (no RIDEV_NOLEGACY): the menus' cursor,
	// and DirectInput's buttons.
	bool M6RegisterMouse()
	{
		if (!g_hMainWnd || !M6LookUpRawInput())
			return false;
		if (s_hM6RawWnd == g_hMainWnd)
			return true;

		M6RawInputDevice rid;
		rid.usUsagePage	= M6_HID_USAGE_PAGE_GENERIC;
		rid.usUsage		= M6_HID_USAGE_GENERIC_MOUSE;
		rid.dwFlags		= 0;
		rid.hwndTarget	= g_hMainWnd;

		if (!s_pfnM6Register(&rid, 1, sizeof(rid)))
			return false;

		s_hM6RawWnd = g_hMainWnd;
		return true;
	}

	void M6UnregisterMouse()
	{
		if (!s_hM6RawWnd || !M6LookUpRawInput())
			return;

		M6RawInputDevice rid;
		rid.usUsagePage	= M6_HID_USAGE_PAGE_GENERIC;
		rid.usUsage		= M6_HID_USAGE_GENERIC_MOUSE;
		rid.dwFlags		= M6_RIDEV_REMOVE;
		rid.hwndTarget	= NULL;

		s_pfnM6Register(&rid, 1, sizeof(rid));
		s_hM6RawWnd = NULL;
	}

	// The game window's client area, in screen coordinates.
	bool M6ClientScreenRect(RECT* pRect)
	{
		if (!g_hMainWnd || !GetClientRect(g_hMainWnd, pRect))
			return false;

		MapWindowPoints(g_hMainWnd, NULL, (POINT*)pRect, 2);
		return pRect->right > pRect->left && pRect->bottom > pRect->top;
	}

	void M6ClientCentre(POINT* pPt)
	{
		RECT rc;
		if (M6ClientScreenRect(&rc))
		{
			pPt->x = (rc.left + rc.right) / 2;
			pPt->y = (rc.top + rc.bottom) / 2;
		}
		else
		{
			pPt->x = GetSystemMetrics(SM_CXSCREEN) / 2;
			pPt->y = GetSystemMetrics(SM_CYSCREEN) / 2;
		}
	}

	// The cursor to the window's middle, if it is not there; its motion
	// since the last call in *pDX, *pDY (when they are given).
	void M6Recentre(LONG* pDX, LONG* pDY)
	{
		POINT ptCentre, pt;
		M6ClientCentre(&ptCentre);
		if (!GetCursorPos(&pt))
			return;

		if (pDX)
		{
			*pDX = pt.x - ptCentre.x;
			*pDY = pt.y - ptCentre.y;
		}
		if (pt.x != ptCentre.x || pt.y != ptCentre.y)
			SetCursorPos(ptCentre.x, ptCentre.y);
	}

	// Hide the cursor, keep it in the window, and park it in the middle.
	void M6Confine()
	{
		RECT rc;
		if (M6ClientScreenRect(&rc))
			ClipCursor(&rc);

		M6Recentre(NULL, NULL);
		SetCursor(NULL);
	}

	// What the retail game has: the full-screen clip, or none in a window.
	void M6Release()
	{
		HCONSOLEVAR hVar = g_pLTClient->GetConsoleVar("Windowed");
		bool bWindowed = hVar && g_pLTClient->GetVarValueFloat(hVar) == 1.0f;

		if (!bWindowed && g_prcClip)
			ClipCursor(g_prcClip);
		else
			ClipCursor(NULL);
	}

	void M6ResetMotion()
	{
		s_nM6DX = 0;
		s_nM6DY = 0;
		s_bM6HaveAbs = false;
	}

	M6Mode M6WantedMode()
	{
		if (M6Mouse_GetFlag(M6_VAR_OLDMOUSELOOK))
			return M6_MODE_OLD;
		if (!M6Mouse_GetFlag(M6_VAR_NORAWINPUT) && M6RegisterMouse())
			return M6_MODE_RAW;
		return M6_MODE_CURSOR;
	}

	// Take the mouse in a mode (captured).
	void M6Capture(M6Mode eMode)
	{
		if (s_bM6Captured && s_eM6Mode != M6_MODE_OLD && eMode == M6_MODE_OLD)
			M6Release();

		s_eM6Mode = eMode;
		if (eMode == M6_MODE_OLD)
		{
			g_pLTClient->RunConsoleString("CursorCenter 1");
		}
		else
		{
			g_pLTClient->RunConsoleString("CursorCenter 0");
			if (s_bM6Active)
				M6Confine();
		}

		M6ResetMotion();
		s_bM6Captured = true;
	}

	void M6OnRawInput(LPARAM lParam)
	{
		union
		{
			M6RawInputMouse	raw;
			BYTE			ab[64];
		} buf;
		UINT nSize = sizeof(buf);

		if (s_pfnM6GetData((HANDLE)lParam, M6_RID_INPUT, &buf, &nSize, sizeof(M6RawInputHeader)) == (UINT)-1)
			return;
		if (nSize < sizeof(M6RawInputMouse) || buf.raw.header.dwType != M6_RIM_TYPEMOUSE)
			return;

		const M6RawMouse& m = buf.raw.mouse;

		if (m.usFlags & M6_MOUSE_MOVE_ABSOLUTE)
		{
			// 0..65535 across the (virtual) screen: into pixels, and motion
			bool bVirtual = (m.usFlags & M6_MOUSE_VIRTUAL_DESKTOP) != 0;
			int nW = GetSystemMetrics(bVirtual ? M6_SM_CXVIRTUALSCREEN : SM_CXSCREEN);
			int nH = GetSystemMetrics(bVirtual ? M6_SM_CYVIRTUALSCREEN : SM_CYSCREEN);
			LONG nX = MulDiv(m.lLastX, nW, 65535);
			LONG nY = MulDiv(m.lLastY, nH, 65535);

			if (s_bM6HaveAbs)
			{
				s_nM6DX += nX - s_nM6AbsX;
				s_nM6DY += nY - s_nM6AbsY;
			}
			s_nM6AbsX = nX;
			s_nM6AbsY = nY;
			s_bM6HaveAbs = true;
		}
		else
		{
			s_nM6DX += m.lLastX;
			s_nM6DY += m.lLastY;
		}
	}
}

void M6Mouse_UseCursor(LTBOOL bUseCursor)
{
	if (bUseCursor)
	{
		if (s_bM6Captured && s_eM6Mode != M6_MODE_OLD)
			M6Release();

		g_pLTClient->RunConsoleString("CursorCenter 0");
		s_bM6Captured = false;
		M6ResetMotion();
		return;
	}

	M6Capture(M6WantedMode());
}

void M6Mouse_Update()
{
	s_afM6Offsets[0] = s_afM6Offsets[1] = s_afM6Offsets[2] = 0.0f;

	if (!s_bM6Captured)
	{
		M6ResetMotion();
		return;
	}

	// OldMouseLook or NoRawInput changed (the menu, the console)
	M6Mode eWanted = M6WantedMode();
	if (eWanted != s_eM6Mode)
		M6Capture(eWanted);

	if (s_eM6Mode == M6_MODE_OLD)
		return;

	LONG nDX = 0, nDY = 0;

	if (!s_bM6Active)
	{
		M6ResetMotion();
		return;
	}

	if (s_eM6Mode == M6_MODE_CURSOR)
	{
		M6Recentre(&nDX, &nDY);
	}
	else
	{
		nDX = s_nM6DX;
		nDY = s_nM6DY;
		s_nM6DX = s_nM6DY = 0;

		// keep the (hidden) cursor from an edge: where raw input is made
		// from the pointer's position (Wine without a clip, a remote
		// desktop) an edge would stop it
		M6Recentre(NULL, NULL);
	}

	if (!nDX && !nDY)
		return;

	// The retail scale (CGameClientShell::SetMouseSensitivityScale)
	HCONSOLEVAR hVar = g_pLTClient->GetConsoleVar("MouseSensitivity");
	LTFLOAT fSens = hVar ? g_pLTClient->GetVarValueFloat(hVar) : 1.0f;
	LTFLOAT fPerCount = (0.166f + (fSens / 2.75f)) * s_fM6Scale * 0.001f * M6_GAIN;

	s_afM6Offsets[0] = (LTFLOAT)nDX * fPerCount;
	s_afM6Offsets[1] = (LTFLOAT)nDY * fPerCount * 1.1f;
}

void M6Mouse_GetAxisOffsets(LTFLOAT* pOffsets)
{
	if (!s_bM6Captured || s_eM6Mode == M6_MODE_OLD)
	{
		g_pLTClient->GetAxisOffsets(pOffsets);
		return;
	}

	pOffsets[0] = s_afM6Offsets[0];
	pOffsets[1] = s_afM6Offsets[1];
	pOffsets[2] = s_afM6Offsets[2];
}

void M6Mouse_SetLookScale(LTFLOAT fScale)
{
	s_fM6Scale = fScale;
}

void M6Mouse_OnWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == M6_WM_INPUT)
	{
		if (s_bM6Captured && s_bM6Active && s_eM6Mode == M6_MODE_RAW && s_pfnM6GetData)
			M6OnRawInput(lParam);
	}
	else if (uMsg == WM_SETCURSOR)
	{
		// hidden over the window while the game has the mouse
		if (s_bM6Captured && s_eM6Mode != M6_MODE_OLD && LOWORD(lParam) == HTCLIENT)
			SetCursor(NULL);
	}
	else if (uMsg == WM_ACTIVATEAPP)
	{
		// let the cursor go while another application is in front
		s_bM6Active = wParam != 0;
		if (s_bM6Captured && s_eM6Mode != M6_MODE_OLD)
		{
			if (s_bM6Active)
				M6Confine();
			else
				ClipCursor(NULL);
		}
		M6ResetMotion();
	}
}

void M6Mouse_OnRenderInit()
{
	// a new mode: the window may have moved or changed size
	if (s_bM6Captured && s_eM6Mode != M6_MODE_OLD && s_bM6Active)
		M6Confine();
	M6ResetMotion();
}

void M6Mouse_OnWindowUnhooked()
{
	M6UnregisterMouse();
	if (s_bM6Captured && s_eM6Mode != M6_MODE_OLD)
		ClipCursor(NULL);
	s_bM6Captured = false;
}

#endif // M6_MOUSE_IMPLEMENTATION

#endif // __M6_MOUSE_H__
