// *********************************************************************** //
//  ph/lmsurfaces.cpp -- Primal Hunt's d3d.ren, TU39 (0x10035611..0x10035c7e),
//  /O1.
//
//  The lightmap surfaces: the six lightmap bases (a polygon's lightmap
//  axes come from the one its flags pick), and the pools of small
//  square lightmap textures the dynamic and animated lightmaps are drawn
//  through -- one list of video-memory textures and one system-memory
//  scratch texture for each of five sizes.  A lightmap is drawn by locking
//  the scratch texture (LMDrawDesc::Begin), then loaded into the least
//  recently used texture of its size (LMDrawDesc::End).  No string names
//  this object; "lmsurfaces" is a guess.  .bss 0x10080590..0x10081abc.
//
//  Not identical yet: LMDrawDesc::Begin (the size search's registers),
//  the pools' constructor and Reset (retail loads the list pointer ahead
//  of the scratch-array clear), and the empty initialiser 0x10035b45
//  (retail's lists have no constructor call; the object behind that
//  initialiser is not identified).  CLightmapPages::Reset is an ordinary
//  member whose body never reads `this` (it reuses ecx as its loop
//  pointer); d3d_texture.cpp's caller loads it at 0x1001f94f.
// *********************************************************************** //

#include <ddraw.h>
#include <d3d.h>
#include <string.h>

#include "ltbasedefs.h"


#define NUM_LMSIZES		5

// pixelformat.cpp's PFormat, as far as it is used here.
class PFormat
{
public:
	virtual void	Init(uint32 Type, uint32 aMask, uint32 rMask, uint32 gMask, uint32 bMask);

	uint32			m_eType;
	uint32			m_Masks[4];
	uint32			m_nBits[4];
	uint32			m_FirstBits[4];
};

// d3d_texture.cpp's RTexture, as far as it is used here.
struct RTexture
{
	void*					m_pVTable;
	float					m_fInvWidth;                // +0x04
	float					m_fInvHeight;               // +0x08
	LPDIRECTDRAWSURFACE7	m_pSurface;                 // +0x0c
	uint32					m_MemoryUse;                // +0x10
	uint16					m_FrameCode;                // +0x14
	uint16					m_AlphaRef;                 // +0x16
	RTexture*				m_pTexture;                 // +0x18
	LTLink					m_Link;                     // +0x1c
};

struct TexPair
{
	float	u, v;
};

class CLMPage;

struct WorldPoly
{
	uint8		m_Pad[0x48];
	CLMPage		*m_pLMPage;		// +0x48
	uint8		m_LMWidth;		// +0x4c
	uint8		m_LMHeight;		// +0x4d
	uint8		m_LMPageX;		// +0x4e
	uint8		m_LMPageY;		// +0x4f
};

class CLMPage
{
public:
	uint8					m_Pad[0x1c];
	LPDIRECTDRAWSURFACE7	m_pSurface;		// +0x1c
};

// One size's textures, least recently used first.
struct LMTextureList : public CheapLTLink
{
	void		*m_pData;
};

// A lightmap being drawn (lmdynamic.cpp draws into it).
struct LMDrawDesc
{
	LTBOOL		Begin(WorldPoly *pPoly, LTBOOL bCopy, uint32 width, uint32 height);
	LTBOOL		End(LTBOOL bChanged);

	void		*m_pData;		// +0x00
	uint32		m_Pitch;		// +0x04, bytes
	uint32		m_iSize;		// +0x08
	PFormat		m_Format;		// +0x0c
	uint32		m_Width;		// +0x44
	uint32		m_Height;		// +0x48
	LMTextureList	*m_pList;	// +0x4c, its size's textures
	RTexture	*m_pScratch;	// +0x50
};

// The lightmap texture pools.
class CLightmapPages
{
public:
				CLightmapPages();

	void		Reset();
	void		Term();
	void		SetAllocator(void* (*fn)(uint32 width, uint32 height, uint32 caps));
};

extern LPDIRECT3DDEVICE7	g_pD3DDevice;               // 0x100638c8
extern uint32				g_TextureStage;             // 0x100622d0
extern uint16				g_CurFrameCode;             // 0x1005d238
extern RTexture*			g_pCurTextures[];           // 0x10067270
extern TexPair				g_TexelSize[];              // 0x100672a8
extern int32				g_CV_LightMap;              // 0x1004a790
extern LTBOOL				g_100638d4;                 // 0x100638d4
extern LTBOOL				g_bLightmapTextures;        // 0x100682dc

void	r_SetPFormat(DDPIXELFORMAT* pPF, PFormat* pFormat);    // 0x10011794
void	d3d_FreeTextureList(LTLink *pList);                     // 0x10020600


// ---------------------------------------------------------------------- //
//  The lightmap bases.
// ---------------------------------------------------------------------- //
struct LMBasis
{
	LMBasis(LTVector v0, LTVector v1, LTVector v2)
	{
		m_v0 = v0;
		m_v1 = v1;
		m_v2 = v2;
	}

	LTVector	m_v0;
	LTVector	m_v1;
	LTVector	m_v2;
};

// $E 0x10035611
LMBasis g_LMBases[6] =
{
	LMBasis(LTVector(1.0f, 0.0f, 0.0f), LTVector(0.0f, 0.0f, -1.0f), LTVector(0.0f, 1.0f, 0.0f)),
	LMBasis(LTVector(1.0f, 0.0f, 0.0f), LTVector(0.0f, 0.0f, 1.0f), LTVector(0.0f, -1.0f, 0.0f)),
	LMBasis(LTVector(1.0f, 0.0f, 0.0f), LTVector(0.0f, 1.0f, 0.0f), LTVector(0.0f, 0.0f, 1.0f)),
	LMBasis(LTVector(1.0f, 0.0f, 0.0f), LTVector(0.0f, -1.0f, 0.0f), LTVector(0.0f, 0.0f, -1.0f)),
	LMBasis(LTVector(0.0f, 0.0f, 1.0f), LTVector(0.0f, -1.0f, 0.0f), LTVector(1.0f, 0.0f, 0.0f)),
	LMBasis(LTVector(0.0f, 0.0f, -1.0f), LTVector(0.0f, -1.0f, 0.0f), LTVector(-1.0f, 0.0f, 0.0f)),
};


// 0x10035860 -- a polygon's lightmap axes.
void d3d_GetLMAxes(int iBasis, LTVector *pNormal, LTVector *pRight, LTVector *pUp)
{
	*pRight = pNormal->Cross(g_LMBases[iBasis].m_v1);
	*pUp = pRight->Cross(*pNormal);
}


// ---------------------------------------------------------------------- //
//  The lightmap texture pools.
// ---------------------------------------------------------------------- //
uint32 g_LMSizes[NUM_LMSIZES] = {4, 8, 16, 32, 64};         // 0x1004df4c
uint32 g_LMSizeCounts[NUM_LMSIZES] = {125, 72, 24, 10, 10}; // 0x1004df60

RTexture*	g_LMScratch[NUM_LMSIZES];                   // 0x10080668
extern LMTextureList	g_LMLists[NUM_LMSIZES];         // 0x10081a80


// 0x100358b3 -- lock a scratch texture big enough for the polygon's
// lightmap (or width x height), filled with its paged lightmap or black.
LTBOOL LMDrawDesc::Begin(WorldPoly *pPoly, LTBOOL bCopy, uint32 width, uint32 height)
{
	LPDIRECTDRAWSURFACE7 pSurface;
	CLMPage *pPage;
	RECT lockRect, srcRect;
	DDBLTFX fx;
	DDSURFACEDESC2 ddsd;
	HRESULT hr;
	uint32 i;

	if(width <= 0)
		width = pPoly->m_LMWidth;
	m_Width = width;

	if(height <= 0)
		height = pPoly->m_LMHeight;
	m_Height = height;

	m_pList = LTNULL;
	for(i=0; i < NUM_LMSIZES; i++)
	{
		if(width <= g_LMSizes[i] && height <= g_LMSizes[i])
		{
			m_iSize = i;
			m_pList = &g_LMLists[i];
			break;
		}
	}

	if(!m_pList || m_pList->m_pNext == m_pList->AsLTLink())
		return LTFALSE;

	m_pScratch = g_LMScratch[m_iSize];
	pSurface = m_pScratch->m_pSurface;

	if(bCopy)
	{
		pPage = pPoly->m_pLMPage;
		if(pPage && g_CV_LightMap)
		{
			srcRect.left = pPoly->m_LMPageX;
			srcRect.top = pPoly->m_LMPageY;
			srcRect.right = width + srcRect.left;
			srcRect.bottom = height + srcRect.top;
			hr = pSurface->BltFast(0, 0, pPage->m_pSurface, &srcRect, DDBLTFAST_WAIT);
		}
		else
		{
			memset(&fx, 0, sizeof(fx));
			fx.dwSize = sizeof(fx);
			fx.dwFillColor = 0;
			hr = pSurface->Blt(LTNULL, LTNULL, LTNULL, DDBLT_COLORFILL, &fx);
		}

		if(hr != DD_OK)
			return LTFALSE;
	}

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	lockRect.left = 0;
	lockRect.top = 0;
	lockRect.right = m_Width;
	lockRect.bottom = m_Height;
	if(pSurface->Lock(&lockRect, &ddsd, DDLOCK_WAIT | DDLOCK_NOSYSLOCK, LTNULL) != DD_OK)
		return LTFALSE;

	m_Pitch = ddsd.lPitch;
	m_pData = ddsd.lpSurface;
	r_SetPFormat(&ddsd.ddpfPixelFormat, &m_Format);
	return LTTRUE;
}


// 0x10035a3f -- unlock it, and if it was drawn into, load it into the
// least recently used texture of its size and bind that.
LTBOOL LMDrawDesc::End(LTBOOL bChanged)
{
	RTexture *pTexture;
	RECT rect;
	HRESULT hr;

	rect.left = rect.top = 0;
	rect.right = m_Width;
	rect.bottom = m_Height;
	m_pScratch->m_pSurface->Unlock(&rect);

	if(bChanged)
	{
		pTexture = (RTexture*)m_pList->m_pNext->m_pData;

		if(pTexture->m_FrameCode != g_CurFrameCode)
			pTexture->m_FrameCode = g_CurFrameCode;

		if(!g_100638d4)
			hr = g_pD3DDevice->Load(pTexture->m_pSurface, LTNULL, m_pScratch->m_pSurface, &rect, 0);
		else
			hr = g_pD3DDevice->Load(pTexture->m_pSurface, LTNULL, m_pScratch->m_pSurface, LTNULL, 0);

		if(hr != D3D_OK)
			return LTFALSE;

		// Most recently used go last.
		dl_Remove(&pTexture->m_Link);
		dl_Insert(m_pList->m_pPrev, &pTexture->m_Link);

		g_pCurTextures[g_TextureStage] = pTexture;
		g_pD3DDevice->SetTexture(g_TextureStage, pTexture->m_pSurface);
		g_TexelSize[g_TextureStage].v = g_TexelSize[g_TextureStage].u = 1.0f / g_LMSizes[m_iSize];
	}

	return LTTRUE;
}


LMTextureList		g_LMLists[NUM_LMSIZES];             // 0x10081a80
CLightmapPages		g_LightmapPages;                    // 0x1008067c  $E 0x10035b4b


// 0x10035b55
CLightmapPages::CLightmapPages()
{
	LMTextureList *pList;
	uint32 i;

	for(i=0; i < NUM_LMSIZES; i++)
		g_LMScratch[i] = LTNULL;

	for(pList=g_LMLists; pList < &g_LMLists[NUM_LMSIZES]; pList++)
		dl_TieOff(pList);
}


// 0x10035b7b -- fill the pools.
void CLightmapPages::SetAllocator(void* (*fn)(uint32 width, uint32 height, uint32 caps))
{
	uint32 i, j, size;
	RTexture *pTexture;

	if(!g_bLightmapTextures)
		return;

	for(i=0; i < NUM_LMSIZES; i++)
	{
		size = g_LMSizes[i];
		for(j=0; j < g_LMSizeCounts[i]; j++)
		{
			pTexture = (RTexture*)fn(size, size, DDSCAPS_VIDEOMEMORY);
			if(pTexture)
			{
				pTexture->m_Link.m_pData = pTexture;
				dl_Insert(&g_LMLists[i], &pTexture->m_Link);
			}
		}

		g_LMScratch[i] = (RTexture*)fn(size, size, DDSCAPS_SYSTEMMEMORY);
	}
}


// 0x10035c00 -- the same, after the device's textures went.
void CLightmapPages::Reset()
{
	LMTextureList *pList;
	uint32 i;

	for(i=0; i < NUM_LMSIZES; i++)
		g_LMScratch[i] = LTNULL;

	for(pList=g_LMLists; pList < &g_LMLists[NUM_LMSIZES]; pList++)
		dl_TieOff(pList);
}


// 0x10035c24 -- free the pools, the scratch textures with them.
void CLightmapPages::Term()
{
	LTLink link;
	uint32 i;

	for(i=0; i < NUM_LMSIZES; i++)
	{
		dl_TieOff(&link);

		if(g_LMScratch[i])
		{
			link.m_pData = g_LMScratch[i];
			dl_Insert(&g_LMLists[i], &link);
			g_LMScratch[i] = LTNULL;
		}

		d3d_FreeTextureList(g_LMLists[i].AsLTLink());
	}
}
