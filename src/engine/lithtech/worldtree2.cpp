// *********************************************************************** //
//
//  MODULE:   N48C540 (.CPP)  -- staging translation unit (file name ours)
//
//  WHAT THIS IS: the bottom half of the CLIENT's WORLD TREE -- the quadtree
//  NODE class, the WORLD-TREE OBJECT base class that gets linked into it,
//  and the box query and the insert walk.  src/engine/lithtech/worldtree.cpp
//  has the TOP half (0x0048d210..0x0048db00, the segment/box query entry
//  points on the tree itself); every call it makes downward lands in here.
//
//  ADDRESS RANGE: 0x0048c540 .. 0x0048cfe1, seventeen functions, 2358 bytes.
//  The enumerated range was 0x0048c730 + 1803 bytes; the three rows BELOW
//  it (0x0048c540, 0x0048c6e0, 0x0048c720) are unclaimed and are the same
//  two classes, so they are written here too.
//
//  GROUND TRUTH: the retail lithtech.exe, cl 8447, /O2 (so /Oy: frameless,
//  %esp-relative).  Every offset, constant and branch below was read off that
//  image and is cited by address.  No build of ours was read.
//
//  NOT A PREBUILT LIBRARY.  Relocation-free windows from five of these
//  functions -- 0x0048c7a0 (0x4c bytes), 0x0048c730 (0x4a), 0x0048cca0
//  (0x31), 0x0048c970 (0x11) and 0x0048c830 (0x30) -- were searched for
//  byte-for-byte over the WHOLE of the retail server.dll.  None of the
//  five appears anywhere in it, at the rezmgr delta 0x0FBAC620 or at any
//  other offset.  server.dll has no world tree: its nearest equivalent,
//  sm_RemoveFromWorldTree (src/engine/server/smgr.cpp), dispatches through
//  g_ObjTypeHandlers instead.  ==> engine code, compiled once.
//
//  ALREADY RECONSTRUCTED?  No, except for two addresses that are NOT in
//  this file: 0x0048cce0 (the tree's constructor) and 0x0048cd50 (its Term)
//  are src/engine/lithtech/lt2new4.cpp's CUnknown48CCE0.  Nothing else in
//  src/engine/ or src/lithshared/ names any address in 0x0048c540..
//  0x0048cfe1 as reconstructed source; the only other hits are commentary
//  (worldtree.cpp, lttexmod.cpp, ltnew1.cpp, ltnew2.cpp, clienttexmgr.cpp).
//  See the "WHAT worldtree.cpp GETS WRONG" note at the foot of this file.
//
// *********************************************************************** //
//
//  HOW THE TWO CLASSES WERE IDENTIFIED, FROM DATA
//  ----------------------------------------------
//
//  1. TWO PARALLEL VTABLES, COLUMN BY COLUMN.  0x0048c730 is written as a
//     function pointer in .rdata exactly twice, and both sites are slot 2
//     of an eight-slot table; the two tables are otherwise identical except
//     at slot 3:
//
//         slot   0x004a38d8 (base)   0x004a1fd4 (derived)
//         ----   -----------------   --------------------
//           0    0x00424570          0x00424570   scalar deleting dtor
//           1    0x0042d1b0          0x0042d1b0   `xor eax,eax; ret $4`
//           2    0x0048c730          0x0048c730   *** this file ***
//           3    0x004928e7          0x004244d0   _purecall  vs  a real body
//           4    0x00432040          0x00432040   `xor eax,eax; ret`
//           5    0x0042d1b0          0x0042d1b0   `xor eax,eax; ret $4`
//           6    0x00459f70          0x00459f70   `ret $4`
//           7    0x00459f70          0x00459f70   `ret $4`
//
//     0x004928e7 is `push $0x19; call 0x00494dc9`, i.e. _amsg_exit
//     (_RT_PUREVIRT) -- so 0x004a38d8 belongs to an ABSTRACT class with one
//     pure virtual at slot 3, and 0x004a1fd4 to a concrete class derived
//     from it that overrides only that slot.  Slot 3 is the one called at
//     0x0048c5ca (`call *0xc(%edx)`) with two pointer arguments, and
//     0x004244d0 fills two LTVectors out of member floats at +0x68..+0x74:
//     slot 3 is GetDims(LTVector*, LTVector*).
//
//     0x00424570 is `call 0x0048c720; testb $1,0x8(%esp); je ...; push this;
//     call 0x004290c0` -- the textbook VC6 scalar deleting destructor.  That
//     PINS 0x0048c720 as the class's real destructor and makes the virtual
//     dtor slot 0, so slot 2's function is the third thing declared.
//
//     Slots 6 and 7 share 0x00459f70 and slots 1 and 5 share 0x0042d1b0.
//     There is no /Gy in this build, so only IN-CLASS function
//     definitions fold: those four are `{}` / `{return 0;}` bodies written
//     inside the class declaration.
//
//  2. THE NODE, read straight off the bytes.  0x0048c7a0 clears the whole
//     object and gives every field's offset and type in one go:
//
//         +0x00  CheapLTLink m_Objects[3]  3x TieOff, `add $0x8` (0x0048c7b0)
//         +0x18  LTVector    m_vBoxMin     0x18/0x1c/0x20 (0x0048c7d3..)
//         +0x24  LTVector    m_vBoxMax     0x24/0x28/0x2c
//         +0x30  LTVector    m_vCenter     0x30/0x34/0x38 (0x0048c7ca..)
//         +0x3c  float       m_fSmallestDim
//         +0x40  float       m_fRadius
//         +0x44  uint32      m_nNumObjects 0x0048c7a6, ++ at 0x0048ccc6,
//                                          -- at 0x0048c761
//         +0x48  node*       m_pParent     0x0048c7a3, read at 0x0048cca ..
//                                          and written at 0x0048ca8a
//         +0x4c  node*       m_Children[4] 0x4c/0x50/0x54/0x58, and the
//                                          `cmp $4` loops at 0x0048c976
//                                          and 0x0048ca87
//                                                              size 0x5c
//
//     0x5c is confirmed independently: `push $0x5c; call 0x004290b0` at
//     0x0048ca60 is `new WorldTreeNode`.
//
//  3. THE OBJECT, likewise, off 0x0048c6e0:
//
//         +0x00  vptr                       0x004a38d8 (0x0048c6e7)
//         +0x04  WTLink m_Links[5]          5 x 0x10 (0x0048c6f0..0x0048c703)
//         +0x54  uint32 m_nObjType          the ctor's one argument
//         +0x58  uint32 m_nTouchCode        0xffffffff (0x0048c709)
//
//     A WTLink is an LTLink (0x0c bytes: m_pPrev, m_pNext, m_pData) plus a
//     back-pointer to the node at +0x0c: the ctor stores this into +0x08 and
//     LTNULL into +0x0c (0x0048c6f5/0x0048c6f8), 0x0048ccbc stores the node
//     into +0x0c, and 0x0048c75e/0x0048c768 chase it up the parent chain.
//     Five is confirmed at 0x0048cf9a, `cmp $0x5,%eax; jae` -- the bound
//     check before `lea 0x4(%eax,%edx,1)` with %eax already shifted left 4.
//
//  4. WHAT THE THREE OBJECT LISTS ARE.  m_Objects is indexed two ways.
//     0x0048c592 and 0x0048cca6 index it with a RUNTIME value supplied by
//     the caller (`lea (%ecx,%eax,8)`), and the two call sites in the engine
//     pass 0 and 2 (0x0045b011, 0x0045b03b).  m_Objects[2] is ALSO reached
//     with the index baked in, at 0x0048c561 (`lea 0x10(%ebp),%ebx`),
//     0x0048ceed and 0x0048cfca -- so list 2 is special, and the box query
//     short-circuits on it.  Names below say WT_NUMLISTS = 3 and leave the
//     meaning of list 2 unnamed; nothing in this range says what it is.
//
//  NAMING: everything here carries the suffix N48C so it cannot collide with
//  src/engine/lithtech/worldtree.cpp (suffix N48D) or with lt2new4.cpp.
//
// *********************************************************************** //

#include <math.h>                // sqrt -- an intrinsic under /O2 (/Oi)

#include "ltbasedefs.h"          // uint32, uint8, LTBOOL, LTMIN, LTNULL
#include "ltvector.h"            // LTVector = _CVector<float>
#include "ltlink.h"              // CheapLTLink / LTLink
#include "iltstream.h"           // ILTStream: Read is vtable slot 2


// ======================================================================= //
//  THE TYPES
// ======================================================================= //

#define WT_NUMLISTS   3          // 0x0048c7b0's `mov $0x3,%esi`
#define WT_NUMLINKS   5          // 0x0048c6e2's `mov $0x5,%ecx`, and the
                                 // `cmp $0x5,%eax; jae` at 0x0048cf9a
#define WT_NUMKIDS    4          // 0x0048c976, 0x0048ca87
#define WT_MAXPATH    4          // `cmp $0x4,%esi; jae` at 0x0048cdb6


class CWTNodeN48C;
class CWorldTreeN48C;
struct WTFindInfoN48C;
struct WTInsertInfoN48C;


// The link an object is threaded into a node with: ltlink.h's LTLink plus a
// back-pointer to the node that owns it.  +0x0c is stored at 0x0048ccbc and
// read at 0x0048c752 / 0x0048c75b / 0x0048c765.
class CWTLinkN48C : public LTLink
{
public:
	CWTNodeN48C* m_pNode;                    // +0x0c
};


// ----------------------------------------------------------------------- //
//  The world-tree object base -- vtable 0x004a38d8, eight slots.
//
//  Slots 1, 4, 5, 6 and 7 are defined IN-CLASS, which is why 1 folds with 5
//  (0x0042d1b0) and 6 with 7 (0x00459f70): no /Gy in this build, so nothing
//  else can fold.  Their argument counts come from the `ret $N` of the
//  shared bodies and from the call sites:
//
//    slot 1  `ret $0x4`   called 0x0048cebd: one pointer (the tree)
//    slot 3  pure         called 0x0048c5ca and 0x0048cdf9: two LTVector*
//    slot 4  `ret`        called 0x0048cfb8: no arguments
//    slot 5  `ret $0x4`   called 0x0048cfc7: one uint32, returns a CWTLink*
//                         (0x0048cfcd immediately links the result into
//                         m_Objects[2])
//    slot 6  `ret $0x4`   called 0x0048cefc: one object pointer
//    slot 7  `ret $0x4`   called 0x0048c581: one WTFindInfo pointer
//
//  Slot 2 is the only one this file defines.
// ----------------------------------------------------------------------- //
class CWTObjN48C
{
public:
	CWTObjN48C(uint32 nObjType);                              // 0x0048c6e0
	virtual ~CWTObjN48C();                                    // 0x0048c720

	virtual LTBOOL       WantsTree(CWorldTreeN48C *pTree)  { return LTFALSE; }
	virtual void         RemoveFromLists();                   // 0x0048c730
	virtual void         GetDims(LTVector *pMin, LTVector *pMax) = 0;
	virtual LTBOOL       WantsExtraLink()                  { return LTFALSE; }
	virtual CWTLinkN48C* MakeExtraLink(uint32 nIndex)      { return LTNULL; }
	virtual void         NotifyInserted(CWTObjN48C *pObj)  {}
	virtual void         OnFoundInBox(WTFindInfoN48C *pInfo) {}

public:
	CWTLinkN48C m_Links[WT_NUMLINKS];        // +0x04
	uint32      m_nObjType;                  // +0x54
	uint32      m_nTouchCode;                // +0x58
};


// ----------------------------------------------------------------------- //
//  The quadtree node.  Split is on X and Z only: every descent in this file
//  compares .x against m_vCenter.x and .z against m_vCenter.z and never
//  touches .y, and m_fSmallestDim (below) is a min over X and Z alone.
//
//  Child order, read off the four recursive calls at 0x0048c665 / 0x0048c683
//  / 0x0048c6ae / 0x0048c6ca and confirmed by the four SetDims calls in
//  AllocChildren:
//
//        m_Children[0]   -X  -Z        m_Children[2]   +X  -Z
//        m_Children[1]   -X  +Z        m_Children[3]   +X  +Z
// ----------------------------------------------------------------------- //
class CWTNodeN48C
{
public:
	CWTNodeN48C();                                            // 0x0048c780
	~CWTNodeN48C();                                           // 0x0048c790

	void   Init();                                            // 0x0048c7a0
	void   Term();                                            // 0x0048c930
	void   FreeChildren();                                    // 0x0048c970

	void   GetDims(LTVector *pMin, LTVector *pMax);           // 0x0048c7f0
	void   SetDims(LTVector vMin, LTVector vMax);             // 0x0048c830

	LTBOOL AllocChildren();                                   // 0x0048ca50
	LTBOOL LoadNodes(ILTStream *pStream,
	                 uint8 *pCurByte, uint8 *pCurBit);        // 0x0048c9b0

	void   AddObjectLink(CWTLinkN48C *pLink, uint32 nList);   // 0x0048cca0

public:
	CheapLTLink  m_Objects[WT_NUMLISTS];     // +0x00
	LTVector     m_vBoxMin;                  // +0x18
	LTVector     m_vBoxMax;                  // +0x24
	LTVector     m_vCenter;                  // +0x30
	float        m_fSmallestDim;             // +0x3c
	float        m_fRadius;                  // +0x40
	uint32       m_nNumObjects;              // +0x44
	CWTNodeN48C* m_pParent;                  // +0x48
	CWTNodeN48C* m_Children[WT_NUMKIDS];     // +0x4c
};                                           // 0x5c


// The descriptor the box query carries.  Offsets are loads in 0x0048c540;
// the layout agrees field for field with the one worldtree.cpp derived
// independently from 0x0048d810 (its CWTBoxInfoN48D), which is the strongest
// cross-check available that both readings are right.
typedef void (*WTBoxCallbackN48C)(CWTObjN48C *pObj, void *pUserData);
typedef int  (*WTNodeTestN48C)(CWTNodeN48C *pNode);

struct WTFindInfoN48C
{
	uint32            m_nListIndex;          // +0x00  0x0048c590
	LTVector          m_vCenter;             // +0x04  (not read here)
	float             m_fRadius;             // +0x10  (not read here)
	WTBoxCallbackN48C m_pfnCallback;         // +0x14  `call *0x14(%esi)`
	uint32            m_n018;                // +0x18
	void*             m_pUserData;           // +0x1c  `mov 0x1c(%esi),%edx`
	uint32            m_n020;                // +0x20
	uint32            m_n024;                // +0x24
	WTNodeTestN48C    m_pfnNodeTest;         // +0x28  `call *0x28(%esi)`
	LTVector          m_vBoxMin;             // +0x2c
	LTVector          m_vBoxMax;             // +0x38
	CWorldTreeN48C*   m_pTree;               // +0x44  `mov 0x44(%esi),%eax`
};


// The descriptor the INSERT walk carries.  0x0048ce20 builds it at %esp+0x14
// and 0x0048cee0 consumes it; every offset below is a store in the one and a
// load in the other.
struct WTInsertInfoN48C
{
	CWTObjN48C*  m_pObject;                  // +0x00  0x0048ce36 / 0x0048cef7
	uint32       m_nListIndex;               // +0x04  0x0048ce65 / 0x0048cfa1
	LTVector     m_vBoxMin;                  // +0x08  0x0048ce69..
	LTVector     m_vBoxMax;                  // +0x14  0x0048ce79..
	float        m_fMaxDim;                  // +0x20  0x0048cea0 / 0x0048cf08
	uint32       m_nNumLinks;                // +0x24  0x0048ceb5 / 0x0048cf97
};


// The four-byte quadrant path 0x0048cd80 walks: a bit string, two bits per
// level, little end first, plus the number of levels.  `mov (%esi,%edi,1),%dl`
// at 0x0048cda3 has the array at offset 0 and `mov 0x4(%edi),%ecx` the count
// at +0x04; `cmp $0x4,%esi; jae` at 0x0048cdb6 caps the array at four bytes.
struct WTPathN48C
{
	uint8  m_Bytes[WT_MAXPATH];              // +0x00
	uint32 m_nDepth;                         // +0x04
};


// The tree.  Only the three members this file defines are spelled out; the
// constructor (0x0048cce0) and Term (0x0048cd50) are already written up in
// src/engine/lithtech/lt2new4.cpp and are deliberately NOT repeated here.
// m_nCurCode at +0x08 is what the box query stamps objects with
// (0x0048c56b `mov 0x44(%esi),%eax; mov 0x8(%eax),%eax`).
class CWorldTreeN48C
{
public:
	void         AddObject(CWTObjN48C *pObj, uint32 nListIndex);   // 0x0048cde0
	void         AddObject(CWTObjN48C *pObj, LTVector *pMin,
	                       LTVector *pMax, uint32 nListIndex);     // 0x0048ce20
	CWTNodeN48C* FindNodeFromPath(WTPathN48C *pPath);              // 0x0048cd80
	void         SetCodeSource(void *pSource);                     // 0x0048cd30
	uint32       CodeSourceSlot1();                                // 0x0048cd20

public:
	void*        m_pVTable;                  // +0x00  0x004a38f8
	void*        m_pCodeSource;              // +0x04
	uint32       m_nCurCode;                 // +0x08
	CWTNodeN48C  m_RootNode;                 // +0x0c
	void*        m_p068;                     // +0x68
	void*        m_p06c;                     // +0x6c
	void*        m_p070;                     // +0x70
	CheapLTLink  m_RootObjects;              // +0x74
};


// The two free functions in this file, declared before use because both
// recurse and because 0x0048cee0 is reached from 0x0048ce20.
void wt_FindObjectsInBoxN48C(CWTNodeN48C *pNode, WTFindInfoN48C *pInfo,
                             uint32 nLevel);                       // 0x0048c540
void wt_InsertObjectN48C(CWTNodeN48C *pNode, WTInsertInfoN48C *pInfo);
                                                                   // 0x0048cee0


// ======================================================================= //
//  1.  0x0048c540, 410 bytes -- the recursive BOX QUERY.
//      cdecl, three arguments; the engine enters it from
//      CWorldTree::FindObjectsInBox at 0x0048d8cb (worldtree.cpp), which
//      pushes `$0x0` for the third.
//
//  WHAT EACH PIECE IS
//  ------------------
//   0x0048c550  `push %ebp; call *0x28(%esi); add $0x4,%esp` -- the node
//               filter, cdecl, one argument, and the whole body is guarded
//               by it (`je 0x48c6d2` is the epilogue).
//
//   0x0048c55e  m_Objects[2] walked with the index BAKED IN (`mov
//               0x14(%ebp),%edi; lea 0x10(%ebp),%ebx`), and if that list is
//               NOT empty the function RETURNS after it: 0x0048c586's
//               `jne 0x48c568` falls straight into the epilogue at
//               0x0048c588.  That is an if/else, not a fallthrough.
//
//   0x0048c590  m_Objects[pInfo->m_nListIndex] walked -- `mov (%esi),%eax;
//               mov 0x4(%ebp,%eax,8),%ebx; lea 0x0(%ebp,%eax,8),%eax`, the
//               stride-8 CheapLTLink array.  The head is spilled to
//               0x2c(%esp), which is the INCOMING arg1 slot: VC6 reusing a
//               dead parameter, not a source fact.
//
//   the stamp  `mov 0x44(%esi),%eax; mov 0x8(%eax),%eax` is
//               pInfo->m_pTree->m_nCurCode, compared against the object's
//               +0x58 and then written to it.  It appears TWICE in each
//               loop because the source writes it twice (an
//               expression used on both sides of a store is repeated).
//
//   the box test  six `fcomps` against pInfo->+0x38/0x3c/0x40 (max) and
//               +0x2c/0x30/0x34 (min).  The first three use `test $0x41 /
//               je` -- jump away when NOT (less or equal) -- so the source
//               condition that CONTINUES is `vMin.x > pInfo->m_vBoxMax.x`;
//               the last three use `test $0x1 / jne`, i.e. `<`.  Written as
//               six early-outs, which is the shape that shipped.
//
//   the descent  `test $0x1 / je` on the min side is `<`; `test $0x41 /
//               jne` on the max side is `>`.  m_Children[0] is loaded ONCE
//               (0x0048c636) and reused as the argument at 0x0048c664 --
//               one expression, CSEd -- so the guard really is
//               `if (pNode->m_Children[0])`.
//
//   nLevel     is only ever `+1`d into the recursive calls; the last one
//               uses `inc %edi` because %edi is dead after it.
// ======================================================================= //
void wt_FindObjectsInBoxN48C(CWTNodeN48C *pNode, WTFindInfoN48C *pInfo,
                             uint32 nLevel)
{
	CheapLTLink *pHead;
	LTLink      *pCur;
	CWTObjN48C  *pObj;
	LTVector     vMin, vMax;

	if (!pInfo->m_pfnNodeTest(pNode))
		return;

	pHead = &pNode->m_Objects[2];
	if (pHead->m_pNext != (LTLink*)pHead)
	{
		pCur = pHead->m_pNext;
		do
		{
			pObj = (CWTObjN48C*)pCur->m_pData;
			pCur = pCur->m_pNext;

			if (pObj->m_nTouchCode != pInfo->m_pTree->m_nCurCode)
			{
				pObj->m_nTouchCode = pInfo->m_pTree->m_nCurCode;
				pObj->OnFoundInBox(pInfo);
			}
		} while (pCur != (LTLink*)pHead);

		return;
	}

	pCur  = pNode->m_Objects[pInfo->m_nListIndex].m_pNext;
	pHead = &pNode->m_Objects[pInfo->m_nListIndex];
	while (pCur != (LTLink*)pHead)
	{
		pObj = (CWTObjN48C*)pCur->m_pData;
		pCur = pCur->m_pNext;

		if (pObj->m_nTouchCode != pInfo->m_pTree->m_nCurCode)
		{
			pObj->m_nTouchCode = pInfo->m_pTree->m_nCurCode;
			pObj->GetDims(&vMin, &vMax);

			if (vMin.x > pInfo->m_vBoxMax.x) goto nextObject;
			if (vMin.y > pInfo->m_vBoxMax.y) goto nextObject;
			if (vMin.z > pInfo->m_vBoxMax.z) goto nextObject;
			if (vMax.x < pInfo->m_vBoxMin.x) goto nextObject;
			if (vMax.y < pInfo->m_vBoxMin.y) goto nextObject;
			if (vMax.z < pInfo->m_vBoxMin.z) goto nextObject;

			pInfo->m_pfnCallback(pObj, pInfo->m_pUserData);
		}

nextObject:
		;
	}

	if (pNode->m_Children[0])
	{
		if (pInfo->m_vBoxMin.x < pNode->m_vCenter.x)
		{
			if (pInfo->m_vBoxMin.z < pNode->m_vCenter.z)
				wt_FindObjectsInBoxN48C(pNode->m_Children[0], pInfo, nLevel + 1);

			if (pInfo->m_vBoxMax.z > pNode->m_vCenter.z)
				wt_FindObjectsInBoxN48C(pNode->m_Children[1], pInfo, nLevel + 1);
		}

		if (pInfo->m_vBoxMax.x > pNode->m_vCenter.x)
		{
			if (pInfo->m_vBoxMin.z < pNode->m_vCenter.z)
				wt_FindObjectsInBoxN48C(pNode->m_Children[2], pInfo, nLevel + 1);

			if (pInfo->m_vBoxMax.z > pNode->m_vCenter.z)
				wt_FindObjectsInBoxN48C(pNode->m_Children[3], pInfo, nLevel + 1);
		}
	}
}


// ======================================================================= //
//  2.  0x0048c6e0, 54 bytes -- CWTObj::CWTObj(uint32), thiscall, `ret $0x4`,
//      returns `this` (`mov %ecx,%eax` at 0x0048c6e0, bare `mov` not a
//      reload, and the epilogue leaves %eax alone).
//
//  The vptr goes in FIRST (0x0048c6e7) because there is no base class and
//  no member with a constructor.  The link loop is a real loop in the image
//  (`mov $0x5,%ecx` ... `dec %ecx; jne 0x48c6f0`), strength-reduced to a
//  `add $0x10` walk, so it is a real loop in the source too.
//
//  Inside it the store order is +0x04, +0x00, +0x08, +0x0c.  +0x04 before
//  +0x00 is ltlink.h's TieOff() to the letter -- `m_pPrev = m_pNext =
//  (LTLink*)this;`, a chained assignment, right to left.
//
//  The two tail stores are +0x58 then +0x54, and the argument is loaded
//  just before them (0x0048c705); that is the emitted order and it is what
//  is written below.
// ======================================================================= //
CWTObjN48C::CWTObjN48C(uint32 nObjType)
{
	uint32 i;

	for (i = 0; i < WT_NUMLINKS; i++)
	{
		m_Links[i].TieOff();
		m_Links[i].m_pData = this;
		m_Links[i].m_pNode = LTNULL;
	}

	m_nTouchCode = 0xffffffff;
	m_nObjType   = nObjType;
}


// ======================================================================= //
//  3.  0x0048c720, 11 bytes -- CWTObj::~CWTObj(), thiscall.
//
//      48c720  movl $0x4a38d8,(%ecx)
//      48c726  jmp  0x0048c730
//
//  The vptr store followed by a TAIL JUMP: the destructor's entire body is
//  one call to RemoveFromLists(), and because the call is made from inside a
//  destructor VC6 resolves it statically (the dynamic type is fixed there),
//  which is why it is a direct `jmp` and not `call *0x8(%eax)`.
//
//  It is the destructor and not something else because 0x00424570, the
//  function in vtable slot 0 of BOTH tables, is
//      `push %esi; mov %ecx,%esi; call 0x0048c720;
//       testb $0x1,0x8(%esp); je L; push %esi; call 0x004290c0; L: ...`
//  -- VC6's `??_G` scalar deleting destructor around exactly this address.
// ======================================================================= //
CWTObjN48C::~CWTObjN48C()
{
	RemoveFromLists();
}


// ======================================================================= //
//  4.  0x0048c730, 74 bytes -- CWTObj::RemoveFromLists(), vtable slot 2.
//      thiscall, no arguments, bare `ret`.
//
//  It is virtual: 0x0048c730 is the value of slot 2 in BOTH 0x004a38d8 and
//  0x004a1fd4, and 0x0048c953 (`call *0x8(%edx)`) dispatches to it through
//  the vtable.  0x0045aa97 calls it directly on a fixed type.
//
//  BODY, instruction by instruction:
//    0x0048c731  m_nTouchCode = 0xffffffff, emitted first, so it is the
//                first statement.
//    0x0048c73b  `mov $0x5,%edx` with a `add $0x10,%eax` walk: the loop
//                over the five links.
//    0x0048c740..0x0048c74d  ltlink.h's CheapLTLink::Remove(), exactly:
//                `m_pPrev->m_pNext = m_pNext;` then `m_pNext->m_pPrev =
//                m_pPrev;` -- and note BOTH members are RELOADED for the
//                second statement (0x0048c748, 0x0048c74b), which is the
//                header's spelling and not a missed CSE.
//    0x0048c754/0x0048c757  TieOff(), +0x04 before +0x00, the chained
//                assignment again.  The compiler hoisted the `while`'s
//                entry load (0x0048c74f) ABOVE the two TieOff stores.
//    0x0048c75b..0x0048c770  the walk up the parent chain.  m_pNode is
//                loaded THREE times per iteration -- once for the count,
//                once for the parent -- because the source names it three
//                times; the loop-bottom test reuses the value just stored.
//
//  The link is named as a local (`pLink = &m_Links[i]`).  Written as
//  `m_Links[i].` at every use, VC6 kept the while's entry load of +0x0c
//  BELOW the two TieOff stores; through the pointer it hoists it above them,
//  as 0x0048c74f has it.
// ======================================================================= //
void CWTObjN48C::RemoveFromLists()
{
	uint32 i;

	m_nTouchCode = 0xffffffff;

	for (i = 0; i < WT_NUMLINKS; i++)
	{
		CWTLinkN48C *pLink = &m_Links[i];
		pLink->Remove();

		while (pLink->m_pNode)
		{
			pLink->m_pNode->m_nNumObjects--;
			pLink->m_pNode = pLink->m_pNode->m_pParent;
		}
	}
}


// ======================================================================= //
//  5.  0x0048c780, 12 bytes -- CWTNode::CWTNode(), thiscall, returns `this`.
//
//      push %esi; mov %ecx,%esi; call 0x0048c7a0; mov %esi,%eax; pop; ret
//
//  Init() is NOT inlined into it, and not into Term() either (0x0048c964),
//  so Init is an out-of-line member.  That it is a constructor and not an
//  Init wrapper is settled by its two call sites: 0x0048ca70, straight after
//  `push $0x5c; call 0x004290b0` (operator new) and guarded by the
//  null-test/`xor %eax,%eax` that VC6 wraps every `new` in, and 0x0048cce6,
//  the base-class slot of the tree's constructor.
// ======================================================================= //
CWTNodeN48C::CWTNodeN48C()
{
	Init();
}


// ======================================================================= //
//  6.  0x0048c790, 5 bytes -- CWTNode::~CWTNode().
//
//      48c790  jmp 0x0048c930
//
//  A destructor whose entire body is `Term();`, tail-jumped.  Its caller
//  inside this module is 0x0048c983, the `delete` in FreeChildren:
//  `mov %esi,%ecx; call 0x0048c790; push %esi; call 0x004290c0` -- VC6's
//  expansion of `delete p` for a class with a NON-virtual destructor.
//
//  CAUTION: 0x0048c790 has two callers OUTSIDE this module, 0x0041127c and
//  0x004138d7, and src/engine/lithtech/clientmgr3.cpp:435 and
//  clientshell.cpp:707 have both claimed it as an empty opaque destructor
//  for unrelated classes.  Those two claims cannot both be right and cannot
//  be right at all: the body is a jump to the world-tree node's Term, and
//  with no /Gy in this build there is no COMDAT folding to explain a shared
//  address.  Either those two members really ARE CWTNodes, or the claims
//  are wrong.  Not resolved here -- flagged.
// ======================================================================= //
CWTNodeN48C::~CWTNodeN48C()
{
	Term();
}


// ======================================================================= //
//  7.  0x0048c7a0, 76 bytes -- CWTNode::Init(), thiscall, bare `ret`.
//
//  One `xor %edx,%edx` feeds +0x48, +0x44 and then every float field; a
//  separate `xor %eax,%eax` (0x0048c7bb) feeds the four child slots, because
//  %eax was the link-loop cursor and is only free after it.  Which register
//  carries a zero is not a source fact; the STORE ORDER is, and it is:
//
//      +0x48, +0x44, [3x TieOff], +0x4c +0x50 +0x54 +0x58,
//      +0x30 +0x34 +0x38, +0x18 +0x1c +0x20, +0x24 +0x28 +0x2c, +0x3c, +0x40
//
//  The three vector triples are stored as INTEGER zeroes, which is
//  ltvector.h's `Init(T px=0, T py=0, T pz=0)` -- `x = px` on a float
//  parameter, moved as four bytes, not `fldz/fstps`.  The four child stores
//  are straight-line, but they ARE a loop: VC6 unrolls a four-trip
//  `for (i = 0; ...) m_Children[i] = LTNULL;` and the fresh `xor %eax,%eax`
//  at 0x0048c7bb is that loop's `i = 0`.  Four explicit statements reuse
//  %edx's zero instead and come out two bytes short.
// ======================================================================= //
void CWTNodeN48C::Init()
{
	uint32 i;

	m_pParent     = LTNULL;
	m_nNumObjects = 0;

	for (i = 0; i < WT_NUMLISTS; i++)
		m_Objects[i].TieOff();

	for (i = 0; i < WT_NUMKIDS; i++)
		m_Children[i] = LTNULL;

	m_vCenter.Init();
	m_vBoxMin.Init();
	m_vBoxMax.Init();

	m_fSmallestDim = 0.0f;
	m_fRadius      = 0.0f;
}


// ======================================================================= //
//  8.  0x0048c7f0, 51 bytes -- CWTNode::GetDims(LTVector*, LTVector*),
//      thiscall, `ret $0x8`.  Its one caller is 0x00424fd6, which
//      immediately does `flds 0x34(%esp); fsubs 0x24(%esp)` on the two
//      results -- a max-minus-min, confirming the argument order.
//
//  `lea 0x18(%ecx),%eax` and `add $0x24,%ecx` are both taken BEFORE either
//  copy (0x0048c7f4, 0x0048c7f8): the two 12-byte memberwise copies, with
//  the `pop %esi` scheduled into the middle of the first one.
// ======================================================================= //
void CWTNodeN48C::GetDims(LTVector *pMin, LTVector *pMax)
{
	*pMin = m_vBoxMin;
	*pMax = m_vBoxMax;
}


// ======================================================================= //
//  9.  0x0048c830, 251 bytes -- CWTNode::SetDims(LTVector, LTVector),
//      thiscall, `ret $0x18`: two 12-byte vectors BY VALUE.  Six callers,
//      four of them AllocChildren's and two in worldtree.cpp's range
//      (0x0048d986, 0x0048da5b).  NO CALLS AT ALL in the body, so every
//      vector operation here is inlined -- which is only possible with
//      ltvector.h's in-class definitions.
//
//  THE FRAME.  `sub $0x18,%esp` and the parameters land at %esp+0x1c
//  (vMin.x/y/z) and %esp+0x28 (vMax.x/y/z).  The 0x18 bytes of locals are
//  six float temporaries, %esp+0x00 .. %esp+0x14.
//
//  READING IT BACK
//  ---------------
//   * 0x0048c84c/0x0048c85e/0x0048c869, through `lea 0x18(%ecx),%eax` --
//     three INTEGER moves straight from the parameter slots: the memberwise
//     copy `m_vBoxMin = vMin`.  Likewise 0x0048c887.. through `lea
//     0x24(%ecx),%eax` for m_vBoxMax.
//
//   * dx is computed FIRST (0x0048c833) and then LIVES ON THE x87 STACK for
//     the whole function: `fld %st(0)` at 0x0048c842 duplicates it, and
//     every later fstps pops back down to it.  dy (0x0048c848) and dz
//     (0x0048c85a) go to temporaries.  All three are then multiplied by
//     0x004a1a74 (= 0.5f) into %esp+0x00/0x04/0x08.  Three components
//     subtracted and three multiplied is `(vMax - vMin) * 0.5f` -- and
//     ltvector.h's `operator-(const _CVector<T> v)` takes v BY VALUE, which
//     is why AllocChildren (where the operand is a member, not a parameter)
//     has to copy it first and this one does not.
//
//   * 0x0048c8a6/0x0048c8b8/0x0048c8ca add vMin back in and 0x0048c8c4..
//     move the three results, as integers, through `lea 0x30(%ecx),%eax`:
//     `m_vCenter = vMin + vHalfDims`, the by-value operator+ and then the
//     memberwise assignment of its return temporary.  The three adds put
//     their operands in DIFFERENT orders (0x0048c8aa has the half first,
//     0x0048c8bc has vMin first); /O2 canonicalises commutative FP sums, so
//     that is not reachable from source.
//
//   * 0x0048c8dd recomputes vMax.z - vMin.z rather than reusing %esp+0x14,
//     stores it at %esp+0x1c and then both COMPARES against it (0x0048c8f0)
//     and RELOADS it (0x0048c8fd).  Two uses of one value, against dx which
//     is still in %st(0) and is also used twice.  That is LTMIN's expansion
//     -- ltbasedefs.h:1245, `((a) < (b) ? (a) : (b))` -- with both arms CSEd:
//     `test $0x1 / jne` keeps %st(0) when a < b and otherwise pops it and
//     loads b.  It is the ternary, not `if (a >= b) a = b;`, because the two
//     arms join on a single `fstps 0x3c(%ecx)`.
//
//   * 0x0048c904..0x0048c922 is ltvector.h's Mag() on the half-dims:
//     z*z, then y*y, faddp, then x*x, faddp, fsqrt.  The header spells it
//     `(x*x)+(y*y)+(z*z)`; the reassociation is /O2's.
// ======================================================================= //
void CWTNodeN48C::SetDims(LTVector vMin, LTVector vMax)
{
	LTVector vHalfDims;

	m_vBoxMin = vMin;
	m_vBoxMax = vMax;

	vHalfDims = (vMax - vMin) * 0.5f;
	m_vCenter = vMin + vHalfDims;

	m_fSmallestDim = LTMIN(vMax.x - vMin.x, vMax.z - vMin.z);
	m_fRadius      = vHalfDims.Mag();
}


// ======================================================================= //
//  10.  0x0048c930, 62 bytes -- CWTNode::Term(), thiscall, bare `ret`.
//
//   0x0048c936  FreeChildren() first.
//   0x0048c93d  `mov $0x3,%ebx` with `add $0x8,%edi`: the three object
//               lists, walked with the head in %edi and compared against it
//               (`cmp %edi,%eax`), i.e. the CheapLTLink circular-list idiom.
//   0x0048c949  pNext is read BEFORE m_pData here (the box query reads them
//               the other way round), and the call is
//               `mov (%eax),%edx; call *0x8(%edx)` -- vtable slot 2,
//               RemoveFromLists, dispatched virtually.  Reading pNext first
//               is necessary and is what the source says: the callee unlinks
//               pCur.
//   0x0048c964  Init() last, restoring the node to the empty state.
// ======================================================================= //
void CWTNodeN48C::Term()
{
	LTLink     *pCur, *pNext;
	CWTObjN48C *pObj;
	uint32      i;

	FreeChildren();

	for (i = 0; i < WT_NUMLISTS; i++)
	{
		pCur = m_Objects[i].m_pNext;
		while (pCur != (LTLink*)&m_Objects[i])
		{
			pNext = pCur->m_pNext;
			pObj  = (CWTObjN48C*)pCur->m_pData;
			pObj->RemoveFromLists();
			pCur  = pNext;
		}
	}

	Init();
}


// ======================================================================= //
//  11.  0x0048c970, 49 bytes -- CWTNode::FreeChildren(), thiscall, `ret`.
//
//   `lea 0x4c(%ecx),%edi; mov $0x4,%ebx` then `add $0x4,%edi` -- the child
//   array.  The null test at 0x0048c97d guards BOTH the delete and the
//   `movl $0x0,(%edi)` that follows it, so the `if` is in the source and
//   `delete`'s own redundant null test was folded away, not the other way
//   round.  0x0048c983 is the destructor and 0x0048c989 `call 0x004290c0`
//   is operator delete (it forwards to 0x00429080, free).
// ======================================================================= //
void CWTNodeN48C::FreeChildren()
{
	uint32 i;

	for (i = 0; i < WT_NUMKIDS; i++)
	{
		if (m_Children[i])
		{
			delete m_Children[i];
			m_Children[i] = LTNULL;
		}
	}
}


// ======================================================================= //
//  12.  0x0048c9b0, 155 bytes -- CWTNode::LoadNodes(ILTStream*, uint8*,
//       uint8*), thiscall, `ret $0xc`, returns LTBOOL.
//
//  Its other caller is 0x0048da6d, in worldtree.cpp's CWorldTree::Load.
//
//   0x0048c9bb  FreeChildren() before anything is read.
//   0x0048c9c8  `cmpb $0x8,(%esi)` -- arg3 is the BIT counter, one byte.
//   0x0048c9d6  `mov (%ecx),%eax; call *0x8(%eax)` on arg1 with (%ebp, $1):
//               ILTStream vtable slot 2 is Read(void*, unsigned long) --
//               slot 0 is the protected virtual dtor, slot 1 Release --
//               so arg2 is the BYTE buffer.  `ret $8`, no esp fixup after.
//   0x0048c9de..0x0048c9ee  `mov $1,%eax; shl %cl,%eax; and %dl,%al` then
//               `neg %al; sbb %eax,%eax; neg %eax`, VC6's 0/1 materialisation
//               of a != 0 -- so the bit is taken into a variable, not tested
//               in place.  `inc %cl` and the store to (%esi) sit BETWEEN the
//               materialisation and the branch, which is the post-increment.
//   0x0048ca08/0x0048ca35/0x0048ca48  three separate `ret $0xc` epilogues,
//               returning AllocChildren's own 0, then 1, then 0.
//   0x0048ca3c  FreeChildren() again on the failure path -- and through the
//               SPILLED `this` at 0x10(%esp) (stored at 0x0048c9b7), because
//               %edi has been walking the child array since 0x0048ca0d.
// ======================================================================= //
LTBOOL CWTNodeN48C::LoadNodes(ILTStream *pStream, uint8 *pCurByte,
                              uint8 *pCurBit)
{
	LTBOOL bHasChildren;
	uint32 i;

	FreeChildren();

	if (*pCurBit == 8)
	{
		pStream->Read(pCurByte, 1);
		*pCurBit = 0;
	}

	bHasChildren = ((*pCurByte & (1 << *pCurBit)) != 0);
	(*pCurBit)++;

	if (bHasChildren)
	{
		if (!AllocChildren())
			return LTFALSE;

		for (i = 0; i < WT_NUMKIDS; i++)
		{
			if (!m_Children[i]->LoadNodes(pStream, pCurByte, pCurBit))
			{
				FreeChildren();
				return LTFALSE;
			}
		}
	}

	return LTTRUE;
}


// ======================================================================= //
//  13.  0x0048ca50, 590 bytes -- CWTNode::AllocChildren(), thiscall, no
//       arguments, LTBOOL back.  Also called from 0x0048daba
//       (worldtree.cpp's CopyNodes).
//
//  THE ALLOCATION LOOP (0x0048ca60..0x0048ca8d).  `push $0x5c; call
//  0x004290b0; add $0x4,%esp; test %eax,%eax; je L1; mov %eax,%ecx; call
//  0x0048c780; jmp L2; L1: xor %eax,%eax; L2:` -- VC6's `new CWTNode`, with
//  the node's size 0x5c and its constructor.  The store into
//  m_Children[i] and the `je` failure exit come next, then
//  `mov %esi,0x48(%eax)`, the child's parent pointer, in the loop tail.
//  Failure goes to 0x0048cc8d, `mov %esi,%ecx; call 0x0048c930` -- Term(),
//  not FreeChildren() -- and returns 0.
//
//  THE HALF-DIMENSIONS (0x0048ca8f..0x0048cad4).  m_vBoxMin is COPIED, all
//  three components, into a 12-byte stack temporary (0x0048ca9a, 0x0048caad,
//  0x0048cab4) and the subtractions read the COPY.  That copy is the whole
//  argument for spelling this as ltvector.h's `operator-`, which takes its
//  operand BY VALUE.  Only .x and .z of the result are then multiplied by
//  0.5f; the .y component is dead and was eliminated.  So:
//
//      LTVector vHalfDims = (m_vBoxMax - m_vBoxMin) * 0.5f;
//
//  and NOT two scalar expressions, which would need no copy at all.
//
//  THE FOUR CHILDREN.  Each block builds two 12-byte temporaries and then
//  copies them, as integers, into an argument area opened by two `sub
//  $0xc,%esp` -- VC6 passing a class temporary by value.  In every block the
//  SECOND argument's temporary is filled FIRST, which is argument evaluation
//  right to left, so the source really does construct both vectors inline in
//  the call.  The two temporaries also SWAP stack slots between blocks
//  (block 1 has vMax at %esp+0x1c and vMin at %esp+0x28, block 2 the other
//  way round), which means they are per-call temporaries and not two locals
//  reused -- live-range allocation.
//
//  The quadrants, read off the four `mov 0x4c/0x50/0x54/0x58(%esi),%ecx`:
//
//      [0] x in [cx-hx, cx]      z in [cz-hz, cz]
//      [1] x in [cx-hx, cx]      z in [cz,    cz+hz]
//      [2] x in [cx,    cx+hx]   z in [cz-hz, cz]
//      [3] x in [cx,    cx+hx]   z in [cz,    cz+hz]
//
//  and y is m_vBoxMin.y .. m_vBoxMax.y in all four: the tree does not split
//  on Y.  That is the same quadrant numbering the box query descends with.
// ======================================================================= //
LTBOOL CWTNodeN48C::AllocChildren()
{
	LTVector vHalfDims;
	uint32   i;

	for (i = 0; i < WT_NUMKIDS; i++)
	{
		m_Children[i] = new CWTNodeN48C;
		if (!m_Children[i])
		{
			Term();
			return LTFALSE;
		}

		m_Children[i]->m_pParent = this;
	}

	vHalfDims = (m_vBoxMax - m_vBoxMin) * 0.5f;

	m_Children[0]->SetDims(
		LTVector(m_vCenter.x - vHalfDims.x, m_vBoxMin.y, m_vCenter.z - vHalfDims.z),
		LTVector(m_vCenter.x,               m_vBoxMax.y, m_vCenter.z));

	m_Children[1]->SetDims(
		LTVector(m_vCenter.x - vHalfDims.x, m_vBoxMin.y, m_vCenter.z),
		LTVector(m_vCenter.x,               m_vBoxMax.y, m_vCenter.z + vHalfDims.z));

	m_Children[2]->SetDims(
		LTVector(m_vCenter.x,               m_vBoxMin.y, m_vCenter.z - vHalfDims.z),
		LTVector(m_vCenter.x + vHalfDims.x, m_vBoxMax.y, m_vCenter.z));

	m_Children[3]->SetDims(
		LTVector(m_vCenter.x,               m_vBoxMin.y, m_vCenter.z),
		LTVector(m_vCenter.x + vHalfDims.x, m_vBoxMax.y, m_vCenter.z + vHalfDims.z));

	return LTTRUE;
}


// ======================================================================= //
//  14.  0x0048cca0, 52 bytes -- CWTNode::AddObjectLink(CWTLink*, uint32),
//       thiscall, `ret $0x8`.  Four callers: 0x0045b017, 0x0045b041 and
//       0x0045b087 in the engine (passing list index 0, 2 and 0, and always
//       `lea 0x4(%obj)` = &m_Links[0]) and 0x0048cfaf below.
//
//   0x0048cca6  `lea (%ecx,%eax,8),%edx` -- m_Objects, stride 8, at offset
//               zero, indexed by the second argument.
//   0x0048ccad..0x0048ccb9  ltlink.h's CheapLTLink::AddAfter to the letter,
//               including the RELOAD of pLink->m_pPrev at 0x0048ccb7: the
//               header's last line is the chained
//               `pLink->m_pPrev->m_pNext = pLink->m_pNext->m_pPrev = pLink;`
//               and a chained assignment goes right to left, which is why
//               `(%edx) = %eax` comes before `0x4(%edx) = %eax`.
//   0x0048ccbc  the link's node back-pointer.
//   0x0048cca4 / 0x0048ccbf..0x0048cccf  the walk up the parent chain
//               incrementing m_nNumObjects.  `test %ecx,%ecx` is hoisted to
//               the third instruction of the function; it is the while's
//               entry test, not a null check on `this`.
// ======================================================================= //
void CWTNodeN48C::AddObjectLink(CWTLinkN48C *pLink, uint32 nList)
{
	CWTNodeN48C *pNode;

	m_Objects[nList].AddAfter(pLink);
	pLink->m_pNode = this;

	pNode = this;
	while (pNode)
	{
		pNode->m_nNumObjects++;
		pNode = pNode->m_pParent;
	}
}


// ======================================================================= //
//  15.  0x0048cd30, 18 bytes -- CWorldTree::SetCodeSource(void*), thiscall,
//       `ret $0x4`.  Two callers, 0x0040f92c and 0x00473244.
//
//       48cd30  mov 0x4(%esp),%eax ; mov %eax,0x4(%ecx)
//       48cd37  lea 0x74(%ecx),%eax ; mov %eax,0x4(%eax) ; mov %eax,(%eax)
//
//  +0x04 is the same member lt2new4.cpp's constructor clears first, and the
//  three-instruction tail is CheapLTLink::TieOff() on m_RootObjects, +0x04
//  before +0x00 exactly as everywhere else.
// ======================================================================= //
void CWorldTreeN48C::SetCodeSource(void *pSource)
{
	m_pCodeSource = pSource;
	m_RootObjects.TieOff();
}


// ======================================================================= //
//  16.  0x0048cd20, 8 bytes -- a two-hop forward through m_pCodeSource.
//
//       48cd20  mov 0x4(%ecx),%ecx ; mov (%ecx),%eax ; jmp *0x4(%eax)
//
//  thiscall, no arguments, and a TAIL JUMP, so the callee's return value is
//  this function's.  Slot 1 of whatever m_pCodeSource points at; slot 0 of
//  the same object is what worldtree.cpp's 0x0048d219 and 0x0048d81a call
//  to mint a new query stamp.  Its one caller is 0x0045af9a.
//
//  The return TYPE is not pinned by anything in the image -- only that it is
//  a single register.  uint32 below; marked accordingly.
// ======================================================================= //
class IWTCodeSourceN48C
{
public:
	virtual uint32 NextCode() = 0;                            // slot 0
	virtual uint32 Slot1() = 0;                               // slot 1
};

uint32 CWorldTreeN48C::CodeSourceSlot1()                      // 0x0048cd20
{
	return ((IWTCodeSourceN48C*)m_pCodeSource)->Slot1();
}


// ======================================================================= //
//  17.  0x0048cd80, 87 bytes -- CWorldTree::FindNodeFromPath(WTPath*),
//       thiscall, `ret $0x4`, returns a node pointer.  It is SLOT 0 of the
//       tree's own seven-slot vtable at 0x004a38f8 (worldtree.cpp's table);
//       nothing calls it directly.
//
//   0x0048cd88  `lea 0xc(%ecx),%eax` -- the walk starts at &m_RootNode, and
//               %eax stays the result register all the way to both exits.
//   0x0048cd8f  `mov 0x4(%edi),%ecx; test %ecx,%ecx; je 0x48cdd0` -- a zero
//               depth returns the root, NOT null: 0x0048cdd0 is entered with
//               %eax still holding &m_RootNode.  Only 0x0048cdce zeroes it.
//   0x0048cda3  `mov (%esi,%edi,1),%dl` -- the byte array is at offset 0 of
//               the argument and %esi is its index.
//   0x0048cda6/0x0048cdae  two bits per level, and the byte index only
//               advances when the bit position reaches 8.
//   0x0048cdb6  `cmp $0x4,%esi; jae 0x48cdce` -- four bytes, so sixteen
//               levels, and running past them returns null.
//   0x0048cd98  `mov 0x4c(%eax),%ecx; test; je 0x48cdce` -- a leaf reached
//               before the path is exhausted also returns null.
//   0x0048cdbe  `mov 0x4c(%eax,%edx,4),%eax` -- descend.
//
//   The loop test is `!=`, not `<`: 0x0048cd96 is `je` and 0x0048cdc5 is
//   `jne` (a `<` gives `jbe`/`jb`).  The three zeroings are emitted in
//   source order -- %ebp (i), %ebx (nBit), then %esi (nByte) after the
//   depth load -- so i is initialised as its own statement ahead of them.
// ======================================================================= //
CWTNodeN48C* CWorldTreeN48C::FindNodeFromPath(WTPathN48C *pPath)
{
	CWTNodeN48C *pNode;
	uint32       i, nByte, nBit, nQuadrant;

	pNode = &m_RootNode;

	i     = 0;
	nBit  = 0;
	nByte = 0;

	for (; i != pPath->m_nDepth; i++)
	{
		if (!pNode->m_Children[0])
			return LTNULL;

		nQuadrant = (pPath->m_Bytes[nByte] >> nBit) & 3;
		nBit += 2;

		if (nBit == 8)
		{
			nByte++;
			nBit = 0;

			if (nByte >= WT_MAXPATH)
				return LTNULL;
		}

		pNode = pNode->m_Children[nQuadrant];
	}

	return pNode;
}


// ======================================================================= //
//  18.  0x0048cde0, 59 bytes -- CWorldTree::AddObject(CWTObj*, uint32),
//       thiscall, `ret $0x8`.  TWELVE callers (0x00414393, 0x004205e7,
//       0x00420873, 0x00420918, 0x004209a2, 0x00424f09, 0x0045144b,
//       0x00454237, 0x004547a8, 0x004695a8, 0x004721e6, 0x00476652).
//
//  `sub $0x18,%esp` is exactly two LTVectors; 0x0048cdf9 fills them through
//  the object's vtable slot 3 (GetDims) and 0x0048ce0e hands both, with the
//  object and the list index, to the four-argument form.  The argument
//  pointers come back out in the order (this, pObj, &vMin, &vMax, nList) --
//  the `lea 0xc(%esp)` block is the first GetDims argument and it is also
//  the third argument of the call, so it is the MIN.
//
//  NOTE: worldtree.cpp's per-address table calls 0x0048cde0 "remove an
//  object from the tree".  That is WRONG.  It inserts: 0x0048ce20 below
//  removes first and then walks the tree LINKING the object in
//  (0x0048cfaf).  clientworld.cpp:389 ("Discard") has the same mistake.
// ======================================================================= //
void CWorldTreeN48C::AddObject(CWTObjN48C *pObj, uint32 nListIndex)
{
	LTVector vMin, vMax;

	pObj->GetDims(&vMin, &vMax);
	AddObject(pObj, &vMin, &vMax, nListIndex);
}


// ======================================================================= //
//  19.  0x0048ce20, 189 bytes -- CWorldTree::AddObject(CWTObj*, LTVector*,
//       LTVector*, uint32), thiscall, `ret $0x10`.
//
//   0x0048ce2f  `mov (%esi),%eax; mov %esi,%ecx; call *0x8(%eax)` -- slot 2,
//               RemoveFromLists, on the object, BEFORE anything else: this
//               is a re-insert, so it unlinks first.
//   0x0048ce36.. the 0x28-byte descriptor is filled in declaration order at
//               %esp+0x14: object, list index, min, max, max-dim, count.
//   0x0048ce49.. `mov (%edx)/0x4(%edx)/0x8(%edx)` copies ALL THREE
//               components of *pMin into %esp+0x08 before a single
//               subtraction happens, and only .x and .z of the difference
//               are ever used.  The full copy of a value whose .y is dead is
//               ltvector.h's by-value `operator-` again.
//   0x0048ce8a/0x0048ce9b  `fcoms; test $0x41; jne` keeps %st(0) when it is
//               GREATER -- LTMAX (ltbasedefs.h:1246), the mirror of the
//               LTMIN in SetDims.
//   0x0048cebd  `call *0x4(%edx)` with the tree pushed -- slot 1, `ret $4`,
//               and a NON-zero result SKIPS the insert entirely.
//   0x0048cec8  `add $0xc,%edi` -- the walk starts at &m_RootNode.
//   ORDER: the difference is computed BEFORE the descriptor is filled.
//               Shipped copies *pMin into the operator-'s by-value temp and
//               starts the subtraction (0x0048ce3c..0x0048ce61) before it
//               copies min/max into the descriptor; with `vDims = ...`
//               written after the four info stores, VC6 copies the
//               descriptor first and needs %ebx (193 bytes, 12 blocks).
// ======================================================================= //
void CWorldTreeN48C::AddObject(CWTObjN48C *pObj, LTVector *pMin,
                               LTVector *pMax, uint32 nListIndex)
{
	WTInsertInfoN48C info;
	LTVector         vDims;

	pObj->RemoveFromLists();

	vDims = *pMax - *pMin;

	info.m_pObject    = pObj;
	info.m_nListIndex = nListIndex;
	info.m_vBoxMin    = *pMin;
	info.m_vBoxMax    = *pMax;

	info.m_fMaxDim = LTMAX(vDims.x, vDims.z);

	info.m_nNumLinks = 0;

	if (!pObj->WantsTree(this))
		wt_InsertObjectN48C(&m_RootNode, &info);
}


// ======================================================================= //
//  20.  0x0048cee0, 258 bytes -- the recursive INSERT walk, cdecl, two
//       arguments.  server.dll's twin is 0x10039d00 (src/engine/server/
//       worldtree.cpp, wt_InsertObject_E), which CALLS a five-argument
//       child-visitor (0x10039d98) with itself as the callback.  Here that
//       visitor is expanded: the four child calls are direct `call
//       0x0048cee0`s (0x0048cf39, 0x0048cf53, 0x0048cf7a) and the last one
//       is a tail `jmp 0x0048ceea` with m_Children[3] (0x0048cf8f).
//       m_Children[0] is loaded once (0x0048cf16) for the guard and reused
//       as the first call's argument.
//   0x0048ceed  m_Objects[2] -- the first object on it is told about the
//               insert (vtable slot 6).
//   0x0048cf9a  `cmp $0x5; jae` -- the link-count bound.
//   0x0048cfca  CheapLTLink::AddAfter on m_Objects[2], expanded.
// ======================================================================= //
typedef void (*WTNodeCallbackN48C)(CWTNodeN48C *pNode, void *pUser);

static inline void wt_ForEachChildInBoxN48C(LTVector *pMin, LTVector *pMax,
                                            CWTNodeN48C *pNode,
                                            WTNodeCallbackN48C pfn,
                                            void *pUser)
{
	if (pMin->x < pNode->m_vCenter.x)
	{
		if (pMin->z < pNode->m_vCenter.z)
			pfn(pNode->m_Children[0], pUser);

		if (pMax->z > pNode->m_vCenter.z)
			pfn(pNode->m_Children[1], pUser);
	}

	if (pMax->x > pNode->m_vCenter.x)
	{
		if (pMin->z < pNode->m_vCenter.z)
			pfn(pNode->m_Children[2], pUser);

		if (pMax->z > pNode->m_vCenter.z)
			pfn(pNode->m_Children[3], pUser);
	}
}

void wt_InsertObjectN48C(CWTNodeN48C *pNode, WTInsertInfoN48C *pInfo)
{
	CWTLinkN48C *pLink;

	if (pNode->m_Objects[2].m_pNext != (LTLink*)&pNode->m_Objects[2])
		((CWTObjN48C*)pNode->m_Objects[2].m_pNext->m_pData)->NotifyInserted(pInfo->m_pObject);

	if (pNode->m_fSmallestDim * 0.5f > pInfo->m_fMaxDim && pNode->m_Children[0])
	{
		wt_ForEachChildInBoxN48C(&pInfo->m_vBoxMin, &pInfo->m_vBoxMax, pNode,
		                         (WTNodeCallbackN48C)wt_InsertObjectN48C, pInfo);
	}
	else if (pInfo->m_nNumLinks < WT_NUMLINKS)
	{
		pNode->AddObjectLink(&pInfo->m_pObject->m_Links[pInfo->m_nNumLinks],
		                     pInfo->m_nListIndex);

		if (pInfo->m_pObject->WantsExtraLink())
		{
			pLink = pInfo->m_pObject->MakeExtraLink(pInfo->m_nNumLinks);
			pNode->m_Objects[2].AddAfter(pLink);
		}

		pInfo->m_nNumLinks++;
	}
}
