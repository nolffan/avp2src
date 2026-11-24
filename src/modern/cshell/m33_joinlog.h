// ----------------------------------------------------------------------- //
//
// MODULE  : m33_joinlog.h
//
// PURPOSE : M-33 (docs/MODERN.md): each join of a multiplayer server and
//           its result go to the console, as nolfsrc does (its M-P6-06):
//
//             Joining <address>:<port>
//             Joined <address>:<port>
//             Could not join <address>:<port> (error <n>)
//             Disconnected from <address>:<port> (reason <n>, event <n>)
//
//           The last line comes from the disconnect event. Its reason is
//           the LT_DISCON_ code that the engine gave the shell
//           (SetDisconnectCode; ltcodes.h), -1 for none; the event value
//           is the event's own parameter.
//
//           Included by GameClientShell.cpp and InterfaceMgr.cpp.
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M33_JOINLOG_H__
#define __M33_JOINLOG_H__

#include <string.h>

// The server of the last join (also for M-34's rejoin).
struct M33Server
{
	char	szIP[64];
	uint16	nPort;
	char	szPassword[64];
	LTBOOL	bHave;
};

// Defined in GameClientShell.cpp (one copy for the two files that use it).
extern M33Server g_M33Server;

inline M33Server& M33_Server()
{
	return g_M33Server;
}

// From JoinMultiplayerGameAtIP, before StartGame.
inline void M33_OnJoin(const char* szIP, uint16 nPort, const char* szPassword)
{
	M33Server& s = M33_Server();
	if (szIP != s.szIP)		// (M-34's rejoin gives this copy back)
	{
		strncpy(s.szIP, szIP ? szIP : "", sizeof(s.szIP) - 1);
		s.szIP[sizeof(s.szIP) - 1] = '\0';
	}
	if (szPassword != s.szPassword)
	{
		strncpy(s.szPassword, szPassword ? szPassword : "", sizeof(s.szPassword) - 1);
		s.szPassword[sizeof(s.szPassword) - 1] = '\0';
	}
	s.nPort = nPort;
	s.bHave = LTTRUE;

	g_pLTClient->CPrint("Joining %s:%d", s.szIP, (int)s.nPort);
}

// From JoinMultiplayerGameAtIP: the result of StartGame.
inline void M33_OnJoinResult(LTRESULT dr)
{
	M33Server& s = M33_Server();
	if (dr == LT_OK)
		g_pLTClient->CPrint("Joined %s:%d", s.szIP, (int)s.nPort);
	else
		g_pLTClient->CPrint("Could not join %s:%d (error %lu)", s.szIP, (int)s.nPort, (unsigned long)dr);
}

// From the disconnect event of a multiplayer game.
inline void M33_OnDisconnect(uint32 nReason, uint32 nEventParam)
{
	M33Server& s = M33_Server();
	if (!s.bHave) return;
	g_pLTClient->CPrint("Disconnected from %s:%d (reason %ld, event %lu)", s.szIP, (int)s.nPort, (long)nReason, (unsigned long)nEventParam);
}

#endif
