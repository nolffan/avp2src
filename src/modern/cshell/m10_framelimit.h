// ----------------------------------------------------------------------- //
//
// MODULE  : m10_framelimit.h
//
// PURPOSE : The modern build's frame-rate limit (docs/MODERN.md, M-13):
//           each client frame, once it is drawn, waits until its share of
//           a second (1/FramerateLimit, m10_timing.h) has passed since the
//           previous one was let go.  Single player and multiplayer alike
//           (what made a faster frame rate wrong is put right in the
//           engine: M-10, M-11), and not while a world is loading.
//
//           Included once, by GameClientShell.cpp (its PostUpdate calls
//           M10_EndFrame).
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M10_FRAMELIMIT_H__
#define __M10_FRAMELIMIT_H__

#include "m10_timing.h"

namespace
{
	LARGE_INTEGER	s_liM10Freq;			// counts a second; 0: no counter
	LARGE_INTEGER	s_liM10Next;			// when the next frame may go
	LTBOOL			s_bM10Init = LTFALSE;
	LTBOOL			s_bM10Running = LTFALSE;	// s_liM10Next is set

	void M10_Init()
	{
		typedef UINT (WINAPI *TimePeriodFn)(UINT);

		s_bM10Init = LTTRUE;

		if (!QueryPerformanceFrequency(&s_liM10Freq) || s_liM10Freq.QuadPart <= 0)
			s_liM10Freq.QuadPart = 0;

		// Sleep(1) then sleeps about a millisecond, not a scheduler tick
		// (10-16 ms), so most of the wait can be slept.  The engine has
		// winmm.dll loaded already (its clock); CShell.dll does not link
		// it.  For the life of the process.
		HMODULE hWinMM = GetModuleHandle("winmm.dll");
		if (hWinMM)
		{
			TimePeriodFn pfnBegin = (TimePeriodFn)GetProcAddress(hWinMM, "timeBeginPeriod");
			if (pfnBegin)
				pfnBegin(1);
		}
	}
}

// The end of a client frame: wait for the limit.  bLoading: a world is
// being loaded (the limit waits for the next frame after).
static void M10_EndFrame(LTBOOL bLoading)
{
	LARGE_INTEGER liNow;
	LONGLONG nPeriod;
	int nFPS;

	if (!s_bM10Init)
		M10_Init();

	nFPS = M10_GetFramerateLimit();

	if (!s_liM10Freq.QuadPart || nFPS <= 0 || bLoading)
	{
		// no limit: the next limited frame starts the count afresh
		s_bM10Running = LTFALSE;
		return;
	}

	QueryPerformanceCounter(&liNow);
	nPeriod = s_liM10Freq.QuadPart / nFPS;

	if (!s_bM10Running)
	{
		s_bM10Running = LTTRUE;
		s_liM10Next.QuadPart = liNow.QuadPart + nPeriod;
		return;
	}

	while (liNow.QuadPart < s_liM10Next.QuadPart)
	{
		// sleep while more than 2 ms are left, then spin
		if ((s_liM10Next.QuadPart - liNow.QuadPart) * 1000 > 2 * s_liM10Freq.QuadPart)
			Sleep(1);
		else
			Sleep(0);
		QueryPerformanceCounter(&liNow);
	}

	// Frames keep the cadence (a frame a little late is made up by the
	// next); after a long one (a hitch, a save) the count starts from now
	// rather than letting the frames after it run unlimited to catch up.
	s_liM10Next.QuadPart += nPeriod;
	if (s_liM10Next.QuadPart < liNow.QuadPart)
		s_liM10Next.QuadPart = liNow.QuadPart + nPeriod;
}

#endif // __M10_FRAMELIMIT_H__
