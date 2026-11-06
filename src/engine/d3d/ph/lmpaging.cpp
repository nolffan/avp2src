// *********************************************************************** //
//  ph/lmpaging.cpp -- Primal Hunt's d3d.ren, TU38 (0x10034df0..0x10035611),
//  /O1.
//
//  Lightmap paging ("Lightmaps paged in"): the world's polygon lightmaps
//  packed into 64x64 pages, each page's space kept as a bitmap of 4x4
//  blocks.  .bss 0x1007f180..0x1008058c.
//
//  Not identical yet: the space search 0x10034df0 (its loops' registers),
//  the per-polygon pager 0x1003508d and the world pager 0x10035389 (two
//  loop counters' frame slots swapped).  0x10035389 calls TU37's
//  lightmap upload 0x10034000, not written yet.
// *********************************************************************** //

#include <windows.h>
#include <mmsystem.h>
#include <ddraw.h>
#include <string.h>

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
	uint8			*m_pBits;		// +0x0c, the allocation map (4x4 blocks)
	uint32			m_nUsed;		// +0x10, pixels used
	uint32			m_TextureSize;	// +0x14, bytes of texture memory
	uint32			m_Unused18;
	LPDIRECTDRAWSURFACE7	m_pSurface;	// +0x1c
	LTBOOL			m_bDirty;		// +0x20
	CLMPage			*m_pNext;		// +0x24
};


#define LMPAGE_SIZE			64

// Is the 4x4 block holding (x, y) of a page taken?
#define LMPAGE_BIT(x, y)	((((y) >> 2) << 6) + ((x) >> 2))
#define LMPAGE_SET(pBits, x, y)	((pBits)[LMPAGE_BIT(x, y) >> 3] |= (1 << (LMPAGE_BIT(x, y) & 7)))
#define LMPAGE_TEST(pBits, x, y)	((pBits)[LMPAGE_BIT(x, y) >> 3] & (1 << (LMPAGE_BIT(x, y) & 7)))

struct D3DTextureFormat
{
	LTLink			m_Link;			// +0x00
	DDPIXELFORMAT	m_PF;			// +0x0c
	uint32			m_PixelShift;	// +0x2c
	uint32			m_PixelSize;	// +0x30
};

struct RenderStruct
{
	uint8			m_Pad[0x50];
	uint32			m_SystemTexMemory;	// +0x50
};

class WorldData;

// The world's lightmap pages.
struct LMPager
{
	CLMPage			*m_pPages;		// +0x00
	uint32			m_nPages;		// +0x04
	WorldData		*m_pWorldData;	// +0x08
};

extern LPDIRECTDRAW7	g_pDirectDraw;				// 0x1005d2a8
extern RenderStruct*	g_pStruct;					// 0x1005df08
extern uint32			g_TextureStage;				// 0x100622d0

void*	dalloc_z(uint32 size);								// 0x10013a7f
void	dfree(void *ptr);									// 0x10013ac5
void	AddDebugMessage(int32 debugLevel, const char* pMsg, ...);	// 0x10013b13
D3DTextureFormat* d3d_GetLightmapFormat();					// 0x1001f410

uint32	g_LMPageMemory;								// 0x1007f180
uint32	g_LMTexelMemory;							// 0x1007f184
uint32	g_LMPageCount;								// 0x1007f188


// 0x10034df0 -- find room for a width x height lightmap in one of the
// pages, scanning in 4-pixel steps.
// RETAIL BUG: a page is skipped when its USED pixel count is below the
// request's size, so a fresh page is never searched until something else
// has been put on it; most lightmaps open a new page.
static LTBOOL d3d_FindLMSpace(CLMPage **ppPages, uint32 width, uint32 height,
	int *pX, int *pY, CLMPage **ppPage)
{
	CLMPage *pPage;
	uint32 x, y, testX, testY, size;
	LTBOOL bFits;

	size = width * height;
	for(pPage=*ppPages; pPage; pPage=pPage->m_pNext)
	{
		if(pPage->m_nUsed < size)
			continue;

		for(y=0; y < (LMPAGE_SIZE+1) - height; y += 4)
		{
			for(x=0; x < (LMPAGE_SIZE+1) - width; x += 4)
			{
				if(LMPAGE_TEST(pPage->m_pBits, x, y))
					continue;

				bFits = LTTRUE;
				for(testX=0; testX < width; testX += 4)
				{
					for(testY=0; testY < height; testY += 4)
					{
						if(LMPAGE_TEST(pPage->m_pBits, x+testX, y+testY))
						{
							bFits = LTFALSE;
							break;
						}
					}

					if(!bFits)
						break;
				}

				if(bFits)
				{
					*pX = x;
					*pY = y;
					*ppPage = pPage;
					return LTTRUE;
				}
			}
		}
	}

	return LTFALSE;
}


// 0x10034f32 -- add a new page to the world's list.
// RETAIL BUG: with no lightmap texture format, the page is freed but its
// bitmap leaks.
static CLMPage* d3d_AddLMPage(LMPager *pPager)
{
	CLMPage *pPage;
	D3DTextureFormat *pFormat;
	DDSURFACEDESC2 ddsd;

	pPage = new CLMPage;
	if(!pPage)
		return LTNULL;

	pPage->m_pBits = (uint8*)dalloc_z((LMPAGE_SIZE * LMPAGE_SIZE) / 32);
	if(!pPage->m_pBits)
	{
		dfree(pPage);
		return LTNULL;
	}

	pFormat = d3d_GetLightmapFormat();
	if(!pFormat)
	{
		dfree(pPage);
		return LTNULL;
	}

	pPage->m_TextureSize = pFormat->m_PixelSize * (LMPAGE_SIZE * LMPAGE_SIZE);

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_TEXTURESTAGE;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_HINTDYNAMIC | DDSCAPS2_DONOTPERSIST;
	ddsd.dwTextureStage = g_TextureStage;
	ddsd.dwWidth = LMPAGE_SIZE;
	ddsd.dwHeight = LMPAGE_SIZE;
	memcpy(&ddsd.ddpfPixelFormat, &pFormat->m_PF, sizeof(DDPIXELFORMAT));

	if(g_pDirectDraw->CreateSurface(&ddsd, &pPage->m_pSurface, LTNULL) != DD_OK)
	{
		dfree(pPage->m_pBits);
		dfree(pPage);
		AddDebugMessage(4, "Unable to create (%dx%d) lightmap page.", LMPAGE_SIZE, LMPAGE_SIZE);
		return LTNULL;
	}

	pPage->m_pNext = pPager->m_pPages;
	pPager->m_nPages++;
	pPager->m_pPages = pPage;

	g_LMPageMemory += LMPAGE_SIZE * LMPAGE_SIZE * 2;
	g_pStruct->m_SystemTexMemory += pPage->m_TextureSize;
	return pPage;
}

// 0x1003503f
CLMPage::CLMPage()
{
	m_Unused04 = 0;
	m_Unused08 = 0;
	m_pBits = LTNULL;
	m_nUsed = 0;
	m_TextureSize = 0;
	m_Unused18 = 0;
	m_pSurface = LTNULL;
	m_pNext = LTNULL;
	m_bDirty = LTTRUE;
}


struct WorldPolyHeader
{
	uint8		m_Pad[0x2c];
	uint16		m_LMFrame;		// +0x2c
	uint8		m_Pad2[0x30 - 0x2e];
};

struct Surface
{
	uint8		m_Pad[0x30];
	uint32		m_Flags;		// +0x30
};


struct SPolyVertex
{
	LTVector	*m_Vec;			// +0x00
	uint8		m_Pad[0xc - 0x4];
	float		m_U, m_V;		// +0x0c, the lightmap coordinates
	uint8		m_Pad2[0x18 - 0x14];
};

struct WorldPoly
{
	uint8		m_Pad[0x14];
	uint16		m_PageFlags;	// +0x14, low six bits: paged; 0x3800: the basis
	uint8		m_Pad16[0x28 - 0x16];
	LTVector	*m_pNormal;		// +0x28
	Surface		*m_pSurface;	// +0x2c
	uint8		m_Pad30[0x38 - 0x30];
	LTVector	m_LMOrigin;		// +0x38
	uint8		m_Pad44[0x48 - 0x44];
	CLMPage		*m_pLMPage;		// +0x48
	uint8		m_LMWidth;		// +0x4c
	uint8		m_LMHeight;		// +0x4d
	uint8		m_LMPageX;		// +0x4e
	uint8		m_LMPageY;		// +0x4f
	SPolyVertex	*m_pVertices;	// +0x50
	uint16		m_nVertices;	// +0x54
	uint16		m_nExtraVertices;	// +0x56
	SPolyVertex	m_Vertices[1];	// +0x58

	uint32		GetNumVertices()
	{
		if(m_pVertices != m_Vertices)
			return m_nVertices + m_nExtraVertices;
		else
			return m_nVertices;
	}
};

class WorldBsp;

// One of the world's BSPs (only its WorldBsp is used here).
struct WorldModelData
{
	uint32		m_Reserved;
	WorldBsp	*m_pBsp;		// +0x04
};

struct LightAnim
{
	uint8			m_Pad[0x28];
	uint8			m_nFrames;		// +0x28
};

class WorldData
{
public:
	virtual LightAnim*	FindLightAnim(const char *pName, uint32 flags);	// +0x00

	uint8			m_Pad[0xf8 - 0x4];
	float			m_LMGridSize;	// +0xf8
	uint8			m_Pad2[0x18c - 0xfc];
	WorldModelData	**m_Models;		// +0x18c
	uint32			m_nModels;		// +0x190
};

struct LMPagerHooks;

class WorldBsp
{
public:
	virtual void		Reserved00();
	virtual void		Reserved04();
	virtual void		Reserved08();
	virtual void		Reserved0c();
	virtual void		Reserved10();
	virtual void		Reserved14();
	virtual void		Reserved18();
	virtual void		Reserved1c();
	virtual void		Reserved20();
	virtual void		Reserved24();
	virtual void		Reserved28();
	virtual void		ForEachLightAnimPoly(LMPagerHooks *pHooks);	// +0x2c

	uint8				m_Pad[0x20 - 0x4];
	WorldPoly			**m_Polies;			// +0x20
	uint32				m_nPolies;			// +0x24
	uint8				m_Pad2[0x84 - 0x28];
	uint32				m_nLightAnims;		// +0x84
	WorldPolyHeader	*m_Headers;			// +0x88
	uint32				m_nHeaders;			// +0x8c
	uint8				m_Pad3[0xa0 - 0x90];
	WorldPoly		**m_LMPolies;		// +0xa0
	uint32				m_nLMPolies;		// +0xa4
};

LMPager *g_pLMPager;						// 0x1008058c

void dfree(void *ptr);
// TU39's: the polygon's lightmap axes, by its basis index.
void d3d_GetLMAxes(int iBasis, LTVector *pNormal, LTVector *pRight, LTVector *pUp);	// 0x10035860


// 0x1003508d -- find room for a polygon's lightmap and map its vertices
// into the page.
LTBOOL d3d_PageInPolyLM(LMPager *pPager, WorldPoly *pPoly)
{
	int x, y;
	uint32 i, j;
	CLMPage *pPage;
	DDSURFACEDESC2 ddsd;
	LTVector right, up, vec;
	SPolyVertex *pVert, *pEnd;
	float scale;

	if(!(pPoly->m_pSurface->m_Flags & SURF_LIGHTMAP))
		return LTTRUE;

	if(!pPoly->m_LMHeight || !pPoly->m_LMWidth)
	{
		pPoly->m_pSurface->m_Flags &= ~SURF_LIGHTMAP;
		return LTTRUE;
	}

	// Already paged in?
	if(pPoly->m_PageFlags & 0x3F)
		return LTTRUE;

	pPoly->m_PageFlags = (pPoly->m_PageFlags & ~0x3F) | 1;

	if(!d3d_FindLMSpace(&pPager->m_pPages, pPoly->m_LMWidth, pPoly->m_LMHeight, &x, &y, &pPage))
	{
		pPage = d3d_AddLMPage(pPager);
		if(!pPage)
			return LTFALSE;

		y = 0;
		x = 0;
	}

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	if(x + pPoly->m_LMWidth > LMPAGE_SIZE || y + pPoly->m_LMHeight > LMPAGE_SIZE)
		return LTFALSE;

	for(j=0; j < pPoly->m_LMHeight; j++)
	{
		for(i=0; i < pPoly->m_LMWidth; i++)
		{
			LMPAGE_SET(pPage->m_pBits, x+i, y+j);
			pPage->m_nUsed++;
		}
	}

	d3d_GetLMAxes((pPoly->m_PageFlags >> 11) & 7, pPoly->m_pNormal, &right, &up);

	scale = 1.0f / LMPAGE_SIZE;
	pEnd = &pPoly->m_Vertices[pPoly->GetNumVertices()];
	for(pVert=pPoly->m_Vertices; pVert != pEnd; pVert++)
	{
		vec = *pVert->m_Vec - pPoly->m_LMOrigin;

		pVert->m_U = right.Dot(vec) / pPager->m_pWorldData->m_LMGridSize + 0.5f;
		pVert->m_U = (pVert->m_U + (float)x) * scale;
		pVert->m_V = (up.Dot(vec) / pPager->m_pWorldData->m_LMGridSize + (float)y) * scale + (0.5f / LMPAGE_SIZE);
	}

	pPoly->m_LMPageX = (uint8)x;
	pPoly->m_LMPageY = (uint8)y;
	g_LMTexelMemory += pPoly->m_LMWidth * pPoly->m_LMHeight * 2;
	pPoly->m_pLMPage = pPage;
	return LTTRUE;
}


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
	uint32			m_Reserved2;
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


// 0x10035575 -- forget a BSP's polygons' pages.
void d3d_ClearLMPages(WorldBsp *pBsp)
{
	uint32 i;

	for(i=0; i < pBsp->m_nLMPolies; i++)
		pBsp->m_LMPolies[i]->m_pLMPage = LTNULL;
}


// 0x1003559e
void d3d_TermRenderWorld(void *pWorld)
{
	LMPager *pPager = (LMPager*)pWorld;
	CLMPage *pPage, *pNext;
	uint32 i;

	d3d_FreeLMPageBits(&pPager->m_pPages);

	for(pPage=pPager->m_pPages; pPage; pPage=pNext)
	{
		pNext = pPage->m_pNext;

		if(pPage->m_pSurface)
		{
			pPage->m_pSurface->Release();
			g_pStruct->m_SystemTexMemory -= pPage->m_TextureSize;
		}

		delete pPage;
	}

	pPager->m_pPages = LTNULL;

	for(i=0; i < pPager->m_pWorldData->m_nModels; i++)
		d3d_ClearLMPages(pPager->m_pWorldData->m_Models[i]->m_pBsp);
}


LTBOOL d3d_UploadPolyLightmap(WorldData *pWorldData, WorldPoly *pPoly, LTBOOL bForce);	// 0x10034000

// 0x10035389 -- page in every lightmap of the world.
LTBOOL d3d_CreateLightmapPages(void *pWorld)
{
	LMPager *pPager = (LMPager*)pWorld;
	WorldData *pWorldData;
	WorldBsp *pBsp;
	LightAnim *pAnim;
	uint32 j, i, startTime;

	pWorldData = pPager->m_pWorldData;
	g_LMPageCount = 0;
	g_LMPageMemory = 0;
	g_LMTexelMemory = 0;

	startTime = timeGetTime();

	for(i=0; i < pWorldData->m_nModels; i++)
		d3d_ResetLMPaging(pWorldData->m_Models[i]->m_pBsp);

	pAnim = pWorldData->FindLightAnim("LightAnim_BASE", 0);
	if(!pAnim || pAnim->m_nFrames < 1)
		return LTFALSE;

	for(i=0; i < pWorldData->m_nModels; i++)
	{
		pBsp = pWorldData->m_Models[i]->m_pBsp;

		if(pBsp->m_nLightAnims > 0)
		{
			for(j=0; j < pBsp->m_nHeaders; j++)
			{
				WorldPolyHeader *pHeader = &pBsp->m_Headers[j];
				LMPagerHooks hooks;

				g_pLMPager = pPager;
				hooks.m_pUser = pHeader;
				hooks.m_Flags = (uint32)pBsp;
				hooks.m_EndFn = (LMPagerFn)d3d_PageInWorldLMs;
				hooks.m_pUser2 = (void*)LMPager_End;
				pBsp->ForEachLightAnimPoly(&hooks);
			}
		}
		else
		{
			for(j=0; j < pBsp->m_nLMPolies; j++)
			{
				if(!d3d_PageInPolyLM(pPager, pBsp->m_LMPolies[j]))
					goto Failed;
			}
		}
	}

	d3d_FreeLMPageBits(&pPager->m_pPages);

	for(i=0; i < pWorldData->m_nModels; i++)
	{
		pBsp = pWorldData->m_Models[i]->m_pBsp;

		for(j=0; j < pBsp->m_nLMPolies; j++)
		{
			if(!(pBsp->m_LMPolies[j]->m_PageFlags & 0x3F))
			{
				pBsp->m_LMPolies[j]->m_pLMPage = LTNULL;
			}
			else if(!d3d_UploadPolyLightmap(pWorldData, pBsp->m_LMPolies[j], LTTRUE))
			{
				goto Failed;
			}
		}
	}

	AddDebugMessage(0, "Lightmaps paged in %.1f seconds.", (float)(timeGetTime() - startTime) * 0.001f);
	return LTTRUE;

Failed:
	d3d_TermRenderWorld(pPager);
	return LTFALSE;
}
