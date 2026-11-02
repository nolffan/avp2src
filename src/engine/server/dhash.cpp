// server.dll — the engine's hash table (reconstruction). Build /O1.
//
// Four key kinds share one table: a word, a string, a case-insensitive
// string, and a filename (case-insensitive with the path separators folded
// together). The kind is chosen at create time and indexes the two function
// tables below, which is why the create rewrites both every time rather than
// initialising them once.
//
// The file name is ours.
#include "servermgr.h"

// The bank's two ends, inline so the static-initialisation pair below
// expands them as the shipped one does: 0x1000d967 stores the vptr
// (0x1004d638) and calls sb_Init2(&bank, 0x18, 0x20, 0x40); the atexit
// thunk 0x1000d991 stores it again, calls sb_FreeChunks(&bank, 1) and ends
// on the base's (0x1004d260, CBankBase's). servermgr.h only declares them.
inline CHashElementBank::CHashElementBank()
{
	sb_Init2(&m_Bank, 0x18, 0x20, 0x40);
}

inline CHashElementBank::~CHashElementBank()
{
	sb_FreeChunks(&m_Bank, 1);
}

// 0x100583e8 — the bank word-keyed elements come out of (servermgr.h). Its
// static-initialisation pair, 0x1000d95d (the constructor expanded) and the
// atexit thunk 0x1000d991 (the destructor expanded), opens this unit's code
// in the shipped image. Its free list at +0x1c (0x10058404) is where the
// frees below put recycled elements.
CHashElementBank g_HashElementBank;

// 0x1005840c and 0x1005841c — the per-key-kind compare and hash tables
// dhash_Create fills (servermgr.h); they follow the bank in .bss (zeroed).
DHashCmpFn g_DHashCmpFns[DHASHKEY_COUNT];
DHashFn    g_DHashFns[DHASHKEY_COUNT];


// ---- the hashes --------------------------------------------------------- //

// A word key hashes to itself.
uint32 dhash_HashWord(void* pKey, uint32 nKeyLen)
{
	return *(uint16*)pKey;
}

// The string hashes weight each character by its position, so an anagram
// lands somewhere else. The case-insensitive one folds as it goes rather
// than copying the key first.
//
// NEAR (46 of 46 bytes): a straight ecx/edx transposition — the shipped
// build keeps the character in cl and the index in edx, ours the other way
// round. Same instructions, same length.
uint32 dhash_HashNoCase(void* pKey, uint32 nKeyLen)
{
	char   c;
	char   cUpper;
	uint32 nHash = 0;
	uint32 i;

	for (i = 0; i < nKeyLen; i++)
	{
		c = ((char*)pKey)[i];

		// The fold writes a SECOND character rather than folding in place,
		// which is what the bytes say: `c -= 0x20` on the variable itself
		// compiles to `add $0xe0`, and only an if/else into another char
		// gives the shipped `sub $0x20` (and with it the whole function's
		// register assignment).
		if (c >= 'a' && c <= 'z')
			cUpper = (char)(c - ('a' - 'A'));
		else
			cUpper = c;

		nHash += cUpper * i;
	}

	return nHash;
}

uint32 dhash_HashString(void* pKey, uint32 nKeyLen)
{
	uint32 nHash = 0;
	uint32 i;

	for (i = 0; i < nKeyLen; i++)
		nHash += ((char*)pKey)[i] * i;

	return nHash;
}

// A filename key folds case and treats the two path separators as the same
// character, so "Sounds/X.wav" and "sounds\x.wav" collide on purpose.
// `inline`: lithtech.exe (/O2) expands it into dhash_HashFilename (0x42cc70)
// and both sides of dhash_CmpFilename (0x42cd60); server.dll (/O1) keeps it
// out of line and calls it (0x1000daf9).
inline char dhash_FoldFileChar(char c)
{
	if (c >= 'a' && c <= 'z')
		c -= 'a' - 'A';

	if (c == '/')
		c += '\\' - '/';

	return c;
}

uint32 dhash_HashFilename(void* pKey, uint32 nKeyLen)
{
	uint32 nHash = 0;
	uint32 i;

	for (i = 0; i < nKeyLen; i++)
		nHash += dhash_FoldFileChar(((char*)pKey)[i]) * i;

	return nHash;
}


// ---- the comparisons ---------------------------------------------------- //

LTBOOL dhash_CmpWord(void* pKey1, void* pKey2, uint32 nKeyLen)
{
	return *(uint16*)pKey1 == *(uint16*)pKey2;
}

// The same fold as dhash_HashNoCase, and written the same way: each character
// is read, and an if/else picks the folded or the plain one into a SECOND
// character. Folding in place (`c1 -= 0x20`) gives `add $0xe0` where the
// shipped build has `sub $0x20`.
LTBOOL dhash_CmpNoCase(void* pKey1, void* pKey2, uint32 nKeyLen)
{
	char* p1 = (char*)pKey1;
	char* p2 = (char*)pKey2;
	char* pEnd = p1 + nKeyLen;
	char  c1, c1Upper;
	char  c2, c2Upper;

	while (p1 < pEnd)
	{
		c1 = *p1;
		if (c1 >= 'a' && c1 <= 'z')
			c1Upper = (char)(c1 - ('a' - 'A'));
		else
			c1Upper = c1;

		c2 = *p2;
		if (c2 >= 'a' && c2 <= 'z')
			c2Upper = (char)(c2 - ('a' - 'A'));
		else
			c2Upper = c2;

		if (c1Upper != c2Upper)
			return LTFALSE;

		p1++;
		p2++;
	}

	return LTTRUE;
}

LTBOOL dhash_CmpString(void* pKey1, void* pKey2, uint32 nKeyLen)
{
	char* p1 = (char*)pKey1;
	char* p2 = (char*)pKey2;
	char* pEnd = p1 + nKeyLen;

	while (p1 < pEnd)
	{
		if (*p1 != *p2)
			return LTFALSE;

		p1++;
		p2++;
	}

	return LTTRUE;
}

LTBOOL dhash_CmpFilename(void* pKey1, void* pKey2, uint32 nKeyLen)
{
	char* p1 = (char*)pKey1;
	char* p2 = (char*)pKey2;
	char* pEnd = p1 + nKeyLen;

	while (p1 < pEnd)
	{
		if (dhash_FoldFileChar(*p1) != dhash_FoldFileChar(*p2))
			return LTFALSE;

		p1++;
		p2++;
	}

	return LTTRUE;
}


// ---- the table ---------------------------------------------------------- //

void* dhash_Create(int nSlots, int nKeyType)
{
	CHashTable* pTable;
	uint32      nSize;
	uint32      i;

	if (!nSlots)
		return LTNULL;

	if (nKeyType < 0 || nKeyType >= DHASHKEY_COUNT)
		return LTNULL;

	nSize = sizeof(CHashTable) + (nSlots - 1) * sizeof(CHashSlot);

	pTable = (CHashTable*)dalloc(nSize);
	memset(pTable, 0, nSize);

	pTable->m_nKeyType = nKeyType;
	pTable->m_nSlots = nSlots;

	for (i = 0; i < (uint32)nSlots; i++)
	{
		pTable->m_Slots[i].m_pTable = pTable;
		pTable->m_Slots[i].m_nIndex = i;
		pTable->m_Slots[i].m_Head.TieOff();
	}

	g_DHashFns[DHASHKEY_WORD]     = dhash_HashWord;
	g_DHashFns[DHASHKEY_STRING]   = dhash_HashNoCase;
	g_DHashFns[DHASHKEY_NOCASE]   = dhash_HashString;
	g_DHashFns[DHASHKEY_FILENAME] = dhash_HashFilename;

	g_DHashCmpFns[DHASHKEY_WORD]     = dhash_CmpWord;
	g_DHashCmpFns[DHASHKEY_STRING]   = dhash_CmpNoCase;
	g_DHashCmpFns[DHASHKEY_NOCASE]   = dhash_CmpString;
	g_DHashCmpFns[DHASHKEY_FILENAME] = dhash_CmpFilename;

	return pTable;
}


// ---- the lookups -------------------------------------------------------- //

// 0x1000dccd — which bucket a key lands in. The modulus is one less than the
// slot count, so the last slot only ever holds what collides into it.
uint32 dhash_GetSlot(CHashTable* pTable, void* pKey, uint32 nKeyLen)
{
	uint32 nHash = g_DHashFns[pTable->m_nKeyType](pKey, nKeyLen);

	return nHash % (pTable->m_nSlots - 1);
}

void* dhash_GetData(void* hElement)
{
	if (!hElement)
		return LTNULL;

	return ((CHashElement*)hElement)->m_pData;
}

// 0x1000ddfb — the key an element was filed under, which is how a name comes
// back out of an identifier: the string is the key, stored inline.
char* dhash_GetKey(void* hElement, uint32* pLen)
{
	if (!hElement)
		return LTNULL;

	if (pLen)
		*pLen = ((CHashElement*)hElement)->m_nKeyLen;

	return ((CHashElement*)hElement)->m_Key;
}

void* dhash_FindElement(CHashTable* pTable, void* pKey, uint32 nKeyLen)
{
	CHashSlot* pSlot;
	LTLink*    pHead;
	LTLink*    pCur;

	if (!pTable)
		return LTNULL;

	pSlot = &pTable->m_Slots[dhash_GetSlot(pTable, pKey, nKeyLen)];
	pCur = pSlot->m_Head.m_pNext;
	pHead = &pSlot->m_Head;

	while (pCur != pHead)
	{
		CHashElement* pElement = (CHashElement*)pCur->m_pData;

		if (pElement->m_nKeyLen == nKeyLen &&
		    g_DHashCmpFns[pTable->m_nKeyType](pElement->m_Key, pKey, nKeyLen))
		{
			return pElement;
		}

		pCur = pCur->m_pNext;
	}

	return LTNULL;
}

// The same walk continued from an element already found, for the callers that
// file more than one thing under a name.
//
// Unlike the sibling lookup above, the list head is NOT held in a local here:
// the shipped build computes it with a `lea` off a still-live slot pointer
// rather than stepping the slot pointer forward in place, which is what the
// loop condition testing `&pSlot->m_Head` directly produces. Held in a
// `pHead` local it is one `add` and a three-register transposition off.
void* dhash_FindNextElement(CHashTable* pTable, void* hElement, void* pKey, uint32 nKeyLen)
{
	CHashSlot* pSlot;
	LTLink*    pCur;

	if (!pTable)
		return LTNULL;

	if (!hElement)
		return LTNULL;

	pSlot = ((CHashElement*)hElement)->m_pSlot;
	pCur = ((CHashElement*)hElement)->m_Link.m_pNext;

	while (pCur != &pSlot->m_Head)
	{
		CHashElement* pElement = (CHashElement*)pCur->m_pData;

		if (pElement->m_nKeyLen == nKeyLen &&
		    g_DHashCmpFns[pTable->m_nKeyType](pElement->m_Key, pKey, nKeyLen))
		{
			return pElement;
		}

		pCur = pCur->m_pNext;
	}

	return LTNULL;
}

// Word-keyed elements are all the same size, so they go on a free list rather
// than back to the allocator.
void dhash_Remove(void* hTable, void* hElement)
{
	CHashTable*   pTable = (CHashTable*)hTable;
	CHashElement* pElement = (CHashElement*)hElement;

	if (!pTable)
		return;

	if (!pElement)
		return;

	pElement->m_pSlot->m_nElements--;

	pElement->m_Link.m_pPrev->m_pNext = pElement->m_Link.m_pNext;
	pElement->m_Link.m_pNext->m_pPrev = pElement->m_Link.m_pPrev;
	pElement->m_Link.TieOff();

	if (pTable->m_nKeyType == DHASHKEY_WORD)
	{
		pElement->m_Link.m_pPrev = (LTLink*)g_HashElementBank.m_Bank.m_pFreeList;
		g_HashElementBank.m_Bank.m_pFreeList = pElement;
	}
	else
	{
		dfree(pElement);
	}
}


// ---- iteration and teardown --------------------------------------------- //

// 0x1000de4c — the walk both iterators run: step past the given link, and
// when that lands on a bucket's own head, carry on into the next bucket
// (name ours). Elements are linked by a link embedded at their own offset
// zero, so a link and its element are the same address.
void* dhash_NextFrom(CHashSlot* pSlot, LTLink* pLink)
{
	pLink = pLink->m_pNext;

	while (pLink == &pSlot->m_Head)
	{
		if (pSlot->m_nIndex == pSlot->m_pTable->m_nSlots - 1)
			return LTNULL;

		pLink = pSlot[1].m_Head.m_pNext;
		pSlot++;
	}

	return pLink;
}

void* dhash_GetFirst(void* hTable)
{
	CHashTable* pTable = (CHashTable*)hTable;

	if (!pTable)
		return LTNULL;

	return dhash_NextFrom(&pTable->m_Slots[0], &pTable->m_Slots[0].m_Head);
}

// The iterator is the element itself; the caller's variable is advanced
// before the current one is handed back.
void* dhash_GetNext(void** phIter)
{
	CHashElement* pCur;

	if (phIter)
	{
		pCur = (CHashElement*)*phIter;

		if (pCur)
		{
			*phIter = dhash_NextFrom(pCur->m_pSlot, &pCur->m_Link);
			return pCur->m_Link.m_pData;
		}
	}

	return LTNULL;
}

void dhash_Free(void* hTable)
{
	CHashTable* pTable = (CHashTable*)hTable;
	uint32      i;

	if (!pTable)
		return;

	for (i = 0; i < pTable->m_nSlots; i++)
	{
		LTLink* pHead = &pTable->m_Slots[i].m_Head;
		LTLink* pCur = pHead->m_pNext;

		while (pCur != pHead)
		{
			LTLink*       pNext = pCur->m_pNext;
			CHashElement* pElement = (CHashElement*)pCur->m_pData;

			if (pTable->m_nKeyType == DHASHKEY_WORD)
			{
				pElement->m_Link.m_pPrev = (LTLink*)g_HashElementBank.m_Bank.m_pFreeList;
				g_HashElementBank.m_Bank.m_pFreeList = pElement;
			}
			else
			{
				dfree(pElement);
			}

			pCur = pNext;
		}
	}

	dfree(pTable);
}


// ---- adding ------------------------------------------------------------- //

#ifndef LT_SERVER_BUILD
// lithtech.exe only (0x0042ce61..0x0042ce8f): /O2 expands the bank's
// Allocate -- the forward onto sb_Allocate and sb_Allocate itself (free-list
// head, the sb_Grow call 0x00490c20, the unlink) -- where server.dll /O1
// calls the out-of-line forward 0x1000ed6f (servermessage.cpp). servermgr.h
// only declares that member, so this TU sees the body through a local
// stand-in with the same forward (name ours).
inline CHashElement* dhash_BankAllocate(CHashElementBank* pBank)
{
	return (CHashElement*)sb_Allocate(&pBank->m_Bank);
}
#endif

// A new element for a key. Word keys are all one size so they come out of a
// bank; every other kind carries its key inline and is sized to fit, header
// and key together. The table's own count only moves when a bucket goes from
// empty to occupied, so it counts buckets in use rather than elements.
void* dhash_AddElement(CHashTable* pTable, void* pKey, uint32 nKeyLen)
{
	CHashSlot*    pSlot;
	CHashElement* pElement;
	LTLink*       pHead;

	if (!pTable)
		return LTNULL;

	pSlot = &pTable->m_Slots[dhash_GetSlot(pTable, pKey, nKeyLen)];

	if (pTable->m_nKeyType == DHASHKEY_WORD)
#ifdef LT_SERVER_BUILD
		pElement = g_HashElementBank.Allocate();
#else
		pElement = dhash_BankAllocate(&g_HashElementBank);
#endif
	else
		pElement = (CHashElement*)dalloc(nKeyLen + 0x16);

	pElement->m_nKeyLen = (uint16)nKeyLen;
	memcpy(pElement->m_Key, pKey, nKeyLen);
	pElement->m_pData = LTNULL;
	pElement->m_pSlot = pSlot;
	pElement->m_Link.m_pData = pElement;

	pHead = &pSlot->m_Head;

	pElement->m_Link.m_pPrev = pHead;
	pElement->m_Link.m_pNext = pHead->m_pNext;
	pElement->m_Link.m_pNext->m_pPrev = &pElement->m_Link;
	pElement->m_Link.m_pPrev->m_pNext = &pElement->m_Link;

	pSlot->m_nElements++;

	if (pSlot->m_nElements > 1)
		pTable->m_Unknown04++;

	return pElement;
}

// 0x1000de23 — hang the caller's payload off an element (name ours).
void dhash_SetData(void* hElement, void* pData)
{
	if (hElement)
		((CHashElement*)hElement)->m_pData = pData;
}
