// *********************************************************************** //
//  ph/drawlight.cpp -- Primal Hunt's d3d.ren, TU25
//  (0x10024540..0x10024ebb), /O2.
//
//  Dynamic lights and line systems: the OT_LIGHT and OT_LINESYSTEM
//  ProcessObject handlers (0x1004dae8, 0x1004db38), the walk that hangs a
//  light on every world-model polygon it reaches, and the line-system
//  draw.
// *********************************************************************** //

#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/struct_bank.h"


struct LTObject;
struct SceneContext;
typedef void (*DrawObjectFn)(SceneContext* pContext, LTObject* pObject);

extern void		AddDebugMessage(int level, const char* pMsg, ...);  // 0x10013b13

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

	void		Draw(SceneContext* pContext, DrawObjectFn fn);  // 0x100397e0

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
extern SceneContext		g_SceneContext;                 // 0x1005b790
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
//  0x10024990 -- light everything the visible lights reach.
// ---------------------------------------------------------------------- //
extern void d3d_ProcessLightSet(SceneContext* pContext, LTObject* pLight);  // 0x10024800

void d3d_ProcessLights()
{
	if (g_CV_DynamicLight)
		d3d_GetVisibleSet()->m_Lights.Draw(&g_SceneContext, d3d_ProcessLightSet);
}


// ---------------------------------------------------------------------- //
//  0x100249c0 -- the OT_LINESYSTEM ProcessObject handler.
// ---------------------------------------------------------------------- //
void d3d_ProcessLineSystem(LTObject *pObject)
{
	d3d_GetVisibleSet()->m_LineSystems.Add(pObject);
}


static void d3d_QueueLineSystem(SceneContext* pContext, LTObject* pObject);

// ---------------------------------------------------------------------- //
//  0x10024a10
// ---------------------------------------------------------------------- //
void d3d_DrawLineSystems()
{
	if (g_CV_DrawLineSystems)
	{
		BaseObjectSet* pSet = &d3d_GetVisibleSet()->m_LineSystems;
		if (pSet->m_nObjects)
			pSet->Draw(&g_SceneContext, d3d_QueueLineSystem);
	}
}


extern void d3d_DrawLineSystem(SceneContext* pContext, LTObject* pObject);  // 0x10024a60

// ---------------------------------------------------------------------- //
//  0x10024a40
// ---------------------------------------------------------------------- //
static void d3d_QueueLineSystem(SceneContext* pContext, LTObject* pObject)
{
	g_pObjectDrawList->Add(pObject, d3d_DrawLineSystem);
}
