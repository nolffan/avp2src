// The inter-object links and the smart-link bodies (Primal Hunt). File name
// ours.
//
// Primal Hunt gives this module a translation unit of its own in both
// images, bracketed by its own static initializers:
//
//   lithtech.exe 0x00447bc0..0x004480cb: the body list's _$E (0x00447bc0,
//     constructor 0x00447bd0, atexit 0x00447c00, destructor 0x00447c10)
//     first, <string>'s ctype<unsigned short>::id initializer (0x004480a0)
//     last;
//   server.dll 0x10018c25..0x100190e1: the same _$E (0x10018c25), the
//     list's out-of-line members at 0x10019001..0x100190c0, and the same
//     <string> initializer (0x100190c1).
//
// NOLF and AvP2 1.0.9.6 keep the link workers in packet.cpp (AvP2's with an
// STLport list of its own, lithtech.exe 0x00443c60..0x00444298); packet.cpp
// still builds them for 1.0.9.6, and this file is Primal Hunt's alone.
//
// What a link is: an InterObjectLink record (servermgr.h) from
// CServerMgr::m_Bank848, threaded onto the owner's SObjData::m_Links through
// a node from the link-node bank, and onto the linked end's the same way for
// types 0 and 1. A type-2 link's far end is a SoundInstance holding the
// record in its m_pObjectRef slot; a type-3 link's is an LTSmartLink_Body
// (ltsmartlink.h) out of the body list below, which keeps no node either.
//
// AvP2 1.0.9.6 has the same module, against STLport (build/build.mk,
// ENG_AVP_STL_SRC): server.dll 0x10018069..0x10018553, the list's _$E
// (0x10018069, atexit 0x1001807b) first, then the workers and the list's
// out-of-line members. Its body list is found with find_if, and its node
// bank's free is servermgr.h's sb_Free on the global bank.
#ifdef PRIMAL_HUNT
// PH: <string> as everywhere else in this engine: this translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id (guard
// 0x004f8014, atexit of the empty 0x00401790; lithtech.exe 0x004480a0;
// server.dll 0x100190c1, guard 0x1008ec84).
#include <string>
#endif
#include <list>
#ifndef PRIMAL_HUNT
#include <algorithm>
#endif
#include "servermgr.h"
#include "ltengineobjects.h"
#include "iltmessage.h"                     // ltsmartlink.h's serializers
#include "ltsmartlink.h"

// The node bank (0x004f0120 / 0x1008e008): the 0x0c-byte LTLink records
// svnew.cpp defines as g_LinkNodeBank. Both images CALL its Allocate here
// (lithtech.exe 0x00404fb0, server.dll 0x1003883a) and expand servermgr.h's
// sb_Free into its embedded bank (lithtech.exe `mov $0x4f0124,%edx`, free
// list 0x004f013c). Spelled as smgr.cpp spells it, CObjQueueBank /
// g_ObjQueueBank, which is the name both images' links carry onto the one
// object (link/aliases.rsp).
class CObjQueueBank
{
public:
#if defined(PRIMAL_HUNT) || defined(LT_SERVER_BUILD)
	// (AvP2 1.0.9.6's server.dll calls it, as Primal Hunt does.)
	LTLink* Allocate();
#else
	// AVP2 1.0.9.6's lithtech.exe expands it, sb_Allocate and all, at both
	// of sm_CreateInterObjectLink's sites (0x00444174, 0x004441d5: the bank
	// at 0x004def74, its free list 0x004def8c), as packet.cpp's
	// CLinkNodeBank has it.
	LTLink* Allocate() { return (LTLink*)sb_Allocate(&m_Bank); }
#endif

	uint8      m_vptr[4];
	StructBank m_Bank;                       // +0x04
};

extern CObjQueueBank g_ObjQueueBank;

// Every body handed out, one 0x14-byte list node each (the 0x14 pushed to
// the allocator at lithtech.exe 0x00447bd5 and 0x00447c9d): the node's two
// links, then the body's count, object and server. Head 0x004f5454 / count
// 0x004f5458 (server.dll 0x1008c96c / 0x1008c970); the byte at 0x004f5450 is
// the list's allocator. Name ours.
static std::list<LTSmartLink_Body> g_SmartLinkBodies;

#ifndef PRIMAL_HUNT
// AVP2: the two searches of the body list are find_if calls (server.dll
// 0x100184de and 0x10018519, each copying its iterators and taking the
// predicate by value), one predicate for each key. Names ours.
struct SmartLinkBodyOf
{
	SmartLinkBodyOf(LTObject* pObject) : m_pObject(pObject) {}
	bool operator()(const LTSmartLink_Body& body) const
		{ return body.m_pObject == m_pObject; }

	LTObject* m_pObject;
};

struct SmartLinkBodyAt
{
	SmartLinkBodyAt(LTSmartLink_Body* pBody) : m_pBody(pBody) {}
	bool operator()(const LTSmartLink_Body& body) const
		{ return &body == m_pBody; }

	LTSmartLink_Body* m_pBody;
};

// 0x10018091 -- AvP2's GetSmartLinkBody: the body whose object is the
// owner, or a new one pushed onto the end.
LTSmartLink_Body* sm_GetSmartLinkBodyQ502(LTObject* pOwner)
{
	std::list<LTSmartLink_Body>::iterator it = std::find_if(
		g_SmartLinkBodies.begin(), g_SmartLinkBodies.end(),
		SmartLinkBodyOf(pOwner));

	if (it != g_SmartLinkBodies.end())
		return &(*it);

	g_SmartLinkBodies.push_back(LTSmartLink_Body(pOwner));
	return &g_SmartLinkBodies.back();
}

// 0x100180f0 -- AvP2's ReleaseSmartLinkBody, as Primal Hunt's below.
void sm_ReleaseSmartLinkBodyQ502(LTSmartLink_Body* pBody)
{
	std::list<LTSmartLink_Body>::iterator it = std::find_if(
		g_SmartLinkBodies.begin(), g_SmartLinkBodies.end(),
		SmartLinkBodyAt(pBody));

	if (it != g_SmartLinkBodies.end())
	{
		if (it->m_nCount <= 0 && !it->m_pObject)
			g_SmartLinkBodies.erase(it);
	}
}
#else


// 0x00447c70 / 0x10018c6b -- the body an owner's smart links share, made on
// first use: a walk of the list for the body whose object is the owner, and
// a new body with no references pushed onto the end when there is none.
// lithtech.exe expands the whole push_back (0x00447c97..: the node, its two
// links, the copy and the count); server.dll calls the list's insert
// (0x1001902c) with the temporary built in its frame.
LTSmartLink_Body* sm_GetSmartLinkBodyQ502(LTObject* pOwner)
{
	std::list<LTSmartLink_Body>::iterator it;

	for (it = g_SmartLinkBodies.begin(); it != g_SmartLinkBodies.end(); ++it)
	{
		if (it->m_pObject == pOwner)
			break;
	}

	if (it != g_SmartLinkBodies.end())
		return &(*it);

	g_SmartLinkBodies.push_back(LTSmartLink_Body(pOwner));
	return &g_SmartLinkBodies.back();
}

// 0x00447d00 / 0x10018cba -- let a body go. It is found by address, and
// only erased once nothing references it AND its object is gone: the count
// is tested `jg` (a count at or below zero passes) and the object pointer
// for null. A body whose object is still alive stays on the list, which is
// what lets the owner's next GetSmartLinkBody find it again.
void sm_ReleaseSmartLinkBodyQ502(LTSmartLink_Body* pBody)
{
	std::list<LTSmartLink_Body>::iterator it;

	for (it = g_SmartLinkBodies.begin(); it != g_SmartLinkBodies.end(); ++it)
	{
		if (&(*it) == pBody)
			break;
	}

	if (it != g_SmartLinkBodies.end())
	{
		if (it->m_nCount <= 0 && !it->m_pObject)
			g_SmartLinkBodies.erase(it);
	}
}
#endif


// 0x00447d60 / 0x10018d00 -- take the link between two objects apart again.
// The owner's list is walked and every record whose two ends match is
// unthreaded from both ends, its nodes returned to the node bank and the
// record itself to the manager's.
//
// The type filter accepts 0 and 2 and nothing else, so a container (type 1)
// or smart (type 3) link is not breakable here. The two node pointers are
// re-read for the free rather than held in a local, which is the
// `mov 0x10(%ecx),%eax` appearing twice around each unthread. The last
// argument is a keep-going flag, not a type: it is compared against zero
// after the removal, and zero stops the walk at the first match.
void sm_BreakInterObjectLink(CServerMgr* pMgr, LTObject* pOwner,
                             LTObject* pLinked, int bAll)
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
					((SoundInstance*)pLink->m_pLinked)->m_pObjectRef = LTNULL;
				}
				else
				{
					dl_Remove(pLink->m_pLinkedLink);

					sb_Free(&g_ObjQueueBank.m_Bank, pLink->m_pLinkedLink);
				}

				dl_Remove(pLink->m_pOwnerLink);

				sb_Free(&g_ObjQueueBank.m_Bank, pLink->m_pOwnerLink);

				sb_Free(&pMgr->m_Bank848, pLink);

				if (!bAll)
					return;
			}
		}

		pCur = pNext;
	}
}


// 0x00447e50 / 0x10018de7 -- break every link of one type off an object:
// the same walk over the same list, selecting on the type alone and with no
// early exit, since this is the teardown side.
//
// The arms differ in what the far end is. A type-2 link's is a playing
// sound: killed on the spot unless the game asked for a handle to it, and
// its back-reference cleared. A type-3 link's is a smart-link body: its
// object is cleared, which is what makes every LTSmartLink on it answer
// null from now on, and it is offered back to the list. Neither has a node
// on that side. Every other type has one, and the owner's game object is
// told the link broke first -- MID_LINKBROKEN, the far end as the data --
// when the last argument asks for it; that flag gates only the message.
void sm_BreakAttachments(CServerMgr* pMgr, LTObject* pObject,
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
				SoundInstance* pSound = (SoundInstance*)pLink->m_pLinked;

				if (!(pSound->m_PlaySoundFlags & PLAYSOUND_GETHANDLE))
				{
					pSound->m_fTimer = 0.0f;
					pSound->m_bKill  = 1;
				}

				pSound->m_pObjectRef = LTNULL;
			}
			else if (pLink->m_Type == 3)
			{
				LTSmartLink_Body* pBody = (LTSmartLink_Body*)pLink->m_pLinked;

				if (pBody)
				{
					pBody->m_pObject = LTNULL;
					sm_ReleaseSmartLinkBodyQ502(pBody);
				}
			}
			else
			{
				if (bNotify && pLink->m_pOwner->m_pSData->m_pBaseClass)
					pLink->m_pOwner->m_pSData->m_pBaseClass->EngineMessageFn(
					    MID_LINKBROKEN, pLink->m_pLinked, 0.0f);

				dl_Remove(pLink->m_pLinkedLink);

				sb_Free(&g_ObjQueueBank.m_Bank, pLink->m_pLinkedLink);
			}

			dl_Remove(pLink->m_pOwnerLink);

			sb_Free(&g_ObjQueueBank.m_Bank, pLink->m_pOwnerLink);

			sb_Free(&pMgr->m_Bank848, pLink);
		}

		pCur = pNext;
	}
}


static LTBOOL sm_InterObjectLinkExists(LTObject* pOwner, LTObject* pLinked,
                                       int nType);

// 0x00447f70 / 0x10018f02 -- thread a new link between two objects.
//
// Container (1) and smart (3) links are made once: if the owner already
// has one to the same end, the answer is LT_OK and nothing is added. Then
// an object may not be linked to itself. The record goes on the owner's
// list always; a type-2 far end keeps it in its own slot instead, types 0
// and 1 get a node on the far end's list, and a type-3 end gets nothing.
//
// Nothing checks the allocation: the stores go straight through whatever
// sb_Allocate returned (lithtech.exe 0x00447fd2 jumps past the free-list
// pop on null and lands on them). That is what shipped.
LTRESULT sm_CreateInterObjectLink(CServerMgr* pMgr, LTObject* pOwner,
                                  LTObject* pLinked, int nType)
{
	InterObjectLink* pLink;
	LTLink*          pNode;

	if (nType == 3 || nType == 1)
	{
		if (sm_InterObjectLinkExists(pOwner, pLinked, nType))
			return LT_OK;
	}

	if (pOwner == pLinked)
		return LT_ERROR;

	pLink = (InterObjectLink*)sb_Allocate(&pMgr->m_Bank848);
#ifdef AVP2SRC_MODERN
	// M-B05.  RETAIL BUG (RETAIL_BUGS.md): no allocation was checked; one
	// that fails now gives back what was taken and LT_ERROR.
	if (!pLink)
		return LT_ERROR;
#endif

	pLink->m_Type    = nType;
	pLink->m_pOwner  = pOwner;
	pLink->m_pLinked = pLinked;

	pNode = g_ObjQueueBank.Allocate();
#ifdef AVP2SRC_MODERN
	if (!pNode)
	{
		sb_Free(&pMgr->m_Bank848, pLink);
		return LT_ERROR;
	}
#endif
	pNode->m_pData = pLink;
	pLink->m_pOwnerLink = pNode;
	dl_Insert(&pOwner->m_pSData->m_Links, pNode);

	if (nType == 2)
	{
		((SoundInstance*)pLinked)->m_pObjectRef = (SoundObjectRef*)pLink;
	}
	else if (nType == 0 || nType == 1)
	{
		pNode = g_ObjQueueBank.Allocate();
#ifdef AVP2SRC_MODERN
		if (!pNode)
		{
			dl_Remove(pLink->m_pOwnerLink);
			sb_Free(&g_ObjQueueBank.m_Bank, pLink->m_pOwnerLink);
			sb_Free(&pMgr->m_Bank848, pLink);
			return LT_ERROR;
		}
#endif
		pNode->m_pData = pLink;
		pLink->m_pLinkedLink = pNode;
		dl_Insert(&pLinked->m_pSData->m_Links, pNode);
	}

	return LT_OK;
}

// 0x00448050 / 0x10018fc6 -- whether the owner already holds a link of this
// type to that end (name ours). Only the owner's list is read.
static LTBOOL sm_InterObjectLinkExists(LTObject* pOwner, LTObject* pLinked,
                                       int nType)
{
	LTLink* pHead = &pOwner->m_pSData->m_Links;
	LTLink* pCur;

	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		InterObjectLink* pLink = (InterObjectLink*)pCur->m_pData;

		if (pLink->m_Type == nType && pLink->m_pOwner == pOwner &&
		    pLink->m_pLinked == pLinked)
			return LTTRUE;
	}

	return LTFALSE;
}
