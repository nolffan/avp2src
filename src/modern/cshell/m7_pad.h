// ----------------------------------------------------------------------- //
//
// MODULE  : m7_pad.h
//
// PURPOSE : The modern build's controller (docs/MODERN.md, M-7), the
//           client shell's half.  The engine's ##gamepad device
//           (src/modern/engine/p7_gamepad.h) gives its buttons to the
//           bindings like any key's, through the profile's joystick
//           column (m7_padbindings.h); this reads the pad itself for what
//           must not depend on a binding:
//
//           Look: the right stick past a radial dead zone (PadLookDeadZone),
//           the rest of its travel rescaled to 0..1 and raised to
//           PadLookCurve (fine aim near the centre), is a turn rate --
//           180 degrees a second across and 120 up and down, times
//           PadLookSpeed and the frame time -- added to the mouse's axis
//           offsets where PlayerMovement reads them (an offset of 1 is 90
//           degrees of yaw there, 45 of pitch).  So it turns the Primal
//           Hunt turrets too, keeps to what multiplayer allows the mouse,
//           and slows with the Predator's zoom as the mouse does (the scale
//           SetMouseSensitivityScale is given).  PadInvertY turns its pitch
//           over; the mouse's own invert applies on top.
//
//           Movement: the left stick past PadMoveDeadZone sets the
//           movement flags by octant (what the animations and the server
//           see), and its direction is the acceleration's
//           (CharacterMovement::UpdateAcceleration); past three-quarters of
//           its travel it runs, below that it walks at that share of the
//           walk speed, a third at least.  Run held turns that over, as it
//           does the keyboard's run; RunLock does not apply to the stick.
//
//           Menus: outside play the d-pad or the left stick is the arrow
//           keys (repeating after 0.4 s, every 0.12 s), A Enter, B and
//           Start the menu key (Escape), the right bumper Tab (the next
//           tab of a page that has them).  In play Start is the menu key,
//           and A skips a cinematic (the cinematic skip key) or goes on
//           from death (any key: Space).  The keys go to the client shell's
//           own OnKeyDown / OnKeyUp, as the engine delivers a key.
//
//           Retail files include this inside #ifdef AVP2SRC_MODERN; one
//           of them (GameClientShell.cpp) defines M7_PAD_IMPLEMENTATION
//           first and so holds the code.  The pad is read once a frame
//           (M7Pad_Update), while the game's window has the foreground,
//           through engine/m7_padread.h -- the reader the engine's device
//           uses, so the test pad (AVP2SRC_PADTEST) drives both.
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M7_PAD_H__
#define __M7_PAD_H__

// The console variables (saved in autoexec.cfg) and their defaults
#define M7_VAR_LOOKSPEED		"PadLookSpeed"
#define M7_VAR_LOOKCURVE		"PadLookCurve"
#define M7_VAR_LOOKDEADZONE		"PadLookDeadZone"
#define M7_VAR_MOVEDEADZONE		"PadMoveDeadZone"
#define M7_VAR_INVERTY			"PadInvertY"
#define M7_DEF_LOOKSPEED		1.0f
#define M7_DEF_LOOKCURVE		2.0f
#define M7_DEF_LOOKDEADZONE		0.15f
#define M7_DEF_MOVEDEADZONE		0.2f
#define M7_DEF_INVERTY			0.0f

void	M7Pad_Update(LTFLOAT fFrameTime);			// CGameClientShell::Update, every frame
void	M7Pad_SetLookScale(LTFLOAT fScale);			// SetMouseSensitivityScale's (the zoom's)
LTBOOL	M7Pad_IsLooking();							// the right stick is out of its dead zone, in play
void	M7Pad_AddLook(LTFLOAT* pOffsets);			// the right stick, into the axis offsets
void	M7Pad_MoveFlags(uint32* pdwCtrl);			// the left stick's direction and run flags
LTBOOL	M7Pad_GetMove(LTFLOAT* pRight, LTFLOAT* pForward);	// the left stick's direction, a unit vector
LTFLOAT	M7Pad_SpeedScale();							// the share of the walk speed the stick asks for

// A Pad* variable's value (its default if it is not set).
inline LTFLOAT M7Pad_GetVar(const char* pName, LTFLOAT fDefault)
{
	HCONSOLEVAR hVar = g_pLTClient->GetConsoleVar((char*)pName);
	return hVar ? g_pLTClient->GetVarValueFloat(hVar) : fDefault;
}

// Set one, to be saved in autoexec.cfg.
inline void M7Pad_SetVar(const char* pName, LTFLOAT fValue)
{
	char szCmd[96];
	sprintf(szCmd, "+%s %f", pName, fValue);
	g_pLTClient->RunConsoleString(szCmd);
}

// Is there a ##gamepad (the engine found XInput, or the test pad)?  It is
// then the joystick, whose column of the profile's bindings is its.
inline LTBOOL M7Pad_DeviceExists()
{
	char szName[64];

	memset(szName, 0, sizeof(szName));
	if (g_pLTClient->GetDeviceName(DEVICETYPE_JOYSTICK, szName, sizeof(szName) - 1) != LT_OK)
		return LTFALSE;

	return stricmp(szName, "##gamepad") == 0;
}

#ifdef M7_PAD_IMPLEMENTATION

#include "..\engine\m7_padread.h"
#include <math.h>

extern HWND g_hMainWnd;		// GameClientShell.cpp

namespace
{
	const LTFLOAT	c_fM7YawRate	= 180.0f / 90.0f;	// degrees a second, in yaw offsets (90 degrees)
	const LTFLOAT	c_fM7PitchRate	= 120.0f / 45.0f;	// ... in pitch offsets (45 degrees)
	const LTFLOAT	c_fM7RunAt		= 0.75f;			// of the stick's travel past the dead zone
	const LTFLOAT	c_fM7MinWalk	= 1.0f / 3.0f;
	const LTFLOAT	c_fM7Octant		= 0.3827f;			// sin(22.5 degrees)
	const LTFLOAT	c_fM7RepeatDelay = 0.4f;			// menu keys: the first repeat, then every
	const LTFLOAT	c_fM7RepeatRate	= 0.12f;

	M7PadState	g_M7Pad;
	LTFLOAT		g_fM7FrameTime;
	LTFLOAT		g_fM7LookScale = 1.0f;
	LTFLOAT		g_fM7MoveX, g_fM7MoveY, g_fM7MoveMag;	// past the dead zone; 0 when not moving
	LTFLOAT		g_fM7LookX, g_fM7LookY;
	WORD		g_wM7PrevButtons;
	int			g_nM7HeldKey;
	LTFLOAT		g_fM7HeldTime;
	LTFLOAT		g_fM7NextRepeat;

	// A stick, -32768..32767 a side (Y up positive), past a radial dead
	// zone: its direction kept, its length rescaled from the dead zone's
	// edge to 1.  Returns the length.
	LTFLOAT M7Stick(SHORT nX, SHORT nY, LTFLOAT fDeadZone, LTFLOAT* pX, LTFLOAT* pY)
	{
		LTFLOAT fX = nX / 32767.0f, fY = nY / 32767.0f;
		LTFLOAT fLen = (LTFLOAT)sqrt(fX * fX + fY * fY);
		LTFLOAT fScaled;

		if (fDeadZone < 0.0f) fDeadZone = 0.0f;
		if (fDeadZone > 0.9f) fDeadZone = 0.9f;

		if (fLen <= fDeadZone)
		{
			*pX = *pY = 0.0f;
			return 0.0f;
		}

		fScaled = (fLen - fDeadZone) / (1.0f - fDeadZone);
		if (fScaled > 1.0f)
			fScaled = 1.0f;

		*pX = fX / fLen * fScaled;
		*pY = fY / fLen * fScaled;
		return fScaled;
	}

	// A key, down and up, as the engine gives one to the client shell.
	void M7PressKey(int nKey)
	{
		IClientShell* pShell = g_pGameClientShell;

		pShell->OnKeyDown(nKey, 1);
		pShell->OnKeyUp(nKey);
	}

	// The arrow key the d-pad or the left stick holds, or 0.
	int M7MenuDirection()
	{
		LTFLOAT fX, fY;

		if (g_M7Pad.wButtons & M7PAD_DPAD_UP)		return VK_UP;
		if (g_M7Pad.wButtons & M7PAD_DPAD_DOWN)		return VK_DOWN;
		if (g_M7Pad.wButtons & M7PAD_DPAD_LEFT)		return VK_LEFT;
		if (g_M7Pad.wButtons & M7PAD_DPAD_RIGHT)	return VK_RIGHT;

		if (M7Stick(g_M7Pad.sThumbLX, g_M7Pad.sThumbLY, 0.5f, &fX, &fY) <= 0.0f)
			return 0;

		if (fabs(fY) >= fabs(fX))
			return fY > 0.0f ? VK_UP : VK_DOWN;

		return fX > 0.0f ? VK_RIGHT : VK_LEFT;
	}

	void M7UpdateKeys(WORD wPressed)
	{
		GameState eState = g_pInterfaceMgr->GetGameState();
		int nKey;

		if (eState == GS_PLAYING || eState == GS_DIALOGUE || eState == GS_LOADINGLEVEL)
		{
			g_nM7HeldKey = 0;

			if (eState == GS_LOADINGLEVEL)
				return;

			if ((wPressed & M7PAD_START) && eState == GS_PLAYING)
				M7PressKey(g_pClientButeMgr->GetEscapeKey());

			// A, bound to Jump (a command, not a key), skips a cinematic
			// or a line of dialogue, and goes on from the death of a
			// single player game (which takes any key)
			if (wPressed & M7PAD_A)
			{
				if (eState == GS_DIALOGUE || g_pGameClientShell->IsCinematic())
					M7PressKey(g_pClientButeMgr->GetCinematicSkipKey());
				else if (g_pGameClientShell->IsPlayerDead() && !g_pGameClientShell->IsMultiplayerGame())
					M7PressKey(VK_SPACE);
			}

			return;
		}

		if (wPressed & M7PAD_A)
			M7PressKey(VK_RETURN);

		if (wPressed & (M7PAD_B | M7PAD_START))
			M7PressKey(g_pClientButeMgr->GetEscapeKey());

		if (wPressed & M7PAD_RIGHT_SHOULDER)
			M7PressKey(VK_TAB);

		nKey = M7MenuDirection();

		if (nKey != g_nM7HeldKey)
		{
			g_nM7HeldKey    = nKey;
			g_fM7HeldTime   = 0.0f;
			g_fM7NextRepeat = c_fM7RepeatDelay;

			if (nKey)
				M7PressKey(nKey);
		}
		else if (nKey)
		{
			g_fM7HeldTime += g_fM7FrameTime;

			if (g_fM7HeldTime >= g_fM7NextRepeat)
			{
				g_fM7NextRepeat += c_fM7RepeatRate;
				M7PressKey(nKey);
			}
		}
	}
}

void M7Pad_Update(LTFLOAT fFrameTime)
{
	WORD wPressed;

	g_fM7FrameTime = fFrameTime;

	if (!M7PadRead_Available() || GetForegroundWindow() != g_hMainWnd || !M7PadRead_State(&g_M7Pad))
		memset(&g_M7Pad, 0, sizeof(g_M7Pad));

	wPressed         = g_M7Pad.wButtons & ~g_wM7PrevButtons;
	g_wM7PrevButtons = g_M7Pad.wButtons;

	if (g_pInterfaceMgr->GetGameState() == GS_PLAYING)
	{
		g_fM7MoveMag = M7Stick(g_M7Pad.sThumbLX, g_M7Pad.sThumbLY,
		                       M7Pad_GetVar(M7_VAR_MOVEDEADZONE, M7_DEF_MOVEDEADZONE), &g_fM7MoveX, &g_fM7MoveY);
		M7Stick(g_M7Pad.sThumbRX, g_M7Pad.sThumbRY,
		        M7Pad_GetVar(M7_VAR_LOOKDEADZONE, M7_DEF_LOOKDEADZONE), &g_fM7LookX, &g_fM7LookY);
	}
	else
	{
		g_fM7MoveX = g_fM7MoveY = g_fM7MoveMag = 0.0f;
		g_fM7LookX = g_fM7LookY = 0.0f;
	}

	M7UpdateKeys(wPressed);
}

void M7Pad_SetLookScale(LTFLOAT fScale)
{
	g_fM7LookScale = fScale;
}

LTBOOL M7Pad_IsLooking()
{
	return (g_fM7LookX != 0.0f || g_fM7LookY != 0.0f) ? LTTRUE : LTFALSE;
}

void M7Pad_AddLook(LTFLOAT* pOffsets)
{
	LTFLOAT fLen = (LTFLOAT)sqrt(g_fM7LookX * g_fM7LookX + g_fM7LookY * g_fM7LookY);
	LTFLOAT fCurve, fScale;

	if (fLen <= 0.0f)
		return;

	// The curve on the length, so that the direction stays the stick's
	fCurve = M7Pad_GetVar(M7_VAR_LOOKCURVE, M7_DEF_LOOKCURVE);
	if (fCurve < 1.0f)
		fCurve = 1.0f;

	fScale = (LTFLOAT)pow(fLen, fCurve) / fLen * M7Pad_GetVar(M7_VAR_LOOKSPEED, M7_DEF_LOOKSPEED) *
	         g_fM7LookScale * g_fM7FrameTime;

	pOffsets[0] += g_fM7LookX * fScale * c_fM7YawRate;

	// A positive pitch offset looks down: the stick's up looks up, unless
	// inverted
	if (M7Pad_GetVar(M7_VAR_INVERTY, M7_DEF_INVERTY) != 0.0f)
		pOffsets[1] += g_fM7LookY * fScale * c_fM7PitchRate;
	else
		pOffsets[1] -= g_fM7LookY * fScale * c_fM7PitchRate;
}

void M7Pad_MoveFlags(uint32* pdwCtrl)
{
	LTBOOL bRun;

	if (g_fM7MoveMag <= 0.0f)
		return;

	if (g_fM7MoveY >  c_fM7Octant * g_fM7MoveMag) *pdwCtrl |= CM_FLAG_FORWARD;
	if (g_fM7MoveY < -c_fM7Octant * g_fM7MoveMag) *pdwCtrl |= CM_FLAG_BACKWARD;
	if (g_fM7MoveX >  c_fM7Octant * g_fM7MoveMag) *pdwCtrl |= CM_FLAG_STRAFERIGHT;
	if (g_fM7MoveX < -c_fM7Octant * g_fM7MoveMag) *pdwCtrl |= CM_FLAG_STRAFELEFT;

	// How far it is pushed walks or runs, not RunLock; Run held turns that
	// over
	bRun = g_fM7MoveMag >= c_fM7RunAt;
	if (g_pLTClient->IsCommandOn(COMMAND_ID_RUN))
		bRun = !bRun;

	if (bRun)
		*pdwCtrl |= CM_FLAG_RUN;
	else
		*pdwCtrl &= ~CM_FLAG_RUN;
}

LTBOOL M7Pad_GetMove(LTFLOAT* pRight, LTFLOAT* pForward)
{
	if (g_fM7MoveMag <= 0.0f)
		return LTFALSE;

	*pRight   = g_fM7MoveX / g_fM7MoveMag;
	*pForward = g_fM7MoveY / g_fM7MoveMag;
	return LTTRUE;
}

LTFLOAT M7Pad_SpeedScale()
{
	LTFLOAT fScale;

	if (g_fM7MoveMag <= 0.0f || g_fM7MoveMag >= c_fM7RunAt)
		return 1.0f;

	fScale = g_fM7MoveMag / c_fM7RunAt;
	return fScale < c_fM7MinWalk ? c_fM7MinWalk : fScale;
}

#endif // M7_PAD_IMPLEMENTATION

#endif // __M7_PAD_H__
