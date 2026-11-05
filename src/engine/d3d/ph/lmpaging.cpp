// *********************************************************************** //
//  ph/lmpaging.cpp -- Primal Hunt's d3d.ren, TU38 (0x10034df0..0x10035611),
//  /O1.
//
//  Lightmap paging ("Lightmaps paged in"): the world's polygon lightmaps
//  packed into 64x64 pages.  .bss 0x1007f180..0x1008058c.
//
//  Written so far: the page texture class (0x1003503f..0x1003508c), the
//  paging callbacks (0x100352e7..0x10035310) and the small walkers
//  0x100352ba, 0x10035311 and 0x10035335.  Not yet: the allocator
//  0x10034df0, 0x10034f32, the per-polygon pager 0x1003508d (and the
//  WorldPoly accessor 0x100352a2 it instantiates), 0x10035389, 0x10035575
//  and 0x1003559e.
// *********************************************************************** //

#include "ltbasedefs.h"


// The texture base class (vtable 0x100483ac; TU19 has its destructor).
class CLMTextureBase
{
public:
	virtual			~CLMTextureBase() {}
	virtual uint32	GetData()=0;
	virtual uint32	GetPitch()=0;
};

// One lightmap page (vtable 0x100484d0).
class CLMPage : public CLMTextureBase
{
public:
					CLMPage();

	virtual uint32	GetData()		{return 0;}		// 0x10035069 (both)
	virtual uint32	GetPitch()		{return 0;}
	virtual uint32	GetWidth()		{return 64;}	// 0x1003506c (both)
	virtual uint32	GetHeight()		{return 64;}

	uint32			m_Unused04;
	uint32			m_Unused08;
	void			*m_pBits;		// +0x0c, the allocation map
	uint32			m_nUsed;		// +0x10
	uint32			m_Unused14;
	uint32			m_Unused18;
	uint32			m_Unused1c;
	LTBOOL			m_bDirty;		// +0x20
	CLMPage			*m_pNext;		// +0x24
};

// 0x1003503f
CLMPage::CLMPage()
{
	m_Unused04 = 0;
	m_Unused08 = 0;
	m_pBits = LTNULL;
	m_nUsed = 0;
	m_Unused14 = 0;
	m_Unused18 = 0;
	m_Unused1c = 0;
	m_pNext = LTNULL;
	m_bDirty = LTTRUE;
}


struct WorldPoly;
struct LMPager;

struct WorldBspPolyHeader
{
	uint8		m_Pad[0x2c];
	uint16		m_LMFrame;		// +0x2c
	uint8		m_Pad2[0x30 - 0x2e];
};

struct WorldBspPoly
{
	uint8		m_Pad[0x14];
	uint16		m_PageFlags;	// +0x14, low six bits
};

struct WorldBsp
{
	uint8				m_Pad[0x20];
	WorldPoly			**m_Polies;			// +0x20
	uint32				m_nPolies;			// +0x24
	uint8				m_Pad2[0x88 - 0x28];
	WorldBspPolyHeader	*m_Headers;			// +0x88
	uint32				m_nHeaders;			// +0x8c
	uint8				m_Pad3[0xa0 - 0x90];
	WorldBspPoly		**m_LMPolies;		// +0xa0
	uint32				m_nLMPolies;		// +0xa4
};

extern LMPager *g_pLMPager;					// 0x1008058c

void dfree(void *ptr);
LTBOOL d3d_PageInPolyLM(LMPager *pPager, WorldPoly *pPoly);	// 0x1003508d


// 0x100352ba
void d3d_PageInWorldLMs(WorldBsp *pBsp)
{
	uint32 i;

	for(i=0; i < pBsp->m_nPolies; i++)
	{
		if(!d3d_PageInPolyLM(g_pLMPager, pBsp->m_Polies[i]))
			break;
	}
}


// The pager's hooks.
typedef void (*LMPagerFn)();
typedef LTBOOL (*LMPagerTestFn)();

// 0x1003530c (the link folds the two)
static void LMPager_Start()
{
}

static void LMPager_End()
{
}

// 0x1003530d
static LTBOOL LMPager_True()
{
	return LTTRUE;
}

struct LMPagerHooks
{
					LMPagerHooks();

	void			*m_pUser;
	uint32			m_Flags;
	LMPagerFn		m_StartFn;
	LMPagerFn		m_EndFn;
	void			*m_pUser2;
	LMPagerTestFn	m_TestFn;
	uint32			m_Reserved;
};

// 0x100352e7
LMPagerHooks::LMPagerHooks()
{
	m_pUser = LTNULL;
	m_Flags = 0;
	m_StartFn = LMPager_Start;
	m_EndFn = LMPager_End;
	m_pUser2 = LTNULL;
	m_TestFn = LMPager_True;
	m_Reserved = 0;
}


// 0x10035311
void d3d_FreeLMPageBits(CLMPage **ppHead)
{
	CLMPage *pPage;

	for(pPage=*ppHead; pPage; pPage=pPage->m_pNext)
	{
		if(pPage->m_pBits)
		{
			dfree(pPage->m_pBits);
			pPage->m_pBits = LTNULL;
		}
	}
}

// 0x10035335
void d3d_ResetLMPaging(WorldBsp *pBsp)
{
	uint32 i;

	for(i=0; i < pBsp->m_nHeaders; i++)
		pBsp->m_Headers[i].m_LMFrame = 0;

	for(i=0; i < pBsp->m_nLMPolies; i++)
		pBsp->m_LMPolies[i]->m_PageFlags &= ~0x3F;
}
