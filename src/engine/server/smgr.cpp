// server.dll — CServerMgr's interface-facing half (reconstruction). Build /O1.
#ifdef PRIMAL_HUNT
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x40d050).
// (server.dll too: 0x10010dab, 0x10033e4d, guard 0x1008ec84)
#include <string>
#endif
#ifdef PRIMAL_HUNT
// PH: the world's end gathers the models still in use into an STL set
// (sub_48d400Q502), and the child-model table is an STL map of maps keyed
// by strings (sm_LoadChildModelMapQ523), in both images.
#include <set>
#include <map>
#include <string>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <new.h>
#include "servermgr.h"
#include "servermsg.h"
#include "commonmodel.h"
#include "servermodel.h"
#ifdef AVP2_DEBUG
#include "avp2src_debug.h"
#endif
#include "commonphysics.h"
#include "packet.h"
#include "de_error.h"
#include "ltengineobjects.h"

// Three of servermgr.h's declarations for functions defined in this file
// have the wrong return type -- each body ends `xor %eax,%eax` on its only
// exit, which is LT_OK, where the header says void -- so they are defined
// under a 2 suffix (the convention sm_ObjectRemoved2 and
// sm_UpdateClientState2 already set) and declared here so their callers in
// this file reach the definitions rather than the header's declarations.
LTRESULT sm_ClientEnterWorld2(CServerMgr* pMgr, class Client* pClient);   // 0x1002759a
LTRESULT sm_SendObjectInfo2(CServerMgr* pMgr, class Client* pClient);     // 0x10026b4d

// struct_bank.h's allocate, inline as the header had it. servermgr.h declares
// it extern because server.dll calls 0x10001c86 at every site -- but the same
// source compiled /O2 into lithtech.exe carries the body expanded at those
// same sites (si_CreateObjectList is 69 bytes there against a 33-byte call,
// and sm_FindObjectsTouchingSphere opens with the free-list head, the grow and
// the unlink). An extern function cannot be expanded, so the declaration the
// original saw was inline and it is /Os that declines it: exactly the /O1-vs-
// /O2 split. Defined under a local name so servermgr.h's declaration -- and
// structbank.cpp's out-of-line copy, which server.dll still calls -- stand.
void sb_Grow(StructBank* pBank, uint32 nCount);

inline void* sm_sb_Allocate(StructBank* pBank)
{
	void* pStruct = pBank->m_pFreeList;

	if (!pStruct)
	{
		sb_Grow(pBank, pBank->m_Unknown08);

		pStruct = pBank->m_pFreeList;
		if (!pStruct)
			return LTNULL;
	}

	pBank->m_pFreeList = *(void**)pStruct;
	return pStruct;
}

// 0x10028933 / 0x004665xx -- structbank.cpp's CLEARED allocate, the same
// story as sm_sb_Allocate above: server.dll CALLS it (0x100288f3 in
// snd_FindSentState below) and lithtech.exe expands it at that same site
// (0x00466582..0x004665c5, no call but sb_Grow), which is the /Os-declines /
// /Ot-takes split, so the original declaration was `inline`.
//
// The fill is the CRT `memset`, not a named engine routine: the shipped call
// at 0x10028960 is 0x1003e280, whose normal-path `mov 0x8(%esp),%eax` at
// 0x1003e2cd is taken with %edi already pushed, i.e. it answers its
// DESTINATION, which is memset's contract. /O1 has no /Oi so it stays a
// call; /O2 does, which is why lithtech.exe has `rep stosl ; rep stosb`
// there and no call at all.
inline void* sm_sb_AllocateCleared(StructBank* pBank)
{
	void* pStruct = pBank->m_pFreeList;

	if (!pStruct)
	{
		sb_Grow(pBank, pBank->m_Unknown08);

		pStruct = pBank->m_pFreeList;
		if (!pStruct)
			return LTNULL;
	}

	pBank->m_pFreeList = *(void**)pStruct;

	memset(pStruct, 0, pBank->m_StructSize);

	return pStruct;
}

// CMessageRef::Init now lives in servermsg.h, `inline`, where the original
// had it -- the local copy this file used to carry was a workaround for its
// absence and would now be a duplicate definition.

// 0x10023e86 — servermsg.h's write cursor takes the message directly. Same
// story as the reference above: lithtech.exe's sm_WriteClientUpdate opens
// each of its two packet allocations with NO calls between msg_AllocPacket
// and the returned reference's test -- both the reference's Init and this
// one expanded, the
// freshly-constructed nulls folding both Release guards away -- where
// server.dll calls 0x1000ebab and 0x10023e86. servermessage.cpp owns the
// out-of-line definition and keeps it, so the body is reached through a view
// of the same layout; at /O1 the only difference from the shipped call is its
// masked target.
class CMsgCursorView
{
public:
	ServerMessage* m_pMsg;

	inline ServerMessage* Init(ServerMessage* pMsg)
	{
		if (m_pMsg)
			m_pMsg->Release();

		m_pMsg = pMsg;

		if (m_pMsg)
			m_pMsg->AddRef();

		return m_pMsg;
	}
};

// ---- this file's globals ------------------------------------------------ //
//
// The server manager's own .bss run, 0x100587a0 .. 0x10058bf4 (every
// reference to these is from CServerMgr code in this file; g_ServerInterface
// at 0x10058bb0 is servermgr.cpp's and the bank at 0x10058bc0 svnew.cpp's).
// All zero in the shipped image. Names are servermgr.h's.
uint32          g_Unknown587a0;          // 0x100587a0  gather-tick accumulator
ClientIDList*   g_pFillIDList;           // 0x100587a4  the list being refilled
// 0x100587ac: 256 slots, not the 64 the dispatch can reach (it masks the id
// with 0x3f) -- sm_ResetProfileCounters' memset clears 0x400 bytes, and the
// next global (g_ServerInterface) is at 0x10058bb0 = 0x100587ac + 0x400 + 4.
PacketHandlerFn g_PacketHandlers[256];
int             g_FindSphereCount;       // 0x10058be4
int             g_FindSphereTicks;       // 0x10058be8
uint32          g_Unknown58bec;          // 0x10058bec
// What each world object adds to it: 0x210 in Primal Hunt (lithtech
// 0x0048c626 `sub $0x210` in the removal), 0x20c in AvP2 1.0.9.6 (server.dll
// 0x1003d6d6 / 0x1003d85b); NOLF's engine counted 0x1fc.
#ifdef PRIMAL_HUNT
#define SM_OBJECT_MEMORY 0x210
#else
#define SM_OBJECT_MEMORY 0x20c
#endif
// AVP2: si_FindPoliesTouchingBox's two stats beside the sphere query's,
// cleared by Update and printed under "ShowPolyFindTicks" (Primal Hunt
// lithtech.exe 0x004f6f90 ticks, 0x004f6f7c count; the cvar's record at
// 0x004e3878 points at 0x004f4934). The cvar's storage is defined here
// until dsi.cpp's table carries it.
int             g_FindPolyTicksQ502;
int             g_FindPolyCountQ502;
int             g_ShowPolyFindTicksQ502;
// 0x10058bf0 — the manager the old-API wrappers reach through
// (servermgr.h); SetupGlobals below refreshes it on every API entry. It
// follows this unit's own counters (0x10058be4..0x10058bec) in .bss, zeroed.
CServerMgr* g_pServerMgr;
CClassMgr*      g_pClassMgr;             // 0x10058bf4

// 0x10058bc0 — the bank the queued-object records come out of. Its Allocate
// is 0x1000ed6f, the same three-instruction forward onto the embedded struct
// bank that CHashElementBank::Allocate is (one body, folded by /OPT:ICF), so
// only the shape is established and not which class it belongs to.
//
// Defined in-class rather than declared: 0x1000ed6f IS the /Os compile of
// this body -- `add $0x4,%ecx; push %ecx; call sb_Allocate; pop %ecx; ret`,
// the forward with sm_sb_Allocate DECLINED inside it -- while lithtech.exe's
// sm_RemoveObject (0x0046a2a3) carries the whole two-level expansion, free
// list head, the grow with the bank at 0x004b17ac, and the unlink. An extern
// declaration can be neither, so the original saw a body here; /Ot accepts
// both levels and /Os declines both, which is why server.dll still calls the
// forward. Same argument, and the same local-COMDAT/ICF arrangement, as
// sm_sb_Allocate at the top of this file. One call site in this TU.
class CObjQueueBank
{
public:
	// Image seam (measured): with the body visible, server.dll's /Os expands
	// this forward and calls sm_sb_Allocate directly from sm_RemoveObject
	// (push $imm/pop, 93 vs 92); lithtech.exe's /O2 needs the body to expand
	// both levels (0x0046a2a3). So server.dll sees only the declaration.
#ifdef LT_SERVER_BUILD
	LTLink* Allocate();                      // 0x1000ed6f
#else
	LTLink* Allocate() { return (LTLink*)sm_sb_Allocate(&m_Bank); }
#endif

	uint8      m_vptr[4];
	StructBank m_Bank;                       // +0x04
};

// 0x10058bc0. The object is packet.cpp's g_LinkNodeBank, defined once in
// svnew.cpp; link/aliases.rsp carries this spelling onto that one.
extern CObjQueueBank g_ObjQueueBank;

// 0x10058bdc, the shared LTLink free pool Term and sm_DrainObjectQueue thread
// spent links onto through their m_pPrev, is not a global of its own: it is
// this bank's free-list head (m_Bank.m_pFreeList, +0x1c). servermgr.h's
// `extern LTLink* g_FreeLinks` named that address as if it were one.
#define SM_FREE_LINKS (*(LTLink**)&g_ObjQueueBank.m_Bank.m_pFreeList)

void CServerMgr::SetupGlobals()
{
	g_pServerMgr = this;
	g_pClassMgr = &m_ClassMgr;
}

// SetupError RETURNS dsi_SetupMessage's result: that is what keeps eax busy
// and pushes the reload of nError for the store into ecx (0x10034660;
// lithtech 0x47638d has the same ecx reload, and the client's twin at
// lithtech 0x41fc60 returns the result outright). Declared void, the store
// reloads through eax and the function is two bytes off.
LTRESULT CServerMgr::SetupError(int nError, ...)
{
	va_list marker;
	LTRESULT dResult;

	va_start(marker, nError);
	dResult = dsi_SetupMessage(m_ErrorString, sizeof(m_ErrorString), nError, marker);
	m_ErrorCode = nError;
	return dResult;
}

LTBOOL CServerMgr::AddResources(char** pResources, uint32 nResources)
{
	int nTrees;
	void* pTrees[50];

	SetupGlobals();

	if (nResources == 0)
	{
		SetupError(57);
		return LTFALSE;
	}

	fs_AddResources(&m_FileMgr, pResources, nResources, pTrees, &nTrees);
	if (nTrees == 0)
	{
		SetupError(27, pResources[0]);
		return LTFALSE;
	}

	return LTTRUE;
}

LTBOOL CServerMgr::LoadBinaries()
{
	SetupGlobals();
	return !cb_LoadModule(&m_ClassMgr);
}

void CServerMgr::SetGameInfo(void* pGameInfo, uint32 nGameInfoLen)
{
	dfree(m_pGameInfo);

	m_pGameInfo = dalloc(nGameInfoLen);
	if (m_pGameInfo)
	{
		memcpy(m_pGameInfo, pGameInfo, nGameInfoLen);
		m_GameInfoLen = nGameInfoLen;
	}
}

int CServerMgr::GetNumClients()
{
	return m_ClientList.m_nElements;
}

LTBOOL CServerMgr::GetClientName(int index, char* pName, int maxChars)
{
	SetupGlobals();

	pName[0] = 0;

	if (index < (int)m_ClientList.m_nElements)
	{
		LTLink* pCur = m_ClientList.m_Head.m_pNext;
		while (index > 0)
		{
			pCur = pCur->m_pNext;
			index--;
		}

		GetClientName((Client*)pCur->m_pData, pName, maxChars);
		return LTTRUE;
	}

	return LTFALSE;
}

LTBOOL CServerMgr::GetClientInfo(int index, ClientInfo* pInfo)
{
	if (!pInfo)
		return LTFALSE;

	SetupGlobals();

	if (index < (int)m_ClientList.m_nElements)
	{
		LTLink* pCur = m_ClientList.m_Head.m_pNext;
		while (index > 0)
		{
			pCur = pCur->m_pNext;
			index--;
		}

		Client* pClient = (Client*)pCur->m_pData;
		GetClientName(pClient, pInfo->m_sName, sizeof(pInfo->m_sName));
		pInfo->m_ClientID = pClient->m_ClientID;

		pInfo->m_Ping = -1.0f;
		if (pClient->m_pConnection)
			pInfo->m_Ping = pClient->m_pConnection->m_Ping;

		return LTTRUE;
	}

	return LTFALSE;
}

LTBOOL CServerMgr::BootClient(uint32 dwClientID)
{
	Client* pClient = FindClient(dwClientID);
	if (!pClient)
		return LTFALSE;

	m_NetMgr.Disconnect(pClient->m_pConnection, 6);
	return LTTRUE;
}

Client* CServerMgr::FindClient(uint32 nClientId)
{
	LTLink* pCur = m_ClientList.m_Head.m_pNext;
	while (pCur != &m_ClientList.m_Head)
	{
		Client* pClient = (Client*)pCur->m_pData;
		if (pClient->m_ClientID == nClientId)
			return pClient;

		pCur = pCur->m_pNext;
	}

	return LTNULL;
}

void CServerMgr::GetErrorString(char* pString, int maxLen)
{
	strncpy(pString, m_ErrorString, maxLen);
}

// ---- the ILTSoundMgr personality at +0x04 --------------------------------- //

// The duration comes back through an inlined float-returning accessor
// (name ours; a stand-in for whatever SoundInstance getter the original
// used). lithtech 0x475c8f copies it `flds 0x24(%eax) ; mov ref ; fstps`,
// the value loaded before the reference; a plain member assignment gives
// /O2 an integer move with the reference loaded first. server.dll
// (0x10034233) is `flds/fstps` either way.
inline LTFLOAT sm_GetSoundInstanceDuration(SoundInstance* pSound)
{
	return pSound->m_fDuration;
}

LTRESULT CServerMgr::GetSoundDuration(HLTSOUND hSound, LTFLOAT &fDuration)
{
	SoundInstance* pSound = (SoundInstance*)hSound;

	if (!pSound || !pSound->m_hSample)
		RETURN_ERROR(1, "CServerMgr::GetSoundDuration", LT_INVALIDPARAMS);

	fDuration = sm_GetSoundInstanceDuration(pSound);
	return LT_OK;
}

// The done test is an inlined helper with early returns, not an if/else
// chain in IsSoundDone itself. Written inline in the caller, every
// arrangement of the arms (flat chain, swapped, OR-merged, both nestings,
// ternaries) left server 0x100342b3 with the two result blocks swapped:
// VC6 merged the two LTFALSE sites and hosted the survivor before the timer
// compare. The expansion's returns become jumps to the inline's exit, which
// makes the timer compare fall through to LTFALSE and jump forward (`jbe`)
// to LTTRUE, as shipped. /O2 (lithtech 0x475ce0) is identical either way.
inline LTBOOL sm_IsSoundInstanceDone(SoundInstance* pSound)
{
	if (!pSound->m_hSample)
		return LTFALSE;
	if (pSound->m_fTimer > 0.0f)
		return LTFALSE;
	return LTTRUE;
}

LTRESULT CServerMgr::IsSoundDone(HLTSOUND hSound, LTBOOL &bDone)
{
	SoundInstance* pSound = (SoundInstance*)hSound;

	if (!pSound)
		RETURN_ERROR(1, "CServerMgr::IsSoundDone", LT_INVALIDPARAMS);

	bDone = sm_IsSoundInstanceDone(pSound);
	return LT_OK;
}

// 0x100343f2 — record that a kept sound changed, so every client hears about
// it in the next update. The object map has to still know the sound (its
// entry type is 2 and its record is live) or the call is refused.
//
// LTRESULT, not the void servermgr.h declares (as sm_SetSoundChangeFlag):
// the good path ends `xor %eax,%eax` and the refusal answers LT_ERROR. The
// error site's own literal names the function "SetSoundTrackChangeFlags",
// which is the only naming evidence there is; renamed to it here.
//
// `inline`, and defined here rather than with the rest of the sound helpers,
// because lithtech.exe's KillSoundLoop (0x00475e88..0x00475f02) carries this
// whole body expanded -- its "SetSoundTrackChangeFlags" literal and all --
// while server.dll's calls it (0x100343d8). /Os declines a body this size and
// /Ot takes it: the same /O1-vs-/O2 split as ObjectCreateStruct::Clear. The
// inliner is forward-only, so the definition has to precede its caller; it
// keeps its standalone COMDAT at /O1 because the call site there still needs
// one. The two other expansion sites (0x004838dc, 0x00483a45) are another TU
// reaching it through servermgr.h.
inline LTRESULT sm_SetSoundTrackChangeFlags(CServerMgr* pServerMgr,
                                            SoundInstance* pSound, uint32 dwFlag)
{
	CObjectMapEntry* pEntry =
		&pServerMgr->m_Unknown8fc.m_pArray[((uint32*)pSound->m_pUnknown0c)[2]];

	if (pEntry->m_nFlags != 2 || pEntry->m_pRecord == LTNULL)
		RETURN_ERROR(1, "SetSoundTrackChangeFlags", LT_ERROR);

	if (pSound->m_Unknown2c == 0)
	{
		SoundInstance** ppHead = &pServerMgr->m_pChangedSounds;

		pSound->m_pNextChanged = *ppHead;
		*ppHead = pSound;
	}

	pSound->m_Unknown2c |= (uint16)dwFlag;

	return LT_OK;
}

LTRESULT CServerMgr::KillSound(HLTSOUND hSound)
{
	SoundInstance* pSound = (SoundInstance*)hSound;

	if (!pSound)
		RETURN_ERROR(1, "CServerMgr::KillSound", LT_INVALIDPARAMS);

	// Only sounds the game kept a handle to can be killed by handle.
	if (!(pSound->m_PlaySoundFlags & PLAYSOUND_GETHANDLE))
		RETURN_ERROR(1, "CServerMgr::KillSound", LT_ERROR);

	// The timer is cleared first. /O1 hides that -- it hoists the `fldz`
	// above the integer store and lands `movl $1,0x10 / fstps 0x20`, which
	// reads as the other order -- but /O2 turns the float zero into a plain
	// dword store and keeps source order: 0x20 then 0x10 (0x00475de1).
	pSound->m_fTimer = 0.0f;
	pSound->m_bKill = LTTRUE;
	return LT_OK;
}

LTRESULT CServerMgr::KillSoundLoop(HLTSOUND hSound)
{
	SoundInstance* pSound = (SoundInstance*)hSound;

	if (!pSound)
		RETURN_ERROR(1, "CServerMgr::KillSoundLoop", LT_INVALIDPARAMS);

	if ((pSound->m_PlaySoundFlags & (PLAYSOUND_LOOP | PLAYSOUND_GETHANDLE)) !=
	    (PLAYSOUND_LOOP | PLAYSOUND_GETHANDLE))
	{
		RETURN_ERROR(1, "CServerMgr::KillSoundLoop", LT_ERROR);
	}

	// This method overrides the second base, so it receives the sound
	// subobject and the compiler steps back four bytes to form `this`.
	sm_SetSoundTrackChangeFlags(this, pSound, 0x20);

	// Timer first, as in KillSound: /O2 keeps source order (0x00475ebd).
	pSound->m_fTimer = 0.0f;
	pSound->m_bKill = LTTRUE;
	return LT_OK;
}

int CServerMgr::GetErrorCode()
{
	return m_ErrorCode;
}

// 0x1003318d / 0x00474660 — the two images differ by ONE call, and it is the
// same mechanism serverengine.cpp's se_RegisterModel is written against:
// shared server source that reaches into the CLIENT when a client is in the
// process.
//
//   * lithtech.exe fills m_pUserData from `call 0x00414920` (0x004746aa) and
//     server.dll stores a zero there (`mov %edi,-0x10(%ebp)` at 0x100331d7).
//   * 0x00414920 is three instructions — `mov 0x004b17e0,%eax ; mov
//     0x11d8(%eax),%eax ; ret` — off the CLIENT manager global, and
//     src/engine/lithtech/clientmgr2.cpp already reconstructs it as
//     cm_GetUnknown11d8. Its ONLY caller in the whole image is this one site.
//
// Everything else about the two bodies lines up: the constructor's four
// stores hoist above m_World.Term() in both, and the memset is a real call at
// /O1 and four inline dword stores at /O2 (that is /Oi, not source).
#ifndef LT_SERVER_BUILD
uint32 cm_GetUnknown11d8();               // 0x00414920 (client only)
void   cs_Sub4149e0(int n);               // 0x004149e0 (client only)

// lithtech.exe 0x00474650, 9 bytes, directly in front of LoadWorld -- the
// load hook the client build hands the world loader, where server.dll hands
// dsi_OnReturnError (its empty release hook).  0x004746ba stores it
// (`movl $0x474650,0x1c(%esp)`), not lithtech's dsi_OnReturnError at
// 0x0042d760.  The body is `push $0; call 0x004149e0; pop %ecx; ret`: the
// argument is never read, and the call is the client manager poke CacheFile
// makes too (name ours).
void sm_ClientLoadHook(int nUnused)
{
	cs_Sub4149e0(0);
}
#endif

LTRESULT CServerMgr::LoadWorld(ILTStream* pStream, char* pWorldName)
{
	WorldLoadRequest request;

	m_World.Term();
	pStream->SeekTo(0);

	memset(&request, 0, sizeof(request));
	request.m_pStream = pStream;
#ifdef LT_SERVER_BUILD
	request.m_pUserData = LTNULL;
#else
	request.m_pUserData = (void*)cm_GetUnknown11d8();
#endif
#ifdef LT_SERVER_BUILD
	request.m_LoadHook = dsi_OnReturnError;
#else
	request.m_LoadHook = sm_ClientLoadHook;
#endif

	if (m_World.Load(&request) == 0 && m_World.LoadObjects(pStream) == 0)
	{
		m_bWorldLoaded = 1;
		return LT_OK;
	}

	m_World.Term();
	SetupError(42, pWorldName);
	RETURN_ERROR_INFO(1, "CServerMgr::LoadWorld", LT_INVALIDWORLDFILE, pWorldName);
}

// CServerMgr members AvP2 added that servermgr.h does not declare yet, all
// thiscalls on the manager (names ours, by address in Primal Hunt's
// lithtech.exe). The class doubles as a view of the fields they use, which
// servermgr.h has as m_Unknown66c's padding and m_pUnknown928: the CRC
// string SetCRCString fills (+0x66c), the two sums the CRC members leave
// behind it (+0xa6c, +0xa70), and the manager's ILTServer at +0xd38 (whose
// +0x08 is ILTCSBase's m_pCommonLT: `mov 0x8(%eax),%ecx; call *0x3c`, slot
// 15, ILTCommon::GetCRC).
class CServerMgrQ502
{
public:
	LTRESULT CreateStringCRC();                              // 0x0048bac0
	LTRESULT CreateWorldCRC();                               // 0x0048bd70
	void     Sub48f570(Client* pClient, ServerMessage* pMsg);   // 0x0048f570

	uint8       m_pad_000[0x664];
	int         m_bWorldLoaded;              // 0x664
	void*       m_hWorldFile;                // 0x668
	char        m_szCRCFiles[0x400];         // 0x66c
	uint32      m_nStringCRC;                // 0xa6c
	uint32      m_nWorldCRC;                 // 0xa70
	uint8       m_pad_a74[0xd38 - 0xa74];
	ILTServer*  m_pServer;                   // 0xd38
	uint8       m_pad_d3c[0xe3c - 0xd3c];
	uint8       m_FileMgr[4];                // 0xe3c  the CFileMgr
};

// AVP2: ILTServer::CreateStringCRC's body (Primal Hunt lithtech.exe
// 0x0048bac0, 687 bytes; server.dll 0x10040836, 526 bytes). The CRC string
// is split on "; \n\r\t" (0x1008167c); a name without a '*' is opened
// and summed as it is, a name with one is a pattern: everything before the
// star is a prefix and its directory (cut back to the last '\\'), everything
// after it a suffix, and every file of the directory listing that starts
// with the one and ends with the other (_strnicmp, 0x1006fca0) is summed.
// No file at all leaves the sum 1, never 0. The file count is kept and never
// read (server.dll -0xc(%ebp), `incl` only). Always LT_OK.
LTRESULT CServerMgrQ502::CreateStringCRC()
{
	char       szPrefix[0x80];
	char       szDir[0x80];
	char       szSuffix[0x80];
	char       szFiles[0x400];
	uint32     nCRC;
	uint32     nFiles;
	uint32     dwCRC;
	char*      pTok;
	ILTStream* pStream;

	if (m_szCRCFiles[0])
	{
#ifdef PRIMAL_HUNT
		// PH: the two zeroes ahead of the copy (lithtech.exe 0x0048bae8
		// `xor %ebp,%ebp` inside the expanded strcpy, server.dll 0x1004085e
		// before its call); AvP2 1.0.9.6 zeroes them after it (lithtech.exe
		// 0x00483a34). Each image matches only its own order.
		nFiles = 0;
		nCRC   = 0;

		strcpy(szFiles, m_szCRCFiles);
#else
		strcpy(szFiles, m_szCRCFiles);

		nFiles = 0;
		nCRC   = 0;
#endif

		pTok = strtok(szFiles, "; \n\r\t");
		while (pTok)
		{
			char* pStar = strstr(pTok, "*");

#ifdef AVP2_MODERN
			// RETAIL BUG: a pattern's prefix and suffix are copied into
			// 0x80-byte buffers unchecked, from a token of up to 0x3ff
			// characters; the modern build skips a pattern that will not fit.
			if (pStar && (pStar - pTok >= (int)sizeof(szPrefix) ||
			              strlen(pStar + 1) >= sizeof(szSuffix)))
			{
				pTok = strtok(LTNULL, "; \n\r\t");
				continue;
			}
#endif

			if (pStar)
			{
				strncpy(szPrefix, pTok, pStar - pTok);
				strcpy(szSuffix, pStar + 1);
				szPrefix[pStar - pTok] = 0;

				strcpy(szDir, szPrefix);
				for (int i = strlen(szDir); i > 0 && szDir[i] != '\\'; i--)
					szDir[i] = 0;

				FileEntry* pList  = fs_GetFileList((CFileMgr*)m_FileMgr, szDir);
				FileEntry* pEntry = pList;
				uint32 nPrefixLen = strlen(szPrefix);
				uint32 nSuffixLen = strlen(szSuffix);

				for (; pEntry; pEntry = pEntry->m_pNext)
				{
					if (pEntry->m_Type == TYPE_FILE &&
					    !_strnicmp(pEntry->m_pFullFilename, szPrefix, nPrefixLen) &&
					    !_strnicmp(pEntry->m_pFullFilename +
					               strlen(pEntry->m_pFullFilename) - nSuffixLen,
					               szSuffix, nSuffixLen))
					{
						pStream = fs_OpenFile((CFileMgr*)m_FileMgr,
						                      pEntry->m_pFullFilename);
						if (pStream)
						{
							m_pServer->Common()->GetCRC(pStream, dwCRC);
							nCRC += dwCRC;
							nFiles++;
							pStream->Release();
						}
					}
				}

				fs_FreeFileList(pList);
			}
			else
			{
				pStream = fs_OpenFile((CFileMgr*)m_FileMgr, pTok);
				if (pStream)
				{
					m_pServer->Common()->GetCRC(pStream, dwCRC);
					nCRC += dwCRC;
					nFiles++;
					pStream->Release();
				}
			}

			pTok = strtok(LTNULL, "; \n\r\t");
		}

		if (!nCRC)
			nCRC = 1;

		m_nStringCRC = nCRC;
	}

	return LT_OK;
}

// AVP2: ILTServer::CreateWorldCRC's body (Primal Hunt lithtech.exe
// 0x0048bd70, 156 bytes; server.dll 0x10040a44, 148 bytes): with a world
// loaded, its file named and the server interface up, the world file's
// name (fs_GetFileName, cut at 0xff), upper-cased (_strupr, lithtech.exe
// 0x004ce856), is opened again and summed into +0xa70. Anything missing is
// LT_ERROR, the trailing statement.
LTRESULT CServerMgrQ502::CreateWorldCRC()
{
	char szName[0x100];

	if (m_bWorldLoaded && m_hWorldFile && m_pServer)
	{
		strncpy(szName, fs_GetFileName((CFileMgr*)m_FileMgr, m_hWorldFile), 0xff);
		_strupr(szName);

		ILTStream* pStream = fs_OpenFile((CFileMgr*)m_FileMgr, szName);
		if (pStream)
		{
			m_pServer->Common()->GetCRC(pStream, m_nWorldCRC);
			pStream->Release();
			return LT_OK;
		}
	}

	return LT_ERROR;
}

LTRESULT CServerMgr::DoRunWorld()
{
	if (!m_pWorldFile)
		RETURN_ERROR(1, "CServerMgr::DoRunWorld", LT_NOTINWORLD);

	sm_CacheObjectFiles(this);
	sm_CacheFiles(this);

	m_Unknown198 = 0;
	m_UpdatesPerSec = g_UpdatesPerSec;

	m_ClassMgr.m_pServerShell->PostStartWorld();
	sm_FlushObjectQueue(this);

	dsi_ServerError("Running world");
	return LT_OK;
}

// 0x1002c929 — the server handed back. Term() is the whole body; everything
// after it is the compiler destroying the nine sub-objects that have
// destructors, in reverse declaration order, and then restoring CBaseMgr's
// vftable.
//
// server.dll only. lithtech.exe has neither out of line: clientshell.cpp's
// CreateServerMgr (0x00413590) expands the constructor in place and the
// deleting destructor at 0x00413740 expands this one, both through
// clientshell.cpp's own CServerMgrView; nothing in the image calls a
// standalone CServerMgr constructor or destructor.
#ifdef LT_SERVER_BUILD
CServerMgr::~CServerMgr()
{
	Term();
}

CServerMgr::CServerMgr()
{
	// The type first: Primal Hunt's /GX build stores it ahead of the two
	// counters (server.dll 0x1003832d). AvP2 1.0.9.6's lifts the counters
	// above the vtable stores (0x100355f2), where the order is not visible.
	m_ClientServerType = ServerType;
	m_Unknown7fc = 0;
	m_Unknown800 = 0;
}
#endif

ServerMessage* CServerMgr::StartMessage()
{
	CMessageRef msg;

	msg.Init(msg_AllocPacket(0x44c, 0x44c));

	msg.m_pMsg->m_Write.m_pObjectIO = (CMsgObjectIO*)m_Unknown804;

	// The reference the caller gets: the AddRef here is what the scoped
	// reference's destructor balances on the way out.
	msg.m_pMsg->AddRef();
	msg.m_pMsg->m_MsgFlags |= 1;

	return msg.m_pMsg;
}

// 0x1003318d's mechanism again, and this time the client's half is a whole
// loop. lithtech.exe's FreeUnusedModels (0x00475560..0x00475663) is ours plus
// ONE call — `call 0x00414a00` at 0x0047558d, between the object walk and the
// first dhash pass, return value discarded. 0x00414a00 runs the SAME walk over
// the CLIENT manager's object list (global 0x004b17e0, list head at +0x420)
// and sets the same bit 0 on the same +0x1dc model;
// src/engine/lithtech/csvis.cpp already reconstructs it as cs_DirtyAllModels
// and its note names 0x0047558d as its only call site. So the shipped source
// marks both managers' models and server.dll is the build where the client
// half is compiled out.
#ifndef LT_SERVER_BUILD
void cs_DirtyAllModels();                 // 0x00414a00 (client only)
#endif

// The instance walk names its head first and takes the cursor off it. With
// `pCur = ObjectListHead()->m_pNext; while (pCur != ObjectListHead())` instead, every
// instruction is the same and `this` and the swept `Model*` trade esi for
// edi — the register rotation this function was written off with for two
// sessions. Same lever as sm_EndWorldA/B and sub_1002af00.
LTRESULT CServerMgr::FreeUnusedModels()
{
	LTLink* pCur;
	LTLink* pHead;
	Model* pModel;
	void* hCur;
	void* hElement;

	// Mark every model an object still references...
	pHead = ObjectListHead();
	pCur = pHead->m_pNext;
	while (pCur != pHead)
	{
		pModel = ((ModelInstance*)pCur->m_pData)->m_AnimTracker.m_TimeRef.m_pModel;
		if (pModel)
			pModel->m_Flags |= 1;

		pCur = pCur->m_pNext;
	}

	// ...and, when a client shares the process, the models ITS objects hold...
#ifndef LT_SERVER_BUILD
	cs_DirtyAllModels();
#endif

	// ...count the hash's references per model...
	hCur = dhash_GetFirst(m_hModelHash);
	while (hCur)
	{
		pModel = (Model*)dhash_GetData(dhash_GetNext(&hCur));
		pModel->m_CacheCount++;
	}

	// ...and free each model whose last reference goes by unmarked.
	hCur = dhash_GetFirst(m_hModelHash);
	while (hCur)
	{
		hElement = dhash_GetNext(&hCur);
		pModel = (Model*)dhash_GetData(hElement);

		// AVP2: the count only comes down while it is nonzero, compared
		// unsigned, and a level-3 report follows the level-2 one (Primal
		// Hunt 0x0048da59..0x0048daad).
		if (pModel->m_CacheCount > 0)
			pModel->m_CacheCount--;

		if (pModel->m_CacheCount == 0 && !(pModel->m_Flags & 1))
		{
			if (g_DebugLevel >= 2)
				dsi_ServerError("Debug: Server caching out model %s", pModel->GetFilename());

			if (g_DebugLevel >= 3)
				dsi_ServerError("Removing model from resource list: %s\n", pModel->GetFilename());

			dhash_Remove(m_hModelHash, hElement);
			delete pModel;
		}
	}

	return LT_OK;
}

// 0x1000d020 — CConnList's buffer release, reached through a view so that a
// copy of the body lives in this translation unit. lithtech.exe's Term does
// not call it: the object map's SetSize expands here and the release expands
// inside that, leaving the allocator's virtual Free reached directly and two
// `if (m_pItems)` tests on the same pointer still present: SetSize's, and
// BaseDelete's `if (!ptr) return;` (sm_BaseDeleteItems above) -- this one's
// own folds into SetSize's. The count BaseDelete's empty destruct loop takes
// is what lithtech parks in the frame. With the BaseDelete layer, server.dll
// also emits this body (0x1000d020) and SetSize folds with SetSize2 (0x1000be59). servermgr.h's declaration and netmgr.cpp's out-of-line
// definition -- which server.dll still calls -- are untouched; the call this
// emits at /O1 differs from the shipped one only in its (masked) target.
// Stand-in for collection.h's BaseDelete<T> on a flat view (an element type
// with no destructor): the null test and the dead count loop survive the
// /O2 expansion in lithtech Term (0x473623..0x473644: the count parked in
// 0x10(%esp), the second `cmp %ebp,%eax; je`).
inline void sm_BaseDeleteItems(CMoAllocator* pAlloc, void* ptr, uint32 nElements)
{
	if (!ptr)
		return;

	while (nElements)
	{
		--nElements;
	}

	pAlloc->Free(ptr);
}

class CConnListFreeView
{
public:
	uint8  m_pad_00[0x04];                   // the vptr
	void*  m_pItems;                         // +0x04
	uint32 m_nItems;                         // +0x08
	uint32 m_nSlack;                         // +0x0c
	uint32 m_nGrowBy;                        // +0x10

	inline void FreeItems(CMoAllocator* pAlloc, uint32 nItems)
	{
		if (m_pItems)
		{
			sm_BaseDeleteItems(pAlloc, m_pItems, nItems);
			m_pItems = LTNULL;
		}

		m_nSlack = 0;
	}
};

// 0x1002ccee — the object map's resize is CMoArray<CObjectMapEntry>::SetSize2
// (servermgr.h): lithtech.exe's Term carries it expanded, server.dll calls it.

void CServerMgr::Term()
{
	SetupGlobals();

	m_Thread.Term(LTTRUE);
	ProcessThreadMessages();

	EndWorld(0);

	dfree(m_pGameInfo);
	m_pGameInfo = LTNULL;
	m_GameInfoLen = 0;

	// Remove every client. The removal unlinks, so walk with the next
	// pointer in hand.
	{
		LTLink* pListHead = &m_ClientList.m_Head;
		LTLink* pCur = pListHead->m_pNext;
		while (pCur != pListHead)
		{
			LTLink* pNext = pCur->m_pNext;
			sm_RemoveClient(this, (Client*)pCur->m_pData);
			pCur = pNext;
		}
	}
	dl_InitList(&m_ClientList);

	m_NetMgr.Term();

	sm_RemoveObjects(this, LTTRUE);
	sm_FreeList44(this);

	// Give the spare links back to the shared pool, threading them on
	// through their prev pointers.
	{
		LTLink* pCur = m_Link8c4.m_pNext;
		while (pCur != &m_Link8c4)
		{
			LTLink* pNext = pCur->m_pNext;
			pCur->m_pPrev = SM_FREE_LINKS;
			SM_FREE_LINKS = pCur;
			pCur = pNext;
		}
	}
	m_Link8c4.TieOff();
	m_Unknown8bc = 0;

	m_Unknown8fc.SetSize2(0, (LAlloc*)&g_PacketDataBank);
	m_World.Term();
	sm_TermModels(this);

	dhash_Free(m_hModelHash);
	m_hModelHash = LTNULL;

	m_ClassMgr.Term();
	cb_Term();

	cc_TermState(&m_ConsoleState);
	dsi_SetErrorHook(sm_ErrorHook, LTNULL);

	fs_Term(&m_FileMgr);

	sb_Term(&m_ObjectListBank);
	sb_Term(&m_ObjectLinkBank);
	sb_Term(&m_Bank848);
	sb_Term(&m_Bank9D4);
	sb_Term(&m_Bank864);
	sb_Term(&m_Bank0C);
	sb_Term(&m_Bank28);
	sb_Term(&m_PropBank);
	sb_Term(&m_Bank89C);
	m_MessageBank.Term();

	dhash_Free(m_hHash8c0);
	m_hHash8c0 = LTNULL;

	if (m_Unknown91c)
	{
		dfree(m_Unknown91c);
		m_Unknown91c = LTNULL;
	}

	m_Unknown924 = 0;
	m_Unknown920 = 0;

	un390_Term(&m_Unknown390);

	if (m_pUnknown6a8)
	{
		operator delete(m_pUnknown6a8);
		m_pUnknown6a8 = LTNULL;
	}

	if (m_pUnknown928)
	{
		delete m_pUnknown928;
		m_pUnknown928 = LTNULL;
	}

	if (m_pUnknown808)
	{
		m_pUnknown808->Release();
		m_pUnknown808 = LTNULL;
	}

	if (m_Unknown804)
	{
		operator delete(m_Unknown804);
		m_Unknown804 = LTNULL;
	}

	if (m_pPlaceholderModel)
	{
		delete m_pPlaceholderModel;
		m_pPlaceholderModel = LTNULL;
	}

	m_pAppHandler = LTNULL;
	g_pServerMgr = LTNULL;
	g_pClassMgr = LTNULL;
}

// The packet-slot rewind is INDEXED, not walked. Written with a
// `ClientPacketSet* pSet = &pClient->m_PacketSets[0][0];` cursor and `pSet++`
// VC6 destroys the client pointer with `add $0x134,%ecx` and the function
// comes out a byte short; written as `pClient->m_PacketSets[i][j]` it
// strength-reduces to the same walking pointer but forms it beside the client
// with `lea 0x134(%ecx),%edx`, which is what shipped. (Loops want
// the array indexed, not walked — and this was the last byte of a function
// previously written off as the SpriteControl::SetCurPos SIB near class.)
void sub_48d400Q502(CServerMgr* pMgr);    // 0x0048d400

void CServerMgr::EndWorld(int bKeepWorldData)
{
	LTLink* pCur;
	int i, j;

	m_bWorldLoaded = 0;

	// Rewind every client's packet slots to one byte.
	for (pCur = m_ClientList.m_Head.m_pNext; pCur != &m_ClientList.m_Head; pCur = pCur->m_pNext)
	{
		Client* pClient = (Client*)pCur->m_pData;
		for (i = 0; i < 2; i++)
		{
			for (j = 0; j < 2; j++)
			{
				CPacket* pPacket = pClient->m_PacketSets[i][j].m_pPacket;
				pPacket->m_Pos = 1;
				pPacket->m_DataLen = 1;
			}
		}
	}

	sm_EndWorldA(this);
	sm_EndWorldB(this);

	{
		LTLink* pHead = &m_ClientList.m_Head;
		for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
			sm_ResetClient(this, (Client*)pCur->m_pData, 1);
	}

	sm_EndWorldC(this);
	sm_RemoveObjects(this, LTFALSE);
	sm_EndWorldD(this);

	m_World.TermObjects();
	if (!bKeepWorldData)
		m_World.Term();

	m_Unknown198 = 1;
	// AVP2: one more teardown step after the flag (Primal Hunt 0x0048d396
	// -> 0x0048d400, AvP2 1.0.9.6 0x00485175 likewise; cdecl, the manager
	// its one argument; not yet reconstructed).
	sub_48d400Q502(this);
	dsi_ServerError("World ended");
}

#ifdef PRIMAL_HUNT
// PH: CLTServer::BroadcastFileMessage again, as a free function of the
// manager (server.dll 0x10041ad0, lithtech.exe 0x0048d660; name ours): tell
// every client -- or, without bToAll, every client not flagged 2 -- about
// one file. The writes are WriteType's, called at /O1 (0x10041b42,
// 0x10041b4d) and expanded at /O2, as in servermessage.cpp's PH copy.
void sm_BroadcastFileMessageQ523(CServerMgr* pMgr, uint32 nFileType,
                                 uint32 nMsgID, uint16 nFileID, int bToAll)
{
	CMessageRef msg;
	LTLink*     pListHead;
	LTLink*     pCur;

	pListHead = &pMgr->m_ClientList.m_Head;
	for (pCur = pListHead->m_pNext; pCur != pListHead; pCur = pCur->m_pNext)
	{
		Client* pClient = (Client*)pCur->m_pData;

		if (!bToAll && (pClient->m_ClientFlags & 2))
			continue;

		msg.Init(msg_AllocPacket(0x44c, 0x44c));

		((CPacket*)msg.m_pMsg)->WriteType((uint8)nFileType);
		((CPacket*)msg.m_pMsg)->WriteType((uint16)nFileID);

		sm_SendToClient(pMgr, pClient, nMsgID, msg.m_pMsg, 0, 0x80);
	}
}

// PH: CLTServer::UnloadModel as a free function of the manager (server.dll
// 0x10041a4e, lithtech.exe 0x0048d5d0; name ours), which the world's end
// calls for every model nothing uses any more (sub_48d400Q502). The missing
// file is reported under "ThreadLoadTexture", the literal the interface's
// copy has too.
LTRESULT sm_UnloadModelFileQ523(CServerMgr* pMgr, char* pFilename)
{
	CFileIdentifier* pFile;

	if (!fs_GetFileIdentifier(&pMgr->m_FileMgr, pFilename, 0, (void**)&pFile))
#ifdef AVP2_MODERN
		RETURN_ERROR_INFO(1, "UnloadModel", LT_MISSINGFILE, pFilename);
#else
		RETURN_ERROR_INFO(1, "ThreadLoadTexture", LT_MISSINGFILE, pFilename);
#endif

	LTRESULT dResult = sm_UnloadModel(pMgr, pFilename, pFile);
	if (dResult != LT_OK)
		return dResult;

	sm_BroadcastFileMessageQ523(pMgr, FT_MODEL, 0x18, pFile->m_FileID, 0);
	return LT_OK;
}

// PH: lithtech.exe 0x0048d400, server.dll 0x10041995 -- the last step of
// EndWorld. Every model in the manager's model hash whose +0x190 is set goes
// into a set (so each is visited once, in address order), and then each of
// them is unloaded by name and the clients told. The set's own code is the
// STL's: its insert, iterator step and destructor are the template members
// around this function (server.dll 0x10043541, 0x10044089, 0x10043251;
// lithtech.exe 0x004904e0 ...).
void sub_48d400Q502(CServerMgr* pMgr)
{
	std::set<Model*> models;
	void*            hIter;
	Model*           pModel;

	hIter = dhash_GetFirst(pMgr->m_hModelHash);
	while (hIter)
	{
		pModel = (Model*)dhash_GetData(dhash_GetNext(&hIter));

		if (pModel->m_Unknown190)
			models.insert(pModel);
	}

	for (std::set<Model*>::iterator it = models.begin(); it != models.end(); ++it)
		sm_UnloadModelFileQ523(pMgr, (*it)->GetFilename());
}
#endif

// Detach every client's tracked object (name ours).
void sm_EndWorldC(CServerMgr* pMgr)
{
	// A for loop off pHead->m_pNext: the while form with pCur read from the
	// list member swapped esi/edi in lithtech (0x00475524); server.dll is
	// identical either way.
	LTLink* pHead = &pMgr->m_ClientList.m_Head;
	for (LTLink* pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		Client* pClient = (Client*)pCur->m_pData;
		LTObject* pObj = pClient->m_pUnknown3AC;
		if (pObj)
		{
			pObj->m_pSData->m_pOwningClient = LTNULL;
			pClient->m_pUnknown3AC = LTNULL;
		}
	}
}

// ltbasedefs.h's ObjectCreateStruct is used here as the header has it. The
// local view this file used to carry redeclared it with Clear() out of line,
// on the theory that /Ob1 would otherwise expand the SDK's inline body into
// every caller and lose the calls the shipped code makes. The shipped bytes
// say the opposite, and say it in both images:
//
//   * server.dll's PrecacheModels (0x10033e76, which we already match) opens
//     `call 0x10001bd7` and calls it twice more inside the loops -- so /Os
//     DECLINED the inline body. 0x10001bd7 is Clear itself, and it is the
//     header's body exactly: m_CreateFlags=1, the two uint16 zeroes, the two
//     flag words, m_Pos.Init(), m_Scale.Init(1,1,1), m_Rotation.Init(), then
//     the four string heads with MAX_MODEL_TEXTURES=2 rolled as a loop and
//     the two trailing floats as `fldz/fstps` at 0x27c and 0x280 -- which is
//     what pinned the tail as m_NextUpdate and m_fDeactivationTime rather
//     than the eight bytes of pad the view called it.
//   * lithtech.exe's (0x00475670) has that body EXPANDED at the two explicit
//     oc.Clear() sites and CALLED once at the top, because the constructor
//     -- `{ Clear(); }`, small enough for /Ot -- is what got expanded there
//     and the inliner does not recurse into an expansion it just made.
//
// One spelling, both shapes: exactly the /O1-vs-/O2 split. Declaring Clear
// out of line bought server.dll nothing and cost lithtech.exe 292 bytes of
// this function.

// ---- the world's model precache ------------------------------------------ //

// 0x10001865 — a class by name out of the class manager's hash; CClassMgr
// does not declare it, so it is reached through a view (name ours, from what
// its answer is used as: CreateObject's hClass).
class CClassMgrFindView
{
public:
	HCLASS FindClass(char* pszName);         // 0x10001865
};

// The shipped body (0x10001865, 47 bytes, thiscall, `ret $0x4`): strlen of
// the name, the hash lookup on m_hClassHash (+0x24) with that length, and
// the element's data if there was one. Both cleanups merge into one
// `add $0x10,%esp`. Its home in the image is between CClassMgr::Term
// (0x100017c8) and cb_LoadModule (0x10001894), i.e. counter.cpp's run of
// CClassMgr members; it is written here, through the view, because this is
// the file that calls it.
HCLASS CClassMgrFindView::FindClass(char* pszName)
{
	void* hElement = dhash_FindElement(((CClassMgr*)this)->m_hClassHash,
		pszName, strlen(pszName));

	if (hElement)
		return (HCLASS)dhash_GetData(hElement);

	return LTNULL;
}

// The parts of a resource tree the precache reads. servermgr.h models the
// tree as a pad and one list pointer and the list as three named fields, and
// neither covers what is needed here, so both are reached through views
// (names by offset). The flag word at the list's +0x48 decides which of the
// two shapes the tree holds: bit 3 an indexed table of names, bits 4-5 a
// single name hanging off the tree's own +0x0c.
class CPrecacheListView
{
public:
	uint8  m_pad_00[0x04];
	uint32 m_Table04;                        // +0x04  the table 0x10007e44 indexes
	uint8  m_pad_08[0x48 - 0x08];
	uint32 m_Flags48;                        // +0x48
	uint8  m_pad_4c[0x140 - 0x4c];
	uint32 m_nEntries140;                    // +0x140
};

class CPrecacheNameView
{
public:
	uint8 m_pad_00[0x04];
	char  m_Name[1];                         // +0x04  inline
};

class CPrecacheTreeView
{
public:
	uint8              m_pad_00[0x04];
	CPrecacheListView* m_pList;              // +0x04
	uint8              m_pad_08[0x0c - 0x08];
	CPrecacheNameView* m_pName0c;            // +0x0c
};

// 0x10007e44 — the i'th name out of the table at the list's +0x04, copied
// into the caller's buffer; nonzero if there was one (name ours).
int fs_GetTableName(void* pTable, uint32 iEntry, char* pName, uint32 nMaxLen);

// 0x10033e76 — every file the world's resource trees name gets an object
// made for it, so whatever loading it needs happens now rather than mid
// frame. LTBOOL, not the void servermgr.h declares (`push $1; pop %eax` on
// the good path, `xor %eax,%eax` on every refusal), so it is defined through
// a view of the manager and DoStartWorld calls it through the same view.
#ifndef LT_SERVER_BUILD
void cs_PreLoadWorld(char* pWorldName);     // 0x004148d0 (client only)
#endif

class CPrecacheView
{
public:
	LTBOOL PrecacheModels();                 // 0x10033e76
};

#ifdef PRIMAL_HUNT
// PH: Primal Hunt's "child model" table, defined below sm_CacheObjectFiles
// (names ours).
void     sm_LoadChildModelMapQ523();
void     sm_ApplyChildModelsQ523(ILTServer* pServer, char* pWorldName);
#endif

LTRESULT CServerMgr::DoStartWorld(char* pszWorldName, uint32 flags, float fTime)
{
	char str[256];
	LTBOOL bSameWorld;
	void* hFile;
	LTRESULT dResult;

#ifdef PRIMAL_HUNT
	// PH: server.dll 0x100413fe, before anything else.
	sm_LoadChildModelMapQ523();
#endif

	// Reseed everything from the clock, the game shell included.
	uint32 seed = (uint32)dsi_GetTime();
	srand(seed);
	m_ClassMgr.m_pServerShell->SRand(seed);

	// Does the request name the world already loaded?
	bSameWorld = LTFALSE;
	if ((flags & 4) && m_hWorldFile)
	{
		strncpy(str, fs_GetFilename(&m_FileMgr, m_hWorldFile), 255);
		_strupr(str);

		char* pExt = strstr(str, ".DAT");
		if (pExt)
			*pExt = 0;

		if (_stricmp((char*)pszWorldName, str) == 0)
			bSameWorld = LTTRUE;
	}

	LTBOOL bWasRunning = (m_Unknown198 == 0);
	m_ClassMgr.m_pServerShell->PreStartWorld(bWasRunning);
	if (bWasRunning)
		EndWorld(bSameWorld);

	memset(m_SkyObjects, 0xff, sizeof(m_SkyObjects));
	// Integer store order here and below is lithtech's (0x004751e6,
	// 0x00475339, 0x00475356..0x0047538f), which keeps source order; /O1
	// hoists its `andl $0` stores and matches either way.
	m_Time = 0.0f;
	m_pChangedObjects = LTNULL;
	m_pChangedSounds = LTNULL;
	m_Link83c.TieOff();

	sprintf(str, "%s.dat", pszWorldName);

	ILTStream* pStream = fs_OpenFile(&m_FileMgr, str);
	if (!pStream)
	{
		SetupError(41, pszWorldName);
		RETURN_ERROR_INFO(1, "CServerMgr::DoStartWorld", LT_MISSINGWORLDFILE, pszWorldName);
	}

	if (!fs_GetFileIdentifier(&m_FileMgr, str, 1, &hFile))
	{
		SetupError(41, pszWorldName);
		RETURN_ERROR_INFO(1, "CServerMgr::DoStartWorld", LT_MISSINGWORLDFILE, pszWorldName);
	}

#ifndef LT_SERVER_BUILD
	// lithtech.exe only (0x00475287 `call 0x004148d0`): the client shell
	// hears about the world before it loads. server.dll has no such call.
	cs_PreLoadWorld(pszWorldName);
#endif

#ifdef PRIMAL_HUNT
	// PH: server.dll 0x10041588 (lithtech.exe 0x0048cd99, after the client
	// shell's notice), once the world file is known.
	sm_ApplyChildModelsQ523((ILTServer*)m_pUnknown928, pszWorldName);
#endif

	if (bSameWorld && m_hWorldFile)
	{
		m_bWorldLoaded = 1;
	}
	else
	{
		dResult = LoadWorld(pStream, pszWorldName);
		if (dResult != LT_OK)
		{
			pStream->Release();
			return dResult;
		}
	}

	m_hWorldFile = hFile;

	sm_StartWorldA(this, 1);
	((CPrecacheView*)this)->PrecacheModels();

	dResult = sm_LoadWorldObjects(this, pStream, pszWorldName, flags & 1);
	if (dResult != LT_OK)
	{
		dsi_ServerError("FAILED to load world: %s", pszWorldName);
		return dResult;
	}

	m_pWorldStream = pStream;
	m_Unknown324 |= 1;
	dsi_ServerError("Loaded world: %s", pszWorldName);

	m_Unknown6c4 = 0;
	m_Unknown6bc = fTime;
	m_Unknown6c0 = 0.0f;
	m_Unknown6d0 = 0;
	m_Unknown6c8 = 0.0f;
	m_Time = 0.0f;
	m_Unknown6b8 = 0.0f;
	m_Unknown6d8 = fTime;
	m_FrameTime = 0.0f;

	if (flags & 2)
		DoRunWorld();

	return LT_OK;
}

#ifdef PRIMAL_HUNT
extern std::map<std::string, std::map<std::string, std::string> > g_ChildModelMapQ523;
extern std::string g_sChildModelDefaultQ523;

// PH: server.dll 0x1004167e -- fill the child-model table from
// childmodel.map, once. Each line is a world, a model file and the child
// model that stands in for it, separated by any of "; \t\n\r", all three
// lowered; a line short of three is skipped.
// RETAIL BUG: the file is never closed (no fclose after the loop,
// 0x100417ff); the modern build closes it.
//
// NEAR on both images, one register fact: shipped tests the size in
// memory (server.dll 0x10041691 `cmp %ebx,0x1008dfe4`) and lithtech.exe
// zeroes %eax again before the "none" strlen (0x0048cef4) -- the shipped
// compiler did not know the size it had tested was in a register and zero.
// Ours loads it and reuses it (server.dll 401/400, lithtech.exe 885/887).
// Unmoved by an if-block round the body, `!size()`, empty(); `<= 0`,
// `>= 1` and a cast give `ja`.
void sm_LoadChildModelMapQ523()
{
	if (g_ChildModelMapQ523.size() != 0)
		return;

	g_sChildModelDefaultQ523 = "none";

	FILE* pFile = fopen("childmodel.map", "r");
	if (!pFile)
		return;

	char szLine[256];

	while (fgets(szLine, 256, pFile))
	{
		char* pWorld = strtok(szLine, "; \t\n\r");
		char* pModel = strtok(LTNULL, "; \t\n\r");
		char* pChild = strtok(LTNULL, "; \t\n\r");

		_strlwr(pWorld);
		_strlwr(pModel);
		_strlwr(pChild);

		if (pWorld && pModel && pChild)
			g_ChildModelMapQ523[pWorld][pModel] = pChild;
	}

#ifdef AVP2_MODERN
	fclose(pFile);
#endif
}
#endif

// The object Init hangs at +0x6a8: the object mover, whose vtable is the
// thirteen-slot table at 0x1004e8c8 (Init's inline construction stores it at
// 0x10031fbe, then the manager at +0x04). servermgr.h's CUnknown6A8Obj
// declares one slot of it, so constructing that emitted a one-entry vtable;
// the class is objmover.cpp's CObjectMoverQ47C, restated here with the same
// slots and signatures so this construction plants the full table and each
// slot links to the body that file (or spritecontrol.cpp, through
// link/aliases.rsp) defines.
class CObjectMoverQ47C
{
public:
	CObjectMoverQ47C(CServerMgr* pMgr)
	{
		m_pMgr = pMgr;
	}

	virtual void SetObjectChangeFlags(LTObject* pObj, uint32 nFlags);  // 0x1003524a
	virtual CollisionInfo** GetCollisionInfo();                          // 0x10035260
	virtual void TouchNotify(LTObject* pObj, HOBJECT hToucher,
	                         LTVector* pStopVel, float fForceMag);        // 0x10035269
	virtual void Attach(LTObject* pObj, LTObject* pParent);             // 0x100352d5
	virtual void Detach(LTObject* pObj);                                 // 0x100352f5
	virtual void UpdateAttachments(WMMoveState* pState);                 // 0x1003530b
	virtual int  _v18();                                                 // 0x10035541
	virtual uint32 Crush(LTObject* pObj, void* pData);                   // 0x100352b4
	virtual void _v20();                                                 // 0x10035568
	virtual int  _v24();                                                 // 0x100355db
	virtual LTBOOL CanDeactivate(LTObject* pObj);                        // 0x100355df
	virtual char*  GetObjectClassName(LTObject* pObj);                   // 0x100355ec
	virtual uint32 _v30();                                               // 0x10032332

	CServerMgr* m_pMgr;                      // +0x04
};

#ifdef PRIMAL_HUNT
extern "C" {
__declspec(dllimport) long __stdcall InterlockedIncrement(long* pAddend);
__declspec(dllimport) long __stdcall InterlockedDecrement(long* pAddend);
}

// PH: a reference-counted object, released through its virtual destructor
// when the last reference goes (server.dll 0x1003f2d1: InterlockedDecrement
// of +0x04, then `delete this` -- the null test and slot 0 with 1 pushed).
// What it is is not established; only the count at +0x04 and the virtual
// destructor are read (names ours).
class CRefObjQ523
{
public:
	virtual ~CRefObjQ523();

	CRefObjQ523* AddRef()
	{
		InterlockedIncrement(&m_nRefs);
		return this;
	}

	void Release()                           // 0x1003f2d1
	{
		if (InterlockedDecrement(&m_nRefs) <= 0)
			delete this;
	}

	long m_nRefs;                            // +0x04
};

// PH: the one-pointer smart reference that holds one (names ours):
// assignment (server.dll 0x10043361) releases what it held and takes a
// reference on what it is given; the destructor (0x1003f2c5) releases.
class CRefPtrQ523
{
public:
	CRefPtrQ523();                           // 0x100430c1
	~CRefPtrQ523();                          // 0x1003f2c5

	CRefObjQ523* operator=(CRefObjQ523* p)
	{
		if (m_p != p)
		{
			if (m_p)
				m_p->Release();

			m_p = p ? p->AddRef() : LTNULL;
		}

		return p;
	}

	CRefObjQ523* m_p;
};

// Out of line: the global's initializer reaches both by a tail jump
// (server.dll 0x1003f2aa, 0x1003f2c0), and the constructor's six bytes are
// shared by identical-code folding with three other users.
CRefPtrQ523::CRefPtrQ523()
{
	m_p = LTNULL;
}

CRefPtrQ523::~CRefPtrQ523()
{
	if (m_p)
		m_p->Release();
}

// PH: server.dll 0x1008dfc8 (its initializer 0x1003f29b, the destructor
// registered through atexit at 0x1003f2af); cleared by Init. Name ours.
CRefPtrQ523 g_RefQ523;

// PH: what Primal Hunt keeps in the 0x408 bytes servermgr.h carries as
// m_Unknown66c: Init empties a 0x400-byte string at the front and zeroes the
// two dwords after it (server.dll 0x1003f79e, 0x1003f7a4, 0x1003f7b2).
// servermgr.h is shared with files this one does not own, so the fields are
// reached through a view (names ours).
class CServerMgrPHViewQ523
{
public:
	uint8  m_pad_000[0x66c];
	char   m_sz66c[0x400];                   // 0x66c
	uint32 m_na6c;                           // 0xa6c
	uint32 m_na70;                           // 0xa70
};
#endif

LTBOOL CServerMgr::Init()
{
	SetupGlobals();

	m_Unknown804 = new CMessageObjectHelper(this);
	m_pPlaceholderModel = new CUnknownB28Obj(&g_PacketDataBank, &g_PacketDataBank);
	m_pUnknown6a8 = (CUnknown6A8Obj*)new CObjectMoverQ47C(this);

	if (un390_Init(&m_Unknown390, 0) == 0 &&
	    m_Unknown804 && m_pPlaceholderModel && m_pUnknown6a8 &&
	    m_Thread.Init(this) != 0 &&
	    m_Thread.Start(3) == 0)
	{
		m_Unknown210.Init(&m_Unknown390);
		m_NetMgr.SetNetName("SERVER_PLAYER");

		m_NetMgr.m_pHandler = this;
		m_Link83c.TieOff();
		m_pAppHandler = LTNULL;
		m_pCurProps = LTNULL;
		m_ErrorString[0] = 0;
		m_Unknown8fc.SetCacheSize(100);
		m_Unknown91c = LTNULL;
		m_Unknown924 = 0;
		m_Unknown920 = 0;
		memset(m_SkyObjects, 0xff, sizeof(m_SkyObjects));
		m_hGlobalLightObject = 0;
		m_Unknown8b8 = 200;
		m_Unknown8bc = 0;
		m_pGameInfo = LTNULL;
		m_GameInfoLen = 0;
		m_Unknown198 = 1;
		m_ServerFlags = 0;
		m_Unknown1a0 = 0;
		g_Unknown58bec = 0;
		m_Unknown7f8 = 0;
		m_Unknown7f4 = 0;

		sb_Init2(&m_ObjectListBank, 8, 0x10, 4);
		sb_Init2(&m_ObjectLinkBank, 8, 0x80, 0x40);
		sb_Init2(&m_Bank848, 0x14, 0x20, 0x20);
		sb_Init2(&m_Bank9D4, 0x164, 0x30, 0x20);
		sb_Init2(&m_Bank864, 0x18, 0x20, 0x10);
		sb_Init2(&m_Bank0C, 0x1c, 0x10, 0x10);
#ifdef PRIMAL_HUNT
		// PH: the sound instance is 0x68 bytes (server.dll 0x1003f6a2), the
		// sound type and user data at +0x60/+0x64 that sub_100298d9 sends.
		sb_Init2(&m_Bank28, 0x68, 0x10, 0x10);
#else
		sb_Init2(&m_Bank28, 0x60, 0x10, 0x10);
#endif
		sb_Init2(&m_PropBank, 0xc0, 0x10, 0x10);
		sb_Init2(&m_Bank89C, 10, 0x20, 0x20);

		m_MessageBank.m_Bank.m_Unknown08 = 0x40;

		m_Link8c4.TieOff();
		m_Link8d0.TieOff();
		dl_InitList(&m_List8dc);
		dl_InitList(&m_ClientList);
		dl_InitList(&m_List8ec);
		dl_InitList(&m_List44);
		dl_InitList(&m_List54);

		m_SkyDef.m_Min.Init(-10.0f, -10.0f, -10.0f);
		m_SkyDef.m_Max.Init(10.0f, 10.0f, 10.0f);
		m_SkyDef.m_ViewMin.Init();
		m_SkyDef.m_ViewMax.Init();

		// Store order from lithtech 0x47345c..0x4734aa, which keeps these
		// zero stores in source order (server.dll's /O1 is identical either way).
		m_bWorldLoaded = 0;
		m_hWorldFile = LTNULL;
#ifdef PRIMAL_HUNT
		((CServerMgrPHViewQ523*)this)->m_sz66c[0] = 0;
		((CServerMgrPHViewQ523*)this)->m_na6c = 0;
		((CServerMgrPHViewQ523*)this)->m_na70 = 0;
#endif
		m_pUnknown808 = LTNULL;
		m_FrameTime = 0.0f;
		m_UpdatesPerSec = 0.0f;
		m_Unknown6bc = 0.0f;
		m_Unknown6c0 = 0.0f;
		m_Unknown6c4 = 0;
		m_Unknown6c8 = 0.0f;
		m_Time = 0.0f;
		m_Unknown6d0 = 0;
		m_Unknown6d4 = 0.0f;
		m_Unknown6d8 = 0.0f;
		m_Unknown6b8 = 0.0f;

		m_hHash8c0 = dhash_Create(500, 1);
		m_hModelHash = dhash_Create(100, 1);

		cc_InitState(this, &m_ConsoleState);
		cc_LoadConfigFile(&m_ConsoleState, "s_autoexec.cfg", 2, 0);

		m_pUnknown928 = sm_CreateUnknown928(this);

		fs_Init(&m_FileMgr, this);
		m_ClassMgr.Init(this);
		m_UnknownA14.Init(300);

		m_pCollisionInfo = LTNULL;
		sm_ResetProfileCounters();
#ifdef PRIMAL_HUNT
		// PH: server.dll 0x1003f87f, a call to the assignment with zero;
		// lithtech.exe expands it (0x0048a7xx: the release, then the store).
		g_RefQ523 = LTNULL;
#endif
		return LTTRUE;
	}

	return LTFALSE;
}

LTRESULT CServerMgr::PlaySound(PlaySoundInfo* pPlaySoundInfo, HLTSOUND& hResult)
{
	SoundInstance* pSound = LTNULL;
	void* pTrack = LTNULL;
	char* pSoundName;
	void* hFile;

	if (!pPlaySoundInfo)
		return LT_INVALIDPARAMS;

	// A world must be up, and an attached sound must name its object.
	if (!m_pWorldFile)
		return LT_ERROR;

	if ((pPlaySoundInfo->m_dwFlags & PLAYSOUND_ATTACHED) && !pPlaySoundInfo->m_hObject)
		return LT_ERROR;

	pSoundName = pPlaySoundInfo->m_szSoundName;
	if (!fs_GetFileIdentifier(&m_FileMgr, pSoundName, 0, &hFile))
	{
		if (g_DebugLevel >= 2)
			dsi_ServerError("Missing sound file %s", pSoundName);

		return LT_MISSINGFILE;
	}

	// Spatialized sounds need sane radii.
	if (pPlaySoundInfo->m_dwFlags & (PLAYSOUND_AMBIENT | PLAYSOUND_3D))
	{
		pPlaySoundInfo->m_fOuterRadius =
			LTCLAMP(pPlaySoundInfo->m_fOuterRadius, 1.0f, 65000.0f);
		pPlaySoundInfo->m_fInnerRadius =
			LTCLAMP(pPlaySoundInfo->m_fInnerRadius, 1.0f, 65000.0f);
	}

	// Drop the control flags that ask for the defaults anyway.
	if ((pPlaySoundInfo->m_dwFlags & PLAYSOUND_CTRL_VOL) && pPlaySoundInfo->m_nVolume == 100)
		pPlaySoundInfo->m_dwFlags &= ~PLAYSOUND_CTRL_VOL;

	if ((pPlaySoundInfo->m_dwFlags & PLAYSOUND_CTRL_PITCH) && pPlaySoundInfo->m_fPitchShift == 1.0f)
		pPlaySoundInfo->m_dwFlags &= ~PLAYSOUND_CTRL_PITCH;

	// AVP2: and the two the AvP2 SDK added (Primal Hunt 0x0048dfe4 and
	// 0x0048dffa: the byte at +0x134, the dword at +0x138).
	if ((pPlaySoundInfo->m_dwFlags & PLAYSOUND_CTRL_TYPE) && pPlaySoundInfo->m_nUserSoundType == 0)
		pPlaySoundInfo->m_dwFlags &= ~PLAYSOUND_CTRL_TYPE;

	if ((pPlaySoundInfo->m_dwFlags & PLAYSOUND_USER_DATA) && pPlaySoundInfo->m_UserData == 0)
		pPlaySoundInfo->m_dwFlags &= ~PLAYSOUND_USER_DATA;

	if (pPlaySoundInfo->m_dwFlags & (PLAYSOUND_TIMESYNC | PLAYSOUND_TIME |
	    PLAYSOUND_GETHANDLE | PLAYSOUND_ATTACHED | PLAYSOUND_LOOP))
	{
		// The server has to keep this one.
		if (pPlaySoundInfo->m_dwFlags & PLAYSOUND_ATTACHED)
			pPlaySoundInfo->m_dwFlags &= ~PLAYSOUND_CLIENTLOCAL;

		// A kept, non-looping timed sound needs its length data up front.
		if ((pPlaySoundInfo->m_dwFlags & (PLAYSOUND_TIMESYNC | PLAYSOUND_TIME | PLAYSOUND_ATTACHED)) &&
		    !(pPlaySoundInfo->m_dwFlags & PLAYSOUND_LOOP) &&
		    (pTrack = GetSoundTrack(hFile)) == LTNULL)
		{
			return LT_ERROR;
		}

		StructBank* pBank = &m_Bank28;
		pSound = (SoundInstance*)sm_sb_Allocate(pBank);
		if (!pSound)
			return LT_ERROR;

		if (pPlaySoundInfo->m_dwFlags & PLAYSOUND_GETHANDLE)
			pPlaySoundInfo->m_hSound = (HLTSOUND)pSound;
		else
			pPlaySoundInfo->m_hSound = LTNULL;

		if (!pSound->Init(pPlaySoundInfo, m_Time, hFile, pTrack))
		{
			pPlaySoundInfo->m_hSound = LTNULL;

			// Hand the instance straight back to its bank. AVP2: through
			// the header's sb_Free, whose bank test survives (`test
			// %ebp,%ebp`, Primal Hunt 0x0048e0c5).
			sb_Free(pBank, pSound);
			return LT_ERROR;
		}

		dl_AddHead(&m_List54, &pSound->m_Link, pSound);
	}
	else
	{
		// Fire and forget: queue the play for the clients and be done.
		SoundRecord* pRecord = (SoundRecord*)sm_AddSoundRecord(this, 0);
		PLAYSOUNDINFO_COPY(pRecord->m_Info, *pPlaySoundInfo);
		pRecord->m_hFile = hFile;
	}

	// pSound is still LTNULL on the fire-and-forget path.
	hResult = (HLTSOUND)pSound;
	return LT_OK;
}

LTBOOL CServerMgr::Update(long flags, float fTime)
{
	int nUpdates;
	volatile long* pCount;
	int i;

	SetupGlobals();
	ProcessThreadMessages();

	// Everything below runs on accumulated server time.
	fTime += m_Unknown6b8;
	m_Unknown6d4 = fTime - m_Unknown6d8;
#ifdef AVP2_MODERN
	// M-P1-23: a game tick takes at least 10 ms (the clamp below), and the
	// client calls this every frame: above 100 frames a second the world
	// ran faster than real time (3x at 300).  So a tick runs only once
	// 10 ms have passed since the last one, measured from that tick; the
	// packets and the network still go every call.  Time that runs
	// backwards -- a new world re-bases it (m_Unknown6b8), leaving the last
	// tick's time ahead -- ticks at once and starts afresh, as the clamp
	// made of it; waiting for it stalled the new world for as long as the
	// old one had run.
	bool bP1Tick = m_Unknown198 != 0 || (flags & 1) || (m_ServerFlags & 1) ||
		m_Unknown6d4 >= 0.01f || m_Unknown6d4 < 0.0f;
	if (bP1Tick)
		m_Unknown6d8 = fTime;
#else
	m_Unknown6d8 = fTime;
#endif

	m_NetMgr.Update("Server: ", fTime, 1);

	g_Unknown585a0 = 0.0f;
	g_Unknown5872c = 0;
	g_Unknown58728 = 0;
	g_nIsectQueries = 0;
	g_nIsectTime = 0;
	g_FindSphereTicks = 0;
	g_FindSphereCount = 0;
	g_FindPolyTicksQ502 = 0;
	g_FindPolyCountQ502 = 0;

	if (sm_ProcessPackets(this))
		return LTFALSE;

	if (m_Unknown198 == 0)
	{
		if (!(flags & 1) && !(m_ServerFlags & 1))
		{
#ifdef AVP2_MODERN
		  // M-P1-23: see above
		  if (bP1Tick)
		  {
#endif
			// Fixed-step: run however many update ticks real time has banked.
			if (g_UpdatesPerSec != m_UpdatesPerSec)
			{
				m_Unknown6c4 = 0;
				m_Unknown6c8 = m_Time;
				m_Unknown6bc = m_Unknown6c0;
				m_Unknown6d0 = 0;
			}

			m_UpdatesPerSec = g_UpdatesPerSec;

			float fInv = 1.0f / g_UpdatesPerSec;
			m_FrameTime = g_TimeScale * fInv;
			m_Unknown6c0 = m_Unknown6bc + (float)m_Unknown6c4 * fInv;

			nUpdates = (int)((fTime - m_Unknown6c0) / fInv);
			if (nUpdates < 0)
				nUpdates = 0;

			m_FrameTime = m_Unknown6d4;
			m_FrameTime = LTCLAMP(m_FrameTime, 0.01f, 0.2f);
			m_Time += m_FrameTime;

			// The one-trip tick loop is a GOTO loop with a plain register
			// counter. lithtech keeps it (0x0047443c `xor %ebp,%ebp`,
			// 0x004744a3 `inc %ebp ; cmp $0x1,%ebp ; jl`) where VC6 deletes
			// every for/while/do spelling of a constant one-trip loop; the
			// old spelling ran the counter through the volatile fTime slot
			// and was 711 bytes against 698. Only the m_Unknown1a0 store
			// still goes through that slot (0x0047443e). server.dll
			// (0x10032e69) is identical either way. Left in lithtech (Primal
			// Hunt 0x0048b5d0: 761 of 761 bytes, 6 blocks): the busy-wait has a pre-test there
			// (0x0047446a) and not in server.dll, the float zero to
			// g_Unknown585a0 comes from %edi, and the slot store is
			// scheduled ahead of the m_Time store. Every while/goto/pointer
			// spelling of the wait tried costs server.dll 4 bytes.
			pCount = (volatile long*)&fTime;
			*pCount = m_Unknown1a0;
			i = 0;
		UpdateLoop:
			UpdateSounds(m_FrameTime);

			if ((m_Unknown1a0 & 4) && !(m_Unknown1a0 & 8))
			{
				// The famous hoisted-read busy-wait.
				do { } while (m_Unknown6dc == 0);
				m_Unknown6dc = m_Unknown6dc - 1;
			}

			if (m_ClassMgr.m_pServerShell)
				m_ClassMgr.m_pServerShell->Update(m_FrameTime);

			UpdateObjects();

			m_Unknown6d0++;
			i++;
			m_Unknown6d4 = 0.0f;
			if (i < 1)
				goto UpdateLoop;

			m_Unknown6c4 += nUpdates;
			m_Unknown6c0 = m_Unknown6bc + (float)m_Unknown6c4 * fInv;
#ifdef AVP2_MODERN
		  }
#endif
		}
		else
		{
			// Variable-step: one update with the real elapsed time.
			m_Unknown6d8 = fTime;
			sm_UpdateClients(this);
		}

		sm_FlushObjectQueue(this);
		RemoveDeadSounds();
	}

	if (g_ShowGameTime)
		dsi_ServerError("Game time: %.2f", m_Time);

#ifdef PRIMAL_HUNT
	// PH: the stats go through the varargs printer, each call popped on its
	// own (0x0048b871/0x0048b87b); AvP2 1.0.9.6 batches each pair into one
	// `add $0x10` (0x004837d0, 0x004837fc).
	if (g_ShowTickCounts)
	{
		dsi_ServerError("ILTServer::FindObjectsTouchingSphere ticks: %d", g_FindSphereTicks);
		dsi_ServerError("ILTServer::FindObjectsTouchingSphere count: %d", g_FindSphereCount);
	}

	if (g_ShowPolyFindTicksQ502)
	{
		dsi_ServerError("ILTServer::FindPoliesTouchingBox ticks: %d", g_FindPolyTicksQ502);
		dsi_ServerError("ILTServer::FindPoliesTouchingBox count: %d", g_FindPolyCountQ502);
	}
#else
	if (g_ShowTickCounts)
	{
		dsi_PrintStat("ILTServer::FindObjectsTouchingSphere ticks: %d", g_FindSphereTicks);
		dsi_PrintStat("ILTServer::FindObjectsTouchingSphere count: %d", g_FindSphereCount);
	}

	// AVP2: the poly query's pair, under its own cvar (AvP2 1.0.9.6
	// 0x004837d3).
	if (g_ShowPolyFindTicksQ502)
	{
		dsi_PrintStat("ILTServer::FindPoliesTouchingBox ticks: %d", g_FindPolyTicksQ502);
		dsi_PrintStat("ILTServer::FindPoliesTouchingBox count: %d", g_FindPolyCountQ502);
	}
#endif

	return LTTRUE;
}

// The seven packet handlers 0x10029ee7 installs. Four are already written
// under names of their own, with LTRESULT returns or an HCLIENT third
// argument, so the table stores cast them to the table's signature (the
// cast is free: each store is `movl $fn,slot`):
//   7  0x10029fb6  sm_ReadClientCommands (below)
//   8  0x10029f42  sm_Unknown29f42 (below)
//   10 0x1002a0f2  sub_1002a0f2 (below)
//   11 0x1002a11e  sm_GiveMessageToShell (below; Client* passed as HCLIENT)
int sub_1002a193(CServerMgr* pMgr, ServerMessage* pMsg, Client* pClient);
int sub_1002a0d0(CServerMgr* pMgr, ServerMessage* pMsg, Client* pClient);
LTRESULT sm_ReadClientCommands(CServerMgr* pMgr, ServerMessage* pMsg,
                               Client* pClient);
LTRESULT sm_Unknown29f42(CServerMgr* pMgr, ServerMessage* pMsg,
                         Client* pClient);
int sub_1002a167(CServerMgr* pMgr, ServerMessage* pMsg, Client* pClient);
int sub_1002a0f2(CServerMgr* pMgr, ServerMessage* pMsg, Client* pClient);
LTRESULT sm_GiveMessageToShell(CServerMgr* pMgr, ServerMessage* pMsg,
                               HCLIENT hClient);
int sub_100358baQ502(CServerMgr* pMgr, ServerMessage* pMsg, Client* pClient);

// 0x10029ee7 — clear the packet-handler table and install the seven handlers
// the server has. Another declaration in servermgr.h whose name does not
// match what the body does (there are no profile counters here); kept.
//
// Two things read off the bytes. The memset covers 0x400 bytes, so the table
// is 256 entries wide and not the 64 servermgr.h declares — the dispatch
// masks the id with 0x3f, so only the first 64 are ever reached, but the
// array is four times that. And the store order is 6, 7, 8, 10, 11, 9, 5,
// which is source order: VC6 keeps independent stores where it found them.
//
// IT SITS HERE, above UpdateObjects, for the memset alone. Neither build
// names an intrinsic setting for it: /O1 has no /Oi so the memset is the
// library call server.dll makes (0x10029ef3), /O2 has /Oi so it is the
// `rep stos` lithtech.exe has (0x00468f2d). Below UpdateObjects it would be
// inside the `#pragma intrinsic(memset)` region that function needs, and
// the `#pragma function(memset)` that closes that region for /O1's sake
// also closes it for /O2's, which is what cost this function there.
void sm_ResetProfileCounters()
{
	memset(g_PacketHandlers, 0, 0x400);

	g_PacketHandlers[6]  = sub_1002a0d0;
	g_PacketHandlers[7]  = (PacketHandlerFn)sm_ReadClientCommands;
	g_PacketHandlers[8]  = (PacketHandlerFn)sm_Unknown29f42;
	g_PacketHandlers[10] = sub_1002a0f2;
	g_PacketHandlers[11] = (PacketHandlerFn)sm_GiveMessageToShell;
	g_PacketHandlers[9]  = sub_1002a167;
	g_PacketHandlers[5]  = sub_1002a193;
	// AVP2: an eighth handler, 13, stored last (lithtech.exe 0x0047d215 ->
	// 0x0047d630; AvP2 1.0.9.6 0x00476bd5 likewise; server.dll 0x100356f0
	// -> 0x100358ba).
	g_PacketHandlers[13] = sub_100358baQ502;
}

// The shipped UpdateObjects pads the class name with an inlined rep-stos
// memset while PlaySound's PLAYSOUNDINFO_COPY stays a memcpy call, so the
// pragma names memset alone.
#pragma intrinsic(memset)

// NEAR MATCH (874 of 868 bytes, 251 of 250 instructions, 3 mnemonic blocks).
// The never-ticked retire block now sits out of line after the footer
// prints, behind the long je at 0x10032a57 with its `jmp` back at
// 0x10032b65, as shipped: the j-loop body is ONE nested positive guard
// (`if (!printed) { if (ticks) {...} else retire; }`) instead of two
// `continue`s. The only residual is strlen's result: shipped keeps it in eax
// into the intrinsic-memset padding (0x10032abd), ours copies it to edx.
// That extra `mov %eax,%edx` is the whole 1-instruction delta, and its two
// bytes push the `if (pBest)` skip (short `je` at 0x10032a91) past rel8
// range, which is the other four bytes. Probed without effect: nLen at
// function scope / top of the i-loop block, int with unsigned compare,
// sizeof bound, named pad count and/or pointer, assignment in the test,
// `sName + nLen`, reusing nLen (or nTicks) for the average. lithtech.exe
// shows the same eax-vs-edx choice (0x00473e4a). Allocation.
//
// lithtech.exe 0x00473bc0: 947 of 947, size-exact, 2 blocks, and the only
// residual is that same strlen register (shipped eax, ours edx). What made it
// size-exact: no tick counter at all in that build (see below), the census
// written through pStats (0x00473cfc loads the class chain before the
// index), and m_Ticks read at each use in the averages (0x00473d48 loads it
// after the m_nFrames store). Also probed for lithtech without effect: int
// nLen, `sName + nLen`, a named pad count, `30 > nLen`; strlen written out
// at each use is not CSE'd (969 bytes).
//
// PH: every print here goes through the varargs printer, each call popped on
// its own (0x0048b08c, 0x0048b099, 0x0048b0ac ... 0x0048b234), where AvP2
// 1.0.9.6 batches the header and footer prints (0x0048316b..0x00483181), as
// these fixed prototypes do. So in Primal Hunt's build the three names are
// the varargs printer itself. Primal Hunt's 0x0048aed0 is then 965 of 965,
// with only the strlen register above left (shipped eax, ours edx).
// server.dll 0x1003fdda (877, PH counter per object below) has the same
// residual, 879 against 877: the length is copied to %edx before the
// compare. Unmoved by an int length (worse: `jge`), the length at function
// scope, and `sName + nLen`.
#ifdef PRIMAL_HUNT
#define dsi_PrintString      dsi_ServerError
#define dsi_PrintClassRow    dsi_ServerError
#define dsi_PrintFrameTotals dsi_ServerError
#endif
void CServerMgr::UpdateObjects()
{
	char sName[32];
#if defined(LT_SERVER_BUILD) && !defined(PRIMAL_HUNT)
	CBaseCounter counter(0);
#endif
	LTLink* pCur;
	LTObject* pObject;
	CClassStats* pStats;
	int i, j;
	int nTotalObjects, nTotalUpdated;

	// Reset the per-class frame counts, and the running averages every
	// 600 frames.
	for (i = 0; i < m_ClassMgr.m_nClasses; i++)
	{
		m_ClassMgr.m_pClassStats[i].m_Ticks = 0;
		m_ClassMgr.m_pClassStats[i].m_nUpdated = 0;
		m_ClassMgr.m_pClassStats[i].m_nObjects = 0;
		m_ClassMgr.m_pClassStats[i].m_bPrinted = 0;
		if (m_ClassMgr.m_pClassStats[i].m_nFrames >= 600)
		{
			m_ClassMgr.m_pClassStats[i].m_nFrames = 0;
			m_ClassMgr.m_pClassStats[i].m_TotalTicks = 0;
			m_ClassMgr.m_pClassStats[i].m_MaxTicks = 0;
		}
	}

	// Run every active object's update, timing each one.
	LTLink* pListHead = &m_List8dc.m_Head;
	pCur = pListHead->m_pNext;
	while (pCur != pListHead)
	{
		pObject = (LTObject*)pCur->m_pData;
		pCur = pCur->m_pNext;

		if (pObject->m_ChangeFlags & 0x38)
			break;

		if (pObject->m_ChangeFlags & 4)
		{
			// Count the object's update timer down; when it runs out, the
			// deactivation flag goes up.
			if (g_bAutoDeactivate)
			{
				SObjData* pSData = pObject->m_pSData;
				if (pSData->m_UpdateTimer > 0.0f)
				{
					pSData->m_UpdateTimer -= m_FrameTime;
					if (pObject->m_pSData->m_UpdateTimer <= 0.0f)
						pObject->m_ChangeFlags |= 0x200;
				}
			}

			pStats = pObject->m_pSData->m_pClass->m_Stats[m_ClassMgr.m_nIndex];

			// lithtech.exe times nothing here: no counter is constructed and
			// the update is a bare call (0x00473cc9 -> 0x004697e0), so the
			// class tick counts it prints are always zero.
#ifdef LT_SERVER_BUILD
#ifdef PRIMAL_HUNT
			// PH: the counter is the update's own, constructed for each
			// object (server.dll 0x1003fecb `push $0 ... call`, after the
			// stats are found).
			CBaseCounter counter(0);
#endif
			cnt_StartCounter(&counter);
			sm_UpdateObject(this, pObject);
			pStats->m_Ticks += cnt_EndCounter(&counter);
#else
			sm_UpdateObject(this, pObject);
#endif

			pStats->m_nUpdated++;
		}
	}

	if (g_ShowClassTicks)
	{
		// Census: how many objects of each class are in the world.
		nTotalObjects = 0;
		pCur = pListHead->m_pNext;
		while (pCur != pListHead)
		{
			pObject = (LTObject*)pCur->m_pData;
			pStats = pObject->m_pSData->m_pClass->m_Stats[m_ClassMgr.m_nIndex];
			pStats->m_nObjects++;
			nTotalObjects++;
			pCur = pCur->m_pNext;
		}

		// Fold this frame into the running averages.
		for (i = 0; i < m_ClassMgr.m_nClasses; i++)
		{
			pStats = &m_ClassMgr.m_pClassStats[i];
			pStats->m_nFrames++;
			pStats->m_TotalTicks += pStats->m_Ticks;
			if (pStats->m_MaxTicks < pStats->m_Ticks)
				pStats->m_MaxTicks = pStats->m_Ticks;
		}

		nTotalUpdated = 0;
		g_TotalTicks = 0;

		dsi_PrintString("------------------------- Class Tick Counts -------------------------------------");
		dsi_PrintString("Class                (OBJECTS) Total Updated   (TICKS) Average    Max     Frame");
		dsi_PrintString("---------------------------------------------------------------------------------");

		// Selection sort by average ticks, printing as we go.
		for (i = 0; i < m_ClassMgr.m_nClasses; i++)
		{
			CClassStats* pBest = LTNULL;

			for (j = 0; j < m_ClassMgr.m_nClasses; j++)
			{
				pStats = &m_ClassMgr.m_pClassStats[j];
				if (pStats->m_bPrinted == 0)
				{
					if (pStats->m_TotalTicks != 0)
					{
						if (!pBest)
							pBest = pStats;

						if (pStats->m_TotalTicks / pStats->m_nFrames <=
						    pBest->m_TotalTicks / pBest->m_nFrames)
						{
							pBest = pStats;
						}
					}
					else
					{
						// Never ticked: retire the class without a row.
						pStats->m_bPrinted = 1;
					}
				}
			}

			if (pBest)
			{
				pBest->m_bPrinted = 1;

				strncpy(sName, pBest->m_pClass->m_pClassName, 32);
				sName[31] = 0;

				uint32 nLen = strlen(sName);
				if (nLen < 30)
					memset(&sName[nLen], ' ', 30 - nLen);

				dsi_PrintClassRow("%s| %3d |  %3d  |           %6d | %6d | %6d",
					sName, pBest->m_nObjects, pBest->m_nUpdated,
					pBest->m_TotalTicks / pBest->m_nFrames,
					pBest->m_MaxTicks, pBest->m_Ticks);

				g_TotalTicks += pBest->m_Ticks;
				nTotalUpdated += pBest->m_nUpdated;
			}
		}

		dsi_PrintString(" ");
		dsi_PrintString("FRAME TOTALS:");
		dsi_PrintFrameTotals("  (OBJECTS) Total: %d, Updated: %d,  (TICKS) %d",
			nTotalObjects, nTotalUpdated, g_TotalTicks);
		dsi_PrintString("---------------------------------------------------------------------------------");
		dsi_PrintString(" ");
	}
	else
	{
		g_TotalTicks = 0;
		for (i = 0; i < m_ClassMgr.m_nClasses; i++)
		{
			g_TotalTicks += m_ClassMgr.m_pClassStats[i].m_Ticks;
			m_ClassMgr.m_pClassStats[i].m_nFrames = 0;
			m_ClassMgr.m_pClassStats[i].m_TotalTicks = 0;
		}
	}
}
#ifdef PRIMAL_HUNT
#undef dsi_PrintString
#undef dsi_PrintClassRow
#undef dsi_PrintFrameTotals
#endif

void CServerMgr::UpdateSounds(float fFrameTime)
{
	LTLink* pCur = m_List54.m_Head.m_pNext;
	while (pCur != &m_List54.m_Head)
	{
		SoundInstance* pSound = (SoundInstance*)pCur->m_pData;
		if (!pSound)
			break;

		LTLink* pNext = pCur->m_pNext;

		// A killed sound stops timing unless something still owns it.
		if (!pSound->m_bKill || pSound->m_Unknown28)
			pSound->UpdateTimer(fFrameTime);

		pCur = pNext;
	}
}

void sm_UpdateClients(CServerMgr* pMgr)
{
	LTLink* pListHead;
	LTLink* pCur;

	pListHead = &pMgr->m_ClientList.m_Head;
	pCur = pListHead->m_pNext;
	while (pCur != pListHead)
	{
		sm_UpdateClient(pMgr, (Client*)pCur->m_pData);
		pCur = pCur->m_pNext;
	}
}

// 0x10032dab — a sound instance lets go of its slot. The manager's table
// entry goes back first; the mixer handle is simply forgotten.
//
// Defined above its first caller and `inline`: lithtech.exe has NO standalone
// copy of this body and expands it at all three call sites -- here,
// sm_FlushList44 (0x004765a8, the `mov 0xc(%esi),%eax` / `call 0x00473130`
// pair inline) and sm_EndWorldD -- while server.dll keeps 0x10032dab and
// calls it at every one. A member defined out-of-class is never an /Ob1
// candidate and VC6's inliner only looks forward, so both the keyword and
// the position are needed; /Os declines the 34-byte body, /Ot takes it.
// (`m_pUnknown0c` is uint16*, so the overload picked is servermgr.h's
// `sm_Free28(CServerMgr*, void*)` here exactly as it was further down.)
inline void SoundInstance::Term()
{
	if (m_pUnknown0c)
		sm_Free28(g_pServerMgr, m_pUnknown0c);

	m_hSample = LTNULL;
	m_pUnknown0c = LTNULL;
}

// 0x10029b24 — the object a sound is following. An attached sound holds a
// reference record and reads the object out of it; a client-local one holds
// the object directly. Anything else is not following an object at all.
//
// `inline` and defined here, above RemoveDeadSounds, for the same reason as
// Term() just above: lithtech.exe has no standalone copy and expands it in
// RemoveDeadSounds (0x474166: `testb $0x8,0x34(%esi)`, `mov 0x38(%esi)`,
// `mov 0x4(%eax)`, no call), while server.dll keeps 0x10029b24 and calls it.
inline LTObject* SoundInstance::GetObject()
{
	if (m_PlaySoundFlags & PLAYSOUND_ATTACHED)
	{
		if (!m_pObjectRef)
			return LTNULL;

		return m_pObjectRef->m_pObject;
	}

	if (m_PlaySoundFlags & PLAYSOUND_CLIENTLOCAL)
		return m_pObject;

	return LTNULL;
}

void CServerMgr::RemoveDeadSounds()
{
	LTLink* pCur = m_List54.m_Head.m_pNext;
	while (pCur != &m_List54.m_Head)
	{
		SoundInstance* pSound = (SoundInstance*)pCur->m_pData;
		if (!pSound)
			break;

		LTLink* pNext = pCur->m_pNext;

		if (pSound->m_bKill && pSound->m_Unknown2c == 0 && !pSound->m_Unknown28)
		{
			// An attached sound tells its object it is going away.
			if (pSound->m_PlaySoundFlags & PLAYSOUND_ATTACHED)
			{
				if (pSound->GetObject())
				{
					LTObject* pObject = pSound->GetObject();
					// The sound is the far end of a type-2 link off its
					// object; break every such link between the two.
					if (pObject)
						sm_BreakInterObjectLink(this, pObject, (LTObject*)pSound, 1);
				}
			}

			dl_RemoveAt(&m_List54, &pSound->m_Link);

			pSound->Term();

			// Hand the instance straight back to its bank. AVP2: through
			// the header's sb_Free, whose bank test survives (`lea
			// 0x28(%edi),%eax; cmp %ebp,%eax`, Primal Hunt 0x0048b4e9).
			sb_Free(&m_Bank28, pSound);
		}

		pCur = pNext;
	}
}

void sm_DrainObjectQueue(CServerMgr* pMgr)
{
	LTLink* pCur;

	pCur = pMgr->m_Link83c.m_pNext;
	while (pCur != &pMgr->m_Link83c)
	{
		LTObject* pObject = (LTObject*)pCur->m_pData;
		sm_RemoveObjectFromWorld(pMgr, pObject->m_pSData->m_pBaseClass);

		LTLink* pNext = pCur->m_pNext;

		// AVP2: the link is tied off after the unlink (CheapLTLink::Remove:
		// `mov %esi,0x4(%esi); mov %esi,(%esi)`, AvP2 1.0.9.6 0x00477dd0,
		// Primal Hunt 0x0047e77c), then goes back to the pool.
		pCur->Remove();
#ifdef PRIMAL_HUNT
		// PH: through the header's null-tested sb_Free (`test %esi`, then
		// the bank 0x004f0124 tested, 0x0047e77f..0x0047e78c); AvP2 1.0.9.6
		// has the two bare stores.
		sb_Free(&g_ObjQueueBank.m_Bank, pCur);
#else
		// The spent link goes back to the global pool through its prev slot.
		pCur->m_pPrev = SM_FREE_LINKS;
		SM_FREE_LINKS = pCur;
#endif

		pCur = pNext;
	}
}

void sm_ClearChangedObjects(CServerMgr* pMgr)
{
	LTObject* pObject = pMgr->m_pChangedObjects;
	while (pObject)
	{
		pObject->m_pSData->m_ClientChangeFlags = 0;
		pObject = pObject->m_pSData->m_pNextChanged;
	}

	pMgr->m_pChangedObjects = LTNULL;
}

void sm_ClearChangedSounds(CServerMgr* pMgr)
{
	SoundInstance* pSound = pMgr->m_pChangedSounds;
	while (pSound)
	{
		pSound->m_Unknown2c = 0;
		pSound = pSound->m_pNextChanged;
	}

	pMgr->m_pChangedSounds = LTNULL;
}

// The walk is a `for` whose cursor is initialised THROUGH a named head, and
// both halves of that matter. All three callee-saved registers are spoken for
// (pMgr, the cursor, and the inner loops' scratch), so the head cannot live in
// one and VC6 re-materialises `lea 0x7e8(%edi),%eax` in the preheader and
// again in the latch — the six-byte-bigger `for` shape of the rule
// "`for` rematerialises a loop invariant into both predecessors" — and
// initialising the cursor off the head instead of off the list rotates the
// pair the shipped way (pMgr in edi, the cursor in esi). Written as
// `pCur = pMgr->m_ClientList.m_Head.m_pNext; while (pCur != &...m_Head)` the
// instructions are identical and pMgr/pCur are swapped in every one of them.
void sm_ApplyChangeFlags(CServerMgr* pMgr)
{
	LTLink* pCur;
	LTLink* pHead;
	Client* pClient;
	LTObject* pObject;
	SoundInstance* pSound;

	pHead = &pMgr->m_ClientList.m_Head;

	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		pClient = (Client*)pCur->m_pData;
		if (pClient->m_State == 3)
		{
			pObject = pMgr->m_pChangedObjects;
			while (pObject)
			{
				pClient->m_pObjInfos[pObject->m_ObjectID].m_ChangeFlags |=
					pObject->m_pSData->m_ClientChangeFlags;
				pObject = pObject->m_pSData->m_pNextChanged;
			}

			// The sound's index is the same uint32 at +8 that
			// sm_SetSoundTrackChangeFlags reads, copied into a uint32 local
			// and narrowed with a (uint16) cast at the subscript. /O1 folds
			// that into one `movzwl` off memory (0x10032cb9); /O2 keeps the
			// full dword load and zero-extends the register with
			// `and $0xffff` (0x004740ae). Measured alternatives: a bare
			// `& 0xffff` mask gives mov/and on BOTH images (server near);
			// a `(uint16)` cast on the field itself, or a uint16 local,
			// makes /O2 narrow the load to `mov 8(%ecx),%cx` (lithtech near).
			pSound = pMgr->m_pChangedSounds;
			while (pSound)
			{
				uint32 nIndex = ((uint32*)pSound->m_pUnknown0c)[2];
				pClient->m_pObjInfos[(uint16)nIndex].m_ChangeFlags |= pSound->m_Unknown2c;
				pSound = pSound->m_pNextChanged;
			}
		}
	}

	sm_ClearChangedObjects(pMgr);
	sm_ClearChangedSounds(pMgr);
}

void sm_SendClientUpdates(CServerMgr* pMgr)
{
	LTLink* pListHead;
	LTLink* pCur;

	pListHead = &pMgr->m_ClientList.m_Head;
	pCur = pListHead->m_pNext;
	while (pCur != pListHead)
	{
		sm_WriteClientUpdate(pMgr, (Client*)pCur->m_pData);
		pCur = pCur->m_pNext;
	}

	pMgr->m_Unknown7fc = 0;
	pMgr->m_Unknown800 = 0;
}

void sm_FlushObjectQueue(CServerMgr* pMgr)
{
	sm_UpdateClients(pMgr);
	sm_ApplyChangeFlags(pMgr);

	// Processing a queued object can queue more, so drain until dry.
	while (pMgr->m_Link83c.m_pNext != &pMgr->m_Link83c)
		sm_DrainObjectQueue(pMgr);

	// AVP2: both changed lists are dropped before the sends (Primal Hunt
	// 0x0048b2d8, AvP2 1.0.9.6 0x00483e3d, server.dll 0x10040177).
	pMgr->m_pChangedObjects = LTNULL;
	pMgr->m_pChangedSounds = LTNULL;

	sm_SendClientUpdates(pMgr);
}

void sm_UpdateClient(CServerMgr* pMgr, Client* pClient)
{
	if (pClient->m_State == 1)
		sm_SetClientState(pMgr, pClient, 3);
	else if (pClient->m_State == 2)
		sm_UpdateClientState2(pMgr, pClient);

	sm_FlushClientPackets(pMgr, pClient);
}

// The packet comes back BY VALUE (msg_AllocPacket, servermsg.h): a frame
// temporary (`lea 0x18(%esp),%eax; push %eax` before the two
// sizes, 0x0047d087), copied into msg with an AddRef (0x0047d0b0) and
// destroyed with a Release (0x0047d0c2) under its own EH state. Identical on
// server.dll with that; for Primal Hunt's lithtech.exe see the NEAR note
// inside the loop.
int sm_ProcessPackets(CServerMgr* pMgr)
{
	CMessageRef msg;
	int nResult;

	// AVP2: the packet buffer is 0x44c bytes, not 0x1000 (Primal Hunt
	// 0x0047d082).
	msg.Init(msg_AllocPacket(0x44c, 0x44c));

	pMgr->m_NetMgr.StartGettingPackets();

	while (pMgr->m_NetMgr.GetPacket(msg.m_pMsg, 2))
	{
		// AVP2: the id is kept as a byte (`mov %dl,0x28(%esp)`, 0x0047d0ec).
		uint8 nPacketID = msg.m_pMsg->m_pData[0] & 0x3f;

		// The handler SLOT, not the index, is what the loop keeps: both
		// shipped builds materialise `&g_PacketHandlers[nPacketID]` in a
		// callee-saved register and reach the handler through it --
		// `lea 0x100587ac(,%eax,4),%esi ... call *(%esi)` at 0x10029e65 /
		// 0x10029e88, and `lea 0x4b44bc(,%edx,4),%edi ... call *(%edi)` at
		// 0x00468e92 / 0x00468eb5. Indexing the array twice gets there at
		// /O1 (the address is the common subexpression either way, which is
		// why server.dll already matched) but NOT at /O2, where /Ot keeps
		// the INDEX in edi instead and re-indexes at the call. Naming the
		// slot leaves only one candidate.
		PacketHandlerFn* ppHandler = &g_PacketHandlers[nPacketID];

		// NEAR on Primal Hunt (0x0047d060, 346 vs 349 bytes): shipped reads
		// the connection ONCE, between the handler test and its `je`
		// (0x0047d109), and both arms push that %eax.  A connection local
		// above the test gets lithtech.exe to 346/346 with 3 blocks (the
		// handler in %ecx, the connection loaded first) but costs
		// server.dll this function, which matches as written; inside the
		// first arm or above nPacketID is no better.
		if (*ppHandler)
		{
			Client* pClient;
			if (msg.m_pMsg->m_pConnection)
				pClient = sm_FindClientByConnection(pMgr, msg.m_pMsg->m_pConnection);
			else
				pClient = LTNULL;

			nResult = (*ppHandler)(pMgr, msg.m_pMsg, pClient);
			if (nResult)
			{
#ifdef AVP2_DEBUG
				AVP2SRC_LOG(("packet %d: its handler failed (%d), the server stops", nPacketID, nResult));
#endif
				pMgr->m_NetMgr.EndGettingPackets();
				return nResult;
			}
		}
		else
		{
			// No handler: hand the raw packet to its client, if we know one.
			Client* pClient = sm_FindClientByConnection(pMgr, msg.m_pMsg->m_pConnection);
			if (pClient)
				xfer_HandleMessage((CClientFiles*)pClient->m_pUnknown18C, msg.m_pMsg);
		}
	}

	pMgr->m_NetMgr.EndGettingPackets();
	return 0;
}

Client* sm_FindClientByConnection(CServerMgr* pMgr, NetConnection* pConn)
{
	LTLink* pCur = pMgr->m_ClientList.m_Head.m_pNext;
	while (pCur != &pMgr->m_ClientList.m_Head)
	{
		Client* pClient = (Client*)pCur->m_pData;
		if (pClient->m_pConnection == pConn)
			return pClient;

		pCur = pCur->m_pNext;
	}

	return LTNULL;
}

int sm_SetClientState(CServerMgr* pMgr, Client* pClient, int nState)
{
	if (pClient->m_State == nState)
		return 1;

	if (nState == 3)
	{
		// Entering the world can be refused.
		if (sm_PrepClientEnterWorld(pMgr, pClient))
		{
			sm_ClientEnterWorld2(pMgr, pClient);
			return 1;
		}

		return 0;
	}

	if (pClient->m_State == 3)
		sm_ClientExitWorld(pMgr, pClient);

	pClient->m_State = nState;
	return 1;
}

// Both defined at the foot of this file, under the signatures the shipped
// bodies actually have rather than the ones servermgr.h declares.
LTBOOL sm_FinishUpdatePacket(CUpdateContext* pCtx, class ServerMessage* pMsg,
                             int nA, int nThreshold);                       // 0x100278ad
void   sm_VisGatherObjects(LTLink* pListHead, LTObject*** pppList,
                           uint32* pnCount);                                // 0x100283ff

// 0x100285de — whether this client's update can be skipped this frame: a
// backed-up queue either way, or the net layer saying it is saturated.
//
// Defined here rather than after its caller, and `inline`: lithtech.exe's
// sm_WriteClientUpdate opens with this body expanded (the connection test,
// both queue depths against g_MaxQueueSize with `ja`, then the ack-timeout
// call) where server.dll calls 0x100285de. VC6's inliner only looks forward,
// so the original had it above the caller and marked inline; /Os declines the
// expansion -- which is why server.dll still has both the call and the
// out-of-line copy -- and /Ot takes it.
inline int sm_SkipClientUpdate(Client* pClient)
{
	NetConnection* pConn = pClient->m_pConnection;

	if (pConn)
	{
		uint32 nMessages = pConn->m_GuaranteedQueue.m_nMessages;
		if (nMessages > g_MaxQueueSize)
			return 1;

		if (pConn->m_SendQueue.m_nMessages > g_MaxQueueSize)
			return 1;

		if (pConn->CheckAckTimeout())
			return 1;
	}

	return 0;
}

// 0x10024cac and 0x10024cad — what a visibility query does by default: a
// one-byte `ret`, and a predicate that answers yes. Declared here and
// DEFINED further down, beside the gather that is their reason to exist;
// only their addresses are needed above.
static void vq_Ignore(void* pUser, void* pObject);
static void vq_IgnoreToo(void* pUser, void* pObject);
static int  vq_Accept(void* pUser, void* pObject);

// 0x100283c6 — a query centred nowhere with no radius, every slot defaulted.
// The caller fills in the two callbacks and the context afterwards; what the
// constructor leaves is the shape those go into.
//
// Defined here rather than beside the callbacks, and `inline`, for exactly
// the reason sm_SkipClientUpdate above is: all THREE of its users --
// sm_WriteClientUpdate (0x00465639), sm_SendSFXMessage (0x00476929) and
// sm_PingObjects -- carry the body EXPANDED in lithtech.exe, the two
// function-pointer immediates 0x00491b40 and 0x00432030 and all, while
// server.dll calls 0x100283c6 at every one of them (sm_SendSFXMessage's
// `lea -0x5c(%ebp),%ecx; call` at 0x10034a73). A constructor defined
// out-of-class is the one shape /Ob1 never expands, and VC6's inliner only
// looks forward, so the original had it above its first caller and marked
// inline; /Os declines a 57-byte body and /Ot takes it.
//
// The position is zeroed by `LTVector::Init()` (all three parameters
// default to 0.0f), not by three component assignments, and that is the
// whole difference between this and the near it used to be: **an inlined
// member function's body is a scheduling barrier**. Written as three
// separate `= 0.0f` statements, VC6 treats all four float stores as one
// group and pulls two of the following integer stores into each `fldz`'s
// shadow (two fillers per pair, evenly spread). With Init() the three
// position stores are a unit no integer store may enter, so they come out
// back-to-back exactly as shipped and only the radius store gets fillers.
// Statement order is *not* the lever -- moving the radius assignment
// anywhere in the list produces identical bytes either way.
inline CVisQuery::CVisQuery()
{
	m_Position.Init();

	m_Radius = 0.0f;

	m_Fn1 = vq_Ignore;
	m_Fn2 = LTNULL;

	m_pUser = LTNULL;
	m_pFn20 = vq_IgnoreToo;

	m_Unknown00 = 0;

	m_pFn24 = vq_Accept;
	m_Unknown28 = 0;
}

#ifndef LT_SERVER_BUILD
// sm_WriteObjectUpdate's body, `inline` for lithtech.exe (see the note over
// sub_10027918 further down). Defined HERE, above both of its callers,
// because VC6's inliner is forward-only: sm_WriteClientUpdate's forced-update
// loop carries this body expanded too (0x00465b1d..0x00465c4c -- the id-list
// grow through dalloc/dfree at 0x00465b5a/0x00465b8c, the size estimate,
// then 0x00464840, 0x00467540, 0x00464da0 and the 0x1000 store), where
// server.dll calls sm_WriteObjectUpdate (0x100279d9).
int sub_100291c7(CServerMgr* pMgr, Client* pClient, LTObject* pObject,
                  CObjInfo* pInfo, class ServerMessage* pMsg, uint32* pSize);      // 0x100291c7
void sub_10027ad0(CUpdateContext* pCtx, LTObject* pObject,
                  CObjInfo* pInfo);                                 // 0x10027ad0

inline void sm_WriteObjectUpdateInline(CUpdateContext* pCtx, LTObject* pObject,
                                       CObjInfo* pInfo)
{
	uint32 nSize;

	if (pInfo->m_ChangeFlags & 0x1000)
		return;

	if (g_pFillIDList->m_nIDs >= g_pFillIDList->m_nMaxIDs)
	{
		uint16* pNew = (uint16*)dalloc(
			(g_pFillIDList->m_nMaxIDs + 200) * sizeof(uint16));

		memcpy(pNew, g_pFillIDList->m_pIDs,
			g_pFillIDList->m_nIDs * sizeof(uint16));

		dfree(g_pFillIDList->m_pIDs);

		g_pFillIDList->m_pIDs = pNew;
		g_pFillIDList->m_nMaxIDs += 200;
	}

	g_pFillIDList->m_pIDs[g_pFillIDList->m_nIDs] = pObject->m_ObjectID;
	g_pFillIDList->m_nIDs++;

	// AVP2: no estimate any more -- the writer measures itself first, with
	// no message and the size (server.dll 0x10032d14..0x10032d25, the size
	// homed in pInfo's slot; AvP2 1.0.9.6 alike), then writes; and bit
	// 0x4000 of the flags (the velocity was sent) survives the reset
	// (`and $0x4000; or $0x10,%ah`, 0x10032d5a).
	nSize = 0;

	sub_100291c7(pCtx->m_pMgr, pCtx->m_pClient, pObject, pInfo,
		LTNULL, &nSize);

	sm_FinishUpdatePacket(pCtx, pCtx->m_Msg1.m_pMsg, 8, nSize);

	sub_100291c7(pCtx->m_pMgr, pCtx->m_pClient, pObject, pInfo,
		pCtx->m_Msg1.m_pMsg, LTNULL);

	sub_10027ad0(pCtx, pObject, pInfo);

	pInfo->m_ChangeFlags = (uint16)((pInfo->m_ChangeFlags & 0x4000) | 0x1000);
}
#endif // !LT_SERVER_BUILD

// 0x10027e8b / 0x00465610 (2,028 bytes; the row ends at 0x00465dfc -- the
// next start, 0x00465e00, is the visibility callback stored at 0x00465a07).
// lithtech.exe has no tick counters here (no cnt_Start/EndCounter call and
// no add into 0x587a0's twin), expands the context's constructor and
// destructor, and expands sm_WriteObjectUpdate in the forced-update loop.
// RESIDUAL (lithtech, 2,109 vs 2,028 bytes, near): at the three early
// returns shipped CALLS the member release 0x004666f0 (0x0046569f,
// 0x004656c0, 0x004656fe, then the shared 0x0046570a/0x00465713 tail) and
// expands it only at the final exit; we expand it at all four. That is the
// /O2 inline budget (spent from the end backwards): wrapping the
// two flag tests at the foot in inline helpers already turns the early
// cursor releases into calls (measured, 2,056 bytes), so the original spent
// more budget below them than this source does. What it spent it on is not
// visible, and invented wrappers are not kept.
//
// Primal Hunt: byte-identical in lithtech.exe (0x00478a50, 2,277 bytes).
// NEAR in server.dll (0x1003313a, 1,393 vs 1,390): shipped puts the SECOND
// msg_AllocPacket temporary in pClient's dead slot as well as the first
// (`lea 0xc(%ebp)` at 0x10033319 and 0x10033353); ours gives the second a
// frame slot of its own (-0x48), which moves every frame offset below it.
void sm_Unknown27c71(CServerMgr* pMgr, Client* pClient, ServerMessage* pMsg,
                     uint32* pSize);                               // 0x10032f0f

void sm_WriteClientUpdate(CServerMgr* pMgr, Client* pClient)
{
	CUpdateContext ctx;
	CVisQuery query;
	LTObject* pObjects[MAX_FORCEUPDATE_OBJECTS];
	ForceUpdate forceUpdate;
	LTObject* pViewed;
	LTLink* pCur;
	uint32 i;
	uint32 nTrailerSize;

	if (sm_SkipClientUpdate(pClient))
		return;

	if (pClient->m_State != 3)
		return;

	if (!(pClient->m_ClientFlags & 0x12))
	{
		// Not time yet?
		if (!pClient->m_RateControl.Update())
			return;

		// How much of last frame's output made it out.
		float fRate;
		if (pMgr->m_Unknown7fc)
			fRate = 1.0f - (float)pMgr->m_Unknown800 / (float)pMgr->m_Unknown7fc;
		else
			fRate = 1.0f;

		if (g_DebugLevel > 0)
		{
			if (fRate < 0.999f)
				dsi_ServerError("Overflowing at %0.0f percent", (1.0f - fRate) * 100.0f);
		}

		// Scale down further when the whole server is over its bandwidth.
		if (g_NetBandwidth > 0)
		{
			float* pBandwidth = &pMgr->m_NetMgr.m_RateE0.m_Value;
			float fRatio = pBandwidth[0] / pBandwidth[1];
			float fBandwidth = (float)g_NetBandwidth;
			float fUsage = fRatio / fBandwidth;
			if (fUsage > 0.75f)
			{
				fUsage += 0.25f;
				if (fUsage >= 2.0f)
					fUsage = 2.0f;

				float fScale = 2.0f - fUsage;
				fRate *= fScale * fScale;
			}
		}

		// The scaled rate is its own local: lithtech.exe gives it a frame
		// slot of its own (0x14(%esp), 0x00478cc9) and puts fNew in the
		// first rate's (0x10(%esp), 0x00478d08).
		float fTarget = fRate * pClient->m_Unknown128;
		if (2.0f > fTarget)
			fTarget = 2.0f;

		// Ease the governor toward the new rate.
		float fCur = pClient->m_RateControl.GetRate();
		float fNew;
		if (fTarget > fCur)
			fNew = fCur + (fTarget - fCur) * 0.01f;
		else
			fNew = (fCur + fTarget) * 0.5f;

		if (g_DebugLevel > 0)
		{
			if ((float)fabs(fNew - fTarget) > 0.1f)
				dsi_ServerError("Connection %d: choked update rate %0.0f", pClient->m_ClientID, fNew);
		}

		pClient->m_RateControl.SetRate(fNew);
	}

	pClient->m_ClientFlags &= ~0x10;

	ctx.m_pMgr = pMgr;
	ctx.m_pClient = pClient;

	((CMsgCursorView*)&ctx.m_Cursors[1])->Init(ctx.m_Msg1.Init(msg_AllocPacket(0x44c, 0x44c)));
	((CMsgCursorView*)&ctx.m_Cursors[0])->Init(ctx.m_Msg2.Init(msg_AllocPacket(0x44c, 0x44c)));

	ctx.m_Unknown18 = 0;
	ctx.m_bLocal = (pClient->m_ClientFlags >> 9) & 1;

	sm_WriteUpdateHeader(pMgr, &ctx, pClient);
	// AVP2: NOLF's first counter (around the whole update) is gone, and the
	// second is constructed where it starts (server.dll 0x100333fb, after
	// the id-list swap; AvP2 1.0.9.6 0x10030b7d alike).
	sm_Unknown26755(pMgr, pClient);

	// Swap the sent-id lists: clear last frame's sent bits, refill the other.
	ClientIDList* pOldList = &pClient->m_IDLists[pClient->m_CurIDList];
	ClientIDList* pNewList = &pClient->m_IDLists[pClient->m_CurIDList == 0];
	g_pFillIDList = pNewList;
	for (i = 0; i < pOldList->m_nIDs; i++)
		pClient->m_pObjInfos[pOldList->m_pIDs[i]].m_ChangeFlags &= ~0x1000;
	pNewList->m_nIDs = 0;

#ifdef LT_SERVER_BUILD
	CBaseCounter counter2(0);
	cnt_StartCounter(&counter2);
#endif

	// Gather what this client can see.
	query.m_Position = pClient->m_Position;
	query.m_Radius = 10000.0f;
	query.m_Fn1 = sm_VisCallbackA;
	query.m_Fn2 = (int (*)(void*, void*))sm_VisGatherObjects;
	query.m_pUser = &ctx;
	((IObjectQuery*)&pMgr->m_Unknown210)->FindObjects(&query);

	sm_WriteChangedObjects(pMgr, &pMgr->m_Unknown390, &ctx);

#ifdef LT_SERVER_BUILD
	g_Unknown587a0 += cnt_EndCounter(&counter2);
#endif

	if (g_Unknown5845c)
		g_Unknown5845c = 0;

	// The forced set: the viewed object, the sky, and the game's own picks.
	forceUpdate.m_Objects = (HOBJECT*)pObjects;
	forceUpdate.m_nObjects = 0;

	if (pClient->m_pViewClient)
		pViewed = pClient->m_pViewClient->m_pUnknown3AC;
	else
		pViewed = pClient->m_pUnknown3AC;

	if (pViewed)
	{
		forceUpdate.m_Objects[0] = (HOBJECT)pViewed;
		forceUpdate.m_nObjects = 1;
	}

	for (i = 0; i < 30; i++)
	{
		if (pMgr->m_SkyObjects[i] != 0xffff)
		{
			LTObject* pObject = sm_FindObjectByID(pMgr, pMgr->m_SkyObjects[i]);
			if (pObject)
				forceUpdate.m_Objects[forceUpdate.m_nObjects++] = (HOBJECT)pObject;
		}
	}

	if (pViewed)
		pViewed->m_pSData->m_pBaseClass->EngineMessageFn(MID_GETFORCEUPDATEOBJECTS, &forceUpdate, 0.0f);

	for (i = 0; i < forceUpdate.m_nObjects; i++)
	{
		LTObject* pObject = (LTObject*)forceUpdate.m_Objects[i];
		if (pObject)
		{
			CObjInfo* pInfo = &pClient->m_pObjInfos[pObject->m_ObjectID];
			if (!(pInfo->m_ChangeFlags & 0x1000))
#ifdef LT_SERVER_BUILD
				sm_WriteObjectUpdate(&ctx, pObject, pInfo);
#else
				sm_WriteObjectUpdateInline(&ctx, pObject, pInfo);
#endif
		}
	}

	// Flush the client's queued messages into the guaranteed stream.
	LTLink* pNext;
	for (pCur = pClient->m_List3CC.m_Head.m_pNext;
	     pCur != &pClient->m_List3CC.m_Head;
	     pCur = pNext)
	{
		CQueuedMsg* pRec;
		pNext = pCur->m_pNext;
		pRec = (CQueuedMsg*)pCur->m_pData;

		sm_FinishUpdatePacket(&ctx, ctx.m_Msg1.m_pMsg, 8, 0x23);
		sm_WriteQueuedMessage(pMgr, pRec, pClient, ctx.m_Msg1.m_pMsg);

		dl_RemoveAt(&pClient->m_List3CC, pCur);
		pRec->Release();
	}

	// AVP2: no 0x10-byte close-off between the removed objects and the id
	// list (server.dll 0x100335c4 -> 0x100335d0).
	sm_WriteRemovedObjects(&ctx, ctx.m_Msg1.m_pMsg);
	sm_WriteIDList(&ctx, pOldList);

	// AVP2: the trailer is measured, the packet closed to fit, then written
	// (0x100335d5..0x100335fd), the size homed at -0x44(%ebp).
	nTrailerSize = 0;
	sm_Unknown27c71(pMgr, pClient, LTNULL, &nTrailerSize);
	sm_FinishUpdatePacket(&ctx, ctx.m_Msg2.m_pMsg, 0xa, nTrailerSize);
	sm_Unknown27c71(pMgr, pClient, ctx.m_Msg2.m_pMsg, LTNULL);

	ctx.m_Msg1.m_pMsg->m_MsgFlags |= 8;
	ctx.m_Msg2.m_pMsg->m_MsgFlags |= 0x10;

	if (ctx.m_Msg1.m_pMsg->m_Unknown34 - 1 > 1)
		sm_FinishUpdatePacket(&ctx, ctx.m_Msg1.m_pMsg, 8, -1);

	sm_FinishUpdatePacket(&ctx, ctx.m_Msg2.m_pMsg, 0xa, -1);

	pClient->m_CurIDList = (pClient->m_CurIDList == 0);

	if (pClient->m_ClientFlags & 0x20)
	{
		pClient->m_ClientFlags &= ~0x20;
		sm_SendSkyInfo(pMgr, pClient);
	}

	if (pClient->m_ClientFlags & 0x400)
	{
		pClient->m_ClientFlags &= ~0x400;
		sm_Unknown2873d(pMgr, pClient);
	}
}

void sm_MarkSkyChanged(CServerMgr* pMgr)
{
	LTLink* pCur = pMgr->m_ClientList.m_Head.m_pNext;
	while (pCur != &pMgr->m_ClientList.m_Head)
	{
		((Client*)pCur->m_pData)->m_ClientFlags |= 0x20;
		pCur = pCur->m_pNext;
	}
}

// The lea this used to carry was the single tail-hosted exit, and nothing
// else: with the body wrapped in one `if` and a `break` out of the search,
// every revision materialized &m_ChangeFlags and stored through it (104
// bytes against 102). Written with the returns INLINE -- a guard clause for
// the flag, a `return` where the `break` was, and the trailing one -- VC6
// runs its address CSE before it tail-merges the three, so the store keeps
// its displacement and the merge still produces the shipped single exit.
// Byte-identical on both images.
LTRESULT sm_RemoveObjectFromSky(CServerMgr* pMgr, LTObject* pObject)
{
	uint32 i;
	uint32 nFlags;
	uint16 nID;

	nFlags = pObject->m_ChangeFlags;
	if (~nFlags & 0x100)
		return LT_OK;

	pObject->m_ChangeFlags = nFlags & ~0x100;
	sm_MarkSkyChanged(pMgr);

	nID = pObject->m_ObjectID;
	for (i = 0; i < 30; i++)
	{
		if (g_pServerMgr->m_SkyObjects[i] == nID)
		{
			pMgr->m_SkyObjects[i] = 0xffff;
			sm_MarkSkyChanged(g_pServerMgr);
			return LT_OK;
		}
	}

	return LT_OK;
}

LTRESULT sm_RemoveObjectFromWorld(CServerMgr* pMgr, BaseClass* pClass)
{
	LTObject* pObject = (LTObject*)pClass->m_hObject;
	if (!pObject)
		RETURN_ERROR(1, "sm_RemoveObjectFromWorld", LT_ERROR);

	if (pObject->m_pSData)
	{
		pObject->m_ChangeFlags |= 1;

		// Unless the world is going down whole, attached children get told.
		if (~pMgr->m_Unknown1a0 & 1)
		{
			Attachment* pAttachment = pObject->m_pAttachments;
			while (pAttachment)
			{
				LTObject* pChild = sm_FindObjectByID(pMgr, pAttachment->m_nChildID);
				if (pChild)
				{
					pChild->m_pSData->m_pBaseClass->EngineMessageFn(
						MID_PARENTATTACHMENTREMOVED, LTNULL, 0.0f);
				}

				pAttachment = pAttachment->m_pNext;
			}
		}

		if (pObject->m_ChangeFlags & 0x100)
		{
			sm_MarkSkyChanged(pMgr);
			sm_RemoveObjectFromSky(pMgr, pObject);
			pObject->m_ChangeFlags &= ~0x100;
		}

		// A client's own object refuses to go.
		if (pObject->m_pSData->m_pOwningClient)
			RETURN_ERROR(1, "sm_RemoveObjectFromWorld", LT_TRIEDTOREMOVECLIENTOBJECT);

		sm_BreakAttachments(pMgr, pObject, 0, 1);

		if (pObject->m_pSData->m_Unknown28)
		{
			sm_Free28(pMgr, pObject->m_pSData->m_Unknown28);

			// Free the object's id.
			pMgr->m_Unknown8fc.m_pArray[pObject->m_ObjectID].m_nFlags = 0;
			pMgr->m_Unknown8fc.m_pArray[pObject->m_ObjectID].m_pRecord = LTNULL;
		}

		obj_UnlinkTrackersA(pObject);
		obj_UnlinkTrackersB(pObject);
		obj_FreeData(pObject);
		sm_ObjectRemoved(pMgr, pObject);

		pObject->m_pSData = LTNULL;
	}

	sm_RemoveFromWorldTree(pMgr, pObject);
	g_Unknown58bec -= SM_OBJECT_MEMORY;

	return un390_RemoveObject(&pMgr->m_Unknown390, pObject);
}

LTRESULT sm_UpdateClientState2(CServerMgr* pMgr, Client* pClient)
{
	CMessageRef msg;
	LTLink* pListHead;
	LTLink* pCur;
	CObjInfo* pInfo;

	// Stage one: tell the client which files it needs.
	if (pClient->m_Unknown3BC == 1 || (pClient->m_ClientFlags & 2))
	{
		uint32 i, j;

		msg.Init(msg_AllocPacket(0x44c, 0x44c));

		for (i = 0; i < pMgr->m_nFileTrees; i++)
		{
			CFileList* pList = pMgr->m_pFileTrees[i]->m_pList;
			for (j = 0; j < pList->m_nFiles; j++)
			{
				if (((CFileRec*)(pList->m_pFiles + j * 0x24))->m_nFlags)
				{
					{
						ServerMessage* pMsg = msg.m_pMsg;
						pMsg->m_Unknown38 = 1;
						pMsg->m_Unknown34 = 1;
					}
					msg.m_pMsg->WriteWord((uint16)i);
					msg.m_pMsg->WriteWord((uint16)j);
					{
						// A named byte: server.dll homes it in a frame slot
						// and pushes the slot (0x1003191a, 0x1003191d),
						// which is what the shipped 0xc frame is for.
						uint8 nFlags = (uint8)((CFileRec*)(pList->m_pFiles + j * 0x24))->m_nFlags;

						msg.m_pMsg->WriteByte(nFlags);
					}
					sm_SendToClient(pMgr, pClient, 0x13, msg.m_pMsg, 0, 0x80);
				}
			}
		}

		sm_SendWorldInfo(pMgr, pClient);
		pClient->m_Unknown10C = 0;
		sm_SendObjectInfo2(pMgr, pClient);
		sm_SendSkyInfo(pMgr, pClient);

		pClient->m_Unknown3BC = 2;
	}

	// Stage two: put the client's object into the world.
	if (pClient->m_Unknown3BC == 3 || (pClient->m_ClientFlags & 2))
	{
		msg.Init(msg_AllocPacket(0x44c, 0x44c));

		BaseClass* pClass = pMgr->m_ClassMgr.m_pServerShell->OnClientEnterWorld(
			(HCLIENT)pClient, pClient->m_pClientData, pClient->m_ClientDataLen);
		if (!pClass)
		{
			dsi_PrintString("Error: ServerShell::OnClientEnterWorld returned LTNULL!");
			return LT_ERROR;
		}

		pClient->m_pUnknown3AC = (LTObject*)pClass->m_hObject;
		pClient->m_pUnknown3AC->m_pSData->m_pOwningClient = pClient;

		// The object is spoken for now: pull its pending-announce record.
		pListHead = &pMgr->m_List8ec.m_Head;
		pCur = pListHead->m_pNext;
		while (pCur != pListHead)
		{
			CUnknown8ECRec* pRec = (CUnknown8ECRec*)pCur->m_pData;
			if (pRec->m_nObjectID == pClient->m_pUnknown3AC->m_ObjectID)
			{
				pClient->m_pUnknown3AC->m_ChangeFlags &= ~0x80;
				dl_RemoveAt(&pMgr->m_List8ec, &pRec->m_Link);
				dfree(pRec);
				break;
			}

			pCur = pCur->m_pNext;
		}

		if (pClient->m_pViewClient == LTNULL)
		{
			{
				ServerMessage* pMsg = msg.m_pMsg;
				pMsg->m_Unknown38 = 1;
				pMsg->m_Unknown34 = 1;
			}
			// WriteType<uint16>, expanded here and ONLY here in this
			// function: lithtech.exe calls the instantiation three times in
			// the file loop above (0x0046319a, 0x004631a2 word, 0x004631b8
			// byte) and expands this one (0x00463370..0x0046341e). The
			// inliner's budget is per call site, so the three stay spelled
			// as the wrapper.
			((CPacket*)msg.m_pMsg)->WriteType(pClient->m_pUnknown3AC->m_ObjectID);
			sm_SendToClient(pMgr, pClient, 7, msg.m_pMsg, 1, 0x80);
		}

		pClient->m_IDLists[1].m_nIDs = 0;
		pClient->m_IDLists[0].m_nIDs = 0;

		// Every updating object starts with its fresh change flags...
		pListHead = &pMgr->m_List8dc.m_Head;
		pCur = pListHead->m_pNext;
		while (pCur != pListHead)
		{
			LTObject* pObject = (LTObject*)pCur->m_pData;
			pInfo = &pClient->m_pObjInfos[pObject->m_ObjectID];
			pInfo->m_ChangeFlags = sm_GetObjectChangeFlags(pMgr, pObject);
			pCur = pCur->m_pNext;
		}

		// ...and every live sound plays from the top.
		pCur = pMgr->m_List54.m_Head.m_pNext;
		while (pCur != &pMgr->m_List54.m_Head)
		{
			SoundInstance* pSound = (SoundInstance*)pCur->m_pData;
			pInfo = &pClient->m_pObjInfos[*(uint32*)&pSound->m_pUnknown0c[4]];
			// Change flags first: /O1 normalises the pair to (byte, word)
			// -- server.dll stores 0x2 with `andb $0` BEFORE the word --
			// while /O2 keeps source order, and lithtech.exe has the word
			// first. Everything else in this 978-byte function is already
			// byte-identical in both images, so the order can only come
			// from the source.
			pInfo->m_ChangeFlags = 3;
			pInfo->m_Unknown02 = 0;
			pCur = pCur->m_pNext;
		}

		pClient->m_State = 3;
	}

	return LT_OK;
}



// 0x10032617 — what the string bank reports for anything still alive at
// shutdown.
void sm_ErrorHook(char* pString)
{
	if (g_DebugLevel >= 1)
		dsi_ServerError("Unfreed (server) string: %s", pString);
}

// 0x10031f2f — take a link out of wherever it is and put it on the manager's
// spare list.
void sm_Free28(CServerMgr* pMgr, LTLink* pLink)
{
	LTLink* pHead;

	pLink->m_pPrev->m_pNext = pLink->m_pNext;
	pLink->m_pNext->m_pPrev = pLink->m_pPrev;
	pLink->TieOff();

	pHead = &pMgr->m_Link8c4;
	pLink->m_pPrev = pHead;
	pLink->m_pNext = pHead->m_pNext;
	pLink->m_pNext->m_pPrev = pLink;
	pLink->m_pPrev->m_pNext = pLink;
}

// 0x1002acd3 — an object by its id. The map is indexed straight by id and a
// slot only counts when its flag says the object is live.
LTObject* sm_FindObjectByID(CServerMgr* pMgr, uint16 nID)
{
	CObjectMapEntry* pEntry;

	if (nID >= pMgr->m_Unknown8fc.m_nElements)
		return LTNULL;

	pEntry = &pMgr->m_Unknown8fc.m_pArray[nID];
	if (pEntry->m_nFlags == 1)
		return (LTObject*)pEntry->m_pRecord;

	return LTNULL;
}

// ---- the world, session and interface block (0x1002fb47-0x1003030a) ----- //

// The squared length of the world's box diagonal. The subtraction inlines and
// MagSqr does not, which is the shape the engine's own ltvector.h gives:
// operator- carries its body in the class, Mag/MagSqr only their declarations
// (the same split SPhysicsLT::SetVelocity shows for Dist/DistSqr).
float IWorldBsp::GetBoxDiagonalSqr()
{
	return (m_vBoxMax - m_vBoxMin).MagSqr();
}

// Save and restore both open the stream here and hand it to the object
// serializer; the interface half is only the open, the check and the close.
LTRESULT si_SaveObjects(char* pszSaveFileName, ObjectList* pList,
                        uint32 dwParam, uint32 flags)
{
	ILTStream* pStream = streamsim_Open(pszSaveFileName, "wb");
	if (!pStream)
		RETURN_ERROR(2, "ILTPhysics::SaveObjects", LT_ERROR);

	sav_SaveObjects(g_pServerMgr, pStream, pList, dwParam, flags);
	pStream->Release();
	return LT_OK;
}

// The restore half is saveio.cpp's sm_RestoreObjects (server 0x1001027c,
// lithtech.exe 0x00432200), declared there on its own view of the manager.
class SavServerMgr;
LTRESULT sm_RestoreObjects(SavServerMgr* pMgr, ILTStream* pStream,
                           uint32 dwParam, uint32 flags);

LTRESULT si_RestoreObjects(char* pszRestoreFileName, uint32 dwParam, uint32 flags)
{
	ILTStream* pStream = streamsim_Open(pszRestoreFileName, "rb");
	if (!pStream)
		RETURN_ERROR(2, "ILTPhysics::RestoreObjects", LT_ERROR);

	LTRESULT dResult = sm_RestoreObjects((SavServerMgr*)g_pServerMgr, pStream, dwParam, flags);
	pStream->Release();
	return dResult;
}

// The world is started at the manager's own idea of the time, not the
// caller's: the manager pointer is fetched once and read for both.
LTRESULT si_LoadWorld(char* pszWorldFileName, uint32 flags)
{
	CServerMgr* pMgr = g_pServerMgr;

	return pMgr->DoStartWorld(pszWorldFileName, flags, pMgr->m_Unknown6d8);
}

LTRESULT si_RunWorld()
{
	return g_pServerMgr->DoRunWorld();
}

LTRESULT si_UpdateSessionName(const char* sName)
{
	return g_pServerMgr->m_NetMgr.UpdateSessionName(sName);
}

LTRESULT si_GetSessionName(char* sName, uint32 dwBufferSize)
{
	if (!sName)
		RETURN_ERROR(1, "ILTPhysics::GetSessionName", LT_INVALIDPARAMS);

	return g_pServerMgr->m_NetMgr.GetSessionName(sName, dwBufferSize);
}

LTRESULT si_GetTcpIpAddress(char* sAddress, uint32 dwBufferSize, uint16& hostPort)
{
	if (!sAddress)
		RETURN_ERROR(1, "ILTPhysics::GetTcpIpAddress", LT_INVALIDPARAMS);

	return g_pServerMgr->m_NetMgr.GetTcpIpAddress(sAddress, dwBufferSize, hostPort);
}

// The one interface entry that talks back out of the engine: with no host
// application attached there is nothing to send to.
LTRESULT si_SendToServerApp(char* pMsg, uint32 nLen)
{
	ServerAppHandler* pHandler = g_pServerMgr->m_pAppHandler;
	if (pHandler)
		return pHandler->ShellMessageFn(pMsg, nLen);

	return LT_NOTFOUND;
}

FileEntry* si_GetFileList(char* pDirName)
{
	if (!pDirName)
		return LTNULL;

	return fs_GetFileList(&g_pServerMgr->m_FileMgr, pDirName);
}


// ---- the interface table (0x1002fd9b) ------------------------------------ //
//
// Everything ILTServer exposes that is not virtual is a function pointer on
// the interface object, and this is where all 125 of them are set. It is the
// map from the SDK's header to the shipped code: each store's offset names the
// field (iltserver.h's declaration order fixes the offsets) and its operand
// names the implementation, so the table below is the index of the whole
// server interface — including the parts we have not reconstructed yet.
//
// Entries whose code lives outside this file are declared here as they are
// used. Two prefixes appear: si_ for a server-interface entry that sits in
// another server TU, ci_ for one the client interface shares (the counter and
// model-node entries forward to the same common-module helpers both tables
// point at). Names for the unreconstructed ones are ours, taken from the
// field they fill; the addresses are what the stores actually write.
//
// Four fields are never assigned and stay null: GetPointAreas and
// GetBoxIntersecters (0x090, 0x094) and the model-node hide pair (0x1dc,
// 0x1e0) — the SDK still declares them, but the shipped engine no longer
// implements them. GetPropVector and GetPropColor share one implementation,
// which is why its address is loaded into a register and stored twice.

LTRESULT si_GetGameInfo(void **ppData, uint32 *pLen);                                                // 0x020
HCLASS si_GetClass(char *pName);                                                                     // 0x024
LTRESULT si_GetStaticObject(HCLASS hClass, HOBJECT *obj);                                            // 0x028
HCLASS si_GetObjectClass(HOBJECT hObject);                                                           // 0x02c
LTBOOL si_IsKindOf(HCLASS hClass, HCLASS hTest);                                                     // 0x030
LPBASECLASS si_CreateObject(HCLASS hClass, ObjectCreateStruct *pStruct);                             // 0x034
LPBASECLASS si_CreateObjectProps(HCLASS hClass, ObjectCreateStruct *pStruct, char *pszProps );       // 0x038
uint32 si_GetServerFlags();                                                                          // 0x03c
uint32 si_SetServerFlags(uint32 flags);                                                              // 0x040
LTRESULT si_CacheFile(uint32 fileType, char *pFilename);                                             // 0x044
LTRESULT si_GetNextPoly(HPOLY *hPoly);                                                               // 0x048
void ci_StartCounter(LTCounter *pCounter);                                                           // 0x04c
uint32 ci_EndCounter(LTCounter *pCounter);                                                           // 0x050
LTFLOAT ci_Random(LTFLOAT min, LTFLOAT max);                                                         // 0x054
int si_IntRandom(int min, int max);                                                                  // 0x058
LTFLOAT si_RandomScale(LTFLOAT scale);                                                               // 0x05c
void si_BPrint(char *pMsg, ...);                                                                     // 0x060
LTRESULT si_GetSkyDef(SkyDef *pDef);                                                                 // 0x068
LTRESULT si_SetSkyDef(SkyDef *pDef);                                                                 // 0x06c
LTRESULT si_AddObjectToSky(HOBJECT hObj, uint32 index);                                              // 0x070
LTRESULT si_RemoveObjectFromSky(HOBJECT hObj);                                                       // 0x074
LTRESULT si_GetGlobalLightObject(HOBJECT *hObj);                                                     // 0x078
LTRESULT si_SetGlobalLightObject(HOBJECT hObj);                                                      // 0x07c
LTRESULT si_GetPortalFlags(char *pPortalName, uint32 *pFlags);                                       // 0x080
LTRESULT si_SetPortalFlags(char *pPortalName, uint32 flags);                                         // 0x084
LTBOOL si_CastRay(IntersectQuery *pQuery, IntersectInfo *pInfo);                                     // 0x08c
ObjectList* si_CreateObjectList();                                                                   // 0x0a4
ObjectLink* si_AddObjectToList(ObjectList *pList, HOBJECT hObj);                                     // 0x0a8
void si_RemoveObjectFromList(ObjectList *pList, HOBJECT hObj);                                       // 0x0ac
LTRESULT si_AttachClient(HCLIENT hParent, HCLIENT hChild);                                           // 0x0b0
LTRESULT si_DetachClient(HCLIENT hClient);                                                           // 0x0b4
HCLIENT si_GetNextClient(HCLIENT hPrev);                                                             // 0x0b8
HCLIENTREF si_GetNextClientRef(HCLIENTREF hRef);                                                     // 0x0bc
uint32 si_GetClientRefInfoFlags(HCLIENTREF hClient);                                                 // 0x0c0
LTBOOL si_GetClientRefName(HCLIENTREF hClient, char *pName, int maxLen);                             // 0x0c4
HOBJECT si_GetClientRefObject(HCLIENTREF hClient);                                                   // 0x0c8
uint32 si_GetClientID(HCLIENT hClient);                                                              // 0x0cc
LTBOOL si_GetClientName(HCLIENT hClient, char *pName, int maxLen);                                   // 0x0d0
void si_SetClientInfoFlags(HCLIENT hClient, uint32 dwClientFlags );                                  // 0x0d4
uint32 si_GetClientInfoFlags(HCLIENT hClient);                                                       // 0x0d8
void si_SetClientUserData(HCLIENT hClient, void *pData);                                             // 0x0dc
void* si_GetClientUserData(HCLIENT hClient);                                                         // 0x0e0
void si_KickClient(HCLIENT hClient, uint32 nReasonCode);                                             // 0x0e4
LTRESULT si_SetClientViewPos(HCLIENT hClient, LTVector *pPos);                                       // 0x0e8
void si_RunGameConString(char *pString);                                                             // 0x0ec
void si_SetGameConVar(char *pName, char *pVal);                                                      // 0x0f0
HCONVAR si_GetGameConVar(char *pName);                                                               // 0x0f4
LTRESULT si_GetPropString(char *pPropName, char *pRet, int maxLen);                                  // 0x0f8
LTRESULT si_GetPropVector(char *pPropName, LTVector *pRet);                                          // 0x0fc
LTRESULT si_GetPropReal(char *pPropName, float *pRet);                                               // 0x104
LTRESULT si_GetPropFlags(char *pPropName, uint32 *pRet);                                             // 0x108
LTRESULT si_GetPropBool(char *pPropName, LTBOOL *pRet);                                              // 0x10c
LTRESULT si_GetPropLongInt(char *pPropName, long *pRet);                                             // 0x110
LTRESULT si_GetPropRotation(char *pPropName, LTRotation *pRet);                                      // 0x114
LTRESULT si_GetPropRotationEuler(char *pPropName, LTVector *pAngles);                                // 0x118
LTRESULT si_DoesPropExist(char *pPropName, int *pPropType);                                          // 0x120
HOBJECT si_ObjectToHandle(LPBASECLASS pObject);                                                      // 0x124
LPBASECLASS si_HandleToObject(HOBJECT hObject);                                                      // 0x128
LTBOOL si_IsCommandOn(HCLIENT hClient, int command);                                                 // 0x12c
LTBOOL ci_UpperStrcmp(char *pStr1, char *pStr2);                                                     // 0x130
int si_Parse(char *pCommand, char **pNewCommandPos, char *argBuffer, char **argPointers, int *nArgs); // 0x134
LTRESULT si_PlaySound( PlaySoundInfo *pPlaySoundInfo );                                              // 0x138
LTRESULT si_GetSoundDuration( HLTSOUND hSound, LTFLOAT *fDuration );                                 // 0x13c
LTRESULT si_IsSoundDone( HLTSOUND hSound, LTBOOL *bDone );                                           // 0x140
LTRESULT si_KillSound( HLTSOUND hSound );                                                            // 0x144
LTRESULT si_KillSoundLoop( HLTSOUND hSound );                                                        // 0x148
LTRESULT si_CreateInterObjectLink(HOBJECT hOwner, HOBJECT hLinked);                                  // 0x14c
void si_BreakInterObjectLink(HOBJECT hOwner, HOBJECT hLinked);                                       // 0x150
LTRESULT si_CreateAttachment(HOBJECT hParent, HOBJECT hChild, char *pSocketName, LTVector *pOffset, LTRotation *pRotationOffset, HATTACHMENT *pAttachment); // 0x154
LTRESULT si_RemoveAttachment(HATTACHMENT hAttachment);                                               // 0x158
LTRESULT si_FindAttachment(HOBJECT hParent, HOBJECT hChild, HATTACHMENT *hAttachment);               // 0x15c
LTBOOL si_GetObjectColor(HOBJECT hObject, float *r, float *g, float *b, float *a);                   // 0x160
LTBOOL si_SetObjectColor(HOBJECT hObject, float r, float g, float b, float a);                       // 0x164
uint32 si_GetObjectUserFlags(HOBJECT hObj);                                                          // 0x168
LTRESULT si_SetObjectUserFlags(HOBJECT hObj, uint32 flags);                                          // 0x16c
HOBJECT si_GetNextObject(HOBJECT hObj);                                                              // 0x170
HOBJECT si_GetNextInactiveObject(HOBJECT hObj);                                                      // 0x174
void si_SetNextUpdate(HOBJECT hObj, LTFLOAT nextUpdate);                                             // 0x178
void si_SetDeactivationTime(HOBJECT hObj, LTFLOAT fDeactivationTime);                                // 0x17c
void si_PingObjects(HOBJECT hObj);                                                                   // 0x180
void si_GetObjectPos(HOBJECT hObj, LTVector *pos);                                                   // 0x184
void si_SetObjectPos(HOBJECT hObj, LTVector *pos);                                                   // 0x188
void si_ScaleObject(HOBJECT hObj, LTVector *pNewScale);                                              // 0x18c
LTRESULT si_GetObjectScale(HOBJECT hObj, LTVector *pScale);                                          // 0x190
LTRESULT si_TeleportObject(HOBJECT hObj, LTVector *pNewPos);                                         // 0x194
// Every object whose box overlaps the sphere's box gets handed to the
// callback, which does the round test and adds what survives. The counter
// around it feeds the two stats the console reports.
void sm_SphereGather(LTObject* pObj, void* pUser);                 // 0x1002f861

// lithtech.exe's copy (0x0046faa0, 293 bytes) has NO tick counter: no
// CBaseCounter, no cnt_Start/EndCounter calls and no add into the ticks
// global -- only the count (0x004b48bc) survives.  Guarded here so the one
// source serves both images (identical on both).
ObjectList* sm_FindObjectsTouchingSphere(LTVector* pPosition, float radius)
{
	SphereQuery  query;
	LTVector     vMin;
	LTVector     vMax;

	// The list lives in the query record and nowhere else — every use reads
	// it back out, which is what keeps the frame at 0x40.
	query.m_pList = (ObjectList*)sm_sb_Allocate(&g_pServerMgr->m_ObjectListBank);
	if (!query.m_pList)
		return LTNULL;

	query.m_pList->m_nInList = 0;
	query.m_pList->m_pFirstLink = LTNULL;

	query.m_pPosition = pPosition;
	query.m_Radius = radius;
	query.m_RadiusSqr = radius * radius;
	query.m_Diameter = radius + radius;

#ifdef LT_SERVER_BUILD
	// Declared here and not at the top: server.dll constructs the counter
	// after the list is allocated and the query filled in (0x1003c1b8),
	// so the early return has no counter to skip.
	CBaseCounter counter(0);
	cnt_StartCounter(&counter);
#endif

	vMin = *pPosition - radius;
	vMax = *pPosition + radius;

	((IObjectQuery*)&g_pServerMgr->m_Unknown210)->FindObjectsInBox(
		&vMin, &vMax, (int (*)(LTObject*, void*))sm_SphereGather, &query, 0);

#ifdef LT_SERVER_BUILD
	g_FindSphereTicks += cnt_EndCounter(&counter);
#endif
	g_FindSphereCount += query.m_pList->m_nInList;
	return query.m_pList;
}

// Swing a direction onto a plane without changing its length: the axis is the
// cross of the plane normal with it, and the result is that axis crossed back
// with the normal. A direction already along the normal has no axis to swing
// about, which is what the magnitude test catches.
void si_TiltToPlane(LTObject* hObj, LTVector* pNormal)
{
	LTVector vAxis;
	LTVector vNew;

	vAxis = pNormal->Cross(hObj->m_Acceleration);

	if (vAxis.MagSqr() > 0.001f)
	{
		vAxis.Norm();
		vNew = vAxis.Cross(*pNormal);
		hObj->m_Acceleration = vNew * hObj->m_Acceleration.Mag();
	}
}

// Hand a list back to the manager: every link first, then the list itself.
// The manager pointer is re-read on each turn of the loop because the free
// writes through it.
void sm_RelinquishList(ObjectList* pList)
{
	ObjectLink* pLink;
	ObjectLink* pNext;

	if (!pList)
		return;

	pLink = pList->m_pFirstLink;
	while (pLink)
	{
		pNext = pLink->m_pNext;
		sb_Free(&g_pServerMgr->m_ObjectLinkBank, pLink);
		pLink = pNext;
	}

	sb_Free(&g_pServerMgr->m_ObjectListBank, pList);
}

// ---- object creation and the container links ---------------------------- //

LPBASECLASS si_CreateObject(HCLASS hClass, ObjectCreateStruct* pStruct)
{
	if (!hClass || !pStruct)
		return LTNULL;

	return g_pServerMgr->CreateObject(hClass, pStruct);
}

// The two entries whose work is done elsewhere: both are the thin server-side
// half of a world query, with the manager's own state handed in.

LTBOOL si_IntersectSegment(IntersectQuery* pQuery, IntersectInfo* pInfo)
{
	if (!g_pServerMgr)
		return LTFALSE;

	return w_IntersectSegment(pQuery, pInfo, &g_pServerMgr->m_Unknown210, 1);
}

// The lookup answers in bytes and the caller wants floats, and the channels
// come back the other way round.
LTBOOL si_GetPointShade(LTVector* pPoint, LTVector* pColor)
{
	uint8 color[4];

	w_GetPointShade(g_pServerMgr->m_ShadeState, pPoint, color);

	pColor->x = color[2];
	pColor->y = color[1];
	pColor->z = color[0];
	return LTTRUE;
}

// sm_FreeObjectInline and sm_AllocateObjectInline -- 0x1002abf8's and
// 0x10001c54's bodies again, `inline` under local names -- used to be
// defined here; they are in servermodel.h now, because servermodel.cpp's
// sm_LoadWorldObjects expands both in lithtech.exe too (0x0046a048,
// 0x0046a14c). lithtech.exe has NO standalone copy of either and expands
// them at every site in this file -- si_CreateObjectProps (0x00470c8d,
// 0x00470efb), CServerMgr::CreateObject (0x004745a0, 0x00474631) and
// sub_100334cc (0x00474a86) -- while server.dll calls 0x10001c54 /
// 0x1002abf8 at all of them.
//
// THE do/while(0) IS WHAT SEPARATES THE TWO IMAGES. Written straight, /Os
// expands the free too and server.dll loses all three callers (measured:
// CreateObject, si_CreateObjectProps and sub_100334cc all break). Any
// statement-level control flow in the body -- the `do { } while (0)`, or an
// `if (1)` -- makes /Os decline the expansion while /Ot still takes it, and
// the flow folds away so the emitted code is unchanged. The same rule
// explains SoundInstance::Term (its `if`) and sm_AllocateObjectInline (the
// `if` inside sb_Allocate): /Os-declined, /Ot-expanded, both with a branch
// in the body.

// Creating an object from a property string: parse the string into the
// manager's property list, let the object read it back through the PT_
// readers during its precreate, then throw the list away again. The list is
// the same one si_FindProp walks, which is why it hangs off the manager
// rather than being passed anywhere.
LPBASECLASS si_CreateObjectProps(HCLASS hClass, ObjectCreateStruct* pStruct, char* pszProps)
{
	CommandParser parser;
	LPBASECLASS   pObject;
	ObjectProp*   pProp;
	ObjectProp*   pNext;
	LTObject*     pNewObj;
	LTRESULT      dResult;
	int           i;
	int           nLen;

	parser.m_pPos = LTNULL;

	pObject = sm_AllocateObjectInline(g_pServerMgr, ((CClassInfo*)hClass)->m_pClassDef);
	g_pServerMgr->m_pCurProps = LTNULL;

	if (pszProps)
	{
		parser.m_pPos = pszProps;

		while (parser.Next())
		{
			if (parser.m_nArgs > 0)
			{
				pProp = (ObjectProp*)sm_sb_Allocate(&g_pServerMgr->m_PropBank);

				pProp->m_Type = PT_STRING;
				pProp->m_Name[0] = 0;
				pProp->m_String[0] = 0;

				strncpy(pProp->m_Name, parser.m_ArgPointers[0], sizeof(pProp->m_Name) - 1);
				pProp->m_Name[sizeof(pProp->m_Name) - 1] = 0;

				nLen = 0;
				for (i = 1; i < parser.m_nArgs; i++)
				{
					nLen += strlen(parser.m_ArgPointers[i]);
					if (nLen > 100)
						break;

					if (i != 1)
						strcat(pProp->m_String, " ");

					strcat(pProp->m_String, parser.m_ArgPointers[i]);
				}

				pProp->m_pNext = g_pServerMgr->m_pCurProps;
				g_pServerMgr->m_pCurProps = pProp;
			}
		}
	}

	pObject->EngineMessageFn(MID_PRECREATE, pStruct, PRECREATE_STRINGPROP);

	dResult = sm_CreateObjectInWorld(g_pServerMgr, pObject,
	                                 ((CClassInfo*)hClass)->m_pClassDef, pStruct,
	                                 0xffff, PRECREATE_STRINGPROP, &pNewObj);

	pProp = g_pServerMgr->m_pCurProps;
	while (pProp)
	{
		pNext = pProp->m_pNext;
		sb_Free(&g_pServerMgr->m_PropBank, pProp);
		pProp = pNext;
	}

	g_pServerMgr->m_pCurProps = LTNULL;

	if (!dResult)
		return pObject;

	sm_FreeObjectInline(g_pServerMgr, ((CClassInfo*)hClass)->m_pClassDef, pObject);
	return LTNULL;
}

// Both container walks run the same list — the inter-object links hanging off
// the object's own data — and differ only in which end of each link they are
// looking at. The flag list is written in step with the object list and left
// zeroed; the SDK asks for it to be the same size for that reason.
uint32 si_GetObjectContainers(LTObject* hObj, HOBJECT* pContainerList,
                              uint32* pFlagList, uint32 maxListSize)
{
	LTLink* pHead;
	LTLink* pCur;
	uint32  nFound;

	if (!hObj)
		return 0;

	pHead = &hObj->m_pSData->m_Links;
	nFound = 0;

	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		InterObjectLink* pLink = (InterObjectLink*)pCur->m_pData;

		if (pLink->m_Type != INTERLINK_CONTAINER)
			continue;

		if (hObj == pLink->m_pOwner)
			continue;

		if (nFound >= maxListSize)
			return nFound;

		pContainerList[nFound] = pLink->m_pOwner;
		pFlagList[nFound] = 0;
		nFound++;
	}

	return nFound;
}

uint32 si_GetContainedObjects(LTObject* hContainer, HOBJECT* pObjectList,
                              uint32* pFlagList, uint32 maxListSize)
{
	LTLink* pHead;
	LTLink* pCur;
	uint32  nFound;

	if (!hContainer)
		return 0;

	pHead = &hContainer->m_pSData->m_Links;
	nFound = 0;

	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		InterObjectLink* pLink = (InterObjectLink*)pCur->m_pData;

		if (pLink->m_Type != INTERLINK_CONTAINER)
			continue;

		if (hContainer != pLink->m_pOwner)
			continue;

		if (nFound >= maxListSize)
			return nFound;

		pObjectList[nFound] = pLink->m_pLinked;
		pFlagList[nFound] = 0;
		nFound++;
	}

	return nFound;
}

// ---- lights, the model play flag and the model filenames ---------------- //

// The light accessors forward to the object-colour pair, which is where a
// light's colour actually lives; the alpha channel a light has no use for is
// dropped on the way in and scaled back out of its byte on the way out.
void si_GetLightColor(LTObject* hObj, float* r, float* g, float* b)
{
	if (hObj)
		si_GetObjectColor(hObj, r, g, b, LTNULL);
}

void si_SetLightColor(LTObject* hObj, float r, float g, float b)
{
	if (!hObj)
		return;

	si_SetObjectColor(hObj, r, g, b, hObj->m_ColorA * (1.0f / 255.0f));
}

// Neither radius accessor checks for null — a light's radius overlays the
// world pointer, so both go straight at the type byte.
float si_GetLightRadius(LTObject* hObj)
{
	if (hObj->m_ObjectType == OT_LIGHT)
		return hObj->m_LightRadius;

	return 1.0f;
}

void si_SetLightRadius(LTObject* hObj, float radius)
{
	if (hObj->m_ObjectType != OT_LIGHT)
		return;

	hObj->m_LightRadius = radius;
	sm_SetObjectChangeFlag(g_pServerMgr, hObj, CF_COLORINFO);
}

// Playing is a bit on the model's own tracker, and only a real change is
// broadcast — which is what the compare against the caller's flag is doing.
//
// The bit is set and cleared on the member itself (`m_Flags |=`/`&=`), not
// through a uint16 local stored back once: the local made VC6 widen the
// clear to `and eax, imm32` where the shipped build has the 16-bit
// `and ax, 0fffeh` (server 0x10031bab). The tracker is reached through its
// own pointer: on lithtech.exe that keeps the flag store (0x00472d00) above
// the inlined change-flag's read of the object's internal flags.
void si_SetModelPlaying(LTObject* hObj, LTBOOL bPlaying)
{
	ModelInstance* pInst = (ModelInstance*)hObj;

	if (pInst->m_ObjectType != OT_MODEL)
		return;

	LTAnimTracker* pTracker = &pInst->m_AnimTracker;
	if ((pTracker->m_Flags & AT_PLAYING) == (bPlaying != 0))
		return;

	if (bPlaying)
		pTracker->m_Flags |= AT_PLAYING;
	else
		pTracker->m_Flags &= ~AT_PLAYING;

	sm_SetObjectChangeFlag(g_pServerMgr, pInst, CF_MODELINFO);
}

LTBOOL si_GetModelPlaying(LTObject* hObj)
{
	ModelInstance* pInst = (ModelInstance*)hObj;

	if (pInst->m_ObjectType == OT_MODEL)
		return pInst->m_AnimTracker.m_Flags & AT_PLAYING;

	return LTFALSE;
}

// The object keeps its model and skin as file identifiers, not strings, so
// both come back through the file manager. An unset one leaves an empty
// string rather than failing.
//
// AVP2: each buffer is optional -- a null buffer or a size below one skips
// it (`test %esi,%esi; je` then `test %ecx,%ecx; jle`, AvP2 lithtech.exe
// 0x0048195f..0x00481969 and 0x004819a3..0x004819ad), as the SDK comment
// promises ("You can pass in LTNULL for pFilename and pSkinName").
// PH: a third buffer pair after the skin's, for the model's extra texture
// (SObjData +0x44: lithtech.exe 0x00488cf1..0x00488d07, the arguments at
// 0x20/0x24 off %esp past the two pushes).
LTBOOL si_GetModelFilenames(LTObject* hObj, char* pFilename, int fileBufLen,
                            char* pSkinName, int skinBufLen
#ifdef PRIMAL_HUNT
                            , char* pExtraName, int extraBufLen
#endif
                            )
{
	if (!hObj)
		return LTFALSE;

	if (hObj->m_ObjectType != OT_MODEL)
		return LTFALSE;

	if (pFilename && fileBufLen > 0)
	{
		if (hObj->m_pSData->m_pModelFile)
		{
			strncpy(pFilename,
			        fs_GetFileName(&g_pServerMgr->m_FileMgr, hObj->m_pSData->m_pModelFile),
			        fileBufLen - 1);
		}
		else
		{
			pFilename[0] = 0;
		}
	}

	if (pSkinName && skinBufLen > 0)
	{
		if (hObj->m_pSData->m_pSkinFiles[0])
		{
			strncpy(pSkinName,
			        fs_GetFileName(&g_pServerMgr->m_FileMgr, hObj->m_pSData->m_pSkinFiles[0]),
			        skinBufLen - 1);
		}
		else
		{
			pSkinName[0] = 0;
		}
	}

#ifdef PRIMAL_HUNT
	if (pExtraName && extraBufLen > 0)
	{
		if (hObj->m_pSData->m_pFile44)
		{
			strncpy(pExtraName,
			        fs_GetFileName(&g_pServerMgr->m_FileMgr, hObj->m_pSData->m_pFile44),
			        extraBufLen - 1);
		}
		else
		{
			pExtraName[0] = 0;
		}
	}
#endif

	return LTTRUE;
}

// AVP2: ILTServer::GetModelFilenames2 (Primal Hunt 0x00488d50, AvP2
// 1.0.9.6 0x004819f0): the names of a model's files, straight into a
// create struct, each cut at MAX_CS_FILENAME_LEN - 1 (`push $0x7e`); an
// unset one leaves an empty string.
//
// NEAR on Primal Hunt, both images size-exact (lithtech.exe 0x00488d50,
// 252/252; server.dll 0x1003e93d, 242/242): one instruction each, the skin
// load's base and index swapped (`(%eax,%esi,1)` shipped, 0x00488dc4,
// 0x1003e9b1). Allocation (guide 09); a uint32 counter is worse (`jb`).
LTBOOL si_GetModelFilenames2(LTObject* hObj, ObjectCreateStruct* pStruct)
{
	if (!hObj)
		return LTFALSE;

	if (hObj->m_ObjectType != OT_MODEL)
		return LTFALSE;

	if (hObj->m_pSData->m_pModelFile)
	{
		strncpy(pStruct->m_Filename,
		        fs_GetFileName(&g_pServerMgr->m_FileMgr,
		                       hObj->m_pSData->m_pModelFile),
		        MAX_CS_FILENAME_LEN - 1);
	}
	else
	{
		pStruct->m_Filename[0] = 0;
	}

	for (int i = 0; i < MAX_MODEL_TEXTURES; i++)
	{
		if (hObj->m_pSData->m_pSkinFiles[i])
		{
			strncpy(pStruct->m_SkinNames[i],
			        fs_GetFileName(&g_pServerMgr->m_FileMgr,
			                       hObj->m_pSData->m_pSkinFiles[i]),
			        MAX_CS_FILENAME_LEN - 1);
		}
		else
		{
			pStruct->m_SkinNames[i][0] = 0;
		}
	}

#ifdef PRIMAL_HUNT
	// PH: the create struct's extra name after the skins (+0x300), written
	// through the member at each use: server.dll adds 0x300 only on the
	// copying arm (0x1003ea0e) and clears `0x300(%esi)` on the other.
	if (hObj->m_pSData->m_pFile44)
	{
		strncpy(pStruct->m_ExtraTextureName,
		        fs_GetFileName(&g_pServerMgr->m_FileMgr,
		                       hObj->m_pSData->m_pFile44),
		        MAX_CS_FILENAME_LEN - 1);
	}
	else
	{
		pStruct->m_ExtraTextureName[0] = 0;
	}
#endif

	return LTTRUE;
}

int si_Parse(char* pCommand, char** pNewCommandPos, char* argBuffer,
             char** argPointers, int* nArgs)
{
	return parse_Command(pCommand, pNewCommandPos, argBuffer, argPointers, nArgs);
}

// ---- teleport, blocking priority and the sprite clip poly ---------------- //

// The sprite's clip poly sits past the end of LTObject (name ours).
class SpriteObject : public LTObject
{
public:
	uint8 m_pad_1bc[0x1c4 - 0x1bc];
	HPOLY m_ClipPoly;                        // 0x1c4
};

LTRESULT si_TeleportObject(LTObject* hObj, LTVector* pNewPos)
{
	if (!hObj)
		RETURN_ERROR(1, "ILTPhysics::TeleportObject", LT_INVALIDPARAMS);

	si_SetObjectPos(hObj, pNewPos);
	return LT_OK;
}

uint8 si_GetBlockingPriority(LTObject* hObj)
{
	if (hObj)
		return hObj->m_BlockingPriority;

	return 0;
}

LTRESULT si_ClipSprite(LTObject* hObj, HPOLY hPoly)
{
	SpriteObject* pSprite = (SpriteObject*)hObj;

	// Two checks, not one `||`: the shipped code keeps the error inline where
	// the second one falls through instead of sinking it past the body.
	if (!pSprite)
		return LT_ERROR;

	if (pSprite->m_ObjectType != OT_SPRITE)
		return LT_ERROR;

	if (pSprite->m_ClipPoly != hPoly)
	{
		pSprite->m_ClipPoly = hPoly;
		sm_SetObjectChangeFlag(g_pServerMgr, pSprite, CF_MODELINFO);
	}

	return LT_OK;
}

void si_SetBlockingPriority(LTObject* hObj, uint8 pri)
{
	if (hObj)
		hObj->m_BlockingPriority = pri;
}

// ---- server state, the console and the object lists --------------------- //

// Two of the three SS_ bits live in the manager's other status word, so the
// reader assembles them rather than returning a field.
uint32 si_GetServerFlags()
{
	uint32      nFlags;

	nFlags = ((g_pServerMgr->m_Unknown1a0 >> 1) & SS_DEMOPLAYBACK) | g_pServerMgr->m_ServerFlags;

	if (g_pServerMgr->m_Unknown1a0 & 0x10)
		nFlags |= SS_CACHING;

	return nFlags;
}

// Only SS_PAUSED is settable, and the store is read back rather than reused:
// the global could be what was just written through.
uint32 si_SetServerFlags(uint32 flags)
{
	g_pServerMgr->m_ServerFlags = flags & SS_PAUSED;
	return g_pServerMgr->m_ServerFlags;
}

LTRESULT si_CacheFile(uint32 fileType, char* pFilename)
{
	return sm_CacheFile(g_pServerMgr, fileType, pFilename);
}


// AVP2: the SDK's KickClient takes the client alone (iltserver.h), and the
// reason pushed is always 6 (Primal Hunt 0x00487010: `push $6; push
// %eax`, the second argument never read). The table's slot type here keeps
// NOLF's reason parameter, unused.
void si_KickClient(Client* pClient, uint32 nReasonCode)
{
	if (!pClient)
		return;

	g_pServerMgr->m_NetMgr.Disconnect(pClient->m_pConnection, 6);
}

// The one check site in the file that reports at level 0, and the one whose
// name literal carries no class prefix.
LTRESULT si_SetClientViewPos(Client* pClient, LTVector* pPos)
{
	if (!pClient)
		RETURN_ERROR(0, "SetClientViewPos", LT_INVALIDPARAMS);

	pClient->m_Position = *pPos;
	return LT_OK;
}

void si_RunGameConString(char* pString)
{
	con_RunString(&g_pServerMgr->m_ConsoleState, pString);
}

void si_SetGameConVar(char* pName, char* pVal)
{
	if (!pName || !pVal)
		return;

	con_SetVar(&g_pServerMgr->m_ConsoleState, pName, pVal);
}

HCONVAR si_GetGameConVar(char* pName)
{
	if (!pName)
		return LTNULL;

	return con_FindVar(&g_pServerMgr->m_ConsoleState, pName);
}

// The command states are double-buffered per client — one 0xff-byte page per
// buffer, the live one indexed by the counter after them — which is what the
// multiply by 0xff says.
LTBOOL si_IsCommandOn(Client* pClient, int command)
{
	if (!pClient || command >= MAX_CLIENTCOMMANDS)
		return LTFALSE;

	return pClient->m_CommandStates[pClient->m_nCurCommands][command];
}

ObjectList* si_CreateObjectList()
{
	ObjectList* pList = (ObjectList*)sm_sb_Allocate(&g_pServerMgr->m_ObjectListBank);

	pList->m_pFirstLink = LTNULL;
	pList->m_nInList = 0;
	return pList;
}

ObjectLink* si_AddObjectToList(ObjectList* pList, HOBJECT hObj)
{
	ObjectLink* pLink = (ObjectLink*)sm_sb_Allocate(&g_pServerMgr->m_ObjectLinkBank);

	pLink->m_pNext = pList->m_pFirstLink;
	pList->m_pFirstLink = pLink;
	pLink->m_hObject = hObj;
	pList->m_nInList++;
	return pLink;
}

void si_RemoveObjectFromList(ObjectList* pList, HOBJECT hObj)
{
	ObjectLink** ppPrev = &pList->m_pFirstLink;
	ObjectLink*  pCur   = pList->m_pFirstLink;

	while (pCur)
	{
		if (pCur->m_hObject == hObj)
		{
			*ppPrev = pCur->m_pNext;
			pList->m_nInList--;
			sb_Free(&g_pServerMgr->m_ObjectLinkBank, pCur);
			return;
		}

		ppPrev = &pCur->m_pNext;
		pCur = pCur->m_pNext;
	}
}

// Every message the manager hands out starts pointing at the same object, so
// the writer can reach it without going back through the manager.
void CServerMgr::InitMessageOwner(ServerMessage* pMsg)
{
	pMsg->m_Write.m_pObjectIO = (CMsgObjectIO*)m_Unknown804;
}

// Changing the global light is a change every client has to hear about, so it
// is marked on all of them rather than on an object.
void CServerMgr::SetGlobalLightObject(HOBJECT hObj)
{
	LTLink* pCur;

	m_hGlobalLightObject = hObj;

	for (pCur = m_ClientList.m_Head.m_pNext;
	     pCur != &m_ClientList.m_Head;
	     pCur = pCur->m_pNext)
	{
		((Client*)pCur->m_pData)->m_ClientFlags |= CLIENTFLAG_SKYCHANGED;
	}
}

// A ray is an intersect query with the far end computed for the caller: the
// direction is normalised to a fixed reach and added to the start. A
// direction too short to normalise is not a ray at all.
LTBOOL si_CastRay(IntersectQuery* pQuery, IntersectInfo* pInfo)
{
	float fMag = pQuery->m_Direction.Mag();

	if (fMag < 0.00001f)
		return LTFALSE;

	fMag = 10000.0f / fMag;

	pQuery->m_To = pQuery->m_Direction * fMag;
	pQuery->m_To += pQuery->m_From;

	return si_IntersectSegment(pQuery, pInfo);
}

// AVP2: what the polygon box walk carries (0x24 bytes; names ours): the
// caller's list and its room, the count found so far, the caller's filter,
// and the box as a sphere -- its centre and half its diagonal.
//
// The constructor is the shipped server.dll 0x1003ced1 (thiscall, `ret 8`),
// emitted after its one caller: defined in the class, so /O1 declines it and
// leaves a COMDAT behind, and lithtech.exe's /O2 expands it in place
// (0x00486093..0x004860c8). A list with no room is a list of none.
class CPolyBoxQueryQ523
{
public:
	CPolyBoxQueryQ523(HPOLY* pList, uint32 nMax)
	{
		m_pList     = pList;
		m_nMax      = pList ? nMax : 0;
		m_nFound    = 0;
		m_FilterFn  = LTNULL;
		m_pUserData = LTNULL;
		m_vCenter.Init();
		m_fRadius   = 0.0f;
	}

	HPOLY*         m_pList;                  // +0x00
	uint32         m_nMax;                   // +0x04
	uint32         m_nFound;                 // +0x08
	ObjectFilterFn m_FilterFn;               // +0x0c
	void*          m_pUserData;              // +0x10
	LTVector       m_vCenter;                // +0x14
	float          m_fRadius;                // +0x20
};

// The box's six faces as planes whose normals are fixed and whose distances
// each query sets (server.dll 0x1008c608, lithtech.exe 0x004f4088, 0x10
// apart), and the box's bounding sphere (server.dll 0x1008bb48 radius,
// 0x1008bb4c centre; lithtech.exe 0x004f35c8, 0x004f35cc). The walk's
// callback reads them; names ours. The sphere is ONE object: as two
// separate globals server.dll's /O1 scheduled the first dword of the
// centre's copy ahead of the radius store (0x1003ce42), which it does not
// do into one aggregate.
struct CPolyBoxPlaneQ523
{
	float m_Normal[3];
	float m_Dist;
};

CPolyBoxPlaneQ523 g_PolyBoxPlanesQ523[6];
struct CPolyBoxSphereQ523
{
	float    m_fRadius;
	LTVector m_vCenter;
};

CPolyBoxSphereQ523 g_PolyBoxSphereQ523;

// The world tree's per-object callback for the walk (server.dll 0x1003bf5a,
// lithtech.exe 0x00484f50; defined below, before sm_SphereGather): world
// models only, through the caller's filter, then the model's polygons
// against the box.
void sm_PolyBoxGatherQ523(LTObject* pObj, void* pUser);

// AVP2: ILTServer::FindPoliesTouchingBox -- server.dll 0x1003cd82 (with the
// counter LT_SERVER_TIMERS gives it), lithtech.exe 0x00486090 (without). The
// box goes to the world tree as a box, and the walk's callback gets the same
// box as six planes and a sphere through the globals above. The count is
// added to the "ShowPolyFindTicks" stats whether or not the caller asked for
// it.
void si_FindPoliesTouchingBox(const LTVector& vBoxMin, const LTVector& vBoxMax,
                              HPOLY* pPolyList, uint32 nPolyListSize,
                              uint32* pnNumPoliesFound, ObjectFilterFn FilterFn,
                              void* pUserData)
{
#ifdef LT_SERVER_BUILD
	CBaseCounter counter(0);

	cnt_StartCounter(&counter);
#endif

	CPolyBoxQueryQ523 query(pPolyList, nPolyListSize);

	query.m_vCenter = (vBoxMax + vBoxMin) * 0.5f;
	query.m_fRadius = (vBoxMax - vBoxMin).Mag() * 0.5f;
	query.m_FilterFn  = FilterFn;
	query.m_pUserData = pUserData;

	g_PolyBoxSphereQ523.m_fRadius = query.m_fRadius;
	g_PolyBoxSphereQ523.m_vCenter = query.m_vCenter;

	g_PolyBoxPlanesQ523[0].m_Dist = vBoxMin.x;
	g_PolyBoxPlanesQ523[1].m_Dist = -vBoxMax.x;
	g_PolyBoxPlanesQ523[2].m_Dist = vBoxMin.y;
	g_PolyBoxPlanesQ523[3].m_Dist = -vBoxMax.y;
	g_PolyBoxPlanesQ523[4].m_Dist = vBoxMin.z;
	g_PolyBoxPlanesQ523[5].m_Dist = -vBoxMax.z;

	((IObjectQuery*)&g_pServerMgr->m_Unknown210)->FindObjectsInBox(
		(LTVector*)&vBoxMin, (LTVector*)&vBoxMax,
		(int (*)(LTObject*, void*))sm_PolyBoxGatherQ523,
		&query, 0);

#ifdef LT_SERVER_BUILD
	g_FindPolyTicksQ502 += cnt_EndCounter(&counter);
#endif
	g_FindPolyCountQ502 += query.m_nFound;

	if (pnNumPoliesFound)
		*pnNumPoliesFound = query.m_nFound;
}

// ---- the object property readers (0x1003047c-0x100306f0) ---------------- //
//
// Every one of these is the same three steps: find the named property on the
// object currently being created, check that its type is the one this reader
// speaks for, and hand back the value at +0x58. The type checks run straight
// down the PT_ list, which is what identifies each reader.

// 0x100304ac — the list walk the readers share (name ours).
ObjectProp* si_FindProp(char* pPropName)
{
	ObjectProp* pProp = g_pServerMgr->m_pCurProps;

	while (pProp)
	{
		if (strcmp(pProp->m_Name, pPropName) == 0)
			return pProp;

		pProp = pProp->m_pNext;
	}

	return LTNULL;
}

LTRESULT si_GetPropString(char* pPropName, char* pRet, int maxLen)
{
	ObjectProp* pProp = si_FindProp(pPropName);

	if (!pProp || pProp->m_Type != PT_STRING)
		return LT_NOTFOUND;

	strncpy(pRet, pProp->m_String, maxLen - 1);
	return LT_OK;
}

// One reader serves PT_VECTOR and PT_COLOR, which is why the table stores its
// address twice.
LTRESULT si_GetPropVector(char* pPropName, LTVector* pRet)
{
	ObjectProp* pProp = si_FindProp(pPropName);

	if (!pProp || (pProp->m_Type != PT_VECTOR && pProp->m_Type != PT_COLOR))
		return LT_NOTFOUND;

	*pRet = *(LTVector*)pProp->m_Vec;
	return LT_OK;
}

LTRESULT si_GetPropReal(char* pPropName, float* pRet)
{
	ObjectProp* pProp = si_FindProp(pPropName);

	if (!pProp || pProp->m_Type != PT_REAL)
		return LT_NOTFOUND;

	*pRet = pProp->m_Float;
	return LT_OK;
}

LTRESULT si_GetPropFlags(char* pPropName, uint32* pRet)
{
	ObjectProp* pProp = si_FindProp(pPropName);

	if (!pProp || pProp->m_Type != PT_FLAGS)
		return LT_NOTFOUND;

	*pRet = (uint32)pProp->m_Float;
	return LT_OK;
}

LTRESULT si_GetPropBool(char* pPropName, LTBOOL* pRet)
{
	ObjectProp* pProp = si_FindProp(pPropName);

	if (!pProp || pProp->m_Type != PT_BOOL)
		return LT_NOTFOUND;

	*pRet = pProp->m_Bool;
	return LT_OK;
}

LTRESULT si_GetPropLongInt(char* pPropName, long* pRet)
{
	ObjectProp* pProp = si_FindProp(pPropName);

	if (!pProp || pProp->m_Type != PT_LONGINT)
		return LT_NOTFOUND;

	*pRet = (long)pProp->m_Float;
	return LT_OK;
}

LTRESULT si_GetPropRotation(char* pPropName, LTRotation* pRet)
{
	ObjectProp* pProp = si_FindProp(pPropName);

	if (!pProp || pProp->m_Type != PT_ROTATION)
		return LT_NOTFOUND;

	// The angles go through a local float array filled element by element.
	// An aggregate local is what makes VC6 load all three onto the x87 stack
	// in member order before popping them into the argument slots (server
	// 0x100305b5..0x100305cb); passing m_Vec[0..2] straight, via float
	// locals, via a float* local, or via a float-returning inline all
	// evaluate one argument at a time instead. The FOURTH element is the
	// /O2 frame: lithtech 0x470a60 reserves 0x10 bytes and uses only
	// three slots. An LTVector or float[3] gives the same /O1 bytes but a
	// 0xc frame at /O2.
	float v[4];
	v[0] = pProp->m_Vec[0];
	v[1] = pProp->m_Vec[1];
	v[2] = pProp->m_Vec[2];
	rot_SetupEuler(v[0], v[1], v[2], pRet);
	return LT_OK;
}

LTRESULT si_GetPropRotationEuler(char* pPropName, LTVector* pAngles)
{
	ObjectProp* pProp = si_FindProp(pPropName);

	if (!pProp || pProp->m_Type != PT_ROTATION)
		return LT_NOTFOUND;

	// The angles go through an LTRotation local and out through Init():
	// that is the 0x10 frame (0x100305e0, 0x00470ac4), and it is what keeps
	// only the first angle on the x87 stack while the other two are integer-
	// copied into their slots and reloaded (0x100305f5..0x10030612). A float
	// v[4] with three member stores got the frame but not the traffic.
	LTRotation r;
	r.m_Quat[0] = pProp->m_Vec[0];
	r.m_Quat[1] = pProp->m_Vec[1];
	r.m_Quat[2] = pProp->m_Vec[2];
	pAngles->Init(r.m_Quat[0], r.m_Quat[1], r.m_Quat[2]);
	return LT_OK;
}

// The one reader that answers whatever type the property happens to be: it
// clears the caller's record and then fills whichever fields the type maps
// to, through the four setters that know the mapping. The bool case computes
// its float and falls into the same call the numeric cases make.
LTRESULT si_GetPropGeneric(char* pPropName, GenericProp* pProp)
{
	ObjectProp* pFound = si_FindProp(pPropName);

	if (!pFound)
		return LT_NOTFOUND;

	gp_Clear(pProp);

	switch (pFound->m_Type)
	{
		case PT_STRING:
			gp_SetString(pProp, pFound->m_String);
			break;

		case PT_VECTOR:
		case PT_COLOR:
			gp_SetVector(pProp, (LTVector*)pFound->m_Vec);
			break;

		case PT_REAL:
		case PT_FLAGS:
		case PT_LONGINT:
			gp_SetFloat(pProp, pFound->m_Float);
			break;

		case PT_BOOL:
			gp_SetFloat(pProp, pFound->m_Bool);
			break;

		case PT_ROTATION:
			gp_SetRotation(pProp, (LTVector*)pFound->m_Vec);
			break;

		default:
			return LT_NOTFOUND;
	}

	return LT_OK;
}

LTRESULT si_DoesPropExist(char* pPropName, int* pPropType)
{
	ObjectProp* pProp = si_FindProp(pPropName);

	if (pProp)
	{
		*pPropType = pProp->m_Type;
		return LT_OK;
	}

	return LT_NOTFOUND;
}

HOBJECT si_ObjectToHandle(LPBASECLASS pObject)
{
	if (!pObject)
		return LTNULL;

	return pObject->m_hObject;
}

LPBASECLASS si_HandleToObject(LTObject* hObject)
{
	if (!hObject)
		return LTNULL;

	return hObject->m_pSData->m_pBaseClass;
}

// ---- the client accessors (0x10030c50-0x10030def) ----------------------- //
//
// An HCLIENT is a Client*, an HCLIENTREF the pending-announce record the
// manager keeps for a client that has not finished joining; both open with
// the LTLink that ties them into their list, which is what lets the two
// walkers step from one handle to the next without touching the manager.

// The explicit `else` is load-bearing. Both arms end in `return
// pCur->m_pData`, so VC6 cross-jumps that tail to the end of the function
// either way; what the `else` decides is which of the two `return LTNULL`
// blocks survives. Written as two guarded early returns the second copy
// hosts the block and the first arm needs a `jmp` to reach it (two bytes
// more than shipped); written as an if/else chain the FIRST arm hosts it
// inline and the count check jumps backwards into it, which is what shipped.
// (The shared-exit host: which arm carries the common tail.)
HCLIENT si_GetNextClient(Client* pPrev)
{
	LTLink* pCur;

	if (pPrev)
	{
		pCur = pPrev->m_Link.m_pNext;
		if (pCur == &g_pServerMgr->m_ClientList.m_Head)
			return LTNULL;

		return (HCLIENT)pCur->m_pData;
	}
	else
	{
		if (g_pServerMgr->m_ClientList.m_nElements == 0)
			return LTNULL;

		return (HCLIENT)g_pServerMgr->m_ClientList.m_Head.m_pNext->m_pData;
	}
}

HCLIENTREF si_GetNextClientRef(HCLIENTREF hRef)
{
	CServerMgr* pMgr = g_pServerMgr;
	LTLink*     pCur = (LTLink*)hRef;

	if (!pCur)
		pCur = &pMgr->m_List8ec.m_Head;

	pCur = pCur->m_pNext;
	if (pCur == &pMgr->m_List8ec.m_Head)
		return LTNULL;

	return (HCLIENTREF)pCur->m_pData;
}

// Bit 1 of the record is CIF_LOCAL, answered as a test-and-select rather
// than a shift: VC6 turns the select into `shr 1; and 1` itself, and it is
// that conversion which keeps the zero-extension (server 0x10030cb5 movzbl,
// lithtech 0x004713b9 `mov dword; and 0ffh`). The field is a uint32 (see
// servermgr.h): as a uint8, /O2 loads it `xor ecx,ecx; mov cl,...`.
uint32 si_GetClientRefInfoFlags(HCLIENTREF hClient)
{
	if (!hClient)
		return 0;

	if (((CUnknown8ECRec*)hClient)->m_Flags & 2)
		return CIF_LOCAL;
	else
		return 0;
}

LTBOOL si_GetClientRefName(HCLIENTREF hClient, char* pName, int maxLen)
{
	if (!hClient)
	{
		pName[0] = 0;
		return LTFALSE;
	}

	strncpy(pName, ((CUnknown8ECRec*)hClient)->m_Name, maxLen - 1);
	return LTTRUE;
}

HOBJECT si_GetClientRefObject(HCLIENTREF hClient)
{
	if (!hClient)
		return LTNULL;

	return sm_FindObjectByID(g_pServerMgr, ((CUnknown8ECRec*)hClient)->m_nObjectID);
}

uint32 si_GetClientID(Client* pClient)
{
	if (!pClient)
		return 0xffffffff;

	return pClient->m_ClientID;
}

LTBOOL si_GetClientName(Client* pClient, char* pName, int maxLen)
{
	if (!pClient)
		return LTFALSE;

	strncpy(pName, pClient->m_pName, maxLen - 1);
	return LTTRUE;
}

// The public CIF_ bits and the internal client flags agree on three values
// and disagree on the other three, so both directions are written out one
// bit at a time rather than masked wholesale.
void si_SetClientInfoFlags(Client* pClient, uint32 dwClientFlags)
{
	if (!pClient)
		return;

	pClient->m_ClientFlags &= ~(CLIENTFLAG_FULLRES | CLIENTFLAG_SENDCOBJROTATION |
	                            CLIENTFLAG_FORCENEXTUPDATE | CLIENTFLAG_AUTOACTIVATE);

	if (dwClientFlags & CIF_FULLRES)
		pClient->m_ClientFlags |= CLIENTFLAG_FULLRES;

	if (dwClientFlags & CIF_SENDCOBJROTATION)
		pClient->m_ClientFlags |= CLIENTFLAG_SENDCOBJROTATION;

	if (dwClientFlags & CIF_FORCENEXTUPDATE)
		pClient->m_ClientFlags |= CLIENTFLAG_FORCENEXTUPDATE;

	if (dwClientFlags & CIF_AUTOACTIVATEOBJECTS)
		pClient->m_ClientFlags |= CLIENTFLAG_AUTOACTIVATE;
}

uint32 si_GetClientInfoFlags(Client* pClient)
{
	uint32 dwFlags;

	if (!pClient)
		return 0;

	dwFlags = 0;

	if (pClient->m_ClientFlags & CLIENTFLAG_LOCAL)
		dwFlags = CIF_LOCAL;

	if (pClient->m_ClientFlags & CLIENTFLAG_PLAYBACK)
		dwFlags |= CIF_PLAYBACK;

	if (pClient->m_ClientFlags & CLIENTFLAG_FULLRES)
		dwFlags |= CIF_FULLRES;

	if (pClient->m_ClientFlags & CLIENTFLAG_SENDCOBJROTATION)
		dwFlags |= CIF_SENDCOBJROTATION;

	if (pClient->m_ClientFlags & CLIENTFLAG_FORCENEXTUPDATE)
		dwFlags |= CIF_FORCENEXTUPDATE;

	if (pClient->m_ClientFlags & CLIENTFLAG_AUTOACTIVATE)
		dwFlags |= CIF_AUTOACTIVATEOBJECTS;

	return dwFlags;
}

void si_SetClientUserData(Client* pClient, void* pData)
{
	if (pClient)
		pClient->m_pUserData = pData;
}

void* si_GetClientUserData(Client* pClient)
{
	if (!pClient)
		return LTNULL;

	return pClient->m_pUserData;
}

LTRESULT si_GetLastCollision(CollisionInfo *pInfo);                                                  // 0x198
LTRESULT si_GetObjectRotation(HOBJECT hObj, LTRotation *pRotation);                                  // 0x19c
LTRESULT si_SetObjectRotation(HOBJECT hObj, LTRotation *pRotation);                                  // 0x1a0
LTRESULT si_RotateObject(HOBJECT hObj, LTRotation *pRotation);                                       // 0x1a4
void si_TiltToPlane(HOBJECT hObj, LTVector *pNormal);                                                // 0x1a8
void si_SetObjectState(HOBJECT hObj, int state);                                                     // 0x1ac
int si_GetObjectState(HOBJECT hObj);                                                                 // 0x1b0
void si_SetBlockingPriority(HOBJECT hObj, uint8 pri);                                                // 0x1b4
uint8 si_GetBlockingPriority(HOBJECT hObj);                                                          // 0x1b8
LTRESULT si_ClipSprite(HOBJECT hObj, HPOLY hPoly);                                                   // 0x1bc
void si_GetLightColor(HOBJECT hObj, float *r, float *g, float *b);                                   // 0x1c0
void si_SetLightColor(HOBJECT hObj, float r, float g, float b);                                      // 0x1c4
float si_GetLightRadius(HOBJECT hObj);                                                               // 0x1c8
void si_SetLightRadius(HOBJECT hObj, float radius);                                                  // 0x1cc
LTRESULT ci_GetNextModelNode(HOBJECT hObject, HMODELNODE hNode, HMODELNODE *pNext);                  // 0x1d0
LTRESULT ci_GetModelNodeName(HOBJECT hObject, HMODELNODE hNode, char *pName, uint32 maxLen);         // 0x1d4
LTRESULT si_GetModelCommandString(HOBJECT hObj, char *pStr, uint32 maxLen);                          // 0x1d8
const char *ci_GetAnimName(HOBJECT hObject, HMODELANIM hAnim);                                       // 0x1e4
void si_SetModelPlaying(HOBJECT hObj, LTBOOL bPlaying);                                              // 0x1e8
LTBOOL si_GetModelPlaying(HOBJECT hObj);                                                             // 0x1ec
LTBOOL si_GetModelFilenames(HOBJECT hObj, char *pFilename, int fileBufLen, char *pSkinName, int skinBufLen
#ifdef PRIMAL_HUNT
                            , char *pExtraName, int extraBufLen
#endif
                            ); // 0x1f0
uint32 si_GetObjectContainers(HOBJECT hObj, HOBJECT *pContainerList, uint32 *pFlagList, uint32 maxListSize); // 0x1f4
uint32 si_GetContainedObjects(HOBJECT hContainer, HOBJECT *pObjectList, uint32 *pFlagList, uint32 maxListSize); // 0x1f8
LTRESULT si_SaveObjects(char *pszSaveFileName, ObjectList *pList, uint32 dwParam, uint32 flags);     // 0x1fc
LTRESULT si_RestoreObjects( char *pszRestoreFileName, uint32 dwParam, uint32 flags );                // 0x200
LTRESULT si_LoadWorld(char *pszWorldFileName, uint32 flags);                                         // 0x204
LTRESULT si_RunWorld();                                                                              // 0x208
LTRESULT si_UpdateSessionName(const char* sName);                                                    // 0x20c
LTRESULT si_GetSessionName(char* sName, uint32 dwBufferSize);                                        // 0x210
LTRESULT si_SendToServerApp( char *pMsg, uint32 nLen );                                              // 0x214
LTRESULT si_GetTcpIpAddress(char* sAddress, uint32 dwBufferSize, uint16 &hostPort);                  // 0x218
FileEntry* si_GetFileList(char *pDirName);                                                           // 0x21c
void fs_FreeFileList(FileEntry *pHead);                                                              // 0x220

// AVP2: the ILTServer entries AvP2's SDK adds, not yet reconstructed (names ours)
LTRESULT si_SetCRCString(char *szCRCFiles);
LTRESULT si_CreateStringCRC();
LTRESULT si_CreateWorldCRC();
LTRESULT si_GetNextWMPoly(HPOLY *hPoly, HOBJECT hWorldModel);
void si_FindPoliesTouchingBox(const LTVector & vBoxMin, const LTVector & vBoxMax, HPOLY * pPolyList, uint32 nPolyListSize, uint32 * pnNumPoliesFound, ObjectFilterFn FilterFn = LTNULL, void * pUserData = LTNULL);
LTRESULT si_GetSmartLinkBody(HOBJECT hOwner, LTSmartLink_Body ** pRet);
LTRESULT si_ReleaseSmartLinkBody(LTSmartLink_Body * pBody);
LTBOOL si_GetModelFilenames2(HOBJECT hObj, ObjectCreateStruct *pStruct);

void si_SetupInterface(CLTServer* pServer)
{
	// AVP2: one run of stores from +0x20 to +0x240, in the order every AvP2
	// image has them (Primal Hunt lithtech.exe 0x00485860 and server.dll
	// 0x1003c604, AvP2 1.0.9.6's two alike) -- the table's order but for a
	// few blocks: FindPoliesTouchingBox's pair, CreateObjectList's run and
	// the attachment/link groups sit where the source evidently had them.
	pServer->SetCRCString               = si_SetCRCString;   // AVP2: new, to be reconstructed
	pServer->CreateStringCRC            = si_CreateStringCRC;   // AVP2: new
	pServer->CreateWorldCRC             = si_CreateWorldCRC;   // AVP2: new
	pServer->GetGameInfo                = si_GetGameInfo;
	pServer->GetClass                   = si_GetClass;
	pServer->GetStaticObject            = si_GetStaticObject;
	pServer->GetObjectClass             = si_GetObjectClass;
	pServer->IsKindOf                   = si_IsKindOf;
	pServer->CreateObject               = si_CreateObject;
	pServer->CreateObjectProps          = si_CreateObjectProps;
	pServer->GetServerFlags             = si_GetServerFlags;
	pServer->SetServerFlags             = si_SetServerFlags;
	pServer->CacheFile                  = si_CacheFile;
	pServer->GetNextPoly                = si_GetNextPoly;
	pServer->GetNextWMPoly              = si_GetNextWMPoly;   // AVP2: new, to be reconstructed
	pServer->StartCounter               = ci_StartCounter;
	pServer->EndCounter                 = ci_EndCounter;
	pServer->Random                     = ci_Random;
	pServer->IntRandom                  = si_IntRandom;
	pServer->RandomScale                = si_RandomScale;
	pServer->BPrint                     = si_BPrint;
	pServer->DebugOut                   = con_Printf;
	pServer->GetSkyDef                  = si_GetSkyDef;
	pServer->SetSkyDef                  = si_SetSkyDef;
	pServer->AddObjectToSky             = si_AddObjectToSky;
	pServer->RemoveObjectFromSky        = si_RemoveObjectFromSky;
	pServer->GetGlobalLightObject       = si_GetGlobalLightObject;
	pServer->SetGlobalLightObject       = si_SetGlobalLightObject;
	pServer->GetPortalFlags             = si_GetPortalFlags;
	pServer->SetPortalFlags             = si_SetPortalFlags;
	pServer->IntersectSegment           = si_IntersectSegment;
	pServer->CastRay                    = si_CastRay;
	pServer->GetPointShade              = si_GetPointShade;
	pServer->FindObjectsTouchingSphere  = sm_FindObjectsTouchingSphere;
	pServer->FindPoliesTouchingBox      = si_FindPoliesTouchingBox;   // AVP2: new, to be reconstructed
	pServer->RelinquishList             = sm_RelinquishList;
	pServer->ObjectToHandle             = si_ObjectToHandle;
	pServer->HandleToObject             = si_HandleToObject;
	pServer->CreateObjectList           = si_CreateObjectList;
	pServer->AddObjectToList            = si_AddObjectToList;
	pServer->RemoveObjectFromList       = si_RemoveObjectFromList;
	pServer->GetPropString              = si_GetPropString;
	pServer->GetPropVector              = si_GetPropVector;
	pServer->GetPropColor               = si_GetPropVector;
	pServer->GetPropReal                = si_GetPropReal;
	pServer->GetPropFlags               = si_GetPropFlags;
	pServer->GetPropBool                = si_GetPropBool;
	pServer->GetPropLongInt             = si_GetPropLongInt;
	pServer->GetPropRotation            = si_GetPropRotation;
	pServer->GetPropRotationEuler       = si_GetPropRotationEuler;
	pServer->GetPropGeneric             = si_GetPropGeneric;
	pServer->DoesPropExist              = si_DoesPropExist;
	pServer->AttachClient               = si_AttachClient;
	pServer->DetachClient               = si_DetachClient;
	pServer->GetNextClient              = si_GetNextClient;
	pServer->GetNextClientRef           = si_GetNextClientRef;
	pServer->GetClientRefInfoFlags      = si_GetClientRefInfoFlags;
	pServer->GetClientRefName           = si_GetClientRefName;
	pServer->GetClientRefObject         = si_GetClientRefObject;
	pServer->GetClientID                = si_GetClientID;
	pServer->GetClientName              = si_GetClientName;
	pServer->SetClientInfoFlags         = si_SetClientInfoFlags;
	pServer->GetClientInfoFlags         = si_GetClientInfoFlags;
	pServer->SetClientUserData          = si_SetClientUserData;
	pServer->GetClientUserData          = si_GetClientUserData;
	pServer->KickClient                 = si_KickClient;
	pServer->SetClientViewPos           = si_SetClientViewPos;
	pServer->RunGameConString           = si_RunGameConString;
	pServer->SetGameConVar              = si_SetGameConVar;
	pServer->GetGameConVar              = si_GetGameConVar;
	pServer->IsCommandOn                = si_IsCommandOn;
	pServer->UpperStrcmp                = ci_UpperStrcmp;
	pServer->GetLastCollision           = si_GetLastCollision;
	pServer->PlaySound                  = si_PlaySound;
	pServer->IsSoundDone                = si_IsSoundDone;
	pServer->GetSoundDuration           = si_GetSoundDuration;
	pServer->KillSound                  = si_KillSound;
	pServer->KillSoundLoop              = si_KillSoundLoop;
	pServer->CreateInterObjectLink      = si_CreateInterObjectLink;
	pServer->BreakInterObjectLink       = si_BreakInterObjectLink;
	pServer->GetSmartLinkBody           = si_GetSmartLinkBody;   // AVP2: new, to be reconstructed
	pServer->ReleaseSmartLinkBody       = si_ReleaseSmartLinkBody;   // AVP2: new, to be reconstructed
	pServer->CreateAttachment           = si_CreateAttachment;
	pServer->RemoveAttachment           = si_RemoveAttachment;
	pServer->FindAttachment             = si_FindAttachment;
	pServer->GetObjectColor             = si_GetObjectColor;
	pServer->SetObjectColor             = si_SetObjectColor;
	pServer->GetObjectUserFlags         = si_GetObjectUserFlags;
	pServer->SetObjectUserFlags         = si_SetObjectUserFlags;
	pServer->GetNextObject              = si_GetNextObject;
	pServer->GetNextInactiveObject      = si_GetNextInactiveObject;
	pServer->SetNextUpdate              = si_SetNextUpdate;
	pServer->SetDeactivationTime        = si_SetDeactivationTime;
	pServer->PingObjects                = si_PingObjects;
	pServer->GetObjectPos               = si_GetObjectPos;
	pServer->SetObjectPos               = si_SetObjectPos;
	pServer->ScaleObject                = si_ScaleObject;
	pServer->GetObjectScale             = si_GetObjectScale;
	pServer->TeleportObject             = si_TeleportObject;
	pServer->GetObjectRotation          = si_GetObjectRotation;
	pServer->SetObjectRotation          = si_SetObjectRotation;
	pServer->RotateObject               = si_RotateObject;
	pServer->GetBlockingPriority        = si_GetBlockingPriority;
	pServer->SetBlockingPriority        = si_SetBlockingPriority;
	pServer->ClipSprite                 = si_ClipSprite;
	pServer->TiltToPlane                = si_TiltToPlane;
	pServer->GetObjectState             = si_GetObjectState;
	pServer->SetObjectState             = si_SetObjectState;
	pServer->GetLightColor              = si_GetLightColor;
	pServer->SetLightColor              = si_SetLightColor;
	pServer->GetLightRadius             = si_GetLightRadius;
	pServer->SetLightRadius             = si_SetLightRadius;
	pServer->GetNextModelNode           = ci_GetNextModelNode;
	pServer->GetModelNodeName           = ci_GetModelNodeName;
	pServer->GetModelCommandString      = si_GetModelCommandString;
	pServer->GetAnimName                = ci_GetAnimName;
	pServer->SetModelPlaying            = si_SetModelPlaying;
	pServer->GetModelPlaying            = si_GetModelPlaying;
	pServer->GetModelFilenames          = si_GetModelFilenames;  // PH: with the third buffer pair (iltserver.h)
	pServer->GetModelFilenames2         = si_GetModelFilenames2;   // AVP2: new, to be reconstructed
	pServer->Parse                      = si_Parse;
	pServer->GetObjectContainers        = si_GetObjectContainers;
	pServer->GetContainedObjects        = si_GetContainedObjects;
	pServer->SaveObjects                = si_SaveObjects;
	pServer->RestoreObjects             = si_RestoreObjects;
	pServer->LoadWorld                  = si_LoadWorld;
	pServer->RunWorld                   = si_RunWorld;
	pServer->UpdateSessionName          = si_UpdateSessionName;
	pServer->GetSessionName             = si_GetSessionName;
	pServer->SendToServerApp            = si_SendToServerApp;
	pServer->GetTcpIpAddress            = si_GetTcpIpAddress;
	pServer->GetFileList                = si_GetFileList;
	pServer->FreeFileList               = fs_FreeFileList;
}


// CServerMgrQ502, the manager members AvP2 added, is declared above
// CServerMgr::LoadWorld, beside the two CRC members it defines.

// AVP2: ILTServer::SetCRCString, 0x00485d50: a string shorter than 0x400
// bytes is copied into the manager's 0x400-byte buffer at +0x66c (strncpy,
// the CRT's 0x004c03e0, with the full 0x400); anything else -- null
// included -- is LT_ERROR, the one trailing `mov $1,%eax`.
LTRESULT si_SetCRCString(char *szCRCFiles)
{
	if (szCRCFiles && strlen(szCRCFiles) < 0x400)
	{
		strncpy((char*)g_pServerMgr->m_Unknown66c, szCRCFiles, 0x400);
		return LT_OK;
	}

	return LT_ERROR;
}

// AVP2: ILTServer::CreateStringCRC (0x00485d90) and CreateWorldCRC
// (0x00485da0), each a tail jump into a manager member.
LTRESULT si_CreateStringCRC()
{
	return ((CServerMgrQ502*)g_pServerMgr)->CreateStringCRC();
}

LTRESULT si_CreateWorldCRC()
{
	return ((CServerMgrQ502*)g_pServerMgr)->CreateWorldCRC();
}

// The three entries the table opens with. GetGameInfo re-reads the manager
// for the second field because the store through the first out-parameter
// could have written it.
LTRESULT si_GetGameInfo(void** ppData, uint32* pLen)
{
	*ppData = g_pServerMgr->m_pGameInfo;
	*pLen = g_pServerMgr->m_GameInfoLen;
	return LT_OK;
}

HCLASS si_GetClass(char* pName)
{
	void* hElement = dhash_FindElement(g_pServerMgr->m_ClassMgr.m_hClassHash, pName, strlen(pName));
	if (hElement)
		return (HCLASS)dhash_GetData(hElement);

	return LTNULL;
}

LTRESULT si_GetStaticObject(HCLASS hClass, HOBJECT* obj)
{
	*obj = LTNULL;
	if (!hClass)
		RETURN_ERROR(1, "ILTPhysics::GetStaticObject", LT_INVALIDPARAMS);

	LTObject* pObject = ((CClassInfo*)hClass)->m_pStaticObject;
	if (pObject)
	{
		*obj = pObject;
		return LT_OK;
	}

	RETURN_ERROR(1, "ILTPhysics::GetStaticObject", LT_ERROR);
}

// ---- the flat ILTServer function-pointer entries (0x1003030b-0x10030c4f) - //
//
// Plain cdecl functions the interface table points at (si_ names ours; the
// error literals still say "ILTPhysics::"). What GetObjectClass hands back
// pins what an HCLASS really is: the CClassInfo slot-table entry at
// m_ClassMgr.m_nIndex — the per-module class record — not the CClassInfo.

HCLASS si_GetObjectClass(LTObject* pObj)
{
	if (!pObj)
		RETURN_ERROR_VALUE_L(1, "ILTPhysics::GetObjectClass", LTNULL, LTNULL);

	return (HCLASS)pObj->m_pSData->m_pClass->m_Stats[g_pServerMgr->m_ClassMgr.m_nIndex];
}

LTBOOL si_IsKindOf(HCLASS hClass, HCLASS hTest)
{
	ClassDef* pDef;

	if (!hClass || !hTest)
		return LTFALSE;

	// A plain `while (pDef)` with hTest's class read inside the test: the
	// guard doubles as the null check on hClass's def (0x1003036d), the
	// invariant load is hoisted below it (0x10030374 / 0x00470747), and the
	// rotated loop falls straight into the FALSE return. A separate
	// `if (!pDef)` plus a pTest local loaded first put the load above the
	// guard on /O2 and a forward `je; jmp` exit on /O1.
	pDef = ((CClassInfo*)hClass)->m_pClassDef;
	while (pDef)
	{
		if (pDef == ((CClassInfo*)hTest)->m_pClassDef)
			return LTTRUE;

		pDef = pDef->m_ParentClass;
	}

	return LTFALSE;
}

LTRESULT si_GetPortalFlags(char* pPortalName, uint32* pFlags)
{
	WorldPortal* pPortal = w_FindPortal(&g_pServerMgr->m_World, pPortalName, 0, 0);
	if (!pPortal)
		RETURN_ERROR(1, "ILTPhysics::GetPortalFlags", LT_NOTFOUND);

	*pFlags = pPortal->m_Flags;
	return LT_OK;
}

LTRESULT si_SetPortalFlags(char* pPortalName, uint32 flags)
{
	return sm_SetPortalFlags(g_pServerMgr, pPortalName, flags);
}

LTRESULT si_GetNextPoly(HPOLY* hPoly)
{
	if (!hPoly)
		RETURN_ERROR(1, "ILTPhysics::GetNextPoly", LT_INVALIDPARAMS);

	CServerMgr* pMgr = g_pServerMgr;
	if (!pMgr->m_pWorldFile)
		RETURN_ERROR(1, "ILTPhysics::GetNextPoly", LT_NOTINITIALIZED);

	HPOLY hCur = *hPoly;
	if (hCur == INVALID_HPOLY)
	{
		// The "before the first poly" marker: world 0, index 0xffff.
		*hPoly = 0xffff;
	}
	else
	{
		uint32 iWorld = hCur >> 16;
		uint32 iPoly = (hCur & 0xffff) + 1;

		if (iPoly >= pMgr->m_pFileTrees[iWorld]->m_pList->m_nPolies)
		{
			iWorld++;
			iPoly = 0;
		}
		if (iWorld >= pMgr->m_nFileTrees)
			return LT_FINISHED;

		*hPoly = (iWorld << 16) | (iPoly & 0xffff);
	}

	return LT_OK;
}

// AVP2: the world model's BSP as GetNextWMPoly reaches it, at +0x1b8 of
// the object (Primal Hunt 0x004869a5): slot 0 is IsBusy, +0xa4 the poly
// count and the uint16 at +0xf8 the world's index, the high half of an
// HPOLY (names ours).
class CWMPolyBspQ502
{
public:
	virtual uint32 IsBusy() = 0;             // slot 0

	uint8  m_pad_04[0xa4 - 0x04];
	uint32 m_nPolies;                        // +0xa4
	uint8  m_pad_a8[0xf8 - 0xa8];
	uint16 m_wIndex;                         // +0xf8
};

class CWMPolyObjQ502
{
public:
	uint8           m_pad_000[0x1b8];
	CWMPolyBspQ502* m_pBsp;                  // +0x1b8

	// The BSP, unless it is busy (name ours).
	CWMPolyBspQ502* GetIdleBsp()
	{
#ifdef AVP2_MODERN
		if (!m_pBsp || m_pBsp->IsBusy())
#else
		if (m_pBsp->IsBusy())
#endif
			return LTNULL;

		return m_pBsp;
	}
};

// AVP2: ILTServer::GetNextWMPoly, 0x00486920 -- GetNextPoly over one world
// model's polies: INVALID_HPOLY starts at its first, the last answers
// LT_FINISHED with the handle set back to INVALID_HPOLY.
//
// The busy test is an inline that hands back the BSP or null: the virtual
// call, then the pointer reloaded and tested (0x004869a5..0x004869bb), and
// one error block after it.
//
// RETAIL BUG: the BSP's IsBusy is called before the BSP pointer is tested,
// so a world model without one crashes instead of answering
// LT_INVALIDDATA.
LTRESULT si_GetNextWMPoly(HPOLY* hPoly, LTObject* hWorldModel)
{
	CWMPolyObjQ502* pObj = (CWMPolyObjQ502*)hWorldModel;

	if (!hPoly)
		RETURN_ERROR(1, "ILTServer::GetNextWMPoly", LT_INVALIDPARAMS);

	if (!hWorldModel)
		RETURN_ERROR(1, "ILTServer::GetNextWMPoly", LT_INVALIDPARAMS);

	CWMPolyBspQ502* pBsp = pObj->GetIdleBsp();
	if (!pBsp)
		RETURN_ERROR(1, "ILTServer::GetNextWMPoly", LT_INVALIDDATA);

	if (*hPoly == INVALID_HPOLY)
	{
		*hPoly = pBsp->m_wIndex << 16;
		return LT_OK;
	}

	uint32 iPoly = (*hPoly & 0xffff) + 1;
	if (iPoly < pBsp->m_nPolies)
	{
		*hPoly = (pBsp->m_wIndex << 16) | (iPoly & 0xffff);
		return LT_OK;
	}

	*hPoly = INVALID_HPOLY;
	return LT_FINISHED;
}

int si_IntRandom(int min, int max)
{
	return rand() % (max - min + 1) + min;
}

// One parameter, not two: this entry fills ILTServer::RandomScale (0x05c),
// while the two-argument Random (0x054) is a different function in the common
// module. Reading the table is what settled that — the body never touched a
// second argument, which read as Monolith ignoring it.
LTFLOAT si_RandomScale(LTFLOAT scale)
{
	return scale * (rand() * (1.0f / 32767.0f));
}

void sm_BroadcastPrintf(char* pMsg, ...);

void si_BPrint(char* pMsg, ...)
{
	char msg[500];
	va_list marker;

	va_start(marker, pMsg);
	_vsnprintf(msg, sizeof(msg) - 1, pMsg, marker);
	sm_BroadcastPrintf(msg);
}

LTRESULT si_GetSkyDef(SkyDef* pDef)
{
	memcpy(pDef, &g_pServerMgr->m_SkyDef, sizeof(SkyDef));
	return LT_OK;
}

LTRESULT si_SetSkyDef(SkyDef* pDef)
{
	memcpy(&g_pServerMgr->m_SkyDef, pDef, sizeof(SkyDef));
	sm_MarkSkyChanged(g_pServerMgr);
	return LT_OK;
}

LTRESULT si_AddObjectToSky(LTObject* pObj, uint32 index)
{
	if (!pObj)
		RETURN_ERROR_INFO(1, "ILTPhysics::AddObjectToSky", LT_ERROR, "object is LTNULL");
	if (index >= 30)
		RETURN_ERROR_INFO(1, "ILTPhysics::AddObjectToSky", LT_ERROR, "invalid index");

	if (!(pObj->m_ChangeFlags & 0x100))
	{
		pObj->m_ChangeFlags |= 0x100;
		g_pServerMgr->m_SkyObjects[index] = pObj->m_ObjectID;
		sm_MarkSkyChanged(g_pServerMgr);
	}

	return LT_OK;
}

LTRESULT si_RemoveObjectFromSky(LTObject* pObj)
{
	if (!pObj)
		RETURN_ERROR_INFO(1, "ILTPhysics::RemoveObjectFromSky", LT_ERROR, "object is LTNULL");

	return sm_RemoveObjectFromSky(g_pServerMgr, pObj);
}

LTRESULT si_GetGlobalLightObject(HOBJECT* hObj)
{
	if (!hObj)
		return LT_ERROR;

	*hObj = g_pServerMgr->m_hGlobalLightObject;
	return LT_OK;
}

LTRESULT si_SetGlobalLightObject(HOBJECT hObj)
{
	g_pServerMgr->SetGlobalLightObject(hObj);
	return LT_OK;
}

LTRESULT si_AttachClient(Client* pParent, Client* pChild)
{
	if (!pParent || !pChild)
		RETURN_ERROR(1, "ILTPhysics::AttachClient", LT_INVALIDPARAMS);

	return sm_AttachClient(g_pServerMgr, pParent, pChild);
}

LTRESULT si_DetachClient(Client* pClient)
{
	if (!pClient)
		RETURN_ERROR(1, "ILTPhysics::DetachClient", LT_INVALIDPARAMS);

	return sm_DetachClient(g_pServerMgr, pClient);
}

LTRESULT si_GetLastCollision(CollisionInfo* pInfo)
{
	if (!pInfo)
		RETURN_ERROR(1, "ILTPhysics::GetLastCollision", LT_INVALIDPARAMS);

	if (!g_pServerMgr->m_pCollisionInfo)
		return LT_ERROR;

	// Statement-per-field, each re-walking the global chain — which is why
	// the shipped bytes reload g_pServerMgr four times.
	pInfo->m_Plane = g_pServerMgr->m_pCollisionInfo->m_Plane;
	pInfo->m_hObject = g_pServerMgr->m_pCollisionInfo->m_hObject;
	pInfo->m_hPoly = g_pServerMgr->m_pCollisionInfo->m_hPoly;
	pInfo->m_vStopVel = g_pServerMgr->m_pCollisionInfo->m_vStopVel;
	return LT_OK;
}

// The old flat sound API: CServerSoundMgr is CServerMgr's second base, so
// the calls below carry the +4 this-adjust the shipped bytes show.

LTRESULT si_PlaySound(PlaySoundInfo* pPlaySoundInfo)
{
	HLTSOUND hResult;
	return g_pServerMgr->PlaySound(pPlaySoundInfo, hResult);
}

LTRESULT si_GetSoundDuration(HLTSOUND hSound, LTFLOAT* fDuration)
{
	return g_pServerMgr->GetSoundDuration(hSound, *fDuration);
}

LTRESULT si_IsSoundDone(HLTSOUND hSound, LTBOOL* bDone)
{
	return g_pServerMgr->IsSoundDone(hSound, *bDone);
}

LTRESULT si_KillSound(HLTSOUND hSound)
{
	return g_pServerMgr->KillSound(hSound);
}

LTRESULT si_KillSoundLoop(HLTSOUND hSound)
{
	return g_pServerMgr->KillSoundLoop(hSound);
}

LTRESULT si_CreateInterObjectLink(LTObject* pOwner, LTObject* pLinked)
{
	if (!pOwner || !pLinked)
		RETURN_ERROR(1, "ILTPhysics::CreateInterObjectLink", LT_INVALIDPARAMS);

	return sm_CreateInterObjectLink(g_pServerMgr, pOwner, pLinked, 0);
}

void si_BreakInterObjectLink(LTObject* pOwner, LTObject* pLinked)
{
	if (!pOwner || !pLinked)
		return;

	sm_BreakInterObjectLink(g_pServerMgr, pOwner, pLinked, 0);
}

// AVP2: the smart-link body an owner keeps, found or made by 0x00447c70
// (a walk of the list at 0x004f5454 matching the owner at +0x0c, answering
// the record's +0x08), and let go by 0x00447d00. Names ours; Primal Hunt's
// are interlink.cpp's (server.dll 0x10018c6b / 0x10018cba).
LTSmartLink_Body* sm_GetSmartLinkBodyQ502(LTObject* pOwner);         // 0x00447c70
void              sm_ReleaseSmartLinkBodyQ502(LTSmartLink_Body* pBody);   // 0x00447d00

// AVP2: ILTServer::GetSmartLinkBody, 0x00487340. The body is linked to its
// owner as an inter-object link of type 3 (sm_CreateInterObjectLink,
// 0x00447f70), whose answer is the result; no body is LT_OK with *pRet
// null. The error literal is "CreateSmartLink"; the failure block is the
// trailing statement (0x00487380), so the test is a positive guard.
LTRESULT si_GetSmartLinkBody(LTObject* hOwner, LTSmartLink_Body** pRet)
{
	if (hOwner && pRet)
	{
		LTRESULT dResult = LT_OK;

		*pRet = LTNULL;
		*pRet = sm_GetSmartLinkBodyQ502(hOwner);
		if (*pRet)
			dResult = sm_CreateInterObjectLink(g_pServerMgr, hOwner, (LTObject*)*pRet, 3);

		return dResult;
	}

	RETURN_ERROR(1, "ILTPhysics::CreateSmartLink", LT_INVALIDPARAMS);
}

// AVP2: ILTServer::ReleaseSmartLinkBody, 0x004873c0.
LTRESULT si_ReleaseSmartLinkBody(LTSmartLink_Body* pBody)
{
	if (!pBody)
		RETURN_ERROR(1, "ILTPhysics::ReleaseSmartLinkBody", LT_INVALIDPARAMS);

	sm_ReleaseSmartLinkBodyQ502(pBody);
	return LT_OK;
}

#ifdef PRIMAL_HUNT
// PH: an attachment's world transform -- the offset turned by the parent's
// rotation and added to its position, and the two rotations composed --
// (server.dll 0x1002a7ed, 157 bytes; not yet reconstructed, name ours).
void sm_GetAttachmentTransformQ523(LTObject* pParent, Attachment* pAttach,
                                   LTVector* pPos, LTRotation* pRot);
#endif

LTRESULT si_CreateAttachment(LTObject* hParent, LTObject* hChild, char* pSocketName,
                             LTVector* pOffset, LTRotation* pRotationOffset,
                             Attachment** pAttachment)
{
	int iNode;
	Attachment* pAttach;

	if (!hParent || !hChild)
		RETURN_ERROR(1, "ILTPhysics::CreateAttachment", LT_INVALIDPARAMS);

	iNode = -1;
	if (hParent->m_ObjectType == OT_MODEL && pSocketName)
	{
		Model* pModel = ((ModelInstance*)hParent)->m_AnimTracker.m_TimeRef.m_pModel;
		if (!pModel)
			RETURN_ERROR(1, "ILTPhysics::CreateAttachment", LT_INVALIDDATA);

		// Sockets first; a node index is biased past them.
		if (!pModel->FindSocket(pSocketName, (HMODELSOCKET*)&iNode))
		{
			if (!pModel->FindNode(pSocketName, (HMODELNODE*)&iNode))
				RETURN_ERROR(1, "ILTPhysics::CreateAttachment", LT_NODENOTFOUND);
			iNode += pModel->m_Sockets.m_nElements;
		}
	}

	LTRESULT dResult = am_CreateAttachment(&g_pServerMgr->m_Unknown390, hParent,
		hChild->m_ObjectID, iNode, pOffset, pRotationOffset, &pAttach);
	if (dResult != LT_OK)
		return dResult;

#ifdef PRIMAL_HUNT
	// PH: the child is put where the attachment holds it at once, and
	// marked attached -- change bit 0x400, which si_RemoveAttachment clears
	// -- with the attachment change flagged on both (server.dll
	// 0x1003e37d..0x1003e3c3).
	{
		LTVector   vPos;
		LTRotation rRot;

		sm_GetAttachmentTransformQ523(hParent, pAttach, &vPos, &rRot);
		si_SetObjectPos(hChild, &vPos);
		si_SetObjectRotation(hChild, &rRot);

		sm_SetObjectChangeFlag(g_pServerMgr, hParent, 0x100);
		hChild->m_ChangeFlags |= 0x400;
		sm_SetObjectChangeFlag(g_pServerMgr, hChild, 0x100);
	}
#else
	sm_SetObjectChangeFlag(g_pServerMgr, hParent, 0x100);
#endif
	if (pAttachment)
		*pAttachment = pAttach;
	return LT_OK;
}

LTRESULT si_RemoveAttachment(Attachment* hAttachment)
{
	if (!hAttachment)
		RETURN_ERROR(1, "ILTPhysics::RemoveAttachment", LT_INVALIDPARAMS);

	LTObject* pParent = sm_FindObjectByID(g_pServerMgr, hAttachment->m_nParentID);
	if (!pParent)
		RETURN_ERROR(1, "ILTPhysics::RemoveAttachment", LT_ERROR);

#ifdef PRIMAL_HUNT
#ifdef AVP2_MODERN
	uint16 nChildID = hAttachment->m_nChildID;
#endif
#endif

	if (am_RemoveAttachment(&g_pServerMgr->m_Unknown390, pParent, hAttachment) == LT_OK)
	{
		sm_SetObjectChangeFlag(g_pServerMgr, pParent, 0x100);

#ifdef PRIMAL_HUNT
		// PH: the child is let go as well -- change bit 0x400 cleared and
		// the attachment change flagged on it too (server.dll 0x1003e4c9..
		// 0x1003e4f4).
		// RETAIL BUG: the child's id is read out of the attachment record
		// AFTER am_RemoveAttachment has handed it back to its bank. Only the
		// record's first dword is written by the free, so the id survives,
		// but it is a read of freed memory; the modern build reads it first.
#ifdef AVP2_MODERN
		LTObject* pChild = sm_FindObjectByID(g_pServerMgr, nChildID);
#else
		LTObject* pChild = sm_FindObjectByID(g_pServerMgr, hAttachment->m_nChildID);
#endif

		if (pChild)
		{
			pChild->m_ChangeFlags &= ~0x400;
			sm_SetObjectChangeFlag(g_pServerMgr, pChild, 0x100);
		}
#endif

		return LT_OK;
	}

	RETURN_ERROR(1, "ILTPhysics::RemoveAttachment", LT_NOTFOUND);
}

LTRESULT si_FindAttachment(LTObject* hParent, LTObject* hChild, Attachment** pAttachment)
{
	if (!hParent || !hChild || !pAttachment)
		RETURN_ERROR(1, "ILTPhysics::FindAttachment", LT_INVALIDPARAMS);

	*pAttachment = LTNULL;
	Attachment* pCur = hParent->m_pAttachments;
	while (pCur)
	{
		if ((LTObject*)sm_FindObjectByID(g_pServerMgr, pCur->m_nChildID) == hChild)
		{
			*pAttachment = pCur;
			return LT_OK;
		}
		pCur = pCur->m_pNext;
	}

	// Not an error worth printing: the silent not-found return.
	return LT_NOTFOUND;
}

// The color change-flag thunk both color setters share (name ours).
void sm_ObjectColorChanged(LTObject* pObj)
{
	sm_SetObjectChangeFlag(g_pServerMgr, pObj, 0x40);
}

LTBOOL si_GetObjectColor(LTObject* hObject, float* r, float* g, float* b, float* a)
{
	if (!hObject)
		return LTFALSE;

	float fConv = 1.0f / 255.0f;
	if (r)
		*r = hObject->m_ColorR * fConv;
	if (g)
		*g = hObject->m_ColorG * fConv;
	if (b)
		*b = hObject->m_ColorB * fConv;
	if (a)
		*a = hObject->m_ColorA * fConv;

	return LTTRUE;
}

LTBOOL si_SetObjectColor(LTObject* hObject, float r, float g, float b, float a)
{
	if (!hObject)
		return LTFALSE;

	uint8 nColor[4];

	float fConv = 255.0f;
	nColor[0] = (uint8)(r * fConv);
	nColor[1] = (uint8)(g * fConv);
	nColor[2] = (uint8)(b * fConv);
	nColor[3] = (uint8)(a * fConv);

	if (hObject->m_ColorR != nColor[0] || hObject->m_ColorG != nColor[1] ||
	    hObject->m_ColorB != nColor[2] || hObject->m_ColorA != nColor[3])
	{
		hObject->m_ColorR = nColor[0];
		hObject->m_ColorG = nColor[1];
		hObject->m_ColorB = nColor[2];
		hObject->m_ColorA = nColor[3];
		sm_ObjectColorChanged(hObject);
	}

	return LTTRUE;
}

uint32 si_GetObjectUserFlags(LTObject* hObj)
{
	if (!hObj)
		return 0;

	return hObj->m_UserFlags;
}

LTRESULT si_SetObjectUserFlags(LTObject* hObj, uint32 flags)
{
	if (!hObj)
		RETURN_ERROR(1, "ILTPhysics::SetObjectUserFlags", LT_ERROR);

	if (hObj->m_UserFlags != flags)
	{
		hObj->m_UserFlags = flags;
		sm_SetObjectChangeFlag(g_pServerMgr, hObj, 8);
	}

	return LT_OK;
}

// The 0x8dc list holds every object: actives walked forward from the head,
// inactives (state bits 0x38) from the tail.
//
// Each arm carries its own end-of-list test (shipped lt 0x472110 keeps the
// two `cmp; jne; xor; ret` copies apart, srv 0x10031523 cross-jumps them).
// With one shared test after the if/else both images hoisted the
// g_pServerMgr load above the branch; the per-arm copy loads it in each arm
// as shipped (lt 0x4720ce/0x4720d9, srv 0x100314ea/0x100314f5). Identical on
// both images. GetObjectState (3 bytes) still keeps a movzbl our compilers
// prove away against the following mask.

LTObject* si_GetNextObject(LTObject* hObj)
{
	LTLink* pLink;

	if (hObj)
	{
		pLink = hObj->m_pSData->m_StateLink.m_pNext;
		if (pLink == &g_pServerMgr->m_List8dc.m_Head)
			return LTNULL;
	}
	else
	{
		pLink = g_pServerMgr->m_List8dc.m_Head.m_pNext;
		if (pLink == &g_pServerMgr->m_List8dc.m_Head)
			return LTNULL;
	}

	LTObject* pObj = (LTObject*)pLink->m_pData;
#ifdef PRIMAL_HUNT
	return (pObj->m_ChangeFlags & 0x38) ? LTNULL : pObj;
#else
	// AvP2 1.0.9.6 tests the low byte alone (server.dll 0x1003acae /
	// 0x1003acf6 `mov 0x124(%eax),%cl`).
	return ((uint8)pObj->m_ChangeFlags & 0x38) ? LTNULL : pObj;
#endif
}

LTObject* si_GetNextInactiveObject(LTObject* hObj)
{
	LTLink* pLink;

	if (hObj)
	{
		pLink = hObj->m_pSData->m_StateLink.m_pPrev;
		if (pLink == &g_pServerMgr->m_List8dc.m_Head)
			return LTNULL;
	}
	else
	{
		pLink = g_pServerMgr->m_List8dc.m_Head.m_pPrev;
		if (pLink == &g_pServerMgr->m_List8dc.m_Head)
			return LTNULL;
	}

	LTObject* pObj = (LTObject*)pLink->m_pData;
#ifdef PRIMAL_HUNT
	return (pObj->m_ChangeFlags & 0x38) ? pObj : LTNULL;
#else
	// AvP2 1.0.9.6 tests the low byte alone (server.dll 0x1003acae /
	// 0x1003acf6 `mov 0x124(%eax),%cl`).
	return ((uint8)pObj->m_ChangeFlags & 0x38) ? pObj : LTNULL;
#endif
}

void si_SetNextUpdate(LTObject* hObj, LTFLOAT nextUpdate)
{
	if (!hObj)
		return;

	hObj->m_pSData->m_NextUpdate = nextUpdate;
}

void si_SetDeactivationTime(LTObject* hObj, LTFLOAT fDeactivationTime)
{
	if (!hObj)
		return;

	hObj->m_pSData->m_DeactivationTime = fDeactivationTime;
	hObj->m_pSData->m_UpdateTimer = fDeactivationTime;

	if (hObj->m_ChangeFlags & 0x20)
		sm_SetObjectState(g_pServerMgr, hObj, hObj->m_ChangeFlags & 0x18);

	if ((g_bAutoDeactivate && hObj->m_Unknown58 == -1) || !g_pServerMgr->m_pWorldFile)
		g_pServerMgr->m_Unknown210.BumpObject(hObj, 0);
}

void si_PingObjects(LTObject* hObj)
{
	if (!hObj)
		return;

	sm_PingObjects(g_pServerMgr, hObj);
}

void si_GetObjectPos(LTObject* hObj, LTVector* pos)
{
	if (!hObj)
	{
		pos->x = 0.0f;
		pos->y = 0.0f;
		pos->z = 0.0f;
		return;
	}

	*pos = hObj->m_Pos;
}

void si_SetObjectPos(LTObject* hObj, LTVector* pos)
{
	if (!hObj)
		return;

	sm_MoveObject((IWorldSim*)g_pServerMgr, hObj, pos, 0xf);
}

void si_SetObjectState(LTObject* hObj, int state)
{
	uint32 nBits;

	if (!hObj)
		return;

	switch (state)
	{
		default:
			nBits = 0;
			break;

		case OBJSTATE_AUTODEACTIVATE_NOW:
			if (!g_bAutoDeactivate)
				return;
			nBits = 0x20;
			break;

		case OBJSTATE_INACTIVE_TOUCH:
			nBits = 0x10;
			break;

		case OBJSTATE_INACTIVE:
			nBits = 8;
			break;
	}

	sm_SetObjectState(g_pServerMgr, hObj, nBits);
}

int si_GetObjectState(LTObject* hObj)
{
	uint32 nFlags;

	// Two separate returns tail-merge, keeping the inactive one inline; the
	// value temp keeps the flags in a register instead of an address temp.
	// The last two states are an if/else chain, not `(nFlags >> 3) & 2`:
	// VC6 derives the shift from the select and keeps the movzbl (server
	// 0x100316aa) / `and 0ffh` (lithtech 0x0047230e) that a written shift
	// loses. The else-if also puts the null refusal in the shipped place.
	if (!hObj)
		return OBJSTATE_INACTIVE;

	nFlags = hObj->m_ChangeFlags;
	if (nFlags & 8)
		return OBJSTATE_INACTIVE;
	else if (nFlags & 0x10)
		return OBJSTATE_INACTIVE_TOUCH;
	else
		return OBJSTATE_ACTIVE;
}

LTRESULT si_GetObjectRotation(LTObject* hObj, LTRotation* pRotation)
{
	if (!hObj)
	{
		pRotation->Init();
		RETURN_ERROR(1, "ILTPhysics::GetObjectRotation", LT_ERROR);
	}

	*pRotation = hObj->m_Rotation;
	return LT_OK;
}

// The six-field WMMoveState fill, as commonphysics.cpp's wm_InitMoveState
// (static there, so repeated here as a stand-in of the same shape; name
// ours). sm_SetObjectRotation below needs the helper, not six stores:
// its arguments are evaluated ahead of the body, which is what loads the
// 0x6a8 object before the container is formed and parks the priority in
// edx (lithtech 0x472478..0x4724a4). With the stores written out, lithtech
// was four blocks off and server.dll held the zero in edx instead of edi.
// The type test is likewise an inline helper (commonphysics.cpp's
// wm_IsWorldModelType shape): with a `char nType` local, lithtech laid the
// rotation-copy arm first and duplicated sm_SetObjectChangeFlag's error
// tail (523 bytes vs 500), and server.dll tested the type as a switch
// (`movsbl ; dec ; dec`).
inline LTBOOL sm_IsWorldModelType(LTObject* pObj)
{
	return pObj->m_ObjectType == OT_WORLDMODEL ||
	       pObj->m_ObjectType == OT_CONTAINER;
}

static inline void sm_InitMoveState(WMMoveState* pState, void* pContainer,
                                    void* pUnknown04, LTObject* pObj,
                                    uint32 nPriority)
{
	pState->m_pContainer = (CUnknown210*)pContainer;
	pState->m_pUnknown04 = (CUnknown6A8Obj*)pUnknown04;
	pState->m_pObj       = pObj;
	pState->m_nUnknown0C = nPriority;
	pState->m_pPushList = 0;
	pState->m_nPushCount = 0;
}

LTRESULT sm_SetObjectRotation(LTObject* hObj, LTRotation* pRotation, int bGuaranteed)
{
	WMMoveState state;

	if (!hObj)
		RETURN_ERROR(1, "ILTPhysics::SetObjectRotation", LT_ERROR);

	if (hObj->m_Rotation.m_Quat[0] == pRotation->m_Quat[0] &&
	    hObj->m_Rotation.m_Quat[1] == pRotation->m_Quat[1] &&
	    hObj->m_Rotation.m_Quat[2] == pRotation->m_Quat[2] &&
	    hObj->m_Rotation.m_Quat[3] == pRotation->m_Quat[3])
		return LT_OK;

	if (sm_IsWorldModelType(hObj))
	{
		sm_InitMoveState(&state, &g_pServerMgr->m_Unknown210,
		                 g_pServerMgr->m_pUnknown6a8, hObj, hObj->m_BlockingPriority);
		wm_RotateWorldModel(&state, pRotation, 1);
	}
	else
	{
		hObj->m_Rotation = *pRotation;
	}

	sm_SetObjectChangeFlag(g_pServerMgr, hObj, (bGuaranteed ? 0x400 : 0) | 4);
	return LT_OK;
}

LTRESULT si_SetObjectRotation(LTObject* hObj, LTRotation* pRotation)
{
	return sm_SetObjectRotation(hObj, pRotation, 1);
}

LTRESULT si_RotateObject(LTObject* hObj, LTRotation* pRotation)
{
	return sm_SetObjectRotation(hObj, pRotation, 0);
}

void si_ScaleObject(LTObject* hObj, LTVector* pScale)
{
	hObj->m_Scale = *pScale;
	sm_SetObjectChangeFlag(g_pServerMgr, hObj, 0x10);
}

LTRESULT si_GetObjectScale(LTObject* hObj, LTVector* pScale)
{
	if (!hObj || !pScale)
		RETURN_ERROR(1, "ILTPhysics::GetObjectScale", LT_ERROR);

	*pScale = hObj->m_Scale;
	return LT_OK;
}

LTRESULT si_GetModelCommandString(LTObject* hObj, char* pStr, uint32 maxLen)
{
	if (!hObj || !pStr || hObj->m_ObjectType != OT_MODEL)
		RETURN_ERROR(1, "ILTPhysics::GetModelCommandString", LT_INVALIDPARAMS);

	// The local names the MODEL and the command string is read at both uses:
	// naming the string instead is a one-register near in lithtech.exe
	// (0x00472c10, 170 of 169 bytes: the first error arm's format pointer
	// goes to %ecx where shipped reuses the dead %eax).
	Model* pModel = ((ModelInstance*)hObj)->m_AnimTracker.m_TimeRef.m_pModel;
	if (!pModel->m_pCommandString)
		RETURN_ERROR(1, "ILTPhysics::GetModelCommandString", LT_ERROR);

	// The full buffer length — no reserve for the terminator here.
	strncpy(pStr, pModel->m_pCommandString, maxLen);
	return LT_OK;
}

// am_RemoveAttachment (0x10025a1b / 0x0045bf10) is objbank.cpp's, the one
// definition; servermgr.h's declaration reaches it through link/aliases.rsp,
// as am_CreateAttachment's does. The second definition that stood here
// matched server.dll too but was two bytes off in lithtech.exe, and so was a
// function the client image has no counterpart for.

// The masked flag word is the `flags` PARAMETER, narrowed in place, not a
// local initialised from it — and that one word of source is what decides
// the callee-saved register assignment. With `uint32 nNew = flags & 1;` the
// local outranks the portal record and takes esi, leaving edi for the
// record; masking the parameter itself (`flags &= 1;`) starts its live range
// at function entry, which drops it below the record and produces the
// shipped esi=record / edi=flags pairing exactly. 232 of 232 bytes, and
// what had been written up as a register lottery was a source fact.
LTRESULT sm_SetPortalFlags(CServerMgr* pMgr, char* pPortalName, uint32 flags)
{
	CMessageRef msg;
	uint32 nWorld;
	uint32 nPortal;
	WorldPortal* pPortal;

	pPortal = w_FindPortal(&pMgr->m_World, pPortalName, &nWorld, &nPortal);
	if (!pPortal)
		RETURN_ERROR(1, "ILTServer::SetPortalFlags", LT_NOTFOUND);

	flags &= 1;
	if ((uint16)flags != pPortal->m_Flags)
	{
		msg.Init(msg_AllocPacket(0x44c, 0x44c));

		msg.m_pMsg->WriteWord((uint16)nWorld);
		msg.m_pMsg->WriteWord((uint16)nPortal);

		// The BYTE goes through CPacket::WriteType<uint8> and the two words
		// do not, and lithtech.exe says so directly: at 0x00476790 the two
		// word writes are still calls (0x00476845, 0x00476851 -> 0x0042fa50)
		// and the byte write is EXPANDED at 0x00476856 -- `inc %edx` on
		// m_Pos, `inc %ecx` on m_DataLen, the SetAtGrow slow path at
		// 0x00476875 and the "CPacket::WriteType" / "ERROR: Packet write
		// overrun!" REPORT_ERROR at 0x004768b8, ending in
		// `or $0x4,0x40(%esi)` (PACKETFLAG_WRITEOVERFLOW). A member TEMPLATE
		// is an inline candidate at /Ob1 with or without the keyword;
		// ServerMessage::WriteWord is a non-template declared out of class,
		// which /Ob1 never expands. Same lever, same file, as
		// sm_DetachClient's WriteType<uint16> above.
		//
		// server.dll cannot move: ServerMessage::WriteByte IS
		// CPacket::WriteType<uint8> (both 0x1001393e -- netmgr.cpp says so),
		// so /O1 emits `call <same address>` for either spelling and
		// the byte comparison masks the relocated bytes.
		((CPacket*)msg.m_pMsg)->WriteType((uint8)flags);
		sm_BroadcastToClients(g_pServerMgr, 0x13, msg.m_pMsg);
	}

	pPortal->m_Flags = (uint16)flags;
	return LT_OK;
}

// What sm_SendSFXMessage hands its callbacks (names ours).
struct SFXSendInfo
{
	CServerMgr*    m_pMgr;
	ServerMessage* m_pMsg;
	LTObject*      m_pObject;
	uint8          m_nPacketID;
	uint32         m_Flags;
};

// The vis-query callbacks (names ours): the first sends to one client-owned
// object's owner, the second collects client-owned objects from a list.
void sm_SFXSendToOwner(LTObject* pObj, SFXSendInfo* pInfo)
{
	if (pObj->m_Unknown54 == 0 && pObj->m_pSData->m_pOwningClient)
	{
		sm_QueueMessageToClient(pInfo->m_pMgr, pObj->m_pSData->m_pOwningClient,
			pInfo->m_nPacketID, pInfo->m_pMsg, pInfo->m_Flags);
	}
}

void sm_SFXGatherOwned(LTLink* pListHead, LTObject*** pppList, uint32* pnCount)
{
	LTLink* pCur = pListHead->m_pNext;
	while (pCur != pListHead)
	{
		LTObject* pObj = (LTObject*)pCur->m_pData;
		if (!pObj->m_pSData)
			return;
		if (pObj->m_pSData->m_pOwningClient)
		{
			(*pppList)[*pnCount] = pObj;
			(*pnCount)++;
		}
		pCur = pCur->m_pNext;
	}
}

LTRESULT sm_SendSFXMessage(CServerMgr* pMgr, int nPacketID, ServerMessage* pMsg,
                           LTObject* pObj, LTVector* pPos, uint32 flags)
{
	CVisQuery query;
	SFXSendInfo info;

	if (pMgr->m_ClientList.m_nElements == 0)
		return LT_OK;

	// One client, still connecting: send straight to it.
	if (pMgr->m_ClientList.m_nElements == 1)
	{
		Client* pClient = (Client*)pMgr->m_ClientList.m_Head.m_pNext->m_pData;
		if (pClient->m_ClientFlags & 2)
		{
			sm_QueueMessageToClient(pMgr, pClient, nPacketID, pMsg, flags);
			return LT_OK;
		}
	}

	info.m_pMgr = pMgr;
	info.m_pMsg = pMsg;
	info.m_pObject = pObj;
	info.m_nPacketID = (uint8)nPacketID;
	// Flags before the position copy: lithtech 0x4769c9 loads the flags
	// argument ahead of pPos (server.dll is identical either way).
	info.m_Flags = flags;
	query.m_Position = *pPos;
	query.m_pUser = (CUpdateContext*)&info;
	query.m_Radius = 10000.0f;
	query.m_Fn1 = (void (*)(void*, void*))sm_SFXSendToOwner;
	query.m_Fn2 = (int (*)(void*, void*))sm_SFXGatherOwned;

	((IObjectQuery*)&pMgr->m_Unknown210)->FindObjects(&query);
	return LT_OK;
}

// The client-only statement inside sm_CacheFile's FT_MODEL arm, on the same
// LT_SERVER_BUILD discriminator serverengine.cpp's se_RegisterModel already
// uses (see the long note there; the macro's real name is not known).
// lithtech.exe's copy (0x00476ac0) has
//
//     push %ebx ; or %ebx,0x20(%eax) ; call 0x004149e0 ; add $0x4,%esp
//
// at 0x00476b56 where server.dll's copy (0x10034ca5) has only the `or`.
// 0x004149e0 is src/engine/lithtech/clientmgr2.cpp's cs_Sub4149e0 (already
// byte-identical there) -- reconstructed as no-argument, because its body
// never reads one, but this call site pushes a dword and cleans four bytes,
// so the prototype here takes one. Without the call the /O2 epilogue is
// scheduled ahead of the `|=` and the read-modify-write splits into
// load/or/store, which is the whole four-byte gap: with it both images are
// byte-exact.
#ifndef LT_SERVER_BUILD
void cs_Sub4149e0(int n);          // 0x004149e0 (client only)
#endif

// 0x10034b7a / 0x00476ac0 -- ILTServer::CacheFile's body. BYTE-IDENTICAL in
// both images. Four source facts, each one measured:
//
//  * IT IS A SWITCH, not an if/else chain. lithtech.exe settles it: /O2 emits
//    `cmp $0x3,%ebx ; ja default ; jmp *0x476cb0(,%ebx,4)` over a four-entry
//    jump table at 0x00476cb0. server.dll's /O1 spells the same switch as a
//    linear range test (`cmp %eax,%edi ; je case0 ; jbe default ; cmp $0x2
//    ; jbe case12 ; cmp $0x3 ; jne default`) -- the `jbe` after a compare
//    against a register known to hold zero is the giveaway: it is the
//    range tree's "below the first case" edge, not anything a hand-written
//    `fileType >= 1` would produce. Written as an if/else chain the function
//    came out 384 of 383 with fourteen differing mnemonic blocks; as a
//    switch, four. VC6 emits the case BODIES in reverse source order at /O1,
//    which is why the shipped layout runs case 3, case 1-2, case 0 while the
//    tests run 0, 1-2, 3.
//
//  * THE FT_SPRITE/FT_TEXTURE ARM HAS NO CACHE-RECORD BRANCH. Its shipped
//    block (0x10034c41) goes straight from the fs_GetFileIdentifier test to
//    `if (pFile) sm_BroadcastCacheFile(...)` with no `testb $0x10,0x1a0`
//    anywhere -- only FT_MODEL and FT_SOUND consult the flag. Worth 24 bytes.
//
//  * THE FT_MODEL ARM IS AN IF/ELSE, NOT A GUARD. All three arms raise the
//    same RETURN_ERROR_INFO and VC6 tail-merges them, but the FT_MODEL copy
//    can only share the suffix from the `jl` on (esi holds 1 there, so its
//    g_DebugLevel compare is `cmp %esi,mem` and the other two `cmpl $0x1,mem`).
//    Which copy hosts that suffix follows from which one is emitted LAST, and
//    writing the FT_MODEL test as `if (... == LT_OK) { ... } else { RETURN }`
//    sinks its error block past the case body to the end of the function --
//    exactly the shipped 0x10034cc8. As a guard (`if (... != LT_OK) RETURN;`)
//    the error block is the fall-through instead, the FT_SOUND/FT_SPRITE copy
//    ends up hosting the suffix, and the function is 397 of 383 -- same 134
//    instructions, fourteen bytes of near-vs-short jumps.
//
//  * pFile IS ONE FUNCTION-SCOPE LOCAL, not one per arm. Three block-scoped
//    copies put FT_MODEL's pFile in the pFilename parameter slot and pModel
//    in pMgr's; the shipped frame has pFile at 0x8(%ebp) in all three arms
//    and pModel at 0x10(%ebp). Last four bytes. (Swapping the two block-scoped
//    declarations instead changes nothing -- measured.)
LTRESULT sm_CacheFile(CServerMgr* pMgr, uint32 fileType, char* pFilename)
{
	CFileIdentifier* pFile;

	if (!g_bCachingEnabled)
		return LT_OK;

	if (!pFilename)
		RETURN_ERROR(1, "ILTServer::CacheFile", LT_ERROR);

	switch (fileType)
	{
		case FT_MODEL:
		{
			Model* pModel;

			if (sm_LoadModel(pMgr, pFilename, &pModel, &pFile, 1, 1) == LT_OK)
			{
				if (pMgr->m_Unknown1a0 & 0x10)
				{
					pModel->m_Flags |= 1;
#ifndef LT_SERVER_BUILD
					cs_Sub4149e0(1);
#endif
				}
				else if (pFile)
				{
					sm_BroadcastCacheFile(pMgr, 0, pFile->m_FileID);
				}
			}
			else
			{
				RETURN_ERROR_INFO(1, "ILTServer::CacheFile", LT_NOTFOUND, pFilename);
			}
		}
		break;

		case FT_SPRITE:
		case FT_TEXTURE:
		{
			if (!fs_GetFileIdentifier(&pMgr->m_FileMgr, pFilename, 0, (void**)&pFile))
				RETURN_ERROR_INFO(1, "ILTServer::CacheFile", LT_NOTFOUND, pFilename);

			if (pFile)
			{
				sm_BroadcastCacheFile(pMgr, fileType, pFile->m_FileID);
			}
		}
		break;

		case FT_SOUND:
		{
			if (!fs_GetFileIdentifier(&pMgr->m_FileMgr, pFilename, 0, (void**)&pFile))
				RETURN_ERROR_INFO(1, "ILTServer::CacheFile", LT_NOTFOUND, pFilename);

			if (pMgr->m_Unknown1a0 & 0x10)
			{
				CCacheRec* pRec = pMgr->AddCachedFile(pFile);
				if (pRec)
					pRec->m_bWanted = 1;
			}
			else if (pFile)
			{
				sm_BroadcastCacheFile(pMgr, 3, pFile->m_FileID);
			}
		}
		break;
	}

	return LT_OK;
}


// ---- the object map's array ---------------------------------------------- //

// 0x1000c4cf (Init) and 0x1002cbe3 (the constructor) are
// CMoArray<CObjectMapEntry>'s own; see servermgr.h.


// ---- the world teardown -------------------------------------------------- //

// 0x1002af34 — every object still in the world goes out through the ordinary
// removal, so the game hears about each one. Objects the flag does not mark
// are left for the caller (name ours).
//
// The head is named FIRST and the cursor initialised through it. Written the
// other way round (`pCur = pMgr->m_List8dc.m_Head.m_pNext; pHead =
// &pMgr->m_List8dc.m_Head;`) the instruction sequence is identical and all
// three callee-saved registers rotate — ebx/esi/edi hold the head, the next
// link and the manager instead of the manager, the head and the next link.
// Same lever as sub_1002af00 and sm_EndWorldB.
void sm_EndWorldA(CServerMgr* pMgr)
{
	LTLink* pHead = &pMgr->m_List8dc.m_Head;
	LTLink* pCur = pHead->m_pNext;

	while (pCur != pHead)
	{
		LTLink*   pNext = pCur->m_pNext;
		LTObject* pObject = (LTObject*)pCur->m_pData;

		if (pObject->m_ChangeFlags & 0x80)
			sm_RemoveObjectFromWorld(pMgr, pObject->m_pSData->m_pBaseClass);

		pCur = pNext;
	}
}

// 0x1002aec2 — the other list holds records the manager owns outright, so
// they go straight back to the allocator and the list is tied off empty
// (name ours).
//
// Head-first, cursor through the head, for the same reason sm_EndWorldA is.
void sm_EndWorldB(CServerMgr* pMgr)
{
	LTLink* pHead = &pMgr->m_List8ec.m_Head;
	LTLink* pCur = pHead->m_pNext;

	while (pCur != pHead)
	{
		LTLink* pNext = pCur->m_pNext;

		dfree(pCur->m_pData);

		pCur = pNext;
	}

	dl_InitList(&pMgr->m_List8ec);
}


// 0x1002abf8 — an object goes back to its class's bank. The game's destructor
// runs first, through the class definition's own pointer, because the object
// belongs to the game module; the memory then goes onto the per-class free
// list rather than back to the allocator (name ours).
void sm_FreeObject(CServerMgr* pMgr, ClassDef* pClassDef, LPBASECLASS pObject)
{
	CClassStats* pStats =
		(CClassStats*)pClassDef->m_pInternal[pMgr->m_ClassMgr.m_nIndex];

	pClassDef->m_DestructFn(pObject);

	sb_Free(&pStats->m_Bank, pObject);
}


// 0x1003401a — a client's name into the caller's buffer. The manager is not
// touched: a client already knows its own name.
void CServerMgr::GetClientName(Client* pClient, char* pName, int maxChars)
{
	strncpy(pName, pClient->m_pName, maxChars);
}

// 0x100277c2 — everything the client still has queued goes out now, at
// whatever the manager's current time is (name ours).
void sm_FlushClientPackets(CServerMgr* pMgr, Client* pClient)
{
	xfer_Update((CClientFiles*)pClient->m_pUnknown18C, pMgr->m_Unknown6d4);
}


// ---- the manager's own slots --------------------------------------------- //
//
// Every one of these publishes the manager through the globals first: the
// net layer can reach them at times when nothing else has.

LTBOOL CServerMgr::_v04(void* pA, void* pB)
{
	SetupGlobals();

	sm_AddClientForConnection(this, (NetConnection*)pA, (int)pB);

	return LTTRUE;
}

void CServerMgr::_v08(NetConnection* pConn)
{
	SetupGlobals();

	sm_DropConnection(this, pConn);
}

// A packet the net layer made nothing of goes to the application first and
// then to the game; either may be absent, and neither answer is looked at.
void CServerMgr::ProcessPacket(CPacket* pPacket, uint8* senderAddr, uint16 senderPort)
{
	if (m_pAppHandler)
	{
		m_pAppHandler->ProcessPacket((char*)pPacket->m_Data.m_pArray,
		                             pPacket->m_DataLen,
		                             senderAddr, senderPort);
	}

	if (m_ClassMgr.m_pServerShell)
	{
		m_ClassMgr.m_pServerShell->ProcessPacket((char*)pPacket->m_Data.m_pArray,
		                                         pPacket->m_DataLen,
		                                         senderAddr, senderPort);
	}
}

// SoundInstance::Term is DEFINED ABOVE CServerMgr::RemoveDeadSounds,
// `inline` -- see the note there.

// 0x100338b8 — the shell caches what it wants, and everything nothing is
// using afterwards is let go. The flag says the caching is over.
LTRESULT sm_CacheShellFiles(CServerMgr* pMgr)
{
	pMgr->m_ClassMgr.m_pServerShell->CacheFiles();

	pMgr->m_Unknown1a0 &= ~0x10;

	pMgr->FreeUnusedModels();
	sm_FreeUnusedSounds(pMgr);

	return LT_OK;
}


// 0x1002bab8 — take the object out of whatever container its type lives in.
// Types with nothing to do have no handler and the answer is nothing.
uint32 sm_RemoveFromWorldTree(CServerMgr* pMgr, LTObject* pObject)
{
	uint32 (*pFn)(CServerMgr*, LTObject*) =
		g_ObjTypeHandlers[pObject->m_ObjectType].m_pRemove;

	if (pFn)
		return pFn(pMgr, pObject);

	return 0;
}

// 0x1003471d — the queued-object list goes back to the manager's own bank,
// one node at a time, and the list is left empty (name ours).
void sm_ReturnList44(CServerMgr* pMgr)
{
	LTLink* pCur;
	LTLink* pHead;
	LTLink* pNext;

	sm_FlushList44(pMgr);

	pCur = pMgr->m_List44.m_Head.m_pNext;
	pHead = &pMgr->m_List44.m_Head;

	while (pCur != pHead)
	{
		pNext = pCur->m_pNext;

		sb_Free(&pMgr->m_Bank0C, pCur->m_pData);

		pCur = pNext;
	}

	dl_InitList(&pMgr->m_List44);
}

// 0x1003512f — the worker thread's own construction; the base runs first and
// the manager is filled in later, by Init.
CServerThread::CServerThread()
{
	m_pServerMgr = LTNULL;
}

// 0x10035145 — which manager the thread works for.
LTBOOL CServerThread::Init(CServerMgr* pMgr)
{
	m_pServerMgr = pMgr;

	return LTTRUE;
}


// 0x10029bc4 — a queued message goes out unless something already dealt with
// it. The manager is accepted and not read (name ours).
void sm_WriteQueuedMessage(CServerMgr* pMgr, CQueuedMsg* pRec, Client* pClient,
                           ServerMessage* pMsg)
{
	if (pRec->m_Unknown00 == 0)
		sm_WriteQueuedMessageBody((SoundRecord*)pRec, pClient, pMsg);
}

// 0x10016971 — the notification slot's default is to ignore the event. Three
// bytes, `ret 0xc`: the arguments are accepted and none of them is read, and
// nothing is put in eax either, so a caller that looks at the result of this
// override reads whatever the last call left behind.
void CBaseMgr::_v10(int nEvent, char* pName, int n)
{
}


// 0x1002d34a — a one-line forward, and the only reason it exists is that the
// worker takes the manager by argument where the caller has it in `this`.
// The worker (0x1002d355) is what actually builds the object; nothing names
// either it or what it builds, so both keep their addresses.
class CUnknown928Obj* sub_1002d355(CServerMgr* pMgr);

CUnknown928Obj* sm_CreateUnknown928(CServerMgr* pMgr)
{
	return sub_1002d355(pMgr);
}


// 0x10027135 — a connection going away only matters if a client was sitting
// on it. The lookup answers null for a connection that never got that far —
// one dropped during the handshake — and then there is nothing to remove.
void sm_DropConnection(CServerMgr* pMgr, NetConnection* pConn)
{
	Client* pClient = sm_FindClientByConnection(pMgr, pConn);

	if (pClient)
		sm_RemoveClient(pMgr, pClient);
}


// 0x10027be0 and 0x100348cc — whether this client still owes the object an
// update, and the queue it goes on when it does (names ours, from the shape
// of the call; nothing in the image names either).
//
// Both take the manager and the object, which settles sm_VisCallbackA's
// argument roles: its *second* parameter is the LTObject and the manager
// comes out of the first's +0x00.
// The model instance's own model pointer, which is serverengine.h's
// ModelInstanceObj::m_pModel; declared here so this file need not pull that
// header in.
class CModelInstView
{
public:
	// AvP2's tracker sits further in (commonmodel.h's ModelInstance), so
	// its model is at +0x1fc in Primal Hunt (lithtech 0x004793b0), +0x1f8
	// in AvP2 1.0.9.6 (server.dll 0x10030668).
#ifdef PRIMAL_HUNT
	uint8        m_pad_00[0x1fc];
#else
	uint8        m_pad_00[0x1f8];
#endif
	class Model* m_pModel;                   // +0x1fc (AvP2 +0x1f8)
};

// 0x10027be0 — does this object owe the clients an update at all? Change
// flag 4 has to be set, and unless FLAG_FORCECLIENTUPDATE overrides it the
// object also has to be one the clients can perceive: visible, or solid or
// ray-hittable, and an OT_NORMAL has to be carrying the engine-data flag at
// SObjData+0x2c. A model still holding the placeholder has nothing to send.
//
// `inline`, and defined above every caller: lithtech.exe (/O2) expands it at
// all four sites -- sm_VisCallbackA (0x00464ed0), sub_10027918 (0x00464a00,
// twice) and sm_VisGatherObjects (0x00465e00) -- and has no copy of its own,
// while server.dll's /Os declines every site and keeps 0x10027be0.
inline int sub_10027be0(CServerMgr* pMgr, LTObject* pObject)
{
	uint32 nFlags = pObject->m_Flags;

	if (!(nFlags & FLAG_FORCECLIENTUPDATE))
	{
		// One mask in AvP2 (lithtech 0x0047937f `test $0x3001`); NOLF's
		// tested the visible bit apart from the other two.
		if (!(nFlags & (FLAG_VISIBLE | FLAG_RAYHIT | FLAG_SOLID)))
			return 0;

		if (pObject->m_ObjectType == OT_NORMAL &&
			pObject->m_pSData->m_pSFXMessage == 0)
			return 0;
	}

	if (!(pObject->m_ChangeFlags & 4))
		return 0;

	if (pObject->m_ObjectType == OT_MODEL &&
		((CModelInstView*)pObject)->m_pModel == pMgr->m_pPlaceholderModel)
		return 0;

	return 1;
}

// 0x100348cc — restart the object's update countdown and put it back in the
// state its change flags now ask for. The deactivation time is the interval;
// a zero or negative one is copied through unchanged, which is why one arm
// stores through the FPU and the other with an integer `mov`.
void sub_100348cc(LTObject* pObject)
{
	SObjData* pSData = pObject->m_pSData;

	if (pSData->m_DeactivationTime > 0.0f)
		pSData->m_UpdateTimer = g_pServerMgr->m_FrameTime + pSData->m_DeactivationTime;
	else
		pSData->m_UpdateTimer = pSData->m_DeactivationTime;

	if (pObject->m_ChangeFlags & 0x20)
		sm_SetObjectState(g_pServerMgr, pObject, pObject->m_ChangeFlags & 0x18);
}

// An object the sweep may deactivate is told so through its own vptr, slot 2.
// ltobject_internal.h models LTObject as a plain structure, which is right
// for every other use of it in this file, so the one virtual call reached
// from here goes through a view of the same object rather than making the
// whole class polymorphic (name ours; only the slot index is established).
class IObjectDeactivate
{
public:
	virtual void _v00() = 0;
	virtual void _v04() = 0;
	virtual void _v08() = 0;
};

// 0x10027c3c — one candidate object out of the visibility gather. Three tests
// have to pass before anything happens, and they are asked in the cheapest
// order: a field on the context, then the worker, then a second context field.
//
// The context is the first argument and the object the second, which is the
// reverse of what the parameter names suggest -- the fields read off the
// first argument (+0x54, and the flag byte at +0x121 it clears) belong to the
// thing being written to, and the second's +0x00 is what the worker is handed.
// Left named as declared; the argument roles are recorded here rather than
// renamed, because what either structure is has not been established.
void sm_VisCallbackA(void* pUser, void* pObject)
{
	if (((uint32*)pUser)[0x54 / 4] != 0)
		return;

	if (!sub_10027be0(*(CServerMgr**)pObject, (LTObject*)pUser))
		return;

	if (((uint32*)pObject)[0x1c / 4] == 0)
		return;

	// A DWORD clear of change flag 0x200: lithtech.exe (0x00464f42) ands
	// m_ChangeFlags itself, reusing the register the expanded `& 4` test
	// loaded; server.dll's /O1 narrows the same statement to the `andb
	// $0xfd,0x121` at 0x10027c60.
	((LTObject*)pUser)->m_ChangeFlags &= ~0x200;

	sub_100348cc((LTObject*)pUser);
}


// 0x1002a11e — hand one arrived message to the game's shell (name ours). The
// enumeration gives this 188 bytes; the `ret` is at 0x1002a166, so it is 73
// and the row runs into the function after it.
//
// The message id is the packet's LAST byte, not its first: ReadByteFromEnd
// takes it off the end and shortens the packet, so what the shell is handed
// through m_Write is the payload with the id already removed.
//
// That id lives in the pMgr parameter's slot -- pMgr is in %esi from the
// first instruction -- and it is stored as a byte and pushed as a dword, so
// the three bytes above it are whatever the pointer left there. Harmless,
// since the callee's parameter is a uint8, and it is the same byte-saving
// trick CBaseStream::ReadString's length uses.
//
// lithtech.exe 0x00469390 (120 bytes; the enumeration's 286 ran on into
// the next handler at 0x00469410) EXPANDS ReadByteFromEnd at
// 0x004693b6..0x004693e7 where server.dll calls 0x1002a1da. /O1 still calls
// it, so the header declaration stays out of line and /O2 gets this twin of
// servermsg.cpp's body (a stand-in: the shipped header presumably defined it
// inline, which server.dll's /O1 then declined to expand).
#ifndef LT_SERVER_BUILD
inline uint8 ReadByteFromEndInline(ServerMessage* pMsg)
{
	if ((uint32)(pMsg->m_Unknown34 - pMsg->m_Unknown38) >= 1)
	{
		uint8 nRet = pMsg->m_pData[pMsg->m_Unknown34 - 1];

		pMsg->m_Unknown34--;

		return nRet;
	}

	return 0;
}
#endif

// 0x1002a0d0 — packet handler 6 (name by address), lithtech.exe 0x00469330:
// the sender asks to go, so its connection is dropped with reason 5. Only a
// remote sender has a connection (ServerMessage::m_pConnection is null for
// local packets); the third argument is not read. Both images: `mov
// 0x3c(%eax),%eax ; test ; je`, then `push $0 ; push $5 ; push %eax` and a
// thiscall on pMgr+0x6e0 to CNetMgr::Disconnect (0x10021c18 / 0x00456320),
// and `xor %eax,%eax` at the one exit.

int sub_1002a0d0(CServerMgr* pMgr, ServerMessage* pMsg, Client* pClient)
{
	if (pMsg->m_pConnection)
		pMgr->m_NetMgr.Disconnect(pMsg->m_pConnection, 5);

	return 0;
}

// 0x1002a0f2 — packet handler 10 (name by address): a console string from a
// client, run on the server's own console, but only for a client whose
// internal flags carry bit 1 (m_ClientFlags, +0x3c0); the answer is the
// console's (cc_RunConsoleString, sharedcommon.cpp). The string is read in
// place off the packet (servermsg.cpp's CMsgStringReader, 0x1002609f), and
// the read comes first: the second argument is evaluated before the first.
class CMsgStringReader
{
public:
	char* ReadStringInPlace();               // 0x1002609f
};

int cc_RunConsoleString(ConsoleState* pState, char* pString);   // 0x10028fc6

int sub_1002a0f2(CServerMgr* pMgr, ServerMessage* pMsg, Client* pClient)
{
	if (pClient->m_ClientFlags & 2)
		return cc_RunConsoleString(&pMgr->m_ConsoleState,
			((CMsgStringReader*)pMsg)->ReadStringInPlace());

	return 0;
}

// AVP2: packet handler 13 (name by address), lithtech.exe 0x0047d630,
// server.dll 0x100358ba: with a client and a server shell (+0xc30, the
// same test sm_GiveMessageToShell makes) the packet goes to a CServerMgr
// thiscall of two arguments, client first (lithtech.exe 0x0048f570,
// server.dll 0x10042e1e; 969 bytes, not yet reconstructed). Its result is
// dropped: `xor %eax,%eax` follows the call.
int sub_100358baQ502(CServerMgr* pMgr, ServerMessage* pMsg, Client* pClient)
{
	if (pClient && pMgr->m_ClassMgr.m_pServerShell)
		((CServerMgrQ502*)pMgr)->Sub48f570(pClient, pMsg);

	return 0;
}

LTRESULT sm_GiveMessageToShell(CServerMgr* pMgr, ServerMessage* pMsg,
                               HCLIENT hClient)
{
	uint8 nMsgId;

	// One positive condition around the body, not two guarded returns: the
	// shipped LT_OK sits at the very end with both tests jumping to it and
	// the body falling into it. As guards it goes inline in the middle
	// instead.
	if (hClient && pMgr->m_ClassMgr.m_pServerShell)
	{
		pMgr->InitMessageOwner(pMsg);

#ifdef LT_SERVER_BUILD
		nMsgId = pMsg->ReadByteFromEnd(0);
#else
		nMsgId = ReadByteFromEndInline(pMsg);
#endif

		pMgr->m_ClassMgr.m_pServerShell->OnMessage(
			hClient, nMsgId, (HMESSAGEREAD)&pMsg->m_Write);
	}

	return LT_OK;
}


// 0x1002a167 / lithtech.exe 0x00469410 — packet handler 9 (name by
// address): one byte off the packet, and the client's handshake stage
// (m_Unknown3BC) moves to 3 if it is set and to 1 if not. The read is
// CPacket::ReadType<uint8>: server.dll calls 0x100138d6, lithtech.exe
// expands it with its "CPacket::ReadType" overrun report (0x00469416..
// 0x0046947f). The lithtech row was the tail of the enumeration's 286-byte
// 0x00469390 row.
int sub_1002a167(CServerMgr* pMgr, ServerMessage* pMsg, Client* pClient)
{
	if (!((CPacket*)pMsg)->ReadType((uint8*)0))
		pClient->m_Unknown3BC = 1;
	else
		pClient->m_Unknown3BC = 3;

	return 0;
}

#ifdef LT_SERVER_BUILD
// 0x1002a193 / lithtech.exe 0x004694b0 — packet handler 5 (name by
// address): the client data the game hands over when it connects (a uint16
// length, then that many bytes), into a new buffer at Client+0x110 with
// its length at +0x114, which OnClientEnterWorld passes the server shell;
// then the flag word's 0x100 bit. CPacket::ReadType<uint16> is the call
// to 0x1000ebd0 (lithtech.exe expands it), the copy servermsg.cpp's
// ReadData (0x100260d9).
//
// Server-only: lithtech.exe's is lt2new1.cpp's sub_4694b0. Until it was
// written, server.dll's link/aliases.rsp stood LTMatrix::operator* in for
// it, so the first client to join a dedicated server (the one program that
// runs server.dll, NolfServ.exe) sent this packet into a matrix multiply,
// and the server stopped.
// servermsg.h types ReadData's length uint32; the call here loads it as
// sixteen bits (`mov (%esi),%cx`), as netmgr.cpp's view of the same function
// does (link/aliases.rsp joins the two).
class ServerMessageTypeView
{
public:
	void ReadData(void* pData, uint16 nBytes);   // 0x100260d9
};

int sub_1002a193(CServerMgr* pMgr, ServerMessage* pMsg, Client* pClient)
{
	pClient->m_ClientDataLen = ((CPacket*)pMsg)->ReadType((uint16*)0);

	if (pClient->m_ClientDataLen)
	{
		pClient->m_pClientData = new uint8[pClient->m_ClientDataLen];

		((ServerMessageTypeView*)pMsg)->ReadData(pClient->m_pClientData,
			(uint16)pClient->m_ClientDataLen);
	}

	pClient->m_ClientFlags |= 0x100;

	return 0;
}
#endif


// 0x1002a205 — after a flag change, put the object back in the containers
// under its new flags. An object the sweep is allowed to deactivate is
// re-seated in the world's own container; one it is not gets its second
// vtable slot called instead and never touches the container at all.
//
// Answers LT_OK on every path, including the one that does nothing.
LTRESULT sm_UpdateObjectBBox(CServerMgr* pMgr, LTObject* pObj)
{
	if (!sm_CanDeactivate(pObj))
	{
		if (pObj->m_Unknown58 == 0xffffffff)
			pMgr->m_Unknown210.BumpObject(pObj, 0);
	}
	else
	{
		((IObjectDeactivate*)pObj)->_v08();
	}

	return LT_OK;
}


// 0x10024cac and 0x10024cad — what a visibility query does by default: a
// one-byte `ret`, and a predicate that answers yes. Both exist only so the
// gather can call through the slots without testing them, which is why the
// constructor installs them rather than leaving nulls (names ours).
static void vq_Ignore(void* pUser, void* pObject)
{
}

// Two of them, not one used twice: the shipped constructor stores the same
// address into both slots as an immediate each time, where one source
// function assigned twice makes VC6 hold the pointer in a register and store
// from that. Two symbols mean two relocations and no such sharing -- and
// with /Gy the linker then folds the identical bodies back onto one address,
// which is why only 0x10024cac survives in the image.
static void vq_IgnoreToo(void* pUser, void* pObject)
{
}

static int vq_Accept(void* pUser, void* pObject)
{
	return 1;
}

// CVisQuery's constructor is DEFINED ABOVE sm_WriteClientUpdate, `inline`,
// for the same reason sm_SkipClientUpdate is -- see the note there.


// 0x10032e2f — drain whatever the loader thread sent back, on the thread
// that asked. The message is a 32-byte local constructed once and refilled
// by every take, so the loop reuses one frame slot rather than allocating;
// the queue is the outgoing one, which from the manager's side is the
// inbound direction.
void CServerMgr::ProcessThreadMessages()
{
	LThreadMessage msg;

	while (m_Thread.m_OutgoingQueue.GetMessage(&msg, 0) == LT_OK)
		OnModelLoaded(&msg);
}


// 0x1002ba90 — hand the object's new file names to whatever knows about that
// object's type. A type with nothing to say has no handler.
LTRESULT w_ApplyObjectFilenames(CServerMgr* pMgr, LTObject* pObj,
                                ObjectCreateStruct* pStruct)
{
	uint32 (*pFn)(CServerMgr*, LTObject*, ObjectCreateStruct*) =
		g_ObjTypeHandlers[pObj->m_ObjectType].m_pApplyFilenames;

	if (pFn)
		return pFn(pMgr, pObj, pStruct);

	return 0;
}


// 0x1002cecf — the object map's allocate is CMoArray<CObjectMapEntry>::
// _AllocateTArray (servermgr.h).

// 0x1002a239 — may the sweep deactivate this object? Anything the engine
// still owes work on is left alone, and so is anything carrying one of the
// flags that mean "keep me live" — unless it also carries the one that
// overrides them (name ours).
// MOVED TO servermgr.h, `inline`. It has to be a header body for two
// independent reasons: /Ob1 never expands a file-scope function, and VC6's
// inliner is FORWARD-ONLY so the caller at line ~5152 above precedes this
// definition anyway. lithtech.exe has NO standalone copy -- the constant
// 0x21213001 occurs exactly twice in .text, at 0x0047c568 and 0x00469588,
// and both are expansions. server.dll keeps 0x1002a239 and CALLS it from
// both of its sites, which is the census that makes the move /O1-safe.


// ======================================================================== //
//  Reconstructions. Each carries the shipped address it
//  was read off; anything the headers do not model is declared here as a
//  local view and cast through, exactly as IObjectDeactivate above.
// ======================================================================== //


// 0x1003481c — every live sound instance is terminated and handed back to
// the bank at 0x28, and the list left empty.
//
// The name in servermgr.h is a misnomer this reconstruction cannot fix
// (the header is not ours to edit): the list walked here is m_List54, the
// SoundInstance list, not m_List44. sm_ReturnList44 (0x1003471d) calls this
// first and then does the same job for m_List44.
void sm_FlushList44(CServerMgr* pMgr)
{
	LTLink* pCur;
	LTLink* pHead;
	LTLink* pNext;

	pCur = pMgr->m_List54.m_Head.m_pNext;
	pHead = &pMgr->m_List54.m_Head;

	while (pCur != pHead)
	{
		SoundInstance* pSound = (SoundInstance*)pCur->m_pData;
		pNext = pCur->m_pNext;

		pSound->Term();
		sb_Free(&pMgr->m_Bank28, pSound);

		pCur = pNext;
	}

	dl_InitList(&pMgr->m_List54);
}


// The cache record with the one member servermgr.h does not model: the
// reset at 0x100356e9 is a thiscall (the shipped sweep loads the record into
// ecx and pushes nothing), and it clears a count at +0x14 and a float at
// +0x10. Named by what it does; nothing in the image names the record.
class CCacheRecView
{
public:
	void Reset();                            // 0x100356e9
	// 0x1003567c — fill the record in from the file, the opened source and
	// the handle's second dword; nonzero if it took (name ours).
	int  Init(void* hFile, class ICacheSource* pSource, void* pParam);

	LTLink m_Link;                           // +0x00
	void*  m_hFile;                          // +0x0c
	float  m_f10;                            // +0x10
	uint32 m_n14;                            // +0x14
	uint32 m_bWanted;                        // +0x18
};

// 0x100356e9 — the two counters a fresh (or recycled) record starts from.
// Source order is only visible at /O2: /O1 rewrites the integer zero as
// `andl $0x0,0x14(%ecx)` and schedules that ahead of the `fldz/fstps`
// whatever order they are written in (0x100356e9), while /O2 folds both to
// one `xor %eax,%eax` and keeps them as spelled -- and lithtech.exe stores
// +0x10 first (0x0047d302).
void CCacheRecView::Reset()
{
	m_f10 = 0.0f;
	m_n14 = 0;
}

// The 0x38-byte description 0x100392a9 fills in from the opened source (it
// memsets exactly 0x38 bytes before parsing). Only two fields are read here,
// and the division of one by the other plus a tenth of a second says what
// they are: a byte total over a byte rate is a duration. Names ours.
struct CCacheSourceInfo
{
	uint8  m_pad_00[0x08];
	uint32 m_nBytesPerSec;                   // +0x08
	uint8  m_pad_0c[0x18 - 0x0c];
	uint32 m_nDataSize;                      // +0x18
	uint8  m_pad_1c[0x38 - 0x1c];
};

// 0x100392a9 — parse the opened source's header into that description;
// nonzero if it made sense of it (name ours).
int fs_GetSourceInfo(class ICacheSource* pSource, CCacheSourceInfo* pInfo);

// 0x1003567c — fill the record in from the file. The third argument is
// validated and never read; the shipped code checks it and nothing else.
//
// The failure return sits at the very end with the success return jumping
// over it, which is the AND-merged shape: one positive guard around the
// whole body rather than three guarded returns.
int CCacheRecView::Init(void* hFile, ICacheSource* pSource, void* pParam)
{
	CCacheSourceInfo info;

	if (hFile && pSource && pParam)
	{
		Reset();

		m_hFile = hFile;

		if (fs_GetSourceInfo(pSource, &info))
		{
			m_bWanted = 0;
			m_n14 = 0;
			m_f10 = (float)info.m_nDataSize / info.m_nBytesPerSec + 0.1f;

			return 1;
		}
	}

	return 0;
}

// 0x10034757 — the cache-pass sweep: every record nobody asked for this
// pass is unlinked, reset and returned to the bank at 0x0c.
//
// Another misnamed declaration in servermgr.h: the list is m_List44 (the
// records AddCachedFile at 0x1003448f builds, 0x1c bytes out of m_Bank0C
// with the wanted flag at +0x18), and the sweep has nothing to do with
// sounds. Kept under the declared name.
//
// The record's own LTLink is at offset 0 and its m_pData points back at the
// record, which is why the unlink operates on the record address rather
// than on the link the walk is holding.
void sm_FreeUnusedSounds(CServerMgr* pMgr)
{
	LTLink* pCur;
	LTLink* pHead;

	pCur = pMgr->m_List44.m_Head.m_pNext;
	pHead = &pMgr->m_List44.m_Head;

	while (pCur != pHead)
	{
		CCacheRecView* pRec = (CCacheRecView*)pCur->m_pData;
		pCur = pCur->m_pNext;

		if (pRec->m_bWanted == 0)
		{
			dl_RemoveAt(&pMgr->m_List44, (LTLink*)pRec);

			pRec->Reset();

			sb_Free(&pMgr->m_Bank0C, pRec);
		}
	}
}


// 0x10009d1e — the world's object store is shut down through its own vtable
// (slot 1) and then forgotten.
//
// The pointer lives at CServerWorld+0x184, which is well past the eight
// bytes servermgr.h gives the class; Term (0x10009be5) reaches +0x1d4. The
// class is therefore much bigger than the header models, so the member is
// reached through a view here rather than declared.
class IWorldObjectStore
{
public:
	virtual void _v00() = 0;
	virtual void Term() = 0;                 // slot 1
};

class CServerWorldView
{
public:
	// Only the address of each sub-object is reached; what they are is not
	// established, so they are named by offset.
	uint8   m_pad_00[0x04];
	uint32  m_Collection04;                  // 0x04  SetSize(0, alloc) at teardown
	uint32  m_Unknown08;                     // 0x08  the four the light-array
	uint32  m_Unknown0c;                     // 0x0c  teardown clears by hand
	uint32  m_Unknown10;                     // 0x10
	uint32  m_Unknown14;                     // 0x14
	uint8   m_pad_18[0x6c - 0x18];
	uint32  m_Unknown6c;                     // 0x6c
	uint8   m_pad_70[0xe8 - 0x70];
	LTLink  m_List0E8;                       // 0xe8  bare head; its payloads
	                                         //       are deleted one by one
	uint8   m_pad_f4[0xfc - 0xf4];
	uint32  m_Unknown0fc;                    // 0xfc
	uint8   m_pad_100[0x184 - 0x100];
	IWorldObjectStore* m_pObjects;           // 0x184
	uint32  m_Collection188;                 // 0x188
	void**  m_pUnknown18c;                   // 0x18c  array, count below
	uint32  m_nUnknown190;                   // 0x190
	uint8   m_pad_194[0x1b0 - 0x194];
	void*   m_pUnknown1b0;                   // 0x1b0  net_Free'd
	uint8   m_pad_1b4[0x1b8 - 0x1b4];
	void*   m_pUnknown1b8;                   // 0x1b8  dfree'd
	uint8   m_pad_1bc[0x1d0 - 0x1bc];
	LTLink  m_Children;                      // 0x1d0  child worlds
	LTLink  m_Link1dc;                       // 0x1dc  this world's own link
	uint32  m_bIsChild;                      // 0x1e8  nonzero takes the short
	                                         //        teardown
};

void CServerWorld::TermObjects()
{
	uint32 i;
	CServerWorldView* pThis = (CServerWorldView*)this;

	if (pThis->m_pObjects)
	{
		pThis->m_pObjects->Term();
		pThis->m_pObjects = LTNULL;
	}
}


// 0x10028a35 is CPacket::WriteType instantiated for *float* — not for a
// generic four-byte value: the store is `flds`/`fstps`, which only the float
// specialisation emits, and `push $4; pop %ebx` is sizeof(float). packet.h
// carries the template; this is the translation unit its float instantiation
// belongs to, so the call sites reach it through a CPacket view of the
// message and the writes below are what emit it.

// 0x10027c71 — the unguaranteed stream is closed off with a sentinel object
// id and the server's current time. The client parameter is accepted and
// never read (the manager supplies the time and the message is the third
// argument), which is why the declared signature keeps it.
//
// AVP2: and, like the object writer, it measures as well as writes (Primal
// Hunt server.dll 0x10032f0f, 55 bytes): sm_WriteClientUpdate calls it once
// with no message and a size, closes the packet off to that size, then
// calls it again to write (0x100335d5..0x100335fd). servermgr.h's
// three-argument declaration is NOLF's; this is a new overload.
void sm_Unknown27c71(CServerMgr* pMgr, Client* pClient, ServerMessage* pMsg,
                     uint32* pSize)
{
	if (pMsg)
	{
		// Both are CPacket::WriteType: NOLF's lithtech.exe expands both
		// (0x00464f67, 0x0046501c) with Insert2 left a call; server.dll
		// calls both.
		((CPacket*)pMsg)->WriteType((uint16)0xffff);
		((CPacket*)pMsg)->WriteType(pMgr->m_Time);
	}

	if (pSize)
		*pSize += 6;
}


// 0x10021c55 — the net manager's queued send. servermgr.h's CNetMgr does not
// model it (SendPacket at 0x10022990 is a different function): this one
// masks the caller's flags to 0xc0, refuses a connection whose queues are
// over the limit, and answers 0/1. Reached through a view of the embedded
// manager; the name is ours, from the arguments.
class CNetMgrSendView
{
public:
	// 0x10021c55 — DEFINED IN netmgr.cpp, which is the TU the address falls
	// in; this is only the declaration. It was reconstructed here as well
	// before dupes.py noticed the two, and the signature has to match
	// netmgr.cpp's exactly or the two TUs name different symbols.
	LTBOOL SendToConnection(class ServerMessage* pMsg, class NetConnection* pConn,
	                        int nFlags);

	// 0x10022990, the send this one gates. servermgr.h has CNetMgr::SendPacket
	// returning void; the gate returns whatever it answers straight out, so
	// the view's copy is typed to carry that. Its third argument is a *byte*:
	// the shipped call pushes the dword slot the byte was stored into with
	// no zero extension, which only a byte-sized parameter allows.
	// Typed LTBOOL to agree with netmgr.cpp, which established the return
	// (servermgr.h still has it as void; see the note there).
	LTBOOL SendPacket(class ServerMessage* pMsg, class NetConnection* pConn,
	                  uint8 nOldByte);
};

// 0x100297b8 — stamp the packet id into the payload's first byte and hand
// the message to every client that is actually in the world. The id is
// written once, into the shared message, rather than per client.
void sm_BroadcastToClients(CServerMgr* pMgr, int nPacketID, ServerMessage* pMsg)
{
	LTLink* pHead;
	LTLink* pCur;

	pMsg->m_pData[0] = (uint8)nPacketID;

	pHead = &pMgr->m_ClientList.m_Head;
	pCur = pHead->m_pNext;

	while (pCur != pHead)
	{
		Client* pClient = (Client*)pCur->m_pData;

		if (pClient->m_pConnection && pClient->m_State == 3)
		{
			((CNetMgrSendView*)&pMgr->m_NetMgr)->SendToConnection(
				pMsg, pClient->m_pConnection, 0x80);
		}

		pCur = pCur->m_pNext;
	}
}


// Ends the intrinsic region opened before UpdateObjects: every memset below
// this point is a call in server.dll. server.dll only: lithtech.exe's /O2
// expands the memsets below as `rep stos` (sm_AddSoundRecord 0x00468231,
// sm_AddClientForConnection 0x004637e2), so for it the region stays open.
#ifdef LT_SERVER_BUILD
#pragma function(memset)
#endif

// (CObjQueueBank and g_ObjQueueBank are declared near the top of the file,
// beside the free-list head Term and sm_DrainObjectQueue use.)

// 0x1002ac20 — take an object out of the world. Nothing happens while the
// manager is already sweeping objects (bit 0 of 0x1a0) or if this object is
// already queued (bit 0 of its change flags); otherwise it goes on the
// queued-object list at 0x83c, is marked queued, has its engine-side memory
// released, and loses the flag at bit 2.
//
// The list head at 0x83c is a bare LTLink with no element count, so the
// insert is dl_Insert and not dl_AddHead.
void sm_RemoveObject(CServerMgr* pServerMgr, HOBJECT hObj)
{
	if (~pServerMgr->m_Unknown1a0 & 1)
	{
		if (~hObj->m_ChangeFlags & 1)
		{
			LTLink* pLink = g_ObjQueueBank.Allocate();

			pLink->m_pData = hObj;
			dl_Insert(&pServerMgr->m_Link83c, pLink);

			hObj->m_ChangeFlags |= 1;

			obj_FreeData(hObj);

			hObj->m_ChangeFlags &= ~4;
		}
	}
}


// The queued message with the members servermgr.h does not model: a list at
// +0x144 whose nodes' payloads go back to the manager's bank at 0x864, and
// the reference count at +0x154 the release counts down. The record itself
// comes out of the bank at 0x9d4. AvP2's record is 0x164 bytes, four more
// than NOLF's, and both members sit four further in (lithtech 0x0047c21c
// `lea 0x148(%esi)`, 0x0047c2cc `mov 0x154(%esi)`).
class CQueuedMsgView
{
public:
	uint8  m_pad_00[0x144];
	LTList m_List140;                        // +0x144
	uint32 m_RefCount;                       // +0x154
	uint8  m_pad_158[0x164 - 0x158];
};

// 0x10031d47 — the last reference to a queued message empties it and returns
// both its nodes' payloads and itself to their banks. The manager is reached
// through the global rather than being carried on the record.
void CQueuedMsg::Release()
{
	CQueuedMsgView* pThis = (CQueuedMsgView*)this;

	if (--pThis->m_RefCount == 0)
	{
		LTLink* pCur = pThis->m_List140.m_Head.m_pNext;
		LTLink* pHead = &pThis->m_List140.m_Head;

		while (pCur != pHead)
		{
			LTLink* pNext = pCur->m_pNext;

			sb_Free(&g_pServerMgr->m_Bank864, pCur->m_pData);

			pCur = pNext;
		}

		dl_InitList(&pThis->m_List140);

		sb_Free(&g_pServerMgr->m_Bank9D4, pThis);
	}
}


// 0x10034702 — every cache record's wanted flag is cleared, so a fresh pass
// can mark the ones that still matter. A thiscall on the manager with no
// arguments, which CServerMgr does not declare; reached through a view.
class CCachePassView
{
public:
	void ClearWantedFlags();                 // 0x10034702

	uint8  m_pad_00[0x44];
	LTList m_List44;                         // +0x44
};

void CCachePassView::ClearWantedFlags()
{
	LTLink* pCur;
	LTLink* pNext;
	LTLink* pHead;

	pCur = m_List44.m_Head.m_pNext;
	pHead = &m_List44.m_Head;

	while (pCur != pHead)
	{
		pNext = pCur->m_pNext;

		((CCacheRecView*)pCur->m_pData)->m_bWanted = 0;

		pCur = pNext;
	}
}

// 0x1003385a — open a caching pass: forget last pass's count, clear every
// cache record's wanted flag, clear bit 0 of every model reference's flags,
// and set or clear the "shell is caching" bit from the argument.
//
// LTRESULT, not the void servermgr.h declares: the body ends `xor %eax,%eax`
// on its single exit, which is LT_OK. Renamed here for that reason — see the
// report; the header's sm_StartWorldA cannot be defined with this body.
LTRESULT sm_StartCachePass(CServerMgr* pMgr, int bCaching)
{
	void* hElement;

	pMgr->m_Unknown920 = 0;

	((CCachePassView*)pMgr)->ClearWantedFlags();

	hElement = dhash_GetFirst(pMgr->m_hModelHash);
	while (hElement)
	{
		uint32* pRef = (uint32*)dhash_GetData(dhash_GetNext(&hElement));

		pRef[0x20 / 4] &= ~1;
	}

	if (bCaching)
		pMgr->m_Unknown1a0 |= 0x10;
	else
		pMgr->m_Unknown1a0 &= ~0x10;

	return LT_OK;
}


// 0x1000e632 and 0x1000e641 — what a packet queue still owes: whether
// anything is waiting, and whether anything is still unacknowledged (names
// ours, from where the answers are used).
// Nothing in the image names the queue; the two dwords the answers come
// straight out of are reached through a view (offsets ours). Both are cdecl
// with one argument and both duplicate their epilogue, so each is a positive
// guard with the zero return after it, not a `return p ? ... : 0`.
class CPacketQueueView
{
public:
	uint8  m_pad_00[0x48];
	uint32 m_nQueued;                        // +0x48
	uint32 m_nUnacked;                       // +0x4c
};

int msg_HasQueuedPackets(void* pQueue)
{
	if (pQueue)
		return ((CPacketQueueView*)pQueue)->m_nQueued;

	return 0;
}

int msg_HasUnackedPackets(void* pQueue)
{
	if (pQueue)
		return ((CPacketQueueView*)pQueue)->m_nUnacked;

	return 0;
}

// 0x10027533 — may this client be let into the world yet? Not while the
// manager is between worlds, and not unless the client has asked (bit 8).
// A client that has already been told (bit 1) is ready; otherwise its packet
// queue has to have drained, and bit 0 decides how thoroughly.
//
// The queue pointer's address is named in a local, which is why the two
// pushes reload through one register rather than each recomputing the member
// address.
int sm_PrepClientEnterWorld(CServerMgr* pMgr, Client* pClient)
{
	if (pMgr->m_Unknown198)
	{
		return 0;
	}
	else
	{
		uint32 nFlags = pClient->m_ClientFlags;

		if (!(nFlags & 0x100))
			return 0;

		if (nFlags & 2)
			return 1;

		if (nFlags & 1)
		{
			void** ppQueue = &pClient->m_pUnknown18C;

			return !(msg_HasQueuedPackets(*ppQueue) || msg_HasUnackedPackets(*ppQueue));
		}

		return !msg_HasQueuedPackets(pClient->m_pUnknown18C);
	}
}


// 0x1000e845 — how much room is left in the message: m_MaxDataLen (+0x36)
// minus the write position (+0x38) minus a seven-byte header reserve,
// clamped at zero. A thiscall (the splitter loads the message into ecx and
// pushes nothing), so it is a member ServerMessage does not declare;
// reached through a view.
//
// The name here used to be GetDataLen, which was the wrong way round: every
// comparison below reads as "does the message still fit". netmgr.cpp's
// declaration of the same address as GetSpaceLeft is the accurate one and
// is what the definition carries.
class CMsgLenView
{
public:
	int GetSpaceLeft();                      // 0x1000e845
	int GetSpaceLeftCall();                  // stand-in, see below
#ifdef PRIMAL_HUNT
	int GetSpaceLeftInline();                // stand-in, see below
#endif

	uint8  m_pad_00[0x36];
	uint16 m_MaxDataLen;                     // +0x36
	uint16 m_Pos;                            // +0x38
};

// Image seam (measured): lithtech.exe expands this at every site but one
// (the lone call to 0x00463730 is at 0x00463584, sm_SendObjectInfo2), so
// /O2 must see an inline body; server.dll calls 0x1000e845 from all six of
// its callers here, and making it inline there costs five of them.
#ifndef LT_SERVER_BUILD
inline
#endif
int CMsgLenView::GetSpaceLeft()
{
	int nLeft = m_MaxDataLen - m_Pos - 7;

	if (nLeft < 0)
		nLeft = 0;

	return nLeft;
}

// STAND-IN for the one lithtech.exe site that CALLS GetSpaceLeft
// (0x00463584, the model-hash loop of sm_SendObjectInfo2) while the next
// site in the same function expands it (0x0046361b): an out-of-line copy of
// the body, which is what 0x00463730 is. What really kept the original from
// expanding that site is not visible in the bytes. server.dll calls the one
// function everywhere, so there it simply forwards.
#ifndef LT_SERVER_BUILD
int CMsgLenView::GetSpaceLeftCall()
{
	int nLeft = m_MaxDataLen - m_Pos - 7;

	if (nLeft < 0)
		nLeft = 0;

	return nLeft;
}
#else
inline int CMsgLenView::GetSpaceLeftCall()
{
	return GetSpaceLeft();
}
#endif

#ifdef PRIMAL_HUNT
// PH: the converse STAND-IN. Primal Hunt's server.dll calls its copy
// (0x1003102f) from six sites but EXPANDS it at one: the cache-record loop
// of sm_SendObjectInfo2 (0x10031cc1..0x10031cd3), the same site lithtech.exe
// expands. Inline in the class sense in both builds, so that site expands
// in both.
inline int CMsgLenView::GetSpaceLeftInline()
{
	int nLeft = m_MaxDataLen - m_Pos - 7;

	if (nLeft < 0)
		nLeft = 0;

	return nLeft;
}
#endif

// 0x100277e2 — whatever the manager owes a packet before it goes out (two
// arguments, the manager and the message; named by address, nothing in the
// image says more).
// The packet trace stream. The object at CServerMgr+0x808 is what
// streamsim_Open (0x10035a8d) hands back — an ILTStream — and only Write is
// reached here, at slot 9 (0x24) counting the SDK's virtual destructor as
// slot 0. servermgr.h models the member as a two-slot CUnknown808Obj, so the
// call goes through this view.
class ITraceStream
{
public:
	virtual void _v00() = 0;                 // 0x00  ~ILTStream
	virtual void _v04() = 0;                 // 0x04  Release
	virtual void _v08() = 0;                 // 0x08  Read
	virtual void _v0c() = 0;                 // 0x0c  ReadString
	virtual void _v10() = 0;                 // 0x10  ErrorStatus
	virtual void _v14() = 0;                 // 0x14  SeekTo
	virtual void _v18() = 0;                 // 0x18  GetPos
	virtual void _v1c() = 0;                 // 0x1c  GetLen
	virtual void _v20() = 0;                 // 0x20  WriteStream
	virtual LTRESULT Write(const void* pData, uint32 nSize) = 0;   // 0x24
};

// 0x10058474 gates the trace entirely; 0x10051c64 gates only the marker at
// the end of each packet (names by address).
extern uint32 g_bPacketTrace58474;
extern uint32 g_bTraceMarker51c64;

// 0x100518a0 — the mode string streamsim_Open is given, the same one
// fs_CopyStreamToFile uses (serverengine.h). In .data, not .rdata, so it is
// a writable array rather than a pooled literal.
extern char g_szStreamWriteMode[];

// 0x100277e2 — dump the packet about to go out to "packet.trc", byte by
// byte, and mark the end of it with "*END*". The stream is opened on first
// use and kept on the manager.
//
// The five marker bytes are five separate `char` locals, not a string: each
// one's address is passed on its own and the shipped build parks two of them
// in the dead parameter slots and three in the single dword of frame it
// reserves. A `char[5]` would be one contiguous run and one `lea` per
// element from a common base.
//
// They are declared in the reverse of the order they are written, and that
// is the whole difference between a near and a match here: the slots each
// one gets are fixed by the order they are *used*, but the initialiser
// stores come out in declaration order, so declaring them forwards gives
// five right slots with the stores emitted backwards.
void sub_100277e2(CServerMgr* pMgr, ServerMessage* pMsg)
{
	uint32 i;

	if (!g_bPacketTrace58474)
		return;

	if (!pMgr->m_pUnknown808)
	{
		pMgr->m_pUnknown808 =
			(CUnknown808Obj*)streamsim_Open("packet.trc", g_szStreamWriteMode);

		if (!pMgr->m_pUnknown808)
			return;
	}

	for (i = 0; i < pMsg->m_Unknown34; i++)
	{
		((ITraceStream*)pMgr->m_pUnknown808)->Write(&pMsg->m_pData[i], 1);
	}

	if (g_bTraceMarker51c64)
	{
		ITraceStream* pStream = (ITraceStream*)pMgr->m_pUnknown808;
		char c5 = '*', c4 = 'D', c3 = 'N', c2 = 'E', c1 = '*';

		pStream->Write(&c1, 1);
		pStream->Write(&c2, 1);
		pStream->Write(&c3, 1);
		pStream->Write(&c4, 1);
		pStream->Write(&c5, 1);
	}
}

// 0x100278ad — close the update packet off and start the next one, unless it
// is not full enough to bother: a threshold of -1 means "always", otherwise
// the packet has to be under it. The guaranteed stream (nA == 8) goes with
// flag 0x80 and the unguaranteed with none.
//
// LTBOOL, not the void servermgr.h declares (as sm_EndUpdatePacket): the
// body answers 0 on the "not yet" path and 1 when it sent, and VC6 could not
// have compiled a value-returning function that fell off the end. Renamed
// here for that reason; every call site discards the answer.
//
// NEAR on Primal Hunt (0x00477c80, 451 of 451 bytes, 3 blocks): only the
// SIB base/index roles of the two byte copies and one lea inside the
// expanded WriteType's Insert2 (0x00477d48, 0x00477d5f, 0x00477da1:
// shipped `(%eax,%ecx,1)`, ours `(%ecx,%eax,1)`) -- allocation.
LTBOOL sm_FinishUpdatePacket(CUpdateContext* pCtx, ServerMessage* pMsg,
                             int nA, int nThreshold)
{
	// AVP2: four bytes of slack over the threshold (`add $0x4,%ecx; cmp;
	// jle`, Primal Hunt 0x00477ca5, AvP2 1.0.9.6 0x00471769 alike).
	if (nThreshold != -1 && ((CMsgLenView*)pMsg)->GetSpaceLeft() > nThreshold + 4)
		return LTFALSE;

	{
		uint32 nFlags = 0;

		if ((uint8)nA == 8)
			nFlags = 0x80;

		sub_100277e2(pCtx->m_pMgr, pMsg);

		sm_SendToClient(pCtx->m_pMgr, pCtx->m_pClient, nA, pMsg, 0, nFlags);

		pMsg->m_Unknown38 = 1;
		pMsg->m_Unknown34 = 1;
		// CPacket::WriteType<uint8>: server.dll's /O1 calls it (0x1001393e),
		// lithtech.exe's /O2 expands it (0x004648a6..0x004649ff, the `cmp $2`
		// folding the m_Pos just stored).
		((CPacket*)pMsg)->WriteType((uint8)0);

		pCtx->m_Unknown18++;
	}

	return LTTRUE;
}


// 0x1000e3f9 once more, through a two-string fixed prototype: the unfreed-
// sound warning batches its stack cleanup with fs_GetFilename's, which is
// what says this site was not compiled varargs (see dsi_PrintStat).
void dsi_PrintFileName(char* pFormat, char* pFilename);

// The sound instance's file handle, which servermgr.h leaves in the padding
// at +0x14. Named from the only use: the unfreed-sound warning resolves it
// through the file manager.
class CSoundFileView
{
public:
	uint8 m_pad_00[0x14];
	void* m_pFile;                           // +0x14
};

// 0x100347a4 — the world's sounds are taken down one by one, and anything
// still holding a file at debug level 3 or above is complained about first.
// Unlike sm_FlushList44 (0x1003481c) this one unlinks each instance as it
// goes and leaves no list re-init behind, because the list is empty by
// construction once the walk ends.
void sm_EndWorldD(CServerMgr* pMgr)
{
	LTLink* pCur;
	LTLink* pHead;

	pCur = pMgr->m_List54.m_Head.m_pNext;
	pHead = &pMgr->m_List54.m_Head;

	while (pCur != pHead)
	{
		SoundInstance* pSound = (SoundInstance*)pCur->m_pData;
		pCur = pCur->m_pNext;

		if ((pSound->m_PlaySoundFlags & 0x10) && g_DebugLevel >= 3 &&
			((CSoundFileView*)pSound)->m_pFile)
		{
#ifdef PRIMAL_HUNT
			// PH: through the varargs prototype -- fs_GetFilename's call
			// is popped on its own (`add $0x8,%esp` at 0x0048eb52) before
			// the print, where AvP2 1.0.9.6 (0x004866ac) batches both into
			// one `add $0x10`.
			dsi_ServerError("Unfreed sound file (server) %s",
				fs_GetFilename(&pMgr->m_FileMgr,
					((CSoundFileView*)pSound)->m_pFile));
#else
			dsi_PrintFileName("Unfreed sound file (server) %s",
				fs_GetFilename(&pMgr->m_FileMgr,
					((CSoundFileView*)pSound)->m_pFile));
#endif
		}

		dl_RemoveAt(&pMgr->m_List54, &pSound->m_Link);

		pSound->Term();

		sb_Free(&pMgr->m_Bank28, pSound);
	}
}


// Two members servermgr.h leaves inside Client's padding: the link an
// attached client is filed on (+0x164), and the head of the list of clients
// attached to this one (+0x180). Named from what sm_AttachClient does with
// them.
class CClientAttachView
{
public:
	uint8  m_pad_00[0x164];
	LTLink m_AttachLink;                     // +0x164
	uint8  m_pad_170[0x180 - 0x170];
	LTLink m_AttachHead;                     // +0x180
};

// 0x10027259 — stop a client watching another's view.
//
// SHIPPED BUG. The message this sends is a null pointer: the two header
// words are written to absolute addresses 0x38 and 0x34 (there is no
// relocation anywhere in this function, so those are literally `(char*)0 +
// 0x34`), the word write is a thiscall with `xor %ecx,%ecx` for `this`, and
// the fourth argument handed to sm_SendToClient is a pushed zero. The
// sibling sm_AttachClient (0x10027155) does the same sequence properly, off
// a `CMessageRef` it fills from msg_AllocPacket — so what is missing here is
// exactly that allocation. Reconstructed as it shipped, null and all.
LTRESULT sm_DetachClient(CServerMgr* pMgr, Client* pClient)
{
	ServerMessage* pMsg = LTNULL;

	if (pClient->m_pViewClient)
	{
		dl_Remove(&((CClientAttachView*)pClient)->m_AttachLink);

		pClient->m_pViewClient = LTNULL;

		if (pClient->m_pUnknown3AC)
		{
			pMsg->m_Unknown38 = 1;
			pMsg->m_Unknown34 = 1;

			// CPacket::WriteType<uint16>, not the ServerMessage::WriteWord
			// wrapper servermsg.h names: 0x1000ec3d IS that instantiation
			// (m_Pos/m_MaxDataLen/m_DataLen, SetAtGrow, and the
			// "CPacket::WriteType" error), so /O1 calls it either way and
			// only /O2 can tell the spellings apart -- lithtech.exe carries
			// the body expanded here (0x00463f54..0x0046403a).
			((CPacket*)pMsg)->WriteType(pClient->m_pUnknown3AC->m_ObjectID);

			sm_SendToClient(pMgr, pClient, 7, pMsg, 1, 0x80);
		}
	}

	return LT_OK;
}


// 0x10033126 — make one object of a class: allocate the game-side instance,
// let it pre-create itself, then put it in the world. A refused world entry
// frees the instance again and answers null.
//
// The out-parameter for the LTObject lands in the dead `hClass` argument
// slot, which is why the function carries an ebp frame with no locals of its
// own; the class record at +0x24 is re-read at each of its three uses.
//
// Both helpers are the file's inline copies: lithtech.exe (0x004745a0, 173
// bytes, not the 185 its old enumerated extent said) expands the allocation and
// the free; server.dll calls 0x10001c54 and 0x1002abf8 (see the notes at
// sm_FreeObjectInline and sm_AllocateObjectInline).
LPBASECLASS CServerMgr::CreateObject(HCLASS hClass, ObjectCreateStruct* pStruct)
{
	LTObject* pObject;

	LPBASECLASS pClass = sm_AllocateObjectInline(this, ((CClassInfo*)hClass)->m_pClassDef);

	pClass->EngineMessageFn(MID_PRECREATE, pStruct, 0.0f);

	if (sm_CreateObjectInWorld(this, pClass, ((CClassInfo*)hClass)->m_pClassDef, pStruct,
	                           0xffff, 0.0f, &pObject) == LT_OK)
	{
		return pClass;
	}

	sm_FreeObjectInline(this, ((CClassInfo*)hClass)->m_pClassDef, pClass);

	return LTNULL;
}


// The visibility query in flight, published in .data at 0x10058c6c by the
// world's own walk (0x10038ea0 stores it). Its +0x18 is where the walk keeps
// the query's user pointer, which is the update context the gather needs;
// what the rest of the object is has not been established, so only that one
// field is modelled (names ours).
class CVisQueryState
{
public:
	uint8           m_pad_00[0x18];
	CUpdateContext* m_pUser;                 // +0x18
};

extern CVisQueryState* g_pVisQueryState;

// 0x100283ff — the second query callback: one leaf's worth of objects, of
// which the ones this client still owes an update go into the caller's array.
//
// servermgr.h declares this as `int sm_VisCallbackB(void* pUser, void* pObject)`,
// which is wrong twice over. It takes THREE arguments and has exactly the
// shape sm_SFXGatherOwned (0x10034b0c) has — the same slot's other occupant,
// assigned to m_Fn2 by sm_SendSFXMessage — so m_Fn2 is a per-list gather and
// not a per-object callback. And it returns nothing: the shared exit puts
// nothing in eax. Renamed and given the real signature.
//
// The context is not passed in: unlike sm_VisCallbackA, which gets it as its
// second argument, this one fetches it out of the global the world's walk
// publishes, and holds it in a local that has to spill because all three
// callee-saved registers are already busy.

void sm_VisGatherObjects(LTLink* pListHead, LTObject*** pppList, uint32* pnCount)
{
	CUpdateContext* pCtx = g_pVisQueryState->m_pUser;

	LTLink* pCur = pListHead->m_pNext;
	while (pCur != pListHead)
	{
		LTObject* pObj = (LTObject*)pCur->m_pData;

		if (!pObj->m_pSData)
			return;

		if (sub_10027be0(pCtx->m_pMgr, pObj))
		{
			(*pppList)[*pnCount] = pObj;
			(*pnCount)++;
		}

		pCur = pCur->m_pNext;
	}
}


// 0x10034973 — what the ping walk does to each object it can see (name
// ours; it is only ever installed here).
void sub_10034973(void* pUser, void* pObject);

// 0x1003491b — wake everything within ten thousand units of the object, so
// deactivated objects it might touch come back. Nothing happens while
// auto-deactivation is off or before a world is up, but the query is
// constructed either way: its declaration comes before both guards, so its
// constructor runs first.
//
// The gather takes no context, which is why m_pUser is left null here where
// sm_WriteClientUpdate fills it in — and it is the same zero VC6 already had
// in a register for the two guards.
void sm_PingObjects(CServerMgr* pMgr, LTObject* pObj)
{
	CVisQuery query;

	if (g_bAutoDeactivate == 0)
		return;

	if (pMgr->m_pWorldFile == 0)
		return;

	query.m_Position = pObj->m_Pos;
	query.m_pUser = LTNULL;
	query.m_Radius = 10000.0f;
	query.m_Fn1 = sub_10034973;

	((IObjectQuery*)&pMgr->m_Unknown210)->FindObjects(&query);
}


// The class record's flag word, which servermgr.h leaves in CClassInfo's
// padding at +0x08. Bit 2 marks a class the world teardown leaves alone
// unless it is told to remove everything (name ours, from that one use).
class CClassFlagsView
{
public:
	uint8  m_pad_00[0x08];
	uint32 m_Flags;                          // +0x08
};

// 0x100325bd — empty the world of objects. The queued-object list is drained
// first, and then the live-object list is walked from its head every time
// round, because removing an object unlinks the node the walk was holding.
// The flag at bit 0 of 0x1a0 is what stops sm_RemoveObject re-queueing while
// this runs.
void sm_RemoveObjects(CServerMgr* pMgr, int bRemoveAll)
{
	LTLink* pHead;
	LTLink* pCur;

	pMgr->m_Unknown1a0 |= 1;

	sm_DrainObjectQueue(pMgr);

	pCur = pMgr->m_List8dc.m_Head.m_pNext;
	pHead = &pMgr->m_List8dc.m_Head;

	while (pCur != pHead)
	{
		SObjData* pSData = ((LTObject*)pCur->m_pData)->m_pSData;

		if (bRemoveAll)
		{
			sm_RemoveObjectFromWorld(pMgr, pSData->m_pBaseClass);
		}
		else if (!(((CClassFlagsView*)pSData->m_pClass)->m_Flags & 4))
		{
			sm_RemoveObjectFromWorld(pMgr, pSData->m_pBaseClass);
		}

		pCur = pMgr->m_List8dc.m_Head.m_pNext;
	}

	pMgr->m_Unknown1a0 &= ~1;
}


// ltobject_internal.h declares LTObject's vtable slot 0x34 (index 13) as
// void; the write gate below tests its answer, so the object is reached
// through a view that gives the slot a return type. Only that slot is used
// (name ours, from what it decides).
class IObjectUpdateGate
{
public:
	virtual void _v00() = 0; virtual void _v04() = 0; virtual void _v08() = 0;
	virtual void _v0c() = 0; virtual void _v10() = 0; virtual void _v14() = 0;
	virtual void _v18() = 0; virtual void _v1c() = 0; virtual void _v20() = 0;
	virtual void _v24() = 0; virtual void _v28() = 0; virtual void _v2c() = 0;
	virtual void _v30() = 0;
	virtual int  IsSkipped() = 0;            // 0x34
};

#ifdef LT_SERVER_BUILD
// (lithtech.exe's /O2 copy, 0x00464a00, is further down, after
// sm_WriteObjectUpdate, so that both of its helpers can be expanded.)
// 0x10027918 — write one changed object into the update (two arguments, the
// object and the context; named by address). The object has to be one the
// client can be told about at all, and a world model whose BSP is busy is
// left alone; everything attached to it goes out with it, resolved by child
// id and gated the same way.
void sub_10027918(LTObject* pObject, CUpdateContext* pCtx)
{
	Attachment* pAttach;

	if (pObject->m_Unknown54)
		return;

	if (((IObjectUpdateGate*)pObject)->IsSkipped())
		return;

	if (pObject->m_ObjectType == OT_WORLDMODEL &&
		pObject->m_pWorldBsp->IsBusy() == 1)
		return;

	if (!sub_10027be0(pCtx->m_pMgr, pObject))
		return;

	sm_WriteObjectUpdate(pCtx, pObject,
		&pCtx->m_pClient->m_pObjInfos[pObject->m_ObjectID]);

	pAttach = pObject->m_pAttachments;

	while (pAttach)
	{
		LTObject* pChild = sm_FindObjectByID(pCtx->m_pMgr, pAttach->m_nChildID);

		if (pChild && sub_10027be0(pCtx->m_pMgr, pChild))
		{
			sm_WriteObjectUpdate(pCtx, pChild,
				&pCtx->m_pClient->m_pObjInfos[pChild->m_ObjectID]);
		}

		pAttach = pAttach->m_pNext;
	}
}
#else
void sub_10027918(LTObject* pObject, CUpdateContext* pCtx);  // defined below
#endif // LT_SERVER_BUILD

// Eleven lists inside the object at 0x390, starting at +0x220. Only their
// heads are reached, and only from here (name ours).
#define NUM_CHANGEDOBJECT_LISTS 11

class CChangedObjectLists
{
public:
	uint8  m_pad_00[0x220];
	LTList m_Lists[NUM_CHANGEDOBJECT_LISTS];    // +0x220
};

// 0x10027e26 — write every object whose state changed into this client's
// update, eleven buckets' worth. An object flagged 0x200 is moved between
// the active and inactive halves of the manager's list first, keeping only
// the two state bits it already had plus 0x20.
//
// The bucket counter lives in the dead p390 argument slot, because ebx, esi
// and edi are all spoken for by the two walks.
void sm_WriteChangedObjects(CServerMgr* pMgr, CUnknown390* p390,
                            CUpdateContext* pCtx)
{
	LTList* pList = ((CChangedObjectLists*)p390)->m_Lists;
	int     nLists = NUM_CHANGEDOBJECT_LISTS;

	do
	{
		LTLink* pHead = &pList->m_Head;
		LTLink* pCur = pHead->m_pNext;

		while (pCur != pHead)
		{
			LTObject* pObj = (LTObject*)pCur->m_pData;
			uint32    nFlags = pObj->m_ChangeFlags;

			if (nFlags & 0x200)
			{
				nFlags &= ~0x200;
				pObj->m_ChangeFlags = nFlags;

				sm_SetObjectState(pMgr, pObj, (nFlags & 0x18) | 0x20);
			}

			sub_10027918(pObj, pCtx);

			pCur = pCur->m_pNext;
		}

		pList++;
	}
	while (--nLists);
}


// 0x1002759a — a client is let into the world: it is told the world has
// started (packet 5), then every console variable by name and value (packet
// 0x10, one message each), then the server clock and the world file's id
// (packet 6). A local client is taken straight on to the next state; a
// remote one waits for its own answer.
//
// LTRESULT, not the void servermgr.h declares: the single exit ends
// `xor %eax,%eax`. Renamed with a 2 suffix for that reason, the way
// sm_ObjectRemoved2 and sm_UpdateClientState2 are -- the header's
// declaration cannot be defined with this body.
//
// The console-variable pointer lives in the dead pMgr argument slot -- ebx,
// esi and edi are already holding the client, the manager and the send
// flags -- and the send flag itself is one materialised 0x80 shared by all
// three sends.
// 0x1002642c, defined further down; declared here because this is the
// first use.
LTRESULT sm_SendCacheEntry(CServerMgr* pMgr, Client* pClient, uint32 iEntry);

// The model reference the manager's hash at 0x918 holds: the cache pass
// clears bit 0 of its flags at +0x20, and the object-info send writes its
// file id out of +0x1c as a word (names by offset).
class CModelRefView
{
public:
	uint8  m_pad_00[0x1c];
	uint16 m_FileID;                         // +0x1c
};

// 0x10026b4d — tell an entering client what it has to have loaded: the
// cache list first (section 0 and then whatever sm_SendCacheEntry adds),
// then every model reference by file id (section 2), then every wanted
// cache record's file id (section 5), then an empty section 1 to close.
// Each section is flushed whenever the message has less than 0x32 bytes of
// room left, and the section tag is written again after the flush.
//
// LTRESULT, not the void servermgr.h declares: the single exit ends
// `xor %eax,%eax`. Renamed with a 2 suffix for that reason.
//
// The cache-list cursor lives in the dead pMgr parameter slot -- the manager
// itself is in edi -- and the list head is re-`lea`d at both ends of the
// walk rather than named, which is the opposite of every other walk in this
// file and is what the shipped bytes have.
//
// Primal Hunt server.dll 0x10031b67: identical. NEAR on lithtech.exe
// 0x00476550 (678/678): the cursor's next pointer is loaded into %edx where
// shipped reuses %eax (0x0047669d). Allocation; unmoved by pRec at function
// scope.
LTRESULT sm_SendObjectInfo2(CServerMgr* pMgr, Client* pClient)
{
	CMessageRef msg;
	void*       hIter;
	LTLink*     pCur;

	msg.Init(msg_AllocPacket(0x44c, 0x44c));

	msg.m_pMsg->WriteByte(0);

	sm_SendToClient(pMgr, pClient, 0x15, msg.m_pMsg, 0, 0x80);

	{
		ServerMessage* pMsg = msg.m_pMsg;

		pMsg->m_Unknown38 = 1;
		pMsg->m_Unknown34 = 1;
	}

	sm_SendCacheEntry(pMgr, pClient, 0);

	msg.m_pMsg->WriteByte(2);

	hIter = dhash_GetFirst(pMgr->m_hModelHash);
	while (hIter)
	{
		CModelRefView* pRef =
			(CModelRefView*)dhash_GetData(dhash_GetNext(&hIter));

		msg.m_pMsg->WriteWord(pRef->m_FileID);

		if (((CMsgLenView*)msg.m_pMsg)->GetSpaceLeftCall() < 0x32)
		{
			sm_SendToClient(pMgr, pClient, 0x15, msg.m_pMsg, 0, 0x80);

			{
				ServerMessage* pMsg = msg.m_pMsg;

				pMsg->m_Unknown38 = 1;
				pMsg->m_Unknown34 = 1;
			}

			msg.m_pMsg->WriteByte(2);
		}
	}

	sm_SendToClient(pMgr, pClient, 0x15, msg.m_pMsg, 0, 0x80);

	{
		ServerMessage* pMsg = msg.m_pMsg;

		pMsg->m_Unknown38 = 1;
		pMsg->m_Unknown34 = 1;
	}

	msg.m_pMsg->WriteByte(5);

	pCur = pMgr->m_List44.m_Head.m_pNext;
	while (pCur != &pMgr->m_List44.m_Head)
	{
		CCacheRecView* pRec = (CCacheRecView*)pCur->m_pData;

		pCur = pCur->m_pNext;

		if (pRec)
		{
			if (pRec->m_bWanted && pRec->m_hFile)
			{
				// A named id: written into the call, the value took %eax
				// and was pushed after the message load; named, it reuses
				// the record's register and is pushed first (server.dll
				// 0x10031cb4, lithtech.exe 0x004766b6).
				uint16 nFileID = ((CFileIdentifier*)pRec->m_hFile)->m_FileID;

				msg.m_pMsg->WriteWord(nFileID);
			}

#ifdef PRIMAL_HUNT
			if (((CMsgLenView*)msg.m_pMsg)->GetSpaceLeftInline() < 0x32)
#else
			if (((CMsgLenView*)msg.m_pMsg)->GetSpaceLeft() < 0x32)
#endif
			{
				sm_SendToClient(pMgr, pClient, 0x15, msg.m_pMsg, 0, 0x80);

				{
					ServerMessage* pMsg = msg.m_pMsg;

					pMsg->m_Unknown38 = 1;
					pMsg->m_Unknown34 = 1;
				}

				msg.m_pMsg->WriteByte(5);
			}
		}
	}

	sm_SendToClient(pMgr, pClient, 0x15, msg.m_pMsg, 0, 0x80);

	{
		ServerMessage* pMsg = msg.m_pMsg;

		pMsg->m_Unknown38 = 1;
		pMsg->m_Unknown34 = 1;
	}

	// The last of this function's eight writes and the only one lithtech.exe
	// expands (0x00463680..0x004636fb); the other seven are calls, in the
	// order byte, byte, word, byte, byte, word, byte that our sites already
	// stand in.
	((CPacket*)msg.m_pMsg)->WriteType((uint8)1);

	sm_SendToClient(pMgr, pClient, 0x15, msg.m_pMsg, 0, 0x80);

	return LT_OK;
}


// 0x10026137 — append a length-prefixed string to a message's payload; the
// one writer servermsg.h does not declare (name ours). Declared here rather
// than beside its other user below, because this is the first use.
class CMsgStringView
{
public:
	void WriteString(char* pStr);            // 0x10026137
};

#ifndef LT_SERVER_BUILD
class CServerWorldQ2B;
extern CServerWorldQ2B* g_pServerWorldQ2B;  // 0x004b17e8 (client only)
#endif

// The writer calls these sites reach (sm_ClientEnterWorld2, sub_100291c7, sub_100290d7).
class CMsgWriteQ291
{
public:
	void WriteDWord(uint32 nValue);          // 0x1000ecd6 / 0x004192e0-class
	void WriteFloat(float fValue);           // 0x10028a35
	void WriteString(char* pStr);            // 0x10026137
	int  GetSpaceLeft();                     // 0x1000e845
};

// AVP2: what servermgr.h leaves as m_Unknown66c[0x408]: a 0x400-byte name
// sm_ClientEnterWorld2 sends as a string, then two dwords (names ours).
class CServerMgrWorldInfoQ541
{
public:
	uint8  m_pad_000[0x66c];
	char   m_Name[0x400];                    // 0x66c
	uint32 m_Unknowna6c;                     // 0xa6c
	uint32 m_Unknowna70;                     // 0xa70
};

// Byte-identical in server.dll (0x100327b4, 449 bytes). NEAR in lithtech.exe
// (0x004776f0..0x00477a3f, 840 vs 848; the function table splits it at
// 0x0047798b): the time goes to the out-of-line WriteType<float> (0x00479d40)
// through a dead copy in a stack slot (`mov %eax,0x2c(%esp)` at 0x004777b2),
// which a plain call does not make; CPacket::WriteType expands it (998), and
// a by-value inline wrapper costs the register shape below it (847, 7 blocks).
LTRESULT sm_ClientEnterWorld2(CServerMgr* pMgr, Client* pClient)
{
	CMessageRef msg;
	void*       hIter;

	msg.Init(msg_AllocPacket(0x44c, 0x44c));

	if (pMgr->m_Unknown198 == 0)
	{
		{
			ServerMessage* pMsg = msg.m_pMsg;

			pMsg->m_Unknown38 = 1;
			pMsg->m_Unknown34 = 1;
		}

		sm_SendToClient(pMgr, pClient, 5, msg.m_pMsg, 0, 0x80);

		// lithtech.exe only (0x004643fc..0x0046440f): a local client gets
		// the server's world published for the client loader to inherit
		// (startload.cpp's DoLoadWorld reads it at 0x00413eb8). Same symbol
		// startload.cpp declares; server.dll's copy has no such store.
#ifndef LT_SERVER_BUILD
		if (pClient->m_ClientFlags & CLIENTFLAG_LOCAL)
			g_pServerWorldQ2B = (CServerWorldQ2B*)&pMgr->m_World;
#endif

		hIter = dhash_GetFirst(pMgr->m_ConsoleState.m_hVarHash);
		while (hIter)
		{
			void* hElement = dhash_GetNext(&hIter);

			if (hElement)
			{
				LTCommandVar* pVar = (LTCommandVar*)dhash_GetData(hElement);

				{
					ServerMessage* pMsg = msg.m_pMsg;

					pMsg->m_Unknown38 = 1;
					pMsg->m_Unknown34 = 1;
				}

				((CMsgStringView*)msg.m_pMsg)->WriteString(pVar->m_pName);
				((CMsgStringView*)msg.m_pMsg)->WriteString(pVar->m_pStringVal);

				sm_SendToClient(pMgr, pClient, 0x10, msg.m_pMsg, 0, 0x80);
			}
		}

		// AVP2: the shell vets the client first (IServerShell::VerifyClient,
		// slot 4 of m_ClassMgr.m_pServerShell, with the client data at +0x110
		// and a code starting at zero homed in pMgr's slot; Primal Hunt
		// server.dll 0x10032897..0x100328a6), and the code goes out after
		// the world file, followed by the manager's name at +0x66c and the
		// two dwords after it (0x100328d9..0x100328f8).
		uint32 nVerifyCode = 0;
		pMgr->m_ClassMgr.m_pServerShell->VerifyClient((HCLIENT)pClient,
			pClient->m_pClientData, nVerifyCode);

		{
			ServerMessage* pMsg = msg.m_pMsg;

			pMsg->m_Unknown38 = 1;
			pMsg->m_Unknown34 = 1;
		}

		// NOLF's lithtech.exe expanded both of these (0x004644ed,
		// 0x004645a6); Primal Hunt's calls both (0x004777b7, 0x004777c6)
		// and expands only the last two dwords.
		((CMsgWriteQ291*)msg.m_pMsg)->WriteFloat(pMgr->m_Time);
		msg.m_pMsg->WriteWord(((CFileIdentifier*)pMgr->m_hWorldFile)->m_FileID);

		((CMsgWriteQ291*)msg.m_pMsg)->WriteDWord(nVerifyCode);
		((CMsgStringView*)msg.m_pMsg)->WriteString(
			((CServerMgrWorldInfoQ541*)pMgr)->m_Name);
		((CPacket*)msg.m_pMsg)->WriteType(
			((CServerMgrWorldInfoQ541*)pMgr)->m_Unknowna6c);
		((CPacket*)msg.m_pMsg)->WriteType(
			((CServerMgrWorldInfoQ541*)pMgr)->m_Unknowna70);

		sm_SendToClient(pMgr, pClient, 6, msg.m_pMsg, 0, 0x80);

		pClient->m_Unknown3BC = 0;
		pClient->m_State = 2;

		if (pClient->m_ClientFlags & CLIENTFLAG_LOCAL)
			sm_UpdateClientState2(pMgr, pClient);
	}

	return LT_OK;
}


// 0x100276eb — a client leaves the world: the shell is told, the object it
// was driving stops belonging to it, and the client is sent the exit packet.
// The message is allocated before any of that, which is why its reference
// lives in the highest frame slot.
#ifndef LT_SERVER_BUILD
void cs_InvalidateAndTerm();              // 0x00414900 (client only)
#endif

void sm_ClientExitWorld(CServerMgr* pMgr, Client* pClient)
{
	CMessageRef msg;

	msg.Init(msg_AllocPacket(0x44c, 0x44c));

	pMgr->m_ClassMgr.m_pServerShell->OnClientExitWorld((HCLIENT)pClient);

	{
		LTObject** ppObj = &pClient->m_pUnknown3AC;

		if (*ppObj)
			(*ppObj)->m_pSData->m_pOwningClient = LTNULL;

		*ppObj = LTNULL;
	}

	{
		ServerMessage* pMsg = msg.m_pMsg;

		pMsg->m_Unknown38 = 1;
		pMsg->m_Unknown34 = 1;
	}

	sm_SendToClient(pMgr, pClient, 5, msg.m_pMsg, 0, 0x80);

	// lithtech.exe only (0x004646c8..0x004646d7): a local client's exit also
	// tears the client-side world down, through clientmgr2.cpp's
	// cs_InvalidateAndTerm (0x00414900). server.dll has no client shell, and
	// its copy (0x100276eb) ends at the send.
#ifndef LT_SERVER_BUILD
	if (pClient->m_ClientFlags & CLIENTFLAG_LOCAL)
		cs_InvalidateAndTerm();
#endif
}


// 0x100286a9 — hand the client the sky box and its thirty object slots. One
// variable serves as both the count written into the message and the loop's
// own counter, which is why the constant is materialised in a register
// rather than pushed as an immediate.
#define MAX_SKYOBJECTS 30

void sm_SendSkyInfo(CServerMgr* pMgr, Client* pClient)
{
	CMessageRef msg;
	uint16*     pID;
	int         nObjects;

	msg.Init(msg_AllocPacket(0x44c, 0x44c));

	msg.m_pMsg->WriteData(&pMgr->m_SkyDef, sizeof(SkyDef));

	nObjects = MAX_SKYOBJECTS;

	// Both of this function's writes are WriteType<uint16> expanded in
	// lithtech.exe (0x00466180 and 0x00466230) and both are calls at /O1.
	((CPacket*)msg.m_pMsg)->WriteType((uint16)nObjects);

	pID = pMgr->m_SkyObjects;

	do
	{
		((CPacket*)msg.m_pMsg)->WriteType(*pID);

		pID++;
	}
	while (--nObjects);

	sm_SendToClient(pMgr, pClient, 0x11, msg.m_pMsg, 0, 0x80);
}


// The same 0x1002616a servermsg.h declares as WriteData, but with the byte
// count as the uint16 it really is: the callee widens its dword slot with
// `movzwl %di,%edi`, and the update header's call pushes a register whose
// upper half is left as garbage by a 16-bit `dec`. Only that call site can
// see the difference, so it goes through a view rather than being corrected
// in the header.
class CMsgDataView
{
public:
	void WriteData(void* pData, uint16 nBytes);   // 0x1002616a
};

// 0x10029609 — the same broadcast sm_BroadcastToClients (0x100297b8) is,
// with the send flags handed in and no world-state test on the client: every
// connected client gets it. Name ours.
void sm_BroadcastToClientsEx(CServerMgr* pMgr, int nPacketID,
                             class ServerMessage* pMsg, uint32 flags);

// 0x10034521 — format a console line and send it to every client.
//
// servermgr.h declares this `void sm_BroadcastPrint(char* pMsg)`, one fixed
// argument; the shipped body takes the address of its *second* argument slot
// and hands it to _vsnprintf, so it is varargs. Renamed for that reason;
// si_BPrint's call site is unaffected either way, because cl
// 8447 merges a pending fixed-prototype cleanup into a varargs call's
// `add esp` and both spellings therefore give one `add $0x14,%esp`.
//
// The va_list never reaches a frame slot: it is not address-taken, so
// va_start folds into the `lea` of the argument and va_end's store dies.
void sm_BroadcastPrintf(char* pMsg, ...)
{
	CMessageRef msg;
	char        sBuffer[500];
	va_list     marker;

	// A positive guard round the whole body: the null manager falls through
	// to the reference's release with everything else (Primal Hunt
	// server.dll 0x10042448 jumps straight to it, and the send's `add esp`
	// stays inside the body, 0x100424be).
	if (g_pServerMgr)
	{
		va_start(marker, pMsg);
		_vsnprintf(sBuffer, sizeof(sBuffer) - 1, pMsg, marker);

		msg.Init(msg_AllocPacket(0x44c, 0x44c));

		// WriteType<uint8> -- 0x1001393e is that instantiation, the same
		// body as the word one a byte wide; lithtech.exe expands it at
		// 0x00476074.
		((CPacket*)msg.m_pMsg)->WriteType((uint8)0);
		((CMsgStringView*)msg.m_pMsg)->WriteString(sBuffer);

		sm_BroadcastToClientsEx(g_pServerMgr, 0xd, msg.m_pMsg, 0x80);
	}
}


// 0x1002ad4a — stow the object's SFX message so late-joining clients get it.
//
// SObjData+0x2c is a CMessageRef, not the plain word ltobject_internal.h
// calls m_Unknown2c: the shipped code hands its address to CMessageRef::Init
// and then writes through the ServerMessage behind it. That also explains
// what sm_CanDeactivate reads there — an object holding an SFX message is an
// object the inactive sweep has to leave alone.
//
// The payload is copied minus its first byte (the packet id), and the stored
// message's length is set to match. msg_AllocPacket's returned temporary
// lands in the dead pObj argument slot, the object itself having been cached
// in a register.
#ifndef LT_SERVER_BUILD
// lithtech.exe only. The /O2 copy (0x0046a47c..0x0046a4f3) resets the stowed
// message with the store order m_Unknown34, m_MaxDataLen, m_Unknown38,
// m_Unknown3a, m_MsgFlags, owner -- /O2 keeps integer stores in source order,
// so that is the order the source has, while servermsg.h's Init writes
// m_MsgFlags first (invisible at /O1, where the `andl $0` goes to the top
// anyway) -- and it EXPANDS the packet data's Init: SetSize(0) (0x0040bfe0)
// twice, the 21-byte reset 0x00433570, then SetSize(n). STAND-IN for the
// header's ServerMessage::Init / CMsgPacketData::Init at this one site.
class CSFXPacketDataView
{
public:
	LTBOOL SetSize(uint32 nItems, uint32* pBank);        // 0x0040bfe0
	void   InitArray(uint32 nGrowBy);                    // 0x00433570

	void Init(uint32 nItems, uint32 nGrowBy)
	{
		SetSize(0, &g_PacketDataBank);
		InitArray(nGrowBy);
		SetSize(nItems, &g_PacketDataBank);
	}
};

inline void sm_SFXMsgInit(ServerMessage* pMsg, uint16 nMaxDataLen, uint16 nGrowSize)
{
	pMsg->m_Unknown34  = 1;
	pMsg->m_MaxDataLen = nMaxDataLen;
	pMsg->m_Unknown38  = 1;
	pMsg->m_Unknown3a  = 1;
	pMsg->m_MsgFlags   = 0;

	// The owner store through a volatile lvalue: without it /O2 sinks
	// this store and the m_MsgFlags one below the m_DataSize load; a
	// volatile READ of m_DataSize instead (the header's /O1 barrier)
	// swaps the first two stores here. Measured both.
	*(ServerMessage* volatile*)&pMsg->m_Write.m_pOwner = pMsg;

	if (nMaxDataLen > pMsg->m_DataSize)
	{
		CSFXPacketDataView* pData = (CSFXPacketDataView*)&pMsg->m_PacketData;

		pData->SetSize(0, &g_PacketDataBank);
		pData->Init(nMaxDataLen, nGrowSize);
	}
	else
	{
		pMsg->m_DataGrowBy = nGrowSize;
	}
}
#endif

void sm_SetObjectSFXMessage(CServerMgr* pMgr, LTObject* pObj, ServerMessage* pMsg)
{
	if (pObj->m_pSData->m_pSFXMessage == 0)
	{
		((CMessageRef*)&pObj->m_pSData->m_pSFXMessage)->Init(
			msg_AllocPacket(0x44c, 0x44c));
	}

#ifdef LT_SERVER_BUILD
	pObj->m_pSData->m_pSFXMessage->Init(pMsg->m_Unknown34, 0x1000);
#else
	sm_SFXMsgInit(pObj->m_pSData->m_pSFXMessage, pMsg->m_Unknown34, 0x1000);
#endif

	memcpy(pObj->m_pSData->m_pSFXMessage->m_pData,
		pMsg->m_pData + 1, pMsg->m_Unknown34 - 1);

	pObj->m_pSData->m_pSFXMessage->m_Unknown34 =
		pMsg->m_Unknown34 - 1;

	sm_UpdateObjectBBox(pMgr, pObj);
}


// 0x10027c97 — open this client's update with whatever the two guaranteed
// packet sets still owe. Each of the two cursors gets both of its client
// packet sets: a set small enough to fit is appended into the cursor's
// message, one too big goes out on its own, and either way the set is reset
// to empty afterwards. Both loop counters live in frame slots because ebx,
// esi and edi are all spoken for by the two walks and the set's message.
//
// Written as two indexed loops, `m_Cursors[i]` and `m_PacketSets[i][j]`. VC6
// strength-reduces both subscripts into its own pointers and turns the unread
// counters into the two frame down-counters (0x10027d1d, 0x10027d2f); the
// compiler's pointers are what put the cursor walk in edi and the set's
// message in ebx (0x10027cb2, 0x10027cc1). Hand-written `pSet++`/`pCursor++`
// pointers gave the same bytes with those two registers swapped.
void sm_WriteUpdateHeader(CServerMgr* pMgr, CUpdateContext* pCtx, Client* pClient)
{
	int i;
	int j;

	for (i = 0; i < 2; i++)
	{
		ServerMessage* pMsg = pCtx->m_Cursors[i].m_pMsg;

		for (j = 0; j < 2; j++)
		{
			ClientPacketSet* pSet = &pClient->m_PacketSets[i][j];
			ServerMessage* pSetMsg = (ServerMessage*)pSet->m_pPacket;

			if (pSetMsg->m_Unknown34 - 1 > 0)
			{
				if (pSetMsg->m_Unknown34 - 1 <
					((CMsgLenView*)pMsg)->GetSpaceLeft())
				{
					ServerMessage* pOut = (ServerMessage*)pSet->m_pPacket;

					((CMsgDataView*)pMsg)->WriteData(pOut->m_pData + 1,
						pOut->m_Unknown34 - 1);
				}
				else
				{
					sm_SendToClient(pMgr, pClient, 0xe,
						(ServerMessage*)pSet->m_pPacket, 1, pSet->m_Unknown08);
				}

				{
					ServerMessage* pReset = (ServerMessage*)pSet->m_pPacket;

					pReset->m_Unknown38 = 1;
					pReset->m_Unknown34 = 1;
				}
			}
		}

		// lithtech.exe expands this, Insert2 and all (0x00465175..
		// 0x004652cf); server.dll calls 0x1001393e.
		((CPacket*)pMsg)->WriteType((uint8)0);
	}
}


// 0x10027155 — put one client's view on another's eyes. A client cannot
// watch itself; anything else detaches whatever it was watching first, files
// the child on the parent's attachment list, and tells the child which
// object it is now looking through.
//
// The message is allocated up front and released on both exits. The error
// path's release is NOT folded away even though the reference is provably
// null there, because the reference's address is taken later in the function
// and the two printer calls in between could alias it — which is exactly why
// sm_BroadcastPrintf's early return, taken before any call, does fold.
//
// msg_AllocPacket's returned temporary goes into the dead pParent argument
// slot, the parent having been cached in esi.
LTRESULT sm_AttachClient(CServerMgr* pMgr, Client* pParent, Client* pChild)
{
	CMessageRef msg;

	if (pParent == pChild)
		RETURN_ERROR(1, "sm_AttachClient", LT_INVALIDPARAMS);

	msg.Init(msg_AllocPacket(0x44c, 0x44c));

	sm_DetachClient(pMgr, pChild);

	pChild->m_pViewClient = pParent;

	dl_Insert(&((CClientAttachView*)pParent)->m_AttachHead,
		&((CClientAttachView*)pChild)->m_AttachLink);

	{
		LTObject** ppObj = &pParent->m_pUnknown3AC;

		if (*ppObj)
		{
			{
				ServerMessage* pMsg = msg.m_pMsg;

				pMsg->m_Unknown38 = 1;
				pMsg->m_Unknown34 = 1;
			}

			// WriteType<uint16>, expanded at 0x00463dea in lithtech.exe.
			((CPacket*)msg.m_pMsg)->WriteType((*ppObj)->m_ObjectID);

			sm_SendToClient(pMgr, pChild, 7, msg.m_pMsg, 1, 0x80);
		}
	}

	return LT_OK;
}


// 0x1000e3f9 again, through the three-argument fixed prototype the removal
// notice was compiled against (its cleanup batches with the two calls that
// follow it). Name ours.
void dsi_PrintRemoveClient(char* pFormat, int nClientID, int nRemaining);

// Three more of the client teardown, named by address: 0x100272d6 undoes
// whatever 0x10027155 did to the clients attached to this one, 0x100273bc
// takes one object out of a client's table, and 0x100273e9 releases the
// client record itself.
void sub_100272d6(CServerMgr* pMgr, Client* pClient);
void sub_100273e9(CServerMgr* pMgr, Client* pClient);

// 0x100273bc — one client stops hearing a sound: the client's own flag byte
// is marked, the instance's listener count comes down, and the timer is
// zeroed when the last listener goes. A thiscall on the SoundInstance with
// the flag byte as its one stack argument (`ret $4`), which is a member
// servermgr.h does not declare (name ours).
// The count at +0x28 is a count, not the `void*` servermgr.h declares: this
// function decrements it and compares it against zero unsigned (`jbe`), and
// zeroes m_fTimer when it reaches nothing.
//
// THE BODY IS IN THE CLASS, because lithtech.exe's /O2 build EXPANDS it into
// sm_RemoveClient. 0x004640fc..0x0046412c carries the whole thing with no
// call in it — `mov 0x34(%eax),%edx ; test $0x428,%edx ; je` (the three
// PLAYSOUND bits), `test $0x4,%dl ; jne` (the LOOP bit), `or %bl,0x2(%ecx)`
// with the 1 hoisted into %bl for the loop, `dec 0x28(%eax)` under a `jbe`,
// and `movl $0x0,0x20(%eax)`. A non-template member DEFINED OUT OF CLASS is
// the one construct /Ob1 never expands, which is why we were calling it.
//
// server.dll cannot move. The shipped /O1 build compiled this SAME source —
// the one that /O2 expands, so an inline candidate there too — and CALLS it
// at every one of its three sites (0x10027363, 0x10027dff, 0x10029f9f). /Os
// therefore declines it at this size with this source, and our body is
// byte-identical at /O1 (0x100273bc, 45 of 45), so neither this function nor
// its callers can change.
class CSoundListenerView
{
public:
	void DropListener(uint8* pFlags)         // 0x100273bc
	{
		uint32 nFlags = m_PlaySoundFlags;

		if ((nFlags & (PLAYSOUND_ATTACHED | PLAYSOUND_TIME | PLAYSOUND_TIMESYNC)) &&
			!(nFlags & PLAYSOUND_LOOP))
		{
			*pFlags |= 1;

			if (m_nListeners > 0)
				m_nListeners--;

			if (m_nListeners == 0)
				m_fTimer = 0.0f;
		}
	}

	uint8  m_pad_00[0x20];
	float  m_fTimer;                         // +0x20
	uint8  m_pad_24[0x28 - 0x24];
	uint32 m_nListeners;                     // +0x28
	uint8  m_pad_2c[0x34 - 0x2c];
	uint32 m_PlaySoundFlags;                 // +0x34
};

// 0x10027308 — remove one client. Its view attachment goes first, then the
// sounds it still owns are taken out of its object table, then the state is
// reset, the shell told, and the record unlinked and freed.
//
// The leaving count in the log line is the list count minus one — the client
// is still on the list when the message is printed.
//
// 180 of 180 at /O1. The flag-byte block that had been written up as a SIB
// base/index near was an ARTEFACT OF THE CALL: because we passed
// &pInfo->m_Unknown02 to an out-of-line DropListener we had to materialise
// the address (`lea (%edx,%eax,4),%eax ; mov 0x2(%eax),%dl ; add $0x2,%eax`),
// where lithtech.exe with DropListener EXPANDED folds it into one addressing
// mode (`lea (%edx,%ecx,4),%ecx ; test %bl,0x2(%ecx)`, 0x004640f4). The body
// move above is what closes it; the source below is unchanged.
void sm_RemoveClient(CServerMgr* pMgr, Client* pClient)
{
	LTLink* pCur;
	LTLink* pHead;

	dsi_PrintRemoveClient("Removing client, id %d, leaving %d.",
		pClient->m_ClientID, pMgr->m_ClientList.m_nElements - 1);

	sm_DetachClient(pMgr, pClient);
	sub_100272d6(pMgr, pClient);

	pCur = pMgr->m_List54.m_Head.m_pNext;
	pHead = &pMgr->m_List54.m_Head;

	while (pCur != pHead)
	{
		SoundInstance* pSound = (SoundInstance*)pCur->m_pData;
		CObjInfo* pInfo =
			&pClient->m_pObjInfos[((uint32*)pSound->m_pUnknown0c)[2]];

		if (!(pInfo->m_Unknown02 & 1))
			((CSoundListenerView*)pSound)
				->DropListener(&pInfo->m_Unknown02);

		pCur = pCur->m_pNext;
	}

	if (pClient->m_ClientFlags & CLIENTFLAG_LOCAL)
		pMgr->m_Unknown1a0 &= ~2;

	sm_SetClientState(pMgr, pClient, 0);

	pMgr->m_ClassMgr.m_pServerShell->OnRemoveClient((HCLIENT)pClient);

	dl_RemoveAt(&pMgr->m_ClientList, &pClient->m_Link);

	sub_100273e9(pMgr, pClient);
}


// 0x100296d3 — queue one message on a client, or send it straight out.
//
// LTRESULT, not the void servermgr.h declares: the single exit ends
// `xor %eax,%eax`. And the manager argument is never read — every send here
// goes through the g_pServerMgr global instead, which is presumably why the
// parameter survived at all.
//
// The packet set is picked out of two bits of the caller's flags: bit 7
// chooses the row and bit 1 the column of Client::m_PacketSets. A local
// client, a message with neither queue bit set, or one longer than 255 bytes
// skips the queue entirely; so does one that will not fit in the set even
// after the set has been flushed, and that arm cross-jumps into the direct
// send's argument pushes (only the packet id differs).
//
// GetSpaceLeft's answer is compared against the message length plus one, and
// the flush happens when the message is the larger, so what 0x1000e845
// really reports is the room left rather than the bytes used; the view's
// name is kept as it was first written.
// STAND-IN: the full set's send and reset, as a helper. lithtech.exe spends
// /O2 inline budget on it, which keeps the Append loop's Insert2 a call
// (0x004680fa -> 0x00412f50) inside the expanded WriteType<uint8> below;
// written straight, Insert2 expands too (662 bytes against 459). server.dll's
// /Os declines a plain inline here and calls it (the queue function then
// loses its match, 196 against 229), so there it is forced: its code is the
// same as when the body is written out at the call site.
#ifdef LT_SERVER_BUILD
__forceinline
#else
inline
#endif
void sm_FlushPacketSet(Client* pClient, ClientPacketSet* pSet)
{
	sm_SendToClient(g_pServerMgr, pClient, 0xe,
		(ServerMessage*)pSet->m_pPacket, 1, pSet->m_Unknown08);

	{
		ServerMessage* pReset = (ServerMessage*)pSet->m_pPacket;

		pReset->m_Unknown38 = 1;
		pReset->m_Unknown34 = 1;
	}
}

LTRESULT sm_QueueMessageToClient2(CServerMgr* pMgr, Client* pClient, int nPacketID,
                                  ServerMessage* pMsg, uint32 flags)
{
	if (!(pClient->m_ClientFlags & CLIENTFLAG_LOCAL) && (flags & 3) &&
		pMsg->m_Unknown34 <= 0xff)
	{
		ClientPacketSet* pSet =
			&pClient->m_PacketSets[(flags >> 7) & 1][(flags >> 1) & 1];

		if (pMsg->m_Unknown34 + 1 >
			((CMsgLenView*)pSet->m_pPacket)->GetSpaceLeft())
		{
			sm_FlushPacketSet(pClient, pSet);
		}

		if (pMsg->m_Unknown34 + 1 >
			((CMsgLenView*)pSet->m_pPacket)->GetSpaceLeft())
		{
			sm_SendToClient(g_pServerMgr, pClient, 0xe, pMsg, 1, flags);
		}
		else
		{
			pMsg->m_pData[0] = (uint8)nPacketID;

			// lithtech.exe 0x004680c0..0x00468160 expands this byte as
			// CPacket::WriteType<uint8> (the "CPacket::WriteType" overrun
			// report is inline); server.dll calls ServerMessage::WriteByte.
#ifdef LT_SERVER_BUILD
			((ServerMessage*)pSet->m_pPacket)->WriteByte((uint8)pMsg->m_Unknown34);
#else
			((CPacket*)pSet->m_pPacket)->WriteType((uint8)pMsg->m_Unknown34);
#endif

			((CMsgDataView*)pSet->m_pPacket)->WriteData(pMsg->m_pData,
				pMsg->m_Unknown34);
		}
	}
	else
	{
		sm_SendToClient(g_pServerMgr, pClient, nPacketID, pMsg, 1, flags);
	}

	return LT_OK;
}


// 0x1002acfa — the object map entry for an id, or null (name ours; it is
// sm_FindObjectByID's sibling and hands back the entry rather than the
// object behind it).
CObjectMapEntry* sm_FindObjectMapEntry(CServerMgr* pMgr, uint16 nID)
{
	if (nID < pMgr->m_Unknown8fc.m_nElements)
		return &pMgr->m_Unknown8fc.m_pArray[nID];

	return LTNULL;
}

uint32 sm_GetObjectChangeFlags2(CServerMgr* pMgr, class LTObject* pObject);  // 0x1002a264

// 0x10027d39 — write the ids this client was sent last frame back to it, so
// it can drop what it will not hear about again. Every id that is still
// flagged 0x1000 is skipped; the rest go out in one run behind a two-byte
// header, and the packet is split whenever it fills up (a split resets the
// run, so the header goes out again).
//
// The run counter doubles as the "header written" flag and as the literal
// zero the first header byte carries. The trip count lives in the dead
// pList argument slot, and the `dec`/`inc` pair around the zero test is the
// post-decrement of `while (i--)`.
//
// The map entry's type decides what the id meant: type 1 is an object, whose
// change flags are recomputed, and anything else is a sound instance, which
// loses this client as a listener and is marked with the fixed flags 3.
void sm_WriteIDList(CUpdateContext* pCtx, ClientIDList* pList)
{
	uint32   i = pList->m_nIDs;
	uint16*  pID = pList->m_pIDs;
	uint32   nWritten = 0;

	while (i--)
	{
		if (!(pCtx->m_pClient->m_pObjInfos[*pID].m_ChangeFlags & 0x1000))
		{
			{
				ServerMessage* pMsg = pCtx->m_Msg1.m_pMsg;

				if (sm_FinishUpdatePacket(pCtx, pMsg, 8, 0x10))
					nWritten = 0;
			}

			if (nWritten == 0)
			{
				// lithtech.exe (0x00465300) expands the second byte write and
				// the word write below but CALLS the first (0x0046537c ->
				// 0x004159f0, CPacket::WriteType<uint8>): the /O2 inline budget
				// runs out at the earliest site. STAND-IN: WriteByte is
				// servermsg.h's name for that same out-of-line writer; three
				// WriteType spellings expand all three (820 bytes vs 670).
				// server.dll calls all three either way.
				pCtx->m_Msg1.m_pMsg->WriteByte((uint8)nWritten);
				((CPacket*)pCtx->m_Msg1.m_pMsg)->WriteType((uint8)3);
			}

			((CPacket*)pCtx->m_Msg1.m_pMsg)->WriteType(*pID);

			nWritten++;

			{
				CObjectMapEntry* pEntry =
					sm_FindObjectMapEntry(pCtx->m_pMgr, *pID);

				if (pEntry && pEntry->m_pRecord)
				{
					if (pEntry->m_nFlags == 1)
					{
						pCtx->m_pClient->m_pObjInfos[*pID].m_ChangeFlags =
							(uint16)sm_GetObjectChangeFlags2(pCtx->m_pMgr,
								(LTObject*)pEntry->m_pRecord);
					}
					else
					{
						// The flag byte is spelled out at both uses, not held
						// in a `uint8* pFlags` local: with the local, /O2 keeps
						// the tested byte in %al and ORs the register, where
						// lithtech.exe tests memory and reloads for the OR
						// (0x00465537, 0x0046554c). server.dll is identical
						// either way.
						if (!(pCtx->m_pClient->m_pObjInfos[*pID].m_Unknown02 & 1))
						{
							((CSoundListenerView*)pEntry->m_pRecord)
								->DropListener(&pCtx->m_pClient->m_pObjInfos[*pID].m_Unknown02);
						}

						pCtx->m_pClient->m_pObjInfos[*pID].m_ChangeFlags = 3;
					}
				}
			}
		}

		pID++;
	}
}


// 0x100264a5 — write every entry of one file type out of the cache list into
// the client's message, the reference its caller holds. The fifth argument is the byte that introduces
// the run and the sixth the entry type it matches (name ours).
LTRESULT sub_100264a5(CServerMgr* pMgr, Client* pClient, uint32 iEntry,
                      CMessageRef* pRef, int nLeadByte, uint16 nType);

// 0x1002642c — tell one client about one entry of the cache list (name ours;
// the index is the entry just added). Four passes, one per file type, in the
// order 0,1,2,3 with lead bytes 2,4,3,5.
//
// LTRESULT, not the void first declared here: the single exit ends
// `xor %eax,%eax`. msg_AllocPacket's reference is initialised straight
// into the local the four passes share, and released on the way out.
LTRESULT sm_SendCacheEntry(CServerMgr* pMgr, Client* pClient, uint32 iEntry)
{
	CMessageRef ref = msg_AllocPacket(0x44c, 0x44c);

	sub_100264a5(pMgr, pClient, iEntry, &ref, 2, 0);
	sub_100264a5(pMgr, pClient, iEntry, &ref, 4, 1);
	sub_100264a5(pMgr, pClient, iEntry, &ref, 3, 2);
	sub_100264a5(pMgr, pClient, iEntry, &ref, 5, 3);

	return LT_OK;
}

// One entry of the file-cache list at CServerMgr+0x91c: what kind of file it
// is and which file id, four bytes apiece (names ours). The count is at
// 0x920 and how many the block holds at 0x924.
class CCacheListEntry
{
public:
	uint16 m_Type;                           // +0x00
	uint16 m_FileID;                         // +0x02
};

// 0x100264a5 / 0x004629d0 — the body of the prototype declared above and
// never defined, the fifth of those this sweep has turned up. Its own caller,
// sm_SendCacheEntry, sits just below it and has been reconstructed for ages;
// only this half was missing, and a bare-address grep could not see it
// because the declaration carries the server address.
//
// Two signature corrections, both read off the bytes rather than guessed:
//
//   * the return is LTRESULT, not the declared void — `xor %eax,%eax` at
//     0x1002657f / 0x00462b6f;
//   * the sixth parameter is uint16, not int — the compare at 0x100264dd /
//     0x00462a0b is sixteen bits wide, and an `int` would have promoted the
//     member and compared thirty-two.
//
// The lead byte goes out on the FIRST match and again after every flush, and
// the closing send happens only if at least one entry matched, which is what
// the flag initialised to one and cleared at the first write is for.
//
// The loop runs on its own counter, i, seeded from iEntry: shipped stores
// the seed straight back into iEntry's slot (`mov %eax,0x10(%ebp)` at
// 0x100264bf), reloads it at every use and bumps it with `incl 0x10(%ebp)`
// (0x10026544) -- a second variable coalesced into the parameter's home.
// Looping on iEntry itself left it enregistered for the first compare and
// the first index (214 bytes, `inc %eax`); with i, byte-identical at /O1.
//
// lithtech.exe 0x004629d0: IDENTICAL (422). Shipped CALLS the first two
// writes (0x00462a24 -> 0x004159f0, 0x00462a3f -> 0x0042fa50), expands
// GetSpaceLeft (the `js` at 0x00462a57), and EXPANDS only the re-opening
// byte after a flush (0x00462a88..0x00462b25), with Insert2 still a call
// (0x00462abd -> 0x00412f50). That last site is CPacket::WriteType; the
// first two keep the ServerMessage stand-ins, which /Ob1 cannot expand.
// With the flush written out at both sites the /O2 budget was left big
// enough to expand Insert2 as well (601 bytes); the flush as an inline
// helper, expanded twice, spends it -- the helper is ours, but it is also
// the only spelling measured that leaves Insert2 a call. server.dll is
// unchanged (223, identical): /O1 calls 0x1001393e for either spelling.
inline void sm_FlushCacheMsg(CServerMgr* pMgr, Client* pClient, CMessageRef* pRef)
{
	sm_SendToClient(pMgr, pClient, 0x15, pRef->m_pMsg, 0, 0x80);

	{
		// 0x10026531 then 0x10026537 — 0x38 BEFORE 0x34, in both
		// images (0x00462a80 / 0x00462a84).
		ServerMessage* pMsg = pRef->m_pMsg;

		pMsg->m_Unknown38 = 1;
		pMsg->m_Unknown34 = 1;
	}
}

LTRESULT sub_100264a5(CServerMgr* pMgr, Client* pClient, uint32 iEntry,
                      CMessageRef* pRef, int nLeadByte, uint16 nType)
{
	// 0x100264b2 / 0x004629ee — a DWORD, set to 1 and cleared with `and $0x0`
	// at /O1. "Nothing written yet", so the closing send is skipped when the
	// run is empty. Server keeps it in its own home slot and never spills a
	// second local; /O2 enregisters it and spills to a fresh slot at
	// 0x00462b36, which is why the lithtech frame carries two dwords.
	LTBOOL bFirst = LTTRUE;
	uint32 i;

	for (i = iEntry; i < pMgr->m_Unknown920; i++)
	{
		// 0x100264d0 / 0x00462a00 — m_Unknown91c is reloaded on every
		// iteration and AGAIN for the id below; the source names the member
		// at each use rather than caching the base.
		if (((CCacheListEntry*)pMgr->m_Unknown91c)[i].m_Type == nType)
		{
			if (bFirst)
			{
				pRef->m_pMsg->WriteByte((uint8)nLeadByte);

				bFirst = LTFALSE;
			}

			pRef->m_pMsg->WriteWord(
				((CCacheListEntry*)pMgr->m_Unknown91c)[i].m_FileID);

			// 0x1002650f call / 0x00462a44 expanded. 0x32 is the same
			// threshold smgr.cpp already uses for the model hash and the
			// cache-record list.
			if (((CMsgLenView*)pRef->m_pMsg)->GetSpaceLeft() < 0x32)
			{
				sm_FlushCacheMsg(pMgr, pClient, pRef);

				// 0x1002653d — the run is re-opened, and bFirst is NOT put
				// back: the closing send still has to happen.
				((CPacket*)pRef->m_pMsg)->WriteType((uint8)nLeadByte);
			}
		}
	}

	// 0x10026556 / 0x00462b40
	if (!bFirst)
	{
		sm_FlushCacheMsg(pMgr, pClient, pRef);
	}

	return LT_OK;                                    // 0x1002657f / 0x00462b6f
}


// 0x10034cf9 — remember that a file has to be cached, and tell every client
// about it unless the shell is doing a caching pass of its own (bit 4 of
// 0x1a0). A file already on the list is not added twice.
//
// The list grows twenty entries at a time by hand — allocate, copy, free —
// rather than through any container.
void sm_BroadcastCacheFile(CServerMgr* pMgr, uint32 fileType, uint16 nFileID)
{
	uint32           nEntries;
	uint32           i;

	nEntries = pMgr->m_Unknown920;

	for (i = 0; i < nEntries; i++)
	{
		if (((CCacheListEntry*)pMgr->m_Unknown91c)[i].m_FileID == nFileID)
			return;
	}

	if (nEntries == pMgr->m_Unknown924)
	{
		uint32 nNew = pMgr->m_Unknown924 + 20;
		CCacheListEntry* pNew =
			(CCacheListEntry*)dalloc(nNew * sizeof(CCacheListEntry));

		memcpy(pNew, pMgr->m_Unknown91c,
			pMgr->m_Unknown924 * sizeof(CCacheListEntry));

		dfree(pMgr->m_Unknown91c);

		pMgr->m_Unknown91c = pNew;
		pMgr->m_Unknown924 = nNew;
	}

	((CCacheListEntry*)pMgr->m_Unknown91c)[pMgr->m_Unknown920].m_FileID = nFileID;

	{
		CCacheListEntry** ppEntries = (CCacheListEntry**)&pMgr->m_Unknown91c;

		(*ppEntries)[pMgr->m_Unknown920].m_Type = (uint16)fileType;
	}

	pMgr->m_Unknown920++;

	if (pMgr->m_Unknown1a0 & 0x10)
		return;

	{
		LTLink* pCur = pMgr->m_ClientList.m_Head.m_pNext;
		LTLink* pHead = &pMgr->m_ClientList.m_Head;

		while (pCur != pHead)
		{
			sm_SendCacheEntry(pMgr, (Client*)pCur->m_pData,
				pMgr->m_Unknown920 - 1);

			pCur = pCur->m_pNext;
		}
	}
}


// The object's model pointer, which ltobject_internal.h does not reach:
// +0x1dc, compared against the manager's placeholder to tell a model that is
// still loading from one that really is there (name ours).
class CObjectModelView
{
public:
	// AVP2: +0x1fc, as CModelInstView has it (Primal Hunt server.dll
	// 0x10041185 reads it there); NOLF's was +0x1f0.
	// AvP2 1.0.9.6: +0x1f8 and +0x24c (server.dll 0x100330dd).
#ifdef PRIMAL_HUNT
	uint8        m_pad_00[0x1fc];
	class Model* m_pModel;                   // +0x1fc
	uint8        m_pad_200[0x260 - 0x200];
#else
	uint8        m_pad_00[0x1f8];
	class Model* m_pModel;                   // +0x1f8
	uint8        m_pad_1fc[0x24c - 0x1fc];
#endif
	// +0x204 — only ever read as "does this container-type object have one";
	// nonzero makes the change-flag reader set the attachment bit even with
	// no attachment list (name ours).
	uint32       m_Unknown204;               // +0x260
};

// A second object list at CServerMgr+0x5e4, inside the padding servermgr.h
// leaves after the world-model list. Its objects' model files are cached as
// type 1 where the main list's are types 0 and 2 (name ours).
class CServerMgrList5E4View
{
public:
	uint8  m_pad_00[0x5e8];
	LTLink m_List5E4;                        // +0x5e8
};

// 0x100338e1 — walk the world's objects and put every model and skin file
// they name on the cache list. An object still showing the placeholder model
// is skipped: it has not finished loading, so its file names are not its
// own yet.
//
// Two lists, both naming their head in a local — and the first is a `for`
// where the second is a `while`. That is the whole difference between this
// and the 189-of-195 near it used to be: with all three callee-saved
// registers already spoken for, the head address cannot live in one, and
// **a `for` loop rematerialises the loop-invariant into BOTH predecessors of
// the header** (the preheader and the latch, back edge aimed at the test),
// where the same body written as a `while` keeps one copy inside the header
// and aims the back edge at that. Six bytes, one `lea 0x5c4(%esi),%eax`,
// and no register allocation involved. Same lever as CNetMgr::GetPacket's
// unrotated back edge.
LTRESULT sm_CacheObjectFiles(CServerMgr* pMgr)
{
	LTLink* pCur;
	LTLink* pHead;

	pHead = pMgr->ObjectListHead();

	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		LTObject* pObj = (LTObject*)pCur->m_pData;

		if (((CObjectModelView*)pObj)->m_pModel != pMgr->m_pPlaceholderModel)
		{
			if (pObj->m_pSData->m_pModelFile)
			{
				sm_CacheFile(pMgr, 0,
					fs_GetFilename(&pMgr->m_FileMgr, pObj->m_pSData->m_pModelFile));
			}

			// AVP2: the object data has four skins (ltobject_internal.h's
			// SObjData), and Primal Hunt's an extra texture after them.
			// Every skin is cached (server.dll 0x100411bd..0x100411ec;
			// AvP2 1.0.9.6's 0x1003d94f..0x1003d97e), then the first
			// again, and in Primal Hunt then the extra one with no test at
			// all (0x10041214): fs_GetFilename is handed whatever is there.
			{
				int i;

				for (i = 0; i < MAX_MODEL_TEXTURES; i++)
				{
					if (pObj->m_pSData->m_pSkinFiles[i])
					{
						sm_CacheFile(pMgr, 2,
							fs_GetFilename(&pMgr->m_FileMgr,
								pObj->m_pSData->m_pSkinFiles[i]));
					}
				}
			}

			if (pObj->m_pSData->m_pSkinFiles[0])
			{
				sm_CacheFile(pMgr, 2,
					fs_GetFilename(&pMgr->m_FileMgr,
						pObj->m_pSData->m_pSkinFiles[0]));
			}

#ifdef PRIMAL_HUNT
			sm_CacheFile(pMgr, 2,
				fs_GetFilename(&pMgr->m_FileMgr,
					pObj->m_pSData->m_pFile44));
#endif
		}
	}

	{
		pHead = &((CServerMgrList5E4View*)pMgr)->m_List5E4;

		pCur = pHead->m_pNext;

		while (pCur != pHead)
		{
			LTObject* pObj = (LTObject*)pCur->m_pData;

			if (pObj->m_pSData->m_pModelFile)
			{
				sm_CacheFile(pMgr, 1,
					fs_GetFilename(&pMgr->m_FileMgr, pObj->m_pSData->m_pModelFile));
			}

			pCur = pCur->m_pNext;
		}
	}

	return LT_OK;
}


#ifdef PRIMAL_HUNT
// PH: Primal Hunt's "child model" table: for each world, which model file
// stands for which extra child model. Read from childmodel.map once (the
// first world start that finds the table empty) and handed to the server
// interface world by world. The globals' initializers sit here in
// server.dll's code (0x1004128b map, 0x100412c8 string), between
// sm_CacheObjectFiles and the first function that uses them, which is
// where the definitions are (names ours).
std::map<std::string, std::map<std::string, std::string> > g_ChildModelMapQ523;
std::string g_sChildModelDefaultQ523;

// server.dll 0x10041303, lithtech.exe 0x0048ca10 -- hand the interface
// every link the table has for this world (ILTServer's
// ResetModelToChildModelLink, then LinkModelToExtraChildModel once per
// entry, each with a one-name list). The world name is lowered first: the
// table's keys are.
void sm_ApplyChildModelsQ523(ILTServer* pServer, char* pWorldName)
{
	char szWorld[256];

	pServer->ResetModelToChildModelLink();

	strcpy(szWorld, pWorldName);
	_strlwr(szWorld);

	std::string sWorld(szWorld);

	std::map<std::string, std::string>& links = g_ChildModelMapQ523[sWorld];

	if (links.size())
	{
		for (std::map<std::string, std::string>::iterator it = links.begin();
		     it != links.end(); ++it)
		{
			char* pChild = (char*)it->second.c_str();

			pServer->LinkModelToExtraChildModel((char*)it->first.c_str(),
			                                    &pChild, 1);
		}
	}
}
#endif

// 0x1002a365 — shut every model down at server teardown. Three passes over
// the model hash: bump every cache count so nothing can go while the sweep
// runs, then bring each one back down and delete the ones that reach zero,
// then complain about whatever is left.
//
// The delete is a real `delete`: the null test and the `push $1` into vtable
// slot 0 are VC6's scalar deleting destructor, not something the source
// wrote.
void sm_TermModels(CServerMgr* pMgr)
{
	void* hIter;

	hIter = dhash_GetFirst(pMgr->m_hModelHash);
	while (hIter)
	{
		Model* pModel = (Model*)dhash_GetData(dhash_GetNext(&hIter));

		pModel->m_CacheCount++;
	}

	hIter = dhash_GetFirst(pMgr->m_hModelHash);
	while (hIter)
	{
		void*  hElement = dhash_GetNext(&hIter);
		Model* pModel = (Model*)dhash_GetData(hElement);

		// AVP2: the count only comes down while it is nonzero, compared
		// unsigned (`jbe` over the decrement, Primal Hunt 0x0047da62), each removal is
		// reported at debug level 3, and both prints are gated on it
		// (AvP2 1.0.9.6 0x00477096 / 0x004781aa alike).
		if (pModel->m_CacheCount > 0)
			pModel->m_CacheCount--;

		if (pModel->m_CacheCount == 0)
		{
			if (g_DebugLevel >= 3)
				dsi_ServerError("Removing model from resource list:  %s\n", pModel->GetFilename());

			dhash_Remove(pMgr->m_hModelHash, hElement);

			delete pModel;
		}
	}

	hIter = dhash_GetFirst(pMgr->m_hModelHash);
	while (hIter)
	{
		Model* pModel = (Model*)dhash_GetData(dhash_GetNext(&hIter));

		if (g_DebugLevel >= 3)
			dsi_ServerError("Warning!  Server unable to unload model %s due to reference count!",
				pModel->GetFilename());
	}
}


// 0x1002ade4 — move an object between the two halves of the live-object list
// and tell the game object which way it went. Nothing happens when the state
// bits it is already carrying are the ones asked for.
//
// The manager argument is never read: the list this walks is the global
// manager's, reached through g_pServerMgr. Active objects go on the tail and
// inactive ones on the head, and the two arms cross-jump into one
// EngineMessageFn call, the message id being all that differs.
void sm_SetObjectState(CServerMgr* pMgr, LTObject* pObj, uint32 nStateBits)
{
	uint32 nFlags = pObj->m_ChangeFlags;

	if (nStateBits != (nFlags & 0x38))
	{
		pObj->m_ChangeFlags = (nFlags & ~0x38) | nStateBits;

		dl_RemoveAt(&g_pServerMgr->m_List8dc, &pObj->m_pSData->m_StateLink);

		if (nStateBits != 0)
		{
			dl_AddTail(&g_pServerMgr->m_List8dc, &pObj->m_pSData->m_StateLink,
				pObj);

			pObj->m_pSData->m_pBaseClass->EngineMessageFn(MID_DEACTIVATING,
				LTNULL, 0.0f);
		}
		else
		{
			dl_AddHead(&g_pServerMgr->m_List8dc, &pObj->m_pSData->m_StateLink,
				pObj);

			pObj->m_pSData->m_pBaseClass->EngineMessageFn(MID_ACTIVATING,
				LTNULL, 0.0f);
		}
	}
}


// 0x100334cc and 0x1002af00 — the first half of the removal bookkeeping, and
// the pending-notice record for an object if it has one (names by address).
// 0x100334cc — hand the game-side object back to its class and forget it.
// The engine data pointer is reloaded after the call (the free could have
// moved anything), which is why the store is not through a named local.
//
// ltobject_internal.h has SObjData+0x24 as a `CClassInfo*`; the dword there
// is pushed straight in as sm_FreeObject's `ClassDef*` with no second load,
// so it is a ClassDef* and the cast below carries the correction.
void sub_100334cc(CServerMgr* pMgr, LTObject* pObject)
{
	if (pObject->m_pSData && pObject->m_pSData->m_pBaseClass)
	{
		sm_FreeObjectInline(pMgr, (ClassDef*)pObject->m_pSData->m_pClass,
			pObject->m_pSData->m_pBaseClass);

		pObject->m_pSData->m_pBaseClass = LTNULL;
	}
}

// 0x1002af00 — the late-join notice this object still has outstanding, or
// null. The record's LTLink is at its +0x00 and m_pData points back at the
// record, so the record address is what the caller unlinks and frees.
//
// The object id is loaded into si ahead of the loop, which is the hoist that
// rotates the walk: the head test at the top is the preheader's.
//
// The cursor is initialised THROUGH the head, and that is what decides the
// register roles: `for (pCur = pHead->m_pNext; ...)` loads the cursor first
// and then leas the head into a scratch (edx), which leaves the callee-saved
// esi for the object id the loop hoists into its preheader. Writing it the
// other way — `pCur = pMgr->m_List8ec.m_Head.m_pNext; pHead =
// &pMgr->m_List8ec.m_Head;` with a `while` — spends esi on the head and puts
// the id in dx instead, which is instruction-identical and two registers
// wrong. (Naming the list in an `LTList*` local and taking both off that
// reaches the shipped form too.)
LTLink* sub_1002af00(CServerMgr* pMgr, LTObject* pObject)
{
	LTLink* pHead = &pMgr->m_List8ec.m_Head;
	LTLink* pCur;

	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		CUnknown8ECRec* pRec = (CUnknown8ECRec*)pCur->m_pData;

		if (pRec->m_nObjectID == pObject->m_ObjectID)
			return &pRec->m_Link;
	}

	return LTNULL;
}

// 0x1002ceac — let go of everything the engine-side object data owns; a
// thiscall on the SObjData with one argument, which ltobject_internal.h does
// not declare (name ours).
//
// Its shape is a scalar deleting destructor's — release what the object
// owns, `operator delete(this)` when bit 0 of the flag is set, return
// `this` — but it cannot be one: SObjData's +0x00 is its LTLink pair, so the
// class has no vptr and VC6 emits no `??_G` for it. So the flag, the
// `operator delete` (0x10028ace, the wrapper one call above dfree) and the
// returned `this` are all in the source, and the one call site pushes zero.
//
// What the object at +0x2c is is not determined here; only that it is
// released through slot 2 of its own vtable with nothing pushed.
class IObjDataOwned
{
public:
	virtual void _v00() = 0;
	virtual void _v04() = 0;
	virtual void Release() = 0;              // slot 2
};

class CSObjDataTermView
{
public:
	// 0x1002ceac. DEFINED IN-CLASS so /Ob1 will expand it: the shipped
	// client carries this body in line at sm_ObjectRemoved2's tail, with
	// the `nFlags & 1` arm folded away because the argument is 0. The
	// server build is /Os and declines the expansion, which is what keeps
	// the out-of-line copy there.
	void* Term(int nFlags)                   // 0x1002ceac
	{
		if (m_pOwned2c)
			m_pOwned2c->Release();

		if (nFlags & 1)
			operator delete(this);

		return this;
	}

	uint8          m_pad_00[0x2c];
	IObjDataOwned* m_pOwned2c;               // +0x2c
};

#ifdef PRIMAL_HUNT
// PH: the object data goes back to the message bank through a member of the
// bank, not through two statements at the call site: server.dll CALLS it
// (0x10040d47, thiscall on CServerMgr+0xe00 with the data pushed; the
// bank's vtable slot 2, 0x10038a68, forwards to it too) and its body is
// 0x10038c4c -- the record's destructor (0x10015563; lithtech.exe
// 0x00418e40, savobj.cpp's SavObjDataQ430 destructor), then sb_Free's
// guarded push onto the free list. lithtech.exe's /O2 expands it, which is
// why the client shows the two statements in line. CMessageBank is
// servermgr.h's, shared with files this one does not own, so the member is
// reached through a view of the same layout, as is the record, whose only
// member with a destructor is the save-data reference at +0x2c (names ours).
class CSObjDataDtorQ523
{
public:
	uint8       m_pad_00[0x2c];
	CMessageRef m_SaveData;                  // +0x2c
};

class CMessageBankFreeQ523
{
public:
	void Free(void* pStruct)                 // 0x10038c4c
	{
		((CSObjDataDtorQ523*)pStruct)->~CSObjDataDtorQ523();
		sb_Free(&m_Bank, pStruct);
	}

	void*      m_pVTable;                    // +0x00
	StructBank m_Bank;                       // +0x04
};
#endif

// ---- putting a created object into the world ----------------------------- //

// The record sm_ClaimObjectID hands back; only the id at its +0x08 is read
// here, and it is a dword the object's uint16 id is truncated from (name by
// offset).
class CObjectIDRec
{
public:
	uint8  m_pad_00[0x08];
	uint32 m_nID;                            // +0x08
};

// 0x1002585f — take an LTObject of the type the create struct asks for out
// of the object manager's per-type bank, and 0x100332e8 — fill it in from
// the create struct and thread it into the world. 0x1001eae7 is what a
// world model or a container needs doing to it once its box is known. Names
// by address.
LTRESULT sub_1002585f(class CUnknown390* p390, ObjectCreateStruct* pStruct,
                      LTObject** ppObject);
LTRESULT sub_100332e8(CServerMgr* pMgr, ObjectCreateStruct* pStruct,
                      struct ClassDef* pClassDef, LTObject* pLTObject,
                      LPBASECLASS pObject, class SObjData** ppSData);
void     sub_1001eae7(LTObject* pObject);

// 0x10031dfa — spritecontrol.cpp's name for the id-map claim, and 0x100333fc
// — sm_ObjectRemoved2 below, both declared here because this is the first
// use of either in the file.
int      sm_ClaimObjectID(CServerMgr* pMgr, void** ppRec, uint16 nID);
LTRESULT sm_ObjectRemoved2(CServerMgr* pMgr, LTObject* pObject);

// 0x100334fe — the middle third of creating an object: the engine-side
// LTObject is allocated, filled in, given an id, activated and handed back.
// Anything that fails after the allocation unwinds it again.
//
// The function's own error literal names it "sm_AddObjectToWorld", which is
// the only naming evidence there is; servermgr.h's sm_CreateObjectInWorld is
// ours, so the name is kept and the discrepancy recorded.
//
// The LTObject the allocator answers lives in the dead pMgr parameter slot
// -- the manager itself is in edi from the first instruction -- and the
// manager's object member has its address named in a local, because both
// unwind paths reach it.
//
// The id the claim hands back has to go through a `uint32` of its own: read
// straight into the object's uint16 m_ObjectID, VC6 narrows the load to
// `mov 0x8(%eax),%ax` where the shipped code loads the whole dword and
// stores the low half.
//
// Each unwind path has its OWN `return dResult` -- three failure blocks,
// the filename and id-claim failures written separately. lithtech.exe
// (0x474b58, 0x474b94, 0x474bd6) keeps all three tails apart; server.dll
// (0x100335a3 jumping into 0x100335f8's `mov %ebx,%eax`) merges them
// itself. An earlier `goto` into one shared tail with the claim nested under
// the filename result matched server.dll but left lithtech 33 bytes short.
LTRESULT sm_CreateObjectInWorld(CServerMgr* pMgr, LPBASECLASS pObject,
                                ClassDef* pClassDef, ObjectCreateStruct* pStruct,
                                uint32 nObjectID, float fData,
                                LTObject** ppObject)
{
	CUnknown390* p390;
	LTObject*    pLTObject;
	void*        pRec;
	LTRESULT     dResult;

	*ppObject = LTNULL;

	if (!pMgr->m_bWorldLoaded)
		RETURN_ERROR(1, "sm_AddObjectToWorld", LT_NOTINWORLD);

	p390 = &pMgr->m_Unknown390;

	dResult = sub_1002585f(p390, pStruct, &pLTObject);
	if (dResult != LT_OK)
		return dResult;

	pObject->m_hObject = (HOBJECT)pLTObject;

	dResult = sub_100332e8(pMgr, pStruct, pClassDef, pLTObject, pObject,
		&pLTObject->m_pSData);

	if (dResult != LT_OK)
	{
		un390_RemoveObject(p390, pLTObject);
		return dResult;
	}

	pObject->EngineMessageFn(MID_ACTIVATING, LTNULL, 0.0f);

	dResult = w_ApplyObjectFilenames(pMgr, pLTObject, pStruct);
	if (dResult != LT_OK)
	{
		sm_ObjectRemoved2(pMgr, pLTObject);
		un390_RemoveObject(p390, pLTObject);
		return dResult;
	}

	dResult = sm_ClaimObjectID(pMgr, &pRec, (uint16)nObjectID);
	if (dResult != LT_OK)
	{
		sm_ObjectRemoved2(pMgr, pLTObject);
		un390_RemoveObject(p390, pLTObject);
		return dResult;
	}

	pLTObject->m_ChangeFlags = 0x44;
	pLTObject->m_pSData->m_Unknown28 = pRec;
	{
		uint32 nID = ((CObjectIDRec*)pRec)->m_nID;

		pLTObject->m_ObjectID = (uint16)nID;
	}

	pMgr->m_Unknown8fc.m_pArray[pLTObject->m_ObjectID].m_nFlags = 1;
	pMgr->m_Unknown8fc.m_pArray[pLTObject->m_ObjectID].m_pRecord = pLTObject;

	pLTObject->m_pSData->m_pBaseClass = pObject;

	if (pLTObject->m_ObjectType == OT_WORLDMODEL ||
		pLTObject->m_ObjectType == OT_CONTAINER)
		sub_1001eae7(pLTObject);

	sm_UpdateObjectBBox(pMgr, pLTObject);

	pObject->EngineMessageFn(MID_INITIALUPDATE, pStruct, fData);

	sm_SetObjectChangeFlag(pMgr, pLTObject,
		sm_GetObjectChangeFlags2(pMgr, pLTObject));

	g_Unknown58bec += SM_OBJECT_MEMORY;

	*ppObject = pLTObject;

	return LT_OK;
}


// 0x100333fc — the manager's own last rites for an object: drop its name
// from the identifier hash, drop any pending late-join notice, break all
// three kinds of attachment, unthread it from the live-object list, and give
// the engine-side data back.
//
// The data goes back to the *message* bank at 0x9f0 — SObjData and
// ServerMessage are both 0x54 bytes and share it.
//
// LTRESULT, not the void servermgr.h declares: the body ends `xor %eax,%eax`.
// Renamed for that reason.
LTRESULT sm_ObjectRemoved2(CServerMgr* pMgr, LTObject* pObject)
{
	sub_100334cc(pMgr, pObject);

	if (pObject->m_pSData->m_hIdentifier)
		dhash_Remove(pMgr->m_hHash8c0, pObject->m_pSData->m_hIdentifier);

	if (pObject->m_ChangeFlags & 0x80)
	{
		LTLink* pLink = sub_1002af00(pMgr, pObject);

		if (pLink)
		{
			dl_RemoveAt(&pMgr->m_List8ec, pLink);

			dfree(pLink);
		}
	}

	sm_BreakAttachments(pMgr, pObject, 0, 1);
	sm_BreakAttachments(pMgr, pObject, 1, 0);
	sm_BreakAttachments(pMgr, pObject, 2, 0);
	// AVP2: a fourth attachment kind (`push $0; push $3`, Primal Hunt
	// 0x0048c0b3, AvP2 1.0.9.6 0x00483fc4).
	sm_BreakAttachments(pMgr, pObject, 3, 0);

	dl_RemoveAt(&pMgr->m_List8dc, &pObject->m_pSData->m_StateLink);

#ifdef PRIMAL_HUNT
	// PH: the release is a call, `mov %edi,%ecx; call 0x00418e40`, to the
	// record's out-of-line 13-byte destructor (`mov 0x2c(%ecx),%ecx; test;
	// je; jmp *0x8(%eax)`), from the bank's Free above, which server.dll
	// calls and lithtech.exe expands; AvP2 1.0.9.6 expands the release too.
	((CMessageBankFreeQ523*)&pMgr->m_MessageBank)->Free(pObject->m_pSData);
#else
	{
		SObjData* pSData = pObject->m_pSData;

		((CSObjDataTermView*)pSData)->Term(0);

		sb_Free(&pMgr->m_MessageBank.m_Bank, pSData);
	}
#endif

	return LT_OK;
}


// 0x10026584 — write one light animation into a message (five arguments: the
// manager, the animation, its index, the message and a byte cap):
// serverlightanim.cpp's la_WriteChange, under its own signature.
void la_WriteChange(CServerMgr* pMgr, LightAnim* pAnim, uint32 hLightAnim,
                    class ServerMessage* pMsg, uint32 nFlags);

// 0x1002666c — tell an entering client about the world: every light
// animation, packed into as few messages as they fit in. A message that has
// filled up goes out mid-loop and is reset, and whatever is left over goes
// out at the end.
//
// The animation index lives in the dead pMgr argument slot and the byte
// offset VC6 strength-reduces the subscript into lives in the returned
// packet reference's, both
// because edi and esi are already holding the manager and the send flags.
//
// NEAR: 233 of 233 bytes, 82 of 82 instructions, and every differing byte is
// one of those two frame displacements — ours puts the index in the
// temporary's slot
// and the strength-reduction offset in the parameter slot, the shipped build
// the other way round. Writing the byte offset out as a second local of my
// own makes it worse (three slots and a real 0xc frame), and the two stores
// are already in the shipped order relative to the loop guard. Frame-slot
// rotation. Also measured, all unchanged: `i + m_pLightAnims`; a
// `while` loop; `i` initialised at its declaration; `i` in its own block; a
// named element pointer at function scope; a `pServerMgr` copy; an extra
// `if (m_nLightAnims)` wrapper (238 B); an inline element accessor; the
// callee's index as uint16; (under NOLF's out-parameter spelling) the
// out-parameter at function scope in all six declaration orders (234 B
// each); the allocation as an inline helper (not inlined).
//
// lithtech.exe (0x00462ee0, 239 bytes) is IDENTICAL, and the pServerMgr copy
// is what does it: walking the array through a local copy of the manager
// pointer makes /O2 form the element address as `mov 0x1ac(%edi),%eax;
// mov %ebp,%ecx; add %eax,%ecx` (offset + base) as shipped, where the
// parameter itself gives `mov 0x1ac(%edi),%ecx; add %ebp,%ecx` (237 bytes).
// server.dll is the same near either way (which pointer variable
// the uses go through decides the add's operand order).
void sm_SendWorldInfo(CServerMgr* pMgr, Client* pClient)
{
	CMessageRef msg;
	uint32      i;

	msg.Init(msg_AllocPacket(0x44c, 0x44c));

	CServerMgr* pServerMgr = pMgr;

	for (i = 0; i < pServerMgr->m_nLightAnims; i++)
	{
		la_WriteChange(pMgr, &pServerMgr->m_pLightAnims[i], i, msg.m_pMsg, 0xff);

#ifdef PRIMAL_HUNT
		// PH: server.dll expands this one (0x100316a1), see
		// GetSpaceLeftInline.
		if (((CMsgLenView*)msg.m_pMsg)->GetSpaceLeftInline() <= 0x16)
#else
		if (((CMsgLenView*)msg.m_pMsg)->GetSpaceLeft() <= 0x16)
#endif
		{
			sm_SendToClient(pMgr, pClient, 0x19, msg.m_pMsg, 0, 0x80);

			{
				ServerMessage* pMsg = msg.m_pMsg;

				pMsg->m_Unknown38 = 1;
				pMsg->m_Unknown34 = 1;
			}
		}
	}

	{
		ServerMessage* pMsg = msg.m_pMsg;

		if (pMsg->m_Unknown34 - 1 > 0)
		{
			sm_SendToClient(pMgr, pClient, 0x19, pMsg, 0, 0x80);

			{
				ServerMessage* pReset = msg.m_pMsg;

				pReset->m_Unknown38 = 1;
				pReset->m_Unknown34 = 1;
			}
		}
	}
}


// 0x1002a264 — what the client still has to be told about this object, as a
// bitmask. The manager argument is accepted and never read.
//
// The identity rotation and the unit scale are compared against 0.0f and
// 1.0f out of .rdata, and VC6 loads the 1.0f once and leaves it in st(0) for
// all four comparisons that need it — which is why the early-out arm has to
// `fstp %st(0)` to unwind the stack.
//
// The colour test differs by object type: type 3 (a sprite) is default at
// white and everything else at black, and both arms end in the same alpha
// comparison, which VC6 cross-jumps into one copy.
//
// uint32, not the uint16 servermgr.h declares: the answer leaves in eax
// whole (`mov %esi,%eax`), where a uint16 return truncates to `mov %si,%ax`.
// Renamed for that reason; its one caller stores the result into a uint16
// field either way.
uint32 sm_GetObjectChangeFlags2(CServerMgr* pMgr, LTObject* pObject)
{
	uint32 nFlags = 0x201;

	if (pObject->m_Rotation.m_Quat[0] != 0.0f ||
		pObject->m_Rotation.m_Quat[1] != 0.0f ||
		pObject->m_Rotation.m_Quat[2] != 0.0f ||
		pObject->m_Rotation.m_Quat[3] != 1.0f)
	{
		nFlags = 0x601;
	}

	// AVP2: the mask takes bit 18 as well (`testl $0x4ffff,0x88`, AvP2
	// 1.0.9.6 0x00476f25, Primal Hunt 0x0047d915).
	if ((pObject->m_Flags & 0x4ffff) != 0 || pObject->m_UserFlags != 0)
		nFlags |= 8;

	if (pObject->m_Scale.x != 1.0f || pObject->m_Scale.y != 1.0f ||
		pObject->m_Scale.z != 1.0f)
	{
		nFlags |= 0x10;
	}

	if (pObject->m_ObjectType == 3)
	{
		if (pObject->m_ColorR != 0xff || pObject->m_ColorG != 0xff ||
			pObject->m_ColorB != 0xff || pObject->m_ColorA != 0xff)
		{
			nFlags |= 0x40;
		}
	}
	else
	{
		if (pObject->m_ColorR != 0 || pObject->m_ColorG != 0 ||
			pObject->m_ColorB != 0 || pObject->m_ColorA != 0xff)
		{
			nFlags |= 0x40;
		}
	}

	if (pObject->m_ObjectType == 1)
		nFlags |= 0x20;

	// AVP2: a third reason, bit 10 of the object's change flags (`mov
	// 0x124(%ecx),%eax; test $0x4,%ah`, Primal Hunt 0x0047d9c7, AvP2
	// 1.0.9.6 likewise).
	if (pObject->m_pAttachments != 0 ||
		(pObject->m_ObjectType == 1 &&
			((CObjectModelView*)pObject)->m_Unknown204 != 0) ||
		(pObject->m_ChangeFlags & 0x400))
	{
		nFlags |= 0x100;
	}

	return nFlags;
}


// One node of a queued message's client list: two links, one in the client's
// own queue (whose payload is the record) and one in the record's list of
// nodes (whose payload is the node itself, which is what the release walks
// to hand them back). Names ours; the record comes out of the bank at 0x864.
class CQueuedMsgNode
{
public:
	LTLink m_ClientLink;                     // +0x00
	LTLink m_RecordLink;                     // +0x0c
};

// 0x1002980d — make one queued message and hand it to every client that is
// in the world. The record is 0x164 bytes out of the bank at 0x9d4, cleared
// whole, and its reference count ends up as the number of clients that took
// a copy — which is what CQueuedMsg::Release (0x10031d47) counts back down.
//
// The declared name is servermgr.h's; what the bytes build is a CQueuedMsg,
// and the `int` it stores at +0x00 is the word the writer gate tests. The
// walk keeps its cursor in the dead pMgr argument slot because the manager,
// the record and the client all hold registers.
void* sm_AddSoundRecord(CServerMgr* pMgr, int n)
{
	CQueuedMsgView* pRec = (CQueuedMsgView*)sm_sb_Allocate(&pMgr->m_Bank9D4);
	LTLink*         pCur;
	LTLink*         pHead;

	memset(pRec, 0, sizeof(CQueuedMsgView));

	dl_InitList(&pRec->m_List140);

	pHead = &pMgr->m_ClientList.m_Head;

	pCur = pHead->m_pNext;

	while (pCur != pHead)
	{
		Client* pClient = (Client*)pCur->m_pData;

		if (pClient->m_State == 3)
		{
			CQueuedMsgNode* pNode =
				(CQueuedMsgNode*)sm_sb_Allocate(&pMgr->m_Bank864);

			dl_AddHead(&pRec->m_List140, &pNode->m_RecordLink, pNode);
			dl_AddHead(&pClient->m_List3CC, &pNode->m_ClientLink, pRec);

			pRec->m_RefCount++;
		}

		pCur = pCur->m_pNext;
	}

	*(uint32*)pRec = (uint32)n;

	return pRec;
}


// Three small inline accessors, STAND-INS spending lithtech.exe's /O2
// inline budget (per function, spent from the end backwards).
// Shipped 0x00475670 expands the constructor but CALLS the Clear() inside it
// (0x00475680 -> 0x004147f0), then expands both explicit oc.Clear() sites in
// full, m_Rotation.Init() included (0x00475729..0x00475741). Written without
// them the constructor's Clear was expanded too and the rotation Init at
// both it and the else-arm Clear stayed calls (794 bytes against 664); each
// helper added walked the constructor one step further down (Rotation, then
// Scale, then Clear itself), and with all three it is identical. Names ours;
// the original presumably reached the trees and the class manager through
// accessors of its own. server.dll expands all three and is unchanged.
inline uint32 sm_PrecacheNumTrees(CServerMgr* pMgr)
{
	return pMgr->m_nFileTrees;
}

inline CPrecacheTreeView* sm_PrecacheTree(CServerMgr* pMgr, uint32 i)
{
	CPrecacheTreeView* pTree = (CPrecacheTreeView*)pMgr->m_pFileTrees[i];

	return pTree;
}

inline HCLASS sm_PrecacheFindClass(CServerMgr* pMgr)
{
	HCLASS hClass = ((CClassMgrFindView*)&pMgr->m_ClassMgr)->FindClass("BaseClass");

	return hClass;
}

LTBOOL CPrecacheView::PrecacheModels()
{
	ObjectCreateStruct oc;
	CPrecacheTreeView* pTree;
	HCLASS             hClass;
	uint32             i;
	uint32             j;

	hClass = sm_PrecacheFindClass((CServerMgr*)this);

	if (!hClass)
		return LTFALSE;

	for (i = 0; i < sm_PrecacheNumTrees((CServerMgr*)this); i++)
	{
		pTree = sm_PrecacheTree((CServerMgr*)this, i);

		if (pTree->m_pList->m_Flags48 & 8)
		{
			for (j = 0; j < pTree->m_pList->m_nEntries140; j++)
			{
				oc.Clear();

				oc.m_ObjectType = 2;
				oc.m_Flags = 0x3000;

				if (!fs_GetTableName(&pTree->m_pList->m_Table04, j,
				                     oc.m_Filename, sizeof(oc.m_Filename)))
					return LTFALSE;

				if (!((CServerMgr*)this)->CreateObject(hClass, &oc))
					return LTFALSE;
			}
		}
		else if (pTree->m_pList->m_Flags48 & 0x30)
		{
			oc.Clear();

			oc.m_ObjectType = 2;
			oc.m_Flags = 0x3000;

			strncpy(oc.m_Filename, pTree->m_pName0c->m_Name,
				sizeof(oc.m_Filename) - 1);
			oc.m_Filename[sizeof(oc.m_Filename) - 1] = 0;

			if (!((CServerMgr*)this)->CreateObject(hClass, &oc))
				return LTFALSE;
		}
	}

	return LTTRUE;
}


// AVP2: ILTSoundMgr::KillSoundFade, the slot after KillSoundLoop that
// NOLF's interface lacks (lithtech.exe 0x0048e3f0, server.dll 0x100422d1;
// AvP2 1.0.9.6 has the "CServerMgr::KillSoundFade" literal too). Like
// KillSoundLoop it is reached through the sound personality at +0x04 and
// steps back four bytes for the manager (server.dll `add $-4,%ecx`,
// 0x10042347); lithtech.exe expands SetSoundTrackChangeFlags as it does
// there. Only a sound the game kept a handle to (PLAYSOUND_GETHANDLE, 0x10
// at +0x34) can be faded; it is flagged changed (0x20), its timer cleared
// and killed, and the fade time goes in +0x5c -- lithtech.exe's /O2 stores
// in source order, +0x20, +0x10, +0x5c (0x0048e4e1..0x0048e4ef).
// servermgr.h's CServerSoundMgr has five slots and CServerMgr does not
// declare this one, so it is defined on a view of the sound personality
// (names ours).
class CSoundFadeInstanceQ551
{
public:
	uint8 m_pad_00[0x5c];
	float m_fFadeOutTime;                    // 0x5c
};

class CServerSoundFadeQ551
{
public:
	LTRESULT KillSoundFade(HLTSOUND hSound, LTFLOAT fFadeOutTime);
};

LTRESULT CServerSoundFadeQ551::KillSoundFade(HLTSOUND hSound, LTFLOAT fFadeOutTime)
{
	SoundInstance* pSound = (SoundInstance*)hSound;

	if (!pSound)
		RETURN_ERROR(1, "CServerMgr::KillSoundFade", LT_INVALIDPARAMS);

	if (!(pSound->m_PlaySoundFlags & PLAYSOUND_GETHANDLE))
		RETURN_ERROR(1, "CServerMgr::KillSoundFade", LT_ERROR);

	sm_SetSoundTrackChangeFlags((CServerMgr*)((uint8*)this - 4), pSound, 0x20);

	pSound->m_fTimer = 0.0f;
	pSound->m_bKill = LTTRUE;
	((CSoundFadeInstanceQ551*)pSound)->m_fFadeOutTime = fFadeOutTime;
	return LT_OK;
}


// 0x1003446d — is this file already on the cache list? A thiscall on the
// manager (the caller pushes only the handle and cleans nothing up), so it
// is a member CServerMgr does not declare; reached through a view. Name
// ours: it is the only thing AddCachedFile consults before making a record.
class CCacheLookupView
{
public:
	CCacheRecView* FindCachedFile(void* hFile);   // 0x1003446d

	uint8  m_pad_00[0x44];
	LTList m_List44;                         // +0x44
};

CCacheRecView* CCacheLookupView::FindCachedFile(void* hFile)
{
	LTLink* pCur;
	LTLink* pHead;

	pCur = m_List44.m_Head.m_pNext;
	pHead = &m_List44.m_Head;

	while (pCur != pHead)
	{
		CCacheRecView* pRec = (CCacheRecView*)pCur->m_pData;

		if (pRec->m_hFile == hFile)
			return pRec;

		pCur = pCur->m_pNext;
	}

	return LTNULL;
}

// 0x1002bcd9 — the file manager's loader for a cache pass: it hands back a
// counted object whose slot 1 is released as soon as the record has been
// built from it (name ours, from the shape).
class ICacheSource
{
public:
	virtual void _v00() = 0;
	virtual void Release() = 0;
};

ICacheSource* fs_OpenCacheSource(CFileMgr* pFileMgr, char* pFilename);  // 0x1002bcd9

// 0x1003448f — put a file on the current cache pass's list and hand back its
// record. A file already recorded is handed back as it stands; a file that
// cannot be opened leaves the list alone.
//
// servermgr.h declares this address twice, as GetSoundTrack and as
// AddCachedFile; the record it builds and links onto m_List44 is what the
// pass sweeper (0x10034757) later walks, so AddCachedFile is the accurate
// one and is what is defined here.
//
// The bank pointer is named in the source and lands in the dead hFile
// argument slot -- the handle itself is in ebp by then. The filename lookup
// happens *before* the null test on the handle, which is not an ordering the
// compiler could have chosen: the source really does ask for the name first.
CCacheRec* CServerMgr::AddCachedFile(void* hFile)
{
	CCacheRecView* pRec = ((CCacheLookupView*)this)->FindCachedFile(hFile);

	if (!pRec)
	{
		char* pszName = fs_GetFilename(&m_FileMgr, hFile);

		if (hFile)
		{
			ICacheSource* pSource = fs_OpenCacheSource(&m_FileMgr, pszName);

			if (pSource)
			{
				StructBank* pBank = &m_Bank0C;
				int         bOK;

				pRec = (CCacheRecView*)sm_sb_Allocate(pBank);

				bOK = pRec->Init(hFile, pSource, ((void**)hFile)[1]);

				pSource->Release();

				if (!bOK)
				{
					sb_Free(pBank, pRec);

					return LTNULL;
				}

				dl_AddHead(&m_List44, &pRec->m_Link, pRec);
			}
		}
	}

	return (CCacheRec*)pRec;
}


// The sub-object teardowns CServerWorld::Term reaches, each a thiscall on
// the address of a member (names by address; nothing in the image names
// them).
class CWorldSub6CView
{
public:
	void Term();                             // 0x10039bbc
};

// 0x10014295 -- the point-shade grid's teardown: free the one pointer it
// owns and clear the rest (20 bytes; commonmodel.cpp's CWorldSubFC, whose
// Release at 0x10014185 is a tail jump here). Its home in the image is that
// class's run in commonmodel.cpp; written here through the view because this
// is the file that calls it.
class CWorldSubFCView
{
public:
	void*  m_pData;                          // +0x00

	void Term();                             // 0x10014295
	void Clear();                            // 0x1001424e  CWorldSubFC::Clear
};

void CWorldSubFCView::Term()
{
	dfree(m_pData);
	Clear();
}

// One entry of the array at +0x18c: a flag byte whose bit 0 says the second
// dword is worth tearing down first (names ours).
class CWorldArrayEntry
{
public:
	uint8  m_Flags;                          // +0x00
	uint8  m_pad_01[0x04 - 0x01];
	class CWorldArrayInner* m_pInner;        // +0x04

	void Term();                             // 0x10007ebe
};

class CWorldArrayInner
{
public:
	void Term();                             // 0x1000ae5c
};

// 0x10058c7c is the allocator both of Term's array teardowns pass.
extern uint32 g_WorldAllocator;              // 0x10058c7c

// 0x10007a6a — the world's own file-scope teardown, taking the world
// (cdecl), and 0x10007f73 — the last step, a thiscall on it. Names by
// address.
//
// The list at +0xf4 is the world-object records worldbsp.cpp's loader pushes
// (WorldObjectRecQT1 there, 0x14 bytes): each one's object is handed back to
// its class's free function (the class's +0x08, a plain function pointer --
// `call *0x8(%eax)` with no `this`), the record is dfree'd, and the head is
// left null. lithtech.exe's copy is standalone too (0x00421c70, 68 bytes).
class CWorldObjClassView
{
public:
	void*  m_pMake;                          // +0x00
	uint8  m_pad_04[0x08 - 0x04];
	void   (*m_pFree)(void* pObject);        // +0x08
};

class CWorldObjRecView
{
public:
	void*  m_pBsp;                           // +0x00
	void*  m_pSurface;                       // +0x04
	void*  m_pObject;                        // +0x08
	CWorldObjClassView* m_pClass;            // +0x0c
	CWorldObjRecView*   m_pNext;             // +0x10
};

class CWorldObjListView
{
public:
	uint8  m_pad_00[0xf4];
	CWorldObjRecView* m_pObjRecs;            // +0xf4
};

void w_TermWorldData(class CServerWorld* pWorld)
{
	CWorldObjRecView* pRec = ((CWorldObjListView*)pWorld)->m_pObjRecs;
	CWorldObjRecView* pNext;

	while (pRec)
	{
		pNext = pRec->m_pNext;

		pRec->m_pClass->m_pFree(pRec->m_pObject);
		dfree(pRec);

		pRec = pNext;
	}

	((CWorldObjListView*)pWorld)->m_pObjRecs = LTNULL;
}

class CWorldFinishView
{
public:
	void TermFinish();                       // 0x10007f73
};

// One thing hanging off the 0xe8 list: only its virtual destructor is
// reached, through `delete` (name ours).
class CWorldListItem
{
public:
	virtual ~CWorldListItem();
};

// The CMoArray<T, DefaultCache> at +0x04, for the one member Term needs:
// TermPointers, inlined in the shipped code (0x10009c3e).
class CWorldArray04View
{
public:
	void*   m_vptr;
	void*   m_pArray;
	uint32  m_nElements;
	uint32  m_CacheSize;
	uint32  m_WantedCache;

	void TermPointers()
	{
		m_pArray = 0;
		m_nElements = 0;
		m_CacheSize = 0;
		m_WantedCache = 0;
	}
};

// 0x10009be5 — take the world down, children first (the walk calls Term on
// each child recursively) and then this world's own link out of whatever
// list it is in.
//
// After that the teardown forks on the flag at +0x1e8. A world with it set
// drops the +0x04 array's pointers without freeing them and tears down the array at +0x18c entry by
// entry; a world with it clear does the full job — the collection at +0x04,
// the object store, the 0xe8 list's contents deleted one by one, the two
// raw pointers freed, and two more sub-objects. Both arms end by releasing
// the collection at +0x188 and calling the finisher.
//
// CServerWorld is nowhere near the eight bytes servermgr.h gives it, so
// everything here goes through the view above.
//
// The four zero stores in the child arm are CMoArray::TermPointers inlined on
// the DefaultCache array at +0x04 (vptr, m_pArray, m_nElements, and the two
// cache words at 0x08..0x14): a child world shares its parent's storage and
// forgets the pointers instead of freeing them. Spelled as four member stores
// the scheduler hoisted the loop's `xor %ebx,%ebx` and guard above them; as
// the inline call (0x10009c3e..0x10009c47 then 0x10009c4a) it does not.
// The two element types Term's arrays are instantiated on (names ours): the
// +0x04 collection's 0x60-byte record, whose SetSize2 is 0x1000c267, and the
// +0x188 array's pointer to a sixteen-byte element with a destructor
// (0x10007eb9 / 0x00422280).
//
// Term calls collection.h's own templates on them, not two out-of-line
// helpers: server.dll's /O1 declines both and calls 0x1000c267 and
// DeleteAndClearArray2 (0x1000d071) as it always did, while lithtech.exe's
// /O2 expands them -- 0x00424bc0 is identical only that way. Both
// DeleteAndClearArray2 sites expand there, with their BaseDelete folded into
// the loop's null test; the child arm's SetSize2 stays a call (0x00414fa0)
// and the other arm's expands around a called _DeleteAndDestroyArray
// (0x00413050), and the +0x04 SetSize2 expands around a called BaseDelete
// (0x004152b0). VC6 picked every one of those itself.
//
// The 0x60 record is collection.cpp's CCollElem60 -- a constructor (the
// one BaseNew 0x1000d1f2 runs) and no destructor, which is why the release
// is the plain BaseDelete -- restated here exactly as collection.cpp's
// DECLARE_COLLCTOR spells it, so that the array's COMDATs are collection.cpp's
// own (a constructor-less 0x60 stand-in compiled a second _AllocateTArray
// the shipped image does not have).
class CCollElem60
{
public:
	CCollElem60();
	uint8 m_pad[0x60];
};

// commonmodel.cpp's world record, as far as the teardown needs it: its
// destructor (0x10007eb9, a tail jump onto its Term) is defined there.
class CServerWorldRec
{
public:
	~CServerWorldRec();                      // 0x10007eb9

	uint8 m_pad[0x10];
};

void CServerWorld::Term()
{
	uint32 i;
	CServerWorldView* pThis = (CServerWorldView*)this;
	LTLink*           pCur;
	LTLink*           pHead;

	((CWorldSub6CView*)&pThis->m_Unknown6c)->Term();

	pCur = pThis->m_Children.m_pNext;
	pHead = &pThis->m_Children;

	while (pCur != pHead)
	{
		CServerWorld* pChild = (CServerWorld*)pCur->m_pData;
		LTLink*       pNext = pCur->m_pNext;

		pChild->Term();

		pCur = pNext;
	}

	dl_Remove(&pThis->m_Link1dc);

	if (pThis->m_bIsChild != 0)
	{
		((CWorldArray04View*)&pThis->m_Collection04)->TermPointers();

		for (i = 0; i < pThis->m_nUnknown190; i++)
		{
			CWorldArrayEntry* pEntry =
				(CWorldArrayEntry*)pThis->m_pUnknown18c[i];

			if (pEntry)
			{
				if (pEntry->m_Flags & 1)
					pEntry->m_pInner->Term();

				pEntry->Term();
			}
		}

		DeleteAndClearArray2(*(CMoArray<CServerWorldRec*, DefaultCache>*)&pThis->m_Collection188,
			(LAlloc*)&g_WorldAllocator);
	}
	else
	{
		((CMoArray<CCollElem60, DefaultCache>*)&pThis->m_Collection04)->SetSize2(0,
			(LAlloc*)&g_WorldAllocator);

		TermObjects();

		pCur = pThis->m_List0E8.m_pNext;
		pHead = &pThis->m_List0E8;

		while (pCur != pHead)
		{
			LTLink*         pNext = pCur->m_pNext;
			CWorldListItem* pItem = (CWorldListItem*)pCur->m_pData;

			delete pItem;

			pCur = pNext;
		}

		dfree(pThis->m_pUnknown1b8);

		w_TermWorldData(this);

		((CWorldSubFCView*)&pThis->m_Unknown0fc)->Term();

		if (pThis->m_pUnknown1b0)
			::operator delete(pThis->m_pUnknown1b0);

		DeleteAndClearArray2(*(CMoArray<CServerWorldRec*, DefaultCache>*)&pThis->m_Collection188,
			(LAlloc*)&g_WorldAllocator);
	}

	((CWorldFinishView*)this)->TermFinish();
}


// 0x10035152 — is a load request for this file already in flight? Both of
// the thread's queues are locked and then searched by the file handle each
// message carries in its fourth payload dword.
//
// Each walk is the cursor-alias form the circular lists want, and the
// "already back at the tail" test compiles branchlessly (`sub`/`neg`/`sbb`/
// `and`), which is what says the source wrote a conditional expression and
// not an `if`.
//
// The two locks are scoped objects (below), both at function scope: that
// is retail's /GX frame and its two unwind states (Primal Hunt lithtech.exe
// 0x004982c0, server.dll 0x10044f4e), and without /GX it is AvP2 1.0.9.6's
// server.dll 0x10040602 too -- all three byte-identical. (A block of its own
// for the second lock is as good on Primal Hunt and hosts AvP2's shared
// `return LTTRUE` in the wrong place.)
// The first lock's section is named in a local first: server.dll's /O1
// homes it next to the lock's own copy (`mov %ecx,-0x14(%ebp); mov
// %ecx,-0x18(%ebp)`, 0x10044f63), four bytes of frame the bare argument
// does not have; lithtech.exe's /O2 drops it either way.
// A critical section held for a scope (name ours): the constructor enters
// and the destructor leaves, which is what gives retail's /GX frame and the
// unwind entries that leave each lock (lithtech.exe 0x004982c0; the client
// loader thread's IsRequestQueued, 0x00419490, is the same code).
class CCSLockQ551
{
public:
	CCSLockQ551(LCriticalSection* pCS)
	{
		m_pCS = pCS;
		m_pCS->Enter();
	}

	~CCSLockQ551()
	{
		m_pCS->Leave();
	}

	LCriticalSection* m_pCS;
};

LTBOOL CServerThread::IsRequestQueued(void* pFile)
{
	LCriticalSection* pCS = &m_IncomingQueue.m_CS;
	CCSLockQ551 lock1(pCS);
	CCSLockQ551 lock2(&m_OutgoingQueue.m_CS);

	if (m_pServerMgr)
	{
		LThreadMessage* pCur;
		LThreadMessage* pRec;

		pCur = m_IncomingQueue.m_MessageList.m_pTail;

		while (pCur)
		{
			pRec = pCur;
			pCur = (pCur->m_pPrev == m_IncomingQueue.m_MessageList.m_pTail) ? LTNULL : pCur->m_pPrev;

			if ((void*)pRec->m_Data[3] == pFile)
				return LTTRUE;
		}

		pCur = m_OutgoingQueue.m_MessageList.m_pTail;

		while (pCur)
		{
			pRec = pCur;
			pCur = (pCur->m_pPrev == m_OutgoingQueue.m_MessageList.m_pTail) ? LTNULL : pCur->m_pPrev;

			if ((void*)pRec->m_Data[3] == pFile)
				return LTTRUE;
		}
	}

	return LTFALSE;
}


// serverengine.cpp's loader (0x1002b22c); serverengine.h is not included here.
LTRESULT se_LoadModelData(CServerMgr* pMgr, char* pFilename, void* pFile,
                          class Model** ppModel);

// 0x100351d9 / lithtech 0x47bfa0 — slot 0x0c of the thread's vftable: one
// load request off the incoming queue. The request is the same eight-dword
// record se_LoadModel posts (payload dwords 0 and 2 zero, the file handle in
// dword 3); anything else is ignored. The model is loaded here, on the
// worker, and the answer goes back on the outgoing queue in the shape
// OnModelLoaded reads: dword 0 is the failure flag (the `neg/sbb/neg` is
// `!= LT_OK`), dword 3 the file and dword 4 the model.
void CServerThread::ProcessMessage(LThreadMessage* pMsg)
{
	LThreadMessage reply;
	Model*         pModel;

	if (m_pServerMgr && pMsg->m_Data[0] == 0 && pMsg->m_Data[2] == 0)
	{
		void* pFile     = (void*)pMsg->m_Data[3];
		char* pFilename = fs_GetFileName(&m_pServerMgr->m_FileMgr, pFile);

		LTRESULT dResult = se_LoadModelData(m_pServerMgr, pFilename, pFile,
		                                    &pModel);

		reply.m_Data[0] = (dResult != LT_OK);
		reply.m_Data[2] = 0;
		reply.m_Data[3] = (uint32)pFile;
		reply.m_Data[4] = (uint32)pModel;

		m_OutgoingQueue.PostMessage(&reply);
	}
}


// The sent-id list being refilled is g_pFillIDList (0x100587a4, defined at
// the top of this file): it grows two hundred entries at a time by hand,
// which is where the 0x190 in the allocation and the 0xc8 added to the
// capacity (ClientIDList::m_nMaxIDs, +0x02) come from.

// 0x100291c7 — write the object's changed state into the update, and
// 0x10027ad0 — whatever follows it (names by address).
// RETURNS int, not void: the shipped exit at 0x10029601 is `push $1 ; pop
// %eax`, so the answer is 1. The caller ignores it, which is why the
// declaration could sit here as void for so long without anyone noticing.
int sub_100291c7(CServerMgr* pMgr, Client* pClient, LTObject* pObject,
                  CObjInfo* pInfo, class ServerMessage* pMsg, uint32* pSize);
// 0x100250a4 — how many animation trackers the object has: the list at
// LTObject+0x1f8 walked through +0x04, counted. A thiscall with no
// arguments, which ltobject_internal.h does not declare (name ours).
class CObjTrackerCountView
{
public:
	uint32 GetTrackerCount();                // 0x100250a4 / 0x0045b430
	char*  GetFilename();                    // 0x100250b7 / 0x0045b450

#ifdef PRIMAL_HUNT
	uint8          m_pad_000[0x224];
#else
	uint8          m_pad_000[0x220];         // AvP2 1.0.9.6 (0x10031b9a)
#endif
	LTAnimTracker* m_pTrackerList;           // 0x224 (AvP2 0x220)  (0x004672a3)
};

// 0x10013585 — the full rotation, written as whatever its four floats decode
// to (it converts through 0x10015bcd and 0x1001210b first), and 0x100290d7 —
// the object's animation state, which reports by name when an object carries
// more than 0x100 trackers. Both cdecl; names by address.
void sub_10013585(class ServerMessage* pMsg, LTRotation* pRot);
void sub_100290d7(LTObject* pObject, class ServerMessage* pMsg, uint32* pSize);

// 0x100290d7 / 0x00467250 (NOLF) — the object's animation state into a
// client update. NOLF's shape (count, then per tracker the animation word,
// the elapsed byte and, past the first tracker, the weight set) is gone:
//
// AVP2: the walk both writes and MEASURES (Primal Hunt server.dll
// 0x10034528, 326 bytes; AvP2 1.0.9.6 0x10031b19): either half is optional,
// as in sm_WriteObjectFiles -- with no message nothing is written, with a
// size every field adds its width. sub_10027ad0 calls it with no message
// and a size to estimate (0x10032dd3), and again with the message and no
// size to write (0x10032e75); sub_100291c7 passes both of its own through
// (0x1003491a). New in AvP2: the object's dimensions go first, as a
// compressed vector (slot 0x5c on the message's writer, LTObject+0x1a0, nine
// bytes), and every tracker now sends seven fields, fourteen bytes:
//
//   WriteWord   the current animation with AT_ bits 1 and 0 at 0x8000/0x4000
//   WriteByte   the elapsed time as 0..255 of the animation's length
//   WriteByte   m_bAllowInterpolation (+0x24)
//   WriteDWord  m_TimeScaleNum (+0x20), then m_TimeScaleDenom (+0x1c)
//   WriteByte   the previous and current frame's weight sets (+0x34, +0x40)
//
// so NOLF's "every tracker but the first sends its weight set" index is gone.
// The trackers are walked through the link's own next pointer (+0x04).
void sub_100290d7(LTObject* pObject, ServerMessage* pMsg, uint32* pSize)
{
	LTAnimTracker* pTracker;
	ModelAnim*     pAnim;
	uint16         nCount;
	uint16         nValue;
	uint32         nPercent;

	if (pMsg)
		((IMessageWrite*)&pMsg->m_Write)->WriteCompVector(&pObject->m_Dims);

	if (pSize)
		*pSize += 9;

	nCount = (uint16)((CObjTrackerCountView*)pObject)->GetTrackerCount();

	if (nCount > 0x100)
	{
		if (g_DebugLevel >= 1)
		{
			dsi_ServerError(
				"Model (%s) with > 256 AnimTrackers, ignoring animation.",
				((CObjTrackerCountView*)pObject)->GetFilename());
		}

		if (pMsg)
			pMsg->WriteByte(0);

		if (pSize)
			(*pSize)++;

		return;
	}

	if (pMsg)
		pMsg->WriteByte((uint8)nCount);

	if (pSize)
		(*pSize)++;

	for (pTracker = ((CObjTrackerCountView*)pObject)->m_pTrackerList;
	     pTracker;
	     pTracker = pTracker->GetNext())
	{
		if (pMsg)
		{
			nValue = pTracker->m_TimeRef.m_Cur.m_iAnim;

			if (pTracker->m_Flags & 2)
				nValue |= 0x8000;

			if (pTracker->m_Flags & 1)
				nValue |= 0x4000;

			nPercent = 0;

			pAnim = pTracker->m_TimeRef.m_pModel->m_AnimArray.m_pArray
			            [pTracker->m_TimeRef.m_Cur.m_iAnim].m_pAnim;

			if (pAnim && pAnim->GetLength())
			{
				nPercent = pTracker->m_TimeRef.m_Cur.m_Time * 255
				           / pAnim->GetLength();
			}

			// The first two are calls in both images; the last five are
			// CPacket::WriteType, expanded at /O2 (lithtech.exe 0x0047ab92..
			// 0x0047aeb8, five overrun reports) and calls at /O1.
			pMsg->WriteWord(nValue);
			pMsg->WriteByte((uint8)nPercent);
			((CPacket*)pMsg)->WriteType((uint8)pTracker->m_bAllowInterpolation);
			((CPacket*)pMsg)->WriteType((uint32)pTracker->m_TimeScaleNum);
			((CPacket*)pMsg)->WriteType((uint32)pTracker->m_TimeScaleDenom);
			((CPacket*)pMsg)->WriteType((uint8)pTracker->m_TimeRef.m_Prev.m_iWeightSet);
			((CPacket*)pMsg)->WriteType((uint8)pTracker->m_TimeRef.m_Cur.m_iWeightSet);
		}

		if (pSize)
			*pSize += 14;
	}
}


// The unguaranteed half of an object's update: position, rotation and
// animation, each sent only if the object's NETFLAG_*UNGUARANTEED bit asks
// for it, and all three flagged in the high bits of the object-id word that
// introduces them.
//
// The third parameter is accepted and never read.
//
// A shipped oddity worth recording: pObject is dereferenced at 0x10027ad8 to
// reach its SObjData and only null-tested at 0x10027b2a, on the animation
// path. The test is in the source (VC6 cannot invent one) and it is dead.
//
// nAnimFlag is SIXTEEN BITS, which the /O1 image states and the /O2 image
// confirms: server.dll stores it 32 bits wide into the dead pObject slot and
// ANDs the slot 32 bits wide, then reads it back TWICE with `cmpw`
// (0x10027b23), and lithtech.exe keeps it in %ebp with a 32-bit `mov`/`and`
// and reads it twice with `test %bp,%bp` (0x00464df9, 0x00464ead). Both
// images widen the write and narrow the read, which is what VC6 does with a
// uint16 local it has given a dword home. Written as a uint32 with the mask
// as a second statement -- the shape this function carried before -- the
// narrowing disappears and the `and`'s own flags serve the first test, which
// is one instruction and four bytes short of the shipped /O2 function.
//
// The remaining /O1 residual is where the local lives: server.dll leaves it
// in memory and we put it in a register. That is allocation, not shape; the
// /O2 build already agrees with us on the register.
//
// It is a straight two-into-one tie-break, and it is worth all six bytes.
// The function has NO frame (`push ebp ; mov esp,ebp ; push esi ; push edi`
// and no `sub esp`), so the only memory home available is a dead parameter
// slot, and only one variable can have it: whichever of nSendFlags and
// nAnimFlag loses the last callee-saved register. server.dll gives ebx to
// nSendFlags (`xor %ebx,%ebx`, `mov $0x4000,%ebx`, `or $0x80,%bh`,
// `test $0x40,%bh`) and homes nAnimFlag at 0xc(%ebp), computing it in place
// as `mov %ebx,0xc(%ebp)` + `and %eax,0xc(%ebp)` and re-reading it twice
// with `cmpw`. Ours does the exact opposite. Every byte of the difference is
// that swap: +1 on the clear, +2 on the 0x4000 store, +1 on each `orb`,
// +3 at the mask, +1 on the `or` into the id word, +1 on each of the three
// `testb`s, and -2 on each of the two `cmpw`-versus-`test` reads.
//
// Delete nAnimFlag and test `nSendFlags & 0x1000` at both sites and ours
// puts nSendFlags in ebx and the whole prologue through the mask goes
// byte-identical (266 bytes) -- so the ranking is not about nSendFlags at
// all, it is that nAnimFlag, whose live range runs to the very end of the
// function, outranks it in our build. Probed and negative, all 278 with
// identical bytes: declaring nAnimFlag before nSendFlags, nSendFlags as
// uint16, computing nAnimFlag in two statements (`= nSendFlags` then
// `&= 0x1000`, which is exactly the shape of the shipped pair and which VC6
// folds back), and moving `nSize = 14` after the mask.
//
// The tie DOES flip when nSize stops being set right before the test: with
// `uint16 nSize = 14` at the declaration (or `nSize = 14` anywhere up to the
// rotation block), or with the 14 on an `else` arm, ebx goes to nSendFlags
// as shipped. Neither is the shipped shape, though: the early 14 lands in
// edx and nAnimFlag is computed in ecx and stored (271 bytes, 4 blocks); the
// else arm adds a `jmp` (273 bytes, 3 blocks). Shipped sets the 14 into ecx
// between `mov %ebx,0xc(%ebp)` and `and %eax,0xc(%ebp)`, which is the
// original statement order -- and that order gives ebx to nAnimFlag. Left
// at the original, which also keeps the lithtech.exe twin byte-identical.
// Also negative (278, 13-15 blocks): nested `if`s for the pObject test,
// `!= 0` spellings, nSize as uint32, `nSendFlags | m_ObjectID` order.
// AVP2: the estimate is 25 plus what the animation walk measures itself,
// through its size argument with no message (server.dll 0x10032db7 `movl
// $0x19,-0x4(%ebp)`, 0x10032dd3; AvP2 1.0.9.6 0x1003055f, 0x10030577), where
// NOLF had 14 + 4 per tracker + 1; the write at the end passes no size.
//
// Byte-identical in lithtech.exe (0x004781d0, 299 bytes). NEAR in server.dll
// (0x10032d69, 286 vs 280): the old ebx tie-break above, nAnimFlag winning
// ebx where shipped gives it nSendFlags and homes nAnimFlag in pObject's
// slot. With nSize declared first and `= 25` at its declaration server.dll
// gets ebx and the slots right (279 vs 280: nAnimFlag computed in %ecx and
// stored, the 25 stored at entry) but lithtech.exe loses its match (a frame
// slot for nSize, 301 vs 299), so the lithtech-identical order is kept.
// Also tried: `nAnimFlag = nSendFlags; nSize = 25; nAnimFlag &= 0x1000;`
// (286), nAnimFlag declared before nSendFlags (286).
void sub_10027ad0(CUpdateContext* pCtx, LTObject* pObject, CObjInfo* pInfo)
{
	uint16 nNetFlags = pObject->m_pSData->m_NetFlags;
	uint32 nSendFlags;
	uint16 nAnimFlag;
	uint32 nSize;                            // AVP2: its address is passed

	if (!(nNetFlags & (NETFLAG_ANIMUNGUARANTEED | NETFLAG_POSUNGUARANTEED |
	                   NETFLAG_ROTUNGUARANTEED)))
		return;

	nSendFlags = 0;

	if (nNetFlags & NETFLAG_POSUNGUARANTEED)
		nSendFlags = 0x4000;

	if (nNetFlags & NETFLAG_ROTUNGUARANTEED)
	{
		if (pObject->m_Flags & FLAG_YROTATION)
			nSendFlags |= 0x8000;
		else
			nSendFlags |= 0x2000;
	}

	if (nNetFlags & NETFLAG_ANIMUNGUARANTEED)
		nSendFlags |= 0x1000;

	nSize = 25;

	nAnimFlag = (uint16)(nSendFlags & 0x1000);

#ifdef PRIMAL_HUNT
	// PH: and the object is tested first (`test %edi,%edi` at 0x10032dc8,
	// which AvP2 1.0.9.6 does not have).
	if (nAnimFlag && pObject)
#else
	if (nAnimFlag)
#endif
		sub_100290d7(pObject, LTNULL, &nSize);

	sm_FinishUpdatePacket(pCtx, pCtx->m_Msg2.m_pMsg, 10, nSize);

	pCtx->m_Msg2.m_pMsg->WriteWord((uint16)(pObject->m_ObjectID | nSendFlags));

	if (nSendFlags & 0x4000)
	{
		lmsg_WriteCompPos(&pCtx->m_Msg2.m_pMsg->m_Write, &pObject->m_Pos,
			(CPosRef*)&pCtx->m_pMgr->m_World);

		((IMessageWrite*)&pCtx->m_Msg2.m_pMsg->m_Write)->WriteCompVector(
			&pObject->m_Velocity);
	}

	if (nSendFlags & 0x8000)
	{
		sub_10013585(pCtx->m_Msg2.m_pMsg, &pObject->m_Rotation);
	}
	else if (nSendFlags & 0x2000)
	{
		lmsg_WriteCompRotation(&pCtx->m_Msg2.m_pMsg->m_Write,
			&pObject->m_Rotation);
	}

	if (nAnimFlag)
		sub_100290d7(pObject, pCtx->m_Msg2.m_pMsg, LTNULL);
}

// 0x100279d9 (NOLF) / Primal Hunt 0x10032c86 — write one object into this
// client's update, first making sure the packet has room for it. NOLF
// estimated the size (a hundred bytes, the SFX message, twenty-five per
// attachment); AvP2 measures it exactly with the writer itself (below).
//
// The object's id also goes on the list of ids the client is being told
// about this frame, which is grown two hundred entries at a time — allocate,
// copy, free, by hand. Every one of the seven uses re-reads the global.
void sm_WriteObjectUpdate(CUpdateContext* pCtx, LTObject* pObject,
                          CObjInfo* pInfo)
{
	uint32 nSize;

	if (pInfo->m_ChangeFlags & 0x1000)
		return;

	if (g_pFillIDList->m_nIDs >= g_pFillIDList->m_nMaxIDs)
	{
		uint16* pNew = (uint16*)dalloc(
			(g_pFillIDList->m_nMaxIDs + 200) * sizeof(uint16));

		memcpy(pNew, g_pFillIDList->m_pIDs,
			g_pFillIDList->m_nIDs * sizeof(uint16));

		dfree(g_pFillIDList->m_pIDs);

		g_pFillIDList->m_pIDs = pNew;
		g_pFillIDList->m_nMaxIDs += 200;
	}

	g_pFillIDList->m_pIDs[g_pFillIDList->m_nIDs] = pObject->m_ObjectID;
	g_pFillIDList->m_nIDs++;

	// AVP2: no estimate any more -- the writer measures itself first, with
	// no message and the size (server.dll 0x10032d14..0x10032d25, the size
	// homed in pInfo's slot; AvP2 1.0.9.6 alike), then writes; and bit
	// 0x4000 of the flags (the velocity was sent) survives the reset
	// (`and $0x4000; or $0x10,%ah`, 0x10032d5a).
	nSize = 0;

	sub_100291c7(pCtx->m_pMgr, pCtx->m_pClient, pObject, pInfo,
		LTNULL, &nSize);

	sm_FinishUpdatePacket(pCtx, pCtx->m_Msg1.m_pMsg, 8, nSize);

	sub_100291c7(pCtx->m_pMgr, pCtx->m_pClient, pObject, pInfo,
		pCtx->m_Msg1.m_pMsg, LTNULL);

	sub_10027ad0(pCtx, pObject, pInfo);

	pInfo->m_ChangeFlags = (uint16)((pInfo->m_ChangeFlags & 0x4000) | 0x1000);
}

#ifndef LT_SERVER_BUILD
// lithtech.exe 0x00464a00 (926 bytes) — sub_10027918's /O2 copy. It EXPANDS
// both helpers server.dll calls: sub_10027be0's filter (0x00464a3f..
// 0x00464a9e, again for each child at 0x00464bfe) and the whole of
// sm_WriteObjectUpdate (0x00464aa4..0x00464bc6 and 0x00464c5e..0x00464d84).
// Neither has an out-of-line lithtech.exe copy (0x00464a00 is followed
// directly by sub_10027ad0 at 0x00464da0, and 0x00464ed0 sm_VisCallbackA
// expands the filter too), so both were `inline` in the original and /O1
// declined them. sub_10027be0 is (see above); sm_WriteObjectUpdateInline is
// still a stand-in twin for the other.
void sub_10027918(LTObject* pObject, CUpdateContext* pCtx)
{
	Attachment* pAttach;

	if (pObject->m_Unknown54)
		return;

	if (((IObjectUpdateGate*)pObject)->IsSkipped())
		return;

	if (pObject->m_ObjectType == OT_WORLDMODEL &&
		pObject->m_pWorldBsp->IsBusy() == 1)
		return;

	if (!sub_10027be0(pCtx->m_pMgr, pObject))
		return;

	sm_WriteObjectUpdateInline(pCtx, pObject,
		&pCtx->m_pClient->m_pObjInfos[pObject->m_ObjectID]);

	pAttach = pObject->m_pAttachments;

	while (pAttach)
	{
		LTObject* pChild = sm_FindObjectByID(pCtx->m_pMgr, pAttach->m_nChildID);

		if (pChild && sub_10027be0(pCtx->m_pMgr, pChild))
		{
			sm_WriteObjectUpdateInline(pCtx, pChild,
				&pCtx->m_pClient->m_pObjInfos[pChild->m_ObjectID]);
		}

		pAttach = pAttach->m_pNext;
	}
}
#endif // !LT_SERVER_BUILD


// 0x100270d4 — a client, freshly built. Nothing here is body code: the four
// stores are all member initialisation and the shipped order proves it, since
// the four-trip packet-slot loop is emitted BETWEEN the rate governor's
// constructor at +0x118 and the message list's tie-off at +0x3cc, which is
// where declaration order puts ClientPacketSet's own constructor and nowhere a
// statement could be scheduled to.
//
// The list is tied off inline rather than called because ltlink.h's LTList
// constructor is an in-class one-liner; the rate governor's is a call because
// its body is out of line.
//
// `inline`, and that is the /O2 image's evidence: lithtech.exe has NO
// standalone Client constructor, it expands this body into its only caller
// (0x004637aa..0x004637d9, sm_AddClientForConnection), and it sits in
// server.dll at 0x100270d4 as a COMDAT emitted right after that caller,
// beside the out-of-line CheapLTLink::AddAfter (0x100270be) -- where /O1
// leaves inline bodies it declined, not where a defined function would go.
// Out of line, our lithtech build called it (1332 bytes against 1386, and a
// stray 72-byte ??0Client row); inline it is expanded (1380), server.dll
// unchanged.
inline Client::Client()
	: m_pClientData(LTNULL)
{
}


// The sound instance's audible radius and world position, both inside the
// padding servermgr.h leaves after +0x3c (names ours, from the range test
// below).
class CSoundRangeView
{
public:
	uint8    m_pad_00[0x14];
	// +0x14 — a record whose +0x08 is the sound's id, read as a uint16 both
	// for the wire and as snd_FindSentState's hash key (names ours).
	class CSoundIDRec* m_pIDRec;             // +0x14
	uint8    m_pad_18[0x1c - 0x18];
	float    m_fStartTime;                   // +0x1c  m_Time when it began
	uint8    m_pad_20[0x34 - 0x20];
	// +0x34 — servermgr.h types this uint32; the shipped read is `mov ..,%ax`
	// and the mask that follows is 16-bit, so the field is a WORD.
	uint16   m_Flags;                        // +0x34
	uint8    m_pad_36[0x3c - 0x36];
	uint8    m_Unknown3c;                    // +0x3c
	uint8    m_pad_3d[0x40 - 0x3d];
	float    m_fRadius;                      // +0x40
	float    m_fInnerRadius;                 // +0x44  see the ratio below
	uint8    m_Unknown48;                    // +0x48
	uint8    m_pad_49[0x4c - 0x49];
	float    m_f4c;                          // +0x4c
	LTVector m_vPos;                         // +0x50
#ifdef PRIMAL_HUNT
	// PH: a float, sent scaled by ten and clamped to a byte (server.dll
	// 0x1003522a), and two more fields after it (names ours).
	float    m_f5c;                          // +0x5c
	uint8    m_Unknown60;                    // +0x60
	uint8    m_pad_61[0x64 - 0x61];
	uint32   m_Unknown64;                    // +0x64
#else
	uint32   m_Unknown5c;                    // +0x5c
#endif

	// Stand-in accessors (names ours): a float RETURNED from an inline
	// reaches its use on the x87 stack, which is how sub_100298d9 reads both
	// fields in lithtech.exe (0x0046855c, 0x0046857c).
	float    GetF4c() { return m_f4c; }
	float    GetStartTime() { return m_fStartTime; }
};

// The two little records the sound points at. The one at +0x0c has its dword
// read whole and handed to WriteWord, so it is a uint32 there and not the
// uint16 servermgr.h's `uint16* m_pUnknown0c` implies; the one at +0x14 has
// its +0x08 read as a word (names ours).
class CSoundClientRec
{
public:
	uint8  m_pad_00[0x08];
	uint32 m_nIndex;                         // +0x08
};

class CSoundIDRec
{
public:
	uint8  m_pad_00[0x08];
	uint16 m_nID;                            // +0x08
};

// One client's memory of what it was last told about one sound. Ten bytes,
// and the size is not inferred: sb_Init2(&m_Bank89C, 10, 0x20, 0x20) at
// smgr.cpp's manager constructor is where these come from. The change byte's
// three bits are the three fields below it, in order (names ours).
#define SNDSENT_FLAGS  1
#define SNDSENT_BYTE04 2
#define SNDSENT_RANGE  4

class CSoundSentState
{
public:
	uint8  m_ChangeFlags;                    // +0x00
	uint8  m_pad_01[0x02 - 0x01];
	uint16 m_Flags;                          // +0x02  the sound's flag word
	uint8  m_Unknown04;                      // +0x04  SoundInstance's +0x3c
	uint8  m_pad_05[0x06 - 0x05];
	uint16 m_nRange;                         // +0x06  min(m_fRadius, 65535)
	uint8  m_nInnerRatio;                    // +0x08  inner/outer * 255, capped
};

// 0x100288be — find this client's record for one sound, or make one. The hash
// is the client's own at +0x3dc, the key is the two-byte sound id, and a new
// record comes out of g_pServerMgr->m_Bank89C (names ours).
CSoundSentState* snd_FindSentState(Client* pClient, uint16 nSoundID);

// 0x10029b53 — merge the freshly computed state into the stored one and leave
// the change byte saying which of the three fields moved. Per field: if the
// bit is ALREADY set the value is copied unconditionally, otherwise it is
// compared and the bit set only on a difference — so a field that changed and
// was not yet sent stays marked (name ours).
void snd_MergeSentState(CSoundSentState* pStored, CSoundSentState* pNew);

// 0x10029b42 — take a hold on the instance so a killed sound keeps updating
// until the last client has heard about it: if any of PLAYSOUND 0x008 / 0x020
// / 0x400 is set and 0x004 is not, `++m_Unknown28`.
//
// THAT INCREMENT CORRECTS servermgr.h, which types m_Unknown28 `void*` and
// describes it as "keeps a killed sound updating". The description is right
// and the type is not: nothing ever stores a pointer there and this function
// increments it, so it is a hold COUNT (name ours).
//
// IN-CLASS, i.e. `inline`, and that is what gives both images their shape:
// server.dll's /Os declines the expansion and calls the out-of-line copy at
// 0x10029b42 (17 bytes, `test $0x428,%ax` -- /Os picks the short 16-bit
// test), while lithtech.exe's /Ot expands it into the sound writer
// (0x0046841f..0x0046842d, `test $0x428,%eax`). Measured in a scratch TU:
// every spelling of this test is declined at /O1 and accepted at /O2.
class CSoundHoldView
{
public:
	uint8  m_pad_00[0x28];
	uint32 m_nHolds;                         // +0x28  SoundInstance::m_Unknown28
	uint8  m_pad_2c[0x34 - 0x2c];
	uint32 m_PlaySoundFlags;                 // +0x34

	void AddHold()                           // 0x10029b42
	{
		if ((m_PlaySoundFlags & 0x428) && !(m_PlaySoundFlags & 4))
			++m_nHolds;
	}
};

// 0x10028a35 — ServerMessage's four-byte float write, the sibling of the
// dword one at 0x1000ecd6: same room test against m_MaxDataLen and m_DataSize
// and the same grow through 0x1000c537 with &g_DefAlloc, then `flds` the
// argument into the buffer (name ours).
class CMsgFloatWriteView
{
public:
	void WriteFloat(float fValue);           // 0x10028a35
	void WriteDWord(uint32 nValue);          // 0x1000ecd6  (netmgr.cpp's too)
	// 0x1000ec3d again, the word writer, but declared with a DWORD
	// parameter. The callee only reads the low word (`mov 0x8(%ebp),%dx`),
	// so the two declarations compile the same body; what differs is the
	// caller. sub_100298d9 pushes the client-record index whole,
	// `push 0x8(%eax)` at 0x10029972, and lithtech.exe loads it whole too
	// (`mov 0x8(%edx),%eax` at 0x0046840d). Through the uint16 prototype
	// VC6 narrows it to `mov 0x8(%eax),%ax ; push %eax`, whatever the
	// field's type or the cast.
	void WriteWord(uint32 nValue);
};

// 0x100298d9 / lithtech.exe 0x00468330 (1326 bytes, identical in both) —
// write one sound's state into the update (name ours).
//
// NOT RECONSTRUCTED (587 bytes). Every callee and every constant in it is
// identified below, so what is left is transcription rather than reading.
//
// Callees, all six of them:
//   0x10029b24  SoundInstance::GetObject      already in servermgr.h
//   0x10029b42  snd_AddHold                   above
//   0x10029b53  snd_MergeSentState            above
//   0x100288be  snd_FindSentState             above
//   0x10028a35  CMsgFloatWriteView::WriteFloat above
//   0x1000ecd6  ServerMessageWriteView::WriteDWord  already in netmgr.cpp
//   0x10012f79  lmsg_WriteCompPos             already in servermsg.h:217
//
// Two access paths worth writing down because they are not obvious:
//   * lmsg_WriteCompPos's first argument is pMsg + 0x08, which is
//     ServerMessage's EMBEDDED LMessageImpl -- servermsg.h already records
//     that the constructor stores LMessageImpl's vtable into its own +0x08.
//   * its CPosRef* is pMgr + 0x1a4, i.e. CServerWorld. The world is the
//     reference frame a compressed position is relative to.
//
// The body, in order:
//   * `if (!pMsg) return;` -- the message is tested before anything else.
//   * nFlags = pSound->m_PlaySoundFlags, kept as a dword in pSound's own
//     (dead) parameter slot; the 0x800 bit is then read and written through
//     the byte at 0xd(%ebp), which is that dword's second byte.
//   * if (nFlags & 0x808): resolve GetObject(). If it is the client's own
//     object (Client+0x3ac), set 0x800 in the local copy and clear bit 1 of
//     pInfo->m_ChangeFlags. Otherwise, if 0x800 was already set, re-resolve,
//     copy that object's m_Pos (LTObject+0x18c) into the sound's m_vPos, and
//     SET bit 1 of pInfo. Then nFlags = (nFlags & ~0x800) | 0x2.
//   * WriteByte(0), WriteByte(1), WriteByte(pInfo->m_ChangeFlags),
//     WriteWord(pSound->m_pUnknown0c[4])  -- the +0x08 of the 0x0c pointer,
//     which servermgr.h already calls the client-table index.
//   * if (pInfo->m_ChangeFlags & 1): snd_AddHold, then
//     WriteWord(pSound->m_pUnknown14->+0x08), then the range pair:
//         nRange = (uint16)min(pSound->m_fRadius, 65535.0f)
//         nInner = (uint8)min((pSound->m_f44 * 255.0f) / nRange, 255)
//     -- 65535.0f is 0x1004e248 and 255.0f is 0x1004d374, and the divide is
//     `fidivl` against nRange as an integer, so m_f44 is an inner radius and
//     the byte is the ratio scaled to 255.
//     Then snd_FindSentState; a null answer means "first time", which is
//     spelled as a local record with its change byte set to 7 and used
//     directly instead of merging. Otherwise fill a local record and
//     snd_MergeSentState(pStored, pLocal).
//     Then write only what the change byte says, clearing each bit as it
//     goes, and note that the VALUES come from the originals rather than
//     from the record -- the record carries the bits, not the payload.
//   * if (nFlags & 0x40): WriteByte(pSound->+0x48).
//   * if (nFlags & 0x80): WriteFloat(pSound->+0x4c).
//   * if (nFlags & 0x400): the elapsed play time,
//         nMs = (uint32)((pMgr->m_f6cc - pSound->m_f1c) * 1000.0 + 0.5)
//     -- 1000.0 at 0x1004e240 and 0.5 at 0x1004e238, both doubles, so this is
//     seconds to milliseconds with rounding. Written as a byte when it fits;
//     0xff is the escape and the full value follows as a dword.
//   * WriteDWord(pSound->+0x5c).
//   * if (pInfo->m_ChangeFlags & 2): lmsg_WriteCompPos(&pMsg's LMessageImpl,
//     &pSound->m_vPos, (CPosRef*)&pMgr->m_World).
//
// MATCH, 587 of 587 bytes (AvP2 1.0.9.6; Primal Hunt adds the PH blocks
// at the end, 673 bytes in server.dll). The last residuals were:
//   * The flag word: a uint32 FIELD read into a uint16 LOCAL. That is a
//     truncating `mov 0x34(%ebx),%ax` with a dead upper half, which is what
//     the shipped code has; sm_WriteQueuedMessageBody forced the spelling
//     first.
//   * The elapsed-time subtraction: an inline accessor read of the start time, see
//     the site.
//   * The client-record index at 0x10029972: shipped pushes the dword
//     whole. That is a WriteWord declared with a uint32 parameter; see
//     CMsgFloatWriteView above. No field type or cast does it through the
//     uint16 prototype.
//   * The radius clamp at 0x10029999: shipped does `flds C ; flds r ;
//     fcomp %st(1)` where we did `fcoms r`. The fix is the SDK's LTMIN,
//     parentheses included; see the site. Measured negatives, all without
//     the macro's parentheses: `<`/`<=`/`>`/`>=`/`!(<)` in either operand
//     order, a float local or a static (const or not) for the bound, and an
//     if-assign through a float local (591, `fcomps C`). Every one leaves
//     the compare on memory.
//
// Two facts the diff forced, and both are corrections rather than guesses:
// snd_AddHold is a THISCALL member (the shipped call is `mov %ebx,%ecx; call`
// where a free function would push), and the inner-ratio local is a uint16,
// not a uint32 -- `movzbw %al,%ax` into a word slot rather than `movzbl`.
//
// What is still unnamed is only SoundInstance's own padding: +0x14 (a pointer
// whose +0x08 is a word), +0x1c (the start time), +0x3c, +0x44 (the inner
// radius), +0x48, +0x4c, +0x5c.
void sub_100298d9(CServerMgr* pMgr, SoundInstance* pSound, CObjInfo* pInfo,
                  Client* pClient, class ServerMessage* pMsg)
{
	CSoundRangeView* pSnd = (CSoundRangeView*)pSound;
	CSoundSentState* pState;
	CSoundSentState  state;
	uint16           nFlags;
	uint32           nRange;
	uint16           nInner;
	uint32           nTmp;
	uint16           nMs;

	if (!pMsg)
		return;

	nFlags = (uint16)pSnd->m_Flags;

	if (nFlags & 0x808)
	{
		if (pSound->GetObject() == pClient->m_pUnknown3AC)
		{
			nFlags |= 0x800;
			pInfo->m_ChangeFlags &= ~2;
		}
		else
		{
			if (nFlags & 0x800)
			{
				pSnd->m_vPos = pSound->GetObject()->m_Pos;
				pInfo->m_ChangeFlags |= 2;
			}

			nFlags = (nFlags & ~0x800) | 2;
		}
	}

	pMsg->WriteByte(0);
	pMsg->WriteByte(1);
	pMsg->WriteByte((uint8)pInfo->m_ChangeFlags);
	// The dword-parameter WriteWord: see CMsgFloatWriteView (0x10029972).
	((CMsgFloatWriteView*)pMsg)->WriteWord(
		((CSoundClientRec*)pSound->m_pUnknown0c)->m_nIndex);

	if (pInfo->m_ChangeFlags & 1)
	{
		((CSoundHoldView*)pSound)->AddHold();

		pMsg->WriteWord(pSnd->m_pIDRec->m_nID);

		// The SDK's LTMIN macro, and its parentheses are what matter. At
		// /O1 `(r) < C ? r : C` loads C, then loads r and does
		// `fcomp %st(1)` (0x10029999). The same test without parentheses
		// round the left operand, in any operand order, folds r into
		// `fcoms mem`. Measured in a scratch TU.
		nRange = (uint32)LTMIN(pSnd->m_fRadius, 65535.0f);

		nTmp = (uint32)((pSnd->m_fInnerRadius * 255.0f) / (int)(uint16)nRange);

		if ((uint8)nTmp < 0xff)
			nInner = (uint8)nTmp;
		else
			nInner = 0xff;

		pState = snd_FindSentState(pClient, pSnd->m_pIDRec->m_nID);

		if (!pState)
		{
			state.m_ChangeFlags = 7;
			pState = &state;
		}
		else
		{
			state.m_Flags       = (uint16)nFlags;
			state.m_Unknown04   = pSnd->m_Unknown3c;
			state.m_nRange      = (uint16)nRange;
			state.m_nInnerRatio = (uint8)nInner;

			snd_MergeSentState(pState, &state);
		}

		pMsg->WriteByte(pState->m_ChangeFlags);

		if (pState->m_ChangeFlags & SNDSENT_FLAGS)
		{
			pState->m_ChangeFlags &= ~SNDSENT_FLAGS;
			pMsg->WriteWord((uint16)nFlags);
		}

		if (pState->m_ChangeFlags & SNDSENT_BYTE04)
		{
			pState->m_ChangeFlags &= ~SNDSENT_BYTE04;
			pMsg->WriteByte(pSnd->m_Unknown3c);
		}

		if ((nFlags & 3) && (pState->m_ChangeFlags & SNDSENT_RANGE))
		{
			pState->m_ChangeFlags &= ~SNDSENT_RANGE;
			pMsg->WriteWord((uint16)nRange);
			pMsg->WriteByte((uint8)nInner);
		}

		if (nFlags & 0x40)
			pMsg->WriteByte(pSnd->m_Unknown48);

		// Read through an inline float accessor: lithtech.exe's call
		// (0x00468562, to WriteType<float> 0x00466700) passes the float
		// through x87, `push %ecx ; mov %esi,%ecx ; flds 0x4c(%edx) ; fstps
		// (%esp)`, where a plain read at /O2 pushes it with `mov ; push` --
		// 4 bytes short. A volatile read gets the flds too, but scheduled
		// ahead of the reserve push (0x00468559); the accessor's returned
		// value lands exactly where shipped has it. server.dll's /O1 code
		// (0x10029a99) is `flds` either way and still matches.
		if (nFlags & 0x80)
			((CMsgFloatWriteView*)pMsg)->WriteFloat(pSnd->GetF4c());

		if (nFlags & 0x400)
		{
			// The start time is read through an inline accessor: shipped
			// loads it on its own (`flds 0x1c(%ebx)`, 0x10029aad) and
			// subtracts from m_Time with `fsubrs`, and lithtech.exe does the
			// same at 0x0046857c, after the pMgr load. A plain read folds it
			// into `fsubs` as a memory operand instead; a volatile read
			// gets the fsubrs but schedules the flds before the pMgr load
			// (0x00468578).
			nMs = (uint16)((pMgr->m_Time -
				pSnd->GetStartTime()) * 1000.0 + 0.5);

			// These four, and ONLY these four, are the WriteType spelling:
			// lithtech.exe calls the out-of-line instantiations at every one
			// of the eleven write sites above (0x004159f0 uint8, 0x0042fa50
			// uint16) and carries four full expansions here -- 0x004685a3
			// and 0x00468642 storing a byte with `incw`, 0x004686e7 and
			// 0x00468787 storing a dword with `add %bp` -- each with its own
			// "CPacket::WriteType" overrun RETURN_ERROR. That is the entire
			// 706-byte residual. /Os declines WriteType at every site, so
			// server.dll keeps four `call`s here exactly as it had -- only
			// the callee SYMBOL changes, and 0x1001393e is already known to
			// be WriteType<uint8> (see sm_SendWorldInfo) while 0x1000ecd6 is
			// the four-byte body netmgr.cpp reaches as WriteDWord.
			if (nMs < 0xff)
			{
				((CPacket*)pMsg)->WriteType((uint8)nMs);
			}
			else
			{
				((CPacket*)pMsg)->WriteType((uint8)0xff);
				((CPacket*)pMsg)->WriteType((uint32)nMs);
			}
		}

#ifdef PRIMAL_HUNT
		// PH: the dword at +0x5c is gone from here; two flag bits of their
		// own send a byte and a dword instead (server.dll 0x100351e2,
		// 0x100351f3).
		if (nFlags & 0x2000)
			((CPacket*)pMsg)->WriteType(pSnd->m_Unknown60);

		if (nFlags & 0x4000)
			((CPacket*)pMsg)->WriteType(pSnd->m_Unknown64);
#else
		((CPacket*)pMsg)->WriteType(pSnd->m_Unknown5c);
#endif
	}

	if (pInfo->m_ChangeFlags & 2)
	{
		lmsg_WriteCompPos((LMessageImpl*)((uint8*)pMsg + 8), &pSnd->m_vPos,
		                  (CPosRef*)&pMgr->m_World);
	}

#ifdef PRIMAL_HUNT
	// PH: change bit 0x20 sends the float at +0x5c, times ten and clamped to
	// 0..255, as a byte (server.dll 0x10035224; 10.0f at 0x100753c0, 0.0f
	// at 0x10074220, 255.0f at 0x10074400). The byte is a named local:
	// handed straight to the write, lithtech.exe's /O2 lowered the clamp to
	// immediate stores into a temporary; named, the value stays on the x87
	// stack into the ftol as shipped (0x0047c918..0x0047c952). Both images
	// byte-identical (server.dll 0x10034fc7, lithtech.exe 0x0047c310).
	if (pInfo->m_ChangeFlags & 0x20)
	{
		uint8 nVal = (uint8)LTCLAMP(pSnd->m_f5c * 10.0f, 0.0f, 255.0f);

		((CPacket*)pMsg)->WriteType(nVal);
	}
#endif
}

// 0x10028454 — write the sounds this client still has to hear about.
//
// The declared name is servermgr.h's and it is a misnomer: the list walked
// is m_List54, the sound instances, and what the loop skips is a sound the
// client cannot hear. A sound whose 0x20 bit the client's table still has
// is always written; otherwise a killed sound is skipped, a playing one is
// written only while its timer has time left, and a sound with either of
// the low two play flags has to be within
// twice its radius (the test is against radius squared times four) of either
// the object the client is looking through or the client's own view
// position.
//
// The one four-byte frame slot serves two unrelated locals — the squared
// range while the tests run, and the reallocated id array afterwards.
//
// The id written onto the fill list goes through a BLOCK-SCOPED uint32 temp,
// which is what defeats VC6's load-narrowing peephole: the shipped build
// loads the full dword (`mov 0x8(%ecx),%ecx`) and stores a word, where the
// field read inline — or through a function-scope temp — narrows the load to
// `mov 0x8(%ecx),%cx`.
void sm_WriteRemovedObjects(CUpdateContext* pCtx, ServerMessage* pMsg)
{
	LTLink* pCur;

	for (pCur = pCtx->m_pMgr->m_List54.m_Head.m_pNext;
		pCur != &pCtx->m_pMgr->m_List54.m_Head;
		pCur = pCur->m_pNext)
	{
		SoundInstance* pSound = (SoundInstance*)pCur->m_pData;

		// AVP2: the client table's 0x20 bit is tested FIRST, for every
		// sound, and a set bit writes the sound with no further test; only
		// without it do the kill, timer and range tests apply, a killed
		// sound being skipped outright (Primal Hunt 0x00479408..0x0047944b:
		// `testb $0x20; jne <write>`, then m_bKill `jne <next>`).
		if (!(pCtx->m_pClient->m_pObjInfos[
				((uint32*)pSound->m_pUnknown0c)[2]].m_ChangeFlags & 0x20))
		{
			if (pSound->m_bKill)
				continue;

			if (pSound->m_hSample)
			{
				if (pSound->m_fTimer <= 0.0f)
					continue;
			}

			if (pSound->m_PlaySoundFlags & 3)
			{
				float fRangeSqr = ((CSoundRangeView*)pSound)->m_fRadius *
					((CSoundRangeView*)pSound)->m_fRadius * 4.0f;

				// AVP2: written as the in-range test, negated: cl 9044 keeps
				// NaN apart where 8966 does not, and AvP2 1.0.9.6's server.dll
				// (0x10030f29 `test $0x5; jnp`, 0x10030f44 `test $0x5; jp`)
				// skips a sound whose distance is unordered.
				if (!(((CSoundRangeView*)pSound)->m_vPos.DistSqr(
						pCtx->m_pClient->m_pUnknown3AC->m_Pos) < fRangeSqr ||
					((CSoundRangeView*)pSound)->m_vPos.DistSqr(
						pCtx->m_pClient->m_Position) < fRangeSqr))
				{
					continue;
				}
			}
		}

		sm_FinishUpdatePacket(pCtx, pMsg, 8, 0x2d);

		{
			CObjInfo* pInfo = &pCtx->m_pClient->m_pObjInfos[
				((uint32*)pSound->m_pUnknown0c)[2]];

			if (g_pFillIDList->m_nIDs >= g_pFillIDList->m_nMaxIDs)
			{
				uint16* pNew = (uint16*)dalloc(
					(g_pFillIDList->m_nMaxIDs + 200) * sizeof(uint16));

				memcpy(pNew, g_pFillIDList->m_pIDs,
					g_pFillIDList->m_nIDs * sizeof(uint16));

				dfree(g_pFillIDList->m_pIDs);

				g_pFillIDList->m_pIDs = pNew;
				g_pFillIDList->m_nMaxIDs += 200;
			}

			// The id goes through a block-scoped uint32 of its own, and
			// that block is load-bearing: written as one expression --
			// with the (uint16) cast or without it, signed or unsigned --
			// VC6 narrows the load itself to `mov 0x8(%ecx),%cx`, where
			// the shipped code loads the whole word (`mov 0x8(%ecx),%ecx`)
			// and lets the store take the low half. A function-scope temp
			// is no good either: it has to be kept live across
			// sm_FinishUpdatePacket and costs four bytes of frame. One
			// byte, and the only difference this function had.
			{
				uint32 nID = ((uint32*)pSound->m_pUnknown0c)[2];

				g_pFillIDList->m_pIDs[g_pFillIDList->m_nIDs] =
					(uint16)nID;
			}
			g_pFillIDList->m_nIDs++;

			if (pInfo->m_ChangeFlags != 0 && !(pInfo->m_Unknown02 & 1))
			{
				sub_100298d9(pCtx->m_pMgr, pSound, pInfo, pCtx->m_pClient,
					pMsg);
			}

			pInfo->m_ChangeFlags = 0x1000;
		}
	}
}


// 0x1000f878 — transform a point by a 4x4 (the matrix's translation row is
// at +0x30 and the homogeneous divisor comes back as the return value, which
// this caller discards; hence the bare `fstp %st(0)` before the next call).
// Name ours, from the arithmetic.
float MatVMul_H(LTVector* pOut, class LTMatrix* pMat, LTVector* pV);

// 0x10012c3a — is a point inside this world's geometry? Two flag bits at
// +0x16 of the argument short-circuit it. It is worldsim.cpp's
// bsp_PointInside (worldsim.h), lithtech.exe 0x00435540 (called from
// 0x0046feaf); declared here as that file declares it.
class BSPNode;
int bsp_PointInside(BSPNode* pRoot, LTVector* pPoint);

// The world's own geometry pointer at IWorldBsp+0x9c, which worldbsp.h does
// not reach from here (name ours).
class CWorldBspInsideView
{
public:
	virtual int _v00() = 0;                  // slot 0: nonzero disqualifies

	uint8  m_pad_04[0x9c - 0x04];
	void*  m_pGeometry;                      // +0x9c
};

#ifndef LT_SERVER_BUILD
// lithtech 0x0046fd00 expands IWorldBsp::GetBoxDiagonalSqr (the box
// subtraction and MagSqr at 0x0046fd76..0x0046fddc) where server.dll calls
// 0x1002fb47; /Ob1 needs an inline body to do that, so the /O2 copy gets
// this inline twin (stand-in, name ours).
inline float GetBoxDiagonalSqrInline(IWorldBsp* pBsp)
{
	return (pBsp->m_vBoxMax - pBsp->m_vBoxMin).MagSqr();
}
#endif

// 0x1002faaa — one candidate for GetPointContainers: an object that is a
// container (type 9), is not excluded, and whose world model really does
// contain the point goes on the caller's list.
//
// void, not the `int` servermgr.h declares as sm_GatherContainersCB — every
// exit leaves eax as whatever the last thing to touch it left, and VC6 could
// not have compiled a value-returning body that falls off the end. Renamed
// for that reason.
//
// The cheap tests come first: the box diagonal against the squared distance
// before the point is transformed into the model's space at all. The
// distance is parked in the dead pObj argument slot, the object being in a
// register by then.
//
// lithtech 0x00485430 (/O2): with the box diagonal inline (above) it is
// byte-identical. The named pMat local is what puts rows 1..3 of the
// expanded MatVMul_H in shipped's x-before-z product order (without it all
// rows come out z-first, 12 instructions off). The box compare must be
// written `fDistSqr > box` (shipped fcompp order, jne).
void sm_GatherContainers(LTObject* pObj, void* pUser)
{
	PointContainerGather* pGather = (PointContainerGather*)pUser;
	LTVector              vLocal;

	if (pObj->m_Unknown54 != 0)
		return;

	if (pObj->m_ObjectType != 9)
		return;

	if (pGather->m_nElements >= pGather->m_MaxListSize)
		return;

	{
		CWorldBspInsideView** ppBsp = (CWorldBspInsideView**)&pObj->m_pWorldBsp;

		if ((*ppBsp)->_v00() != 0)
			return;

		{
			CWorldBspInsideView* pBsp = *ppBsp;
			float fDistSqr = pGather->m_Position.DistSqr(pObj->m_Pos);

#ifdef LT_SERVER_BUILD
			if (((IWorldBsp*)pBsp)->GetBoxDiagonalSqr() < fDistSqr)
				return;
#else
			if (fDistSqr > GetBoxDiagonalSqrInline((IWorldBsp*)pBsp))
				return;
#endif

			// The container's inverse transform: NOLF's +0x230, eight
			// further in with AvP2's LTObject (lithtech 0x00485555 reads
			// its first row at 0x238).
			LTMatrix* pMat = (class LTMatrix*)((uint8*)pObj + 0x238);
			MatVMul_H(&vLocal, pMat, &pGather->m_Position);

			if (bsp_PointInside((BSPNode*)pBsp->m_pGeometry, &vLocal) != 0)
				return;
		}
	}

	pGather->m_pList[pGather->m_nElements] = (HOBJECT)pObj;
	pGather->m_nElements++;
}


// FindPoliesTouchingBox's per-object callback and the BSP walk under it
// (lithtech.exe 0x00484f50 and 0x00484fa0; server.dll 0x1003bf5a and
// 0x1003bfa4; AVP2: AvP2 1.0.9.6 has both, lithtech.exe 0x0047dc70 and
// 0x0047dcc0, server.dll 0x10038de0 and 0x10038e2a). Read off the bytes:
//
//   * the callback takes world models and containers only (type 2 or 9 at
//     +0xbc, 0x00484f63/0x00484f67), skips an excluded object (+0x54), asks
//     the caller's filter if there is one, and hands the walk the tree the
//     model's interface at +0x1b8 returns from slot 1, the query and the
//     interface itself (0x00484f84..0x00484f92). Nothing is put in eax on
//     the early exits, so it is void like sm_SphereGather; the query slot it
//     is installed in is cast at the call site.
//   * a node is its polygon (+0x00), its back and front children (+0x04,
//     +0x08) and the flag pair at +0x16 that marks the tree's ends -- the
//     same record wt_FindNodeForSphere (ltobject.cpp) descends. The plane is
//     read through the polygon and its surface (0x00484fda..0x00484fe7),
//     and the sphere's centre is copied for a by-value DistTo
//     (0x00484fee..0x00485002, server.dll's three movs at 0x1003bff1).
//   * wholly in front goes front, wholly behind goes back; a node the sphere
//     straddles has its polygon tested against the box (w_IsPolyBlocking
//     with no bounds, 0x0048505e / 0x1003c03c), the handle slot 2 makes of
//     the node is added to the list once, and the back child is pushed on a
//     static stack of 1000 nodes (lithtech.exe 0x004f5fb8..0x004f6f57,
//     server.dll 0x1008d024..0x1008dfc3: the next global referenced is
//     0xfa0 bytes on in both) before the walk carries on forward.
//   * the count goes up for every new handle, the list taking it only while
//     there is room (0x004850a4..0x004850ae), so the caller learns how many
//     there were (names ours). The search is by pointer, bounded by the
//     list's room and by the count (two `lea (%ecx,%reg,4)` bounds,
//     0x1003c05d/0x1003c067); an index loop is 23 bytes short on server.dll.
//   * the three ways on are one if / else-if / else storing pNode: server.dll
//     hosts the store and the jump back once, in the back arm (0x1003c02e);
//     `continue` in each arm hosts it at the end instead.
//
// Byte-identical: both callbacks and server.dll's walk. NEAR on lithtech.exe
// (walk 316 of 319 bytes): the query and the handle swap ebx/ebp -- shipped
// keeps the list's room in %eax across the search and so moves the handle
// to %ebx (0x00485077), ours keeps the handle in %eax. Unmoved by the handle
// declared at function scope; the search as an inline helper costs
// server.dll its match.
class CPolyWalkNodeQ551
{
public:
	WorldPolyOwner*    m_pPoly;              // 0x00
	CPolyWalkNodeQ551* m_pBack;              // 0x04
	CPolyWalkNodeQ551* m_pFront;             // 0x08
	uint8              m_pad_0c[0x16 - 0x0c];
	uint16             m_Flags;              // 0x16  either low bit: no node
};

class CPolyWalkTreeQ551
{
public:
	virtual void               _v00() = 0;
	virtual CPolyWalkNodeQ551* GetRoot() = 0;                         // 0x04
	virtual HPOLY              MakeHPoly(CPolyWalkNodeQ551* pNode) = 0; // 0x08
};

int w_IsPolyBlocking(WorldPolyOwner* pOwner, LTVector* pMin, LTVector* pMax);

CPolyWalkNodeQ551* g_PolyWalkStackQ551[1000];

inline LTPlane* sm_GetPolyPlaneQ551(WorldPolyOwner* pPoly)
{
	return pPoly ? pPoly->GetPlane() : LTNULL;
}

void sm_PolyBoxWalkQ551(CPolyWalkNodeQ551* pNode, CPolyBoxQueryQ523* pQuery,
                        CPolyWalkTreeQ551* pTree);

void sm_PolyBoxGatherQ523(LTObject* pObj, void* pUser)
{
	CPolyBoxQueryQ523* pQuery = (CPolyBoxQueryQ523*)pUser;

	if (pObj->m_Unknown54 != 0)
		return;

	if (pObj->m_ObjectType != 2 && pObj->m_ObjectType != 9)
		return;

	if (pQuery->m_FilterFn && !pQuery->m_FilterFn((HOBJECT)pObj, pQuery->m_pUserData))
		return;

	{
		CPolyWalkTreeQ551* pTree = (CPolyWalkTreeQ551*)pObj->m_pWMIface;

		sm_PolyBoxWalkQ551(pTree->GetRoot(), pQuery, pTree);
	}
}

void sm_PolyBoxWalkQ551(CPolyWalkNodeQ551* pNode, CPolyBoxQueryQ523* pQuery,
                        CPolyWalkTreeQ551* pTree)
{
	CPolyWalkNodeQ551** pStack = g_PolyWalkStackQ551;

	for (;;)
	{
		if (pNode->m_Flags & 3)
		{
			if (pStack == g_PolyWalkStackQ551)
				return;

			pNode = *--pStack;
		}

		WorldPolyOwner* pPoly = pNode->m_pPoly;
		float fDist = sm_GetPolyPlaneQ551(pPoly)->DistTo(pQuery->m_vCenter);

		if (fDist > pQuery->m_fRadius)
		{
			pNode = pNode->m_pFront;
		}
		else if (fDist < -pQuery->m_fRadius)
		{
			pNode = pNode->m_pBack;
		}
		else
		{
			if (pPoly)
			{
				if (w_IsPolyBlocking(pPoly, LTNULL, LTNULL))
				{
					HPOLY  hPoly = pTree->MakeHPoly(pNode);
					HPOLY* pCur  = pQuery->m_pList;
					HPOLY* pEnd  = &pQuery->m_pList[pQuery->m_nMax];

					for (; pCur < pEnd && pCur < &pQuery->m_pList[pQuery->m_nFound]; pCur++)
					{
						if (*pCur == hPoly)
							break;
					}

					if (pCur >= &pQuery->m_pList[pQuery->m_nFound])
					{
						if (pQuery->m_pList && pQuery->m_nFound < pQuery->m_nMax)
							pQuery->m_pList[pQuery->m_nFound] = hPoly;

						pQuery->m_nFound++;
					}
				}
			}

			if (!(pNode->m_pBack->m_Flags & 3))
				*pStack++ = pNode->m_pBack;

			pNode = pNode->m_pFront;
		}
	}
}


// 0x1002f861 — one candidate from the sphere query's box walk. The box test
// the container did is generous, so this does the round one: the object's
// bounding sphere against the query sphere, which expands to
// R*2r + R*R + r*r — exactly (R + r) squared, the query having pre-doubled
// and pre-squared its own radius.
//
// void, not the `int` servermgr.h declares as sm_SphereGatherCB: nothing is
// put in eax on any path. Renamed for that reason (the query slot it is
// installed in is cast at the call site).
//
// The bare `lea 0x1a4(%esi),%eax` whose result is never used is the residue
// of a pointer local for the radius: VC6 emits it for the third read of the
// same field whether the source names it or not (ours does too).
//
// The three terms are NOT factored, which is the whole difficulty: written
// as `R * D + R * R + RSqr` with the radius read straight from the object
// three times, VC6 factors it to `(D + R) * R + RSqr` and the function comes
// out twelve bytes short. Reading it once into a local and twice through the
// pointer blocks that and gives the shipped `fld %st(1)` / `fmul %st(2)`.
// WHICH read goes where is the last three bytes: the local in the product
// and the pointer in the square gives the shipped `flds 0x1a4(%esi); lea;
// fmuls 0x10(%ebx)` (server 0x1002f8bd). The other way round, VC6 loads the
// diameter and folds the radius into the fmul, whichever operand order the
// product is written in (all four orders measured identical).
void sm_SphereGather(LTObject* pObj, void* pUser)
{
	SphereQuery* pQuery = (SphereQuery*)pUser;

	if (pObj->m_Unknown54 != 0)
		return;

	{
		// AVP2: the difference is ASSIGNED to a named vector before it is
		// measured: server.dll copies the operator's temporary into the
		// local before the MagSqr call (0x1003c0f4..0x1003c108; AvP2
		// 1.0.9.6's 0x10038f7e alike). lithtech.exe's /O2 folds the copy.
		LTVector vDiff;
		vDiff = pObj->m_Pos - *pQuery->m_pPosition;
		float  fDistSqr = vDiff.MagSqr();
		float* pRadius = &pObj->m_Radius;
		float  fRadius = *pRadius;

		if (fDistSqr < pQuery->m_Diameter * fRadius +
			*pRadius * *pRadius + pQuery->m_RadiusSqr)
		{
			ObjectLink* pLink =
				(ObjectLink*)sm_sb_Allocate(&g_pServerMgr->m_ObjectLinkBank);

			if (pLink)
			{
				pLink->m_hObject = (HOBJECT)pObj;
				pLink->m_pNext = pQuery->m_pList->m_pFirstLink;
				pQuery->m_pList->m_pFirstLink = pLink;
				pQuery->m_pList->m_nInList++;
			}
		}
	}
}


// 0x10029653 — send one message to a client, and optionally to every client
// attached to it. The packet id is stamped into the payload's first byte
// once, for all the recipients.
//
// The two counters at 0x7fc and 0x800 are only bumped for the client itself:
// 0x7fc for every attempt and 0x800 for the ones the net manager refused.
// The attached clients' sends are not counted and their results not looked
// at.
void sm_SendToClient(CServerMgr* pMgr, Client* pClient, int nPacketID,
                     ServerMessage* pMsg, int nFlag, int nPriority)
{
	pMsg->m_pData[0] = (uint8)nPacketID;

	if (pClient->m_pConnection)
	{
		if (((CNetMgrSendView*)&pMgr->m_NetMgr)->SendToConnection(pMsg,
				pClient->m_pConnection, nPriority) == 0)
		{
			pMgr->m_Unknown800++;
		}

		pMgr->m_Unknown7fc++;
	}

	if (nFlag)
	{
		LTLink* pCur = ((CClientAttachView*)pClient)->m_AttachHead.m_pNext;
		LTLink* pHead = &((CClientAttachView*)pClient)->m_AttachHead;

		while (pCur != pHead)
		{
			Client* pChild = (Client*)pCur->m_pData;

			if (pChild->m_pConnection)
			{
				((CNetMgrSendView*)&pMgr->m_NetMgr)->SendToConnection(pMsg,
					pChild->m_pConnection, nPriority);
			}

			pCur = pCur->m_pNext;
		}
	}
}


// What the global light object keeps that the sky update sends on: a colour
// at +0x10 and one more float at +0x28. It is a game-side class derived from
// BaseClass, so the offsets are all this side knows (names ours).
class CGlobalLightView
{
public:
	uint8    m_pad_00[0x10];
	LTVector m_vColor;                       // +0x10
	uint8    m_pad_1c[0x28 - 0x1c];
	float    m_fUnknown28;                   // +0x28
};

// 0x1002873d — tell one client where the global light is pointing and what
// colour it is. The direction is the forward basis vector of the object's
// rotation, which is why the rotation is turned into a matrix and the matrix
// asked for its three basis vectors even though only the last is sent.
//
// The object at CServerMgr+0x928 that servermgr.h calls CUnknown928Obj is a
// CLTServer: the two calls here go through its function-pointer members at
// +0x128 and +0x19c, which servermsg.h already names HandleToObject and
// GetObjectRotation, and the caller cleans their arguments up (they are
// plain indirect cdecl calls, not virtuals).
//
// The light handle for the FIRST call is read into its own local: shipped
// loads it into eax and pushes the register (0x10028775/0x10028787) while the
// second call pushes the member straight off memory (0x10028796). Without the
// local VC6 CSEs `&m_hGlobalLightObject` into a register, which put the
// manager in ebx, turned `lea 0x928` into an `add` and reloaded pMgr from its
// argument slot for the send (286 bytes).
//
// NEAR: 283 of 283, 101 of 101 instructions. The one residual is the
// epilogue: shipped pops edi/esi/ebx BEFORE the CMessageRef destructor's
// `test %ecx,%ecx` (0x1002884a); ours tests first, then pops. Elsewhere in
// this file shipped puts the pops first only where the destructor block is a
// join target (sm_SendWorldInfo 0x10026748); on a fall-through tail it tests
// first (sm_SendSkyInfo, sm_SendObjectInfo2), as ours does here. Tried
// without effect: the whole body flat at function scope; the send inside the
// inner block; the body in `do { } while (0)`. Also tried without effect: a
// trailing `return;`; `goto done; done: ;`; the body as `for (;;) { ...
// break; }`; the out-parameter (when there was one) at function scope; msg
// in its own inner scope; a local copy
// of pMgr used for the body and the send; a second local with an empty
// inline destructor declared after msg. Read from the bytes: ours puts the
// pops between `test` and `je`, so the destructor's test shares the send's
// block. Shipped puts them before the test, which means the test starts a
// block of its own, and nothing tried here has produced that.
//
// lithtech.exe 0x00466320 (441 bytes; ours 1040): shipped CALLS the first six
// float writes (0x004663b3..0x004663fb -> WriteType<float> 0x00466700) and
// expands only the seventh; ours expands the first five. Measured and NOT
// kept: the six through CMsgFloatWriteView::WriteFloat, each argument through
// a value-returning accessor `(&v.x)[i]` (a plain member, a `(float)` cast or
// LTVector::operator[] all push by `mov`, where shipped `flds/fstps`), gives
// 441 of 441 with one register swap left in the expanded write (edx/ecx at
// 0x00466403). It costs the file the WriteType<float> COMDAT (0x00466700,
// no out-of-line caller left), so it is a net loss while it stays a near.
//
// Measured further: the CALLS-FIRST / EXPANDS-LAST order is what VC6 does
// with one chained expression. Written as `p << vForward.x << ... <<
// m_fUnknown28` through an inline `CPacket& operator<<(CPacket&, float)`
// (stand-in, not kept), /O2 expands the outermost (LAST) sites first: the
// first three are calls and the last four expand, and the calls push through
// x87 exactly as shipped (flds 0x4c(%esp) straight off the matrix, y/z via the
// copied vector) with no accessor stand-in, and the WriteType<float> COMDAT
// stays. Statement form spends the budget forward instead (first five
// expand). Shipped has budget for ONE: ~3 more small inline sites before the
// chain reach 441 of 441 with the COMDAT kept, and what is left is the
// edx/ecx swap at 0x00466403 (allocation). Explicit instantiation
// `template void CPacket::WriteType(float);` emits nothing in VC6 (the
// `<float>` spelling is an ICE), and a defined WriteFloat wrapper is 349 bytes
// against 0x00466700's 356, so neither keeps the COMDAT another way.
void sm_Unknown2873d(CServerMgr* pMgr, Client* pClient)
{
	CMessageRef msg;
	LTVector    vForward;
	LTVector    vUp;
	LTVector    vRight;
	LTRotation  rRot;
	LTMatrix    mRot;

	msg.Init(msg_AllocPacket(0x44c, 0x44c));

	{
		HOBJECT           hLight = pMgr->m_hGlobalLightObject;
		CLTServer**       ppServer = (CLTServer**)&pMgr->m_pUnknown928;
		CGlobalLightView* pLight = (CGlobalLightView*)
			(*ppServer)->HandleToObject(hLight);

		(*ppServer)->GetObjectRotation(pMgr->m_hGlobalLightObject, &rRot);

		quat_ConvertToMatrix(&rRot, &mRot);

		mRot.GetBasisVectors(&vRight, &vUp, &vForward);

		((CPacket*)msg.m_pMsg)->WriteType(vForward.x);
		((CPacket*)msg.m_pMsg)->WriteType(vForward.y);
		((CPacket*)msg.m_pMsg)->WriteType(vForward.z);
		((CPacket*)msg.m_pMsg)->WriteType(pLight->m_vColor.x);
		((CPacket*)msg.m_pMsg)->WriteType(pLight->m_vColor.y);
		((CPacket*)msg.m_pMsg)->WriteType(pLight->m_vColor.z);
		((CPacket*)msg.m_pMsg)->WriteType(pLight->m_fUnknown28);
	}

	sm_SendToClient(pMgr, pClient, 0x1a, msg.m_pMsg, 0, 0x80);
}


// 0x10029f42 — the tail of the client-command read below; three arguments and
// the only thing established about it is that a null third one answers zero
// (name ours).
// *********************************************************************** //
//  MOVED HERE from svlast.cpp: lithtech.exe 0x00468f80 is this body at /O2
//  (ReadType and DropListener expanded), and svlast.cpp is not in
//  lithtech.exe's build. smgr.cpp used to only DECLARE it, at the top of the
//  client-command reader that calls it:
//
//      LTRESULT sm_Unknown29f42(CServerMgr* pMgr, ServerMessage* pMsg,
//                               Client* pClient);
//
//  READ THIS FIRST: the enumerated row for this function is 0x10029f4f / 103
//  bytes, and 0x10029f4f IS NOT A FUNCTION. It is the fall-through of this
//  function's own null guard. The real function starts at 0x10029f42, which
//  the enumeration lists separately as 13 bytes because the guard's early
//  `ret` at 0x10029f4e looked like an end; 0x10029f42 + 13 = 0x10029f4f, so
//  the two rows are one 116-byte function, 0x10029f42..0x10029fb6. The one
//  call site (0x1002a0c1, in the client-command reader at 0x10029fb6)
//  reaches 0x10029f42 and nothing reaches 0x10029f4f. An extent check validates a
//  row's extent and never its start, which is exactly this failure mode.
//
//  What it is: the tail of the client-command packet. Whatever ids are left
//  unread in the message are sound instances this client is no longer to
//  hear, and each one is dropped the same way sm_WriteIDList drops them —
//  which is the model to read this against, because the two share the map
//  lookup, the CObjInfo flag byte and DropListener.
//
//  Read off the bytes:
//
//    * the arguments are (pMgr, pMsg, pClient) and only the THIRD is
//      checked: `mov 0x10(%esp),%ebx ; test ; jne` before anything else, and
//      the failing arm returns zero. That is the one fact smgr.cpp's
//      existing declaration comment already records.
//    * the loop bound is `movzwl 0x34 ; movzwl 0x38 ; sub ; je` — a
//      SUBTRACTION tested against zero, not a compare, and 0x34/0x38 load in
//      that order. Written `!=` it would be a `cmp`. The caller does the
//      same subtraction the other way round and tests it `jle`, which is why
//      the two look alike and are not.
//    * the read is 0x1000ebd0 = CPacket::ReadType<uint16> — `push $0`,
//      `mov %edi,%ecx`, no cleanup. The dummy pointer is the VC6-era way of
//      naming the template argument (packet.h documents it); smgr.cpp
//      already casts ServerMessage* to CPacket* for the WriteType side.
//    * the id is a uint16 LOCAL, not the dword the read left in eax: it is
//      pushed whole to sm_FindObjectMapEntry (whose parameter is uint16
//      already) but re-zero-extended with `movzwl %si,%ecx` for the array
//      index. A uint32 local would not have the movzwl.
//    * the entry test order is `pEntry != 0`, then `m_nFlags == 2`, then the
//      record — and the record is loaded ONCE, into the register that later
//      becomes `this` for DropListener. Hence the explicit local; a third
//      `&&` arm re-reading pEntry->m_pRecord is the same bytes only if VC6
//      chooses to CSE it across the pFlags address computation, and the
//      standing rule on this codebase is that a pointer held across other
//      work is held because the source held it.
//      COIN-FLIP, both spellings exact:
//          pRecord = pEntry->m_pRecord; if (pRecord) { ... }     // used here
//          if (pEntry && pEntry->m_nFlags == 2 && pEntry->m_pRecord) { ... }
//    * `m_nFlags == 2` and not `!= 1`: `cmpb $0x2,(%eax) ; jne`. Note
//      sm_WriteIDList treats type 1 as the object case and *anything else*
//      as the sound case; this one takes only type 2, so the two are not
//      the same predicate written twice.
//    * the flag byte is CObjInfo::m_Unknown02 — `testb $0x1,0x2(%edx,%ecx,4)`
//      against a four-byte record indexed by id off Client+0x190, which is
//      m_pObjInfos, and the address is then handed to DropListener as its
//      one stack argument. NOT through a `uint8* pFlags` local, though:
//      lithtech.exe 0x0046903c tests the byte in memory (`testb $0x1,
//      (%eax)`) and re-reads it for DropListener's `or` (0x00469051), which
//      is the element written out at both uses. Through a pointer local
//      /O2 loads the byte once into %al and keeps the address in %esi.
//      server.dll is the same either way.
//    * single `xor %eax,%eax` at each of the two exits, so LTRESULT/LT_OK.
//
//  CSoundListenerView is smgr.cpp's local view class (0x100273bc). It is
//  re-declared here with only the member this file calls, which is enough
//  for the mangled name and for `this` at offset zero; when this body moves
//  to smgr.cpp the re-declaration goes away and the real one is used.
// *********************************************************************** //

LTRESULT sm_Unknown29f42(CServerMgr* pMgr, ServerMessage* pMsg,
                         Client* pClient)
{
	uint16           nID;
	CObjectMapEntry* pEntry;
	void*            pRecord;

	if (!pClient)
		return LT_OK;

	while (pMsg->m_Unknown34 - pMsg->m_Unknown38)
	{
		nID = ((CPacket*)pMsg)->ReadType((uint16*)0);

		pEntry = sm_FindObjectMapEntry(pMgr, nID);

		if (pEntry && pEntry->m_nFlags == 2)
		{
			pRecord = pEntry->m_pRecord;

			if (pRecord)
			{
				if (!(pClient->m_pObjInfos[nID].m_Unknown02 & 1))
					((CSoundListenerView*)pRecord)->DropListener(&pClient->m_pObjInfos[nID].m_Unknown02);
			}
		}
	}

	return LT_OK;
}


// 0x10029fb6 (282 bytes) — a client's command state off the wire, and
// IDENTICAL (name ours).
//
// THE ENUMERATED ROW OF 360 BYTES IS THREE FUNCTIONS, and an earlier
// version of this banner talked itself out of the right answer. The `ret` at
// 0x1002a0cf IS the end: 0x1002a0d0 and 0x1002a0f2 are two more packet
// handlers, both `__cdecl` off %esp with no frame at all, and all four of
// this handler, those two and sm_Unknown29f42 are reached only from the
// ten-byte-stride dispatch table at file offset 0x29f01 (their addresses
// sit at 0x29f01, 0x29f0b, 0x29f15 and 0x29f1f). A table reference is not
// a call, which is why the enumeration calls 0x10029fb6 a `gap` row and
// swallows its two neighbours whole. lithtech.exe says the same thing
// independently: the body there ends `ret` at 0x0046932b and 0x00469330 is
// the first of the pair again, four `nop` later. The score is taken on OUR
// length, not the row's, so 282 exact scores the row — measured, it does.
//
// The four things that were wrong, each fixed against a named address:
//
//   * THE RATE IS AN int, NOT A uint32. 0x1002a02d is `fildl` on the dead
//     pClient slot; a uint32 gives `fildll` with a zero high dword written
//     first, which is what we emitted. Reading it as `(float)ReadByte(0)`
//     — uint8 promoted to int — is the shape: the value never needs a
//     named local at all, because the converted float stays on the FPU
//     stack across both the compare and SetRate (0x1002a030 `fcoms`,
//     0x1002a037 `fsts`, 0x1002a040 `fstps`).
//   * THE COMMAND COUNT IS A uint16 TESTED `> 0`. 0x1002a056 is
//     `movzbw %al,%ax` and 0x1002a05a `test %ax,%ax / jbe` — a sixteen-bit
//     destination and an unsigned-greater test, where a uint32 and `!= 0`
//     give `movzbl` and `je`.
//   * THE LOOP COUNTER IS A SECOND, 32-BIT VARIABLE. The widening store
//     0x1002a05f..0x1002a062 (`movzwl %ax,%eax ; mov %eax,0x10(%ebp)`)
//     happens INSIDE the taken arm, after the `jbe`, and the loop then
//     decrements a dword (`decl`, 0x1002a06f). So the uint16 is copied to
//     a uint32 once the guard has passed; one variable of either width
//     gives a 16-bit `add $0xffff` / `cmpw` pair instead.
//   * THE DIFF INDEX IS A uint16 TOO: 0x1002a092 is `cmp $0xff,%ax`, a
//     sixteen-bit bound, and the change list is filled with `%al`.
//
// With those four the frame falls to 0x100 — exactly the 256-byte change
// list, no spill — the constant zero stops claiming %ebx, pLive lives in
// %ebx as it does in the shipped build, and the body is byte-identical.
// The old banner's "%ebx lottery" was a symptom of the uint32 rate: the
// 64-bit `fild` temp is what pushed the frame out and cost the register.
//
// The tail was a semantic fault as well and is fixed: the shipped `jle`
// sends the false arm to `xor %eax,%eax` while the true arm jumps PAST it
// (0x1002a0ba / 0x1002a0c9), so the call's value is returned, not LT_OK;
// and the subtraction is m_Unknown34 MINUS m_Unknown38 (0x1002a0b6
// `sub %ecx,%edx` with %edx holding +0x34), i.e. bytes REMAINING, not the
// other way round. lithtech.exe agrees at 0x00469304/0x00469309.
LTRESULT sm_ReadClientCommands(CServerMgr* pMgr, ServerMessage* pMsg,
                               Client* pClient)
{
	uint8  changed[MAX_CLIENTCOMMANDS + 1];
	uint8* pLive;
	uint8* pNext;
	float  fRate;
	uint16 n;
	uint32 nCount;
	uint16 i;
	uint32 nChanged;

	// One positive condition around the whole body: the shipped LT_OK is at
	// the very end with the null test jumping to it and the body falling in.
	if (pClient)
	{
		pLive = pClient->m_CommandStates[pClient->m_nCurCommands];
		pNext = pClient->m_CommandStates[pClient->m_nCurCommands == 0];

		memset(pNext, 0, MAX_CLIENTCOMMANDS);

		pClient->m_nCurCommands = (pClient->m_nCurCommands == 0);

#ifdef LT_SERVER_BUILD
		fRate = (float)pMsg->ReadByte(0);
#else
		fRate = (float)((CPacket*)pMsg)->ReadType((uint8*)0);
#endif

		if (fRate != pClient->m_Unknown128)
		{
			pClient->m_Unknown128 = fRate;

			pClient->m_RateControl.SetRate(fRate);
		}

#ifdef LT_SERVER_BUILD
		n = pMsg->ReadByte(0);
#else
		n = ((CPacket*)pMsg)->ReadType((uint8*)0);
#endif

		if (n > 0)
		{
			nCount = n;

			do
			{
#ifdef LT_SERVER_BUILD
				pNext[pMsg->ReadByte(0)] = 1;
#else
				pNext[((CPacket*)pMsg)->ReadType((uint8*)0)] = 1;
#endif

				nCount--;
			}
			while (nCount != 0);
		}

		nChanged = 0;

		for (i = 0; i < MAX_CLIENTCOMMANDS; i++)
		{
			if (pNext[i] != pLive[i])
			{
				changed[nChanged] = (uint8)i;

				nChanged++;
			}
		}

		pMgr->NotifyCommandChanges(pClient, changed, nChanged);

		if ((int)((uint16)pMsg->m_Unknown34 - (uint16)pMsg->m_Unknown38) > 0)
			return sm_Unknown29f42(pMgr, pMsg, pClient);
	}

	return LT_OK;
}


// *********************************************************************** //
//  THREE SMALL ROWS FROM THE lithtech.exe UNEXAMINED SWEEP
//
//  All three are lithtech.exe addresses that sit inside smgr.obj's band and
//  have no server.dll twin recorded, so they carry their /O2 address. They
//  are here because an out-of-line member definition is emitted whether or
//  not the TU calls it, which is what lets a nine-byte constructor be
//  credited before the 1,300-byte function that constructs it is written.
// *********************************************************************** //

// 0x00463ce0, 9 bytes — a one-pointer constructor, run over an array of four.
//
//      8b c1                  mov %ecx,%eax
//      c7 00 00 00 00 00      movl $0x0,(%eax)
//      c3                     ret
//
// `mov %ecx,%eax` first, no arguments, plain `ret`, `this` back in %eax:
// __thiscall constructor. Its one caller, 0x004637c8, is a peeled array
// construction that gives the element size and count outright —
//
//      0x004637bb  lea 0x134(%esi),%edi     ; &m_Array[0]
//      0x004637c1  mov $0x4,%ebx            ; four elements
//      0x004637c6  mov %edi,%ecx
//      0x004637c8  call 0x00463ce0
//      0x004637cd  add $0xc,%edi            ; sizeof(element) == 0xc
//      0x004637d0  dec %ebx ; jne 0x004637c6
//
// — so the member is a four-element array at +0x134 of the object that
// caller builds, each element 12 bytes with only its head pointer set.
class CElem463CE0
{
public:
	CElem463CE0();

	void*  m_pHead;                          // 0x00
	uint32 m_Unknown04;                      // 0x04  (untouched here)
	uint32 m_Unknown08;                      // 0x08
};

CElem463CE0::CElem463CE0()
{
	m_pHead = LTNULL;
}


// 0x004666f0, 12 bytes — release through a one-pointer holder.
//
//      8b 09         mov (%ecx),%ecx
//      85 c9         test %ecx,%ecx
//      74 05         je 0x004666fb
//      8b 01         mov (%ecx),%eax
//      ff 60 08      jmp *0x8(%eax)
//      c3            ret
//
// `mov (%ecx),%ecx` makes the holder exactly one pointer wide, and the tail
// `jmp *0x8(%eax)` is a no-argument virtual call whose result is this
// function's. What says holder rather than object is the call sites: eleven
// of them, and 0x0046569a walks an array BACKWARDS in four-byte steps
// (`sub $0x4,%esi ; mov %esi,%ecx ; call`) while 0x004787ab passes
// `lea 0x10(%esp),%ecx`, a stack temporary. Slot 2 with no arguments on a
// LithTech interface is Release.
class IRefCounted4666F0
{
public:
	virtual void _v00();
	virtual void _v04();
	virtual void Release();                  // slot 2 == +0x08
};

class CRefHolder4666F0
{
public:
	void ReleaseRef();                       // 0x004666f0

	IRefCounted4666F0* m_pRef;               // 0x00
};

void CRefHolder4666F0::ReleaseRef()
{
	if (m_pRef)
		m_pRef->Release();
}


// 0x00476770, 19 bytes — a lazy-init guard.
//
//      8b 44 24 04   mov 0x4(%esp),%eax
//      8b 48 54      mov 0x54(%eax),%ecx
//      85 c9         test %ecx,%ecx
//      75 07         jne 0x00476782
//      50            push %eax
//      e8 ..         call 0x00476660
//      59            pop %ecx
//      c3            ret
//
// cdecl, one argument. The `jne` goes FORWARD, past the only other
// statement, to the bare `ret` — so the guard is positive and its body is a
// trailing statement, i.e. the source tests the case that CONTINUES. The
// reloaded member at +0x54 is tested, not compared, so it is a pointer. The
// callee 0x00476660 is in the same module and reads both 0x188(%edx) and
// the file-scope pointer at 0x004b48c8.
struct SObj476770
{
	uint8 m_pad_00[0x54];
	void* m_pUnknown54;                      // 0x54
};

void sub_476660(SObj476770* p);              // 0x00476660

void sm_EnsureQ476770(SObj476770* p)
{
	if (!p->m_pUnknown54)
		sub_476660(p);
}


// *********************************************************************** //
//  0x10026d23 / 0x00463750 — a connection becomes a client.
//
//  A prototype servermgr.h has carried since it was written and never
//  defined: the second of the three ways a finished function hides from an
//  address scan, and the second one this sweep found. Its declared return
//  type was LTRESULT and is really Client*, which the tail settles —
//  0x00463cb1 `mov eax,esi` puts the client in %eax on the way out.
//
//  Every offset, constant and branch below is cited by the lithtech address
//  it was read at; `S:` cites the server twin where that is the clearer
//  witness. The /O2 image expands Client's constructor, memset,
//  CMessageRef::Init and the CPacket::WriteType templates exactly where /O1
//  calls 0x100270d4, 0x1003e280, 0x1000ebab and 0x1000ec3d/0x1001393e — the
//  two-image oracle working as intended.
//
//  MEASURED: a NEAR on both images — 937 bytes against 917 at /O1 and 1356
//  against 1386 at /O2, in opposite directions, which is itself informative:
//  we are over on the /O1 side and under on the /O2 side, so the residual is
//  not one missing statement.
//
//  One block was diagnosed and fixed from the diff. The attach head is tied
//  off with TWO STORES WRITTEN OUT, not through dl_TieOff: /Ob1 cannot have
//  inlined that call, because dl_TieOff is out of line everywhere in the tree
//  and both images call it at every other site. Spelling it out took the
//  mnemonic diff from 24 blocks to 19.
//
//  What is left is the allocation tax this file keeps paying: our VC6
//  materialises the branchless-select constant into %eax and ANDs with the
//  register (`mov $X,%eax ; and %eax,%ebx`) where the shipped build ANDs with
//  the immediate. Same shape, more bytes, and nothing in the source reaches
//  it.
// *********************************************************************** //

// 0x0042fbc0 / 0x1000e507 — allocate the 0x88-byte packet holder
// m_pUnknown18C points at, from a five-dword descriptor. The descriptor's
// shape is svtiny.cpp's, read off server 0x10026ea8..0x10026ebe and confirmed
// byte for byte at lithtech 0x0046393f..0x00463966 (name ours).
class CPacketHolderDesc
{
public:
	void*  (*m_pfnGet74)(void* pHolder, void* pArg);          // +0x00
	uint32 (*m_pfnDispatch)(void* pUnused, void* pObj);       // +0x04
	int    (*m_pfnAccept)(void);                              // +0x08
	void*   m_pNetMgr;                                        // +0x0c
	void*   m_pConn;                                          // +0x10
};

void* ph_CreateHolder(CPacketHolderDesc* pDesc, uint32 nFlags);  // 0x0042fbc0
// servermessage.cpp's spelling: the holder view it defines this against.
class CPacketHolderView;
void  ph_SetUnknown74(CPacketHolderView* pHolder, uint32 nValue);  // 0x1000e622

// The three slots the descriptor carries. 0x00463d10 is byte-for-byte server
// 0x1002712c, which svtiny.cpp reconstructs as cb_Sub2712C; 0x00463d20 is
// `mov eax,1 ; ret`; 0x00463cf0 is the client's forwarder. All three are
// separate rows and none is this function's work — declared so the stores
// compile.
void*  ph_Forward74(void* pHolder, void* pArg);              // 0x00463cf0

// 0x1002712c / 0x00463d10 — the descriptor's dispatch slot (moved here from
// svtiny.cpp, whose banner "6." has the derivation): __cdecl, two dwords,
// the second is `this` and slot 1 of its vtable is tail-jumped with no
// arguments. The object the descriptor's second entry dispatches on. Only
// slot 1 is reached; slot 0 exists to place it (names ours).
class CSub2712CView
{
public:
	virtual void   _v00();
	virtual uint32 _v04();
};

uint32 cb_Sub2712C(void* pUnused, CSub2712CView* pObj)
{
	return pObj->_v04();
}

// 0x00463d20 / server 0x1003a438 — the descriptor's accept hook: always yes.
// lithtech.exe keeps it as a function of its own right after cb_Sub2712C's
// twin (`mov $1,%eax ; ret`); in server.dll /OPT:ICF folded it into the one
// `push 1 ; pop eax ; ret` body worldtree.cpp's wt_DefaultNodeTest_F also
// has, which is why the server's descriptor store (0x10026eb7) points into
// the world tree's run (name ours).
int ph_AcceptAlways(void)
{
	return 1;
}

int   xfer_AddFile(CClientFiles* pState, char* pName, uint32 nUnknown,
                   uint32 nFileID, uint16 nFlags);           // 0x0042fd10
void  xfer_SendPendingFiles(CClientFiles* pState);           // 0x0042fe40
// 0x00414930 — the client model's inherit-the-server's-file step, which
// csvis.cpp defines (lithtech.exe only) on its own view of the model.
class CInheritOwnerQ414;
void  cs_InheritFileFromServerQ414(CInheritOwnerQ414* pOwner);
void  dl_TieOff(LTLink* pHead);

extern int g_LocalNoFlag;                                    // 0x004b329c

// ClientPacketSet's first member is a CMessageRef, not a bare CPacket*:
// 0x004638c3..0x004638e5 is CMessageRef::Init's body verbatim (release the
// old through vtable+8, store, AddRef the new through vtable+4), and +0x04 is
// a FLOAT (5.0f / 15.0f at 0x00463902 / 0x0046390c). Kept as a view rather
// than edited into servermgr.h so nothing else in the tree moves.
class CClientPacketSetView
{
public:
	CMessageRef m_Ref;                       // +0x00
	float       m_fRate;                     // +0x04
	uint32      m_nSize;                     // +0x08
};

// One more Client field servermgr.h leaves in padding: +0x12c, a float set to
// 35.0f (0x420c0000) at 0x004639e6. (+0x164 / +0x180 are smgr.cpp's own
// CClientAttachView, reused as-is; +0x3dc is now a real member.)
class CClientF12CView
{
public:
	uint8 m_pad_000[0x12c];
	float m_f12c;                            // +0x12c
};

// serverengine.h's CMsgWriteView and CFileRef, cut to what this function
// touches. Local copies rather than an include, so smgr.cpp's header set does
// not move; the offsets are serverengine.h's unchanged.
class CMsgWriteView
{
public:
	void WriteDWord(uint32 nValue);          // 0x1000ecd6
};

class CFileRef
{
public:
	void*  m_hElement;                       // 0x00
	uint32 m_Unknown04;                      // 0x04
	uint32 m_nFileID;                        // 0x08
	uint16 m_Flags;                          // 0x0c
};

Client* sm_AddClientForConnection(CServerMgr* pMgr, NetConnection* pConn,
                                  int bLocal)
{
	CMessageRef  msg;
	// A LOCAL POINTER, not `msg.m_pMsg` written twice, and the reload says
	// so. `msg` has its address taken (msg.Init at 0x10026d44 takes `&msg`),
	// so a 16-bit store through a pointer read out of it may alias it and
	// VC6 must re-read the pointer between the two stores -- our build did,
	// at both packet sites. The shipped build loads it ONCE
	// (S:0x10026fa5 `mov -0x4(%ebp),%eax`, then both `movw $0x1` through
	// %eax) and only re-reads for the __thiscall `this` at S:0x10026fb6,
	// which is a separate source mention of msg.m_pMsg. Worth 2 instructions
	// and 6 bytes across the two sites.
	ServerMessage* pMsg;
	Client*      pClient;
	LTLink*      pHead;
	LTLink*      pCur;
	uint32       i;
	uint32       j;
	uint16       nID;
	void*        hIter;

	// 0x00463753 `lea eax,[esp+8]`, then 0x0046375a..0x00463765: the standard
	// idiom, identical to netmgr.cpp:176 and lmessage.cpp:452. m_pMsg is
	// still the constructor's null here, so Init's release folds away and
	// only the AddRef at 0x0046377a survives.
	msg.Init(msg_AllocPacket(0x44c, 0x44c));   // 0x00463787

	// 0x0046378a `cmp DWORD PTR ds:0x4b329c,edi` with edi zero, and the store
	// at 0x00463792 goes into the bLocal PARAMETER slot. S:0x10026d61
	// `mov [ebp+0x10],ebx` says the same on a frame-pointer build, which is
	// what settles it.
	if (g_LocalNoFlag)
		bLocal = 0;

	// 0x00463797 `push 0x3e0` — sizeof(Client), which is why m_hSoundHash is
	// now a member. The constructor at 0x004637aa is expanded here and called
	// at S:0x10026d76.
	pClient = new Client;

	memset(pClient, 0, sizeof(Client));                  // 0x004637e2 rep stos

	// 0x004637ed..0x00463816. CLIENTFLAG_LOCAL is 2 and so is the manager
	// bit; the shipped code materialises the 2 once into %ecx and ors it into
	// both, which is allocation, not source.
	if (bLocal)
	{
		pClient->m_ClientFlags |= CLIENTFLAG_LOCAL;
		pMgr->m_Unknown1a0 |= 2;
	}

	pClient->m_RateControl.SetRate(10.0f);               // 0x00463827
	pClient->m_ClientFlags |= CLIENTFLAG_SENDCOBJROTATION;  // 0x00463834
	pClient->m_Unknown128 = 10.0f;                       // 0x00463837
	pClient->m_State = 1;                                // 0x00463847
	pClient->m_pName = (char*)dalloc(1);                 // 0x00463851
	pClient->m_pName[0] = 0;                             // 0x0046385c

	// 0x00463865..0x00463882. The attach LINK's m_pData only — the link is
	// not tied off here, the attach HEAD is.
	((CClientAttachView*)pClient)->m_AttachLink.m_pData = pClient;

	// 0x00463877/0x0046387a — the attach head tied off INLINE, +0x04 then
	// +0x00: CheapLTLink::TieOff's chained `m_pPrev = m_pNext = this`,
	// byte-identical to the two stores this used to spell out. (The old
	// dl_TieOff measurement here, 943 -> 945, was against the NON-inline
	// `void dl_TieOff(LTLink*)` overload declared above this function, which
	// out-ranks ltlink.h's inline CheapLTLink* one -- it never tested the
	// inline.) Written as the member because it is one more inline expansion
	// against the per-function budget described at dl_AddHead below.
	((CClientAttachView*)pClient)->m_AttachHead.TieOff();

	dl_InitList(&pClient->m_List3CC);

	// 0x00463888..0x00463921. Two nested counted loops over m_PacketSets[2][2];
	// the address arithmetic at 0x004638ac is `(j + 2*i) * 12`, i.e. row-major
	// [i][j] with the stride the declaration already gives.
	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 2; j++)
		{
			CClientPacketSetView* pSet =
				(CClientPacketSetView*)&pClient->m_PacketSets[i][j];

			pSet->m_Ref.Init(msg_AllocPacket(0x44c, 0x44c));   // 0x004638f0

			// 0x0046388e `dec ; neg ; sbb ; and 0x80 ; add 0x80` — VC6's
			// branchless select, hoisted out of the inner loop by /Og in BOTH
			// images (S:0x10026e0f, after j is zeroed, which is what says the
			// source wrote it inside).
			//
			// THE FALSE ARM IS ZERO, not 0x100, and the constants say so.
			// VC6's branchless select is `(r & (C_false - C_true)) + C_true`
			// with r = 0 when the test holds and -1 otherwise, so the pair of
			// literals in the instruction stream IS the pair in the source.
			// S:0x10026e14 is `and $0xffffff80,%ebx ; add $0x80,%ebx` --
			// C_true = 0x80, C_false = 0x80 + (-0x80) = 0 -- and lithtech's
			// 0x00463893 `and $0x80,%al ; add $0x80,%eax` is the same pair
			// with the upper 24 bits of the -1 left standing. 0x100 gave
			// `mov $0x80,%eax ; and %eax,%ebx ; add %eax,%ebx`, one
			// instruction over, because C_false - C_true was then +0x80.
			pSet->m_nSize = (i == 1) ? 0x80 : 0;         // stored 0x004638f9

			// 5.0f at .rdata 0x004a1ffc, 15.0f at 0x004a2ed0; the `jne` takes
			// the 15.0f arm, so zero falls through to 5.0f.
			pSet->m_fRate = (j == 0) ? 5.0f : 15.0f;     // stored 0x0046390c
		}
	}

	// 0x00463927..0x0046397c. The shipped store order is [0][1][3][2][4] here
	// and [3][0][1][2][4] in server.dll, so it is scheduling, not source, and
	// field order is used.
	{
		CPacketHolderDesc desc;
		// AN IF, NOT A TERNARY, and before the desc stores. `bLocal ? 4 : 0`
		// gives VC6's branchless select -- `mov 0x10(%ebp),%eax ; neg ;
		// sbb ; and $0x4` -- where S:0x10026e8e is `xor %eax,%eax ;
		// cmp %eax,0x10(%ebp) ; je ; push $0x4 ; pop %eax`: a zero default
		// in a register with a branch over the assignment, which is the
		// if/else shape (a ternary over two constants gives
		// branchless setne/add; an if/else branches). Writing it after the
		// desc stores left four of the five stores ahead of the branch;
		// writing it first puts all five behind the join, as shipped.
		uint32 nHolderFlags = 0;

		if (bLocal)
			nHolderFlags = 4;

		desc.m_pfnGet74    = ph_Forward74;               // 0x0046393f
		desc.m_pfnDispatch =
			(uint32 (*)(void*, void*))cb_Sub2712C;       // 0x0046394d
		desc.m_pfnAccept   = ph_AcceptAlways;            // 0x0046395e
		desc.m_pNetMgr     = &pMgr->m_NetMgr;            // 0x00463955
		desc.m_pConn       = pConn;                      // 0x00463966

		pClient->m_pUnknown18C =
			ph_CreateHolder(&desc, nHolderFlags);        // 0x0046396a
	}

	ph_SetUnknown74((CPacketHolderView*)pClient->m_pUnknown18C,
	                (uint32)pMgr);                           // 0x00463977

	// 0x00463985 `mov [esi+8],esi`, 0x00463989 `call 0x00463cc0`
	// (CheapLTLink::AddAfter, out of line in both images), 0x00463994
	// `inc [ebx+0x7e4]` — dl_AddAfter's three statements in order with the
	// head as the anchor, i.e. dl_AddHead.
	//
	// THE ONE RESIDUAL LEFT IN THIS FUNCTION, and it is this call: we expand
	// AddAfter's six stores inline, both shipped images CALL it. That costs
	// 3 instructions and 2 bytes and is the whole of the remaining gap.
	//
	// CAUSE, MEASURED: VC6 has a PER-FUNCTION INLINE-EXPANSION BUDGET. A
	// scratch TU with N `g(i); dl_AddHead(...)` pairs expands every AddAfter
	// up to N=9 at /O1; at N=10 the first two are CALLED, at N=14 the first
	// nine, and the out-of-line COMDAT appears. Here, fifteen calls to an
	// empty `inline void f() {}` placed right after this line make VC6 call
	// AddAfter and nothing else: 925/288 -> 923/285 SIZE-EXACT, +1 matched
	// (the AddAfter COMDAT itself claims 0x100270be), leaving only a
	// temporary/pSet frame-slot swap and a 2-insn xor/lea order at 0x10026f38.
	// (14 is not enough, 15..34 all give the same.) In lithtech.exe the
	// same probe (13+) also makes LTList::LTList() inside the expanded
	// Client constructor a call, exactly as shipped 0x004637d9, and claims
	// two more rows there. So the shipped source spent roughly fifteen more
	// inline expansions in this function than ours, near or after this
	// point -- probably small inline helpers/accessors the original used
	// where we spell out member stores. WHICH ones is not recovered; nops at
	// the TOP of the function do nothing (the earliest sites are refused
	// first), and an inline id-search helper for the loop below, the dword
	// write through WriteType<uint32>, and real LTLink members at Client
	// +0x164/+0x180 each move nothing on server.dll.
	//
	// What has been RULED OUT, so nobody re-derives it:
	//  * it is not a whole-image setting. The COMDAT at 0x100270be has
	//    exactly ONE caller in server.dll and 0x00463cc0 exactly one in
	//    lithtech.exe, and both are this site; every other dl_Insert /
	//    dl_AddHead site in either image has AddAfter inlined, and the
	//    functions containing them are byte-identical matches, so ltlink.h's
	//    `inline` and its definition position are both fine.
	//  * (an earlier note here said "not an inline budget" because eight
	//    dl_AddHeads and forty dl_InitLists AHEAD of this line all
	//    expanded. That was the wrong side: the budget refuses the earliest
	//    sites first, so extra expansions must come after this one.)
	//  * it is not inline DEPTH. Five hand-written inline wrappers on top of
	//    the four-deep dl_AddHead -> dl_AddAfter -> dl_Insert -> AddAfter
	//    chain still expanded.
	//  * it is not the spelling. dl_AddAfter with the head passed
	//    explicitly, m_pData + dl_Insert + ++m_nElements, and m_pData +
	//    m_Head.AddAfter + ++m_nElements all give the same inline expansion
	//    (922/286 against 923/285, and all three LOSE the `lea
	//    0x8b8(%edi),%ebx` CSE that dl_AddHead reproduces).
	//  * it is not the compiler revision. CL_REV=8168 builds this TU to the
	//    same 271 matched and the same bytes here.
	dl_AddHead(&pMgr->m_ClientList, &pClient->m_Link, pClient);

	// 0x0046399b..0x004639d2. The size expression is WRITTEN TWICE: the member
	// is reloaded at 0x004639b4 rather than kept, which is the fingerprint of
	// two separate source expressions, and server.dll reloads it too
	// (S:0x10026f06). sizeof(CObjInfo) is 4.
	pClient->m_pObjInfos =
		(CObjInfo*)dalloc(pMgr->m_Unknown8b8 * sizeof(CObjInfo));

	memset(pClient->m_pObjInfos, 0, pMgr->m_Unknown8b8 * sizeof(CObjInfo));

	pClient->m_pConnection = pConn;                      // 0x004639da
	pClient->m_pUnknown3AC = LTNULL;                     // 0x004639e0
	((CClientF12CView*)pClient)->m_f12c = 35.0f;         // 0x004639e6
	pClient->m_ClientID    = 0;                          // 0x004639f0

	// 0x004639f7..0x00463a19 — the lowest id nobody is using. The walk is
	// FORWARD (`[edx+4]` is CheapLTLink::m_pNext; ltlink.h puts m_pPrev
	// first), and the outer bound is 30000.
	pHead = &pMgr->m_ClientList.m_Head;

	for (nID = 0; nID < 30000; nID++)
	{
		// THE MATCH EXITS THE INNER LOOP BY GOTO AND THE WALK FALLING OFF
		// THE END IS THE `break`, not the other way round. Written as
		// `for (...) { if (match) break; } if (pCur == pHead) break;` VC6
		// INVERTS the inner loop -- test at the bottom, entry test
		// duplicated, four instructions and nine bytes over -- and while,
		// do/while and for(;;) with the head test spelled inside all give
		// the same inversion (the shape of
		// CNetMgr::GetPacket). With the natural exit carrying the outer
		// `break` the loop is not inverted: S:0x10026f40..0x10026f64 keeps
		// the test at the top and jumps back to it with `jmp 0x10026f43`,
		// and so do we. Worth 4 instructions and 9 bytes.
		for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
		{
			if (((Client*)pCur->m_pData)->m_ClientID == nID)
				goto taken;
		}

		break;

	taken:
		;
	}

	pClient->m_ClientID = nID;                           // 0x00463a22

	// 0x00463a29 — 500 slots, key kind 0.
	pClient->m_hSoundHash = dhash_Create(500, 0);

	// 0x00463a45, the literal at 0x004acff4. The id is RE-READ out of the
	// client (0x00463a3d) rather than reusing nID, which is what says the
	// source wrote the member expression.
	dsi_ServerError("New client, id %d, for a total of %d.",
		pClient->m_ClientID, pMgr->m_ClientList.m_nElements);

	// 0x00463a4f `mov ecx,[ebx+0x820]` — CClassMgr::m_pServerShell — then
	// vtable slot 2, which iservershell.h makes OnAddClient (slot 0 is the
	// virtual destructor, slot 1 ServerAppMessageFn).
	pMgr->m_ClassMgr.m_pServerShell->OnAddClient((HCLIENT)pClient);

	// 0x00463a5e..0x00463a80. Packet id 4, four-byte payload 6. The dword
	// write is a CALL in BOTH images, so it is spelled through the out-of-line
	// CMsgWriteView rather than the template.
	pMsg = msg.m_pMsg;
	pMsg->m_Unknown38 = 1;
	pMsg->m_Unknown34 = 1;

	((CMsgWriteView*)msg.m_pMsg)->WriteDWord(6);

	sm_SendToClient(pMgr, pClient, 4, msg.m_pMsg, 0, 0x80);

	// 0x00463a85..0x00463be3. Packet id 0xc carries the client's own id and
	// the local flag. BOTH are the packet.h TEMPLATES — lithtech expands them
	// here (the grow loop at 0x00463aaf / 0x00463b59 and the "ERROR: Packet
	// write overrun!" tail at 0x00463aff / 0x00463ba0 are WriteType's body
	// verbatim) and server.dll calls 0x1000ec3d / 0x1001393e.
	pMsg = msg.m_pMsg;
	pMsg->m_Unknown38 = 1;
	pMsg->m_Unknown34 = 1;

	((CPacket*)msg.m_pMsg)->WriteType(pClient->m_ClientID);
	((CPacket*)msg.m_pMsg)->WriteType((uint8)bLocal);

	sm_SendToClient(pMgr, pClient, 0xc, msg.m_pMsg, 0, 0x80);

	// 0x00463be8..0x00463c41 — every file the manager knows about goes into
	// the new client's queue with bit 1 forced on.
	hIter = dhash_GetFirst(pMgr->m_FileMgr.m_hFileHash);

	while (hIter)
	{
		CFileRef* pRef = (CFileRef*)dhash_GetData(dhash_GetNext(&hIter));
		// THE KEY IS A STATEMENT OF ITS OWN. Nested inside the argument list
		// VC6 pushes arguments 5..3, then evaluates the nested call, then
		// pushes its result -- and since %ecx is live across it (it holds
		// the flags byte) the pending cleanups come out as four `pop %ecx`.
		// The shipped build calls dhash_GetKey FIRST (S:0x10027034
		// `push $0x0 ; push (%ebx) ; call`) and only then reads m_Flags,
		// which is what right-to-left evaluation of a call whose second
		// argument is already a value looks like; it also frees %ebx to
		// hold pRef and batches all four calls' cleanup into one
		// `add $0x24,%esp`. Worth 4 instructions, and it drags the whole
		// tail of the function into agreement.
		char*     pKey = dhash_GetKey(pRef->m_hElement, LTNULL);

		xfer_AddFile((CClientFiles*)pClient->m_pUnknown18C, pKey,
			pRef->m_Unknown04, pRef->m_nFileID, (uint16)(pRef->m_Flags | 2));
	}

	xfer_SendPendingFiles((CClientFiles*)pClient->m_pUnknown18C);   // 0x00463c4f

	// 0x00463c54 `test al,2` on m_ClientFlags, then 0x00463c61..0x00463c97,
	// the model hash one element at a time.
	//
	// THE LT_SERVER_BUILD SEAM. server.dll's copy (S:0x10027088..0x1002709d)
	// is this loop with the body EMPTY — dhash_GetData's result is computed
	// and dropped. lithtech.exe hands each element to the client shell.
	if (pClient->m_ClientFlags & CLIENTFLAG_LOCAL)
	{
		hIter = dhash_GetFirst(pMgr->m_hModelHash);

		while (hIter)
		{
			void* pModel = dhash_GetData(dhash_GetNext(&hIter));

#ifndef LT_SERVER_BUILD
			cs_InheritFileFromServerQ414((CInheritOwnerQ414*)pModel);  // 0x00463c89
#else
			(void)pModel;
#endif
		}
	}

	sm_SetClientState(pMgr, pClient, 1);                 // 0x00463c9c

	return pClient;                                      // 0x00463cb1
}


// *********************************************************************** //
//  0x10029bdf / 0x00468900 — the queued sound write itself; 0x10029bc4 is the
//  gate in front of it.
//
//  The third declared-never-defined prototype this sweep turned up, and the
//  one that corrects its own declaration. servermgr.h gave the first argument
//  as CQueuedMsg*; it is SoundRecord*. All eleven PlaySoundInfo members fall
//  out of the offsets with nothing left over, and each is gated by the exact
//  PLAYSOUND_ bit ltbasedefs.h says gates it.
//
//  THE TENSION, STATED RATHER THAN HIDDEN: the gate above passes what it
//  calls a CQueuedMsg, and the gate is a 27-byte MATCH. Both readings survive
//  that, because the gate only tests +0x00 and the pointer type never reaches
//  its codegen — so the cast at the call site is where the two spellings meet,
//  and the offsets decide. If it turns out the queued list really does
//  hold CQueuedMsg, this signature is what to revisit.
//
//  MEASURED: 543 bytes against 545, and 174 instructions against 174 --
//  instruction-exact. It got there from 597 on ONE finding: there is no delta
//  vector in this function. It was first written as `vDelta = a - b;` then
//  `vDelta.MagSqr()`, which made our build copy twelve bytes into a temporary
//  -- three `movsl` with no counterpart in the shipped code. The shipped build
//  takes the ADDRESS instead and calls _CVector::DistSqr (0x1000f8f8), which
//  is how commonphysics.cpp, objbank.cpp and sub_100298d9 in this same file
//  all already spell the same test. Worth 54 bytes.
//
//  NOW 545 of 545, size-exact. The clamp is closed: LTMIN gives the shipped
//  `flds C ; flds r ; fcomp %st(1)` at 0x10029c9f, the same fix as in
//  sub_100298d9. What is left is one frame-slot swap. Shipped homes nSpatial
//  in pRec's dead argument slot +8, and because the variable lives in
//  memory the `& 3` and the test are done there: `mov %ebx,8(%ebp) ; andl
//  $3,8(%ebp) ; cmp %di,8(%ebp)` at 0x10029c36. That is one instruction
//  fewer, 174 against our 175. fRadius, the fidivl temporary and nInner
//  share -4 in shipped. Ours gives +8 to that group and -4 to nSpatial,
//  which it computes in eax. All of these left the slots where they are:
//  nSpatial declared first or last; fRadius at function scope, declared
//  first or last, or with an initialiser; `nSpatial = nFlags; nSpatial &=
//  3;`; the assignment inside the if; nSpatial as short; bOwnObject = 0
//  before nFlags; nInner as a ternary or through a uint32 temporary; an
//  (int) divisor; nRange as uint16. Worse: fRadius assigned before the if
//  (13 blocks); nInner as uint32 (costs another function in the file); a
//  uint32 nSpatial (538/543, the 16-bit compare disappears). The earlier
//  negatives for the clamp all predate LTMIN.
//
//  A NOTE THAT PAYS ELSEWHERE. The unsolved shape recorded at sub_100298d9
//  below — "the shipped load is `mov 0x34(%ebx),%ax`, sixteen bits into a
//  register whose top half is then STALE ... a uint16 field read into a
//  uint32 local gives movzwl; reading the uint32 field whole gives a dword
//  load; neither reproduces a narrowed load with a dead upper half" — has an
//  option that was not on that list and is the one the offsets here force: a
//  uint32 FIELD read into a uint16 LOCAL. That is a truncating load, no
//  zero-extension, upper half dead, and every later use here is sixteen bits
//  or narrower. server.dll agrees: 0x10029c3d is `cmp di,[ebp+8]`, a
//  sixteen-bit compare on the slot. Spelled that way below; it is the first
//  thing to try on sub_100298d9's remaining bytes.
// *********************************************************************** //

void sm_WriteQueuedMessageBody(SoundRecord* pRec, Client* pClient,
                               ServerMessage* pMsg)
{
	CSoundSentState* pState;
	CSoundSentState  state;
	uint16           nFlags;
	uint16           nSpatial;
	int              bOwnObject;
	uint32           nRange;
	uint16           nInner;

	// 0x0046890a / 0x00468916 / 0x00468921.  Three guards, and all three
	// jump to the SAME tail (0x00468e1a) in both images while the body's own
	// normal exit at 0x00468de5 is a SEPARATE epilogue -- which is exactly
	// what /O2 does with early returns whose landing site is the end of the
	// function.  The argument the guards test in the middle is the message,
	// not the client.
	if (!pRec)
		return;

	if (!pMsg)
		return;

	if (!pRec->m_hFile)
		return;

	// 0x00468929 `mov ax,[ebp+8]` -- see the note above; the local is uint16
	// and the field is the SDK's uint32.
	nFlags     = (uint16)pRec->m_Info.m_dwFlags;
	bOwnObject = 0;                                  // 0x00468938

	// 0x00468931 `test ah,8` = PLAYSOUND_CLIENTLOCAL.
	if (nFlags & PLAYSOUND_CLIENTLOCAL)
	{
		// 0x00468942 -- the object the sound is glued to against the
		// client's own object.
		if (pRec->m_Info.m_hObject == (HOBJECT)pClient->m_pUnknown3AC)
		{
			bOwnObject = 1;                          // 0x00468952
		}
		else
		{
			// 0x0046895c / 0x00468961 -- drop CLIENTLOCAL, force 3D.
			nFlags = (nFlags & ~PLAYSOUND_CLIENTLOCAL) | PLAYSOUND_3D;
		}
	}

	// 0x00468967 `and eax,3 ; test ax,ax` -- a SIXTEEN-BIT test, which is
	// what makes this a uint16 local of its own rather than a repeated
	// expression.
	nSpatial = (uint16)(nFlags & (PLAYSOUND_AMBIENT | PLAYSOUND_3D));

	if (nSpatial)
	{
		float fRadius;

		// 0x004689d0 `mov ecx,[ebp+0x120]` -- an INTEGER move of a float
		// member into a slot that is then read twice by the x87, which is
		// the fingerprint of a float LOCAL rather than two member reads.
		fRadius = pRec->m_Info.m_fOuterRadius;

		// NO DELTA VECTOR. This was first written as
		// `vDelta = a - b; ... vDelta.MagSqr()`, which made our build copy
		// twelve bytes into a temporary -- three `movsl` that have no
		// counterpart in the shipped code. The shipped build takes the
		// ADDRESS instead: 0x10029c52..0x10029c61 loads m_pUnknown3AC, adds
		// 0x18c to reach m_Pos, pushes that pointer and calls
		// _CVector::DistSqr (0x1000f8f8, __thiscall, the other vector by
		// reference). commonphysics.cpp and objbank.cpp already spell it
		// this way, and sub_100298d9 in this very file does the same test
		// against the same 4.0f.
		//
		// The radius is squared AFTER the call -- `fld [ebp-4]`,
		// `fmul [ebp-4]`, `fmul 4.0` at 0x10029c66..0x10029c6c -- so the
		// distance is the left operand of the compare.
		if (pRec->m_Info.m_vPosition.DistSqr(
				((LTObject*)pClient->m_pUnknown3AC)->m_Pos) >
			fRadius * fRadius * 4.0f)
		{
			return;
		}
	}

	// 0x00468a0c / 0x00468a15 / 0x00468a1e.  Two zero bytes then the file's
	// id; the id is the uint16 at m_hFile+0x08, which serverengine.h:229
	// calls CFileRef::m_nFileID.
	pMsg->WriteByte(0);
	pMsg->WriteByte(0);
	pMsg->WriteWord((uint16)((CFileRef*)pRec->m_hFile)->m_nFileID);

	// 0x00468a2d..0x00468a88.  65535.0f is .rdata 0x004a2ef0 and 255.0f is
	// 0x004a1a5c; the divide is `fidivl` against the range NARROWED to
	// sixteen bits (0x00468a64 `and ecx,0xffff`), which is what says the
	// range is stored uint16 before it is used as the divisor.
	// LTMIN, as in sub_100298d9. The parenthesised left operand in the
	// macro is what gives `flds C ; flds r ; fcomp %st(1)` at 0x10029c9f
	// rather than `fcoms r`.
	nRange = (uint32)LTMIN(pRec->m_Info.m_fOuterRadius, 65535.0f);

	nInner = (uint16)((pRec->m_Info.m_fInnerRadius * 255.0f) /
		(float)(uint16)nRange);

	if (nInner >= 0xff)                              // 0x00468a77
		nInner = 0xff;

	// 0x00468a89 -- keyed by the same file id the wire got.
	pState = snd_FindSentState(pClient,
		(uint16)((CFileRef*)pRec->m_hFile)->m_nFileID);

	// 0x00468a9c.  A null answer is "first time": a local record whose change
	// byte says all three fields moved, used in place of the stored one.
	// Same spelling sub_100298d9 already uses.
	if (!pState)
	{
		state.m_ChangeFlags = 7;                     // 0x00468aa0
		pState = &state;                             // 0x00468aa5
	}
	else
	{
		state.m_Flags       = nFlags;                // 0x00468aba
		state.m_Unknown04   = pRec->m_Info.m_nPriority;   // 0x00468ac3
		state.m_nRange      = (uint16)nRange;        // 0x00468ac9
		state.m_nInnerRatio = (uint8)nInner;         // 0x00468ace

		snd_MergeSentState(pState, &state);          // 0x00468ad2
	}

	// 0x00468ada -- the change byte itself goes out first, then each field
	// the byte still claims, CLEARING the bit as it goes.  The values come
	// from the originals, not from the record.
	pMsg->WriteByte(pState->m_ChangeFlags);

	if (pState->m_ChangeFlags & SNDSENT_FLAGS)       // 0x00468ae6, bit 1
	{
		pState->m_ChangeFlags &= ~SNDSENT_FLAGS;
		pMsg->WriteWord(nFlags);                     // 0x00468af5
	}

	if (pState->m_ChangeFlags & SNDSENT_BYTE04)      // 0x00468afc, bit 2
	{
		pState->m_ChangeFlags &= ~SNDSENT_BYTE04;
		pMsg->WriteByte(pRec->m_Info.m_nPriority);   // 0x00468b0d
	}

	// 0x00468b12 `cmpw $0,[esp+0x14]` -- nSpatial again, tested BEFORE the
	// change bit, so the range pair only travels for a positioned sound.
	if (nSpatial && (pState->m_ChangeFlags & SNDSENT_RANGE))   // 0x00468b1c
	{
		pState->m_ChangeFlags &= ~SNDSENT_RANGE;

		pMsg->WriteWord((uint16)nRange);             // 0x00468b27, out of line
		((CPacket*)pMsg)->WriteType((uint8)nInner);  // 0x00468b2c, EXPANDED
	}

	if (nFlags & PLAYSOUND_CTRL_VOL)                 // 0x00468b97, 0x40
		((CPacket*)pMsg)->WriteType(pRec->m_Info.m_nVolume);      // 0x00468ba2

	if (nFlags & PLAYSOUND_CTRL_PITCH)               // 0x00468c8f, 0x80
		((CPacket*)pMsg)->WriteType(pRec->m_Info.m_fPitchShift);  // 0x00468c97

	// 0x00468d42.  A sound on the client's own object does not need a
	// position; a positioned one does, compressed against the world.
	if (!bOwnObject && nSpatial)
	{
		lmsg_WriteCompPos(&pMsg->m_Write, &pRec->m_Info.m_vPosition,
			(CPosRef*)&g_pServerMgr->m_World);       // 0x00468d6a
	}

#ifdef PRIMAL_HUNT
	// PH: the sound type and the user data each travel only under the flag
	// the SDK gives them (server.dll 0x10035532 `test $0x20,%bh`, a byte
	// from +0x13c; 0x10035545 `test $0x40,%bh`, a dword from +0x140).
	if (nFlags & PLAYSOUND_CTRL_TYPE)
		((CPacket*)pMsg)->WriteType(pRec->m_Info.m_nUserSoundType);

	if (nFlags & PLAYSOUND_USER_DATA)
		((CPacket*)pMsg)->WriteType(pRec->m_Info.m_UserData);
#else
	// 0x00468d72 -- the last field, expanded, and its overrun tail at
	// 0x00468de6 is the block the enumerator mistook for a second function.
	((CPacket*)pMsg)->WriteType(pRec->m_Info.m_UserData);
#endif
}


// *********************************************************************** //
//  0x100291c7 — write one object's changed state into the update.
//
//  MOVED HERE from src/engine/lithtech/ccobj.cpp, which is a lithtech-only
//  translation unit -- so server.dll has never compiled this function at
//  all, and smgr.obj carried it as an undefined external while the
//  declaration above and sm_WriteObjectUpdate's call sat here waiting for
//  it. The /O1 side has no shape question: across
//  0x100291c7..0x10029609 every writer is an out-of-line call, which is
//  exactly what the body below spells.
//
//  The views come with it. They are the .cpp-local classes ccobj.cpp
//  gathered because a lithtech TU cannot reach three different server
//  .cpp-local ones; re-suffixed Q291 here so nothing collides with this
//  file's own. The one exception is sub_100290d7, which this file now
//  defines for real, so the local declaration of it is dropped.
//
//  THE ENUMERATED /O2 ROW IS WRONG IN LENGTH, per the note this block
//  carried: 0x00467540 is reported as 174 bytes, but its last instruction
//  branches to 0x004675ee, the next enumerated row. The two are one
//  function, 0x00467540..0x00467eec, three rets and no branch target
//  outside it -- the early `return 0` at 0x004675ed is a full epilogue, so
//  the byte after it looks like a start.
// *********************************************************************** //

// The six free writers under their real names (servercommon.cpp and
// servermsg.cpp define them; same signatures, so the calls are unchanged).
// lithtech.exe reaches the same definitions: 0x004670a0, 0x004358d0,
// 0x00435960, 0x00435ad0, 0x00435d00, 0x00435d60 (the call sequence over
// 0x00467540..0x00467eec).
void sm_WriteObjectFiles(LTObject* pObj, ServerMessage* pMsg, uint32* pnSize);  // 0x1003448c (servercommon.cpp)
void lmsg_CompressPos(uint8* pOut, LTVector* pVec, CPosRef* pRef);  // 0x10012ec5
void lmsg_WriteCompPosWords(IMessageWrite* pMsg, uint8* pData);     // 0x10012f49
void sub_1001307f(LTRotation* pRot, uint8* pOut);                   // 0x1001307f
void lmsg_WriteCompRotationBytes(IMessageWrite* pMsg, uint8* pData);// 0x10013287
void lmsg_WriteCompRotation(LMessageImpl* pMsg, LTRotation* pRot);  // 0x100132da

// The ServerMessage members servermsg.h does not carry.  Each already exists
// under exactly this shape elsewhere in the tree -- servermessage.cpp's
// CMsgWriteView (WriteDWord 0x1000ecd6, WriteString 0x10026137), smgr.cpp's
// CMsgFloatWriteView (WriteFloat 0x10028a35) and CMsgLenView (GetSpaceLeft
// 0x1000e845) -- and they are gathered on one view here only because this
// file cannot reach three different .cpp-local classes.

// A sprite object's own field, the one CF_MODELINFO sends for OT_SPRITE.
// objtypes.h has no sprite class yet; this is the single dword both images
// read at LTObject+0x1bc under `cmpb $0x3` (0x10029381 / the /O2 twin).
class CObjSpriteQ291
{
public:
	uint8  m_pad_000[0x1c4];
	uint32 m_Unknown1bc;                     // 0x1c4
};

// A model object's field at 0x204, sent at the end of the attachment block
// and only for OT_MODEL (0x100295f4).
class CObjModelQ291
{
public:
#ifdef PRIMAL_HUNT
	uint8  m_pad_000[0x260];
#else
	uint8  m_pad_000[0x24c];                 // AvP2 1.0.9.6
#endif
	uint32 m_Unknown204;                     // 0x260 (AvP2 0x24c)
};

// A container's code at 0x270 is objtypes.h's CObjContainer::m_ContainerCode;
// repeated on a view for the same reason as the two above.
class CObjContainerQ291
{
public:
	uint8  m_pad_000[0x278];
	uint16 m_ContainerCode;                  // 0x278
};

// The file record the file id comes out of: smgr.cpp's CSoundIDRec is the
// same eight-bytes-then-a-word shape, and this is the file manager's.
class CFileIDRecQ291
{
public:
	uint8  m_pad_00[0x08];
	uint16 m_nID;                            // +0x08
};

// STAND-INS for sub_100291c7's lithtech.exe (/O2) shape, measured against
// 0x00467540..0x00467eec (2,476 bytes).  Neither changes server.dll.
//
// sm_FloatArgQ291: the SIX float writes that ship CALLS (0x00467800 ..
// 0x00467872, into WriteType<float> 0x00466700) load the member onto the
// x87 stack and `fstps (%esp)` it, where a plain member argument gives
// `mov/push` at /O2.  A value-returning inline accessor is what
// materialises a float in st(0) before a push; what the original accessor
// was is not visible.  (/O1 emits fld/fstp either way.)
inline float sm_FloatArgQ291(const float& f) { return f; }

#ifndef LT_SERVER_BUILD
// sm_WritePacketDWordQ291: CPacket::WriteType<uint32> with the grow reaching
// Insert2 DIRECTLY instead of through Append->Insert.  Only at that depth
// does /O2 expand BaseDelete inside the expanded Insert2 (the virtual
// Free at 0x00467e2d) at the function's LAST write, as shipped does.
// packet.h's two-level chain leaves BaseDelete a call there (measured).
//
// RESIDUAL (lithtech, 2,655 vs 2,476 bytes, 4 mnemonic blocks): the
// `WriteType((uint16)0xffff)` after the attachment loop still expands
// Insert2, where shipped CALLS it (0x00467cec) -- /O2's inline budget runs
// out one site earlier in shipped.  Not moved by an extra wrapper level on
// either site, by dropping sm_FloatArgQ291, or by the direct-Insert2 helper
// at the 0xffff site (then the LAST site loses BaseDelete instead).
// Everything from the attachment loop on (GetSpaceLeft, the seven writes)
// is expanded in shipped, so those sites are written as the inline
// CPacket::WriteType / CMsgLenView::GetSpaceLeft; every earlier write is a
// call in both images.
inline void sm_WritePacketDWordQ291(CPacket* p, uint32 val)
{
	if (p->m_Pos + sizeof(uint32) < p->m_MaxDataLen)
	{
		while (p->m_DataLen + sizeof(uint32) >= p->m_Data.GetSize())
			p->m_Data.Insert2(p->m_Data.m_nElements, 0, &g_DefAlloc);

		*(uint32*)&p->m_Data.m_pArray[p->m_Pos] = val;
		p->m_Pos += sizeof(uint32);
		p->m_DataLen += sizeof(uint32);
	}
	else
	{
		p->ReportWriteOverrun();
		p->m_Flags |= PACKETFLAG_WRITEOVERFLOW;
	}
}
#endif

// 0x100291c7 / 0x00467540.
//
// HOW THE FLAG WORD IS SPELLED.  It is a uint16 with a dword home -- both
// images store it 32 bits wide and read it back with `cmpw` / `test %bx,%bx`
// (0x1002925e, 0x004675e1), which smgr.cpp's sub_10027ad0 note
// records as what VC6 does with a uint16 local.  The masks are written
// POSITIVE: the shipped immediates are 0x0000fbfb, 0x0000fffd, 0x0000fffb,
// 0x0000ffdf, 0x0000ffbf and 0x0000fdff, and `nFlags &= ~0x0404` on a uint16
// would give 0xfffffbfb instead.  So the source has the sixteen-bit
// complements, not `~CF_`.
//
// The three-way clear at the top is ONE statement (`and` then `or`, one
// store, 0x10029236..0x10029240) where every other clear is its own.
//
// The header is two WriteBytes or one, with the SECOND call shared: the
// wide arm writes (low | 0x80) and falls into the common WriteByte with the
// high byte, the narrow arm jumps into the same call with the whole word.
// That is a tail merge, not two separate statements.
//
// Every member read is WRITTEN OUT at each use.  pObject->m_pSData is
// fetched four times, pObject->m_pSData->m_pSFXMessage three, and
// pObject->m_ObjectType six; the single load each straight-line region shows
// is VC6's CSE inside a block, and the re-read after every call is what says
// there is no local.
//
// The light radius is a comparison in the SOURCE, not a conversion artefact:
// `fcoms` against 0.0f (0x1004d208 / 0x004a14d8) with the `< 0` arm FALLING
// THROUGH to `push $0` and the `>= 0` arm jumping to __ftol, and the two
// WriteWord calls merged.  A plain `(uint16)fRadius` emits __ftol with no
// compare at all.
// The radius IS a local, the one exception to the written-out rule: ship
// keeps the loaded value on the x87 stack across the compare (`fcoms`, then
// `fstp %st(0)` on the `< 0` arm, 0x10029438..0x10029443), where reading the
// member twice gives `fcomps` + a `lea` + a reload.
// AVP2: the whole writer takes a running SIZE as well as the message
// (Primal Hunt server.dll 0x1003466e, 1,673 bytes; AvP2 1.0.9.6 0x10031c6b,
// 1,665, the same source at another revision): every field is written only
// with a message and counted only with a size, `if (pMsg) ...; if (pSize)
// *pSize += width;` at every site, which is how sm_WriteObjectUpdate measures
// an object before it writes it. Beyond that, read off the PH bytes:
//
//   * flag 0x4000 (the velocity was last sent unzeroed) forces CF_POSITION
//     (0x1003469f `testb $0x40,-0x3(%ebp); orl $2`), and the wide header
//     takes 0x2700, not 0x700 (0x10034717 `test $0x27,%ah`).
//   * the narrow header, with a message, refuses a flag word whose low byte
//     is empty (0x10034747 `test %al,%al`, landing on the same `return 0` as
//     the empty-word test).
//   * the new-object byte is kept as a uint16 in pClient's dead slot
//     (0xc(%ebp)) and tested twice: once for the header, once more after it
//     for the blocking priority (+0xbd).
//   * the flags go out as a dword masked to 0x4ffff, the second flag word as
//     a word, then the user flags (0x1003494f..0x1003497c).
//   * the position block sends a COPY of the velocity (`movsl` x3 into
//     -0x20(%ebp)), zeroed when CF_POSITION itself is not set, and records
//     in the info's flags whether it went out live (0x4000).
//   * attachments lose the space check, and a full-resolution object sends
//     each offset as three floats and a rotation (28 bytes) rather than a
//     compressed vector and rotation (15). The count byte in front is bit
//     10 of LTObject::m_ChangeFlags.
//
// lithtech.exe's copy is 0x0047aee0..0x0047bec8 (4,073 bytes; the function
// table splits it at the second `ret`, 0x0047afe8). Which writes it expands
// is read off it: every write up to the scale's z is a call; the scale's z,
// the position's three floats, the attachment count byte and each
// attachment's word, dword and three floats are CPacket::WriteType expanded
// with Insert2 CALLED (0x004162f0); the 0xffff and the model dword expand
// Insert2 too. NOT MATCHED in lithtech.exe: written with the attachment
// floats as CPacket::WriteType, ours also expands Insert2 at their y and z
// (4,528 vs 4,073) -- the /O2 inline budget again (see
// sm_WritePacketDWordQ291); wrapped in sm_FloatArgQ291 only z is over
// (4,373) but they load through the x87 where shipped copies the dword
// (0x0047b97f). The attachment floats are therefore spelled as server.dll
// wants them (WriteFloat through sm_FloatArgQ291: CPacket::WriteType swaps
// the `fstps`/`mov %edi,%ecx` pair at the first, 0x10034c41).
int sub_100291c7(CServerMgr* pMgr, Client* pClient, LTObject* pObject,
                     CObjInfo* pInfo, ServerMessage* pMsg, uint32* pSize)
{
	uint16   nFlags;
	uint16   nNewObject;
	uint8    nInfo;
	LTVector vVelocity;
	uint8    posData[7];
	uint8    rotData[6];

	nFlags = pInfo->m_ChangeFlags;

	if (!(pClient->m_ClientFlags & CLIENTFLAG_SENDCOBJROTATION) &&
		pClient->m_pUnknown3AC == pObject)
	{
		nFlags &= 0xfbfb;                    // ~(CF_ROTATION | 0x400)
	}

	if (nFlags & 0x4000)
		nFlags |= CF_POSITION;

	if (pObject->m_pSData->m_NetFlags & NETFLAG_POSUNGUARANTEED)
		nFlags &= 0xfffd;                    // ~CF_POSITION

	if (pObject->m_pSData->m_NetFlags & NETFLAG_ROTUNGUARANTEED)
		nFlags &= 0xfffb;                    // ~CF_ROTATION

	if (pObject->m_ObjectType == OT_MODEL &&
		(pObject->m_pSData->m_NetFlags & NETFLAG_ANIMUNGUARANTEED))
	{
		nFlags &= 0xffdf;                    // ~CF_MODELINFO
	}

	if (nFlags & CF_NEWOBJECT)
		nFlags = (uint16)((nFlags & 0xfdff) | CF_POSITION);

	if ((nFlags & CF_COLORINFO) &&
		(pObject->m_ObjectType == OT_NORMAL ||
		 pObject->m_ObjectType == OT_CONTAINER ||
		 pObject->m_ObjectType == OT_CAMERA))
	{
		nFlags &= 0xffbf;                    // ~CF_COLORINFO
	}

	if (nFlags == 0)
		return 0;

	if (nFlags & 0x2700)
	{
		if (pMsg)
		{
			pMsg->WriteByte((uint8)(nFlags | 0x80));
			pMsg->WriteByte((uint8)(nFlags >> 8));
		}

		if (pSize)
			*pSize += 2;
	}
	else
	{
		if (pMsg)
		{
			if (!(uint8)nFlags)
				return 0;

			pMsg->WriteByte((uint8)nFlags);
		}

		if (pSize)
			(*pSize)++;
	}

	if (pMsg)
		pMsg->WriteWord(pObject->m_ObjectID);

	if (pSize)
		*pSize += 2;

	nNewObject = (uint16)(nFlags & CF_NEWOBJECT);

	if (nFlags & CF_NEWOBJECT)
	{
		nInfo = (uint8)pObject->m_ObjectType;

		if (pObject->m_pSData->m_pSFXMessage)
		{
			nInfo |= 0x40;

			if (pObject->m_pSData->m_pSFXMessage->m_Unknown34 >= 0x100)
				nInfo |= 0x80;
		}

		if (pObject->m_pSData->m_bAutoLoad & 1)
			nInfo |= 0x20;

		if (pMsg)
			pMsg->WriteByte(nInfo);

		if (pSize)
			(*pSize)++;

		if (pObject->m_pSData->m_pSFXMessage)
		{
			if (pMsg)
			{
				if (nInfo & 0x80)
					pMsg->WriteWord(pObject->m_pSData->m_pSFXMessage->m_Unknown34);
				else
					pMsg->WriteByte((uint8)pObject->m_pSData->m_pSFXMessage->m_Unknown34);

				// THROUGH CMsgDataView, whose length parameter is uint16
				// (`mov 0x34(%eax),%cx`, 0x10034813).
				((CMsgDataView*)pMsg)->WriteData(
					pObject->m_pSData->m_pSFXMessage->m_pData,
					pObject->m_pSData->m_pSFXMessage->m_Unknown34);
			}

			if (pSize)
			{
				*pSize += (nInfo & 0x80) ? 2 : 1;
				*pSize += pObject->m_pSData->m_pSFXMessage->m_Unknown34;
			}
		}

		if (pObject->m_ObjectType == OT_WORLDMODEL)
		{
			if (pMsg)
				((CMsgWriteQ291*)pMsg)->WriteString(pObject->m_pWorldBsp->m_Name);

			if (pSize)
				*pSize += 65;
		}
		else if (pObject->m_ObjectType == OT_CONTAINER)
		{
			if (pMsg)
			{
				((CMsgWriteQ291*)pMsg)->WriteString(pObject->m_pWorldBsp->m_Name);

				pMsg->WriteWord(((CObjContainerQ291*)pObject)->m_ContainerCode);
			}

			if (pSize)
				*pSize += 67;
		}
		else if (pObject->m_ObjectType == OT_MODEL)
		{
			// AvP2 1.0.9.6 passes the size too (0x10031eb0).
			sm_WriteObjectFiles(pObject, pMsg, pSize);
		}
		else if (pObject->m_ObjectType == OT_SPRITE)
		{
			if (pMsg)
				pMsg->WriteWord(((CFileIDRecQ291*)
					pObject->m_pSData->m_pModelFile)->m_nID);

			if (pSize)
				*pSize += 2;
		}
	}

	if (nNewObject)
	{
		if (pMsg)
			pMsg->WriteByte(pObject->m_BlockingPriority);

		if (pSize)
			(*pSize)++;
	}

	if (nFlags & (CF_MODELINFO | 0x2000))
	{
		if (pObject->m_ObjectType == OT_MODEL)
		{
			sub_100290d7(pObject, pMsg, pSize);
		}
		else if (pObject->m_ObjectType == OT_SPRITE)
		{
			if (pMsg)
				((CMsgWriteQ291*)pMsg)->WriteDWord(
					((CObjSpriteQ291*)pObject)->m_Unknown1bc);

			if (pSize)
				*pSize += 4;
		}
	}

	if (nFlags & CF_FLAGS)
	{
		if (pMsg)
		{
			((CMsgWriteQ291*)pMsg)->WriteDWord(pObject->m_Flags & 0x4ffff);
			pMsg->WriteWord((uint16)pObject->m_Flags2);
			((CMsgWriteQ291*)pMsg)->WriteDWord(pObject->m_UserFlags);
		}

		if (pSize)
			*pSize += 10;
	}

	if (nFlags & CF_COLORINFO)
	{
		if (pMsg)
		{
			pMsg->WriteByte(pObject->m_ColorR);
			pMsg->WriteByte(pObject->m_ColorG);
			pMsg->WriteByte(pObject->m_ColorB);
			pMsg->WriteByte(pObject->m_ColorA);
		}

		if (pSize)
			*pSize += 4;

		if (pObject->m_ObjectType == OT_LIGHT)
		{
			if (pMsg)
			{
				float fRadius = pObject->m_LightRadius;
				if (fRadius < 0.0f)
					pMsg->WriteWord(0);
				else
					pMsg->WriteWord((uint16)fRadius);
			}

			if (pSize)
				*pSize += 2;
		}
	}

	if (nFlags & CF_SCALE)
	{
		if (pMsg)
		{
			((CMsgWriteQ291*)pMsg)->WriteFloat(sm_FloatArgQ291(pObject->m_Scale.x));
			((CMsgWriteQ291*)pMsg)->WriteFloat(sm_FloatArgQ291(pObject->m_Scale.y));
		}

		if (pSize)
			*pSize += 8;

		if (pObject->m_ObjectType != OT_SPRITE)
		{
			if (pMsg)
				((CPacket*)pMsg)->WriteType(pObject->m_Scale.z);

			if (pSize)
				*pSize += 4;
		}
	}

	if (nFlags & (CF_POSITION | 0x200))
	{
		vVelocity = pObject->m_Velocity;

		if (pMsg)
		{
			if (!(pInfo->m_ChangeFlags & CF_POSITION))
			{
				pInfo->m_ChangeFlags &= 0xbfff;
				vVelocity.x = 0.0f;
				vVelocity.y = 0.0f;
				vVelocity.z = 0.0f;
			}
			else
			{
				pInfo->m_ChangeFlags |= 0x4000;
			}
		}

		if (pObject->m_Flags & FLAG_FULLPOSITIONRES)
		{
			if (pMsg)
			{
				((CPacket*)pMsg)->WriteType(pObject->m_Pos.x);
				((CPacket*)pMsg)->WriteType(pObject->m_Pos.y);
				((CPacket*)pMsg)->WriteType(pObject->m_Pos.z);

				((IMessageWrite*)&pMsg->m_Write)->WriteVector(&vVelocity);
			}

			if (pSize)
				*pSize += 24;
		}
		else
		{
			if (pMsg)
			{
				lmsg_CompressPos(posData, &pObject->m_Pos,
					(CPosRef*)&pMgr->m_World);

				lmsg_WriteCompPosWords((IMessageWrite*)&pMsg->m_Write,
					posData);

				((IMessageWrite*)&pMsg->m_Write)->WriteCompVector(&vVelocity);
			}

			if (pSize)
				*pSize += 16;
		}
	}

	if (nFlags & (CF_ROTATION | 0x400))
	{
		if (pObject->m_Flags & FLAG_FULLPOSITIONRES)
		{
			if (pMsg)
				((IMessageWrite*)&pMsg->m_Write)->WriteRotation(
					&pObject->m_Rotation);

			if (pSize)
				*pSize += 16;
		}
		else
		{
			if (pMsg)
			{
				sub_1001307f(&pObject->m_Rotation, rotData);

				lmsg_WriteCompRotationBytes((IMessageWrite*)&pMsg->m_Write,
					rotData);
			}

			if (pSize)
				*pSize += 6;
		}
	}

	if (nFlags & CF_ATTACHMENTS)
	{
		Attachment* pAttach;

		if (pMsg)
			((CPacket*)pMsg)->WriteType((uint8)((pObject->m_ChangeFlags >> 10) & 1));

		if (pSize)
			(*pSize)++;

		for (pAttach = pObject->m_pAttachments; pAttach; pAttach = pAttach->m_pNext)
		{
			if (pMsg)
			{
				((CPacket*)pMsg)->WriteType((uint16)pAttach->m_nChildID);
				((CPacket*)pMsg)->WriteType((uint32)pAttach->m_iNode);
			}

			if (pSize)
				*pSize += 6;

			if (pObject->m_Flags & FLAG_FULLPOSITIONRES)
			{
				if (pMsg)
				{
					((CMsgWriteQ291*)pMsg)->WriteFloat(sm_FloatArgQ291(pAttach->m_Offset.m_Pos.x));
					((CMsgWriteQ291*)pMsg)->WriteFloat(sm_FloatArgQ291(pAttach->m_Offset.m_Pos.y));
					((CMsgWriteQ291*)pMsg)->WriteFloat(sm_FloatArgQ291(pAttach->m_Offset.m_Pos.z));

					((IMessageWrite*)&pMsg->m_Write)->WriteRotation(
						&pAttach->m_Offset.m_Rot);
				}

				if (pSize)
					*pSize += 28;
			}
			else
			{
				if (pMsg)
				{
					((IMessageWrite*)&pMsg->m_Write)->WriteCompVector(
						&pAttach->m_Offset.m_Pos);

					lmsg_WriteCompRotation(&pMsg->m_Write,
						&pAttach->m_Offset.m_Rot);
				}

				if (pSize)
					*pSize += 15;
			}
		}

		if (pMsg)
			((CPacket*)pMsg)->WriteType((uint16)0xffff);

		if (pSize)
			*pSize += 2;

		if (pObject->m_ObjectType == OT_MODEL)
		{
			if (pMsg)
			{
#ifndef LT_SERVER_BUILD
				sm_WritePacketDWordQ291((CPacket*)pMsg,
					((CObjModelQ291*)pObject)->m_Unknown204);
#else
				((CPacket*)pMsg)->WriteType((uint32)
					((CObjModelQ291*)pObject)->m_Unknown204);
#endif
			}

			if (pSize)
				*pSize += 4;
		}
	}

	return 1;
}


// 0x100288be (117 bytes, /O1) / 0x00466550 (186 bytes, /O2) — this client's
// record for one sound, or a new one. Ported from the /O2-tuned copy in
// src/engine/lithtech/ccobj.cpp, which is byte-identical at 0x00466550; that
// copy is kept in step rather than deleted, exactly as sub_100291c7's is.
//
// THE KEY IS THE PARAMETER'S OWN ADDRESS. Both images pass `lea 0x10(%esp)` /
// `lea 0x14(%esp)` as the hash key (0x100288c4, 0x0046655d) and that slot IS
// the uint16 parameter — there is no local to copy it into. So the source
// hands the hash `&nSoundID` with a length of two, twice.
//
// The redundant null test is real and in the source: the allocate's own
// failure arm and the caller's `if (!pState)` both land on the same return-0
// block (0x100288fd -> 0x10028922, 0x004665aa/0x004665c7 -> 0x004665c9).
//
// THE FAILURE PATH FREES BANK MEMORY WITH dfree, and that is a shipped bug
// rather than a misread: 0x1002891c / 0x004665ed is the plain allocator free
// (0x1000d398 / 0x00429080), not sb_Free, on a record that came out of
// m_Bank89C two statements earlier. Transcribed as shipped.
CSoundSentState* snd_FindSentState(Client* pClient, uint16 nSoundID)
{
	void*            hElement;
	CSoundSentState* pState;

	hElement = dhash_FindElement((CHashTable*)pClient->m_hSoundHash,
		&nSoundID, sizeof(nSoundID));

	if (hElement)
	{
		pState = (CSoundSentState*)dhash_GetData(hElement);

		if (pState)
			return pState;
	}

	pState = (CSoundSentState*)
		sm_sb_AllocateCleared(&g_pServerMgr->m_Bank89C);

	if (!pState)
		return LTNULL;

	pState->m_ChangeFlags = SNDSENT_FLAGS | SNDSENT_BYTE04 | SNDSENT_RANGE;

	hElement = dhash_AddElement((CHashTable*)pClient->m_hSoundHash,
		&nSoundID, sizeof(nSoundID));

	if (!hElement)
	{
		dfree(pState);
		return LTNULL;
	}

	dhash_SetData(hElement, pState);

	return pState;
}


// 0x100273e9 (211 bytes, /O1) / 0x004641a0 (251 bytes, /O2) — the last thing
// sm_RemoveClient does: everything the client record owns, then the record.
// Ported from the /O2-tuned copy in src/engine/lithtech/ccobj.cpp, which is
// byte-identical at 0x004641a0; that copy is kept in step rather than
// deleted, exactly as sub_100291c7's is.
//
// The manager parameter is ACCEPTED AND NEVER READ. Both images load only
// the second argument (`mov 0xc(%ebp),%esi` at 0x100273ee), so the pair in
// the declaration above is confirmed and the first one is dead.
//
// Client+0x18c IS THE FILE-TRANSFER STATE, which corrects servermgr.h's
// "unhandled packets go here": 0x100273f7 hands it to svnew.cpp's xfer_Term
// (0x1000e5c5), so it is a CClientFilesView*.
//
// THE FREES ARE THREE DIFFERENT FUNCTIONS and the split is the shape:
// 0x1000d398 is `dfree` (the four engine-allocated buffers), 0x10028ace is
// the one-call wrapper servermgr.h calls net_Free and that is also the
// image's `operator delete` (the client data block, and the record itself),
// and dhash_Free (0x1000dbe0) takes the hash.
//
// The five cdecl calls between the message loop and the hash walk share ONE
// `add $0x14,%esp` (0x10027457), which is cl 8447 batching consecutive
// fixed-prototype cleanups -- it says nothing about the prototypes and
// nothing about statement grouping.
//
// The hash walk frees each record back onto m_Bank89C BY HAND -- 0x10027476
// leas the global's +0x8b4, which is m_Bank89C's m_pFreeList at +0x18 --
// i.e. servermgr.h's inline sb_Free. snd_FindSentState above is where those
// records come from.
//
// The advance is its own statement: nested inside the GetData call, VC6's
// right-to-left ordering would hoist it and the shipped batching is lost.
//
// THE DESTRUCTOR IS REAL AND servermgr.h DOES NOT MODEL IT. `delete
// pClient` reaches 0x100274bc, a 32-byte out-of-line body that destroys the
// 2x2 packet array at Client+0x134 BACKWARDS, four trips, stride 12, each
// trip a guarded `call *0x8(%vptr)`. That is the array-destructor idiom and
// nothing else, so ClientPacketSet has a destructor that releases its
// packet. servermgr.h is not changed here: the slot is smgr.cpp's own
// CClientPacketSetView above, whose first member is servermsg.h's
// CMessageRef -- and CMessageRef ALREADY carries exactly that destructor
// (`if (m_pMsg) m_pMsg->Release()`, Release being slot 2, which is the
// `call *0x8(%eax)` at 0x100274d3). So the destructor needs no new code at
// all, only a layout view that puts four of those slots at +0x134.
class CClientDtorView
{
public:
	uint8                m_pad_000[0x134];
	CClientPacketSetView m_PacketSets[2][2];             // 0x134
	uint8                m_pad_164[0x3e0 - 0x164];
};

class CClientFilesView;
void xfer_Term(CClientFilesView* pState);                // 0x1000e5c5

void sub_100273e9(CServerMgr* pMgr, Client* pClient)
{
	LTLink* pCur;
	void*   hIter;
	void*   hElement;

	xfer_Term((CClientFilesView*)pClient->m_pUnknown18C);

	pCur = pClient->m_List3CC.m_Head.m_pNext;

	while (pCur != &pClient->m_List3CC.m_Head)
	{
		LTLink* pNext = pCur->m_pNext;

		((CQueuedMsg*)pCur->m_pData)->Release();

		pCur = pNext;
	}

	dfree(pClient->m_pObjInfos);
	dfree(pClient->m_IDLists[0].m_pIDs);
	dfree(pClient->m_IDLists[1].m_pIDs);
	dfree(pClient->m_pName);

	hIter = dhash_GetFirst(pClient->m_hSoundHash);

	while (hIter)
	{
		hElement = dhash_GetNext(&hIter);

		if (hElement)
			sb_Free(&g_pServerMgr->m_Bank89C, dhash_GetData(hElement));
	}

	dhash_Free(pClient->m_hSoundHash);

	::operator delete(pClient->m_pClientData);

	delete (CClientDtorView*)pClient;
}


// 0x10013585 (sub_10013585, the facing as one byte) is servermsg.cpp's.


// ---- light-animation changes to one client ------------------------------ //
//
// Here rather than in serverlightanim.cpp: both images put sm_Unknown26755
// inside this file's run (lithtech.exe 0x00462fd0 between 0x00462ee0 and
// 0x004630f0; server.dll 0x10026755 before 0x10026859).
#include "serverlightanim.h"

void la_WriteChange(CServerMgr* pMgr, LightAnim* pAnim, uint32 hLightAnim,
                    ServerMessage* pMsg, uint32 nChangeFlags);

// 0x1000e845 — how many more bytes the message will take: the declared
// maximum less the write cursor, less seven, floored at zero. servermsg.h
// does not carry it and is not this reconstruction's to change, so the
// message is cast to reach it (name ours).
class CMsgSpaceView
{
public:
	// IN-CLASS, and that is load-bearing. Both images are /Ob1, so only an
	// in-class (or `inline`-marked) body is an expansion candidate at all.
	// server.dll's /Os declines it and CALLS 0x1000e845; lithtech.exe's /Ot
	// accepts it, and the expansion lets the optimiser FOLD the floor into
	// the caller's comparison -- at 0x00463055 the `js` jumps straight into
	// the guarded block, because a negative left-count is certainly <= 0x16.
	// That fold is the whole 286-vs-261 difference between the two images.
	int GetSpaceLeft()
	{
		ServerMessage* pMsg = (ServerMessage*)this;
		int            nLeft;

		nLeft = pMsg->m_MaxDataLen - pMsg->m_Unknown38 - 7;

		if (nLeft < 0)
			nLeft = 0;

		return nLeft;
	}
};


// 0x10026755 — flush a client's queued light-animation changes. One packet is
// filled with as many changes as fit and sent whenever it runs low on room,
// then the remainder goes out at the end and the queue is emptied.
// servermgr.h names it sm_Unknown26755, which is the name both call sites
// already use.
//
// Read off the bytes:
//
//   * "runs low on room" is the space accessor answering 0x16 or less, and it
//     is asked *after* the change has been written, so the flush is a
//     look-ahead for the next one.
//   * a sent packet is reset by storing 1 into the payload length and the
//     write cursor, in that order (0x38 before 0x34) — the same pair of
//     statements every direct send in smgr.cpp opens with.
//   * the tail flush tests `m_Unknown34 - 1 > 0` rather than `> 1`: the
//     shipped code decrements and tests, so the subtraction is in the source.
//   * an animation index the world does not have is skipped without being
//     dequeued separately — the whole queue is cleared at the end regardless.
//   * the loop counter and the queue cursor are both memory-homed because the
//     manager, the client and the hoisted 0x80 priority take all three
//     callee-saved registers; the cursor lands in the dead pClient parameter
//     slot, and the element pointer is a body local, which is what puts the
//     induction variable on the record's first byte rather than on its
//     +2 field.
//   * the animation index is read off the change at each use rather than
//     through a named local: with a local, `&m_pLightAnims[h]` accumulated
//     into %ecx; read in place, the base is added straight into %eax as
//     shipped (0x100267c1), and lithtech.exe's twin (0x00462fd0) went from
//     six mnemonic blocks to three.
//   * the shipped frame is 8 bytes: the reference and msg_AllocPacket's
//     returned temporary, which does not share its slot with `i`.
//
// NEAR, size-exact on server.dll (260 of 260 bytes, 94 instructions), and
// lithtech.exe 290 vs 286 with three mnemonic blocks. The whole residual on
// both images is where `i` is homed: the shipped builds put it in the dead
// pMgr parameter slot (0x8(%ebp) here; 0x14(%esp) at 0x00463013), ours gives
// it a frame slot of its own. On server.dll that also lets VC6 park a zero
// in %ebx for the reference's constructor, the temporary's test, `i = 0` and
// the loop guard (shipped: `andl $0x0` and `cmpl $0x0` each time), which
// moves the three callee-saved pushes from after the temporary's release to
// the prologue.
// Dead-parameter-slot homing has no known source lever in this tree.
//
// Falsified against it: cl 8168, 8447 and 8966 (all three identical here);
// (with NOLF's out-parameter spelling) the out-parameter in an inner block;
// `i` at function scope, first among the locals,
// after pLAClient, in the for-init, as an `int`, as a while loop, and
// initialized before the allocation; the queue walked by index, by a hoisted
// cursor, and by a body-local element pointer; the index as a uint32 or
// uint16 local; pLAClient dropped for casts at each use; the reset pairs
// written through msg.m_pMsg directly (265 bytes); `pIO != LTNULL`
// (likewise under the out-parameter spelling).
void sm_Unknown26755(CServerMgr* pMgr, Client* pClient)
{
	CMessageRef msg;
	CLAClient*   pLAClient = (CLAClient*)pClient;

	msg.Init(msg_AllocPacket(0x44c, 0x44c));

	for (uint32 i = 0; i < pLAClient->m_nChanges; i++)
	{
		CLAChange* pChange = &pLAClient->m_Changes[i];

		if (pChange->m_hLightAnim < pMgr->m_nLightAnims)
		{
			la_WriteChange(pMgr, &pMgr->m_pLightAnims[pChange->m_hLightAnim],
			               pChange->m_hLightAnim, msg.m_pMsg,
			               pChange->m_ChangeFlags);

			if (((CMsgSpaceView*)msg.m_pMsg)->GetSpaceLeft() <= 0x16)
			{
				sm_SendToClient(pMgr, pClient, 0x19, msg.m_pMsg, 0, 0x80);

				{
					ServerMessage* pMsg = msg.m_pMsg;

					pMsg->m_Unknown38 = 1;
					pMsg->m_Unknown34 = 1;
				}
			}
		}
	}

	if (msg.m_pMsg->m_Unknown34 - 1 > 0)
	{
		sm_SendToClient(pMgr, pClient, 0x19, msg.m_pMsg, 0, 0x80);

		{
			ServerMessage* pMsg = msg.m_pMsg;

			pMsg->m_Unknown38 = 1;
			pMsg->m_Unknown34 = 1;
		}
	}

	pLAClient->m_nChanges = 0;
}
