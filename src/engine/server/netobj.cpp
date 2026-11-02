// =====================================================================
//  w9_458000.cpp -- SCRATCH reconstruction driver for the five rows of
//  the 0x00458000 enumeration that are not yet written.
//
//  Eleven of the sixteen enumerated rows are NOT
//  reconstruction work at all: they are /O2-shape rows for functions
//  src/engine/server/netmgr.cpp, servermsg.h and objtypes.cpp ALREADY
//  CONTAIN, every one of which matches server.dll at /O1.  Three of the
//  sixteen rows are not even separate functions.  Nothing for those is
//  written here.
//
//  What IS here, with the file each function belongs in once measured:
//
//    0x0045a3b0   73   netmgr.cpp   CModelSub14 chain walk   (/O1 0x10024431)
//    0x0045bf90   55   objtypes.cpp un390_ClearSaveIndices   (/O1 0x10025a82)
//    0x0045aff0  212   objtypes.cpp CObjWorldModel slot 1    (/O1 0x10024d6e)
//    0x0045aea0  288   objtypes.cpp CObjWorldModel slot 7    (/O1 0x10024c21)
//    0x0045a160   91   msgterm.cpp  net-bank record teardown (no /O1 twin)
//
//  This is a HOLDING PEN in svnew.cpp's sense: every symbol carries the
//  Q458 tag so nothing can collide, and every view class is local.  The
//  first four have an /O1 twin in server.dll that was read instruction
//  by instruction alongside the /O2 bytes; every offset, constant and
//  branch below is cited by the lithtech.exe address it was read at.
//
//  Ground truth throughout: the retail lithtech.exe (/O2) with
//  the retail server.dll (/O1) as the structural oracle.  Build /O2.
// =====================================================================

#include "ltbasedefs.h"          // uint8/uint16/uint32, LTBOOL, LTNULL, LTVector, LTPlane
#include "ltlink.h"              // LTLink / LTList / CheapLTLink
#include "collection.h"          // CMoArray<T,C>, DefaultCache, LAlloc, g_DefAlloc
#include "de_error.h"            // dsi_ServerError


// =====================================================================
//  SECTION A -- 0x0045a3b0, 73 bytes.  BELONGS IN netmgr.cpp.
//
//  CModelSub14's chain walk.  netmgr.cpp already carries the class (its
//  constructor at 0x100243f6 and Init at 0x10024409 both match
//  server.dll) and already writes down, at netmgr.cpp:2924, exactly what
//  this body does -- it was simply never written.  The /O1 twin is
//  server.dll 0x10024431, which netmgr.cpp names; it was read here
//  instruction for instruction against the /O2 bytes and the two agree
//  in every field offset, in the call's argument order and in the
//  return value.
//
//  What the two images disagree about is loop rotation, which register
//  carries `this`, and whether the next-node load comes before or after
//  the hook-list load -- all allocation, none of it source:
//
//     /O1 0x10024441  mov 0x8(%edi),%ebx ; mov (%edi),%esi   (next first)
//     /O2 0x0045a3c0  mov (%edi),%esi ; mov 0x8(%edi),%eax   (hooks first)
//
//  so the source order below (pNext saved first, the way a
//  remove-safe walk is always written) is taken from /O1 and is not
//  contradicted by /O2.
//
//  The return is LTFALSE and /O2 never materialises it: the fall-out of
//  the outer loop at 0x0045a3e9 leaves %eax holding the null it just
//  tested, and 0x0045a3f2's `xor %eax,%eax` is the empty-list arm's own.
//  /O1 spells it `xor %eax,%eax` once at 0x10024466 for both.  A `void`
//  return contradicts both images -- they are `ret $0x8` with a defined
//  %eax -- and 0x10024419's caller discards it, which is why the type is
//  only pinned to "one register".
// =====================================================================

class CModelSub14NodeQ458;

// The hook's function pointer is called CDECL with four arguments -- the
// owner, the node, and the walk's own two -- pushed at 0x0045a3d1..
// 0x0045a3d4 as ebp, eax, edi, ebx and torn down with `add $0x10,%esp`
// at 0x0045a3da.  /O1 pushes the same four at 0x1002444a..0x10024451.
typedef void (*CModelSub14FnQ458)(void* pOwner, CModelSub14NodeQ458* pNode,
                                  void* pArg1, void* pArg2);

// The inner list: a function pointer and its own next.  0x0045a3d5
// `call *(%esi)` and 0x0045a3d7 `mov 0x4(%esi),%esi`.
class CModelSub14HookQ458
{
public:
	CModelSub14FnQ458    m_pFn;              // +0x00  0x0045a3d5
	CModelSub14HookQ458* m_pNext;            // +0x04  0x0045a3d7
};

// The outer list: the hook chain at +0x00 (0x0045a3c0 `mov (%edi),%esi`),
// one dword this walk never reads, and the next node at +0x08
// (0x0045a3c2 `mov 0x8(%edi),%eax`).
class CModelSub14NodeQ458
{
public:
	CModelSub14HookQ458* m_pHooks;           // +0x00
	uint32               m_Unknown04;        // +0x04
	CModelSub14NodeQ458* m_pNext;            // +0x08
};

// netmgr.cpp's CModelSub14, repeated locally so this driver needs no
// include of it: m_pHead at +0x00 (0x0045a3b4 `mov (%ebx),%edi`) and the
// owner at +0x04, both established there by 0x10024409.
class CModelSub14Q458
{
public:
	LTBOOL WalkQ458(void* pArg1, void* pArg2);   // 0x0045a3b0 / 0x10024431

	CModelSub14NodeQ458* m_pHead;            // +0x00
	void*                m_pOwner;           // +0x04
};

LTBOOL CModelSub14Q458::WalkQ458(void* pArg1, void* pArg2)
{
	CModelSub14NodeQ458* pNode;
	CModelSub14NodeQ458* pNext;
	CModelSub14HookQ458* pHook;

	pNode = m_pHead;

	// 0x0045a3b6 `test %edi,%edi` / `je 0x45a3f2`, so the outer loop is
	// entered under a guard and rotated -- 0x0045a3e9 `jne 0x45a3c0`.
	while (pNode)
	{
		pNext = pNode->m_pNext;

		// 0x0045a3c5 `test %esi,%esi` / `je 0x45a3e5`, then the rotated
		// body at 0x0045a3cd..0x0045a3df.
		for (pHook = pNode->m_pHooks; pHook; pHook = pHook->m_pNext)
			pHook->m_pFn(this, pNode, pArg1, pArg2);

		pNode = pNext;
	}

	return LTFALSE;
}


// =====================================================================
//  SECTION B -- 0x0045bf90, 55 bytes.  BELONGS IN objtypes.cpp.
//
//  un390_ClearSaveIndices.  saveio.cpp:275 already DECLARES it, by that
//  name and with its /O1 address (0x10025a82), as "every live object's
//  save index back to not-saved", and saveio.cpp:165 already names the
//  field -- SavObject::m_nSaveIndex at +0xba, with SAV_NOINDEX 0xffff.
//  Nothing defines it.  Its two callers are inside saveio.cpp's own
//  sav_SaveObjects (0x004317f2, inside the 0x00431770 row) and one more
//  save-side function at 0x004325c9; both pass the manager's +0x390.
//
//  The walk is CUnknown390::Lock's (0x1002461c), which objtypes.cpp
//  already reconstructs byte-identically, with the body swapped: eleven
//  per-type lists at +0x220, the inner traversal's initialiser hoisted
//  to the outer loop's top, and `i` strength-reduced into a list pointer
//  plus a count-down.  Written the same way, for the same reason.
//
//    /O2 0x0045bf96  mov  $0xb,%esi          the count-down
//    /O2 0x0045bf9b  lea  0x224(%eax),%ecx   &m_Lists[0].m_Head
//    /O2 0x0045bfbe  add  $0x10,%ecx         sizeof(LTList)
//
//  The store: /O2 parks 0xffff in %edx once (0x0045bfa1) and stores the
//  word (0x0045bfb0 `mov %dx,0xba(%edi)`); /O1 spells the same
//  assignment `or $0xffff,0xba(%esi)` at 0x10025a9a, which /Os prefers
//  because it needs no register.  Both are `= 0xffff` on a uint16.
// =====================================================================

// ltobject_internal.h's LTObject and objtypes.h's CObjBase both model
// this object; only the one word is reached from here, so the view is
// saveio.cpp's SavObject cut down to it.
class CObjSaveIndexViewQ458
{
public:
	uint8  m_pad_000[0x0ba];
	uint16 m_nSaveIndex;                     // 0x0ba  saveio.cpp:179
};

// objtypes.cpp's CUnknown390, cut to the one member this reaches. The
// eleven and the +0x220 are that file's, read off un390_Init; LTList is
// ltlink.h's, which is what makes the stride 0x10.
class CUnknown390SaveViewQ458
{
public:
	uint8  m_pad_000[0x220];
	LTList m_Lists[11];                      // 0x220
};

#define SAV_NOINDEXQ458 0xffff

void un390_ClearSaveIndicesQ458(CUnknown390SaveViewQ458* p390)
{
	LTLink* pLink;
	int     i;

	for (i = 0; i < 11; i++)
	{
		for (pLink = p390->m_Lists[i].m_Head.m_pNext;
		     pLink != &p390->m_Lists[i].m_Head;
		     pLink = pLink->m_pNext)
		{
			((CObjSaveIndexViewQ458*)pLink->m_pData)->m_nSaveIndex =
				SAV_NOINDEXQ458;
		}
	}
}


// =====================================================================
//  SECTIONS C and D -- the two CObjWorldModel virtuals the base leaves
//  unreconstructed.  BOTH BELONG IN objtypes.cpp.
//
//  WHAT NAMES THEM.  objtypes.h models the object hierarchy with a base
//  of fourteen virtuals and leaves slots 4..7 as `_v10.._v1c`, declared
//  and never defined.  worldtree2.cpp independently reconstructed a
//  world-tree object base -- CWTObjN48C, vtable 0x004a38d8, eight slots
//  -- and the two tables LINE UP EXACTLY on their shared tail:
//
//      CObjBase (0x004a2c08)   slot 4  0x00432040   slot 5  0x0042d1b0
//                              slot 6  0x00459f70   slot 7  0x00459f70
//      CWTObj   (0x004a38d8)   slot 4  0x00432040   slot 5  0x0042d1b0
//                              slot 6  0x00459f70   slot 7  0x00459f70
//
//  and on slot 3, which is abstract in CWTObj ("two LTVector*") and is
//  CObjBase::GetDims(LTVector*, LTVector*).  So CObjBase IS a CWTObj,
//  slot for slot, and objtypes.h's `_v04` and `_v1c` are worldtree2.cpp's
//  WantsTree(CWorldTree*) and OnFoundInBox(WTFindInfo*).  That also
//  explains the object's +0x04..+0x54 hole, which objtypes.h has as
//  m_pad_04 and worldtree2.cpp has as `CWTLink m_Links[WT_NUMLINKS]`
//  with m_nObjType at +0x54 -- and it is the +0x04 both bodies below
//  hand to CWTNode::AddObjectLink.
//
//  Both have an /O1 twin in server.dll and svnew.cpp already writes both
//  up as NOT RECONSTRUCTED (svnew.cpp:893 for 0x10024c21, svnew.cpp:904
//  for 0x10024d6e) together with the query record's constructor.
// =====================================================================

// --------------------------------------------------------------------
//  The pieces both bodies reach.  Every one is declared and not defined:
//  each has its own home (worldtree2.cpp, objtypes.cpp) and what this
//  driver needs is the CALL, not the callee.
// --------------------------------------------------------------------

// ltobject_internal.h's IWorldBsp, cut to the two slots used here: slot 0
// IsBusy (0x0045b062 `call *(%edx)`) and slot 6 GetWorldFlags
// (0x0045b003 `call *0x18(%eax)`).  Pure, so no vftable is emitted for it.
class IWorldBspQ458
{
public:
	virtual uint32 IsBusy() = 0;             // 0x00
	virtual void   _v04() = 0;
	virtual void   _v08() = 0;
	virtual void   _v0c() = 0;
	virtual void   _v10() = 0;
	virtual void   _v14() = 0;
	virtual uint32 GetWorldFlags() = 0;      // 0x18

	// The section's name, which the error message below prints, and the
	// +0x48 the tree is asked to resolve.  Offsets read at 0x0045b09d
	// (`lea 0x4(%esi),%eax`) and 0x0045b073 (`add $0x48,%eax`).
	char   m_Name[0x48 - 0x04];              // 0x04  the section's name
	uint8  m_NodePath[0x4c - 0x48];          // 0x48
};

// worldtree2.cpp's CWTLinkN48C -- ltlink.h's LTLink plus the owning node
// at +0x0c, 0x10 bytes.  Only its ADDRESS is used from here.
class CWTLinkQ458
{
public:
	uint8 m_pad_00[0x10];
};

// worldtree2.cpp's CWTNodeN48C, 0x5c bytes.  AddObjectLink is 0x0048cca0,
// thiscall, `ret $0x8`: the link then the list index.
class CWTNodeQ458
{
public:
	void AddObjectLinkQ458(CWTLinkQ458* pLink, uint32 nList);   // 0x0048cca0

	uint8 m_pad_00[0x5c];
};

// worldtree2.cpp's CWorldTreeN48C as slot 1 sees it: a vptr whose slot 0
// resolves a path to a node (0x0048cd80, FindNodeFromPath -- the table at
// 0x004a38f8 puts it in slot 0, and 0x0045b079 `call *(%edx)` is that
// slot), and the root node embedded at +0x0c (0x0045b014 `add $0xc,%ecx`,
// which is also worldtree2.cpp's own `lea 0xc(%ecx),%eax` at 0x0048cd88).
class CWorldTreeQ458
{
public:
	virtual CWTNodeQ458* FindNodeFromPathQ458(void* pPath) = 0;   // slot 0

	uint8       m_pad_004[0x0c - 0x04];
	CWTNodeQ458 m_RootNode;                  // 0x0c
};

// worldtree2.cpp's 0x0048cd20 (server.dll 0x10039ba2): thiscall, no
// arguments, a tail jump through m_pCodeSource's slot 1.  The shipped
// call at 0x0045af9a takes its `this` straight out of %ecx with nothing
// pushed, so it is a MEMBER here and not worldtree2.cpp's free-function
// spelling, which would push.
class CWTCodeHolderQ458
{
public:
	uint32 CodeSourceSlot1Q458();            // 0x0048cd20 / 0x10039ba2
};

// worldtree2.cpp's WTFindInfoN48C, extended by the one field past its end
// that slot 7 reaches (+0x44, read at 0x0045af8b).  Offsets 0x04..0x24
// are that file's, derived independently from 0x0048c540.
typedef void (*WTBoxCallbackQ458)(void* pObj, void* pUserData);

struct WTFindInfoQ458
{
	uint32             m_nListIndex;         // +0x00
	LTVector           m_vCenter;            // +0x04   0x0045aeeb
	float              m_fRadius;            // +0x10
	WTBoxCallbackQ458  m_pfnCallback;        // +0x14   0x0045af73
	uint32             m_n018;               // +0x18   0x0045af7d
	void*              m_pUserData;          // +0x1c   0x0045af76
	uint32             m_n020;               // +0x20   0x0045af84
	uint32             m_n024;               // +0x24   0x0045af9f
	uint8              m_pad_028[0x44 - 0x28];
	CWTCodeHolderQ458* m_pCodeHolder;        // +0x44   0x0045af8b
};


// --------------------------------------------------------------------
//  objtypes.cpp's BSP-walk views and w_FindPolyAtPoint, transcribed
//  UNCHANGED except for the tag and the `inline`.
//
//  WHY IT HAS TO BE `inline` AND HOISTED.  server.dll CALLS it out of
//  line from 0x10024c41; lithtech.exe EXPANDS it whole into 0x0045aea0
//  (0x0045aeda..0x0045af71 is this body, instruction for instruction,
//  including the uint16 answer carried in %dx and the `>= -0.0001f`
//  side test at 0x0045af34).  /Os declining what /Ot accepts is the
//  ordinary reason for that asymmetry, and the
//  definition must sit ABOVE its caller for VC6 to consider it at all.
//
//  /O1.  This copy is byte-identical to server.dll 0x10024cb1.  Its nine
//  frame-slot differences (blamed on cl-8168 in objtypes.cpp) were the
//  order of the first two statements -- see below.  The copy here is a
//  SECOND COMDAT under a different name; objtypes.cpp is untouched.
// --------------------------------------------------------------------

class CBspPolyOwnerQ458;

class CBspWalkNodeQ458
{
public:
	CBspPolyOwnerQ458* m_pOwner;             // 0x00
	CBspWalkNodeQ458*  m_pChild[2];          // 0x04  [0] behind, [1] in front
	uint8              m_pad_0c[0x14 - 0x0c];
	uint16             m_nPolyIndex;         // 0x14
	uint8              m_Flags;              // 0x16

	LTPlane* GetPlane();
};

class CBspPolyParentQ458
{
public:
	uint8    m_pad_00[0x34];
	LTPlane* m_pPlane;                       // 0x34
};

class CBspPolyOwnerQ458
{
public:
	uint8               m_pad_00[0x28];
	CBspPolyParentQ458* m_pParent;           // 0x28
};

inline LTPlane* CBspWalkNodeQ458::GetPlane()
{
	return m_pOwner->m_pParent->m_pPlane;
}

class CBspWorldPolyQ458
{
public:
	uint8  m_pad_00[0x16];
	uint16 m_nIndex;                         // 0x16
	uint8  m_pad_18[0x30 - 0x18];
};

class CBspWorldViewQ458
{
public:
	uint8               m_pad_00[0x88];
	CBspWorldPolyQ458*  m_pPolys;            // 0x88   0x0045af5f
	uint8               m_pad_8c[0x9c - 0x8c];
	CBspWalkNodeQ458*   m_pRoot;             // 0x9c   0x0045aeda
};

#define BSPNODE_LEAFQ458 0x03

inline CBspWorldPolyQ458* w_FindPolyAtPointQ458(CBspWorldViewQ458* pWorld,
                                                LTVector* pPoint)
{
	uint16             nIndex;
	LTVector           vPoint;
	CBspWalkNodeQ458*  pNode;
	LTPlane*           pPlane;

	// Root first, THEN the sentinel -- the order both images emit it
	// (0x0045aeda before 0x0045aee0; server.dll 0x10024cc0 before the
	// 0x10024cc6 store).  Written the other way round, /O1 gives nIndex
	// a frame slot of its own (a 0x10 frame); this way it goes into the
	// dead pWorld parameter slot, 0x8(%ebp), with the shipped 0xc frame.
	pNode  = pWorld->m_pRoot;                // 0x0045aeda
	nIndex = 0xffff;                         // 0x0045aee0

	// The root-is-a-leaf case is NOT its own `return LTNULL`. Writing it
	// that way makes VC6 materialise the null and jump (`je` + `xor
	// %eax,%eax` + `jmp`, seven bytes more); the shipped build has a bare
	// `jne 0x0045af5b` at 0x0045aee9 straight onto the failure return the
	// `nIndex == 0xffff` test below already owns. It can do that because
	// nIndex is still 0xffff on this path and the optimiser knows it, so
	// the two null exits are ONE block. Guarding the traversal instead of
	// returning early is what puts them there.
	if (!(pNode->m_Flags & BSPNODE_LEAFQ458))   // 0x0045aee5
	{
		vPoint = *pPoint;                    // 0x0045aeef..0x0045af01

		do
		{
			if (pNode->m_nPolyIndex != 0xffff)   // 0x0045af05
				nIndex = pNode->m_nPolyIndex;

			pPlane = pNode->GetPlane();      // 0x0045af11 / 0x0045af17 / 0x0045af1a

			// 0x0045af34 fcomp against 0x004a2c7c = -9.9999997e-05f, so a
			// point exactly on a plane goes IN FRONT.
			pNode = pNode->m_pChild[pPlane->DistTo(vPoint) >= -0.0001f ? 1 : 0];
		}
		while (!(pNode->m_Flags & BSPNODE_LEAFQ458));   // 0x0045af4e
	}

	if (nIndex == 0xffff)                    // 0x0045af53
		return LTNULL;

	return &pWorld->m_pPolys[nIndex];        // 0x0045af6b `lea (%edx,%edx,2)` + shl $4
}


// --------------------------------------------------------------------
//  svnew.cpp's CObjWorldQuery (its constructor is server.dll 0x10024c87
//  and is already reconstructed there), WITH THE CONSTRUCTOR MOVED
//  IN-CLASS.  That move is forced, not cosmetic: server.dll CALLS the
//  constructor from 0x10024c2e and lithtech.exe has no standalone copy
//  at all -- the seven stores are open-coded at 0x0045aeb2..0x0045aed6.
//  /Ob1 never expands a non-template function declared OUT of class, so
//  the shipped source has it in-class, and /Os is what declines it on
//  the server side (which is why svnew.cpp's out-of-line spelling still
//  produces the /O1 COMDAT).
//
//  The three defaults are svnew.cpp's ow_Ignore / ow_IgnoreToo /
//  ow_Accept: 0x00491b40 twice (an empty `ret`, shared) and 0x00432030
//  (`push $1; pop eax; ret`).  Every one of the seven is overwritten by
//  the caller below, which is why the field TYPES are svnew.cpp's
//  reading and not this one's.
// --------------------------------------------------------------------

// Declared under svnew.cpp's own names (server.dll 0x10024cac twice and
// 0x10024cad), so the link reaches its definitions.
void ow_Ignore(void* pUser, void* pObject);          // 0x00491b40
void ow_IgnoreToo(void* pUser, void* pObject);       // 0x00491b40 (folded)
int  ow_Accept(void* pUser, void* pObject);          // 0x00432030

class CObjWorldQueryQ458
{
public:
	// 0x0045aeb2..0x0045aed6 -- declaration order, one register carrying
	// the zero.  svnew.cpp's note applies unchanged.
	CObjWorldQueryQ458()
	{
		m_Unknown00 = LTNULL;
		m_Unknown04 = LTNULL;
		m_pFn08 = ow_Ignore;
		m_pFn0c = ow_IgnoreToo;
		m_Unknown10 = 0;
		m_pFn14 = ow_Accept;
		m_Unknown18 = LTNULL;
	}

	// svnew.cpp types the first two and +0x18 as uint32; they are typed
	// as void* here only so the assignments below need no cast. A dword
	// store is a dword store -- the codegen is identical either way.
	void*  m_Unknown00;                      // +0x00
	void*  m_Unknown04;                      // +0x04
	void (*m_pFn08)(void*, void*);           // +0x08
	void (*m_pFn0c)(void*, void*);           // +0x0c
	uint32 m_Unknown10;                      // +0x10
	int  (*m_pFn14)(void*, void*);           // +0x14
	void*  m_Unknown18;                      // +0x18
	// Never written by the constructor; 0x0045afa2 is its only store.
	uint32 m_Unknown1c;                      // +0x1c
};

// fntdraw.cpp's visibility traversal, 0x0048b8e0 (server.dll 0x10038e77),
// NOT RECONSTRUCTED there; fntdraw.cpp:1579 already records that the
// query record above is its own argument.  Cdecl, one pointer
// (0x0045afae `push %eax` / 0x0045afb4 `add $0x4,%esp`).
void wt_VisibilityTraversalQ458(CObjWorldQueryQ458* pQuery);   // 0x0048b8e0


// --------------------------------------------------------------------
//  The object itself, cut to what the two bodies touch.  objtypes.h's
//  CObjWorldModel is the real class; this view deliberately carries NO
//  virtuals, so no vftable COMDAT is emitted for it -- neither body ever
//  reads its own vptr, and a member's offset is all the codegen sees.
//  IsWorldBusy is DECLARED AND NOT DEFINED on purpose: objtypes.cpp owns
//  that body (server.dll 0x10024e7f, lithtech.exe 0x0045b150, 24 bytes,
//  already reconstructed) and 0x0045b051 is a call to it, not an
//  expansion.
// --------------------------------------------------------------------

class CObjWorldModelQ458
{
public:
	// slot 1 -- worldtree2.cpp's CWTObj::WantsTree, objtypes.h's _v04.
	LTBOOL WantsTreeQ458(CWorldTreeQ458* pTree);        // 0x0045aff0 / 0x10024d6e
	// slot 7 -- worldtree2.cpp's CWTObj::OnFoundInBox, objtypes.h's _v1c.
	void   OnFoundInBoxQ458(WTFindInfoQ458* pInfo);     // 0x0045aea0 / 0x10024c21

	LTBOOL IsWorldBusyQ458();                           // 0x0045b150 (objtypes.cpp)

	void*          m_pVtbl;                  // 0x000
	// worldtree2.cpp's CWTObjN48C::m_Links[WT_NUMLINKS]; objtypes.h has
	// the same run as CObjBase::m_pad_04.  Raw bytes rather than an array
	// of a class with a constructor, which is objtypes.h's own idiom for
	// CObjWorldModel::m_Links1b4 and keeps a `??_H` out of the object.
	uint8          m_Links[0x054 - 0x004];   // 0x004
	uint8          m_pad_054[0x1a8 - 0x054];
	IWorldBspQ458* m_pWorldBsp;              // 0x1a8
};


// =====================================================================
//  SECTION C -- 0x0045aff0, 212 bytes (server.dll 0x10024d6e, 165).
//
//  The world model's hookup into the tree.  Two literal flag cases, then
//  the path-resolved terrain node, then the complaint.
//
//  THREE THINGS READ OFF THE BYTES RATHER THAN GUESSED:
//
//   * m_pWorldBsp is RE-READ from the object at every one of its four
//     later uses (0x0045b025, 0x0045b05a, 0x0045b069, 0x0045b095) and
//     only the null test and the first GetWorldFlags share one load
//     (0x0045aff3).  So it is the member named at each use and not a
//     local -- the repeated-field-load rule.
//
//   * the busy test is ONE `if` with `&&`, not two nested ones:
//     0x0045b056 `test %eax,%eax; je 0x45b0be` and 0x0045b064
//     `cmp $0x1,%eax; jne 0x45b0be` both go to the SAME LTFALSE, which
//     is what a short-circuit gives.  It is redundant source --
//     IsWorldBusy is itself `m_pWorldBsp && IsBusy() == 1` -- and it is
//     written out anyway because both images emit both tests.
//
//   * the name is a `?:` on a pointer the enclosing guard has already
//     proved non-null (0x0045b09b), and `lea 0x4(%esi),%eax` is computed
//     BEFORE the test, which is the `?:` form and not an `if`.
//
//  /O1 cross-jumps the first two AddObjectLink sites into one tail
//  (server.dll 0x10024d95, reached by two `jmp`s) and /O2 duplicates
//  them -- which is exactly the tail-merge asymmetry, and is why
//  svnew.cpp called the cross-jump "the whole difficulty" for the /O1
//  row.  For /O2 the plain spelling below is the one that matches.
// =====================================================================

LTBOOL CObjWorldModelQ458::WantsTreeQ458(CWorldTreeQ458* pTree)
{
	CWTNodeQ458* pNode;
	uint8*       pPath;

	if (m_pWorldBsp)                                             // 0x0045aff9
	{
		if (m_pWorldBsp->GetWorldFlags() & 0x10)                 // 0x0045b006
		{
			pTree->m_RootNode.AddObjectLinkQ458(
				(CWTLinkQ458*)m_Links, 0);                       // 0x0045b017
			return LTTRUE;
		}

		if (m_pWorldBsp->GetWorldFlags() & 0x20)                 // 0x0045b030
		{
			pTree->m_RootNode.AddObjectLinkQ458(
				(CWTLinkQ458*)m_Links, 2);                       // 0x0045b041
			return LTTRUE;
		}

		if (IsWorldBusyQ458() && m_pWorldBsp->IsBusy() == 1)     // 0x0045b051 / 0x0045b064
		{
			// The argument is HOISTED, and that is the whole two-byte
			// difference. Shipped computes `&m_pWorldBsp->m_NodePath`
			// first (0x0045b069 `mov 0x1a8(%esi),%eax`, then `add
			// $0x48,%eax`) and only then loads pTree at 0x0045b06f;
			// written as a single expression VC6 loads pTree first and
			// the eax/edx roles swap right through the call.
			pPath = m_pWorldBsp->m_NodePath;                     // 0x0045b069
			pNode = pTree->FindNodeFromPathQ458(pPath);          // 0x0045b079

			if (pNode)                                           // 0x0045b07b
			{
				pNode->AddObjectLinkQ458(
					(CWTLinkQ458*)m_Links, 0);                   // 0x0045b087
				return LTTRUE;
			}

			dsi_ServerError(
				"ERROR!!  Terrain section '%s' has invalid m_NodePath.",
				m_pWorldBsp ? (char*)m_pWorldBsp->m_Name
				            : "---NONAME---");                   // 0x0045b0ad

			return LTTRUE;
		}
	}

	return LTFALSE;                                              // 0x0045b0be
}


// =====================================================================
//  SECTION D -- 0x0045aea0, 288 bytes (server.dll 0x10024c21, 102).
//
//  The world model's answer to a box query: build the traversal record
//  on the frame, seed it with the polygon the query's centre falls in
//  and with the model's own BSP, copy the query's five parameters plus
//  the tree's current code across, and hand it to the visibility walk.
//
//  The 186-byte difference between the two images is entirely two
//  expansions /Ot takes and /Os declines: the record's constructor
//  (0x10024c2e is a call; 0x0045aeb2 is seven open-coded stores) and
//  w_FindPolyAtPoint (0x10024c41 is a call; 0x0045aeda..0x0045af71 is
//  the whole body).  Nothing else differs -- every field offset, the
//  argument of the poly walk and the six sources copied out of the query
//  agree between them instruction for instruction.
//
//  THE ONE COIN FLIP, AND IT IS NOT SETTLED.  The record's fields are
//  assigned in an order that is NOT declaration order in either image,
//  and the two images do not agree with each other either:
//
//     /O1 stores  00 08 18 0c 04 10 1c 14   loads  14 1c 20 18 [44] 24
//     /O2 stores  00 08 04 18 10 0c 1c 14   loads  14 1c 18 20 [44] 24
//
//  What they DO agree on, and what therefore is source, is that the
//  +0x14 store comes LAST -- after the call that fills +0x1c.  The order
//  written below is /O1's load order, which is the less-scheduled of the
//  two; /O2's differs only in whether +0x18 or +0x20 is read first, so
//  the alternative spelling swaps those two statements.  If this comes
//  back two or four bytes off, that swap is the first thing to try.
// =====================================================================

void CObjWorldModelQ458::OnFoundInBoxQ458(WTFindInfoQ458* pInfo)
{
	CObjWorldQueryQ458 query;                                    // 0x0045aeb2
	IWorldBspQ458*     pWorldBsp;

	// ONE READ on /O1, and it is a local only for the STORE.  server.dll
	// loads m_pWorldBsp once (0x10024c33 `mov 0x1a8(%esi),%edi`), throws
	// `this` away (esi becomes pInfo at 0x10024c39) and stores %edi into
	// +0x04 after the call (0x10024c63); the member written twice reloads
	// it through a `lea` across the call.  The ARGUMENT stays the member:
	// passing the local as well (or an inline helper, a chained
	// assignment, a void*/register local) matches /O1 but flips the
	// inlined DistTo's y/z terms at lithtech 0x0045af13..0x0045af24.
	pWorldBsp = m_pWorldBsp;
	query.m_Unknown00 = w_FindPolyAtPointQ458(
		(CBspWorldViewQ458*)m_pWorldBsp, &pInfo->m_vCenter);     // 0x0045af80
	query.m_Unknown04 = pWorldBsp;                               // 0x0045af8e
	query.m_pFn08     = (void (*)(void*, void*))pInfo->m_pfnCallback;  // 0x0045af79
	query.m_Unknown18 = pInfo->m_pUserData;                      // 0x0045af92
	query.m_pFn0c     = (void (*)(void*, void*))pInfo->m_n020;   // 0x0045af96
	query.m_Unknown10 = pInfo->m_n018;                           // 0x0045af87
	query.m_Unknown1c = pInfo->m_pCodeHolder->CodeSourceSlot1Q458();   // 0x0045afa2
	query.m_pFn14     = (int (*)(void*, void*))pInfo->m_n024;    // 0x0045afaa

	wt_VisibilityTraversalQ458(&query);                          // 0x0045afaf
}


// =====================================================================
//  SECTION E -- 0x0045a160, 91 bytes.  BELONGS IN msgterm.cpp.
//
//  The net bank's record torn down IN ITS DELETING FORM.  This is
//  msgterm.cpp's 0x00455bc0 verbatim, shifted by the 0x0c the record
//  puts in front of the message and with a delete flag on the end:
//
//     0x00455bc0   [esi+0x50]=0x004a1ab8  free  [esi+0x54/58/5c]=0  [esi]=0x004a2b44
//     0x0045a160   [esi+0x5c]=0x004a1ab8  free  [esi+0x60/64/68]=0  [esi+0x0c]=0x004a2b44
//                  then `test 0x8(%esp),$1` / operator delete / return this
//
//  same instructions, same order, same two vtables (0x004a1ab8 the
//  payload collection's, 0x004a2b44 CAbstractIO's), same expansion depth
//  -- BaseDelete open-coded into the `call 0x004152b0` at 0x0045a180,
//  which is one level DEEPER than 0x00455bc0's `call 0x0040bfe0`.  The
//  record shape is netmgr.cpp's CNetSentPacketView and servermsg.cpp's
//  CNetMsgRecord: three words then a whole ServerMessage at +0x0c.
//
//  WHAT CALLS IT, AND WHY IT EXISTS AT ALL.  Exactly one site:
//  0x00455226, inside NetConnection::Term (0x004551c0, netmgr.cpp's
//  0x1002126a), which at /O2 has netbank_FreeMessages expanded into it
//  and CNetBank::Free expanded inside THAT -- the free-list splice at
//  0x0045522b..0x00455230 is Free's own body.  Two levels, which
//  netmgr.cpp:547 already records this module doing.  At the third level
//  VC6 stops and emits a CALL to the record's teardown, where the
//  STANDALONE CNetBank::Free (0x00459dc0) carries the same teardown
//  inline.  server.dll has no counterpart: its Free calls the plain,
//  non-deleting destructor at 0x100242c1, which servermsg.cpp already
//  matches and which this does not touch.
//
//  HONEST CONFIDENCE: this is the weakest of the five.  What is certain
//  is the BODY -- it is a byte-for-byte analogue of a row already
//  written.  What is inferred is the C++ SPELLING of the deleting
//  wrapper.  VC6 emits `??_G` only for a class with a virtual
//  destructor, and the record stores NO vptr of its own at +0x00, so the
//  real source is most likely a two-base class whose polymorphic base
//  (the message) lands at +0x0c.  Modelling that needs servermsg.h's
//  ServerMessage to gain a real destructor, which servermsg.cpp
//  deliberately declines for six other translation units' sake.  The
//  wrapper below is the same machine code written without the
//  inheritance: an explicit destructor call, the flag test, `operator
//  delete`, and `return this`.  If it comes back near, the difference
//  will be in where the flag byte is loaded (0x0045a196 hoists it into
//  %al ahead of the two remaining stores; server.dll's 0x1002420c
//  instead spells it `testb $0x1,0x8(%esp)` in place) and the answer is
//  to give the record the inheritance and let VC6 emit the real `??_G`.
// =====================================================================

// 0x004a2b44 -- CAbstractIO's vftable (its own slot 0 is 0x00455c30, the
// four-slot table {0x00455c30, 0x00455c00, 0x00455c10, 0x00455c20}).
// Stored as a plain address, for the reason servermsg.cpp gives.
extern void* g_AbstractIOVtblQ458;                   // 0x004a2b44

class CTermAbstractIOQ458
{
public:
	~CTermAbstractIOQ458() { m_pVtbl = &g_AbstractIOVtblQ458; }

	void*  m_pVtbl;                                  // +0x00
	uint32 m_nRefCount;                              // +0x04
};

// msgterm.cpp's CTermMessageQ454, with the destructor IN-CLASS.  That is
// the one deliberate difference: msgterm.cpp defines it out of line so
// the 0x00455bc0 COMDAT is emitted at all, and here it has to be
// expanded into the wrapper below, which /Ob1 will only do for an
// in-class definition.  Declaration order is destruction order reversed:
// the shipped bytes destroy +0x50 first and +0x00 last.
class CTermMessageQ458
{
public:
	~CTermMessageQ458() {}

	CTermAbstractIOQ458           m_Base;            // +0x00
	uint8                         m_pad_08[0x50 - 0x08];
	CMoArray<uint8, DefaultCache> m_PacketData;      // +0x50
};

// netmgr.cpp's CNetSentPacketView / servermsg.cpp's CNetMsgRecord: the
// bank's free-list next, two more words, then the message.  0x0c + 0x64
// = 0x70, which is the bank's element size exactly.
class CNetMsgRecordQ458
{
public:
	~CNetMsgRecordQ458() {}

	// The scalar deleting destructor by hand -- see the note above.
	// __thiscall, one uint argument, returns `this`: 0x0045a1b5
	// `mov %esi,%eax` then `ret $0x4`.
	void* DeleteQ458(unsigned int nFlags);           // 0x0045a160

	uint32           m_Unknown00;                    // +0x00
	uint32           m_Unknown04;                    // +0x04
	uint32           m_Unknown08;                    // +0x08
	CTermMessageQ458 m_Msg;                          // +0x0c
};

void* CNetMsgRecordQ458::DeleteQ458(unsigned int nFlags)
{
	this->~CNetMsgRecordQ458();                      // 0x0045a163..0x0045a1a9

	if (nFlags & 1)                                  // 0x0045a1a1
		::operator delete(this);                     // 0x0045a1ad

	return this;                                     // 0x0045a1b5
}
