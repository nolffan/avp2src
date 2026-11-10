// *********************************************************************** //
//
//  MODULE:   VISQUERY (.CPP)   (file name ours) -- lithtech.exe only
//
//  The client's potentially-visible-set walk (Primal Hunt 0x004a8900 ..
//  0x004a91de; AvP2 1.0.9.6 0x0048b8e0 .. 0x0048c19e), split out of
//  fntdraw.cpp: no call edge to the font or video code, its own .bss
//  (0x004b4d70 ..), and in Primal Hunt its own <string> guard pair after
//  the run (0x004a9200, 0x004a9220).
//
// *********************************************************************** //

#if defined(PRIMAL_HUNT) && !defined(LT_SERVER_BUILD)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x4a9200).
#include <string>
#endif
#include <windows.h>
#include <string.h>

#include "ltbasedefs.h"
#include "ltcodes.h"
#include "ltrect.h"

// binkvideo.cpp's LTLink, renamed (videomgr.cpp has the same).
struct LTLinkQ488
{
	LTLinkQ488* m_pPrev;                     // 0x00
	LTLinkQ488* m_pNext;                     // 0x04
	void*       m_pData;                     // 0x08
};



// ======================================================================= //
//  THE VISIBILITY TRAVERSAL -- 0x0048b8e0, 0x0048bca0, 0x0048bd70,
//  0x0048be40.  NOT video code.
//
//  0x0048b8e0 opens `mov $0x4e30,%eax ; call 0x00492c60` (__chkstk) and
//  then publishes the TOP of that 20,016-byte frame as a global:
//
//      lea 0x4e34(%esp),%eax ; mov %eax,0x004b4d70
//
//  so 0x004b4d70 is the scratch buffer's limit and everything below
//  bounds-checks against it.  Its argument is stored to 0x004b4d74 and is a
//  query object with a world at +0x04, three callbacks at +0x0c, +0x10 and
//  +0x14 (the +0x10 one DEFAULTED to 0x0048bc50 when the caller left it
//  null), and user data at +0x18.  0x004b4d78/0x004b4d7c are a bit index
//  and a bit array -- `mov $1,%al ; and $7,%cl ; shl %cl,%al ;
//  and (%ecx,%edi,1),%al ; inc <index>` is a sequential PVS bit read, which
//  is what makes this the client's potentially-visible-set walk.
//
//  Its two callers are 0x00426b55 and 0x0045afaf, neither in this region.
//
//  0x0048be40 is the per-node recursion (it calls itself at 0x0048c077 and
//  is called from 0x0048bb0c), and the two filters below are its object-list
//  splitters.  Both are written below (see vis_TraversalQ488 and
//  vis_WalkNodeQ488).
// ======================================================================= //

#include "ltplane.h"

// The walked object.  +0x18c and +0x1a4 are src/engine/server's LTObject
// m_Pos and m_Radius, which ltobject_internal.h and clientmgr.h both carry;
// +0x58 is ltobject_internal.h's m_Unknown58.
class CVisObjectQ488
{
public:
	uint8    m_pad_00[0x58];
	uint32   m_Unknown58;                    // 0x058
	uint8    m_pad_5c[0x18c - 0x5c];
	// AVP2: as Primal Hunt (PH addresses) -- the object is eight bytes longer below here (0x004a8e0b reads
	// m_Pos at +0x194, 0x004a8e3e the radius at +0x1ac).
	uint8 m_padExtra[8];
	LTVector m_Pos;                          // 0x18c
	uint8    m_pad_198[0x1a4 - 0x198];
	float    m_Radius;                       // 0x1a4
};

// ---- the traversal's world, node and tree (0x0048b8e0 / 0x0048be40) ---- //
// Layouts as src/engine/server/worldtree.cpp's sections C and D read them
// off server.dll's twins 0x10038e77 / 0x10038ff1; every offset below is also
// a load in 0x0048b8e0 or 0x0048be40.

// An object-list head: the 8-byte half of a link (the SDK's CheapLTLink);
// the links on it are LTLinkQ488s, and vis_CollectListQ488 takes the head
// as one.
struct VisListHeadQ488
{
	LTLinkQ488* m_pPrev;                     // 0x00
	LTLinkQ488* m_pNext;                     // 0x04
};

// A leaf record, 0x30 bytes (`lea (%ecx,%ecx,2); shl $4`).
struct VisRecordQ488
{
	uint8 m_pad[0x30];
};

// A chunk, 0x24 bytes (`lea (%eax,%eax,8); lea (%ecx,%edx,4)`).
struct VisChunkQ488
{
	uint8 m_pad_00[0x06];
	uint8 m_Flags;                           // 0x06  bit 0
	uint8 m_pad_07[0x24 - 0x07];
};

// AVP2: AvP2's and Primal Hunt's node polygon carries its plane itself at
// +0x28, and the walk only takes it when the polygon and the pointer at
// +0x2c are both there, NULL otherwise (PH 0x004a8fea..0x004a8ffc and
// 0x004a910d..0x004a9137, AvP2 0x0049ea37..; NOLF read +0x28 as a surface
// and its plane at +0x34, unconditionally).
struct VisPolyQ488
{
	uint8    m_pad_00[0x28];
	LTPlane* m_pPlane;                       // 0x28
	void*    m_Unknown2c;                    // 0x2c
};

struct VisNodeQ488
{
	VisPolyQ488* m_pPoly;                    // 0x00
	VisNodeQ488* m_Sides[2];                 // 0x04
	VisListHeadQ488 m_Objects;               // 0x0c  the node's object list
	uint16       m_nLeaf;                    // 0x14  0xffff: none
	uint8        m_Flags;                    // 0x16  bit 3: descend

	LTPlane* GetPlane()
	{
		if (m_pPoly && m_pPoly->m_Unknown2c)
			return m_pPoly->m_pPlane;

		return NULL;
	}
};

class CVisRecordArrayQ488
{
public:
	uint32         m_Unknown00;              // 0x00
	VisRecordQ488* m_pArray;                 // 0x04
	uint32         m_nElements;              // 0x08

	inline VisRecordQ488* GetRecord(uint32 i)
	{
		return i < m_nElements ? &m_pArray[i] : LTNULL;
	}
};

// A world leaf, 0x18 bytes, at world +0x70/+0x74 (0x0048bbaf, `add $0x18`).
struct VisLeafEntryQ488
{
	uint8           m_pad_00[0x0c];
	VisListHeadQ488 m_Objects;               // 0x0c
	uint16     m_iRecord;                    // 0x14  0xffff = none
	uint8      m_pad_16[0x18 - 0x16];
};

class CVisWorldQ488
{
public:
	uint8               m_pad_00[0x70];
	VisLeafEntryQ488*   m_pLeaves;           // 0x70
	uint32              m_nLeaves;           // 0x74
	uint8               m_pad_78[0x84 - 0x78];
	CVisRecordArrayQ488 m_Records;           // 0x84
	uint8               m_pad_90[0x9c - 0x90];
	VisNodeQ488*        m_pRootNode;         // 0x9c
	uint8               m_pad_a0[0xb0 - 0xa0];
	VisChunkQ488*       m_pChunks;           // 0xb0

	VisChunkQ488* GetChunk(uint32 i) { return &m_pChunks[i]; }

	// The walk's lookup (0x0048be9c..0x0048bec3): straight off the world,
	// the array base loaded into its own register before the add.  The index
	// is a const reference to the node's word: by value Primal Hunt's cl 8966
	// loads the base second (PH 0x004a8ef5, 0x004a8a1f).
	VisRecordQ488* GetRecord(const uint16& i)
	{
		if (i < m_Records.m_nElements)
			return &m_Records.m_pArray[i];
		return LTNULL;
	}
};

struct VisTreeEntryQ488
{
	uint16 m_iChunk;                         // 0x00
	uint16 m_pad_02;
	uint8* m_pBits;                          // 0x04  the entry's PVS bits
};

struct VisTreeQ488
{
	uint8             m_pad_00[0x10];
	VisTreeEntryQ488* m_pEntries;            // 0x10
	uint8             m_pad_14[0x2e - 0x14];
	uint16            m_nEntries;            // 0x2e

	VisTreeEntryQ488* GetEntry(uint32 i) { return &m_pEntries[i]; }
};

typedef void (*VisObjFnQ488)(CVisObjectQ488* pObj, void* pUser);
typedef void (*VisRecordFnQ488)(VisRecordQ488* pRecord, void* pUser);
typedef void (*VisCollectFnQ488)(LTLinkQ488* pHead, CVisObjectQ488*** ppArray,
                                 uint32* pnCount);
typedef int  (*VisFilterFnQ488)(VisChunkQ488* pChunk);

class CVisQueryQ488
{
public:
	VisTreeQ488*     m_pTree;                // 0x00
	CVisWorldQ488*   m_pWorld;               // 0x04
	VisObjFnQ488     m_ObjFn;                // 0x08
	VisRecordFnQ488  m_RecordFn;             // 0x0c
	VisCollectFnQ488 m_CollectFn;            // 0x10
	VisFilterFnQ488  m_FilterFn;             // 0x14
	void*            m_pUser;                // 0x18
	uint32           m_Unknown1c;            // 0x01c  the visit mark

	// STAND-IN (name and existence ours): the leaves path's record report
	// written as an inline helper.  It is here for its INLINE COST: without
	// it 0x0048b8e0's /O2 budget still covers vis_FilterNegQ488 (1053 bytes,
	// the filter expanded and the third bit read called); with it both
	// filters are called and every bit read expanded, as shipped (880).
	// Which inline the original spent that budget on is unknown.
	void ReportRecord(VisRecordQ488* pRecord)
	{
		if (pRecord)
			m_RecordFn(pRecord, m_pUser);
	}
};

// The traversal's state, defined here: nothing else in the image touches
// these addresses (all .bss, so no initialisers).
CVisObjectQ488** g_pVisLimitQ488;     // 0x004b4d70
CVisQueryQ488*   g_pVisQueryQ488;     // 0x004b4d74
uint32           g_nVisBitQ488;       // 0x004b4d78
uint8*           g_pVisBitsQ488;      // 0x004b4d7c
VisRecordQ488*   g_pVisRecordQ488;    // 0x004b5220
CVisObjectQ488** g_pVisCursorQ488;    // 0x004b5224
uint32           g_nVisFilteredQ488;  // 0x004b5228
CVisObjectQ488** g_pVisFilteredQ488;  // 0x004b522c

#define VIS_BUFFER_SIZE_Q488  5000           // 0x4e30 - 0x10 = 5000 * 4

void vis_CollectListQ488(LTLinkQ488* pHead, CVisObjectQ488*** ppArray,
                         uint32* pnCount);
CVisObjectQ488** vis_FilterNegQ488(LTPlane* pPlane, CVisObjectQ488** pIn,
                                   uint32 nIn, uint32* pnOut, LTBOOL bAppend);
CVisObjectQ488** vis_FilterPosQ488(LTPlane* pPlane, CVisObjectQ488** pIn,
                                   uint32 nIn, uint32* pnOut);
void vis_WalkNodeQ488(VisNodeQ488* pNode, CVisObjectQ488** pArray,
                      uint32 nCount);

// The next bit of the PVS stream -- server.dll's 0x10039151, expanded at
// every use here (0x0048b9bc, 0x0048baaa, 0x0048bb18, 0x0048be70, ...).
inline uint8 vis_ReadBitQ488()
{
	uint8 bBit = (uint8)((1 << (g_nVisBitQ488 & 7)) & g_pVisBitsQ488[g_nVisBitQ488 >> 3]);
	g_nVisBitQ488++;
	return bBit;
}


// ----------------------------------------------------------------------- //
//  0x0048b8e0, 880 bytes -- the entry point (server.dll 0x10038e77,
//  src/engine/server/worldtree.cpp's wt_VisibilityTraversal).  The one
//  difference from the server is that the per-node walk 0x0048be40 is
//  EXPANDED here for the root call (0x0048b996..0x0048bb7c: the walk's
//  loop, with its two plane filters called out of line at 0x0048baef /
//  0x0048bb58 and its recursion calling 0x0048be40 at 0x0048bb0c).
//  Callers 0x00426b55 and 0x0045afaf (netobj.cpp's
//  wt_VisibilityTraversalQ458 is this function).
// ----------------------------------------------------------------------- //
void vis_TraversalQ488(CVisQueryQ488* pQuery)
{
	CVisObjectQ488*  buffer[VIS_BUFFER_SIZE_Q488];
	CVisObjectQ488** pObjects;
	uint32           i, j, nObjects;

	g_pVisLimitQ488 = &buffer[VIS_BUFFER_SIZE_Q488];

	if (!pQuery->m_CollectFn)
		pQuery->m_CollectFn = vis_CollectListQ488;

	g_pVisQueryQ488 = pQuery;

	if (pQuery->m_pTree && pQuery->m_pTree->m_nEntries > 0)
	{
		for (i = 0; i < pQuery->m_pTree->m_nEntries; i++)
		{
			VisTreeEntryQ488* pEntry = pQuery->m_pTree->GetEntry(i);

			if (i != 0)
			{
				VisChunkQ488* pChunk = pQuery->m_pWorld->GetChunk(pEntry->m_iChunk);

				if (!(pChunk->m_Flags & 1))
					continue;

				if (!pQuery->m_FilterFn(pChunk))
					continue;
			}

			g_pVisBitsQ488 = pEntry->m_pBits;
			g_nVisBitQ488 = 0;
			vis_WalkNodeQ488(pQuery->m_pWorld->m_pRootNode, buffer, 0);
		}
	}
	else
	{
		for (i = 0; i < pQuery->m_pWorld->m_nLeaves; i++)
		{
			VisLeafEntryQ488* pLeaf = &pQuery->m_pWorld->m_pLeaves[i];

			pObjects = buffer;
			nObjects = 0;

			if (pLeaf->m_iRecord != 0xffff)
			{
				pQuery->ReportRecord(pQuery->m_pWorld->m_Records.GetRecord(pLeaf->m_iRecord));
			}

			pQuery->m_CollectFn((LTLinkQ488*)&pLeaf->m_Objects, &pObjects, &nObjects);

			for (j = 0; (int)j < (int)nObjects; j++)
				pQuery->m_ObjFn(pObjects[j], pQuery->m_pUser);
		}
	}
}


// ----------------------------------------------------------------------- //
//  0x0048bc50, 76 bytes -- the default +0x10 callback.  Not in the enumeration;
//  free neighbour, and the thing 0x0048b8e0 installs at 0x0048b908 when the
//  caller left the slot null, so its three arguments are exactly the three
//  0x0048bc0a pushes: a list head, the ADDRESS of the output cursor, and the
//  address of the count.
//
//  The array base is re-read from *ppArray on every iteration (0x0048bc66)
//  and the limit and query globals with it, which is what says the source
//  writes the double dereference rather than caching it.
//
//  The walk follows +0x00, the PREV link, and stops on the head -- the same
//  circular list binkvideo.cpp's LTLink describes.
// ----------------------------------------------------------------------- //

void vis_CollectListQ488(LTLinkQ488* pHead, CVisObjectQ488*** ppArray,
                         uint32* pnCount)
{
	LTLinkQ488* pCur;

	pCur = pHead->m_pPrev;

	while (pCur != pHead)
	{
		CVisObjectQ488* pObj = (CVisObjectQ488*)pCur->m_pData;

		if (*ppArray < g_pVisLimitQ488 &&
		    pObj->m_Unknown58 != g_pVisQueryQ488->m_Unknown1c)
		{
			(*ppArray)[*pnCount] = pObj;
			(*pnCount)++;
		}

		pCur = pCur->m_pPrev;
	}
}


// ----------------------------------------------------------------------- //
//  0x0048bca0, 203 bytes -- keep the objects on ONE side of a plane.
//
//  Five arguments; its only call site, 0x0048baef, pushes them as
//  (plane, array, count, &g_Count, flag) with `add $0x14`.  The fifth
//  decides whether the survivors are written OVER the input or just PAST it
//  (`mov %edi,%esi ; je ; lea (%edi,%eax,4),%esi`).
//
//  `cmp %ebx,%ecx ; jae` on `pOut + nIn` against 0x004b4d70 is an UNSIGNED
//  pointer compare -- the scratch-buffer overflow guard -- and it returns
//  pOut with the count left at zero.
//
//  The distance is LTPlane::DistTo taken BY VALUE: 0x0048bcfb copies the
//  object's three position floats into consecutive frame slots and only
//  then loads them, which is the by-value LTVector parameter
//  shape.  The multiply order is z, y, x -- a flat three-term sum
//  emits its terms backwards -- so the source is m_Normal.Dot(vec), i.e.
//  ltplane.h's DistTo verbatim.
//
//  `fcompp` compares the SECOND-loaded value with the first, so
//  `test $0x41,%ah ; jne <skip>` skips when (radius + 0.1f) <= dist: the
//  kept case is `dist < radius + 0.1f`.  0x004a1e24 is 0x3dcccccd, 0.1f.
//
//  `mov (%edx),%eax` appears TWICE around the store (0x0048bd49 and
//  0x0048bd4e) because the store through pOut may alias the counter, which
//  is what says *pnOut is written directly rather than through a local.
//
//  The loop is `while (nIn--)` over a walking pIn: `mov %eax,%ecx ; dec
//  %eax ; test %ecx,%ecx ; je` at 0x0048bcd6 is the post-decrement test,
//  the count lives in a stack slot decremented at the bottom, and pIn is
//  bumped by `add $4` AFTER the body -- so `pIn++` is the loop's last
//  statement, not `*pIn++` in the load (that puts the add at the top).
// ----------------------------------------------------------------------- //

inline CVisObjectQ488** vis_FilterNegQ488(LTPlane* pPlane, CVisObjectQ488** pIn,
                                   uint32 nIn, uint32* pnOut, LTBOOL bAppend)
{
	CVisObjectQ488** pOut;

	pOut = pIn;

	if (bAppend)
		pOut = pIn + nIn;

	*pnOut = 0;

	if (pOut + nIn >= g_pVisLimitQ488)
		return pOut;

	while (nIn--)
	{
		CVisObjectQ488* pObj = *pIn;

		if (pObj->m_Unknown58 != g_pVisQueryQ488->m_Unknown1c)
		{
			if (pPlane->DistTo(pObj->m_Pos) < pObj->m_Radius + 0.1f)
			{
				pOut[*pnOut] = pObj;
				(*pnOut)++;
			}
		}

		pIn++;
	}

	return pOut;
}


// ----------------------------------------------------------------------- //
//  0x0048bd70, 199 bytes -- the twin, for the other side of the plane.
//
//  FOUR arguments (0x0048bb58 pushes four and cleans 0x10); the survivors
//  always go over the input, so there is no selector and no `lea`.  The
//  overflow guard's early return has its OWN epilogue here (0x0048bd97)
//  because %ebp is not yet pushed, which is why it is written as a guarded
//  return rather than a jump to the end.
//
//  The only other difference is `fchs` at 0x0048be0a and `test $0x1,%ah ;
//  je` in place of `test $0x41,%ah ; jne` -- the negated bound and the
//  strictly-greater sense, i.e. `dist > -(radius + 0.1f)`.  `fchs` AFTER
//  the add says the source negates the whole sum, not the radius.  Same
//  `while (nIn--)` loop with `pIn++` last (0x0048bda0, 0x0048be23).
// ----------------------------------------------------------------------- //

inline CVisObjectQ488** vis_FilterPosQ488(LTPlane* pPlane, CVisObjectQ488** pIn,
                                   uint32 nIn, uint32* pnOut)
{
	CVisObjectQ488** pOut;

	pOut = pIn;

	*pnOut = 0;

	if (pOut + nIn >= g_pVisLimitQ488)
		return pOut;

	while (nIn--)
	{
		CVisObjectQ488* pObj = *pIn;

		if (pObj->m_Unknown58 != g_pVisQueryQ488->m_Unknown1c)
		{
			if (pPlane->DistTo(pObj->m_Pos) > -(pObj->m_Radius + 0.1f))
			{
				pOut[*pnOut] = pObj;
				(*pnOut)++;
			}
		}

		pIn++;
	}

	return pOut;
}


// ----------------------------------------------------------------------- //
//  0x0048be40, 862 bytes -- one node of the walk (server.dll 0x10038ff1,
//  worldtree.cpp's vis_WalkNode_D).  Recursive (0x0048c077) and tail-
//  looping (0x0048c179 / 0x0048c191 rewrite the argument slots and `jmp
//  0x0048be47`).  Both plane filters are EXPANDED here (0x0048bfde..,
//  0x0048c0f2.., `fchs` included), and so is the bit read.
// ----------------------------------------------------------------------- //
inline void vis_WalkNodeQ488(VisNodeQ488* pNode, CVisObjectQ488** pArray,
                             uint32 nCount)
{
	uint32 bBack;

	for (;;)
	{
		g_pVisQueryQ488->m_CollectFn((LTLinkQ488*)&pNode->m_Objects, &pArray, &nCount);

		if (pNode->m_nLeaf != 0xffff && vis_ReadBitQ488())
		{
			g_pVisRecordQ488 = g_pVisQueryQ488->m_pWorld->GetRecord(pNode->m_nLeaf);
			if (g_pVisRecordQ488)
				g_pVisQueryQ488->m_RecordFn(g_pVisRecordQ488, g_pVisQueryQ488->m_pUser);

			g_pVisCursorQ488 = pArray;
			while (nCount--)
			{
				if ((*g_pVisCursorQ488)->m_Unknown58 != g_pVisQueryQ488->m_Unknown1c)
				{
					(*g_pVisCursorQ488)->m_Unknown58 = g_pVisQueryQ488->m_Unknown1c;
					g_pVisQueryQ488->m_ObjFn(*g_pVisCursorQ488, g_pVisQueryQ488->m_pUser);
				}

				g_pVisCursorQ488++;
			}

			nCount = 0;
		}

		bBack = pNode->m_Sides[1]->m_Flags & 8;

		if ((pNode->m_Sides[0]->m_Flags & 8) && vis_ReadBitQ488())
		{
			g_pVisFilteredQ488 = vis_FilterNegQ488(pNode->GetPlane(), pArray, nCount,
			                                       &g_nVisFilteredQ488, bBack);
			if (bBack)
			{
				vis_WalkNodeQ488(pNode->m_Sides[0], g_pVisFilteredQ488, g_nVisFilteredQ488);
			}
			else
			{
				pArray = g_pVisFilteredQ488;
				nCount = g_nVisFilteredQ488;
				pNode  = pNode->m_Sides[0];
				continue;
			}
		}

		if (bBack && vis_ReadBitQ488())
		{
			pArray = vis_FilterPosQ488(pNode->GetPlane(), pArray, nCount, &nCount);
			pNode  = pNode->m_Sides[1];
			continue;
		}

		return;
	}
}

// ======================================================================= //
//  0x0048b8e0 (880) and 0x0048be40 (862) -- NOW WRITTEN above
//  (vis_TraversalQ488 / vis_WalkNodeQ488), both byte-identical in Primal
//  Hunt (0x004a8900, 0x004a8e80) and AvP2 1.0.9.6.  The record lookup was
//  the last residual: the world's GetRecord takes its index by const
//  reference (the walk's `xor; mov %ax` and base-first scaling at
//  0x004a8ee4..0x004a8f01), and the leaf loop asks the record array itself
//  (the `and $0xffff` on the value already in %cx at 0x004a8be2).  By value,
//  as uint32 or uint16, cl 8966 scales the index into a fresh register and
//  adds it to the base instead.  The notes below are the evidence as first
//  gathered.
//
//  BOTH ARE SINGLE FUNCTIONS.  Both extents are clean: no branch
//  target leaves either range and the last instruction ends exactly on the
//  boundary.  0x0048b8e0's two `ret`s are the two exits of its two
//  alternative top-level loops (0x0048bb9c and 0x0048bc4f); 0x0048be40's
//  one `ret` is at 0x0048c19d.  0x0048be40 is recursive (0x0048c077 calls
//  itself) AND self-tail-looping: 0x0048c179 and 0x0048c191 rewrite its own
//  argument slots and `jmp 0x0048be47`, three bytes past the entry, which is
//  VC6 turning the tail call into a loop.
//
//  WHY THEY ARE NOT DONE HERE.  The two plane filters above are CALLED out
//  of line by 0x0048b8e0 (0x0048baef, 0x0048bb58) and EXPANDED INLINE by
//  0x0048be40 (0x0048bfde..0x0048c059 is vis_FilterNeg, 0x0048c0f2..
//  0x0048c166 is vis_FilterPos, `fchs` included).  The rule --
//  "the inline decision is CALLER-CONTEXT-DEPENDENT, not a property of the
//  callee" -- says no keyword in one header separates those two sites, so
//  whoever writes 0x0048be40 has to write the filter bodies out by hand
//  inside it and leave the standalone pair alone.  That is a source-shape
//  decision this file deliberately does not pre-empt: the two functions
//  above are written as ordinary out-of-line helpers, which is what
//  0x0048b8e0's call sites want and what the presence of the standalone
//  bodies at 0x0048bca0/0x0048bd70 proves the original had.
//
//  THE STATE, all of it read off the bytes:
//
//    0x004b4d70  Obj**    the scratch buffer's limit -- `mov $0x4e30,%eax ;
//                         call 0x00492c60` (__chkstk) then
//                         `lea 0x4e34(%esp),%eax ; mov %eax,0x004b4d70`
//    0x004b4d74  Query*   0x0048b8e0's own argument, published at 0x0048b90f
//    0x004b4d78  uint32   a bit INDEX, incremented once per test
//    0x004b4d7c  uint8*   the bit array those indices walk:
//                           mov $1,%al ; and $7,%cl ; shl %cl,%al ;
//                           and (%ecx,%edi,1),%al ; inc <index>
//                         -- a sequential PVS read, which is what makes this
//                         the client's potentially-visible-set traversal
//    0x004b5220  void*    the current leaf/poly record
//    0x004b5224  Obj**    the current object cursor
//    0x004b5228  uint32   the filtered count (the fourth argument
//                         0x0048baef pushes is literally $0x4b5228)
//    0x004b522c  Obj**    the filtered array
//
//  THE QUERY CLASS, by displacement:
//    +0x00  a tree/root object; null takes the whole function down its
//           SECOND path at 0x0048bb9d
//    +0x04  the world, with +0x70/+0x74 an array of 0x18-byte records and
//           its count, +0x88/+0x8c an array of 0x30-byte records and its
//           count (`lea (%ecx,%ecx,2),%ecx ; shl $0x4,%ecx`), +0x9c a node
//           pointer and +0xb0 an array of 0x24-byte records
//           (`lea (%eax,%eax,8),%edx ; lea (%ecx,%edx,4)`)
//    +0x08  void (*)(Obj*, void* pUser)   -- "this object is visible"
//    +0x0c  void (*)(void* pRecord, void* pUser)
//    +0x10  void (*)(void* pNode, Obj*** ppArray, uint32* pnCount)
//           DEFAULTED to 0x0048bc50 at 0x0048b908 when the caller left it
//           null -- which is how that function's signature is known
//    +0x14  int  (*)(void* pRecord)       -- one argument, cdecl
//    +0x18  void* the user pointer handed to +0x08 and +0x0c
//    +0x1c  uint32 the visit MARK, compared against and written to an
//           object's +0x58 (`if (pObj->m_58 != mark) { pObj->m_58 = mark;
//           query->m_Fn08(pObj, query->m_pUser); }`)
//
//  THE NODE, by displacement: +0x04 and +0x08 the two children, +0x0c the
//  argument handed to the +0x10 callback, +0x14 a uint16 index with 0xffff
//  meaning "none", and each child's +0x16 carrying a flag byte whose bit 3
//  gates that side of the descent.
//
//  Its two callers are 0x00426b55 and 0x0045afaf, neither in this region.
// ======================================================================= //