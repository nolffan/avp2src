// ----------------------------------------------------------------------- //
//
// MODULE  : m7_padread.h
//
// PURPOSE : The modern build's controller (docs/MODERN.md, M-7): reading
//           the pad, for the two modules that do -- lithtech.exe's
//           ##gamepad device (p7_gamepad.h: the buttons and axes the
//           bindings see) and the client shell (cshell/m7_pad.h: the
//           sticks, and the menus, which must not depend on what a button
//           is bound to).  Each includes this once and has its own copy
//           of the state.
//
//           The pad is the first connected XInput pad (slots 0..3; asking
//           about an empty slot is slow, so while none is connected they
//           are looked through at most once a second), from whichever of
//           xinput1_4, xinput1_3 and xinput9_1_0 loads (Wine has all
//           three).  VC6's headers predate XInput: its two structures are
//           declared here.
//
//           The test pad: with AVP2SRC_PADTEST naming a file, XInput is
//           not used and the pad's state is that file's one line,
//           "<buttons> <lx> <ly> <rx> <ry> <lt> <rt>" in XInput's units
//           (XINPUT_GAMEPAD's wButtons; the sticks -32768..32767, Y up
//           positive; the triggers 0..255), read again every 15 ms; no
//           file, or one that does not read, is no pad connected.  The
//           playtest's `pad` command writes it (build/playtest.sh).
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M7_PADREAD_H__
#define __M7_PADREAD_H__

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// XINPUT_GAMEPAD
struct M7PadState
{
	WORD	wButtons;
	BYTE	bLeftTrigger;
	BYTE	bRightTrigger;
	SHORT	sThumbLX;
	SHORT	sThumbLY;
	SHORT	sThumbRX;
	SHORT	sThumbRY;
};

// XINPUT_GAMEPAD's button bits, and its trigger threshold
enum
{
	M7PAD_DPAD_UP			= 0x0001,
	M7PAD_DPAD_DOWN			= 0x0002,
	M7PAD_DPAD_LEFT			= 0x0004,
	M7PAD_DPAD_RIGHT		= 0x0008,
	M7PAD_START				= 0x0010,
	M7PAD_BACK				= 0x0020,
	M7PAD_LEFT_THUMB		= 0x0040,
	M7PAD_RIGHT_THUMB		= 0x0080,
	M7PAD_LEFT_SHOULDER		= 0x0100,
	M7PAD_RIGHT_SHOULDER	= 0x0200,
	M7PAD_A					= 0x1000,
	M7PAD_B					= 0x2000,
	M7PAD_X					= 0x4000,
	M7PAD_Y					= 0x8000,
	M7PAD_TRIGGER_THRESHOLD	= 30
};

namespace
{
	struct M7XInputState				// XINPUT_STATE
	{
		DWORD		dwPacketNumber;
		M7PadState	Gamepad;
	};

	typedef DWORD (WINAPI* M7XInputGetState)(DWORD dwUserIndex, M7XInputState* pState);

	int					g_nM7PadAvailable = -1;	// -1: not decided yet
	M7XInputGetState	g_pM7GetState;
	int					g_nM7PadSlot = -1;		// the slot read, -1: none connected
	DWORD				g_dwM7LastScan;
	bool				g_bM7Scanned;

	const char*			g_pM7TestFile;			// AVP2SRC_PADTEST
	bool				g_bM7TestRead;
	DWORD				g_dwM7TestRead;
	bool				g_bM7TestPad;
	M7PadState			g_M7TestState;

	// Can there be a pad at all: XInput loads, or the test pad is on.
	// Decided once.
	bool M7PadRead_Available()
	{
		static const char* const s_Dlls[] = { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" };
		int i;

		if (g_nM7PadAvailable >= 0)
			return g_nM7PadAvailable != 0;

		g_pM7TestFile = getenv("AVP2SRC_PADTEST");
		if (g_pM7TestFile && !*g_pM7TestFile)
			g_pM7TestFile = NULL;

		if (g_pM7TestFile)
		{
			g_nM7PadAvailable = 1;
			return true;
		}

		for (i = 0; i < 3 && !g_pM7GetState; i++)
		{
			HMODULE hXInput = LoadLibrary(s_Dlls[i]);

			if (hXInput)
			{
				g_pM7GetState = (M7XInputGetState)GetProcAddress(hXInput, "XInputGetState");
				if (!g_pM7GetState)
					FreeLibrary(hXInput);
			}
		}

		g_nM7PadAvailable = g_pM7GetState ? 1 : 0;
		return g_nM7PadAvailable != 0;
	}

	// The test pad's file, read at most every 15 ms.
	bool M7PadRead_Test(M7PadState* pPad)
	{
		DWORD dwNow = GetTickCount();

		if (!g_bM7TestRead || dwNow - g_dwM7TestRead >= 15)
		{
			FILE* pFile = fopen(g_pM7TestFile, "r");
			int   n[7];

			g_bM7TestRead  = true;
			g_dwM7TestRead = dwNow;
			g_bM7TestPad   = false;

			if (pFile)
			{
				if (fscanf(pFile, "%i %i %i %i %i %i %i",
				           &n[0], &n[1], &n[2], &n[3], &n[4], &n[5], &n[6]) == 7)
				{
					g_M7TestState.wButtons      = (WORD)n[0];
					g_M7TestState.sThumbLX      = (SHORT)n[1];
					g_M7TestState.sThumbLY      = (SHORT)n[2];
					g_M7TestState.sThumbRX      = (SHORT)n[3];
					g_M7TestState.sThumbRY      = (SHORT)n[4];
					g_M7TestState.bLeftTrigger  = (BYTE)n[5];
					g_M7TestState.bRightTrigger = (BYTE)n[6];
					g_bM7TestPad = true;
				}

				fclose(pFile);
			}
		}

		*pPad = g_M7TestState;
		return g_bM7TestPad;
	}

	// The pad's state; false, and everything released, when none is
	// connected.
	bool M7PadRead_State(M7PadState* pPad)
	{
		M7XInputState state;
		DWORD         dwNow;
		int           i;

		memset(pPad, 0, sizeof(*pPad));

		if (!M7PadRead_Available())
			return false;

		if (g_pM7TestFile)
			return M7PadRead_Test(pPad);

		if (g_nM7PadSlot >= 0)
		{
			if (g_pM7GetState(g_nM7PadSlot, &state) == ERROR_SUCCESS)
			{
				*pPad = state.Gamepad;
				return true;
			}

			g_nM7PadSlot = -1;
		}

		dwNow = GetTickCount();
		if (g_bM7Scanned && dwNow - g_dwM7LastScan < 1000)
			return false;

		g_bM7Scanned    = true;
		g_dwM7LastScan  = dwNow;

		for (i = 0; i < 4; i++)
		{
			if (g_pM7GetState(i, &state) == ERROR_SUCCESS)
			{
				g_nM7PadSlot = i;
				*pPad        = state.Gamepad;
				return true;
			}
		}

		return false;
	}
}

#endif // __M7_PADREAD_H__
