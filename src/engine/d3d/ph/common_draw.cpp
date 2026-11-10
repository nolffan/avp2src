// *********************************************************************** //
//
//  MODULE:   COMMON_DRAW (.CPP) -- Primal Hunt's d3d.ren, TU10
//            (0x10010140..0x100116e3, docs/D3D_OBJECTS.md)
//
//  PURPOSE:  The renderer's shared draw state (g_ViewParams and the frame
//            and view globals every drawing module reads), the frame set-up
//            d3d_InitFrame and the view/frustum helpers.
//            The view set-up d3d_InitSceneContext 0x100104cb calls the
//            ltmatrix.h inlines MatTranspose3x3 and
//            MatVMul_InPlace_Transposed3x3 out of line: their copies are
//            this object's (0x10010f99, 0x10010fc2).
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


// The renderer's console variable (common_stuff.cpp's, 0x10011f36).
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

	void        SetCameraPos(LTVector vPos);                // 0x1000ff40 (d3d_viewparams.cpp)

	ViewBox     m_ViewBox;                                  // +0x00
	int32       m_Left;                                     // +0x1c
	int32       m_Top;                                      // +0x20
	int32       m_Right;                                    // +0x24
	int32       m_Bottom;                                   // +0x28
	float       m_fScreenMinX;                              // +0x2c
	float       m_fScreenMaxX;                              // +0x30 (the last pixel's)
	float       m_fScreenMinY;                              // +0x34
	float       m_fScreenMaxY;                              // +0x38
	int32       m_ClipLeft;                                 // +0x3c
	int32       m_ClipTop;                                  // +0x40
	int32       m_ClipRight;                                // +0x44
	int32       m_ClipBottom;                               // +0x48
	float       m_xFov;                                     // +0x4c
	float       m_yFov;                                     // +0x50
	float       m_fScreenWidth;                             // +0x54
	float       m_fScreenHeight;                            // +0x58
	float       m_fHalfWidth;                               // +0x5c
	float       m_fHalfHeight;                              // +0x60
	float       m_fInvHalfWidth;                            // +0x64
	float       m_fInvHalfHeight;                           // +0x68
	float       m_fCenterX;                                 // +0x6c
	float       m_fCenterY;                                 // +0x70
	float       m_fScaleX;                                  // +0x74
	float       m_fScaleY;                                  // +0x78
	float       m_fInvScaleX;                               // +0x7c
	float       m_fInvScaleY;                               // +0x80
	float       m_fScaleXY;                                 // +0x84
	float       m_NearZ;                                    // +0x88
	float       m_FarZ;                                     // +0x8c
	float       m_FarZ2;                                    // +0x90
	float       m_fFovXScale;                               // +0x94
	float       m_fFovYScale;                               // +0x98
	LTMatrix    m_mInvView;                                 // +0x09c
	LTMatrix    m_mView;                                    // +0x0dc
	LTMatrix    m_mFarProjection;                           // +0x11c
	LTMatrix    m_mWorldToCamera;                           // +0x15c
	LTMatrix    m_DeviceTimesProjection;                    // +0x19c
	LTMatrix    m_mFullTransform;                           // +0x1dc
	LTMatrix    m_mIdentity;                                // +0x21c
	LTMatrix    m_mShear;                                   // +0x25c
	LTMatrix    m_mShearView;                               // +0x29c
	LTMatrix    m_mProjection;                              // +0x2dc
	float       m_fProjX;                                   // +0x31c
	float       m_fProjXZ;                                  // +0x320
	float       m_fProjY;                                   // +0x324
	float       m_fProjYZ;                                  // +0x328
	float       m_fProjZ;                                   // +0x32c
	float       m_fProjZW;                                  // +0x330
	LTVector    m_ViewPoints[5];                            // +0x334 (the eye and the far corners)
	LTPlane     m_CSClipPlanes[6];                          // +0x370
	LTPlane     m_ClipPlanes[6];                            // +0x3d0
	LTPlane     m_ReallyClosePlanes[6];                     // +0x430
	LTVector    m_vUp;                                      // +0x490
	LTVector    m_vRight;                                   // +0x49c
	LTVector    m_vForward;                                 // +0x4a8
	LTVector    m_Pos;                                      // +0x4b4
	LTVector    m_SkyViewPos;                               // +0x4c0
	uint32      m_4cc;                                      // +0x4cc
	float       m_4d0;                                      // +0x4d0
	uint32      m_4d4;                                      // +0x4d4
};

#define MAX_FARZ            100000.0f

#define CPLANE_NEAR_INDEX   0
#define CPLANE_FAR_INDEX    1
#define CPLANE_LEFT_INDEX   2
#define CPLANE_TOP_INDEX    3
#define CPLANE_RIGHT_INDEX  4
#define CPLANE_BOTTOM_INDEX 5
#define NUM_CLIPPLANES      6

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


// 0x1001047e -- the device-times-projection matrix's non-zero terms, for the
// hand-written vertex transforms.
void d3d_SetupProjectionTerms(ViewParams *pContext)
{
	pContext->m_fProjX = pContext->m_DeviceTimesProjection.m[0][0];
	pContext->m_fProjXZ = pContext->m_DeviceTimesProjection.m[0][2];
	pContext->m_fProjY = pContext->m_DeviceTimesProjection.m[1][1];
	pContext->m_fProjYZ = pContext->m_DeviceTimesProjection.m[1][2];
	pContext->m_fProjZ = pContext->m_DeviceTimesProjection.m[2][2];
	pContext->m_fProjZW = pContext->m_DeviceTimesProjection.m[2][3];
}

// Round to nearest in the current FPU mode (LithTech's RoundFloatToInt).
inline int32 RoundFloatToInt(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

void d3d_SetupPerspectiveMatrix(LTMatrix *pMatrix, float nearZ, float farZ);
void d3d_SetupSkyStuff();

extern CD3DConVar g_CV_ReallyCloseNearZ;                    // 0x1005a328 (d3d_viewparams.cpp)

// 0x100104cb -- the view: the screen rectangle, the world-to-camera,
// projection and device transforms, the frustum's far corners and its
// clipping planes in camera and world space.
//
// RETAIL BUG: the projection is built with g_ViewParams' far Z, not the
// view's own: a view set up into another ViewParams (a portal's, the
// sky's) takes the main view's far plane of the frame before.
int32 d3d_InitSceneContext(ViewParams* pParams, ViewBox* pViewBox,
	float screenMinX, float screenMinY, float screenMaxX, float screenMaxY,
	LTMatrix* pMat, LTVector vScale)
{
	LTMatrix mTempWorld, mRotation, mScale;
	LTMatrix mFOVScale, mBackTransform;
	LTMatrix mDevice, mBackTranslate;
	LTMatrix mProjectionTransform, mFarScale, mFarPerspective;
	float leftX, rightX, topY, bottomY, normalZ;
	float xFarZ, yFarZ, fInvFarZ, fNearOverFar;
	LTVector forwardVec, zPlanePos;
	uint32 i;

	pParams->m_4d4 = 0;
	pParams->m_mIdentity.Identity();
	pParams->m_mInvView = *pMat;

	memcpy(&pParams->m_ViewBox, pViewBox, sizeof(pParams->m_ViewBox));
	pMat->GetTranslation(pParams->m_Pos);
	pParams->SetCameraPos(pParams->m_Pos);

	pParams->m_FarZ = pViewBox->m_fFar;
	if (pParams->m_FarZ < 3.0f)
		pParams->m_FarZ = 3.0f;
	if (pParams->m_FarZ > MAX_FARZ)
		pParams->m_FarZ = MAX_FARZ;
	pParams->m_NearZ = pViewBox->m_fNear;
	pParams->m_FarZ2 = pParams->m_FarZ;

	pParams->m_Left = RoundFloatToInt(screenMinX);
	pParams->m_Top = RoundFloatToInt(screenMinY);
	pParams->m_Right = RoundFloatToInt(screenMaxX);
	pParams->m_Bottom = RoundFloatToInt(screenMaxY);

	pParams->m_fScreenMinX = screenMinX;
	pParams->m_fScreenMaxX = screenMaxX - 1.0f;
	pParams->m_fScreenMinY = screenMinY;
	pParams->m_fScreenMaxY = screenMaxY - 1.0f;

	pParams->m_ClipLeft = pParams->m_Left;
	pParams->m_ClipTop = pParams->m_Top;
	pParams->m_ClipRight = pParams->m_Right - 1;
	pParams->m_ClipBottom = pParams->m_Bottom - 1;

	// The rotation and translation into camera space.
	mRotation = *pMat;
	mRotation.SetTranslation(0.0f, 0.0f, 0.0f);
	mRotation.GetBasisVectors(&pParams->m_vRight, &pParams->m_vUp, &pParams->m_vForward);

	MatTranspose3x3(&mRotation);
	mBackTranslate.Init(
		1, 0, 0, -pParams->m_Pos.x,
		0, 1, 0, -pParams->m_Pos.y,
		0, 0, 1, -pParams->m_Pos.z,
		0, 0, 0, 1);
	MatMul(&mTempWorld, &mRotation, &mBackTranslate);

	mScale.Init(
		vScale.x, 0, 0, 0,
		0, vScale.y, 0, 0,
		0, 0, vScale.z, 0,
		0, 0, 0, 1);
	MatMul(&pParams->m_mView, &mScale, &mTempWorld);

	// Shear so the centre of projection is on the z axis.
	pParams->m_mShear.Init(
		1.0f, 0.0f, -pViewBox->m_fOffX / pViewBox->m_fFocal, 0.0f,
		0.0f, 1.0f, -pViewBox->m_fOffY / pViewBox->m_fFocal, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	// The frustum's sides to unit slopes.
	pParams->m_fFovXScale = pViewBox->m_fFocal / pViewBox->m_fHalfW;
	pParams->m_fFovYScale = pViewBox->m_fFocal / pViewBox->m_fHalfH;
	pParams->m_xFov = MATH_PI - (float)atan(pParams->m_fFovXScale) * 2.0f;
	pParams->m_yFov = MATH_PI - (float)atan(pParams->m_fFovYScale) * 2.0f;

	mFOVScale.Init(
		pParams->m_fFovXScale, 0.0f, 0.0f, 0.0f,
		0.0f, pParams->m_fFovYScale, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	pParams->m_mShearView = pParams->m_mShear * pParams->m_mView;
	pParams->m_mWorldToCamera = mFOVScale * pParams->m_mShearView;
	pParams->m_mProjection = mFOVScale * pParams->m_mShear;

	d3d_SetupPerspectiveMatrix(&mProjectionTransform, pViewBox->m_fNear, g_ViewParams.m_FarZ);

	// Projection space to the screen.
	pParams->m_fScreenWidth = screenMaxX - screenMinX;
	pParams->m_fScreenHeight = screenMaxY - screenMinY;
	pParams->m_fHalfWidth = pParams->m_fScreenWidth * 0.5f;
	pParams->m_fHalfHeight = pParams->m_fScreenHeight * 0.5f;
	pParams->m_fInvHalfWidth = 1.0f / pParams->m_fHalfWidth;
	pParams->m_fInvHalfHeight = 1.0f / pParams->m_fHalfHeight;
	pParams->m_fCenterX = pParams->m_fHalfWidth + screenMinX;
	pParams->m_fCenterY = pParams->m_fHalfHeight + screenMinY;
	pParams->m_fScaleX = pParams->m_fHalfWidth * pParams->m_fFovXScale;
	pParams->m_fScaleY = pParams->m_fHalfHeight * pParams->m_fFovYScale;
	pParams->m_fInvScaleX = 1.0f / pParams->m_fScaleX;
	pParams->m_fInvScaleY = 1.0f / pParams->m_fScaleY;
	pParams->m_fScaleXY = pParams->m_fScaleX * pParams->m_fScaleY;

	mDevice.Identity();
	mDevice.m[0][0] = pParams->m_fHalfWidth - 0.0001f;
	mDevice.m[0][3] = pParams->m_fCenterX;
	mDevice.m[1][1] = -(pParams->m_fHalfHeight - 0.0001f);
	mDevice.m[1][3] = pParams->m_fCenterY;

	pParams->m_DeviceTimesProjection = mDevice * mProjectionTransform;
	pParams->m_mFullTransform = pParams->m_DeviceTimesProjection * pParams->m_mWorldToCamera;
	d3d_SetupProjectionTerms(pParams);

	// The projection with the far plane at 1.
	fInvFarZ = 1.0f / pViewBox->m_fFar;
	mFarScale.Init(
		fInvFarZ, 0.0f, 0.0f, 0.0f,
		0.0f, fInvFarZ, 0.0f, 0.0f,
		0.0f, 0.0f, fInvFarZ, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	fNearOverFar = fInvFarZ * pViewBox->m_fNear;
	fInvFarZ = 1.0f - fNearOverFar;
	mFarPerspective.Init(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f / fInvFarZ, -(fNearOverFar / fInvFarZ),
		0.0f, 0.0f, 1.0f, 0.0f);

	pParams->m_mFarProjection = mFarPerspective * mFarScale * mFOVScale * pParams->m_mShear;

	// The frustum's far corners in camera space, then into world space.
	xFarZ = (pParams->m_FarZ * pViewBox->m_fHalfW) / pViewBox->m_fFocal;
	yFarZ = (pParams->m_FarZ * pViewBox->m_fHalfH) / pViewBox->m_fFocal;

	pParams->m_ViewPoints[0].Init(0.0f, 0.0f, 0.0f);
	pParams->m_ViewPoints[1].Init(-xFarZ, yFarZ, pParams->m_FarZ);
	pParams->m_ViewPoints[2].Init(xFarZ, yFarZ, pParams->m_FarZ);
	pParams->m_ViewPoints[3].Init(-xFarZ, -yFarZ, pParams->m_FarZ);
	pParams->m_ViewPoints[4].Init(xFarZ, -yFarZ, pParams->m_FarZ);

	for (i=0; i < 5; i++)
	{
		MatVMul_InPlace_Transposed3x3(&pParams->m_mView, &pParams->m_ViewPoints[i]);
		pParams->m_ViewPoints[i] += pParams->m_Pos;
	}

	// The camera-space clipping planes.
	leftX = pViewBox->m_fOffX - pViewBox->m_fHalfW;
	rightX = pViewBox->m_fOffX + pViewBox->m_fHalfW;
	topY = pViewBox->m_fOffY + pViewBox->m_fHalfH;
	bottomY = pViewBox->m_fOffY - pViewBox->m_fHalfH;
	normalZ = pViewBox->m_fFocal;

	pParams->m_CSClipPlanes[CPLANE_NEAR_INDEX].m_Normal.Init(0.0f, 0.0f, 1.0f);
	pParams->m_CSClipPlanes[CPLANE_NEAR_INDEX].m_Dist = 1.0f;

	pParams->m_CSClipPlanes[CPLANE_FAR_INDEX].m_Normal.Init(0.0f, 0.0f, -1.0f);
	pParams->m_CSClipPlanes[CPLANE_FAR_INDEX].m_Dist = -pParams->m_FarZ;

	pParams->m_CSClipPlanes[CPLANE_LEFT_INDEX].m_Normal.Init(normalZ, 0.0f, -leftX);
	pParams->m_CSClipPlanes[CPLANE_RIGHT_INDEX].m_Normal.Init(-normalZ, 0.0f, rightX);
	pParams->m_CSClipPlanes[CPLANE_TOP_INDEX].m_Normal.Init(0.0f, -normalZ, topY);
	pParams->m_CSClipPlanes[CPLANE_BOTTOM_INDEX].m_Normal.Init(0.0f, normalZ, -bottomY);

	pParams->m_CSClipPlanes[CPLANE_LEFT_INDEX].m_Normal.Norm();
	pParams->m_CSClipPlanes[CPLANE_TOP_INDEX].m_Normal.Norm();
	pParams->m_CSClipPlanes[CPLANE_RIGHT_INDEX].m_Normal.Norm();
	pParams->m_CSClipPlanes[CPLANE_BOTTOM_INDEX].m_Normal.Norm();

	pParams->m_CSClipPlanes[CPLANE_TOP_INDEX].m_Dist = pParams->m_CSClipPlanes[CPLANE_BOTTOM_INDEX].m_Dist = 0.0f;
	pParams->m_CSClipPlanes[CPLANE_LEFT_INDEX].m_Dist = pParams->m_CSClipPlanes[CPLANE_RIGHT_INDEX].m_Dist = 0.0f;

	// The world-space clipping planes.
	mBackTransform = pParams->m_mView;
	MatTranspose3x3(&mBackTransform);
	for (i=0; i < NUM_CLIPPLANES; i++)
	{
		if (i != CPLANE_NEAR_INDEX && i != CPLANE_FAR_INDEX)
		{
			MatVMul_3x3(&pParams->m_ClipPlanes[i].m_Normal, &mBackTransform, &pParams->m_CSClipPlanes[i].m_Normal);
			pParams->m_ClipPlanes[i].m_Dist = pParams->m_ClipPlanes[i].m_Normal.Dot(pParams->m_Pos);
		}
	}

	// The z planes go through the near and far points ahead of the eye.
	forwardVec.Init(mRotation.m[2][0], mRotation.m[2][1], mRotation.m[2][2]);

	zPlanePos = forwardVec * pViewBox->m_fNear;
	zPlanePos += pParams->m_Pos;
	MatVMul_3x3(&pParams->m_ClipPlanes[CPLANE_NEAR_INDEX].m_Normal,
		&mBackTransform, &pParams->m_CSClipPlanes[CPLANE_NEAR_INDEX].m_Normal);
	pParams->m_ClipPlanes[CPLANE_NEAR_INDEX].m_Dist =
		pParams->m_ClipPlanes[CPLANE_NEAR_INDEX].m_Normal.Dot(zPlanePos);

	zPlanePos = forwardVec * pParams->m_FarZ;
	zPlanePos += pParams->m_Pos;
	MatVMul_3x3(&pParams->m_ClipPlanes[CPLANE_FAR_INDEX].m_Normal,
		&mBackTransform, &pParams->m_CSClipPlanes[CPLANE_FAR_INDEX].m_Normal);
	pParams->m_ClipPlanes[CPLANE_FAR_INDEX].m_Dist =
		pParams->m_ClipPlanes[CPLANE_FAR_INDEX].m_Normal.Dot(zPlanePos);

	// The really-close view clips in camera space, its near plane closer.
	memcpy(pParams->m_ReallyClosePlanes, pParams->m_CSClipPlanes, sizeof(pParams->m_ReallyClosePlanes));
	pParams->m_ReallyClosePlanes[CPLANE_NEAR_INDEX].m_Dist = g_CV_ReallyCloseNearZ.m_fValue;

	pParams->m_4cc = 0;
	pParams->m_4d0 = 1.0f;

	d3d_SetupSkyStuff();
	return LTTRUE;
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
