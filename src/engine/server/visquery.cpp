//------------------------------------------------------------------
//
//  MODULE  : VISQUERY.CPP
//
//  PURPOSE : server.dll's visibility traversal and PVS walk (0x10038e77 ..
//            0x100392a8; Primal Hunt 0x10048fa0 .. 0x100493fc), its own
//            unit as the client's src/engine/lithtech/visquery.cpp is:
//            Primal Hunt's copy ends in its own <string> guard pair
//            (0x100493e8). Split out of worldtree.cpp (the end of its
//            SECTION C and the start of SECTION D).
//
//------------------------------------------------------------------

#ifdef PRIMAL_HUNT
// PH: <string>'s guarded ctype initializer ends the unit (0x100493e8).
#include <string>
#endif
#include "worldbsp.h"
#include "servermgr.h"


// ---- the visibility traversal's entry point -----------------------------

class CVisObject_C
{
public:
	uint8    m_pad_00[0x58];
	uint32   m_Unknown58;                    // 0x058  the visit mark
};

// The list head is the 8-byte half of a link (the SDK's CheapLTLink); the
// links on it carry the object at +8 (LTLink).
class CVisLink_C;

class CVisLinkHead_C
{
public:
	CVisLink_C*   m_pPrev;                   // 0x00
	CVisLink_C*   m_pNext;                   // 0x04
};

class CVisLink_C : public CVisLinkHead_C
{
public:
	CVisObject_C* m_pData;                   // 0x08
};

// 0x30 bytes, at CWorldBsp +0x88/+0x8c.
class CVisRecord_C
{
public:
	uint8   m_pad_00[0x30];
};

// 0x18 bytes: one per world leaf, at CWorldBsp +0x70/+0x74.
class CVisLeaf_C
{
public:
	uint8          m_pad_00[0x0c];
	CVisLinkHead_C m_Objects;                // 0x0c  the leaf's object list
	uint16         m_iRecord;                // 0x14  0xffff = none
	uint8      m_pad_16[0x18 - 0x16];
};

// 0x24 bytes, at CWorldBsp +0xb0 (m_pChunks).
class CVisChunk_C
{
public:
	uint8   m_pad_00[0x06];
	uint8   m_Flags;                         // 0x06  bit 0 tested
	uint8   m_pad_07[0x24 - 0x07];
};

class CVisRecordArray_C
{
public:
	uint32        m_Unknown00;               // 0x00
	CVisRecord_C* m_pArray;                  // 0x04
	uint32        m_nElements;               // 0x08

	inline CVisRecord_C* GetRecord(uint32 i)
	{
		return i < m_nElements ? &m_pArray[i] : LTNULL;
	}
};

// CWorldBsp, as the traversal reaches it.
class CVisWorld_C
{
public:
	uint8        m_pad_00[0x70];
	CVisLeaf_C*  m_pLeaves;                  // 0x70
	uint32       m_nLeaves;                  // 0x74
	uint8        m_pad_78[0x84 - 0x78];
	CVisRecordArray_C m_Records;             // 0x84
	uint8        m_pad_90[0x9c - 0x90];
	void*        m_pRootNode;                // 0x9c
	uint8        m_pad_a0[0xb0 - 0xa0];
	CVisChunk_C* m_pChunks;                  // 0xb0

};

// The tree the query may name: 8-byte entries at +0x10, count at +0x2e.
class CVisTreeEntry_C
{
public:
	uint16  m_iChunk;                        // 0x00
	uint16  m_pad_02;
	uint32  m_Value;                         // 0x04
};

class CVisTree_C
{
public:
	uint8            m_pad_00[0x10];
	CVisTreeEntry_C* m_pEntries;             // 0x10
	uint8            m_pad_14[0x2e - 0x14];
	uint16           m_nEntries;             // 0x2e
};

typedef void (*VisObjFn_C)(CVisObject_C* pObj, void* pUser);
typedef void (*VisRecordFn_C)(CVisRecord_C* pRecord, void* pUser);
typedef void (*VisCollectFn_C)(CVisLinkHead_C* pHead, CVisObject_C*** ppArray,
                               uint32* pnCount);
typedef int  (*VisFilterFn_C)(CVisChunk_C* pChunk);

class CWorldVisQuery_C
{
public:
	CVisTree_C*    m_pTree;                  // 0x00
	CVisWorld_C*   m_pWorld;                 // 0x04
	VisObjFn_C     m_ObjFn;                  // 0x08
	VisRecordFn_C  m_RecordFn;               // 0x0c
	VisCollectFn_C m_CollectFn;              // 0x10
	VisFilterFn_C  m_FilterFn;               // 0x14
	void*          m_pUser;                  // 0x18
	uint32         m_Mark;                   // 0x1c
};

#define VIS_BUFFER_SIZE_C  5000

// 0x10058c68 .. 0x10058c74 — the traversal's state. Section D's recursion
// (0x10038ff1) reads all four.
CVisObject_C**    g_pVisLimit_C;             // 0x10058c68
CWorldVisQuery_C* g_pVisQuery_C;             // 0x10058c6c
uint32            g_nVisBit_C;               // 0x10058c70
uint32            g_VisValue_C;              // 0x10058c74

// 0x10038ff1 — the per-node recursion (section D).
void wt_VisitNode_C(void* pNode, CVisObject_C** pBuffer, uint32 nBuffered);

// 0x10038fae — the default collector (client 0x0048bc50).
void wt_CollectList_C(CVisLinkHead_C* pHead, CVisObject_C*** ppArray,
                      uint32* pnCount);

// 0x10038e77 (client 0x0048b8e0, which adds a PVS walk the server lacks).
// The record lookup is an inline accessor on a sub-object at +0x84; the
// same ternary written on the world itself loads the array base into its
// own register first.
void wt_VisibilityTraversal(CWorldVisQuery* pVisQuery)
{
	CWorldVisQuery_C* pQuery = (CWorldVisQuery_C*)pVisQuery;
	CVisObject_C*     buffer[VIS_BUFFER_SIZE_C];
	CVisObject_C**    pObjects;
	uint32            i, j, nObjects;

	g_pVisLimit_C = &buffer[VIS_BUFFER_SIZE_C];

	if (!pQuery->m_CollectFn)
		pQuery->m_CollectFn = wt_CollectList_C;

	g_pVisQuery_C = pQuery;

	if (pQuery->m_pTree && pQuery->m_pTree->m_nEntries > 0)
	{
		for (i = 0; i < pQuery->m_pTree->m_nEntries; i++)
		{
			CVisTreeEntry_C* pEntry = &pQuery->m_pTree->m_pEntries[i];

			if (i != 0)
			{
				CVisChunk_C* pChunk = &pQuery->m_pWorld->m_pChunks[pEntry->m_iChunk];

				if (!(pChunk->m_Flags & 1))
					continue;

				if (!pQuery->m_FilterFn(pChunk))
					continue;
			}

			g_VisValue_C = pEntry->m_Value;
			g_nVisBit_C = 0;
			wt_VisitNode_C(pQuery->m_pWorld->m_pRootNode, buffer, 0);
		}
	}
	else
	{
		for (i = 0; i < pQuery->m_pWorld->m_nLeaves; i++)
		{
			CVisLeaf_C* pLeaf = &pQuery->m_pWorld->m_pLeaves[i];

			nObjects = 0;
			pObjects = buffer;

			if (pLeaf->m_iRecord != 0xffff)
			{
				CVisRecord_C* pRecord = pQuery->m_pWorld->m_Records.GetRecord(pLeaf->m_iRecord);
				if (pRecord)
					pQuery->m_RecordFn(pRecord, pQuery->m_pUser);
			}

			pQuery->m_CollectFn(&pLeaf->m_Objects, &pObjects, &nObjects);

			for (j = 0; (int)j < (int)nObjects; j++)
				pQuery->m_ObjFn(pObjects[j], pQuery->m_pUser);
		}
	}
}

void wt_CollectList_C(CVisLinkHead_C* pHead, CVisObject_C*** ppArray,
                      uint32* pnCount)
{
	CVisLink_C* pCur;

	pCur = pHead->m_pPrev;

	while (pCur != (CVisLink_C*)pHead)
	{
		CVisObject_C* pObj = pCur->m_pData;

		if (*ppArray < g_pVisLimit_C &&
		    pObj->m_Unknown58 != g_pVisQuery_C->m_Mark)
		{
			(*ppArray)[*pnCount] = pObj;
			(*pnCount)++;
		}

		pCur = pCur->m_pPrev;
	}
}

// ==== END SECTION C =======================================================

// ==== SECTION D: 0x10038ff1 .. 0x100399a5 ================================
//
// Three runs, each the server twin of client code already written up:
//
//   0x10038ff1 .. 0x100392a8  the PVS walk's per-node recursion and its
//       three helpers: lithtech.exe 0x0048be40 (which EXPANDS all three
//       inline at /O2) and 0x0048bca0 / 0x0048bd70; fntdraw.cpp documents
//       the query and the globals.  Server.dll calls all three out of line.
//   0x100392a9 .. 0x100395a3  the RIFF/WAVE reader: lithtech.exe 0x0048c1a0
//       and 0x0048c270, lttexmod.cpp's ParseWaveFile / ParseWaveChunk.
//       smgr.cpp's fs_GetSourceInfo is the first of these.
//   0x100395a4 .. 0x100399a4  the world tree's object base and node class:
//       lithtech.exe 0x0048c540 .. 0x0048c9b0, worldtree2.cpp items 1-12.
//       0x10039707/0x10039735/0x10039740 are objtypes.h's CObjRoot ctor,
//       BaseTerm and Unlink (vtable 0x1004ea04: slot 2 is 0x10039740, slot
//       3 _purecall).
//
// Names carry the section suffix _D.

#include <string.h>
#include "ltplane.h"

// ---- the PVS walk ------------------------------------------------------ //

// The walked object: +0x58 the visit mark, LTObject's m_Pos and m_Radius.
// AVP2: ltobject_internal.h's 0x194/0x1ac, NOLF's +8 (both AvP2 1.0.9.6's
// and Primal Hunt's filters read them there: PH 0x100492ff, 0x10049326).
class CVisObj_D
{
public:
	uint8    m_pad_00[0x58];
	uint32   m_nMark;                        // 0x058
	uint8    m_pad_5c[0x194 - 0x5c];
	LTVector m_Pos;                          // 0x194
	uint8    m_pad_1a0[0x1ac - 0x1a0];
	float    m_Radius;                       // 0x1ac
};

// A leaf record, 0x30 bytes (`lea (%ecx,%ecx,2); shl $4`).
struct VisLeaf_D
{
	uint8 m_pad[0x30];
};

class CVisWorld_D
{
public:
	uint8      m_pad_00[0x88];
	VisLeaf_D* m_pLeafs;                     // 0x88
	uint32     m_nLeafs;                     // 0x8c

	VisLeaf_D* GetLeaf(uint32 i)
	{
		return (i < m_nLeafs) ? &m_pLeafs[i] : LTNULL;
	}
};

struct VisQuery_D
{
	void*         m_p00;                     // 0x00
	CVisWorld_D*  m_pWorld;                  // 0x04
	void        (*m_pfnObject)(CVisObj_D* pObj, void* pUser);        // 0x08
	void        (*m_pfnLeaf)(VisLeaf_D* pLeaf, void* pUser);         // 0x0c
	void        (*m_pfnGather)(void* pList, CVisObj_D*** ppArray,
	                           uint32* pnCount);                     // 0x10
	int         (*m_pfn14)(void* pRecord);                           // 0x14
	void*         m_pUser;                   // 0x18
	uint32        m_nMark;                   // 0x1c
};

// AVP2: the node's polygon carries its plane at +0x28 itself, guarded by
// the word after it: both AvP2 1.0.9.6's walk and Primal Hunt's read
// `node->+0 ? (+0x2c ? +0x28 : 0) : 0` (PH 0x100491f8..0x10049209), where
// NOLF went through a surface at +0x28 to its plane at +0x34 (names ours).
struct VisPoly_D
{
	uint8         m_pad_00[0x28];
	LTPlane*      m_pPlane;                  // 0x28
	uint32        m_Unknown2c;               // 0x2c
};

struct VisNode_D
{
	VisPoly_D*  m_pPoly;                     // 0x00
	VisNode_D*  m_Sides[2];                  // 0x04
	CheapLTLink m_Objects;                   // 0x0c
	uint16      m_nLeaf;                     // 0x14  0xffff: none
	uint8       m_Flags;                     // 0x16

	LTPlane* GetPlane()
	{
		return (m_pPoly && m_pPoly->m_Unknown2c) ? m_pPoly->m_pPlane : LTNULL;
	}
};

extern CVisObj_D**  g_pVisLimit_D;           // 0x10058c68
extern VisQuery_D*  g_pVisQuery_D;           // 0x10058c6c
extern uint32       g_nVisBit_D;             // 0x10058c70
extern uint8*       g_pVisBits_D;            // 0x10058c74
// 0x1005911c..0x10059128 (.bss): the walk's own four scratch globals, used
// by nothing but vis_WalkNode_D, and so defined here (the four above are
// SECTION C's, under their _C names).
VisLeaf_D*   g_pVisLeaf_D;                   // 0x1005911c
CVisObj_D**  g_pVisCursor_D;                 // 0x10059120
uint32       g_nVisFiltered_D;               // 0x10059124
CVisObj_D**  g_pVisFiltered_D;               // 0x10059128

uint8       vis_ReadBit_D();                                         // 0x10039151
CVisObj_D** vis_FilterNeg_D(LTPlane* pPlane, CVisObj_D** pIn, uint32 nIn,
                            uint32* pnOut, LTBOOL bAppend);          // 0x10039177
CVisObj_D** vis_FilterPos_D(LTPlane* pPlane, CVisObj_D** pIn, uint32 nIn,
                            uint32* pnOut);                          // 0x10039211

// 0x10038ff1 — one node of the walk (lithtech 0x0048be40). The loop is in
// the source: written as two plain recursive calls, VC6 keeps both calls,
// where the shipped code rewrites its own argument slots and jumps back.
// NEAR (354 vs 352, one block): the leaf address is built index-first
// into %ecx (`lea; shl; add 0x88(%edx),%ecx`) in the shipped code and
// base-first through %edi here. Ternary/if accessor, byte arithmetic,
// operand order, index type, a local and bBack's scope all measured: no
// change; nor does modelling +0x84 as a CMoArray<VisLeaf_D> (GetSize() /
// operator[], ternary or if/return), and a result-local GetLeaf loses the
// ternary's jmp. Register allocation.
// PH NEAR (376 against 374, server.dll 0x1004911a): only GetLeaf's
// address, which the shipped code forms in one register (`lea ; shl ; add
// 0x88(%edx),%ecx`) and ours through a second (allocation).
void vis_WalkNode_D(VisNode_D* pNode, CVisObj_D** pArray, uint32 nCount)
{
	uint32 bBack;

	for (;;)
	{
		g_pVisQuery_D->m_pfnGather(&pNode->m_Objects, &pArray, &nCount);

		if (pNode->m_nLeaf != 0xffff && vis_ReadBit_D())
		{
			g_pVisLeaf_D = g_pVisQuery_D->m_pWorld->GetLeaf(pNode->m_nLeaf);
			if (g_pVisLeaf_D)
				g_pVisQuery_D->m_pfnLeaf(g_pVisLeaf_D, g_pVisQuery_D->m_pUser);

			g_pVisCursor_D = pArray;
			while (nCount--)
			{
				if ((*g_pVisCursor_D)->m_nMark != g_pVisQuery_D->m_nMark)
				{
					(*g_pVisCursor_D)->m_nMark = g_pVisQuery_D->m_nMark;
					g_pVisQuery_D->m_pfnObject(*g_pVisCursor_D, g_pVisQuery_D->m_pUser);
				}

				g_pVisCursor_D++;
			}

			nCount = 0;
		}

		bBack = pNode->m_Sides[1]->m_Flags & 8;

		if ((pNode->m_Sides[0]->m_Flags & 8) && vis_ReadBit_D())
		{
			g_pVisFiltered_D = vis_FilterNeg_D(pNode->GetPlane(), pArray, nCount,
			                                   &g_nVisFiltered_D, bBack);
			if (bBack)
			{
				vis_WalkNode_D(pNode->m_Sides[0], g_pVisFiltered_D, g_nVisFiltered_D);
			}
			else
			{
				pArray = g_pVisFiltered_D;
				nCount = g_nVisFiltered_D;
				pNode  = pNode->m_Sides[0];
				continue;
			}
		}

		if (bBack && vis_ReadBit_D())
		{
			pArray = vis_FilterPos_D(pNode->GetPlane(), pArray, nCount, &nCount);
			pNode  = pNode->m_Sides[1];
			continue;
		}

		return;
	}
}

// 0x10039151 — the next bit of the PVS stream.
uint8 vis_ReadBit_D()
{
	uint8 bBit = (uint8)((1 << (g_nVisBit_D & 7)) & g_pVisBits_D[g_nVisBit_D >> 3]);
	g_nVisBit_D++;
	return bBit;
}

// 0x10039177 — keep the objects on the negative side (lithtech 0x0048bca0).
CVisObj_D** vis_FilterNeg_D(LTPlane* pPlane, CVisObj_D** pIn, uint32 nIn,
                            uint32* pnOut, LTBOOL bAppend)
{
	CVisObj_D** pOut;

	pOut = pIn;

	if (bAppend)
		pOut += nIn;

	*pnOut = 0;

	if (pOut + nIn < g_pVisLimit_D)
	{
		while (nIn--)
		{
			CVisObj_D* pObj = *pIn;

			if (pObj->m_nMark != g_pVisQuery_D->m_nMark)
			{
				if (pPlane->DistTo(pObj->m_Pos) < pObj->m_Radius + 0.1f)
				{
					pOut[*pnOut] = pObj;
					(*pnOut)++;
				}
			}

			pIn++;
		}
	}

	return pOut;
}

// 0x10039211 — and the positive side (lithtech 0x0048bd70).
CVisObj_D** vis_FilterPos_D(LTPlane* pPlane, CVisObj_D** pIn, uint32 nIn,
                            uint32* pnOut)
{
	CVisObj_D** pOut;

	pOut = pIn;

	*pnOut = 0;

	if (pOut + nIn >= g_pVisLimit_D)
		return pOut;

	while (nIn--)
	{
		CVisObj_D* pObj = *pIn;

		if (pObj->m_nMark != g_pVisQuery_D->m_nMark)
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


