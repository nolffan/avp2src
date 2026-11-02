// *********************************************************************** //
//
//  MODULE:   CLIENTTEXTURES (.CPP)  -- delivered as lt3/csbase.cpp
//
//  READ THIS FIRST: THIS FILE IS *NOT* csbase.
//
//  The task that produced it named the range 0x0041f911..0x0042552f after
//  the ILTCSBase strings it pushes. Those strings are real, but every
//  function that pushes them is ALREADY MATCHED in lithtech.exe out of
//  src/engine/server/csbase.cpp and src/engine/server/servermath.cpp:
//
//      0x0041ec90 .. 0x0041fbd0   ILTCSBase's forty-four own methods
//                                 (csbase.obj) folded with CLTServer's
//                                 message accessors (servermsg.obj) and
//                                 CLTServerMath's rotation maths
//                                 (servermath.obj)
//
//  Nothing in that block is new. The enumeration's 0x0041f911 row is not even
//  a function: it is the `jne` target inside ILTCSBase::SetupEuler, whose
//  real entry is 0x0041f8d0. See the tables at the foot of this file.
//
//  What IS untouched in the range is the client's own code that begins where
//  csbase.obj ends. The block 0x0041fbe0..0x004209b0 is the CClientMgr TU
//  that src/engine/lithtech/clientmgr2.cpp calls "clientmgr.cpp" and does
//  not yet cover: the SharedTexture cache and the local-object slot table.
//  Two things pin it, and neither is a guess:
//
//    * src/engine/lithtech/lttexmod.cpp already reconstructed SharedTexture
//      (0x34 bytes, m_pEngineData at +0x08, m_nFrameCode at +0x30, and a
//      15-bit reference count plus one spare bit in the uint16 at +0x32) and
//      already declares 0x0041ff90 as "resolve one CClientFileRef to a
//      SharedTexture". 0x0041ff50, which 0x0041ff90 forwards to, calls
//      0x0041fe60 -- and 0x0041fe60 is the head of this block. Everything
//      below marks, sweeps or frees the objects on the list 0x0041fe60
//      builds.
//
//    * The spare bit at +0x32 is the sweep's mark. 0x00420060 sets it
//      (`orb $0x80,0x33`), 0x004200c0 clears it over the whole list
//      (`andw $0x7fff,0x32`), 0x00420420 frees everything without it
//      (`testb $0x80,0x33`), and 0x004200f0 skips textures whose *count* is
//      zero with the same `testw $0x7fff / jbe` that lttexmod.cpp's
//      tm_IsValidTextureHandle uses. The two halves of that uint16 read the
//      same way from four independent call sites.
//
//  src/engine/lithtech/clientsoundinst.cpp already declares 0x004204f0 as
//  `tbl_ReleaseSlot` against the eight-byte records at CClientMgr+0x11dc.
//  That is a declaration and not a definition -- nothing in the repo defines
//  it -- so the slot-table half below keeps that name and that record.
//
//  THE VIEW CLASSES ARE LOCAL ON PURPOSE. clientmgr.h's CClientMgr stops
//  modelling at 0x1538 and puts 0x1110-0x11e0 inside two pads;
//  clientmgr2.cpp's CClientMgrView carries the same LTList at 0x1110 that
//  this file needs. When these functions are moved into the tree, reconcile
//  CClientMgrTexBankView against CClientMgrView (they agree at 0x1110) and
//  SharedTextureView against lttexmod.cpp's SharedTexture (they agree at
//  +0x08, +0x30 and +0x32; this file additionally names +0x00, +0x0c, +0x10
//  and +0x24, which lttexmod.cpp left inside pads).
//
//  WHAT IS DELIBERATELY NOT HERE is listed at the foot of the file, with the
//  reason for each. Nothing below was guessed: every offset, every constant
//  and every branch is read off the retail lithtech.exe.
//
// *********************************************************************** //

#include <string.h>
#include <new.h>                     // placement new

#include "ltbasedefs.h"
#include "ltlink.h"


// ======================================================================= //
//  Layout
// ======================================================================= //

// The engine's shared texture, as much of it as this file reaches.
//
// lttexmod.cpp has the same class from the other end and the two agree
// exactly where they overlap; the fields it left inside pads are named here
// from the stores that reach them:
//
//   +0x00  while the object is on the bank's free list its first dword is
//          the intrusive next pointer -- 0x0041ffc0's last two stores are
//          `mov 0x1158(%edi),%eax; mov %eax,(%esi); mov %esi,0x1158(%edi)`,
//          which is a free-list push and nothing else.
//   +0x0c  a second pointer, zeroed beside m_pEngineData by 0x0041fe60 and
//          the only thing 0x004202f0 tests before telling the render DLL the
//          texture changed. Its name here is ours.
//   +0x10  the LTLink by which the texture hangs on the manager's list.
//          0x0041fe60 stores &m_TextureList.m_Head into +0x10 and the
//          texture itself into +0x18, which is m_pPrev / m_pData of a link
//          at +0x10; 0x0041ffc0 runs CheapLTLink::Remove over the same three
//          words in the header's own order.
//   +0x24  the file identifier that owns this texture. 0x0041fe60 stores the
//          identifier here and the texture into the identifier's own first
//          dword; 0x0041ffc0 clears that back pointer before freeing.
//
// The whole object is 0x34 bytes, and the shipped code says so twice: the
// `rep stos` of 0xd dwords in 0x0041fe60, and the bank's element size.
// The eight-byte sub-object at the texture's +0x00 (cmgrinit.cpp's
// CBankLinkQ46C, binkvideo.cpp's CBankLinkK): constructor 0x0045a350, Init
// 0x0045a370. Both out of class -- 0x0041fe60 CALLS both. Its first dword is
// also the struct bank's free-list link while the texture is free. The class
// is netmgr.cpp's CModelSub14 (the definitions of both at those addresses),
// seen through this file's field names.
class CModelSub14
{
public:
	CModelSub14();                           // 0x0045a350
	LTBOOL Init(void* pOwner);               // 0x0045a370

public:
	class SharedTextureView* m_pNextFree;    // 0x00  free-list link
	void*                    m_pOwner;       // 0x04
};

class SharedTextureView
{
public:
	// cmgrinit.cpp's SharedTextureQ46C() verbatim (0x0041feb0..0x0041fee0
	// expands it inside 0x0041fe60's inlined ObjectBank::Allocate).
	SharedTextureView()
	{
		m_pEngineData  = LTNULL;
		m_pRenderData  = LTNULL;
		m_Link.m_pData = this;
		memset(&m_Unknown1c, 0, sizeof(m_Unknown1c));
		m_pFileRef     = LTNULL;
		m_BankLink.Init(this);
		m_Unknown28    = 0;
		m_nRefCount    = 0;
		m_bMarked      = 0;
		m_nFrameCode   = 0;
	}

	// Declared, never defined here: 0x0041ffc0 calls it as a thiscall with
	// no arguments (`mov %esi,%ecx; call 0x0045a360`) immediately before
	// pushing the storage onto the free list, which is the destruct-then-
	// recycle half of an object bank's Free. The body lives outside this run
	// (0x0045a360 is a bare `jmp 0x0045a390`).
	~SharedTextureView();

	CModelSub14        m_BankLink;           // 0x00  free-list link at +0
	void*              m_pEngineData;        // 0x08  TextureData* (lttexmod)
	void*              m_pRenderData;        // 0x0c  name ours
	LTLink             m_Link;               // 0x10  prev / next / data
	uint32             m_Unknown1c[2];       // 0x1c, 0x20
	class CFileIdentifierView* m_pFileRef;   // 0x24
	uint32             m_Unknown28;          // 0x28
	uint32             m_Unknown2c;          // 0x2c
	uint16             m_nFrameCode;         // 0x30  (lttexmod.cpp)
	uint16             m_nRefCount : 15;     // 0x32  (lttexmod.cpp)
	// The sweep's mark. Set by tm_MarkTexture, cleared by
	// tm_UnmarkTextures, and read by tm_FreeUnmarkedTextures.
	uint16             m_bMarked    : 1;     // 0x32 bit 15
};

// The file identifier a texture was resolved from. Only its first dword is
// reached from here: it caches the SharedTexture, which is what makes
// 0x0041fe60's lookup a lookup (name ours).
class CFileIdentifierView
{
public:
	SharedTextureView* m_pTexture;           // 0x00
};

// One entry of the client manager's local-object slot table: a type byte and
// a pointer, eight bytes with the pointer at +0x04. The width is the shipped
// `,%eax,8` scaling in all four accessors; the type byte is the `movb` and
// the `cmpb $0x1`. Type 1 is the only kind tbl_GetObject will hand out
// (names ours; clientsoundinst.cpp already calls the table's slots slots).
class CClientSlot
{
public:
	uint8  m_nType;                          // 0x00
	void*  m_pData;                          // 0x04
};

// struct_bank.h's StructBank as cmgrinit.cpp reads it (0x1c bytes), with
// the free list typed for the one element this bank holds.
struct CTextureBankView;
void sb_GrowT(CTextureBankView* pBank, uint32 nCount);       // 0x00490c20

struct CTextureBankView
{
	uint32             m_nItemSize;          // 0x00
	uint32             m_nAlignedSize;       // 0x04
	uint32             m_nItemsPerBlock;     // 0x08  sb_Grow's count
	uint32             m_Unknown0c;          // 0x0c
	uint32             m_Unknown10;          // 0x10
	void*              m_pBlocks;            // 0x14
	SharedTextureView* m_pFreeList;          // 0x18

	// struct_bank.h's inline sb_Allocate, release build.
	SharedTextureView* sb_Allocate()
	{
		SharedTextureView* pRet;

		pRet = m_pFreeList;
		if (!pRet)
		{
			sb_GrowT(this, m_nItemsPerBlock);
			pRet = m_pFreeList;
			if (!pRet)
				return 0;
		}

		m_pFreeList = pRet->m_BankLink.m_pNextFree;
		return pRet;
	}

	// object_bank.h's inline ObjectBank::Allocate (NullCS).
	SharedTextureView* Allocate()
	{
		SharedTextureView* pRet;

		pRet = sb_Allocate();
		if (pRet)
		{
			::new(pRet) SharedTextureView;
		}

		return pRet;
	}
};

// CClientMgr, from the four fields this file uses. clientmgr2.cpp's
// CClientMgrView has the same LTList at 0x1110 (it calls it m_List1110) and
// keeps 0x1120-0x11d8 in a pad; the free head at 0x1158 and the slot table
// at 0x11dc/0x11e0 are named here.
//
// CClientMgr has no vtable -- clientmgr.h and clientmgr2.cpp both start
// their views at offset 0x00 with a plain pad -- so nothing below has to
// subtract a vptr.
class CClientMgrTexBankView
{
public:
	// The fields below 0x1110 are read only by tm_MarkUsedTextures
	// (0x004200f0); see there.
	uint8              m_pad_00[0x18c];
	class CWorldBspPairView** m_pWorldBsps;  // 0x18c
	uint32             m_nWorldBsps;         // 0x190
	uint8              m_pad_194[0x41c - 0x194];
	LTList             m_List41c;            // 0x41c  +0x1b8 of each: two textures
	uint8              m_pad_42c[0x46c - 0x42c];
	LTList             m_List46c;            // 0x46c  +0x1e8 of each: one texture
	uint8              m_pad_47c[0x10f0 - 0x47c];
	LTList             m_List10f0;           // 0x10f0 +0xc0 of each: one texture
	LTList             m_List1100;           // 0x1100 texture groups
	// Every live SharedTexture, as an LTList: count at 0x1110, head at
	// 0x1114 (so m_Head.m_pNext is 0x1118, which is what every walk here
	// loads first) and each element's link inside the texture at +0x10.
	LTList             m_TextureList;        // 0x1110
	uint8              m_pad_1120[0x1140 - 0x1120];
	// The object bank the textures come out of: its StructBank at 0x1140
	// (the ObjectBank's vptr is 0x113c -- cmgrinit.cpp). 0x0041fe60 grows it
	// with `0x00490c20(&bank, bank+0x08)` and re-reads the free head at
	// bank+0x18 = 0x1158 afterwards.
	CTextureBankView   m_TextureBank;        // 0x1140
	uint8              m_pad_115c[0x11dc - 0x115c];
	CClientSlot*       m_pSlots;             // 0x11dc
	uint32             m_nSlots;             // 0x11e0
	uint8              m_pad_11e4[0x150c - 0x11e4];
	void*              m_pConnection;        // 0x150c  (clientmgr.h's)
	uint8              m_pad_1510[0x1534 - 0x1510];
	// The client file manager (clientmgr.h's +0x1534), handed to
	// 0x00404680 by 0x0041ff50.
	class CClientFileMgrView* m_pFileMgr;    // 0x1534
};

// The 0x3c-byte record tm_MarkGroupTextures walks: a SharedTexture pointer
// array and its count. Nothing here reads any other field, so nothing else
// is named -- but the stride is proven (`add $0x3c,%edi`) and so is the
// pair's placement.
class CTextureSetView
{
public:
	uint8               m_pad_00[0x20];
	SharedTextureView** m_ppTextures;        // 0x20
	uint32              m_nTextures;         // 0x24
	uint8               m_pad_28[0x3c - 0x28];
};

// Its owner: the array at +0x0c and the count at +0x10.
class CTextureGroupView
{
public:
	uint8            m_pad_00[0x0c];
	CTextureSetView* m_pSets;                // 0x0c
	uint32           m_nSets;                // 0x10
};

// The 0x48-byte record tm_MarkSkinTextures walks -- one SharedTexture at
// +0x30. The same record and the same +0x78/+0x7c pair turn up again at
// 0x00423c30, which stamps 0xffff into its +0x40, so the stride is confirmed
// from two independent functions.
class CSkinView
{
public:
	uint8              m_pad_00[0x30];
	SharedTextureView* m_pTexture;           // 0x30
	uint8              m_pad_34[0x48 - 0x34];
};

class CSkinOwnerView
{
public:
	uint8      m_pad_00[0x78];
	CSkinView* m_pSkins;                     // 0x78
	uint32     m_nSkins;                     // 0x7c
};


// ======================================================================= //
//  What this file calls and does not define
// ======================================================================= //

// 0x00428fd0 / 0x00429080 -- the engine's allocator pair (clientmgr2.cpp
// declares the second one).
void* dalloc(uint32 size);
void  dfree(void* pMem);

// 0x00462900 -- give a SharedTexture's data back: it hands the texture to
// the render DLL's own release entry (through the pointer at 0x004b43d0,
// only while the renderer at 0x004b439c is up), then frees m_pEngineData
// through 0x00461dc0 unless it carries lttexmod.cpp's TDFLAG_TEXMOD, then
// clears the field.
//
// NOTE: src/engine/lithtech/clientmgr2.cpp declares
// this address as `void dfree(void* pMem); // 0x00462900`. That looks wrong.
// 0x00429080 is dfree; 0x00462900 is the texture-data release above, and
// 0x0041ffc0 calls it *before* destructing the object it was handed, which a
// free could not survive.
void tex_FreeTextureData(SharedTextureView* pTexture);

// 0x004628e0 -- tell the render DLL a texture's contents changed
// (lttexmod.cpp already declares it, two cdecl arguments).
void sub_4628e0(SharedTextureView* pTexture, int nUnknown);

// 0x00490200 -- a bare `ret` in the shipped image, so its argument is all
// that is left of it. /OPT:ICF has folded it with every other empty function
// in lithtech.exe, which is why it cannot be named from its body. Written
// as exactly that: an empty body (defined out of line, so /Ob1 leaves both
// calls as calls).
void sub_490200(CClientMgrTexBankView* pClientMgr)
{
}

// 0x004200f0 -- mark every texture something currently in use refers to
// (written below, after tm_MarkSkinTextures).
void tm_MarkUsedTextures(CClientMgrTexBankView* pClientMgr);


// ======================================================================= //
//  The texture cache -- find or create (0x0041fe60 / 0x0041ff50 / 0x0041ff90)
// ======================================================================= //

// 0x00404680 -- the file manager's lookup (clientconsole.cpp's
// fm_GetFileRec); cdecl, a byte flag last. 3 is the texture type.
class CClientFileRefView;
CFileIdentifierView* fm_GetFileRec(CClientFileMgrView* pMgr, CClientFileRefView* pRef,
                                   uint8 nFlag);             // 0x00404680

// 0x0041fe60 -- the texture a file identifier resolves to, created (bank
// allocate, zeroed, onto the head of the list, both back pointers) when the
// identifier does not cache one yet; either way a texture without a render
// copy is handed to the render DLL (the tm_ReloadTextures pair). The
// `rep stos` of 0xd dwords AFTER the inlined constructor is a memset of the
// whole object over the freshly constructed texture, and *ppTexture is
// stored before it and re-read after, so every use goes through the out
// pointer. (names ours)
LTRESULT cm_AddSharedTextureIdent(CClientMgrTexBankView* pClientMgr,
                                  CFileIdentifierView* pIdent,
                                  SharedTextureView*& pTexture)
{
	pTexture = LTNULL;

	if (pIdent->m_pTexture)
	{
		pTexture = pIdent->m_pTexture;
	}
	else
	{
		pTexture = pClientMgr->m_TextureBank.Allocate();
		memset(pTexture, 0, sizeof(SharedTextureView));
		dl_AddHead(&pClientMgr->m_TextureList, &pTexture->m_Link, pTexture);
		pTexture->m_pFileRef = pIdent;
		pIdent->m_pTexture = pTexture;
	}

	if (!pTexture->m_pRenderData)
	{
		sub_4628e0(pTexture, 0);
		sub_490200(pClientMgr);
	}

	return LT_OK;
}

// 0x0041ff50 -- the same from a file reference: LT_MISSINGFILE (0x16) when
// the file manager does not know it.
LTRESULT cm_AddSharedTexture2(CClientMgrTexBankView* pClientMgr,
                              CClientFileRefView* pRef,
                              SharedTextureView*& pTexture)
{
	CFileIdentifierView* pIdent;

	pTexture = LTNULL;

	// POSITIVE GUARD: shipped `je` jumps forward to the LT_MISSINGFILE tail.
	pIdent = fm_GetFileRec(pClientMgr->m_pFileMgr, pRef, 3);
	if (pIdent)
		return cm_AddSharedTextureIdent(pClientMgr, pIdent, pTexture);

	return LT_MISSINGFILE;
}

// 0x0041ff90 -- the texture or null. The out variable lives in the second
// argument's own slot (`lea 0x8(%esp)`), and `neg/sbb/not/and` is
// `r == LT_OK ? pRet : 0`.
SharedTextureView* cm_AddSharedTexture(CClientMgrTexBankView* pClientMgr,
                                       CClientFileRefView* pRef)
{
	SharedTextureView* pRet;

	if (cm_AddSharedTexture2(pClientMgr, pRef, pRet) == LT_OK)
		return pRet;
	else
		return LTNULL;
}


// ======================================================================= //
//  The texture cache -- mark, sweep, free
// ======================================================================= //

// 0x00420060 -- mark one texture as still wanted. Thirteen bytes: a null
// check and `orb $0x80,0x33(%eax)`, which is how VC6 sets a one-bit field at
// bit 15 of a uint16. Twelve call sites, every one of them inside this
// block's marking pass.
void tm_MarkTexture(SharedTextureView* pTexture)
{
	if (pTexture)
		pTexture->m_bMarked = 1;
}

// 0x004200c0 -- clear the mark on every live texture. The walk is the
// shipped one exactly: &m_Head hoisted into a register once, m_pNext loaded
// from 0x1118 before the loop, and `andw $0x7fff,0x32(%ecx)` for the store,
// which is the same one-bit field cleared instead of set.
void tm_UnmarkTextures(CClientMgrTexBankView* pClientMgr)
{
	LTLink* pCur;
	LTLink* pHead;

	// The HEAD ADDRESS is taken first: shipped emits `lea <head>` BEFORE
	// loading m_pNext, where the inline condition makes VC6 load the link
	// first and compute the sentinel after.
	pHead = &pClientMgr->m_TextureList.m_Head;

	for (pCur = pClientMgr->m_TextureList.m_Head.m_pNext;
	     pCur != pHead;
	     pCur = pCur->m_pNext)
	{
		((SharedTextureView*)pCur->m_pData)->m_bMarked = 0;
	}
}

// 0x0041ffc0 -- one texture off the list and back into its bank.
//
// The first eleven instructions are ltlink.h's own dl_RemoveAt with
// CheapLTLink::Remove inlined, in the header's exact order: the two
// neighbour fixups, then TieOff writing m_pNext before m_pPrev (the header's
// `m_pPrev = m_pNext = this` is a chained assignment), then the count.
//
// The tail is the bank's Free: release the texture's data, destruct the
// object in place, push the storage on the intrusive free list.
void tm_FreeTexture(CClientMgrTexBankView* pClientMgr, SharedTextureView* pTexture)
{
	dl_RemoveAt(&pClientMgr->m_TextureList, &pTexture->m_Link);

	if (pTexture->m_pFileRef)
		pTexture->m_pFileRef->m_pTexture = LTNULL;

	tex_FreeTextureData(pTexture);

	pTexture->~SharedTextureView();

	pTexture->m_BankLink.m_pNextFree = pClientMgr->m_TextureBank.m_pFreeList;
	pClientMgr->m_TextureBank.m_pFreeList = pTexture;
}

// 0x00420020 -- the whole cache away. The next link is taken before the
// element is freed, which is why this reads as a `while` with its own
// pNext and not as a `for`.
//
// The enumeration splits this one function into two rows, 0x00420020 (14) and
// 0x00420034 (29). 0x00420034 is the `je` target at the bottom of the loop,
// not a function: the only `ret` between 0x00420020 and the padding is at
// 0x00420050 (csv row now 0x00420020,49).
//
// The named pHead is what puts pClientMgr in ebx and the head in edi, as
// shipped: with `&pClientMgr->m_TextureList.m_Head` written at both uses the
// two registers swap (size-exact, 4 blocks).
void tm_FreeAllTextures(CClientMgrTexBankView* pClientMgr)
{
	LTLink* pCur;
	LTLink* pNext;
	LTLink* pHead = &pClientMgr->m_TextureList.m_Head;

	pCur = pHead->m_pNext;
	while (pCur != pHead)
	{
		pNext = pCur->m_pNext;

		tm_FreeTexture(pClientMgr, (SharedTextureView*)pCur->m_pData);

		pCur = pNext;
	}
}

// 0x00420420 -- the sweep half: everything the marking pass did not touch
// goes. Same walk as tm_FreeAllTextures with the mark test in front of the
// free; `testb $0x80,0x33(%eax)` is the one-bit field read as a condition.
void tm_FreeUnmarkedTextures(CClientMgrTexBankView* pClientMgr)
{
	LTLink* pCur;
	LTLink* pNext;

	pCur = pClientMgr->m_TextureList.m_Head.m_pNext;
	while (pCur != &pClientMgr->m_TextureList.m_Head)
	{
		pNext = pCur->m_pNext;

		if (!((SharedTextureView*)pCur->m_pData)->m_bMarked)
			tm_FreeTexture(pClientMgr, (SharedTextureView*)pCur->m_pData);

		pCur = pNext;
	}
}

// 0x004202c0 -- the whole collection, twice.
//
// Twice is not a guess: `mov $0x2,%edi` in front of a loop whose body is the
// three calls and whose bottom is `dec %edi; jne`, which is what VC6 emits
// for a two-iteration counted loop whose index the body never reads. The
// three pushes of the manager share one `add $0xc,%esp`, so all three calls
// are in one basic block.
//
// A second pass is meaningful because freeing a texture can drop the last
// reference to another one (tm_FreeTexture clears the file identifier's back
// pointer), so one sweep does not reach everything.
void tm_FreeUnusedTextures(CClientMgrTexBankView* pClientMgr)
{
	int i;

	for (i = 0; i < 2; i++)
	{
		tm_UnmarkTextures(pClientMgr);
		tm_MarkUsedTextures(pClientMgr);
		tm_FreeUnmarkedTextures(pClientMgr);
	}
}

// 0x004202f0 -- hand every texture that has no render-side copy back to the
// render DLL. The same two calls under the same test appear inlined at the
// tail of 0x0041fe60, so the pair is one statement in the original and this
// is not the only place it was written.
void tm_ReloadTextures(CClientMgrTexBankView* pClientMgr)
{
	LTLink* pCur;
	SharedTextureView* pTexture;

	for (pCur = pClientMgr->m_TextureList.m_Head.m_pNext;
	     pCur != &pClientMgr->m_TextureList.m_Head;
	     pCur = pCur->m_pNext)
	{
		pTexture = (SharedTextureView*)pCur->m_pData;

		if (!pTexture->m_pRenderData)
		{
			sub_4628e0(pTexture, 0);
			sub_490200(pClientMgr);
		}
	}
}

// 0x00420070 -- mark every texture in one group of texture sets. Two nested
// counted loops over unsigned counts (`test %eax,%eax; jbe` is `count == 0`
// on an unsigned, not `<= 0`), with both the array base and the bound
// re-loaded on every iteration because the call in the middle could have
// changed them.
//
// Called from 0x004200f0 over the manager's list at +0x1104.
void tm_MarkGroupTextures(CTextureGroupView* pGroup)
{
	uint32 i;
	uint32 j;

	for (i = 0; i < pGroup->m_nSets; i++)
	{
		for (j = 0; j < pGroup->m_pSets[i].m_nTextures; j++)
			tm_MarkTexture(pGroup->m_pSets[i].m_ppTextures[j]);
	}
}

// 0x00420280 -- mark the texture on every skin record of one owner. The
// shipped loop keeps a byte offset and adds 0x48 to it, which is VC6's
// strength reduction of `m_pSkins[i]`; the count is re-loaded each time.
//
// Called twice from 0x004200f0, on the +0x04 and +0x08 members of each entry
// of the manager's array at +0x18c.
void tm_MarkSkinTextures(CSkinOwnerView* pOwner)
{
	uint32 i;

	for (i = 0; i < pOwner->m_nSkins; i++)
		tm_MarkTexture(pOwner->m_pSkins[i].m_pTexture);
}


// 0x004200f0 -- the marking pass: every texture something still refers to.
// Eight sources, in the shipped order: textures with a reference count
// (`testw $0x7fff / jbe`), the +0xc0 texture of each entry on the list at
// 0x10f0, each texture's own +0x28 texture (the list at 0x1110 walked a
// second time -- the head stays in %ebx), both BSPs of every world entry in
// the array at 0x18c/0x190 (the second may be null), the two textures at
// +0x1b8 of each entry on the list at 0x41c (`mov $2; dec; jne`), every
// texture group on the list at 0x1100, the +0x1e8 texture on the list at
// 0x46c, and, while the renderer is up, the environment map and the two
// global pan textures (0x004b445c; 0x004b4460 stride 0x14).
//
// The world array is guarded by the connection AND by the manager itself
// (`test %ebp,%ebp` after it has already been dereferenced), written here as
// the two tests; the original was probably an inlined accessor handing back
// the manager or null.
//
// Register allocation is set by the head locals: the texture list's head is
// its own local (pTexHead, %ebx) live across the second walk, and every
// other walk reuses one pHead (%edi). With the heads written in each `for`
// the allocation swaps (10 blocks); with one shared local for all (6).
// The inner pair loop reads its owner once into pOwner, as shipped does
// (`lea 0x1b8(%eax),%esi` hoisted), and both counters are uint32 (`jb`).
// (view names ours)
class CWorldBspPairView
{
public:
	uint32          m_Unknown00;             // 0x00
	CSkinOwnerView* m_pOriginalBsp;          // 0x04
	CSkinOwnerView* m_pWorldBsp;             // 0x08  may be null
};

class CTexOwner1b8View
{
public:
	uint8              m_pad_00[0x1b8];
	SharedTextureView* m_pTextures[2];       // 0x1b8
};

class CTexOwner1e8View
{
public:
	uint8              m_pad_00[0x1e8];
	SharedTextureView* m_pTexture;           // 0x1e8
};

class CTexOwnerC0View
{
public:
	uint8              m_pad_00[0xc0];
	SharedTextureView* m_pTexture;           // 0xc0
};

// clientmgr.h's PanInfo with its first word typed: the texture.
class CPanInfoT
{
public:
	SharedTextureView* m_pTexture;           // 0x00
	uint8              m_pad_04[0x14 - 0x04];
};

// The render-up flag, the environment map and the pan info are fields of
// clientrender.cpp's g_RenderStruct (0x004b4358), not globals of their own.
class RenderStruct
{
public:
	uint8              m_pad_00[0x44];
	int                m_bInited;                        // 0x044  0x004b439c
	uint8              m_pad_48[0x104 - 0x48];
	SharedTextureView* m_pEnvMapTexture;                 // 0x104  0x004b445c
	CPanInfoT          m_GlobalPanInfo[2];               // 0x108  0x004b4460
};
extern RenderStruct g_RenderStruct;                         // 0x004b4358

void tm_MarkUsedTextures(CClientMgrTexBankView* pClientMgr)
{
	LTLink*            pCur;
	LTLink*            pTexHead;
	LTLink*            pHead;
	SharedTextureView* pTexture;
	CWorldBspPairView* pWorld;
	CTexOwner1b8View*  pOwner;
	uint32             i;
	uint32             j;

	pTexHead = &pClientMgr->m_TextureList.m_Head;

	for (pCur = pTexHead->m_pNext; pCur != pTexHead; pCur = pCur->m_pNext)
	{
		pTexture = (SharedTextureView*)pCur->m_pData;
		if (pTexture->m_nRefCount > 0)
			tm_MarkTexture(pTexture);
	}

	pHead = &pClientMgr->m_List10f0.m_Head;

	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		tm_MarkTexture(((CTexOwnerC0View*)pCur->m_pData)->m_pTexture);
	}

	for (pCur = pTexHead->m_pNext; pCur != pTexHead; pCur = pCur->m_pNext)
	{
		tm_MarkTexture((SharedTextureView*)((SharedTextureView*)pCur->m_pData)->m_Unknown28);
	}

	if (pClientMgr->m_pConnection && pClientMgr)
	{
		for (i = 0; i < pClientMgr->m_nWorldBsps; i++)
		{
			pWorld = pClientMgr->m_pWorldBsps[i];

			tm_MarkSkinTextures(pWorld->m_pOriginalBsp);
			if (pWorld->m_pWorldBsp)
				tm_MarkSkinTextures(pWorld->m_pWorldBsp);
		}
	}

	pHead = &pClientMgr->m_List41c.m_Head;

	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		pOwner = (CTexOwner1b8View*)pCur->m_pData;

		for (j = 0; j < 2; j++)
			tm_MarkTexture(pOwner->m_pTextures[j]);
	}

	pHead = &pClientMgr->m_List1100.m_Head;

	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		tm_MarkGroupTextures((CTextureGroupView*)pCur->m_pData);
	}

	pHead = &pClientMgr->m_List46c.m_Head;

	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		tm_MarkTexture(((CTexOwner1e8View*)pCur->m_pData)->m_pTexture);
	}

	if (g_RenderStruct.m_bInited)
	{
		tm_MarkTexture(g_RenderStruct.m_pEnvMapTexture);

		for (j = 0; j < 2; j++)
			tm_MarkTexture(g_RenderStruct.m_GlobalPanInfo[j].m_pTexture);
	}
}


// ======================================================================= //
//  The local-object slot table -- CClientMgr+0x11dc / +0x11e0
// ======================================================================= //

// 0x00420460 -- make room for one id.
//
// Every accessor below opens with `mov 0x8(%esp),%eax; and $0xffff,%eax`,
// which is how VC6 reads a sixteen-bit stack parameter it is about to use as
// an int; clientsoundinst.cpp already read the same prologue the same way.
// The compare is `ja`, unsigned, so the count is unsigned too.
//
// The growth is `nID + 100`, a fresh zeroed block, the old contents copied
// over and the old block freed. memset and memcpy are the /O2 intrinsics
// with a variable count -- `rep movsl` on the dword part, `rep movsb` on the
// remainder -- so <string.h> has to be included for them to expand.
//
// One frame fact worth keeping: the new count is spilled to 0x14(%esp),
// which is arg1's OWN HOME SLOT. arg1 is live in %ebx from the second
// instruction on, so VC6 reuses its incoming slot for the local rather than
// growing the frame -- the same lever as at 0x0041ff90, one
// step over: here it is an ordinary local and not an inlined callee's
// temporary.
void tbl_EnsureSize(CClientMgrTexBankView* pClientMgr, uint16 nID)
{
	uint32       nNewSlots;
	CClientSlot* pNewSlots;

	if (pClientMgr->m_nSlots > nID)
		return;

	nNewSlots = nID + 100;

	pNewSlots = (CClientSlot*)dalloc(nNewSlots * sizeof(CClientSlot));
	memset(pNewSlots, 0, nNewSlots * sizeof(CClientSlot));
	memcpy(pNewSlots, pClientMgr->m_pSlots,
	       pClientMgr->m_nSlots * sizeof(CClientSlot));
	dfree(pClientMgr->m_pSlots);

	pClientMgr->m_pSlots = pNewSlots;
	pClientMgr->m_nSlots = nNewSlots;
}

// 0x004204f0 -- give one slot back. clientsoundinst.cpp already declares
// this address under this name and describes exactly these two stores; it is
// defined here for the first time. The table pointer is re-loaded for the
// second store, so the two clears are two statements.
void tbl_ReleaseSlot(CClientMgrTexBankView* pClientMgr, uint16 nID)
{
	if (nID >= pClientMgr->m_nSlots)
		return;

	pClientMgr->m_pSlots[nID].m_nType = 0;
	pClientMgr->m_pSlots[nID].m_pData = LTNULL;
}

// 0x00420520 -- the object in one slot, or null. Two guarded returns of the
// same value, so VC6 hosts the shared `xor %eax,%eax; ret` after the
// successful return rather than in front of it.
void* tbl_GetObject(CClientMgrTexBankView* pClientMgr, uint16 nID)
{
	CClientSlot* pSlot;

	// NESTED POSITIVE GUARDS, one trailing return -- see tbl_GetSlot. Both
	// shipped guards are forward jumps onto one shared `xor %eax,%eax`.
	if (nID < pClientMgr->m_nSlots)
	{
		pSlot = &pClientMgr->m_pSlots[nID];

		if (pSlot->m_nType == 1)
			return pSlot->m_pData;
	}

	return LTNULL;
}

// 0x00420550 -- the slot itself, whatever is in it. The same bounds check
// and the same `lea (%ecx,%eax,8),%eax`, without the type test.
CClientSlot* tbl_GetSlot(CClientMgrTexBankView* pClientMgr, uint16 nID)
{
	// POSITIVE GUARD with ONE trailing return. Written as an early
	// `if (bad) return LTNULL;` VC6 emits `jb` into the body and duplicates
	// the null return inline; the shipped bytes are a single `jae` onto a
	// shared failure tail.
	if (nID < pClientMgr->m_nSlots)
		return &pClientMgr->m_pSlots[nID];

	return LTNULL;
}


// ======================================================================= //
//  0x00420580 lives in cchandler.cpp (sub_420580) -- its real home is here,
//  between tbl_GetSlot and the dims warning below. It is identical where it
//  is and was left there.
// ======================================================================= //

// ======================================================================= //
//  0x00420610 -- the "suspiciously large internal dims" warning
// ======================================================================= //

// 0x004476b0 -- `mov 0x4(%ecx),%eax; ret`, reached as an out-of-line
// thiscall, so it is a non-inline accessor defined in another TU. For a
// model it is the filename the format string wants (name ours).
class CDimsModelView
{
public:
	char* GetFilename();                     // 0x004476b0
};

// The object, as far as the warning reads it: the type byte at +0xbc -- a
// SIGNED char, the second arm widens it with `movsbl` for the %d -- and
// CClientModel::m_pModel at +0x1dc (clientmgr.h).
class CDimsObjectView
{
public:
	uint8           m_pad_000[0xbc];
	char            m_Type;                  // 0x0bc
	uint8           m_pad_0bd[0x1dc - 0xbd];
	CDimsModelView* m_pModel;                // 0x1dc
};

extern int g_DebugLevel;                                   // 0x004b0ab0
void       con_ConsolePrint(char* pFormat, ...);           // 0x0042e6f0

// 0x00420610 -- cdecl(object, radius). Its one caller is the position
// setter at 0x004207a0, which passes the object's own +0x1a4 after it tests
// it against a limit (`fcomps 0x4a1ee4; test $0x41,%ah`). Nothing prints
// below debug level 1 (`cmpl $0x1; jl` onto the final `ret`); a model with
// a model pointer is named by file, anything else by its type (name ours).
void obj_WarnLargeDims(CDimsObjectView* pObj, float fDims)
{
	if (g_DebugLevel >= 1)
	{
		if (pObj->m_Type == OT_MODEL && pObj->m_pModel)
		{
			con_ConsolePrint("Model %s has suspiciously large internal dims (%f).",
			                 pObj->m_pModel->GetFilename(), fDims);
		}
		else
		{
			con_ConsolePrint("An object (type %d) has suspiciously large internal dims (%f).",
			                 pObj->m_Type, fDims);
		}
	}
}


// ======================================================================= //
//  0x004207a0 / 0x00420880 -- set a local object's position / rotation
// ======================================================================= //

// The world tree at CClientMgr+0x6c (clientmgr2.cpp, cchandler.cpp):
// 0x0048cde0 is worldtree's AddObject, a thiscall of two.
class CObjWorldTreeView
{
public:
	void AddObject(void* pObj, uint32 nListIndex);           // 0x0048cde0
};

// The object as these two setters reach it (clientmgr.h / predobj.cpp
// offsets): rotation at +0x9c, type and blocking priority bytes at
// +0xbc/+0xbd, position at +0x18c, radius at +0x1a4.
class CObjSetView
{
public:
	// 0x0045aaa0 -- LTObject::SetPos, the vector BY VALUE (predobj.cpp).
	void SetPos(LTVector vPos);                              // 0x0045aaa0

	uint8      m_pad_000[0x9c];
	LTRotation m_Rotation;                                   // 0x09c
	uint8      m_pad_0ac[0xbc - 0xac];
	char       m_ObjectType;                                 // 0x0bc
	uint8      m_BlockingPriority;                           // 0x0bd
	uint8      m_pad_0be[0x18c - 0xbe];
	LTVector   m_Pos;                                        // 0x18c
	LTVector   m_Dims;                                       // 0x198
	float      m_Radius;                                     // 0x1a4
};

// CClientMgr: the world tree at +0x6c and the 0x508 pointer
// sm_SetObjectRotation's server twin passes as the move state's +0x04.
class CClientMgrObjView
{
public:
	uint8             m_pad_000[0x6c];
	CObjWorldTreeView m_WorldTree;                           // 0x06c
	uint8             m_pad_070[0x508 - 0x70];
	void*             m_pUnknown508;                         // 0x508
};

// servermgr.h's WMMoveState, 0x68 bytes (cltapi.cpp / cchandler.cpp).
struct WMMoveStateT
{
	void*  m_pContainer;                     // 0x00
	void*  m_pUnknown04;                     // 0x04
	void*  m_pObj;                           // 0x08
	uint32 m_nUnknown0C;                     // 0x0c
	uint32 m_pPushList;                      // 0x10
	uint32 m_nPushCount;                     // 0x14
	uint8  m_pad_18[0x64 - 0x18];
	uint32 m_nUnknown64;                     // 0x64
};

// 0x00450c30 -- the client's obj_RetransformWorldModel (commonphysics.cpp).
void obj_RetransformWorldModelT(CObjSetView* pObj);         // 0x00450c30
// 0x00454530 -- the client's wm_RotateWorldModel (wmrotate.cpp).
void wm_RotateWorldModelT(WMMoveStateT* pState, LTRotation* pRotation,
                          int bUnknown);                     // 0x00454530

// 0x004207a0 -- cdecl(manager, object, position, force). Unforced moves
// shorter than sqrt(0.001) are dropped (`fcomps 0x4a1a54`, a float 0.001);
// a radius over 3000 (`0x4a1ee4`) gets the dims warning; with a manager the
// world model is retransformed (the inlined type-2/9 test) and the object
// re-seated in the world tree. (name ours)
void cm_SetObjectPos(CClientMgrObjView* pClientMgr, CObjSetView* pObj,
                     LTVector* pPos, int bForce)
{
	// The difference written out, not ltvector.h's DistSqr: DistSqr's
	// (*this - vec) reorders the three squares (zz+xx+yy against shipped's
	// zz+yy+xx).
	if (!bForce)
	{
		if ((pObj->m_Pos - *pPos).MagSqr() < 0.001f)
			return;
	}

	pObj->SetPos(*pPos);

	// The radius is RE-READ for the call (shipped `flds 0x1a4` twice, no
	// spill); every plain spelling keeps the compared value and spills it
	// with `fsts`. The volatile read at the one site is the barrier
	// (as in w_PointInPoly) -- a stand-in, not a claim about the type.
	if (pObj->m_Radius > 3000.0f)
		obj_WarnLargeDims((CDimsObjectView*)pObj, *(volatile float*)&pObj->m_Radius);

	if (pClientMgr)
	{
		char nType = pObj->m_ObjectType;

		if (nType == OT_WORLDMODEL || nType == OT_CONTAINER)
			obj_RetransformWorldModelT(pObj);

		pClientMgr->m_WorldTree.AddObject(pObj, 0);
	}
}

// 0x00420670 -- cdecl(manager, object), 28 bytes: re-seat an object at its
// own position, forced (`push $1; lea 0x18c(%eax)` onto 0x004207a0). Its
// home is between the dims warning and 0x00420690; it sits after
// cm_SetObjectPos here only so the views above are in scope (definition
// order does not change its bytes -- nothing is inline). clientmgr.h,
// cltnew.cpp and cchandler.cpp declare it as sub_420670. (name ours)
void cm_ResetObjectPos(CClientMgrObjView* pClientMgr, CObjSetView* pObj)
{
	cm_SetObjectPos(pClientMgr, pObj, &pObj->m_Pos, 1);
}

// 0x00420880 -- cdecl(manager, object, rotation): smgr.cpp's
// sm_SetObjectRotation shape on the client: the state's four clears, the
// rotation (its last dword store lands after the clears), then a world model is turned through the simulation (flag 0), and
// the object is re-seated in the world tree. (name ours)
void cm_SetObjectRotation(CClientMgrObjView* pClientMgr, CObjSetView* pObj,
                          LTRotation* pRotation)
{
	WMMoveStateT state;

	state.m_pContainer = LTNULL;
	state.m_pUnknown04 = LTNULL;
	state.m_pObj = LTNULL;
	state.m_nUnknown64 = 0;

	pObj->m_Rotation = *pRotation;

	if (pClientMgr)
	{
		char nType = pObj->m_ObjectType;

		if (nType == OT_WORLDMODEL || nType == OT_CONTAINER)
		{
			state.m_pContainer = &pClientMgr->m_WorldTree;
			state.m_pUnknown04 = pClientMgr->m_pUnknown508;
			state.m_pObj       = pObj;
			state.m_nUnknown0C = pObj->m_BlockingPriority;
			state.m_pPushList  = 0;
			state.m_nPushCount = 0;
			wm_RotateWorldModelT(&state, pRotation, 0);
		}

		pClientMgr->m_WorldTree.AddObject(pObj, 0);
	}
}


// ======================================================================= //
//  0x004209b0 -- five fields of one record through slot 0 of another object
// ======================================================================= //

// The object: slot 0 of its vtable is a thiscall taking five dwords, the
// first a mode (names ours).
class CModeTarget9b0View
{
public:
	virtual void SetMode(uint32 nMode, uint32 a, uint32 b, uint32 c, uint32 d);
};

// The record: a format word at +0x0c whose value 16 picks mode 2 and
// anything else mode 3, and four dwords behind it (names ours).
class CModeRecord9b0View
{
public:
	uint8  m_pad_00[0x0c];
	uint32 m_nFormat;                        // 0x0c
	uint32 m_n10;                            // 0x10
	uint32 m_n14;                            // 0x14
	uint32 m_n18;                            // 0x18
	uint32 m_n1c;                            // 0x1c
};

// 0x004209b0 -- cdecl(record, object). Callers 0x0048ae53 and 0x0048b5f0,
// in the smack-video unit; neither reads %eax afterwards. The mode comes out
// as the branchless `setne; add $2` either way, but only the if/else into a
// named local gets the shipped registers: the ternary, inline or into the
// local, and `2 + (fmt != 16)` all load m_nFormat late and burn %ebx on the
// setne (46 bytes against 48).
//
// The record's offsets are DDPIXELFORMAT's -- dwRGBBitCount at +0x0c, then
// the R, G, B and alpha masks at +0x10..+0x1c -- so this is very likely a
// DirectDraw pixel format converted into the engine's own (16 bits -> 2,
// else 3), alpha mask first. Kept as views: nothing here proves the target
// class.
void sub_4209b0(CModeRecord9b0View* pRec, CModeTarget9b0View* pTarget)
{
	uint32 nMode;

	if (pRec->m_nFormat == 16)
		nMode = 2;
	else
		nMode = 3;

	pTarget->SetMode(nMode, pRec->m_n1c, pRec->m_n10, pRec->m_n14, pRec->m_n18);
}


// ======================================================================= //
//  0x00421f60 -- A FREE IDENTIFICATION, AND NOT A FUNCTION TO WRITE
// ======================================================================= //
//
//  0x00421f60 (92 bytes) is `MatVMul`, src/LT2/sdk/inc/ltmatrix.h line 206,
//  emitted out of line. It is the same situation servercommon.cpp records
//  for MatVMul_H at 0x1000f878: an SDK header inline whose COMDAT the
//  shipped build kept.
//
//  The proof is in the argument registers and the term order together. The
//  prologue is `mov 0x4(%esp),%edx; mov 0x8(%esp),%eax; mov 0xc(%esp),%ecx`
//  -- pDest, pMat, pSrc, the header's order -- and each of the three rows is
//
//      flds  m[i][2] ; fmuls src.z
//      flds  m[i][1] ; fmuls src.y ; faddp
//      flds  m[i][0] ; fmuls src.x ; faddp
//      fadds m[i][3]
//      fstps dest[i]
//
//  which is VC6's reverse-order x87 evaluation of exactly
//  `m[i][0]*x + m[i][1]*y + m[i][2]*z + m[i][3]`. There is no homogeneous
//  divide and no return value, so it is MatVMul and not MatVMul_H.
//
//  And the arithmetic closes it. The SAME header's MatVMul_3x3 (line 267) is
//  ALREADY MATCHED in lithtech.exe at 0x00431650, 83 bytes, out of
//  servermath.obj and objbank.obj. MatVMul is MatVMul_3x3 plus one
//  `+ m[i][3]` per row, which is three `fadds disp8` at three bytes each:
//  83 + 9 = 92, which is the size of 0x00421f60 to the byte.
//
//  The one thing needed to claim it is a CALLER that VC6 will not inline.
//  src/engine/server/ltobject.cpp already calls MatVMul twice and VC6
//  inlines it there, so that TU emits no COMDAT; whichever TU owns
//  0x00421f60's call site is the one that has to reference it.
//
//  DO NOT WRITE A BODY FOR IT. Referencing `MatVMul` from a caller in the
//  right TU is what makes VC6 emit the header's own COMDAT; a redefinition
//  would be a second symbol.
//
//
// ======================================================================= //
//  ALREADY SOURCED IN THE REPO -- do not rewrite any of these
// ======================================================================= //
//
//  Every address below is in the enumeration and every one of them already
//  has source.
//
//  csbase.cpp / servermath.cpp -- matched, nothing to do
//    0x0041f911   NOT A FUNCTION. It is the `jne 0x0041f911` target inside
//                 ILTCSBase::SetupEuler, whose entry is 0x0041f8d0 and which
//                 matches out of src/engine/server/csbase.cpp. The whole run
//                 0x0041ec90..0x0041fbd0 is matched, csbase.obj folded with
//                 servermsg.obj and servermath.obj:
//                   0x0041f930 InterpolateRotation        (108)
//                   0x0041f9a0 SetupTransformationMatrix  ( 95)
//                   0x0041fa00 SetupTranslationMatrix     ( 83)
//                   0x0041fa60 SetupRotationMatrix        ( 83)
//                   0x0041fac0 SetupTranslationFromMatrix ( 83)
//                   0x0041fb20 SetupRotationFromMatrix    ( 83)
//                   0x0041fb80 SetupRotationAroundPoint   ( 95)
//                 Their `ret N` agrees with iltcsbase.h's declaration order
//                 slot for slot, which is how the vtable was named.
//
//  src/engine/server/fsstream.cpp
//    0x00420a40   the atexit half of g_TreeStreamBank's initializer (the
//                 constructor half, 0x004209f0, matches as fsstream's _$E1).
//    0x00420ad0   the same for g_MemStreamBank (constructor 0x00420a80).
//    0x00420b00   CTreeStream::Release  (server 0x10006e3f)
//    0x00420b30   CMemStream::Release   (server 0x10006e4b)
//                 Both are 12 bytes in server.dll and 47 here because the
//                 client's /O2 inlined CStreamBank::Free into them -- lock,
//                 in-place destruct through slot 0 with a zero flag, free
//                 list, unlock. The source is proven right by server.dll;
//                 the difference is the compile.
//    0x00420ec0   fs_OpenStreamInTree ("stream %p open file %s size = %u"
//                 and "stream %p open rez %s size = %u" are both inside it)
//    0x00421200   CMemStream::Read
//    0x004218c0   CStreamBank::Allocate
//    0x00421940   CMemStreamBank::Allocate
//    0x00421ac0   CStreamFields::~CStreamFields ("close stream %p num reads
//                 = %u bytes read = %u" at 0x00421adc)
//
//  src/engine/server/filemgr.cpp
//    0x00420d50, 0x004212d0 (ar_FindNext -- "%s\%s\*.*" at 0x00421315),
//    0x00421670, 0x00421750. The four unmatched filemgr symbols left are
//    fs_OpenFileEx, fs_AddResources, ar_FindFile and fs_BuildFileList; the
//    matched neighbours 0x00420b70 fs_OpenTree, 0x00420cd0 fs_CloseTree,
//    0x00420d30 and 0x00420e80 fix the TU.
//
//  src/engine/server/commonmodel.cpp
//    0x004224d0   CServerWorldFull::Load ("World load: %d ticks,
//                 precalculate: %d ticks" at 0x0042291d)
//    0x00423e10   the world-light property reader ("Light", "DirLight",
//                 "ObjectLight", "LightRadius", "LightColor", "InnerColor",
//                 "OuterColor", "Pos", "LightObjects", "FastLightObjects",
//                 "Rotation", "FOV" all fall inside it; commonmodel.cpp
//                 already carries the FastLightObjects arm)
//    0x00421be0   a 138-byte wrapper round the matched w_ReadWorldHeader at
//                 0x00421b20 that answers 0x2a on failure
//    0x00421c70, 0x00421d10, 0x00421fc0, 0x004221f0, 0x00422240,
//    0x004222f0, 0x00422970  -- all inside commonmodel.obj's run, whose
//                 unmatched list still holds w_ReadWorldLights (1698),
//                 LoadChildModels (1952), CollectFlaggedPolies,
//                 CServerWorldFull's constructor and CServerWorldTeardown's
//                 destructor. Nears of proven source, not new work.
//    0x0042552f-side: 0x00425096 is NOT a function -- it is inside the
//                 matched CWorldModelHost::AttachWorldModel at 0x00424f20
//                 (439 bytes, so it ends at 0x004250d7).
//    0x004222a0   the 70-byte `gap` row: a thiscall that tests bits 1 and 2
//                 of its first byte and frees +0x04 / +0x08 through
//                 0x00426300 then 0x004290c0. Same TU.
//
//  src/engine/server/worldmodels.cpp
//    0x00425420, 0x004254e0  -- between the matched AttachWorldModel,
//    CWorldData::LoadObjects (0x004250e0, and "LightAnim_BASE" at
//    0x004253cf is inside it) and CWorldModelList::FindPoly (0x00425530).
//
//
// ======================================================================= //
//  DELIBERATELY SKIPPED -- and why
// ======================================================================= //
//
//  These are in the client's own TU, so they are real work; they are not
//  written because writing them would have meant inventing something.
//
//    (0x0041fe60, the cache's find-or-create, was listed here; it is now
//      written above with ObjectBank::Allocate expanded through a local
//      bank view, together with its callers 0x0041ff50 and 0x0041ff90.)
//
//    (0x004200f0, the marking pass, was listed here; it is now written
//      above, after tm_MarkSkinTextures, and matches.)
//
//    * 0x00420330 (238) -- the two-entry texture pair at object+0x1b8 being
//      reconciled across the manager's list at +0x420. Same objection.
//
//    (0x00420610, the dims warning, and 0x004209b0, the pixel-format
//      hand-off, were listed here; both are now written above.)
//
//    * 0x00423c30 (95), 0x00423cc0 (182), 0x004244d0 (153), 0x00424590
//      (818), 0x004248d0 (746), 0x00424bc0 (487), 0x00424dd0 (275) -- the
//      clientworld.cpp / worldmodels.cpp seam. clientworld.obj has only
//      four small unmatched symbols left, so most of these are new, but each
//      needs a class not yet established.
//
//    * 0x00424242 (640) -- inside the un-named C++ subsystem lttexmod.cpp
//      already documents and already refuses: 0x004243ce constructs a
//      0xa0-byte derived class on vtable 0x004a1fd4 ("pEB"), whose base is
//      0x0048c6e0. lttexmod.cpp leaves it unidentified rather than
//      guess, and so does this file.
//
// *********************************************************************** //
