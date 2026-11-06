// *********************************************************************** //
//  ph/drawworldmodel.cpp -- Primal Hunt's d3d.ren, TU34
//  (0x100300f0..0x10031a21), /O2.
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
//  Not yet written: the polygon draw 0x10030220 (822 bytes), its
//  animated lightmaps 0x10030560 (716), d3d_DrawTranslucentWorldModel
//  0x10030e50 (764) and 0x10031150 (1888).
//  Near: d3d_DrawWorldModel 0x10030860, size-exact; only the order of the
//  products in the clip-plane distance and in the light position's
//  MatVMul_H differs (x87 operand ordering).
// *********************************************************************** //

#include <d3d.h>

#include "ltbasedefs.h"


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
struct WorldSurface
{
	uint8		m_pad_00[0x30];
	uint32		m_Flags;                                // +0x30
};

#define PHSURF_TRANSLUCENT		0x8
#define PHSURF_NOSORT			0x80000

struct WorldPoly
{
	uint8			m_pad_00[0x2c];
	WorldSurface*	m_pSurface;                         // +0x2c
};

class WorldBsp
{
public:
	virtual uint32	GetBspType();                       // +0x00
	virtual void	Slot04();
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

struct VisibleSet
{
	uint8			m_pad_000[0x1e4];
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
						LTVector *pLightPos, uint32 nClipFlags);                          // 0x10030220
extern void			d3d_SetDrawMode(int32 nMode);                                         // 0x10014f30
extern void			d3d_FlushDeferredPolies(LTBOOL bAdditive);                             // 0x10015070
extern LTVector		g_vGlobalLightColor;        // 0x1005b780
extern uint8		g_VertexAlpha;              // 0x1005d20c
extern void			d3d_DrawTranslucentWorldModel(ViewParams *pContext, LTObject *pObject); // 0x10030e50


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

	if (pObject->m_Flags2 & PHFLAG2_PORTALOBJECT)
	{
		d3d_ProcessPortalObject(pObject);
		return;
	}

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


// 0x10030220 (822 bytes, the world model's polygon draw) and 0x10030560
// (716 bytes, its animated lightmaps) are not written yet.


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


// 0x10030e50 (d3d_DrawTranslucentWorldModel, 764 bytes) and 0x10031150
// (1888 bytes) are not written yet.


// ---------------------------------------------------------------------- //
//  0x100318b0 -- the unsorted translucent world models' draw.
//
//  RETAIL BUG: the blend states are StateSets scoped to the if/else arms,
//  so they are restored before the draw: the unsorted world models are
//  drawn with the ordinary translucent blend, not additively.
// ---------------------------------------------------------------------- //
static void d3d_DrawNoSortWorldModel(ViewParams *pContext, LTObject *pObject)
{
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
}
