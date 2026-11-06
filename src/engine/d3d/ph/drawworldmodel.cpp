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

struct WorldBsp
{
	uint8		m_pad_00[0xa0];
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
	uint8		m_pad_090[0x1b0 - 0x90];
	WorldBsp*	m_pWorldBsp;                            // +0x1b0
};

#define PHFLAG2_CHROMAKEY		0x2
#define PHFLAG2_ADDITIVE		0x4
#define PHFLAG2_PORTALOBJECT	0x8000

struct SceneContext
{
	uint8		m_pad_000[0x4d4];
	uint32		m_bDrawHidden;                          // +0x4d4
};

typedef void (*DrawObjectFn)(SceneContext* pContext, LTObject* pObject);

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

	void		Draw(SceneContext* pContext, DrawObjectFn fn);  // 0x100397e0

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
extern SceneContext			g_SceneContext;             // 0x1005b790

extern VisibleSet*	d3d_GetVisibleSet();                // 0x1003abb0
extern void			d3d_ProcessPortalObject(LTObject *pObject);         // 0x1000fc52

extern void			d3d_DrawWorldModel(SceneContext *pContext, LTObject *pObject);            // 0x10030860
extern void			d3d_DrawTranslucentWorldModel(SceneContext *pContext, LTObject *pObject); // 0x10030e50


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

	pSet->Draw(&g_SceneContext, d3d_DrawWorldModel);
}


// 0x10030860 (d3d_DrawWorldModel, 1092 bytes) is not written yet.


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

	pSet->Draw(&g_SceneContext, d3d_DrawWorldModel);
}


static void d3d_DrawNoSortWorldModel(SceneContext *pContext, LTObject *pObject);

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
			if (g_SceneContext.m_bDrawHidden && (pObject->m_Flags2 & 1))
				continue;
		}
		else
		{
			if (g_SceneContext.m_bDrawHidden && !(pObject->m_Flags & 0x400))
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
static void d3d_DrawNoSortWorldModel(SceneContext *pContext, LTObject *pObject)
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
