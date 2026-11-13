// ----------------------------------------------------------------------- //
//
// MODULE  : m20_serverlog.h
//
// PURPOSE : The modern dedicated server's console log (docs/MODERN.md,
//           M-20). AVP2Serv.exe and AVP2XServ.exe show their console only
//           in their window, so a server run with nobody watching it (a
//           machine of its own, a container under Xvfb) left no record.
//           Every console line also goes to server-console.log in the game
//           directory (the current directory: the servers run from it),
//           emptied by each run's first line.
//
//           Included by ServerDlg.cpp inside its AVP2SRC_MODERN blocks.
//
// ----------------------------------------------------------------------- //

#ifndef __M20_SERVERLOG_H__
#define __M20_SERVERLOG_H__

#ifdef AVP2SRC_MODERN

#include <stdio.h>
#include <time.h>

#define M20_LOG_FILE		"server-console.log"

inline void M20_LogConsoleLine(const char* szLine)
{
	static bool s_bStarted = false;

	FILE* pFile = fopen(M20_LOG_FILE, s_bStarted ? "a" : "w");
	if (!pFile) return;
	s_bStarted = true;

	char szTime[16];
	time_t t = time(NULL);
	strftime(szTime, sizeof(szTime), "%H:%M:%S", localtime(&t));

	fprintf(pFile, "%s %s\n", szTime, szLine);
	fclose(pFile);
}

#endif // AVP2SRC_MODERN

#endif // __M20_SERVERLOG_H__
