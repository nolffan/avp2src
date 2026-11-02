// Force the CPacket template instantiations the images carry.
//
// The templates live in packet.h. VC6 parses `template void CPacket::WriteType
// (uint8);` and then emits nothing for it, so the instantiation has to be
// forced some other way. This driver is ours — it exists in no shipped binary
// and is excluded from the byte comparison.
//
// CALLING each one, which is what this did until now, DOES NOT WORK and had
// been silently producing nothing for either image. A member template defined
// in the class body is an expansion candidate with or without the keyword, so
// at /Ob1 VC6 expands every call right here into the driver and emits no
// standalone COMDAT at all — measured: seven symbols in this object, not one
// of them a ReadType or a WriteType. The same thing cost net_TracePacketQ454
// 964 bytes for the same reason at sixteen call sites.
//
// TAKING THE ADDRESS is what works. A pointer-to-member needs a real function
// to point at, so the body must be emitted, and it is emitted as the COMDAT
// the shipped images carry. The type rides in on the pointer's own type,
// which also sidesteps the thing packet.h's header note describes: VC6 cannot
// parse explicit template arguments on a member, so `&CPacket::ReadType<T>`
// is not available and deduction from the target type is the only route.
#include "packet.h"
#include "ltlink.h"

typedef void   (CPacket::*PacketWriteFn8)(uint8);
typedef void   (CPacket::*PacketWriteFn16)(uint16);
typedef void   (CPacket::*PacketWriteFn32)(uint32);
typedef uint8  (CPacket::*PacketReadFn8)(uint8*);
typedef uint16 (CPacket::*PacketReadFn16)(uint16*);
typedef uint32 (CPacket::*PacketReadFn32)(uint32*);

PacketWriteFn8  g_pPacketWrite8  = &CPacket::WriteType;
PacketWriteFn16 g_pPacketWrite16 = &CPacket::WriteType;
PacketWriteFn32 g_pPacketWrite32 = &CPacket::WriteType;
PacketReadFn8   g_pPacketRead8   = &CPacket::ReadType;
PacketReadFn16  g_pPacketRead16  = &CPacket::ReadType;
PacketReadFn32  g_pPacketRead32  = &CPacket::ReadType;


// 0x1000c537, 0x1000c690 and 0x1000d020 are CMoArray<uint8, DefaultCache>'s
// Insert2, _AllocateTArray and _DeleteAndDestroyArray (collection.h), which
// the WriteType instantiations above bring into this object: CPacket::m_Data
// is a real CMoArray<uint8>. server.dll's /O1 build calls Insert2 out of line
// from every grow loop; lithtech.exe's /O2 build expands it into WriteType
// (0x004303c0 and its siblings), calling BaseNew<uint8> (0x0040c060) and
// BaseDelete<uint8> (0x004152b0) from inside the expansion.
//
// These were once reconstructed here by hand as `CPacketData::SetAtGrow`,
// AllocItems and FreeItems on a flat view of the array. The shapes the bytes
// show -- the element count cached across the guard, the buffer reloaded in
// both copy loops, the two `return LTFALSE` sites tail-merged, the thiscall
// allocator that never reads `this`, the free whose count is never read --
// are exactly the template's.


// *********************************************************************** //
//  The inter-object link workers, 0x100139d0 and 0x10013b63.
//
//  They sit immediately after this file's two byte-wide CPacket template
//  instantiations in the image (0x100138d6 and 0x1001393e), which is what
//  puts them in this translation unit; servermgr.h declares both under the
//  sm_ prefix. The views below are local so that this file does not have to
//  pull the whole manager header in — only five structures are reached.
// *********************************************************************** //

// servermgr.h's StructBank and its two operations. The free is inline in the
// header the engine used — the freed record's first dword becomes the
// next-free link — which is why both frees here are three instructions and
// not a call.
class StructBank
{
public:
	uint8  m_pad_00[0x08];
	uint32 m_Unknown08;                      // +0x08  the grow count
	uint8  m_pad_0c[0x18 - 0x0c];
	void*  m_pFreeList;                      // +0x18  intrusive free list
};

// 0x1003e1e0 / 0x00490c20 -- grows the bank by one chunk.
void sb_Grow(StructBank* pBank, uint32 nCount);

// servermgr.h's inline sb_Allocate (0x10001c86 / 0x00414a40), repeated for
// this view. server.dll's /O1 calls it; lithtech.exe's /O2 expands it into
// both allocations of sm_CreateInterObjectLink (0x0043bdb0), which is where
// that function's two `call 0x490c20` grow calls come from.
inline void* sb_Allocate(StructBank* pBank)
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

inline void sb_Free(StructBank* pBank, void* pStruct)
{
	*(void**)pStruct = pBank->m_pFreeList;
	pBank->m_pFreeList = pStruct;
}

// 0x10058bc0 — the bank the list nodes come out of; its Allocate is the
// shared three-instruction forward at 0x1000ed6f (smgr.cpp calls the same
// address CObjQueueBank::Allocate). The returns do not go back through it:
// they are the inline sb_Free above on its embedded bank, which is why the
// free list at 0x10058bdc is touched directly.
class CLinkNodeBank
{
public:
	// server.dll CALLS the shared forward at 0x1000ed6f; lithtech.exe
	// EXPANDS it, sb_Allocate and all (0x004b17ac is this bank and
	// 0x004b17c4 its free list, in sm_CreateInterObjectLink 0x0043bdb0).
	// An in-class body is expanded at /O1 as well -- measured, virtual or
	// not -- so the two images get the two shapes they show.
#ifdef LT_SERVER_BUILD
	LTLink* Allocate();                      // 0x1000ed6f
#else
	LTLink* Allocate() { return (LTLink*)sb_Allocate(&m_Bank); }
#endif

	uint8      m_vptr[4];
	StructBank m_Bank;                       // +0x04
};

extern CLinkNodeBank g_LinkNodeBank;         // 0x10058bc0

// The manager, as far as these two need it: the 0x14-byte bank the link
// records themselves come out of (CServerMgr::m_Bank848).
class CLinkMgrView
{
public:
	uint8      m_pad_00[0x848];
	StructBank m_Bank848;                    // +0x848
};

// SObjData's inter-object link list is its first member, so the object's
// m_pSData doubles as the list head.
class CLinkSObjData
{
public:
	LTLink m_Links;                          // +0x00
	uint8  m_pad_0c[0x20 - 0x0c];
	class CLinkBaseClass* m_pBaseClass;      // +0x20  the game object
};

// One inter-object link: the type, both ends, and the two list nodes that
// hold it — 0x14 bytes, which is what m_Bank848 hands out. servermgr.h's
// view stops at the second end; the two node pointers are what the break
// walks, and what says a type-2 link has only one of them.
class InterObjectLink
{
public:
	uint32 m_Type;                           // +0x00
	class CLinkedObject* m_pOwner;           // +0x04  the container end
	class CLinkedObject* m_pLinked;          // +0x08  the contained end
	LTLink* m_pOwnerLink;                    // +0x0c  in the owner's list
	LTLink* m_pLinkedLink;                   // +0x10  in the linked end's list
};

// The two LTObject fields these reach. A type-2 link is not threaded through
// the linked end's list at all: the record is held in a single slot at
// +0x38, which the break clears with `and $0,0x38(%edx)`.
class CLinkedObject
{
public:
	uint8            m_pad_00[0x38];
	InterObjectLink* m_pLink38;              // +0x038
	uint8            m_pad_3c[0x188 - 0x3c];
	CLinkSObjData*   m_pSData;               // +0x188
};


// 0x10013b63 — thread a new link between two objects. The record goes on the
// owner's list always; the linked end gets a list node of its own unless the
// type is 2, in which case it keeps the record in its own slot instead.
//
// Nothing checks the allocation: the stores go straight through whatever
// sb_Allocate returned. That is what shipped.
LTRESULT sm_CreateInterObjectLink(CLinkMgrView* pMgr, CLinkedObject* pOwner,
                                  CLinkedObject* pLinked, int nType)
{
	InterObjectLink* pLink;
	LTLink*          pNode;

	if (pOwner == pLinked)
		return LT_ERROR;

	pLink = (InterObjectLink*)sb_Allocate(&pMgr->m_Bank848);

	pLink->m_Type    = nType;
	pLink->m_pOwner  = pOwner;
	pLink->m_pLinked = pLinked;

	pNode = g_LinkNodeBank.Allocate();
	pNode->m_pData = pLink;
	pLink->m_pOwnerLink = pNode;
	dl_Insert(&pOwner->m_pSData->m_Links, pNode);

	if (nType == 2)
	{
		pLinked->m_pLink38 = pLink;
	}
	else
	{
		pNode = g_LinkNodeBank.Allocate();
		pNode->m_pData = pLink;
		pLink->m_pLinkedLink = pNode;
		dl_Insert(&pLinked->m_pSData->m_Links, pNode);
	}

	return LT_OK;
}


// 0x100139d0 — take the link between two objects apart again. The owner's
// list is walked and every record whose two ends match is unthreaded from
// both ends, its nodes returned to the node bank and the record itself to
// the manager's.
//
// Three things read off the bytes. The type filter accepts 0 and 2 and
// nothing else, so a type-1 link is not breakable here. The two node
// pointers are re-read for the free rather than held in a local, which is
// the `mov 0x10(%ecx),%eax` appearing twice around each unthread. And the
// last argument is a keep-going flag, not a type: it is compared against
// zero, after the removal, and zero stops the walk at the first match.
void sm_BreakInterObjectLink(CLinkMgrView* pMgr, CLinkedObject* pOwner,
                             CLinkedObject* pLinked, int bAll)
{
	LTLink* pHead = &pOwner->m_pSData->m_Links;
	LTLink* pCur  = pHead->m_pNext;

	while (pCur != pHead)
	{
		InterObjectLink* pLink = (InterObjectLink*)pCur->m_pData;
		LTLink*          pNext = pCur->m_pNext;

		if (pLink->m_Type == 0 || pLink->m_Type == 2)
		{
			if (pLink->m_pOwner == pOwner && pLink->m_pLinked == pLinked)
			{
				if (pLink->m_Type == 2)
				{
					pLink->m_pLinked->m_pLink38 = LTNULL;
				}
				else
				{
					dl_Remove(pLink->m_pLinkedLink);

					sb_Free(&g_LinkNodeBank.m_Bank, pLink->m_pLinkedLink);
				}

				dl_Remove(pLink->m_pOwnerLink);

				sb_Free(&g_LinkNodeBank.m_Bank, pLink->m_pOwnerLink);

				sb_Free(&pMgr->m_Bank848, pLink);

				if (!bAll)
					return;
			}
		}

		pCur = pNext;
	}
}


// The other end a type-2 link can hold: a playing sound. servermgr.h's
// SoundInstance, as far as this reaches it — and its m_pObjectRef at +0x38 is
// the link record itself, which is why servermgr.h's SoundObjectRef (a dword
// then an LTObject*) is InterObjectLink's first two fields under another
// name.
class CLinkSound
{
public:
	uint8            m_pad_00[0x10];
	LTBOOL           m_bKill;                // +0x10
	uint8            m_pad_14[0x20 - 0x14];
	float            m_fTimer;               // +0x20  seconds left
	uint8            m_pad_24[0x34 - 0x24];
	uint32           m_PlaySoundFlags;       // +0x34  PLAYSOUND_*
	InterObjectLink* m_pObjectRef;           // +0x38
};

// The game object hung off SObjData+0x20 — the SDK's BaseClass
// (ltengineobjects.h). Its destructor is virtual and declared first, which
// puts EngineMessageFn at slot 1; the call site pushes three arguments and
// the callee cleans up, so the signature is the header's exactly.
class CLinkBaseClass
{
public:
	virtual void   _v00() = 0;
	virtual uint32 EngineMessageFn(uint32 messageID, void* pData, float fData) = 0;
};

// From ltserverobj.h; nothing here includes that header, so the value is
// spelled out (the same way servermodel.cpp spells MID_UPDATE out).
#define MID_LINKBROKEN 4

// The sound the caller asked for a handle to is the game's to dispose of;
// any other attached sound dies with the object.
#define PLAYSOUND_GETHANDLE 0x0010


// 0x10013a87 — break every link of one type off an object. The same walk as
// sm_BreakInterObjectLink over the same list, but selecting on the type
// alone rather than on both ends, and with no early exit: this is the
// teardown side, so it clears all of them.
//
// The two arms differ in what the far end is. A type-2 link's far end is a
// SoundInstance, not an object: it is killed on the spot unless the game
// asked for a handle to it, its back-reference is cleared, and there is no
// node on that side to unthread. Every other type has a node at both ends,
// and the owner's game object is told the link broke first — MID_LINKBROKEN,
// with the far end as the message data.
//
// The second flag argument gates only that notification, not the removal.
void sm_BreakAttachments(CLinkMgrView* pMgr, CLinkedObject* pObject,
                         int nType, int bNotify)
{
	LTLink* pHead = &pObject->m_pSData->m_Links;
	LTLink* pCur  = pHead->m_pNext;

	while (pCur != pHead)
	{
		InterObjectLink* pLink = (InterObjectLink*)pCur->m_pData;
		LTLink*          pNext = pCur->m_pNext;

		if (pLink->m_Type == nType)
		{
			if (pLink->m_Type == 2)
			{
				CLinkSound* pSound = (CLinkSound*)pLink->m_pLinked;

				if (!(pSound->m_PlaySoundFlags & PLAYSOUND_GETHANDLE))
				{
					pSound->m_fTimer = 0.0f;
					pSound->m_bKill  = 1;
				}

				pSound->m_pObjectRef = LTNULL;
			}
			else
			{
				if (bNotify && pLink->m_pOwner->m_pSData->m_pBaseClass)
					pLink->m_pOwner->m_pSData->m_pBaseClass->EngineMessageFn(
					    MID_LINKBROKEN, pLink->m_pLinked, 0.0f);

				dl_Remove(pLink->m_pLinkedLink);

				sb_Free(&g_LinkNodeBank.m_Bank, pLink->m_pLinkedLink);
			}

			dl_Remove(pLink->m_pOwnerLink);

			sb_Free(&g_LinkNodeBank.m_Bank, pLink->m_pOwnerLink);

			sb_Free(&pMgr->m_Bank848, pLink);
		}

		pCur = pNext;
	}
}


// *********************************************************************** //
//  The BSP line query's recursion, 0x10013bf5.
//
//  It follows the two link workers in the image and is declared in
//  worldsim.h as bspq_IntersectLineRec; the views below are again local so
//  this file stays free of the world headers. Everything the SDK already
//  has — LTPlane, LTVector, IntersectQuery, PolyFilterFn — comes in through
//  packet.h's ltbasedefs.h.
// *********************************************************************** //

// worldsim.h's BSPNode and the two hops from it to its plane (the walk
// WorldPolyOwner::GetPlane makes). Both low bits of the flag byte at 0x16
// mark a leaf.
class CBspWorldData
{
public:
	uint8    m_pad_00[0x34];
	LTPlane* m_pPlane;                       // 0x34
	uint32   m_Flags;                        // 0x38  bit 0 gates the poly test
};

// One entry of the polygon's vertex table: 0x18 bytes, of which only the
// vertex pointer is read from here (name ours).
class CBspVert
{
public:
	LTVector* m_pVec;                        // 0x00
	uint8     m_pad_04[0x18 - 0x04];
};

class CBspPolyOwner
{
public:
	uint8          m_pad_00[0x18];
	LTVector       m_Center;                 // 0x18  the bounding sphere
	float          m_Radius;                 // 0x24
	CBspWorldData* m_pParent;                // 0x28
	uint8          m_pad_2c[0x50 - 0x2c];
	uint16         m_nVerts;                 // 0x50
	uint8          m_pad_52[0x54 - 0x52];
	CBspVert       m_Verts[1];               // 0x54  runs on past the record
};

class CBspNode
{
public:
	CBspPolyOwner* m_pOwner;                 // 0x00
	CBspNode*      m_pChild[2];              // 0x04  [0] behind, [1] in front
	uint8          m_pad_0c[0x16 - 0x0c];
	uint8          m_Flags;                  // 0x16
};

#define BSPNODE_LEAF 0x03

// IWorldBsp (ltobject_internal.h); only slot 2 is reached, and it is what
// turns the node the walk stopped on into the handle the poly filter takes.
class CBspWorld
{
public:
	virtual uint32    _v00() = 0;
	virtual CBspNode* _v04() = 0;
	virtual HPOLY     MakeHPoly(CBspNode* pPoly) = 0;
};

// worldsim.h's CLineQuery, with the two dwords it leaves as m_Unknown0c and
// m_Unknown10 named. They are not unknown: +0x0c is the SDK's own
// IntersectQuery — this reaches its m_PolyFilterFn at 0x2c and its
// m_pUserData at 0x30, which is exactly where ltbasedefs.h puts them — and
// +0x10 is the world whose MakeHPoly makes the handle that filter is handed.
// So a null filter, a null function or a null world all mean "accept".
class CBspLineQuery
{
public:
	LTVector*       m_pStart;                // 0x00
	LTVector*       m_pEnd;                  // 0x04
	LTVector*       m_pHit;                  // 0x08
	IntersectQuery* m_pQuery;                // 0x0c
	CBspWorld*      m_pWorldBsp;             // 0x10
	CBspNode*       m_pPoly;                 // 0x14  what the walk found
};

// 0x10013ddc — the polygon test the line query stops on: the crossing point
// is on the polygon's plane, and this asks whether it is inside the polygon
// itself.
//
// Two rejections before the edge walk: a flag on the poly's world record,
// and the bounding sphere at +0x18/+0x24 (the squared radius against the
// squared distance, so nothing takes a root).
//
// The walk itself is the standard convex test. Each edge runs from the
// previous vertex to the current one — the cursor starts at the *last*
// vertex, which is what `lea -0x18(%eax),%ecx` off the end pointer is — and
// the plane's normal crossed with the edge gives the inward normal of the
// edge's own plane. The point has to be on the inside of every one of them,
// to within a hundredth of a unit.
//
// `Cross`, `Norm` and `MagSqr` are calls and `Dot` inlines, which is what
// the SDK's own ltvector.h gives: all four take their vector argument by
// value, and the by-value copies are the `movsl` triples interleaved
// through the float work. Instantiating them here is also what brings
// _CVector<float>::Mag, MagSqr, Norm and Cross into this object, and all
// four are byte-identical to the shipped bodies.
//
// The edge plane is a real LTPlane local, not a vector and a float: the two
// sit at -0x5c and -0x50 of the frame, exactly LTPlane's m_Normal/m_Dist
// spacing, and the test is `DistTo` written out (see below for the reload).
//
// The difference vector is one function-scoped LTVector, vTemp, shared by
// the radius reject and every edge. That is the shipped -0x40 slot: the
// radius test's `MagSqr` receiver and, in the loop, the edge -- stored and
// kept (vTemp's address escapes through the out-of-line MagSqr call), with
// Cross's by-value copy taken from operator-'s own temp. It is also why the
// frame is 0x5c: Cross's return temp cannot reuse -0x40. The operator-
// temporary passed straight to Cross, a separate block- or function-scoped
// edge local, a `const LTVector&` bound to the temp, and `operator^` all
// collapse to one blit and a 0x50 frame (324/131).
//
// The plane test re-reads m_Dist from memory: `fstps -0x50` at 0x10013ee6
// and `fsubs -0x50` at 0x10013f00. Plain `DistTo(*pPoint)` (and the same
// written out) forwards the stored value on the FPU instead (`fsts` +
// `fsub %st(1)` + `fstp %st(0)`, 333 bytes). The read goes through a
// volatile float, which is what forces the reload; a volatile STORE instead
// pins the store ahead of the *pPoint blit and is wrong (332, 9 insns off).
//
// INLINE, and defined ABOVE bspq_IntersectLineRec, because lithtech.exe's
// copy of the recursion (0x0043beb0, 1106 bytes) has this whole body
// expanded at 0x0043c0bf..0x0043c249 -- there is no out-of-line copy in the
// client image.  /O1 declines the expansion, so server.dll keeps 0x10013ddc
// and both server rows stay identical.  Written as a forward-declared
// out-of-line function the client recursion was 667 bytes; inline it is
// 1105.  What is left there (NOT identical, not yet a scored near): shipped
// spent its inline budget before the end of this body -- Norm's Mag
// (0x0043c19f -> 0x00436760) and the second Dot (0x0043c226 -> 0x00436790)
// are CALLS in the client, where ours expands both -- and the allocation
// that follows (shipped keeps pNode in its argument slot and nSide at
// 0x18(%esp); ours holds them in %ebx/%ebp).  Probed: the operator form of
// vMid, or Init() on vStart/m_pHit, tips the budget over (Mag and both Dots
// become calls, plus a constructor) but also changes the server body;
// trivial GetPlane()/IsLeaf() accessors do not move it at all.
inline int w_PointInPoly(CBspPolyOwner* pOwner, LTVector* pPoint)
{
	CBspVert* pCur;
	CBspVert* pPrev;
	CBspVert* pEnd;
	LTPlane*  pPlane;
	LTPlane   edgePlane;
	LTVector  vTemp;

	if (!(pOwner->m_pParent->m_Flags & 1))
		return 0;

	vTemp = pOwner->m_Center - *pPoint;

	if (vTemp.MagSqr() > pOwner->m_Radius * pOwner->m_Radius)
	{
		return 0;
	}

	pPlane = pOwner->m_pParent->m_pPlane;

	pEnd  = &pOwner->m_Verts[pOwner->m_nVerts];
	pCur  = pOwner->m_Verts;
	pPrev = pEnd - 1;

	while (pCur != pEnd)
	{
		vTemp = *pPrev->m_pVec - *pCur->m_pVec;

		edgePlane.m_Normal = pPlane->m_Normal.Cross(vTemp);

		edgePlane.m_Normal.Norm();

		edgePlane.m_Dist = edgePlane.m_Normal.Dot(*pCur->m_pVec);

		if (edgePlane.m_Normal.Dot(*pPoint) - *(volatile float*)&edgePlane.m_Dist < -0.01f)
			return 0;

		pPrev = pCur;
		pCur++;
	}

	return 1;
}

// The plane-thickness the walk works to, and the fraction past which a miss
// is not worth splitting again (both read out of .rdata: 0x3c23d70a and
// 0x3f7d70a4).
#define BSP_LINE_EPSILON 0.01f
#define BSP_LINE_MAXFRAC 0.99f


// 0x10013bf5. The classic BSP segment walk: a segment entirely on one side
// of the node's plane descends that side without splitting, and one that
// straddles it is cut at the crossing and the near half recursed into first.
// The tail — the far half against the other child — is a loop rather than a
// second recursion, which is what makes the whole function one `for (;;)`
// with the parameters reassigned.
//
// Four things read off the bytes rather than assumed:
//
//   * the side to take first comes from where the *start* is, except when
//     the start is within the plane's thickness, where it comes from the end
//     instead — and there the sense is inverted (an end in front picks child
//     zero, which is behind). That is what shipped.
//   * the split is skipped entirely when the two distances are equal, and
//     when the start is on the plane; both paths fall straight through to
//     the other child.
//   * the polygon test only runs on the front-first side (nSide == 1), and
//     the fraction cut-off only runs when it does *not* — a miss inside the
//     polygon walks on without consulting the cut-off at all.
//   * nSide lives in the dead pNode parameter slot; the node itself is kept
//     in a register from the entry load on, which is what frees it.
int bspq_IntersectLineRec(CBspNode* pNode, CBspLineQuery* pQuery,
                          LTVector vStart, LTVector vEnd)
{
	float    t;
	LTVector vMid;
	int      nSide;

	for (;;)
	{
		if (pNode->m_Flags & BSPNODE_LEAF)
			return 0;

		LTPlane* pPlane = pNode->m_pOwner->m_pParent->m_pPlane;

		// Both distances are accumulated rather than written as one sum, and
		// the statements are what pins the term order: VC6 canonicalises a
		// flat float sum of struct components into ASCENDING field-offset
		// order no matter how the terms are written (all six permutations
		// probed, and `LTPlane::DistTo`, all emit x then y then z). The
		// shipped code multiplies z first, then y, then x, and only separate
		// `+=` statements reproduce that.
		float fStart = vStart.z * pPlane->m_Normal.z;

		fStart += vStart.y * pPlane->m_Normal.y;
		fStart += vStart.x * pPlane->m_Normal.x;
		fStart -= pPlane->m_Dist;

		float fEnd = vEnd.z * pPlane->m_Normal.z;

		fEnd += vEnd.y * pPlane->m_Normal.y;
		fEnd += vEnd.x * pPlane->m_Normal.x;
		fEnd -= pPlane->m_Dist;

		if (fStart > BSP_LINE_EPSILON && fEnd > BSP_LINE_EPSILON)
		{
			pNode = pNode->m_pChild[1];

			continue;
		}

		if (fStart < -BSP_LINE_EPSILON)
		{
			if (fEnd < -BSP_LINE_EPSILON)
			{
				pNode = pNode->m_pChild[0];

				continue;
			}

			if (fStart > 0.0f)
				nSide = 1;
			else
				nSide = 0;
		}
		else if (fStart <= BSP_LINE_EPSILON)
		{
			if (fEnd < 0.0f)
				nSide = 1;
			else
				nSide = 0;
		}
		else
		{
			if (fStart > 0.0f)
				nSide = 1;
			else
				nSide = 0;
		}

		float fDelta = fEnd - fStart;

		if (fDelta != 0.0f &&
		    (fStart < -BSP_LINE_EPSILON || fStart > BSP_LINE_EPSILON))
		{
			t = -fStart / fDelta;

			vMid.x = (vEnd.x - vStart.x) * t + vStart.x;
			vMid.y = (vEnd.y - vStart.y) * t + vStart.y;
			vMid.z = (vEnd.z - vStart.z) * t + vStart.z;

			if (bspq_IntersectLineRec(pNode->m_pChild[nSide], pQuery, vStart, vMid))
				return 1;

			if (nSide == 1 && pNode->m_pOwner)
			{
				if (w_PointInPoly(pNode->m_pOwner, &vMid))
				{
					if (!pQuery->m_pQuery ||
					    !pQuery->m_pQuery->m_PolyFilterFn ||
					    !pQuery->m_pWorldBsp ||
					    pQuery->m_pQuery->m_PolyFilterFn(
					        pQuery->m_pWorldBsp->MakeHPoly(pNode),
					        pQuery->m_pQuery->m_pUserData))
					{
						pQuery->m_pPoly = pNode;

						*pQuery->m_pHit = vMid;

						return 1;
					}
				}
			}
			else if (t > BSP_LINE_MAXFRAC)
			{
				return 0;
			}

			vStart = vMid;
		}

		pNode = pNode->m_pChild[nSide == 0];
	}
}
