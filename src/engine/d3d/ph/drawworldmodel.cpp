// *********************************************************************** //
//  ph/drawworldmodel.cpp -- Primal Hunt's d3d.ren, TU34
//  (0x100300f0..0x10031a21), /O2 /Ob1 (/Ob2 expands d3d_LightWorldPoly
//  into both of 0x10030220's loops; retail expands it in one).
//
//  World models: the OT_WORLDMODEL / OT_CONTAINER ProcessObject handler
//  (0x1004da9c), the three world-model sets (solid, translucent,
//  chromakey), and their draw.  Translucent world models go on the sorted
//  list, except those whose first surface asks not to be sorted, which are
//  queued after the rest with their own blend states.
//
//  AvP2 1.0.9.6's counterpart is w31_10066d10_qipni.cpp (0x10067aa0, the
//  draw), d3dr2d.cpp (0x10068840) and d3dr2a.cpp (0x10068890).
//
//  The world-model polygon draws (0x10030220 and the translucent
//  0x10031150) light a polygon's vertices through the inline
//  d3d_LightWorldPoly, which calls d3d_draw.cpp's d3d_AddLightAnim
//  (0x10019040); the animated-lightmap path's copy, 0x10030560, blends
//  the frames itself.
//  Near: 0x10030220, 0x10030560, 0x10030e50 and 0x10031150 (register
//  choice and scheduling; 0x10031150 is ~6% longer).
//  Near: d3d_DrawWorldModel 0x10030860, size-exact; only the order of the
//  products in the clip-plane distance and in the light position's
//  MatVMul_H differs (x87 operand ordering).
// *********************************************************************** //

#include <d3d.h>

#include "ltbasedefs.h"
#include <math.h>


// The renderer's console variable (TU12's, constructor 0x10011f36).
class CD3DConVar
{
public:
	CD3DConVar(const char* pName, float fDefault, int32* pnValue, float* pfValue);

	int32		m_nValue;
	float		m_fValue;
	float		m_fDefault;
	int32*		m_pnValue;
	float*		m_pfValue;
	const char*	m_pName;
	void*		m_hParam;
	CD3DConVar*	m_pNext;
};


// ---------------------------------------------------------------------- //
//  The structures, as this file reads them.
// ---------------------------------------------------------------------- //
// A texture in video memory, one for each stage it is bound to.
class RTexture
{
public:
	virtual void	Slot00();

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
	uint8				m_pad_10[0x28 - 0x10];
	SharedTexture*		m_pLinkedTexture;               // +0x28 (detail or env map)
	uint32				m_LinkType;                     // +0x2c (0 detail, else env map)
	uint16				m_FrameCode;                    // +0x30
	uint8				m_pad_32[0x34 - 0x32];
	CRenderStateList*	m_pStateList;                   // +0x34
};

struct WorldSurface
{
	uint8		m_pad_00[0x2c];
	SharedTexture*	m_pTexture;                         // +0x2c
	uint32		m_Flags;                                // +0x30
	uint8		m_pad_34[0x3a - 0x34];
	uint16		m_iLMAnim;                              // +0x3a
};

#define PHSURF_INVISIBLE		0x4
#define PHSURF_TRANSLUCENT		0x8
#define PHSURF_SKY				0x80
#define PHSURF_FLASH			0x8000
#define PHSURF_NOSORT			0x80000
#define LMANIM_NONE				0x7fff

// A polygon vertex: its colour, or the dynamic light cleared as a whole.
struct LitVertex
{
	LTVector*	m_pVec;                                 // +0x00
	float		m_U, m_V;                               // +0x04
	uint8		m_Reserved0c[0x14 - 0x0c];
	union
	{
		uint8	m_Color[4];                             // +0x14 (b, g, r, a)
		uint32	m_DynLight;
	};
};

// A polygon's reference to a light animation and its first vertex there.
struct LightAnimRef
{
	uint16		m_iLight;                               // +0x00
	uint16		m_iVert;                                // +0x02
};

struct WorldPoly
{
	uint8			m_pad_00[0x0c];
	LightAnimRef*	m_pLightIndices;                    // +0x0c
	uint32			m_nLightIndices;                    // +0x10
	uint16			m_Flags;                            // +0x14
	uint8			m_pad_16[0x18 - 0x16];
	LTVector		m_vCenter;                          // +0x18
	float			m_fRadius;                          // +0x24
	LTPlane*		m_pPlane;                           // +0x28
	WorldSurface*	m_pSurface;                         // +0x2c
	struct DynLightNode*	m_pLights;                  // +0x30
	uint8			m_pad_34[0x46 - 0x34];
	uint16			m_FrameCode;                        // +0x46
	uint8			m_pad_48[0x50 - 0x48];
	LitVertex*		m_pFixedVerts;                      // +0x50
	uint16			m_nVerts;                           // +0x54
	uint16			m_nFixedVerts;                      // +0x56
	LitVertex		m_Verts[1];                         // +0x58
};

#define WPF_DYNAMICLIGHT		0x4000

// A dynamic light on a polygon's list (the light and where it is).
struct LightObject
{
	uint8		m_pad_000[0x94];
	uint8		m_ColorR, m_ColorG, m_ColorB, m_ColorA;     // +0x094
	uint8		m_pad_098[0x1b0 - 0x98];
	float		m_LightRadius;                          // +0x1b0
};

struct DynLightNode
{
	DynLightNode*	m_pNext;                            // +0x00
	LightObject*	m_pLight;                           // +0x04
	LTVector		m_Pos;                              // +0x08
};

// A node of a world model's BSP: its splitting polygon and its sides.
struct WorldNode
{
	WorldPoly*	m_pPoly;                                // +0x00
	WorldNode*	m_Sides[2];                             // +0x04
	uint8		m_pad_0c[0x16 - 0x0c];
	uint8		m_Flags;                                // +0x16
};

#define NF_IN		1
#define NF_OUT		2

#define MAX_WMPOLY_VERTS	128
#define MAX_WMNODE_STACK	1024

// A light animation's frame: a colour for each of its vertices.
struct LightAnimFrame
{
	uint8		m_Reserved00[0x08];
	uint8*		m_R;                                    // +0x08
	uint8*		m_G;                                    // +0x0c
	uint8*		m_B;                                    // +0x10
	uint8		m_nVerts;                               // +0x14
	uint8		m_Reserved15[3];
};

// The world's light animations (its m_Lights).
struct LightAnim
{
	uint8		m_Reserved00[0x24];
	uint8**		m_Frames;                               // +0x24
	uint8		m_Reserved28[0x30 - 0x28];
	uint16		m_nVerts;                               // +0x30
	uint16		m_Reserved32;
	int32		m_iFrames[2];                           // +0x34
	uint8		m_Blend;                                // +0x3c
	uint8		m_Reserved3d[0x40 - 0x3d];
	float		m_fBrightness;                          // +0x40
	uint8		m_Reserved44[0x60 - 0x44];
};

struct LightTable;

struct SceneWorld
{
	uint8		m_pad_00[0x08];
	LightAnim*	m_Lights;                               // +0x08
	uint32		m_nLights;                              // +0x0c
	uint8		m_pad_10[0xfc - 0x10];
	uint8		m_LightTable[1];                        // +0xfc (the light grid)
};

// A colour's bytes.
struct ColorBytes
{
	uint8		b, g, r, a;
};

#define COLORBYTES(c)	(*(ColorBytes*)&(c))

// The 40-byte clip vertex (FVF 0x2c4) of ph/d3d_clip.h, and its clipper,
// whose one copy is drawmodel_clip.cpp's (0x10008779).
union VColor
{
	uint32		m_n;
	uint8		m_b[4];
};

struct ClipVert2
{
	LTVector	m_vPos;                                 // +0x00
	float		m_fRHW;                                 // +0x0c
	VColor		m_Diffuse;                              // +0x10
	VColor		m_Specular;                             // +0x14
	float		m_fU;                                   // +0x18
	float		m_fV;                                   // +0x1c
	float		m_fU2;                                  // +0x20
	float		m_fV2;                                  // +0x24
};

template<class V>
LTBOOL ClipPoly(uint32 nClipFlags, V** ppVerts, int32* pnVerts);

// A transformed vertex, as the shared helpers take it.
struct TLVertex
{
	LTVector	m_Vec;                                  // +0x00
	float		m_RHW;                                  // +0x0c
	uint32		m_Color;                                // +0x10
	uint32		m_Specular;                             // +0x14
	float		m_TU, m_TV;                             // +0x18
};
struct RenderPoly;

class CMulTable
{
public:
	CMulTable();

	uint8		m_Table[256][256];
};

extern CMulTable	g_MulTable;                         // 0x10087c00
// d3d_tables.cpp's saturating add: an identity table, then 256 bytes of
// 0xFF, so a sum of two bytes indexes it (into m_Full) clamped.
class CIdentTable
{
public:
			CIdentTable();

	uint8	m_Ident[256];
	uint8	m_Full[256];
};

extern CIdentTable	g_IdentTable;                       // 0x10097c00 (d3d_tables.cpp)

class WorldBsp
{
public:
	virtual uint32	GetBspType();                       // +0x00
	virtual WorldNode*	GetRootNode();                  // +0x04
	virtual void	Slot08();
	virtual void	Slot0c();
	virtual void	Slot10();
	virtual void	Slot14();
	virtual void	Slot18();
	virtual void	Slot1c();
	virtual void	Slot20();
	virtual void	Slot24();
	virtual float	GetBoundRadius();                   // +0x28

	uint8		m_pad_04[0x54 - 0x04];
	WorldPoly**	m_TerrainPolies;                        // +0x54
	uint32		m_nTerrainPolies;                       // +0x58
	uint8		m_pad_5c[0xa0 - 0x5c];
	WorldPoly**	m_Polies;                               // +0xa0
	uint32		m_nPolies;                              // +0xa4
};

class LTObject
{
public:
	virtual void	Slot00();
	virtual void	Slot04();
	virtual void	Slot08();
	virtual void	Slot0c();
	virtual void	Slot10();
	virtual void	Slot14();
	virtual void	Slot18();
	virtual void	Slot1c();
	virtual void	Slot20();
	virtual void	Slot24();
	virtual void	Slot28();
	virtual void	Slot2c();
	virtual void	Slot30();
	virtual void	Slot34();
	virtual LTBOOL	IsMainWorldModel();                 // +0x38

	uint8		m_pad_004[0x88 - 0x04];
	uint32		m_Flags;                                // +0x088
	uint32		m_Flags2;                               // +0x08c
	uint8		m_pad_090[0x94 - 0x90];
	uint8		m_ColorR, m_ColorG, m_ColorB, m_ColorA;     // +0x094
	uint8		m_pad_098[0x194 - 0x98];
	LTVector	m_Pos;                                  // +0x194
	uint8		m_pad_1a0[0x1b0 - 0x1a0];
	WorldBsp*	m_pWorldBsp;                            // +0x1b0
	uint8		m_pad_1b4[0x1f8 - 0x1b4];
	LTMatrix	m_Transform;                            // +0x1f8
	LTMatrix	m_BackTransform;                        // +0x238
};

#define PHFLAG_FOGDISABLE		0x80
#define PHFLAG2_CHROMAKEY		0x2
#define PHFLAG2_ADDITIVE		0x4
#define PHFLAG2_PORTALOBJECT	0x8000

struct ViewParams
{
	uint8		m_pad_000[0x15c];
	LTMatrix	m_mWorldToCamera;                       // +0x15c
	LTMatrix	m_mProjection;                          // +0x19c
	LTMatrix	m_mFullTransform;                       // +0x1dc
	uint8		m_pad_21c[0x3d0 - 0x21c];
	LTPlane		m_ClipPlanes[6];                        // +0x3d0
	uint8		m_pad_430[0x4b4 - 0x430];
	LTVector	m_vLightPos;                            // +0x4b4
	uint8		m_pad_4c0[0x4d4 - 0x4c0];
	uint32		m_bDrawHidden;                          // +0x4d4
	uint8		m_pad_4d8[0x4e4 - 0x4d8];
	LTVector	m_vCameraPos;                           // +0x4e4

	void	SetCameraPos(LTVector vPos);                // 0x1000ff40 (d3d_viewparams.cpp)
};

typedef void (*DrawObjectFn)(ViewParams* pContext, LTObject* pObject);

extern void		AddDebugMessage(int32 debugLevel, const char* pMsg, ...);  // 0x10013b13

class BaseObjectSet
{
public:

				BaseObjectSet()
				{
					m_Link.m_pData = this;
					m_nObjects = 0;
					m_pObjects = NULL;
					m_pSetName = "";
					m_nMaxObjects = 0;
				}

	LTBOOL		IsEmpty()	{return m_nObjects == 0;}

	void		ClearSet()
	{
		m_nObjects = 0;
	}

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
};

template<int maxObjects>
class ObjectSet : public BaseObjectSet
{
public:

				ObjectSet()
				{
					m_nMaxObjects = maxObjects;
					m_pObjects = m_ObjectArray;
				}

	LTObject	*m_ObjectArray[maxObjects];
};

class AllocSet : public BaseObjectSet
{
public:
	LTObject	**m_pArray;                             // +0x1c
};

struct LMAnimEntry
{
	WorldPoly*	m_pPoly;                                // +0x00
	LTMatrix*	m_pTransform;                           // +0x04
};

#define MAX_LMANIM_POLIES	32

struct VisibleSet
{
	void		AddLMAnimPoly(WorldPoly *pPoly, LTMatrix *pTransform)
	{
		uint32 i;
		LMAnimEntry *pEntry;

		if(m_nLMAnimPolies >= MAX_LMANIM_POLIES)
			return;

		for(i=0; i < m_nLMAnimPolies; i++)
		{
			if(m_LMAnimPolies[i].m_pPoly == pPoly)
				return;
		}

		pEntry = &m_LMAnimPolies[m_nLMAnimPolies];
		pEntry->m_pPoly = pPoly;
		pEntry->m_pTransform = pTransform;
		m_nLMAnimPolies++;
	}

	uint8			m_pad_000[0x40];
	LMAnimEntry		m_LMAnimPolies[MAX_LMANIM_POLIES];  // +0x040
	uint32			m_nLMAnimPolies;                    // +0x140
	uint8			m_pad_144[0x1e4 - 0x144];
	AllocSet		m_SolidWorldModels;                 // +0x1e4
	AllocSet		m_TranslucentWorldModels;           // +0x204
	AllocSet		m_ChromakeyWorldModels;             // +0x224
};

class ObjectDrawList
{
public:
	void	Add(LTObject* pObject, DrawObjectFn fn);    // 0x10029980
};

extern LPDIRECT3DDEVICE7	g_pD3DDevice;               // 0x100638c8
extern ObjectDrawList*		g_pObjectDrawList;          // 0x100713cc
extern ViewParams			g_ViewParams;             // 0x1005b790

extern VisibleSet*	d3d_GetVisibleSet();                // 0x1003abb0
extern LTBOOL			d3d_ProcessPortalObject(LTObject *pObject);         // 0x1000fc52 (fogvolume.cpp)

extern void			d3d_DrawWorldModel(ViewParams *pContext, LTObject *pObject);            // 0x10030860
extern void			d3d_DrawWorldModelPolies(LTObject *pObject, WorldPoly **ppPolies, uint32 nPolies,
						LTVector *pCameraPos, uint32 nClipFlags);                         // 0x10030220
extern void			d3d_SetDrawMode(int32 nMode);                                         // 0x10014f30
extern void			d3d_FlushDeferredPolies(LTBOOL bAdditive);                             // 0x10015070
extern LTVector		g_vGlobalLightColor;        // 0x1005b780
extern uint8		g_VertexAlpha;              // 0x1005d20c
extern void			d3d_DrawTranslucentWorldModel(ViewParams *pContext, LTObject *pObject); // 0x10030e50

extern SceneWorld*	g_pSceneWorld;                      // 0x1005c208
extern uint16		g_CurFrameCode;                     // 0x1005d238
extern uint32		g_ClipFlags;                        // 0x1005bd0c
extern uint32		g_nWorldPoliesDrawn;                // 0x1005c120
extern uint32		g_bAnimateLightmaps;                // 0x100628b0
extern int32		g_CV_FixTJunc;                      // 0x1005dbb4

extern uint32		g_FogColorRGB;                      // 0x1005d230
extern uint32		g_nSkyLightsApplied;                // 0x1005c164
extern int32		g_CV_Saturate;                      // 0x1005d384
extern CD3DConVar	g_CV_EnvMapWorld;                   // 0x10055438 (drawworld.cpp)
extern CD3DConVar	g_CV_DetailTextures;                // 0x10055478 (drawworld.cpp)
extern LTBOOL		g_100638c4;                         // 0x100638c4 (d3d_init.cpp)
extern uint32		g_iCurTextureStage;                 // 0x100638c0
extern uint16		g_CurTextureFrameCode;              // 0x1005d250
extern RTexture*	g_pCurTextures[];                   // 0x10067270
// A texture stage's texel size (d3d_texture.cpp's, one per stage).
struct TexPair
{
	float	u, v;
};

extern TexPair		g_TexelSize[];                      // 0x100672a8 (d3d_texture.cpp)
extern CRenderStateList	g_SavedStates;                  // 0x10069728 (d3d_savedstates.cpp)
extern void			(__fastcall *g_pfnCalcFog)(const LTVector* pPos, uint32* pSpecular);  // 0x1005e1c4 (d3d_draw.cpp)

extern void		dd_ConsolePrint(const char* pMsg, ...); // 0x10013ade
struct LightTable;
extern void		d3d_GetLightGridColor(LightTable* pTable, LTVector* pPos, uint8* pColor);  // 0x1000c818 (drawmodel.cpp)
extern void		d3d_CalcEnvMapUV(const LTVector* pEye, const LTVector* pPos,
					const LTVector* pNormal, float* pU, float* pV);     // 0x100115fe (common_draw.cpp)
extern void		d3d_ProjectTLVert(TLVertex* pVert, ViewParams* pParams);   // 0x1002739d
extern LTBOOL	d3d_DrawPolyFlat(RenderPoly* pPoly);   // 0x10014b80
extern int		d3d_SetTexture(SharedTexture* pTexture, uint32 iStage, uint32 flags);  // 0x100079e4
extern void		d3d_SetDetailTextureOp();               // 0x1000a1ea
extern void		d3d_UnsetDetailTextureOp();             // 0x1000a20e
extern RTexture*	d3d_CreateRenderTexture(SharedTexture* pTexture, uint32 iStage, LTBOOL bStageCopy);  // 0x10020cd0
extern void		d3d_SetTextureDirect(RTexture* pTexture);   // 0x10007a89
extern RTexture*	d3d_GetStageTexture(RTexture* pTexture, uint8 iStage);  // 0x10009300

// The polygon draw functions the surface selects.
// The world-poly drawers d3d_draw.cpp and drawlightmaps.cpp set up.
struct RenderPoly;
extern void (*g_pfnDrawGouraudPoly)(RenderPoly *pPoly);     // 0x1005e280 (d3d_draw.cpp)
extern void (*g_pfnDrawLightmapPoly)(RenderPoly *pPoly);    // 0x1005fd9c (d3d_draw.cpp)
extern void (*g_pfnDrawTexturedPoly)(RenderPoly *pPoly);    // 0x1005e6bc (d3d_draw.cpp)
extern void (*g_pfnDrawDeferredPoly)(RenderPoly *pPoly);    // 0x1005e770 (d3d_draw.cpp)


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


// d3d_texture.cpp's d3d_SetTexture, as a header inline: bind a shared
// texture's copy for a stage, made (and chained) if there is none yet.
inline LTBOOL d3d_BindTexture(SharedTexture* pTexture, uint32 iStage, uint32 iLOD)
{
	RTexture *pRenderData, *pCur;

	pRenderData = pTexture->m_pRenderData;
	pTexture->m_FrameCode = g_CurTextureFrameCode;

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
		pRenderData = pTexture->m_pRenderData;
		if (pRenderData)
		{
			pCur = d3d_CreateRenderTexture(pTexture, iStage, LTTRUE);
			if (!pCur)
				return LTFALSE;

			pCur->m_pNext = pRenderData->m_pNext;
			pRenderData->m_pNext = pCur;
		}
		else
		{
			pCur = d3d_CreateRenderTexture(pTexture, iStage, LTFALSE);
			if (!pCur)
				return LTFALSE;
		}

		d3d_SetTextureDirect(pCur);
	}

	if (pCur->m_LOD != iLOD)
	{
		pCur->m_pSurface->SetLOD(iLOD);
		pCur->m_LOD = (uint16)iLOD;
	}

	return LTTRUE;
}

// Take the texture off a stage.
inline void d3d_UnbindTexture(uint32 iStage)
{
	if (g_pCurTextures[iStage])
	{
		g_pD3DDevice->SetTexture(iStage, LTNULL);
		g_pCurTextures[iStage] = LTNULL;
	}
}

// 0x10008719 -- a vertex into view space (drawmodel_clip.cpp's copy).
inline void d3d_TransformToCamera(TLVertex* pVert, LTMatrix* pMat)
{
	float x = pMat->m[0][0]*pVert->m_Vec.x + pMat->m[0][1]*pVert->m_Vec.y + pMat->m[0][2]*pVert->m_Vec.z + pMat->m[0][3];
	float y = pMat->m[1][0]*pVert->m_Vec.x + pMat->m[1][1]*pVert->m_Vec.y + pMat->m[1][2]*pVert->m_Vec.z + pMat->m[1][3];
	float z = pMat->m[2][0]*pVert->m_Vec.x + pMat->m[2][1]*pVert->m_Vec.y + pMat->m[2][2]*pVert->m_Vec.z + pMat->m[2][3];

	pVert->m_Vec.z = z;
	pVert->m_Vec.x = x;
	pVert->m_Vec.y = y;
}


// The surface flags of a world model (its first polygon's).
inline uint32 d3d_GetWorldModelFlags(LTObject *pObject)
{
	if (pObject->m_pWorldBsp && pObject->IsMainWorldModel() && pObject->m_pWorldBsp->m_nPolies > 0)
		return pObject->m_pWorldBsp->m_Polies[0]->m_pSurface->m_Flags;

	return 0;
}


// ---------------------------------------------------------------------- //
//  Globals.
// ---------------------------------------------------------------------- //
CD3DConVar	g_CV_DrawWorldModels("DrawWorldModels", 1.0f, LTNULL, LTNULL);  // 0x1007ad48  $E 0x100300f0

LTVector	g_vWorldModelLightAdd;                      // $E 0x10030110


// ---------------------------------------------------------------------- //
//  0x10030120 -- the PreFrame handler of the types with nothing to do.
// ---------------------------------------------------------------------- //
void d3d_NullPreFrame()
{
}


// ---------------------------------------------------------------------- //
//  0x10030130 -- the OT_WORLDMODEL and OT_CONTAINER ProcessObject handler.
// ---------------------------------------------------------------------- //
void d3d_ProcessWorldModel(LTObject *pObject)
{
	uint32 flags;

#ifdef PRIMAL_HUNT
	// Primal Hunt's fog volumes are portal objects.
	if (pObject->m_Flags2 & PHFLAG2_PORTALOBJECT)
	{
		d3d_ProcessPortalObject(pObject);
		return;
	}
#endif

	flags = d3d_GetWorldModelFlags(pObject);
	if (flags & PHSURF_TRANSLUCENT)
	{
		d3d_GetVisibleSet()->m_TranslucentWorldModels.Add(pObject);
	}
	else if (pObject->m_Flags2 & PHFLAG2_CHROMAKEY)
	{
		d3d_GetVisibleSet()->m_ChromakeyWorldModels.Add(pObject);
	}
	else
	{
		d3d_GetVisibleSet()->m_SolidWorldModels.Add(pObject);
	}
}


extern LTBOOL	d3d_AddLightAnim(LitVertex* pVerts, uint32 nVerts, LightAnim* pAnim, LightAnimRef* pRef);  // 0x10019040 (d3d_draw.cpp)

static void d3d_AnimateWorldPolyLights(SceneWorld *pWorld, WorldPoly *pPoly);


// A polygon's dynamic light: its vertices' light cleared, and every
// animated light that touches it added.
inline void d3d_LightWorldPoly(SceneWorld *pWorld, WorldPoly *pPoly)
{
	LitVertex *pVerts;
	LightAnim *pLight;
	uint32 nVerts, i;
	uint32 iLight;

	if (g_CV_FixTJunc)
	{
		pVerts = pPoly->m_pFixedVerts;
		nVerts = pPoly->m_nFixedVerts;
	}
	else
	{
		pVerts = pPoly->m_Verts;
		nVerts = pPoly->m_nVerts;
	}

	for (i=0; i < nVerts; i++)
		pVerts[i].m_DynLight = 0;

	for (i=0; i < pPoly->m_nLightIndices; i++)
	{
		iLight = pPoly->m_pLightIndices[i].m_iLight;
		if (iLight < pWorld->m_nLights)
		{
			pLight = &pWorld->m_Lights[iLight];
			if (pLight->m_iFrames[0] != -1 && pLight->m_fBrightness >= 0.02f)
				d3d_AddLightAnim(pVerts, nVerts, pLight, &pPoly->m_pLightIndices[i]);
		}
	}
}


// A polygon of a world model, drawn by its surface's draw function.
inline void d3d_DrawWorldModelPoly(WorldPoly *pPoly, LTBOOL bAnimated)
{
	WorldSurface *pSurface;
	uint32 flags;

	g_nWorldPoliesDrawn++;

	pSurface = pPoly->m_pSurface;
	flags = pSurface->m_Flags;
	if (flags & PHSURF_INVISIBLE)
		return;

	if (pSurface->m_pTexture)
	{
		if (flags & PHSURF_SKY)
		{
			g_pfnDrawTexturedPoly((RenderPoly*)pPoly);
			return;
		}

		if (pPoly->m_Flags & WPF_DYNAMICLIGHT)
		{
			if (bAnimated)
				d3d_AnimateWorldPolyLights(g_pSceneWorld, pPoly);
			else
				d3d_LightWorldPoly(g_pSceneWorld, pPoly);
			pPoly->m_Flags &= ~WPF_DYNAMICLIGHT;
		}

		if (flags & PHSURF_FLASH)
			g_pfnDrawLightmapPoly((RenderPoly*)pPoly);
		else
			g_pfnDrawGouraudPoly((RenderPoly*)pPoly);
	}
	else
	{
		g_pfnDrawDeferredPoly((RenderPoly*)pPoly);
	}
}


// ---------------------------------------------------------------------- //
//  0x10030220 -- a world model's polygons, each once a frame, those facing
//  the camera.  With lightmap animation on, the polygons with an animated
//  lightmap go to the visible set's list instead.
//
//  RETAIL BUG: the animated-lightmap test is inverted against the world's
//  (tagnodes.cpp's d3d_QueuePoly): only an INVISIBLE surface's polygon is
//  queued, and a visible one with an animated lightmap is dropped.
// ---------------------------------------------------------------------- //
void d3d_DrawWorldModelPolies(LTObject *pObject, WorldPoly **ppPolies, uint32 nPolies,
	LTVector *pCameraPos, uint32 nClipFlags)
{
	VisibleSet *pVisibleSet;
	WorldPoly *pPoly;
	WorldSurface *pSurface;
	uint16 frameCode;

	g_ClipFlags = nClipFlags;
	frameCode = g_CurFrameCode + 1;

	if (g_bAnimateLightmaps)
	{
		pVisibleSet = d3d_GetVisibleSet();

		for (; nPolies; nPolies--, ppPolies++)
		{
			pPoly = *ppPolies;
			if (pPoly->m_FrameCode == frameCode)
				continue;

			pPoly->m_FrameCode = frameCode;
			if (pPoly->m_pPlane->DistTo(*pCameraPos) <= 0.0001f)
				continue;

			pSurface = pPoly->m_pSurface;
			if (g_bAnimateLightmaps && pSurface->m_iLMAnim != LMANIM_NONE)
			{
				if (pSurface->m_Flags & PHSURF_INVISIBLE)
					pVisibleSet->AddLMAnimPoly(pPoly, &pObject->m_Transform);
			}
			else
			{
				d3d_DrawWorldModelPoly(pPoly, LTTRUE);
			}
		}
	}
	else
	{
		for (; nPolies; nPolies--, ppPolies++)
		{
			pPoly = *ppPolies;
			if (pPoly->m_FrameCode == frameCode)
				continue;

			pPoly->m_FrameCode = frameCode;
			if (pPoly->m_pPlane->DistTo(*pCameraPos) <= 0.0001f)
				continue;

			d3d_DrawWorldModelPoly(pPoly, LTFALSE);
		}
	}
}


// ---------------------------------------------------------------------- //
//  A light animation's blended frame added to a polygon's vertex colours:
//  this file's own copy of d3d_draw.cpp's d3d_AddLightAnim (0x10019040),
//  for the animated-lightmap path.
// ---------------------------------------------------------------------- //
static inline LTBOOL d3d_AddLightAnimFrame(LitVertex* pVerts, uint32 nVerts, LightAnim* pAnim, LightAnimRef* pRef)
{
	LightAnimFrame *pFrame1, *pFrame2;
	uint8 blend, invBlend;
	uint16 iVert;
	uint32 i;

	iVert = pRef->m_iVert;
	if (iVert >= pAnim->m_nVerts)
		return FALSE;

	blend = pAnim->m_Blend;
	pFrame1 = (LightAnimFrame*)(pAnim->m_Frames[pAnim->m_iFrames[0]] + iVert * sizeof(LightAnimFrame));
	pFrame2 = (LightAnimFrame*)(pAnim->m_Frames[pAnim->m_iFrames[1]] + iVert * sizeof(LightAnimFrame));

	if (blend == 0)
		pFrame2 = pFrame1;
	else if (blend == 0xFF)
		pFrame1 = pFrame2;

	if (nVerts >= LTMIN(pFrame1->m_nVerts, pFrame2->m_nVerts))
		nVerts = LTMIN(pFrame1->m_nVerts, pFrame2->m_nVerts);

	if (pFrame1 == pFrame2)
	{
		for (i = 0; i < nVerts; i++)
		{
			pVerts[i].m_Color[2] = g_IdentTable.m_Ident[pFrame1->m_R[i] + pVerts[i].m_Color[2]];
			pVerts[i].m_Color[1] = g_IdentTable.m_Ident[pFrame1->m_G[i] + pVerts[i].m_Color[1]];
			pVerts[i].m_Color[0] = g_IdentTable.m_Ident[pFrame1->m_B[i] + pVerts[i].m_Color[0]];
		}
	}
	else
	{
		invBlend = 255 - blend;
		for (i = 0; i < nVerts; i++)
		{
			pVerts[i].m_Color[2] = g_IdentTable.m_Ident[g_IdentTable.m_Ident[g_MulTable.m_Table[pFrame2->m_R[i]][blend] + g_MulTable.m_Table[pFrame1->m_R[i]][invBlend]] + pVerts[i].m_Color[2]];
			pVerts[i].m_Color[1] = g_IdentTable.m_Ident[g_IdentTable.m_Ident[g_MulTable.m_Table[pFrame2->m_G[i]][blend] + g_MulTable.m_Table[pFrame1->m_G[i]][invBlend]] + pVerts[i].m_Color[1]];
			pVerts[i].m_Color[0] = g_IdentTable.m_Ident[g_IdentTable.m_Ident[g_MulTable.m_Table[pFrame2->m_B[i]][blend] + g_MulTable.m_Table[pFrame1->m_B[i]][invBlend]] + pVerts[i].m_Color[0]];
		}
	}

	return TRUE;
}


// ---------------------------------------------------------------------- //
//  0x10030560 -- the animated-lightmap path's polygon light: as
//  d3d_LightWorldPoly, with the frames blended here.
// ---------------------------------------------------------------------- //
static void d3d_AnimateWorldPolyLights(SceneWorld *pWorld, WorldPoly *pPoly)
{
	LitVertex *pVerts;
	LightAnim *pLight;
	uint32 nVerts, i;
	uint32 iLight;

	if (g_CV_FixTJunc)
	{
		pVerts = pPoly->m_pFixedVerts;
		nVerts = pPoly->m_nFixedVerts;
	}
	else
	{
		pVerts = pPoly->m_Verts;
		nVerts = pPoly->m_nVerts;
	}

	for (i=0; i < nVerts; i++)
		pVerts[i].m_DynLight = 0;

	for (i=0; i < pPoly->m_nLightIndices; i++)
	{
		iLight = pPoly->m_pLightIndices[i].m_iLight;
		if (iLight < pWorld->m_nLights)
		{
			pLight = &pWorld->m_Lights[iLight];
			if (pLight->m_iFrames[0] != -1 && pLight->m_fBrightness >= 0.02f)
				d3d_AddLightAnimFrame(pVerts, nVerts, pLight, &pPoly->m_pLightIndices[i]);
		}
	}
}


// ---------------------------------------------------------------------- //
//  0x10030830
// ---------------------------------------------------------------------- //
void d3d_DrawSolidWorldModels()
{
	BaseObjectSet *pSet;

	if (!g_CV_DrawWorldModels.m_nValue)
		return;

	pSet = &d3d_GetVisibleSet()->m_SolidWorldModels;
	if (pSet->IsEmpty())
		return;

	pSet->Draw(&g_ViewParams, d3d_DrawWorldModel);
}


// ---------------------------------------------------------------------- //
//  0x10030860 -- a world model: culled against the view's clip planes
//  (keeping the planes it crosses), lit with its colour, and drawn with
//  the view moved into its space.
// ---------------------------------------------------------------------- //
void d3d_DrawWorldModel(ViewParams *pContext, LTObject *pObject)
{
	LTMatrix mSaveWorldToCamera, mSaveFullTransform;
	LTVector vSaveCameraPos, vCameraPos, vLightPos;
	LTPlane *pPlane;
	unsigned long nOldFog;
	float fRadius, fDist;
	uint32 nClipFlags;
	int i;

	fRadius = pObject->m_pWorldBsp->GetBoundRadius();
	nClipFlags = 0x3f;

	pPlane = g_ViewParams.m_ClipPlanes;
	for (i=0; i < 6; i++)
	{
		fDist = pPlane->DistTo(pObject->m_Pos);

		if (fDist < -fRadius)
			return;

		if (fDist > fRadius)
			nClipFlags &= ~(1 << i);

		pPlane++;
	}

	g_vWorldModelLightAdd.x = (float)pObject->m_ColorR * (1.0f / 255.0f) * g_vGlobalLightColor.x;
	g_vWorldModelLightAdd.y = (float)pObject->m_ColorG * (1.0f / 255.0f) * g_vGlobalLightColor.y;
	g_vWorldModelLightAdd.z = (float)pObject->m_ColorB * (1.0f / 255.0f) * g_vGlobalLightColor.z;
	g_VertexAlpha = pObject->m_ColorA;

	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_FOGENABLE, &nOldFog);
	if (nOldFog)
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, !(pObject->m_Flags & PHFLAG_FOGDISABLE));

	mSaveWorldToCamera = pContext->m_mWorldToCamera;
	mSaveFullTransform = pContext->m_mFullTransform;

	MatMul(&pContext->m_mWorldToCamera, &mSaveWorldToCamera, &pObject->m_Transform);
	MatMul(&pContext->m_mFullTransform, &pContext->m_mProjection, &pContext->m_mWorldToCamera);

	vSaveCameraPos = pContext->m_vCameraPos;
	MatVMul_H(&vCameraPos, &pObject->m_BackTransform, &vSaveCameraPos);
	pContext->SetCameraPos(vCameraPos);
	d3d_SetDrawMode(0);

	MatVMul_H(&vLightPos, &pObject->m_BackTransform, &pContext->m_vLightPos);

	if (pObject->m_pWorldBsp->GetBspType() == 0)
	{
		d3d_DrawWorldModelPolies(pObject, pObject->m_pWorldBsp->m_Polies,
			pObject->m_pWorldBsp->m_nPolies, &vLightPos, nClipFlags);
	}
	else if (pObject->m_pWorldBsp->GetBspType() == 1)
	{
		d3d_DrawWorldModelPolies(pObject, pObject->m_pWorldBsp->m_TerrainPolies,
			pObject->m_pWorldBsp->m_nTerrainPolies, &vLightPos, nClipFlags);
	}

	d3d_FlushDeferredPolies(FALSE);

	pContext->m_mWorldToCamera = mSaveWorldToCamera;
	pContext->m_mFullTransform = mSaveFullTransform;
	pContext->SetCameraPos(vSaveCameraPos);

	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, nOldFog);
}


// ---------------------------------------------------------------------- //
//  0x10030cb0
// ---------------------------------------------------------------------- //
void d3d_DrawChromakeyWorldModels()
{
	BaseObjectSet *pSet;

	if (!g_CV_DrawWorldModels.m_nValue)
		return;

	pSet = &d3d_GetVisibleSet()->m_ChromakeyWorldModels;
	if (pSet->IsEmpty())
		return;

	pSet->Draw(&g_ViewParams, d3d_DrawWorldModel);
}


static void d3d_DrawNoSortWorldModel(ViewParams *pContext, LTObject *pObject);

// ---------------------------------------------------------------------- //
//  0x10030ce0 -- translucent world models go on the sorted list; the
//  ones that shouldn't be sorted are queued after the rest.
// ---------------------------------------------------------------------- //
void d3d_QueueTranslucentWorldModels()
{
	ObjectSet<1024> noSortSet;
	BaseObjectSet *pSet;
	LTObject *pObject;
	uint32 i;

	if (!g_CV_DrawWorldModels.m_nValue)
		return;

	pSet = &d3d_GetVisibleSet()->m_TranslucentWorldModels;
	noSortSet.ClearSet();

	for (i=0; i < pSet->m_nObjects; i++)
	{
		pObject = pSet->m_pObjects[i];

		if (pObject->m_Flags & FLAG_VISIBLE)
		{
			if (g_ViewParams.m_bDrawHidden && (pObject->m_Flags2 & 1))
				continue;
		}
		else
		{
			if (g_ViewParams.m_bDrawHidden && !(pObject->m_Flags & 0x400))
				continue;
		}

		if (d3d_GetWorldModelFlags(pObject) & PHSURF_NOSORT)
			noSortSet.Add(pObject);
		else
			g_pObjectDrawList->Add(pObject, d3d_DrawTranslucentWorldModel);
	}

	if (noSortSet.m_nObjects > 0)
	{
		for (i=0; i < noSortSet.m_nObjects; i++)
			g_pObjectDrawList->Add(noSortSet.m_ObjectArray[i], d3d_DrawNoSortWorldModel);
	}
}


static void d3d_DrawTranslucentWorldModelPoly(WorldPoly *pPoly);

// ---------------------------------------------------------------------- //
//  0x10030e50 -- a translucent world model: lit with its colour, the view
//  moved into its space, and its BSP walked back to front so its
//  polygons blend in order.
// ---------------------------------------------------------------------- //
void d3d_DrawTranslucentWorldModel(ViewParams *pContext, LTObject *pObject)
{
	WorldNode *nodeStack[MAX_WMNODE_STACK];
	WorldNode *sideStack[MAX_WMNODE_STACK];
	LTMatrix mSaveWorldToCamera, mSaveFullTransform;
	LTVector vSaveCameraPos, vPos;
	LTMatrix *pTransform;
	WorldNode *pNode;
	WorldPoly *pPoly;
	WorldSurface *pSurface;
	unsigned long nOldFog;
	int iStack, side;

	g_vWorldModelLightAdd.x = (float)pObject->m_ColorR * (1.0f / 255.0f) * g_vGlobalLightColor.x;
	g_vWorldModelLightAdd.y = (float)pObject->m_ColorG * (1.0f / 255.0f) * g_vGlobalLightColor.y;
	g_vWorldModelLightAdd.z = (float)pObject->m_ColorB * (1.0f / 255.0f) * g_vGlobalLightColor.z;
	g_VertexAlpha = pObject->m_ColorA;

	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_FOGENABLE, &nOldFog);
	if (nOldFog)
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, !(pObject->m_Flags & PHFLAG_FOGDISABLE));

	mSaveWorldToCamera = pContext->m_mWorldToCamera;
	mSaveFullTransform = pContext->m_mFullTransform;
	vSaveCameraPos = pContext->m_vCameraPos;
	pContext->SetCameraPos(LTVector(0.0f, 0.0f, 0.0f));

	pTransform = &pObject->m_Transform;
	MatMul(&pContext->m_mWorldToCamera, &mSaveWorldToCamera, pTransform);
	MatMul(&pContext->m_mFullTransform, &pContext->m_mProjection, &pContext->m_mWorldToCamera);

	pNode = pObject->m_pWorldBsp->GetRootNode();
	iStack = 0;
	for (;;)
	{
		if (pNode->m_Flags & (NF_IN|NF_OUT))
		{
			if (iStack == 0)
				break;
		}
		else
		{
			do
			{
				vPos = g_ViewParams.m_vLightPos;
				side = (pNode->m_pPoly->m_pPlane->m_Normal.Dot(vPos) - pNode->m_pPoly->m_pPlane->m_Dist >= 0.0f) ? 1 : 0;

				nodeStack[iStack] = pNode;
				sideStack[iStack] = pNode->m_Sides[side];
				iStack++;
				if (iStack >= MAX_WMNODE_STACK)
					break;

				pNode = pNode->m_Sides[!side];
			}
			while (!(pNode->m_Flags & (NF_IN|NF_OUT)));
		}

		iStack--;
		pNode = nodeStack[iStack];
		pPoly = pNode->m_pPoly;
		if (pPoly)
		{
			g_ClipFlags = 0x3f;
			pSurface = pPoly->m_pSurface;
			if (!(pSurface->m_Flags & PHSURF_INVISIBLE))
			{
				if (g_bAnimateLightmaps && pSurface->m_iLMAnim != LMANIM_NONE)
					d3d_GetVisibleSet()->AddLMAnimPoly(pPoly, pTransform);
				else
					d3d_DrawTranslucentWorldModelPoly(pPoly);
			}
		}

		pNode = sideStack[iStack];
	}

	pContext->m_mWorldToCamera = mSaveWorldToCamera;
	pContext->m_mFullTransform = mSaveFullTransform;
	pContext->SetCameraPos(vSaveCameraPos);

	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, nOldFog);
}


// ---------------------------------------------------------------------- //
//  0x10031150 -- a translucent world model's polygon: its vertices lit
//  from the light table and the world model's colour, then by the
//  dynamic lights that reach its plane, clipped and drawn with its
//  texture (and its detail texture).
//
//  RETAIL BUG: the dynamic lights are applied to pPoly->m_nVerts
//  vertices, whatever the vertex count FixTJunc chose: with T-junctions
//  fixed, the added vertices go unlit.
// ---------------------------------------------------------------------- //
static void d3d_DrawTranslucentWorldModelPoly(WorldPoly *pPoly)
{
	ClipVert2 verts[MAX_WMPOLY_VERTS];
	ClipVert2 *pVerts, *pVert;
	LitVertex *pPolyVerts, *pPolyVert;
	SharedTexture *pTexture;
	DynLightNode *pNode;
	LightObject *pLight;
	LTVector vLightPos, vDelta;
	ColorBytes color;
	LTBOOL bEnvMap;
	int32 nVerts;
	uint32 i, nLightVerts;
	uint32 r, g, b, scaleR, scaleG, scaleB;
	int32 lr, lg, lb;
	float fLightR, fLightG, fLightB;
	float fDist, fDistSqr, fRadiusSqr, fInvRadius, fScale;

	bEnvMap = FALSE;

	if (pPoly->m_FrameCode == g_CurFrameCode + 1)
		return;

	pPoly->m_FrameCode = g_CurFrameCode + 1;

	if (pPoly->m_Flags & WPF_DYNAMICLIGHT)
	{
		d3d_LightWorldPoly(g_pSceneWorld, pPoly);
		pPoly->m_Flags &= ~WPF_DYNAMICLIGHT;
	}

	d3d_GetLightGridColor((LightTable*)g_pSceneWorld->m_LightTable, &pPoly->m_vCenter, (uint8*)&color);

	pTexture = pPoly->m_pSurface->m_pTexture;
	if (pTexture)
		bEnvMap = pTexture->m_LinkType && g_CV_EnvMapWorld.m_nValue;

	if (g_CV_FixTJunc)
	{
		pPolyVerts = pPoly->m_pFixedVerts;
		nVerts = pPoly->m_nFixedVerts;
	}
	else
	{
		pPolyVerts = pPoly->m_Verts;
		nVerts = pPoly->m_nVerts;
	}

	if (nVerts > MAX_WMPOLY_VERTS)
	{
		dd_ConsolePrint("Error: vertex buffer overflow");
		return;
	}

	r = COLORBYTES(g_FogColorRGB).r + color.r;
	g = COLORBYTES(g_FogColorRGB).g + color.g;
	b = COLORBYTES(g_FogColorRGB).b + color.b;
	scaleR = (uint32)(g_vWorldModelLightAdd.x * 255.0f);
	scaleG = (uint32)(g_vWorldModelLightAdd.y * 255.0f);
	scaleB = (uint32)(g_vWorldModelLightAdd.z * 255.0f);

	pVert = verts;
	pPolyVert = pPolyVerts;
	for (i=nVerts; i; i--)
	{
		pVert->m_vPos = *pPolyVert->m_pVec;

		lr = pPolyVert->m_Color[2] + r;
		lg = pPolyVert->m_Color[1] + g;
		lb = pPolyVert->m_Color[0] + b;
		if ((uint32)lr > 0xFF) lr = 0xFF;
		if ((uint32)lg > 0xFF) lg = 0xFF;
		if ((uint32)lb > 0xFF) lb = 0xFF;

		pVert->m_Diffuse.m_b[2] = (uint8)((lr * scaleR) >> 8);
		pVert->m_Diffuse.m_b[1] = (uint8)((lg * scaleG) >> 8);
		pVert->m_Diffuse.m_b[0] = (uint8)((lb * scaleB) >> 8);
		pVert->m_Diffuse.m_b[3] = g_VertexAlpha;

		pVert->m_fU = pPolyVert->m_U;
		pVert->m_fV = pPolyVert->m_V;

		if (bEnvMap)
			d3d_CalcEnvMapUV(&g_ViewParams.m_vLightPos, &pVert->m_vPos, &pPoly->m_pPlane->m_Normal,
				&pVert->m_fU2, &pVert->m_fV2);

		pVert++;
		pPolyVert++;
	}

	pVerts = verts;
	for (pNode = pPoly->m_pLights; pNode; pNode = pNode->m_pNext)
	{
		pLight = pNode->m_pLight;
		vLightPos = pNode->m_Pos;
		g_nSkyLightsApplied++;

		fLightR = (float)pLight->m_ColorR;
		fLightG = (float)pLight->m_ColorG;
		fLightB = (float)pLight->m_ColorB;
		fLightR -= 255.0f - fLightR;
		fLightG -= 255.0f - fLightG;
		fLightB -= 255.0f - fLightB;

		fDist = pPoly->m_pPlane->m_Normal.Dot(vLightPos) - pPoly->m_pPlane->m_Dist;
		if (fDist < 0.0f)
			fDist = -fDist;

		if (fDist >= pLight->m_LightRadius)
			continue;

		fRadiusSqr = pLight->m_LightRadius * pLight->m_LightRadius;
		fInvRadius = 1.0f / pLight->m_LightRadius;

		pVert = verts;
		for (nLightVerts=pPoly->m_nVerts; nLightVerts; nLightVerts--, pVert++)
		{
			vDelta = pVert->m_vPos - vLightPos;
			fDistSqr = vDelta.MagSqr();
			if (fDistSqr >= fRadiusSqr)
				continue;

			fScale = 1.0f - (float)sqrt(fDistSqr) * fInvRadius;
			lr = (int32)(fScale * fLightR) + pVert->m_Diffuse.m_b[2];
			lg = (int32)(fScale * fLightG) + pVert->m_Diffuse.m_b[1];
			lb = (int32)(fScale * fLightB) + pVert->m_Diffuse.m_b[0];

			if (g_CV_Saturate)
			{
				lr += lr;
				lg += lg;
				lb += lb;
			}

			pVert->m_Diffuse.m_b[2] = (uint8)LTCLAMP(lr, 0, 0xFF);
			pVert->m_Diffuse.m_b[1] = (uint8)LTCLAMP(lg, 0, 0xFF);
			pVert->m_Diffuse.m_b[0] = (uint8)LTCLAMP(lb, 0, 0xFF);
		}
	}

	g_ClipFlags = 0x3f;

	pVert = pVerts;
	for (i=nVerts; i; i--)
	{
		d3d_TransformToCamera((TLVertex*)pVert, &g_ViewParams.m_mWorldToCamera);
		g_pfnCalcFog((const LTVector*)pVert, &pVert->m_Specular.m_n);
		pVert++;
	}

	if (ClipPoly(g_ClipFlags, &pVerts, &nVerts))
	{
		pVert = pVerts;
		for (i=nVerts; i; i--)
		{
			d3d_ProjectTLVert((TLVertex*)pVert, &g_ViewParams);
			pVert++;
		}

		pTexture = pPoly->m_pSurface->m_pTexture;
		if (pTexture)
		{
			if (!d3d_BindTexture(pTexture, g_iCurTextureStage, 0))
			{
				d3d_UnsetDetailTextureOp();
				d3d_DrawPolyFlat((RenderPoly*)pPoly);
				return;
			}

			g_SavedStates.Restore();
			if (pPoly->m_pSurface->m_pTexture->m_pStateList)
				g_SavedStates.Apply(pPoly->m_pSurface->m_pTexture->m_pStateList, g_iCurTextureStage);

			pTexture = pPoly->m_pSurface->m_pTexture;
			if (g_100638c4 && g_CV_DetailTextures.m_nValue && g_pCurTextures[0] &&
				pTexture->m_pLinkedTexture &&
				(!pTexture->m_LinkType || g_CV_EnvMapWorld.m_nValue) &&
				d3d_SetTexture(pTexture->m_pLinkedTexture, 1, 0))
			{
				d3d_SetDetailTextureOp();
			}
			else
			{
				d3d_UnsetDetailTextureOp();
			}
		}
		else
		{
			d3d_UnbindTexture(g_iCurTextureStage);
			d3d_UnsetDetailTextureOp();
		}

		pVert = pVerts;
		for (i=nVerts; i; i--)
		{
			pVert->m_fU *= g_TexelSize[0].u;
			pVert->m_fV *= g_TexelSize[0].v;
			pVert++;
		}

		g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2, pVerts, nVerts, 0);
		g_SavedStates.Restore();
	}

	d3d_UnsetDetailTextureOp();
}




// ---------------------------------------------------------------------- //
//  0x100318b0 -- the unsorted translucent world models' draw.
//
//  RETAIL BUG: the blend states are StateSets scoped to the if/else arms,
//  so they are restored before the draw: the unsorted world models are
//  drawn with the ordinary translucent blend, not additively.
// ---------------------------------------------------------------------- //
static void d3d_DrawNoSortWorldModel(ViewParams *pContext, LTObject *pObject)
{
#ifdef PRIMAL_HUNT
#ifdef AVP2SRC_MODERN
	// The draw inside each arm, so it is made with that arm's states.
	if (pObject->m_Flags2 & PHFLAG2_ADDITIVE)
	{
		StateSet ssSrcBlend(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
		StateSet ssDestBlend(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
		d3d_DrawTranslucentWorldModel(pContext, pObject);
	}
	else
	{
		StateSet ssSrcBlend(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
		StateSet ssDestBlend(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
		StateSet ssFogColor(D3DRENDERSTATE_FOGCOLOR, 0);
		d3d_DrawTranslucentWorldModel(pContext, pObject);
	}
#else
	if (pObject->m_Flags2 & PHFLAG2_ADDITIVE)
	{
		StateSet ssSrcBlend(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
		StateSet ssDestBlend(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
	}
	else
	{
		StateSet ssSrcBlend(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
		StateSet ssDestBlend(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
		StateSet ssFogColor(D3DRENDERSTATE_FOGCOLOR, 0);
	}

	d3d_DrawTranslucentWorldModel(pContext, pObject);
#endif
#else
	// AvP2's: additively, with no fog colour, over the draw.
	StateSet ssSrcBlend(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
	StateSet ssDestBlend(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
	StateSet ssFogColor(D3DRENDERSTATE_FOGCOLOR, 0);

	d3d_DrawTranslucentWorldModel(pContext, pObject);
#endif
}
