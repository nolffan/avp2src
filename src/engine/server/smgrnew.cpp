// *********************************************************************** //
//
//  MODULE:   W9_474000 (.CPP)   (scratch delivery file; name ours)
//
//  WHAT THIS IS.  Six functions from the 0x00474000..0x00478000 enumeration.
//  They belong in TWO EXISTING translation units, not in a file of their
//  own -- see "INTEGRATION" below.  This file exists only as one artefact
//  to paste from; compiling it as its own TU is
//  legal but an isolated harness TU can invent a
//  residual through string-literal emission, and two of these functions
//  raise RETURN_ERROR literals.
//
//  THE CLUSTER IS NOT WHAT THE ENUMERATION MAKES IT LOOK LIKE.
//  0x00474000..0x00476e7b is smgr.obj, which src/engine/server/smgr.cpp
//  ALREADY reconstructs and which the lithtech build ALREADY compiles.  Of
//  the thirteen rows, five are functions we have already written that are
//  byte-identical in server.dll (/O1) and NOT in lithtech.exe (/O2) -- pure
//  /O2-shape work, not reconstruction -- one is an interior block of
//  CServerMgr::DoStartWorld, and one needs a header change (servermgr.h)
//  not made here.
//
//  ROW-BY-ROW VERDICT
//  ------------------
//   addr        sz   what it is                          verdict
//   0x00474130  169  CServerMgr::RemoveDeadSounds        ALREADY WRITTEN, /O2 gap
//   0x004745a0  185  CServerMgr::CreateObject            ALREADY WRITTEN, /O2 gap
//   0x00474740  202  sm_GrowClientObjInfos (name ours)   NEW -- section A1
//   0x00474810  351  sm_CreateServerData (name shipped)  NEW -- section A2
//   0x00474a60   73  sub_100334cc                        ALREADY WRITTEN, /O2 gap
//   0x00474ab0  749  sm_AddObjectToWorld (name shipped)  ALREADY WRITTEN, near
//   0x00475302  171  NOT A FUNCTION -- interior block of CServerMgr::DoStartWorld
//   0x00475910   89  CServerMgr::NotifyCommandChanges    NEW -- section A3
//   0x00476150  531  sm_CreateNewID (name shipped)       WRITTEN -- section A3b
//   0x004763a0  138  sm_CheckObjectIntegrity (shipped)   NEW -- section A4
//   0x00476920  263  sm_SendSFXMessage                   ALREADY WRITTEN, /O2 gap
//   0x00477ce0   54  OT_SPRITE create hook               NEW -- section B1
//   0x00477d20   58  OT_CONTAINER create hook            NEW -- section B2
//
//  ALL THIRTEEN EXTENTS WERE CONFIRMED by disassembling to the boundary and
//  sweeping .text for every e8/e9 displacement landing inside each row.
//  Exactly ONE is wrong: 0x00475302 (`gap`, and the only row in the set with
//  no caller and no data reference at all).  Both `ptr` rows are sound.
//
//  INTEGRATION
//  -----------
//    * sections A1..A4 -> src/engine/server/smgr.cpp.  The shipped source
//      order, read off server.dll's COFF layout (COFF order IS source order),
//      is:
//
//          0x10033126  CServerMgr::CreateObject
//          0x1003318d  CServerMgr::LoadWorld
//          0x10033253  A1  sm_GrowClientObjInfos   <-- here
//          0x100332e8  A2  sm_CreateServerData     <-- here
//          0x100333fc  sm_ObjectRemoved2
//          0x100334cc  sub_100334cc
//          0x100334fe  sm_CreateObjectInWorld
//          ...
//          0x10033fca  A3  NotifyCommandChanges    <-- here
//          0x1003401a  CServerMgr::GetClientName
//          ...
//          0x100345b6  A3b sm_CreateNewID          <-- here
//          0x10034660  (unidentified, 40 bytes)
//          0x10034688  A4  sm_CheckObjectIntegrity <-- here
//          0x10034702  CPrecacheView::ClearWantedFlags
//
//    * sections B1..B2 -> src/engine/lithtech/cmodelload.cpp, immediately
//      after cm_WorldModelExtraInitQ474 (0x00477b80) and before the
//      "A DIFFERENT MODULE" block.  They are the OT_SPRITE and OT_CONTAINER
//      entries of the same create-hook table that file already documents.
//
//  EVERY symbol introduced here carries a Q474 suffix.  Every offset,
//  constant and branch below is cited by the address it was read at.  The
//  /O1 oracle (the retail server.dll) was used on all four smgr rows and
//  is cited alongside.
//
//  GROUND TRUTH: the retail lithtech.exe and server.dll.
//
// *********************************************************************** //

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ltbasedefs.h"
#include "ltcodes.h"
#include "ltlink.h"


// ======================================================================= //
//  PART A -- smgr.obj  (src/engine/server/smgr.cpp)
//
//  When this is pasted into smgr.cpp, DELETE every stand-in in this block
//  and use servermgr.h / ltobject_internal.h / de_error.h directly; the
//  names below are chosen to match those headers so the paste is textual.
//  The three views that must SURVIVE the paste are marked KEEP -- they name
//  fields the headers pad over or give a signature the header gets wrong,
//  and each is a header change still to make.
// ======================================================================= //

#ifndef SMGR_CPP_Q474      // <- everything in this #ifndef is stand-in only

typedef unsigned char  uint8Q;
typedef unsigned short uint16Q;
typedef unsigned long  uint32Q;

// 0x0042d760 / server 0x10036d21 -- de_error.h
extern "C" void dsi_OnReturnError(int code);
extern int      g_DebugLevel;                            // 0x004b0ab0
extern char*    g_pErrorFormat;                           // 0x004a7110
void dsi_ServerError(char* pFormat, ...);                 // 0x0042e6f0

#define LT_ERRSTR_Q474_(x) #x
#define LT_ERRSTR_Q474(x)  LT_ERRSTR_Q474_(x)
#define LT_ERRDESC_Q474    " "

#define REPORT_ERROR_Q474(level, fnName, code) \
	dsi_OnReturnError(code); \
	if (g_DebugLevel >= level) \
		dsi_ServerError(g_pErrorFormat, fnName, LT_ERRSTR_Q474(code), LT_ERRDESC_Q474);

#define RETURN_ERROR_Q474(level, fnName, code) \
	{ \
		REPORT_ERROR_Q474(level, fnName, code) \
		return code; \
	}

#define RETURN_ERROR_SYM_DESC_Q474(level, fnName, code, desc) \
	{ \
		dsi_OnReturnError(code); \
		if (g_DebugLevel >= level) \
			dsi_ServerError(g_pErrorFormat, fnName, #code, desc); \
		return code; \
	}

// 0x00428fd0 / 0x00429080 (server 0x1000d2ef / 0x1000d398) -- the engine
// allocator pair.
void* dalloc(uint32 size);
void  dfree(void* p);

// 0x0042ce30 / 0x0042d090 (server 0x1000dc42 / 0x1000de23) -- servermgr.h
// lines 2492/2495 verbatim.
void* dhash_AddElement(class CHashTable* pTable, void* pKey, uint32 nKeyLen);
void  dhash_SetData(void* hElement, void* pData);

// 0x00490c20 (server 0x1003d240) -- structbank.cpp's grow, declared here
// exactly as smgr.cpp declares it so sm_sb_Allocate can be inline.
class StructBank
{
public:
	uint32 m_StructSize;                     // +0x00
	uint32 m_Unknown04;                      // +0x04
	uint32 m_Unknown08;                      // +0x08  (0x0047477e reads it)
	uint8  m_pad_0c[0x18 - 0x0c];
	void*  m_pFreeList;                      // +0x18  (0x00474748)
};
void sb_Grow(StructBank* pBank, uint32 nCount);

// smgr.cpp lines 38..53 verbatim -- KEEP the real one on paste.
inline void* sm_sb_AllocateQ474(StructBank* pBank)
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

class CBankBaseQ474
{
public:
	virtual ~CBankBaseQ474() {}
	virtual void _v04();
	virtual void _v08(void* pRec);
	virtual void Term();
};

class CMessageBank : public CBankBaseQ474
{
public:
	// server 0x1002ce15 -- a THISCALL member (`lea 0x9f0(%eax),%ecx` at
	// 0x100332f7, no pushed argument), so it has to be a member here and
	// not the free function it used to be; the body is below, out of class,
	// so /O1 declines it and /O2 expands it exactly as both images do.
	void* Allocate();

	// 0x1002ccb0 — slot 1 of the bank's vftable (0x1004e2f0), five bytes:
	// `jmp 0x1002ce15`, Allocate as a tail call. Defined below Allocate.
	virtual void _v04();

	StructBank m_Bank;                       // +0x04  (CServerMgr+0x9f4)
	uint32     m_Unknown20;
};

// servermsg.h's ServerMessage, at the one slot this file reaches.
class ServerMessage
{
public:
	virtual void _v00();
	virtual void _v04();
	virtual void Release();                  // slot 0x08 (0x0047489b)
	virtual void AddRef();                   // slot 0x0c
};

// smgr.cpp's CMsgCursorView, restated.  server.dll's /Os declines the
// inline and calls smgr.cpp's out-of-line copy (0x10023e86), so the server
// build only DECLARES it and link/aliases.rsp binds it there: compiled here
// it was a second 35-byte body (this file's ServerMessage view orders the
// AddRef/Release slots for its own purposes).  lithtech.exe's /O2 expands
// it, so the client build keeps the body.
class CMsgCursorViewQ474
{
public:
	ServerMessage* m_pMsg;

#ifdef LT_SERVER_BUILD
	ServerMessage* Init(ServerMessage* pMsg);
#else
	inline ServerMessage* Init(ServerMessage* pMsg)
	{
		if (m_pMsg)
			m_pMsg->Release();

		m_pMsg = pMsg;

		if (m_pMsg)
			m_pMsg->AddRef();

		return m_pMsg;
	}
#endif
};

class CClassInfo
{
public:
	char* m_pClassName;                      // 0x00
	uint8 m_pad_04[0x24 - 0x04];
	struct ClassDef* m_pClassDef;            // 0x24
};

class CObjInfo                               // servermgr.h line 902
{
public:
	uint16 m_ChangeFlags;
	uint8  m_Unknown02;
	uint8  m_Unknown03;
};

class CObjectMapEntry                        // servermgr.h line 452
{
public:
	uint8  m_nFlags;                         // +0x00  (0x004763c9)
	uint8  m_pad_01[0x04 - 0x01];
	void*  m_pRecord;                        // +0x04  (0x004763d4)
};

class Client
{
public:
	uint8     m_pad_000[0x190];
	CObjInfo* m_pObjInfos;                   // 0x190  (0x00474781)
	uint8     m_pad_194[0x1a8 - 0x194];
	uint8     m_CommandStates[2][0xff];      // 0x1a8  (0x0047593e)
	uint8     m_pad_3a6[0x3a8 - 0x3a6];
	uint32    m_nCurCommands;                // 0x3a8  (0x00475928)
};

class SObjData;

class LTObject
{
public:
	virtual void _v00();
	uint8     m_pad_004[0xb8 - 0x04];
	uint16    m_ObjectID;                    // 0xb8  (0x004763c2)
	uint8     m_pad_0ba[0x188 - 0xba];
	SObjData* m_pSData;                      // 0x188 (0x004763fc)
};

class SObjData
{
public:
	LTLink m_Links;                          // +0x00  (0x00474903)
	void*  m_hIdentifier;                    // +0x0c  (0x004748c7)
	float  m_NextUpdate;                     // +0x10  (0x0047487d)
	float  m_DeactivationTime;               // +0x14  (0x00474886)
	float  m_UpdateTimer;                    // +0x18  (0x0047488f)
	class Client* m_pOwningClient;           // +0x1c  (0x00474874)
	class BaseClass* m_pBaseClass;           // +0x20  (0x00474869)
	CClassInfo* m_pClass;                    // +0x24  (0x00474871)
	void*  m_Unknown28;                      // +0x28  (0x004748a1)
	uint32 m_Unknown2c;                      // +0x2c  (the SFX message ref)
	void*  m_pModelFile;                     // +0x30  (0x0047486e)
	void*  m_pSkinFile;                      // +0x34  (0x0047486c)
	uint8  m_pad_38[0x3c - 0x38];
	LTLink m_StateLink;                      // +0x3c  (0x004748fa)
	class LTObject* m_pNextChanged;          // +0x48
	uint8  m_pad_4c[0x50 - 0x4c];
	uint16 m_ClientChangeFlags;              // +0x50  (0x004748a7)
	uint16 m_NetFlags;                       // +0x52  (0x004748ab)
};

class IServerShell
{
public:
	virtual void _v00();  virtual void _v04();  virtual void _v08();
	virtual void _v0c();  virtual void _v10();  virtual void _v14();
	virtual void _v18();  virtual void _v1c();  virtual void _v20();
	virtual void _v24();
	virtual void OnCommandOn(void* hClient, int command);    // slot 0x28
	virtual void OnCommandOff(void* hClient, int command);   // slot 0x2c
};

class CClassMgr
{
public:
	uint8 m_pad_00[0x14];
	IServerShell* m_pServerShell;            // +0x14  (CServerMgr+0x820)
	uint8 m_pad_18[0x30 - 0x18];             // servermgr.h line 735: 0x80c..0x83b
};

class LAlloc;
extern LAlloc g_DefAlloc;                    // 0x10058c7c

// lithtech.exe (/O2) expands the object map's Insert2 into sm_CreateNewID
// (0x00476212..0x0047634d), which the real collection.h template body gives
// (BaseNew<T> 0x004152d0 stays a call, _DeleteAndDestroyArray/BaseDelete
// expand to g_DefAlloc's virtual Free at 0x004762e6); /O1 calls Insert2,
// and the same template instantiated here IS server.dll's 0x1000ce44 (with
// _AllocateTArray 0x1002cecf and _DeleteAndDestroyArray 0x1000d020), so both
// images use the real class -- the declared-only server stand-in that stood
// here named an Insert2 nothing defined.  The map is CMoArray<CObjectMapEntry>
// (collection.h's header lists CUnknown8FC among the classes derived from it).
#include "collection.h"

class CUnknown8FC : public CMoArray<CObjectMapEntry>
{
};

class CServerMgr
{
public:
	uint8        m_pad_000[0x7e4];
	LTList       m_ClientList;               // 0x7e4  head 0x7e8 (0x00474751)
	uint8        m_pad_7f4[0x80c - 0x7f4];
	CClassMgr    m_ClassMgr;                 // 0x80c  shell at 0x820 (0x00475947)
	uint8        m_pad_83c[0x8b8 - 0x83c];
	uint32       m_Unknown8b8;               // 0x8b8  (0x0047477b)
	uint32       m_Unknown8bc;               // 0x8bc
	class CHashTable* m_hHash8c0;            // 0x8c0  (0x004748d8)
	LTLink       m_Link8c4;                  // 0x8c4  free id records (0x004761e4)
	uint8        m_pad_8d0[0x8dc - 0x8d0];
	LTList       m_List8dc;                  // 0x8dc  head 0x8e0 (0x004748fd)
	uint8        m_pad_8ec[0x8fc - 0x8ec];
	CUnknown8FC  m_Unknown8fc;               // 0x8fc  (0x004763b7)
	uint8        m_pad_910[0x9f0 - 0x910];
	CMessageBank m_MessageBank;              // 0x9f0  (0x00474751)
};

#endif  // SMGR_CPP_Q474


// ======================================================================= //
//  A1.  0x00474740 .. 0x00474809, 202 bytes -- EVERY CLIENT'S PER-OBJECT
//       TABLE GROWS BY FORTY ENTRIES
//
//  /O1 ORACLE: server.dll 0x10033253, 149 bytes.  It calls dalloc, memcpy,
//  memset and dfree out of line; /O2 has /Oi and expands the last three, so
//  the whole 53-byte difference is the two intrinsics.  Everything below --
//  the field offsets, the call order, the trailing statement -- is read off
//  the /O1 twin and confirmed against the /O2 bytes.
//
//  IT IS A MEMBER, thiscall: `ret $0x4` at 0x004747f2 with %ecx live-in at
//  0x00474745.  src/engine/server/spritecontrol.cpp ALREADY declares it, as
//  `CServerMgrIDView::Unknown33253(uint32 nCount)` (line 456), for exactly
//  the reason repeated here: servermgr.h does not carry it.  The same view
//  trick is used below so no header has to change.
//
//  WHAT IT DOES.  m_Unknown8b8 is the number of CObjInfo entries every
//  client's table at Client+0x190 currently holds -- servermgr.h's comment
//  "= 200 at Init".  sm_ClaimObjectID's grow path runs this when the id
//  count has passed it (0x004730e3 `cmp 0x8b8`, 0x004730ee the only call),
//  and it reallocates each client's table to the caller's count plus forty.
//
//  WHY THE LOOP IS A `for`.  The head address is re-materialised in BOTH
//  predecessors -- `lea 0x7e8(%esi),%eax` at 0x00474751 (preheader) and
//  again at 0x004747c6 (latch) -- which the rule "`for` rematerialises a
//  loop invariant into both predecessors; `while` keeps one copy" says is
//  the `for` spelling with the head in a named local.  All three callee-
//  saved registers are spoken for (the cursor in %ebx, the client in %ebp,
//  the new table in %edi), so the head cannot live in one.  The cursor is
//  loaded BEFORE the head (0x0047474b then 0x00474751), which is
//  "order the loads before the head".
//
//  TWO READS OF m_Unknown8b8, not one: 0x0047477b for the copy count and
//  0x004747ab for the memset's base.  Repeated member loads are
//  written out at each use.
// ======================================================================= //

class CServerMgrObjInfoViewQ474
{
public:
	void GrowClientObjInfos(uint32 nCount);  // 0x00474740 / 0x10033253

	uint8  m_pad_000[0x7e4];
	LTList m_ClientList;                     // 0x7e4  (head 0x7e8)
	uint8  m_pad_7f4[0x8b8 - 0x7f4];
	uint32 m_Unknown8b8;                     // 0x8b8
};

void CServerMgrObjInfoViewQ474::GrowClientObjInfos(uint32 nCount)
{
	LTLink* pCur;
	LTLink* pHead;

	pCur  = m_ClientList.m_Head.m_pNext;     // 0x0047474b
	pHead = &m_ClientList.m_Head;            // 0x00474751

	for (; pCur != pHead; pCur = pCur->m_pNext)
	{
		Client*   pClient = (Client*)pCur->m_pData;              // 0x0047476b
		CObjInfo* pNew;

		// 0x0047476e `lea 0xa0(,%eax,4)` -- (nCount + 40) * 4.
		pNew = (CObjInfo*)dalloc((nCount + 40) * sizeof(CObjInfo));

		// 0x0047477b..0x004747a0 -- /Oi memcpy, byte count in %eax.
		memcpy(pNew, pClient->m_pObjInfos,
		       m_Unknown8b8 * sizeof(CObjInfo));

		// 0x004747a6 `mov $0x28,%ecx` + `rep stos` -- 40 dwords, i.e. a
		// CONSTANT 0xa0 bytes (server.dll pushes $0xa0 at 0x100332a1).
		memset(pNew + m_Unknown8b8, 0, 40 * sizeof(CObjInfo));

		dfree(pClient->m_pObjInfos);         // 0x004747bd

		pClient->m_pObjInfos = pNew;         // 0x004747cc
	}

	// 0x004747e3 / 0x004747f9 -- duplicated in both exits by /O2.
	m_Unknown8b8 = nCount + 40;
}


// ======================================================================= //
//  A2.  0x00474810 .. 0x0047496e, 351 bytes -- sm_CreateServerData
//
//  THE NAME IS SHIPPED: 0x00474957 pushes 0x004ae12c, which reads
//  "sm_CreateServerData"; 0x004ae140 reads "20", so the macro is de_error.h's
//  RETURN_ERROR (numeric stringize) at level 1 with code 20.
//
//  /O1 ORACLE: server.dll 0x100332e8, 276 bytes.  ALREADY CALLED FROM OUR
//  SOURCE: src/engine/server/smgr.cpp line 7569 declares and calls it as
//  `sub_100332e8(pMgr, pStruct, pClassDef, pLTObject, pObject,
//  &pLTObject->m_pSData)`, so the six-argument signature is not a guess --
//  and the fifth argument really is unread in both images (nothing touches
//  %ebp+0x18 at /O1 or %esp+0x14 at /O2).
//
//  THE 75-BYTE DIFFERENCE BETWEEN THE IMAGES IS ONE INLINE CHAIN.
//  server.dll calls the bank's Allocate (0x1002ce15, thiscall on
//  CServerMgr+0x9f0) and the message-cursor Init (0x10023e86).  lithtech.exe
//  expands BOTH:
//
//    0x00474748..0x0047484c  is sm_sb_Allocate (smgr.cpp already has it
//                            inline) plus Allocate's own `if (p)` -- the
//                            `cmp %esi,%ebx` at 0x00474847 is the guarded-
//                            inline tell, a test on a
//                            register nothing has touched.
//    0x0047485d..0x00474862  is the record constructor 0x10010e03, three
//                            zero stores, expanded.
//    0x00474892..0x0047489e  is CMsgCursorView::Init(LTNULL) expanded, the
//                            trailing `if (m_pMsg) AddRef()` folded away
//                            because m_pMsg is a freshly-stored zero.
//
//  So the SHARED source is one statement, `pMgr->m_MessageBank.Allocate()`,
//  and the bank's Allocate has to be visible at this call site.  IT IS A
//  THISCALL MEMBER, not the free function this file used to carry: server
//  0x100332f7 is `lea 0x9f0(%eax),%ecx` straight into the call with nothing
//  pushed and no cleanup, where a free function taking the bank gives
//  `add $0x9f0,%eax ; push %eax ; ... ; pop %ecx` -- one byte more, and it
//  was one of the two bytes this row was short by.  Declared on the local
//  CMessageBank view and defined out of class below, `inline`, so /O1
//  declines it (server.dll calls 0x1002ce15) and /O2 expands it, which is
//  what both images do.  On the paste it belongs in servermgr.h beside
//  sb_Free, ABOVE this function (VC6's inliner is forward-only).
//
//  WHAT IS LEFT, AND IT IS 2 BYTES AND IRREDUCIBLE.  273 against 276:
//
//    * 1 byte is the revision transformation "removal of a
//      zero-extension -- only our cl 8447 does it".  server.dll loads the
//      create flags with `movzbl 0x4(%edi),%eax` (0x10033388) and masks
//      once; every spelling of a one-byte read of that uint32 field gives
//      us `mov 0x4(%edi),%al` (the `(uint8)` cast) or a folded dword load
//      (`& 0xff`), both 3 bytes against the shipped 4.  Measured: both
//      spellings, and CL_REV=8168 on this TU, all three give 273.
//    * 2 bytes are register placement of pLTObject.  The shipped build
//      pushes the PARAMETER for dhash_SetData (0x100333b6
//      `push 0x14(%ebp)`) and reloads it into %edi afterwards (0x100333c8);
//      ours enregisters it into %ebx the moment the constant zero dies
//      (one 3-byte load saved at each of the two uses, 2 bytes net gained).
//      That is the register-versus-memory placement class this file already
//      records for sm_CreateObjectInWorld and sub_10027ad0.
//      It is tied to the constant zero in %ebx: moving
//      `m_hIdentifier = LTNULL` into an `else` keeps that zero live across
//      the if-body, and pLTObject then gets the shipped memory home and
//      reload exactly, but the else costs a jmp (277 bytes). Also measured:
//      the zero store above the flags statement, and `(void*)pLTObject` at
//      dhash_SetData (both 273, unchanged). A `uint8 : 1` bitfield view of
//      the flags gives the same `mov 0x4(%edi),%al ; and $0x1,%eax` as the
//      (uint8) cast, so no field-type spelling reaches the movzbl either.
//
//  WHY THE ERROR BLOCK IS LAST HERE AND FIRST AT /O1.  It has ONE
//  predecessor at /O1 (the call's null test) and TWO at /O2 (sb_Allocate's
//  post-grow test at 0x0047483f and the merged Allocate/caller test at
//  0x0047484c), with the free-list pop between them, so the guarded-return
//  spelling `if (!pSData) RETURN_ERROR(...)` cannot host it as a
//  fall-through once the callee is inlined and VC6 sinks it past the
//  success return.  The /O1 twin is what fixes the spelling as the NEGATED
//  guard; the /O2 placement follows from the inline, not from the source.
//
//  WHY +0x4c IS STORED TWICE (0x004748c0 after 0x004748a4 stored zero):
//  the intervening load is through pStruct, which VC6 must assume may alias
//  pSData, so the first store cannot be dead-store-eliminated.  Both images
//  keep both.  It is two source statements, not one.
//
//  THE (uint8) CAST AT 0x004748b2 IS REAL.  /O2 emits `mov 0x4(%edi),%eax;
//  and $0xff,%eax; and $0x1,%eax` -- a dword load narrowed and THEN masked.
//  /O1 (cl 8447) folds the narrowing into `movzbl` (0x10033388), which is
//  the zero-extension removal that only 8447 performs.  A plain
//  `m_CreateFlags & OCS_AUTOLOAD` would emit one `and $0x1` in both.
//
//  +0x1fc IS m_Name, NOT m_Filename.  ltbasedefs.h's ObjectCreateStruct has
//  m_NextUpdate at 0x27c and m_fDeactivationTime at 0x280 (both read here),
//  so m_Name[128] starts at 0x1fc, m_SkinNames at 0xfc and m_Filename at
//  0x7c -- which is the offset src/engine/lithtech/cmodelload.cpp already
//  records for m_Filename.  The hash this fills is the object-NAME hash
//  (smgr.cpp line 7641 removes from the same table on teardown).
// ======================================================================= //

// The object-data bank's Allocate, CServerMgr+0x9f0 (server 0x1002ce15).
// sizeof(the record) is 0x54, read straight off the bank's own Init
// (0x1002ce3f `push $0x54`), which is why SObjData and ServerMessage share
// this bank.
// The record's constructor, server 0x10010e03 (savrest.cpp carries the
// byte-identical body).  `andl $0x0,0x2c(%ecx)` is the message ref's own
// zeroing constructor and `lea 0x34(%ecx),%edi ; xor ; stos ; stos` is VC6
// constructing a two-element array whose element constructor zeroes one
// pointer; the record's own body is empty.  `inline` and out of class:
// /Os declines it (server.dll CALLS 0x10010e03 from 0x1002ce28) and /O2
// expands it into sub_100332e8 (0x0047485d..0x00474862).
class CSObjDataRefQ474
{
public:
	CSObjDataRefQ474() { m_p = 0; }
	void* m_p;
};

class CSObjDataCtorQ474
{
public:
	CSObjDataCtorQ474();

	uint8            m_pad_00[0x2c];
	CSObjDataRefQ474 m_Ref2c;                // +0x2c
	void*            m_pModelFile;           // +0x30  untouched
	CSObjDataRefQ474 m_Files[2];             // +0x34
	uint8            m_pad_3c[0x54 - 0x3c];
};

inline CSObjDataCtorQ474::CSObjDataCtorQ474()
{
}

inline void* operator new(size_t, CSObjDataCtorQ474* p)
{
	return p;
}

inline void* CMessageBank::Allocate()
{
	void* pRec = sm_sb_AllocateQ474(&m_Bank);

	if (pRec)
		new ((CSObjDataCtorQ474*)pRec) CSObjDataCtorQ474;

	return pRec;
}

void CMessageBank::_v04()
{
	Allocate();
}

LTRESULT sub_100332e8(CServerMgr* pMgr, ObjectCreateStruct* pStruct,
                          struct ClassDef* pClassDef, LTObject* pLTObject,
                          class BaseClass* pObject, SObjData** ppSData)
{
	uint32 nAutoLoad;
	SObjData* pSData;

	*ppSData = LTNULL;                                       // 0x00474824

	pSData = (SObjData*)pMgr->m_MessageBank.Allocate();

	if (!pSData)
		RETURN_ERROR_Q474(1, "sm_CreateServerData",
		                  LT_TRIEDTOREMOVECLIENTOBJECT);     // 20 (0x00474934)

	pSData->m_pBaseClass    = LTNULL;                        // 0x00474869
	pSData->m_pSkinFile     = LTNULL;                        // 0x0047486c
	pSData->m_pModelFile    = LTNULL;                        // 0x0047486e
	pSData->m_pClass        = (CClassInfo*)pClassDef;        // 0x00474871
	pSData->m_pOwningClient = LTNULL;                        // 0x00474874

	pSData->m_NextUpdate       = pStruct->m_NextUpdate;        // 0x0047487d
	pSData->m_DeactivationTime = pStruct->m_fDeactivationTime; // 0x00474886
	pSData->m_UpdateTimer      = pStruct->m_fDeactivationTime; // 0x0047488f

	((CMsgCursorViewQ474*)&pSData->m_Unknown2c)->Init(LTNULL);

	pSData->m_Unknown28        = LTNULL;                     // 0x004748a1
	*(uint32*)((uint8*)pSData + 0x4c) = 0;                   // 0x004748a4
	pSData->m_ClientChangeFlags = 0;                         // 0x004748a7
	pSData->m_NetFlags          = 0;                         // 0x004748ab

	// THE READ IS VOLATILE. Both images keep a zero-extension that a plain
	// read folds away: server.dll `movzbl 0x4(%edi),%eax ; and $0x1`
	// (0x10033388) and lithtech.exe `mov 0x4(%edi),%eax ; and $0xff ; ... ;
	// and $0x1` (0x004748af). Probed over 42 spellings (field plain or
	// volatile; local uint32/int/long; (uint8)x, x & 0xff, (uint8)(x & 0xff),
	// ...): every plain read folds the narrowing into the load (`mov
	// X,%al`) or drops the mask, and ONLY a volatile dword read narrowed as
	// `(uint8)(x & 0xff)` gives both shipped forms at once -- /O1 movzbl
	// and /O2 dword-load-and-mask. server.dll 0x100332e8 is byte-identical
	// with it. What made the original read volatile (a volatile-qualified
	// view, or a field declared that way in an engine-private copy of the
	// struct) is not recoverable; the cast says only what the bytes need.
	// lithtech.exe 0x00474810 is now size-exact (351/351); what is left
	// there is the hash table load scheduled ahead of strlen's `not %ecx`
	// into %eax (shipped loads it after the length push, into %ecx). A
	// named length, a `(uint32)` cast, `&m_Name[0]` and a name local
	// leave it.
	nAutoLoad = (uint8)(*(volatile uint32*)&pStruct->m_CreateFlags & 0xff); // 0x004748af
	*(uint32*)((uint8*)pSData + 0x4c) =
		nAutoLoad & OCS_AUTOLOAD;                            // 0x004748c0

	// The NULL store is in BOTH arms, not one statement ahead of the test:
	// the shipped store sits AFTER the `cmp` of m_Name[0] (0x004748c7,
	// 0x1003339a), and the single unconditional store put it ahead of the
	// load and gave pLTObject a register (ebx) that shipped never gives it.
	// VC6 merges the two copies back into one store above the `je`.
	if (!pStruct->m_Name[0])                                 // 0x004748c3
	{
		pSData->m_hIdentifier = LTNULL;                      // 0x004748c7
	}
	else
	{
		pSData->m_hIdentifier = LTNULL;

		pSData->m_hIdentifier =
			dhash_AddElement(pMgr->m_hHash8c0, pStruct->m_Name,
			                 strlen(pStruct->m_Name) + 1);   // 0x004748e0

		dhash_SetData(pSData->m_hIdentifier, pLTObject);     // 0x004748ee
	}

	dl_TieOff(&pSData->m_Links);                             // 0x00474903
	dl_AddHead(&pMgr->m_List8dc, &pSData->m_StateLink, pLTObject);

	*ppSData = pSData;                                       // 0x0047492c

	return LT_OK;
}


// ======================================================================= //
//  A3.  0x00475910 .. 0x00475968, 89 bytes -- CServerMgr::NotifyCommandChanges
//
//  /O1 ORACLE: server.dll 0x10033fca, 80 bytes.  The nine-byte difference is
//  entirely revision codegen: 8447 writes `imul %ecx,%ecx,0xff` and
//  `movzbl` where 8168 writes `shl $0x8; sub` and `xor %eax,%eax; mov ...,%al`
//  (the zero-extension removal, which only 8447 performs), and 8447
//  compares the state byte against memory (`cmpb $0x0,...`) where 8168 loads
//  it into %cl first.  No source difference.
//
//  servermgr.h line 1683 ALREADY DECLARES IT, and smgr.cpp line 9253 already
//  calls it -- only the definition was missing.  ONE THING IN THAT
//  DECLARATION IS WRONG: `uint32 nChanged` must be `int`.  Both the entry
//  guard (0x0047591c `jle`, server 0x10033fd4 `jle`) and the back edge
//  (0x00475960 `jl`, server 0x10034012 `jl`) are SIGNED; a uint32 bound
//  against a uint32 counter gives `jbe`/`jb`, and a mixed int/uint compare
//  promotes to unsigned.  Until servermgr.h is changed the definition has to
//  hang off a view, which is what is done here.
//
//  THE COMMAND-STATE INDEX.  0x00475928 reads Client+0x3a8 (servermgr.h's
//  m_nCurCommands), 0x00475935 multiplies it by 255 and 0x0047593e indexes
//  Client+0x1a8 -- servermgr.h's `uint8 m_CommandStates[2][MAX_CLIENTCOMMANDS]`
//  with MAX_CLIENTCOMMANDS 0xff, which is exactly the 0x1a8..0x3a5 the pad
//  above m_nCurCommands leaves.
//
//  IT IS AN if/else, not two statements: the shell pointer and its vptr are
//  loaded ONCE, above the branch (0x00475947/0x0047594d), and each arm is a
//  single indirect call (slots 0x28 and 0x2c = IServerShell::OnCommandOn and
//  OnCommandOff, counted off src/LT2/sdk/inc/iservershell.h with the virtual
//  destructor at slot 0).  The shell is re-loaded EVERY iteration, so the
//  source reaches it through m_ClassMgr.m_pServerShell inside the loop.
// ======================================================================= //

class CServerMgrCmdViewQ474
{
public:
	void NotifyCommandChanges(Client* pClient, uint8* pChanged, int nChanged);

	uint8         m_pad_000[0x820];
	IServerShell* m_pServerShell;            // 0x820  (0x00475947)
};

void CServerMgrCmdViewQ474::NotifyCommandChanges(Client* pClient,
                                                 uint8* pChanged,
                                                 int nChanged)
{
	int i;

	for (i = 0; i < nChanged; i++)
	{
		if (pClient->m_CommandStates[pClient->m_nCurCommands][pChanged[i]])
			m_pServerShell->OnCommandOn((void*)pClient, pChanged[i]);
		else
			m_pServerShell->OnCommandOff((void*)pClient, pChanged[i]);
	}
}


// ======================================================================= //
//  A3b. server 0x100345b6, 170 bytes -- sm_CreateNewID
//
//  THE NAME IS SHIPPED: the REPORT_ERROR it raises pushes 0x10055158 =
//  "sm_CreateNewID" with code "1" (0x10051128) and the lone-space
//  description (0x10051114).  spritecontrol.cpp's sm_ClaimObjectID calls it
//  from both of its grow paths (it called it sm_GrowObjectIDMap).  It sits
//  in smgr.obj between 0x10034521 and A4 (0x10034688), the same place its
//  lithtech twin 0x00476150 sits before A4's 0x004763a0.
//
//  One more id: a record out of the id-record bank at 0x10058bc0 (the
//  0x1000ed6f forward; S:0x1000e5a2's sibling), numbered with the current
//  count, put at the head of the free list 0x8c4, and a zeroed entry
//  appended to the object map.
//
//  lithtech.exe (/O2) expands both the bank's allocate and Insert2 inline.
//  Both are lithtech-only here (#ifndef LT_SERVER_BUILD): an inline
//  Allocate body and the real collection.h CMoArray<CObjectMapEntry> for
//  the map.  Identical on both images.
// ======================================================================= //

#ifndef SMGR_CPP_Q474      // <- stand-in only

// spritecontrol.cpp's CObjIDRec: the link is the record, +0x08 the index.
class CObjIDRecQ474
{
public:
	LTLink* m_pPrev;                         // +0x00
	LTLink* m_pNext;                         // +0x04
	uint32  m_nIndex;                        // +0x08
};

// 0x10058bc0 / lithtech 0x004b17a8 -- the id-record bank: smgr.cpp's
// CObjQueueBank / g_ObjQueueBank, under its real names so the reference
// here is the same symbol smgr.obj's is.  /O1 calls the forward (0x1000ed6f); /O2 expands it and
// sb_Allocate with it (0x004761af..).  DECLARED ONLY on server.dll --
// measured: an in-class body, or an out-of-class `inline` one placed after
// this function, is expanded at /O1 as well (`push $bank+4 ; call
// sm_sb_Allocate`, 175 bytes against 170), the same split servermessage.cpp's
// CXferFileBank records.  So the body is lithtech-only.
class CObjQueueBank
{
public:
#ifdef LT_SERVER_BUILD
	LTLink* Allocate();                      // 0x1000ed6f
#else
	// lithtech 0x004761af..0x004761de: the forward and sb_Allocate both
	// expanded, which only an inline body gives -- and only lithtech may
	// have one (see above).
	LTLink* Allocate() { return (LTLink*)sm_sb_AllocateQ474(&m_Bank); }
#endif

	uint8      m_vptr[4];
	StructBank m_Bank;                       // +0x04
};

extern CObjQueueBank g_ObjQueueBank;       // 0x10058bc0

#endif  // SMGR_CPP_Q474

int sm_CreateNewID(CServerMgr* pMgr, void** ppRec)
{
	CObjIDRecQ474*  pRec;
	CObjectMapEntry entry;

	if (pMgr->m_Unknown8bc + 1 >= 0xfff)                     // S:0x100345c6
		RETURN_ERROR_Q474(1, "sm_CreateNewID", 1);

	*ppRec = LTNULL;                                         // S:0x1003460a

	pRec = (CObjIDRecQ474*)g_ObjQueueBank.Allocate();       // S:0x1003460d
	pRec->m_nIndex = pMgr->m_Unknown8bc;                     // S:0x10034625
	pMgr->m_Unknown8bc++;                                    // S:0x10034628

	dl_Insert(&pMgr->m_Link8c4, (CheapLTLink*)pRec);         // S:0x10034634

	entry.m_nFlags  = 0;                                     // S:0x10034643
	entry.m_pRecord = LTNULL;                                // S:0x10034647
	pMgr->m_Unknown8fc.Append(entry);                        // S:0x10034652

	*ppRec = pRec;                                           // S:0x10034657
	return 0;
}


// ======================================================================= //
//  A4.  0x004763a0 .. 0x00476429, 138 bytes -- sm_CheckObjectIntegrity
//
//  THE NAME IS SHIPPED: 0x00476413 pushes 0x004ae2d0 = "sm_CheckObjectIntegrity",
//  and 0x004a7360 = "LT_ERROR", so the macro is de_error.h's
//  RETURN_ERROR_SYM_DESC at level 1 with code LT_ERROR and the object's class
//  name as the description.
//
//  /O1 ORACLE: server.dll 0x10034688, 122 bytes.  Same instruction sequence;
//  the sixteen bytes are `movzx`-vs-`xor/mov %cx`, `cmpb $0x1,mem` vs
//  `mov mem,%bl; cmp $0x1,%bl`, and the duplicated epilogue /O2 keeps where
//  /O1 tail-merges into a `jmp`.
//
//  src/engine/server/saveio.cpp line 606 ALREADY DECLARES IT, as
//  `void sm_Sub34688(SavServerMgr*)`, and calls it three times; savobj.cpp
//  line 231 names the client-side twin 0x00433d50.  It returns LTRESULT, not
//  void -- both exits set %eax (0x004763e4 `xor`, 0x00476423 `mov $0x1`) --
//  so the saveio declaration is the one that is wrong, harmlessly (the
//  callers discard it).
//
//  ONE `||`, NOT TWO GUARDS.  The two failing tests (0x004763d2 and
//  0x004763d9) share ONE error block, and that block sits AFTER the success
//  return (0x004763e7), which reads as a single OR-merged
//  condition: "a shared error block *after* the body -- one `if (A || B)`".
//  Two separate `if (...) RETURN_ERROR` statements would host it between the
//  checks.
//
//  THE HEAD IS A NAMED LOCAL: `lea 0x8e0(%ecx),%edi` at 0x004763ad computes
//  it into a fresh callee-saved register and keeps it for the whole walk,
//  which is distinct from the inline `&pMgr->m_List8dc.m_Head`
//  spelling (that lets VC6 destroy the base with `add`).  The cursor is
//  loaded first (0x004763a7), the head second -- but the SOURCE names the
//  head first and reads the cursor through it: `pCur = pMgr->...m_pNext`
//  before `pHead = &...` swaps the head (%edx at server 0x10034694) with the
//  hoisted m_pEntries (%edi at 0x1003469e).  Lithtech is identical either way.
// ======================================================================= //

LTRESULT sm_CheckObjectIntegrityQ474(CServerMgr* pMgr)
{
	LTLink* pCur;
	LTLink* pHead;

	pHead = &pMgr->m_List8dc.m_Head;                         // 0x004763ad
	pCur  = pHead->m_pNext;                                  // 0x004763a7

	while (pCur != pHead)
	{
		LTObject* pObject = (LTObject*)pCur->m_pData;        // 0x004763bd

		// 0x004763b7 hoists m_pEntries; 0x004763c2 is the uint16 id;
		// 0x004763c9 is the entry's first BYTE and 0x004763d4 its +0x4.
		if (pMgr->m_Unknown8fc.m_pArray[pObject->m_ObjectID].m_nFlags != 1 ||
		    !pMgr->m_Unknown8fc.m_pArray[pObject->m_ObjectID].m_pRecord)
		{
			RETURN_ERROR_SYM_DESC_Q474(1, "sm_CheckObjectIntegrity", LT_ERROR,
			                           pObject->m_pSData->m_pClass->m_pClassName);
		}

		pCur = pCur->m_pNext;                                // 0x004763db
	}

	return LT_OK;                                            // 0x004763e4
}


// ======================================================================= //
//  PART B -- cmodelload.obj  (src/engine/lithtech/cmodelload.cpp)
//
//  These two are the OT_SPRITE and OT_CONTAINER entries of the create-hook
//  table cmodelload.cpp already documents at line 1064.  WHAT PLACES THEM
//  IS THE TABLE, not the string pool: the dispatcher at 0x00477d60 indexes
//  0x004ae340 by the object's +0xbc type byte with a stride of EIGHT and
//  calls slot 0; 0x00477d90 calls slot 1.  Reading .data at that stride:
//
//      type 1 (OT_MODEL)      slot0 0x004778a0   slot1 0x00477b60
//      type 2 (OT_WORLDMODEL) slot0 0x00477b80   slot1 0
//      type 3 (OT_SPRITE)     slot0 0x00477ce0   slot1 0        <-- B1
//      type 9 (OT_CONTAINER)  slot0 0x00477d20   slot1 0        <-- B2
//
//  (0x004ae358 and 0x004ae388 are exactly 0x30 apart, which is what the
//  enumeration's two `ptr` provenances are: slot 0 of types 3 and 9.  BOTH
//  ADDRESSES WERE READ BACK AS LITTLE-ENDIAN TEXT FIRST.
//  0x00477ce0 is "\xe0|G\0", which is not printable, so it cannot be a
//  string tail.  0x00477d20 IS printable -- " }G\0" -- so it had to be
//  cleared the other way: the eight bytes of .data BELOW 0x004ae388 are all
//  zero (0x004ae380..0x004ae387), so there is no string whose last three
//  characters those could be, and the dword sits four-byte aligned inside a
//  run the dispatcher at 0x00477d60 indexes by construction.  Neither row is
//  an ASCII accident.)
//
//  The four arguments and their ORDER are fixed by 0x00477d76..0x00477d85,
//  which forwards its own (a1,a2,a3,a4) unchanged: (pClientMgr, pObject,
//  pCreateRequest, dResult).  Both bodies below leave a4 unread except as
//  the guard, exactly as 0x00477b80 does.
// ======================================================================= //

// lithtech.exe only: cmodelload.obj is client code with no server.dll twin,
// so the server build does not compile Part B (its callees are client-only).
#ifndef LT_SERVER_BUILD

#ifndef CMODELLOAD_CPP_Q474    // <- stand-in only; delete on paste

class CClientMgrQ474;

class CFileIdentQ474
{
public:
	uint32 m_nFileType;
	char*  m_pFilename;
	uint16 m_nLocalID;
};

class ObjectCreateStructQ474
{
public:
	uint16 m_ObjectType;                                       // 0x00
	uint16 m_ContainerCode;                                    // 0x02
};

class CClientCreateRequestQ474
{
public:
	CFileIdentQ474          m_ObjectFile;                      // 0x00
	CFileIdentQ474          m_SkinFiles[2];                    // 0x0c
	ObjectCreateStructQ474* m_pStruct;                         // 0x24
};

class CSpriteQ474;

class CSpriteTrackerQ474
{
public:
	uint8 m_pad_00[0x14];
};

// 0x00476e80 -- cmodelload.cpp's own cm_LoadSprite.
LTRESULT cm_LoadSpriteQ474(CClientMgrQ474* pMgr, CFileIdentQ474* pIdent,
                           CSpriteQ474** ppSprite);
// 0x00483cc0 -- clientmgr.h line 540.
void sub_483cc0Q474(CSpriteTrackerQ474* pRec, void* pSprite);

class CClientModelInstQ474
{
public:
	uint8              m_pad_000[0x1a8];
	CSpriteTrackerQ474 m_SpriteTracker;                        // 0x1a8
	uint8              m_pad_1bc[0x270 - 0x1bc];
	uint16             m_nUnknown270;                          // 0x270
};

// 0x00477b80 -- cmodelload.cpp's cm_WorldModelExtraInitQ474.
LTRESULT cm_WorldModelExtraInitQ474(CClientMgrQ474* pMgr,
                                    CClientModelInstQ474* pObject,
                                    CClientCreateRequestQ474* pRequest,
                                    LTRESULT dResult);

#endif  // CMODELLOAD_CPP_Q474


// ======================================================================= //
//  B1.  0x00477ce0 .. 0x00477d15, 54 bytes -- the SPRITE type's create hook
//
//  It has NO prologue at all and still passes a pointer into its own frame:
//  `lea 0xc(%esp),%eax` at 0x00477ce8 takes the address of the THIRD
//  ARGUMENT'S HOME SLOT, which the instruction two before it has just read
//  into %ecx and which nothing reads again.  That is the dead-
//  parameter-slot local (`ci_GetTcpIpAddress` is the worked example), so the
//  sprite pointer is an ordinary local and no `sub %esp` is needed.
//
//  ONE `ret`, TWO RETURNS.  The `jne 0x477d15` at 0x00477cf9 goes to a bare
//  `ret` with %eax already carrying the result, and the fall-through ends
//  `xor %eax,%eax` immediately above it: two `return` statements whose
//  epilogues (nothing to restore) tail-merge.  Written as a positive guard
//  the store of zero would be the ONLY producer and the early value would
//  have to be materialised somewhere; it is not.
//
//  The second argument to cm_LoadSprite is the request's FIRST field, so the
//  `&pRequest->m_ObjectFile` of the source is a bare register move at
//  0x00477ce0 with no `add` -- the same thing cm_InitSkin's caller does.
// ======================================================================= //

LTRESULT cm_SpriteExtraInitQ474(CClientMgrQ474* pMgr,
                                CClientModelInstQ474* pObject,
                                CClientCreateRequestQ474* pRequest,
                                LTRESULT dResult)
{
	CSpriteQ474* pSprite;
	LTRESULT     dRes;

	dRes = cm_LoadSpriteQ474(pMgr, &pRequest->m_ObjectFile, &pSprite);

	if (dRes != LT_OK)                                   // 0x00477cf7
		return dRes;

	sub_483cc0Q474(&pObject->m_SpriteTracker, pSprite);  // 0x00477d0b

	return LT_OK;                                        // 0x00477d13
}


// ======================================================================= //
//  B2.  0x00477d20 .. 0x00477d59, 58 bytes -- the CONTAINER type's create hook
//
//  A container IS a world model plus a code, so the body forwards to the
//  WORLDMODEL hook (0x00477d3b -> 0x00477b80) with a LITERAL LT_OK as its
//  fourth argument -- `push $0x0` at 0x00477d36, not `push %eax`, even
//  though %eax has just been proved zero -- and then copies the create
//  struct's m_ContainerCode (ltbasedefs.h's uint16 at ObjectCreateStruct+0x02,
//  read at 0x00477d4a) onto the object at +0x270.
//
//  THE GUARD IS POSITIVE.  0x00477d28 `jne` skips FORWARD to 0x00477d55, a
//  block past the body's own `return dRes` exit at 0x00477d57 --
//  a conditional jump forward to a block sitting past every other
//  return means that block is a trailing statement and the guard above it is
//  positive.  Its sibling 0x00477b80 is the other spelling and shows the
//  contrast directly: there the early return FALLS THROUGH (0x00477b8b `je`
//  skips it) and carries a duplicated epilogue.
// ======================================================================= //

LTRESULT cm_ContainerExtraInitQ474(CClientMgrQ474* pMgr,
                                   CClientModelInstQ474* pObject,
                                   CClientCreateRequestQ474* pRequest,
                                   LTRESULT dResult)
{
	if (dResult == LT_OK)                                // 0x00477d25
	{
		LTRESULT dRes = cm_WorldModelExtraInitQ474(pMgr, pObject,
		                                           pRequest, LT_OK);

		if (dRes != LT_OK)                               // 0x00477d43
			return dRes;

		pObject->m_nUnknown270 = pRequest->m_pStruct->m_ContainerCode;
	}

	return LT_OK;                                        // 0x00477d55
}

#endif  // !LT_SERVER_BUILD
