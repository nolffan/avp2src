// lithtech.exe 0x00464000..0x00468000 — the rows still
// untouched in that window, reconstructed.  HOLDING PEN, not a shipped TU: the
// MOVE-ME banner over each function names the file it belongs in.  Every new
// symbol carries the Q464 tag so this object cannot collide at link time.
//
// GROUND TRUTH is the retail lithtech.exe for the /O2 bytes and
// the retail server.dll for the /O1 bytes of the SAME source.  Having both
// oracles is what makes these readable at all: /O1 keeps the calls that /O2
// expands, so the server image names the callees and the client image gives
// the shape to match.  Every address comment below is one or the other and
// says which image it is from by its value (0x004xxxxx = lithtech.exe,
// 0x100xxxxx = server.dll).
//
// WHAT WAS IN THE WINDOW, and the honest split:
//
//   0x004641a0  251   sub_100273e9        NEW      — written below
//   0x00464640  170   sm_ClientExitWorld  written  — smgr.cpp, /O2 shape work
//   0x00464ed0  134   sm_VisCallbackA     smgr.cpp, matching both images (moved)
//   0x00465610 2186   sm_WriteClientUpdate written — smgr.cpp, ditto
//   0x00466550  186   snd_FindSentState   NEW      — written below
//   0x004669aa  370   NOT A FUNCTION      — the fall-through of __chkstk; the
//                                           real start is 0x004669a0, which is
//                                           cc_ObjectInfo.  MOVED OUT to
//                                           src/engine/server/console.cpp and
//                                           byte-identical in BOTH images
//                                           (0x004669a0 380, 0x10028be3 356),
//                                           with oi_FindClass beside it
//                                           (0x00466b20 / 0x10028d47)
//   0x00466bd0  597   cc_SpawnObject      console.cpp, matching both images (moved)
//   0x00467540  174   EXTENT WRONG        — the real function is
//                                           0x00467540..0x00467eec (2,476
//                                           bytes) = sub_100291c7; written
//                                           below, with the caveat in its
//                                           own banner
//
// Two more, found while identifying 0x004669a0 and taken because they are
// three lines each.  Both are under the scan's 120-byte floor and so
// were never in the enumeration, and both are unclaimed:
//
//   0x00466870   42   cc_Portal           — in server/console.cpp
//   0x00466b90   52   cc_DisableWMPhysics — in server/console.cpp (the
//                                           decoded-map row naming it
//                                           lt2new1 sub_466b90 is wrong)
//
// BUILD NOTE: compile with -I src/engine/server (this file includes
// servermgr.h for CServerMgr, StructBank, the dhash entry points and dfree).

#include <string.h>
#include <stdlib.h>          // atoi

#include "servermgr.h"
#include "servermsg.h"       // ServerMessage, LMessageImpl, IMessageWrite, CPosRef
#include "servermodel.h"     // the CF_* change-flag names
#include "packet.h"          // CPacket::WriteType, for sm_DetachClientQ464
#include "de_error.h"       // dsi_ServerError


// ======================================================================= //
//  MOVE-ME: src/engine/server/smgr.cpp, immediately under the declaration
//  at smgr.cpp:6855 (`void sub_100273e9(CServerMgr*, Client*);`), which
//  sm_RemoveClient's last statement already calls.
//
//  0x100273e9 / 0x004641a0 — release the client record itself.
// ======================================================================= //

// 0x1000e5c5 / 0x0042fc80 — svnew.cpp's xfer_Term, already reconstructed
// there.  Client+0x18c is therefore a CClientFilesView*: the per-client file
// transfer state.  Declared by address rather than by including svnew.cpp's
// view, because nothing here needs its layout.
void xfer_TermQ464(void* pState);                      // 0x0042fc80

// 0x10031d47 / 0x00472f60 — CQueuedMsg::Release, already declared by
// servermgr.h at that exact address.  Repeated as a view because the shipped
// record is much bigger than servermgr.h's two-field CQueuedMsg (the release
// touches +0x140, +0x144, +0x148 and +0x150) and nothing here needs those.
class CQueuedMsgQ464
{
public:
	void Release();                                     // 0x00472f60
};

// Slot 2 of whatever a ClientPacketSet holds.  The shipped teardown is
// `mov (%ecx),%edx ; call *0x8(%edx)` with nothing pushed and no cleanup, so
// it is a thiscall virtual in the third slot and takes no arguments.  Two
// unnamed slots in front of it are all that is needed to place it.
class CPacketQ464
{
public:
	virtual void _v00() = 0;
	virtual void _v04() = 0;
	virtual void Release() = 0;                         // slot 2
};

// servermgr.h's ClientPacketSet PLUS THE DESTRUCTOR IT IS MISSING.  This is
// the one real layout finding in the function: both images destroy the 2x2
// array at Client+0x134 BACKWARDS, four trips, stride 12
// (0x004641 71..0x0046428c, and out of line at 0x100274bc), which is the
// array-destructor idiom and nothing else.  Nothing anywhere else in the
// image needs ~ClientPacketSet, so declaring it in servermgr.h moves no
// matched byte -- but servermgr.h is left unchanged, so the whole
// Client view is local here.
class ClientPacketSetQ464
{
public:
	ClientPacketSetQ464() { m_pPacket = LTNULL; }
	~ClientPacketSetQ464()
	{
		if (m_pPacket)
			m_pPacket->Release();
	}

	CPacketQ464* m_pPacket;                             // +0x00
	uint32       m_Unknown04;
	uint32       m_Unknown08;
};

// The client record, at the offsets both images read.  servermgr.h's Client
// is the same class; this copy exists only so ClientPacketSetQ464 above can
// carry its destructor, because `delete pClient` is what the tail of this
// function is and the destructor has to be in scope for it.
class ClientQ464
{
public:
	uint8               m_pad_000[0x110];
	void*               m_pClientData;                  // 0x110  net_Free'd
	uint8               m_pad_114[0x134 - 0x114];
	ClientPacketSetQ464 m_PacketSets[2][2];             // 0x134
	uint8               m_pad_164[0x18c - 0x164];
	void*               m_pXferState;                   // 0x18c  CClientFilesView*
	CObjInfo*           m_pObjInfos;                    // 0x190
	ClientIDList        m_IDLists[2];                   // 0x194  0x198 / 0x1a0
	uint8               m_pad_1a4[0x3c4 - 0x1a4];
	char*               m_pName;                        // 0x3c4
	uint8               m_pad_3c8[0x3cc - 0x3c8];
	LTList              m_List3CC;                      // 0x3cc  head 0x3d0
	void*               m_hSoundHash;                   // 0x3dc
};

// 0x100273e9 (211 bytes, /O1) / 0x004641a0 (251 bytes, /O2) — the last thing
// sm_RemoveClient does: everything the client record owns, then the record.
//
// The manager parameter is ACCEPTED AND NEVER READ.  Both images load only
// the second argument (`mov 0xc(%ebp),%esi` at 0x100273ee, `mov 0x14(%esp),
// %edi` at 0x004641a3) and sm_RemoveClient's call site pushes two
// (`push %esi ; push %edi ; call 0x004641a0 ; add $0x8,%esp`, 0x0046416b..
// 0x0046418e).  The declaration already in smgr.cpp has the pair, so this is
// only confirmation that it is right and that the first one is dead.
//
// THE FREES ARE TWO DIFFERENT FUNCTIONS and the split is the shape:
// 0x1000d398 / 0x00429080 is `dfree` and 0x10028ace / 0x004290c0 is the
// one-call wrapper in front of it that servermgr.h calls `net_Free` and that
// smgr.cpp:7452 identifies as the image's `operator delete`.  The four
// engine-allocated buffers go through dfree; the client data block goes
// through net_Free; and the record itself goes through `delete`, which is
// why its free lands on the wrapper too.
//
// The five cdecl calls between the message loop and the hash walk share ONE
// `add $0x14,%esp` in both images (0x10027457, 0x00464213), which is cl 8447
// batching consecutive fixed-prototype cleanups -- it says nothing about the
// prototypes and nothing about statement grouping.
//
// The hash walk frees each record back onto CServerMgr::m_Bank89C by hand
// -- `*(void**)pRec = pBank->m_pFreeList ; pBank->m_pFreeList = pRec`,
// expanded at 0x10027476 and 0x00464235 -- which is servermgr.h's inline
// sb_Free, already used this way across the file.  snd_FindSentState below
// is where those records come from.
//
// The advance is its own statement, as in console.cpp's cc_ShowUsedFiles:
// nested inside the GetData call, VC6's right-to-left ordering would hoist
// it and the shipped batching is lost.
// NOT COMPILED: a duplicate of smgr.cpp's sub_100273e9, which is the copy
// lithtech.exe links (both place at 0x004641a0; the shipped image has one).
#if 0
void sub_100273e9Q464(CServerMgr* pMgr, ClientQ464* pClient)
{
	LTLink* pCur;
	void*   hIter;
	void*   hElement;

	xfer_TermQ464(pClient->m_pXferState);

	pCur = pClient->m_List3CC.m_Head.m_pNext;

	while (pCur != &pClient->m_List3CC.m_Head)
	{
		LTLink* pNext = pCur->m_pNext;

		((CQueuedMsgQ464*)pCur->m_pData)->Release();

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

	net_Free(pClient->m_pClientData);

	delete pClient;
}
#endif


// ======================================================================= //
//  MOVE-ME: DONE, AND THIS COPY DELIBERATELY KEPT. The body below now also
//  lives in src/engine/server/smgr.cpp as sub_100291c7, where it reaches
//  server.dll for the first time -- smgr.obj had carried it as an undefined
//  external while the declaration and sm_WriteObjectUpdate's call sat there
//  waiting. On the /O1 side it lands at 1096 bytes against 1090, three
//  mnemonic blocks.
//
//  This copy is NOT deleted because it is the /O2-tuned one and ccobj.obj
//  still matches against it; removing it is a separate change that has to be
//  measured on its own. Keep the two in step until then.
//
//  (original note follows)
//  MOVE-ME: src/engine/server/smgr.cpp, under the declaration at
//  smgr.cpp:8542 (`CSoundSentState* snd_FindSentState(Client*, uint16);`).
//
//  0x100288be / 0x00466550 — this client's record for one sound, or a new
//  one.
// ======================================================================= //

// smgr.cpp's CSoundSentState, repeated here because this file cannot include
// it.  Ten bytes, the size sb_Init2(&m_Bank89C, 10, 0x20, 0x20) gives them.
class CSoundSentStateQ464
{
public:
	uint8  m_ChangeFlags;                               // +0x00
	uint8  m_pad_01[0x02 - 0x01];
	uint16 m_Flags;                                     // +0x02
	uint8  m_Unknown04;                                 // +0x04
	uint8  m_pad_05[0x06 - 0x05];
	uint16 m_nRange;                                    // +0x06
	uint8  m_nInnerRatio;                               // +0x08
};

// structbank.cpp's sb_AllocateCleared, AS AN INLINE AND WITH `memset`, which
// is the one change this row asks for in existing source.  Two findings, both
// read off the pair of images:
//
//   * server.dll CALLS it (0x100288f3 -> 0x10028933) and lithtech.exe EXPANDS
//     it (0x00466582..0x004665c5, no call but sb_Grow).  That is the same
//     /Os-declines / /Ot-takes split servermgr.h already records for
//     sb_Allocate, and the fix is the same: mark it `inline` and move it
//     beside sb_Allocate.  /O1 still emits the COMDAT, so 0x10028933 does not
//     move.
//   * the fill is `memset`, not a named engine routine.  structbank.cpp calls
//     0x1003e280 `lt_MemSet` and says it "returns its fill VALUE rather than
//     its destination".  IT DOES NOT: the normal-path `mov 0x8(%esp),%eax` at
//     0x1003e2cd is taken with %edi ALREADY PUSHED (0x1003e292), so it is the
//     same `0x4(%esp)` the zero-length path returns at 0x1003e2d3 -- the
//     destination, which is what memset returns.  0x1003e280 is the CRT
//     memset, /O1 has no /Oi so it stays a call, and /O2 does, which is why
//     lithtech.exe has the `rep stosl ; rep stosb` pair at 0x004665b9 and no
//     call at all.  Spelling it `lt_MemSet` costs the client the intrinsic.
// sb_Grow is servermgr.h's own declaration (0x1003e1e0 / 0x00490c20).

inline void* sb_AllocateClearedQ464(StructBank* pBank)
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

// 0x100288be (117 bytes, /O1) / 0x00466550 (186 bytes, /O2).
//
// THE KEY IS THE PARAMETER'S OWN ADDRESS.  Both images pass `lea 0x10(%esp)`
// / `lea 0x14(%esp)` as the hash key (0x100288c4, 0x0046655d) and that slot
// is the uint16 parameter itself -- there is no local to copy it into.  So
// the source hands the hash `&nSoundID` with a length of two, twice, and the
// key type is the sound id smgr.cpp:8481 already names.
//
// The redundant null test is real and in the source: the allocate's own
// failure arm and the caller's `if (!pState)` BOTH land on the same return-0
// block (0x004665aa and 0x004665c7 -> 0x004665c9), and VC6 scheduled the
// second test INTO the middle of the inline memset (between the `rep stosl`
// and the `rep stosb`, 0x004665c3).  Written without the second test the
// shipped `test %esi,%esi` there has no source.
//
// THE FAILURE PATH FREES BANK MEMORY WITH dfree, and that is a shipped bug
// rather than a misread: 0x004665ed / 0x1002891c is the plain allocator free
// (0x00429080 / 0x1000d398), not sb_Free, on a record that came out of
// m_Bank89C two statements earlier.  sub_100273e9 above shows how these
// records are SUPPOSED to go back.  Transcribed as shipped.
//
// The `push %esi` at 0x004665ea is hoisted out of both arms -- it is the
// second argument of dhash_SetData on one side and the only argument of
// dfree on the other -- which is allocation, not a shared statement.
CSoundSentStateQ464* snd_FindSentStateQ464(ClientQ464* pClient, uint16 nSoundID)
{
	void*                hElement;
	CSoundSentStateQ464* pState;

	hElement = dhash_FindElement((CHashTable*)pClient->m_hSoundHash,
		&nSoundID, sizeof(nSoundID));

	if (hElement)
	{
		pState = (CSoundSentStateQ464*)dhash_GetData(hElement);

		if (pState)
			return pState;
	}

	pState = (CSoundSentStateQ464*)
		sb_AllocateClearedQ464(&g_pServerMgr->m_Bank89C);

	if (!pState)
		return LTNULL;

	// All three fields dirty: smgr.cpp's SNDSENT_FLAGS | SNDSENT_BYTE04 |
	// SNDSENT_RANGE.  Spelled 7 here only because those macros live in
	// smgr.cpp and this file cannot see them.
	pState->m_ChangeFlags = 7;

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


// ======================================================================= //
//  MOVE-ME: src/engine/server/smgr.cpp, under the declaration at
//  smgr.cpp:8282 (`void sub_100291c7(CServerMgr*, Client*, LTObject*,
//  CObjInfo*, ServerMessage*);`) that sm_WriteObjectUpdate already calls.
//
//  0x100291c7 (1,090 bytes, /O1) / 0x00467540..0x00467eec (2,476 bytes,
//  /O2) — write one object's changed state into the update.
//
//  THE ENUMERATED ROW IS WRONG IN LENGTH.  The enumeration gives
//  0x00467540 as 174 bytes; the last instruction of that
//  row branches to 0x004675ee, which is the next enumerated row (2,302
//  bytes, `gap`).  The two are ONE function: 0x00467540..0x00467eec, three
//  `ret`s, no branch target outside it, and a sweep of every e8/e9 in .text
//  finds no call or jump landing anywhere inside it but at 0x00467540.  The
//  split is the enumeration's: the early `return 0` at 0x004675ed is a full
//  epilogue, so the byte after it looks like a function start.
//
//  ITS RETURN TYPE IS int, NOT void.  Both images put 0 in eax on the
//  no-flags path (0x10029265 / 0x004675e7) and 1 on every other
//  (0x10029601 / 0x00467ee7), and the declaration in smgr.cpp says void.
//  Nothing reads the answer at the one call site, which is why the wrong
//  type survived; the declaration has to change with this body.
//
//  BYTE-IDENTITY, HONESTLY: this should land on server.dll's 0x100291c7 as
//  written, because every callee there is an out-of-line call and the /O1
//  body reads statement for statement.  It will NOT land on lithtech.exe's
//  0x00467540 until ServerMessage's writers become the inline
//  CPacket::WriteType they really are -- the /O2 body carries eleven
//  expansions of it, each ending in the "CPacket::WriteType" /
//  "ERROR: Packet write overrun!" REPORT_ERROR (0x004679cc, 0x00467a77,
//  0x00467b26, ...).  That is the same blocker smgr.cpp:4540 already
//  records for sm_SendSFXMessage and is not specific to this function.
// ======================================================================= //

// Already declared in the server headers at these addresses; repeated here
// because this holding-pen file does not include them all.
//   servercommon.h:127   sm_WriteObjectFiles
//   smgr.cpp:8297        sub_100290d7
//   servermsg.h / servermsg.cpp:353..385  the four compressed-codec halves
void sm_WriteObjectFilesQ464(LTObject* pObj, ServerMessage* pMsg);      // 0x10029087
void sub_100290d7Q464(LTObject* pObject, ServerMessage* pMsg);          // 0x100290d7
void lmsg_CompressPosQ464(uint8* pOut, LTVector* pVec, CPosRef* pRef);  // 0x10012ec5
void lmsg_WriteCompPosWordsQ464(IMessageWrite* pMsg, uint8* pData);     // 0x10012f49
void sub_1001307fQ464(LTRotation* pRot, uint8* pOut);                   // 0x1001307f
void lmsg_WriteCompRotationBytesQ464(IMessageWrite* pMsg, uint8* pData);// 0x10013287
void lmsg_WriteCompRotationQ464(LMessageImpl* pMsg, LTRotation* pRot);  // 0x100132da

// The ServerMessage members servermsg.h does not carry.  Each already exists
// under exactly this shape elsewhere in the tree -- servermessage.cpp's
// CMsgWriteView (WriteDWord 0x1000ecd6, WriteString 0x10026137), smgr.cpp's
// CMsgFloatWriteView (WriteFloat 0x10028a35) and CMsgLenView (GetSpaceLeft
// 0x1000e845) -- and they are gathered on one view here only because this
// file cannot reach three different .cpp-local classes.
class CMsgWriteQ464
{
public:
	void WriteDWord(uint32 nValue);          // 0x1000ecd6 / 0x004192e0-class
	void WriteFloat(float fValue);           // 0x10028a35
	void WriteString(char* pStr);            // 0x10026137
	int  GetSpaceLeft();                     // 0x1000e845
};

// A sprite object's own field, the one CF_MODELINFO sends for OT_SPRITE.
// objtypes.h has no sprite class yet; this is the single dword both images
// read at LTObject+0x1bc under `cmpb $0x3` (0x10029381 / the /O2 twin).
class CObjSpriteQ464
{
public:
	uint8  m_pad_000[0x1bc];
	uint32 m_Unknown1bc;                     // 0x1bc
};

// A model object's field at 0x204, sent at the end of the attachment block
// and only for OT_MODEL (0x100295f4).
class CObjModelQ464
{
public:
	uint8  m_pad_000[0x204];
	uint32 m_Unknown204;                     // 0x204
};

// A container's code at 0x270 is objtypes.h's CObjContainer::m_ContainerCode;
// repeated on a view for the same reason as the two above.
class CObjContainerQ464
{
public:
	uint8  m_pad_000[0x270];
	uint16 m_ContainerCode;                  // 0x270
};

// SObjData's two fields ltobject_internal.h does not name at these offsets:
// +0x2c is the stowed SFX ServerMessage (smgr.cpp's sm_SetObjectSFXMessage
// already treats it as one) and +0x4c is a flag word whose bit 0 sets 0x20
// in the info byte (0x100292d7 / 0x00467661).
class CSObjDataQ464
{
public:
	uint8          m_pad_000[0x2c];
	ServerMessage* m_pSFXMessage;            // +0x2c
	void*          m_pModelFile;             // +0x30
	uint8          m_pad_034[0x4c - 0x34];
	uint32         m_Unknown4c;              // +0x4c
	uint16         m_ClientChangeFlags;      // +0x50
	uint16         m_NetFlags;               // +0x52
};

// The file record the file id comes out of: smgr.cpp's CSoundIDRec is the
// same eight-bytes-then-a-word shape, and this is the file manager's.
class CFileIDRecQ464
{
public:
	uint8  m_pad_00[0x08];
	uint16 m_nID;                            // +0x08
};

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
// That is one shared tail, not two separate statements.
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
// NOT COMPILED: a duplicate of smgr.cpp's lithtech-shape sub_100291c7, the
// copy lithtech.exe links for 0x00467540 (the shipped image has one).  Kept
// in the text as the /O2 reference the banner above describes.
#if 0
int sub_100291c7Q464(CServerMgr* pMgr, Client* pClient, LTObject* pObject,
                     CObjInfo* pInfo, ServerMessage* pMsg)
{
	uint16 nFlags;
	uint8  nInfo;
	// TWO buffers, not one: server.dll gives the position seven bytes at
	// -0x10(%ebp) and the rotation six at -0x8(%ebp), and both images size
	// the frame at 0x10 for exactly that pair.  The spellings are
	// servermsg.cpp's own (lmsg_WriteCompPos / lmsg_WriteCompRotation).
	uint8  posData[7];
	uint8  rotData[6];

	nFlags = pInfo->m_ChangeFlags;

	if (!(pClient->m_ClientFlags & CLIENTFLAG_SENDCOBJROTATION) &&
		pClient->m_pUnknown3AC == pObject)
	{
		nFlags &= 0xfbfb;                    // ~(CF_ROTATION | 0x400)
	}

	if (((CSObjDataQ464*)pObject->m_pSData)->m_NetFlags & NETFLAG_POSUNGUARANTEED)
		nFlags &= 0xfffd;                    // ~CF_POSITION

	if (((CSObjDataQ464*)pObject->m_pSData)->m_NetFlags & NETFLAG_ROTUNGUARANTEED)
		nFlags &= 0xfffb;                    // ~CF_ROTATION

	if (pObject->m_ObjectType == OT_MODEL &&
		(((CSObjDataQ464*)pObject->m_pSData)->m_NetFlags & NETFLAG_ANIMUNGUARANTEED))
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

	if (nFlags & 0x700)
	{
		pMsg->WriteByte((uint8)(nFlags | 0x80));
		pMsg->WriteByte((uint8)(nFlags >> 8));
	}
	else
	{
		pMsg->WriteByte((uint8)nFlags);
	}

	pMsg->WriteWord(pObject->m_ObjectID);

	if (nFlags & CF_NEWOBJECT)
	{
		nInfo = (uint8)pObject->m_ObjectType;

		if (((CSObjDataQ464*)pObject->m_pSData)->m_pSFXMessage)
		{
			nInfo |= 0x40;

			if (((CSObjDataQ464*)pObject->m_pSData)->m_pSFXMessage->m_Unknown34
				>= 0x100)
			{
				nInfo |= 0x80;
			}
		}

		if (((CSObjDataQ464*)pObject->m_pSData)->m_Unknown4c & 1)
			nInfo |= 0x20;

		pMsg->WriteByte(nInfo);

		if (((CSObjDataQ464*)pObject->m_pSData)->m_pSFXMessage)
		{
			if (nInfo & 0x80)
			{
				pMsg->WriteWord(((CSObjDataQ464*)pObject->m_pSData)
					->m_pSFXMessage->m_Unknown34);
			}
			else
			{
				pMsg->WriteByte((uint8)((CSObjDataQ464*)pObject->m_pSData)
					->m_pSFXMessage->m_Unknown34);
			}

			pMsg->WriteData(
				((CSObjDataQ464*)pObject->m_pSData)->m_pSFXMessage->m_pData,
				((CSObjDataQ464*)pObject->m_pSData)->m_pSFXMessage->m_Unknown34);
		}

		if (pObject->m_ObjectType == OT_WORLDMODEL)
		{
			((CMsgWriteQ464*)pMsg)->WriteString(pObject->m_pWorldBsp->m_Name);
		}
		else if (pObject->m_ObjectType == OT_CONTAINER)
		{
			((CMsgWriteQ464*)pMsg)->WriteString(pObject->m_pWorldBsp->m_Name);

			pMsg->WriteWord(((CObjContainerQ464*)pObject)->m_ContainerCode);
		}
		else if (pObject->m_ObjectType == OT_MODEL)
		{
			sm_WriteObjectFilesQ464(pObject, pMsg);
		}
		else if (pObject->m_ObjectType == OT_SPRITE)
		{
			pMsg->WriteWord(((CFileIDRecQ464*)((CSObjDataQ464*)
				pObject->m_pSData)->m_pModelFile)->m_nID);
		}
	}

	if (nFlags & CF_MODELINFO)
	{
		if (pObject->m_ObjectType == OT_MODEL)
		{
			sub_100290d7Q464(pObject, pMsg);
		}
		else if (pObject->m_ObjectType == OT_SPRITE)
		{
			((CMsgWriteQ464*)pMsg)->WriteDWord(
				((CObjSpriteQ464*)pObject)->m_Unknown1bc);
		}
	}

	if (nFlags & CF_FLAGS)
	{
		pMsg->WriteWord((uint16)pObject->m_Flags);
		pMsg->WriteByte((uint8)pObject->m_Flags2);
		((CMsgWriteQ464*)pMsg)->WriteDWord(pObject->m_UserFlags);
	}

	if (nFlags & CF_COLORINFO)
	{
		pMsg->WriteByte(pObject->m_ColorR);
		pMsg->WriteByte(pObject->m_ColorG);
		pMsg->WriteByte(pObject->m_ColorB);
		pMsg->WriteByte(pObject->m_ColorA);

		if (pObject->m_ObjectType == OT_LIGHT)
		{
			if (pObject->m_LightRadius < 0.0f)
				pMsg->WriteWord(0);
			else
				pMsg->WriteWord((uint16)pObject->m_LightRadius);
		}
	}

	if (nFlags & CF_SCALE)
	{
		((CMsgWriteQ464*)pMsg)->WriteFloat(pObject->m_Scale.x);
		((CMsgWriteQ464*)pMsg)->WriteFloat(pObject->m_Scale.y);

		if (pObject->m_ObjectType != OT_SPRITE)
			((CMsgWriteQ464*)pMsg)->WriteFloat(pObject->m_Scale.z);
	}

	if (nFlags & (CF_POSITION | 0x200))
	{
		if (pObject->m_Flags & FLAG_FULLPOSITIONRES)
		{
			((CMsgWriteQ464*)pMsg)->WriteFloat(pObject->m_Pos.x);
			((CMsgWriteQ464*)pMsg)->WriteFloat(pObject->m_Pos.y);
			((CMsgWriteQ464*)pMsg)->WriteFloat(pObject->m_Pos.z);

			((IMessageWrite*)&pMsg->m_Write)->WriteVector(&pObject->m_Velocity);
		}
		else
		{
			lmsg_CompressPosQ464(posData, &pObject->m_Pos,
				(CPosRef*)&pMgr->m_World);

			lmsg_WriteCompPosWordsQ464((IMessageWrite*)&pMsg->m_Write,
				posData);

			((IMessageWrite*)&pMsg->m_Write)->WriteCompVector(
				&pObject->m_Velocity);
		}
	}

	if (nFlags & (CF_ROTATION | 0x400))
	{
		if (pObject->m_Flags & FLAG_FULLPOSITIONRES)
		{
			((IMessageWrite*)&pMsg->m_Write)->WriteRotation(
				&pObject->m_Rotation);
		}
		else
		{
			sub_1001307fQ464(&pObject->m_Rotation, rotData);

			lmsg_WriteCompRotationBytesQ464((IMessageWrite*)&pMsg->m_Write,
				rotData);
		}
	}

	if (nFlags & CF_ATTACHMENTS)
	{
		Attachment* pAttach = pObject->m_pAttachments;

		while (pAttach)
		{
			if (((CMsgWriteQ464*)pMsg)->GetSpaceLeft() >= 30)
			{
				pMsg->WriteWord(pAttach->m_nChildID);

				((CMsgWriteQ464*)pMsg)->WriteDWord(pAttach->m_iNode);

				((CMsgWriteQ464*)pMsg)->WriteFloat(pAttach->m_Offset.m_Pos.x);
				((CMsgWriteQ464*)pMsg)->WriteFloat(pAttach->m_Offset.m_Pos.y);
				((CMsgWriteQ464*)pMsg)->WriteFloat(pAttach->m_Offset.m_Pos.z);

				lmsg_WriteCompRotationQ464(&pMsg->m_Write,
					&pAttach->m_Offset.m_Rot);
			}

			pAttach = pAttach->m_pNext;
		}

		pMsg->WriteWord(0xffff);

		if (pObject->m_ObjectType == OT_MODEL)
		{
			((CMsgWriteQ464*)pMsg)->WriteDWord(
				((CObjModelQ464*)pObject)->m_Unknown204);
		}
	}

	return 1;
}
#endif


// 0x00466870 ("Portal") and 0x00466b90 ("DisableWMPhysics"), the last two
// of g_ConsoleCommands' eight built-ins, are server/console.cpp's cc_Portal
// and cc_DisableWMPhysics, byte-identical on both images from there. The
// copies that used to stand here were removed as duplicates.


// ======================================================================= //
//  MOVE-ME: src/engine/server/servermessage.cpp, between ph_CreateHolder
//  (0x0042fbc0) and ph_GetUnknown74 (0x0042fcd0) -- the address order puts
//  it there, and server.dll has it at 0x1000e5c5 between CClientFiles'
//  constructor and destructor.  server.dll's /O1 body is svnew.cpp's
//  xfer_Term, which CALLS ~CClientFiles (0x1000e5ec); lithtech.exe's /O2
//  expands it, so the destructor chain has to be in-class for this image.
//
//  0x0042fc80 (70 bytes, /O2) / 0x1000e5c5 (39 bytes, /O1).
// ======================================================================= //

// The bank's base: nothing but a vptr (0x004a14c8 is its table, stored back
// at 0x0042fcaf on the way down).
class CGlobalBankBaseQ464
{
public:
	virtual ~CGlobalBankBaseQ464() {}
};

// CClientFiles+0x18, servermessage.cpp's CXferFileBank: its own vptr
// (0x004a22bc), then sb_FreeChunks(&m_Bank, 1), then the base vptr.
class CXferFileBankQ464 : public CGlobalBankBaseQ464
{
public:
	~CXferFileBankQ464() { sb_FreeChunks(&m_Bank, 1); }

	StructBank m_Bank;                                  // +0x04
	uint32     m_Unknown20;                             // +0x20
};

// Only the head of servermessage.cpp's CClientFiles.  The base is
// servermgr.h's CUnknownA14, whose destructor (0x00490fe0) stays a call.
class CClientFilesQ464 : public CUnknownA14
{
public:
	~CClientFilesQ464() {}

	CXferFileBankQ464 m_FileBank;                       // 0x18
};

void xfer_FreeAllFilesQ464(void* pState);               // 0x0042ff60
void xfer_FinishTransferQ464(void* pState);             // 0x0042ffe0

void xfer_TermQ464(void* pState)
{
	xfer_FreeAllFilesQ464(pState);

	xfer_FinishTransferQ464(pState);

	delete (CClientFilesQ464*)pState;
}


// ======================================================================= //
//  MOVE-ME: src/engine/server/smgr.cpp:6117, sm_DetachClient.
//
//  0x00463f00 (349 bytes, /O2) / 0x10027259 (/O1).  smgr.cpp's body (a
//  null `ServerMessage* pMsg` local) is 326 bytes here: VC6 forwards the
//  two header stores into WriteType's loads and folds m_Pos+2 and
//  m_DataLen+2 to the constant 3.  Shipped re-reads 0x38 and 0x34.
// ======================================================================= //

class CClientAttachQ464
{
public:
	uint8  m_pad_00[0x164];
	LTLink m_AttachLink;                                // +0x164
};

LTRESULT sm_DetachClientQ464(CServerMgr* pMgr, Client* pClient)
{
	CMessageRef msg;

	if (pClient->m_pViewClient)
	{
		dl_Remove(&((CClientAttachQ464*)pClient)->m_AttachLink);

		pClient->m_pViewClient = LTNULL;

		if (pClient->m_pUnknown3AC)
		{
			{
				ServerMessage* pMsg = msg.m_pMsg;

				pMsg->m_Unknown38 = 1;
				pMsg->m_Unknown34 = 1;
			}

			((CPacket*)msg.m_pMsg)->WriteType(pClient->m_pUnknown3AC->m_ObjectID);

			sm_SendToClient(pMgr, pClient, 7, msg.m_pMsg, 1, 0x80);
		}
	}

	return LT_OK;
}


