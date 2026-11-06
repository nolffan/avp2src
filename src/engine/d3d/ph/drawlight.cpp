// *********************************************************************** //
//  ph/drawlight.cpp -- Primal Hunt's d3d.ren, TU25
//  (0x10024540..0x10024ebb), /O2.
//
//  Dynamic lights and line systems: the OT_LIGHT and OT_LINESYSTEM
//  ProcessObject handlers (0x1004dae8, 0x1004db38), the walk that hangs a
//  light on every world-model polygon it reaches, and the line-system
//  draw.
// *********************************************************************** //

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/struct_bank.h"


struct LTObject;
struct ViewParams;
typedef void (*DrawObjectFn)(ViewParams* pContext, LTObject* pObject);

extern void		AddDebugMessage(int32 level, const char* pMsg, ...);  // 0x10013b13

class BaseObjectSet
{
public:
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

struct VisibleSet
{
	uint8			m_pad_000[0x244];
	BaseObjectSet	m_Lights;                           // +0x244
	uint8			m_pad_260[0x2a4 - 0x260];
	BaseObjectSet	m_LineSystems;                      // +0x2a4
};

class ObjectDrawList
{
public:
	void	Add(LTObject* pObject, DrawObjectFn fn);    // 0x10029980
};

extern ObjectDrawList*	g_pObjectDrawList;              // 0x100713cc
extern ViewParams		g_ViewParams;                 // 0x1005b790
extern int32			g_CV_DynamicLight;              // 0x1004a76c
extern int32			g_CV_DrawLineSystems;           // 0x1004a78c

extern VisibleSet*	d3d_GetVisibleSet();                // 0x1003abb0


// ---------------------------------------------------------------------- //
//  0x10024540 -- the OT_LIGHT ProcessObject handler.
// ---------------------------------------------------------------------- //
void d3d_ProcessLight(LTObject *pObject)
{
	if (g_CV_DynamicLight)
		d3d_GetVisibleSet()->m_Lights.Add(pObject);
}


// ---------------------------------------------------------------------- //
//  The light walk.
// ---------------------------------------------------------------------- //
struct WorldPoly
{
	uint8		m_pad_00[0x18];
	LTVector	m_vCenter;                              // +0x18
	float		m_fRadius;                              // +0x24
	LTPlane*	m_pPlane;                               // +0x28
	void*		m_pSurface;                             // +0x2c
	struct LightLink*	m_pLights;                      // +0x30
	uint8		m_pad_34[0x46 - 0x34];
	uint16		m_FrameCode;                            // +0x46
};

struct Node
{
	WorldPoly*	m_pPoly;                                // +0x00
	Node*		m_Sides[2];                             // +0x04
	uint8		m_pad_0c[0x16 - 0x0c];
	uint8		m_Flags;                                // +0x16
};

class WorldBsp
{
public:
	virtual void	Slot0();
	virtual Node*	GetRootNode();
};

struct LightLink
{
	LightLink*	m_pNext;                                // +0x00
	LTObject*	m_pLight;                               // +0x04
	LTVector	m_vPos;                                 // +0x08
};

struct LitPoly
{
	WorldPoly*	m_pPoly;                                // +0x00
	LitPoly*	m_pNext;                                // +0x04
};

struct LTObject
{
	uint8		m_pad_000[0x88];
	uint32		m_Flags;                                // +0x088
	uint8		m_pad_08c[0xbc - 0x8c];
	uint8		m_BspType;                              // +0x0bc
	uint8		m_pad_0bd[0x194 - 0xbd];
	LTVector	m_Pos;                                  // +0x194
	uint8		m_pad_1a0[0x1b0 - 0x1a0];
	union
	{
		float		m_LightRadius;                      // +0x1b0 (lights)
		WorldBsp*	m_pWorldBsp;                        // +0x1b0 (world models)
	};
	uint8		m_pad_1b4[0x238 - 0x1b4];
	LTMatrix	m_BackTransform;                        // +0x238
};

#define NODE_FLAGS_LEAF		3

extern StructBank	g_LitPolyBank;                      // 0x1005bcd8
extern StructBank	g_LightLinkBank;                    // 0x1005bcb8
extern LitPoly*		g_pLitPolies;                       // 0x1005d24c
extern uint16		g_CurFrameCode;                     // 0x1005d238
extern uint32		g_nLightPolyRejects;                // 0x1005c128

// MatVMul_H, out of line in Primal Hunt's renderer (TU07's 0x1000dcec);
// this SDK's ltmatrix.h has it inline.
extern float	d3d_MatVMul_H(LTVector *pDest, LTMatrix *pMat, LTVector *pSrc);  // 0x1000dcec

// 0x10024590
void d3d_LightWorldModel(LTObject* pLight, LTObject* pWorldModel)
{
	LTVector vLightPos, vPos;
	float fDist, fRadius;
	Node* pNode;
	Node** pStack;
	Node* nodeStack[1024];
	WorldPoly* pPoly;
	LTPlane* pPlane;
	LightLink* pLink;
	LitPoly* pLitPoly;

	vPos = pLight->m_Pos;
	d3d_MatVMul_H(&vLightPos, &pWorldModel->m_BackTransform, &vPos);

	pStack = nodeStack;
	pNode = pWorldModel->m_pWorldBsp->GetRootNode();

	for (;;)
	{
		while (pNode->m_Flags & NODE_FLAGS_LEAF)
		{
			if (pStack == nodeStack)
				return;

			pNode = *--pStack;
		}

		pPoly = pNode->m_pPoly;
		pPlane = (pPoly && pPoly->m_pSurface) ? pPoly->m_pPlane : LTNULL;
		fDist = pPlane->m_Normal.Dot(vLightPos) - pPlane->m_Dist;

		if (fDist < -pLight->m_LightRadius)
		{
			pNode = pNode->m_Sides[0];
			continue;
		}

		if (fDist > pLight->m_LightRadius)
		{
			pNode = pNode->m_Sides[1];
			continue;
		}

		if (pPoly->m_FrameCode == g_CurFrameCode &&
			!((pLight->m_Flags & 0x40) && fDist < 0.01f))
		{
			fRadius = pPoly->m_fRadius + pLight->m_LightRadius;
			if (!((vLightPos - pPoly->m_vCenter).MagSqr() > fRadius * fRadius))
			{
				if (!pPoly->m_pLights)
				{
					pLitPoly = (LitPoly*)sb_Allocate(&g_LitPolyBank);
					if (pLitPoly)
					{
						pLitPoly->m_pPoly = pPoly;
						pLitPoly->m_pNext = g_pLitPolies;
						g_pLitPolies = pLitPoly;
					}
				}
				else
				{
					for (pLink = pPoly->m_pLights; pLink; pLink = pLink->m_pNext)
					{
						if (pLink->m_pLight == pLight)
							goto NextNode;
					}
				}

				pLink = (LightLink*)sb_Allocate(&g_LightLinkBank);
				if (!pLink)
					goto NextNode;

				pLink->m_pLight = pLight;
				pLink->m_vPos = vLightPos;
				pLink->m_pNext = pPoly->m_pLights;
				pPoly->m_pLights = pLink;

				*pStack++ = pNode->m_Sides[1];
				pNode = pNode->m_Sides[0];
				continue;
			}
		}

		g_nLightPolyRejects++;

NextNode:
		*pStack++ = pNode->m_Sides[1];
		pNode = pNode->m_Sides[0];
	}
}


// ---------------------------------------------------------------------- //
//  0x100247e0 -- the world tree's callback: light each world model.
// ---------------------------------------------------------------------- //
void d3d_LightWorldModelCB(LTObject* pWorldModel, LTObject* pLight)
{
	if (pWorldModel->m_BspType == 2)
		d3d_LightWorldModel(pLight, pWorldModel);
}


// ---------------------------------------------------------------------- //
//  0x10024800 -- one visible light: list it for the models, and hang it
//  on the world models its box reaches.
// ---------------------------------------------------------------------- //

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

struct FrameCounters
{
	uint8		m_pad_00[0x18];
	uint32*		m_pLightTicks;                          // +0x18
};

// The world tree's box query.
typedef void (*WTObjCallback)(LTObject *pObj, LTObject *pUser);

class FindBoxInfo
{
public:
				FindBoxInfo()
				{
					m_pTree = LTNULL;
					m_iObjArray = 0;
					m_Min.Init();
					m_Max.Init();
					m_Pos.Init();
					m_CB = LTNULL;
					m_pCBUser = LTNULL;
				}

	void*			m_pTree;                            // +0x00
	uint32			m_iObjArray;                        // +0x04
	LTVector		m_Min;                              // +0x08
	LTVector		m_Max;                              // +0x14
	LTVector		m_Pos;                              // +0x20
	WTObjCallback	m_CB;                               // +0x2c
	LTObject*		m_pCBUser;                          // +0x30
};

class WorldTree
{
public:
	virtual void	Slot00();
	virtual void	Slot04();
	virtual void	FindObjectsInBox(FindBoxInfo *pInfo);   // +0x08
};

class SceneWorld
{
public:
	uint8		m_pad_00[0x6c];
	WorldTree	m_WorldTree;                            // +0x6c
};

#define MAX_VISIBLE_LIGHTS	40
#define FLAG_DONTLIGHTWORLD	0x80

extern FrameCounters*	g_pFrameCounters;               // 0x1005c14c
extern SceneWorld*		g_pSceneWorld;                  // 0x1005c208
extern LTObject*		g_DynamicLights[];              // 0x1005c168
extern uint32			g_nDynamicLights;               // 0x1005bcb0

void d3d_ProcessLightSet(ViewParams* pContext, LTObject* pLight)
{
	FindBoxInfo info;
	CountAdder cntAdd(g_pFrameCounters->m_pLightTicks);

	if (!(pLight->m_Flags & FLAG_ONLYLIGHTWORLD))
	{
		if (g_nDynamicLights < MAX_VISIBLE_LIGHTS)
		{
			g_DynamicLights[g_nDynamicLights] = pLight;
			g_nDynamicLights++;
		}
	}

	if (pLight->m_Flags & FLAG_DONTLIGHTWORLD)
		return;

	if (g_pSceneWorld)
	{
		info.m_Min = pLight->m_Pos - LTVector(pLight->m_LightRadius, pLight->m_LightRadius, pLight->m_LightRadius);
		info.m_Max = pLight->m_Pos + LTVector(pLight->m_LightRadius, pLight->m_LightRadius, pLight->m_LightRadius);
		info.m_CB = d3d_LightWorldModelCB;
		info.m_pCBUser = pLight;
		g_pSceneWorld->m_WorldTree.FindObjectsInBox(&info);
	}
}


// ---------------------------------------------------------------------- //
//  0x10024990 -- light everything the visible lights reach.
// ---------------------------------------------------------------------- //

void d3d_ProcessLights()
{
	if (g_CV_DynamicLight)
		d3d_GetVisibleSet()->m_Lights.Draw(&g_ViewParams, d3d_ProcessLightSet);
}


// ---------------------------------------------------------------------- //
//  0x100249c0 -- the OT_LINESYSTEM ProcessObject handler.
// ---------------------------------------------------------------------- //
void d3d_ProcessLineSystem(LTObject *pObject)
{
	d3d_GetVisibleSet()->m_LineSystems.Add(pObject);
}


static void d3d_QueueLineSystem(ViewParams* pContext, LTObject* pObject);

// ---------------------------------------------------------------------- //
//  0x10024a10
// ---------------------------------------------------------------------- //
void d3d_DrawLineSystems()
{
	if (g_CV_DrawLineSystems)
	{
		BaseObjectSet* pSet = &d3d_GetVisibleSet()->m_LineSystems;
		if (pSet->m_nObjects)
			pSet->Draw(&g_ViewParams, d3d_QueueLineSystem);
	}
}


void d3d_DrawLineSystem(ViewParams* pContext, LTObject* pObject);

// ---------------------------------------------------------------------- //
//  0x10024a40
// ---------------------------------------------------------------------- //
static void d3d_QueueLineSystem(ViewParams* pContext, LTObject* pObject)
{
	g_pObjectDrawList->Add(pObject, d3d_DrawLineSystem);
}


// ---------------------------------------------------------------------- //
//  0x10024a60 -- draw one line system: every line transformed, clipped
//  and projected here, then drawn as a TL line list.
// ---------------------------------------------------------------------- //
struct TLVertex
{
	LTVector	m_Vec;                                  // +0x00
	float		rhw;                                    // +0x0c
	uint8		m_Color[4];                             // +0x10 (b, g, r, a)
	uint32		specular;                               // +0x14
	float		tu, tv;                                 // +0x18
};

struct ViewParams
{
	uint8		m_pad_000[0x15c];
	LTMatrix	m_mWorldToCamera;                      // +0x15c
};

// The engine's rotation, as the transform reads it.
struct Rotation
{
	float		x, y, z, w;
};

struct LSLinePoint
{
	LTVector	m_Pos;
	float		r, g, b, a;
};

struct LSLine
{
	LSLinePoint	m_Points[2];                            // +0x00
	void*		m_pSystem;                              // +0x38
	LSLine		*m_pPrev, *m_pNext;                     // +0x3c
};

struct LineSystem
{
	uint8		m_pad_000[0x97];
	uint8		m_ColorA;                               // +0x097
	uint8		m_pad_098[0x9c - 0x98];
	Rotation	m_Rotation;                             // +0x09c
	LTVector	m_Scale;                                // +0x0ac
	uint8		m_pad_0b8[0x194 - 0xb8];
	LTVector	m_Pos;                                  // +0x194
	uint8		m_pad_1a0[0x1b8 - 0x1a0];
	LSLine		m_LineHead;                             // +0x1b8
};

#define CLIPFLAGS_ALL		0x3f

class RTexture;

extern LPDIRECT3DDEVICE7	g_pD3DDevice;               // 0x100638c8
extern ViewParams			g_ViewParams;               // 0x1005b790
extern int32				g_CV_FogEnable;             // 0x1005df34
extern uint32				g_BindTextureStage;         // 0x100638c0
extern RTexture*			g_pCurTextures[];           // 0x10067270

extern LTBOOL	d3d_ClipLine(TLVertex* pVerts, uint32 clipFlags);          // 0x10016c60
extern void		d3d_ProjectTLVert(TLVertex* pVert, ViewParams* pParams);   // 0x1002739d
extern void		d3d_RotationToMatrix(Rotation* pRot, LTMatrix* pMat);      // 0x1003655d

// Round to nearest in the current FPU mode (LithTech's RoundFloatToInt).
inline int32 RoundFloatToInt(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

// Unbind a stage's texture (TU06's 0x1000a22b, expanded here).
inline void d3d_DisableTexture(uint32 iStage)
{
	if (g_pCurTextures[iStage])
	{
		g_pD3DDevice->SetTexture(iStage, LTNULL);
		g_pCurTextures[iStage] = LTNULL;
	}
}

// TU05's 0x100091f0, expanded here.
inline void d3d_SetupTransformation(LTVector* pPos, Rotation* pRot, LTVector* pScale, LTMatrix* pMat)
{
	if (pRot->x > 100000.0f || pRot->x < -100000.0f)
		pRot->x = 0.0f;

	if (pRot->y > 100000.0f || pRot->y < -100000.0f)
		pRot->y = 0.0f;

	if (pRot->z > 100000.0f || pRot->z < -100000.0f)
		pRot->z = 0.0f;

	if (pRot->w > 100000.0f || pRot->w < -100000.0f)
		pRot->w = 1.0f;

	d3d_RotationToMatrix(pRot, pMat);

	pMat->m[0][0] *= pScale->x;
	pMat->m[1][0] *= pScale->x;
	pMat->m[2][0] *= pScale->x;

	pMat->m[0][1] *= pScale->y;
	pMat->m[1][1] *= pScale->y;
	pMat->m[2][1] *= pScale->y;

	pMat->m[0][2] *= pScale->z;
	pMat->m[1][2] *= pScale->z;
	pMat->m[2][2] *= pScale->z;

	pMat->m[0][3] = pPos->x;
	pMat->m[1][3] = pPos->y;
	pMat->m[2][3] = pPos->z;
}

void d3d_DrawLineSystem(ViewParams* pContext, LTObject* pObject)
{
	LineSystem* pSystem;
	LSLine* pLine;
	TLVertex verts[2];
	LTMatrix mSystem, mFull;
	float fAlphaScale;

	if (g_CV_FogEnable)
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);

	d3d_DisableTexture(g_BindTextureStage);

	pSystem = (LineSystem*)pObject;

	d3d_SetupTransformation(&pSystem->m_Pos, &pSystem->m_Rotation, &pSystem->m_Scale, &mSystem);
	MatMul(&mFull, &g_ViewParams.m_mWorldToCamera, &mSystem);

	fAlphaScale = (float)pSystem->m_ColorA;

	pLine = pSystem->m_LineHead.m_pNext;
	while (pLine != &pSystem->m_LineHead)
	{
		verts[0].rhw = MatVMul_H(&verts[0].m_Vec, &mFull, &pLine->m_Points[0].m_Pos);
		verts[0].m_Color[2] = (uint8)RoundFloatToInt(pLine->m_Points[0].r * 255.0f);
		verts[0].m_Color[1] = (uint8)RoundFloatToInt(pLine->m_Points[0].g * 255.0f);
		verts[0].m_Color[0] = (uint8)RoundFloatToInt(pLine->m_Points[0].b * 255.0f);
		verts[0].m_Color[3] = (uint8)RoundFloatToInt(pLine->m_Points[0].a * fAlphaScale);

		verts[1].rhw = MatVMul_H(&verts[1].m_Vec, &mFull, &pLine->m_Points[1].m_Pos);
		verts[1].m_Color[2] = (uint8)RoundFloatToInt(pLine->m_Points[1].r * 255.0f);
		verts[1].m_Color[1] = (uint8)RoundFloatToInt(pLine->m_Points[1].g * 255.0f);
		verts[1].m_Color[0] = (uint8)RoundFloatToInt(pLine->m_Points[1].b * 255.0f);
		verts[1].m_Color[3] = (uint8)RoundFloatToInt(pLine->m_Points[1].a * fAlphaScale);

		if (d3d_ClipLine(verts, CLIPFLAGS_ALL))
		{
			d3d_ProjectTLVert(&verts[0], &g_ViewParams);
			d3d_ProjectTLVert(&verts[1], &g_ViewParams);
			g_pD3DDevice->DrawPrimitive(D3DPT_LINELIST, D3DFVF_TLVERTEX, verts, 2, 0);
		}

		pLine = pLine->m_pNext;
	}

	if (g_CV_FogEnable)
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, TRUE);
}
