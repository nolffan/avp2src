// server.dll — the ILTSpriteControl implementation (reconstruction). /O1.
// Two check literals name the class; the structures are recovered from the
// offsets (names ours). A sprite instance hangs its resource at +0x1a8 and
// its play position — anim pointer, frame pointer, milliseconds into the
// frame — at +0x1ac/+0x1b0/+0x1b4.
#if defined(PRIMAL_HUNT)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x4a08c0, ahead of
// SpriteControl's code). Primal Hunt's server.dll the same (guard byte
// 0x1008ec84, atexit of 0x1002eaeb; 0x100458e4, after it), so it is not
// limited to the engine's build.
#include <string>
#endif
#include "servermgr.h"
#include "de_error.h"

class SpriteAnim
{
public:
	uint8   m_pad_00[0x20];
	uint32* m_pFrames;                       // +0x20  one dword per frame
	uint32  m_nFrames;                       // +0x24
	uint8   m_pad_28[0x2c - 0x28];
	uint32  m_FrameRate;                     // +0x2c
	uint8   m_pad_30[0x3c - 0x30];
};

class Sprite
{
public:
	uint8       m_pad_00[0x0c];
	SpriteAnim* m_pAnims;                    // +0x0c
	uint32      m_nAnims;                    // +0x10
};

class SpriteInstance
{
public:
	uint8       m_pad_000[0x1b0];
	Sprite*     m_pSprite;                   // +0x1b0
	SpriteAnim* m_pCurAnim;                  // +0x1b4
	uint32*     m_pCurFrame;                 // +0x1b8
	uint32      m_FrameTime;                 // +0x1bc  ms into the current frame
	// +0x1b8 — a plain dword the interface both reads and writes and does
	// nothing else with: the getter copies it out and the setter stores the
	// argument raw, with no range check and no side effect on the play
	// position. Name ours; nothing here says what it holds.
	uint32      m_Unknown1b8;                // +0x1c0
};

// Six slots, and its vftable is at 0x1004e06c -- the six pointers between
// CObjSprite's own last slot at 0x1004e068 and the 363.0f at 0x1004e084 that
// CObjSprite::GetScaledRadius uses. That is what settles the interface's size
// and the order below, which is iltspritecontrol.h's. The methods are
// virtual because that vtable is emitted by objbank.cpp (the sprite's
// constructor installs it) and has to name these bodies; the bodies are
// the same either way.
class SpriteControl
{
public:
	virtual LTRESULT GetNumAnims(uint32& nAnims);                   // 0x10035929  slot 0x00
	virtual LTRESULT GetNumFrames(uint32 iAnim, uint32& nFrames);   // 0x10035940  slot 0x04
	virtual LTRESULT GetCurPos(uint32& iAnim, uint32& iFrame);      // 0x1003599a  slot 0x08
	virtual LTRESULT SetCurPos(uint32 iAnim, uint32 iFrame);        // 0x100359d8  slot 0x0c
	virtual LTRESULT GetUnknown1b8(uint32& val);                    // 0x10035a67  slot 0x10
	virtual LTRESULT SetUnknown1b8(uint32 val);                     // 0x10035a7b  slot 0x14

public:
	SpriteInstance* m_pInstance;             // +0x04
};

// The two error reports name the interface differently per image: server.dll
// has "ILTSpriteControl::GetNumFrames" / "ILTSpriteControl::SetCurPos" and
// lithtech.exe "SpriteControl::GetNumFrames" / "SpriteControl::SetCurPos"
// (the SetCurPos names are pushed at 0x10035a4c and 0x00483ee9, the
// GetNumFrames one in lithtech at 0x00483dd6). Relocation masking hides the literal, so the per-function
// match never saw it; the code is otherwise one body for both targets.
#ifdef LT_SERVER_BUILD
#define SPRITECONTROL_NAME(fn) "ILTSpriteControl::" fn
#else
#define SPRITECONTROL_NAME(fn) "SpriteControl::" fn
#endif

// 0x10035929 — slot 0x00. No bounds check to make, so no error arm.
LTRESULT SpriteControl::GetNumAnims(uint32& nAnims)
{
	nAnims = m_pInstance->m_pSprite->m_nAnims;
	return LT_OK;
}

// 0x10035940 -- slot 0x04. No Sprite* local: the resource is read through
// the instance at both uses. With `Sprite* pSprite = m_pInstance->m_pSprite`
// the subscript came out offset-first, `0x24(%eax,%ecx,1)`, against the
// shipped table-first `0x24(%ecx,%eax,1)` at 0x1003598b; written out twice
// (VC6 CSEs the two reads into one ecx anyway) the roles are the shipped
// ones. Naming the element or the table did not move them.
LTRESULT SpriteControl::GetNumFrames(uint32 iAnim, uint32& nFrames)
{
	if (iAnim >= m_pInstance->m_pSprite->m_nAnims)
		RETURN_ERROR(2, SPRITECONTROL_NAME("GetNumFrames"), LT_INVALIDPARAMS);

	nFrames = m_pInstance->m_pSprite->m_pAnims[iAnim].m_nFrames;
	return LT_OK;
}

LTRESULT SpriteControl::GetCurPos(uint32& iAnim, uint32& iFrame)
{
	SpriteAnim* pCur = m_pInstance->m_pCurAnim;
	iAnim = pCur - m_pInstance->m_pSprite->m_pAnims;
	iFrame = m_pInstance->m_pCurFrame - pCur->m_pFrames;
	return LT_OK;
}

// NEAR MATCH (142 of 143 bytes, one mnemonic block). The success path is
// nested inside both bounds checks with the one RETURN_ERROR after them --
// shipped 0x100359d8 lays the store path first and the report block last,
// which the early-return form inverts. No Sprite* local, as in GetNumFrames:
// with one, the anim address was offset-first (`add 0xc(%edx),%eax; mov
// %eax,%esi`); without, the table is loaded first as shipped. Left: shipped
// loads the table into the dead pSprite register and forms pAnim with `mov
// 0xc(%edx),%edx; lea (%edx,%eax,1),%esi` (0x100359ef); ours loads it
// straight into pAnim's esi and adds (`mov 0xc(%edx),%esi; add %eax,%esi`).
// Coalescing, not source: invariant across a named table local, `p += i`,
// a byte-offset cast, pAnim at function scope, pAnim assigned inside the
// condition or a `&&`/comma test, and inline GetAnim()/Anim()/GetAnims()/
// GetSprite() accessors. Removing pAnim altogether reloads (149 bytes).
LTRESULT SpriteControl::SetCurPos(uint32 iAnim, uint32 iFrame)
{
	SpriteInstance* pInst = m_pInstance;

	if (iAnim < pInst->m_pSprite->m_nAnims)
	{
		SpriteAnim* pAnim = &pInst->m_pSprite->m_pAnims[iAnim];
		if (iFrame < pAnim->m_nFrames)
		{
			pInst->m_pCurAnim = pAnim;
			m_pInstance->m_pCurFrame = m_pInstance->m_pCurAnim->m_pFrames + iFrame;
			m_pInstance->m_FrameTime = (iFrame * 1000) / pAnim->m_FrameRate;
			return LT_OK;
		}
	}
	RETURN_ERROR(2, SPRITECONTROL_NAME("SetCurPos"), LT_INVALIDPARAMS);
}


// 0x10035a67 and 0x10035a7b — slots 0x10 and 0x14, the read and the write of
// the dword at +0x1b8. The setter takes no bounds check and touches nothing
// else, which is the whole reason the field could not be named.
LTRESULT SpriteControl::GetUnknown1b8(uint32& val)
{
	val = m_pInstance->m_Unknown1b8;
	return LT_OK;
}

LTRESULT SpriteControl::SetUnknown1b8(uint32 val)
{
	m_pInstance->m_Unknown1b8 = val;
	return LT_OK;
}


// SoundInstance::Init and UpdateTimer, which used to be parked here, are
// soundinst.cpp's: a translation unit of their own just ahead of this one.

// *********************************************************************** //
//  0x10031dfa — sm_ClaimObjectID. Take one slot out of the object-id map's
//  free list and move it to the in-use list, then wipe every client's
//  pending-change entry for that slot so nobody hears about the previous
//  tenant.
//
//  0xffff means "any": the free list is walked for the first record whose
//  0x40000000 bit is clear. A real id instead grows the map until the id is
//  in range and then looks the record up. Either way the map can refuse,
//  and the answer is nonzero.
//
//  Read off the bytes:
//
//    * the walk writes *ppRec on EVERY iteration, not only on the hit — the
//      store is inside the loop body ahead of the test — so a full free list
//      leaves the caller's pointer holding the last record examined before
//      the grow overwrites it.
//    * the "any" path falls out of the loop into the grow, and the hit
//      leaves the loop by a jump into the shared tail; the real-id path
//      reaches the same tail from its own success test. That is one label,
//      not two copies of the tail.
//    * the grow loop for a real id is a rotated `while`: the range test is
//      duplicated, once as the entry guard (`jb`) and once as the back edge
//      (`jae`).
//    * `*ppRec` is re-read from memory at every use in the tail — four
//      times, including once per client — rather than cached in a local.
//    * `&pClient->m_pObjInfos[index]` IS a named local: both stores fold the
//      index back into their own addressing and the shipped code is left
//      with a dead `lea (%esi,%ecx,4),%ecx` at the bottom of the loop, which
//      is the pointer-local fingerprint.
//    * the client list's head is a NAMED LOCAL, not the bound written out
//      inline. Both spellings walk the same list, but `&pMgr->m_ClientList
//      .m_Head` written in the loop condition lets VC6 destroy the manager's
//      register in place (`add $0x7e8,%esi`) while `LTLink* pHead =
//      &pMgr->m_ClientList.m_Head;` makes it compute the address into a
//      fresh register with `lea`, which is what the shipped code does and
//      which then re-binds every register in the loop body. Same size, and
//      it was the last seven bytes.
//    * the error site is RETURN_ERROR(1, "sm_AllocateID", 1) with the name
//      as a literal (`push $imm32`, not `push [imm32]`), and the code really
//      is 1: the shipped `.data` carries "1", not "LT_ERROR".
//
//  The name "sm_AllocateID" is the shipped check literal, so it is the
//  original's name for the function; sm_ClaimObjectID is this file's own
//  earlier name for the address and is kept because the two call sites use
//  it.
// *********************************************************************** //

// One record of the object-id map. The link is the record itself, and the
// third dword is the slot index it stands for: the client tables are indexed
// with a full DWORD load of +0x08, which is what says the four bytes are one
// field. Bit 0x40000000 of that dword marks a record that is not available;
// the shipped code tests it as `testb $0x40,0xb(%eax)` and clears it with
// `andb $0xbf,0xb(%eax)`, which is VC6 narrowing a 32-bit mask to the one
// byte it touches — so this spelling and a separate `uint8` flag byte at
// +0x0b are indistinguishable from the bytes alone, and the dword index load
// is the tie-breaker.
class CObjIDRec
{
public:
	CObjIDRec* m_pPrev;                      // +0x00
	CObjIDRec* m_pNext;                      // +0x04
	uint32     m_nIndex;                     // +0x08
};

#define OBJIDREC_UNAVAILABLE 0x40000000

// 0x100345b6 — make the object-id map bigger, handing back a record through
// the same out-parameter this function uses. Nonzero means it could not.
// The name is shipped (its REPORT_ERROR literal); the body is smgrnew.cpp's.
int sm_CreateNewID(CServerMgr* pMgr, void** ppRec);

// 0x1002ad18 — the record for one id, or null if the map does not have it
// (name ours).
void* sm_FindObjectIDRec(CServerMgr* pMgr, uint16 nID);

// 0x10033253 — a CServerMgr method taking the map's element count, run only
// when the count has passed the 200 Init sets at +0x8b8. servermgr.h does not
// carry it and is not this reconstruction's to change, so the manager is cast
// to reach it (name and view ours).
class CServerMgrIDView
{
public:
	void Unknown33253(uint32 nCount);        // 0x10033253
};

int sm_ClaimObjectID(CServerMgr* pMgr, void** ppRec, uint16 nID)
{
	if (nID == 0xffff)
	{
		CObjIDRec* pCur;

#ifdef PRIMAL_HUNT
		for (pCur = (CObjIDRec*)pMgr->m_Link8c4.m_pNext;
		     pCur != (CObjIDRec*)&pMgr->m_Link8c4;
		     pCur = pCur->m_pNext)
#else
		// AVP2: the free list's head is a named local, as the client list's
		// below (`lea 0xcd4(%esi),%ecx; mov 0x4(%ecx),%eax`, AvP2 1.0.9.6
		// server.dll 0x1003bb98).
		CObjIDRec* pFreeHead = (CObjIDRec*)&pMgr->m_Link8c4;

		for (pCur = pFreeHead->m_pNext; pCur != pFreeHead; pCur = pCur->m_pNext)
#endif
		{
			*ppRec = pCur;

			if (!(pCur->m_nIndex & OBJIDREC_UNAVAILABLE))
				goto claimed;
		}

		{
			int dResult = sm_CreateNewID(pMgr, ppRec);

			if (dResult != 0)
				return dResult;
		}
	}
	else
	{
		while (nID >= pMgr->m_Unknown8bc)
		{
			int dResult = sm_CreateNewID(pMgr, ppRec);

			if (dResult != 0)
				return dResult;
		}

		*ppRec = sm_FindObjectIDRec(pMgr, nID);

		if (!*ppRec)
			RETURN_ERROR(1, "sm_AllocateID", 1);
	}

claimed:
	((CObjIDRec*)*ppRec)->m_nIndex &= ~OBJIDREC_UNAVAILABLE;

	dl_Remove((CheapLTLink*)*ppRec);
	dl_Insert((CheapLTLink*)&pMgr->m_Link8d0, (CheapLTLink*)*ppRec);

	if (pMgr->m_Unknown8bc > pMgr->m_Unknown8b8)
		((CServerMgrIDView*)pMgr)->Unknown33253(pMgr->m_Unknown8bc);

	LTLink* pHead = &pMgr->m_ClientList.m_Head;

	for (LTLink* pLink = pHead->m_pNext; pLink != pHead; pLink = pLink->m_pNext)
	{
		Client*   pClient = (Client*)pLink->m_pData;
		CObjInfo* pInfo   = &pClient->m_pObjInfos[((CObjIDRec*)*ppRec)->m_nIndex];

		pInfo->m_ChangeFlags = 0;
		pInfo->m_Unknown02   = 0;
	}

	return 0;
}


// *********************************************************************** //
//  Two more strays from the same unreconstructed neighbour, parked here for
//  the same reason the two sound-manager functions above are: they sit
//  between commonphysics.cpp's sm_MoveObject (0x100355fe) and this file's
//  own block, and neither is sensitive to which translation unit compiles
//  it.
//
//  Both are slots of ONE vtable, at 0x1004e8c8 — twelve entries, installed
//  by a `mov $0x1004e8c8,...` at 0x10031fc0. Reading the table off .rdata
//  is what gives the slot numbers: 0x1003524a, 0x10035260, 0x10035269,
//  0x100352d5, 0x100352f5, 0x1003530b, 0x10035541, 0x100352b4, 0x10035568,
//  0x100355db, 0x100355df, 0x100355ec — so 0x10035541 is slot 6 and
//  0x10035568 is slot 8. (Slot 9, 0x100355db, is a four-byte body
//  /OPT:ICF has folded together with LThread::IsValid, which is why
//  lthread.h already claims that address.)
//
//  The class is the object mover the world simulation drives: slot 5
//  (0x1003530b) walks an object's attachments and re-places each child
//  through wm_MoveObject, and it reaches the server manager at `this+4`.
//  Only the two slots below are implemented here, so the view is abstract
//  and no vtable of ours is emitted (names ours).
// *********************************************************************** //

// The request record both slots are handed. Only two fields are reached:
// a priority-ish dword at +0x0c (slot 6) and the object being moved at
// +0x08 (slot 8). Slot 5 reads +0x00 and +0x04 as well, so this is the
// front of a larger record — commonphysics.h's forward-declared
// WMMoveState, most likely, which has no definition yet (names ours).
class CMoveRequest
{
public:
	uint8            m_pad_00[0x08];
	class LTObject*  m_pObject;              // +0x08
	uint32           m_nBlockingPriority;    // +0x0c
};

class CObjectMover
{
public:
	virtual void _v00()=0; virtual void _v04()=0; virtual void _v08()=0;
	virtual void _v0c()=0; virtual void _v10()=0; virtual void _v14()=0;
	// 0x10035541 — may this object stop the move? (name ours)
	virtual int  ObjectBlocks(CMoveRequest* pRequest, void* pUnused,
	                          LTObject* pObj);
	virtual void _v1c()=0;
	// 0x10035568 — the "DebugMaxPos" console gate (name ours).
	virtual void CheckMaxPos(CMoveRequest* pRequest, LTVector* pPos);
	virtual void _v24()=0; virtual void _v28()=0; virtual void _v2c()=0;
	// 0x10032332 — slot 12, one past the twelve the note above counted: the
	// table runs to 0x1004e8f8 before the float at 0x1004e8fc ends it. Three
	// loads and a return, no arguments (name ours).
	virtual uint32 GetUnknown928Value();

	// +0x04 — the server manager, which slot 5 already reaches through
	// `this+4`. No pad in front of it: the vptr is what occupies +0x00.
	class CServerMgr*  m_pMgr;               // +0x04
};

// 0x10058530 — the float behind the "DebugMaxPos" console variable. The
// registration table at 0x100523b8 pairs the name string with this address,
// which is what names it; zero or less disables the check (name ours).
// Zero-initialised (.bss), so the check starts disabled. Defined here, its
// only reader, until the TU that owns the console-variable table
// (0x10052000-0x10052430, not reconstructed) exists to hold it.
float g_CV_DebugMaxPos;

// 0x10035541. `this` is never touched and the second argument is never
// read; the shipped code takes both and cleans twelve bytes off the stack.
//
// Read off the bytes: both failures reach ONE `xor %eax,%eax` at the very
// end and the success is `push $0x1; pop %eax; jmp` over it, which is a
// single `&&` guarding `return 1` with a trailing `return 0`. Two separate
// guarded `return 0`s are one byte shorter and branchless — VC6 collapses
// `if (!x) return 0; return 1;` into `neg/sbb/neg`, so the shipped branch
// pair is itself the evidence for the `&&`. The priority compare has the
// request's dword as the destination operand and the object's byte
// zero-extended into a register, so the request's field is the left-hand
// side in the source and the object's is a uint8.
int CObjectMover::ObjectBlocks(CMoveRequest* pRequest, void* pUnused,
                               LTObject* pObj)
{
	if (pRequest->m_nBlockingPriority > pObj->m_BlockingPriority &&
	    pObj->_p30())
	{
		return 1;
	}

	return 0;
}

// 0x10035568. Complain when something is put a very long way from the
// origin — a debugging aid whose console variable ships disabled.
//
// Read off the bytes:
//
//   * the gate is `<= 0.0f` against a .rdata zero (`fcomps`), not a test of
//     the bit pattern, so a negative value disables it too.
//   * the radius test is the squared magnitude against the square of the
//     variable, with the variable read from memory all three times rather
//     than held in a register — which is what naming no local gives.
//   * the two messages are two separate calls to the variadic printer, and
//     the first one's argument slot is never popped: the second call's
//     first `double` needs eight bytes, VC6 pushes four more and writes
//     across the dead slot, and one `add $0x20,%esp` cleans both calls at
//     the end. Thirty-two bytes for what looks like thirty-six.
//   * the class name is the first field of the class record hanging off
//     SObjData+0x24, reached with three loads and pushed straight through.
void CObjectMover::CheckMaxPos(CMoveRequest* pRequest, LTVector* pPos)
{
	// AVP2: the tests are the positive ones. cl 9044 keeps NaN apart where
	// 8966 does not, and AvP2 1.0.9.6's server.dll (0x1004098b `test $0x41;
	// jne`, 0x100409ab `test $0x5; jp`) returns on an unordered compare.
	if (g_CV_DebugMaxPos > 0.0f &&
	    pPos->MagSqr() > g_CV_DebugMaxPos * g_CV_DebugMaxPos)
	{
		dsi_ServerError("Server: position larger than 'DebugMaxPos'");

		dsi_ServerError("Object class: %s, position (%.2f, %.2f, %.2f)",
			pRequest->m_pObject->m_pSData->m_pClass->m_pClassName,
			pPos->x, pPos->y, pPos->z);
	}
}


// 0x10032332 — slot 12 of the mover's table: three loads and a return. Out of
// line rather than in the class body, because nothing in this translation
// unit calls it and the class is abstract, so an inline definition is never
// emitted at all.
uint32 CObjectMover::GetUnknown928Value()
{
	return m_pMgr->m_pUnknown928->m_n0c;
}
