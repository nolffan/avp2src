// server.dll — LTObject's own engine-internal methods (reconstruction). Build /O1.
#include "ltobject_internal.h"

// The world geometry from the inside, as far as the world-model transform
// reaches into it. worldbsp.h models the same object as CWorldBsp with the
// same offsets; it is repeated here rather than included because that header
// belongs to another module. Every offset is read off wm_Transform and
// obj_UpdateTransform (names ours).
class CWMPart
{
public:
	uint8    m_pad_00[0x18];
	LTVector m_vCentre;                      // 0x18
};

class CWMGeometry
{
public:
	uint8     m_pad_00[0x68];
	LTPlane*  m_pPlanes;                     // 0x68
	uint32    m_nPlanes;                     // 0x6c
	uint8     m_pad_70[0xa0 - 0x70];
	CWMPart** m_ppParts;                     // 0xa0
	uint32    m_nParts;                      // 0xa4
	LTVector* m_pVerts;                      // 0xa8
	uint32    m_nVerts;                      // 0xac
	uint8     m_pad_b0[0xe8 - 0xb0];
	LTVector  m_vOffsete8;                   // 0xe8
};

// The bank the poly-grid records come from. servermgr.h models the same
// object as StructBank, with the spare list at +0x18 and the free written
// out inline at every site; it is repeated here rather than included because
// that header belongs to another module -- the same local-view convention
// packet.cpp uses for the same two declarations.
class StructBank
{
public:
	uint8  m_pad_00[0x08];
	uint32 m_GrowCount;                      // +0x08  sb_Grow's argument
	uint8  m_pad_0c[0x18 - 0x0c];
	void*  m_pFreeList;                      // +0x18
};

// 0x1003e1e0 and 0x10001c86. The allocate is INLINE in the header the engine
// used and the grow is not: server.dll calls 0x10001c86 here (39 bytes is
// over the /Os budget) and lithtech.exe expands it, which is exactly what the
// two images show at this one call site -- one `call` at 0x1000d5ce against
// the free-list walk written out at 0x4293a9. servermgr.h carries the body;
// this file only needs to be able to name it.
void  sb_Grow(StructBank* pBank, uint32 nCount);

inline void* sb_Allocate(StructBank* pBank)
{
	void* pStruct = pBank->m_pFreeList;

	if (!pStruct)
	{
		sb_Grow(pBank, pBank->m_GrowCount);

		pStruct = pBank->m_pFreeList;
		if (!pStruct)
			return LTNULL;
	}

	pBank->m_pFreeList = *(void**)pStruct;

	return pStruct;
}

// 0x100583c8 -- the bank the poly-grid records come from: dsi.cpp's
// memtrack bank, which memtrack_Init sets up for 0x20-byte records.
extern StructBank g_MemTrackBank;

// 0x10024e51. A world model carries its own BSP as well as a pointer to the
// world it was placed in; a polygon belongs to whichever of the two claims it.
HPOLY LTObject::MakeHPoly(WorldPoly* pPoly)
{
	if (m_pOriginalBsp)
	{
		HPOLY hPoly = m_pOriginalBsp->MakeHPoly(pPoly);
		if (hPoly != INVALID_HPOLY)
			return hPoly;
	}

	return m_pWorldBsp->MakeHPoly(pPoly);
}


// ---- what an object is standing on (0x1001e7ef-0x1001e8c7) -------------- //
//
// Every object carries two links for this: a list head for the things
// standing on it, and the link by which it hangs in whatever it is standing
// on. Detaching walks the second; detaching everything walks the first.

void obj_UnlinkTrackersA(LTObject* pObj)
{
	LTLink* pLink;

	if (pObj->m_pStandingOn)
	{
		pLink = &pObj->m_StandingLink;

		pLink->m_pPrev->m_pNext = pLink->m_pNext;
		pLink->m_pNext->m_pPrev = pLink->m_pPrev;
		pLink->TieOff();

		pObj->m_pStandingOn = LTNULL;
	}

	pObj->m_pStandingOnPoly = LTNULL;
}

// Everything standing on this one comes off, and each is marked as moved so
// the clients hear about it.
void obj_UnlinkTrackersB(LTObject* pObj)
{
	LTLink* pCur = pObj->m_StandingOnMe.m_pNext;
	LTLink* pHead = &pObj->m_StandingOnMe;

	while (pCur != pHead)
	{
		LTObject* pOn = (LTObject*)pCur->m_pData;
		LTLink*   pNext = pCur->m_pNext;

		obj_UnlinkTrackersA(pOn);
		pOn->m_ChangeFlags |= CHANGEFLAG_MOTION;

		pCur = pNext;
	}
}

// 0x1001e85d — put an object onto another one, or onto nothing (name ours).
void obj_SetStandingOn(LTObject* pObj, LTObject* pOn, WorldPoly* pPoly)
{
	obj_UnlinkTrackersA(pObj);

	if (pOn)
	{
		LTLink* pLink = &pObj->m_StandingLink;
		LTLink* pHead = &pOn->m_StandingOnMe;

		pLink->m_pPrev = pHead;
		pLink->m_pNext = pHead->m_pNext;
		pLink->m_pNext->m_pPrev = pLink;
		pLink->m_pPrev->m_pNext = pLink;
	}

	pObj->m_pStandingOn = pOn;
	pObj->m_pStandingOnPoly = pPoly;
}

// The world model's transform is rebuilt from the object's position and
// rotation; whether it also moves its own geometry is one flag bit.
void obj_RetransformWorldModel(LTObject* pObj)
{
	obj_UpdateTransform(pObj);

	wm_Transform(pObj, &((WorldModelObject*)pObj)->m_Transform1f0,
	             (pObj->m_Flags >> 14) & 1);
}

// 0x1000d8e1. The world's own flags decide whether the object may be
// touched at all, and the world pointer is read a second time for the
// builder because the virtual call could have changed it — which is why the
// shipped code keeps its address rather than its value across the call.
void obj_UpdateTransform(LTObject* pObj)
{
	if (!pObj->m_pWorldBsp->IsBusy())
	{
		WorldModelObject* pWM = (WorldModelObject*)pObj;

		wm_BuildTransforms(&((CWMGeometry*)pObj->m_pWorldBsp)->m_vOffsete8,
		                   &pObj->m_Pos, &pObj->m_Rotation,
		                   &pWM->m_Transform1f0, &pWM->m_Transform230);
	}
}


// 0x10007ae3. Push the world model's transform into its geometry: the
// instance's vertices, plane equations and part centres are all the shared
// authored geometry's, run through the 4x4.
//
// Three things read off the shipped code rather than guessed:
//
//   * the copy runs FROM the geometry at +0x1a8 INTO the one at +0x1ac, so
//     +0x1a8 is the shared authored copy and +0x1ac the per-instance one.
//     `m_pOriginalBsp` is therefore a misleading name for +0x1ac, but it is
//     what ltobject_internal.h already calls it and other modules may lean
//     on it, so it stands.
//   * the whole body is skipped when bMoveGeometry is *non*-zero. The only
//     caller passes flag bit 14 of m_Flags, so the bit turns this off.
//   * the counts are the instance's (+0x1ac), not the authored copy's, in
//     all three loops.
//
// A plane is transformed by putting a point on it (the normal times the
// distance) through the full 4x4, rotating the normal with the upper 3x3, and
// re-deriving the distance as the dot of the two.
//
// Byte-identical on server.dll (0x10007ae3). The distance is the inline
// LTVector::Dot, called on the new normal with vPos as the argument --
// written out as a three-term sum it came to the same bytes except for one
// `mov -0x4(%ebp),%esi` / `add $0x18,%esp` pair scheduled three instructions
// early (shipped has it between the dot's two `faddp`s, 0x10007bf1); the
// inline call is what moves it. `vPos.Dot(normal)` (the other way round) is
// five instructions longer, and an accumulator (`=`, `+=`, `+=`) nineteen
// bytes longer. vPos at function scope rather than in the loop body is
// also load-bearing (twenty-one differing bytes to nine when it moved).
//
// NEAR on lithtech.exe (0x421d10, 584 of 584 bytes): only the x87 operand
// order inside the three expanded MatVMul/MatVMul_3x3 rows differs (ship
// row x is x,z,y; ours z,y,x -- and ship's own out-of-line MatVMul at
// 0x421f60 has OUR order). Writing the rows out by hand in either term or
// operand order gives one identical result (VC6 canonicalises); int vs
// uint32 counter and vPos in loop scope move it, but further away; local
// declaration and assignment order do nothing. Measured as a compiler-state
// lottery, not source.
void wm_Transform(LTObject* pObj, LTMatrix* pMat, int bMoveGeometry)
{
	CWMGeometry* pWorld;
	CWMGeometry* pOrig;
	LTVector     vPos;
	uint32       i;

	if (pObj->m_pWorldBsp->IsBusy())
		return;

	pOrig  = (CWMGeometry*)pObj->m_pOriginalBsp;
	pWorld = (CWMGeometry*)pObj->m_pWorldBsp;

	if (!pWorld)
		return;

	if (!pOrig)
		return;

	if (bMoveGeometry)
		return;

	for (i = 0; i < pOrig->m_nVerts; i++)
		MatVMul(&pOrig->m_pVerts[i], pMat, &pWorld->m_pVerts[i]);

	for (i = 0; i < pOrig->m_nPlanes; i++)
	{
		vPos = pWorld->m_pPlanes[i].m_Normal *
		       pWorld->m_pPlanes[i].m_Dist;

		MatVMul_InPlace(pMat, &vPos);

		MatVMul_3x3(&pOrig->m_pPlanes[i].m_Normal, pMat,
		            &pWorld->m_pPlanes[i].m_Normal);

		pOrig->m_pPlanes[i].m_Dist = pOrig->m_pPlanes[i].m_Normal.Dot(vPos);
	}

	for (i = 0; i < pOrig->m_nParts; i++)
	{
		MatVMul(&pOrig->m_ppParts[i]->m_vCentre, pMat,
		        &pWorld->m_ppParts[i]->m_vCentre);
	}
}


// The world BSP as this file needs it. worldbsp.h models the whole of it as
// CWorldBsp; it is repeated here rather than included, the same way CWMPart
// and CWMGeometry above are. Only 0x9c is reached -- the world tree an object
// gets placed in, which worldbsp.h still carries as a uint32 because nothing
// there dereferences it.
// A leaf of that tree as 0x1000d4a9 reaches it: 0x30 bytes in one flat
// array, with the list of point-set records that landed in it at 0x14
// (names ours).
class CWorldLeafView
{
public:
	uint8  m_pad_00[0x14];
	LTLink m_Link14;                         // 0x14
	uint8  m_pad_20[0x30 - 0x20];
};

class CWorldBspView
{
public:
	uint8            m_pad_00[0x88];
	CWorldLeafView*  m_pLeaves;              // 0x88
	uint32           m_nLeaves;              // 0x8c
	uint8            m_pad_90[0x9c - 0x90];
	void*            m_pTree9c;              // 0x9c
};

// The plane record a node splits on, three indirections off the node: the
// tree it belongs to, that tree's pointer at 0x28, and the plane at 0x34 of
// what THAT points at. worldbsp.h models the middle one as CWorldTree and its
// 0x28 as CWorldTreeSub28; these are the local views, the same way CWMPart
// and CWMGeometry above are (names ours).
class CWorldNodePlanesView
{
public:
	uint8    m_pad_00[0x34];
	LTPlane* m_pPlane;                       // 0x34
};

class CWorldTreeView
{
public:
	uint8 m_pad_00[0x28];
	CWorldNodePlanesView* m_pSub28;          // 0x28
};

// A node of the world tree as the descent reads it: the tree it belongs to,
// then the back child and the front child. 0x1000d4a9 adds the two fields
// that end a descent -- the leaf index, which is 0xffff on every node that
// still has a plane, and the flag pair that stops one early.
class CWorldNodeView
{
public:
	CWorldTreeView*  m_pTree;                // 0x00
	CWorldNodeView*  m_pBack;                // 0x04
	CWorldNodeView*  m_pFront;               // 0x08
	uint8            m_pad_0c[0x14 - 0x0c];
	uint16           m_nLeaf;                // 0x14  0xffff while the node splits
	uint16           m_Flags;                // 0x16  either of the low two bits
	                                         //       ends the walk with nothing
};

// 0x10051a2c and 0x10051a30 — repeated from worldbsp.h, which this file does
// not include. See the note there: the two terminators mean different things.
extern void* g_pWorldTreeFrontTerm;
extern void* g_pWorldTreeBackTerm;

// 0x1000d437 — the tree node a sphere belongs in, or null.
//
// A plane descent with a fat test: the sphere goes forward only when it is
// wholly in front of the plane and back only when it is wholly behind, and
// anything straddling stops at the node it is at. The two terminators are
// what make the arms asymmetric -- running out of tree forwards answers with
// the current node, running out backwards answers with nothing.
//
// The position is copied to a local ONCE, before the loop, because
// LTPlane::DistTo takes its vector BY VALUE; the copy is what a by-reference
// signature would not have produced.
void* wt_FindNodeForSphere(CWorldNodeView* pNode, LTVector* pPos, float radius)
{
	// Both "stop here" exits are BREAKS and the return is after the loop, so
	// the `mov %ecx,%eax; leave; ret` is hosted once at the end and the
	// `return LTNULL` sits inline before it. Returning pNode from inside the
	// arms instead costs one instruction and inverts the front test -- the
	// shared-exit hosting rule again.
	for (;;)
	{
		float dist = pNode->m_pTree->m_pSub28->m_pPlane->DistTo(*pPos);

		if (dist > radius)
		{
			if (pNode->m_pFront == (CWorldNodeView*)g_pWorldTreeFrontTerm)
				break;

			pNode = pNode->m_pFront;
		}
		else if (dist < -radius)
		{
			pNode = pNode->m_pBack;

			if (pNode == (CWorldNodeView*)g_pWorldTreeBackTerm)
				return LTNULL;
		}
		else
		{
			break;
		}
	}

	return pNode;
}

// The three statics 0x1000d4a9 works through, in the order the image lays
// them out: 0x100583b8, 0x100583bc and 0x100583c0, immediately before the
// record bank at 0x100583c8. Both images agree on the layout AND on the
// spelling -- lithtech.exe has the same three dwords at 0x4b3020, 0x4b3024
// and 0x4b3028 with the same gap before its bank at 0x4b3030 -- and all
// three are really statics, not locals: the side is stored and READ BACK
// through memory at /O1 (`cmp %edx,0x100583b8`), which no local of a leaf
// function would be. Only the index survives in a register across the loop,
// because a single store dominates every use of it (names ours).
static int32     g_nPointSide;
static int32     g_nPointIndex;
static PGRecord* g_pPGRecord;

// 0x1000d4a9 — put a point set into the world tree under one link, splitting
// it at every plane it straddles. Six arguments, cdecl (name ours).
//
// The descent is by SIDE AGREEMENT, not by extent: every point is put on one
// side of the node's plane and the set only splits when two of them disagree,
// so a set that is wholly in front or wholly behind is handed down whole and
// only a straddling one is recursed on. The disagreement is signalled by a
// third value of the side variable, which is why the test after the loop is
// `== 2` and the descent that follows indexes the child pair with the side
// itself: 0 is the back child, 1 the front, and 2 never reaches it.
//
// Reaching a leaf allocates one record out of the poly-grid bank and puts it
// in two lists at once -- the leaf's, so the world can find every set in it,
// and the caller's, so the object can free them all again (pg_FreeData).
// That is why the record carries two links and why its m_pData pair points
// at itself rather than at the object: the object is a field of its own.
//
// Three things here are only visible because BOTH images were read:
//
//   * sb_Allocate is a `call` at 0x1000d5ce and the free-list walk written
//     out at 0x4293a9. That is the /Os-vs-/Ot budget on a 39-byte inline,
//     the same split servermgr.h documents, so the header's inline is the
//     one spelling that produces both.
//   * the leaf index is `movzwl` at /O1 and `xor %eax,%eax; mov 0x14(%esi),
//     %ax` at /O2 -- the same uint16 field, two encodings of the same
//     zero-extending load, so the field width is not in question.
//   * the side is recomputed from memory at /O1 and forwarded in a register
//     at /O2, which is what says it is a static rather than a local: a local
//     would be forwarded by both.
//
// The recursion is a real one and the second child is a loop, not a second
// call: the shipped code calls itself for the back child and then walks to
// the front child with `mov 0x8(%ebx),%ebx` and jumps back to the top. That
// is the tail call VC6 turns the `else` arm's assignment into for free, so
// the source is a `for (;;)` whose every arm either returns or re-points
// pNode.
void wt_InsertPoints(CWorldBspView* pWorld, CWorldNodeView* pNode,
                     LTVector* pPoints, int32 nPoints, LTObject* pObject,
                     LTLink* pLink)
{
	int32 nSide;

	for (;;)
	{
		if (pNode->m_Flags & 3)
			return;

		if (pNode->m_nLeaf != 0xffff)
		{
			CWorldLeafView* pLeaf;

			if (pNode->m_nLeaf < pWorld->m_nLeaves)
				pLeaf = pWorld->m_pLeaves + pNode->m_nLeaf;
			else
				pLeaf = LTNULL;

			g_pPGRecord = (PGRecord*)sb_Allocate(&g_MemTrackBank);

			if (!g_pPGRecord)
				return;

			g_pPGRecord->m_Link0c.m_pData = g_pPGRecord;
			g_pPGRecord->m_Link.m_pData   = g_pPGRecord;
			g_pPGRecord->m_pObject        = pObject;
			g_pPGRecord->m_pLeaf          = pLeaf;

			dl_Insert(&pLeaf->m_Link14, &g_pPGRecord->m_Link);
			dl_Insert(pLink, &g_pPGRecord->m_Link0c);

			return;
		}

		if (pNode->m_pTree->m_pSub28->m_pPlane->DistTo(pPoints[0]) > 0.0f)
			nSide = 1;
		else
			nSide = 0;

		for (g_nPointIndex = 1; g_nPointIndex < nPoints; g_nPointIndex++)
		{
			if (pNode->m_pTree->m_pSub28->m_pPlane->DistTo(
			        pPoints[g_nPointIndex]) > 0.0f)
				g_nPointSide = 1;
			else
				g_nPointSide = 0;

			if (g_nPointSide != nSide)
			{
				nSide = 2;
				break;
			}
		}

		if (nSide == 2)
		{
			wt_InsertPoints(pWorld, pNode->m_pBack, pPoints, nPoints,
			                pObject, pLink);

			pNode = pNode->m_pFront;
		}
		else
		{
			pNode = (&pNode->m_pBack)[nSide];
		}
	}
}

// A poly grid's own fields past the object's: the link the tree hangs it by,
// and the footprint it covers. objbank.cpp models the whole subclass as
// CObjPolyGrid; this is the local view, and the two dimensions are its
// m_Unknown1f0/m_Unknown1f4 (names ours).
class CPolyGridView
{
public:
	uint8  m_pad_000[0x1e4];
	LTLink m_Link1e4;                        // 0x1e4
	uint32 m_Width;                          // 0x1f0
	uint32 m_Height;                         // 0x1f4
};

// 0x1000d631 — a poly grid's extra placement work, reached only from the arm
// of obj_LinkIntoWorldTree that inserts through a pointer and only for
// OT_POLYGRID.
//
// It puts the grid in by its eight transformed corners rather than by a
// sphere. The half-extents come off the footprint at 0x1f0/0x1f4 -- unsigned,
// which the `fildll` with a zeroed high dword proves -- and the middle axis
// has no field at all: a flat +/-127, so a grid is a fixed height and only
// its footprint varies. The same 128.0f-and-nothing shape CObjPolyGrid's
// bounding radius uses.
//
// The half-extents are one LTVector local, not two floats: the shipped frame
// is 0xb4, twelve bytes more than two scalars need, and those twelve bytes sit
// between the corners and the conversion slot with only the x stored to them
// (-0x14). As a vector the 0.5 is loaded once and held in an FP register for
// both products (`fmul %st(1),%st`, then `fimul` off the height's slot) and
// the half-height stays in a register through all eight corners; as two
// floats VC6 multiplies each dimension by the constant in memory and the x87
// allocation after that differs throughout. Byte-identical on both images
// (server 0x1000d631, lithtech 0x429440).
//
// The 127.0f is loaded from .rdata for six of its eight uses and stored as an
// integer immediate for the last two. That is register pressure, not a source
// distinction.
void pg_PlaceInTree(CWorldBspView* pWorld, LTObject* pObject)
{
	LTMatrix mTransform;
	LTVector corners[8];
	LTVector vHalf;
	uint32 i;

	vHalf.x = 0.5f * (float)((CPolyGridView*)pObject)->m_Width;
	vHalf.y = 127.0f;
	vHalf.z = 0.5f * (float)((CPolyGridView*)pObject)->m_Height;

	corners[0].Init(-vHalf.x, -vHalf.y, -vHalf.z);
	corners[1].Init( vHalf.x, -vHalf.y, -vHalf.z);
	corners[2].Init( vHalf.x,  vHalf.y, -vHalf.z);
	corners[3].Init(-vHalf.x,  vHalf.y, -vHalf.z);
	corners[4].Init(-vHalf.x, -vHalf.y,  vHalf.z);
	corners[5].Init( vHalf.x, -vHalf.y,  vHalf.z);
	corners[6].Init( vHalf.x,  vHalf.y,  vHalf.z);
	corners[7].Init(-vHalf.x,  vHalf.y,  vHalf.z);

	pObject->GetTransformMatrix(&mTransform);

	for (i = 0; i < 8; i++)
		MatVMul_InPlace_H(&mTransform, &corners[i]);

	wt_InsertPoints(pWorld, (CWorldNodeView*)pWorld->m_pTree9c, corners, 8,
	                pObject,
	                &((CPolyGridView*)pObject)->m_Link1e4);
}

// 0x1000d826 — put an object into the world tree, returning the node it
// landed in, or null if the tree would not take it. Declared in
// ltobject_internal.h, where CObjWorldModel's forwarder can reach it.
//
// The two arms differ in ONE respect and it is not a detail: with engine-side
// data present the object is inserted after the link AT node+0xc, and without
// it after the link the same offset POINTS AT. The shipped code is explicit
// -- `lea 0xc(%edi)` against `mov 0xc(%edi)` -- so node+0xc is a link in one
// case and a pointer to one in the other, and the node type is a union of
// some kind. The poly-grid follow-up hangs off the pointer arm only.
//
// The radius goes through a float LOCAL: lithtech.exe (0x004296a0) homes it
// in the dead pObject slot (`mov %eax,0x18(%esp)`) before pushing it, which
// the bare member read does not produce; server.dll is identical either way.
void* obj_LinkIntoWorldTree(CWorldBspView* pWorld, LTObject* pObject)
{
	void* pNode;
	float fRadius;

	obj_ReleaseMemory(pObject);

	fRadius = pObject->m_Radius;
	pNode = wt_FindNodeForSphere((CWorldNodeView*)pWorld->m_pTree9c,
	                             &pObject->m_Pos,
	                             fRadius);

	if (pNode)
	{
		if (pObject->m_pSData)
		{
			dl_Insert((LTLink*)((uint8*)pNode + 0xc), &pObject->m_Link6c);
		}
		else
		{
			dl_Insert(*(LTLink**)((uint8*)pNode + 0xc), &pObject->m_Link6c);

			if (pObject->m_ObjectType == OT_POLYGRID)
				pg_PlaceInTree(pWorld, pObject);
		}

		pObject->m_pTreeNode78 = pNode;
	}

	return pNode;
}

// 0x1000d8a8 — let go of the engine-side memory an object holds. The list it
// is in is only unlinked from when the flag says it is in one, and only a
// poly grid has data of its own beyond that.
void obj_ReleaseMemory(LTObject* pObj)
{
	if (pObj->m_pTreeNode78)
	{
		LTLink* pLink = &pObj->m_Link6c;

		pLink->m_pPrev->m_pNext = pLink->m_pNext;
		pLink->m_pNext->m_pPrev = pLink->m_pPrev;
		pLink->TieOff();

		pObj->m_pTreeNode78 = LTNULL;
	}

	if (pObj->m_ObjectType == OT_POLYGRID)
		pg_FreeData(pObj);
}

// 0x10024a4c. The world-space box is the position give or take the
// dimensions, and the bounding sphere is the dimensions' own length with a
// tenth of a unit of slack. Both boxes go through the vector operators, so
// each costs a by-value copy of m_Dims into the operator's argument slot and
// a copy of its return temporary out again — that pair of twelve-byte slots
// is the whole 0x18-byte frame. The radius is a real call: VC6 declines to
// inline Mag() however the header writes it.
//
// `inline`, and defined ABOVE SetPos: lithtech.exe's /O2 SetPos (0x0045aaa0,
// 255 bytes) carries this whole body and the client has no copy of its own,
// while server.dll's /Os declines it and SetPos calls 0x10024a4c.
inline void LTObject::UpdateBounds()
{
	m_BBoxMin = m_Pos - m_Dims;
	m_BBoxMax = m_Pos + m_Dims;

	m_Radius = m_Dims.Mag() + 0.1f;
}

// 0x10024a33 — move the object. The position arrives by value, and the
// bounds are rebuilt afterwards.
void LTObject::SetPos(LTVector vPos)
{
	m_Pos = vPos;

	UpdateBounds();
}

// 0x1000d7c7. Every record the grid holds comes out of both lists it is in
// and goes back on the bank's spare list, its first dword taken over as the
// free link. The next link is read before the record is unlinked, because
// the unlink ties the record's own link off.
void pg_FreeData(LTObject* pObj)
{
	LTLink* pCur  = ((PolyGridObject*)pObj)->m_Records.m_pNext;
	LTLink* pHead = &((PolyGridObject*)pObj)->m_Records;

	while (pCur != pHead)
	{
		LTLink*   pNext = pCur->m_pNext;
		PGRecord* pRec  = (PGRecord*)pCur->m_pData;

		pRec->m_Link.Remove();
		pRec->m_Link0c.Remove();

		*(void**)pRec = g_MemTrackBank.m_pFreeList;
		g_MemTrackBank.m_pFreeList = pRec;

		pCur = pNext;
	}
}
