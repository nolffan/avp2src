// *********************************************************************** //
//
//  ph/drawmodel.cpp -- Primal Hunt's d3d.ren, TU07: 0x1000b2f9..0x1000f09f,
//  /O1.
//
//  Models: the model drawer (CD3DModelDraw, one global instance), the
//  model console variables LightModelSprites..ModelUseTnL, and the
//  per-frame model setup.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>


// A console variable (common_stuff.cpp's registrar, 0x10011f36).
struct CConVar
{
	CConVar(const char* pszName, float fDefault, int32* pnValue,
	        float* pfValue);                                // 0x10011f36

	int32       m_nValue;                                   // +0x00
	float       m_fValue;                                   // +0x04
	float       m_fDefault;                                 // +0x08
	int32*      m_pnValue;                                  // +0x0c
	float*      m_pfValue;                                  // +0x10
	const char* m_pszName;                                  // +0x14
	void*       m_hVar;                                     // +0x18
	CConVar*    m_pNext;                                    // +0x1c
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

struct ViewParams;

// The view frustum's six planes, and a set for the really close view.
extern LTPlane  g_ViewPlanes[6];                            // 0x1005bb60
extern LTPlane  g_ReallyClosePlanes[6];                     // 0x1005bbc0
extern ViewParams g_ViewParams;                             // 0x1005bd0c
extern float    g_CV_ModelSunVarianceClamp;                 // 0x1005d8b4

void d3d_InitModelVBs();                                    // 0x10001230
void d3d_TermModelVBs();                                    // 0x10001340


class CD3DModelDraw
{
public:
	CD3DModelDraw();                                        // 0x1000b75f
	~CD3DModelDraw();                                       // 0x1000b984

	void        DrawModel(ModelInstance* pInstance);        // 0x1000d2d6

	uint8       m_pad_000[0x634];
	float       m_fRadius;                                  // +0x634
	uint8       m_pad_638[0x8b4 - 0x638];
};


CD3DModelDraw   g_ModelDraw;                                                       // 0x100569b0
LTVector        g_vModelSunDir;
CConVar         g_CV_LightModelSprites("LightModelSprites", 0.0f, LTNULL, LTNULL); // 0x100572c8
CConVar         g_CV_ModelLODOffset("ModelLODOffset", 0.0f, LTNULL, LTNULL);       // 0x10057708
CConVar         g_CV_ModelZoomScale("ModelZoomScale", 1.0f, LTNULL, LTNULL);       // 0x10057868
CConVar         g_CV_ModelLODBlendEnable("ModelLODBlendEnable", 1.0f, LTNULL, LTNULL); // 0x10057848
CConVar         g_CV_ModelLODBlendDist("ModelLODBlendDist", 60.0f, LTNULL, LTNULL); // 0x10057828
CConVar         g_CV_ExtraFOVXOffset("ExtraFOVXOffset", 0.0f, LTNULL, LTNULL);     // 0x10057748
CConVar         g_CV_ExtraFOVYOffset("ExtraFOVYOffset", 0.0f, LTNULL, LTNULL);     // 0x10057270
CConVar         g_CV_ModelFovTest("ModelFovTest", 0.0f, LTNULL, LTNULL);           // 0x100577e8
CConVar         g_CV_ModelSunVariance("ModelSunVariance", 0.05f, LTNULL, LTNULL);  // 0x10058888
CConVar         g_CV_ModelApplySun("ModelApplySun", 1.0f, LTNULL, LTNULL);         // 0x10057808
CConVar         g_CV_ModelSaturation("ModelSaturation", 2.0f, LTNULL, LTNULL);     // 0x10057290
CConVar         g_CV_DrawModelsRigid("DrawModelsRigid", 1.0f, LTNULL, LTNULL);     // 0x10057728
CConVar         g_CV_ModelCacheRigid("ModelCacheRigid", 0.0f, LTNULL, LTNULL);     // 0x100572e8
CConVar         g_CV_ModelUseTnL("ModelUseTnL", 0.0f, LTNULL, LTNULL);             // 0x10056910

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
                             ViewParams* pParams)
{
	pDraw->m_fRadius = pInstance->GetModel()->m_fRadius;
	pDraw->m_fRadius *= LTMAX(pInstance->m_Scale.x,
		LTMAX(pInstance->m_Scale.y, pInstance->m_Scale.z));

	return d3d_ClipSphere(&pInstance->m_Pos, pDraw->m_fRadius,
		(pInstance->m_Flags & 0x40) ? g_ReallyClosePlanes : g_ViewPlanes,
		(uint32*)pParams) != 0;
}

// 0x1000b4d8
void d3d_DrawModel(void* pParams, ModelInstance* pInstance)
{
	CD3DModelDraw* pDraw;

	if (pInstance->m_AnimTracker.IsValid())
	{
		pDraw = &g_ModelDraw;
		if (d3d_SetupModel(pDraw, pInstance, &g_ViewParams))
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
	d3d_InitModelLists();
	d3d_InitModelVBs();
}

// 0x1000b75a
void d3d_TermModels()
{
	d3d_TermModelVBs();
}
