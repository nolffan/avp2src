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
#if defined(PRIMAL_HUNT) && !defined(LT_SERVER_BUILD)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x419f10).
#include <string>
#endif
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


#if !defined(PRIMAL_HUNT) && !defined(LT_SERVER_BUILD)
// AvP2 1.0.9.6's lithtech.exe only, for now. Primal Hunt's link workers,
// with the smart-link bodies and type-3 links, are a translation unit of
// their own (interlink.cpp: lithtech.exe 0x00447bc0..0x004480cb, server.dll
// 0x10018c25..0x100190e1), and so are AvP2 1.0.9.6's: its server.dll has
// interlink.cpp's STLport build at 0x10018069..0x10018553, byte for byte,
// so these NOLF-layout views (SObjData at +0x188, the record bank at
// +0x848) are no longer built for it.
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
#endif // !PRIMAL_HUNT && !LT_SERVER_BUILD

// The BSP line query's recursion and its polygon test (NOLF 0x10013bf5,
// 0x10013ddc) are worldsim.cpp's: AvP2 and Primal Hunt have them there.
