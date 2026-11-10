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
//  The drawer itself -- DrawModel, its sprite, LOD and lighting set-up,
//  the light grid sample, the vertex transforms, and the SDK inlines they
//  call out of line (MatVMul, Mat_InverseTransformation, TransformPlane,
//  Equals, Dist, ...) -- is the same code in both builds over the same
//  CD3DModelDraw; ModelInstance's tail is 0x14 bytes further on in Primal
//  Hunt, which also cloaks models (FLAG2_CHAMELEON: full colour, the red
//  as the alpha, environment-mapped) and turns the environment map's
//  offset.  AvP2's addresses below (Primal Hunt's 0x1000ba54..0x1000dfeb).
//
//  Near, and why:
//    * 0x1000bafe (AddLight): the spot blend's outer-colour product loads
//      its operands the other way round.
//    * 0x1000c100 (SetupLighting), 0x1000d3a7 (DrawModel), 0x1000ccd9
//      (DrawSprite): frame-exact; the inliner's budget runs out at other
//      sites than retail's (DrawSprite's corners call the _CVector
//      constructor out of line one step earlier).
//    * 0x1000be88, 0x1000c860, 0x1000ddc5, 0x1000de56: scheduling.
//  ModelTLVert's (empty) constructor is what makes its array members call
//  BaseNew<ModelTLVert> (0x1000f148) as retail's do, rather than expand
//  _AllocateTArray.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <d3d.h>
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




struct SharedTexture;

// A piece of a model, and its LODs.
struct PieceLOD
{
	uint8       m_pad_00[0x18];
	uint32      m_nTris;                                    // +0x18
	uint8       m_pad_1c[0x24 - 0x1c];
};

struct ModelPiece
{
	uint8       m_pad_00[0x18];
	uint32      m_nTris;                                    // +0x18 (LOD 0)
	uint8       m_pad_1c[0x38 - 0x1c];
	PieceLOD*   m_LODs;                                     // +0x38 (LODs 1..)
	uint32      m_nLODs;                                    // +0x3c

	// LOD 0 is the piece itself, LOD i its m_LODs[i - 1].
	PieceLOD*   GetLOD(uint32 iLOD)
	{
		if (iLOD == 0)
			return (PieceLOD*)this;

		iLOD--;
		if (iLOD < m_nLODs)
			return &m_LODs[iLOD];

		return LTNULL;
	}
};

// What the model drawer reads of a model (the engine's Model).
struct Model
{
	uint8       m_pad_00[0x38];
	ModelPiece** m_Pieces;                                  // +0x38
	uint32      m_nPieces;                                  // +0x3c
	uint8       m_pad_40[0x64 - 0x40];
	uint32      m_nVerts;                                   // +0x64
	uint8       m_pad_68[0x74 - 0x68];
	LTMatrix*   m_Transforms;                               // +0x74 (one per node)
	uint32      m_nNodes;                                   // +0x78
	uint8       m_pad_7c[0x90 - 0x7c];
	float*      m_LODDists;                                 // +0x90 (LODs 1..)
	uint32      m_nLODs;                                    // +0x94
	uint8       m_pad_98[0xa0 - 0x98];
	float       m_fLODDist0;                                // +0xa0
	float       m_fRadius;                                  // +0xa4
	float       m_fVisRadius;                               // +0xa8
	uint8       m_pad_ac[0xb0 - 0xac];
	float       m_fSpriteNear;                              // +0xb0
	float       m_fSpriteNearSqr;                           // +0xb4
	float       m_fSpriteFar;                               // +0xb8
	float       m_fSpriteFarSqr;                            // +0xbc
	SharedTexture* m_pSpriteTexture;                        // +0xc0
	float       m_fSpriteWidth;                             // +0xc4
	float       m_fSpriteHeight;                            // +0xc8
	uint8       m_pad_cc[0xf4 - 0xcc];
	uint32      m_iSocketNode;                              // +0xf4
	uint8       m_pad_f8[0xfc - 0xf8];
	LTMatrix    m_mSocket;                                  // +0xfc
	LTBOOL      m_bSocketLit;                               // +0x13c
	LTBOOL      m_bFovScale;                                // +0x140
	float       m_fFovX;                                    // +0x144
	float       m_fFovY;                                    // +0x148
	uint8       m_pad_14c[0x150 - 0x14c];
	LTBOOL      m_bRigid;                                   // +0x150
	uint8       m_pad_154[0x16c - 0x154];
	uint32      m_nAnims;                                   // +0x16c

	// LOD 0's distance is the model's own, LOD i's its m_LODDists[i - 1].
	float*      GetLODDist(uint32 iLOD)
	{
		if (iLOD == 0)
			return &m_fLODDist0;

		iLOD--;
		if (iLOD < m_nLODs)
			return &m_LODDists[iLOD];

		return LTNULL;
	}
};

// A model instance's animation tracker.
struct AnimTracker
{
	uint8       m_pad_00[0x28];
	Model*      m_pModel;                                   // +0x28
	uint16      m_iAnim;                                    // +0x2c
	uint8       m_pad_2e[0x38 - 0x2e];
	uint16      m_iDestAnim;                                // +0x38
	uint8       m_pad_3a[0x3c - 0x3a];

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
	virtual void        Slot00();
	virtual void        Slot04();
	virtual void        Slot08();
	virtual void        Slot0c();
	virtual void        Slot10();
	virtual void        Slot14();
	virtual void        Slot18();
	virtual void        Slot1c();
	virtual void        Slot20();
	virtual void        Slot24();
	virtual void        Slot28();
	virtual void        Slot2c();
	virtual void        Slot30();
	virtual void        Slot34();
	virtual void        Slot38();
	virtual void        Slot3c();
	virtual void        Slot40();
	virtual float       GetRadius();                        // +0x44
	virtual LTMatrix*   GetNodeTransforms();                // +0x48

	uint8       m_pad_004[0x88 - 0x04];
	uint32      m_Flags;                                    // +0x88
	uint32      m_Flags2;                                   // +0x8c
	uint8       m_pad_090[0x94 - 0x90];
	uint8       m_ColorR;                                   // +0x94
	uint8       m_ColorG;                                   // +0x95
	uint8       m_ColorB;                                   // +0x96
	uint8       m_ColorA;                                   // +0x97
	uint8       m_pad_098[0x9c - 0x98];
	LTRotation  m_Rotation;                                 // +0x9c
	LTVector    m_Scale;                                    // +0xac
	uint8       m_pad_0b8[0x188 - 0xb8];
	uint32      m_Flags3;                                   // +0x188
	uint8       m_pad_18c[0x194 - 0x18c];
	LTVector    m_Pos;                                      // +0x194
	uint8       m_pad_1a0[0x1bc - 0x1a0];
	ModelInstanceHookFn m_InstanceHookFn;                   // +0x1bc
	uint8       m_pad_1c0[0x1d0 - 0x1c0];
#ifdef PRIMAL_HUNT
	SharedTexture* m_pEnvMap;                               // +0x1d0
	AnimTracker m_AnimTracker;                              // +0x1d4
	uint8       m_pad_210[0x258 - 0x210];
	void*       m_pAttachments;                             // +0x258
	uint8       m_pad_25c[0x2c4 - 0x25c];
	LTVector    m_LastSunPos;                               // +0x2c4
	float       m_fLastSunVis;                              // +0x2d0
#else
	AnimTracker m_AnimTracker;                              // +0x1d0
	uint8       m_pad_20c[0x244 - 0x20c];
	void*       m_pAttachments;                             // +0x244
	uint8       m_pad_248[0x2b0 - 0x248];
	LTVector    m_LastSunPos;                               // +0x2b0
	float       m_fLastSunVis;                              // +0x2bc
#endif

	Model*      GetModel() { return m_AnimTracker.m_pModel; }
};

// The view (common_draw.cpp's): the view frustum's six planes, and a set
// for the really close view.
struct ViewParams
{
	uint8       m_pad_000[0x4c];
	float       m_xFov;                                     // +0x04c
	float       m_yFov;                                     // +0x050
	uint8       m_pad_054[0x88 - 0x54];
	float       m_fNearZ;                                   // +0x088
	uint8       m_pad_08c[0x9c - 0x8c];
	LTMatrix    m_mInvView;                                 // +0x09c
	LTMatrix    m_mView;                                    // +0x0dc
	uint8       m_pad_11c[0x15c - 0x11c];
	LTMatrix    m_mWorldToCamera;                           // +0x15c
	LTMatrix    m_mProjection;                              // +0x19c
	uint8       m_pad_1dc[0x25c - 0x1dc];
	LTMatrix    m_mReallyCloseFov;                          // +0x25c
	LTMatrix    m_mFov;                                     // +0x29c
	LTMatrix    m_mReallyCloseToCamera;                     // +0x2dc
	uint8       m_pad_31c[0x3d0 - 0x31c];
	LTPlane     m_ClipPlanes[6];                            // +0x3d0
	LTPlane     m_ReallyClosePlanes[6];                     // +0x430
	uint8       m_pad_490[0x4b4 - 0x490];
	LTVector    m_Pos;                                      // +0x4b4
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
	uint8       m_pad_000[0x1c];
	LTVector    m_GlobalModelLightAdd;                      // +0x1c
	uint8       m_pad_028[0x80 - 0x28];
	float       m_FrameTime;                                // +0x80
	uint8       m_pad_084[0xfc - 0x84];
	ModelHookFn m_ModelHookFn;                              // +0xfc
	void*       m_ModelHookUser;                            // +0x100
};

extern SceneDesc*   g_pSceneDesc;                           // 0x1005c14c

// A light the model is lit by.
struct ModelLight
{
	ModelLight();                                           // 0x1000b981

	LTVector    m_Pos;                                      // +0x00 (model space)
	float       m_fRadiusSqr;                               // +0x0c
	LTVector    m_Dir;                                      // +0x10
	LTVector    m_Color;                                    // +0x1c
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
	ModelTLVert() {}

	uint8       m_Data[0x20];
};

struct ModelClipVert
{
	uint8       m_Data[0x40];
};


struct TLVert;

// The vertex transform the drawer calls for each vertex (plain, or with
// the model warble).
typedef void (__fastcall *ModelTransformFn)(void* pVert, TLVert* pOut, LTMatrix* pMat);

class CD3DModelDraw
{
public:
	CD3DModelDraw();                                        // 0x1000b75f
	~CD3DModelDraw();                                       // 0x1000b984

	LTBOOL      AllocVerts();                               // 0x1000b9cc
	void        CallModelHook();                            // 0x1000ba1e
	void        DrawModel(ModelInstance* pInstance);        // 0x1000d2d6
	void        DrawPasses();                               // 0x100254d8 (drawmodel_env.cpp)
	float       CalcSunVis();
	void        SetupLOD();
	uint32      GetNumIndices();
	void        SetupNodeTransforms(LTMatrix* pMat);
	void        DrawSprite(ModelInstance* pInstance, uint8 nAlpha);
	void        SetupLighting();
	void        AddLight(LTMatrix* pWorldToModel, LTVector* pPos, float fRadius,
	                     float fInnerR, float fInnerG, float fInnerB,
	                     float fOuterR, float fOuterG, float fOuterB,
	                     LTVector* pDir, float fCosCone);

	Model*      m_pModel;                                   // +0x000
	ModelInstance* m_pInstance;                             // +0x004
	AnimTracker* m_pTracker;                                // +0x008
	ModelInstanceHookData m_InstanceHookData;               // +0x00c
	LTPlane     m_ClipPlane;                                // +0x020 (camera space)
	LTBOOL      m_bDetailTexture;                           // +0x030
	uint8       m_pad_034[0x38 - 0x34];
	int32       m_iLastLOD;                                 // +0x038
	ModelLight  m_Lights[16];                               // +0x03c
	uint32      m_nLights;                                  // +0x2bc
	uint32      m_nMaxLights;                               // +0x2c0
	uint8       m_pad_2c4[0x4d0 - 0x2c4];
	LTMatrix    m_mTransform;                               // +0x4d0 (model to world)
	LTMatrix    m_mModelToCamera;                           // +0x510
	LTMatrix    m_mCameraToModel;                           // +0x550
	LTMatrix    m_mEnvMap;                                  // +0x590
	LTVector    m_vPos;                                     // +0x5d0
	void        (*m_pfnBeginTransform)();                   // +0x5dc
	ModelTransformFn m_pfnTransform;                        // +0x5e0
	uint8       m_pad_5e4[0x60c - 0x5e4];
	uint32      m_iLOD;                                     // +0x60c
	LTBOOL      m_bBlendLOD;                                // +0x610
	float       m_fLODBlend;                                // +0x614
	LTBOOL      m_bEnvMap;                                  // +0x618
	uint8       m_pad_61c[0x634 - 0x61c];
	float       m_fRadius;                                  // +0x634
	LTBOOL      m_bTint;                                    // +0x638
	float       m_fLODDist;                                 // +0x63c
	uint32      m_FogColor;                                 // +0x640
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
	LTVector    m_vSunColor;                                // +0x868
	LTVector    m_vSunDir;                                  // +0x874 (model space)
	float       m_fSunVis;                                  // +0x880
	LTVector    m_vAmbient;                                 // +0x884
	ModelHookData m_HookData;                               // +0x890
	uint8       m_pad_8a4[0x8a8 - 0x8a4];
	uint8       m_nAlpha;                                   // +0x8a8
	uint8       m_pad_8a9[0x8ac - 0x8a9];
	uint32      m_iCacheState;                              // +0x8ac (MCS_)
	LTBOOL      m_bUseTnL;                                  // +0x8b0
	uint8       m_pad_8b4[0x8b8 - 0x8b4];
	uint8       m_ObjectAlpha;                              // +0x8b8 (Primal Hunt's translucent models)
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

float           g_fWarbleTime;                              // 0x100572b0 (RenderScene advances it)
static CheapLTLink g_ModelLights;                              // 0x100572b8
static float    g_fModelSun[32];                            // 0x10057768
static float    g_fModelSunDelta[32];                       // 0x10056930
float           g_fWarbleFrac;                              // 0x100588a8
static CheapLTLink g_ModelShadows;                             // 0x100588b0
#ifdef PRIMAL_HUNT
static float    g_fEnvMapAngle;                             // 0x100588c0
float           g_fEnvMapOffsetU;                           // 0x100588c4 (drawmodel_env.cpp's)
float           g_fEnvMapOffsetV;                           // 0x100588c8
#endif


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
	g_fWarbleFrac = 0.0f;
	g_fWarbleTime = 0.0f;
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
	m_vSpecularLight.Init(2.0f, -2.0f, -2.0f);
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
	if (m_pModel->m_nVerts > m_TLVerts.GetSize())
	{
		if (!m_TLVerts.SetSize2(m_pModel->m_nVerts + 32, &g_DefAlloc))
			return LTFALSE;
	}

	if (m_pModel->m_nNodes > m_ClipVerts.GetSize())
	{
		if (!m_ClipVerts.SetSize2(m_pModel->m_nNodes, &g_DefAlloc))
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

// A box query of the world tree: the callback is called for each object
// whose box meets [m_Min, m_Max].
typedef void (*FindObjectsCB)(LTObject* pObject, void* pUser);

struct FindObjectsQuery
{
	FindObjectsQuery()
	{
		m_pTree = LTNULL;
		m_Flags = 0;
		m_Min.Init();
		m_Max.Init();
		m_Point.Init();
		m_CB = LTNULL;
		m_pUser = LTNULL;
	}

	void*       m_pTree;                                    // +0x00
	uint32      m_Flags;                                    // +0x04
	LTVector    m_Min;                                      // +0x08
	LTVector    m_Max;                                      // +0x14
	LTVector    m_Point;                                    // +0x20
	FindObjectsCB m_CB;                                     // +0x2c
	void*       m_pUser;                                    // +0x30
};

class WorldTree
{
public:
	virtual void        Slot00();
	virtual void        Slot04();
	virtual void        FindObjectsInBox(FindObjectsQuery* pQuery);   // +0x08

	uint8       m_pad_04[0x4c - 0x04];
	float       m_fRootSize;                                // +0x4c
	uint8       m_pad_50[0x90 - 0x50];
};

// The world's light grid: one b, g, r sample per cell.
struct LightSample
{
	uint8       m_b;
	uint8       m_g;
	uint8       m_r;
	uint8       m_a;
};

struct LightTable
{
	LightSample* m_pData;                                   // +0x00
	uint32      m_n04;                                      // +0x04
	int         m_nYStride;                                 // +0x08
	uint32      m_n0c;                                      // +0x0c
	uint32      m_n10;                                      // +0x10
	int         m_nMaxX;                                    // +0x14
	int         m_nMaxY;                                    // +0x18
	int         m_nMaxZ;                                    // +0x1c
	int         m_nZStride;                                 // +0x20
	LTVector    m_vCellSize;                                // +0x24
	LTVector    m_vScale;                                   // +0x30
	LTVector    m_vBase;                                    // +0x3c
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
	WorldTree   m_WorldTree;                                // +0x06c
	LightTable  m_LightTable;                               // +0x0fc
	uint8       m_pad_144[0x18c - 0x144];
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

	if (i_IntersectSegment(&iQuery, &iInfo, &g_pSceneWorld->m_WorldTree, LTNULL))
	{
		pPoly = g_pSceneWorld->GetPolyFromHPoly(iInfo.m_hPoly);
		if (pPoly)
			return (pPoly->m_pSurface->m_Flags & SURFFLAG_SKY) ? LTTRUE : LTFALSE;
	}

	return LTFALSE;
}


#define MAX_MODEL_LIGHTS    16

// Round to nearest in the FPU's mode (LithTech's assembly helper).
inline int32 RoundFloatToInt(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}
// Primal Hunt's cloaked models (the game's FLAG2_CHAMELEON2).
#define FLAG2_CHAMELEON     (1<<11)

#define MIN_MODEL_FOV       MATH_DEGREES_TO_RADIANS(10.0f)
#define MAX_MODEL_FOV       MATH_DEGREES_TO_RADIANS(170.0f)

// ---------------------------------------------------------------------- //
//  The model's lights.
// ---------------------------------------------------------------------- //

extern LTVector g_vGlobalLightColor;                        // 0x1005b780

// What the lighting reads of the engine's render struct.
struct ModelRenderStruct
{
	uint8       m_pad_000[0x10c];
	LTBOOL      m_bEnvMapEnable;                            // +0x10c
	uint8       m_pad_110[0x138 - 0x110];
	LTVector    m_vSunDir;                                  // +0x138
	LTVector    m_vSunColor;                                // +0x144
};

// A dynamic light (an LTObject).
struct DynamicLight
{
	uint8       m_pad_000[0x94];
	uint8       m_ColorR;                                   // +0x94
	uint8       m_ColorG;                                   // +0x95
	uint8       m_ColorB;                                   // +0x96
	uint8       m_pad_097[0x194 - 0x97];
	LTVector    m_Pos;                                      // +0x194
	uint8       m_pad_1a0[0x1b0 - 0x1a0];
	float       m_LightRadius;                              // +0x1b0
};

// A static light of the world (an object of its tree).
struct StaticLight
{
	uint8       m_pad_00[0x68];
	LTVector    m_Pos;                                      // +0x68
	float       m_Radius;                                   // +0x74
	LTVector    m_InnerColor;                               // +0x78
	LTVector    m_Dir;                                      // +0x84
	float       m_FOV;                                      // +0x90 (cos of the cone)
	LTVector    m_OuterColor;                               // +0x94
};

struct ModelLightCBData
{
	CD3DModelDraw* m_pDraw;
	LTMatrix    m_mWorldToModel;
};

extern ModelRenderStruct* g_pStruct;                        // 0x1005df08 (common_stuff.cpp)
extern int32    g_CV_MaxModelLights;                        // 0x1005dad4
extern int32    g_CV_Saturate;                              // 0x1005d384
extern uint32   g_nDynamicLights;                           // 0x1005bcb0
extern DynamicLight* g_DynamicLights[];                     // 0x1005c168

// 0x1000c860 -- the world's light grid at a point: the eight cells around
// it, blended along y, then x, then z (server.dll's w_GetPointShade, the
// renderer's own copy).
void d3d_GetLightGridColor(LightTable* pTable, LTVector* pPoint, uint8* pColor)
{
	LTVector vSample[8];
	LTVector vNear, vFar;
	LTVector vLeft, vRight;
	LTVector vInv;
	LightSample* pCell;
	int ix, iy, iz;

	LTVector v;
	v = *pPoint - pTable->m_vBase;
	v.x *= pTable->m_vScale.x;
	v.y *= pTable->m_vScale.y;
	v.z *= pTable->m_vScale.z;

	ix = (int)v.x;
	iy = (int)v.y;
	iz = (int)v.z;

	if (ix < 0)
		ix = 0;
	else if (ix > pTable->m_nMaxX)
		ix = pTable->m_nMaxX;

	if (iy < 0)
		iy = 0;
	else if (iy > pTable->m_nMaxY)
		iy = pTable->m_nMaxY;

	if (iz < 0)
		iz = 0;
	else if (iz > pTable->m_nMaxZ)
		iz = pTable->m_nMaxZ;

	if (ix == pTable->m_nMaxX)
	{
		v.x = 0.0f;
		vInv.x = 1.0f;
		ix--;
	}
	else
	{
		v.x -= (float)floor(v.x);
		vInv.x = 1.0f - v.x;
	}

	if (iy == pTable->m_nMaxY)
	{
		v.y = 0.0f;
		vInv.y = 1.0f;
		iy--;
	}
	else
	{
		v.y -= (float)floor(v.y);
		vInv.y = 1.0f - v.y;
	}

	if (iz == pTable->m_nMaxZ)
	{
		v.z = 0.0f;
		vInv.z = 1.0f;
		iz--;
	}
	else
	{
		v.z -= (float)floor(v.z);
		vInv.z = 1.0f - v.z;
	}

	pCell = pTable->m_pData + (ix + pTable->m_nYStride * iy + pTable->m_nZStride * iz);

	vSample[0].x = pCell[pTable->m_nYStride].m_r;
	vSample[0].y = pCell[pTable->m_nYStride].m_g;
	vSample[0].z = pCell[pTable->m_nYStride].m_b;
	vSample[1].x = pCell[pTable->m_nYStride + 1].m_r;
	vSample[1].y = pCell[pTable->m_nYStride + 1].m_g;
	vSample[1].z = pCell[pTable->m_nYStride + 1].m_b;
	vSample[2].x = pCell[0].m_r;
	vSample[2].y = pCell[0].m_g;
	vSample[2].z = pCell[0].m_b;
	vSample[3].x = pCell[1].m_r;
	vSample[3].y = pCell[1].m_g;
	vSample[3].z = pCell[1].m_b;

	pCell += pTable->m_nZStride;

	vSample[4].x = pCell[pTable->m_nYStride].m_r;
	vSample[4].y = pCell[pTable->m_nYStride].m_g;
	vSample[4].z = pCell[pTable->m_nYStride].m_b;
	vSample[5].x = pCell[pTable->m_nYStride + 1].m_r;
	vSample[5].y = pCell[pTable->m_nYStride + 1].m_g;
	vSample[5].z = pCell[pTable->m_nYStride + 1].m_b;
	vSample[6].x = pCell[0].m_r;
	vSample[6].y = pCell[0].m_g;
	vSample[6].z = pCell[0].m_b;
	vSample[7].x = pCell[1].m_r;
	vSample[7].y = pCell[1].m_g;
	vSample[7].z = pCell[1].m_b;

	vLeft.x  = vInv.y * vSample[2].x;
	vLeft.y  = vInv.y * vSample[2].y;
	vLeft.z  = vInv.y * vSample[2].z;
	vLeft.x += v.y * vSample[0].x;
	vLeft.y += v.y * vSample[0].y;
	vLeft.z += v.y * vSample[0].z;

	vRight.x  = vInv.y * vSample[3].x;
	vRight.y  = vInv.y * vSample[3].y;
	vRight.z  = vInv.y * vSample[3].z;
	vRight.x += v.y * vSample[1].x;
	vRight.y += v.y * vSample[1].y;
	vRight.z += v.y * vSample[1].z;

	vNear.x  = vInv.x * vLeft.x;
	vNear.y  = vInv.x * vLeft.y;
	vNear.z  = vInv.x * vLeft.z;
	vNear.x += v.x * vRight.x;
	vNear.y += v.x * vRight.y;
	vNear.z += v.x * vRight.z;

	vLeft.x  = vInv.y * vSample[6].x;
	vLeft.y  = vInv.y * vSample[6].y;
	vLeft.z  = vInv.y * vSample[6].z;
	vLeft.x += v.y * vSample[4].x;
	vLeft.y += v.y * vSample[4].y;
	vLeft.z += v.y * vSample[4].z;

	vRight.x  = vInv.y * vSample[7].x;
	vRight.y  = vInv.y * vSample[7].y;
	vRight.z  = vInv.y * vSample[7].z;
	vRight.x += v.y * vSample[5].x;
	vRight.y += v.y * vSample[5].y;
	vRight.z += v.y * vSample[5].z;

	vFar.x  = vInv.x * vLeft.x;
	vFar.y  = vInv.x * vLeft.y;
	vFar.z  = vInv.x * vLeft.z;
	vFar.x += v.x * vRight.x;
	vFar.y += v.x * vRight.y;
	vFar.z += v.x * vRight.z;

	vRight.x  = vInv.z * vNear.x;
	vRight.y  = vInv.z * vNear.y;
	vRight.z  = vInv.z * vNear.z;
	vRight.x += v.z * vFar.x;
	vRight.y += v.z * vFar.y;
	vRight.z += v.z * vFar.z;

	pColor[2] = (uint8)vRight.x;
	pColor[1] = (uint8)vRight.y;
	pColor[0] = (uint8)vRight.z;
}

// 0x1000baa4 -- the world tree's callback for each static light near the
// model.
void d3d_ModelLightCB(LTObject* pObject, void* pUser)
{
	StaticLight* pLight = (StaticLight*)pObject;
	ModelLightCBData* pData = (ModelLightCBData*)pUser;

	pData->m_pDraw->AddLight(&pData->m_mWorldToModel, &pLight->m_Pos,
		pLight->m_Radius,
		pLight->m_InnerColor.x, pLight->m_InnerColor.y, pLight->m_InnerColor.z,
		pLight->m_OuterColor.x, pLight->m_OuterColor.y, pLight->m_OuterColor.z,
		&pLight->m_Dir, pLight->m_FOV);
}

// 0x1000bafe -- light the model with one more light, a point light or
// (fCosCone above -1) a spot light, whose colour runs from vInnerColor on
// its axis to vOuterColor at the cone's edge.
void CD3DModelDraw::AddLight(LTMatrix* pWorldToModel, LTVector* pPos,
                             float fRadius, float fInnerR, float fInnerG,
                             float fInnerB, float fOuterR, float fOuterG,
                             float fOuterB, LTVector* pDir, float fCosCone)
{
	ModelLight* pLight;
	LTVector vDelta;
	float fDist, fDot, fT;

	if (m_nLights >= m_nMaxLights)
		return;

	vDelta = m_vPos - *pPos;
	fDist = vDelta.Mag();
	if (fDist - m_pModel->m_fVisRadius >= fRadius)
		return;

	if (fCosCone > -1.0f && fCosCone < 1.0f && fDist != 0.0f)
	{
		fDot = (vDelta.z * pDir->z + vDelta.y * pDir->y + vDelta.x * pDir->x) * (1.0f / fDist);
		if (fDot < fCosCone)
			return;

		fT = (1.0f - (fDot + 1.0f) * 0.5f) / (1.0f - (fCosCone + 1.0f) * 0.5f);
		fInnerR = (1.0f - fT) * fInnerR + fOuterR * fT;
		fInnerG = (1.0f - fT) * fInnerG + fOuterG * fT;
		fInnerB = (1.0f - fT) * fInnerB + fOuterB * fT;
	}

	pLight = &m_Lights[m_nLights];
	pLight->m_Color.x = fInnerR;
	pLight->m_Color.y = fInnerG;
	pLight->m_Color.z = fInnerB;
	VEC_MUL(pLight->m_Color, g_vGlobalLightColor, pLight->m_Color);
	pLight->m_Color *= m_vObjectColor;

	MatVMul(&pLight->m_Pos, pWorldToModel, pPos);
	pLight->m_fRadiusSqr = fRadius * fRadius;
	pLight->m_Dir = pLight->m_Pos;
	pLight->m_Dir.Norm();
	pLight->m_Dir /= pLight->m_fRadiusSqr;

	m_nLights++;
}

// 0x1000be88 -- how much of the sun reaches the model: 1 when a ray from
// its centre toward the sun ends on the sky, 0 when not.  With a sun
// variance, rays from either side of the model too, and a search for the
// edge of the shadow between them.
float CD3DModelDraw::CalcSunVis()
{
	LTVector vRay, vSunDir, vRight, vUp, vForward, vPerp, vPoint;
	float fMid, fLo, fHi, fStep, fVis;
	LTBOOL bHitLo, bHitHi, bHit;

	vSunDir = g_pStruct->m_vSunDir * (g_pSceneWorld->m_WorldTree.m_fRootSize * -2.0f);
	vRay = vSunDir;

	if (g_CV_ModelSunVariance.m_fValue == 0.0f)
		return d3d_SegmentHitsSky(&m_vPos, &vRay) ? 1.0f : 0.0f;

	m_mTransform.GetBasisVectors(&vRight, &vUp, &vForward);
	vPerp = g_pStruct->m_vSunDir.Cross(vRight);
	vPerp *= m_pInstance->GetRadius();

	vPoint = m_vPos + vPerp;
	bHitHi = d3d_SegmentHitsSky(&vPoint, &vRay);
	vPoint = m_vPos - vPerp;
	bHitLo = d3d_SegmentHitsSky(&vPoint, &vRay);

	if (bHitHi == bHitLo)
		return bHitHi ? 1.0f : 0.0f;

	fLo = -1.0f;
	fStep = 1.0f;
	fHi = 1.0f;
	fVis = 1.0f;
	while (fStep > g_CV_ModelSunVariance.m_fValue && bHitLo != bHitHi)
	{
		fMid = (fLo + fHi) * 0.5f;
		vPoint = m_vPos + vPerp * fMid;
		bHit = d3d_SegmentHitsSky(&vPoint, &vRay);

		fStep *= 0.5f;
		if (!bHit)
			fVis -= fStep;

		if (bHit == bHitHi)
		{
			fHi = fMid;
			bHitHi = bHit;
		}
		else
		{
			fLo = fMid;
			bHitLo = bHit;
		}
	}

	return fVis;
}

// 0x1000c100 -- the model's lights for this frame: the sun, the light
// grid's ambient, the dynamic lights and the world's static lights, all
// in model space.
void CD3DModelDraw::SetupLighting()
{
	FindObjectsQuery query;
	ModelLightCBData cbData;
	LTMatrix mWorldToModel;
	LTVector vZero, vRadius;
	uint8 color[4];
	uint32 i;

	m_nLights = 0;
	m_nMaxLights = g_CV_MaxModelLights;
	m_nMaxLights = LTMIN(m_nMaxLights, MAX_MODEL_LIGHTS);

	if (m_pModel->m_bSocketLit)
	{
		mWorldToModel = m_mTransform * (m_mCameraToModel *
			m_pModel->m_Transforms[m_pModel->m_iSocketNode] * m_pModel->m_mSocket);
		if (m_pInstance->m_Flags & FLAG_REALLYCLOSE)
			mWorldToModel = g_ViewParams.m_mInvView * mWorldToModel;
		mWorldToModel = ~mWorldToModel;
	}
	else if (m_pInstance->m_Flags & FLAG_REALLYCLOSE)
	{
		mWorldToModel = g_ViewParams.m_mInvView * m_mTransform;
		mWorldToModel = ~mWorldToModel;
	}
	else
	{
		Mat_InverseTransformation(&m_mTransform, &mWorldToModel);
	}

#ifdef PRIMAL_HUNT
	// The cloaked: full colour, and the instance's red as the alpha
	// (drawmodel_env.cpp's DrawPasses).
	if (m_pInstance->m_Flags2 & FLAG2_CHAMELEON)
	{
		m_vObjectColor.x = 255.0f;
		m_vObjectColor.y = 255.0f;
		m_vObjectColor.z = 255.0f;
		m_ObjectAlpha = m_pInstance->m_ColorR;
	}
	else
	{
		m_ObjectAlpha = 0;
		m_vObjectColor.x = (float)m_pInstance->m_ColorR;
		m_vObjectColor.y = (float)m_pInstance->m_ColorG;
		m_vObjectColor.z = (float)m_pInstance->m_ColorB;
	}
#else
	m_vObjectColor.x = (float)m_pInstance->m_ColorR;
	m_vObjectColor.y = (float)m_pInstance->m_ColorG;
	m_vObjectColor.z = (float)m_pInstance->m_ColorB;
#endif
	m_vLightAdd = g_pSceneDesc->m_GlobalModelLightAdd;

	CallModelHook();

	m_vObjectColor *= (1.0f / 255.0f);
	if (g_CV_Saturate)
		m_vObjectColor *= g_CV_ModelSaturation.m_fValue;

	if (m_pInstance->m_Flags & FLAG_NOLIGHT)
	{
		m_vSunDir.Init();
		m_fSunVis = 0.0f;
		m_vAmbient.x = m_vAmbient.y = m_vAmbient.z = 255.0f;
		return;
	}

	if (g_pSceneWorld && g_CV_ModelApplySun.m_nValue &&
		(g_vGlobalLightColor.x != 0.0f || g_vGlobalLightColor.y != 0.0f || g_vGlobalLightColor.z != 0.0f) &&
		(g_pStruct->m_vSunColor.x != 0.0f || g_pStruct->m_vSunColor.y != 0.0f || g_pStruct->m_vSunColor.z != 0.0f))
	{
		if (m_pInstance->m_fLastSunVis >= 0.0f &&
			(!(m_pInstance->m_Flags2 & FLAG2_DYNAMICDIRLIGHT) ||
			 m_vPos.Equals(m_pInstance->m_LastSunPos, g_CV_ModelSunVariance.m_fValue + 0.001f)))
		{
			m_fSunVis = m_pInstance->m_fLastSunVis;
		}
		else
		{
			m_pInstance->m_LastSunPos = m_vPos;
			m_pInstance->m_fLastSunVis = m_fSunVis = CalcSunVis();
		}

		MatVMul_3x3(&m_vSunDir, &mWorldToModel, &g_pStruct->m_vSunDir);
		m_vSunDir = -m_vSunDir;
	}
	else
	{
		m_vSunDir.Init();
		m_fSunVis = 0.0f;
	}

	m_vSunColor = g_pStruct->m_vSunColor * g_vGlobalLightColor;

	if (g_pSceneWorld)
	{
		d3d_GetLightGridColor(&g_pSceneWorld->m_LightTable, &m_vPos, color);
		m_vAmbient.x = (float)color[2];
		m_vAmbient.y = (float)color[1];
		m_vAmbient.z = (float)color[0];
	}
	else
	{
		m_vAmbient.Init();
	}
	m_vAmbient *= g_vGlobalLightColor;

	m_nLights = 0;
	vZero.Init();
	for (i = 0; i < g_nDynamicLights; i++)
	{
		DynamicLight* pLight = g_DynamicLights[i];

		AddLight(&mWorldToModel, &pLight->m_Pos, pLight->m_LightRadius,
			(float)pLight->m_ColorR, (float)pLight->m_ColorG, (float)pLight->m_ColorB,
			0.0f, 0.0f, 0.0f, &vZero, -1.0f);
		if (m_nLights >= m_nMaxLights)
			break;
	}

	if (m_nLights < m_nMaxLights && g_pSceneWorld)
	{
		cbData.m_pDraw = this;
		cbData.m_mWorldToModel = mWorldToModel;

		query.m_Flags = 1;
		vRadius.Init(m_pModel->m_fVisRadius, m_pModel->m_fVisRadius, m_pModel->m_fVisRadius);
		query.m_Min = m_vPos - vRadius;
		vRadius.Init(m_pModel->m_fVisRadius, m_pModel->m_fVisRadius, m_pModel->m_fVisRadius);
		query.m_Max = m_vPos + vRadius;
		query.m_CB = d3d_ModelLightCB;
		query.m_pUser = &cbData;
		g_pSceneWorld->m_WorldTree.FindObjectsInBox(&query);
	}
}

// ---------------------------------------------------------------------- //
//  Drawing a model.
// ---------------------------------------------------------------------- //

// How DrawModel found the model in the vertex cache.
#define MCS_NONE        0       // not cached
#define MCS_FILL        1       // cached from this draw
#define MCS_CACHED      2       // drawn from the cache

// The pass list a model's draw could hold (unused: only built).
struct ModelPassList
{
	ModelPassList()
	{
		m_nFlags = 0;
		m_nPasses = 0;
		m_iCurPass = 0;
		m_iCurStage = 0;
		m_iCurTexture = 0;
		m_pEnvMap = LTNULL;
	}

	ModelPass   m_Passes[8];                                // +0x000
	LTBOOL      m_pEnvMap;                                  // +0x120
	uint32      m_nFlags;                                   // +0x124
	uint32      m_nPasses;                                  // +0x128
	uint32      m_iCurPass;                                 // +0x12c
	uint32      m_iCurStage;                                // +0x130
	uint32      m_iCurTexture;                              // +0x134
	uint8       m_Reserved[0x2a8 - 0x138];
};

// A vertex the transform reads (its position at +0x08).
struct ModelSrcVert
{
	uint8       m_pad_00[0x08];
	LTVector    m_Vec;                                      // +0x08
};

struct TLVert
{
	LTVector    m_Vec;                                      // +0x00
	float       m_RHW;                                      // +0x0c
};

// A transformed, lit vertex (D3DFVF_TLVERTEX).
struct ModelSpriteVert
{
	LTVector    m_Vec;                                      // +0x00
	float       m_RHW;                                      // +0x0c
	uint8       m_B, m_G, m_R, m_A;                         // +0x10
	uint32      m_Specular;                                 // +0x14
	float       m_TU, m_TV;                                 // +0x18

	void        SetUV(float u, float v) { m_TU = u; m_TV = v; }
};

class CVertexBufferCache
{
public:
	LTBOOL      Find(uint32 key1, uint32 key2);             // 0x1003b81b
	LTBOOL      Alloc(uint32 key1, uint32 key2, uint32 nVerts, LTBOOL bGrow);  // 0x1003b862
	void        AgeCache();                                 // 0x1003ba10
};

extern CVertexBufferCache g_ModelVBCache;                   // 0x100995a8
extern LPDIRECT3DDEVICE7 g_pD3DDevice;                      // 0x100638c8
extern uint32   g_iCurTextureStage;                         // 0x100638c0
extern uint16   g_CurFrameCode;                             // 0x1005d238
extern int32    g_CV_DrawGuns;                              // 0x1005d310
extern int32    g_CV_EnvMapAll;                             // 0x1005d868
extern int32    g_CV_EnvMapEnable;                          // 0x1005d678
extern int32    g_CV_TintModels;                            // 0x1004a798
extern int32    g_CV_ModelWarble;                           // 0x1005df1c
extern CD3DConVar g_CV_ReallyCloseNearZ;                    // 0x1005a328 (d3d_viewparams.cpp)
extern uint8    g_LightTableR[256];                         // 0x1005fa9c
extern uint8    g_LightTableG[256];
extern uint8    g_LightTableB[256];
extern float    g_fTexelSizeU;                              // 0x100672a8
extern float    g_fTexelSizeV;                              // 0x100672ac
extern void     (__fastcall *g_pfnCalcFog)(const LTVector* pPos, uint32* pSpecular); // 0x1005e1c4
extern uint32   g_nModelsClipped;                           // 0x1005e278
extern uint32   g_nModelsUnclipped;                         // 0x1005e774

int     d3d_SetTexture(SharedTexture* pTexture, uint32 iStage, uint32 flags);  // 0x100079e4
LTBOOL  d3d_ClipTLVerts(ModelSpriteVert** ppVerts, int* pnVerts, ViewParams* pParams, uint32 flags);  // 0x1000aec6
void    d3d_SetupTransformation(LTVector* pPos, LTRotation* pRot, LTVector* pScale, LTMatrix* pMat); // 0x100091f0

static uint32   g_iModelWarble;                             // 0x100572c4

void d3d_BeginTransform();
void __fastcall d3d_Transform(void* pVert, TLVert* pOut, LTMatrix* pMat);
void d3d_BeginWarbleTransform();
void __fastcall d3d_WarbleTransform(void* pVert, TLVert* pOut, LTMatrix* pMat);
static uint16   g_ModelCacheFrameCode;                      // 0x100588cc

// 0x1000d340
static HRESULT d3d_DrawPrimitive(D3DPRIMITIVETYPE type, DWORD fvf, LPVOID pVerts,
                                 DWORD nVerts, DWORD flags)
{
	return g_pD3DDevice->DrawPrimitive(type, fvf, pVerts, nVerts, flags);
}

// 0x1000ccd9 -- the model as its sprite: a camera-facing quad lit from
// the light grid, nAlpha translucent.
void CD3DModelDraw::DrawSprite(ModelInstance* pInstance, uint8 nAlpha)
{
	ModelSpriteVert verts[4];
	ModelSpriteVert* pVerts;
	int nVerts;
	uint8 color[4];
	LTVector vDir, vRight, vTemp;
	Model* pModel;
	LTVector vUp(0.0f, 1.0f, 0.0f);

	pModel = pInstance->GetModel();
	if (!pModel->m_pSpriteTexture)
		return;

	d3d_SetTexture(pModel->m_pSpriteTexture, g_iCurTextureStage, 0);

	vDir = m_vPos - g_ViewParams.m_Pos;
	vRight = vDir.Cross(vUp);
	vRight.y = 0.0f;
	vRight.Norm();

	vUp.Init(0.0f, pModel->m_fSpriteHeight, 0.0f);
	verts[0].m_Vec = m_vPos - vRight * pModel->m_fSpriteWidth * pInstance->m_Scale.x +
		vUp * pInstance->m_Scale.y;
	verts[1].m_Vec = m_vPos + vRight * pModel->m_fSpriteWidth * pInstance->m_Scale.x +
		LTVector(0.0f, pModel->m_fSpriteHeight, 0.0f) * pInstance->m_Scale.y;
	verts[2].m_Vec = m_vPos + vRight * pModel->m_fSpriteWidth * pInstance->m_Scale.x -
		LTVector(0.0f, pModel->m_fSpriteHeight, 0.0f) * pInstance->m_Scale.y;
	verts[3].m_Vec = m_vPos - vRight * pModel->m_fSpriteWidth * pInstance->m_Scale.x -
		LTVector(0.0f, pModel->m_fSpriteHeight, 0.0f) * pInstance->m_Scale.y;

	verts[0].SetUV(g_fTexelSizeU, g_fTexelSizeV);
	verts[1].SetUV(1.0f - g_fTexelSizeU, g_fTexelSizeV);
	verts[2].SetUV(1.0f - g_fTexelSizeU, 1.0f - g_fTexelSizeV);
	verts[3].SetUV(g_fTexelSizeU, 1.0f - g_fTexelSizeV);

	if (g_CV_LightModelSprites.m_nValue && g_pSceneWorld)
	{
		d3d_GetLightGridColor(&g_pSceneWorld->m_LightTable, &m_vPos, color);
		verts[0].m_R = verts[1].m_R = verts[2].m_R = verts[3].m_R = g_LightTableR[color[2]];
		verts[0].m_G = verts[1].m_G = verts[2].m_G = verts[3].m_G = g_LightTableG[color[1]];
		verts[0].m_B = verts[1].m_B = verts[2].m_B = verts[3].m_B = g_LightTableB[color[0]];
	}
	else
	{
		verts[0].m_R = verts[1].m_R = verts[2].m_R = verts[3].m_R = g_LightTableR[255];
		verts[0].m_G = verts[1].m_G = verts[2].m_G = verts[3].m_G = g_LightTableG[255];
		verts[0].m_B = verts[1].m_B = verts[2].m_B = verts[3].m_B = g_LightTableB[255];
	}
	verts[0].m_A = verts[1].m_A = verts[2].m_A = verts[3].m_A = nAlpha;
	verts[0].m_Specular = verts[1].m_Specular = verts[2].m_Specular = verts[3].m_Specular = m_FogColor;

	nVerts = 4;
	g_ClipFlags = 0x3f;
	pVerts = verts;
	if (d3d_ClipTLVerts(&pVerts, &nVerts, &g_ViewParams, 0))
		d3d_DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, pVerts, nVerts, 0);
}

// 0x1000d35f -- the indices of the model's pieces at the current LOD.
uint32 CD3DModelDraw::GetNumIndices()
{
	uint32 i, nTris;
	PieceLOD* pLOD;

	nTris = 0;
	for (i = 0; i < m_pModel->m_nPieces; i++)
	{
		pLOD = m_pModel->m_Pieces[i]->GetLOD(m_iLOD);
		if (pLOD)
			nTris += pLOD->m_nTris;
	}

	return nTris * 3;
}

// 0x1000de1f -- the vertex cache's key for a fog colour and LOD.
static uint32 d3d_GetModelCacheKey(uint32 fogColor, uint32 iLOD)
{
	return ((((((((iLOD & 0xc0) << 6) | (iLOD & 0x30)) << 6) | (iLOD & 0xc)) << 6) | (iLOD & 3)) << 6) |
		((fogColor >> 2) & 0x3f3f3f3f);
}

// 0x1000d3a7 -- draw one model.
void CD3DModelDraw::DrawModel(ModelInstance* pInstance)
{
	ModelPassList passList;
	LTMatrix mProj;
	float fDistSqr, fT, fFovX, fFovY, fSaveNearZ;
	LTMatrix* pTransforms;
	uint32 i, key;

	m_pTracker = &pInstance->m_AnimTracker;
	m_pModel = m_pTracker->m_pModel;
	m_pInstance = pInstance;

	if (pInstance->m_Flags & FLAG_REALLYCLOSE)
		g_ViewParams.m_mInvView.Apply(pInstance->m_Pos, m_vPos);
	else
		m_vPos = pInstance->m_Pos;

	if (!g_CV_DrawModelsRigid.m_nValue && m_pModel->m_bRigid)
		return;

	if (!g_CV_DrawGuns && (pInstance->m_Flags & FLAG_REALLYCLOSE))
		return;

	m_bDetailTexture = (m_pInstance->m_Flags & FLAG_DETAILTEXTURE) != 0;
	g_pfnCalcFog(&m_vPos, &m_FogColor);

	fDistSqr = (m_vPos - g_ViewParams.m_Pos).MagSqr();

	if (m_pModel->m_fSpriteFarSqr >= m_pModel->m_fSpriteNearSqr)
	{
		if (fDistSqr > m_pModel->m_fSpriteFarSqr)
		{
			DrawSprite(pInstance, 0xff);
			return;
		}

		if (fDistSqr > m_pModel->m_fSpriteNearSqr)
		{
			fT = ((float)sqrt(fDistSqr) - m_pModel->m_fSpriteNear) /
				(m_pModel->m_fSpriteFar - m_pModel->m_fSpriteNear);
			m_nAlpha = (uint8)RoundFloatToInt((1.0f - fT) * m_pInstance->m_ColorA);
			DrawSprite(pInstance, 0xff - m_nAlpha);
		}
		else
		{
			m_nAlpha = m_pInstance->m_ColorA;
		}
	}
	else
	{
		if (fDistSqr < m_pModel->m_fSpriteFarSqr)
			return;

		if (fDistSqr < m_pModel->m_fSpriteNearSqr)
		{
			fT = ((float)sqrt(fDistSqr) - m_pModel->m_fSpriteFar) /
				(m_pModel->m_fSpriteNear - m_pModel->m_fSpriteFar);
			m_nAlpha = (uint8)RoundFloatToInt(fT * m_pInstance->m_ColorA);
		}
		else
		{
			m_nAlpha = m_pInstance->m_ColorA;
		}
	}

#ifdef PRIMAL_HUNT
	// Environment-mapped when the console says all, or the model asks for
	// it and has a map, and is cloaked or the console enables maps.
	m_bEnvMap = LTFALSE;
	if (g_CV_EnvMapAll == 1 ||
		((pInstance->m_Flags & FLAG_ENVIRONMENTMAP) &&
		 (g_pStruct->m_bEnvMapEnable || m_pInstance->m_pEnvMap) &&
		 ((pInstance->m_Flags2 & FLAG2_CHAMELEON) || g_CV_EnvMapEnable == 1)))
	{
		m_bEnvMap = LTTRUE;
	}
#else
	m_bEnvMap = (g_CV_EnvMapAll || (pInstance->m_Flags & FLAG_ENVIRONMENTMAP)) &&
		g_CV_EnvMapEnable && g_pStruct->m_bEnvMapEnable;
#endif

	m_fLODDist = g_ViewParams.m_Pos.Dist(m_vPos);
	m_fLODDist /= g_CV_ModelZoomScale.m_fValue;
	SetupLOD();

	m_iCacheState = MCS_NONE;
	if (m_pModel->m_bRigid && g_CV_ModelCacheRigid.m_nValue)
	{
		key = d3d_GetModelCacheKey(m_FogColor, m_iLOD);
		if (!m_bBlendLOD && m_nAlpha == m_pInstance->m_ColorA)
		{
			if (g_ModelVBCache.Find((uint32)m_pInstance, key))
				m_iCacheState = MCS_CACHED;
			else if (g_ModelVBCache.Alloc((uint32)m_pInstance, key, GetNumIndices(), LTFALSE))
				m_iCacheState = MCS_FILL;
		}
	}

	if (g_ModelCacheFrameCode != g_CurFrameCode)
	{
		g_ModelVBCache.AgeCache();
		g_ModelCacheFrameCode = g_CurFrameCode;
	}

	m_bTint = g_CV_TintModels && (pInstance->m_Flags & FLAG_MODELTINT) && pInstance->m_ColorA == 0xff;
	m_bUseTnL = m_iCacheState != MCS_NONE ||
		(g_CV_ModelUseTnL.m_nValue && !(m_pInstance->m_Flags & FLAG_REALLYCLOSE) && !m_pModel->m_bFovScale);

	d3d_SetupTransformation(&m_pInstance->m_Pos, &m_pInstance->m_Rotation,
		&m_pInstance->m_Scale, &m_mTransform);

	if (m_pModel->m_bFovScale)
	{
		LTMatrix mFov;

		if (g_CV_ModelFovTest.m_nValue)
		{
			fFovX = (g_CV_ExtraFOVXOffset.m_fValue + m_pModel->m_fFovX) * g_ViewParams.m_xFov;
			fFovX = LTCLAMP(fFovX, MIN_MODEL_FOV, MAX_MODEL_FOV);
			fFovY = (g_CV_ExtraFOVYOffset.m_fValue + m_pModel->m_fFovY) * g_ViewParams.m_yFov;
		}
		else
		{
			fFovX = MATH_DEGREES_TO_RADIANS(g_CV_ExtraFOVXOffset.m_fValue) + m_pModel->m_fFovX + g_ViewParams.m_xFov;
			fFovX = LTCLAMP(fFovX, MIN_MODEL_FOV, MAX_MODEL_FOV);
			fFovY = MATH_DEGREES_TO_RADIANS(g_CV_ExtraFOVYOffset.m_fValue) + m_pModel->m_fFovY + g_ViewParams.m_yFov;
		}
		fFovY = LTCLAMP(fFovY, MIN_MODEL_FOV, MAX_MODEL_FOV);

		mFov.Init(
			(float)tan(fFovX * 0.5f), 0.0f, 0.0f, 0.0f,
			0.0f, (float)tan(fFovY * 0.5f), 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);

		if (pInstance->m_Flags & FLAG_REALLYCLOSE)
			mProj = mFov * g_ViewParams.m_mReallyCloseFov;
		else
			mProj = mFov * g_ViewParams.m_mFov;

		g_ViewParams.m_mProjection * mProj;
		m_mModelToCamera = mProj * m_mTransform;
	}
	else
	{
		if (pInstance->m_Flags & FLAG_REALLYCLOSE)
			mProj = g_ViewParams.m_mReallyCloseToCamera;
		else
			mProj = g_ViewParams.m_mWorldToCamera;

		switch (m_iCacheState)
		{
			case MCS_FILL:
				m_mModelToCamera = m_mTransform;
				g_ClipFlags = 1;
				break;

			case MCS_CACHED:
				m_mModelToCamera.Identity();
				g_ClipFlags = 1;
				break;

			default:
				if (m_bUseTnL)
				{
					m_mModelToCamera = m_mTransform;
					g_ClipFlags = 1;
				}
				else
				{
					m_mModelToCamera = mProj * m_mTransform;
				}
				break;
		}
	}

	m_mCameraToModel = m_mModelToCamera.MakeInverse();

	if (!AllocVerts())
		return;

	fSaveNearZ = g_ViewParams.m_fNearZ;
	if (m_pInstance->m_Flags & FLAG_REALLYCLOSE)
	{
		g_ClipFlags |= 1;
		g_ViewParams.m_fNearZ = g_CV_ReallyCloseNearZ.m_fValue;
	}

	if (m_iCacheState != MCS_CACHED)
	{
		pTransforms = m_pInstance->GetNodeTransforms();
		for (i = 0; i < m_pModel->m_nNodes; i++)
			m_pModel->m_Transforms[i] = m_mModelToCamera * pTransforms[i];

		if (!g_ClipFlags)
			SetupNodeTransforms(&g_ViewParams.m_mProjection);

		SetupLighting();
	}

	if (m_bEnvMap && m_pModel->m_bSocketLit)
		m_mEnvMap = g_ViewParams.m_mView * m_pModel->m_Transforms[m_pModel->m_iSocketNode] * m_pModel->m_mSocket;
	else
		m_mEnvMap = g_ViewParams.m_mView;

#ifdef PRIMAL_HUNT
	// The environment map's offset circles once every four seconds.
	g_fEnvMapAngle += g_pSceneDesc->m_FrameTime * MATH_HALFPI;
	if (g_fEnvMapAngle > MATH_CIRCLE)
		g_fEnvMapAngle -= MATH_CIRCLE;
	g_fEnvMapOffsetU = (float)sin(g_fEnvMapAngle) * 0.025f;
	g_fEnvMapOffsetV = (float)cos(g_fEnvMapAngle) * 0.025f;
#endif

	if (m_pInstance->m_pAttachments && !(m_pInstance->m_Flags3 & 8))
		g_ClipFlags |= 0x3f;

	m_pfnBeginTransform = g_CV_ModelWarble ? d3d_BeginWarbleTransform : d3d_BeginTransform;
	m_pfnTransform = g_CV_ModelWarble ? d3d_WarbleTransform : d3d_Transform;

	m_InstanceHookData.m_Flags = 0;
	if (m_pInstance->m_InstanceHookFn)
		m_pInstance->m_InstanceHookFn((HLOCALOBJ)m_pInstance, &m_InstanceHookData, 0);

	if (m_InstanceHookData.m_Flags & MIH_CLIPPLANE)
	{
		m_ClipPlane = g_ViewParams.m_mWorldToCamera.TransformPlane(m_InstanceHookData.m_ClipPlane);
		g_ClipFlags |= 1;
	}

	DrawPasses();

	g_ViewParams.m_fNearZ = fSaveNearZ;
	if (g_ClipFlags)
		g_nModelsClipped++;
	else
		g_nModelsUnclipped++;
}

// 0x1000dd18, 0x1000dd19 -- the plain vertex transform.
void d3d_BeginTransform()
{
}

void __fastcall d3d_Transform(void* pVert, TLVert* pOut, LTMatrix* pMat)
{
	pOut->m_RHW = MatVMul_H(&pOut->m_Vec, pMat, &((ModelSrcVert*)pVert)->m_Vec);
}

// 0x1000ddb4, 0x1000ddc5 -- the warbling one: each vertex scaled by the
// next step of the model sun's curve.
void d3d_BeginWarbleTransform()
{
	g_iModelWarble = (uint32)g_fWarbleTime;
}

void __fastcall d3d_WarbleTransform(void* pVert, TLVert* pOut, LTMatrix* pMat)
{
	uint32 i;
	float fScale;
	LTVector vScaled;

	i = g_iModelWarble++ & 31;
	fScale = g_fWarbleFrac * g_fModelSunDelta[i] + g_fModelSun[i];
	VEC_MULSCALAR(vScaled, ((ModelSrcVert*)pVert)->m_Vec, fScale);
	pOut->m_RHW = MatVMul_H(&pOut->m_Vec, pMat, &vScaled);
}

// 0x1000de56 -- the LOD for the model's distance, and whether to blend
// toward the next one.
void CD3DModelDraw::SetupLOD()
{
	float fDist, fBlendDist, fHalf, fToNext;
	float* pNextDist;
	uint32 i, iNext;

	fDist = m_fLODDist;
	m_iLOD = 0;
	for (i = 0; i < m_pModel->m_nLODs + 1; i++)
	{
		if (fDist > *m_pModel->GetLODDist(i))
			m_iLOD = i;
	}

	m_bBlendLOD = LTFALSE;
	if (g_CV_ModelLODBlendEnable.m_nValue)
	{
		iNext = m_iLOD + 1;
		if (iNext < m_pModel->m_nLODs + 1)
		{
			pNextDist = m_pModel->GetLODDist(iNext);
			fBlendDist = g_CV_ModelLODBlendDist.m_fValue;
			fHalf = (*pNextDist - *m_pModel->GetLODDist(m_iLOD)) * 0.5f;
			fBlendDist = LTMIN(fBlendDist, fHalf);
			fToNext = *pNextDist - fDist;
			if (fToNext < fBlendDist)
			{
				m_bBlendLOD = LTTRUE;
				m_fLODBlend = 1.0f - fToNext / fBlendDist;
			}
		}
	}
}

// 0x1000df7a -- each node's transform, projected.
void CD3DModelDraw::SetupNodeTransforms(LTMatrix* pMat)
{
	uint32 i;

	for (i = 0; i < m_pModel->m_nNodes; i++)
		MatMul((LTMatrix*)&m_ClipVerts[i], pMat, &m_pModel->m_Transforms[i]);
}
