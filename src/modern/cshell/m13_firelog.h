// ----------------------------------------------------------------------- //
//
// MODULE  : m13_firelog.h
//
// PURPOSE : A measuring aid for M-13 (docs/MODERN.md), in the modern debug
//           build only (AVP2SRC_MODERN and AVP2SRC_DEBUG): when the
//           environment variable AVP2_FIRELOG is set, each shot is written
//           to a file in the game directory, "<seconds> <what> <weapon id>
//           <result>":
//
//             firelog-client.log  each shot that the client's weapon fires
//                                 (CWeaponModel::Fire, 1 when it had ammo)
//                                 and each fire message that it sends
//                                 (SendFireMsg and the vector shot, "send")
//             firelog-server.log  each fire message that the server takes
//                                 (CPlayerObj::HandleMessage_WeaponFire,
//                                 the result of CWeapon::Fire)
//
//           With these, out/ scripts count the shots of a weapon in a time
//           at different frame-rate limits.
//
//           Included by WeaponModel.cpp and PlayerObj.cpp.
//
// ----------------------------------------------------------------------- //

#ifndef __M13_FIRELOG_H__
#define __M13_FIRELOG_H__

#ifdef AVP2SRC_MODERN
#ifdef AVP2SRC_DEBUG

#include <stdio.h>
#include <windows.h>

inline void M13_FireLog(const char* szFile, const char* szWhat, int nWeapon, int nResult)
{
	static FILE* s_pFile = NULL;
	static int s_nState = 0;		// 0 not checked, 1 on, -1 off
	static LARGE_INTEGER s_Freq, s_Start;

	if (s_nState == 0)
	{
		char szVal[8];
		s_nState = GetEnvironmentVariable("AVP2_FIRELOG", szVal, sizeof(szVal)) ? 1 : -1;
		if (s_nState > 0)
		{
			s_pFile = fopen(szFile, "w");
			if (!s_pFile) s_nState = -1;
			QueryPerformanceFrequency(&s_Freq);
			QueryPerformanceCounter(&s_Start);
		}
	}
	if (s_nState < 0) return;

	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	double t = (double)(now.QuadPart - s_Start.QuadPart) / (double)s_Freq.QuadPart;
	fprintf(s_pFile, "%.4f %s %d %d\n", t, szWhat, nWeapon, nResult);
	fflush(s_pFile);
}

#define M13_FIRELOG(file, what, weapon, result)	M13_FireLog(file, what, weapon, result)
#else
#define M13_FIRELOG(file, what, weapon, result)
#endif
#endif

#endif
