// *********************************************************************** //
//
//  w13_1006c6e0_osoiqip.cpp -- the world's lightmap pages: 0x1006c6e0
//  (create a page), 0x1006c860 (the world-tree node callback), 0x1006c8a0
//  (place one polygon's lightmap on a page), 0x1006cc90
//  (d3d_CreateLightmapPages, called by d3d_CreateWorld at 0x1003319d) and
//  0x1006d130 (d3d_TermRenderWorld, moved here from w12_6d130_os.cpp).
//
//  ONE FILE, AND WHY /Os /Oi- /Qip.
//    * 0x1006c6e0 and 0x1006c8a0 are /Qip two-entry pairs (`mov 4(%esp),%eax`
//      [+ `mov 8(%esp),%edx`] falling into the body at 0x1006c6e4 /
//      0x1006c8a8); their only callers -- 0x1006c8a8 calls 0x1006c6e4 at
//      0x1006ca3a, 0x1006c860 and 0x1006cc90 call 0x1006c8a8 at 0x1006c884
//      and 0x1006ce7f -- enter the register entries, so all of them share
//      this translation unit.
//    * 0x1006cc90's two failure exits (0x1006cf87, 0x1006d056) are
//      d3d_TermRenderWorld's body inlined, and its success path starts with
//      that body's first loop (the pages' pixel data freed): 0x1006d130 is
//      in this TU too, and the loop is a helper both inline.
//    * 0x1006c6e4 calls memset (0x100809a0) and memcpy (0x100805d0), and
//      0x1006c8a8 memset: intrinsics off.  Padding after each body is the
//      /Os shape (`8d 76 00` / 0x90 runs).
//
//  THE DATA (read off the bytes).
//    * the render world: +0x00 page list, +0x04 page count, +0x08 the BSP
//      world (+0xf8 the lightmap grid size, +0x18c / +0x190 the world
//      models; vtable slot 0 finds a light animation by name, slot 1 turns
//      a polygon reference into the polygon).
//    * a world model (+0x04 of each entry): +0x84 its node count, +0x88 /
//      +0x8c the 0x30-byte node array and its size, +0xa0 / +0xa4 the
//      polygons; vtable slot 9 walks the tree under a node with a visitor.
//    * a page (0x28 bytes, vtable 0x10089800): +0x0c a 16x64-bit block map
//      (one bit per 4x4 texels), +0x10 texels placed, +0x14 its memory
//      (0x2000 = 64x64x2), +0x1c the DirectDraw surface, +0x20 dirty,
//      +0x24 next.
//    * a polygon: +0x14 flag word (low six bits: lightmap state), +0x28 its
//      surface (+0x38 flags, 0x80 = lightmapped; vtable slot 0 gives the
//      lightmap's two axis vectors), +0x34 the lightmap origin, +0x44 its
//      page, +0x48 / +0x49 lightmap size, +0x4a / +0x4b its place on the
//      page, +0x4c / +0x50 / +0x52 / +0x54 the vertices (0x18 bytes: +0x00
//      the point, +0x0c / +0x10 the lightmap uv).
//
//  STATUS: written for the d3d.ren link from the shipped bytes (none of
//  these was written before).
// *********************************************************************** //

#include "ltbasedefs.h"

extern "C"
{
	void*	__cdecl memcpy(void* pDst, const void* pSrc, unsigned int nBytes);
	void*	__cdecl memset(void* pDst, int c, unsigned int nBytes);
	__declspec(dllimport) unsigned long __stdcall timeGetTime(void);
}

void* sub_10021c50(uint32 nSize);                       // 0x10021c50 d3d_Alloc
void  sub_10021cb0(void* p);                            // 0x10021cb0 d3d_Free
extern void sub_10021d60(uint32 nLevel, const char* pszMsg, ...);  // 0x10021d60 AddDebugMessage
LTBOOL sub_1006be50(void* pCtxArg, void* pPolyArg, uint32 bForce);  // 0x1006be50 (w8_1006bb30_qip.cpp)

// The visitor's callbacks: 0x1006c670 is a bare `ret` (d3dneutral.cpp),
// 0x1006c680 answers 1 (d3dr4b.cpp).
void  sub_1006c670();                                   // 0x1006c670
int32 sub_1006c680();                                   // 0x1006c680


// ----------------------------------------------------------------------- //
//  DirectDraw, the pieces used.
// ----------------------------------------------------------------------- //
struct DDSurface13;

struct DDSurfaceDesc13                                  // DDSURFACEDESC2
{
	uint32	dwSize;                                     // +0x00
	uint32	dwFlags;                                    // +0x04
	uint32	dwHeight;                                   // +0x08
	uint32	dwWidth;                                    // +0x0c
	uint8	m_pad_10[0x48 - 0x10];
	uint8	ddpfPixelFormat[0x20];                      // +0x48
	uint32	dwCaps;                                     // +0x68
	uint32	dwCaps2;                                    // +0x6c
	uint32	dwCaps3;                                    // +0x70
	uint32	dwCaps4;                                    // +0x74
	uint32	dwTextureStage;                             // +0x78
};

struct DDraw13;
struct DDraw13Vtbl
{
	void*	m_pad_00[6];
	long	(__stdcall* CreateSurface)(DDraw13* pThis, DDSurfaceDesc13* pDesc,
	                                   DDSurface13** ppSurf, void* pOuter);  // +0x18
};
struct DDraw13 { DDraw13Vtbl* lpVtbl; };

struct DDSurface13Vtbl
{
	void*	m_pad_00[2];
	uint32	(__stdcall* Release)(DDSurface13* pThis);   // +0x08
};
struct DDSurface13 { DDSurface13Vtbl* lpVtbl; };

extern DDraw13*	g_pDirectDraw;                          // 0x1008bf74
extern uint32	g_1008d7cc;                             // 0x1008d7cc  the texture stage
extern uint32	g_1008e6c4;                             // 0x1008e6c4  format record; DDPIXELFORMAT at +0x0c

struct RenderStruct6d130
{
	uint8	m_pad_00[0x50];
	uint32	m_SystemTextureMemory;                      // +0x50
};

extern RenderStruct6d130* g_pStruct;                    // 0x1008cbcc


// ----------------------------------------------------------------------- //
//  The world.
// ----------------------------------------------------------------------- //
struct Vec13 { float x, y, z; };

class LMSurface13
{
public:
	virtual void	GetVectors(Vec13* pP, Vec13* pQ);   // slot 0

	uint8	m_pad_04[0x38 - 0x04];
	uint32	m_nFlags38;                                 // +0x38
};

struct LMVert13
{
	Vec13*	m_pPoint;                                   // +0x00
	uint8	m_pad_04[0x08];
	float	m_fU;                                       // +0x0c
	float	m_fV;                                       // +0x10
	uint8	m_pad_14[0x04];
};

struct LMPage13;

struct Poly6d130
{
	uint8			m_pad_00[0x14];
	uint16			m_nFlags14;                         // +0x14
	uint8			m_pad_16[0x28 - 0x16];
	LMSurface13*	m_pSurface28;                       // +0x28
	uint8			m_pad_2c[0x34 - 0x2c];
	Vec13			m_vOrigin34;                        // +0x34
	uint8			m_pad_40[0x04];
	void*			m_pLMPage;                          // +0x44
	uint8			m_nWidth48;                         // +0x48
	uint8			m_nHeight49;                        // +0x49
	uint8			m_nX4a;                             // +0x4a
	uint8			m_nY4b;                             // +0x4b
	LMVert13*		m_pVerts4c;                         // +0x4c
	uint16			m_nVerts50;                         // +0x50
	uint16			m_nExtra52;                         // +0x52
	LMVert13		m_Verts54[1];                       // +0x54
};

struct Node13
{
	uint8			m_pad_00[0x20];
	Poly6d130**		m_pPolies20;                        // +0x20
	uint32			m_nPolies24;                        // +0x24
	uint8			m_pad_28[0x04];
	uint16			m_nStamp2c;                         // +0x2c
	uint8			m_pad_2e[0x30 - 0x2e];
};

class Model6d130;

struct Visitor13
{
	Node13*			m_pNode;                            // +0x00
	Model6d130*		m_pModel;                           // +0x04
	void*			m_pfn08;                            // +0x08
	void*			m_pfn0c;                            // +0x0c  per node
	void*			m_pfn10;                            // +0x10
	void*			m_pfn14;                            // +0x14
	uint32			m_n18;                              // +0x18
};

class Model6d130
{
public:
	virtual void	Slot0();
	virtual void	Slot1();
	virtual void	Slot2();
	virtual void	Slot3();
	virtual void	Slot4();
	virtual void	Slot5();
	virtual void	Slot6();
	virtual void	Slot7();
	virtual void	Slot8();
	virtual void	Traverse(Visitor13* pVisitor);      // slot 9, +0x24

	uint8			m_pad_04[0x84 - 0x04];
	uint32			m_nNodes84;                         // +0x84
	Node13*			m_pNodes88;                         // +0x88
	uint32			m_nNodes8c;                         // +0x8c
	uint8			m_pad_90[0xa0 - 0x90];
	Poly6d130**		m_Polies;                           // +0xa0
	uint32			m_nPolies;                          // +0xa4
};

struct ModelRef6d130
{
	uint8			m_pad_00[0x04];
	Model6d130*		m_pModel;                           // +0x04
};

struct LMFrame13                                        // w8's LMFrameEntry
{
	uint8*			m_pData;                            // +0x00
	uint32			m_nSize;                            // +0x04
};

struct LightAnim13
{
	uint8			m_pad_00[0x24];
	LMFrame13**		m_pFrames24;                        // +0x24
	uint32			m_nFrames28;                        // +0x28
	uint32*			m_pPolyRefs2c;                      // +0x2c
	uint32			m_nPolies30;                        // +0x30
};

class BspWorld6d130
{
public:
	virtual LightAnim13*	FindLightAnim(const char* pszName, uint32 nFlags);  // slot 0
	virtual Poly6d130*		GetPoly(uint32* pRef);                               // slot 1

	uint8			m_pad_004[0xf8 - 0x004];
	float			m_fLMGridSize;                      // +0xf8
	uint8			m_pad_0fc[0x18c - 0x0fc];
	ModelRef6d130**	m_WorldModels;                      // +0x18c
	uint32			m_nWorldModels;                     // +0x190
};


// ----------------------------------------------------------------------- //
//  The pages.
// ----------------------------------------------------------------------- //
// The shipped page vtable (0x10089800: 0x1006c690, 0x1006c6c0 x2,
// 0x1006c6d0 x2), generated into d3ddata.cpp; installed by an explicit
// vptr store, as d3dbatchd.cpp's C1003abe4 does, so icl emits no vtable
// of its own.
extern void* g_vtbl_10089800[];                         // 0x10089800

struct LMPage13
{
	void**			m_pVtbl;                            // +0x00
	uint32			m_n04;                              // +0x04
	uint32			m_n08;                              // +0x08
	uint8*			m_pBlocks;                          // +0x0c
	uint32			m_nUsed;                            // +0x10
	uint32			m_MemUsage;                         // +0x14
	uint32			m_n18;                              // +0x18
	DDSurface13*	m_pSurface;                         // +0x1c
	uint32			m_bDirty;                           // +0x20
	LMPage13*		m_pNext;                            // +0x24

	LMPage13()
	{
		m_pVtbl = g_vtbl_10089800;
		m_n04 = 0;
		m_n08 = 0;
		m_pBlocks = 0;
		m_nUsed = 0;
		m_MemUsage = 0;
		m_n18 = 0;
		m_pSurface = 0;
		m_pNext = 0;
		m_bDirty = 1;
	}
};

// The same object seen through its vtable, for the virtual delete (slot 0,
// flag 1) d3d_TermRenderWorld does.
struct LMPageV13
{
	virtual ~LMPageV13();
};

struct RenderWorld6d130
{
	LMPage13*		m_LMPages;                          // +0x00
	uint32			m_nLMPages;                         // +0x04
	BspWorld6d130*	m_pWorld;                           // +0x08
};

extern RenderWorld6d130*	g_10091940;                 // 0x10091940  the world the node callback places into
extern uint32				g_10091944;                 // 0x10091944
extern uint32				g_10091948;                 // 0x10091948  page memory created
extern uint32				g_1009194c;                 // 0x1009194c  lightmap memory placed

#define LMBLOCK(x, y)		((((y) >> 2) << 6) + ((x) >> 2))
#define LMTEST(p, n)		((p)->m_pBlocks[(n) >> 3] & (1 << ((n) & 7)))


// ======================================================================= //
//  0x1006c6e0 -- a new 64x64 lightmap page on the world's list: the block
//  map, a 64x64 texture surface in the device's lightmap format (texture
//  stage g_1008d7cc), and the memory counters.  Zero on failure (a failed
//  CreateSurface is reported).
// ======================================================================= //
LMPage13* sub_1006c6e0(RenderWorld6d130* pWorld)
{
	LMPage13*		pPage;
	DDSurfaceDesc13	ddsd;

	pPage = new LMPage13;
	if (!pPage)
		return LTNULL;

	pPage->m_pBlocks = (uint8*)sub_10021c50(0x80);
	if (!pPage->m_pBlocks)
	{
		sub_10021cb0(pPage);
		return LTNULL;
	}

	pPage->m_MemUsage = 0x2000;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = 0x101007;
	ddsd.dwWidth = 64;
	ddsd.dwHeight = 64;
	ddsd.dwCaps = 0x1000;
	ddsd.dwCaps2 = 0x40014;
	ddsd.dwTextureStage = g_1008d7cc;
	memcpy(ddsd.ddpfPixelFormat, (void*)(g_1008e6c4 + 0xc), 0x20);

	if (g_pDirectDraw->lpVtbl->CreateSurface(g_pDirectDraw, &ddsd,
	                                         &pPage->m_pSurface, LTNULL) != 0)
	{
		sub_10021cb0(pPage->m_pBlocks);
		sub_10021cb0(pPage);
		sub_10021d60(4, "Unable to create (%dx%d) lightmap page.", 64, 64);
		return LTNULL;
	}

	pPage->m_pNext = pWorld->m_LMPages;
	pWorld->m_nLMPages++;
	pWorld->m_LMPages = pPage;

	g_10091948 += 0x2000;
	g_pStruct->m_SystemTextureMemory += pPage->m_MemUsage;
	return pPage;
}


LTBOOL sub_1006c8a0(RenderWorld6d130* pWorld, Poly6d130* pPoly);

// ======================================================================= //
//  0x1006c860 -- the node callback of 0x1006cc90's tree walk: place each
//  polygon of the node into the world at g_10091940, stopping at the
//  first that fails.
// ======================================================================= //
void sub_1006c860(Node13* pNode)
{
	uint32 i;

	for (i = 0; i < pNode->m_nPolies24; i++)
	{
		if (!sub_1006c8a0(g_10091940, pNode->m_pPolies20[i]))
			return;
	}
}


// ======================================================================= //
//  0x1006c8a0 -- place one lightmapped polygon's lightmap: the first page
//  (whose placed-texel count is at least the lightmap's area) with a free
//  4x4-aligned rectangle, else a new page; mark the texels, then turn each
//  vertex's position into lightmap uv on that page.  A polygon already
//  placed (low six flag bits) or without a lightmap succeeds untouched.
// ======================================================================= //
LTBOOL sub_1006c8a0(RenderWorld6d130* pWorld, Poly6d130* pPoly)
{
	uint32			nWidth, nHeight, nArea;
	LMPage13*		pPage;
	uint32			x, y, x2, y2;
	uint32			i, j;
	uint32			n;
	int32			nX, nY;
	DDSurfaceDesc13	ddsd;
	Vec13			vP, vQ;
	LMVert13*		pVert;
	LMVert13*		pEnd;
	Vec13*			pPt;
	float			dx, dy, dz;

	if (!(pPoly->m_pSurface28->m_nFlags38 & 0x80))
		return LTTRUE;

	if (pPoly->m_nFlags14 & 0x3f)
		return LTTRUE;

	nWidth = pPoly->m_nWidth48;
	nHeight = pPoly->m_nHeight49;
	nArea = nWidth * nHeight;
	pPoly->m_nFlags14 = (pPoly->m_nFlags14 & 0xffc0) | 1;

	for (pPage = pWorld->m_LMPages; pPage; pPage = pPage->m_pNext)
	{
		if (nArea > pPage->m_nUsed)
			continue;

		for (y = 0; y < 65 - nHeight; y += 4)
		{
			for (x = 0; x < 65 - nWidth; x += 4)
			{
				n = LMBLOCK(x, y);
				if (LMTEST(pPage, n))
					continue;

				for (x2 = x; x2 < x + nWidth; x2 += 4)
				{
					for (y2 = y; y2 < y + nHeight; y2 += 4)
					{
						n = LMBLOCK(x2, y2);
						if (LMTEST(pPage, n))
							goto NextX;
					}
				}

				nX = x;
				nY = y;
				goto Found;
NextX:
				;
			}
		}
	}

	pPage = sub_1006c6e0(pWorld);
	if (!pPage)
		return LTFALSE;

	nX = 0;
	nY = 0;

Found:
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	if (nX + pPoly->m_nWidth48 > 64 || nY + pPoly->m_nHeight49 > 64)
		return LTFALSE;

	for (i = 0; i < pPoly->m_nHeight49; i++)
	{
		for (j = 0; j < pPoly->m_nWidth48; j++)
		{
			n = LMBLOCK(nX + j, nY + i);
			pPage->m_pBlocks[n >> 3] |= (1 << (n & 7));
			pPage->m_nUsed++;
		}
	}

	pPoly->m_pSurface28->GetVectors(&vP, &vQ);

	pVert = pPoly->m_Verts54;
	pEnd = &pPoly->m_Verts54[(pPoly->m_pVerts4c == pPoly->m_Verts54)
	                         ? pPoly->m_nVerts50
	                         : pPoly->m_nVerts50 + pPoly->m_nExtra52];
	for (; pVert != pEnd; pVert++)
	{
		pPt = pVert->m_pPoint;
		dz = pPt->z - pPoly->m_vOrigin34.z;
		dy = pPt->y - pPoly->m_vOrigin34.y;
		dx = pPt->x - pPoly->m_vOrigin34.x;

		pVert->m_fU = (dz * vP.z + dy * vP.y + dx * vP.x)
		              / pWorld->m_pWorld->m_fLMGridSize + 0.5f;
		pVert->m_fU += nX;
		pVert->m_fU *= 1.0f / 64.0f;
		pVert->m_fU *= 1.0f;

		pVert->m_fV = (dz * vQ.z + dy * vQ.y + dx * vQ.x)
		              / pWorld->m_pWorld->m_fLMGridSize + 0.5f;
		pVert->m_fV += nY;
		pVert->m_fV *= 1.0f / 64.0f;
		pVert->m_fV *= 1.0f;
	}

	pPoly->m_nX4a = (uint8)nX;
	pPoly->m_nY4b = (uint8)nY;
	g_1009194c += pPoly->m_nWidth48 * 2 * pPoly->m_nHeight49;
	pPoly->m_pLMPage = pPage;
	return LTTRUE;
}


// The pages' block maps are only needed while placing: free them.
static void FreeLMPageBlocks(RenderWorld6d130* pWorld)
{
	LMPage13* pPage;

	for (pPage = pWorld->m_LMPages; pPage; pPage = pPage->m_pNext)
	{
		if (pPage->m_pBlocks)
		{
			sub_10021cb0(pPage->m_pBlocks);
			pPage->m_pBlocks = 0;
		}
	}
}

void sub_1006d130(RenderWorld6d130* pWorld);


// ======================================================================= //
//  0x1006cc90 -- d3d_CreateLightmapPages.  Clear every node's stamp and
//  every polygon's lightmap state; point each polygon of the
//  "LightAnim_BASE" animation at its frame-0 entry; place every polygon
//  (through the tree walk for models with nodes, directly otherwise); free
//  the block maps; build each placed polygon's lightmap (0x1006be50) and
//  report the time taken.  Any failure tears the pages down and answers 0.
//
//  The node-count test is `> 0` (the shipped `test ;
//  jbe`, not `je`); bytes unchanged (690), two mnemonic lines closer.
//  Measured and NOT kept: the surface as a C++ interface deriving from an
//  IUnknown-like base and the page's virtual delete through a derived
//  class give 0x1006d130 the shipped vtable-before-argument
//  order on both virtual calls, but 0x1006d130 123 -> 145 and this
//  function 690 -> 767 (each inlined teardown grows 3 bytes: the zero for
//  the block-map store goes through %edx, and pWorld is held in %esi
//  where shipped uses %edi).  Also unchanged: FreeLMPageBlocks as a
//  while loop, over the list head, or with the clear outside the test.
// ======================================================================= //
LTBOOL sub_1006cc90(RenderWorld6d130* pWorld)
{
	BspWorld6d130*	pBsp;
	uint32			nStart;
	uint32			i, j;
	Model6d130*		pModel;
	LightAnim13*	pAnim;
	LMFrame13**		pFrames;
	Poly6d130*		pPoly;
	Visitor13		visitor;

	pBsp = pWorld->m_pWorld;
	g_10091944 = 0;
	g_10091948 = 0;
	g_1009194c = 0;

	nStart = timeGetTime();

	for (i = 0; i < pBsp->m_nWorldModels; i++)
	{
		pModel = pBsp->m_WorldModels[i]->m_pModel;

		for (j = 0; j < pModel->m_nNodes8c; j++)
			pModel->m_pNodes88[j].m_nStamp2c = 0;

		for (j = 0; j < pModel->m_nPolies; j++)
			pModel->m_Polies[j]->m_nFlags14 &= 0xffc0;
	}

	pAnim = pBsp->FindLightAnim("LightAnim_BASE", 0);
	if (!pAnim || pAnim->m_nFrames28 < 1)
		return LTFALSE;

	pFrames = pAnim->m_pFrames24;
	for (i = 0; i < pAnim->m_nPolies30; i++)
	{
		pPoly = pBsp->GetPoly(&pAnim->m_pPolyRefs2c[i]);
		if (pPoly)
			pPoly->m_pLMPage = &(*pFrames)[i];
	}

	for (i = 0; i < pBsp->m_nWorldModels; i++)
	{
		pModel = pBsp->m_WorldModels[i]->m_pModel;

		if (pModel->m_nNodes84 > 0)
		{
			for (j = 0; j < pModel->m_nNodes8c; j++)
			{
				visitor.m_pNode = &pModel->m_pNodes88[j];
				visitor.m_pModel = pModel;
				visitor.m_pfn08 = (void*)sub_1006c670;
				visitor.m_pfn0c = (void*)sub_1006c860;
				visitor.m_pfn10 = (void*)sub_1006c670;
				visitor.m_pfn14 = (void*)sub_1006c680;
				visitor.m_n18 = 0;
				g_10091940 = pWorld;
				pModel->Traverse(&visitor);
			}
		}
		else
		{
			for (j = 0; j < pModel->m_nPolies; j++)
			{
				if (!sub_1006c8a0(pWorld, pModel->m_Polies[j]))
				{
					sub_1006d130(pWorld);
					return LTFALSE;
				}
			}
		}
	}

	FreeLMPageBlocks(pWorld);

	for (i = 0; i < pBsp->m_nWorldModels; i++)
	{
		pModel = pBsp->m_WorldModels[i]->m_pModel;

		for (j = 0; j < pModel->m_nPolies; j++)
		{
			pPoly = pModel->m_Polies[j];
			if (!(pPoly->m_nFlags14 & 0x3f))
			{
				pPoly->m_pLMPage = LTNULL;
			}
			else if (!sub_1006be50(pBsp, pPoly, 1))
			{
				sub_1006d130(pWorld);
				return LTFALSE;
			}
		}
	}

	sub_10021d60(0, "Lightmaps paged in %.1f seconds.",
	             (timeGetTime() - nStart) * 0.001f);
	return LTTRUE;
}


// ======================================================================= //
//  0x1006d130 -- d3d_TermRenderWorld (d3d_DestroyWorld calls it at
//  0x10032c0f).  Free the block maps; release each page's surface, take
//  its memory back off RenderStruct +0x50 and delete it (virtual, slot 0);
//  empty the list; clear every world-model polygon's page pointer.
//  (Moved from w12_6d130_os.cpp: 0x1006cc90 inlines it, so it is this
//  TU's.  d3ddraw*.cpp's d3d_TermRenderWorld(void*) is aliased onto it.)
// ======================================================================= //
//  Taking the world as `void*` and casting it into a local
//  (the d3ddraw spelling) compiles to the same 214 bytes; pWorld is still
//  rematerialised from 8(%ebp) after the first loop where shipped keeps it
//  in %edi across it.
void sub_1006d130(RenderWorld6d130* pWorld)
{
	LMPage13 *pPage, *pNext;
	uint32 i, j;
	Model6d130* pModel;

	FreeLMPageBlocks(pWorld);

	for (pPage = pWorld->m_LMPages; pPage; pPage = pNext)
	{
		pNext = pPage->m_pNext;
		if (pPage->m_pSurface)
		{
			pPage->m_pSurface->lpVtbl->Release(pPage->m_pSurface);
			g_pStruct->m_SystemTextureMemory -= pPage->m_MemUsage;
		}
		delete (LMPageV13*)pPage;
	}
	pWorld->m_LMPages = 0;

	for (i = 0; i < pWorld->m_pWorld->m_nWorldModels; i++)
	{
		pModel = pWorld->m_pWorld->m_WorldModels[i]->m_pModel;
		for (j = 0; j < pModel->m_nPolies; j++)
			pModel->m_Polies[j]->m_pLMPage = 0;
	}
}
