// ----------------------------------------------------------------------- //
//
// MODULE  : m25_server.h
//
// PURPOSE : The server half of the modern build's console (docs/MODERN.md,
//           M-25): the two client-to-server messages of the console.
//
//             MID_M25_CONSOLE_COMMAND  a line that the player typed in the
//                                      console, for the server's console
//                                      (server variables and programs)
//             MID_M25_SETPOS           SetPos: a position for the player
//
//           The server accepts each message ONLY from the local client
//           (CIF_LOCAL: the player in single player, or the player who
//           hosts a listen server). It ignores the message from each other
//           client, and writes that to its console. (NOLF Modernizer's
//           console message, which these replace, ran the string from
//           each client: a remote player could set the variables of the
//           server.) SetPos is also for single player only.
//
//           GameServerShell.cpp includes this, one time, inside its
//           #ifdef AVP2SRC_MODERN block, and calls the two functions from
//           CGameServerShell::OnMessage.
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M25_SERVER_H__
#define __M25_SERVER_H__

#define M25_SERVER_CMD_LEN		256

// The sender is the player at this computer (single player, or the host
// of a listen server). A dedicated server has no local client.
static LTBOOL M25Server_IsLocal(HCLIENT hSender)
{
	return hSender && (g_pLTServer->GetClientInfoFlags(hSender) & CIF_LOCAL);
}

// MID_M25_CONSOLE_COMMAND: an HSTRING, run as a console string of the
// server, if the local client sent it.
static void M25Server_OnConsoleCommand(HCLIENT hSender, HMESSAGEREAD hMessage)
{
	char    szCmd[M25_SERVER_CMD_LEN];
	HSTRING hCmd;
	char*   pCmd;

	hCmd = g_pLTServer->ReadFromMessageHString(hMessage);
	if (!hCmd)
		return;

	pCmd = g_pLTServer->GetStringData(hCmd);
	strncpy(szCmd, pCmd ? pCmd : "", sizeof(szCmd) - 1);
	szCmd[sizeof(szCmd) - 1] = '\0';
	g_pLTServer->FreeString(hCmd);

	if (!szCmd[0])
		return;

	if (!M25Server_IsLocal(hSender))
	{
		g_pLTServer->CPrint("Console: a command from a remote client is ignored: %s", szCmd);
		return;
	}

	g_pLTServer->RunGameConString(szCmd);
}

// MID_M25_SETPOS: a vector. In single player, from the local client, the
// server moves the player object to it and then sends the client its new
// position (CPlayerObj::Teleport, as the teleport cheat does), so the
// player does not cross the space between the two positions.
static void M25Server_OnSetPos(HCLIENT hSender, HMESSAGEREAD hMessage)
{
	LTVector    vPos;
	CPlayerObj* pPlayer;
	GameType    gameType;

	g_pLTServer->ReadFromMessageVector(hMessage, &vPos);

	gameType = g_pGameServerShell->GetGameType();
	if (gameType.IsMultiplayer() || !M25Server_IsLocal(hSender))
	{
		g_pLTServer->CPrint("Console: SetPos from a remote client, or in multiplayer, is ignored");
		return;
	}

	pPlayer = (CPlayerObj*)g_pLTServer->GetClientUserData(hSender);
	if (!pPlayer || !pPlayer->m_hObject)
		return;

	pPlayer->Teleport(vPos);
}

#endif // __M25_SERVER_H__
