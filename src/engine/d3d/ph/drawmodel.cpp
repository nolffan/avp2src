// *********************************************************************** //
//
//  ph/drawmodel.cpp -- Primal Hunt's d3d.ren, TU07: 0x1000b2f9..0x1000f09f,
//  /O1.
//
//  Models: the model drawer (CD3DModelDraw, one global instance), the
//  model console variables LightModelSprites..ModelUseTnL, and the
//  per-frame model setup.  /O1 without /Ob2: retail calls the empty
//  ModelLight constructor in CD3DModelDraw's array loop, which /Ob2 would
//  inline away.  The three CMoArray members instantiate the template
//  COMDATs 0x1000e008..0x1000f089.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"
#include <math.h>


// A console variable (common_stuff.cpp's registrar, 0x10011f36).
class CD3DConVar
{
public:
	CD3DConVar(const char* pName, float fDefault, int32* pnValue, float* pfValue);

	int32       m_nValue;                                   // +0x00
	float       m_fValue;                                   // +0x04
	float       m_fDefault;                                 // +0x08
	int32*      m_pnValue;                                  // +0x0c
	float*      m_pfValue;                                  // +0x10
	const char* m_pName;                                    // +0x14
	void*       m_hParam;                                   // +0x18
	CD3DConVar* m_pNext;                                    // +0x1c
};




// What the model drawer reads of a model (the engine's Model).
struct Model
{
	uint8       m_pad_00[0xa4];
	float       m_fRadius;                                  // +0xa4
	uint8       m_pad_a8[0x16c - 0xa8];
	uint32      m_nAnims;                                   // +0x16c
};

// A model instance's animation tracker.
struct AnimTracker
{
	uint8       m_pad_00[0x28];
	Model*      m_pModel;                                   // +0x28
	uint16      m_iAnim;                                    // +0x2c
	uint8       m_pad_2e[0x38 - 0x2e];
	uint16      m_iDestAnim;                                // +0x38

	LTBOOL IsValid()
	{
		if (m_pModel && m_iDestAnim < m_pModel->m_nAnims &&
			m_iAnim < m_pModel->m_nAnims)
		{
			return LTTRUE;
		}
		return LTFALSE;
	}
};

// What the model drawer reads of a model instance (the engine's
// ModelInstance, an LTObject).
struct ModelInstance
{
	uint8       m_pad_000[0x88];
	uint32      m_Flags;                                    // +0x88
	uint8       m_pad_08c[0xac - 0x8c];
	LTVector    m_Scale;                                    // +0xac
	uint8       m_pad_0b8[0x194 - 0xb8];
	LTVector    m_Pos;                                      // +0x194
	uint8       m_pad_1a0[0x1d4 - 0x1a0];
	AnimTracker m_AnimTracker;                              // +0x1d4

	Model*      GetModel() { return m_AnimTracker.m_pModel; }
};

// The view (common_draw.cpp's): the view frustum's six planes, and a set
// for the really close view.
struct ViewParams
{
	uint8       m_pad_000[0x3d0];
	LTPlane     m_ClipPlanes[6];                            // +0x3d0
	LTPlane     m_ReallyClosePlanes[6];                     // +0x430
};

extern ViewParams g_ViewParams;                             // 0x1005b790
extern uint32   g_ClipFlags;                                // 0x1005bd0c
extern float    g_CV_ModelSunVarianceClamp;                 // 0x1005d8b4

void d3d_InitModelVBs();                                    // 0x10001230
void d3d_TermModelVBs();                                    // 0x10001340


// What the model drawer reads of the frame's scene (the engine's
// SceneDesc).
struct SceneDesc
{
	uint8       m_pad_000[0xfc];
	ModelHookFn m_ModelHookFn;                              // +0xfc
	void*       m_ModelHookUser;                            // +0x100
};

extern SceneDesc*   g_pSceneDesc;                           // 0x1005c14c

// What the model drawer reads of a model's geometry (the current LOD).
struct ModelLOD
{
	uint8       m_pad_00[0x64];
	uint32      m_nVerts;                                   // +0x64
	uint8       m_pad_68[0x78 - 0x68];
	uint32      m_nTris;                                    // +0x78
};

// A light the model is lit by.
struct ModelLight
{
	ModelLight();                                           // 0x1000b981

	uint8       m_Data[0x28];
};

// One texture pass of the model's render state.
struct ModelPass
{
	ModelPass()
	{
		m_iTexture = -1;
		m_iStage = -1;
		m_pNext = LTNULL;
		Clear();
	}

	void    Clear()                                         // 0x1000b961
	{
		m_pTexture = LTNULL;
		m_nBlendSrc = 0;
		m_nBlendDest = 0;
		m_pEffect = LTNULL;
		m_nAddress = 0;
		m_nFilter = 0;
		m_nFlags = 0;
		m_fScale = 0.0f;
	}

	void*       m_pTexture;                                 // +0x00
	uint16      m_nBlendSrc;                                // +0x04
	uint16      m_nBlendDest;                               // +0x06
	void*       m_pEffect;                                  // +0x08
	int32       m_iTexture;                                 // +0x0c
	uint16      m_nAddress;                                 // +0x10
	uint16      m_nFilter;                                  // +0x12
	uint32      m_nFlags;                                   // +0x14
	int32       m_iStage;                                   // +0x18
	float       m_fScale;                                   // +0x1c
	ModelPass*  m_pNext;                                    // +0x20
};

// The model's render state: its texture passes and texture transform.
struct ModelRenderState
{
	ModelRenderState()                                      // 0x1000b841
	{
		m_nPasses = 0;
		m_iCurPass = 0;
		m_mTexture.Identity();
		m_pEnvMap = LTNULL;
		m_bEnabled = LTTRUE;
	}

	ModelPass   m_Passes[8];                                // +0x000
	LTBOOL      m_bEnabled;                                 // +0x120
	uint32      m_nPasses;                                  // +0x124
	uint32      m_iCurPass;                                 // +0x128
	LTMatrix    m_mTexture;                                 // +0x12c
	void*       m_pEnvMap;                                  // +0x16c
};

// A transformed and lit vertex, and a clipped one.
struct ModelTLVert
{
	uint8       m_Data[0x20];
};

struct ModelClipVert
{
	uint8       m_Data[0x40];
};


class CD3DModelDraw
{
public:
	CD3DModelDraw();                                        // 0x1000b75f
	~CD3DModelDraw();                                       // 0x1000b984

	LTBOOL      AllocVerts();                               // 0x1000b9cc
	void        CallModelHook();                            // 0x1000ba1e
	void        DrawModel(ModelInstance* pInstance);        // 0x1000d2d6

	ModelLOD*   m_pLOD;                                     // +0x000
	ModelInstance* m_pInstance;                             // +0x004
	uint8       m_pad_008[0x38 - 0x08];
	int32       m_iLastLOD;                                 // +0x038
	ModelLight  m_Lights[16];                               // +0x03c
	uint8       m_pad_2bc[0x634 - 0x2bc];
	float       m_fRadius;                                  // +0x634
	uint8       m_pad_638[0x644 - 0x638];
	LTVector    m_vLightDir;                                // +0x644
	LTVector    m_vShadowDir;                               // +0x650
	LTVector    m_vSpecularLight;                           // +0x65c
	uint8       m_pad_668[0x6a4 - 0x668];
	ModelRenderState m_RenderState;                         // +0x6a4
	CMoArray<uint16>        m_Indices;                      // +0x814
	CMoArray<ModelTLVert>   m_TLVerts;                      // +0x828
	CMoArray<ModelClipVert> m_ClipVerts;                    // +0x83c
	LTVector    m_vLightAdd;                                // +0x850
	LTVector    m_vObjectColor;                             // +0x85c
	uint8       m_pad_868[0x890 - 0x868];
	ModelHookData m_HookData;                               // +0x890
	uint8       m_pad_8a4[0x8b4 - 0x8a4];
};


CD3DModelDraw   g_ModelDraw;                                                       // 0x100569b0
LTVector        g_vModelSunDir;
CD3DConVar         g_CV_LightModelSprites("LightModelSprites", 0.0f, LTNULL, LTNULL); // 0x100572c8
CD3DConVar         g_CV_ModelLODOffset("ModelLODOffset", 0.0f, LTNULL, LTNULL);       // 0x10057708
CD3DConVar         g_CV_ModelZoomScale("ModelZoomScale", 1.0f, LTNULL, LTNULL);       // 0x10057868
CD3DConVar         g_CV_ModelLODBlendEnable("ModelLODBlendEnable", 1.0f, LTNULL, LTNULL); // 0x10057848
CD3DConVar         g_CV_ModelLODBlendDist("ModelLODBlendDist", 60.0f, LTNULL, LTNULL); // 0x10057828
CD3DConVar         g_CV_ExtraFOVXOffset("ExtraFOVXOffset", 0.0f, LTNULL, LTNULL);     // 0x10057748
CD3DConVar         g_CV_ExtraFOVYOffset("ExtraFOVYOffset", 0.0f, LTNULL, LTNULL);     // 0x10057270
CD3DConVar         g_CV_ModelFovTest("ModelFovTest", 0.0f, LTNULL, LTNULL);           // 0x100577e8
CD3DConVar         g_CV_ModelSunVariance("ModelSunVariance", 0.05f, LTNULL, LTNULL);  // 0x10058888
CD3DConVar         g_CV_ModelApplySun("ModelApplySun", 1.0f, LTNULL, LTNULL);         // 0x10057808
CD3DConVar         g_CV_ModelSaturation("ModelSaturation", 2.0f, LTNULL, LTNULL);     // 0x10057290
CD3DConVar         g_CV_DrawModelsRigid("DrawModelsRigid", 1.0f, LTNULL, LTNULL);     // 0x10057728
CD3DConVar         g_CV_ModelCacheRigid("ModelCacheRigid", 0.0f, LTNULL, LTNULL);     // 0x100572e8
CD3DConVar         g_CV_ModelUseTnL("ModelUseTnL", 0.0f, LTNULL, LTNULL);             // 0x10056910

static float    g_fModelLights;                             // 0x100572b0
static CheapLTLink g_ModelLights;                              // 0x100572b8
static float    g_fModelSun[32];                            // 0x10057768
static float    g_fModelSunDelta[32];                       // 0x10056930
static float    g_fModelShadows;                            // 0x100588a8
static CheapLTLink g_ModelShadows;                             // 0x100588b0


// NEAR (0x1000b5eb): the loop's two increments come out swapped.
// Clip a bounding sphere against the planes. Clears the bit of each plane
// it is entirely inside in *pClipFlags; FALSE when it is entirely outside
// one of them.
inline LTBOOL d3d_ClipSphere(LTVector* pPos, float fRadius, LTPlane* pPlanes,
                             uint32* pClipFlags)
{
	float fDist;
	int i;

	*pClipFlags = 0x3f;
	for (i = 0; i < 6; i++)
	{
		fDist = pPlanes[i].DistTo(*pPos);
		if (fDist < -fRadius)
			return LTFALSE;

		if (fDist > fRadius)
			*pClipFlags &= ~(1 << i);
	}
	return LTTRUE;
}

inline LTBOOL d3d_SetupModel(CD3DModelDraw* pDraw, ModelInstance* pInstance,
                             uint32* pClipFlags)
{
	pDraw->m_fRadius = pInstance->GetModel()->m_fRadius;
	pDraw->m_fRadius *= LTMAX(pInstance->m_Scale.x,
		LTMAX(pInstance->m_Scale.y, pInstance->m_Scale.z));

	return d3d_ClipSphere(&pInstance->m_Pos, pDraw->m_fRadius,
		(pInstance->m_Flags & 0x40) ? g_ViewParams.m_ReallyClosePlanes : g_ViewParams.m_ClipPlanes,
		pClipFlags) != 0;
}

// 0x1000b4d8
void d3d_DrawModel(void* pParams, ModelInstance* pInstance)
{
	CD3DModelDraw* pDraw;

	if (pInstance->m_AnimTracker.IsValid())
	{
		pDraw = &g_ModelDraw;
		if (d3d_SetupModel(pDraw, pInstance, &g_ClipFlags))
			pDraw->DrawModel(pInstance);
	}
}

// 0x1000b65e
void d3d_InitModelLists()
{
	g_ModelShadows.TieOff();
	g_ModelLights.TieOff();
}

// 0x1000b67d -- the sun's brightness through a model's turn. NEAR: the
// x87 stack shape of the sum (the clamp's result is dropped and the
// unclamped variance reused from the stack).
void d3d_InitModelSun()
{
	int i;

	for (i = 0; i < 32; i++)
	{
		float fSin = (float)sin(((float)i / 32.0f) * MATH_CIRCLE);
		float fVariance = g_CV_ModelSunVarianceClamp;
		float fBase = (1.0f - LTCLAMP(fVariance, 0.0f, 1.0f)) * 0.5f;
		g_fModelSun[i] = fVariance + fBase + fBase * fSin;
	}

	for (i = 0; i < 32; i++)
		g_fModelSunDelta[i] = g_fModelSun[(i + 1) & 31] - g_fModelSun[i];
}

// 0x1000b722
void d3d_InitModels()
{
	d3d_InitModelSun();
	g_fModelShadows = 0.0f;
	g_fModelLights = 0.0f;
	g_ModelShadows.TieOff();
	g_ModelLights.TieOff();
	d3d_InitModelVBs();
}

// 0x1000b75a
void d3d_TermModels()
{
	d3d_TermModelVBs();
}


// 0x1000b75f
CD3DModelDraw::CD3DModelDraw()
{
	m_iLastLOD = -1;

	m_HookData.m_hObject = LTNULL;
	m_HookData.m_Flags = 0;
	m_HookData.m_ObjectFlags = 0;
	m_HookData.m_LightAdd = &m_vLightAdd;
	m_HookData.m_ObjectColor = &m_vObjectColor;

	m_vLightDir.Init(0.0f, -1.0f, 0.0f);
	m_vShadowDir.Init(-2.0f, -2.0f, -2.0f);
	m_vSpecularLight.Init(2.0f, -2.0f, -1.0f);
	m_vLightDir.Norm();
	m_vShadowDir.Norm();
	m_vSpecularLight.Norm();
}

// 0x1000b981
ModelLight::ModelLight()
{
}

// 0x1000b984
CD3DModelDraw::~CD3DModelDraw()
{
}

// 0x1000b9cc -- room for the LOD's vertices.
LTBOOL CD3DModelDraw::AllocVerts()
{
	if (m_pLOD->m_nVerts > m_TLVerts.GetSize())
	{
		if (!m_TLVerts.SetSize2(m_pLOD->m_nVerts + 32, &g_DefAlloc))
			return LTFALSE;
	}

	if (m_pLOD->m_nTris > m_ClipVerts.GetSize())
	{
		if (!m_ClipVerts.SetSize2(m_pLOD->m_nTris, &g_DefAlloc))
			return LTFALSE;
	}

	return LTTRUE;
}

// 0x1000ba1e -- let the game adjust the model before it's drawn.
void CD3DModelDraw::CallModelHook()
{
	ModelInstance* pInstance = m_pInstance;
	ModelHookData* pData = &m_HookData;

	pData->m_Flags = MHF_USETEXTURE;
	pData->m_ObjectFlags = pInstance->m_Flags;
	pData->m_hObject = (HLOCALOBJ)pInstance;

	if (g_pSceneDesc->m_ModelHookFn)
		g_pSceneDesc->m_ModelHookFn(pData, g_pSceneDesc->m_ModelHookUser);
}


// ---------------------------------------------------------------------- //
//  Ray tests against the world.
// ---------------------------------------------------------------------- //

struct ModelWorldSurface
{
	uint8       m_pad_00[0x30];
	uint8       m_Flags;                                    // +0x30
};

#define SURFFLAG_SKY    0x10

struct ModelWorldPoly
{
	uint8       m_pad_00[0x2c];
	ModelWorldSurface* m_pSurface;                          // +0x2c
};

class ModelWorldBsp
{
public:
	virtual void        Slot00();
	virtual void        Slot04();
	virtual void        Slot08();
	virtual ModelWorldPoly* GetPolyFromHPoly(HPOLY hPoly);  // +0x0c
};

struct ModelWorldModel
{
	uint8       m_pad_00[0x04];
	ModelWorldBsp* m_pBsp;                                  // +0x04
};

struct SceneWorld
{
	ModelWorldPoly* GetPolyFromHPoly(HPOLY hPoly)           // 0x1000be09
	{
		uint32 iWorld;
		ModelWorldModel* pModel;

		iWorld = hPoly >> 16;
		if (iWorld >= m_nWorldModels)
			return LTNULL;

		pModel = m_WorldModels[iWorld];
		if (!pModel)
			return LTNULL;

		return pModel->m_pBsp->GetPolyFromHPoly(hPoly);
	}

	uint8       m_pad_000[0x6c];
	uint8       m_WorldTree[0x18c - 0x6c];                  // +0x06c
	ModelWorldModel** m_WorldModels;                        // +0x18c
	uint32      m_nWorldModels;                             // +0x190
};

extern SceneWorld* g_pSceneWorld;                      // 0x1005c208

LTBOOL  i_IntersectSegment(IntersectQuery* pQuery, IntersectInfo* pInfo,
                           void* pWorldTree, void* pUser);  // 0x10031a5d

// 0x1000bcff -- does the segment from pPos along pDir end on the sky?
LTBOOL d3d_SegmentHitsSky(LTVector* pPos, LTVector* pDir)
{
	IntersectQuery iQuery;
	IntersectInfo iInfo;
	ModelWorldPoly* pPoly;

	iQuery.m_From = *pPos;
	iQuery.m_To = *pPos + *pDir;
	iQuery.m_Flags = INTERSECT_HPOLY;

	if (i_IntersectSegment(&iQuery, &iInfo, g_pSceneWorld->m_WorldTree, LTNULL))
	{
		pPoly = g_pSceneWorld->GetPolyFromHPoly(iInfo.m_hPoly);
		if (pPoly)
			return (pPoly->m_pSurface->m_Flags & SURFFLAG_SKY) ? LTTRUE : LTFALSE;
	}

	return LTFALSE;
}
