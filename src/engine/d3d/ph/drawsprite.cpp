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
//  Not yet written: d3d_DrawFlatSprite 0x1002e630 (2715 bytes).
//  Near: 0x1002fdd0, the clip and projection with the Z bias: the first
//  term of the camera transform's x row loads its operands the other way
//  round, and retail CSEs m[3][0]*x and m[3][1]*y between the two 1/w
//  where cl 8966 here recomputes them (791 bytes against 793).
// *********************************************************************** //

#include <d3d.h>

#include "ltbasedefs.h"


// An object's rotation (the engine's quaternion).
struct Rotation
{
	float		m_Quat[4];
};

struct LTObject
{
	uint8		m_pad_000[0x88];
	uint32		m_Flags;                                // +0x088
	uint32		m_Flags2;                               // +0x08c
	uint8		m_pad_090[0x94 - 0x90];
	uint8		m_ColorR, m_ColorG, m_ColorB, m_ColorA;     // +0x094
	uint8		m_pad_098[0x9c - 0x98];
	Rotation	m_Rotation;                             // +0x09c
	float		m_ScaleX;                               // +0x0ac
	float		m_ScaleY;                               // +0x0b0
	float		m_ScaleZ;                               // +0x0b4
	uint8		m_pad_0b8[0xbc - 0xb8];
	char		m_ObjectType;                           // +0x0bc
	uint8		m_pad_0bd[0x194 - 0xbd];
	LTVector	m_Pos;                                  // +0x194
	uint8		m_pad_1a0[0x1b4 - 0x1a0];
	void*		m_pCurAnim;                             // +0x1b4
	struct SpriteEntry*	m_pCurFrame;                    // +0x1b8
	uint8		m_pad_1bc[0x1c4 - 0x1bc];
	HPOLY		m_ClipperPoly;                          // +0x1c4
};

// The clip vertex (the sprite's TL vertex as the clippers see it).
struct ClipVert
{
	LTVector	m_vPos;                                 // +0x00
	float		m_fRHW;                                 // +0x0c
	uint32		m_Diffuse;                              // +0x10
	uint32		m_Specular;                             // +0x14
	float		m_fU;                                   // +0x18
	float		m_fV;                                   // +0x1c
};


struct SpriteEntry
{
	void*		m_pTex;                                 // +0x00
};

// The view being drawn (common_draw.cpp's g_ViewParams).
struct ViewParams
{
	uint8		m_pad_000[0x94];
	float		m_fScaleX;                              // +0x094 the view's sprite scale
	float		m_fScaleY;                              // +0x098
	uint8		m_pad_09c[0x15c - 0x9c];
	LTMatrix	m_mWorldToCamera;                       // +0x15c
	LTMatrix	m_mProjection;                          // +0x19c
	uint8		m_pad_1dc[0x2dc - 0x1dc];
	LTMatrix	m_mReallyCloseToCamera;                 // +0x2dc
	uint8		m_pad_31c[0x4b4 - 0x31c];
	LTVector	m_Pos;                                  // +0x4b4
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


// A texture in video memory, one for each stage it is bound to.
class RTexture
{
public:
	virtual			~RTexture();
	virtual LTBOOL	IsValid();
	virtual uint32	GetFlags();
	virtual uint32	GetWidth();                         // +0x0c
	virtual uint32	GetHeight();                        // +0x10

	uint8					m_pad_04[0x0c - 0x04];
	LPDIRECTDRAWSURFACE7	m_pSurface;                 // +0x0c
	uint8					m_pad_10[0x30 - 0x10];
	RTexture*				m_pNext;                    // +0x30
	uint8					m_pad_34[0x44 - 0x34];
	uint16					m_LOD;                      // +0x44
};

class CRenderStateList
{
public:
	void		Restore();                              // 0x10022a86
	void		Apply(CRenderStateList* pList, uint32 iStage);  // 0x10022a97
};

struct SharedTexture
{
	uint8				m_pad_00[0x0c];
	RTexture*			m_pRenderData;                  // +0x0c
	uint8				m_pad_10[0x30 - 0x10];
	uint16				m_FrameCode;                    // +0x30
	uint8				m_pad_32[0x34 - 0x32];
	CRenderStateList*	m_pStateList;                   // +0x34
	uint8				m_pad_38[0x3c - 0x38];
	uint32				m_MipShift;                     // +0x3c (the first mipmap used)
};

// A colour's bytes, and a sprite's colour.
struct ColorBytes
{
	uint8		b, g, r, a;
};

typedef ColorBytes SpriteColor;

struct TLVertex;
struct ModelInstance;

#define PHFLAG_SPRITEBIAS		0x04
#define PHFLAG_NOLIGHT			0x200
#define MAX_SPRITE_CLIPVERTS	300
#define SPRITE_POSITION_ZBIAS	-20.0f

extern CRenderStateList		g_SavedStates;              // 0x10069728 (d3d_savedstates.cpp)
extern uint32				g_iCurTextureStage;         // 0x100638c0
extern uint16				g_CurTextureFrameCode;      // 0x1005d250
extern RTexture*			g_pCurTextures[];           // 0x10067270
extern float				g_fTexelSizeU;              // 0x100672a8
extern float				g_fTexelSizeV;              // 0x100672ac
extern LTVector				g_vGlobalLightColor;        // 0x1005b780
extern void					(__fastcall *g_pfnCalcFog)(const LTVector* pPos, uint32* pSpecular);  // 0x1005e1c4

extern RTexture*	d3d_CreateRenderTexture(SharedTexture* pTexture, uint32 iStage, LTBOOL bStageCopy);  // 0x10020cd0
extern void			d3d_SetTextureDirect(RTexture* pTexture);   // 0x10007a89
extern RTexture*	d3d_GetStageTexture(RTexture* pTexture, uint8 iStage);  // 0x10009300
extern void			d3d_RotationToMatrix(Rotation* pRot, LTMatrix* pMat);   // 0x1003655d
extern void			d3d_GetLightTableColor(void *pTable, LTVector *pPos, ColorBytes *pColor);  // 0x1000c818 (drawmodel.cpp)
extern void			d3d_GetDynamicLightAdd(ModelInstance* pInstance, LTVector* pAdd);  // 0x10010010 (d3d_viewparams.cpp)
extern LTBOOL		d3d_ClipTLVerts(TLVertex** ppVerts, int* pnVerts, ViewParams* pParams, uint32 flags);  // 0x1000aec6
HRESULT			d3d_DrawPrimitive(D3DPRIMITIVETYPE type, DWORD fvf, LPVOID pVerts, DWORD nVerts, DWORD flags);  // 0x1000d26f (drawmodel.cpp)

struct SpriteVert;
void	d3d_TransformSpriteVerts(SpriteVert *pVerts, uint32 nVerts, LTMatrix *pMat);
LTBOOL	d3d_ClipSprite(LTObject *pInstance, uint32 hPoly, ClipVert **ppPoints, int32 *pnPoints, ClipVert *pOut);
LTBOOL	d3d_ClipSpriteVerts(ClipVert **ppVerts, int32 *pnVerts, ViewParams *pParams, uint32 flags, float fZBias);

// Round to nearest in the FPU's mode (ph/d3d_clip.h's helper).
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}


// The world polygon a sprite is clipped to.
struct SpritePolyVert
{
	LTVector*	m_pVec;                                 // +0x00
	uint8		m_pad_04[0x18 - 0x04];
};

struct SpriteWorldPoly
{
	uint8			m_pad_00[0x28];
	LTPlane*		m_pPlane;                           // +0x28
	uint8			m_pad_2c[0x54 - 0x2c];
	uint16			m_nVerts;                           // +0x54
	uint16			m_pad_56;
	SpritePolyVert	m_Verts[1];                         // +0x58
};

class SpriteWorldBsp
{
public:
	virtual void			Slot00();
	virtual void			Slot04();
	virtual void			Slot08();
	virtual SpriteWorldPoly*	GetPolyFromHPoly(HPOLY hPoly);  // +0x0c
};

struct SpriteWorldModel
{
	uint8			m_pad_00[0x04];
	SpriteWorldBsp*	m_pBsp;                             // +0x04
};

struct SceneWorld
{
	SpriteWorldPoly* GetPolyFromHPoly(HPOLY hPoly)
	{
		uint32 iWorld;
		SpriteWorldModel* pModel;

		iWorld = hPoly >> 16;
		if (iWorld >= m_nWorldModels)
			return LTNULL;

		pModel = m_WorldModels[iWorld];
		if (!pModel)
			return LTNULL;

		return pModel->m_pBsp->GetPolyFromHPoly(hPoly);
	}

	uint8				m_pad_000[0xfc];
	uint8				m_LightTable[0x18c - 0xfc];     // +0x0fc (the light grid)
	SpriteWorldModel**	m_WorldModels;                  // +0x18c
	uint32				m_nWorldModels;                 // +0x190
};

extern SceneWorld*		g_pSceneWorld;                  // 0x1005c208
extern uint32			g_nClipPasses;                  // 0x1005c124


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

#ifdef PRIMAL_HUNT
	// Primal Hunt's: AvP2's sprites have no additive-alpha blend.
	if (pObject->m_Flags2 & PHFLAG2_ADDITIVEALPHA)
	{
		srcBlend	= D3DBLEND_SRCALPHA;
		destBlend	= D3DBLEND_ONE;
	}
#endif
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


// ---------------------------------------------------------------------- //
//  0x1002e630 -- a camera-facing sprite: a quad around its camera-space
//  position, sized by its texture, the view and its scale.
//
//  Near: retail also computes (pObject->m_Pos - pContext->m_Pos).Mag(),
//  unused, with _CVector's constructor and Mag out of line; cl expands
//  them here and drops the dead code (about 50 bytes).
// ---------------------------------------------------------------------- //
#define PHFLAG_GLOWSPRITE		0x20
#define PHFLAG_REALLYCLOSE		0x40
#define PHFLAG2_SPRITETEXROTATE	0x200	// turn a flat sprite's texture by the object's rotation

// The glow sprites' size by distance (their own constants in .rdata).
static const float SPRITE_MINFACTORDIST		= 10.0f;
static const float SPRITE_MAXFACTORDIST		= 500.0f;
static const float SPRITE_MINFACTOR			= 0.1f;
static const float SPRITE_MAXFACTOR			= 2.0f;

extern float			g_NearZ;                        // 0x1005b818
extern float			g_fReallyCloseNearZ;            // 0x1005a32c (g_CV_ReallyCloseNearZ's value)
extern float			g_fSpriteNearZ;                 // 0x1005b74c (g_CV_NearZ's value)
extern void				d3d_ProjectTLVert(ClipVert* pVert, ViewParams* pParams);  // 0x1002739d

inline LTBOOL d3d_ClipPoly(uint32 nClipFlags, ClipVert** ppVerts, int32* pnVerts);

// A camera-space vertex to the screen, its z (and so its 1/w) pushed by
// fZBias: x and y project where the vertex is, so it keeps its size.
inline void d3d_ProjectBiasedVert(ClipVert *pVert, float fZBias)
{
	LTMatrix *pMat;
	float x, y, z, rhw;

	pMat = &g_ViewParams.m_mProjection;
	rhw = 1.0f / (pMat->m[3][0]*pVert->m_vPos.x + pMat->m[3][1]*pVert->m_vPos.y + pMat->m[3][2]*pVert->m_vPos.z + pMat->m[3][3]);
	x = rhw * (pMat->m[0][0]*pVert->m_vPos.x + pMat->m[0][1]*pVert->m_vPos.y + pMat->m[0][2]*pVert->m_vPos.z + pMat->m[0][3]);
	y = rhw * (pMat->m[1][0]*pVert->m_vPos.x + pMat->m[1][1]*pVert->m_vPos.y + pMat->m[1][2]*pVert->m_vPos.z + pMat->m[1][3]);
	z = pVert->m_vPos.z + fZBias;
	rhw = 1.0f / (pMat->m[3][0]*pVert->m_vPos.x + pMat->m[3][1]*pVert->m_vPos.y + pMat->m[3][2]*z + pMat->m[3][3]);
	z = rhw * (pMat->m[2][0]*pVert->m_vPos.x + pMat->m[2][1]*pVert->m_vPos.y + pMat->m[2][2]*z + pMat->m[2][3]);
	pVert->m_vPos.x = x;
	pVert->m_vPos.y = y;
	pVert->m_vPos.z = z;
	pVert->m_fRHW = rhw;
}

void d3d_DrawFlatSprite(ViewParams *pContext, LTObject *pObject, LTVector *pPos,
	float scaleX, float scaleY, void *pTexture)
{
	ClipVert verts[4], *pVerts;
	LTVector vCamPos, vAdd;
	LTMatrix mRotation;
	RTexture *pRenderData, *pCur;
	ColorBytes lightColor;
	SpriteColor color;
	uint32 iStage, specular;
	float fNearZ, fDist, fWidth, fHeight, fSizeX, fSizeY, fFactor, fBias, fOldNearZ;
	float uStart, uEnd, vStart, vEnd, uCenter, vCenter, du, dv;
	float r, g, b;
	int32 nVerts, i;
	SharedTexture *pTex = (SharedTexture*)pTexture;

	// Into camera space (really-close sprites have their own).
	if (pObject->m_Flags & PHFLAG_REALLYCLOSE)
	{
		MatVMul(&vCamPos, &pContext->m_mReallyCloseToCamera, pPos);
		fNearZ = g_fReallyCloseNearZ;
	}
	else
	{
		MatVMul(&vCamPos, &pContext->m_mWorldToCamera, pPos);
		fNearZ = g_fSpriteNearZ;
	}

	g_pfnCalcFog(&pObject->m_Pos, &specular);

	// Bail if it is too close to be seen.
	if (vCamPos.z <= fNearZ)
		return;

	fDist = (pObject->m_Pos - pContext->m_Pos).Mag();   // (unused)

	// Bind the texture.
	iStage = g_iCurTextureStage;
	if (!pTex)
		return;

	pRenderData = pTex->m_pRenderData;
	pTex->m_FrameCode = g_CurTextureFrameCode;

	pCur = LTNULL;
	if (pRenderData)
		pCur = d3d_GetStageTexture(pRenderData, (uint8)iStage);

	if (!pCur || pCur != g_pCurTextures[iStage])
	{
		if (!pCur)
		{
			pRenderData = pTex->m_pRenderData;
			if (pRenderData)
			{
				pCur = d3d_CreateRenderTexture(pTex, iStage, LTTRUE);
				if (!pCur)
					return;

				pCur->m_pNext = pRenderData->m_pNext;
				pRenderData->m_pNext = pCur;
			}
			else
			{
				pCur = d3d_CreateRenderTexture(pTex, iStage, LTFALSE);
				if (!pCur)
					return;
			}
		}

		d3d_SetTextureDirect(pCur);
	}

	if (pCur->m_LOD != 0)
	{
		pCur->m_pSurface->SetLOD(0);
		pCur->m_LOD = 0;
	}

	fWidth = (float)g_pCurTextures[g_iCurTextureStage]->GetWidth();
	fHeight = (float)g_pCurTextures[g_iCurTextureStage]->GetHeight();

	// A texel in from each edge.
	uStart = g_fTexelSizeU * 2.0f;
	uEnd = (fWidth - 2.0f) * g_fTexelSizeU;
	vStart = g_fTexelSizeV * 2.0f;
	vEnd = (fHeight - 2.0f) * g_fTexelSizeV;

	fSizeX = fWidth * pContext->m_fScaleX * scaleX;
	fSizeY = fHeight * pContext->m_fScaleY * scaleY;

	if (pObject->m_Flags & PHFLAG_GLOWSPRITE)
	{
		// Glow sprites grow with distance.
		fFactor = (vCamPos.z - SPRITE_MINFACTORDIST) / (SPRITE_MAXFACTORDIST - SPRITE_MINFACTORDIST);
		fFactor = LTCLAMP(fFactor, 0.0f, 1.0f);
		fFactor = SPRITE_MINFACTOR + ((SPRITE_MAXFACTOR - SPRITE_MINFACTOR) * fFactor);

		fSizeX *= fFactor;
		fSizeY *= fFactor;
	}

	// Its colour: its own, lit by the light table and the dynamic lights.
	color.a = pObject->m_ColorA;
	if (!(pObject->m_Flags & PHFLAG_NOLIGHT) && g_pSceneWorld)
	{
		d3d_GetLightTableColor(g_pSceneWorld->m_LightTable, &pObject->m_Pos, &lightColor);
		d3d_GetDynamicLightAdd((ModelInstance*)pObject, &vAdd);

		r = ((float)pObject->m_ColorR * g_vGlobalLightColor.x + vAdd.x) * ((float)lightColor.r * (1.0f / 255.0f));
		if (r < 0.0f)
			r = 0.0f;
		else if (r > 255.0f)
			r = 255.0f;

		g = ((float)pObject->m_ColorG * g_vGlobalLightColor.y + vAdd.y) * ((float)lightColor.g * (1.0f / 255.0f));
		if (g < 0.0f)
			g = 0.0f;
		else if (g > 255.0f)
			g = 255.0f;

		b = ((float)pObject->m_ColorB * g_vGlobalLightColor.z + vAdd.z) * ((float)lightColor.b * (1.0f / 255.0f));
		if (b < 0.0f)
			b = 0.0f;
		else if (b > 255.0f)
			b = 255.0f;

		color.r = (uint8)RoundF(r);
		color.g = (uint8)RoundF(g);
		color.b = (uint8)RoundF(b);
	}
	else
	{
		color.r = (uint8)RoundF((float)pObject->m_ColorR * g_vGlobalLightColor.x);
		color.g = (uint8)RoundF((float)pObject->m_ColorG * g_vGlobalLightColor.y);
		color.b = (uint8)RoundF((float)pObject->m_ColorB * g_vGlobalLightColor.z);
	}

	verts[0].m_vPos.x = vCamPos.x - fSizeX;
	verts[0].m_vPos.y = vCamPos.y + fSizeY;
	verts[0].m_vPos.z = vCamPos.z;
	verts[0].m_Diffuse = *(uint32*)&color;
	verts[0].m_Specular = specular;
	verts[0].m_fU = uStart;
	verts[0].m_fV = vStart;

	verts[1].m_vPos.x = vCamPos.x + fSizeX;
	verts[1].m_vPos.y = vCamPos.y + fSizeY;
	verts[1].m_vPos.z = vCamPos.z;
	verts[1].m_Diffuse = *(uint32*)&color;
	verts[1].m_Specular = specular;
	verts[1].m_fU = uEnd;
	verts[1].m_fV = vStart;

	verts[2].m_vPos.x = vCamPos.x + fSizeX;
	verts[2].m_vPos.y = vCamPos.y - fSizeY;
	verts[2].m_vPos.z = vCamPos.z;
	verts[2].m_Diffuse = *(uint32*)&color;
	verts[2].m_Specular = specular;
	verts[2].m_fU = uEnd;
	verts[2].m_fV = vEnd;

	verts[3].m_vPos.x = vCamPos.x - fSizeX;
	verts[3].m_vPos.y = vCamPos.y - fSizeY;
	verts[3].m_vPos.z = vCamPos.z;
	verts[3].m_Diffuse = *(uint32*)&color;
	verts[3].m_Specular = specular;
	verts[3].m_fU = uStart;
	verts[3].m_fV = vEnd;

	// Turn the texture about its centre by the object's rotation (clamped
	// so the corners don't wrap).
	if (pObject->m_Flags2 & PHFLAG2_SPRITETEXROTATE)
	{
		d3d_RotationToMatrix(&pObject->m_Rotation, &mRotation);

		uCenter = (uEnd + uStart) * 0.5f;
		vCenter = (vEnd + vStart) * 0.5f;
		for (i=0; i < 4; i++)
		{
			dv = verts[i].m_fV - vCenter;
			du = verts[i].m_fU - uCenter;
			verts[i].m_fV = du*mRotation.m[1][0] + dv*mRotation.m[1][1] + vCenter;
			verts[i].m_fU = du*mRotation.m[0][0] + dv*mRotation.m[0][1] + uCenter;
		}

		g_pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);
	}

	fOldNearZ = g_NearZ;
	pVerts = verts;
	nVerts = 4;

	if (pObject->m_Flags & PHFLAG_REALLYCLOSE)
		g_NearZ = g_fReallyCloseNearZ;

	if (d3d_ClipPoly(g_ClipFlags, &pVerts, &nVerts))
	{
		if (pObject->m_Flags & PHFLAG_SPRITEBIAS)
		{
			// Pull it towards the camera, up to but not past the near plane.
			fBias = SPRITE_POSITION_ZBIAS;
			if (fBias + vCamPos.z < g_fSpriteNearZ)
				fBias = g_fSpriteNearZ - vCamPos.z;

			for (i=0; i < nVerts; i++)
				d3d_ProjectBiasedVert(&pVerts[i], fBias);
		}
		else if (pObject->m_Flags & PHFLAG_REALLYCLOSE)
		{
			for (i=0; i < nVerts; i++)
				d3d_ProjectBiasedVert(&pVerts[i], g_fSpriteNearZ);
		}
		else
		{
			for (i=0; i < nVerts; i++)
				d3d_ProjectTLVert(&pVerts[i], &g_ViewParams);
		}

		g_SavedStates.Restore();
		if (pTex->m_pStateList)
			g_SavedStates.Apply(pTex->m_pStateList, g_iCurTextureStage);

		g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, pVerts, nVerts, 0);
	}

	if (pObject->m_Flags2 & PHFLAG2_SPRITETEXROTATE)
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);

	g_NearZ = fOldNearZ;
}


// The quad's corners in the order the camera sees them, from the front
// and from behind.
uint32		g_SpriteFrontOrder[4] = { 0, 1, 2, 3 };     // 0x1004ddac
uint32		g_SpriteBackOrder[4] = { 3, 2, 1, 0 };      // 0x1004ddbc


// ---------------------------------------------------------------------- //
//  0x1002f0d0 -- a rotatable sprite: a quad in the object's rotation and
//  scale, facing whichever way the camera sees it, lit from the light
//  table and the dynamic lights, clipped to its ClipperPoly and drawn,
//  held off the view by the Z bias with FLAG_SPRITEBIAS.
// ---------------------------------------------------------------------- //
void d3d_DrawRotatableSprite(ViewParams *pContext, LTObject *pObject, LTVector *pPos,
	float scaleX, float scaleY, void *pTexture)
{
	ClipVert clippedVerts[MAX_SPRITE_CLIPVERTS];
	ClipVert verts[4];
	LTMatrix mRot;
	LTVector vDir, vForward, vToCamera, vAdd;
	ClipVert *pVerts;
	int32 nVerts;
	const uint32 *pOrder;
	RTexture *pRenderData, *pCur;
	ColorBytes lightColor;
	uint32 iStage, specular;
	float fDist, fWidth, fHeight;
	float uMin, uMax, vMin, vMax;
	float r, g, b;
	SpriteColor color;
	SharedTexture *pTex = (SharedTexture*)pTexture;

	fDist = (pObject->m_Pos - pContext->m_Pos).Mag();

	// Bind the texture.
	iStage = g_iCurTextureStage;
	if (!pTex)
		return;

	pRenderData = pTex->m_pRenderData;
	pTex->m_FrameCode = g_CurTextureFrameCode;

	pCur = LTNULL;
	if (pRenderData)
		pCur = d3d_GetStageTexture(pRenderData, (uint8)iStage);

	if (pCur)
	{
		if (pCur != g_pCurTextures[iStage])
			d3d_SetTextureDirect(pCur);
	}
	else
	{
		pRenderData = pTex->m_pRenderData;
		if (pRenderData)
		{
			pCur = d3d_CreateRenderTexture(pTex, iStage, LTTRUE);
			if (!pCur)
				return;

			pCur->m_pNext = pRenderData->m_pNext;
			pRenderData->m_pNext = pCur;
		}
		else
		{
			pCur = d3d_CreateRenderTexture(pTex, iStage, LTFALSE);
			if (!pCur)
				return;
		}

		d3d_SetTextureDirect(pCur);
	}

	if (pCur->m_LOD != 0)
	{
		pCur->m_pSurface->SetLOD(0);
		pCur->m_LOD = 0;
	}

	g_SavedStates.Restore();
	if (pTex->m_pStateList)
		g_SavedStates.Apply(pTex->m_pStateList, g_iCurTextureStage);

	fWidth = (float)(int32)(g_pCurTextures[g_iCurTextureStage]->GetWidth() >> pTex->m_MipShift);
	fHeight = (float)(int32)(g_pCurTextures[g_iCurTextureStage]->GetHeight() >> pTex->m_MipShift);

	g_pfnCalcFog(&pObject->m_Pos, &specular);

	// A texel in from the edges.
	uMin = g_fTexelSizeU + g_fTexelSizeU;
	uMax = (fWidth - 2.0f) * g_fTexelSizeU;
	vMin = g_fTexelSizeV + g_fTexelSizeV;
	vMax = (fHeight - 2.0f) * g_fTexelSizeV;

	// A rotation gone bad is reset.
	if (pObject->m_Rotation.m_Quat[0] > 100000.0f || pObject->m_Rotation.m_Quat[0] < -100000.0f)
		pObject->m_Rotation.m_Quat[0] = 0.0f;
	if (pObject->m_Rotation.m_Quat[1] > 100000.0f || pObject->m_Rotation.m_Quat[1] < -100000.0f)
		pObject->m_Rotation.m_Quat[1] = 0.0f;
	if (pObject->m_Rotation.m_Quat[2] > 100000.0f || pObject->m_Rotation.m_Quat[2] < -100000.0f)
		pObject->m_Rotation.m_Quat[2] = 0.0f;
	if (pObject->m_Rotation.m_Quat[3] > 100000.0f || pObject->m_Rotation.m_Quat[3] < -100000.0f)
		pObject->m_Rotation.m_Quat[3] = 1.0f;

	d3d_RotationToMatrix(&pObject->m_Rotation, &mRot);

	mRot.m[0][0] *= pObject->m_ScaleX;
	mRot.m[0][3] = pObject->m_Pos.x;
	mRot.m[1][3] = pObject->m_Pos.y;
	mRot.m[2][3] = pObject->m_Pos.z;
	mRot.m[1][0] *= pObject->m_ScaleX;
	vForward.Init(0.0f, 0.0f, -1.0f);
	mRot.m[2][0] *= pObject->m_ScaleX;
	mRot.m[0][1] *= pObject->m_ScaleY;
	mRot.m[1][1] *= pObject->m_ScaleY;
	mRot.m[2][1] *= pObject->m_ScaleY;
	mRot.m[0][2] *= pObject->m_ScaleZ;
	mRot.m[1][2] *= pObject->m_ScaleZ;
	mRot.m[2][2] *= pObject->m_ScaleZ;

	MatVMul_3x3(&vDir, &mRot, &vForward);
	vForward = vDir;

	// Which way round the camera sees it.
	vToCamera = pContext->m_Pos - *pPos;
	pOrder = (vToCamera.Dot(vForward) < 0.0f) ? g_SpriteBackOrder : g_SpriteFrontOrder;

	color.a = pObject->m_ColorA;
	if (!(pObject->m_Flags & PHFLAG_NOLIGHT) && g_pSceneWorld)
	{
		d3d_GetLightTableColor(g_pSceneWorld->m_LightTable, &pObject->m_Pos, &lightColor);
		d3d_GetDynamicLightAdd((ModelInstance*)pObject, &vAdd);

		r = ((float)pObject->m_ColorR * g_vGlobalLightColor.x + vAdd.x) * ((float)lightColor.r * (1.0f / 255.0f));
		if (r < 0.0f)
			r = 0.0f;
		else if (r > 255.0f)
			r = 255.0f;

		g = ((float)pObject->m_ColorG * g_vGlobalLightColor.y + vAdd.y) * ((float)lightColor.g * (1.0f / 255.0f));
		if (g < 0.0f)
			g = 0.0f;
		else if (g > 255.0f)
			g = 255.0f;

		b = ((float)pObject->m_ColorB * g_vGlobalLightColor.z + vAdd.z) * ((float)lightColor.b * (1.0f / 255.0f));
		if (b < 0.0f)
			b = 0.0f;
		else if (b > 255.0f)
			b = 255.0f;

		color.r = (uint8)RoundF(r);
		color.g = (uint8)RoundF(g);
		color.b = (uint8)RoundF(b);
	}
	else
	{
		color.r = (uint8)RoundF((float)pObject->m_ColorR * g_vGlobalLightColor.x);
		color.g = (uint8)RoundF((float)pObject->m_ColorG * g_vGlobalLightColor.y);
		color.b = (uint8)RoundF((float)pObject->m_ColorB * g_vGlobalLightColor.z);
	}

	// The quad, in the sprite's own space.
	verts[pOrder[0]].m_vPos.Init(fWidth, fHeight, 0.0f);
	verts[pOrder[0]].m_fU = uMin;
	verts[pOrder[0]].m_fV = vMin;
	verts[pOrder[0]].m_Diffuse = *(uint32*)&color;
	verts[pOrder[0]].m_Specular = specular;

	verts[pOrder[1]].m_vPos.Init(-fWidth, fHeight, 0.0f);
	verts[pOrder[1]].m_fU = uMax;
	verts[pOrder[1]].m_fV = vMin;
	verts[pOrder[1]].m_Diffuse = *(uint32*)&color;
	verts[pOrder[1]].m_Specular = specular;

	verts[pOrder[2]].m_vPos.Init(-fWidth, -fHeight, 0.0f);
	verts[pOrder[2]].m_fU = uMax;
	verts[pOrder[2]].m_fV = vMax;
	verts[pOrder[2]].m_Diffuse = *(uint32*)&color;
	verts[pOrder[2]].m_Specular = specular;

	verts[pOrder[3]].m_vPos.Init(fWidth, -fHeight, 0.0f);
	verts[pOrder[3]].m_fU = uMin;
	verts[pOrder[3]].m_fV = vMax;
	verts[pOrder[3]].m_Diffuse = *(uint32*)&color;
	verts[pOrder[3]].m_Specular = specular;

	d3d_TransformSpriteVerts((SpriteVert*)verts, 4, &mRot);

	pVerts = verts;
	nVerts = 4;
	if (pObject->m_ClipperPoly != INVALID_HPOLY)
	{
		if (!d3d_ClipSprite(pObject, pObject->m_ClipperPoly, &pVerts, &nVerts, clippedVerts))
			return;
	}

	if (pObject->m_Flags & PHFLAG_SPRITEBIAS)
	{
		if (!d3d_ClipSpriteVerts(&pVerts, &nVerts, &g_ViewParams, 0, SPRITE_POSITION_ZBIAS))
			return;
	}
	else
	{
		if (!d3d_ClipTLVerts((TLVertex**)&pVerts, (int*)&nVerts, &g_ViewParams, 0))
			return;
	}

	d3d_DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, pVerts, nVerts, 0);
}


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


// 0x1002fdd0 -- the sprite's vertices to camera space, clipped, and
// projected with the Z bias (held off the near plane).
extern float			g_fSpriteNearZ;                 // 0x1005b74c

// The clip vertex, and the clip planes (TU06's ClipNear 0x1000b07d and
// ClipLeft 0x1000b1bc, TU03's ClipTop 0x100063e0, ClipRight 0x10006670,
// ClipBottom 0x10006900, ClipFar 0x10006ba0).

// A clipped vertex's texture coordinates and colours (ph/d3d_clip.h's).
inline void LerpClipVert(ClipVert* a, ClipVert* b, ClipVert* dst, float t)
{
	dst->m_fU = (b->m_fU - a->m_fU) * t + a->m_fU;
	dst->m_fV = (b->m_fV - a->m_fV) * t + a->m_fV;
	((uint8*)&dst->m_Diffuse)[2]  = (uint8)RoundF((((uint8*)&b->m_Diffuse)[2]  - ((uint8*)&a->m_Diffuse)[2])  * t + ((uint8*)&a->m_Diffuse)[2]);
	((uint8*)&dst->m_Diffuse)[1]  = (uint8)RoundF((((uint8*)&b->m_Diffuse)[1]  - ((uint8*)&a->m_Diffuse)[1])  * t + ((uint8*)&a->m_Diffuse)[1]);
	((uint8*)&dst->m_Diffuse)[0]  = (uint8)RoundF((((uint8*)&b->m_Diffuse)[0]  - ((uint8*)&a->m_Diffuse)[0])  * t + ((uint8*)&a->m_Diffuse)[0]);
	((uint8*)&dst->m_Diffuse)[3]  = (uint8)RoundF((((uint8*)&b->m_Diffuse)[3]  - ((uint8*)&a->m_Diffuse)[3])  * t + ((uint8*)&a->m_Diffuse)[3]);
	((uint8*)&dst->m_Specular)[3] = (uint8)RoundF((((uint8*)&b->m_Specular)[3] - ((uint8*)&a->m_Specular)[3]) * t + ((uint8*)&a->m_Specular)[3]);
}


// ---------------------------------------------------------------------- //
//  0x1002f970 -- clip a sprite's vertices to the edges of the world
//  polygon it is clipped to (its ClipperPoly), when the viewer is in
//  front of it.  The vertices go into pOut.
// ---------------------------------------------------------------------- //
#define PLANETEST(pt) (VEC_DOT(thePlane.m_Normal, pt) - thePlane.m_Dist > 0.0f)
#define DOPLANECLIP(pt1, pt2) \
	d1 = VEC_DOT(thePlane.m_Normal, pt1) - thePlane.m_Dist; \
	d2 = VEC_DOT(thePlane.m_Normal, pt2) - thePlane.m_Dist; \
	t = -d1 / (d2 - d1); \
	pOut->m_vPos.x = pt1.x + ((pt2.x - pt1.x) * t);\
	pOut->m_vPos.y = pt1.y + ((pt2.y - pt1.y) * t);\
	pOut->m_vPos.z = pt1.z + ((pt2.z - pt1.z) * t);

LTBOOL d3d_ClipSprite(LTObject *pInstance, uint32 hPoly, ClipVert **ppPoints, int32 *pnPoints, ClipVert *pOut)
{
	LTPlane thePlane;
	float dot, d1, d2, t;
	SpritePolyVert *pPrevPoint, *pCurPoint, *pEndPoint;
	LTVector vecTo;
	ClipVert *pVerts;
	int32 nVerts;
	SpriteWorldPoly *pPoly;
	int bInside[50], *pInside;
	int32 nInside, iPrev, iCur;
	ClipVert *pPrev, *pCur, *pEnd, *pOldOut;

	if (!g_pSceneWorld)
		return LTFALSE;

	// Get the correct poly.
	pPoly = g_pSceneWorld->GetPolyFromHPoly(hPoly);
	if (!pPoly)
		return LTFALSE;

	// First see if the viewer is on the frontside of the poly.
	dot = pPoly->m_pPlane->m_Normal.Dot(g_ViewParams.m_Pos) - pPoly->m_pPlane->m_Dist;
	if (dot <= 0.01f)
		return LTFALSE;

	pVerts = *ppPoints;
	nVerts = *pnPoints;

	// Clip on each edge plane.
	pEndPoint = &pPoly->m_Verts[pPoly->m_nVerts];
	pPrevPoint = pEndPoint - 1;
	for (pCurPoint=pPoly->m_Verts; pCurPoint != pEndPoint; )
	{
		VEC_SUB(vecTo, *pCurPoint->m_pVec, *pPrevPoint->m_pVec);
		VEC_CROSS(thePlane.m_Normal, vecTo, pPoly->m_pPlane->m_Normal);
		VEC_NORM(thePlane.m_Normal);
		thePlane.m_Dist = VEC_DOT(thePlane.m_Normal, *pCurPoint->m_pVec);

		g_nClipPasses++;

		nInside = 0;
		pCur = pVerts;
		pEnd = pCur + nVerts;
		pInside = bInside;
		while (pCur != pEnd)
		{
			*pInside = PLANETEST(pCur->m_vPos);
			nInside += *pInside;
			++pInside;
			++pCur;
		}

		if (nInside == 0)
		{
			return LTFALSE;
		}
		else if (nInside != nVerts)
		{
			pOldOut = pOut;

			iPrev = nVerts - 1;
			pPrev = pVerts + iPrev;
			for (iCur=0; iCur < nVerts; iCur++)
			{
				pCur = pVerts + iCur;

				if (bInside[iPrev])
					*pOut++ = *pPrev;

				if (bInside[iPrev] != bInside[iCur])
				{
					DOPLANECLIP(pPrev->m_vPos, pCur->m_vPos)

					LerpClipVert(pPrev, pCur, pOut, t);

					++pOut;
				}

				iPrev = iCur;
				pPrev = pCur;
			}

			nVerts = pOut - pOldOut;
			pVerts = pOldOut;
			pOut += nVerts;
		}

		pPrevPoint = pCurPoint;
		++pCurPoint;
	}

	*ppPoints = pVerts;
	*pnPoints = nVerts;
	return LTTRUE;
}


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

LTBOOL d3d_ClipSpriteVerts(ClipVert **ppVerts, int32 *pnVerts, ViewParams *pParams, uint32 flags, float fZBias)
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
