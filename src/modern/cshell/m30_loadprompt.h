// ----------------------------------------------------------------------- //
//
// MODULE  : m30_loadprompt.h
//
// PURPOSE : The modern build's "Press any key to continue." after a level
//           has loaded (docs/MODERN.md, M-30; nolfsrc M-P3-27).
//
//           On a computer of today a level loads in one or two seconds,
//           and the loading screen goes away before the player can see
//           it.  After a new level of a single player mission has loaded,
//           the loading screen stays with "Press any key to continue."
//           until the player presses a key, the left mouse button or a
//           controller button (M-7), as No One Lives Forever 2 and
//           Contract J.A.C.K. do (their CScreenPostload):
//
//            - single player only;
//            - only a level of a mission: the first level of a mission
//              (CInterfaceMgr::StartMission) and the next level
//              (CInterfaceMgr::HandleLoadNewLevel).  Not a saved game,
//              a custom level (also +runworld), a demo, a recording or
//              the outro;
//            - no time-out;
//            - the text flashes, white and black in turn, every 0.333 s;
//            - the console variable LoadScreenWait 0 turns it off;
//            - while it waits, the server is paused (MID_GAME_PAUSE), the
//              bound commands do nothing, and the frame-rate limit (M-13)
//              applies;
//            - the game goes on when the key is released, not when it is
//              pressed, so the key does not also act in play.
//
//           The prompt comes where the loading screen would go away
//           (CInterfaceMgr::UpdateLoadingLevelState).  The loading
//           screen, the level's start and its cinematic do not change:
//           they come after the key, as they came after the load.
//
//           Retail files include this inside #ifdef AVP2SRC_MODERN; one
//           of them (GameClientShell.cpp) defines
//           M30_LOADPROMPT_IMPLEMENTATION first and so holds the code.
//           The calls:
//
//             InterfaceMgr.cpp     StartMission, HandleLoadNewLevel
//                                                M30LoadPrompt_Want
//                                  UpdateLoadingLevelState
//                                                M30LoadPrompt_Wait
//             GameClientShell.cpp  DoLoadWorld   M30LoadPrompt_OnLoadWorld
//                                  OnCommandOn   M30LoadPrompt_IsWaiting
//                                  PostUpdate    M30LoadPrompt_IsWaiting
//             LoadingScreen.cpp    Update        M30LoadPrompt_Draw
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M30_LOADPROMPT_H__
#define __M30_LOADPROMPT_H__

#define M30_VAR_LOADSCREENWAIT	"LoadScreenWait"

class CLoadingScreen;

// StartMission and HandleLoadNewLevel, before their LoadWorld: the next
// load is a level of a mission.
void	M30LoadPrompt_Want();

// CGameClientShell::DoLoadWorld, before the loading state: a load with
// bCanWait (not a saved game, not a demo) that M30LoadPrompt_Want asked
// for waits for a key once loaded.  Each load uses up the request.
void	M30LoadPrompt_OnLoadWorld(LTBOOL bCanWait);

// CInterfaceMgr::UpdateLoadingLevelState, once the player is in the
// world: LTTRUE while the loading screen stays (this draws it); LTFALSE
// to go on into play as retail does.
LTBOOL	M30LoadPrompt_Wait(CLoadingScreen & screen);

// LTTRUE while the prompt waits.
LTBOOL	M30LoadPrompt_IsWaiting();

// CLoadingScreen::Update: the prompt, while it waits (in the menu space,
// on the 640x480 page).
void	M30LoadPrompt_Draw(HSURFACE hDest);

#ifdef M30_LOADPROMPT_IMPLEMENTATION

#include "LoadingScreen.h"
#include "VarTrack.h"
#include "WinUtil.h"

extern HWND g_hMainWnd;		// GameClientShell.cpp


namespace
{
	enum EM30State
	{
		M30_NONE,			// no prompt for this load
		M30_WANTED,			// a prompt once the level has loaded
		M30_WAITING			// the prompt is up
	};

	const LTFLOAT	c_fM30FlashTime	= 0.333f;

	// The prompt's place on the loading screen's 640x480 page: centred,
	// under the level's name (CLoadingScreen's m_TextPos, 320,320).
	const int		c_nM30PromptX	= 320;
	const int		c_nM30PromptY	= 420;

	VarTrack	s_vtM30Wait;
	LTBOOL		s_bM30Want = LTFALSE;
	EM30State	s_eM30State = M30_NONE;
	HSTRING		s_hM30Prompt = LTNULL;
	LTBOOL		s_bM30Flash;
	LTFLOAT		s_fM30NextFlash;
	LTBOOL		s_bM30SeenUp;		// all keys and buttons were up after the prompt came
	LTBOOL		s_bM30Pressed;		// then one went down: go on when all are up again

	void M30SendToServer(uint8 nMsg)
	{
		HMESSAGEWRITE hMessage = g_pLTClient->StartMessage(nMsg);
		g_pLTClient->EndMessage(hMessage);
	}

	// A key or the left mouse button held, or pressed after the last
	// call; a controller button held.  Not the right, middle or extra
	// mouse buttons: NOLF2's screen takes the left button only.
	LTBOOL M30AnyInputDown()
	{
		LTBOOL bDown = LTFALSE;
		int nKey;

		if (GetForegroundWindow() != g_hMainWnd)
			return LTFALSE;

		for (nKey = 1; nKey < 0xFF; nKey++)
		{
			if (nKey == VK_RBUTTON || nKey == VK_CANCEL || nKey == VK_MBUTTON ||
				nKey == 0x05 || nKey == 0x06)		// VK_XBUTTON1, VK_XBUTTON2
				continue;

			// (all keys are read: this clears the "pressed after the
			// last call" bit of each)
			if (GetAsyncKeyState(nKey) & 0x8001)
				bDown = LTTRUE;
		}

		if (M7Pad_AnyButtonDown())
			bDown = LTTRUE;

		return bDown;
	}

	void M30StartPrompt(CLoadingScreen & screen)
	{
		// The loading screen's thread stops; from now on this draws it.
		screen.Pause();

		if (!s_hM30Prompt)
			s_hM30Prompt = g_pLTClient->FormatString(IDS_M30_PRESS_ANY_KEY);

		// The game does not run while the prompt waits.
		M30SendToServer(MID_GAME_PAUSE);

		s_bM30Flash     = LTFALSE;
		s_fM30NextFlash = CWinUtil::GetTime() + c_fM30FlashTime;
		s_bM30SeenUp    = LTFALSE;
		s_bM30Pressed   = LTFALSE;
		M30AnyInputDown();		// forget what was pressed while it loaded

		s_eM30State = M30_WAITING;
	}

	void M30EndPrompt()
	{
		M30SendToServer(MID_GAME_UNPAUSE);

		// The key that went on is not a command in play.
		g_pLTClient->ClearInput();

		s_eM30State = M30_NONE;
	}
}

void M30LoadPrompt_Want()
{
	s_bM30Want = LTTRUE;
}

void M30LoadPrompt_OnLoadWorld(LTBOOL bCanWait)
{
	if (!s_vtM30Wait.IsInitted())
		s_vtM30Wait.Init(g_pLTClient, M30_VAR_LOADSCREENWAIT, LTNULL, 1.0f);

	// A load that starts while a prompt waits (a failed load can leave
	// one): the server that it paused is gone.
	s_eM30State = M30_NONE;

	if (s_bM30Want && bCanWait && s_vtM30Wait.GetFloat() != 0.0f)
		s_eM30State = M30_WANTED;

	s_bM30Want = LTFALSE;
}

LTBOOL M30LoadPrompt_Wait(CLoadingScreen & screen)
{
	switch (s_eM30State)
	{
		case M30_WANTED:
		{
			if (g_pGameClientShell->IsMultiplayerGame() || !screen.IsVisible())
			{
				s_eM30State = M30_NONE;
				return LTFALSE;
			}

			M30StartPrompt(screen);
			screen.Update();
			return LTTRUE;
		}

		case M30_WAITING:
		{
			if (M30AnyInputDown())
			{
				if (s_bM30SeenUp)
					s_bM30Pressed = LTTRUE;
			}
			else
			{
				s_bM30SeenUp = LTTRUE;

				// Go on when the key is released, so that it does not go
				// on into play (Space would jump, the left button fire).
				if (s_bM30Pressed)
				{
					M30EndPrompt();
					return LTFALSE;
				}
			}

			screen.Update();
			return LTTRUE;
		}

		default:
			return LTFALSE;
	}
}

LTBOOL M30LoadPrompt_IsWaiting()
{
	return s_eM30State == M30_WAITING;
}

void M30LoadPrompt_Draw(HSURFACE hDest)
{
	CLTGUIFont* pFont;
	LTFLOAT fTime;
	HLTCOLOR hColor, hShadow;
	LITHFONTDRAWDATA dd;
	int x, y;

	if (s_eM30State != M30_WAITING || !s_hM30Prompt)
		return;

	fTime = CWinUtil::GetTime();
	if (fTime > s_fM30NextFlash)
	{
		s_bM30Flash     = !s_bM30Flash;
		s_fM30NextFlash = fTime + c_fM30FlashTime;
	}

	hColor  = s_bM30Flash ? kBlack : kWhite;
	hShadow = s_bM30Flash ? kWhite : kBlack;

	// Smaller than the level's name above it (the title font), as
	// NOLF2's prompt is smaller than its title.
	pFont = g_pInterfaceResMgr->GetLargeFont();
	if (!pFont)
		return;

	x = c_nM30PromptX + g_pInterfaceResMgr->GetXOffset();
	y = c_nM30PromptY + g_pInterfaceResMgr->GetYOffset();

	// Solid: the fonts of the menus have colours of their own, and a
	// plain Draw keeps them
	dd.dwFlags = LTF_DRAW_SOLID;
	dd.byJustify = LTF_JUSTIFY_CENTER;
	dd.bForceFormat = LTTRUE;

	dd.hColor = hShadow;
	pFont->Draw(g_pLTClient->GetStringData(s_hM30Prompt), hDest, &dd, x + 1, y + 1);
	dd.hColor = hColor;
	pFont->Draw(g_pLTClient->GetStringData(s_hM30Prompt), hDest, &dd, x, y);
}

#endif // M30_LOADPROMPT_IMPLEMENTATION

#endif // __M30_LOADPROMPT_H__
