// *********************************************************************** //
//
//  MODULE:   COMMON_DRAW (.CPP) -- Primal Hunt's d3d.ren, TU10
//            (0x10010140..0x100116e3, docs/D3D_OBJECTS.md)
//
//  PURPOSE:  The renderer's shared draw state (g_ViewParams and the frame
//            and view globals every drawing module reads), the frame set-up
//            d3d_InitFrame and the view/frustum helpers.
//            Not written: the view set-up d3d_InitSceneContext 0x100104cb
//            (2766 bytes) with the ltmatrix.h COMDATs it calls out of line
//            (0x10010f99 MatTranspose3x3, 0x10010fc2
//            MatVMul_InPlace_Transposed3x3).
//            Near: d3d_InitFrame 0x10011162 -- every instruction agrees,
//            but the frame's locals sit elsewhere (retail: the view box
//            lowest at -0x88, the camera matrix at -0x6c, the four rect
//            floats at -0x2c..-0x20; cl here puts the matrix lowest and the
//            box at -0x2c, whatever the declaration order).
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>
#include "../../LT2/lithshared/stdlith/struct_bank.h"


// The frame's colours, set by d3d_InitFrame: their (empty) constructors
// are the bare `ret` initialisers 0x10010140..0x10010144.
LTVector        g_vLightAdd;                                // 0x1005bc90
LTVector        g_vLightAdd255;                             // 0x1005bca0
LTVector        g_vFogColor;                                // 0x1005bcf8
LTVector        g_1005c138;                                 // 0x1005c138
LTVector        g_vLightAddScale;                           // 0x1005c158


// The view box: the centre of projection and the view window around it.
struct ViewBox
{
	float       m_fOffX;                                    // +0x00 (the COP)
	float       m_fOffY;                                    // +0x04
	float       m_fFocal;                                   // +0x08
	float       m_fHalfW;                                   // +0x0c (the window)
	float       m_fHalfH;                                   // +0x10
	float       m_fNear;                                    // +0x14
	float       m_fFar;                                     // +0x18
};

// The view state shared by the drawing modules (the engine's ViewParams).
struct ViewParams
{
	ViewParams();                                         // 0x1001014f

	ViewBox     m_ViewBox;                                  // +0x00
	int32       m_Left;                                     // +0x1c
	int32       m_Top;                                      // +0x20
	int32       m_Right;                                    // +0x24
	int32       m_Bottom;                                   // +0x28
	uint8       m_pad_2c[0x19c - 0x2c];
	LTMatrix    m_mFullTransform;                           // +0x19c
	uint8       m_pad_1dc[0x31c - 0x1dc];
	float       m_fProjX;                                   // +0x31c
	float       m_fProjXZ;                                  // +0x320
	float       m_fProjY;                                   // +0x324
	float       m_fProjYZ;                                  // +0x328
	float       m_fProjZ;                                   // +0x32c
	float       m_fProjZW;                                  // +0x330
	uint8       m_pad_334[0x3d0 - 0x334];
	LTPlane     m_ClipPlanes[6];                            // +0x3d0
	LTPlane     m_ReallyClosePlanes[6];                     // +0x430
	uint8       m_pad_490[0x4b4 - 0x490];
	LTVector    m_Pos;                                      // +0x4b4
	LTVector    m_SkyViewPos;                               // +0x4c0
};

ViewParams g_ViewParams;                                // 0x1005b790

ViewParams::ViewParams()
{
	m_Left = 0;
	m_Top = 0;
	m_Right = 0;
	m_Bottom = 0;
}


// The world's blocks, as the frame-code reset walks them.
struct RenderPoly
{
	uint8       m_Reserved00[0x2c];
	uint16      m_FrameCode;                                // +0x2c
	uint16      m_Pad2e;
};

struct RenderLeaf
{
	uint8       m_Reserved00[0x46];
	uint16      m_FrameCode;                                // +0x46
};

struct RenderBlock
{
	uint8       m_Reserved00[0x88];
	RenderPoly* m_Polies;                                   // +0x88
	uint32      m_nPolies;                                  // +0x8c
	uint8       m_Reserved90[0xa0 - 0x90];
	RenderLeaf** m_Leaves;                                  // +0xa0
	uint32      m_nLeaves;                                  // +0xa4
};

struct WorldBsp
{
	uint8       m_Reserved00[0x04];
	RenderBlock* m_pBlock;                                  // +0x04
};

struct SceneWorld
{
	uint8       m_Reserved00[0x144];
	LTVector    m_ExtentsMin;                               // +0x144
	LTVector    m_ExtentsMax;                               // +0x150
	uint8       m_Reserved15c[0x18c - 0x15c];
	WorldBsp**  m_WorldModels;                              // +0x18c
	uint32      m_nWorldModels;                             // +0x190
};

struct RenderWorld
{
	uint8       m_Reserved00[0x08];
	SceneWorld* m_pSceneWorld;                              // +0x08
	uint16      m_FrameCode;                                // +0x0c
};

class LTObject;
struct SceneDesc;

uint32      g_nDynamicLights;                               // 0x1005bcb0
uint32      g_ClipFlags;                                    // 0x1005bd0c
void*       g_pClipScratch;                                 // 0x1005bd14
uint32      g_nClipPasses;                                  // 0x1005c124
SceneDesc*  g_pSceneDesc;                                   // 0x1005c14c
uint32      g_WorldColor;                                   // 0x1005c154
uint16      g_CurFrameCode;                                 // 0x1005d238
LTObject*   g_DynamicLights[40];                            // 0x1005c168
SceneWorld* g_pSceneWorld;                                  // 0x1005c208

void d3d_ClearBlockFrameCodes(RenderBlock* pBlock);

// 0x10010160 -- next frame code; when it wraps, clear every block's.
void d3d_IncFrameCode(RenderWorld* pWorld)
{
	uint32 i;

	if (pWorld->m_FrameCode == 0xFFFF)
	{
		pWorld->m_FrameCode = 1;

		for (i = 0; i < g_pSceneWorld->m_nWorldModels; i++)
		{
			d3d_ClearBlockFrameCodes(g_pSceneWorld->m_WorldModels[i]->m_pBlock);
		}
	}

	pWorld->m_FrameCode++;
}

// 0x100101aa
void d3d_ClearBlockFrameCodes(RenderBlock* pBlock)
{
	uint32 i;

	for (i = 0; i < pBlock->m_nPolies; i++)
		pBlock->m_Polies[i].m_FrameCode = 0;

	for (i = 0; i < pBlock->m_nLeaves; i++)
		pBlock->m_Leaves[i]->m_FrameCode = 0;
}


// A visibility node's box.
struct VisNode
{
	uint8       m_pad_00[0x0c];
	LTVector    m_vCenter;                                  // +0x0c
	LTVector    m_vHalfDims;                                // +0x18
};


// 0x100101f8 -- is any of the node's box in front of every view plane?
int d3d_NodeFilter(VisNode *pNode)
{
	LTVector vCorners[8];
	LTPlane *pPlane;
	int iPlane, i;

	vCorners[0].Init(pNode->m_vCenter.x + pNode->m_vHalfDims.x, pNode->m_vCenter.y + pNode->m_vHalfDims.y, pNode->m_vCenter.z + pNode->m_vHalfDims.z);
	vCorners[1].Init(pNode->m_vCenter.x + pNode->m_vHalfDims.x, pNode->m_vCenter.y - pNode->m_vHalfDims.y, pNode->m_vCenter.z + pNode->m_vHalfDims.z);
	vCorners[2].Init(pNode->m_vCenter.x - pNode->m_vHalfDims.x, pNode->m_vCenter.y + pNode->m_vHalfDims.y, pNode->m_vCenter.z + pNode->m_vHalfDims.z);
	vCorners[3].Init(pNode->m_vCenter.x - pNode->m_vHalfDims.x, pNode->m_vCenter.y - pNode->m_vHalfDims.y, pNode->m_vCenter.z + pNode->m_vHalfDims.z);
	vCorners[4].Init(pNode->m_vCenter.x + pNode->m_vHalfDims.x, pNode->m_vCenter.y + pNode->m_vHalfDims.y, pNode->m_vCenter.z - pNode->m_vHalfDims.z);
	vCorners[5].Init(pNode->m_vCenter.x + pNode->m_vHalfDims.x, pNode->m_vCenter.y - pNode->m_vHalfDims.y, pNode->m_vCenter.z - pNode->m_vHalfDims.z);
	vCorners[6].Init(pNode->m_vCenter.x - pNode->m_vHalfDims.x, pNode->m_vCenter.y + pNode->m_vHalfDims.y, pNode->m_vCenter.z - pNode->m_vHalfDims.z);
	vCorners[7].Init(pNode->m_vCenter.x - pNode->m_vHalfDims.x, pNode->m_vCenter.y - pNode->m_vHalfDims.y, pNode->m_vCenter.z - pNode->m_vHalfDims.z);

	for (iPlane = 0; iPlane < 6; iPlane++)
	{
		pPlane = &g_ViewParams.m_ClipPlanes[iPlane];
		for (i = 0; i < 8; i++)
		{
			if (pPlane->DistTo(vCorners[i]) > 0.0f)
				goto NextPlane;
		}

		return 0;

	NextPlane:;
	}

	return 1;
}

// 0x100102f1 -- a view box from the fields of view.
void d3d_InitViewBox(ViewBox *pBox, float fNearZ, float fFarZ, float xFov, float yFov)
{
	float xTemp, yTemp;

	pBox->m_fNear = fNearZ;
	pBox->m_fFar = fFarZ;

	xTemp = MATH_HALFPI - (xFov * 0.5f);
	yTemp = MATH_HALFPI - (yFov * 0.5f);

	pBox->m_fHalfW = (float)(cos(xTemp) / sin(xTemp));
	pBox->m_fHalfH = (float)(cos(yTemp) / sin(yTemp));

	pBox->m_fOffX = 0.0f;
	pBox->m_fOffY = 0.0f;
	pBox->m_fFocal = 1.0f;
}

// 0x10010383 -- a view box for a rectangle of a previous view's screen.
void d3d_InitViewBox(ViewBox *pBox, float fNearZ, float fFarZ, ViewParams *pPrev,
	float fMinX, float fMinY, float fMaxX, float fMaxY)
{
	float xMinPrev, yMinPrev, xMaxPrev, yMaxPrev;
	float xMin, yMin, xMax, yMax;

	pBox->m_fNear = fNearZ;
	pBox->m_fFar = fFarZ;

	xMinPrev = pPrev->m_ViewBox.m_fOffX - pPrev->m_ViewBox.m_fHalfW;
	yMinPrev = -pPrev->m_ViewBox.m_fOffY - pPrev->m_ViewBox.m_fHalfH;
	xMaxPrev = pPrev->m_ViewBox.m_fOffX + pPrev->m_ViewBox.m_fHalfW;
	yMaxPrev = -pPrev->m_ViewBox.m_fOffY + pPrev->m_ViewBox.m_fHalfH;

	xMin = (fMinX - (float)pPrev->m_Left) / (float)(pPrev->m_Right - pPrev->m_Left);
	yMin = (fMinY - (float)pPrev->m_Top) / (float)(pPrev->m_Bottom - pPrev->m_Top);
	xMax = (fMaxX - (float)pPrev->m_Left) / (float)(pPrev->m_Right - pPrev->m_Left);
	yMax = (fMaxY - (float)pPrev->m_Top) / (float)(pPrev->m_Bottom - pPrev->m_Top);

	xMin = LTLERP(xMinPrev, xMaxPrev, xMin);
	yMin = -LTLERP(yMinPrev, yMaxPrev, yMin);
	xMax = LTLERP(xMinPrev, xMaxPrev, xMax);
	yMax = -LTLERP(yMinPrev, yMaxPrev, yMax);

	pBox->m_fFocal = 1.0f;
	pBox->m_fOffX = (xMin + xMax) * 0.5f;
	pBox->m_fOffY = (yMin + yMax) * 0.5f;
	pBox->m_fHalfW = (xMax - xMin) * 0.5f;
	pBox->m_fHalfH = -(yMax - yMin) * 0.5f;
}


// 0x1001047e -- the full transform's non-zero projection terms, for the
// hand-written vertex transforms.
void d3d_SetupProjectionTerms(ViewParams *pContext)
{
	pContext->m_fProjX = pContext->m_mFullTransform.m[0][0];
	pContext->m_fProjXZ = pContext->m_mFullTransform.m[0][2];
	pContext->m_fProjY = pContext->m_mFullTransform.m[1][1];
	pContext->m_fProjYZ = pContext->m_mFullTransform.m[1][2];
	pContext->m_fProjZ = pContext->m_mFullTransform.m[2][2];
	pContext->m_fProjZW = pContext->m_mFullTransform.m[2][3];
}

// 0x10011021 -- a perspective projection with the given clip depths.
void d3d_SetupPerspectiveMatrix(LTMatrix *pMatrix, float nearZ, float farZ)
{
	pMatrix->Identity();
	pMatrix->m[2][2] = farZ / (farZ - nearZ);
	pMatrix->m[2][3] = (-farZ * nearZ) / (farZ - nearZ);
	pMatrix->m[3][2] = 1.0f;
	pMatrix->m[3][3] = 0.0f;
}


// The frame's scene, as the game describes it.
struct SceneDesc
{
	int32       m_DrawMode;                                 // +0x00
	uint8       m_pad_04[0x34 - 0x04];
	RenderWorld* m_pRenderWorld;                            // +0x34
	LTVector    m_v38;                                      // +0x38
	LTVector    m_v44;                                      // +0x44
	LTVector    m_GlobalLightAdd;                           // +0x50
	LTVector    m_GlobalLightColor;                         // +0x5c
	LTVector    m_FogColor;                                 // +0x68
	uint8       m_pad_74[0x84 - 0x74];
	SkyDef      m_SkyDef;                                   // +0x84 (its view box +0x9c)
	uint8       m_pad_b4[0xbc - 0xb4];
	LTRect      m_Rect;                                     // +0xbc
	float       m_xFov;                                     // +0xcc
	float       m_yFov;                                     // +0xd0
	float       m_FarZ;                                     // +0xd4
	LTVector    m_Pos;                                      // +0xd8
	LTRotation  m_Rotation;                                 // +0xe4
};

#define DRAWMODE_NORMAL     1


// 0x10011059 -- where the sky is seen from: the camera's place in the
// world's extents, mapped into the sky's.
void d3d_SetupSkyStuff()
{
	LTVector percents;

	if (g_pSceneDesc->m_DrawMode == DRAWMODE_NORMAL)
	{
		percents = (g_ViewParams.m_Pos - g_pSceneWorld->m_ExtentsMin) /
			(g_pSceneWorld->m_ExtentsMax - g_pSceneWorld->m_ExtentsMin);
	}
	else
	{
		percents.Init(0.5f, 0.5f, 0.5f);
	}

	const SkyDef& Def = g_pSceneDesc->m_SkyDef;

	g_ViewParams.m_SkyViewPos = Def.m_ViewMin + (Def.m_ViewMax - Def.m_ViewMin) * percents;
}



// ---------------------------------------------------------------------- //
//  The frame's set-up.
// ---------------------------------------------------------------------- //

// The engine's render structure, as the frame set-up reads it.
struct RenderContext;

struct RenderStruct
{
	uint8       m_pad_000[0x28];
	uint32      (*m_GetTime)();                             // +0x28
	uint8       m_pad_02c[0x30 - 0x2c];
	uint32      (*m_GetTextureFrameCode)();                 // +0x30
	uint8       m_pad_034[0x110 - 0x34];
	uint8       m_Context[4];                               // +0x110
};

extern RenderStruct*    g_pStruct;                          // 0x1005df08 (common_stuff.cpp)
extern uint32           g_1005d5f0;                         // 0x1005d5f0: the frames begun

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

extern CD3DConVar       g_CV_NearZ;                         // 0x1005b748 (d3d_viewparams.cpp)

// A colour's bytes.
struct ColorBytes
{
	uint8       b, g, r, a;
};

#define COLORBYTES(c)   (*(ColorBytes*)&(c))

uint32          g_1005bcf4;                                 // 0x1005bcf4
RenderContext*  g_pRenderContext;                           // 0x1005b778
RenderWorld*    g_pRenderWorld;                             // 0x1005bd1c
uint32          g_nFrameTime;                               // 0x1005bc88
uint16          g_CurTextureFrameCode;                      // 0x1005d250
extern LTVector g_1005d240;                                 // 0x1005d240 (common_init.cpp's range)
uint32          g_LightAddColor;                            // 0x1005c130
uint8           g_VertexAlpha;                              // 0x1005d20c
extern LTVector g_vGlobalLightColor;                        // 0x1005b780 (d3d_viewparams.cpp's range)
uint32          g_FogColorRGB;                              // 0x1005d230
float           g_fFillArea;                                // 0x1005c12c
uint32          g_nWorldPoliesDrawn;                        // 0x1005c120
uint32          g_nDrawPrimCalls;                           // 0x1005c144
uint32          g_1005b770;                                 // 0x1005b770
int32           g_nModelTriangles;                          // 0x1005bd04
uint32          g_1005d234;                                 // 0x1005d234
uint32          g_nSkyLightsApplied;                        // 0x1005c164
uint32          g_nLightPolyRejects;                        // 0x1005c128
uint32          g_nSkyPortalPoliesDrawn;                    // 0x1005c150
uint32          g_1005bd08;                                 // 0x1005bd08
uint32          g_nTrianglesDrawn;                          // 0x1005c148
uint32          g_nVisibleLeaves;                           // 0x1005b78c
uint32          g_1005bd18;                                 // 0x1005bd18
uint32          g_nTextureChanges;                          // 0x1005d22c
uint32          g_1005bd10;                                 // 0x1005bd10
uint32          g_1005b774;                                 // 0x1005b774

// Round to nearest in the current FPU mode (LithTech's RoundFloatToInt).
inline int32 RoundFloatToInt(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

void    d3d_ReadConsoleVariables();                         // 0x10013bcd (common_stuff.cpp)
void    d3d_RotationToMatrix(LTRotation* pRot, LTMatrix* pMat); // 0x1003655d
int32   d3d_InitSceneContext(ViewParams* pParams, ViewBox* pBox,
	float fMinX, float fMinY, float fMaxX, float fMaxY,
	LTMatrix* pCamera, LTVector vScale);                    // 0x100104cb

// 0x10011162 -- the frame's state from the scene: the world, the global
// light and fog colours, the counters cleared, and the view.
int32 d3d_InitFrame(SceneDesc* pDesc, void* pClipScratch, uint32 param3)
{
	ViewBox box;
	LTMatrix mCamera;
	LTVector vPos;
	RenderWorld* pWorld;
	float fMinX, fMinY, fMaxX, fMaxY;
	float fNearZ, fFarZ, xFov, yFov;
	short cw;

	// Single precision.
	__asm
	{
		fstcw   cw
		and     cw, 0xFCFF
		fldcw   cw
	}

	d3d_ReadConsoleVariables();

	g_1005d5f0++;
	g_pClipScratch = pClipScratch;
	g_pSceneDesc = pDesc;
	g_1005bcf4 = param3;
	g_pRenderContext = (RenderContext*)g_pStruct->m_Context;

	pWorld = pDesc->m_pRenderWorld;
	if (pDesc->m_DrawMode == DRAWMODE_NORMAL)
	{
		if (!pWorld)
			return LTFALSE;

		g_pSceneWorld = pWorld->m_pSceneWorld;
		g_pRenderWorld = pWorld;
		d3d_IncFrameCode(pWorld);
		g_CurFrameCode = pWorld->m_FrameCode;
	}
	else
	{
		g_pRenderWorld = LTNULL;
		g_pSceneWorld = LTNULL;
	}

	g_nFrameTime = g_pStruct->m_GetTime();
	g_CurTextureFrameCode = (uint16)g_pStruct->m_GetTextureFrameCode();
	g_ClipFlags = 0x3F;

	g_1005d240 = pDesc->m_v38;
	g_1005c138 = pDesc->m_v44;

	g_vLightAdd = pDesc->m_GlobalLightAdd;
	g_vLightAdd255 = g_vLightAdd * 255.0f;
	g_vLightAddScale = LTVector(g_vLightAdd.x, g_vLightAdd.y, g_vLightAdd.z) / 255.0f;
	COLORBYTES(g_LightAddColor).r = (uint8)RoundFloatToInt(g_vLightAdd.x * 255.0f);
	COLORBYTES(g_LightAddColor).g = (uint8)RoundFloatToInt(g_vLightAdd.y * 255.0f);
	COLORBYTES(g_LightAddColor).b = (uint8)RoundFloatToInt(g_vLightAdd.z * 255.0f);
	COLORBYTES(g_LightAddColor).a = 0xFF;
	g_VertexAlpha = 0xFF;

	g_vGlobalLightColor = pDesc->m_GlobalLightColor;
	COLORBYTES(g_WorldColor).r = (uint8)RoundFloatToInt(g_vGlobalLightColor.x * 255.0f);
	COLORBYTES(g_WorldColor).g = (uint8)RoundFloatToInt(g_vGlobalLightColor.y * 255.0f);
	COLORBYTES(g_WorldColor).b = (uint8)RoundFloatToInt(g_vGlobalLightColor.z * 255.0f);
	COLORBYTES(g_WorldColor).a = 0xFF;

	g_vFogColor = pDesc->m_FogColor;
	COLORBYTES(g_FogColorRGB).r = (uint8)RoundFloatToInt(g_vFogColor.x);
	COLORBYTES(g_FogColorRGB).g = (uint8)RoundFloatToInt(g_vFogColor.y);
	COLORBYTES(g_FogColorRGB).b = (uint8)RoundFloatToInt(g_vFogColor.z);
	COLORBYTES(g_FogColorRGB).a = 0xFF;

	g_fFillArea = 0.0f;
	g_nWorldPoliesDrawn = 0;
	g_nDrawPrimCalls = 0;
	g_1005b770 = 0;
	g_nModelTriangles = 0;
	g_1005d234 = 0;
	g_nSkyLightsApplied = 0;
	g_nClipPasses = 0;
	g_nLightPolyRejects = 0;
	g_nSkyPortalPoliesDrawn = 0;
	g_1005bd08 = 0;
	g_nDynamicLights = 0;
	g_nTrianglesDrawn = 0;
	g_nVisibleLeaves = 0;
	g_1005bd18 = 0;
	g_nTextureChanges = 0;
	g_1005bd10 = 0;
	g_1005b774 = 0;

	// The view.
	fFarZ = pDesc->m_FarZ;
	fNearZ = g_CV_NearZ.m_fValue;
	yFov = pDesc->m_yFov;
	xFov = pDesc->m_xFov;
	fMaxY = (float)pDesc->m_Rect.bottom - 0.1f;
	fMaxX = (float)pDesc->m_Rect.right - 0.1f;
	fMinY = (float)pDesc->m_Rect.top;
	fMinX = (float)pDesc->m_Rect.left;
	d3d_RotationToMatrix(&pDesc->m_Rotation, &mCamera);

	vPos = pDesc->m_Pos;
	mCamera.m[0][3] = vPos.x;
	mCamera.m[1][3] = vPos.y;
	mCamera.m[2][3] = vPos.z;

	d3d_InitViewBox(&box, fNearZ, fFarZ, xFov, yFov);

	return d3d_InitSceneContext(&g_ViewParams, &box, fMinX, fMinY, fMaxX, fMaxY,
		&mCamera, LTVector(1.0f, 1.0f, 1.0f));
}


// ---------------------------------------------------------------------- //
//  The dynamic lights touching the frame's objects.
// ---------------------------------------------------------------------- //

// A link from an object to a light touching it.
struct LightLink
{
	LightLink*  m_pNext;                                    // +0x00
	uint8       m_Reserved04[0x10];
};

struct LitObject
{
	uint8       m_Reserved00[0x30];
	LightLink*  m_pLightLinks;                              // +0x30
};

// The frame's list of lit objects.
struct LitObjectRef
{
	LitObject*      m_pObject;                              // +0x00
	LitObjectRef*   m_pNext;                                // +0x04
};

StructBank      g_LightLinkBank;                            // 0x1005bcb8
StructBank      g_LitObjectBank;                            // 0x1005bcd8
StructBank      g_WorldPolyBank;                            // 0x1005d210
LitObjectRef*   g_pLitObjects;                              // 0x1005d24c
uint32          g_nLitObjects;                              // 0x1005bcac

void d3d_FreeLightLinks(LitObjectRef* pRef);

// 0x10011517
void d3d_FreeLitObjects()
{
	LitObjectRef *pCur, *pNext;

	g_nLitObjects = 0;
	pCur = g_pLitObjects;
	while (pCur)
	{
		g_nLitObjects++;
		d3d_FreeLightLinks(pCur);
		pNext = pCur->m_pNext;
		sb_Free(&g_LitObjectBank, pCur);
		pCur = pNext;
	}

	g_pLitObjects = LTNULL;
}

// 0x10011563
void d3d_FreeLightLinks(LitObjectRef* pRef)
{
	LightLink *pCur, *pNext;

	pCur = pRef->m_pObject->m_pLightLinks;
	while (pCur)
	{
		pNext = pCur->m_pNext;
		sb_Free(&g_LightLinkBank, pCur);
		pCur = pNext;
	}

	pRef->m_pObject->m_pLightLinks = LTNULL;
}

// 0x1001159b -- at the start of a frame.
void d3d_InitFrameState()
{
	d3d_FreeLitObjects();
}

// 0x100115a0
void d3d_InitWorldBanks()
{
	sb_Init2(&g_LightLinkBank, sizeof(LightLink), 0x40, 0x40);
	sb_Init2(&g_LitObjectBank, sizeof(LitObjectRef), 0x40, 0x40);
	sb_Init2(&g_WorldPolyBank, 0xc, 0x200, 0x800);
}

// 0x100115da
void d3d_TermWorldBanks()
{
	sb_Term(&g_LightLinkBank);
	sb_Term(&g_LitObjectBank);
	sb_Term(&g_WorldPolyBank);
}

// 0x100115fe -- environment map coordinates: the eye's reflection off the
// surface, its x (or z, for walls facing along x) and y mapped to 0..1.
void d3d_CalcEnvMapUV(const LTVector* pEye, const LTVector* pPos,
	const LTVector* pNormal, float* pU, float* pV)
{
	LTVector vI = *pPos - *pEye;
	LTVector vR = vI - *pNormal * (vI.Dot(*pNormal) * 2.0f);
	vR.Norm();

	if (pNormal->y * pNormal->y > 0.54439f ||
		pNormal->x * pNormal->x < pNormal->z * pNormal->z)
		*pU = 0.5f - vR.x * 0.5f;
	else
		*pU = 0.5f - vR.z * 0.5f;

	*pV = 0.5f - vR.y * 0.5f;
}
