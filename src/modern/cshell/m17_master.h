// ----------------------------------------------------------------------- //
//
// MODULE  : m17_master.h
//
// PURPOSE : The modern build's master server (docs/MODERN.md, M-17).
//
//           MasterServer, a client console variable (`+MasterServer host`,
//           or host:port, saved in autoexec.cfg like every `+` variable):
//           the GameSpy master the Internet server list is read from and a
//           hosted Internet game reports to. Empty (the default) is
//           master.openspy.net; `none` lists and reports nothing.
//
//           The browser's half is GameSpyClientMgr.lib's
//           (GameSpyClient_SetMasterServer). A hosted game's server --
//           the object DLL in this process, or Avp2Serv.exe / AVP2XServ.exe
//           started from here, which inherits the environment -- reads it
//           from the environment variable AVP2_MASTERSERVER
//           (MultiplayerMgr::Setup); a dedicated server started by hand
//           takes it from its -MasterServer argument (ServerDlg.cpp).
//
//           Included by the client shell's files inside their
//           AVP2SRC_MODERN blocks.
//
// ----------------------------------------------------------------------- //

#ifndef __M17_MASTER_H__
#define __M17_MASTER_H__

#ifdef AVP2SRC_MODERN

#include <windows.h>
#include <string.h>

#define M17_MASTER_VAR			"MasterServer"
#define M17_MASTER_ENV			"AVP2_MASTERSERVER"

// In GameSpyClientMgr.lib (GameSpyClientMgr.cpp)
void GameSpyClient_SetMasterServer(const char* sAddr);

// MasterServer's value ("" when it is not set).
inline void M17_GetMasterServer(char* szOut, int nSize)
{
	szOut[0] = '\0';

	HCONSOLEVAR hVar = g_pLTClient->GetConsoleVar((char*)M17_MASTER_VAR);
	if (!hVar) return;

	const char* szVal = g_pLTClient->GetVarValueString(hVar);
	if (!szVal) return;

	strncpy(szOut, szVal, nSize - 1);
	szOut[nSize - 1] = '\0';
}

// Hands MasterServer to the browser and to any server this process starts.
inline void M17_ApplyMasterServer()
{
	char szMaster[128];
	M17_GetMasterServer(szMaster, sizeof(szMaster));

	GameSpyClient_SetMasterServer(szMaster);
	SetEnvironmentVariable(M17_MASTER_ENV, szMaster);
}

#endif // AVP2SRC_MODERN

#endif // __M17_MASTER_H__
