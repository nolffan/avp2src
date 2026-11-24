// ----------------------------------------------------------------------- //
//
// MODULE  : m32_joincheck.h
//
// PURPOSE : M-32 (docs/MODERN.md): the server enforces its own refusal of
//           a client. CGameServerShell::VerifyClient examines each client
//           (the password of a locked server, the player limit, and in
//           Primal Hunt the class lock), but the engine only sends the
//           result to the client with the world, and the CLIENT then
//           refuses to load the world. A changed client that ignores the
//           result goes into the world. Now the server examines the client
//           again when it enters the world, and a refused client is kicked
//           at the next server update (not in OnClientEnterWorld itself:
//           the engine is in the middle of that client's entry).
//
//           GameServerShell.cpp includes this, one time, inside an
//           #ifdef AVP2SRC_MODERN block.
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M32_JOINCHECK_H__
#define __M32_JOINCHECK_H__

#define M32_MAX_KICKS	32

static HCLIENT s_aM32Kicks[M32_MAX_KICKS];		// to kick at the next update
static HCLIENT s_aM32Refused[M32_MAX_KICKS];	// VerifyClient refused them

// From VerifyClient: its result for hClient.
inline void M32_SetRefused(HCLIENT hClient, LTBOOL bRefused)
{
	int i;
	for (i = 0; i < M32_MAX_KICKS; i++)
		if (s_aM32Refused[i] == hClient) s_aM32Refused[i] = LTNULL;
	if (!bRefused) return;
	for (i = 0; i < M32_MAX_KICKS; i++)
	{
		if (!s_aM32Refused[i])
		{
			s_aM32Refused[i] = hClient;
			return;
		}
	}
}

inline LTBOOL M32_IsRefused(HCLIENT hClient)
{
	for (int i = 0; i < M32_MAX_KICKS; i++)
		if (s_aM32Refused[i] == hClient) return LTTRUE;
	return LTFALSE;
}

// Kick hClient at the next server update.
inline void M32_KickLater(HCLIENT hClient)
{
	int i;
	for (i = 0; i < M32_MAX_KICKS; i++)
		if (s_aM32Kicks[i] == hClient) return;
	for (i = 0; i < M32_MAX_KICKS; i++)
	{
		if (!s_aM32Kicks[i])
		{
			s_aM32Kicks[i] = hClient;
			return;
		}
	}
	// (no free entry: kick now)
	g_pLTServer->KickClient(hClient);
}

// The client has gone: forget it (its handle can come back for another).
inline void M32_Forget(HCLIENT hClient)
{
	for (int i = 0; i < M32_MAX_KICKS; i++)
	{
		if (s_aM32Kicks[i] == hClient) s_aM32Kicks[i] = LTNULL;
		if (s_aM32Refused[i] == hClient) s_aM32Refused[i] = LTNULL;
	}
}

// From CGameServerShell::Update: kick the clients that wait for it.
inline void M32_KickPending()
{
	for (int i = 0; i < M32_MAX_KICKS; i++)
	{
		HCLIENT hClient = s_aM32Kicks[i];
		if (!hClient) continue;
		s_aM32Kicks[i] = LTNULL;
		g_pLTServer->KickClient(hClient);
	}
}

#endif
