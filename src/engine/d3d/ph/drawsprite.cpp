// *********************************************************************** //
//  ph/drawsprite.cpp -- Primal Hunt's d3d.ren, TU33, second object
//  (0x1002e400..0x100300e9), /O2.
//
//  Sprites: the OT_SPRITE ProcessObject handler (0x1004dad4), the two
//  sprite sets (translucent ones go on the sorted list, no-Z ones are
//  drawn last with the Z buffer off), and the draw itself, which sets the
//  object's blend states and draws it flat or rotatable.  Its two empty
//  initialisers (0x1002e400, 0x1002e410) mark the start of the object.
//
//  Not yet written: d3d_DrawFlatSprite 0x1002e630 (2715 bytes),
//  d3d_DrawRotatableSprite 0x1002f0d0 (1749) and 0x1002f970 (1105).
//  Near: 0x1002fdd0, the clip and projection with the Z bias: the first
//  term of the camera transform's x row loads its operands the other way
//  round, and retail CSEs m[3][0]*x and m[3][1]*y between the two 1/w
//  where cl 8966 here recomputes them (791 bytes against 793).
// *********************************************************************** //

#include <d3d.h>

#include "ltbasedefs.h"


struct LTObject
{
	uint8		m_pad_000[0x88];
	uint32		m_Flags;                                // +0x088
	uint32		m_Flags2;                               // +0x08c
	uint8		m_pad_090[0xac - 0x90];
	float		m_ScaleX;                               // +0x0ac
	float		m_ScaleY;                               // +0x0b0
	uint8		m_pad_0b4[0xbc - 0xb4];
	char		m_ObjectType;                           // +0x0bc
	uint8		m_pad_0bd[0x194 - 0xbd];
	LTVector	m_Pos;                                  // +0x194
	uint8		m_pad_1a0[0x1b4 - 0x1a0];
	void*		m_pCurAnim;                             // +0x1b4
	struct SpriteEntry*	m_pCurFrame;                    // +0x1b8
};

struct SpriteEntry
{
	void*		m_pTex;                                 // +0x00
};

// The view being drawn (common_draw.cpp's g_ViewParams).
struct ViewParams
{
	uint8		m_pad_000[0x15c];
	LTMatrix	m_mWorldToCamera;                       // +0x15c
	LTMatrix	m_mProjection;                          // +0x19c
};

typedef void (*DrawObjectFn)(ViewParams* pContext, LTObject* pObject);

extern void		AddDebugMessage(int32 debugLevel, const char* pMsg, ...);  // 0x10013b13

class BaseObjectSet
{
public:
	void		Add(LTObject *pObject)
	{
		if(m_nObjects < m_nMaxObjects)
		{
			m_pObjects[m_nObjects] = pObject;
			m_nObjects++;
		}
		else
		{
			AddDebugMessage(1, "Set '%s' overflowed", m_pSetName);
		}
	}

	void		Draw(ViewParams* pContext, DrawObjectFn fn);  // 0x100397e0

	LTObject	**m_pObjects;                           // +0x00
	uint32		m_nObjects;                             // +0x04
	LTLink		m_Link;                                 // +0x08
	uint32		m_nMaxObjects;                          // +0x14
	const char	*m_pSetName;                            // +0x18
	LTObject	**m_pArray;                             // +0x1c
};

struct VisibleSet
{
	uint8			m_pad_000[0x1a4];
	BaseObjectSet	m_TranslucentSprites;               // +0x1a4
	BaseObjectSet	m_NoZSprites;                       // +0x1c4
};

class ObjectDrawList
{
public:
	void	Add(LTObject* pObject, DrawObjectFn fn);    // 0x10029980
};

extern LPDIRECT3DDEVICE7	g_pD3DDevice;               // 0x100638c8
extern uint32				g_ZBufferState;             // 0x10062438
extern uint32				g_ClipFlags;                // 0x1005bd0c
extern ObjectDrawList*		g_pObjectDrawList;          // 0x100713cc
extern ViewParams			g_ViewParams;             // 0x1005b790
extern int32				g_CV_DrawSprites;           // 0x1004a764

extern VisibleSet*	d3d_GetVisibleSet();                // 0x1003abb0

extern void		d3d_DrawFlatSprite(ViewParams *pContext, LTObject *pObject, LTVector *pPos,
					float scaleX, float scaleY, void *pTex);    // 0x1002e630
extern void		d3d_DrawRotatableSprite(ViewParams *pContext, LTObject *pObject, LTVector *pPos,
					float scaleX, float scaleY, void *pTex);    // 0x1002f0d0


// This can be used to set a state, and it will automatically restore the
// state's value in its destructor.
class StateSet
{
public:
	StateSet(D3DRENDERSTATETYPE state, uint32 val)
	{
		m_State = state;
		g_pD3DDevice->GetRenderState(state, (unsigned long *)&m_OldVal);
		g_pD3DDevice->SetRenderState(state, val);
	}

	~StateSet()
	{
		g_pD3DDevice->SetRenderState(m_State, m_OldVal);
	}

	D3DRENDERSTATETYPE	m_State;
	uint32				m_OldVal;
};

#define PHFLAG_SPRITE_NOZ		0x10
#define PHFLAG_ROTATABLESPRITE	0x08
#define PHFLAG_FOGDISABLE		0x80
#define PHFLAG2_ADDITIVE		0x04
#define PHFLAG2_MULTIPLY		0x08
#define PHFLAG2_ADDITIVEALPHA	0x4000
#define PHOT_MODEL				1


// Get special alpha blend states based on the object's flags2.
inline void d3d_GetBlendStates(LTObject *pObject, uint32 &srcBlend, uint32 &destBlend, uint32 &dwFog, uint32 &dwFogColor)
{
	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_FOGCOLOR, (unsigned long *)&dwFogColor);

	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_FOGENABLE, (unsigned long *)&dwFog);
	if ((pObject->m_Flags & PHFLAG_FOGDISABLE) && pObject->m_ObjectType != PHOT_MODEL)
	{
		dwFog = 0;
	}

	if (pObject->m_Flags2 & PHFLAG2_ADDITIVE)
	{
		srcBlend	= D3DBLEND_ONE;
		destBlend	= D3DBLEND_ONE;
		dwFogColor	= 0;
	}
	else if (pObject->m_Flags2 & PHFLAG2_MULTIPLY)
	{
		srcBlend	= D3DBLEND_ZERO;
		destBlend	= D3DBLEND_SRCCOLOR;
		dwFogColor	= 0xFFFFFFFF;
	}
	else
	{
		srcBlend	= D3DBLEND_SRCALPHA;
		destBlend	= D3DBLEND_INVSRCALPHA;
	}

	if (pObject->m_Flags2 & PHFLAG2_ADDITIVEALPHA)
	{
		srcBlend	= D3DBLEND_SRCALPHA;
		destBlend	= D3DBLEND_ONE;
	}
}


// The sprite's TL vertex.
struct SpriteVert
{
	LTVector	m_Vec;                                  // +0x00
	float		m_RHW;                                  // +0x0c
	uint32		m_Color;                                // +0x10
	uint32		m_Specular;                             // +0x14
	float		m_TU, m_TV;                             // +0x18
};


// ---------------------------------------------------------------------- //
//  Globals.
// ---------------------------------------------------------------------- //
LTVector	g_vSpriteRight;                             // $E 0x1002e400
LTVector	g_vSpriteUp;                                // $E 0x1002e410


// ---------------------------------------------------------------------- //
//  0x1002e420
// ---------------------------------------------------------------------- //
void d3d_DrawSprite(ViewParams *pContext, LTObject *pObject)
{
	SpriteEntry *pSe;
	void *pAnim;
	uint32 srcBlend, destBlend, dwFog, dwFogColor;

	pAnim = pObject->m_pCurAnim;
	pSe = pObject->m_pCurFrame;

	if (pAnim && pSe && pSe->m_pTex)
	{
		g_ClipFlags = 0x3f;

		// Set the correct blend states.
		d3d_GetBlendStates(pObject, srcBlend, destBlend, dwFog, dwFogColor);
		StateSet ssSrcBlend(D3DRENDERSTATE_SRCBLEND, srcBlend);
		StateSet ssDestBlend(D3DRENDERSTATE_DESTBLEND, destBlend);
		StateSet ssFog(D3DRENDERSTATE_FOGENABLE, dwFog);
		StateSet ssFogColor(D3DRENDERSTATE_FOGCOLOR, dwFogColor);

		if (pObject->m_Flags & PHFLAG_ROTATABLESPRITE)
		{
			d3d_DrawRotatableSprite(pContext, pObject, &pObject->m_Pos,
				pObject->m_ScaleX, pObject->m_ScaleY, pSe->m_pTex);
		}
		else
		{
			d3d_DrawFlatSprite(pContext, pObject, &pObject->m_Pos,
				pObject->m_ScaleX, pObject->m_ScaleY, pSe->m_pTex);
		}
	}
}


// 0x1002e630 (d3d_DrawFlatSprite, 2715 bytes) and 0x1002f0d0
// (d3d_DrawRotatableSprite, 1749 bytes) are not written yet.


// ---------------------------------------------------------------- //
// External functions.
// ---------------------------------------------------------------- //

// 0x1002f7b0
void d3d_ProcessSprite(LTObject *pObject)
{
	VisibleSet *pVisibleSet = d3d_GetVisibleSet();
	BaseObjectSet *pSet = (pObject->m_Flags & PHFLAG_SPRITE_NOZ) ? &pVisibleSet->m_NoZSprites : &pVisibleSet->m_TranslucentSprites;
	pSet->Add(pObject);
}


static void d3d_QueueSprite(ViewParams *pContext, LTObject *pObject);

// 0x1002f800 -- translucent sprites go on the sorted list.
void d3d_QueueTranslucentSprites()
{
	BaseObjectSet *pSet;

	if (g_CV_DrawSprites)
	{
		pSet = &d3d_GetVisibleSet()->m_TranslucentSprites;
		pSet->Draw(&g_ViewParams, d3d_QueueSprite);
	}
}


// 0x1002f830
static void d3d_QueueSprite(ViewParams *pContext, LTObject *pObject)
{
	g_pObjectDrawList->Add(pObject, d3d_DrawSprite);
}


// 0x1002f850
void d3d_DrawNoZSprites()
{
	VisibleSet *pVisibleSet;
	BaseObjectSet *pSet;

	if (!g_CV_DrawSprites)
		return;

	pVisibleSet = d3d_GetVisibleSet();

	// Draw the NoZ sprites.
	pSet = &pVisibleSet->m_NoZSprites;
	if (pSet->m_nObjects > 0)
	{
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, FALSE);
		pSet->Draw(&g_ViewParams, d3d_DrawSprite);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, g_ZBufferState);
	}
}


// 0x1002f8b0 -- transform the vertices in place, with the divide by w.
void d3d_TransformSpriteVerts(SpriteVert *pVerts, uint32 nVerts, LTMatrix *pMat)
{
	while (nVerts)
	{
		MatVMul_InPlace_H(pMat, &pVerts->m_Vec);
		pVerts++;
		nVerts--;
	}
}


// 0x1002f970 (1105 bytes) is not written yet.


// 0x1002fdd0 -- the sprite's vertices to camera space, clipped, and
// projected with the Z bias (held off the near plane).
extern float			g_fSpriteNearZ;                 // 0x1005b74c

// The clip vertex, and the clip planes (TU06's ClipNear 0x1000b07d and
// ClipLeft 0x1000b1bc, TU03's ClipTop 0x100063e0, ClipRight 0x10006670,
// ClipBottom 0x10006900, ClipFar 0x10006ba0).
struct ClipVert
{
	LTVector	m_vPos;                                 // +0x00
	float		m_fRHW;                                 // +0x0c
	uint32		m_Diffuse;                              // +0x10
	uint32		m_Specular;                             // +0x14
	float		m_fU;                                   // +0x18
	float		m_fV;                                   // +0x1c
};

template<class V> LTBOOL ClipNear(uint8* pUnused, V** ppVerts, int32* pnVerts, V** ppOut);
template<class V> LTBOOL ClipLeft(uint8* pUnused, V** ppVerts, int32* pnVerts, V** ppOut);
template<class V> LTBOOL ClipTop(uint8* pUnused, V** ppVerts, int32* pnVerts, V** ppOut);
template<class V> LTBOOL ClipRight(uint8* pUnused, V** ppVerts, int32* pnVerts, V** ppOut);
template<class V> LTBOOL ClipBottom(uint8* pUnused, V** ppVerts, int32* pnVerts, V** ppOut);
template<class V> LTBOOL ClipFar(uint8* pUnused, V** ppVerts, int32* pnVerts, V** ppOut);

extern int32			g_bUseD3DClip;                  // 0x100622b0
extern void*			g_pClipScratch;                 // 0x1005bd14

// Clip a polygon against the planes in nClipFlags, into the clip scratch
// buffer. FALSE when nothing is left.
inline LTBOOL d3d_ClipPoly(uint32 nClipFlags, ClipVert** ppVerts, int32* pnVerts)
{
	ClipVert* pOut;
	ClipVert* pVerts;
	int32 nVerts;
	uint8 nearPlane, leftPlane, topPlane, rightPlane, bottomPlane, farPlane;

	if (g_bUseD3DClip)
	{
		nClipFlags &= 1;
		if (!nClipFlags)
			return LTTRUE;
	}

	pOut = (ClipVert*)g_pClipScratch;
	pVerts = *ppVerts;
	nVerts = *pnVerts;

	if (nClipFlags & 0x01)
	{
		if (!ClipNear(&nearPlane, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x04)
	{
		if (!ClipLeft(&leftPlane, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x08)
	{
		if (!ClipTop(&topPlane, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x10)
	{
		if (!ClipRight(&rightPlane, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x20)
	{
		if (!ClipBottom(&bottomPlane, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x02)
	{
		if (!ClipFar(&farPlane, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	*ppVerts = pVerts;
	*pnVerts = nVerts;
	return LTTRUE;
}

LTBOOL d3d_ClipSpriteVerts(ClipVert **ppVerts, int32 *pnVerts, ViewParams *pParams, float fZBias)
{
	ClipVert *pVert;
	LTMatrix *pMat;
	float x, y, z, rhw, bias;
	int32 i;

	pVert = *ppVerts;
	for (i=*pnVerts; i; i--)
	{
		pMat = &pParams->m_mWorldToCamera;
		z = pMat->m[2][0]*pVert->m_vPos.x + pMat->m[2][1]*pVert->m_vPos.y + pMat->m[2][2]*pVert->m_vPos.z + pMat->m[2][3];
		x = pVert->m_vPos.x*pMat->m[0][0] + pMat->m[0][1]*pVert->m_vPos.y + pMat->m[0][2]*pVert->m_vPos.z + pMat->m[0][3];
		y = pMat->m[1][0]*pVert->m_vPos.x + pMat->m[1][1]*pVert->m_vPos.y + pMat->m[1][2]*pVert->m_vPos.z + pMat->m[1][3];
		pVert->m_vPos.z = z;
		pVert->m_vPos.x = x;
		pVert->m_vPos.y = y;
		pVert++;
	}

	if (g_ClipFlags && !d3d_ClipPoly(g_ClipFlags, ppVerts, pnVerts))
		return LTFALSE;

	pMat = &g_ViewParams.m_mProjection;
	pVert = *ppVerts;
	for (i=*pnVerts; i; i--)
	{
		bias = (fZBias + pVert->m_vPos.z < g_fSpriteNearZ) ? g_fSpriteNearZ - pVert->m_vPos.z : fZBias;
		rhw = 1.0f / (pMat->m[3][0]*pVert->m_vPos.x + pMat->m[3][1]*pVert->m_vPos.y + pMat->m[3][2]*pVert->m_vPos.z + pMat->m[3][3]);
		x = rhw * (pMat->m[0][0]*pVert->m_vPos.x + pMat->m[0][1]*pVert->m_vPos.y + pMat->m[0][2]*pVert->m_vPos.z + pMat->m[0][3]);
		y = rhw * (pMat->m[1][0]*pVert->m_vPos.x + pMat->m[1][1]*pVert->m_vPos.y + pMat->m[1][2]*pVert->m_vPos.z + pMat->m[1][3]);
		z = pVert->m_vPos.z + bias;
		rhw = 1.0f / (pMat->m[3][0]*pVert->m_vPos.x + pMat->m[3][1]*pVert->m_vPos.y + pMat->m[3][2]*z + pMat->m[3][3]);
		z = rhw * (pMat->m[2][0]*pVert->m_vPos.x + pMat->m[2][1]*pVert->m_vPos.y + pMat->m[2][2]*z + pMat->m[2][3]);
		pVert->m_vPos.x = x;
		pVert->m_vPos.y = y;
		pVert->m_vPos.z = z;
		pVert->m_fRHW = rhw;
		pVert++;
	}

	return LTTRUE;
}
