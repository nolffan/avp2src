// server.dll — the string bank behind HSTRING (reconstruction). Build /O1.
//
// An HSTRING is a refcounted record threaded into a doubly linked list, with
// its characters inline after the header. The ILTCSBase string methods in
// servermessage.cpp are thin forwards onto these; this is where the work is.
//
// The file name is ours. These functions sit at 0x10035e3e-0x10035efa, which
// is between the stream wrapper and the TCP driver, so the original grouped
// them with something we have not reached yet.
#include <string.h>
#include <mbstring.h>
#include "servermgr.h"

extern "C" {
__declspec(dllimport) void* __stdcall LocalFree(void* hMem);
}

// One string record. The characters run from +0x14 to the end of the
// allocation; the length beside them is what the length-reporting getter
// hands back (names ours).
class CStringRec
{
public:
	// The links run next-then-prev, which is the opposite order to the SDK's
	// LTLink; the third field points back at the record the way an LTLink's
	// data field does.
	CStringRec* m_pNext;                     // 0x00
	CStringRec* m_pPrev;                     // 0x04
	CStringRec* m_pSelf;                     // 0x08
	uint32      m_RefCount;                  // 0x0c
	uint16      m_nChars;                    // 0x10  characters, which is not the
	uint16      m_Len;                       // 0x12  byte count on a lead-byte set
	char        m_Data[1];                   // 0x14
};

// 0x10058bf8 — the head of the list every live string is on (name ours).
// Zero-initialised (.bss); the bank's init at 0x10035c2a (streamsim.cpp's
// stream_Init) points both links at the head on its first call. The shipped
// head is only the three link words (0x10058c04, right after them, is that
// init's reference count), so it is streamsim.cpp's LTLink g_StreamList,
// defined there; here it is viewed as a record, of which only the links are
// used. A second definition here left the list stream_Init ties off and the
// one sb_EnumStrings walks as two objects: DeleteServer faulted on the
// untied, zeroed one.
extern LTLink g_StreamList;
#define g_StringList (*(CStringRec*)&g_StreamList)

// 0x1004014d — one character on, which is a byte on most code pages and two
// on some: the CRT's _mbsinc (`_mbctype+1` & _M1 tests the lead byte).

// 0x10058c08 — the shared empty string. It is never unlinked or freed, which
// is the first thing the release path checks (name ours). Zero-initialised
// (.bss): no count, no characters, and a zero first byte of data.
CStringRec g_EmptyString;


// Copying a string is a reference, not a copy.
HSTRING sb_CopyString(HSTRING hString)
{
	if (!hString)
		return LTNULL;

	((CStringRec*)hString)->m_RefCount++;
	return hString;
}

// 0x10035e4b — drop a reference, and unlink and free the record when the last
// one goes. Both list pointers are re-read between the two stores because the
// first can be the same record.
void sb_ReleaseString(HSTRING hString)
{
	CStringRec* pRec = (CStringRec*)hString;

	if (pRec == &g_EmptyString)
		return;

	if (!pRec)
		return;

	if (--pRec->m_RefCount != 0)
		return;

	pRec->m_pPrev->m_pNext = pRec->m_pNext;
	pRec->m_pNext->m_pPrev = pRec->m_pPrev;
	dfree(pRec);
}

// 0x10035e76 / 0x10035e9e — the comparisons are the multibyte ones:
// 0x10040164 is _mbscmp and 0x10040213 is _mbsicmp (each falls back to
// _strcmp 0x1003e1f0 / _stricmp 0x10047080 when no multibyte code page is
// set, which is how they are told apart). The lithtech twins (0x004849c0 /
// 0x004849f0) call the same pair.
LTBOOL sb_CompareStrings(HSTRING hString1, HSTRING hString2)
{
	if (!hString1 || !hString2)
		return LTFALSE;

	return _mbscmp((unsigned char*)((CStringRec*)hString1)->m_Data,
	               (unsigned char*)((CStringRec*)hString2)->m_Data) == 0;
}

LTBOOL sb_CompareStringsUpper(HSTRING hString1, HSTRING hString2)
{
	if (!hString1 || !hString2)
		return LTFALSE;

	return _mbsicmp((unsigned char*)((CStringRec*)hString1)->m_Data,
	                (unsigned char*)((CStringRec*)hString2)->m_Data) == 0;
}

char* sb_GetStringData(HSTRING hString)
{
	if (!hString)
		return LTNULL;

	return ((CStringRec*)hString)->m_Data;
}

// 0x10035ed3 — the same thing with the length, which the message writer needs
// so it can size the payload without walking the characters (name ours).
char* sb_GetStringDataAndLen(HSTRING hString, uint32* pLen)
{
	if (!hString)
	{
		if (pLen)
			*pLen = 0;

		return LTNULL;
	}

	if (pLen)
		*pLen = ((CStringRec*)hString)->m_Len;

	return ((CStringRec*)hString)->m_Data;
}

// 0x1001372f — the guarded release the interface hands out.
void sb_FreeString(HSTRING hString)
{
	if (hString)
		sb_ReleaseString(hString);
}


// ---- creating ----------------------------------------------------------- //

// 0x10035d39 — the formatted-message buffer comes from the system, so it goes
// back the same way.
void str_FreeMessage(void* pRaw)
{
	if (pRaw)
		LocalFree(pRaw);
}

// A fresh record goes on the front of the live list with one reference. The
// empty string is shared and never allocated.
HSTRING sb_CreateString(char* pString)
{
	CStringRec* pRec;
	uint32      nLen;
	uint32      nSize;

	if (!pString)
		return LTNULL;

	if (pString[0] == 0)
		return (HSTRING)&g_EmptyString;

	nLen = strlen(pString);
	nSize = nLen + 1;

	pRec = (CStringRec*)dalloc(nSize + 0x17);

	pRec->m_RefCount = 1;
	pRec->m_nChars = (uint16)nLen;
	pRec->m_Len = (uint16)nSize;
	strcpy(pRec->m_Data, pString);

	pRec->m_pSelf = pRec;
	pRec->m_pPrev = &g_StringList;
	pRec->m_pNext = g_StringList.m_pNext;
	g_StringList.m_pNext->m_pPrev = pRec;
	pRec->m_pPrev->m_pNext = pRec;

	return (HSTRING)pRec;
}

// 0x10035d4b — the same thing counting characters rather than bytes, for text
// that came out of a message resource.
HSTRING sb_CreateStringRaw(char* pString)
{
	CStringRec* pRec;
	char*       pCur;
	uint32      nChars;
	uint32      nSize;

	if (!pString)
		return LTNULL;

	if (pString[0] == 0)
		return (HSTRING)&g_EmptyString;

	nChars = 0;
	pCur = pString;

	do
	{
		nChars++;
		pCur = (char*)_mbsinc((unsigned char*)pCur);
	}
	while (*pCur);

	nSize = (pCur - pString) + 1;

	pRec = (CStringRec*)dalloc(nSize + 0x17);

	pRec->m_RefCount = 1;
	pRec->m_nChars = (uint16)nChars;
	pRec->m_Len = (uint16)nSize;
	memcpy(pRec->m_Data, pString, nSize);

	pRec->m_pSelf = pRec;
	pRec->m_pPrev = &g_StringList;
	pRec->m_pNext = g_StringList.m_pNext;
	g_StringList.m_pNext->m_pPrev = pRec;
	pRec->m_pPrev->m_pNext = pRec;

	return (HSTRING)pRec;
}


// 0x10035c81 — hand every string still alive to a callback. Nothing here
// knows what the callback does; the server uses it at shutdown to report
// what was never released (name ours).
void sb_EnumStrings(void (*pFn)(char* pStr, void* pUser), void* pUser)
{
	CStringRec* pCur = g_StringList.m_pNext;

	while (pCur != &g_StringList)
	{
		pFn(pCur->m_pSelf->m_Data, pUser);

		pCur = pCur->m_pNext;
	}
}
