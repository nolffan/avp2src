// ----------------------------------------------------------------------- //
//
// MODULE  : m25_console.h
//
// PURPOSE : The modern build's console (docs/MODERN.md, M-25), for AvP2
//           and Primal Hunt.  Retail has no console that a player can
//           open (the ~ key does nothing).
//
//           A console over the top half of the screen, in single player
//           only.  It shows the console output of the engine and of the
//           game (ILTClient::SetConsoleOutputFn), an edit line, the
//           commands typed before (Up, Down) and the lines before
//           (Page Up, Page Down, Home, End).  The game pauses while the
//           console is open: the picture stays as it was (a copy of the
//           screen, as the pause screen of the game does), the server
//           pauses and the sounds stop.  While the console is open, it
//           gets each key: no game command and no other key handling.
//
//           Enter runs the line:
//             - a cheat code of CheatMgr.cpp ("mpcanthurtme") runs as a
//               cheat, with its sound;
//             - else the client runs it as a console command.  If its
//               first word is not a command of the client (a variable,
//               or a command that only the server knows), the client
//               also sends it to the server (MID_M25_CONSOLE_COMMAND),
//               which runs it only from the local client (m25_server.h).
//
//           Console programs: Help (the console commands), Cheats (the
//           cheat codes and what they do), GetPos and SetPos (the
//           position of the player).
//
//           The ToggleConsole control (COMMAND_ID_TOGGLE_CONSOLE) opens
//           and closes it.  Its key is the key to the left of 1 by
//           default (m25_consolekey.h); Customize Controls changes it.
//           Shift+Esc opens it too (a keyboard can have a different key
//           there).  Esc closes it.  It does not open in multiplayer,
//           during a loading screen, in Customize Controls, in a dialogue
//           or while the chat line or a message box is open.
//
//           ConsoleBackdrop (console variable): 0 (the default) the
//           console.pcx of the game directory (the facehugger of the
//           retail engine's console), 1 black.
//
//           The console is drawn in the HUD's space (M-16), so its text
//           has the size of the HUD's text.
//
//           Retail files include this inside #ifdef AVP2SRC_MODERN; one
//           of them (GameClientShell.cpp) defines M25_CONSOLE_IMPLEMENTATION
//           first and so holds the code.  The calls:
//
//             GameClientShell.cpp  OnEngineInitialized   M25Console_Init
//                                  OnEngineTerm          M25Console_Term
//                                  OnKeyDown / OnKeyUp   M25Console_OnKeyDown / _OnKeyUp
//                                  OnCommandOn           M25Console_OnCommandOn
//             InterfaceMgr.cpp     OnChar                M25Console_OnChar
//                                  Update                M25Console_Update
//                                  PostUpdate            M25Console_Draw (before the flip)
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M25_CONSOLE_H__
#define __M25_CONSOLE_H__

void	M25Console_Init();
void	M25Console_Term();

LTBOOL	M25Console_IsVisible();

// LTTRUE: the console took the key, the character or the command.
LTBOOL	M25Console_OnKeyDown(int nKey, int nRep);
LTBOOL	M25Console_OnKeyUp(int nKey);
LTBOOL	M25Console_OnChar(char c);
LTBOOL	M25Console_OnCommandOn(int nCommand);

// CInterfaceMgr::Update: LTTRUE when the console holds the game (the copy
// of the screen is drawn; the frame of the world is not).
LTBOOL	M25Console_Update();

// CInterfaceMgr::PostUpdate, before the flip.
void	M25Console_Draw();

#ifdef M25_CONSOLE_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include "GameClientShell.h"
#include "InterfaceMgr.h"
#include "InterfaceResMgr.h"
#include "BaseFolder.h"
#include "ClientSoundMgr.h"
#include "CheatMgr.h"
#include "CommandIDs.h"
#include "MsgIDs.h"
#include "VarTrack.h"
#include "..\..\modern\cshell\m15_uiscale.h"

extern CGameClientShell* g_pGameClientShell;

#define M25_MAX_LINES		512		// the lines kept
#define M25_LINE_LEN		160		// the characters kept of a line
#define M25_MAX_RECALL		32		// the commands that Up and Down recall
#define M25_EDIT_LEN		128		// the edit line
#define M25_MAX_HELP		512		// the commands that Help can list

#define M25_COLOR_EDIT		SETRGB(255, 255, 255)
#define M25_COLOR_ECHO		SETRGB(160, 255, 160)
#define M25_COLOR_NOTE		SETRGB(120, 200, 120)
#define M25_COLOR_EDGE		SETRGB(40, 160, 40)

#define M25_VAR_BACKDROP	"ConsoleBackdrop"
#define M25_BACKDROP_FILE	"console.pcx"

struct M25ConLine
{
	uint32	m_nColor;
	char	m_szText[M25_LINE_LEN];
};

// The lines: a ring of s_nM25Lines lines that ends at s_nM25Head - 1.  The
// engine can print from a different thread (the loading screen), thus a
// lock protects the ring.
static M25ConLine		s_aM25Lines[M25_MAX_LINES];
static int				s_nM25Head = 0;
static int				s_nM25Lines = 0;
static CRITICAL_SECTION	s_csM25Lines;
static LTBOOL			s_bM25CSInit = LTFALSE;

// The commands that Up and Down recall: also a ring
static char				s_aszM25Recall[M25_MAX_RECALL][M25_EDIT_LEN];
static int				s_nM25RecallHead = 0;
static int				s_nM25RecallCount = 0;
static int				s_nM25RecallPos = -1;		// -1: no recall

static char				s_szM25Edit[M25_EDIT_LEN];
static int				s_nM25EditLen = 0;

static LTBOOL			s_bM25Init = LTFALSE;
static LTBOOL			s_bM25Visible = LTFALSE;
static LTBOOL			s_bM25Paused = LTFALSE;		// the console paused the game
static LTBOOL			s_bM25Hold = LTFALSE;		// it holds the game (opened in play)
static LTBOOL			s_bM25Shift = LTFALSE;		// a Shift key is down
static int				s_nM25Scroll = 0;			// lines back from the newest
static int				s_nM25Visible = 1;			// the lines of the last frame

static HSURFACE			s_hM25Copy = LTNULL;		// the screen, as the console opened
static HSURFACE			s_hM25Backdrop = LTNULL;	// console.pcx
static HSURFACE			s_hM25Black = LTNULL;
static HSURFACE			s_hM25Edge = LTNULL;
static LTBOOL			s_bM25NoBackdrop = LTFALSE;	// console.pcx did not load

static VarTrack			s_vtM25Backdrop;

// Help: the names of the commands, from the engine
static const char*		s_apM25HelpNames[M25_MAX_HELP];
static uint32			s_anM25HelpFlags[M25_MAX_HELP];
static int				s_nM25Help = 0;


// ----------------------------------------------------------------------- //
//  The lines
// ----------------------------------------------------------------------- //

static void M25AddLine(uint32 nColor, const char* pText, int nLen)
{
	M25ConLine* pLine;

	if (nLen >= M25_LINE_LEN)
		nLen = M25_LINE_LEN - 1;

	pLine = &s_aM25Lines[s_nM25Head];
	pLine->m_nColor = nColor;
	memcpy(pLine->m_szText, pText, nLen);
	pLine->m_szText[nLen] = '\0';

	// The bitmap fonts have no tab
	for (char* p = pLine->m_szText; *p; p++)
	{
		if (*p == '\t' || *p == '\r')
			*p = ' ';
	}

	s_nM25Head = (s_nM25Head + 1) % M25_MAX_LINES;
	if (s_nM25Lines < M25_MAX_LINES)
		s_nM25Lines++;

	// A player who reads back stays on the same line
	if (s_nM25Scroll > 0 && s_nM25Scroll < s_nM25Lines - 1)
		s_nM25Scroll++;
}

// Each line that the engine's console prints (ILTClient::
// SetConsoleOutputFn).  The engine calls this in the lock of its console:
// never print from here.
static void M25OnConsoleLine(void* pUser, uint32 nColor, int nLevel, const char* pText)
{
	const char* pStart;
	const char* pEnd;

	if (!pText || !s_bM25CSInit)
		return;

	// Colour 0 is the font's own colour: white
	nColor &= 0x00ffffff;
	if (!nColor)
		nColor = SETRGB(255, 255, 255);

	EnterCriticalSection(&s_csM25Lines);

	// One line for each piece between the '\n's, as the engine's console
	// keeps them (a '\n' at the end makes no empty line)
	pStart = pText;
	for (;;)
	{
		pEnd = strchr(pStart, '\n');
		if (!pEnd)
		{
			if (*pStart || pStart == pText)
				M25AddLine(nColor, pStart, strlen(pStart));
			break;
		}

		M25AddLine(nColor, pStart, pEnd - pStart);
		pStart = pEnd + 1;
		if (!*pStart)
			break;
	}

	LeaveCriticalSection(&s_csM25Lines);
}


// ----------------------------------------------------------------------- //
//  The console programs
// ----------------------------------------------------------------------- //

static void M25GatherHelpName(void* pUser, const char* pName, uint32 nFlags)
{
	if (s_nM25Help < M25_MAX_HELP)
	{
		s_apM25HelpNames[s_nM25Help] = pName;
		s_anM25HelpFlags[s_nM25Help] = nFlags;
		s_nM25Help++;
	}
}

static int M25CompareHelpNames(const void* p1, const void* p2)
{
	return stricmp(s_apM25HelpNames[*(const int*)p1], s_apM25HelpNames[*(const int*)p2]);
}

// One group (the game's programs or the engine's commands), sorted,
// some names on each line
static void M25PrintHelpGroup(const char* pTitle, uint32 nFlags)
{
	int  anOrder[M25_MAX_HELP];
	int  nCount = 0;
	int  i;
	char szLine[96];
	int  nLineLen;

	for (i = 0; i < s_nM25Help; i++)
	{
		if ((s_anM25HelpFlags[i] & 1) == nFlags)
			anOrder[nCount++] = i;
	}

	qsort(anOrder, nCount, sizeof(int), M25CompareHelpNames);

	g_pLTClient->CPrint("%s (%d):", pTitle, nCount);

	szLine[0] = '\0';
	nLineLen = 0;
	for (i = 0; i < nCount; i++)
	{
		const char* pName = s_apM25HelpNames[anOrder[i]];
		int nLen = strlen(pName);

		if (nLen > (int)sizeof(szLine) - 3)
			continue;

		if (nLineLen && nLineLen + 2 + nLen > 64)
		{
			g_pLTClient->CPrint("  %s", szLine);
			szLine[0] = '\0';
			nLineLen = 0;
		}

		if (nLineLen)
		{
			strcat(szLine, ", ");
			nLineLen += 2;
		}
		strcat(szLine, pName);
		nLineLen += nLen;
	}

	if (nLineLen)
		g_pLTClient->CPrint("  %s", szLine);
}

static void M25HelpFn(int argc, char** argv)
{
	s_nM25Help = 0;
	g_pLTClient->EnumConsoleCommands(M25GatherHelpName, LTNULL);

	M25PrintHelpGroup("Game programs", 1);
	M25PrintHelpGroup("Engine commands", 0);
	g_pLTClient->CPrint("Type the name of a console variable and a value to set it.");
	g_pLTClient->CPrint("\"Cheats\" lists the cheat codes. Esc closes the console.");
}

// M-25: the cheat codes of CheatMgr.cpp (its table keeps them encoded), and
// what each one does.  The descriptions are ours, from the code.
struct M25CheatText
{
	const char*	m_pCode;
	const char*	m_pText;
};

static const M25CheatText c_aM25Cheats[] =
{
	{ "mpdoctordoctor",			"full health" },
	{ "mpsmithy",				"full armor" },
	{ "mpstockpile",			"all the weapons" },
	{ "mpkohler",				"full ammunition" },
	{ "mpbunker",				"infinite ammunition and all the weapons (on/off)" },
	{ "mpcanthurtme [n]",		"god mode (on/off; with n: on, health stays at n or more)" },
	{ "mpschuckit",				"full health, full armor and all the weapons" },
	{ "mpbeamme [x y z]",		"go to the start point, or to the position x y z" },
	{ "mpsixthsense",			"fly through the walls (on/off)" },
	{ "mpmorph <name|next|prev>","change the character of the player" },
	{ "mpicu",					"third-person camera (on/off)" },
	{ "mpgps",					"show the position (on/off)" },
	{ "mpgrs",					"show the rotation (on/off)" },
	{ "mpsizeme",				"show the dimensions (on/off)" },
	{ "mptachometer",			"show the velocity (on/off)" },
	{ "mpdeathtoall",			"kill all the AI of the level" },
	{ "mpxfiles",				"show all the missions in the mission menus (on/off)" },
	{ "mpmillertime",			"the characters speak the \"Miller\" lines (on/off)" },
	{ "mpreloadbutes <file>",	"read an attribute file again: character, model or fx" },
	{ "mpconfig <file|clear>",	"read a config file, or remove all the bindings" },
	{ "mpwpos, mpwmpos, mpbreach","move the weapon, its muzzle, its breach (developer)" },
	{ "mplightscale, mplightadd","change the light of the weapon (developer)" },
	{ "mpvertextint, mpfov",	"change the vertex tint, the field of view (developer)" },
	{ "mpmph, mptriggers",		"do nothing (the retail game turned them off)" },
#ifdef PRIMAL_HUNT
	{ "nextlevel",				"end the level" },
	{ "nelno",					"a message from a developer (on/off)" },
#endif
};

#define M25_NUM_CHEATS	(sizeof(c_aM25Cheats) / sizeof(c_aM25Cheats[0]))

static void M25CheatsFn(int argc, char** argv)
{
	unsigned int i;

	g_pLTClient->CPrint("Cheat codes (single player; type one and press Enter):");
	for (i = 0; i < M25_NUM_CHEATS; i++)
		g_pLTClient->CPrint("  %s: %s", c_aM25Cheats[i].m_pCode, c_aM25Cheats[i].m_pText);
}

// GetPos: the position of the player, in the form that SetPos takes
static void M25GetPosFn(int argc, char** argv)
{
	HOBJECT  hPlayer = g_pGameClientShell->GetPlayerMovement()->GetObject();
	LTVector vPos;

	if (!hPlayer || !g_pGameClientShell->IsPlayerInWorld())
	{
		g_pLTClient->CPrint("GetPos: no player");
		return;
	}

	g_pLTClient->GetObjectPos(hPlayer, &vPos);
	g_pLTClient->CPrint("SetPos %.1f %.1f %.1f", vPos.x, vPos.y, vPos.z);
}

// SetPos <x> <y> <z>: the player goes there (single player).  The server
// moves its player object and then the client's (MID_M25_SETPOS), so the
// player does not cross the space between the two positions.
static void M25SetPosFn(int argc, char** argv)
{
	HOBJECT hPlayer = g_pGameClientShell->GetPlayerMovement()->GetObject();

	if (argc < 3 || !hPlayer || !g_pGameClientShell->IsPlayerInWorld() ||
		g_pGameClientShell->IsMultiplayerGame())
	{
		g_pLTClient->CPrint("SetPos <x> <y> <z>  (single player; GetPos gives the position)");
		return;
	}

	LTVector vPos((LTFLOAT)atof(argv[0]), (LTFLOAT)atof(argv[1]), (LTFLOAT)atof(argv[2]));
	HMESSAGEWRITE hMsg = g_pLTClient->StartMessage(MID_M25_SETPOS);
	g_pLTClient->WriteToMessageVector(hMsg, &vPos);
	g_pLTClient->EndMessage(hMsg);
	g_pLTClient->CPrint("Player to %.1f %.1f %.1f", vPos.x, vPos.y, vPos.z);
}


// ----------------------------------------------------------------------- //
//  Init / Term
// ----------------------------------------------------------------------- //

void M25Console_Init()
{
	char szCmd[64];

	if (s_bM25Init)
		return;

	if (!s_bM25CSInit)
	{
		InitializeCriticalSection(&s_csM25Lines);
		s_bM25CSInit = LTTRUE;
	}

	s_vtM25Backdrop.Init(g_pLTClient, M25_VAR_BACKDROP, LTNULL, 0.0f);

	// The action, before the profile binds its key (CUserProfile::
	// ApplyBindings).  Primal Hunt adds each action of g_CommandArray
	// itself; AvP2's autoexec.cfg has the others.
	sprintf(szCmd, "AddAction ToggleConsole %d", COMMAND_ID_TOGGLE_CONSOLE);
	g_pLTClient->RunConsoleString(szCmd);

	g_pLTClient->RegisterConsoleProgram("Help", M25HelpFn);
	g_pLTClient->RegisterConsoleProgram("Cheats", M25CheatsFn);
	g_pLTClient->RegisterConsoleProgram("GetPos", M25GetPosFn);
	g_pLTClient->RegisterConsoleProgram("SetPos", M25SetPosFn);

	g_pLTClient->SetConsoleOutputFn(M25OnConsoleLine, LTNULL);

	s_bM25Init = LTTRUE;

	g_pLTClient->CPrint("Console: \"Help\" lists the commands.");
}

static void M25DeleteSurface(HSURFACE& hSurf)
{
	if (hSurf)
	{
		g_pLTClient->DeleteSurface(hSurf);
		hSurf = LTNULL;
	}
}

void M25Console_Term()
{
	if (!s_bM25Init)
		return;

	// The engine stays after this DLL: it must not call it any more
	g_pLTClient->SetConsoleOutputFn(LTNULL, LTNULL);

	g_pLTClient->UnregisterConsoleProgram("Help");
	g_pLTClient->UnregisterConsoleProgram("Cheats");
	g_pLTClient->UnregisterConsoleProgram("GetPos");
	g_pLTClient->UnregisterConsoleProgram("SetPos");

	M25DeleteSurface(s_hM25Copy);
	M25DeleteSurface(s_hM25Backdrop);
	M25DeleteSurface(s_hM25Black);
	M25DeleteSurface(s_hM25Edge);

	s_bM25Visible = LTFALSE;
	s_bM25Init = LTFALSE;
}


// ----------------------------------------------------------------------- //
//  Open and close
// ----------------------------------------------------------------------- //

static LTBOOL M25CanOpen()
{
	GameState eState;

	if (!s_bM25Init || !g_pGameClientShell || !g_pInterfaceMgr || !g_pInterfaceResMgr)
		return LTFALSE;

	// Single player only: a multiplayer game cannot pause
	if (g_pGameClientShell->IsMultiplayerGame())
		return LTFALSE;

	if (g_pInterfaceMgr->GetMessageMgr()->IsEditing() || g_pInterfaceMgr->M25_IsMessageBoxVisible())
		return LTFALSE;

	eState = g_pInterfaceMgr->GetGameState();
	if (eState == GS_FOLDER)
	{
		// Not in Customize Controls (a key can wait for its control
		// there), and not while a control of the folder has the input
		CBaseFolder* pFolder = g_pInterfaceMgr->GetFolderMgr()->GetCurrentFolder();

		if (g_pInterfaceMgr->GetFolderMgr()->GetCurrentFolderID() == FOLDER_ID_CUST_CONTROLS)
			return LTFALSE;
		if (pFolder && pFolder->GetCapture())
			return LTFALSE;
	}
	else if (eState != GS_PLAYING)
	{
		return LTFALSE;		// loading, a dialogue, the pause screen, a movie ...
	}

	return g_pInterfaceResMgr->GetHelpFont() != LTNULL;
}

static void M25ClearEdit()
{
	s_szM25Edit[0] = '\0';
	s_nM25EditLen = 0;
	s_nM25RecallPos = -1;
}

static void M25Open()
{
	if (s_bM25Visible || !M25CanOpen())
		return;

	M25ClearEdit();
	s_nM25Scroll = 0;
	s_bM25Visible = LTTRUE;

	// In play, the console holds the game: it pauses it (the server, the
	// sounds and the input) and shows a copy of the screen.  A menu has
	// already paused the game.
	s_bM25Hold = (g_pInterfaceMgr->GetGameState() == GS_PLAYING);
	s_bM25Paused = LTFALSE;
	if (s_bM25Hold && !g_pGameClientShell->IsGamePaused())
	{
		g_pGameClientShell->PauseGame(LTTRUE, LTTRUE);
		s_bM25Paused = LTTRUE;
	}
}

static void M25Close()
{
	if (!s_bM25Visible)
		return;

	s_bM25Visible = LTFALSE;
	s_bM25Hold = LTFALSE;
	M25ClearEdit();
	M25DeleteSurface(s_hM25Copy);

	if (s_bM25Paused)
	{
		s_bM25Paused = LTFALSE;
		if (g_pGameClientShell && g_pGameClientShell->IsGamePaused())
			g_pGameClientShell->PauseGame(LTFALSE);
	}
}

LTBOOL M25Console_IsVisible()
{
	return s_bM25Visible;
}


// ----------------------------------------------------------------------- //
//  Enter: run the line
// ----------------------------------------------------------------------- //

static void M25Remember(const char* pLine)
{
	int nLast = (s_nM25RecallHead - 1 + M25_MAX_RECALL) % M25_MAX_RECALL;

	// Not two times in sequence
	if (s_nM25RecallCount && strcmp(s_aszM25Recall[nLast], pLine) == 0)
		return;

	strncpy(s_aszM25Recall[s_nM25RecallHead], pLine, M25_EDIT_LEN - 1);
	s_aszM25Recall[s_nM25RecallHead][M25_EDIT_LEN - 1] = '\0';
	s_nM25RecallHead = (s_nM25RecallHead + 1) % M25_MAX_RECALL;
	if (s_nM25RecallCount < M25_MAX_RECALL)
		s_nM25RecallCount++;
}

// A cheat code: CheatMgr runs it (and plays the cheat sound) and says
// LTTRUE.  Its word goes to a buffer of 128 characters without a test of
// the length (CheatMgr::HandleCheat): a longer word is not a cheat.
static LTBOOL M25RunCheat(const char* pLine)
{
	char     szLine[M25_EDIT_LEN];
	ConParse parse;

	if (!g_pCheatMgr)
		return LTFALSE;

	strncpy(szLine, pLine, sizeof(szLine) - 1);
	szLine[sizeof(szLine) - 1] = '\0';
	parse.Init(szLine);

	if (g_pLTClient->Common()->Parse(&parse) != LT_OK || parse.m_nArgs < 1)
		return LTFALSE;
	if (strlen(parse.m_Args[0]) >= 100)
		return LTFALSE;

	return g_pCheatMgr->HandleCheat(parse.m_nArgs, parse.m_Args);
}

// The first word of the line names a command (a program or a command of
// the engine) of the client.  The server does not get these lines: it
// does not know them (a variable can be of the client and of the server).
static const char* s_pM25FindName;
static LTBOOL      s_bM25Found;

static void M25FindCommand(void* pUser, const char* pName, uint32 nFlags)
{
	if (!s_bM25Found && pName && !stricmp(pName, s_pM25FindName))
		s_bM25Found = LTTRUE;
}

static LTBOOL M25IsClientCommand(const char* pLine)
{
	char szName[M25_EDIT_LEN];
	int  i;

	for (i = 0; pLine[i] && pLine[i] != ' ' && pLine[i] != ';' && i < M25_EDIT_LEN - 1; i++)
		szName[i] = pLine[i];
	szName[i] = '\0';

	s_pM25FindName = szName;
	s_bM25Found = LTFALSE;
	g_pLTClient->EnumConsoleCommands(M25FindCommand, LTNULL);

	return s_bM25Found;
}

static void M25Submit()
{
	char  szLine[M25_EDIT_LEN];
	char* pLine;

	strcpy(szLine, s_szM25Edit);
	M25ClearEdit();
	s_nM25Scroll = 0;

	for (pLine = szLine; *pLine == ' '; pLine++)
		;
	if (!*pLine)
		return;

	M25Remember(pLine);

	g_pLTClient->CPrint("> %s", pLine);

	if (M25RunCheat(pLine))
		return;

	// On the client: its variables and programs
	g_pLTClient->RunConsoleString(pLine);

	// And on the server, for its variables: the server runs it only from
	// the local client (m25_server.h).  The console opens in single
	// player only.
	if (g_pLTClient->IsConnected() && g_pGameClientShell->IsPlayerInWorld() &&
		!g_pGameClientShell->IsMultiplayerGame() && !M25IsClientCommand(pLine))
	{
		HSTRING hCmd = g_pLTClient->CreateString(pLine);
		if (hCmd)
		{
			HMESSAGEWRITE hMsg = g_pLTClient->StartMessage(MID_M25_CONSOLE_COMMAND);
			g_pLTClient->WriteToMessageHString(hMsg, hCmd);
			g_pLTClient->EndMessage(hMsg);
			g_pLTClient->FreeString(hCmd);
		}
	}
}

static void M25Recall(int nDir)
{
	int nPos;

	if (!s_nM25RecallCount)
		return;

	// s_nM25RecallPos counts back from the newest: 0 is the last command
	nPos = s_nM25RecallPos + nDir;
	if (nPos >= s_nM25RecallCount)
		nPos = s_nM25RecallCount - 1;

	if (nPos < 0)
	{
		M25ClearEdit();
		return;
	}

	s_nM25RecallPos = nPos;
	strcpy(s_szM25Edit, s_aszM25Recall[(s_nM25RecallHead - 1 - nPos + 2 * M25_MAX_RECALL) % M25_MAX_RECALL]);
	s_nM25EditLen = strlen(s_szM25Edit);
}

static void M25Scroll(int nLines)
{
	int nMax = s_nM25Lines - s_nM25Visible;
	if (nMax < 0)
		nMax = 0;

	s_nM25Scroll += nLines;
	if (s_nM25Scroll > nMax)
		s_nM25Scroll = nMax;
	if (s_nM25Scroll < 0)
		s_nM25Scroll = 0;
}


// ----------------------------------------------------------------------- //
//  Input
// ----------------------------------------------------------------------- //

// The virtual key of a DirectInput key code (DIK_*), for the current
// keyboard layout; -1 if none.
static int M25DikToVK(int nDik)
{
	if (nDik <= 0)
		return -1;

	if (nDik < 0x80)
	{
		UINT nVK = MapVirtualKey(nDik, 1);
		return nVK ? (int)nVK : -1;
	}

	// The extended keys (an E0 prefix)
	switch (nDik)
	{
		case 0x9C:	return VK_RETURN;		// DIK_NUMPADENTER
		case 0x9D:	return VK_CONTROL;		// DIK_RCONTROL
		case 0xB5:	return VK_DIVIDE;		// DIK_DIVIDE
		case 0xB8:	return VK_MENU;			// DIK_RMENU
		case 0xC5:	return VK_PAUSE;		// DIK_PAUSE
		case 0xC7:	return VK_HOME;			// DIK_HOME
		case 0xC8:	return VK_UP;			// DIK_UP
		case 0xC9:	return VK_PRIOR;		// DIK_PRIOR
		case 0xCB:	return VK_LEFT;			// DIK_LEFT
		case 0xCD:	return VK_RIGHT;		// DIK_RIGHT
		case 0xCF:	return VK_END;			// DIK_END
		case 0xD0:	return VK_DOWN;			// DIK_DOWN
		case 0xD1:	return VK_NEXT;			// DIK_NEXT
		case 0xD2:	return VK_INSERT;		// DIK_INSERT
		case 0xD3:	return VK_DELETE;		// DIK_DELETE
		default:	break;
	}

	return -1;
}

// The virtual key of the keyboard key that ToggleConsole is bound to (its
// "##<DIK>" name), or -1
static int M25ConsoleKey()
{
	DeviceBinding* pBindings;
	DeviceBinding* pBinding;
	GameAction*    pAction;
	int            nVK = -1;

	pBindings = g_pLTClient->GetDeviceBindings(DEVICETYPE_KEYBOARD);
	for (pBinding = pBindings; pBinding && nVK < 0; pBinding = pBinding->pNext)
	{
		for (pAction = pBinding->pActionHead; pAction; pAction = pAction->pNext)
		{
			if (pAction->nActionCode == COMMAND_ID_TOGGLE_CONSOLE &&
				pBinding->strRealName[0] == '#' && pBinding->strRealName[1] == '#')
			{
				nVK = M25DikToVK(atoi(&pBinding->strRealName[2]));
				break;
			}
		}
	}

	if (pBindings)
		g_pLTClient->FreeDeviceBindings(pBindings);

	return nVK;
}

static LTBOOL M25IsShift(int nKey)
{
	return nKey == VK_SHIFT || nKey == VK_LSHIFT || nKey == VK_RSHIFT;
}

LTBOOL M25Console_OnKeyDown(int nKey, int nRep)
{
	int nConsoleKey;

	if (!s_bM25Init)
		return LTFALSE;

	if (M25IsShift(nKey))
		s_bM25Shift = LTTRUE;

	nConsoleKey = M25ConsoleKey();

	if (!s_bM25Visible)
	{
		// The console key, or Shift+Esc (M-25: a keyboard can have a
		// different key in the place of the console key)
		LTBOOL bShift = s_bM25Shift || (GetKeyState(VK_SHIFT) & 0x8000);

		if (((nKey == nConsoleKey && nKey > 0) || (nKey == VK_ESCAPE && bShift)) && M25CanOpen())
		{
			M25Open();
			return LTTRUE;
		}
		return LTFALSE;
	}

	// Open: each key is the console's
	if (nKey == VK_ESCAPE || (nKey == nConsoleKey && nKey > 0))
	{
		M25Close();
		return LTTRUE;
	}

	switch (nKey)
	{
		case VK_RETURN:	M25Submit();						break;
		case VK_BACK:
			if (s_nM25EditLen > 0)
				s_szM25Edit[--s_nM25EditLen] = '\0';
			break;
		case VK_UP:		M25Recall(1);						break;
		case VK_DOWN:	M25Recall(-1);						break;
		case VK_PRIOR:	M25Scroll(s_nM25Visible - 1);		break;
		case VK_NEXT:	M25Scroll(-(s_nM25Visible - 1));	break;
		case VK_HOME:	M25Scroll(s_nM25Lines);				break;
		case VK_END:	M25Scroll(-s_nM25Lines);			break;
		default:											break;
	}

	return LTTRUE;
}

LTBOOL M25Console_OnKeyUp(int nKey)
{
	if (M25IsShift(nKey))
		s_bM25Shift = LTFALSE;

	return s_bM25Visible;
}

LTBOOL M25Console_OnChar(char c)
{
	int nConsoleKey;

	if (!s_bM25Visible)
		return LTFALSE;

	// The console key types its character too: not into the line
	nConsoleKey = M25ConsoleKey();
	if (nConsoleKey > 0)
	{
		UINT nChar = MapVirtualKey(nConsoleKey, 2) & 0x7fff;
		if (nChar && (char)nChar == c)
			return LTTRUE;
		if (nConsoleKey == 0xC0 && (c == '`' || c == '~'))	// VK_OEM_3
			return LTTRUE;
	}

	if ((unsigned char)c >= ' ' && (unsigned char)c < 127 && s_nM25EditLen < M25_EDIT_LEN - 1)
	{
		s_szM25Edit[s_nM25EditLen++] = c;
		s_szM25Edit[s_nM25EditLen] = '\0';
		s_nM25RecallPos = -1;
	}

	return LTTRUE;
}

LTBOOL M25Console_OnCommandOn(int nCommand)
{
	if (s_bM25Visible)
		return LTTRUE;		// no game command while it is open

	// A key without a virtual key that M25ConsoleKey knows still gives the
	// command: open the console from the command then
	if (nCommand == COMMAND_ID_TOGGLE_CONSOLE)
	{
		if (M25ConsoleKey() < 0)
			M25Open();
		return LTTRUE;
	}

	return LTFALSE;
}


// ----------------------------------------------------------------------- //
//  The frame
// ----------------------------------------------------------------------- //

LTBOOL M25Console_Update()
{
	if (!s_bM25Visible)
		return LTFALSE;

	// A load, a new state, or multiplayer: the console closes
	if (g_pGameClientShell->IsMultiplayerGame() ||
		g_pInterfaceMgr->GetGameState() == GS_LOADINGLEVEL ||
		(s_bM25Hold && g_pInterfaceMgr->GetGameState() != GS_PLAYING))
	{
		M25Close();
		return LTFALSE;
	}

	if (!s_bM25Hold || !s_hM25Copy)
		return LTFALSE;		// (the first frame in play: drawn, then copied)

	// As the pause screen of the game (CInterfaceMgr::UpdatePausedState)
	g_pLTClient->DrawSurfaceToSurface(g_pLTClient->GetScreenSurface(), s_hM25Copy, LTNULL, 0, 0);

	return LTTRUE;
}

static HSURFACE M25SolidSurface(HSURFACE& hSurf, HLTCOLOR hColor)
{
	if (!hSurf)
	{
		hSurf = g_pLTClient->CreateSurface(8, 8);
		if (hSurf)
			g_pLTClient->FillRect(hSurf, LTNULL, hColor);
	}
	return hSurf;
}

// console.pcx is a loose file of the game directory, not in a .rez file:
// the file manager of the engine does not find it (CreateSurfaceFromBitmap
// fails), and the engine's console opens it with fopen.  This reads it the
// same way: a PCX of 8 bits a pixel, RLE, with its palette at the end.
static HSURFACE M25LoadPcx(const char* pName)
{
	FILE*          pFile;
	long           nSize;
	unsigned char* pData;
	HSURFACE       hSurf = LTNULL;

	pFile = fopen(pName, "rb");
	if (!pFile)
		return LTNULL;

	fseek(pFile, 0, SEEK_END);
	nSize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	pData = (nSize > 128 + 769) ? (unsigned char*)malloc(nSize) : LTNULL;
	if (pData && fread(pData, 1, nSize, pFile) == (size_t)nSize &&
		pData[0] == 10 && pData[2] == 1 && pData[3] == 8 && pData[65] == 1 &&
		pData[nSize - 769] == 12)
	{
		int nWidth  = (pData[8] | (pData[9] << 8)) - (pData[4] | (pData[5] << 8)) + 1;
		int nHeight = (pData[10] | (pData[11] << 8)) - (pData[6] | (pData[7] << 8)) + 1;
		int nLine   = pData[66] | (pData[67] << 8);
		const unsigned char* pPal = &pData[nSize - 768];
		long nPos = 128;

		if (nWidth > 0 && nHeight > 0 && nWidth <= nLine && nWidth <= 2048 && nHeight <= 2048)
			hSurf = g_pLTClient->CreateSurface(nWidth, nHeight);

		for (int y = 0; hSurf && y < nHeight; y++)
		{
			int x = 0;

			while (x < nLine && nPos < nSize - 769)
			{
				int nByte  = pData[nPos++];
				int nCount = 1;

				if ((nByte & 0xC0) == 0xC0)
				{
					nCount = nByte & 0x3F;
					nByte = (nPos < nSize - 769) ? pData[nPos++] : 0;
				}

				for (; nCount > 0 && x < nLine; nCount--, x++)
				{
					if (x < nWidth)
					{
						const unsigned char* pRGB = &pPal[nByte * 3];
						g_pLTClient->SetPixel(hSurf, x, y, SETRGB(pRGB[0], pRGB[1], pRGB[2]));
					}
				}
			}
		}
	}

	if (pData)
		free(pData);
	fclose(pFile);

	return hSurf;
}

static HSURFACE M25Backdrop()
{
	// 1: black; 0: console.pcx (black if it does not load)
	if ((int)s_vtM25Backdrop.GetFloat() != 1 && !s_bM25NoBackdrop)
	{
		if (!s_hM25Backdrop)
		{
			s_hM25Backdrop = M25LoadPcx(M25_BACKDROP_FILE);
			if (!s_hM25Backdrop)
			{
				s_bM25NoBackdrop = LTTRUE;
				g_pLTClient->CPrint("Console: %s did not load; the backdrop is black", M25_BACKDROP_FILE);
			}
		}
		if (s_hM25Backdrop)
			return s_hM25Backdrop;
	}

	return M25SolidSurface(s_hM25Black, SETRGB(0, 0, 0));
}

static void M25DrawText(CLTGUIFont* pFont, HSURFACE hScreen, const char* pText,
						int x, int y, int nJustify, HLTCOLOR hColor)
{
	LITHFONTDRAWDATA dd;

	dd.dwFlags = LTF_DRAW_SOLID;
	dd.byJustify = (uint8)nJustify;
	dd.hColor = hColor;
	dd.bForceFormat = LTTRUE;
	pFont->Draw((char*)pText, hScreen, &dd, x, y);
}

void M25Console_Draw()
{
	HSURFACE    hScreen;
	HSURFACE    hBack;
	HSURFACE    hEdge;
	CLTGUIFont* pFont;
	uint32      nWidth, nHeight;
	int         nConHeight, nLineHeight, nY, i;
	char        szEdit[M25_EDIT_LEN + 8];
	LTRect      rc;

	if (!s_bM25Visible)
		return;

	pFont = g_pInterfaceResMgr ? g_pInterfaceResMgr->GetHelpFont() : LTNULL;
	if (!pFont)
		return;

	hScreen = g_pLTClient->GetScreenSurface();

	// The first frame in play: keep a copy of it, for the frames after
	if (s_bM25Hold && !s_hM25Copy)
	{
		g_pLTClient->GetSurfaceDims(hScreen, &nWidth, &nHeight);
		s_hM25Copy = g_pLTClient->CreateSurface(nWidth, nHeight);
		if (s_hM25Copy)
			g_pLTClient->DrawSurfaceToSurface(s_hM25Copy, hScreen, LTNULL, 0, 0);
	}

	CM15Space space(M15_SPACE_HUD);		// M-16: as large as the HUD's text

	g_pLTClient->GetSurfaceDims(hScreen, &nWidth, &nHeight);

	nConHeight = nHeight / 2;
	nLineHeight = pFont->GetHeight() + 1;
	if (nLineHeight < 4)
		nLineHeight = 4;

	// The backdrop and the edge before the optimized 2D mode: in that
	// mode, black is transparent (as the pause screen draws its copy)
	rc.left = 0;
	rc.top = 0;
	rc.right = nWidth;
	rc.bottom = nConHeight;
	hBack = M25Backdrop();
	if (hBack)
		g_pLTClient->ScaleSurfaceToSurface(hScreen, hBack, &rc, LTNULL);

	// A line along the bottom edge
	hEdge = M25SolidSurface(s_hM25Edge, M25_COLOR_EDGE);
	if (hEdge)
	{
		rc.top = nConHeight;
		rc.bottom = nConHeight + 2;
		g_pLTClient->ScaleSurfaceToSurface(hScreen, hEdge, &rc, LTNULL);
	}

	g_pLTClient->Start3D();
	g_pLTClient->StartOptimized2D();

	// The edit line at the bottom, with a cursor that blinks
	nY = nConHeight - nLineHeight - 3;
	sprintf(szEdit, "> %s%s", s_szM25Edit, ((GetTickCount() / 400) & 1) ? "_" : "");
	M25DrawText(pFont, hScreen, szEdit, 6, nY, LTF_JUSTIFY_LEFT, M25_COLOR_EDIT);

	// The lines above it, the newest at the bottom
	s_nM25Visible = (nY - 4) / nLineHeight;
	if (s_nM25Visible < 1)
		s_nM25Visible = 1;

	EnterCriticalSection(&s_csM25Lines);

	if (s_nM25Scroll > s_nM25Lines - 1)
		s_nM25Scroll = 0;

	for (i = 0; i < s_nM25Visible; i++)
	{
		int nBack = s_nM25Scroll + i;		// 0: the newest line
		M25ConLine* pLine;

		if (nBack >= s_nM25Lines)
			break;

		pLine = &s_aM25Lines[(s_nM25Head - 1 - nBack + 2 * M25_MAX_LINES) % M25_MAX_LINES];
		nY -= nLineHeight;
		M25DrawText(pFont, hScreen, pLine->m_szText, 6, nY, LTF_JUSTIFY_LEFT, pLine->m_nColor);
	}

	LeaveCriticalSection(&s_csM25Lines);

	// Scrolled back: a note where the newest line is
	if (s_nM25Scroll > 0)
	{
		char szMore[48];
		sprintf(szMore, "%d more lines (Page Down)", s_nM25Scroll);
		M25DrawText(pFont, hScreen, szMore, nWidth - 6, nConHeight - nLineHeight - 3,
					LTF_JUSTIFY_RIGHT, M25_COLOR_NOTE);
	}

	g_pLTClient->EndOptimized2D();
	g_pLTClient->End3D();
}

#endif // M25_CONSOLE_IMPLEMENTATION

#endif // __M25_CONSOLE_H__
