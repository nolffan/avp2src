// ----------------------------------------------------------------------- //
//
// MODULE  : m19_netposlog.h
//
// PURPOSE : A measuring aid for M-19 (docs/MODERN.md), in the modern debug
//           build only (AVP2SRC_MODERN and AVP2SRC_DEBUG): with the console
//           variable NetPosLog 1, every frame of a multiplayer game appends
//           where this client sees each other character to netpos.log --
//           "<seconds> <object> <x> <y> <z>" -- for out/ scripts to count
//           how often another player's motion jumps or stalls.
//
//           Included by GameClientShell.cpp (PostUpdate).
//
// ----------------------------------------------------------------------- //

#ifndef __M19_NETPOSLOG_H__
#define __M19_NETPOSLOG_H__

#ifdef AVP2SRC_MODERN
#ifdef AVP2SRC_DEBUG

#include <stdio.h>
#include <windows.h>

inline void M19_LogRemotePlayers(CSFXMgr* pSFXMgr)
{
	static FILE* s_pFile = NULL;
	static LARGE_INTEGER s_Freq, s_Start;

	HCONSOLEVAR hVar = g_pLTClient->GetConsoleVar("NetPosLog");
	if (!hVar || g_pLTClient->GetVarValueFloat(hVar) != 1.0f) return;
	if (!g_pGameClientShell->IsMultiplayerGame()) return;

	if (!s_pFile)
	{
		s_pFile = fopen("netpos.log", "w");
		if (!s_pFile) return;
		QueryPerformanceFrequency(&s_Freq);
		QueryPerformanceCounter(&s_Start);
	}

	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	double t = (double)(now.QuadPart - s_Start.QuadPart) / (double)s_Freq.QuadPart;

	HOBJECT hPlayer = g_pLTClient->GetClientObject();
	CSpecialFXList* pList = pSFXMgr->GetFXList(SFX_CHARACTER_ID);
	if (!pList) return;

	for (CSpecialFXList::Iterator iter = pList->Begin(); iter != pList->End(); ++iter)
	{
		HOBJECT hChar = iter->pSFX ? iter->pSFX->GetServerObj() : LTNULL;
		if (!hChar || hChar == hPlayer) continue;

		LTVector vPos;
		g_pLTClient->GetObjectPos(hChar, &vPos);
		fprintf(s_pFile, "%.4f %p %.2f %.2f %.2f\n", t, (void*)hChar, vPos.x, vPos.y, vPos.z);
	}
	fflush(s_pFile);
}

#endif
#endif

#endif // __M19_NETPOSLOG_H__
