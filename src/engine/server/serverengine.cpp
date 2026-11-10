// server.dll — the se_ family: giving an object its file (reconstruction).
// Build /O1.
#include "serverengine.h"
#include "commonphysics.h"
#include "worldbsp.h"
#include <new.h>
#include <stdio.h>
#include <stdlib.h>
#include "lthread.h"
#include <string.h>
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): the unit ends in <string>'s
// guarded initializer of std::ctype<unsigned short>::id (server.dll
// 0x1003753f: guard 0x1008ec84, atexit of the empty 0x1002eaeb; lithtech.exe
// 0x0047fc90: guard 0x004f8014, atexit of the empty 0x00401790).
#if defined(PRIMAL_HUNT)
#include <string>
#endif


// A byte-wide view of the file record's flag word, for the one operation VC6
// will not narrow on its own.
//
// The field is sixteen bits: se_LoadFile, which creates the record, stores
// its nFlags there as a word (0x1002bf0b `mov %ax,0xc(%esi)`) and reads it
// back as one (0x1002bf58 `mov 0xc(%esi),%cx`), and declaring it `uint8`
// costs both of those. But the shipped clear of bit 0 is `andb $0xfe`
// (0x1002b4cb) where a `uint16` field gives us `andw $0xfffe`, so the
// operation was narrowed and ours is not.
//
// The narrowing is not uniform in our build either, which is what makes this
// worth writing down: `|= 1` on the same `uint16` field DOES come out as
// `orb $0x1` and matches the shipped bytes at 0x1002b699 (sm_UnloadModel)
// and 0x1002b84f without any help. VC6 narrows an OR with a small positive
// immediate and declines to narrow the AND whose immediate has an all-ones
// high byte — even though both are equally safe. So this is an
// immediate-width difference, not a claim that the record has a byte there;
// a byte-typed view at the clear site is simply the only spelling found that
// reproduces it, and it is what took se_RegisterModel from 193 bytes to
// byte-identical.
class CFileRefFlagView
{
public:
	uint8 m_pad_00[0x0c];
	uint8 m_Flags;                           // 0x0c  bit 0 = still loading
};



// 0x1001cb33 — a request the reader can be handed straight away: no stream,
// no context, no child name, the do-nothing callback, and all four flags set.
//
// The store order is the shipped order and is not the declaration order: the
// child name goes first, then the stream, then the CALLBACK POINTER, then the
// context, then the four flags. Both constants live in one register — `xor
// ecx,ecx` serves the three zero stores and the `push $1` hoisted to the top
// of the function is popped back into the same register for the four ones.
//
// THE THIRD AND FOURTH STORES LOOK SWAPPED AT /O1 AND ARE NOT, and only
// lithtech.exe can tell you so. At /O1 the two constants share one register,
// so the three zero stores HAVE to be emitted together before the `pop %ecx`
// turns that register into the ones — which puts +0x08 third and the
// immediate store to +0x04 last (0x1001cb33), whatever the source said. At
// /O2 the caller already holds zero in %ebx and one in %edi, nothing is
// grouped, and the source order survives: se_LoadModelData's expansion of
// this constructor at 0x0046aadf stores $se_NoChildModel to +0x04 BEFORE the
// zero to +0x08. That one transposition was the whole of the 583-byte
// function's difference.
//
// AND IT IS `inline`, which is what se_LoadModelData below needs. lithtech.exe
// (/O2) does NOT have a standalone copy of this constructor anywhere — the
// eight stores appear in line at 0x0046aac0, where `xor %ebx,%ebx` and
// `mov $1,%edi` hold the two constants and then serve the rest of that
// function (`cmp %ebx,%eax` for its null tests, `push %edi` for its ones).
// server.dll (/O1, /Os) calls it out of line at BOTH its sites — 0x1002b238
// inside se_LoadModelData and 0x1001c615 in another translation unit — and
// should carry on doing so, because the expansion GROWS the caller (581 bytes
// against our 561), which is the one thing /Os refuses. Defined above its
// caller because VC6's inliner is forward-only.
// (The body now lives in serverengine.h, above every caller; see there.)


// The bank every dependency link comes out of, and the lock that guards the
// lists. Two separate globals, and the addresses prove it: the bank object is
// at 0x100585d8 with its own critical section embedded at +0x20 (0x100585f8),
// and the list lock is the standalone twenty-four-byte one at 0x100585c0
// immediately before it. 0x10013fab is the bank's global initialiser — vptr,
// the embedded lock's constructor, then sb_Init2(&m_Bank, 0xc, 0x20, 0x20),
// which is where the record size comes from.
//
// THE SHAPE IS FORWARDER-OVER-WORKER, and the two images together are what
// say so. server.dll's vftable for this class (0x1004d7d8) does NOT point at
// the allocate and free bodies; it points at two thunks that do nothing but
// forward to them:
//
//     0x1001409c   jmp 0x100140d3                       (slot 1, 5 bytes)
//     0x100140a1   push 0x4(%esp); call 0x10014107;     (slot 2, 12 bytes)
//                  ret $4
//
// while lithtech.exe's vftable for the same class (0x004a2438) points STRAIGHT
// at a 101-byte and a 38-byte body — the thunks with their targets expanded
// into them. So the class overrides its base's slots with tiny forwarders and
// keeps the typed allocate/free as ordinary members, which is also what the
// call sites want: dl_MakeLink and dl_UnlinkAndFree reach the typed pair
// directly (0x10014053, 0x10014045) and at /O2 expand it (0x0043c500 is
// dl_MakeLink with the whole of Allocate in line, 0x0043c4b0 the same for
// Free), which is only possible if the typed pair is an inline candidate —
// i.e. declared IN-CLASS, as below. /Os declines both expansions at 52 and 38
// bytes, which is why server.dll still calls them, and that is the shipped
// proof that writing them in-class cannot cost server.dll anything.
//
// The forwarders are defined OUT of the class on purpose: that is the one
// construct /Ob1 never expands, so both bodies
// are emitted in both builds whether or not this translation unit ever emits
// a vftable to hold their addresses. In lithtech.exe the typed pair has no
// out-of-line body at all — every call site expands it — and it is the
// forwarders that carry the 101 and 38 bytes at 0x0043c5c0 and 0x0043c630.

// The base whose three slots after the destructor this bank fills in. It is
// servermgr.h's CBankBase — same vftable, 0x1004d260, with _purecall in all
// three slots — but that declaration spells the two middle slots `void _v04()`
// and `void _v08()`, and the thunks above prove this bank's are `void*` with
// no argument and `void` with one. An override has to match exactly, so the
// shared declaration cannot serve here without changing servermgr.h and every
// other bank that derives from it. Declared locally with the right signatures
// until that is in scope; the vftable addresses are relocations either way, so
// the emitted bytes are the same class of thing.
class CDepBankBase
{
public:
	virtual ~CDepBankBase() {}

	virtual void* _v04() = 0;
	virtual void  _v08(void* pStruct) = 0;
	virtual void  Term() = 0;
};

class CDepLinkBank : public CDepBankBase
{
public:
	// ---- from here to the definition of g_DepLinkCS below is one separable
	// addition: the bank's own lifetime, and the two globals this file's
	// address run says are defined here rather than merely referenced. See
	// the note above the constructor. ----
	//
	// 0x10013fab (50 bytes) is NOT a standalone constructor: it is the
	// compiler's _$E initialiser for g_DepLinkBank with this body expanded
	// into it — every store goes to an absolute address (`movl $vft,
	// 0x100585d8`, `push $0x100585dc`, `mov $0x100585f8,%ecx` for the
	// embedded lock) with no `this` register — and 0x10013fa1 is the _$E
	// that calls it and chains to the atexit registration at 0x10013fdd.
	// `__forceinline`, as fsstream.cpp's two stream banks (0x10006d88,
	// 0x10006df7, the same shape): plain `inline`, in class or out, leaves
	// a ten-byte `mov ecx; jmp` initialiser and a 45-byte constructor.
	// Twelve-byte records, thirty-two cached and thirty-two added at a time;
	// sb_Init2 and not sb_Init because the initialiser pushes four arguments
	// and cleans up sixteen bytes.
#ifdef PRIMAL_HUNT
	// PH: the cache and grow counts are the constructor's arguments, and
	// constructor and destructor are both out of line: the initialiser at
	// 0x00448610 is `push $0x20; push $0x20; mov $g_DepLinkBank,%ecx; call
	// 0x00448740`, and the atexit thunk 0x00448630 jumps to the destructor
	// at 0x00448800. AvP2 1.0.9.6 has NOLF's expanded shape.
	CDepLinkBank(uint32 nCacheSize, uint32 nGrowSize);   // 0x00448740
#else
	__forceinline CDepLinkBank()             // 0x10013fab (as _$E)
	{
		sb_Init2(&m_Bank, 0xc, 0x20, 0x20);
	}
#endif
	virtual ~CDepLinkBank();                 // 0x100140ad
	// ---- end of the separable addition's first half ----

	// 0x1001409c / 0x100140a1 — the vftable's own two entries. Defined below.
	virtual void* _v04();
	virtual void  _v08(void* pStruct);
	virtual void  Term();

	// 0x100140d3 — a zeroed link off the bank, under the bank's own lock. The
	// three clearing stores and the null test share one materialised zero
	// (`xor eax,eax; cmp eax,esi`), which is why the test is a compare against
	// a register rather than the `test esi,esi` an isolated `if (p)` would
	// give: the constant the stores need already exists.
	//
	// In-class, and above every caller, because lithtech.exe expands it into
	// dl_MakeLink and into the slot-1 forwarder; VC6's inliner is forward-only.
	CDepLink* Allocate()
	{
		CDepLink* pLink;

		m_Lock.Enter();

		pLink = (CDepLink*)sb_Allocate(&m_Bank);

		if (pLink)
		{
			pLink->m_pRecord = LTNULL;
			pLink->m_pData   = LTNULL;
			pLink->m_pNext   = LTNULL;
		}

		m_Lock.Leave();

		return pLink;
	}

	// 0x10014107 — and back again. The free is servermgr.h's inline sb_Free,
	// so there is no call: the record's own first dword becomes the next-free
	// link.
	void Free(void* pLink)
	{
		m_Lock.Enter();

		sb_Free(&m_Bank, pLink);

		m_Lock.Leave();
	}

	StructBank       m_Bank;                 // 0x04
	LCriticalSection m_Lock;                 // 0x20
};

// 0x1001409c and 0x100140a1 at /O1; 0x0043c5c0 (101 bytes) and 0x0043c630
// (38 bytes) at /O2, where the body each one forwards to has been expanded
// into it. The zero-argument slot tail-jumps and the one-argument slot cannot,
// which is exactly the pair of shapes the shipped thunks have.
//
// DO NOT SCORE THEM AT /O1. The first is five bytes of which four are the
// jump displacement, so a byte matcher's eight-unmasked-byte floor drops it before
// it is searched; the second has exactly eight and will be searched, but a
// `push 0x4(%esp); call; ret $4` thunk is a shape the image carries many
// copies of, so it can be sited on the wrong one. Neither is the reason
// these two exist. They exist because at /O2 they are the ONLY thing holding
// 0x0043c5c0 and 0x0043c630: every call of the typed pair is expanded there,
// so without a body that must be emitted whatever the inliner does, both of
// those addresses would go unclaimed.
void* CDepLinkBank::_v04()
{
	return Allocate();
}

void CDepLinkBank::_v08(void* pStruct)
{
	Free(pStruct);
}

// 0x1001412d — slot 3, the one the base calls Term. Empty the bank's chunks
// and set it up again from scratch, under the bank's own lock; the record
// size and cache depth are the constructor's, minus the grow count, so this
// is sb_Init and not sb_Init2. The two argument pushes are cleaned up
// together (`add $0x14,%esp`), which is why there is no stack adjust between
// the calls.
void CDepLinkBank::Term()
{
	m_Lock.Enter();

	sb_FreeChunks(&m_Bank, 1);
	sb_Init(&m_Bank, 0xc, 0x20);

	m_Lock.Leave();
}

// The list itself is one pointer and nothing else — dl_AddLink writes through
// it and dl_UnlinkAndFree hands its address to 0x1002446d as a `this`, which
// walks the chain through CDepLink::m_pNext at +0x08 and unlinks the node it
// was given. Declared, not defined; it lives outside the run this file covers
// (names ours).
class CDepList
{
public:
	void RemoveLink(void* pLink);            // 0x1002446d

	CDepLink* m_pHead;                       // 0x00
};

// Twelve-byte records, thirty-two of them cached and thirty-two added at a
// time. sb_Init2 and not sb_Init because the initialiser pushes four
// arguments and cleans up sixteen bytes.
//
// INLINE, and defined above the global it constructs. server.dll has no
// standalone constructor: 0x10013fab (50 bytes) is the compiler's _$E
// initialiser for g_DepLinkBank with this body expanded into it — every
// store is to an absolute address (`movl $vft,0x100585d8`, `push $0x100585dc`,
// `mov $0x100585f8,%ecx` for the embedded lock) rather than through a `this`
// register — and 0x10013fa1 is the _$E that calls it and then chains to the
// atexit registration at 0x10013fdd. Written out of line, VC6 emits a
// ten-byte `mov ecx; jmp ctor` initialiser plus a 45-byte constructor, and
// neither pairs.
// ---- second half of the separable addition ----
//
// DEFINED here, not declared: the run this file covers begins at 0x10013fa0
// and the first three things in it are this object's dynamic initialiser
// (0x10013fa1 calls the constructor at 0x10013fab and then registers the
// destructor thunk at 0x10013fe9, which tail-jumps to 0x100140ad) and the
// list lock's (0x10013ffd / 0x10014013). A global with a constructor is what
// puts them there, in this declaration order, and it is also what makes VC6
// emit the vftable that holds the two forwarders above.
//
// The constructor's bytes are the ordinary C++ order and confirm the base:
// the base vftable goes in first (0x1004d260, CBankBase's), then the embedded
// lock is constructed, then this class's own vftable (0x1004d7d8), then the
// body's single sb_Init2. The destructor unwinds exactly that.
// 0x100140ad — hand the chunks back and let the embedded lock and the base
// take themselves down. A standalone body at /O1 (the atexit thunk jumps to
// it and the compiler's deleting destructor at 0x1001415d calls it), but
// `inline`, and defined above the global: lithtech.exe's atexit thunk
// (0x0043c440) carries the whole teardown at the global's own addresses --
// the vptr store, sb_FreeChunks, the lock's destructor, the base vptr --
// with no `this` register and no destructor of its own.
#ifdef PRIMAL_HUNT
CDepLinkBank::CDepLinkBank(uint32 nCacheSize, uint32 nGrowSize)
{
	sb_Init2(&m_Bank, 0xc, nCacheSize, nGrowSize);
}

// PH: 0x00448800, out of line.
CDepLinkBank::~CDepLinkBank()
{
	sb_FreeChunks(&m_Bank, 1);
}

CDepLinkBank    g_DepLinkBank(0x20, 0x20);   // 0x004f5460
#else
inline CDepLinkBank::~CDepLinkBank()
{
	sb_FreeChunks(&m_Bank, 1);
}

CDepLinkBank    g_DepLinkBank;               // 0x100585d8
#endif
LCriticalSection g_DepLinkCS;                // 0x100585c0

// ---- end of the separable addition ----

// 0x1002446d — take one link back out of the list. The cursor is a pointer
// *to* the link pointer, which is what makes the head need no special case
// (`this` is the first cursor, at offset zero) and what makes the walk
// advance into the node with `lea 0x8(%eax),%ecx` instead of keeping a
// previous-node register.
void CDepList::RemoveLink(void* pLink)
{
	CDepLink** ppCur = &m_pHead;
	CDepLink*  pCur;

	while ((pCur = *ppCur) != LTNULL)
	{
		if (pCur == (CDepLink*)pLink)
		{
			*ppCur = pCur->m_pNext;
			return;
		}

		ppCur = &pCur->m_pNext;
	}
}

// 0x1001404e — a link with its two payload words filled in. The record comes
// back already zeroed, so the next pointer needs no store here; an allocation
// that fails is returned as it is, which is why the null test guards only the
// two stores and not the return.
//
// This and dl_AddLink sit in a contiguous run of their own in the shipped
// image (0x10013fa0-0x10014107, the list module) rather than beside the se_
// family — they are here because these are the files this reconstruction
// owns, and the byte comparison scans the whole image regardless of which object a
// function is compiled into.
void* dl_MakeLink(void* pRecord, void* pData)
{
#ifdef PRIMAL_HUNT
	// PH: through the bank's slot-1 entry, called directly on the global
	// (0x004486e0 `mov $0x4f5460,%ecx; call 0x00448860`), not expanded.
	CDepLink* pLink = (CDepLink*)g_DepLinkBank._v04();
#else
	CDepLink* pLink = g_DepLinkBank.Allocate();
#endif

	if (pLink)
	{
		pLink->m_pRecord = pRecord;
		pLink->m_pData   = pData;
	}

	return pLink;
}

// 0x1001401d — the callback of the second dependency record (0x10052df8):
// take the link back out of its list and, if the fourth argument says so,
// give the record back to the bank. Name ours, and it is what the bytes do
// rather than a guess at intent.
//
// The signature is four wide, which is the evidence serverengine.h's
// CDepRecord comment refers to: the gate is the third argument
// (`cmpl $0x0,0x10(%ebp)` before anything else is read), the first is the
// list — it goes straight into %ecx as a `this` — the second is the link,
// and the fourth is a pointer whose first dword decides whether the link is
// freed. se_ModelHashCleanup, reached the same way off the other record,
// reads only the second and third, so one declaration serves both and the
// extra parameter costs nothing there.
//
// Unlike every other function in this file the shipped code carries a real
// `push ebp; mov esp,ebp` frame with no locals at all — the argument pushes
// happen between the two uses of each argument and VC6 declined FPO for it.
LTRESULT dl_UnlinkAndFree(void* pList, CDepLink* pLink, int nArg3, int* pnFreeIt)
{
	if (nArg3 == 0)
	{
		((CDepList*)pList)->RemoveLink(pLink);

		if (pnFreeIt && *pnFreeIt)
			g_DepLinkBank.Free(pLink);
	}

	return LT_OK;
}

// 0x1001406a — put a link on a list, under the list lock. The list is one
// pointer and nothing else, so this is a head insertion; see serverengine.h
// on the name. A null link is refused rather than crashing, and that refusal
// is the only thing the return value ever says.
LTRESULT dl_AddLink(void* pList, void* pLink)
{
	if (pLink)
	{
		g_DepLinkCS.Enter();

		((CDepLink*)pLink)->m_pNext = *(CDepLink**)pList;
		*(CDepLink**)pList = (CDepLink*)pLink;

		g_DepLinkCS.Leave();

		return LT_OK;
	}

	return LT_ERROR;
}


// 0x1002bd75 — a stream on the file a file record names. The record already
// knows which mounted tree it came out of, so nothing is searched: the tree
// is followed through +0x10 and the name is asked of the file manager, and
// the two are handed to the tree's own opener. The third argument is a flag
// both this and fs_OpenFile pass zero for.
//
// Both cdecl cleanups batch into the one `add $0x14,%esp`, which is what says
// both callees have fixed prototypes.
//
// Like dl_MakeLink above, the shipped address sits in the file manager's run
// (0x1002bc12-0x1002bfcd) rather than beside the se_ family at 0x1002af75;
// it is here because these are the files this reconstruction owns.
ILTStream* se_OpenStream(CFileMgr* pFileMgr, void* pFile)
{
	char* pFilename = fs_GetFilename(pFileMgr, pFile);

	return fs_OpenStreamInTree(((CFileRef*)pFile)->m_pTreeRec->m_pTree,
	                           pFilename, 0);
}


// A sprite that will not load is not fatal: the engine has one of its own to
// put there instead, and only says so once. Failing that there is nothing to
// draw and the object cannot be made.
LTRESULT se_InitSprite(CServerMgr* pMgr, LTObject* pObj,
                       ObjectCreateStruct* pStruct)
{
	char* pFilename = pStruct->m_Filename;

	((SpriteInstance*)pObj)->m_nCurFrame = 0xffffffff;

	if (!se_LoadFile(&pMgr->m_FileMgr, pFilename, 0, &pObj->m_pSData->m_pModelFile))
	{
		if (g_DebugLevel >= 1)
			dsi_ServerError("Couldn't find sprite file %s, using sprites/default.spr.",
			                pFilename);

		if (!se_LoadFile(&pMgr->m_FileMgr, "sprites\\default.spr", 0,
		                 &pObj->m_pSData->m_pModelFile))
		{
			RETURN_ERROR_SYM_DESC(1, "se_InitSprite", LT_MISSINGFILE,
			                      "sprites\\default.spr");
		}
	}

	return LT_OK;
}

// The model instance's model pointer where the shipped objects keep it:
// Primal Hunt +0x1fc (0x0047f88e; server.dll 0x1003e76b), AvP2 1.0.9.6
// +0x1f8 (0x00478cfe; server.dll 0x1003b5b3). serverengine.h's
// ModelInstanceObj still has NOLF's 0x1e4.
class ModelInstanceViewQ507
{
public:
#ifdef PRIMAL_HUNT
	uint8  m_pad_000[0x1fc];
#else
	uint8  m_pad_000[0x1f8];
#endif
	Model* m_pModel;                         // PH 0x1fc, AVP2 0x1f8
};

// Letting go of a model is only a count: the model itself outlives every
// instance of it, and something else decides when it goes.
LTRESULT se_TermModel(CServerMgr* pMgr, LTObject* pObj)
{
	if (!pObj)
		return LT_ERROR;

	// AVP2: the count is not taken below zero, and the test is unsigned
	// (`test ecx,ecx ; jbe`, 0x0047f89a; AvP2 1.0.9.6 0x00478d0a): the
	// shipped field is a uint32 where commonmodel.h declares an int.
	Model* pModel = ((ModelInstanceViewQ507*)pObj)->m_pModel;

	if (pModel->m_CacheCount > 0)
		pModel->m_CacheCount--;

	return LT_OK;
}

// A container is a world model, and is made the same way; there is nothing
// of its own to set up here.
LTRESULT se_InitContainer(CServerMgr* pMgr, LTObject* pObj,
                          ObjectCreateStruct* pStruct)
{
	return se_InitWorldModel(pMgr, pObj, pStruct);
}

// Paths arrive written either way round and the file manager only knows one
// of them, so every name is folded to backslashes before it is looked up.
// The walk is destructive — the caller's own buffer is fixed in place — and
// what comes back is where it started (name ours).
char* se_FixSlashes(char* pPath)
{
	char* pRet;

	if (!pPath)
		return LTNULL;

	pRet = pPath;

	while (*pPath)
	{
		if (*pPath == '/')
			*pPath = '\\';

		pPath++;
	}

	return pRet;
}

// A world model is geometry that came in with the level rather than a file of
// its own, so this is a lookup and not a load: the create struct's filename
// is the name it went in under.
//
// Once it is attached, the object is given the geometry's own idea of where
// it is — the BSP's centre — and sized to half the span of its box, which is
// what a half-extent is. A world model whose BSP reports itself busy is left
// alone rather than re-placed.
// AVP2: the move state's +0x68 is cleared with +0x64 (lithtech.exe
// 0x0047f9a7..0x0047f9ae, AvP2 1.0.9.6 0x00478e19..0x00478e20; the frame is
// 0x90, not NOLF's 0x8c).
//
// NEAR (402/402, lithtech.exe 0x0047f970): ebx and edi swap -- shipped keeps
// the zero in ebx and pMgr in edi. The stores in either order and the
// locals declared in either order all leave it.

LTRESULT se_InitWorldModel(CServerMgr* pMgr, LTObject* pObj,
                           ObjectCreateStruct* pStruct)
{
	LTVector        vHalfDims;
	CWorldBsp*      pBsp;
	WMMoveState     state;

	if (pMgr->m_World.AttachWorldModel(pObj, pStruct->m_Filename))
	{
		// POSITIVE GUARD. 0x0046b330's `jne 0x0046b46d` sends the BUSY case
		// forward past the whole body to the single `xor %eax,%eax` exit, so
		// `return LT_OK` is a trailing statement and the test is negated.
		// Written as an early `if (IsBusy()) return LT_OK;` the epilogue is
		// duplicated -- worth 8 bytes in lithtech.exe's /O2 build, and
		// invisible in server.dll's /O1 build, which tail-merges the two.
		if (!pObj->m_pWorldBsp->IsBusy())
		{
		pBsp = (CWorldBsp*)pObj->m_pWorldBsp;

		vHalfDims = (pBsp->m_vBoxMax - pBsp->m_vBoxMin) * 0.5f;

		pObj->SetPos(pBsp->m_ve8);

		state.m_pContainer = &pMgr->m_Unknown210;
		state.m_pUnknown04 = pMgr->m_pUnknown6a8;
		state.m_pObj       = pObj;
		state.m_nUnknown0C = pObj->m_BlockingPriority;
		state.m_pPushList  = LTNULL;
		state.m_nPushCount = 0;

		wm_SetObjectDims(&state, &vHalfDims, 0, 1);
		}

		return LT_OK;
	}

	RETURN_ERROR(1, "se_InitWorldModel", LT_MISSINGWORLDMODEL);
}

// Byte-identical on both images (server 0x1002b4dc, lithtech 0x0046adf0).
// Three separate `return LT_INPROGRESS`s -- not wanted, already queued, just
// queued -- and the synchronous load last, so the `xor eax,eax` and the
// epilogue sit at the very end as at 0x1002b5f7.
//
// The whole body is one explicit if/else chain with every arm, down to the
// queued test and the load's own error test, ending in its own return. That
// is what puts the shared `pop eax; jmp` of LT_MISSINGFILE / LT_INPROGRESS
// at the FIRST of them, the error (0x1002b577), with the in-progress returns
// jumping back into it. Chaining only the outer tests left the inner ones as
// guarded returns, and VC6 hosted the tail at the last in-progress return.
//
// `done` is in its own block so it shares -0x20 with `request` (frame 0x40 at
// 0x1002b4df); at function scope the frame grows to 0x60.
//
// A model by name. Every model the server has loaded is in one hash keyed by
// its filename, so the same model asked for twice is the same object with a
// second reference rather than a second load.
//
// A model that is not there yet is opened, and then one of three things
// happens: it is turned into a model right here, it is queued for the worker
// thread, or — if the caller did not ask for either — nothing, and the answer
// is that it is on its way. Asking twice while it is on its way does not
// queue it twice: the queue itself is searched.
//
// AVP2: no message is built at the top (NOLF's `msg` was constructed and
// never used), and a model loaded here registers itself directly rather
// than through OnModelLoaded (lithtech.exe 0x0047f505..0x0047f523, AvP2
// 1.0.9.6 0x004789ff).
LTRESULT se_LoadModel(CServerMgr* pMgr, char* pFilename, Model** ppModel,
                      void** ppFile, int bWantLoad, int bLoadHere)
{
	void* hElement;

	*ppModel = LTNULL;
	*ppFile  = LTNULL;

	se_FixSlashes(pFilename);

	hElement = dhash_FindElement(pMgr->m_hModelHash, pFilename,
	                             strlen(pFilename));

	if (hElement)
	{
		*ppModel = (Model*)dhash_GetData(hElement);

		return LT_OK;
	}
	else if (!se_LoadFile(&pMgr->m_FileMgr, pFilename, 0, ppFile))
	{
		RETURN_ERROR_SYM_DESC(1, "se_LoadModel", LT_MISSINGFILE, pFilename);
	}
	else if (!bWantLoad)
	{
		return LT_INPROGRESS;
	}
	else if (!bLoadHere)
	{
		if (pMgr->m_Thread.IsRequestQueued(*ppFile))
		{
			return LT_INPROGRESS;
		}
		else
		{
			LThreadMessage request;

			request.m_Data[0] = 0;
			request.m_Data[2] = 0;
			request.m_Data[3] = (uint32)*ppFile;

			pMgr->m_Thread.m_IncomingQueue.PostMessage(&request);

			return LT_INPROGRESS;
		}
	}
	else
	{
		LTRESULT dResult = se_LoadModelData(pMgr, pFilename, *ppFile, ppModel);

		if (dResult == LT_OK)
			se_RegisterModel(pMgr, *ppModel, *ppFile, 1);

		return dResult;
	}
}

// LTAnimTracker::IsValid, the same inline body modelanim.cpp defines. It has
// to be visible here: lithtech.exe (/O2) expands it in both of se_InitModel's
// tracker loops (0x0046b020), which it can only do from an inline definition
// in this translation unit; server.dll (/O1) still declines it at 36 bytes
// and calls the out-of-line copy (0x1002b7e8 -> 0x100010ec). Measured:
// lithtech se_InitModel 487 -> 537 of 541 bytes, 15 mnemonic blocks -> 2;
// server unchanged. The original probably had this in a shared header.
inline LTBOOL LTAnimTracker::IsValid()
{
	Model* pModel = m_TimeRef.m_pModel;

	if (!pModel ||
	    m_TimeRef.m_Cur.m_iAnim >= pModel->m_AnimArray.m_nElements ||
	    m_TimeRef.m_Prev.m_iAnim >= pModel->m_AnimArray.m_nElements)
	{
		return LTFALSE;
	}

	return LTTRUE;
}

// AVP2 (lithtech 0x0047f600, server 0x10036fb1): four skins, Primal Hunt's
// environment map after them, and a word set past the trackers -- see the
// AVP2/PH notes in the body. NEAR (server 544 of 544, lithtech 629 of 633)
// on the one residual below, which is unchanged.
//
// NEAR (NOLF: 460 bytes against 460, and the same hundred and fifty
// instructions): four instructions, one disagreement -- the shipped build loads the skin
// slot's offset and adds the object data to it, ours loads the object data
// and adds the offset. Both spellings of the sum, a byte-pointer sum, an
// `int` offset (worse: 5 blocks), a `for` loop, and an index loop over
// `(&m_pSData->m_pSkinFile)[i]` all give the same bytes.
//
// Giving a model object its model. Three files are involved and each has its
// own fallback: the model itself falls back to the engine's default, and each
// skin to the default skin. A missing skin is reported whatever the debug
// level, unlike everything else here — a model with no texture is visible
// enough to be worth saying out loud every time.
//
// The two skin slots are walked by byte offset into the object data rather
// than by index, which is what the shipped loop counter and its 0x3c bound
// are counting.
//
// What comes back decides how the trackers are set up. A model that loaded
// here is what they are pointed at; a model still on its way leaves them on
// the placeholder, marked so that they are redone when it arrives, and the
// file itself is marked too. Either way the model gains a reference and the
// instance's own tracker is pointed back at the instance.
// 0x1002b6a4. pFileMgr is assigned AFTER pFilename, not initialised at its
// declaration: that moves pMgr's scratch load from eax to ecx, as at
// 0x1002b6aa (six differing instructions down to four). The remaining operand
// order of `nOff + (uint32)pObj->m_pSData` is below what the source reaches.
//
// lithtech.exe (0x0046b020): with IsValid's inline body visible (above) the
// only residual is the same sum in /O2 dress -- shipped `mov 0x188(%ebp),%edx;
// mov %esi,%ecx; add %edx,%ecx` (0x46b0e1), nOff copied and accumulated,
// against our `mov 0x188(%ebp),%ecx; add %esi,%ecx` (537 of 541 bytes). Every
// spelling of the sum gives the same order on both images: nOff as uint8*,
// `(int)`, `(uint32)(void*)`, `*(uint32*)&`, `(uint32)p + nOff`,
// `(uint8*)p + nOff`, `nOff - -(int)p`, even `|` instead of `+`, a file-local
// inline helper returning the slot, and an index loop over both the names and
// `(&m_pSkinFile)[i]` (VC6 strength-reduces it to this same counter).
//
// Two dead parameter slots carry locals: 0xc(%ebp) the running offset (the
// `movl $0x34` is `nOff`) and 0x10(%ebp) the filename, pObj and pStruct both
// living in callee-saved registers from the top.
LTRESULT se_InitModel(CServerMgr* pMgr, LTObject* pObj,
                      ObjectCreateStruct* pStruct)
{
	CFileMgr*      pFileMgr;
	ModelInstance* pInst    = (ModelInstance*)pObj;
	LTAnimTracker* pTracker;
	char*          pFilename;
	char*          pSkinName;
	Model*         pModel;
	void*          pFile;
	uint32         nOff;
	LTRESULT       dResult;

	pFilename = pStruct->m_Filename;
	pFileMgr  = &pMgr->m_FileMgr;

	if (!se_LoadFile(pFileMgr, pFilename, 0, &pObj->m_pSData->m_pModelFile))
	{
		if (g_DebugLevel >= 1)
		{
			dsi_ServerError("Couldn't find model file %s.  Trying models/default.abc",
			                pFilename);
		}

		if (!se_LoadFile(pFileMgr, "models\\default.abc", 0,
		                 &pObj->m_pSData->m_pModelFile))
		{
			RETURN_ERROR_SYM_DESC(1, "se_InitModel", LT_MISSINGFILE, pFilename);
		}

		pFilename = "models\\default.abc";
	}

	nOff      = 0x34;
	pSkinName = pStruct->m_SkinNames[0];

	do
	{
		if (*pSkinName)
		{
			if (!se_LoadFile(pFileMgr, pSkinName, 0,
			                 (void**)(nOff + (uint32)pObj->m_pSData)))
			{
				dsi_ServerError("Couldn't find model skin %s.", pSkinName);

				se_LoadFile(pFileMgr, "skins\\default.dtx", 0,
				            (void**)(nOff + (uint32)pObj->m_pSData));
			}
		}

		nOff      += 4;
		pSkinName += 0x80;
	}
	while (nOff < 0x44);	// AVP2: four skins, 0x34..0x40 (0x0047f70e; AvP2 0x00478be6)

#ifdef PRIMAL_HUNT
	// PH: the extra texture after the skins is the model's environment map,
	// into the object data's +0x44, with the default skin for a missing one
	// (0x0047f713..0x0047f763; the string is in no AvP2 1.0.9.6 image).
	pSkinName = pStruct->m_ExtraTextureName;

	if (*pSkinName)
	{
		if (!se_LoadFile(pFileMgr, pSkinName, 0,
		                 (void**)((uint8*)pObj->m_pSData + 0x44)))
		{
			dsi_ServerError("Couldn't find model environment map %s.", pSkinName);

			se_LoadFile(pFileMgr, "skins\\default.dtx", 0,
			            (void**)((uint8*)pObj->m_pSData + 0x44));
		}
	}
#endif

	dResult = se_LoadModel(pMgr, pFilename, &pModel, &pFile,
	                       pStruct->m_CreateFlags & OCS_AUTOLOAD,
	                       pStruct->m_CreateFlags & OCS_AUTOLOAD);

	if (!dResult)
	{
		for (pTracker = pInst->m_pTrackerList; pTracker;
		     pTracker = pTracker->GetNext())
		{
			if (!pTracker->IsValid() || pTracker->GetModel() != pModel)
				at_Init(pTracker, pModel, LTFALSE);
		}
	}
	else if (dResult == LT_INPROGRESS)
	{
		for (pTracker = pInst->m_pTrackerList; pTracker;
		     pTracker = pTracker->GetNext())
		{
			if (!pTracker->IsValid() || pTracker->GetModel() != pModel)
			{
				at_Init(pTracker, pMgr->m_pPlaceholderModel, LTFALSE);

				pTracker->m_Flags |= 8;
			}
		}

		((CFileRef*)pObj->m_pSData->m_pModelFile)->m_Flags |= 1;
	}
	else
	{
		return dResult;
	}

	pModel->m_CacheCount++;

	pInst->m_AnimTracker.m_Flags  |= 4;
	// AVP2: a word past the instance's trackers set to one (PH +0x2e0,
	// 0x0047f85f; AvP2 1.0.9.6 +0x2cc, 0x00478cd5; name ours).
#ifdef PRIMAL_HUNT
	*(uint32*)((uint8*)pInst + 0x2e0) = 1;
#else
	*(uint32*)((uint8*)pInst + 0x2cc) = 1;
#endif
	pInst->m_AnimTracker.m_pUser1  = pInst;

	return LT_OK;
}

// AVP2: the load request is 0x24 bytes, not serverengine.h's 0x20. A dword
// the manager's server interface answers comes in at +0x0c and pushes the
// child name to +0x10 and the four flags to +0x14..+0x20: se_LoadModelData
// stores them at 0x0047ef8c..0x0047efb0 (child name first and the new word
// last, after the flags), fills +0x0c at 0x0047f107 and tests +0x18 and +0x20
// (0x0047f156, 0x0047f1cb); se_LoadChildModel reads the name at +0x10
// (0x0047ec8b). AvP2 1.0.9.6 has the same stores (0x004784d7). serverengine.h
// is shared with commonmodel.cpp and cmodelload.cpp, so the shape is a view
// here until the header moves (names ours).
class CModelLoadRequestQ507
{
public:
	CModelLoadRequestQ507()
	{
		m_pChildName     = LTNULL;
		m_pStream        = LTNULL;
		m_pfnLoadChild   = se_NoChildModel;
		m_pContext       = LTNULL;
		m_bUnknown14     = LTTRUE;
		m_bTreesValid    = LTTRUE;
		m_bUnknown1C     = LTTRUE;
		m_bChildrenFound = LTTRUE;
		m_Unknown0C      = 0;
	}

	ILTStream* m_pStream;                    // 0x00
	LTRESULT (*m_pfnLoadChild)(CModelLoadRequest* pReq,
	                          Model** ppModel); // 0x04
	CModelLoadContext* m_pContext;           // 0x08
	uint32 m_Unknown0C;                      // 0x0c  from the server interface
	char*  m_pChildName;                     // 0x10
	LTBOOL m_bUnknown14;                     // 0x14
	LTBOOL m_bTreesValid;                    // 0x18
	LTBOOL m_bUnknown1C;                     // 0x1c
	LTBOOL m_bChildrenFound;                 // 0x20
};

// What the manager keeps at +0xd38 (servermgr.h's CUnknown928Obj), reached
// at the one slot se_LoadModelData calls with nothing pushed: +0x194 in
// Primal Hunt (0x0047f101), +0x18c in AvP2 1.0.9.6 (0x00478618) -- the PH
// server interface's two extra slots, as everywhere past slot 44 (names
// ours).
class CServerInterfaceViewQ507
{
public:
	virtual void _v000()=0; virtual void _v004()=0; virtual void _v008()=0; virtual void _v00c()=0;
	virtual void _v010()=0; virtual void _v014()=0; virtual void _v018()=0; virtual void _v01c()=0;
	virtual void _v020()=0; virtual void _v024()=0; virtual void _v028()=0; virtual void _v02c()=0;
	virtual void _v030()=0; virtual void _v034()=0; virtual void _v038()=0; virtual void _v03c()=0;
	virtual void _v040()=0; virtual void _v044()=0; virtual void _v048()=0; virtual void _v04c()=0;
	virtual void _v050()=0; virtual void _v054()=0; virtual void _v058()=0; virtual void _v05c()=0;
	virtual void _v060()=0; virtual void _v064()=0; virtual void _v068()=0; virtual void _v06c()=0;
	virtual void _v070()=0; virtual void _v074()=0; virtual void _v078()=0; virtual void _v07c()=0;
	virtual void _v080()=0; virtual void _v084()=0; virtual void _v088()=0; virtual void _v08c()=0;
	virtual void _v090()=0; virtual void _v094()=0; virtual void _v098()=0; virtual void _v09c()=0;
	virtual void _v0a0()=0; virtual void _v0a4()=0; virtual void _v0a8()=0; virtual void _v0ac()=0;
	virtual void _v0b0()=0; virtual void _v0b4()=0; virtual void _v0b8()=0; virtual void _v0bc()=0;
	virtual void _v0c0()=0; virtual void _v0c4()=0; virtual void _v0c8()=0; virtual void _v0cc()=0;
	virtual void _v0d0()=0; virtual void _v0d4()=0; virtual void _v0d8()=0; virtual void _v0dc()=0;
	virtual void _v0e0()=0; virtual void _v0e4()=0; virtual void _v0e8()=0; virtual void _v0ec()=0;
	virtual void _v0f0()=0; virtual void _v0f4()=0; virtual void _v0f8()=0; virtual void _v0fc()=0;
	virtual void _v100()=0; virtual void _v104()=0; virtual void _v108()=0; virtual void _v10c()=0;
	virtual void _v110()=0; virtual void _v114()=0; virtual void _v118()=0; virtual void _v11c()=0;
	virtual void _v120()=0; virtual void _v124()=0; virtual void _v128()=0; virtual void _v12c()=0;
	virtual void _v130()=0; virtual void _v134()=0; virtual void _v138()=0; virtual void _v13c()=0;
	virtual void _v140()=0; virtual void _v144()=0; virtual void _v148()=0; virtual void _v14c()=0;
	virtual void _v150()=0; virtual void _v154()=0; virtual void _v158()=0; virtual void _v15c()=0;
	virtual void _v160()=0; virtual void _v164()=0; virtual void _v168()=0; virtual void _v16c()=0;
	virtual void _v170()=0; virtual void _v174()=0; virtual void _v178()=0; virtual void _v17c()=0;
	virtual void _v180()=0; virtual void _v184()=0; virtual void _v188()=0;
#ifdef PRIMAL_HUNT
	// PH: the two extra slots.
	virtual void _vX1()=0; virtual void _vX2()=0;
#endif
	virtual uint32 GetLoadWordQ507()=0;      // PH +0x194, AVP2 +0x18c
};

// Reading a model file into a Model. The model is built before the file is
// read, so a failure anywhere below has one thing to undo, and it undoes it
// the same way every time.
//
// Three separate things can be wrong and each says so differently: the file
// would not open, the reader itself failed (and reports its own code), or the
// child models the file names did not resolve — which is two faults, not one,
// because a file can name a tree that is malformed and a file can name
// children that are simply not there.
LTRESULT se_LoadModelData(CServerMgr* pMgr, char* pFilename, void* pFile,
                          Model** ppModel)
{
	CModelLoadRequestQ507 request;
	CModelLoadContext     context;
	Model*            pModel;
	ILTStream*        pStream;
	LTRESULT          dResult;
	void*             pRaw;

	*ppModel = LTNULL;

	// `new`, not `dalloc` plus a placement new: the shipped call goes to
	// 0x1000d25d, which is the eleven-byte `operator new` forwarder, and so
	// do both of se_LoadChildModel's allocations below (0x1002b0d2 and
	// 0x1002b19a). Both spellings are byte-identical because the operand is a masked
	// relocation; only the call targets separate them.
	pModel = new Model((uint32*)&g_DefAlloc, (uint32*)&g_DefAlloc);

	if (!pModel)
		RETURN_ERROR(1, "se_LoadModelData", LT_OUTOFMEMORY);

	se_FixSlashes(pFilename);

	pStream = se_OpenStream(&pMgr->m_FileMgr, pFile);

	request.m_pStream = pStream;

	if (!pStream)
	{
		delete pModel;

		if (g_DebugLevel >= 1)
			dsi_ServerError("Couldn't open model file %s", pFilename);

		RETURN_ERROR_SYM_DESC(1, "se_LoadModelData", LT_MISSINGFILE, pFilename);
	}

	dResult = pModel->Load(pStream, (IMoAllocator*)&g_DefAlloc);

	if (!dResult)
	{
		context.m_pServerMgr = pMgr;
		context.m_pFilename  = pFilename;

		request.m_pfnLoadChild = se_LoadChildModel;
		request.m_pContext     = &context;

		// AVP2: 0x0047f0e1..0x0047f107, and the report at 0x0047f10b.
		request.m_Unknown0C =
			((CServerInterfaceViewQ507*)pMgr->m_pUnknown928)->GetLoadWordQ507();

		if (g_DebugLevel >= 3)
			dsi_ServerError("Calling Model::Load():  %s\n", pFilename);

		dResult = pModel->LoadChildModels((CModelLoadRequest*)&request);
	}

	request.m_pStream->Release();

	if (!dResult && pModel->SetFilename(pFilename))
	{
		if (request.m_bTreesValid == 0)
		{
			delete pModel;

			if (g_DebugLevel >= 1)
			{
				dsi_ServerError("Child model trees invalid in model %s",
				                pFilename);
			}

			RETURN_ERROR(1, "se_LoadModelData", LT_INVALIDMODELFILE);
		}

		if (request.m_bChildrenFound == 0)
		{
			delete pModel;

			if (g_DebugLevel >= 1)
			{
				dsi_ServerError("Missing one or more child models in model %s",
				                pFilename);
			}

			RETURN_ERROR(1, "se_LoadModelData", LT_INVALIDMODELFILE);
		}

		pModel->m_FileFlags = ((CFileRef*)pFile)->m_nFileID;

		*ppModel = pModel;

		return LT_OK;
	}

	delete pModel;

	if (g_DebugLevel >= 1)
		dsi_ServerError("Error %d loading model %s", dResult, pFilename);

	return dResult;
}

// The shell's interface, modelled far enough to reach the one slot the hook
// below uses. servermgr.h only forward-declares IServerShell and is not a
// file this reconstruction owns, so the shape lives here under a name of its
// own and the call goes through a cast.
//
// iservershell.h's declaration order — destructor, ServerAppMessageFn,
// OnAddClient, OnRemoveClient, OnClientEnterWorld, OnClientExitWorld,
// PreStartWorld, PostStartWorld, OnMessage, OnObjectMessage, OnCommandOn,
// OnCommandOff, Update, OnPlaybackFinish, CacheFiles, SRand — puts
// FileLoadNotify at slot 16, which is exactly the 0x40 the shipped call
// indexes, and its two parameters are the two the shipped code pushes: a
// filename and an LTRESULT. The shell's vtable lives in the game DLL, so
// that agreement is the only check available on the slot.
class IShellFileNotify
{
public:
	virtual ~IShellFileNotify() = 0;                      // 0x00
	virtual LTRESULT _p01()=0; virtual LTRESULT _p02()=0;
	virtual LTRESULT _p03()=0; virtual LTRESULT _p04()=0; virtual LTRESULT _p05()=0;
	virtual LTRESULT _p06()=0; virtual LTRESULT _p07()=0; virtual LTRESULT _p08()=0;
	virtual LTRESULT _p09()=0; virtual LTRESULT _p10()=0; virtual LTRESULT _p11()=0;
	virtual LTRESULT _p12()=0; virtual LTRESULT _p13()=0; virtual LTRESULT _p14()=0;
	virtual LTRESULT _p15()=0;
	// AVP2: as Primal Hunt (PH addresses) -- three more slots before it, in both images (lithtech.exe
	// 0x0048b550 and server.dll 0x100403b0 call +0x4c).
	virtual LTRESULT _pX16()=0; virtual LTRESULT _pX17()=0;
	virtual LTRESULT _pX18()=0;
	// 0x40 — the file, and how it went.
	virtual LTRESULT FileLoadNotify(char* pFilename, LTRESULT status)=0;
};

// 0x10032dcd — a model that finished loading on this thread rather than on
// the worker's, taking the very message the worker would have posted back.
// Two of the message's words gate it and two carry the payload: the request
// word at m_Data[2] must be zero or the message is not ours, and m_Data[0]
// is the load's own result — zero means the model at m_Data[4] is real and
// gets registered under the file at m_Data[3], nonzero means it is not.
//
// Both arms tell the game shell, and they get the name from different places
// because on the failing path there is no model to ask: the file record's own
// hash element is what remembers what was being loaded.
//
// A shell that is not there yet is skipped, and each arm tests for that
// separately — which is what the two `test %esi,%esi` are.
void CServerMgr::OnModelLoaded(LThreadMessage* pMsg)
{
	IShellFileNotify* pShell = (IShellFileNotify*)m_ClassMgr.m_pServerShell;
	CFileRef*         pFile  = (CFileRef*)pMsg->m_Data[3];

	if (pMsg->m_Data[2] == 0)
	{
		Model* pModel = (Model*)pMsg->m_Data[4];

		if (pMsg->m_Data[0] == 0)
		{
			se_RegisterModel(this, pModel, pFile, 1);

			if (pShell)
				pShell->FileLoadNotify(pModel->GetFilename(), LT_OK);
		}
		else
		{
			if (pShell)
			{
				char* pFilename = dhash_GetKey(pFile->m_hElement, LTNULL);

				pShell->FileLoadNotify(pFilename, LT_ERROR);
			}
		}
	}
}


// 0x1002af75 — a model going away takes itself back out of the manager's
// hash. The link carries the eight-byte record se_LoadChildModel allocated;
// the model's own name is the hash key, so it is read *before* the record is
// freed and the key is then looked up and removed.
//
// Two things the shipped code does that a careful reader would not. It takes
// the server manager from the global rather than from the record it was
// stored in — the record's first word is written by se_LoadChildModel and
// read by nothing. And a null global is simply a no-op, so a cleanup running
// after the manager has gone leaves the record leaked.
//
// One shared exit serves all three ways out, which is what says the guards
// nest rather than each returning.
//
// The manager local is assigned *unconditionally* and the guard is written
// against the global itself, which looks redundant and is not: it is the one
// spelling that reproduces the shipped bytes. VC6 commons the two reads into
// the five-byte `mov 0x...,%eax` accumulator form, copies it into the
// callee-saved register in the slot between the `test` and the `je`, and so
// runs the copy on the failing path too. Written the obvious way —
// `pMgr = g_pServerMgr; if (pMgr)` — it loads straight into the callee-saved
// register with the six-byte encoding, one byte short and no copy at all;
// written with both reads inside the guard it reloads the global twice and
// comes out six bytes long.
LTRESULT se_ModelHashCleanup(void* pArg1, CDepLink* pLink, int nArg3)
{
#ifdef AVP2_MODERN
	// fixed: se_ModelHashCleanup leaks its record (RETAIL_BUGS.md). The
	// record is freed whether or not there is still a manager. The hash is
	// still reached through the global, not the record's copy: once the
	// manager has gone the global is null, and the copy would dangle.
	if (nArg3 == 0)
	{
		CModelCleanupRec* pRec = (CModelCleanupRec*)pLink->m_pData;
		CServerMgr*       pMgr = g_pServerMgr;

		if (pMgr)
		{
			char* pFilename = pRec->m_pModel->GetFilename();
			void* hElement  = dhash_FindElement(pMgr->m_hModelHash, pFilename,
			                                    strlen(pFilename));

			if (hElement)
			{
				if (g_DebugLevel >= 3)
					dsi_ServerError("Removing model from resource list:  %s\n", pFilename);

				dhash_Remove(pMgr->m_hModelHash, hElement);
			}
		}

		delete pRec;
	}
#else
	if (nArg3 == 0)
	{
		CModelCleanupRec* pRec = (CModelCleanupRec*)pLink->m_pData;
		CServerMgr*       pMgr;
		char*             pFilename;
		void*             hElement;

		pMgr = g_pServerMgr;

		if (g_pServerMgr)
		{
			pFilename = pRec->m_pModel->GetFilename();

			delete pRec;

			hElement = dhash_FindElement(pMgr->m_hModelHash, pFilename,
			                             strlen(pFilename));

			if (hElement)
			{
				// AVP2: a report at level 3 (lithtech 0x0047ebff; the
				// two-space string is in every AvP2 image).
				if (g_DebugLevel >= 3)
					dsi_ServerError("Removing model from resource list:  %s\n", pFilename);

				dhash_Remove(pMgr->m_hModelHash, hElement);
			}
		}
	}
#endif

	return LT_OK;
}

// 0x10052df8 and 0x10054020 — the two dependency records, as the shipped
// .data initialises them. Each is two words in the image, the callback and
// the parent record (0x10052dfc is zero; 0x10054028 is already the next
// global, the per-type handler table), so CDepRecord's last two words are
// ours and stay zero. The model-hash record's parent is the unlink record
// (name of the first ours).
CDepRecord g_DepUnlinkRecord =
{
	(LTRESULT (*)(void*, CDepLink*, int))dl_UnlinkAndFree, LTNULL
};

CDepRecord g_ModelHashCleanup =
{
	se_ModelHashCleanup, &g_DepUnlinkRecord
};

// 0x1003df40 — stdlith's CHelpers::ExtractNames (src/lithshared/stdlith/
// helpers.cpp), linked in statically: full path in, then path, file name,
// title and extension out, each optional. NOT the CRT's _splitpath, whose
// second argument is the drive — the shipped pushes put the buffer in the
// second slot and zeros in the last three, and the callee's first strcpy
// (from its 0x108 path buffer into argument two) is what says the second
// slot is the path. Declared here rather than by including stdlith's
// helpers.h, as sharedcommon.cpp does for CHelpers::UpperStrcmp.
class CHelpers
{
public:
	static int ExtractNames(const char* pFullPath, char* pPathname,
	                        char* pFilename, char* pFiletitle, char* pExt);
};

// What closed the gap from the old 14-block near, each from the shipped
// bytes:
//
//  * `new Model(...)`, not dalloc plus placement new: both shipped calls go
//    to the `operator new` forwarder 0x1000d25d (0x1002b0d2, 0x1002b19a),
//    and with the new-expression VC6 lets the null result share esi with
//    pModel's LTNULL — which fixed the whole three-way register rotation.
//  * the directory comes from CHelpers::ExtractNames, not _splitpath (see
//    the declaration below): the buffer is the SECOND argument.
//  * the cleanup record is `new CModelCleanupRec(mgr, model)` with an
//    inline constructor — the manager load before the model store at
//    0x1002b1a3 is the constructor's argument evaluation.
//  * guards, not nested ifs. lithtech.exe keeps every error block in line
//    where its guard is, and says so: the missing-file block straight after
//    the se_LoadFile test (0x0046a86b) and again after the stream test
//    (0x0046a933), the SetFilename and se_RegisterModel failures sharing one
//    delete-and-LT_ERROR copy (0x0046a9d8), and the dResult failure its own
//    copy at the very end (0x0046aa41) — which is what makes that last one
//    the `if (!dResult) {...}` body's fall-out rather than a guard.
//
// IDENTICAL on both images now (lithtech 0x0047ec30, server 0x10036839). The
// last residual was where VC6 hosts the missing-file tail it merges: shipped
// keeps it at the FIRST site (the se_LoadFile test) and has the stream test
// jump back. Guarded early returns host it at the last site, and so did a
// backward `goto`; one if/else chain in which every arm, inner tests
// included, ends in its own return hosts it at the first.
//
// One child model, named by its parent's file. The name in the file is bare,
// so it is resolved against the parent's own directory — which is why a model
// and its children have to live together.
//
// From there it is the same as loading any model, and it recurses: a child
// with children of its own comes back through here. What is different is the
// end, where the model is given a callback that takes it out of the manager's
// hash again when it goes, so that nothing is left pointing at it.
LTRESULT se_LoadChildModel(CModelLoadRequest* pRequest, Model** ppModel)
{
	char   szDir[0x200];
	char   szPath[0x200];
	void*  pFile;
	CModelLoadContext* pCtx;
	Model* pModel = LTNULL;
	void*  hElement;
	LTRESULT dResult;
	CModelLoadRequestQ507* pReq = (CModelLoadRequestQ507*)pRequest;

	*ppModel = LTNULL;

	pCtx = pReq->m_pContext;

	CHelpers::ExtractNames(pCtx->m_pFilename, szDir, LTNULL, LTNULL, LTNULL);

	if (szDir[0] == 0)
		sprintf(szPath, "%s", pReq->m_pChildName);
	else
		sprintf(szPath, "%s\\%s", szDir, pReq->m_pChildName);

	hElement = dhash_FindElement(pCtx->m_pServerMgr->m_hModelHash, szPath,
	                             strlen(szPath));

	if (hElement)
	{
		*ppModel = (Model*)dhash_GetData(hElement);

		return LT_OK;
	}
	else if (!se_LoadFile(&pCtx->m_pServerMgr->m_FileMgr, szPath, 0, &pFile))
	{
		RETURN_ERROR_SYM_DESC(1, "se_LoadChildModel", LT_MISSINGMODELFILE, szPath);
	}
	else
	{
		pModel = new Model((uint32*)&g_DefAlloc, (uint32*)&g_DefAlloc);

		if (!pModel)
		{
			RETURN_ERROR(1, "se_LoadChildModel", LT_OUTOFMEMORY);
		}
		else
		{
			pReq->m_pStream = se_OpenStream(&pCtx->m_pServerMgr->m_FileMgr, pFile);

			if (!pReq->m_pStream)
			{
				RETURN_ERROR_SYM_DESC(1, "se_LoadChildModel", LT_MISSINGMODELFILE, szPath);
			}
			else
			{
				dResult = pModel->Load(pReq->m_pStream, (IMoAllocator*)&g_DefAlloc);

				if (!dResult)
				{
					// AVP2: the report se_LoadModelData makes, for the child (0x0047ee49).
					if (g_DebugLevel >= 3)
						dsi_ServerError("Calling Model::Load():  %s\n", pReq->m_pChildName);

					dResult = pModel->LoadChildModels(pRequest);
				}

				pReq->m_pStream->Release();

				if (!dResult)
				{
					if (!pModel->SetFilename(szPath))
					{
						delete pModel;

						RETURN_ERROR(1, "se_LoadChildModel", LT_ERROR);
					}

					pModel->m_FileFlags = ((CFileRef*)pFile)->m_nFileID;

					if (se_RegisterModel(pCtx->m_pServerMgr, pModel, pFile, dResult))
					{
						delete pModel;

						RETURN_ERROR(1, "se_LoadChildModel", LT_ERROR);
					}

					// AVP2: the cleanup record is se_RegisterModel's now, for every
					// model. NOLF's was made here, at the model's +0x14: commonmodel.h
					// (not ours) names the run from +0x08 to +0x13 m_Cleanups and the
					// word at +0x14 m_pad_014, so that address is computed rather than
					// taken from a field whose name would be misleading.
					*ppModel = pModel;

					return LT_OK;
				}

				delete pModel;

				RETURN_ERROR(1, "se_LoadChildModel", LT_ERROR);
			}
		}
	}

}

// THE SHARED SERVER SOURCE HAS A CLIENT-ONLY STATEMENT, and se_RegisterModel
// below is where it is. lithtech.exe's copy of this function (0x0046ad10) ends
//
//     push %esi ; call 0x00414930 ; add $0x4,%esp
//
// immediately before its `xor %eax,%eax ; test %ebp,%ebp ; sete %al`, with
// %esi holding pModel. server.dll's copy (0x1002b41b, which our source matches
// byte for byte) has nothing there. 0x00414930 is client code —
// src/engine/lithtech/csvis.cpp already reconstructs it as
// cs_InheritFileFromServer, named for the one string it raises, "Client
// inherited model %s from server!", and that file's own note lists 0x0046add3
// as one of its two call sites. So the shipped serverengine.cpp really does
// call into the client when a client is present, under some preprocessor
// guard, and server.dll is simply the build where that guard is false.
//
// THE MACRO'S REAL NAME IS NOT KNOWN. build/engine.sh now defines
// LT_SERVER_BUILD for the server target only, purely as a named
// discriminator for cases like this one -- do NOT overload
// LT_SERVER_TIMERS, which means something narrower (the scoped timers,
// which AvP2's lithtech.exe has too, so build.mk defines it for both
// images). The original symbol
// build/engine.sh defines for server.dll and not for lithtech.exe (worldsim.cpp
// and four other files already turn on server-only code with it), so its
// negation is used here as the stand-in for "a client is in this process".
// If a properly named discriminator is ever added to build/engine.sh, this is
// the site to move onto it.
//
// The symbol below cannot be linked against csvis.cpp's (the mangling differs
// and the argument class is spelled there, not here), the byte comparison masks every
// relocated byte and the whole link joins the two through link/aliases.rsp,
// so the declaration only has to give the right calling
// convention and argument count.
#ifndef LT_SERVER_BUILD
void cs_InheritFileFromServer(Model* pModel);      // 0x00414930 (client only)
#endif

// 0x1002b41b — a model that has finished loading joins the manager's hash
// under its own filename, and every object that was waiting for it is
// switched over.
//
// The wait works like this: se_InitModel points an object whose model is
// still loading at the manager's placeholder and marks the object's tracker
// with bit 3, and marks the file record with bit 0. So the flag on the file
// is what says "somebody is waiting", and the objects to fix are the ones
// whose engine-side data names this same file. Each one gives the placeholder
// its reference back, takes one on the real model, and loses the marker bit;
// then the file loses its own.
//
// The filename is asked of the model three times rather than kept in a local,
// which is what the three GetFilename calls in the shipped code are.
//
// What comes back is whether the hash insert *failed* — nonzero on failure —
// which is why se_LoadChildModel treats a zero as success.
//
// This was written up as an irreducible callee-saved rotation (ebx and edi
// swapped, 191 of 191) and it was neither. Two source facts were missing and
// they were one fault: the bit-0 clear had to go through the byte-wide view
// declared at the top of this file, and the list head had to be a named local
// assigned *before* the first link is read. With the flag clear one byte too
// long the register pressure was different, so the head-first spelling — which
// had been probed and rejected on its own — did nothing; with the flag fixed
// it puts the manager in edi and the head in a fresh ebx exactly as shipped.
// A rotation is a symptom, and probing one lever at a time can hide the
// second.
//
// NEAR on Primal Hunt (316 of 312, lithtech.exe 0x0047f2a0) since the
// cleanup record moved in: the bit-0 clear goes through %al and the manager
// is reloaded before it, where shipped keeps `andb $0xfe,0xc(%ebp)` and
// reloads after. Through the uint16 field it is `andw` (314); `&= 0xfe`, a
// test through the uint16 field, the record built in a named local, and
// /GX all leave it.
LTRESULT se_RegisterModel(CServerMgr* pMgr, Model* pModel, void* pFile,
                          int nFlag)
{
	void*   hElement;
	LTLink* pHead;
	LTLink* pCur;
	ModelInstance* pInst;

	if (g_DebugLevel >= 2)
		dsi_ServerError("(Server) loaded model: %s", pModel->GetFilename());

	// AVP2: a second report one level up (0x0047f2c7).
	if (g_DebugLevel >= 3)
		dsi_ServerError("Adding model to resource list: %s\n", pModel->GetFilename());

	hElement = dhash_AddElement((CHashTable*)pMgr->m_hModelHash,
	                            pModel->GetFilename(),
	                            strlen(pModel->GetFilename()));

	dhash_SetData(hElement, pModel);

	if (nFlag)
	{
		if (((CFileRefFlagView*)pFile)->m_Flags & 1)
		{
			pHead = pMgr->ObjectListHead();

			for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
			{
				pInst = (ModelInstance*)pCur->m_pData;

				if (pInst->m_pSData->m_pModelFile == pFile)
				{
					// AVP2: not below zero, unsigned (0x0047f35d), as
					// se_TermModel.
					if (pInst->m_AnimTracker.GetModel()->m_CacheCount > 0)
						pInst->m_AnimTracker.GetModel()->m_CacheCount--;

					pInst->m_AnimTracker.SetModel(pModel);

					pModel->m_CacheCount++;

					pInst->m_AnimTracker.m_Flags &= ~8;
				}
			}

			((CFileRefFlagView*)pFile)->m_Flags &= ~1;
		}
	}

	// AVP2: every registered model gets the record that takes it back out of
	// the hash when it goes -- NOLF made it in se_LoadChildModel, for child
	// models only (0x0047f397..0x0047f3be; AvP2 1.0.9.6 likewise).
	//
	// The list head is the model's +0x14 (`lea 0x14(%esi)`, 0x0047f3b9); see
	// se_LoadChildModel's note on commonmodel.h's names for that word.
	dl_AddLink((uint8*)pModel + 0x14,
	           dl_MakeLink(&g_ModelHashCleanup,
	                       new CModelCleanupRec(pMgr, pModel)));

	// Unconditional — 0x0046add2 is the merge point both `je`s above reach.
#ifndef LT_SERVER_BUILD
	cs_InheritFileFromServer(pModel);
#endif

	return hElement == LTNULL;
}

// 0x1002bde9 — walk the manager's mounted trees looking for a named file,
// answering nonzero when one has it. Two out-parameters: the record of the
// tree it was found in, and one dword the tree's own lookup reported about
// it (0x10006fb4 fills a 0x110-byte record and this is the dword at its
// +0x108). Both are optional. Its first parameter is the file manager,
// whose own first field is the tree list, so the callee reads it as a list
// head (name ours; the function lives in the file manager's run).
//
// Defined in smgrmisc.cpp under that file's views of the two records
// (0x1002bde9 is its placed address), so it is declared with them here and
// the call below casts to them.
class CFileMgrQ468;
class CFileTreeRecQ468;
int fs_FindFileInTreesQ468(CFileMgrQ468* pFileMgr, char* pFilename,
                           CFileTreeRecQ468** ppTreeRec, uint32* pUnknown);

// 0x1000e650 — tell one client's queue about a file. It allocates a record
// out of the object at its first argument's +0x18, fills in the three values
// and the flag word, and links it into one of two lists at +0x3c depending on
// bit 0 of the flags. servermessage.cpp's xfer_AddFile, on the client's
// file-transfer state (servermgr.h's CClientFiles, Client+0x18c).
int xfer_AddFile(CClientFiles* pState, char* pName, uint32 nUnknown,
                 uint32 nFileID, uint16 nFlags);

// BYTE-IDENTICAL on lithtech.exe (0x0046b9d0, 303 bytes) and server.dll
// (0x1002be8c, 246 bytes).
//
// The first length is a NAMED LOCAL, `nLen = strlen(pFilename)`, with the +1
// in the call. lithtech.exe proves it: its inlined first strlen ends
// `not %ecx; ... dec %ecx; inc %ecx` — the -1 and the +1 left unfolded,
// which a single `strlen(pFilename) + 1` expression folds away (as the
// second site, 0x0046ba6e, does). On server.dll the same local is what
// moves `mov 0x8(%ebp),%edi` below the strlen call (0x1002bea6) and frees
// the frame slot that the old 245-byte version recycled a parameter slot
// for — the `push %ecx` and `leave` came back with it.
//
// nFlags is a uint16 PARAMETER, not an int narrowed at the store. On
// server.dll the loop's Client pointer is spilled to a dead parameter slot,
// and with an int nFlags VC6 picked ppFile's 0x14(%ebp); the shipped build
// uses nFlags's 0x10(%ebp) (0x1002bf50, 0x1002bf64), which only the
// sixteen-bit parameter gives. (Tried first and NOT it: pClient declared
// first or block-scoped, no pClient local, a for loop, the call folded,
// a uint32 nFlags, the cast dropped.) Callers push a dword 0 either way.
//
// 0x1002be8c — the file manager's "give me this file" entry point, and the
// only thing that ever builds a CFileRef.
//
// Three answers, and callers here only test for nonzero: 1 if the file was
// already known (the record comes straight out of the name hash), 2 if it was
// found in a mounted tree and a record has just been made for it, and 0 if no
// tree has it. The out-parameter is optional and is cleared first either way,
// so a caller that ignores the result still sees null on failure.
//
// The name is measured twice — once for the lookup (into a local) and once
// for the insert — and both lengths include the terminator, which is what
// the two `inc %eax` after the strlen calls are.
//
// Every client is then told, because a client has to be able to name the same
// file by the same id the server used; the id is the manager's file counter at
// the moment the record was made.
int se_LoadFile(CFileMgr* pFileMgr, char* pFilename, uint16 nFlags, void** ppFile)
{
	CFileRef*     pRef;
#if defined(PRIMAL_HUNT) || !defined(LT_SERVER_BUILD)
	uint32        nLen;
#endif
	CFileTreeRec* pTreeRec;
	uint32        nUnknown;
	void*         hElement;
	LTLink*       pHead;
	LTLink*       pCur;
	Client*       pClient;

	if (ppFile)
		*ppFile = LTNULL;

#if !defined(PRIMAL_HUNT) && defined(LT_SERVER_BUILD)
	// AVP2: AvP2 1.0.9.6's server.dll (cl 9044, 0x10034c82) measures the
	// name inside the call; the named length costs it a frame slot (247
	// bytes). NEAR, size-exact: the manager's load comes before the strlen
	// call where shipped has it after, and the two out-values take each
	// other's dead parameter slots (allocation; declaration order unmoved).
	hElement = dhash_FindElement(pFileMgr->m_hFileHash, pFilename,
	                             strlen(pFilename) + 1);
#else
	nLen = strlen(pFilename);

	hElement = dhash_FindElement(pFileMgr->m_hFileHash, pFilename, nLen + 1);
#endif

	if (hElement)
	{
		pRef = (CFileRef*)dhash_GetData(hElement);

		if (ppFile)
			*ppFile = pRef;

		return 1;
	}

	if (!fs_FindFileInTreesQ468((CFileMgrQ468*)pFileMgr, pFilename,
	                            (CFileTreeRecQ468**)&pTreeRec, &nUnknown))
		return 0;

	pRef = (CFileRef*)sb_Allocate(&pFileMgr->m_Bank.m_Bank);

	hElement = dhash_AddElement((CHashTable*)pFileMgr->m_hFileHash, pFilename,
	                            strlen(pFilename) + 1);

	pRef->m_Flags     = (uint16)nFlags;
	pRef->m_pTreeRec  = pTreeRec;
	pRef->m_Unknown04 = nUnknown;
	pRef->m_nFileID   = pFileMgr->m_nFiles;

	pFileMgr->m_nFiles++;

	dhash_SetData(hElement, pRef);

	pRef->m_hElement = hElement;

	if (ppFile)
		*ppFile = pRef;

	pHead = &pFileMgr->m_pServerMgr->m_ClientList.m_Head;
	pCur  = pHead->m_pNext;

	while (pCur != pHead)
	{
		char* pName;

		pClient = (Client*)pCur->m_pData;

		pName = dhash_GetKey(pRef->m_hElement, LTNULL);

		xfer_AddFile((CClientFiles*)pClient->m_pUnknown18C, pName,
		             pRef->m_Unknown04, pRef->m_nFileID, pRef->m_Flags);

		pCur = pCur->m_pNext;
	}

	return 2;
}

// Copy a file out of the resource trees onto disk. The trees are tried in
// list order and the first one that will open the file wins — there is no
// second chance, so a tree that has the name but cannot open it ends the
// search with an error rather than falling through to the next.
//
// The stream's own copy answers 1 for a complete copy, and this inverts it
// into an LTRESULT: LT_OK when everything arrived, LT_ERROR otherwise. The
// stream is released either way, before the answer is worked out, which is
// why the result needs a local at all.
//
// The tree list is the manager's own first field, so no `lea` appears for the
// head — the parameter register doubles as it.
//
// The tree record is a NAMED LOCAL (pTreeRec). lithtech.exe (0x0046b8a3)
// then reads the record and its tree through the same register
// (`mov 0x8(%edi),%eax; mov 0xc(%eax),%eax`) and loads Release's vtable
// before copying the copy's result aside (0x0046b8d7); the nested cast gave
// ecx/edx for both, a 95-of-95-byte near. server.dll is identical either way.
LTRESULT fs_CopyFile(CFileMgr* pFileMgr, const char* pszSourceFile,
                     const char* pszDestFile)
{
	LTLink* pHead = &pFileMgr->m_Trees;
	LTLink* pCur  = pHead->m_pNext;

	while (pCur != pHead)
	{
		CFileTreeRec* pTreeRec = (CFileTreeRec*)pCur->m_pData;
		ILTStream* pStream =
			fs_OpenStreamInTree(pTreeRec->m_pTree,
			                    (char*)pszSourceFile, 0);

		if (pStream)
		{
			int bCopied = fs_CopyStreamToFile(pStream, pszDestFile);

			pStream->Release();

			return bCopied == 0;
		}

		pCur = pCur->m_pNext;
	}

	return LT_NOTFOUND;
}

// The other half of se_RegisterModel: a model going out of the hash, by name.
// Everything still pointing at it is switched to the manager's placeholder and
// marked as waiting again, and if anything was, the file record is marked too
// — which is what makes a later load of the same file fix the objects up.
//
// The lookup measures the name without its terminator, exactly as
// se_RegisterModel's insert does (no `inc %eax` after the strlen in either),
// so the two agree and the name is found.
//
// `delete pModel` is the compiler's own shape: the null test, then slot zero
// of the vtable called with a one — VC6's scalar deleting destructor — and no
// test written in the source.
//
// IDENTICAL on lithtech.exe (0x0047f540) and server.dll. In NOLF's server
// the count of switched objects lived in the dead pFilename parameter slot
// (`andl $0x0,0xc(%ebp)` and `incl 0xc(%ebp)`); with the manager, the list
// head, the cursor, the object, its tracker's model address and the model
// itself all live at once there is no register left for it.
LTRESULT sm_UnloadModel(CServerMgr* pMgr, char* pFilename, CFileIdentifier* pFile)
{
	void*   hElement;
	Model*  pModel;
	LTLink* pHead;
	LTLink* pCur;
	int     nSwitched;

	hElement = dhash_FindElement(pMgr->m_hModelHash, pFilename,
	                             strlen(pFilename));

	if (!hElement)
		return LT_NOTFOUND;

	pModel = (Model*)dhash_GetData(hElement);

	// AVP2: a report at level 3, and no dhash_Remove -- the cleanup record
	// se_RegisterModel hangs on every model takes it out of the hash as it
	// goes (lithtech 0x0047f571..0x0047f5a7; AvP2 1.0.9.6 and both
	// server.dlls carry the string).
	if (g_DebugLevel >= 3)
		dsi_ServerError("Removing model from resource list: %s\n", pModel->GetFilename());

	delete pModel;

	nSwitched = 0;

	pHead = pMgr->ObjectListHead();

	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		ModelInstance* pInst = (ModelInstance*)pCur->m_pData;

		if (pInst->m_AnimTracker.GetModel() == pModel)
		{
			pInst->m_AnimTracker.SetModel(pMgr->m_pPlaceholderModel);

			pInst->m_AnimTracker.m_Flags |= 8;

			nSwitched++;
		}
	}

	if (nSwitched)
		((CFileRef*)pFile)->m_Flags |= 1;

	return LT_OK;
}


// 0x10054028 — the per-object-type hooks (servermgr.h's ObjTypeHandlers,
// indexed by LTObject::m_ObjectType), eleven pairs of { create, remove }.
// smgr.cpp's w_ApplyObjectFilenames (0x1002ba90) reads the first word of an
// entry (`mov 0x10054028(,%eax,8)`) and sm_RemoveFromWorldTree (0x1002bab8)
// the second (0x1005402c); both follow se_InitContainer in .text, and every
// hook in the table is one of this file's se_* functions, so the table is
// this file's. The words are the shipped .data: model 0x1002b6a4/0x1002b870,
// world model 0x1002b92e, sprite 0x1002b88b, container 0x1002ba7b, every
// other type (normal, light, camera, particles, poly grid, lines, canvas)
// nothing.
ObjTypeHandlers g_ObjTypeHandlers[11] =
{
	{ LTNULL,            LTNULL       },     // OT_NORMAL
	{ se_InitModel,      se_TermModel },     // OT_MODEL
	{ se_InitWorldModel, LTNULL       },     // OT_WORLDMODEL
	{ se_InitSprite,     LTNULL       },     // OT_SPRITE
	{ LTNULL,            LTNULL       },     // OT_LIGHT
	{ LTNULL,            LTNULL       },     // OT_CAMERA
	{ LTNULL,            LTNULL       },     // OT_PARTICLESYSTEM
	{ LTNULL,            LTNULL       },     // OT_POLYGRID
	{ LTNULL,            LTNULL       },     // OT_LINESYSTEM
	{ se_InitContainer,  LTNULL       },     // OT_CONTAINER
	{ LTNULL,            LTNULL       },     // OT_CANVAS
};
