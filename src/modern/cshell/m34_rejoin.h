// ----------------------------------------------------------------------- //
//
// MODULE  : m34_rejoin.h
//
// PURPOSE : M-34 (docs/MODERN.md): when the connection to a multiplayer
//           server ends with a time-out (LT_DISCON_TIMEOUT; for example
//           while the server changes the level, or after a break in the
//           network), the client joins the same server again (the server
//           of M-33's log), at most M34_MAX_REJOINS times until it is in a
//           world again. The join cannot start in the disconnect event
//           (the engine is in the middle of the disconnect), thus it
//           starts at the next update of the client shell. As nolfsrc does
//           (its M-P6-05). A rejoin that fails (the server is not there
//           yet) is tried again M34_RETRY_MS later, while rejoins are left.
//
//           Included by GameClientShell.cpp and InterfaceMgr.cpp, after
//           m33_joinlog.h.
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M34_REJOIN_H__
#define __M34_REJOIN_H__

#define M34_MAX_REJOINS		2
#define M34_RETRY_MS		5000

struct M34Rejoin
{
	int		nRejoinsLeft;
	LTBOOL	bPending;		// join again at the next update
	DWORD	dwNotBefore;	// (GetTickCount) not before this time
};

// Defined in GameClientShell.cpp.
extern M34Rejoin g_M34Rejoin;

// From OnEnterWorld: in a world again, so the rejoins are there again.
inline void M34_OnEnterWorld()
{
	g_M34Rejoin.nRejoinsLeft = M34_MAX_REJOINS;
}

// From the disconnect event of a multiplayer game: TRUE when the client
// joins again (the caller then shows no message).
inline LTBOOL M34_OnDisconnect(uint32 nReason, uint32 nEventParam)
{
	if (nReason != LT_DISCON_TIMEOUT && nEventParam != LT_DISCON_TIMEOUT) return LTFALSE;
	// (the engine can send the event two times for one disconnect)
	if (g_M34Rejoin.bPending) return LTTRUE;
	if (!M33_Server().bHave || g_M34Rejoin.nRejoinsLeft <= 0) return LTFALSE;

	g_M34Rejoin.nRejoinsLeft--;
	g_M34Rejoin.bPending = LTTRUE;
	g_M34Rejoin.dwNotBefore = GetTickCount();
	g_pLTClient->CPrint("The connection timed out: joining again (%d of %d)",
		M34_MAX_REJOINS - g_M34Rejoin.nRejoinsLeft, M34_MAX_REJOINS);
	return LTTRUE;
}

// From the client shell's update: TRUE once when the rejoin is to start.
inline LTBOOL M34_TakePending()
{
	if (!g_M34Rejoin.bPending) return LTFALSE;
	if ((long)(GetTickCount() - g_M34Rejoin.dwNotBefore) < 0) return LTFALSE;
	g_M34Rejoin.bPending = LTFALSE;
	return LTTRUE;
}

// After a rejoin that StartGame refused: try again later, while rejoins
// are left.
inline void M34_OnRejoinFailed()
{
	if (g_M34Rejoin.nRejoinsLeft <= 0) return;
	g_M34Rejoin.nRejoinsLeft--;
	g_M34Rejoin.bPending = LTTRUE;
	g_M34Rejoin.dwNotBefore = GetTickCount() + M34_RETRY_MS;
	g_pLTClient->CPrint("The server did not answer: joining again in %d s (%d of %d)",
		M34_RETRY_MS / 1000, M34_MAX_REJOINS - g_M34Rejoin.nRejoinsLeft, M34_MAX_REJOINS);
}

#endif
