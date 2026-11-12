// *********************************************************************** //
//  ph/tagnodes.cpp -- Primal Hunt's d3d.ren, TU42
//  (0x100395e0..0x1003b3d6), /O2.
//
//  The visible set: one global VisibleSet (0x10099240) holding the two
//  world-polygon queues (opaque, and the sky portals d3d_drawsky.cpp reads), the animated-lightmap
//  table, and the fifteen per-type object sets (VS_MODELS ..
//  VS_CANVASES_TRANSLUCENT).  Then the per-frame walk that fills it:
//  the visitor hooks the world tree calls (object, object list, leaf,
//  node), the whole-BSP fallback, attachments, and the opaque polygon
//  draw.  The queues' CMoArray members are instantiated here first
//  (0x1003abc0..0x1003b3c0).
//
//  Still near: VisibleSet's constructor (0x10039910); CMoArray's Init
//  (0x1003b040) is not emitted here at all.  Retail expands each queue's
//  CMoArray constructor one level, calling Clear (0x1002a040) and
//  Init(0, 0) out of line; cl 8966
//  /Ob2 here calls the whole constructor (with Clear and Init expanded in
//  its own COMDAT).  Neither an explicit PolyQueue constructor, a cache-
//  size constructor, nor writing ClearSet out moves it; it looks like the
//  constructor budget of guide 09 (Model::Model), one unit short.  The
//  global's initialiser 0x100395e0 follows the constructor.
//
//  The walk: d3d_QueueAllObjects has external linkage, which is what keeps
//  /Ob2 from folding it into d3d_TagVisibleLeaves (a static one is
//  expanded there, and then d3d_QueuePoly is not).
//
//  The AvP2 1.0.9.6 counterparts (icl) are in d3dqip4.cpp, w49_1007a700.cpp,
//  d3dr5a_o2.cpp, d3dr4a_o2.cpp, d3dbatchc_o2.cpp and w57_1007bb20_qipni.cpp.
// *********************************************************************** //

#include "ltbasedefs.h"
#include "ph_decls.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"


// The profiling counters (TU14: 0x10013fb7, 0x10013fd5, 0x10013fe0).
#define CSTART_NONE			0

class CounterFinal
{
public:
					CounterFinal(unsigned long startMode=CSTART_NONE);
	unsigned long	m_Data[2];
};

typedef CounterFinal Counter;

void			cnt_StartCounterFinal(CounterFinal &cCounter);
unsigned long	cnt_EndCounterFinal(CounterFinal &cCounter);

inline void cnt_StartCounter(Counter &cCounter) { cnt_StartCounterFinal(cCounter); }
inline unsigned long cnt_EndCounter(Counter &cCounter) { return cnt_EndCounterFinal(cCounter); }

class CountAdder
{
	public:
		CountAdder(uint32 *pNum)
		{
			m_pNum = pNum;
			cnt_StartCounter(m_Counter);
		}
		~CountAdder()
		{
			*m_pNum += cnt_EndCounter(m_Counter);
		}
		Counter m_Counter;
		uint32 *m_pNum;
};


// ---------------------------------------------------------------------- //
//  The engine's structures, as this file reads them.
// ---------------------------------------------------------------------- //
struct Attachment
{
	uint8		m_pad_00[0x24];
	Attachment*	m_pNext;                                // +0x24
};

struct LTObject
{
	uint8		m_pad_000[0x58];
	uint32		m_WTFrameCode;                          // +0x058
	uint8		m_pad_05c[0x88 - 0x5c];
	uint32		m_Flags;                                // +0x088
	uint32		m_Flags2;                               // +0x08c
	uint8		m_pad_090[0x98 - 0x90];
	Attachment*	m_Attachments;                          // +0x098
	uint8		m_pad_09c[0xbc - 0x9c];
	char		m_ObjectType;                           // +0x0bc
	uint8		m_pad_0bd[0x124 - 0xbd];
	uint32		m_InternalFlags;                        // +0x124
	uint8		m_pad_128[0x190 - 0x128];
	uint32		m_bInsideBsp;                           // +0x190
};

#define IFLAG_INSKY				0x400

// The object type whose objects live in the leaves (drawn by the leaf
// visitor, never through the object visitor).
#define OT_LEAFOBJECT			7

struct ViewParams
{
	uint8		m_pad_000[0x21c];
	LTMatrix	m_mWorld;                               // +0x21c
	uint8		m_pad_25c[0x3d0 - 0x25c];
	LTPlane		m_ClipPlanes[6];                        // +0x3d0
	uint8		m_pad_430[0x4b4 - 0x430];
	LTVector	m_Pos;                                  // +0x4b4
	uint8		m_pad_4c0[0x4d4 - 0x4c0];
	uint32		m_bDrawHidden;                          // +0x4d4
	LTVector	m_PortalPos;                            // +0x4d8
	uint8		m_pad_4e4[0x4f8 - 0x4e4];
	uint32		m_CurObjectFrameCode;                   // +0x4f8
};

typedef void (*DrawObjectFn)(ViewParams* pContext, LTObject* pObject);

// The renderer's view of the engine's RenderStruct.
struct RenderStruct
{
	LTObject*	(*ProcessAttachment)(LTObject *pObject, Attachment *pAttachment);  // +0x00
	uint8		m_pad_04[0x18 - 0x04];
	void*	(*GetParameter)(const char *pName);                                 // +0x18
	float		(*GetParameterValueFloat)(void* hParam);                         // +0x1c
	uint8		m_pad_20[0x58 - 0x20];
	uint32		m_VisTicks;                                                         // +0x58
};

#define NUM_OBJECTTYPES		11

struct ObjectHandler
{
	void		(*m_ModuleInit)();
	void		(*m_ModuleTerm)();
	uint32		m_bModuleInitted;
	void		(*m_PreFrameFn)();
	void		(*m_ProcessObjectFn)(LTObject *pObject);
};

extern ObjectHandler g_ObjectHandlers[NUM_OBJECTTYPES]; // 0x1004da88


// The world's polygons, as the queues and the polygon draw read them.
struct WorldSurface
{
	uint8		m_pad_00[0x2c];
	void*		m_pTexture;                             // +0x2c
	uint32		m_Flags;                                // +0x30
	uint8		m_pad_34[0x3a - 0x34];
	uint16		m_iLMAnim;                              // +0x3a
};

#define PHSURF_INVISIBLE		0x4
#define PHSURF_SKYPORTAL		0x10
#define PHSURF_SKY				0x80
#define PHSURF_FLASH				0x8000
#define LMANIM_NONE				0x7fff

struct PolyVert
{
	uint8		m_pad_00[0x14];
	uint32		m_DynLight;                             // +0x14
};

struct WorldPoly
{
	uint8		m_pad_00[0x0c];
	uint32*		m_pLightIndices;                        // +0x0c
	uint32		m_nLightIndices;                        // +0x10
	uint16		m_Flags;                                // +0x14
	uint8		m_pad_16[0x18 - 0x16];
	LTVector	m_vCenter;                              // +0x18
	float		m_fRadius;                              // +0x24
	LTPlane*	m_pPlane;                               // +0x28
	WorldSurface*	m_pSurface;                         // +0x2c
	uint8		m_pad_30[0x46 - 0x30];
	uint16		m_FrameCode;                            // +0x46
	uint8		m_pad_48[0x50 - 0x48];
	PolyVert*	m_pFixedVerts;                          // +0x50
	uint16		m_nVerts;                               // +0x54
	uint16		m_nFixedVerts;                          // +0x56
	PolyVert	m_Verts[1];                             // +0x58
};

#define WPF_DYNAMICLIGHT		0x4000

struct WorldLight
{
	uint8		m_pad_00[0x34];
	int32		m_iFrame;                               // +0x34
	uint8		m_pad_38[0x40 - 0x38];
	float		m_fBrightness;                          // +0x40
	uint8		m_pad_44[0x60 - 0x44];
};

struct LeafObjRef
{
	uint8		m_pad_00[0x18];
	LTObject*	m_pObject;                              // +0x18
};

struct WorldLeaf
{
	uint8		m_pad_00[0x14];
	LTLink		m_Objects;                              // +0x14
	WorldPoly**	m_pPolies;                              // +0x20
	uint32		m_nPolies;                              // +0x24
	uint8		m_pad_28[0x2c - 0x28];
	uint16		m_FrameCode;                            // +0x2c
};

struct BspLeaf
{
	uint8		m_pad_00[0x0c];
	LTLink		m_Objects;                              // +0x0c
};

struct VisNode
{
	uint8		m_pad_00[0x30];
	LTVector	m_vCenter;                              // +0x30
	uint8		m_pad_3c[0x40 - 0x3c];
	float		m_fRadius;                              // +0x40
};

struct WorldBsp
{
	uint8		m_pad_00[0x70];
	BspLeaf*	m_Leafs;                                // +0x70
	uint32		m_nLeafs;                               // +0x74
	uint8		m_pad_78[0xa0 - 0x78];
	WorldPoly**	m_Polies;                               // +0xa0
	uint32		m_nPolies;                              // +0xa4
};

typedef void (*VisFn)();

// What the world tree's walk calls back.
struct FindObjInfo
{
					FindObjInfo()
					{
						m_Pos.Init();
						m_MaxDist = 0.0f;
						m_ObjectCB = (VisFn)DefaultObjectCB;
						m_ObjListCB = LTNULL;
						m_pUser = LTNULL;
						m_LeafCB = (VisFn)DefaultLeafCB;
						m_Flags = 0;
						m_NodeFilterCB = (VisFn)ReturnTrue;
						m_PortalFilterCB = LTNULL;
					}

	// Two do-nothing defaults and a yes, which the link folds into
	// lmpaging.cpp's identical LMPager_End and LMPager_True (0x1003530c);
	// lmpaging.cpp defines them, /O1 as they are.
	static void		DefaultObjectCB(LTObject *pObject);
	static void		DefaultLeafCB(WorldLeaf *pLeaf);
	static int		ReturnTrue();                       // 0x1003530d

	uint32		m_Flags;                                // +0x00
	LTVector	m_Pos;                                  // +0x04
	float		m_MaxDist;                              // +0x10
	VisFn		m_ObjectCB;                             // +0x14
	VisFn		m_ObjListCB;                            // +0x18
	void*		m_pUser;                                // +0x1c
	VisFn		m_LeafCB;                               // +0x20
	VisFn		m_NodeFilterCB;                         // +0x24
	VisFn		m_PortalFilterCB;                       // +0x28
	uint8		m_pad_2c[0x48 - 0x2c];                  // the walk's own state
};

class WorldTree
{
public:
	virtual void	Slot00();
	virtual void	Slot04();
	virtual void	Slot08();
	virtual void	Slot0c();
	virtual void	Slot10();
	virtual void	FindObjectsInFrustum(FindObjInfo *pInfo);  // +0x14
};

struct SceneWorld
{
public:
	virtual void		Slot00();
	virtual WorldBsp*	GetVisBSP();                    // +0x04

	uint8		m_pad_04[0x08 - 0x04];
	WorldLight*	m_Lights;                               // +0x08
	uint32		m_nLights;                              // +0x0c
	uint8		m_pad_10[0x6c - 0x10];
	WorldTree	m_WorldTree;                            // +0x6c
	uint8		m_pad_70[0x180 - 0x70];
	uint32		m_Flags;                                // +0x180
};

#define WORLD_HASPVS			2

extern RenderStruct*	g_pStruct;                      // 0x1005df08
extern ViewParams		g_ViewParams;                 // 0x1005b790
extern SceneWorld*		g_pSceneWorld;                  // 0x1005c208
extern uint16			g_CurFrameCode;                 // 0x1005d238
extern uint32			g_ClipFlags;                    // 0x1005bd0c
extern uint32			g_nVisibleLeaves;               // 0x1005b78c
extern uint32			g_nWorldPoliesDrawn;            // 0x1005c120
extern uint32			g_bAnimateLightmaps;            // 0x100628b0
extern int32			g_CV_DrawFlat;                  // 0x1005df74
extern int32			g_CV_FixTJunc;                  // 0x1005dbb4
extern int32			g_CV_LockPVS;                   // 0x1005df3c
extern int32			g_CV_DrawAll;                   // 0x1005df50
extern int32			g_CV_DrawWorld;                 // 0x1004a730

// The polygon draw functions the surface selects.
// The world-poly drawers d3d_draw.cpp and drawlightmaps.cpp set up.
struct RenderPoly;
extern void (*g_pfnDrawGouraudPoly)(RenderPoly *pPoly);     // 0x1005e280 (d3d_draw.cpp)
extern void (*g_pfnDrawLightmapPoly)(RenderPoly *pPoly);    // 0x1005fd9c (d3d_draw.cpp)
extern void (*g_pfnDrawTexturedPoly)(RenderPoly *pPoly);    // 0x1005e6bc (d3d_draw.cpp)
extern void (*g_pfnDrawDeferredPoly)(RenderPoly *pPoly);    // 0x1005e770 (d3d_draw.cpp)

extern void		dd_ConsolePrint(const char* pMsg, ...); // 0x10013ade
extern void		d3d_DrawSky();                   // 0x1002e280
extern void		d3d_ProcessLights();                    // 0x10024990
struct LitVertex;
struct LightAnim;
struct LightAnimRef;
extern LTBOOL	d3d_AddLightAnim(LitVertex *pVerts, uint32 nVerts,
					LightAnim *pAnim, LightAnimRef *pRef);  // 0x10019040 (d3d_draw.cpp)
extern int		d3d_NodeFilter(VisNode *pNode);         // 0x100101f8


// ---------------------------------------------------------------------- //
//  The object sets.
// ---------------------------------------------------------------------- //
struct VisibleSet;

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

	LTBOOL		Init(VisibleSet *pVisibleSet, const char *pSetName);

	void		ClearSet()
	{
		m_nObjects = 0;
	}

	void		Draw(ViewParams *pContext, DrawObjectFn fn);

public:

	LTObject	**m_pObjects;                           // +0x00
	uint32		m_nObjects;                             // +0x04

	LTLink		m_Link;                                 // +0x08
	uint32		m_nMaxObjects;                          // +0x14
	const char	*m_pSetName;                            // +0x18
};


class AllocSet : public BaseObjectSet
{
public:

				AllocSet();
				~AllocSet()
				{
					Term();
				}

	LTBOOL		Init(VisibleSet *pVisibleSet, const char *pSetName, uint32 defaultMax);
	void		Term();

	LTObject	**m_pArray;                             // +0x1c
};


// A world-polygon queue: the array only grows, m_nUsed counts this frame's.
struct PolyQueue
{
	void		Add(WorldPoly *pPoly)
	{
		if(m_nUsed >= m_Polies.GetSize())
		{
			if(m_Polies.Insert2(m_Polies.GetSize(), pPoly, &g_DefAlloc))
				m_nUsed++;
		}
		else
		{
			m_Polies[m_nUsed] = pPoly;
			m_nUsed++;
		}
	}

	CMoArray<WorldPoly*>	m_Polies;                   // +0x00
	uint32					m_nUsed;                    // +0x14
};

struct LMAnimEntry
{
	WorldPoly*	m_pPoly;                                // +0x00
	LTMatrix*	m_pTransform;                           // +0x04
};

#define MAX_LMANIM_POLIES	32

struct VisibleSet
{
							VisibleSet();

	LTBOOL					Init();
	void					Term();

	void					ClearSet();

	void					AddLMAnimPoly(WorldPoly *pPoly, LTMatrix *pTransform)
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

	// The list of sets.
	LTLink					m_Sets;                     // +0x000
	uint32					m_Flags;                    // +0x00c

	PolyQueue				m_SolidPolies;              // +0x010
	PolyQueue				m_SkyPortalPolies;        // +0x028

	LMAnimEntry				m_LMAnimPolies[MAX_LMANIM_POLIES];  // +0x040
	uint32					m_nLMAnimPolies;            // +0x140

	AllocSet				m_SolidModels;              // +0x144
	AllocSet				m_TranslucentModels;        // +0x164
	AllocSet				m_ChromakeyModels;          // +0x184

	AllocSet				m_TranslucentSprites;       // +0x1a4
	AllocSet				m_NoZSprites;               // +0x1c4

	AllocSet				m_SolidWorldModels;         // +0x1e4
	AllocSet				m_TranslucentWorldModels;   // +0x204
	AllocSet				m_ChromakeyWorldModels;     // +0x224

	AllocSet				m_Lights;                   // +0x244

	AllocSet				m_SolidPolyGrids;           // +0x264
	AllocSet				m_TranslucentPolyGrids;     // +0x284

	AllocSet				m_LineSystems;              // +0x2a4
	AllocSet				m_ParticleSystems;          // +0x2c4

	AllocSet				m_SolidCanvases;            // +0x2e4
	AllocSet				m_TranslucentCanvases;      // +0x304
};

#define VSFLAG_REBUILD		1

#define MAX_VISIBLE_MODELS			128
#define MAX_VISIBLE_SPRITES			128
#define MAX_VISIBLE_WORLDMODELS		64
#define MAX_VISIBLE_LIGHTS			40
#define MAX_VISIBLE_POLYGRIDS		32
#define MAX_VISIBLE_LINESYSTEMS		32
#define MAX_VISIBLE_PARTICLESYSTEMS	64
#define MAX_VISIBLE_CANVASES		32


VisibleSet g_VisibleSet;                                // 0x10099240  $E 0x100395e0, 0x10039600


// Queue a world polygon once per frame.
static inline void d3d_QueuePoly(WorldPoly *pPoly)
{
	WorldSurface *pSurface;

	if(pPoly->m_FrameCode == g_CurFrameCode)
		return;

	pSurface = pPoly->m_pSurface;
	if(!(pSurface->m_Flags & PHSURF_INVISIBLE))
	{
		if((pSurface->m_Flags & PHSURF_SKYPORTAL) && !g_CV_DrawFlat)
		{
			g_VisibleSet.m_SkyPortalPolies.Add(pPoly);
		}
		else if(pSurface->m_iLMAnim == LMANIM_NONE)
		{
			g_VisibleSet.m_SolidPolies.Add(pPoly);
		}
		else if(g_bAnimateLightmaps)
		{
			g_VisibleSet.AddLMAnimPoly(pPoly, &g_ViewParams.m_mWorld);
		}
		else
		{
			g_VisibleSet.m_SolidPolies.Add(pPoly);
		}
	}

	pPoly->m_FrameCode = g_CurFrameCode;
}



// ------------------------------------------------------------------------------------ //
// BaseObjectSet.
// ------------------------------------------------------------------------------------ //

// 0x100397a0
LTBOOL BaseObjectSet::Init(VisibleSet *pVisibleSet, const char *pSetName)
{
	if(pVisibleSet)
	{
		dl_Insert(pVisibleSet->m_Sets.m_pPrev, &m_Link);
	}

	m_pSetName = pSetName;
	return LTTRUE;
}


// 0x100397e0
void BaseObjectSet::Draw(ViewParams *pContext, DrawObjectFn fn)
{
	uint32 i;
	LTObject *pObj;

	for(i=0; i < m_nObjects; i++)
	{
		pObj = m_pObjects[i];

		if(pObj->m_Flags & FLAG_VISIBLE)
		{
			if(pContext->m_bDrawHidden && (pObj->m_Flags2 & 1))
				continue;
		}
		else
		{
			if(!pContext->m_bDrawHidden || !(pObj->m_Flags & 0x400))
				continue;
		}

		fn(pContext, pObj);
	}
}


// ------------------------------------------------------------------------------------ //
// AllocSet
// ------------------------------------------------------------------------------------ //

// 0x10039850
LTBOOL AllocSet::Init(VisibleSet *pVisibleSet, const char *pSetName, uint32 defaultMax)
{
	void* hParam;
	uint32 maxNum;

	Term();

	if(!BaseObjectSet::Init(pVisibleSet, pSetName))
		return LTFALSE;

	hParam = g_pStruct->GetParameter(pSetName);
	if(hParam)
	{
		maxNum = (uint32)g_pStruct->GetParameterValueFloat(hParam);
	}
	else
	{
		maxNum = defaultMax;
	}

	m_pObjects = new LTObject*[maxNum];
	if(!m_pObjects)
		return LTFALSE;

	m_nMaxObjects = maxNum;
	return LTTRUE;
}


// 0x100398f0
void AllocSet::Term()
{
	if(m_pObjects)
	{
		delete [] m_pObjects;
		m_pObjects = LTNULL;
	}

	m_nMaxObjects = 0;
	m_nObjects = 0;
}


// ------------------------------------------------------------------------------------ //
// VisibleSet.
// ------------------------------------------------------------------------------------ //

// 0x10039910
VisibleSet::VisibleSet()
{
	dl_TieOff(&m_Sets);
	m_Flags = VSFLAG_REBUILD;
	m_SolidPolies.m_Polies.SetCacheSize(512);
	m_SkyPortalPolies.m_Polies.SetCacheSize(32);
	ClearSet();
}


// 0x10039a80
AllocSet::AllocSet()
{
}


// 0x10039aa0
LTBOOL VisibleSet::Init()
{
	LTBOOL bErr;

	bErr = LTFALSE;

	bErr |= !m_SolidModels.Init(this, "VS_MODELS", MAX_VISIBLE_MODELS);
	bErr |= !m_TranslucentModels.Init(this, "VS_MODELS_TRANSLUCENT", MAX_VISIBLE_MODELS);
	bErr |= !m_ChromakeyModels.Init(this, "VS_MODELS_CHROMAKEY", MAX_VISIBLE_MODELS);

	bErr |= !m_TranslucentSprites.Init(this, "VS_SPRITES", MAX_VISIBLE_SPRITES);
	bErr |= !m_NoZSprites.Init(this, "VS_SPRITES_NOZ", MAX_VISIBLE_SPRITES);

	bErr |= !m_SolidWorldModels.Init(this, "VS_WORLDMODELS", MAX_VISIBLE_WORLDMODELS);
	bErr |= !m_TranslucentWorldModels.Init(this, "VS_WORLDMODELS_TRANSLUCENT", MAX_VISIBLE_WORLDMODELS);
	bErr |= !m_ChromakeyWorldModels.Init(this, "VS_WORLDMODELS_CHROMAKEY", MAX_VISIBLE_WORLDMODELS);

	bErr |= !m_Lights.Init(this, "VS_LIGHTS", MAX_VISIBLE_LIGHTS);

	bErr |= !m_SolidPolyGrids.Init(this, "VS_POLYGRIDS", MAX_VISIBLE_POLYGRIDS);
	bErr |= !m_TranslucentPolyGrids.Init(this, "VS_POLYGRIDS_TRANSLUCENT", MAX_VISIBLE_POLYGRIDS);

	bErr |= !m_LineSystems.Init(this, "VS_LINESYSTEMS", MAX_VISIBLE_LINESYSTEMS);
	bErr |= !m_ParticleSystems.Init(this, "VS_PARTICLESYSTEMS", MAX_VISIBLE_PARTICLESYSTEMS);

	bErr |= !m_SolidCanvases.Init(this, "VS_CANVASES", MAX_VISIBLE_CANVASES);
	bErr |= !m_TranslucentCanvases.Init(this, "VS_CANVASES_TRANSLUCENT", MAX_VISIBLE_CANVASES);

	if(bErr)
	{
		Term();
	}

	return !bErr;
}


// 0x10039e80
void VisibleSet::ClearSet()
{
	LTLink *pCur;
	BaseObjectSet *pSet;

	m_SolidPolies.m_nUsed = 0;
	m_SkyPortalPolies.m_nUsed = 0;
	m_nLMAnimPolies = 0;

	for(pCur=m_Sets.m_pNext; pCur != &m_Sets; pCur=pCur->m_pNext)
	{
		pSet = (BaseObjectSet*)pCur->m_pData;
		pSet->ClearSet();
	}
}


// Init's failure path inlines it; it has no copy of its own in retail.
inline void VisibleSet::Term()
{
	LTLink *pCur, *pNext;

	for(pCur=m_Sets.m_pNext; pCur != &m_Sets; pCur=pNext)
	{
		pNext = pCur->m_pNext;
	}

	dl_TieOff(&m_Sets);
}


// ------------------------------------------------------------------------------------ //
// The walk.
// ------------------------------------------------------------------------------------ //

static inline LTBOOL d3d_ShouldProcessObject(LTObject *pObject)
{
	return g_ObjectHandlers[pObject->m_ObjectType].m_ProcessObjectFn &&
		pObject->m_ObjectType != OT_LEAFOBJECT &&
		!(pObject->m_Flags2 & FLAG2_SKYOBJECT) &&
		(((pObject->m_Flags & FLAG_VISIBLE) &&
			(!g_ViewParams.m_bDrawHidden || !(pObject->m_Flags2 & 1))) ||
		 (!(pObject->m_Flags & FLAG_VISIBLE) && g_ViewParams.m_bDrawHidden &&
			(pObject->m_Flags & 0x400)));
}



static void d3d_ProcessAttachments(LTObject *pObject, uint32 depth);

// 0x10039eb0 -- no PVS: every object in every leaf, then every polygon.
void d3d_QueueAllObjects(WorldBsp *pBsp)
{
	uint32 i;
	LTLink *pListHead, *pCur;
	LTObject *pObject;

	for(i=0; i < pBsp->m_nLeafs; i++)
	{
		pListHead = &pBsp->m_Leafs[i].m_Objects;
		for(pCur=pListHead->m_pPrev; pCur != pListHead; pCur=pCur->m_pPrev)
		{
			pObject = (LTObject*)pCur->m_pData;
			if(pObject->m_bInsideBsp)
				break;

			if(!d3d_ShouldProcessObject(pObject))
				continue;

			pObject->m_WTFrameCode = g_ViewParams.m_CurObjectFrameCode;
			g_ObjectHandlers[pObject->m_ObjectType].m_ProcessObjectFn(pObject);

			if(pObject->m_Attachments)
				d3d_ProcessAttachments(pObject, 0);
		}
	}

	for(i=0; i < pBsp->m_nPolies; i++)
	{
		d3d_QueuePoly(pBsp->m_Polies[i]);
	}
}


// 0x1003a1f0
static void d3d_ProcessAttachments(LTObject *pObject, uint32 depth)
{
	Attachment *pCur;
	LTObject *pAttachedObject;

	// Don't recurse too far.
	if(depth > 32)
		return;

	for(pCur=pObject->m_Attachments; pCur; pCur=pCur->m_pNext)
	{
		pAttachedObject = g_pStruct->ProcessAttachment(pObject, pCur);

		if(pAttachedObject && (pAttachedObject->m_WTFrameCode != g_ViewParams.m_CurObjectFrameCode))
		{
			// Allow it to recursively process attachments.
			d3d_ProcessAttachments(pAttachedObject, depth+1);

			if(d3d_ShouldProcessObject(pAttachedObject))
			{
				g_ObjectHandlers[pAttachedObject->m_ObjectType].m_ProcessObjectFn(pAttachedObject);
				pAttachedObject->m_WTFrameCode = g_ViewParams.m_CurObjectFrameCode;
			}
		}
	}
}


static void d3d_TagVisibleLeaves(LTBOOL bAll);

// 0x1003a2c0
void d3d_QueueVisibleObjects()
{
	if((g_pSceneWorld->m_Flags & WORLD_HASPVS) && !g_CV_DrawAll)
		d3d_TagVisibleLeaves(LTFALSE);
	else
		d3d_TagVisibleLeaves(LTTRUE);
}


static void d3d_ObjectCB(LTObject *pObject);
static void d3d_ObjListCB(LTLink *pListHead, LTObject ***pppObjects, uint32 *pnObjects);
static void d3d_LeafCB(WorldLeaf *pLeaf);
static int d3d_PortalFilterCB(VisNode *pNode);


// 0x1003a2f0 -- rebuild the visible set (unless LockPVS), then draw the
// opaque polygons.
static void d3d_TagVisibleLeaves(LTBOOL bAll)
{
	FindObjInfo info;
	WorldBsp *pBsp;
	WorldPoly **ppPoly, *pPoly;
	WorldSurface *pSurface;
	uint32 nPolies, i, nVerts, flags;
	PolyVert *pVerts;
	float fDist;
	WorldLight *pLight;
	uint32 iLight;
	SceneWorld *pWorld;
	uint32 iPlane;
	LTPlane *pPlane;

	if((g_VisibleSet.m_Flags & VSFLAG_REBUILD) && !g_CV_LockPVS)
	{
		// Rebuild the visible set.
		g_VisibleSet.ClearSet();

		if(bAll)
		{
			pBsp = g_pSceneWorld->GetVisBSP();
			if(pBsp)
				d3d_QueueAllObjects(pBsp);
			else
				dd_ConsolePrint("ERROR: GetVisBSP returned LTNULL!");
		}
		else
		{
			CountAdder cntAdd(&g_pStruct->m_VisTicks);

			info.m_Flags = 0;
			info.m_Pos = g_ViewParams.m_bDrawHidden ? g_ViewParams.m_PortalPos : g_ViewParams.m_Pos;

			info.m_MaxDist = 10000.0f;
			info.m_ObjectCB = (VisFn)d3d_ObjectCB;
			info.m_ObjListCB = (VisFn)d3d_ObjListCB;
			info.m_pUser = LTNULL;
			info.m_LeafCB = (VisFn)d3d_LeafCB;
			info.m_NodeFilterCB = (VisFn)d3d_NodeFilter;
			info.m_PortalFilterCB = (VisFn)d3d_PortalFilterCB;

			g_pSceneWorld->m_WorldTree.FindObjectsInFrustum(&info);
		}
	}

	d3d_DrawSky();
	d3d_ProcessLights();

	if(!g_CV_DrawWorld)
		return;

	ppPoly = g_VisibleSet.m_SolidPolies.m_Polies.GetArray();
	for(nPolies=g_VisibleSet.m_SolidPolies.m_nUsed; nPolies; nPolies--, ppPoly++)
	{
		pPoly = *ppPoly;

		if(pPoly->m_pPlane->DistTo(g_ViewParams.m_Pos) <= 0.01f)
			continue;

		g_ClipFlags = 0x3f;
		pPlane = g_ViewParams.m_ClipPlanes;
		for(iPlane=0; iPlane < 6; pPlane++, iPlane++)
		{
			fDist = pPlane->DistTo(pPoly->m_vCenter);
			if(fDist < -pPoly->m_fRadius)
				goto NextPoly;

			if(fDist > pPoly->m_fRadius)
				g_ClipFlags &= ~(1 << iPlane);
		}

		g_nWorldPoliesDrawn++;

		pSurface = pPoly->m_pSurface;
		flags = pSurface->m_Flags;
		if(flags & PHSURF_INVISIBLE)
			continue;

		if(pSurface->m_pTexture)
		{
			if(flags & PHSURF_SKY)
			{
				g_pfnDrawTexturedPoly((RenderPoly*)pPoly);
				continue;
			}

			if(pPoly->m_Flags & WPF_DYNAMICLIGHT)
			{
				pWorld = g_pSceneWorld;

				if(g_CV_FixTJunc)
				{
					pVerts = pPoly->m_pFixedVerts;
					nVerts = pPoly->m_nFixedVerts;
				}
				else
				{
					pVerts = pPoly->m_Verts;
					nVerts = pPoly->m_nVerts;
				}

				for(i=0; i < nVerts; i++)
					pVerts[i].m_DynLight = 0;

				for(i=0; i < pPoly->m_nLightIndices; i++)
				{
					iLight = (uint16)pPoly->m_pLightIndices[i];
					if(iLight < pWorld->m_nLights)
					{
						pLight = &pWorld->m_Lights[iLight];
						if(pLight->m_iFrame != -1 && pLight->m_fBrightness >= 0.02f)
							d3d_AddLightAnim((LitVertex*)pVerts, nVerts, (LightAnim*)pLight, (LightAnimRef*)&pPoly->m_pLightIndices[i]);
					}
				}

				pPoly->m_Flags &= ~WPF_DYNAMICLIGHT;
			}

			if(flags & PHSURF_FLASH)
				g_pfnDrawLightmapPoly((RenderPoly*)pPoly);
			else
				g_pfnDrawGouraudPoly((RenderPoly*)pPoly);
		}
		else
		{
			g_pfnDrawDeferredPoly((RenderPoly*)pPoly);
		}

NextPoly:;
	}
}


// 0x1003a6b0 -- the walk's object callback.
static void d3d_ObjectCB(LTObject *pObject)
{
	if(pObject->m_InternalFlags & IFLAG_INSKY)
		return;

	if(!d3d_ShouldProcessObject(pObject))
		return;

	g_ObjectHandlers[pObject->m_ObjectType].m_ProcessObjectFn(pObject);

	if(pObject->m_Attachments)
		d3d_ProcessAttachments(pObject, 0);
}


// 0x1003a7f0 -- the walk's object-list callback: collect the drawable
// objects of a list, from the tail, up to the first one inside the BSP.
static void d3d_ObjListCB(LTLink *pListHead, LTObject ***pppObjects, uint32 *pnObjects)
{
	LTLink *pCur;
	LTObject *pObject;

	for(pCur=pListHead->m_pPrev; pCur != pListHead; pCur=pCur->m_pPrev)
	{
		pObject = (LTObject*)pCur->m_pData;
		if(pObject->m_bInsideBsp)
			return;

		if(d3d_ShouldProcessObject(pObject))
		{
			(*pppObjects)[*pnObjects] = pObject;
			(*pnObjects)++;
		}
	}
}


// 0x1003a890 -- the walk's leaf callback.
static void d3d_LeafCB(WorldLeaf *pLeaf)
{
	LTLink *pCur;
	LTObject *pObject;
	WorldPoly **ppPoly, **ppEnd;

	pLeaf->m_FrameCode = g_CurFrameCode;
	g_nVisibleLeaves++;

	for(pCur=pLeaf->m_Objects.m_pNext; pCur != &pLeaf->m_Objects; pCur=pCur->m_pNext)
	{
		pObject = ((LeafObjRef*)pCur->m_pData)->m_pObject;

		if(pObject->m_ObjectType == OT_LEAFOBJECT && (pObject->m_Flags & FLAG_VISIBLE) &&
			pObject->m_WTFrameCode != g_ViewParams.m_CurObjectFrameCode)
		{
			pObject->m_WTFrameCode = g_ViewParams.m_CurObjectFrameCode;
			g_ObjectHandlers[OT_LEAFOBJECT].m_ProcessObjectFn(pObject);

			if(pObject->m_Attachments)
				d3d_ProcessAttachments(pObject, 0);
		}
	}

	ppPoly = pLeaf->m_pPolies;
	ppEnd = &ppPoly[pLeaf->m_nPolies];
	for(; ppEnd != ppPoly; ppPoly++)
	{
		d3d_QueuePoly(*ppPoly);
	}
}


// 0x1003ab40 -- the walk's portal filter: is the node's sphere inside
// every frustum plane?
static int d3d_PortalFilterCB(VisNode *pNode)
{
	uint32 i;

	for(i=0; i < 6; i++)
	{
		if(g_ViewParams.m_ClipPlanes[i].DistTo(pNode->m_vCenter) < -pNode->m_fRadius)
			return LTFALSE;
	}

	return LTTRUE;
}


// 0x1003abb0
VisibleSet* d3d_GetVisibleSet()
{
	return &g_VisibleSet;
}
