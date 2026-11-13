// ----------------------------------------------------------------------- //
//
// MODULE  : m10_timing.h
//
// PURPOSE : The modern build's frame timing on the game side (docs/
//           MODERN.md, M-10..M-13): the frame-rate limit's console
//           variable and its choices, which the frame limiter
//           (m10_framelimit.h, GameClientShell.cpp) and the Display
//           options (FolderDisplay.cpp) share.
//
//           FramerateLimit: frames a second at most, 0 for no limit; one
//           of 60, 120, 144, 240 (the default) or 0 from the menu, any
//           other from the console or autoexec.cfg.  Saved in
//           autoexec.cfg (set with "+", as the game's settings are).
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M10_TIMING_H__
#define __M10_TIMING_H__

#define M10_FRAMERATE_VAR		"FramerateLimit"
#define M10_FRAMERATE_DEFAULT	240

// The menu's choices, in its order: 0 is no limit, and is last.
static const int g_nM10FramerateChoices[] = { 60, 120, 144, 240, 0 };
#define M10_NUM_FRAMERATE_CHOICES	(sizeof(g_nM10FramerateChoices) / sizeof(g_nM10FramerateChoices[0]))

// The limit now (frames a second, 0 none): the variable, made with the
// default the first time it is asked for.
inline int M10_GetFramerateLimit()
{
	HCONSOLEVAR hVar = g_pLTClient->GetConsoleVar(M10_FRAMERATE_VAR);
	if (!hVar)
	{
		char szCmd[64];
		sprintf(szCmd, "+%s %d", M10_FRAMERATE_VAR, M10_FRAMERATE_DEFAULT);
		g_pLTClient->RunConsoleString(szCmd);
		hVar = g_pLTClient->GetConsoleVar(M10_FRAMERATE_VAR);
		if (!hVar)
			return M10_FRAMERATE_DEFAULT;
	}

	int nFPS = (int)g_pLTClient->GetVarValueFloat(hVar);
	return nFPS > 0 ? nFPS : 0;
}

inline void M10_SetFramerateLimit(int nFPS)
{
	char szCmd[64];
	sprintf(szCmd, "+%s %d", M10_FRAMERATE_VAR, nFPS > 0 ? nFPS : 0);
	g_pLTClient->RunConsoleString(szCmd);
}

// The menu's choice for a limit: its own, or for one not among them the
// next higher (no limit above 240).
inline int M10_FramerateChoice(int nFPS)
{
	int i;

	if (nFPS <= 0)
		return M10_NUM_FRAMERATE_CHOICES - 1;

	for (i = 0; i < (int)M10_NUM_FRAMERATE_CHOICES - 1; i++)
	{
		if (nFPS <= g_nM10FramerateChoices[i])
			return i;
	}

	return M10_NUM_FRAMERATE_CHOICES - 1;
}

#endif // __M10_TIMING_H__
