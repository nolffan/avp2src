// server.dll — the object save file: the guard words CSaveIO puts around
// every object's data, and the two ends of the block itself (reconstruction).
// Build /O1.
#ifdef PRIMAL_HUNT
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x43eaf0), in
// server.dll too (0x100158a6, after sav_WriteVector; guard 0x1008ec84).
#include <string>
#endif
#include <string.h>
#include <stdio.h>
#include "de_error.h"
#include "saveio.h"

// ltserverobj.h's CF_ALWAYSLOAD: objects of the class come out of the level,
// so they are not written to the save at all.
#define CF_ALWAYSLOAD (1 << 3)

// No image has this constructor: nothing constructs a CSaveIO (the real
// object, CSaveLoadMessage below, is built by its users), and the 22-byte
// body it was paired with in every engine image is the world intersect
// query record's (see saveio.h). Kept for the record, not built.
#if 0
CSaveIO::CSaveIO()
{
	m_n00 = 0;
	m_n04 = 0;
	m_pStream = LTNULL;
	m_n14 = 0;
	m_n0c = 0;
	m_n10 = 0;
}
#endif

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
	uint8       m_ObjectType;                // 0xbc  OT_*
	uint8       m_pad_bd[0x190 - 0xbd];
	SavObjData* m_pSData;                    // 0x190
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
	// AVP2: the object manager is four bytes further in than NOLF's 0x390
	// (servermgr.h's m_Unknown390): every AvP2 image passes 0x394 (lithtech
	// 0x0043cb4b `lea 0x394(%edi)`).
	uint8            m_pad_338[0x394 - 0x338];
	uint8            m_Unknown390[4];        // 0x394  the object manager
	uint8            m_pad_398[0xac4 - 0x398];
	// servermgr.h's types: five of the seven are floats and the arithmetic
	// at the end of the restore only works out if 0x6b8 and 0x6c0 are.
	float            m_Unknown6b8;           // 0xac4
	float            m_Unknown6bc;           // 0xac8
	float            m_Unknown6c0;           // 0xacc
	uint32           m_Unknown6c4;           // 0xad0
	float            m_Unknown6c8;           // 0xad4
	float            m_Time;                 // 0xad8
	int              m_Unknown6d0;           // 0xadc
	uint8            m_pad_ae0[0xc14 - 0xae0];
	// What the loader resolves saved class names against; the class-manager
	// list head two dwords further on is a different thing and the save
	// filter uses that one (names ours).
	void*            m_pClassMgr;            // 0xc14
	uint8            m_pad_808[4];
	void*            m_ClassMgrFirst;        // 0xc1c
	uint8            m_pad_c20[0xd38 - 0xc20];
	// servermgr.h's m_pUnknown928: the server interface (an ILTCSBase), of
	// which only its ILTModel pointer at +0x10 is reached here.
	class SavServerLTQ535* m_pServerLT;      // 0xd38
	uint8            m_pad_d3c[0xe84 - 0xd3c];
	SavConsoleState  m_ConsoleState;         // 0xe84
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
//
// server.dll's only: lithtech.exe's sav_SaveObject is savobj.cpp's, which
// expands its own copy, and neither engine image keeps one out of line
// (Primal Hunt's saveio unit goes RestoreInterlinks, its <string> guard,
// CSaveXlatInit: 0x0043ea20..0x0043eb60).
#ifdef LT_SERVER_BUILD
void sav_WriteVector(ILTStream* pStream, float* pVec)
{
	pStream->Write(&pVec[0], 4);
	pStream->Write(&pVec[1], 4);
	pStream->Write(&pVec[2], 4);
}
#endif


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


// What sm_RestoreNextObject restores of a model before the object's own
// data (lithtech.exe 0x0043e7b3..0x0043e7e7, server.dll 0x10015570; AvP2
// 1.0.9.6's server.dll 0x10014ae9): its hidden-piece bits and its main
// animation tracker. The model's layout is objbank.cpp's CObjModel (tracker
// 0x1d4, hidden pieces 0x260; AvP2 0x1d0 and 0x24c).
#define SAV_OT_MODEL 1

class SavModelQ535
{
public:
#ifdef PRIMAL_HUNT
	uint8  m_pad_00[0x1d4];
	uint8  m_AnimTracker[0x50];              // 0x1d4  the SDK's LTAnimTracker
	uint8  m_pad_224[0x260 - 0x224];
#else
	uint8  m_pad_00[0x1d0];
	uint8  m_AnimTracker[0x50];              // 0x1d0
	uint8  m_pad_220[0x24c - 0x220];
#endif
	uint32 m_HiddenPieces;                   // 0x260 (AvP2 0x24c)
};

// ILTModel (iltmodel.h) up to ReadTracker, its slot 16 (`call *0x40(%edx)`
// at 0x0043e7e7), which reads a tracker back out of a message.
class SavModelLTQ535
{
public:
	virtual LTRESULT _v00() = 0;
	virtual LTRESULT _v04() = 0;
	virtual LTRESULT _v08() = 0;
	virtual LTRESULT _v0c() = 0;
	virtual LTRESULT _v10() = 0;
	virtual LTRESULT _v14() = 0;
	virtual LTRESULT _v18() = 0;
	virtual LTRESULT _v1c() = 0;
	virtual LTRESULT _v20() = 0;
	virtual LTRESULT _v24() = 0;
	virtual LTRESULT _v28() = 0;
	virtual LTRESULT _v2c() = 0;
	virtual LTRESULT _v30() = 0;
	virtual LTRESULT _v34() = 0;
	virtual LTRESULT _v38() = 0;
	virtual LTRESULT _v3c() = 0;
	virtual LTRESULT ReadTracker(void* pTracker, void* hRead) = 0;
};

// ILTCSBase's members (iltcsbase.h): vptr, ILTMath, common, physics, model.
class SavServerLTQ535
{
public:
	uint8           m_pad_00[0x10];
	SavModelLTQ535* m_pModelLT;              // 0x10
};

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

	if (pObject->m_ObjectType == SAV_OT_MODEL)
	{
		SavModelQ535* pModel = (SavModelQ535*)pObject;

		pStream->Read(&pModel->m_HiddenPieces, 4);

		if (pMgr->m_pServerLT->m_pModelLT)
			pMgr->m_pServerLT->m_pModelLT->ReadTracker(&pModel->m_AnimTracker, &msg);
	}

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

#ifdef LT_SERVER_BUILD
// ======================================================================== //
//  sav_SaveObject and sav_RestoreObject: retail's are this module's, in
//  both server.dll images (Primal Hunt 0x100141be and 0x10014c75, between
//  sav_SaveObjects and CSaveIO's members and after sm_RestoreObjects;
//  AvP2 1.0.9.6 0x100137bf and 0x10014223). lithtech.exe's /O2 copies are
//  src/engine/lithtech/savobj.cpp's. Written apart first (as savrest.cpp),
//  with views of their own (the Q4S names) not yet folded into the ones
//  above.
// ======================================================================== //
//  server.dll /O1 — the two halves of one object's save record:
//
//      sav_SaveObject      0x1000fc39   1111 bytes   (enumeration correct)
//      sav_RestoreObject   0x100106b6   1779 bytes   (enumeration correct;
//                                                     savobj.cpp's 1885 is
//                                                     WRONG — see below)
//
//  BOTH BELONG IN src/engine/server/saveio.cpp, which declares them at lines
//  282 and 614 and defines neither.  Ground truth for every byte cited here
//  is the retail server.dll, disassembled `objdump -M intel`.  The /O2
//  twin of this source is src/engine/lithtech/savobj.cpp (lithtech.exe
//  0x00431ac0 / 0x00432740); where a decision differs between the two files
//  this one says so and says which image settled it.
//
// ------------------------------------------------------------------------ //
//  EXTENT RESOLUTION — 0x100106b6 IS 1779 BYTES, NOT 1885.
// ------------------------------------------------------------------------ //
//
//  The function enumeration says 1779; src/engine/lithtech/savobj.cpp:25 says
//  1885.  1885 - 1779 = 106, and the three enumeration rows that follow
//  0x100106b6 are 0x10010da9 (45) + 0x10010dd6 (45) + 0x10010e03 (16) = 106
//  exactly.  The 1885 figure swallows all three.
//
//  The enumeration is right and savobj.cpp is wrong, on four independent
//  grounds:
//
//   1. THE FUNCTION ENDS AT 0x10010da8.  0x10010da4..0x10010da8 is
//      `pop edi ; pop esi ; pop ebx ; leave ; ret` — a complete stdcall-free
//      cdecl epilogue that undoes exactly the `push ebp ; mov ebp,esp ;
//      sub esp,0x824 ; push ebx ; push esi ; push edi` prologue at
//      0x100106b6..0x100106c1.  The next byte, 0x10010da9, is
//      `mov ax,WORD PTR [esp+0x4]` — a fresh function reading its first
//      stack argument, and it closes with `ret 0x8` at 0x10010e00, a
//      THISCALL two-argument return that the ebp frame above could not
//      produce.
//
//   2. sav_RestoreObject CALLS 0x10010da9 AND 0x10010e03.  A function cannot
//      contain its own callees: 0x10010a26 is `call 0x10010da9`
//      (ServerMessage::Init, servermsg.h) and 0x100106de is
//      `call 0x10010e03` (SObjData's constructor — savobj.cpp:484 itself
//      names both as separate server.dll bodies while its header comment
//      absorbs them into the extent).
//
//   3. 0x10010e03 HAS A SECOND CALLER OUTSIDE THE RANGE.  savobj.cpp:489
//      records smgr.cpp's SObjData bank at 0x1002ce15 calling it.  An
//      interior block of sav_RestoreObject cannot be called from smgr.cpp.
//
//   4. NO e8/e9/0f-8x DISPLACEMENT CROSSES 0x10010da8.  Every branch inside
//      0x100106b6..0x10010da8 lands inside it (the furthest are
//      0x10010783 -> 0x10010b8e and 0x1001072c -> 0x10010d97, both the
//      shared-tail jumps described below); nothing in the range targets
//      0x10010da9+, and nothing outside the range targets an interior
//      address.  0x10010dd6 (the third absorbed row) is likewise a separate
//      thiscall body: 0x10010df7 is its `jbe` join point, reachable only
//      from 0x10010dd2 inside it.
//
//  So the row is sound as enumerated and savobj.cpp's header comment needs a
//  one-line correction.  (savobj.cpp's OTHER extent figures are fine: the
//  lithtech twins 0x00431ac0/1242 and 0x00432740/2730 both check out.)
//
// ------------------------------------------------------------------------ //
//  WHAT /O1 DOES DIFFERENTLY FROM THE /O2 TWIN  (all of it automatic —
//  the SOURCE is the same source; only /Os's inline refusals differ)
// ------------------------------------------------------------------------ //
//
//    construct                    /O2 lithtech      /O1 server.dll
//    ---------------------------  ----------------  ----------------------
//    sav_WriteVector              expanded x2       call 0x100110dc x2
//    ObjectCreateStruct::Clear    expanded x2       call 0x10001bd7 x2
//    w_ReadVector                 expanded x2       call 0x1000d040 x2
//    sm_AllocateObject            expanded          call 0x10001c54
//    CMessageRef::Init            expanded          call 0x1000ebab
//    ServerMessage::Init          expanded          call 0x10010da9
//    SObjData::SObjData           expanded          call 0x10010e03
//    3.0f as a call argument      push 0x40400000   fld [0x1004d6f0]/fstp
//
//  So NOTHING in this file may be marked `inline`, and savobj.cpp's
//  unsettled item 1 ("sm_AllocateObject has to become `inline`") is a
//  lithtech-only need that must NOT be satisfied by moving a body into a
//  header this file depends on without measuring server.dll.
//
//  Still expanded at /O1, so these DO have to be visible:
//    * ObjectCreateStruct's constructor (ltbasedefs.h, in-class; it expands
//      to the single `call Clear` at 0x100106c8)
//    * CMessageRef::~CMessageRef (servermsg.h, in-class) — 0x1001071d,
//      0x10010975, 0x10010bc5 and 0x10010d89 each carry
//      `mov ecx,[ebp-0x64] ; test ecx,ecx ; je .. ; mov eax,[ecx] ;
//       call [eax+0x8]`
//    * dl_AddHead (ltlink.h, inline) — 0x10010d5a..0x10010d76
//
// ------------------------------------------------------------------------ //
//  CORRECTION TO savobj.cpp THAT AFFECTS BOTH IMAGES
// ------------------------------------------------------------------------ //
//
//  savobj.cpp passes 1.0f to MID_PRECREATE and to sm_CreateObjectInWorld.
//  BOTH ARE 3.0f.  server.dll loads 0x1004d6f0, whose four bytes are
//  00 00 40 40 = 0x40400000 = 3.0f (0x10010925 and 0x10010943), and
//  lithtech.exe pushes the immediate 0x40400000 directly at 0x00432bd3 and
//  0x00432be7.  3 is ltserverobj.h's PRECREATE_SAVEGAME, which is what a
//  restore should be passing.  As written, savobj.cpp cannot match either
//  image on those two instructions.
//
// ------------------------------------------------------------------------ //
//  FRAME ACCOUNTS (both close exactly, which is what pins the object sizes)
// ------------------------------------------------------------------------ //
//
//  sav_SaveObject   `sub esp,0x2c` (0x1000fc3c), ebp-relative:
//      ebp-0x2c  CSaveLoadMessage msg   0x0c   (vptr/0x1004d660, +4, +8)
//      ebp-0x20  nEndFieldPos           0x04
//      ebp-0x18  nStartFieldPos         0x04
//      ebp-0x14  nAnim                  0x04
//      ebp-0x10  nSkyIndex              0x04
//      ebp-0x0c  the skin loop's counter
//      ebp-0x08  nValue                 0x04
//      ebp-0x04  spare
//    plus THREE reused dead parameter slots, which is pure /O1 allocation:
//    ebp+0x0c (pObj) holds the skin-loop byte offset and later nPos;
//    ebp+0x13 (a byte inside the pStream slot) holds bHasSaveData;
//    ebp-0x01 holds bHasClient.
//
//  sav_RestoreObject  `sub esp,0x824` (0x100106b9), ebp-relative.  Same
//  seven objects as the /O2 twin, in the OPPOSITE order, and the same total:
//      ebp-0x824  char szClientName[0x200]        0x200
//      ebp-0x624  char szClassName[0x100]         0x100
//      ebp-0x524  ObjectCreateStruct createStruct 0x284
//      ebp-0x2a0  CObjBase tempObject             0x1a8
//      ebp-0x0f8  WMMoveState moveState           0x068
//      ebp-0x090  SObjData sData                  0x054
//      ebp-0x03c  scalars                         0x03c   = 0x824
//    scalars, read off their uses:
//      ebp-0x3c nStart      ebp-0x38 vTemp[3]    ebp-0x2c nAnimTime
//      ebp-0x28 nClientFlags ebp-0x24 nAnim      ebp-0x20 nEnd
//      ebp-0x1c fLightRadius ebp-0x18 pClassDef  ebp-0x14 nSkyIndex
//      ebp-0x10 nDataLen    ebp-0x0c nChangeFlags
//      ebp-0x06 bHasSaveData ebp-0x05 bHasClient ebp-0x04 pNewObject
//    and ebp+0x0c (the pStream parameter slot) is reused twice, for the skin
//    loop's countdown and for msg_AllocPacket's returned temporary.
//
// ------------------------------------------------------------------------ //
//  Build /O1 (/Og /Os /Oy /Ob1).  Symbols introduced here carry a Q4S
//  suffix.  MERGE NOTE: delete the duplicated views, fold the added fields
//  into saveio.cpp's copies, and DROP THE TWO FORWARD DECLARATIONS at
//  saveio.cpp:282 and 614 — and note that saveio.cpp:614 spells the restore
//  `(SavServerMgr*, ILTStream*, SavObject**, uint32)` while the definition
//  below needs the real CServerMgr/LTObject, so the declaration has to go
//  rather than be kept alongside (it would overload, and sm_RestoreObjects
//  would bind to the wrong one).
// ======================================================================== //
#include <string.h>
#include <stdio.h>
#include "de_error.h"
#include "saveio.h"

// ======================================================================== //
//  PART A — views for sav_SaveObject.
//
//  Every one of these is a verbatim copy of saveio.cpp's own (or, where
//  saveio.cpp's stops short, an extension of it at the offsets the shipped
//  bytes reach).  DELETE ON MERGE.
// ======================================================================== //

// ltserverobj.h's ClassDef.  0x1000fc9e pushes [ecx] where ecx is the class
// record, so the name is at +0x00.
struct SavClassDefQ4S
{
	char*            m_ClassName;            // 0x00
	SavClassDefQ4S*  m_ParentClass;          // 0x04
	uint32           m_ClassFlags;           // 0x08
};

// An LTLink as ltlink.h lays it out.
struct SavLinkQ4S
{
	SavLinkQ4S* m_pPrev;                     // 0x00
	SavLinkQ4S* m_pNext;                     // 0x04
	void*       m_pData;                     // 0x08
};

// ltserverobj.h's BaseClass.  Slot 1 of its vtable is EngineMessageFn:
// 0x1001003b loads [ecx] and 0x10010044 calls [eax+0x4].
class SavBaseClassQ4S
{
public:
	virtual ~SavBaseClassQ4S();
	virtual uint32 EngineMessageFn(uint32 messageID, void* pData, float fData);
};

// ltserverobj.h's message ids.  0x10010042 pushes 8, where the already-
// reconstructed sm_RestoreNextObject pushes 7 for MID_LOADOBJECT.
#define SAV_MID_SAVEOBJECT 8

// servermgr.h's Client, at the two fields the record carries: 0x1000ffcf
// reaches +0x3c0 (four bytes) and 0x1000ffe6 pushes +0x3c4 as a string.
class SavClientQ4S
{
public:
	uint8  m_pad_000[0x3c0];
	uint32 m_ClientFlags;                    // 0x3c0  servermgr.h's name
	char*  m_pName;                          // 0x3c4  servermgr.h's name
};

// What SObjData's +0x2c really points at.  The restore half below settles
// it: +0x2c is a CMessageRef and this is its m_pMsg, a ServerMessage.
// 0x1000fe6f reads the byte count at +0x34 (servermsg.h's m_Unknown34) and
// 0x1000fe76 the bytes at +0x54 (m_pData).  ltobject_internal.h carries
// +0x2c as `uint32 m_Unknown2c`, "nonzero keeps the object out of the
// inactive sweep", which is consistent — an object holding save data is not
// idle — but the two dereferences say it is a reference to a message.
class SavMessageQ4S
{
public:
	uint8  m_pad_00[0x34];
	uint16 m_Unknown34;                      // 0x34  servermsg.h's name
	uint8  m_pad_36[0x54 - 0x36];
	uint8* m_pData;                          // 0x54  servermsg.h's name
};

// ltobject_internal.h's SObjData.  saveio.cpp's copy stops at m_pClass; the
// fields from m_hIdentifier down are the ones this function reaches.
//
// m_hFiles has to be an ARRAY: the model arm at 0x1000fd7c starts a byte
// offset at 0x34 in a dead parameter slot and steps it by 4 for two
// iterations (`add [ebp+0xc],ebx ; dec [ebp-0xc] ; jne`), indexing
// [ecx+eax*1] off m_pSData.  ltobject_internal.h names 0x30 m_pModelFile
// and 0x34 m_pSkinFile, which are [0] and [1]; nothing before this reached
// 0x38.  (Unsettled — see the closing notes.)
class SavObjDataQ4S
{
public:
	SavLinkQ4S       m_Links;                // 0x00
	void*            m_hIdentifier;          // 0x0c
	float            m_NextUpdate;           // 0x10
	float            m_DeactivationTime;     // 0x14
	float            m_UpdateTimer;          // 0x18
	SavClientQ4S*    m_pOwningClient;        // 0x1c
	SavBaseClassQ4S* m_pBaseClass;           // 0x20
	SavClassDefQ4S*  m_pClass;               // 0x24
	uint8            m_pad_28[0x2c - 0x28];
	SavMessageQ4S*   m_pSaveData;            // 0x2c  (CMessageRef::m_pMsg)
	// AVP2: the model file and its four skins (ltobject_internal.h's
	// m_pModelFile and m_pSkinFiles), not NOLF's model and one skin.
	void*            m_hFiles[1 + 4];        // 0x30
#ifdef PRIMAL_HUNT
	// PH: the extra texture after the skins, saved with a model (server.dll
	// 0x10014337 pushes +0x44 after the skin loop); ltbasedefs.h's
	// m_ExtraTextureName. Name ours.
	void*            m_hExtraTexture;        // 0x44
#endif
};

// worldbsp.h's world, at the one field the save reads: the name, inline at
// +0x04 (0x1000fd48 is `add ecx,ebx` with ebx == 4).
class SavWorldBspQ4S
{
public:
	uint8 m_pad_00[0x04];
	char  m_Name[0xc4 - 0x04];               // 0x04
};

// ltobject_internal.h's LTObject, at the fields the record carries.
class SavObjectQ4S
{
public:
	uint8            m_pad_000[0x088];
	uint32           m_Flags;                // 0x088
	uint32           m_Flags2;               // 0x08c
	uint32           m_UserFlags;            // 0x090
	uint8            m_ColorR;               // 0x094
	uint8            m_ColorG;               // 0x095
	uint8            m_ColorB;               // 0x096
	uint8            m_ColorA;               // 0x097
	void*            m_pLinks;               // 0x098
	float            m_Rotation[4];          // 0x09c
	float            m_Scale[3];             // 0x0ac
	uint16           m_ObjectID;             // 0x0b8
	uint16           m_nSaveIndex;           // 0x0ba
	char             m_ObjectType;           // 0x0bc  OT_*
	uint8            m_BlockingPriority;     // 0x0bd
	uint8            m_pad_0be[0x0c4 - 0x0be];
	float            m_Velocity[3];          // 0x0c4
	float            m_Acceleration[3];      // 0x0d0
	// AVP2: ltobject_internal.h's m_Unknowndc, saved one byte wide (AvP2
	// 1.0.9.6's server.dll 0x10013a84).
	uint8            m_Unknowndc[4];         // 0x0dc
	float            m_FrictionCoefficient;  // 0x0e0
	float            m_Mass;                 // 0x0e4
	float            m_ForceIgnoreLimitSqr;  // 0x0e8
	uint8            m_pad_0ec[0x124 - 0x0ec];
	uint32           m_ChangeFlags;          // 0x124
	uint8            m_pad_128[0x190 - 0x128];
	SavObjDataQ4S*   m_pSData;               // 0x190
	float            m_Pos[3];               // 0x194
	float            m_Dims[3];              // 0x1a0
	float            m_Radius;               // 0x1ac
	union {
		SavWorldBspQ4S* m_pWorldBsp;         // 0x1b0
		float           m_LightRadius;       // 0x1b0  (OT_LIGHT only)
	};
	uint8            m_pad_1b4[0x1e0 - 0x1b4];
	uint16           m_Model1d8;             // 0x1e0  (name ours)
	uint8            m_pad_1e2[0x1f0 - 0x1e2];
	uint16           m_Model1e8;             // 0x1f0  (name ours)
	uint8            m_pad_1f2[0x1f4 - 0x1f2];
	uint32           m_Model1ec;             // 0x1f4  (name ours)
	uint8            m_pad_1f8[0x278 - 0x1f8];
	uint16           m_Container270;         // 0x278  (name ours)
};

// servermgr.h's CFileMgr, embedded in the manager at 0xa2c (0x1000fd5e
// `add eax,0xa2c` and the two like it).
class SavFileMgrQ4S
{
public:
	uint8 m_pad_00[0x48];
};

// servermgr.h's CServerMgr, at the three offsets this reaches.  0x804 is
// what saveio.cpp calls m_pClassMgr and servermgr.h calls m_Unknown804; the
// save hands it to the CSaveLoadMessage (0x1000fc4b) exactly as
// sm_RestoreNextObject does (0x10010e29).  m_SkyObjects is servermgr.h's
// name for the thirty ids at 0x95c (0x1000ff3c adds 0x95c and 0x1000ff4f
// compares the index against 0x1e).
// AVP2 1.0.9.6: the model's layout as objbank.cpp's CObjModel (tracker
// 0x1d0, hidden pieces 0x24c), and the server interface's ILTModel up to
// WriteTracker, its slot 17 (`call *0x44(%eax)` at 0x10013bcf).
// PH: Primal Hunt's model is four bytes longer ahead of the tracker
// (0x1d4, server.dll 0x100145d0) and has the hidden pieces at 0x260
// (0x100145ad).
class SavModelSaveQ4S
{
public:
#ifdef PRIMAL_HUNT
	uint8  m_pad_00[0x1d4];
	uint8  m_AnimTracker[0x50];              // 0x1d4
	uint8  m_pad_224[0x260 - 0x224];
	uint32 m_HiddenPieces;                   // 0x260
#else
	uint8  m_pad_00[0x1d0];
	uint8  m_AnimTracker[0x50];              // 0x1d0
	uint8  m_pad_220[0x24c - 0x220];
	uint32 m_HiddenPieces;                   // 0x24c
#endif
};

class SavModelLTSaveQ4S
{
public:
	virtual LTRESULT _v00() = 0; virtual LTRESULT _v04() = 0;
	virtual LTRESULT _v08() = 0; virtual LTRESULT _v0c() = 0;
	virtual LTRESULT _v10() = 0; virtual LTRESULT _v14() = 0;
	virtual LTRESULT _v18() = 0; virtual LTRESULT _v1c() = 0;
	virtual LTRESULT _v20() = 0; virtual LTRESULT _v24() = 0;
	virtual LTRESULT _v28() = 0; virtual LTRESULT _v2c() = 0;
	virtual LTRESULT _v30() = 0; virtual LTRESULT _v34() = 0;
	virtual LTRESULT _v38() = 0; virtual LTRESULT _v3c() = 0;
	virtual LTRESULT _v40() = 0;
	virtual LTRESULT WriteTracker(void* pTracker, void* hWrite) = 0;
};

class SavServerLTSaveQ4S
{
public:
	uint8               m_pad_00[0x10];
	SavModelLTSaveQ4S*  m_pModelLT;          // 0x10
};

class SavServerMgrQ4S
{
public:
	uint8         m_pad_000[0xc14];
	void*         m_pClassMgr;               // 0xc14
	uint8         m_pad_c18[0xd38 - 0xc18];
	SavServerLTSaveQ4S* m_pServerLT;         // 0xd38  (servermgr.h's m_pUnknown928)
	uint8         m_pad_d3c[0xd6c - 0xd3c];
	uint16        m_SkyObjects[30];          // 0xd6c
	uint8         m_pad_da8[0xe3c - 0xda8];
	SavFileMgrQ4S m_FileMgr;                 // 0xe3c
};

// ltbasedefs.h's object types, as the ladders read them.  0x1000fd37 tests
// 2, 0x1000fd73 tests 1, 0x1000fdd9 tests bl (== 4) and 0x1000fdf0 tests 9;
// commonphysics.h:194 already records 2 and 9 as OT_WORLDMODEL and
// OT_CONTAINER.
#define SAV_OT_MODEL      1
#define SAV_OT_WORLDMODEL 2
#define SAV_OT_LIGHT      4
#define SAV_OT_CONTAINER  9

// 0x1000ddfb — dhash.cpp's dhash_GetKey: the key an element was filed
// under.  Called with a null length pointer at 0x1000fd1e.
char* dhash_GetKey(void* hElement, uint32* pLen);

// 0x1002bfcd — servermgr.h's fs_GetFileName.
char* fs_GetFileName(SavFileMgrQ4S* pFileMgr, void* hFile);

// 0x10056ff4 — the shared zero-initialised byte every nameless thing in the
// engine reports (0x1000fd2a loads its address as an immediate).
extern char g_szNoName[];

// 0x100110dc — saveio.cpp already defines this, at line 962.  DECLARED AND
// NOT DEFINED here on purpose: /Os declines the expansion at both call
// sites (0x1000fcdb and 0x1000ff11 are both `call`), so a second definition
// in this TU would only duplicate a COMDAT.
//
// NOTE FOR THE MERGE, not for this file: savobj.cpp:245 argues it is
// `static`, because lithtech.exe expands both sites and emits no
// out-of-line body at all.  That is a lithtech-side question — server.dll
// calls it either way — but if saveio.cpp is made `static` the definition
// has to be hoisted above sav_SaveObject or lithtech loses the expansion
// (VC6's inliner is forward-only).
void sav_WriteVector(ILTStream* pStream, float* pVec);

// saveio.cpp's CSaveLoadMessage (delete on merge).  The vtable it plants
// is 0x1004d660 (0x1000fc5d), the same constant sm_RestoreNextObject
// plants.  The constructor stores the STREAM FIRST: 0x1000fc48 loads
// pStream before 0x1000fc4b loads m_pClassMgr, and with the two
// assignments the other way round the two loads come out swapped (the
// stores themselves are scheduled the same either way).
class CSaveLoadMessageQ4S
{
public:
	CSaveLoadMessageQ4S(void* pClassMgr, ILTStream* pStream)
	{
		m_pStream = pStream;
		m_pClassMgr = pClassMgr;
	}

	virtual LTRESULT _v00();

	void*      m_pClassMgr;                  // 0x04
	ILTStream* m_pStream;                    // 0x08
};


// ======================================================================== //
//  0x1000fc39 — one object's record.  1111 bytes, /O1.
// ======================================================================== //
//
//  The record is two back-patched offsets, the class name, the object's own
//  state, and then whatever the object itself writes in answer to
//  MID_SAVEOBJECT, closed by the 0xABCDEFAF guard word.  The two offsets go
//  out as zeroes first and are filled in once their targets are known,
//  which is why the position of each is captured before it is written
//  (0x1000fc67, 0x1000fc81) and seeked back to afterwards (0x10010009,
//  0x10010072).
//
//  Stream vtable slots, from iltstream.h with its virtual destructor in
//  slot 0: +0x08 Read, +0x0c ReadString, +0x14 SeekTo, +0x18 GetPos,
//  +0x24 Write, +0x28 WriteString.  Every indirect call below checks out
//  against that table.
//
//  Order in the two `if` ladders is the bytes': the world-model arm
//  (0x1000fd37) is tested before the model arm (0x1000fd73), and each has a
//  lone `else` reached by an `eb` over it (0x1000fd50, 0x1000fdb6), so both
//  are if/else and neither is a guard.
//
//  The three types that carry extra state each get their own test against
//  the same byte: OT_LIGHT's radius shares LTObject's world pointer
//  (0x1000fde3 takes the address of 0x1a8 where 0x1000fd40 dereferenced
//  it), OT_CONTAINER has one word at 0x270, OT_MODEL has three.
//
//  Two writes are guarded by a `char` flag that goes into the stream ahead
//  of them, and BOTH RE-READ THE FLAG OUT OF MEMORY after the write
//  (0x1000fe48 `cmp BYTE PTR [ebp+0x13],0x0`, 0x1000ffbd
//  `cmp BYTE PTR [ebp-0x1],0x0`) rather than keeping the pointer they set
//  it from — which is what says the source tests the flag, not the pointer.
//
//  The skin loop at 0x1000fd8a closes with `dec [ebp-0xc] ; jne` against a
//  counter preset to 2 while a separate byte offset counts up from 0x34 by
//  4.  That is VC6's strength reduction of a counting `for` with a known
//  trip count, not a hand-written countdown -- but ONLY WHEN THE INDEX IS A
//  uint16: an `int` index gives `cmp [ebp+0xc],0x3c ; jl` with no second
//  counter.  lithtech.exe's /O2 build emits the same two induction
//  variables (0x00431c2b `mov ebp,0x34` / `mov ebx,0x2` ... `add ebp,0x4 ;
//  dec ebx ; jne`).
//
//  Each file name is fetched into pName as its own statement.  Nested as
//  WriteString's argument, VC6 loads the stream's vtable BEFORE the
//  fs_GetFileName call and has to park it across it, which cost 27 bytes
//  and the `push 4 ; pop ebx` constant register the whole function uses.
//
//  THE FRAME.  0x1001002b..0x10010038 builds the unsigned-to-float qword at
//  [ebp-0x1c], ON TOP OF nStartFieldPos at [ebp-0x18] -- which is only
//  possible if nStartFieldPos is block-scoped and the float conversion is
//  in a sibling block.  So nStartFieldPos lives in a block that runs from
//  its GetPos to its back-patch, and the MID_SAVEOBJECT call and the
//  end-of-record patch are a second block.  With nStartFieldPos at
//  function scope the conversion packs with the skin loop's counter
//  instead (temp at [ebp-0x10], every local from nStartFieldPos to
//  nSkyIndex four bytes off).  lithtech.exe agrees: its temp at
//  0x00431f32 is on nStartFieldPos as well.  Each block's nPos gets the
//  dead pMgr home [ebp+0x8] only as the innermost block's local, hence the
//  inner braces round the first back-patch.
void sav_SaveObject(SavServerMgrQ4S* pMgr, SavObjectQ4S* pObj,
                    ILTStream* pStream, uint32 dwParam)
{
	CSaveLoadMessageQ4S msg(pMgr->m_pClassMgr, pStream);
	uint32 nEndFieldPos;
	uint32 nSkyIndex;
	uint32 nValue;
	char   bHasSaveData;
	char   bHasClient;
	char*  pName;

	nValue = 0;                                          // 0x1000fc42

	// The two back-patched fields, each written as a zero over which the
	// real offset goes later.
	pStream->GetPos(&nEndFieldPos);                      // 0x1000fc67
	pStream->Write(&nValue, 4);                          // 0x1000fc76

	{
		uint32 nStartFieldPos;

		pStream->GetPos(&nStartFieldPos);                // 0x1000fc81
		pStream->Write(&nValue, 4);                      // 0x1000fc8d

		pStream->WriteString(pObj->m_pSData->m_pClass->m_ClassName);   // 0x1000fca2

		pStream->Write(&pObj->m_ObjectType, 1);          // 0x1000fcb2
		pStream->Write(&pObj->m_Flags, 4);               // 0x1000fcc1
		pStream->Write(&pObj->m_Flags2, 4);              // 0x1000fcd0

		sav_WriteVector(pStream, pObj->m_Pos);           // 0x1000fcdb

		pStream->Write(pObj->m_Scale, 0x0c);             // 0x1000fcef
		pStream->Write(pObj->m_Rotation, 0x10);          // 0x1000fcff
		pStream->Write(&pObj->m_UserFlags, 4);           // 0x1000fd0e

		// 0x1000fd11-0x1000fd34: load +0x0c, test, the call arm jumps over
		// the `mov eax,0x10056ff4` arm -- a ternary, not an if/else with
		// two Writes.
		pStream->WriteString(pObj->m_pSData->m_hIdentifier
		                     ? dhash_GetKey(pObj->m_pSData->m_hIdentifier, 0)
		                     : g_szNoName);

		if (pObj->m_ObjectType == SAV_OT_WORLDMODEL)     // 0x1000fd37
		{
			pStream->WriteString(pObj->m_pWorldBsp->m_Name);   // 0x1000fd4d
		}
		else
		{
			pName = fs_GetFileName(&pMgr->m_FileMgr, pObj->m_pSData->m_hFiles[0]);
			pStream->WriteString(pName);                 // 0x1000fd70
		}

		if (pObj->m_ObjectType == SAV_OT_MODEL)          // 0x1000fd73
		{
			uint16 i;

			// AVP2: four skins (AvP2 1.0.9.6's server.dll 0x10013902..
			// 0x10013936, counted down from four).
			for (i = 1; i < 1 + 4; i++)                  // 0x1000fd7c..0x1000fdb4
			{
				pName = fs_GetFileName(&pMgr->m_FileMgr, pObj->m_pSData->m_hFiles[i]);
				pStream->WriteString(pName);
			}
#ifdef PRIMAL_HUNT
			// PH: and the file after them (0x10014337; its WriteString is
			// the else arm's, tail-merged).
			pName = fs_GetFileName(&pMgr->m_FileMgr, pObj->m_pSData->m_hExtraTexture);
			pStream->WriteString(pName);
#endif
		}
		else
		{
			pName = fs_GetFileName(&pMgr->m_FileMgr, pObj->m_pSData->m_hFiles[1]);
			pStream->WriteString(pName);                 // 0x1000fdd6
		}

		if (pObj->m_ObjectType == SAV_OT_LIGHT)          // 0x1000fdd9
			pStream->Write(&pObj->m_LightRadius, 4);     // 0x1000fded

		if (pObj->m_ObjectType == SAV_OT_CONTAINER)      // 0x1000fdf0
			pStream->Write(&pObj->m_Container270, 2);    // 0x1000fe06

		pStream->Write(&pObj->m_pSData->m_NextUpdate, 4);    // 0x1000fe18
		// AVP2 1.0.9.6 (server.dll 0x1001398d..0x100139be) and Primal Hunt
		// (0x100143a5..0x100143c6): the deactivation time and the update
		// timer after it.
		pStream->Write(&pObj->m_pSData->m_DeactivationTime, 4);
		pStream->Write(&pObj->m_pSData->m_UpdateTimer, 4);
		pStream->Write(&pObj->m_BlockingPriority, 1);    // 0x1000fe28

		bHasSaveData = (pObj->m_pSData->m_pSaveData != LTNULL);  // 0x1000fe37 setne

		pStream->Write(&bHasSaveData, 1);                // 0x1000fe45

		if (bHasSaveData)                                // 0x1000fe48
		{
			pStream->Write(&pObj->m_pSData->m_pSaveData->m_Unknown34, 2);   // 0x1000fe61
			pStream->Write(pObj->m_pSData->m_pSaveData->m_pData,
			               pObj->m_pSData->m_pSaveData->m_Unknown34);       // 0x1000fe79
		}

		pStream->Write(&pObj->m_ColorR, 1);              // 0x1000fe89
		pStream->Write(&pObj->m_ColorG, 1);              // 0x1000fe99
		pStream->Write(&pObj->m_ColorB, 1);              // 0x1000fea9
		pStream->Write(&pObj->m_ColorA, 1);              // 0x1000feb9

		pStream->Write(pObj->m_Velocity, 0x0c);          // 0x1000fec9
		pStream->Write(pObj->m_Acceleration, 0x0c);      // 0x1000fed9
		pStream->Write(pObj->m_Unknowndc, 1);
		pStream->Write(&pObj->m_FrictionCoefficient, 4); // 0x1000fee8
		pStream->Write(&pObj->m_Mass, 4);                // 0x1000fef7
		pStream->Write(&pObj->m_ForceIgnoreLimitSqr, 4); // 0x1000ff06

		sav_WriteVector(pStream, pObj->m_Dims);          // 0x1000ff11

		pStream->Write(&pObj->m_ChangeFlags, 4);         // 0x1000ff24

		// Which of the thirty sky slots this object is in.  The walk does
		// NOT stop at the first hit -- the store at 0x1000ff49 is inside
		// the loop and the loop runs to 0x1e regardless -- so the LAST slot
		// naming the object wins, and an object in none of them goes out
		// as slot zero.
		//
		// The index is a dword that is COMPARED sixteen bits wide: it is
		// incremented as a dword (0x1000ff4c `inc edx`), stored into the
		// dword nSkyIndex with no widening (0x1000ff49), and closed by
		// `cmp dx,0x1e ; jb` (0x1000ff4f); lithtech.exe has the same
		// `inc eax ... cmp ax,0x1e` (0x00431e27).  A uint16 index instead
		// is kept in memory with a strength-reduced countdown beside it
		// (+14 bytes); a uint32 compared as a dword is `cmp edx ; jb`.
		// nSkyIndex goes out two bytes wide (0x1000ff5a pushes 2).
		if (pObj->m_ChangeFlags & 0x100)                 // 0x1000ff27 test byte
		{
			uint32 i;

			nSkyIndex = 0;                               // 0x1000ff41

			for (i = 0; (uint16)i < 30; i++)
			{
				if (pMgr->m_SkyObjects[i] == pObj->m_ObjectID)   // 0x1000ff44
					nSkyIndex = i;                           // 0x1000ff49
			}

			pStream->Write(&nSkyIndex, 2);               // 0x1000ff5f
		}

		bHasClient = (pObj->m_pSData->m_pOwningClient != LTNULL);   // 0x1000ffac

		pStream->Write(&bHasClient, 1);                  // 0x1000ffba

		if (bHasClient)                                  // 0x1000ffbd
		{
			pStream->Write(&pObj->m_pSData->m_pOwningClient->m_ClientFlags, 4);
			                                             // 0x1000ffd8
			pStream->WriteString(pObj->m_pSData->m_pOwningClient->m_pName);
			                                             // 0x1000ffee
		}

		// Back-patch the start-of-data offset and come back to it.  nPos
		// takes the DEAD pMgr parameter home (`lea 0x8(%ebp),%ecx` at
		// 0x1000fff3; pMgr's last read is 0x1000ff30), which VC6 gives
		// only to an innermost block's local -- hence these braces.
		{
			uint32 nPos;

			pStream->GetPos(&nPos);                      // 0x1000fff9
			nValue = nPos;                               // 0x10010002
			pStream->SeekTo(nStartFieldPos);             // 0x10010009
			pStream->Write(&nValue, 4);                  // 0x10010015
			pStream->SeekTo(nValue);                     // 0x1001001f
		}
	}

	// AVP2 1.0.9.6 (server.dll 0x10013b9a..0x10013bcf) and Primal Hunt
	// (0x100145a2..0x100145d7): a model's hidden pieces and its main
	// tracker go out ahead of the object's own data, where
	// sm_RestoreNextObject (saveio.cpp) reads them back.
	if (pObj->m_ObjectType == SAV_OT_MODEL)
	{
		SavModelSaveQ4S* pModel = (SavModelSaveQ4S*)pObj;

		pStream->Write(&pModel->m_HiddenPieces, 4);

		if (pMgr->m_pServerLT->m_pModelLT)
			pMgr->m_pServerLT->m_pModelLT->WriteTracker(&pModel->m_AnimTracker, &msg);
	}

	{
		uint32 nPos;

		// 0x1001002b-0x10010038: the cast is `fild QWORD` off {dwParam, 0},
		// which is the unsigned-32-to-float sequence -- dwParam is unsigned.
		pObj->m_pSData->m_pBaseClass->EngineMessageFn(SAV_MID_SAVEOBJECT, &msg,
		                                              (float)dwParam);  // 0x10010044

		nPos = SAVE_SENTINEL;                            // 0x10010050

		pStream->Write(&nPos, 4);                        // 0x10010057

		// And the end-of-data offset, which is what tells the restore
		// where the next object's record starts.
		pStream->GetPos(&nPos);                          // 0x10010062
		nValue = nPos;                                   // 0x1001006b
		pStream->SeekTo(nEndFieldPos);                   // 0x10010072
		pStream->Write(&nValue, 4);                      // 0x1001007e
		pStream->SeekTo(nValue);                         // 0x10010088
	}
}


// ======================================================================== //
//  PART B — 0x100106b6, sav_RestoreObject.  1779 bytes, /O1.
//
//  From here down the REAL engine headers are needed, not Part A's views:
//  the frame holds four class objects whose constructors and destructors
//  the shipped code runs, and two of those run INLINE (ObjectCreateStruct's
//  constructor and CMessageRef's destructor), which cannot happen unless
//  the bodies are visible.
//
//  WHAT THE FRAME PROVES.  `sub esp,0x824` at 0x100106b9, and the seven
//  objects tile it exactly (account in the file header).  0x54 for sData is
//  the element size smgr.cpp's SObjData bank is built with
//  (`sb_Init(bank, 0x54, 0x20)` at 0x1002ce31); 0x1a8 for tempObject is
//  objtypes.h's CObjBase, the size its own bank allocates; 0x284 for
//  createStruct is ltbasedefs.h's ObjectCreateStruct, which this function
//  independently re-derives field by field (see the reads below); 0x68 for
//  moveState is servermgr.h's WMMoveState, and its +0x64 is the fourth
//  stray zero at 0x1001070e, written once and never read.
// ======================================================================== //

#include "servermgr.h"
#include "servermsg.h"
#include "commonphysics.h"
#include "objtypes.h"
#include "ltobject_internal.h"
#include "ltengineobjects.h"
#include "servermodel.h"        // sm_SetObjectChangeFlag (inline)

// commonmodel.cpp's w_ReadVector.  CALLED at /O1, at both sites
// (0x100107bb and 0x10010b45); lithtech's /O2 expands both.  Declared and
// NOT defined, and NOT marked `inline`, precisely so this build emits the
// call — see the header note.
void w_ReadVector(class ILTStream* pStream, LTVector* pVec);

// modelanim.cpp's two tracker setters, called out of line in both builds
// (0x10010c0c -> 0x10001153, 0x10010c17 -> 0x10001110).
LTBOOL at_SetAnim(class LTAnimTracker* pTracker, uint32 iAnim, LTBOOL bInterpolate);
void   at_SetTime(class LTAnimTracker* pTracker, uint32 nTime, LTBOOL bInterpolate);

// 0x10001741 — counter.cpp's linear class lookup (servermodel.cpp already
// declares it with this signature and calls it with this exact argument).
struct ClassDef* sub_10001741(void* pModule, char* pName);

// 0x1000d2ef — dalloc.cpp.
void* dalloc(uint32 nBytes);

// SObjData as this function needs it: the same 0x54-byte record
// ltobject_internal.h describes, plus the constructor the shipped image
// proves it has.
//
// THE CONSTRUCTOR IS DEFINED OUT OF CLASS, AND THE COIN-FLIP THIS FILE
// RECORDED HAS NOW BEEN SETTLED BY MEASUREMENT.  0x10010e03 is a real
// sixteen-byte body — `andl $0x0,0x2c(%ecx)` (the CMessageRef member's own
// constructor), then `lea 0x34(%ecx),%edi ; xor %eax,%eax ; stos ; stos`,
// then `mov %ecx,%eax ; ret` — and server.dll CALLS it from 0x100106de,
// while lithtech.exe EXPANDS it as three plain zero stores (0x0043276b,
// 0x00432777 and 0x0043278b, at S+0x70, S+0x74 and S+0x68 = sData's +0x34,
// +0x38 and +0x2c).
//
// The in-class spelling was the known risk and it did
// bite: OUR /Os expanded the three stores inline, and doing so kept a ZERO
// REGISTER live across the whole entry block, which in turn cost the
// function the register the shipped build spends on the literal 4 and
// BLOCKED VC6's cross-jumper at the two LT_CANTRESTOREOBJECT sites (ours
// emitted `cmp %esi,%ecx` at the first where the shared block has
// `test %ecx,%ecx`, so the sixty-byte block could not be folded).  Moving
// the body out of class is the rule — /Ob1 never expands an
// out-of-class definition wherever it sits — and it took sav_RestoreObject
// from 1843 bytes / 593 instructions / 13 mnemonic blocks to
// 1785 / 579 / 5, and lithtech's /O2 twin at 0x00432740 from 2773 to 2761
// against a shipped 2730.  So BOTH images got closer, and the "one source
// cannot make both" reading in the note this replaces was wrong: /O2's
// expansion of the shipped body is not what our /Os does with ours.
//
// SETTLED — 0x10010e03 IS NOW BYTE-IDENTICAL.  The `lea 0x34(%ecx),%edi ;
// xor %eax,%eax ; stos ; stos` pair is not a memset: it is VC6 constructing
// an ARRAY of two objects whose inline constructor zeroes one pointer.
// servermessage.cpp's CUpdateContext (0x1002860b) proves the idiom — its
// CMsgCursor m_Cursors[2] emits the same `lea 0x10(%ecx),%edi ; xor ; stos ;
// stos`.  So +0x34/+0x38 are one two-element array of a class with an inline
// zeroing constructor and NO destructor (the exits release only +0x2c), and
// the SObjData constructor's own body is empty.  Measured and worse: plain
// stores (14 bytes, no %edi), `memset` without /Oi (27 bytes, a call),
// `#pragma intrinsic(memset)` + memset of the pair (16 bytes but `xor`
// before `lea`, under 8168/8447/8804/8966 alike), and a memset(this)
// constructor on an 8-byte member struct (the same `xor`-first order).  The
// array also emits an unreferenced ??_H vector-constructor COMDAT, as
// VC6 does for this construct.  lithtech.exe's /O2 sav_RestoreObject
// (0x00432740) diffs identically before and after.
class SavFileSlotQ4S
{
public:
	SavFileSlotQ4S() { m_h = 0; }
	void* m_h;
};

class SavObjDataRestQ4S
{
public:
#ifdef PRIMAL_HUNT
	// PH: no constructor of its own. Primal Hunt's restore expands the
	// members' (server.dll 0x10014ca0..0x10014caf: `xor ; lea ; stos` x4 and
	// the +0x44 store) where AvP2 1.0.9.6's calls 0x10014989.
#else
	SavObjDataRestQ4S();                     // out of class, below
#endif

	LTLink        m_Links;                   // 0x00
	void*         m_hIdentifier;             // 0x0c
	float         m_NextUpdate;              // 0x10  read at 0x1001090e
	float         m_DeactivationTime;        // 0x14
	float         m_UpdateTimer;             // 0x18
	class Client* m_pOwningClient;           // 0x1c
	class BaseClass* m_pBaseClass;           // 0x20
	class CClassInfo* m_pClass;              // 0x24
	void*         m_Unknown28;               // 0x28
	// The destructor at every exit is `mov ecx,[ebp-0x64] ; test ecx,ecx ;
	// je ; mov eax,[ecx] ; call [eax+0x8]`, and ebp-0x64 is sData+0x2c.
	// That is CMessageRef's in-class destructor, expanded: so +0x2c is a
	// CMessageRef, not ltobject_internal.h's `uint32 m_Unknown2c`.
	CMessageRef   m_SaveData;                // 0x2c
	void*         m_hFile0;                  // 0x30
	// AVP2: ltobject_internal.h's SObjData -- +8 from the state link on
	// (PH +0xc): the change flags at 0x58 (PH 0x5c), the net flags 0x5a (PH 0x5e).
#ifdef PRIMAL_HUNT
	// PH: the four skin slots and the extra texture's (saved after the
	// skins, sav_SaveObject above), all zeroed by the constructor.
	SavFileSlotQ4S m_hFiles12[4];            // 0x34
	SavFileSlotQ4S m_hExtraTexture;          // 0x44
	uint8       m_pad_48[0x5c - 0x48];
#else
	// AVP2: four slots, as the constructor both this restore and the
	// object-data bank call zeroes (server.dll 0x10014989, called from
	// 0x1001424b and 0x10035d42; smgrnew.cpp's CSObjDataCtorQ474).
	SavFileSlotQ4S m_hFiles12[4];            // 0x34
	uint8       m_pad_44[0x58 - 0x44];
#endif
	uint16        m_ClientChangeFlags;       // 0x58 (PH 0x5c)
	uint16        m_NetFlags;                // 0x5a (PH 0x5e)
};

// 0x10010e03.  Out of class ON PURPOSE: /Ob1 never expands a definition
// written here, so this is what makes the /O1 build CALL 0x10010e03 the way
// server.dll does instead of expanding the zero stores into
// sav_RestoreObject's entry block.  The body is empty: the members'
// constructors (CMessageRef at +0x2c, the slot array at +0x34) are all of it.
#ifndef PRIMAL_HUNT
SavObjDataRestQ4S::SavObjDataRestQ4S()
{
}
#endif

// The model object past LTObject's own fields: the animation tracker the
// restore drives (0x10010c02 `lea esi,[edi+0x1c0]`) and the word that
// follows it (0x10010c21 `add edi,0x1d8`).
class SavModelObjQ4S
{
public:
	uint8  m_pad_000[0x1c0];
	uint8  m_Tracker[0x1d8 - 0x1c0];         // 0x1c0  an LTAnimTracker
	uint16 m_n1d8;                           // 0x1d8  (name ours)
};

// The record the restore hangs on CServerMgr::m_List8ec for an object that
// belonged to a client: an LTLink whose data is the record itself
// (0x10010d60 `mov [esi+0x8],esi`), the client's flag word at +0x0c
// (0x10010d2c), the object's runtime id at +0x10 (0x10010d4c) and the
// client's name inline from +0x12 (0x10010d36).  The allocation is
// strlen(name) + 0x14 (0x10010d1e), which is 0x12 for the header and two
// for the terminator and its alignment (names ours).
class SavClientRecQ4S
{
public:
	LTLink m_Link;                           // 0x00
	uint32 m_nClientFlags;                   // 0x0c
	uint16 m_nObjectID;                      // 0x10
	char   m_szName[2];                      // 0x12
};

// LTObject::m_ChangeFlags bits.  0x100 is "this object is one of the thirty
// sky objects" (0x10010b68 and 0x10010cb3 both `test BYTE PTR [ebp-0xb],1`,
// i.e. bit 8 of the dword at ebp-0xc); 0x80 is what the client record above
// marks the object with (0x10010d53 `or BYTE PTR [eax+0x120],0x80`).
#define SAVCHANGE_SKYOBJECT 0x100
#define SAVCHANGE_HADCLIENT 0x80
// AVP2: an object saved with this bit has its client change flag 0x100
// raised again once restored (AvP2 1.0.9.6's server.dll 0x1001484e `testb
// $0x4,-0xb(%ebp)`, then the out-of-line sm_SetObjectChangeFlag 0x1001499b;
// Primal Hunt's lithtech.exe 0x0043e511; name ours).
#define SAVCHANGE_RESEND    0x400

// The mask sm_SetObjectState is handed (0x10010c85 `and eax,0x38`): the
// three state bits servermgr.h documents on 0x1002ade4 (8, 0x10, 0x20).
#define SAVCHANGE_STATEBITS 0x38

// ltobject_internal.h types LTObject::m_pSData as its own SObjData, whose
// +0x2c is a uint32; the save-data path needs the CMessageRef that is
// really there.  The cast is a no-op — the two views start at the same
// address — and putting the whole chain behind a macro keeps each use a
// FRESH walk of m_pSData, which is what the shipped code does: 0x100109fc,
// 0x10010a1d, 0x10010a32, 0x10010a46 and 0x10010a8f each reload +0x188 and
// +0x2c from scratch (VC6 does not CSE across statements, so
// five reloads means five written-out chains).
#define SAV_SAVEDATA(pObj) (((SavObjDataRestQ4S*)(pObj)->m_pSData)->m_SaveData)

// The six-field WMMoveState fill, as an inline helper: a file-local copy of
// commonphysics.cpp's static wm_InitMoveState (which is what
// CWMStateInit::Init/InitFrom are made of). It has to be a call and not six
// written-out stores: the arguments are evaluated right to left before the
// body runs, so the priority (movzbl 0xbd, 0x10010c32), the object
// (0x10010c44), &pMgr->m_Unknown210 (0x10010c50) and pMgr->m_pUnknown6a8
// (0x10010c5e) are all in registers before the stores go out, and the +0x04
// store then lands ahead of the +0x10 zero (0x10010c6b / 0x10010c71). All
// 720 orders of six written-out stores had been built without reaching that.
static inline void sav_InitMoveState(WMMoveState* pState, void* pContainer,
                                     void* pUnknown04, LTObject* pObj,
                                     uint32 nPriority)
{
	pState->m_pContainer = (CUnknown210*)pContainer;
	pState->m_pUnknown04 = (CUnknown6A8Obj*)pUnknown04;
	pState->m_pObj       = pObj;
	pState->m_nUnknown0C = nPriority;
	pState->m_pPushList  = 0;
	pState->m_nPushCount = 0;
}


// Read one object back out of the save and put it into the world.  The
// record is the one sav_SaveObject wrote; this pass stops at the end offset
// and leaves the stream there.  An end offset of all-ones is the run's
// terminator and answers LT_FINISHED (0x10010717), which is what stops
// sm_RestoreObjects' do/while.
//
// The object is built TWICE OVER.  A stack CObjBase takes the state that
// has to be read before the class can be created (type, flags, position,
// scale, rotation) so an ObjectCreateStruct can be filled from it; the real
// object is then created through the class manager and the REST of the
// state is read straight into it.  That is why the type tests before the
// creation ask ebp-0x1e4 (tempObject+0xbc) and the ones after ask
// [ebp-0x4]+0xbc (pNewObject).
//
// Three failures, and each is the error the whole restore fails with: a
// class the game does not have, a save-data block longer than the packet it
// has to fit in, and a sky index past the end of the thirty slots.  The
// first and the third report LT_CANTRESTOREOBJECT through ONE SHARED
// BLOCK — the third is the copy that stays inline (0x10010b82) and the
// first jumps into it (`jmp 0x10010b8e` from 0x10010783), which is
// the rule "of two identical sites VC6 keeps the SECOND inline".
// Both call CServerMgr::SetupError first; the second reports LT_INVALIDFILE
// (0x10010a54, `push 0x4d ; pop esi`) and does not.
//
// HOW /O1 SHAPES THE EXITS, which is the whole difference from the /O2
// twin.  sData's destructor is DUPLICATED at all four exits (0x1001071d,
// 0x10010975, 0x10010bc5, 0x10010d89) because it is inline and cheap, but
// everything after it is TAIL-MERGED onto one block:
//
//     0x10010729   push 0x2          <- LT_FINISHED
//     0x1001072b   pop esi           <- the LT_CANTRESTOREOBJECT arm
//                                       jumps HERE with 0x3a already
//                                       pushed (0x10010bd1/0x10010bd3)
//     0x1001072c   jmp 0x10010d97
//     ...
//     0x10010d95   xor esi,esi       <- LT_OK falls in
//     0x10010d97   tempObject's destructor, mov eax,esi, epilogue
//
// So `return LT_FINISHED`, `return dResult`, both RETURN_ERROR_INFO sites
// and `return LT_OK` all land on one epilogue.  The tail-merge rule runs both
// ways and this is the /O1 side of it: nothing here needs to be written as
// a goto, and nothing needs its guards nested — the ordinary early returns
// below are what produce this.
LTRESULT sav_RestoreObject(CServerMgr* pMgr, ILTStream* pStream,
                           LTObject** ppObject, uint32 dwParam)
{
	ObjectCreateStruct createStruct;         // ctor -> call 0x10001bd7 @0x100106c8
	CObjBase       tempObject;               // ctor    call 0x1002465b @0x100106d3
	SavObjDataRestQ4S  sData;                    // ctor    call 0x10010e03 @0x100106de
	WMMoveState    moveState;
	LTObject*      pNewObject;
	struct ClassDef* pClassDef;
	LPBASECLASS    pObject;
	LTRESULT       dResult;
	uint32         nEnd;
	uint32         nStart;
	uint32         nChangeFlags;
	uint32         nClientFlags;
	uint16         nDataLen;
	uint16         nSkyIndex;
	float          fLightRadius;
	LTVector       vTemp;
	char           bHasSaveData;
	char           bHasClient;
	char           szClassName[0x100];
	char           szClientName[0x200];

	// The light radius has to survive the creation, which is the only
	// reason it is set here: the read that fills it is inside a type test
	// (0x100108e8) and the store that uses it is not (0x100109a8).
	fLightRadius = 0.0f;                     // 0x100106ef fldz / 0x100106f4
	nSkyIndex = 0;                           // 0x100106f9

	pStream->Read(&nEnd, 4);                 // 0x10010714

	if (nEnd == 0xffffffff)                  // 0x10010717
		return LT_FINISHED;

	pStream->Read(&nStart, 4);               // 0x1001073a

	createStruct.Clear();                    // 0x10010743

	pStream->ReadString(szClassName, sizeof(szClassName));   // 0x10010758

	pClassDef = sub_10001741(pMgr->m_ClassMgr.m_pUnknown00, szClassName);
	                                         // 0x1001076b

	if (!pClassDef)                          // 0x10010774
	{
		pMgr->SetupError(LT_CANTRESTOREOBJECT, szClassName);     // via 0x10010b8e

		RETURN_ERROR_INFO(1, "sm_CreateNextObject", LT_CANTRESTOREOBJECT,
		                  szClassName)
	}

	pStream->Read(&tempObject.m_ObjectType, 1);   // 0x10010795  (+0xbc)
	pStream->Read(&tempObject.m_Flags, 4);        // 0x100107a4  (+0x88)
	pStream->Read(&tempObject.m_Flags2, 4);       // 0x100107b3  (+0x8c)

	w_ReadVector(pStream, &vTemp);                // 0x100107bb

	((LTObject*)&tempObject)->SetPos(vTemp);      // 0x100107cf -> 0x10024a33

	pStream->Read(&tempObject.m_Scale, 0x0c);     // 0x100107e1  (+0xac)
	pStream->Read(&tempObject.m_Rotation, 0x10);  // 0x100107f1  (+0x9c)
	pStream->Read(&tempObject.m_UserFlags, 4);    // 0x10010801  (+0x90)

	// The ObjectCreateStruct fill, and it re-derives ltbasedefs.h's layout
	// on its own: 0x1001080c writes a WORD at createStruct+0x00 from a
	// `movsx ax` of the object's `char` type, 0x10010819 +0x08, 0x10010831
	// +0x0c, the `movs` triple at 0x1001083d +0x10, the one at 0x1001084c
	// +0x1c, and the four dwords at 0x1001084f..0x1001087c +0x28..+0x34.
	createStruct.m_ObjectType = tempObject.m_ObjectType;
	createStruct.m_Flags      = tempObject.m_Flags;
	createStruct.m_Flags2     = tempObject.m_Flags2;
	createStruct.m_Pos        = tempObject.m_Pos;
	createStruct.m_Scale      = tempObject.m_Scale;
	createStruct.m_Rotation   = tempObject.m_Rotation;

	// 0x7f, pushed once into edi at 0x10010867 and reused: the length is
	// MAX_CS_FILENAME_LEN (127), not the 0x80 the buffers are.
	pStream->ReadString(createStruct.m_Name, MAX_CS_FILENAME_LEN);      // 0x10010888 (+0x1fc)
	pStream->ReadString(createStruct.m_Filename, MAX_CS_FILENAME_LEN);  // 0x10010897 (+0x07c)

	if (tempObject.m_ObjectType == OT_MODEL)      // 0x1001089a
	{
		int i;

		// 0x100108a3..0x100108c2: base createStruct+0xfc, step 0x80, two
		// iterations closed by `dec [ebp+0xc] ; jne` — the same strength
		// reduction of a counting `for` the save half's skin loop gets.
		for (i = 0; i < MAX_MODEL_TEXTURES; i++)
			pStream->ReadString(createStruct.m_SkinNames[i],
			                    MAX_CS_FILENAME_LEN);
#ifdef PRIMAL_HUNT
		// PH: and the extra texture's name (server.dll 0x10014ee2; its
		// ReadString is the else arm's, tail-merged).
		pStream->ReadString(createStruct.m_ExtraTextureName, MAX_CS_FILENAME_LEN);
#endif
	}
	else
	{
		pStream->ReadString(createStruct.m_SkinName, MAX_CS_FILENAME_LEN);
	}                                             // 0x100108d2

	if (tempObject.m_ObjectType == OT_LIGHT)      // 0x100108d5 (cmp .. ,4)
		pStream->Read(&fLightRadius, 4);          // 0x100108e8

	if (tempObject.m_ObjectType == OT_CONTAINER)  // 0x100108eb (cmp .. ,9)
		pStream->Read(&createStruct.m_ContainerCode, 2);   // 0x10010901 (+0x02)

	pStream->Read(&sData.m_NextUpdate, 4);        // 0x1001090e
	// AVP2 1.0.9.6 and Primal Hunt: the deactivation time and the update
	// timer follow it (server.dll 0x10014488, 0x10014495; PH 0x10014f3b,
	// 0x10014f4b), and the first goes into the create struct beside the
	// next update (0x100144ac; PH 0x10014f73).
	pStream->Read(&sData.m_DeactivationTime, 4);
	pStream->Read(&sData.m_UpdateTimer, 4);

	createStruct.m_NextUpdate = sData.m_NextUpdate;    // 0x10010914 (+0x27c)
	createStruct.m_fDeactivationTime = sData.m_DeactivationTime;

	// CALLED at /O1 (0x10010920); lithtech's /O2 carries the whole body at
	// 0x00432b7f-0x00432bc4.  Do NOT make it `inline` for this build.
	pObject = sm_AllocateObject(pMgr, pClassDef);

	// 3.0f == PRECREATE_SAVEGAME, and it is a POOLED CONSTANT at /O1:
	// `fld DWORD PTR ds:0x1004d6f0 ; fstp DWORD PTR [esp]` (0x10010925,
	// 0x10010936) over a slot sm_AllocateObject's arguments left behind.
	// The four bytes at 0x1004d6f0 are 00 00 40 40 = 0x40400000 = 3.0f.
	// lithtech.exe pushes the same value as an immediate at 0x00432bd3.
	pObject->EngineMessageFn(MID_PRECREATE, &createStruct,
	                         PRECREATE_SAVEGAME);            // 0x10010940

	// The GLOBAL manager, not the parameter: 0x10010961 pushes
	// DWORD PTR ds:0x10058bf0 (servermgr.h's g_pServerMgr), while every
	// other manager use in this function is pMgr.
	dResult = sm_CreateObjectInWorld(g_pServerMgr, pObject, pClassDef,
	                                 &createStruct, 0xffff,
	                                 PRECREATE_SAVEGAME, &pNewObject);
	                                                          // 0x10010967

	if (dResult)                                              // 0x10010971
		return dResult;

	pNewObject->m_UserFlags = tempObject.m_UserFlags;         // 0x10010993

	if (pNewObject->m_ObjectType == OT_LIGHT)                 // 0x1001099c
		pNewObject->m_LightRadius = fLightRadius;             // 0x100109a8

	// AVP2 1.0.9.6 (0x10014549..0x1001455f) and Primal Hunt: the saved timer comes back only
	// when the object kept the saved deactivation time.
	if (pNewObject->m_pSData->m_DeactivationTime == sData.m_DeactivationTime)
		pNewObject->m_pSData->m_UpdateTimer = sData.m_UpdateTimer;

	pStream->Read(&pNewObject->m_BlockingPriority, 1);        // 0x100109bf (+0xbd)
	pStream->Read(&bHasSaveData, 1);                          // 0x100109cb

	// The object's save-data block: a fresh packet off the message bank,
	// sized to the count the file gives, and then the bytes straight into
	// it.  Every statement walks m_pSData again — see SAV_SAVEDATA above.
	if (bHasSaveData)                                         // 0x100109ce
	{
		pStream->Read(&nDataLen, 2);                          // 0x100109e2

		// msg_AllocPacket's returned temporary takes the DEAD pStream
		// parameter home (`lea 0xc(%ebp),%eax` at 0x100109ea, its release
		// through `mov 0xc(%ebp),%ecx` at 0x10010a0a) -- the same slot the
		// skin countdown at 0x100108a9 already borrows.
		SAV_SAVEDATA(pNewObject).Init(msg_AllocPacket(0x44c, 0x44c));
		                                                      // 0x100109f0, 0x10010a05

		SAV_SAVEDATA(pNewObject).m_pMsg->Init(nDataLen, 0x1000);   // 0x10010a26

		SAV_SAVEDATA(pNewObject).m_pMsg->m_Unknown34 = nDataLen;   // 0x10010a3b

		// `cmp DWORD PTR [eax+0x58],ecx ; jae` past the error, i.e. the
		// error is taken when m_DataSize < nDataLen.
		// THE MEMBER IS ON THE LEFT. 0x10010a50 is `cmp [eax+0x58],ecx`, so
		// the source names m_DataSize first; written the other way round it
		// comes out `cmp ecx,[eax+0x58]` with the branch inverted.
		if (SAV_SAVEDATA(pNewObject).m_pMsg->m_DataSize < nDataLen)  // 0x10010a4f
			RETURN_ERROR_INFO(1, "sm_CreateNextObject", LT_INVALIDFILE,
			                  szClassName)

		pStream->Read(SAV_SAVEDATA(pNewObject).m_pMsg->m_pData, nDataLen);
		                                                      // 0x10010a96
	}

	pStream->Read(&pNewObject->m_ColorR, 1);                  // 0x10010aa8 (+0x94)
	pStream->Read(&pNewObject->m_ColorG, 1);                  // 0x10010aba (+0x95)
	pStream->Read(&pNewObject->m_ColorB, 1);                  // 0x10010acc (+0x96)
	pStream->Read(&pNewObject->m_ColorA, 1);                  // 0x10010ade (+0x97)

	pStream->Read(&pNewObject->m_Velocity, 0x0c);             // 0x10010af1 (+0xc4)
	pStream->Read(&pNewObject->m_Acceleration, 0x0c);         // 0x10010b04 (+0xd0)
	// AVP2: the byte NOLF's engine had not before the friction (AvP2
	// 1.0.9.6's server.dll 0x100146c1, Primal Hunt's lithtech.exe 0x0043e2fe).
	pStream->Read(&pNewObject->m_Unknowndc, 1);
	pStream->Read(&pNewObject->m_FrictionCoefficient, 4);     // 0x10010b17 (+0xdc)
	pStream->Read(&pNewObject->m_Mass, 4);                    // 0x10010b2a (+0xe0)
	pStream->Read(&pNewObject->m_ForceIgnoreLimitSqr, 4);     // 0x10010b3d (+0xe4)

	w_ReadVector(pStream, &vTemp);                            // 0x10010b45

	((CObjDimsSetter*)pNewObject)->SetDims(vTemp);            // 0x10010b56 -> 0x10024ae3

	pStream->Read(&nChangeFlags, 4);                          // 0x10010b65

	if (nChangeFlags & SAVCHANGE_SKYOBJECT)                   // 0x10010b68
	{
		pStream->Read(&nSkyIndex, 2);                         // 0x10010b78

		if (nSkyIndex >= MAX_SKYOBJECTS)                      // 0x10010b7b
		{
			pMgr->SetupError(LT_CANTRESTOREOBJECT, szClassName);  // 0x10010b8e

			RETURN_ERROR_INFO(1, "sm_CreateNextObject", LT_CANTRESTOREOBJECT,
			                  szClassName)
		}
	}

	// The six-field fill (0x10010c32..0x10010c77), interleaved by the
	// scheduler with wm_SetObjectDims' argument pushes; see
	// sav_InitMoveState above for why it is an inline call.
	sav_InitMoveState(&moveState, &pMgr->m_Unknown210, pMgr->m_pUnknown6a8,
	                  pNewObject, pNewObject->m_BlockingPriority);

	wm_SetObjectDims(&moveState, &pNewObject->m_Dims, 0, 0);  // 0x10010c7d

	sm_SetObjectState(pMgr, pNewObject, nChangeFlags & SAVCHANGE_STATEBITS);
	                                                          // 0x10010c8d

	pNewObject->m_ChangeFlags = nChangeFlags;                 // 0x10010c9a

	sm_MoveObject((IWorldSim*)pMgr, pNewObject, &createStruct.m_Pos, 0x0a);
	                                                          // 0x10010cab

	if (nChangeFlags & SAVCHANGE_SKYOBJECT)                   // 0x10010cb3
	{
		pMgr->m_SkyObjects[nSkyIndex] = pNewObject->m_ObjectID;   // 0x10010cc7

		sm_MarkSkyChanged(g_pServerMgr);                      // 0x10010cd5
	}

	if (nChangeFlags & SAVCHANGE_RESEND)
		sm_SetObjectChangeFlag(pMgr, pNewObject, 0x100);

	pStream->Read(&bHasClient, 1);                            // 0x10010ce5

	if (bHasClient)                                           // 0x10010ce8
	{
		SavClientRecQ4S* pRec;

		pStream->Read(&nClientFlags, 4);                      // 0x10010cfc
		pStream->ReadString(szClientName, sizeof(szClientName));  // 0x10010d0f

		pRec = (SavClientRecQ4S*)dalloc(strlen(szClientName) + 0x14);
		                                     // 0x10010d19 strlen, 0x10010d22 dalloc

		pRec->m_nClientFlags = nClientFlags;                  // 0x10010d2c

		strcpy(pRec->m_szName, szClientName);                 // 0x10010d3a

		pRec->m_nObjectID = pNewObject->m_ObjectID;           // 0x10010d4c

		pNewObject->m_ChangeFlags |= SAVCHANGE_HADCLIENT;     // 0x10010d53

		// EXPANDED at /O1 (ltlink.h has it inline): 0x10010d5a..0x10010d76
		// is dl_AddAfter's body with `&pList->m_Head` folded to
		// `lea ecx,[eax+0x4]`, closing on `inc DWORD PTR [eax]`.
		dl_AddHead(&pMgr->m_List8ec, &pRec->m_Link, pRec);
	}

	*ppObject = pNewObject;                                   // 0x10010d80

	pStream->SeekTo(nEnd);                                    // 0x10010d86

	return LT_OK;                                             // 0x10010d95
}


// ======================================================================== //
//  WHAT IS SETTLED, AND WHAT IS NOT.
// ======================================================================== //
//
//  SETTLED BY the retail server.dll'S BYTES (and cross-checked against
//  the retail lithtech.exe where the note says so):
//
//   * THE EXTENT.  0x100106b6 is 1779 bytes, ending at 0x10010da8.  Four
//     independent grounds in the file header; savobj.cpp:25's 1885 is the
//     whole COMDAT RUN (1779 + 90 + 16), not the function.
//
//   * A SECOND EXTENT DEFECT, FOUND ON THE WAY.  The function enumeration
//     carries `sub_10010da9,45` and `sub_10010dd6,45` as two rows.  THEY
//     ARE ONE NINETY-BYTE FUNCTION, 0x10010da9..0x10010e02, and it is
//     ServerMessage::Init — already reconstructed, inline, in
//     src/engine/server/servermsg.h, which documents it as "90 bytes ...
//     NEAR (90 bytes against 90, twenty-eight instructions against
//     twenty-eight)".  0x10010dd6 is NOT an instruction boundary: the
//     bytes there are the `79 50` inside `lea edi,[ecx+0x50]` at
//     0x10010dd5, and its only apparent caller (0x1004c04e) is likewise
//     mid-instruction, inside CRT code.  This is the
//     false-call-target family.  Both rows should be replaced by one
//     `0x10010da9, 90`.
//
//   * Every field offset, width and order in both records.  The save
//     writes and the restore reads the same run in the same order, which
//     is a free cross-check on all of them, and the two type ladders line
//     up field for field between the halves.
//
//   * ltbasedefs.h's ObjectCreateStruct layout, re-derived here offset for
//     offset: m_ObjectType 0x00 (a WORD, written from a `movsx` of the
//     object's `char`), m_ContainerCode 0x02, m_Flags 0x08, m_Flags2 0x0c,
//     m_Pos 0x10, m_Scale 0x1c, m_Rotation 0x28, m_Filename 0x7c,
//     m_SkinNames 0xfc stepping 0x80, m_Name 0x1fc, m_NextUpdate 0x27c,
//     total 0x284.  MAX_CS_FILENAME_LEN is 127 (the 0x7f in edi),
//     MAX_OCS_CLASSNAME_LEN 64 (0x7c - 0x3c) and MAX_MODEL_TEXTURES 2 (the
//     loop's trip count).  All three agree with the SDK header.
//
//   * 3.0f, twice — PRECREATE_SAVEGAME.  See the file header; savobj.cpp
//     has 1.0f and cannot match either image there.
//
//   * LTObject's 0x1a8 really is a union: the save writes it as four raw
//     bytes when the type is OT_LIGHT (0x1000fded) and as `*(char**)0x1a8
//     + 4` when it is OT_WORLDMODEL (0x1000fd48), and the restore stores a
//     float back into it for OT_LIGHT (0x100109a8).
//
//   * MID_SAVEOBJECT is 8 (0x10010042), MID_PRECREATE 0 (0x1001093c), and
//     ltserverobj.h agrees with both.
//
//   * SObjData +0x2c is a CMessageRef, not a scalar.  The save
//     dereferences it and reads +0x34 and +0x54; the restore Inits it
//     through CMessageRef::Init (0x10010a05) and releases it in the stack
//     object's destructor at all four exits.  Worth pushing back into
//     ltobject_internal.h, where it is `uint32 m_Unknown2c`.
//
//   * The manager sm_CreateObjectInWorld and sm_MarkSkyChanged are handed
//     is the GLOBAL g_pServerMgr (0x10058bf0, pushed at 0x10010961 and
//     0x10010ccf), not the pMgr this function was called with.  Every
//     other manager use is the parameter.
//
//   * The two LT_CANTRESTOREOBJECT sites share one block, with the later
//     (sky-index) copy inline and the earlier (class-lookup) one jumping
//     into it at 0x10010783.  And the shared block's OWN tail merges with
//     the LT_FINISHED return: `push 0x3a ; jmp 0x1001072b` lands on the
//     `pop esi` of `push 0x2 ; pop esi`.
//
//  NOT SETTLED, in the order they are likely to cost bytes:
//
//   1. (Settled since: the SObjData constructor is out of class with an
//      empty body and a two-element slot array at +0x34; 0x10010e03 is
//      byte-identical.  See the note at the class.)
//
//   2. nSkyIndex's width, and IT DIFFERS BETWEEN THE TWO HALVES.
//      Restore: `cmp WORD PTR [ebp-0x14],0x1e` (0x10010b7b) and
//      `movzx eax,WORD PTR [ebp-0x14]` (0x10010cb9) both say uint16, but
//      the zero at 0x100106f9 is `mov DWORD PTR [ebp-0x14],esi`, a dword
//      store.  Written uint16 because that is the cheaper way to be wrong
//      (uint16 costs one byte at the initialiser — /Os prefers the
//      three-byte dword store to the four-byte `mov ..,si` when the slot
//      is a whole dword — while uint32 costs one byte at the compare and
//      six at the index).  Save: unambiguously uint32, since 0x1000ff41
//      and 0x1000ff49 are both dword stores AND the compare is against the
//      loop counter (`cmp dx,0x1e`), not against the variable.  Written
//      uint32 there.  The two halves may genuinely differ; if a build says
//      otherwise, the restore's is the one to flip.
//
//   3. Whether the second ObjectCreateStruct::Clear() is spelled
//      `createStruct.Clear()` or `INIT_OBJECTCREATESTRUCT(createStruct)`.
//      Identical expansion; noted only because the double clear (the
//      constructor's at 0x100106c8, then this one at 0x10010743) reads
//      like a mistake and is not.
//
//   4. The four-file-handle question.  SObjData's 0x30/0x34/0x38 are
//      written as `void* m_hFiles[3]` because both halves index them with
//      a strength-reduced offset starting at 0x34 and stepping 4 — an
//      array is the only thing that compiles to that — but
//      ltobject_internal.h names 0x30 m_pModelFile and 0x34 m_pSkinFile
//      and nothing before this named 0x38.  If the original had three
//      named members and a loop is wrong, the save's model arm is wrong
//      with it.
//
//   5. nStart.  The restore reads it (0x1001073a) and never looks at it
//      again; sm_RestoreNextObject is what seeks to it on the second pass.
//      Declared and unused here, as in the shipped frame.  Likewise
//      dwParam, which sav_RestoreObject never reads — ebp+0x14 appears
//      nowhere in the body — but which saveio.cpp:615's declaration and
//      the caller's push both carry.
//
//   6. SETTLED: the six moveState stores are an inline helper call
//      (sav_InitMoveState), not six statements.  The helper's argument
//      evaluation is what puts all four values in registers before the
//      first store; no order of written-out stores reproduces 0x10010c6b.
//
//   7. The names of the three model words (0x1d8, 0x1e8, 0x1ec) and the
//      container word (0x270).  The restore says 0x1e8 and 0x1ec are the
//      animation tracker's animation index and time — they go into
//      at_SetAnim and at_SetTime — but the tracker's field names are not
//      established here, so the save half writes them by address.
//
//  MERGE / OTHER-FILE ITEMS:
//
//   * saveio.cpp:962 defines sav_WriteVector non-static.  savobj.cpp:245
//     argues it is `static` on lithtech evidence (both sites expanded, no
//     out-of-line body emitted).  server.dll calls it either way, so this
//     file is indifferent; but if it is made `static`, the definition has
//     to move ABOVE sav_SaveObject, because VC6's inliner is forward-only
//     and lithtech would otherwise lose both expansions.
//
//   * savobj.cpp's unsettled item 1 wants sm_AllocateObject `inline` in
//     servermgr.h for lithtech's sake.  server.dll CALLS it (0x10010920).
//     Every `inline` is a bet on both images, and the census
//     is a hypothesis, not a proof.  Measure server.dll before keeping it.
//
//   * The CMessageRef at SObjData+0x2c belongs in ltobject_internal.h in
//     place of `uint32 m_Unknown2c`, and 0x10010da9/0x10010dd6 are
//     one 90-byte function.
// ======================================================================== //

#endif // LT_SERVER_BUILD
