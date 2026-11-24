// ----------------------------------------------------------------------- //
//
// MODULE  : m31_nettext.h
//
// PURPOSE : M-31 (docs/MODERN.md): text from the network goes into its
//           buffer with a limit. Server names, map names, game types and
//           player names come from the GameSpy reply of a server, and
//           chat lines come from the server. The retail code copies them
//           with strcpy and sprintf into buffers of fixed size, so a
//           server or a client that sends a long text writes past the
//           buffer. Also a race number from a GameSpy reply is an index
//           into a table of names, and the retail code does not examine
//           it.
//
//           Included by MessageMgr.cpp, FolderJoin.cpp, FolderJoinInfo.cpp.
//
// ----------------------------------------------------------------------- //

#ifndef __M31_NETTEXT_H__
#define __M31_NETTEXT_H__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// Copy at most nSize - 1 characters, and always add the terminator.
inline void M31_Copy(char* pDst, int nSize, const char* pSrc)
{
	if (!pDst || nSize <= 0) return;
	if (!pSrc) pSrc = "";
	strncpy(pDst, pSrc, nSize - 1);
	pDst[nSize - 1] = '\0';
}

// sprintf into a buffer of nSize bytes. A long result is cut, and it
// always has the terminator.
inline void M31_Format(char* pDst, int nSize, const char* pFormat, ...)
{
	if (!pDst || nSize <= 0) return;
	va_list marker;
	va_start(marker, pFormat);
	_vsnprintf(pDst, nSize - 1, pFormat, marker);
	va_end(marker);
	pDst[nSize - 1] = '\0';
}

// A number from the network as an index into a table of nCount entries:
// -1 when it is outside the table.
inline int M31_Index(int n, int nCount)
{
	return (n >= 0 && n < nCount) ? n : -1;
}

#endif
