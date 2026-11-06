// *********************************************************************** //
//
//  MODULE:   COMMON_DRAW (.CPP) -- Primal Hunt's d3d.ren, TU10
//            (0x10010140..0x100116e3, docs/D3D_OBJECTS.md)
//
//  PURPOSE:  The renderer's shared draw state (the frame and view globals
//            every drawing module reads) and the view/frustum set-up.
//            Not written: the view set-up d3d_InitSceneContext 0x100104cb
//            (2766 bytes) with the ltmatrix.h COMDATs it calls out of line
//            (0x10010f99 MatTranspose3x3, 0x10010fc2
//            MatVMul_InPlace_Transposed3x3), and d3d_InitFrame 0x10011162.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>
#include "../../LT2/lithshared/stdlith/struct_bank.h"


// Five globals of classes whose constructors do nothing (their
// initialisers are a bare `ret`, 0x10010140..0x10010144).
struct CEmptyInit
{
	CEmptyInit() {}
};

CEmptyInit g_10010140;
CEmptyInit g_10010141;
CEmptyInit g_10010142;
CEmptyInit g_10010143;
CEmptyInit g_10010144;


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
struct SceneContext
{
	SceneContext();                                         // 0x1001014f

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
	uint8       m_pad_334[0x4b4 - 0x334];
	LTVector    m_Pos;                                      // +0x4b4
	LTVector    m_SkyViewPos;                               // +0x4c0
};

SceneContext g_SceneContext;                                // 0x1005b790

SceneContext::SceneContext()
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
	uint8       m_Reserved00[0x0c];
	uint16      m_FrameCode;                                // +0x0c
};

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

extern LTPlane  g_ViewPlanes[6];                            // 0x1005bb60

// 0x100101f8 -- is any of the node's box in front of every view plane?
int d3d_NodeFilter(VisNode *pNode)
{
	LTVector vCorners[8];
	LTPlane *pPlane;
	int i;

	vCorners[0].Init(pNode->m_vCenter.x + pNode->m_vHalfDims.x, pNode->m_vCenter.y + pNode->m_vHalfDims.y, pNode->m_vCenter.z + pNode->m_vHalfDims.z);
	vCorners[1].Init(pNode->m_vCenter.x + pNode->m_vHalfDims.x, pNode->m_vCenter.y - pNode->m_vHalfDims.y, pNode->m_vCenter.z + pNode->m_vHalfDims.z);
	vCorners[2].Init(pNode->m_vCenter.x - pNode->m_vHalfDims.x, pNode->m_vCenter.y + pNode->m_vHalfDims.y, pNode->m_vCenter.z + pNode->m_vHalfDims.z);
	vCorners[3].Init(pNode->m_vCenter.x - pNode->m_vHalfDims.x, pNode->m_vCenter.y - pNode->m_vHalfDims.y, pNode->m_vCenter.z + pNode->m_vHalfDims.z);
	vCorners[4].Init(pNode->m_vCenter.x + pNode->m_vHalfDims.x, pNode->m_vCenter.y + pNode->m_vHalfDims.y, pNode->m_vCenter.z - pNode->m_vHalfDims.z);
	vCorners[5].Init(pNode->m_vCenter.x + pNode->m_vHalfDims.x, pNode->m_vCenter.y - pNode->m_vHalfDims.y, pNode->m_vCenter.z - pNode->m_vHalfDims.z);
	vCorners[6].Init(pNode->m_vCenter.x - pNode->m_vHalfDims.x, pNode->m_vCenter.y + pNode->m_vHalfDims.y, pNode->m_vCenter.z - pNode->m_vHalfDims.z);
	vCorners[7].Init(pNode->m_vCenter.x - pNode->m_vHalfDims.x, pNode->m_vCenter.y - pNode->m_vHalfDims.y, pNode->m_vCenter.z - pNode->m_vHalfDims.z);

	for (pPlane = g_ViewPlanes; pPlane < &g_ViewPlanes[6]; pPlane++)
	{
		for (i = 0; i < 8; i++)
		{
			if (pPlane->DistTo(vCorners[i]) > 0.0f)
				break;
		}

		if (i >= 8)
			return 0;
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
void d3d_InitViewBox(ViewBox *pBox, float fNearZ, float fFarZ, SceneContext *pPrev,
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
void d3d_SetupProjectionTerms(SceneContext *pContext)
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
	uint8       m_pad_04[0x84 - 0x04];
	SkyDef      m_SkyDef;                                   // +0x84 (its view box +0x9c)
};

#define DRAWMODE_NORMAL     1

extern SceneDesc*   g_pSceneDesc;                           // 0x1005c14c

// 0x10011059 -- where the sky is seen from: the camera's place in the
// world's extents, mapped into the sky's.
void d3d_SetupSkyStuff()
{
	LTVector percents, v;

	if (g_pSceneDesc->m_DrawMode == DRAWMODE_NORMAL)
	{
		percents = (g_SceneContext.m_Pos - g_pSceneWorld->m_ExtentsMin) /
			(g_pSceneWorld->m_ExtentsMax - g_pSceneWorld->m_ExtentsMin);
	}
	else
	{
		percents.Init(0.5f, 0.5f, 0.5f);
	}

	const SkyDef& Def = g_pSceneDesc->m_SkyDef;

	v = Def.m_ViewMax - Def.m_ViewMin;
	g_SceneContext.m_SkyViewPos = Def.m_ViewMin + v * percents;
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
