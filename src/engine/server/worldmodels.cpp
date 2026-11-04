// server.dll — the world's world-model list (reconstruction). Build /O1.
#include <string.h>
#include <stdio.h>
#include "worldmodels.h"
#include "ltobject_internal.h"
#include "commonphysics.h"

// A poly is named by two indices, and both are checked before either is
// used: the model, then the poly within that model's own run.
void* CWorldModelList::FindPoly(CWorldPolyKey* pKey)
{
	uint32 iModel = pKey->m_iModel;
	CWorldModelData* pData;
	uint32 iPoly;

	if (iModel >= m_nEntries)
		return LTNULL;

	pData = m_ppEntries[iModel]->m_pData;
	iPoly = pKey->m_iPoly;

	if (iPoly >= pData->m_nPolies)
		return LTNULL;

	return pData->m_pPolies[iPoly];
}

// The two flag searches are the same walk over the same list, and each stops
// at the first model that carries its bit.
CWorldModelData* CWorldModelList::FindFlagged20()
{
	uint32 i;

	for (i = 0; i < m_nEntries; i++)
	{
		CWorldModelData* pData = m_ppEntries[i]->m_pData;

		if (pData->m_Flags & 0x20)
			return pData;
	}

	return LTNULL;
}

CWorldModelData* CWorldModelList::FindFlagged10()
{
	uint32 i;

	for (i = 0; i < m_nEntries; i++)
	{
		CWorldModelData* pData = m_ppEntries[i]->m_pData;

		if (pData->m_Flags & 0x10)
			return pData;
	}

	return LTNULL;
}

// Both copies of every world model go back to how they were loaded — the one
// in play and the one it was made from — and either may be absent.
void CWorldModelList::ResetAll()
{
	uint32 i;

	for (i = 0; i < m_nEntries; i++)
	{
		CWorldModelEntry* pEntry = m_ppEntries[i];

		if (pEntry->m_pData)
			wm_ResetGeometry(pEntry->m_pData);

		if (pEntry->m_pOriginal)
			wm_ResetGeometry(pEntry->m_pOriginal);
	}
}


// ---- the rest of the module ------------------------------------------- //
//
// Everything below needs classes and fields worldmodels.h does not carry.
// They are declared here rather than added to that header because the header
// belongs to another module in this reconstruction; the natural home for all
// of it is worldmodels.h beside CWorldModelData.

// 0x1000d374 — dalloc's cleared form (dalloc.cpp).
void* dalloc_z(uint32 nSize);

// The growable array the world data holds five of. servermgr.h models the
// same shape twice — CConnList and CUnknown8FC — as a vptr, a buffer, a
// count, the buffer's slack and the grow step, 0x14 bytes in all; each
// element size gets a resize of its own, which is why the five calls below
// reach four different addresses (names ours).
// What the loader below reads off each of them is the same two fields at the
// same two offsets — the buffer at +0x04 and the count at +0x08 — which is
// the shape servermgr.h's two copies already show (names ours).

// The light animations themselves: 0x60-byte records the by-name lookup at
// 0x1000a268 walks with a stride of 0x60.
class CWDArray04
{
public:
	LTBOOL SetSize(uint32 nItems, CMoAllocator* pAlloc);  // 0x1000c267

	uint8              m_pad_00[0x04];
	class CLARecord*   m_pItems;             // 0x04
	uint32             m_nItems;             // 0x08
	uint8              m_pad_0c[0x14 - 0x0c];
};

// Four-byte elements, which is why both of the world's two copies of this
// array resize through the same address: the light array holds a pair of
// sixteen-bit words per light and the frame array a pointer per frame.
class CWDArray18
{
public:
	LTBOOL SetSize(uint32 nItems, CMoAllocator* pAlloc);  // 0x1000c470

	uint8              m_pad_00[0x04];
	class CLALight*    m_pItems;             // 0x04
	uint32             m_nItems;             // 0x08
	uint8              m_pad_0c[0x14 - 0x0c];
};

class CWDArray2c
{
public:
	LTBOOL SetSize(uint32 nItems, CMoAllocator* pAlloc);  // 0x1000c470

	uint8                 m_pad_00[0x04];
	class CLAFrameEntry** m_ppItems;         // 0x04
	uint32                m_nItems;          // 0x08
	uint8                 m_pad_0c[0x14 - 0x0c];
};

// One byte per element: the loader hands out spans of it by adding a running
// byte count to the buffer and comparing that count against m_nItems.
class CWDArray40
{
public:
	LTBOOL SetSize(uint32 nItems, CMoAllocator* pAlloc);  // 0x1000be59

	uint8  m_pad_00[0x04];
	uint8* m_pItems;                         // 0x04
	uint32 m_nItems;                         // 0x08
	uint8  m_pad_0c[0x14 - 0x0c];
};

// Eight-byte elements: one span of baked data per light per frame.
class CWDArray54
{
public:
	LTBOOL SetSize(uint32 nItems, CMoAllocator* pAlloc);  // 0x1002ccee

	uint8                m_pad_00[0x04];
	class CLAFrameEntry* m_pItems;           // 0x04
	uint32               m_nItems;           // 0x08
	uint8                m_pad_0c[0x14 - 0x0c];
};

// One light in one animation. Four bytes, read as two separate two-byte
// reads, so the pair is two sixteen-bit fields and not one dword (names
// ours).
class CLALight
{
public:
	uint16 m_n00;                            // 0x00
	uint16 m_n02;                            // 0x02
};

// One light's baked data for one frame: where it is in the byte pool and how
// much of it there is. The pointer is handed out by the loader and only the
// size comes off the stream (names ours).
//
// AVP2: 0x18 bytes, not NOLF's eight (lithtech.exe 0x0042e8aa..0x0042e8b1
// scales the cursor by 0x18): the size is sixteen bits (0x0042e8ec reads
// two), and a world of version 70 carries three more runs per light per
// frame, each m_n14 bytes long and each a slice of the same byte pool
// (0x0042e97b..0x0042ea83); any other version leaves the three null.
class CLAFrameEntry
{
public:
	void*  m_pData;                          // 0x00
	uint16 m_nSize;                          // 0x04
	uint8  m_pad_06[0x08 - 0x06];
	uint8* m_p08;                            // 0x08
	uint8* m_p0c;                            // 0x0c
	uint8* m_p10;                            // 0x10
	uint8  m_n14;                            // 0x14  the length of all three
	uint8  m_pad_15[0x18 - 0x15];
};

// The light-animation record as the loader fills it in. servermgr.h's
// LightAnim names the same 0x60 bytes from the interface's side —
// m_bShadowMap at 0x20, m_nFrames at 0x28, m_iFrames at 0x34 and
// m_fPercentBetween at 0x3c — and leaves the three fields the loader writes
// as padding, so the loader's view is spelled out here (names ours).
class CLARecord
{
public:
	char            m_Name[0x20];            // 0x00
	uint32          m_bShadowMap;            // 0x20
	CLAFrameEntry** m_ppFrames;              // 0x24  m_nFrames of them
	// AVP2: eight bits (0x0042e7e8 reads one byte into it); NOLF's was a
	// dword.
	uint8           m_nFrames;               // 0x28
	uint8           m_pad_29[0x2c - 0x29];
	CLALight*       m_pLights;               // 0x2c  m_nLights of them
	// AVP2: sixteen bits (lithtech.exe 0x0042e7f6 reads two bytes into it,
	// 0x0042ecaa loads it as a word); NOLF's was a dword.
	uint16          m_nLights;               // 0x30
	uint8           m_pad_32[0x60 - 0x32];
};

// One record of the run at +0x88: a bounding sphere and the polygons it
// covers. 0x30 bytes, from the stride the rebuild's loop advances by (names
// ours).
class CWMBlock
{
public:
	LTVector         m_vCentre;              // 0x00
	float            m_fRadius;              // 0x0c
	uint8            m_pad_10[0x20 - 0x10];
	WorldPolyOwner** m_ppPolies;             // 0x20
	uint32           m_nPolies;              // 0x24
	uint8            m_pad_28[0x30 - 0x28];
};

// The two runs the sphere rebuild walks, past where worldmodels.h's view of
// CWorldModelData stops (names ours).
class CWMGeom
{
public:
	uint8            m_pad_00[0x88];
	CWMBlock*        m_pBlocks;              // 0x88
	uint32           m_nBlocks;              // 0x8c
	uint8            m_pad_90[0xa0 - 0x90];
	WorldPolyOwner** m_ppPolies;             // 0xa0
	uint32           m_nPolies;              // 0xa4
};

// The same object CWorldModelList above is a view of, from the side its
// teardown reaches: five growable arrays at the front. Its four virtuals are
// the vtable at 0x1004d330 — the by-name lookup at 0x1000a268 and the three
// searches above it — so the four bytes at the front are the vptr.
class CWorldData
{
public:
	// Slot 0 of that vtable: the animation of a given name, and optionally
	// its index (name ours). Defined below TermArrays, which it follows in
	// the image.
	virtual class CLARecord* FindLightAnim(char* pName, uint32* piAnim);

	void     TermArrays();                   // 0x1000a226
	LTRESULT LoadObjects(class ILTStream* pStream);   // 0x10009f82
#ifndef LT_SERVER_BUILD
	// AVP2: lithtech.exe 0x0042eca0, defined below FindLightAnim (name
	// ours). The client's only: server.dll has no `orb $0xc0,0x15(...)`.
	void     sub_42eca0Q517(class CLARecord* pAnim);
#endif

	CWDArray04 m_Anims;                      // 0x04
	CWDArray18 m_Lights;                     // 0x18
	CWDArray2c m_Frames;                     // 0x2c
	CWDArray40 m_Data;                       // 0x40
	CWDArray54 m_FrameEntries;               // 0x54
	// Where in the file the light-animation block starts.
	uint32     m_nObjectsOffset;             // 0x68
	uint8      m_pad_6c[0x180 - 0x6c];
	uint32     m_Flags180;                   // 0x180
	uint8      m_pad_184[0x18c - 0x184];
	// The world-model list, as CWorldModelHost below sees it.
	CWorldModelEntry** m_ppEntries;          // 0x18c
	uint8      m_pad_190[0x1ec - 0x190];
	// AVP2: the world file's version, which decides whether the light
	// animations carry their extra runs (`cmpl $0x46,0x1ec(%ebx)`,
	// lithtech.exe 0x0042e94c; server.dll and AvP2 1.0.9.6 too). Name ours.
	uint32     m_nVersion;                   // 0x1ec
};

// Bit 2 of that word: the level has a "LightAnim_BASE" animation, which is
// the one every other animation is blended against (name ours).
#define WD_HAS_BASEANIM 0x04

// 0x1000a226. Five resizes to nothing, in layout order — so this is not the
// compiler's destructor, which would run the members back to front, but a
// written-out teardown. Nothing restores a vtable, in the object or in any
// of the five, which says the arrays' own destructors are not what is being
// called here either.
//
// lithtech.exe (0x00425420, identical) expands all five resizes, so what
// TermArrays calls is an in-class resize reached through a view of the array.
// /O2 expands it; server.dll's /O1 declines it and keeps the five calls (their
// targets are masked, so 0x1000a226 still matches). The first four keep
// _DeleteAndDestroyArray out of line (thiscall 0x00413050, n + cache) and only
// clear the count; the eight-byte fifth has it expanded to BaseDelete
// (cdecl 0x004152b0) and clears the pointer and the cache inside the test.
// LoadObjects matches on both images with the out-of-line SetSize, so the
// arrays' own declarations are left alone.
class CWDTermView
{
public:
	void _DeleteAndDestroyArray(CMoAllocator* pAlloc, uint32 nElements);   // 0x00413050
	void* _AllocateTArray(uint32 nElements, CMoAllocator* pAlloc);

	LTBOOL SetSize(uint32 nItems, CMoAllocator* pAlloc)
	{
		if (nItems == m_nItems && nItems)
			return LTTRUE;

		if (m_pItems)
			_DeleteAndDestroyArray(pAlloc, m_nItems + m_nCache);

		m_nItems = nItems;

		if (nItems > 0)
		{
			m_pItems = _AllocateTArray(nItems + m_nGrowBy, pAlloc);

			if (!m_pItems)
			{
				m_nItems = 0;
				return LTFALSE;
			}

			m_nCache = m_nGrowBy;
		}

		return LTTRUE;
	}

	uint8  m_pad_00[0x04];
	void*  m_pItems;                         // 0x04
	uint32 m_nItems;                         // 0x08
	uint32 m_nCache;                         // 0x0c
	uint32 m_nGrowBy;                        // 0x10
};

void wd_BaseDelete(CMoAllocator* pAlloc, void* pItems, uint32 nItems);   // 0x004152b0

class CWDTermView54
{
public:
	void* _AllocateTArray(uint32 nElements, CMoAllocator* pAlloc);

	LTBOOL SetSize(uint32 nItems, CMoAllocator* pAlloc)
	{
		if (nItems == m_nItems && nItems)
			return LTTRUE;

		if (m_pItems)
		{
			wd_BaseDelete(pAlloc, m_pItems, m_nItems + m_nCache);
			m_pItems = LTNULL;
			m_nCache = 0;
		}

		m_nItems = nItems;

		if (nItems > 0)
		{
			m_pItems = _AllocateTArray(nItems + m_nGrowBy, pAlloc);

			if (!m_pItems)
			{
				m_nItems = 0;
				return LTFALSE;
			}

			m_nCache = m_nGrowBy;
		}

		return LTTRUE;
	}

	uint8  m_pad_00[0x04];
	void*  m_pItems;                         // 0x04
	uint32 m_nItems;                         // 0x08
	uint32 m_nCache;                         // 0x0c
	uint32 m_nGrowBy;                        // 0x10
};

// server.dll's five calls reach exactly the five resizes LoadObjects below
// reaches (0x1000c267, 0x1000c470 twice, 0x1000be59, 0x1002ccee), so the
// server build calls the arrays' own SetSize; the views exist only for the
// /O2 image, whose expansion they model. Same one-source-two-budgets split
// as commonmodel.cpp's m_Worlds.SetSizeInit2 / SetSizeInit2Inline. Left
// unexpanded, the views' SetSize would be emitted into server.dll as a
// function the shipped image does not have, calling helpers it has only
// under the template's names.
void CWorldData::TermArrays()
{
#ifdef LT_SERVER_BUILD
	m_Anims.SetSize(0, &g_MoAlloc);
	m_Lights.SetSize(0, &g_MoAlloc);
	m_Frames.SetSize(0, &g_MoAlloc);
	m_Data.SetSize(0, &g_MoAlloc);
	m_FrameEntries.SetSize(0, &g_MoAlloc);
#else
	((CWDTermView*)&m_Anims)->SetSize(0, &g_MoAlloc);
	((CWDTermView*)&m_Lights)->SetSize(0, &g_MoAlloc);
	((CWDTermView*)&m_Frames)->SetSize(0, &g_MoAlloc);
	((CWDTermView*)&m_Data)->SetSize(0, &g_MoAlloc);
	((CWDTermView54*)&m_FrameEntries)->SetSize(0, &g_MoAlloc);
#endif
}

// 0x1000a268 — slot 0 of the vtable: the light animation of a given name,
// compared case-insensitively (stricmp, 0x10047080), and its index through
// piAnim when the caller wants it.
CLARecord* CWorldData::FindLightAnim(char* pName, uint32* piAnim)
{
	uint32 i;

	for (i = 0; i < m_Anims.m_nItems; i++)
	{
		CLARecord* pAnim = &m_Anims.m_pItems[i];

		if (stricmp(pAnim->m_Name, pName) == 0)
		{
			if (piAnim)
				*piAnim = i;

			return pAnim;
		}
	}

	return LTNULL;
}

#ifndef LT_SERVER_BUILD
// AVP2: lithtech.exe 0x0042eca0, thiscall, its one caller 0x004051bb (after
// an animation's frame has been worked out). Every light of the animation
// names a world model and one of its polygons -- the pair the loader reads
// as two words -- and that polygon gets 0xc0 set in its flags byte at +0x15.
// The walk is by pointer to one past the end (`lea (%eax,%esi,4),%esi`,
// `cmp %esi,%eax`). Not in server.dll (name ours).
//
// NEAR, 70/70 bytes and 25/25 instructions: the list base is %edi here and
// %ebx in the shipped code, nothing else. Unmoved by initialising the cursor
// at its declaration and by the geometry as a named local.
void CWorldData::sub_42eca0Q517(CLARecord* pAnim)
{
	CLALight* pLight;
	CLALight* pEnd = &pAnim->m_pLights[pAnim->m_nLights];

	for (pLight = pAnim->m_pLights; pLight != pEnd; pLight++)
	{
		((CWMGeom*)m_ppEntries[pLight->m_n00]->m_pData)->
		    m_ppPolies[pLight->m_n02]->m_Flags15 |= 0xc0;
	}
}
#endif


// The regular grid both world objects keep, and its cells. worldbsp.h models
// the same object as CWorldSub108 with the same offsets; the cell's count is
// split into two words here, which the loader below is what establishes.
class CSubCell
{
public:
	void*  m_pData;                          // 0x00  m_nCount records of six
	                                         //       bytes each
	uint16 m_nCount;                         // 0x04
	uint16 m_n06;                            // 0x06
};

class CSubGrid
{
public:
	void   Term();                           // 0x1000a63a
	LTBOOL Load(class ILTStream* pStream);   // 0x1000a663

	LTVector  m_vCellSize;                   // 0x00
	LTVector  m_vOrigin;                     // 0x0c
	int       m_nCountX;                     // 0x18
	int       m_nCountY;                     // 0x1c
	int       m_nCountZ;                     // 0x20
	int       m_nSliceStride;                // 0x24
	uint32    m_nCells;                      // 0x28
	CSubCell* m_pCells;                      // 0x2c
};

// 0x1000a663. The three counts come off the stream first, because the cell
// run is sized from them; the two vectors follow, in the other order from
// the layout. Then one pass over the cells, each of which carries a count
// and a run of six-byte records it owns.
//
// Two details are the shipped code's and not a reading: the cell run is only
// allocated when there is at least one cell, and the run of records is read
// whether or not the count was zero — so a cell with no records still gets a
// zero-length read through whatever m_pData held, which for a cleared run is
// null.
LTBOOL CSubGrid::Load(ILTStream* pStream)
{
	uint32 i;

	Term();

	pStream->Read(&m_nCountX, 0x0c);

	m_nSliceStride = m_nCountX * m_nCountY;
	m_nCells       = m_nCountZ * m_nCountX * m_nCountY;

	if (m_nCells)
	{
		m_pCells = (CSubCell*)dalloc_z(m_nCells * sizeof(CSubCell));

		if (!m_pCells)
		{
			Term();

			return LTFALSE;
		}
	}

	pStream->Read(&m_vCellSize, 0x0c);
	pStream->Read(&m_vOrigin, 0x0c);

	for (i = 0; i < m_nCells; i++)
	{
		CSubCell* pCell = &m_pCells[i];

		pStream->Read(&pCell->m_nCount, 2);
		pStream->Read(&pCell->m_n06, 2);

		if (pCell->m_nCount > 0)
		{
			pCell->m_pData = dalloc(pCell->m_nCount * 6);

			if (!pCell->m_pData)
			{
				Term();

				return LTFALSE;
			}
		}

		pStream->Read(pCell->m_pData, pCell->m_nCount * 6);
	}

	return LTTRUE;
}


// 0x1000a411. Not a "reset" in the sense of restoring anything: it rebuilds
// every bounding sphere in one world model's geometry from the vertex
// positions, first for the blocks at +0x88 and then for the polygons at
// +0xa0. Both halves are the same three passes written out twice — sum the
// positions and divide by the count for the centre, then walk the same
// vertices again taking the furthest as the radius — and both end by padding
// the radius, by five units for a block and by a fifth of a unit for a
// polygon.
//
// A block's centre divides by the *total* vertex count over all its
// polygons, so a vertex shared by two of them is counted twice; a polygon's
// divides by its own count. A block with no polygons at all divides by zero.
//
// There is NO CWMGeom* local: the parameter is cast at each use. With
// `CWMGeom* pGeom = (CWMGeom*)pData;` the block address at 0x1000a431 came
// out base-first (`mov 0x88(%ecx),%ebx; add <induction>,%ebx`); shipped
// loads the strength-reduced offset first and adds the member. Casting
// pData in place at EITHER of the first loop's two uses (the bound or the
// element address) is enough to flip it; the second loop's uses do not
// matter. (Probed without effect before this was found: `&m_pBlocks[i]` vs
// `m_pBlocks + i`, byte-offset casts either side of the `+`, a
// function-scope pBlock, nTotal assigned after Init(), `k, i` declaration
// order, a CMoArray view of +0x84 with operator[]/Get/GetArray.)
//
// The rest of it is exact, and three source facts had to be found for that:
// nTotal is a *signed* int (an unsigned one converts through a 64-bit
// `fildll`, which is nine bytes more), the polygon half's vertex count goes
// into a named float before the three divides (the block half's does not
// need one, because a plain int local is not invalidated by the stores to
// the centre and VC6 commons the conversion itself), and `k` is at function
// scope — sharing one slot across all three vertex loops, which is worth
// twenty of the twenty-two bytes the block-scoped version had wrong.
void wm_ResetGeometry(CWorldModelData* pData)
{
	uint32   i, k;

	for (i = 0; i < ((CWMGeom*)pData)->m_nBlocks; i++)
	{
		CWMBlock* pBlock = &((CWMGeom*)pData)->m_pBlocks[i];
		int       nTotal = 0;
		uint32    j;

		pBlock->m_vCentre.Init();

		for (j = 0; j < pBlock->m_nPolies; j++)
		{
			WorldPolyOwner* pPoly = pBlock->m_ppPolies[j];

			if (pPoly)
			{

				for (k = 0; k < pPoly->m_nVertices; k++)
				{
					pBlock->m_vCentre += *pPoly->m_Vertices[k].m_pPos;

					nTotal++;
				}
			}
		}

#ifdef AVP2_MODERN
		// fixed: wm_ResetGeometry's unguarded divisions and missing null
		// check (RETAIL_BUGS.md). A block with no vertices keeps the zero
		// centre. (The centre still weights a shared vertex once per polygon;
		// the radius below reaches every vertex, so the sphere is a bound
		// either way.)
		if (nTotal)
		{
			pBlock->m_vCentre.x /= (float)nTotal;
			pBlock->m_vCentre.y /= (float)nTotal;
			pBlock->m_vCentre.z /= (float)nTotal;
		}
#else
		pBlock->m_vCentre.x /= (float)nTotal;
		pBlock->m_vCentre.y /= (float)nTotal;
		pBlock->m_vCentre.z /= (float)nTotal;
#endif

		pBlock->m_fRadius = 0.0f;

		for (j = 0; j < pBlock->m_nPolies; j++)
		{
			WorldPolyOwner* pPoly = pBlock->m_ppPolies[j];

			if (pPoly)
			{

				for (k = 0; k < pPoly->m_nVertices; k++)
				{
					float fDist =
					    pBlock->m_vCentre.Dist(*pPoly->m_Vertices[k].m_pPos);

					if (fDist > pBlock->m_fRadius)
						pBlock->m_fRadius = fDist;
				}
			}
		}

		pBlock->m_fRadius += 5.0f;
	}

	for (i = 0; i < ((CWMGeom*)pData)->m_nPolies; i++)
	{
		WorldPolyOwner* pPoly = ((CWMGeom*)pData)->m_ppPolies[i];
#ifdef AVP2_MODERN
		LTVector*       pCentre;
		float           fCount;

		// The block half skips a null polygon; so does this one now, and a
		// polygon with no vertices.
		if (!pPoly || !pPoly->m_nVertices)
			continue;

		pCentre = &pPoly->m_vCentre;
#else
		LTVector*       pCentre = &pPoly->m_vCentre;
		float           fCount;
#endif

		pCentre->Init();

		for (k = 0; k < pPoly->m_nVertices; k++)
			*pCentre += *pPoly->m_Vertices[k].m_pPos;

		fCount = (float)pPoly->m_nVertices;

		pCentre->x /= fCount;
		pCentre->y /= fCount;
		pCentre->z /= fCount;

		pPoly->m_fRadius = 0.0f;

		for (k = 0; k < pPoly->m_nVertices; k++)
		{
			float fDist = pCentre->Dist(*pPoly->m_Vertices[k].m_pPos);

			if (fDist > pPoly->m_fRadius)
				pPoly->m_fRadius = fDist;
		}

		pPoly->m_fRadius += 0.2f;
	}
}


// ---- attaching a world model to an object ----------------------------- //

// 0x10007aa0 — the world's world-model list, by name: a linear walk over the
// entries comparing the name each one's geometry carries at +0x04. Lives in
// the world loader's run (name ours).
CWorldModelEntry* wd_FindWorldModel(class CWorldModelHost* pHost, char* pName);

// One instance of a world model, out of the run at the geometry's +0x13c.
// 192 bytes, from the `lea (%eax,%eax,2); shl $0x6` the index goes through,
// and only the record at +0x48 is reached — whatever the geometry source
// below is asked to turn into a live object (names ours).
class CWMInstance
{
public:
	uint8 m_pad_00[0x48];
	uint8 m_Data48[0xc0 - 0x48];             // 0x48
};

// The same world model worldmodels.h's CWorldModelData is a view of, from the
// side the attach reaches: the name is inside the record and the instances
// hang off the far end of it (names ours).
class CWMNamedGeom
{
public:
	uint8        m_pad_00[0x04];
	char         m_Name[0x48 - 0x04];        // 0x04
	uint8        m_pad_48[0x13c - 0x48];
	CWMInstance* m_pInstances;               // 0x13c
	uint32       m_nInstances;               // 0x140
};

// What the geometry source hands back: a live piece of world with a bounding
// box in it. 0x100397ec copies +0x18 and +0x24 out, so those are the box's
// two corners (names ours).
class CWMBounded
{
public:
	void GetBounds(LTVector* pMin, LTVector* pMax);   // 0x100397ec
};

// The interface the world keeps at its own +0x6c. Slot zero turns one
// instance record into something with bounds; the call is `mov 0x6c(%edi),%eax
// / lea 0x6c(%edi),%ecx`, which is a virtual on a base at that offset rather
// than a member object (names ours).
class IWMGeomSource
{
public:
	virtual CWMBounded* MakeInstance(void* pData) = 0;
};

// The first 0x6c bytes of the world, which are somebody else's: the five
// growable arrays CWorldData::TermArrays takes down, behind the world's own
// vptr.
class CWorldDataArrays
{
public:
	virtual void _v00() = 0;

	uint8 m_pad_04[0x6c - 0x04];
};

// The world, as the attach sees it: the arrays, the geometry source at +0x6c,
// and the world-model list at +0x18c that CWorldModelList is the other view
// of (names ours).
class CWorldModelHost : public CWorldDataArrays, public IWMGeomSource
{
public:
	int AttachWorldModel(class LTObject* pObj, char* pName);   // 0x10009e31

public:
	uint8              m_pad_70[0x18c - 0x70];
	CWorldModelEntry** m_ppEntries;          // 0x18c
	uint32             m_nEntries;           // 0x190
};

// 0x10007aa0 — the world's world-model list, by name. A plain indexed walk;
// the name each entry carries is inside its geometry, four bytes in, which is
// why the string pointer is built with an `add $0x4` off the loaded record
// rather than read out of the entry. The zero materialised for the index also
// serves the loop's entry guard (`cmp %ebx,0x190(%esi)`), and the hit is sunk
// past the epilogue because the `return LTNULL` is the fall-through.
CWorldModelEntry* wd_FindWorldModel(CWorldModelHost* pHost, char* pName)
{
	uint32            i;
	CWorldModelEntry* pEntry;

	for (i = 0; i < pHost->m_nEntries; i++)
	{
		pEntry = pHost->m_ppEntries[i];

		if (stricmp(((CWMNamedGeom*)pEntry->m_pData)->m_Name, pName) == 0)
			return pEntry;
	}

	return LTNULL;
}

// 0x10024e2e — point an object at a piece of world. The third field takes
// whichever of the two is not null, preferring the original (name ours).
// ltobject_internal.h names the three fields; the class is local here so
// that this file does not have to pull the object header in.
class CObjBspSetter
{
public:
	void SetWorldBsp(void* pBsp, void* pOriginal);   // 0x10024e2e

	uint8 m_pad_000[0x1b0];
	void* m_pWorldBsp;                       // 0x1b0
	void* m_pOriginalBsp;                    // 0x1b4
	void* m_pWMIface;                        // 0x1b8
};

// The third store is a conditional *expression*, not a statement: the value
// goes through %eax, so the fallback is `mov %edx,%eax` inside a jumped-over
// two-byte block rather than a second store.
void CObjBspSetter::SetWorldBsp(void* pBsp, void* pOriginal)
{
	m_pWorldBsp    = pBsp;
	m_pOriginalBsp = pOriginal;
	m_pWMIface     = pOriginal ? pOriginal : pBsp;
}

// Hang a world model's geometry on an object, by name.
//
// Two kinds of name. A plain one names an entry in the world's own list, and
// the entry's two pieces of geometry go straight onto the object. A name that
// *starts* with "#$#" is an instance reference — "#$# <index> <name>" — and
// the geometry is one record out of the named model's instance run, turned
// into a live piece of world by the source at the world's +0x6c and then
// measured, so that the object ends up the size and in the place of the
// instance rather than of the model.
//
// The prefix is a writable `static char[]`: its length is measured at run
// time (`call strlen` on the same pointer strstr was handed), which a literal
// would fold to three, and its address is an immediate in .data.
//
// The parsed index lives in the dead pName parameter slot — pName itself is
// in a register from the first instruction — which is why `lea 0xc(%ebp)`
// appears as an out-parameter address.
int CWorldModelHost::AttachWorldModel(LTObject* pObj, char* pName)
{
	static char sPrefix[] = "#$#";

	char              szName[128];
	uint32            nIndex;
	CWorldModelEntry* pEntry;
	CWMNamedGeom*     pGeom;
	CWMInstance*      pInstance;
	CWMBounded*       pBounded;
	LTVector          vMin, vMax, vDims;

	if (strstr(pName, sPrefix) == pName)
	{
		sscanf(pName + strlen(sPrefix), "%d %s", &nIndex, szName);

		pEntry = wd_FindWorldModel(this, szName);

		if (pEntry)
		{
			pGeom = (CWMNamedGeom*)pEntry->m_pData;

			if (nIndex < pGeom->m_nInstances)
			{
				pInstance = &pGeom->m_pInstances[nIndex];

				((CObjBspSetter*)pObj)->SetWorldBsp(pInstance, LTNULL);

				pBounded = MakeInstance(pInstance->m_Data48);

				if (pBounded)
				{
					pBounded->GetBounds(&vMin, &vMax);

					vDims = (vMax - vMin) * 0.5f;

					((CObjDimsSetter*)pObj)->SetDims(vDims);
					pObj->SetPos(vMin + vDims);

					return 1;
				}
			}
		}
	}
	else
	{
		pEntry = wd_FindWorldModel(this, pName);

		if (pEntry)
		{
			((CObjBspSetter*)pObj)->SetWorldBsp(pEntry->m_pData,
			                                    pEntry->m_pOriginal);

			return 1;
		}
	}

	return 0;
}


// The level's baked light animations, read in one pass. The five counts come
// off the stream first and size the five arrays; every animation then takes
// its own span out of each of them, and the spans are handed out by running
// cursors rather than allocated one at a time — which is why every one of
// them is bounds-checked against its array's count as it is advanced. A file
// that claims more than it declared is refused, and the arrays go with it.
//
// The nesting is: one animation carries m_nLights lights and m_nFrames
// frames; each frame carries one span of baked data per light; and each span
// is a slice of the one byte pool. So the innermost loop runs
// m_nFrames * m_nLights times per animation and reads a size and then that
// many bytes.
//
// Two things NOLF's shipped bytes decide. The light pair is read as two
// two-byte reads rather than one four-byte read, so it is two sixteen-bit
// fields. And the base animation's frame pair and blend percentage are
// cleared here rather than by whatever made the record, which is what the
// flag at +0x180 is set to say.
//
// AVP2: lithtech.exe 0x0042e690 (1274 bytes) is byte-identical: a frame
// count of one byte, a light count and a span size of two, the version-70
// runs, and the base animation's blend cleared as a dword (`mov %edi,
// 0x3c(%eax)`, 0x0042eb65 -- server.dll 0x1000d4f7 stores the same register,
// so it is not a float there either). Two shapes are load-bearing on the
// client: the record's address goes through `pWorld`, a copy of `this`
// (with plain m_Anims, 0x0042e7a5 loads the array before the strength-
// reduced offset; the same trick as worldbsp.cpp's CWorldSimple::Load), and
// each k-loop declares its own pEntry. NEAR on server.dll 0x1000d248, 1046
// of 1046 bytes: two frame slots swapped (-0x10 and -0xc, the cached
// &m_nLights and the first loop's pEntry). One pEntry for the whole frame
// fixes the slots and loses the client's add order, in both images.
LTRESULT CWorldData::LoadObjects(ILTStream* pStream)
{
	uint32 iFrameEntry;
	uint32 iData;
	uint32 iLight;
	uint32 iFrame;
	uint32 i;
	uint32 nAnims, nLights, nFrames, nData, nFrameEntries;
	CWorldData* pWorld = this;              // see the note above

	m_Flags180 &= ~WD_HAS_BASEANIM;

	pStream->SeekTo(m_nObjectsOffset);

	pStream->Read(&nLights, 4);
	pStream->Read(&nFrames, 4);
	pStream->Read(&nData, 4);
	pStream->Read(&nFrameEntries, 4);
	pStream->Read(&nAnims, 4);

	// The five resizes are one `||` chain, which is what sinks the single
	// out-of-memory block past the whole body to the end of the function;
	// written as five separate guards our build hosts it inline after the
	// fifth test and pays a `jne`/`jmp` pair to jump over it.
	if (!m_Anims.SetSize(nAnims, &g_MoAlloc) ||
	    !m_Lights.SetSize(nLights, &g_MoAlloc) ||
	    !m_Frames.SetSize(nFrames, &g_MoAlloc) ||
	    !m_Data.SetSize(nData, &g_MoAlloc) ||
	    !m_FrameEntries.SetSize(nFrameEntries, &g_MoAlloc))
	{
		TermArrays();

		return LT_OUTOFMEMORY;
	}

	iFrameEntry = 0;
	iData       = 0;
	iLight      = 0;
	iFrame      = 0;

	for (i = 0; i < nAnims; i++)
	{
		CLARecord* pAnim = &pWorld->m_Anims.m_pItems[i];
		uint32     j, k, n;


		pStream->ReadString(pAnim->m_Name, sizeof(pAnim->m_Name));

		pAnim->m_ppFrames = &m_Frames.m_ppItems[iFrame];
		pAnim->m_pLights  = &m_Lights.m_pItems[iLight];

		pStream->Read(&pAnim->m_bShadowMap, 4);
		pStream->Read(&pAnim->m_nFrames, 1);
		pStream->Read(&pAnim->m_nLights, 2);

		iFrame += pAnim->m_nFrames;
		iLight += pAnim->m_nLights;

		if (iFrame > m_Frames.m_nItems)
		{
			TermArrays();

			return LT_ERROR;
		}

		if (iLight > m_Lights.m_nItems)
		{
			TermArrays();

			return LT_ERROR;
		}

		for (k = 0; k < pAnim->m_nLights; k++)
		{
			pStream->Read(&pAnim->m_pLights[k].m_n00, 2);
			pStream->Read(&pAnim->m_pLights[k].m_n02, 2);
		}

		for (j = 0; j < pAnim->m_nFrames; j++)
		{
			CLAFrameEntry** ppFrame = &pAnim->m_ppFrames[j];

			*ppFrame = &m_FrameEntries.m_pItems[iFrameEntry];

			iFrameEntry += pAnim->m_nLights;

			if (iFrameEntry > m_FrameEntries.m_nItems)
			{
				TermArrays();

				return LT_ERROR;
			}

			for (k = 0; k < pAnim->m_nLights; k++)
			{
				CLAFrameEntry* pEntry = &(*ppFrame)[k];

				pEntry->m_pData = m_Data.m_pItems + iData;

				pStream->Read(&pEntry->m_nSize, 2);

				iData += pEntry->m_nSize;

				if (iData > m_Data.m_nItems)
				{
					TermArrays();

					return LT_ERROR;
				}

				pStream->Read(pEntry->m_pData, pEntry->m_nSize);
			}

			// AVP2: version 70's three extra runs, read a byte at a time.
			if (m_nVersion == 0x46)
			{
				for (k = 0; k < pAnim->m_nLights; k++)
				{
					CLAFrameEntry* pEntry = &(*ppFrame)[k];

					pStream->Read(&pEntry->m_n14, 1);

					pEntry->m_p08 = m_Data.m_pItems + iData;

					iData += pEntry->m_n14;

					if (iData > m_Data.m_nItems)
					{
						TermArrays();

						return LT_ERROR;
					}

					for (n = 0; n < pEntry->m_n14; n++)
						pStream->Read(&pEntry->m_p08[n], 1);

					pEntry->m_p0c = m_Data.m_pItems + iData;

					iData += pEntry->m_n14;

					if (iData > m_Data.m_nItems)
					{
						TermArrays();

						return LT_ERROR;
					}

					for (n = 0; n < pEntry->m_n14; n++)
						pStream->Read(&pEntry->m_p0c[n], 1);

					pEntry->m_p10 = m_Data.m_pItems + iData;

					iData += pEntry->m_n14;

					if (iData > m_Data.m_nItems)
					{
						TermArrays();

						return LT_ERROR;
					}

					for (n = 0; n < pEntry->m_n14; n++)
						pStream->Read(&pEntry->m_p10[n], 1);
				}
			}
			else
			{
				for (k = 0; k < pAnim->m_nLights; k++)
				{
					CLAFrameEntry* pEntry = &(*ppFrame)[k];

					pEntry->m_p08 = LTNULL;
					pEntry->m_p0c = LTNULL;
					pEntry->m_p10 = LTNULL;
					pEntry->m_n14 = 0;
				}
			}
		}
	}

	if (pStream->ErrorStatus())
	{
		TermArrays();

		return LT_ERROR;
	}

	{
		CLARecord* pBase = FindLightAnim("LightAnim_BASE", LTNULL);

		if (pBase)
		{
			m_Flags180 |= WD_HAS_BASEANIM;

			((uint32*)pBase)[0x38 / 4] = 0;
			((uint32*)pBase)[0x34 / 4] = 0;
			((uint32*)pBase)[0x3c / 4] = 0;
		}
	}

	return LT_OK;
}
