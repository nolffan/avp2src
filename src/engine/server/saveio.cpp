// server.dll — the object save file: the guard words CSaveIO puts around
// every object's data, and the two ends of the block itself (reconstruction).
// Build /O1.
#include <string.h>
#include <stdio.h>
#include "de_error.h"
#include "saveio.h"

// ltserverobj.h's CF_ALWAYSLOAD: objects of the class come out of the level,
// so they are not written to the save at all.
#define CF_ALWAYSLOAD (1 << 3)

CSaveIO::CSaveIO()
{
	m_n00 = 0;
	m_n04 = 0;
	m_pStream = LTNULL;
	m_n14 = 0;
	m_n0c = 0;
	m_n10 = 0;
}

// Read the caller's bytes, then the sentinel that should follow them. A read
// that asked for nothing is not guarded, and neither is one that already
// failed — there is no point checking a boundary that was never reached.
//
// The sentinel is a block-scoped local of its own, not the length
// parameter reused. /O1 homes it in nLen's dead slot either way
// (0x100100b1 `lea 0xc(%ebp)`), but /O2 keeps nLen in %edi across the first
// call only when its address is never taken (lithtech.exe 0x00431fa8 /
// 0x00431ff0). A function-scope local instead lands in pData's slot at /O1
// (`lea 0x8(%ebp)`); the block scope is what gives both images. Measured;
// the same edit closes WriteChecked below.
LTRESULT CSaveIO::ReadChecked(void* pData, uint32 nLen)
{
	LTRESULT dResult = m_pStream->Read(pData, nLen);

	if (nLen == 0 || dResult)
		return dResult;

	{
		uint32 nSentinel;

		dResult = m_pStream->Read(&nSentinel, 4);

		if (nSentinel != SAVE_SENTINEL)
			return LT_ERROR;
	}

	return dResult;
}

LTRESULT CSaveIO::WriteChecked(void* pData, uint32 nLen)
{
	LTRESULT dResult = m_pStream->Write(pData, nLen);

	if (nLen == 0 || dResult)
		return dResult;

	{
		uint32 nSentinel = SAVE_SENTINEL;

		return m_pStream->Write(&nSentinel, 4);
	}
}

LTRESULT CSaveIO::GetNothing(uint32* pOut)
{
	*pOut = 0;

	return LT_OK;
}


// ---- saving a world's objects ---------------------------------------- //
//
// Everything below needs types saveio.h does not carry, and every one of
// them belongs to a module this reconstruction does not own — the object
// list is the SDK's, the object and its engine-side data are
// ltobject_internal.h's, and the manager is servermgr.h's. They are declared
// here as views, at the offsets the shipped code reaches, rather than by
// pulling those headers in.

// The SDK's ILTServer::ObjectList pair (iltserver.h), which is what
// SaveObjects is handed.
struct SavObjectLink
{
	class SavObject* m_hObject;              // 0x00
	SavObjectLink*   m_pNext;                // 0x04
};

struct SavObjectList
{
	SavObjectLink* m_pFirstLink;             // 0x00
	int            m_nInList;                // 0x04
};

// ltserverobj.h's ClassDef: a name, the class it came from, and the CF_
// flags. The walk below is over the parent chain.
struct SavClassDef
{
	char*        m_ClassName;                // 0x00
	SavClassDef* m_ParentClass;              // 0x04
	uint32       m_ClassFlags;               // 0x08
};

// 0x10001778 — the first class in a chain carrying any of the given flags,
// or nothing. Its FIRST parameter is never read: the shipped body loads only
// 0x8(%esp) and 0xc(%esp), so whatever the caller passes as the class
// manager is dead weight, and the two calls in this module pass it all the
// same (name ours).
SavClassDef* cls_FindFlagged(void* pUnused, SavClassDef* pClass, uint32 nFlags);

// An LTLink as the engine lays it out (ltlink.h): prev, next, then the data
// pointer. Declared here rather than used from the SDK because the lists
// below are only ever walked, never built.
struct SavLink
{
	SavLink* m_pPrev;                        // 0x00
	SavLink* m_pNext;                        // 0x04
	void*    m_pData;                        // 0x08
};

// ltobject_internal.h's SObjData, as far as this needs it: the head of the
// object's relation list and the class record.
class SavObjData
{
public:
	SavLink      m_Relations;                // 0x00
	uint8        m_pad_0c[0x20 - 0x0c];
	class SavBaseClass* m_pObject;           // 0x20  the game-side object
	SavClassDef* m_pClass;                   // 0x24
};

// ltserverobj.h's BaseClass, as far as the restore needs it: slot 1 of its
// vtable is EngineMessageFn, which is how the engine hands an object its
// own save data. Declared as a view because ltserverobj.h is the game's
// header, not the engine's.
class SavBaseClass
{
public:
	virtual ~SavBaseClass();
	virtual uint32 EngineMessageFn(uint32 messageID, void* pData, float fData);
};

// ltserverobj.h's message id for "here is your saved data".
#define SAV_MID_LOADOBJECT 7

// One entry on an object's attachment list: an offset, a rotation, the
// attached object's runtime id and a flag word, threaded on +0x24. The two
// bytes at +0x1c are not written to the save, so nothing here names them
// (names ours, from what each write's size and the restore's use of it say).
class SavObjLink
{
public:
	float       m_Offset[3];                 // 0x00
	float       m_Rotation[4];               // 0x0c
	uint16      m_n1c;                       // 0x1c
	uint16      m_nObjectID;                 // 0x1e
	uint32      m_n20;                       // 0x20
	SavObjLink* m_pNext;                     // 0x24
};

// One entry on an object's relation list. The record is on the lists of both
// objects it joins, which is why the walk below has to ask each one whether
// this object is the owning end (names ours).
class SavObjRelation
{
public:
	uint32     m_nType;                      // 0x00
	SavObject* m_pOwner;                     // 0x04
	SavObject* m_pOther;                     // 0x08
};


// ltobject_internal.h's LTObject. The word at 0xba is the object's index in
// the save file — 0x10025a82 sets it to 0xffff for every live object before
// the save starts, and the loop below overwrites it for the objects that are
// actually going out, so an object a saved reference names but which was not
// itself saved reads back as 0xffff (names ours).
#define SAV_NOINDEX 0xffff

class SavObject
{
public:
	uint8       m_pad_00[0x98];
	class SavObjLink* m_pLinks;              // 0x98
	uint8       m_pad_9c[0xb8 - 0x9c];
	uint16      m_nObjectID;                 // 0xb8  the runtime id
	uint16      m_nSaveIndex;                // 0xba
	uint8       m_pad_bc[0x188 - 0xbc];
	SavObjData* m_pSData;                    // 0x188
};

// worldbsp.h's CWorldChunk: 0x24 bytes, and the only thing saved out of one
// is the word at +0x06.
class SavWorldChunk
{
public:
	uint8  m_pad_00[0x06];
	uint16 m_n06;                            // 0x06
	uint8  m_pad_08[0x24 - 0x08];
};

// worldbsp.h's CWorldBsp, reached through the file-tree record's +0x04.
class SavWorldBsp
{
public:
	uint8          m_pad_00[0xb0];
	SavWorldChunk* m_pChunks;                // 0xb0
	uint32         m_nChunks;                // 0xb4
};

class SavFileTreeRec
{
public:
	uint8        m_pad_00[0x04];
	SavWorldBsp* m_pWorldBsp;                // 0x04
};

// servermgr.h's LTCommandVar: the console variable's name (which is also its
// hash key) and its interned string value. The save writes the pair out and
// the restore hands "<name> <value>" straight back to the console, which is
// what says the block is the console's saved variables.
class SavCommandVar
{
public:
	uint8 m_pad_00[0x10];
	char* m_pName;                           // 0x10
	char* m_pStringVal;                      // 0x14
};

// servermgr.h's ConsoleState, embedded in the manager at 0xa74. Only its
// variable hash is reached from here — pMgr+0xab0 is m_hVarHash — and the
// console runner takes the whole state.
class SavConsoleState
{
public:
	uint8 m_pad_00[0x3c];
	void* m_hVarHash;                        // 0x3c  (manager +0xab0)
	uint8 m_pad_40[0x4c - 0x40];
};

// servermgr.h's CServerMgr, at the offsets this reaches (names follow
// servermgr.h where it has them).
class SavServerMgr
{
public:
	uint8            m_pad_000[0x330];
	SavFileTreeRec** m_pFileTrees;           // 0x330
	uint32           m_nFileTrees;           // 0x334
	uint8            m_pad_338[0x390 - 0x338];
	uint8            m_Unknown390[4];        // 0x390  the object manager
	uint8            m_pad_394[0x6b8 - 0x394];
	// servermgr.h's types: five of the seven are floats and the arithmetic
	// at the end of the restore only works out if 0x6b8 and 0x6c0 are.
	float            m_Unknown6b8;           // 0x6b8
	float            m_Unknown6bc;           // 0x6bc
	float            m_Unknown6c0;           // 0x6c0
	uint32           m_Unknown6c4;           // 0x6c4
	float            m_Unknown6c8;           // 0x6c8
	float            m_Time;                 // 0x6cc
	int              m_Unknown6d0;           // 0x6d0
	uint8            m_pad_6d4[0x804 - 0x6d4];
	// What the loader resolves saved class names against; the class-manager
	// list head two dwords further on is a different thing and the save
	// filter uses that one (names ours).
	void*            m_pClassMgr;            // 0x804
	uint8            m_pad_808[4];
	void*            m_ClassMgrFirst;        // 0x80c
	uint8            m_pad_810[0xa74 - 0x810];
	SavConsoleState  m_ConsoleState;         // 0xa74
};

// The object table goes through the global allocation operators, not
// dalloc: 0x1000d25d is the one-argument `::operator new` forwarder in front
// of dalloc (0x1000d2ef), and 0x10028ace is `::operator delete`.

// dhash's two iterators (0x1000de33, 0x1000de74) and the data accessor
// (0x1000de16), all in dhash.cpp.
void* dhash_GetFirst(void* hTable);
void* dhash_GetNext(void** phIter);
void* dhash_GetData(void* hElement);

// 0x10025a82 — every live object's save index back to "not saved". It is
// handed the manager's 0x390 sub-object and reaches the eleven per-type
// lists 0x224 further in (name ours).
void un390_ClearSaveIndices(void* p390);

// The three per-object writers this drives, in the same module (names ours
// by what each pass writes).
void sav_SaveObject(SavServerMgr* pMgr, SavObject* pObj, ILTStream* pStream,
                    uint32 dwParam);                       // 0x1000fc39
void sav_SaveObjectLinks(SavServerMgr* pMgr, SavObject* pObj,
                         ILTStream* pStream);              // 0x10010111
void sav_SaveObjectExtra(SavServerMgr* pMgr, SavObject* pObj,
                         ILTStream* pStream);              // 0x100101c7
// (both defined below sav_SaveObjects)

// 0x10052a60 — the save file's format number, in writable data and written
// out as the first four bytes of the block (name ours). The shipped .data
// dword is 0x459.
uint32 g_nSaveVersion = 1113;

// Write a world's objects. The caller's list is first filtered: an object
// whose class, or any class it came from, carries CF_ALWAYSLOAD comes back
// with the level rather than out of the save, so it is dropped here. What
// survives is numbered, and those numbers are what every saved reference to
// an object uses — which is why every live object's number is reset first
// and only the objects going out get one.
//
// After the header the block has three optional or repeated parts: the
// world-chunk state (flag 2), a string-pair list terminated by a zero byte
// (flag 1), one pass per object, and then a second pass for the links each
// object holds. A -1 stands in wherever a count would be if the part is
// absent.
//
// Byte-identical on both images. Four source facts the bytes settle, each
// measured against the shipped 0x1000f93a (server) and 0x431770 (lithtech):
//
//  * the numbering loop sits behind its own `if (nObjects > 0)`, with the
//    index zeroed ahead of the test. Without the `if` the server build
//    materialises the constant four (`push $0x4; pop %ebx`) before that
//    loop's guard, steps the loop with `add %ebx,%ecx`, and holds ebx for
//    the rest of the function: the flag-2 world pointer spills, 0xc8 stops
//    being enregistered in the flag-1 block, and the filter loop's link and
//    object swap esi/ebx -- twenty-one blocks. With the `if`, the four is
//    loaded after the loop (0x1000f9cc) as shipped. At /O2 the index has to
//    be zeroed before the `if`, not inside it: shipped loads the count into
//    ecx and zeroes eax before the one `test` (0x4317f7).
//  * the name buffer is 204 bytes although only 200 are ever copied into
//    it. That is the frame's "dead" dword: `sub $0xe4` on the server and
//    `sub $0xec` on lithtech are both four over a 200-byte buffer.
//  * the tree count written at the head of the flag-2 part is in a block of
//    its own, which hands it the manager's dead parameter slot (`0x8(%ebp)`,
//    shared with the chunk loop's strength-reduced offset).
//  * the final -1 terminator is at function scope, so it gets its own frame
//    slot (`-0x18`) instead of the flags slot the object index reuses.
//
// Two things the bytes say and a reading would not. The allocation is never
// released on the failure path because there is no failure path: a null
// allocation returns straight away and nothing else here can fail. And the
// function answers nothing at all — neither exit sets eax — so whatever the
// interface wrapper returns, it does not come from here.
void sav_SaveObjects(SavServerMgr* pMgr, ILTStream* pStream,
                     SavObjectList* pList, uint32 dwParam, uint32 flags)
{
	char        szBuf[204];
	int         nObjects;
	uint32      nTemp;
	SavObject** ppObjects;
	uint32      nChunks;
	void*       hIter;
	int         i;
	char        bMore;
	int         nEnd;

	ppObjects = (SavObject**)::operator new(pList->m_nInList * sizeof(SavObject*));

	if (!ppObjects)
		return;

	nObjects = 0;

	{
		SavObjectLink* pLink;

		for (pLink = pList->m_pFirstLink; pLink; pLink = pLink->m_pNext)
		{
			SavObject* pObj = pLink->m_hObject;

			if (!cls_FindFlagged(pMgr->m_ClassMgrFirst,
			                     pObj->m_pSData->m_pClass, CF_ALWAYSLOAD))
			{
				ppObjects[nObjects] = pObj;

				nObjects++;
			}
		}
	}

	un390_ClearSaveIndices(pMgr->m_Unknown390);

	i = 0;

	if (nObjects > 0)
	{
		for (; i < nObjects; i++)
			ppObjects[i]->m_nSaveIndex = (uint16)i;
	}

	pStream->Write(&g_nSaveVersion, 4);
	pStream->Write(&nObjects, 4);
	pStream->Write(&pMgr->m_Unknown6c4, 4);
	pStream->Write(&pMgr->m_Unknown6bc, 4);
	pStream->Write(&pMgr->m_Unknown6c0, 4);
	pStream->Write(&pMgr->m_Unknown6d0, 4);
	pStream->Write(&pMgr->m_Unknown6c8, 4);
	pStream->Write(&pMgr->m_Time, 4);
	pStream->Write(&pMgr->m_Unknown6b8, 4);

	if (flags & 2)
	{
		uint32 iChunk;

		{
			uint32 nTrees = pMgr->m_nFileTrees;

			pStream->Write(&nTrees, 4);
		}

		for (nTemp = 0; nTemp < pMgr->m_nFileTrees; nTemp++)
		{
			SavWorldBsp* pBsp = pMgr->m_pFileTrees[nTemp]->m_pWorldBsp;

			nChunks = pBsp->m_nChunks;

			pStream->Write(&nChunks, 4);

			for (iChunk = 0; iChunk < nChunks; iChunk++)
				pStream->Write(&pBsp->m_pChunks[iChunk].m_n06, 2);
		}
	}
	else
	{
		nTemp = (uint32)-1;

		pStream->Write(&nTemp, 4);
	}

	if (flags & 1)
	{
		bMore = 1;

		hIter = dhash_GetFirst(pMgr->m_ConsoleState.m_hVarHash);

		while (hIter)
		{
			SavCommandVar* pVar =
				(SavCommandVar*)dhash_GetData(dhash_GetNext(&hIter));

			pStream->Write(&bMore, 1);

			strncpy(szBuf, pVar->m_pName, 200);
			pStream->WriteString(szBuf);

			strncpy(szBuf, pVar->m_pStringVal, 200);
			pStream->WriteString(szBuf);
		}
	}

	bMore = 0;

	pStream->Write(&bMore, 1);

	for (i = 0; i < nObjects; i++)
		sav_SaveObject(pMgr, ppObjects[i], pStream, dwParam);

	nEnd = -1;

	pStream->Write(&nEnd, 4);

	for (i = 0; i < nObjects; i++)
	{
		sav_SaveObjectLinks(pMgr, ppObjects[i], pStream);
		sav_SaveObjectExtra(pMgr, ppObjects[i], pStream);
	}

	bMore = 0;

	pStream->Write(&bMore, 1);

	::operator delete(ppObjects);
}


// 0x1002acd3 — the manager's runtime-id table (smgr.cpp): 0x904 entries of
// eight bytes at 0x900, a kind byte and the object. Only kind 1 is a live
// object; anything else, and any id past the end, answers nothing (name
// ours).
SavObject* sm_ObjectFromID(SavServerMgr* pMgr, uint16 nID);

// 0x10010111 — one object's attachment list. Counted first and written
// second, because the count goes out in front of the entries and only the
// attachments whose object is itself going into the save are written at all:
// an attachment to something that stayed behind cannot be restored, so it is
// dropped rather than written with a dangling index.
//
// The count is written as two bytes out of a four-byte slot, and the object
// the entry names is written as its save index rather than its runtime id --
// which is the whole point of the two passes in sav_SaveObjects.
void sav_SaveObjectLinks(SavServerMgr* pMgr, SavObject* pObj,
                         ILTStream* pStream)
{
	SavObjLink* pLink;
	SavObject*  pRef;
	uint32      nLinks;

	nLinks = 0;

	for (pLink = pObj->m_pLinks; pLink; pLink = pLink->m_pNext)
	{
		pRef = sm_ObjectFromID(pMgr, pLink->m_nObjectID);

		if (pRef && pRef->m_nSaveIndex != SAV_NOINDEX)
			nLinks++;
	}

	pStream->Write(&nLinks, 2);

	for (pLink = pObj->m_pLinks; pLink; pLink = pLink->m_pNext)
	{
		pRef = sm_ObjectFromID(pMgr, pLink->m_nObjectID);

		if (pRef && pRef->m_nSaveIndex != SAV_NOINDEX)
		{
			pStream->Write(pLink->m_Offset, 0x0c);
			pStream->Write(pLink->m_Rotation, 0x10);
			pStream->Write(&pRef->m_nSaveIndex, 2);
			pStream->Write(&pLink->m_n20, 4);
		}
	}
}

// 0x100101c7 — and the same shape for the object's relations.
//
// Byte-identical on both images. The saved-index test reads the expression
// itself and only then takes the index's address for the write: lithtech's
// /O2 build shows the order (0x004321af `cmpw $0xffff,0xba(%ecx)` then
// 0x004321b8 `lea 0xba(%ecx),%edi`). Taking the address first and testing
// through it gave 190 bytes on server.dll (the cursor spilled instead of the
// index pointer) and a size-exact near on lithtech. A relation
// record sits on the lists of both objects it joins, so the walk takes only
// the ones this object owns, and only the two kinds the restore knows how to
// rebuild. The type goes out as one byte out of the record's four.
void sav_SaveObjectExtra(SavServerMgr* pMgr, SavObject* pObj,
                         ILTStream* pStream)
{
	SavLink*        pCur;
	SavObjRelation* pRel;
	uint32          nRelations;

	nRelations = 0;

	for (pCur = pObj->m_pSData->m_Relations.m_pNext;
	     pCur != &pObj->m_pSData->m_Relations;
	     pCur = pCur->m_pNext)
	{
		pRel = (SavObjRelation*)pCur->m_pData;

		if (pRel->m_pOwner != pObj)
			continue;

		if (pRel->m_nType != 0 && pRel->m_nType != 1)
			continue;

		if (pRel->m_pOther->m_nSaveIndex != SAV_NOINDEX)
			nRelations++;
	}

	pStream->Write(&nRelations, 2);

	for (pCur = pObj->m_pSData->m_Relations.m_pNext;
	     pCur != &pObj->m_pSData->m_Relations;
	     pCur = pCur->m_pNext)
	{
		uint16* pIndex;
		uint8   nType;

		pRel = (SavObjRelation*)pCur->m_pData;

		if (pRel->m_pOwner != pObj)
			continue;

		if (pRel->m_nType != 0 && pRel->m_nType != 1)
			continue;

		if (pRel->m_pOther->m_nSaveIndex == SAV_NOINDEX)
			continue;

		pIndex = &pRel->m_pOther->m_nSaveIndex;
		nType = (uint8)pRel->m_nType;

		pStream->Write(&nType, 1);
		pStream->Write(pIndex, 2);
	}
}


// ---- restoring a world's objects ------------------------------------- //

// 0x1000d374 and 0x1000d398 — dalloc's cleared form and the release
// (dalloc.cpp).
void* dalloc_z(uint32 nBytes);
void  dfree(void* p);

// 0x100383b6 — the engine's clock in seconds: timeGetTime() less the base
// captured at startup, scaled by the millisecond constant. Returns a float
// in st(0) (name ours).
float time_GetTime();

// 0x1000560c — servermgr.h's three-argument console runner, declared with
// console.cpp's own parameter type so the reference links to it; the caller
// hands it this file's view of the same embedded state.
class ConsoleState;
void con_RunString3(ConsoleState* pState, char* pString, int nSource);

// 0x10034688 — called three times here, once after the header has been
// accepted and twice around the object passes. It walks the manager's object
// list at 0x8e0 against the per-object table at 0x900; nothing reconstructed
// so far says what for, so it keeps its address for a name.
void sm_Sub34688(SavServerMgr* pMgr);

// The four per-object restore passes (names ours by which pass each is).
// Each answers zero to carry on, two to say the file's object run has ended,
// and anything else is the error the whole restore fails with.
#define SAV_MORE 0
#define SAV_DONE 2

// Declared with savrest.cpp's definition's types, so the reference links to
// it; the callers cast this file's views to them.
class CServerMgr;
class LTObject;
LTRESULT sav_RestoreObject(CServerMgr* pMgr, ILTStream* pStream,
                           LTObject** ppObject, uint32 dwParam);  // 0x100106b6
// 0x10010e13 — named by its own error literal, "sm_RestoreNextObject".
LTRESULT sm_RestoreNextObject(SavServerMgr* pMgr, ILTStream* pStream,
                              SavObject* pObject, uint32 dwParam);

// The last two are named by the string literals their own bounds checks
// report -- "RestoreAttachments" and "RestoreInterlinks" -- and are defined
// at the bottom of this file.
LTRESULT RestoreAttachments(SavServerMgr* pMgr, SavObject* pObject,
                            ILTStream* pStream, SavObject** ppObjects,
                            int nObjects);                         // 0x10010f76
LTRESULT RestoreInterlinks(SavServerMgr* pMgr, SavObject* pObject,
                           ILTStream* pStream, SavObject** ppObjects,
                           int nObjects);                          // 0x10011042

// 0x100585b0 and 0x100585ac — how many objects the file says it holds, and
// which one the data pass is on. Both are file-scope rather than locals, and
// the second one has to be: the `incl` against memory and the reload of the
// subscript out of it after every call are what a global gives and a local
// does not (names ours).
uint32 g_iRestoreObject;
int g_nRestoreObjects;

// Read a world's objects back. The header is a version, a count, seven
// timing words, and the world-chunk state; then the console variables the
// save wrote; then the objects, in three passes over the same stretch of
// stream, which is why the position is taken before the first pass and
// seeked back to before the second.
//
// The seven timing words are read into the manager only when the caller asks
// for them (flag 1). Otherwise they are read into seven locals and thrown
// away — the stream still has to be advanced past them, and the shipped
// frame has seven dwords nothing ever reads back, which is what says the
// source wrote seven throwaway variables rather than one seek.
//
// Both arms end with the same read, and VC6 emits that last call once: the
// flag-1 arm jumps into the tail the other arm's copy left behind.
//
// NEAR on server.dll (1074 bytes against 1082 since the count-check change
// below; it was size-exact with the loop-first `if`). The residuals then were
// two documented classes:
//
//  * the chunk loop's address arithmetic is
//    `lea 0x6(%edx,%ecx,1),%ecx` in the shipped code and
//    `lea 0x6(%ecx,%edx,1),%ecx` in ours — the same scale-1 SIB with the
//    base and index operands the other way round, one ModRM byte, no size
//    difference. The documented SIB-base/index-roles class.
//  * the LT_INVALIDFILE block. Its only reference is the stream check inside
//    the console-variable loop, and both builds defer it; the shipped build
//    flushes it at the very end of the function, after the third pass and
//    the release, while ours flushes it at the first block that ends without
//    a fall-through — the second pass's back edge. Size-neutral, and it
//    moves five relative displacements. The deferred-block-emission-order
//    class.
//
// Four source facts had to be found to get this far, and each is worth more
// than the residual:
//
//  * the seven timing words are floats, not dwords. servermgr.h says so and
//    the arithmetic proves it: with dwords the final assignment goes through
//    `fildll` and the function comes out thirty-three bytes short.
//  * the first count check: THE TWO IMAGES DISAGREE and lithtech.exe wins.
//    server.dll sinks the LT_NOTINITIALIZED block past the loop (the in-loop
//    site jumps forward into it); lithtech.exe 0x00432336 hosts it inline at
//    the first site, `je` over it, and keeps a second copy at 0x0043245c for
//    the in-loop site (the enumerator's "start missing at 0x0043245c" is that
//    copy, reached by the `jne` at 0x004323ae -- not a function). Our VC6
//    places the block the same way in both builds for either polarity, so no
//    single spelling reproduces both. Error-first (below) makes lithtech a
//    near (1343 against 1342, same 456 instructions; the residuals are the
//    4-byte frame, 0x43c against 0x438, and LT_INVALIDFILE flushed at the
//    do/while back edge rather than at the end) where the loop-first `if`
//    left it unpaired; it costs the server near eight bytes (1074 against
//    1082, still paired).
//  * the two object passes advance their cursor *inside* the call's argument
//    (`f(ppObject++)`), not in the loop body. The body form increments
//    before the push; the shipped code pushes the old value and increments
//    after.
//  * and the second of those two loops is a `do`/`while`, with the first
//    call written out separately ahead of it. A `while` there is peeled by
//    VC6 a second time and comes out seventeen bytes over; sharing the
//    peeled call's push tail with the loop's (which is what one `while` with
//    an empty body gives) comes out eleven bytes under.
//
// The three answers each pass can give are the whole control structure: zero
// means another object follows, two means the run has ended, and anything
// else is an error that skips straight to the release. So the "loops" have no
// bound at all — the count is only used for the numbering pass and the last
// pair of passes — and a truncated file is caught by the passes themselves.
LTRESULT sm_RestoreObjects(SavServerMgr* pMgr, ILTStream* pStream,
                           uint32 dwParam, uint32 flags)
{
	char        szCommand[0x200];
	char        szName[0x100];
	char        szValue[0x100];
	uint32      nVersion;
	int         nObjects;
	uint32      nTrees;
	uint32      nChunks;
	// The seven words read and dropped when the caller does not want the
	// timing restored. At function scope on purpose: inside the `else` two
	// of them are handed dead parameter slots instead of frame slots and the
	// frame comes out eight bytes short of the shipped 0x430.
	uint32      nSkip1, nSkip2, nSkip3, nSkip4, nSkip5, nSkip6, nSkip7;
	SavObject** ppObjects;
	LTRESULT    dResult;
	char        bMore;

	pStream->Read(&nVersion, 4);
	pStream->Read(&nObjects, 4);

	g_nRestoreObjects = nObjects;

	if (flags & 1)
	{
		pStream->Read(&pMgr->m_Unknown6c4, 4);
		pStream->Read(&pMgr->m_Unknown6bc, 4);
		pStream->Read(&pMgr->m_Unknown6c0, 4);
		pStream->Read(&pMgr->m_Unknown6d0, 4);
		pStream->Read(&pMgr->m_Unknown6c8, 4);
		pStream->Read(&pMgr->m_Time, 4);
		pStream->Read(&pMgr->m_Unknown6b8, 4);
	}
	else
	{
		pStream->Read(&nSkip1, 4);
		pStream->Read(&nSkip2, 4);
		pStream->Read(&nSkip3, 4);
		pStream->Read(&nSkip4, 4);
		pStream->Read(&nSkip5, 4);
		pStream->Read(&nSkip6, 4);
		pStream->Read(&nSkip7, 4);
	}

	pStream->Read(&nTrees, 4);

	if (nTrees != (uint32)-1)
	{
		uint32 iTree;

		if (nTrees != pMgr->m_nFileTrees)
		{
			RETURN_ERROR(1, "sm_RestoreObjects", LT_NOTINITIALIZED);
		}
		else
		{
			for (iTree = 0; iTree < nTrees; iTree++)
			{
				SavWorldBsp* pBsp;
				uint32       iChunk;

				pStream->Read(&nChunks, 4);

				pBsp = pMgr->m_pFileTrees[iTree]->m_pWorldBsp;

				if (nChunks != pBsp->m_nChunks)
					RETURN_ERROR(1, "sm_RestoreObjects", LT_NOTINITIALIZED);

				for (iChunk = 0; iChunk < nChunks; iChunk++)
					pStream->Read(&pBsp->m_pChunks[iChunk].m_n06, 2);
			}
		}
	}

	pMgr->m_Unknown6b8 =
		pMgr->m_Unknown6c0 - (time_GetTime() + pMgr->m_Unknown6b8);

	if (nVersion != g_nSaveVersion)
		RETURN_ERROR(1, "sm_RestoreObjects", LT_INVALIDVERSION);

	sm_Sub34688(pMgr);

	pStream->Read(&bMore, 1);

	while (bMore)
	{
		pStream->ReadString(szName, sizeof(szName));
		pStream->ReadString(szValue, sizeof(szValue));

		if (pStream->ErrorStatus())
			RETURN_ERROR(1, "RestoreObjects", LT_INVALIDFILE);

		sprintf(szCommand, "%s %s", szName, szValue);

		con_RunString3((ConsoleState*)&pMgr->m_ConsoleState, szCommand, 2);

		pStream->Read(&bMore, 1);
	}

	// The stream position is in a block of its own so that it shares a frame
	// slot with the chunk loop's index above, which is what the shipped
	// `-0x8(%ebp)` does; at function scope it gets a slot of its own and the
	// frame is four bytes over. The block is left unindented rather than
	// re-flowing eighty lines for one declaration.
	{
	uint32 nStreamPos;

	pStream->GetPos(&nStreamPos);

	ppObjects = (SavObject**)dalloc_z(nObjects * sizeof(SavObject*));

	pStream->SeekTo(nStreamPos);

	dResult = sav_RestoreObject((CServerMgr*)pMgr, pStream,
	                            (LTObject**)ppObjects, dwParam);

	if (!dResult)
	{
		SavObject** ppObject = ppObjects + 1;

		do
		{
			dResult = sav_RestoreObject((CServerMgr*)pMgr, pStream,
			                            (LTObject**)ppObject++, dwParam);
		}
		while (!dResult);
	}

	if (dResult == SAV_DONE)
	{
		int i;

		un390_ClearSaveIndices(pMgr->m_Unknown390);

		for (i = 0; i < nObjects; i++)
			ppObjects[i]->m_nSaveIndex = (uint16)i;

		sm_Sub34688(pMgr);

		pStream->SeekTo(nStreamPos);

		g_iRestoreObject = 0;

		while (!(dResult = sm_RestoreNextObject(
		             pMgr, pStream, ppObjects[g_iRestoreObject++], dwParam)))
			;

		if (dResult == SAV_DONE)
		{
			for (i = 0; i < nObjects; i++)
			{
				dResult = RestoreAttachments(pMgr, ppObjects[i], pStream,
				                                 ppObjects, nObjects);

				if (dResult)
					break;

				dResult = RestoreInterlinks(pMgr, ppObjects[i], pStream,
				                                 ppObjects, nObjects);

				if (dResult)
					break;
			}
		}
	}

	}

	dfree(ppObjects);

	if (dResult && dResult != SAV_DONE)
		return dResult;

	sm_Sub34688(pMgr);

	return LT_OK;
}


// 0x10025984 and 0x10013b63 — the two record makers the restore drives
// (smgr.cpp and the object module). The first hangs an attachment off the
// parent's list at +0x98, filling in the parent's own runtime id beside the
// child's; the second joins two objects with a relation record and refuses
// to join an object to itself. Both take the manager, or in the first case
// its 0x390 sub-object, as their first argument (names ours).
LTRESULT un390_CreateAttachment(void* p390, SavObject* pParent, uint16 nChildID,
                                uint32 nFlags, float* pOffset, float* pRotation,
                                void* pUnused);
LTRESULT sm_CreateInterlink(SavServerMgr* pMgr, SavObject* pObject1,
                            SavObject* pObject2, uint32 nType);


// 0x10010f76 — read one object's attachments back, in the shape
// sav_SaveObjectLinks wrote them: a two-byte count, then per entry an
// offset, a rotation, the attached object's index in the save and the flag
// word. The index is looked up in the array the restore built, and the
// attachment is remade through the runtime id that object now has -- save
// indices do not survive the load, runtime ids are what the engine uses.
//
// An index past the end of the array is the file being wrong about itself
// and fails the whole restore. The loop counter is sixteen bits wide, which
// is what the word-sized compares against the count say.
LTRESULT RestoreAttachments(SavServerMgr* pMgr, SavObject* pObject,
                            ILTStream* pStream, SavObject** ppObjects,
                            int nObjects)
{
	uint16 nAttachments;
	uint16 nIndex;
	uint32 nFlags;
	float  vOffset[3];
	float  rRotation[4];
	uint16 i;

	pStream->Read(&nAttachments, 2);

	for (i = 0; i < nAttachments; i++)
	{
		pStream->Read(vOffset, 0x0c);
		pStream->Read(rRotation, 0x10);
		pStream->Read(&nIndex, 2);
		pStream->Read(&nFlags, 4);

		if (nIndex >= nObjects)
			RETURN_ERROR(1, "RestoreAttachments", LT_ERROR)

		un390_CreateAttachment(pMgr->m_Unknown390, pObject,
		                       ppObjects[nIndex]->m_nObjectID, nFlags,
		                       vOffset, rRotation, LTNULL);
	}

	return LT_OK;
}

// 0x10011042 — and the relations, which are a kind byte and an index each.
// Unlike the attachments these are remade from the object pointer rather
// than its id, because the record holds pointers.
LTRESULT RestoreInterlinks(SavServerMgr* pMgr, SavObject* pObject,
                           ILTStream* pStream, SavObject** ppObjects,
                           int nObjects)
{
	uint16 nInterlinks;
	uint16 nIndex;
	uint8  nType;
	uint16 i;

	pStream->Read(&nInterlinks, 2);

	for (i = 0; i < nInterlinks; i++)
	{
		pStream->Read(&nType, 1);
		pStream->Read(&nIndex, 2);

		if (nIndex >= nObjects)
			RETURN_ERROR(1, "RestoreInterlinks", LT_ERROR)

		sm_CreateInterlink(pMgr, pObject, ppObjects[nIndex], nType);
	}

	return LT_OK;
}


// 0x100110dc — a vector out to the stream, one component at a time rather
// than as twelve bytes in one call. sav_SaveObject uses it twice; every
// other vector in the file goes out whole, so the difference is the
// original's, not a pattern (name ours).
void sav_WriteVector(ILTStream* pStream, float* pVec)
{
	pStream->Write(&pVec[0], 4);
	pStream->Write(&pVec[1], 4);
	pStream->Write(&pVec[2], 4);
}


// ---- the save translation table (0x1001110d-0x10011171) ---------------- //
//
// The server half of lithtech's warpblit.cpp PART D. One static object whose
// constructor fills an eight-byte table; the compiler emits the quartet
// 0x1001110d (__xc entry), 0x10011117 (??__E, `jmp 0x10011137`), 0x10011121
// (the atexit registration) and 0x1001112d (the ??__F destructor thunk,
// `mov $0x100585b4,%ecx ; jmp 0x10036d21`, and 0x10036d21 is a bare `ret`
// the linker folded), then the constructor at 0x10011137. It sits between
// sav_WriteVector (0x100110dc) and genericprop.cpp's gp_Clear, which is what
// puts it in this file.
//
// The object is 0x100585b4 and the eight bytes are at 0x100585b8, the same
// +4 lithtech's twin (0x004335d0, object 0x004b3394) has; the stores are at
// the same eight offsets in the same order with the same values. Nothing in
// either image reads the table, so the names are placeholders.

// 0x100585b8, eight bytes -- written once, never read.
static uint8 g_SaveXlat[8];

class CSaveXlatInit
{
public:
	CSaveXlatInit();

	~CSaveXlatInit();
};

// 0x10011137, 59 bytes. Out of class so it is a real COMDAT, which is what
// the `jmp 0x10011137` in ??__E needs. /Os spells the zero store `andb $0`
// and hoists it above `mov %ecx,%eax`; both are the flag's, not the source's.
CSaveXlatInit::CSaveXlatInit()
{
	g_SaveXlat[7] = 0;                       // 0x10011137
	g_SaveXlat[6] = 1;                       // 0x10011140
	g_SaveXlat[2] = 2;                       // 0x10011147
	g_SaveXlat[3] = 3;                       // 0x1001114e
	g_SaveXlat[1] = 4;                       // 0x10011155
	g_SaveXlat[0] = 5;                       // 0x1001115c
	g_SaveXlat[4] = 6;                       // 0x10011163
	g_SaveXlat[5] = 7;                       // 0x1001116a
}

// Empty and user-declared: the atexit registration at 0x10011121 proves it
// exists, and the thunk 0x1001112d reaching it by `jmp 0x10036d21` (a bare
// `ret` shared by every empty body in the image) proves it is OUT OF LINE --
// an in-class `{}` is expanded into the thunk, which is then a lone `ret`.
CSaveXlatInit::~CSaveXlatInit()
{
}

// 0x100585b4 -- the object whose presence makes the compiler emit the
// quartet at 0x1001110d..0x1001112d.
static CSaveXlatInit g_SaveXlatInit;


// The object the engine hands MID_LOADOBJECT. Its vtable (0x1004d660) puts
// CSaveIO::ReadChecked in slot 8, so in the original this and CSaveIO are one
// class; only the three guarded accessors above are compiled into this file
// and the other eleven slots (0x100145d2-0x10014990) into the message module,
// so the rest of the class is not ours to declare. What the restore needs is
// the layout the shipped code builds on the stack: a vtable pointer, the
// class manager the loader resolves names against, and the stream (names
// ours).
class CSaveLoadMessage
{
public:
	// The stream is stored before the class manager: the order decides
	// which dead parameter slot sm_RestoreNextObject (0x10010e13) homes each
	// offset in, and this is the order the server build has (savrest.cpp's
	// CSaveLoadMessageQ4S is the same object, in the same order).
	CSaveLoadMessage(void* pClassMgr, ILTStream* pStream)
	{
		m_pStream = pStream;
		m_pClassMgr = pClassMgr;
	}

	virtual LTRESULT _v00();

	void*      m_pClassMgr;                  // 0x04
	ILTStream* m_pStream;                     // 0x08
};


// 0x10010e13 — restore one object's own data, which is the second of the
// three passes over the object run and the only one that gives the object
// itself a say: everything here is bookkeeping around one MID_LOADOBJECT.
//
// The record on the stream is the offset its data ends at, the offset it
// starts at, the object's name, and then the data. An end offset of all-ones
// is the run's terminator and answers LT_FINISHED, which is what stops the
// caller's loop. The start offset is seeked to rather than assumed, so an
// object whose name was written longer than the reader expects still lands
// in the right place.
//
// After the object has read itself the guard word has to be exactly where it
// was written. It not being there means the object read a different number
// of bytes than it wrote, and the four reports that follow say by how much
// and in which direction -- the only diagnostic in the save system that
// names the object. The stream is left wherever the object left it, so the
// whole restore fails from here.
//
// Three of the four parameter slots are dead once their values are in
// registers, and the three offsets live in them: the end offset in the
// stream's slot, the start offset in the manager's, the position in the
// object's.
//
// The position is taken with ILTStream's inline `GetPos()` helper (the one
// that returns the offset), not the virtual `GetPos(&x)`: the helper's own
// local is the address-taken slot and its return is copied into the
// register the reports use. Written with the virtual call and a copy, the
// copy loads into eax and moves to esi (the reverse of shipped
// 0x10010ef6) and the lithtech twin comes out a byte over; with the helper
// the lithtech build (0x4331f0) is byte-identical.
//
// Byte-identical on both images. The server build homes the end offset in
// the stream's dead slot (0xc) and the start offset in the manager's (0x8)
// only when CSaveLoadMessage's constructor stores the stream before the
// class manager (see the class); in the other order the two swap slots
// (nine displacements, 0x10010e24..0x10010f68). Declaration order, names,
// types, block scope, the `>>` stream operators, an inline helper for the
// terminator test and the if/else spellings of the UNDER/OVER report were
// all measured before and moved nothing.
LTRESULT sm_RestoreNextObject(SavServerMgr* pMgr, ILTStream* pStream,
                              SavObject* pObject, uint32 dwParam)
{
	uint32 nSentinel;
	uint32 nEnd;
	uint32 nStart;
	char   szName[0x100];

	CSaveLoadMessage msg(pMgr->m_pClassMgr, pStream);

	pStream->Read(&nEnd, 4);

	if (nEnd == 0xffffffff)
		return LT_FINISHED;

	pStream->Read(&nStart, 4);
	pStream->ReadString(szName, sizeof(szName));
	pStream->SeekTo(nStart);

	pObject->m_pSData->m_pObject->EngineMessageFn(SAV_MID_LOADOBJECT, &msg,
	                                              (float)dwParam);

	pStream->Read(&nSentinel, 4);

	if (nSentinel != SAVE_SENTINEL)
	{
		uint32 nPos;

		nPos = pStream->GetPos();

		dsi_ServerError("LOAD ERROR: Couldn't restore object (%d of %d) '%s'!!!",
		                g_iRestoreObject, g_nRestoreObjects, szName);
		dsi_ServerError("  Object saved %d bytes", nEnd - nStart);

		dsi_ServerError("  Object tried to load %d bytes!", nPos - nStart);

		if (nPos < nEnd)
			dsi_ServerError("  Restore UNDER wrote %d bytes!", nEnd - nPos);
		else
			dsi_ServerError("  Restore OVER wrote %d bytes!", nPos - nEnd);

		RETURN_ERROR(1, "sm_RestoreNextObject", LT_INVALIDFILE)
	}

	sm_Sub34688(pMgr);

	pStream->SeekTo(nEnd);

	return LT_OK;
}
